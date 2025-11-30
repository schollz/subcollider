/**
 * @file supersaw_example.cpp
 * @brief Interactive SuperSaw example with mouse control, stereo filter, and reverb.
 *
 * This example demonstrates the SuperSaw composite UGen with 7 detuned
 * saw oscillators, vibrato, stereo spreading, dual Moog ladder filters (L/R), and
 * FVerb stereo reverb (10% wet).
 * Mouse X controls cutoff frequency, Mouse Y controls drive.
 *
 * Signal chain:
 *   SuperSaw -> Stereo Moog Filters (L/R) -> FVerb (10% wet) -> Output
 *
 * The UGens are initialized at the JACK sample rate. FVerb processes
 * the filtered audio in blocks for high-quality algorithmic reverberation.
 *
 * Compile with: -ljack -lX11
 *
 * Usage:
 *   ./example_supersaw
 *
 * Move mouse to control filter (X = cutoff, Y = drive)
 * Press Ctrl+C to quit
 */

#include <subcollider.h>
#include <subcollider/ugens/FVerb.h>
#include <jack/jack.h>
#include <X11/Xlib.h>
#include <array>
#include <iomanip>
#include <iostream>
#include <atomic>
#include <csignal>
#include <cmath>
#include <time.h>

using namespace subcollider;
using namespace subcollider::ugens;

// Oversampling factor (4x for improved quality)
static constexpr size_t OVERSAMPLE_FACTOR = 1;

// Global state for JACK callback
static constexpr size_t NUM_VOICES = 3;
static constexpr size_t MAX_BLOCK_SIZE = 8192;
static std::array<SuperSaw, NUM_VOICES> g_supersaws;
static Lag g_cutoffLag;
static Lag g_driveLag;
static StereoDownsampler g_downsampler;
static RKSimulationMoogLadder g_filterL;
static RKSimulationMoogLadder g_filterR;
static FVerb g_reverb;

// DC blocking filter state (simple one-pole highpass)
static Sample g_dcBlockL_x1 = 0.0f;
static Sample g_dcBlockL_y1 = 0.0f;
static Sample g_dcBlockR_x1 = 0.0f;
static Sample g_dcBlockR_y1 = 0.0f;
static Sample g_reverbLeftBuffer[MAX_BLOCK_SIZE];
static Sample g_reverbRightBuffer[MAX_BLOCK_SIZE];
static jack_client_t* g_client = nullptr;
static float g_outputRate = 48000.0f;
static constexpr float REVERB_WET = 0.1f;  // 10% wet
static std::atomic<float> g_cpuUsage{0.0f};
static float g_cpuRing[100] = {0.0f};
static size_t g_cpuRingSize = 0;
static size_t g_cpuRingIndex = 0;
static double g_cpuRingSum = 0.0;
static jack_port_t* g_outputPortL = nullptr;
static jack_port_t* g_outputPortR = nullptr;
static std::atomic<bool> g_running{true};
static std::atomic<float> g_cutoff{5000.0f};
static std::atomic<float> g_drive{0.0f};  // normalized 0..1
static constexpr float RESONANCE = 0.01f;
static constexpr std::array<float, NUM_VOICES> VOICE_FREQUENCIES = {55.0f, 329.63f, 523.25f};  // C (requested 55 Hz), E >200 Hz, G >400 Hz

// Mouse control parameters
static constexpr float MIN_CUTOFF = 100.0f;
static constexpr float MAX_CUTOFF = 12000.0f;
static constexpr float MIN_DRIVE_GAIN = 0.01f;   // actual drive multiplier
static constexpr float MAX_DRIVE_GAIN = 100.0f;  // actual drive multiplier

static float driveFromNormalized(float normalized) {
    float norm = std::max(0.0f, std::min(1.0f, normalized));
    return MIN_DRIVE_GAIN * std::pow(MAX_DRIVE_GAIN / MIN_DRIVE_GAIN, norm);
}

static uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ull +
           static_cast<uint64_t>(ts.tv_nsec);
}

/**
 * @brief JACK process callback - ISR-safe audio processing.
 *
 * This function is called by JACK for each audio block.
 * No heap allocation, no blocking, no virtual calls.
 *
 * Audio is generated at 2x the output sample rate, then downsampled
 * with anti-aliasing for improved quality.
 */
int jackProcessCallback(jack_nframes_t nframes, void*) {
    uint64_t start = get_time_ns();

    // Get output buffers from JACK (stereo)
    jack_default_audio_sample_t* outL =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortL, nframes));
    jack_default_audio_sample_t* outR =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortR, nframes));

    // Generate and filter audio with oversampling
    for (jack_nframes_t i = 0; i < nframes; ++i) {
        // Smooth the cutoff and drive values using Lag (at oversampled rate)
        // Read parameters at output rate to reduce atomic operations
        Sample targetCutoff = g_cutoff.load(std::memory_order_relaxed);
        Sample targetDrive = g_drive.load(std::memory_order_relaxed);

        // Generate OVERSAMPLE_FACTOR samples at the higher rate
        for (size_t j = 0; j < OVERSAMPLE_FACTOR; ++j) {
            Sample smoothCutoff = g_cutoffLag.tick(targetCutoff);
            Sample smoothDrive = g_driveLag.tick(targetDrive);
            Sample driveGain = driveFromNormalized(smoothDrive);
            g_filterL.setCutoff(smoothCutoff);
            g_filterL.setResonance(RESONANCE);
            g_filterL.setDrive(driveGain);
            g_filterR.setCutoff(smoothCutoff);
            g_filterR.setResonance(RESONANCE);
            g_filterR.setDrive(driveGain);

            Stereo mix{0.0f, 0.0f};
            for (auto& voice : g_supersaws) {
                Stereo voiceSample = voice.tick();
                mix.left += voiceSample.left;
                mix.right += voiceSample.right;
            }
            Sample invVoices = 1.0f / static_cast<Sample>(NUM_VOICES);
            mix.left *= invVoices;
            mix.right *= invVoices;  // prevent clipping

            Sample filteredL = g_filterL.tick(mix.left);
            Sample filteredR = g_filterR.tick(mix.right);

            // Apply DC blocking filter (removes DC offset and high-frequency artifacts)
            // y[n] = x[n] - x[n-1] + R * y[n-1], where R ≈ 0.995
            constexpr Sample dcBlockCoeff = 0.995f;
            Sample dcBlockedL = filteredL - g_dcBlockL_x1 + dcBlockCoeff * g_dcBlockL_y1;
            Sample dcBlockedR = filteredR - g_dcBlockR_x1 + dcBlockCoeff * g_dcBlockR_y1;
            g_dcBlockL_x1 = filteredL;
            g_dcBlockL_y1 = dcBlockedL;
            g_dcBlockR_x1 = filteredR;
            g_dcBlockR_y1 = dcBlockedR;

            // Write to downsampler
            g_downsampler.write(Stereo(dcBlockedL, dcBlockedR));
        }

        // Read one downsampled output sample (dry signal)
        Stereo output = g_downsampler.read();

        // Store dry signal in reverb buffers
        g_reverbLeftBuffer[i] = output.left;
        g_reverbRightBuffer[i] = output.right;
    }

    // Store dry in temp buffers before reverb processing
    Sample dryLeftBuffer[MAX_BLOCK_SIZE];
    Sample dryRightBuffer[MAX_BLOCK_SIZE];

    for (jack_nframes_t i = 0; i < nframes; ++i) {
        dryLeftBuffer[i] = g_reverbLeftBuffer[i];
        dryRightBuffer[i] = g_reverbRightBuffer[i];
    }

    // Process entire block through reverb (overwrites reverb buffers with wet signal)
    g_reverb.process(g_reverbLeftBuffer, g_reverbRightBuffer, nframes);

    // Mix wet (reverb) and dry signals and write to output
    for (jack_nframes_t i = 0; i < nframes; ++i) {
        outL[i] = dryLeftBuffer[i] * (1.0f - REVERB_WET) + g_reverbLeftBuffer[i] * REVERB_WET;
        outR[i] = dryRightBuffer[i] * (1.0f - REVERB_WET) + g_reverbRightBuffer[i] * REVERB_WET;
    }

    uint64_t end = get_time_ns();
    double block_time_ns =
        (static_cast<double>(nframes) / static_cast<double>(g_outputRate)) *
        1e9;
    if (block_time_ns > 0.0) {
        double cpu = (static_cast<double>(end - start) / block_time_ns) * 100.0;
        float cpuSample = static_cast<float>(cpu);
        if (g_cpuRingSize < 100) {
            g_cpuRing[g_cpuRingIndex] = cpuSample;
            g_cpuRingSum += cpuSample;
            ++g_cpuRingSize;
        } else {
            g_cpuRingSum -= g_cpuRing[g_cpuRingIndex];
            g_cpuRing[g_cpuRingIndex] = cpuSample;
            g_cpuRingSum += cpuSample;
        }
        g_cpuRingIndex = (g_cpuRingIndex + 1) % 100;
        float avgCpu = static_cast<float>(g_cpuRingSum / static_cast<double>(g_cpuRingSize));
        g_cpuUsage.store(avgCpu, std::memory_order_relaxed);
    }

    return 0;
}

/**
 * @brief JACK sample rate callback.
 *
 * Reinitializes all UGens at the oversampled rate when JACK sample rate changes.
 */
int jackSampleRateCallback(jack_nframes_t nframes, void*) {
    // Calculate oversampled rate
    float outputRate = static_cast<float>(nframes);
    float internalRate = outputRate * OVERSAMPLE_FACTOR;
    g_outputRate = outputRate;

    std::cout << "JACK sample rate: " << nframes << " Hz" << std::endl;
    std::cout << "Internal (oversampled) rate: " << static_cast<int>(internalRate) << " Hz" << std::endl;

    // Initialize SuperSaws at the oversampled rate
    for (size_t i = 0; i < NUM_VOICES; ++i) {
        g_supersaws[i].init(internalRate);
        g_supersaws[i].setFrequency(VOICE_FREQUENCIES[i]);
        g_supersaws[i].setDetune(0.2f);
        g_supersaws[i].setVibratoRate(6.0f);
        g_supersaws[i].setVibratoDepth(0.3f);
        g_supersaws[i].setSpread(0.6f);
        g_supersaws[i].setAttack(0.01f);
        g_supersaws[i].setDecay(0.1f);
        g_supersaws[i].setSustain(0.7f);
        g_supersaws[i].setRelease(0.3f);
        g_supersaws[i].gate(1.0f);
    }
    g_filterL.init(internalRate);
    g_filterL.setCutoff(5000.0f);
    g_filterL.setResonance(RESONANCE);
    g_filterL.setDrive(driveFromNormalized(g_drive.load(std::memory_order_relaxed)));
    g_filterR.init(internalRate);
    g_filterR.setCutoff(5000.0f);
    g_filterR.setResonance(RESONANCE);
    g_filterR.setDrive(driveFromNormalized(g_drive.load(std::memory_order_relaxed)));

    // Reset DC blocker state
    g_dcBlockL_x1 = 0.0f;
    g_dcBlockL_y1 = 0.0f;
    g_dcBlockR_x1 = 0.0f;
    g_dcBlockR_y1 = 0.0f;

    // Initialize Lag filters at the oversampled rate (they run in the inner loop)
    g_cutoffLag.init(internalRate, 0.2f);
    g_driveLag.init(internalRate, 0.2f);

    // Initialize downsampler
    g_downsampler.init(outputRate, OVERSAMPLE_FACTOR);

    // Initialize reverb at output rate
    g_reverb.init(outputRate);
    g_reverb.setPredelay(150.0f);
    g_reverb.setDecay(82.0f);
    g_reverb.setTailDensity(80.0f);
    g_reverb.setInputDiffusion1(70.0f);
    g_reverb.setInputDiffusion2(75.0f);
    g_reverb.setDamping(5500.0f);

    return 0;
}

/**
 * @brief JACK shutdown callback.
 */
void jackShutdownCallback(void*) {
    std::cout << "JACK server shutdown" << std::endl;
    g_running.store(false);
}

/**
 * @brief Signal handler for graceful shutdown.
 */
void signalHandler(int) {
    g_running.store(false);
}

int main() {
    std::cout << "SubCollider SuperSaw Example" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << std::endl;

    // Set up signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Initialize X11 for mouse tracking
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X11 display" << std::endl;
        return 1;
    }

    Window root = DefaultRootWindow(display);
    int screenWidth = DisplayWidth(display, DefaultScreen(display));
    int screenHeight = DisplayHeight(display, DefaultScreen(display));

    std::cout << "Screen size: " << screenWidth << "x" << screenHeight << std::endl;
    std::cout << std::endl;

    // Open JACK client
    jack_status_t status;
    g_client = jack_client_open("subcollider_supersaw", JackNoStartServer, &status);

    if (g_client == nullptr) {
        std::cerr << "Failed to open JACK client. Is JACK server running?" << std::endl;
        if (status & JackServerFailed) {
            std::cerr << "Unable to connect to JACK server" << std::endl;
        }
        XCloseDisplay(display);
        return 1;
    }

    std::cout << "Connected to JACK server" << std::endl;

    // Get sample rate and calculate oversampled rate
    jack_nframes_t sampleRate = jack_get_sample_rate(g_client);
    float outputRate = static_cast<float>(sampleRate);
    float internalRate = outputRate * OVERSAMPLE_FACTOR;
    g_outputRate = outputRate;

    std::cout << "JACK sample rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "Internal (oversampled) rate: " << static_cast<int>(internalRate) << " Hz" << std::endl;
    std::cout << "Oversampling factor: " << OVERSAMPLE_FACTOR << "x" << std::endl;

    // Initialize SuperSaws at the oversampled rate (C major chord)
    for (size_t i = 0; i < NUM_VOICES; ++i) {
        g_supersaws[i].init(internalRate, static_cast<unsigned int>(42 + i));  // different seeds
        g_supersaws[i].setFrequency(VOICE_FREQUENCIES[i]);
        g_supersaws[i].setDetune(0.2f);           // 0.2 semitones detune
        g_supersaws[i].setVibratoRate(6.0f);      // 6 Hz vibrato
        g_supersaws[i].setVibratoDepth(0.3f);     // 0.3 semitones vibrato depth
        g_supersaws[i].setSpread(0.6f);           // 60% stereo spread
        g_supersaws[i].setAttack(0.01f);          // 10ms attack
        g_supersaws[i].setDecay(0.1f);            // 100ms decay
        g_supersaws[i].setSustain(0.7f);          // 70% sustain
        g_supersaws[i].setRelease(0.3f);          // 300ms release
        g_supersaws[i].gate(1.0f);
    }
    g_filterL.init(internalRate);
    g_filterL.setCutoff(5000.0f);
    g_filterL.setResonance(RESONANCE);
    g_filterL.setDrive(driveFromNormalized(g_drive.load(std::memory_order_relaxed)));
    g_filterR.init(internalRate);
    g_filterR.setCutoff(5000.0f);
    g_filterR.setResonance(RESONANCE);
    g_filterR.setDrive(driveFromNormalized(g_drive.load(std::memory_order_relaxed)));

    // Reset DC blocker state
    g_dcBlockL_x1 = 0.0f;
    g_dcBlockL_y1 = 0.0f;
    g_dcBlockR_x1 = 0.0f;
    g_dcBlockR_y1 = 0.0f;

    // Initialize Lag filters at the oversampled rate for smooth parameter changes
    g_cutoffLag.init(internalRate, 0.2f);
    g_driveLag.init(internalRate, 0.2f);
    // Set initial values to prevent initial transient
    g_cutoffLag.setValue(5000.0f);
    g_driveLag.setValue(g_drive.load(std::memory_order_relaxed));

    // Initialize the stereo downsampler
    g_downsampler.init(outputRate, OVERSAMPLE_FACTOR);

    // Initialize reverb at output rate
    g_reverb.init(outputRate);
    g_reverb.setPredelay(150.0f);
    g_reverb.setDecay(82.0f);
    g_reverb.setTailDensity(80.0f);
    g_reverb.setInputDiffusion1(70.0f);
    g_reverb.setInputDiffusion2(75.0f);
    g_reverb.setDamping(5500.0f);

    // Set callbacks
    jack_set_process_callback(g_client, jackProcessCallback, nullptr);
    jack_set_sample_rate_callback(g_client, jackSampleRateCallback, nullptr);
    jack_on_shutdown(g_client, jackShutdownCallback, nullptr);

    // Create stereo output ports
    g_outputPortL = jack_port_register(g_client, "output_L",
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsOutput, 0);
    g_outputPortR = jack_port_register(g_client, "output_R",
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsOutput, 0);

    if (g_outputPortL == nullptr || g_outputPortR == nullptr) {
        std::cerr << "Failed to create JACK output ports" << std::endl;
        jack_client_close(g_client);
        XCloseDisplay(display);
        return 1;
    }

    // Activate client
    if (jack_activate(g_client) != 0) {
        std::cerr << "Failed to activate JACK client" << std::endl;
        jack_client_close(g_client);
        XCloseDisplay(display);
        return 1;
    }

    std::cout << "JACK client activated" << std::endl;
    std::cout << std::endl;
    std::cout << "Playing SuperSaw C chord (C=55 Hz, E>200 Hz, G>400 Hz) with 7 detuned voices per note" << std::endl;
    std::cout << "Signal chain: SuperSaw -> Stereo Moog Filters (L/R) -> FVerb (10% wet)" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Move mouse horizontally (X) to control cutoff frequency" << std::endl;
    std::cout << "  Move mouse vertically (Y) to control drive" << std::endl;
    std::cout << "  Press Ctrl+C to quit" << std::endl;
    std::cout << std::endl;

    // Main loop - track mouse position
    Window returnedRoot, returnedChild;
    int rootX, rootY, winX, winY;
    unsigned int mask;

    while (g_running.load()) {
        // Query mouse position
        if (XQueryPointer(display, root, &returnedRoot, &returnedChild,
                         &rootX, &rootY, &winX, &winY, &mask)) {
            // Map mouse X to cutoff frequency (logarithmic scale)
            float normalizedX = static_cast<float>(rootX) / static_cast<float>(screenWidth);
            normalizedX = std::max(0.0f, std::min(1.0f, normalizedX));

            // Logarithmic mapping for cutoff (sounds more musical)
            float logMin = std::log(MIN_CUTOFF);
            float logMax = std::log(MAX_CUTOFF);
            float cutoff = std::exp(logMin + normalizedX * (logMax - logMin));
            g_cutoff.store(cutoff, std::memory_order_relaxed);

            // Map mouse Y to drive (normalized 0..1, inverted so top = more drive)
            float normalizedY = 1.0f - (static_cast<float>(rootY) / static_cast<float>(screenHeight));
            normalizedY = std::max(0.0f, std::min(1.0f, normalizedY));
            float drive = normalizedY;
            g_drive.store(drive, std::memory_order_relaxed);
            float driveGain = driveFromNormalized(drive);

            // Print current values
            float cpu = g_cpuUsage.load(std::memory_order_relaxed);
            std::cout << "\rCutoff: " << static_cast<int>(cutoff) << " Hz   "
                     << "Drive: " << driveGain << "   "
                     << "Utilization: " << std::fixed << std::setprecision(1)
                     << cpu << "%   " << std::flush;
        }

        // Sleep to avoid polling too fast
        usleep(50000);  // 50ms = 20 updates per second
    }

    // Cleanup
    std::cout << std::endl << std::endl << "Shutting down..." << std::endl;
    jack_deactivate(g_client);
    jack_client_close(g_client);
    XCloseDisplay(display);

    std::cout << "Done." << std::endl;
    return 0;
}
