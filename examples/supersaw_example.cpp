/**
 * @file supersaw_example.cpp
 * @brief Interactive SuperSaw example with mouse control and 2x oversampling.
 *
 * This example demonstrates the SuperSaw composite UGen with 7 detuned
 * saw oscillators, vibrato, stereo spreading, and filtering.
 * Mouse X controls cutoff frequency, Mouse Y controls drive.
 *
 * The UGens are initialized at 2x the JACK sample rate (e.g., 96kHz when
 * JACK runs at 48kHz) for improved audio quality, particularly reducing
 * aliasing in the saw wave oscillators. A StereoDownsampler with anti-aliasing
 * filter is used to convert the oversampled audio to the JACK output rate.
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
#include <jack/jack.h>
#include <X11/Xlib.h>
#include <iostream>
#include <atomic>
#include <csignal>
#include <cmath>

using namespace subcollider;
using namespace subcollider::ugens;

// Oversampling factor (2x for improved quality)
static constexpr size_t OVERSAMPLE_FACTOR = 2;

// Global state for JACK callback
static SuperSaw g_supersaw;
static Lag g_cutoffLag;
static Lag g_driveLag;
static StereoDownsampler g_downsampler;
static jack_port_t* g_outputPortL = nullptr;
static jack_port_t* g_outputPortR = nullptr;
static std::atomic<bool> g_running{true};
static std::atomic<float> g_cutoff{5000.0f};
static std::atomic<float> g_drive{0.0f};  // normalized 0..1
static constexpr float RESONANCE = 0.5f;

// Mouse control parameters
static constexpr float MIN_CUTOFF = 100.0f;
static constexpr float MAX_CUTOFF = 12000.0f;
static constexpr float MIN_DRIVE_GAIN = 0.01f;   // actual drive multiplier
static constexpr float MAX_DRIVE_GAIN = 100.0f;  // actual drive multiplier

static float driveFromNormalized(float normalized) {
    float norm = std::max(0.0f, std::min(1.0f, normalized));
    return MIN_DRIVE_GAIN * std::pow(MAX_DRIVE_GAIN / MIN_DRIVE_GAIN, norm);
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
    // Get output buffers from JACK (stereo)
    jack_default_audio_sample_t* outL =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortL, nframes));
    jack_default_audio_sample_t* outR =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortR, nframes));

    // Generate and filter audio with 2x oversampling
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

            // Update SuperSaw parameters with smoothed values
            g_supersaw.setCutoff(smoothCutoff);
            g_supersaw.filter.setResonance(RESONANCE);
            g_supersaw.setDrive(driveGain);

            // Generate SuperSaw output at oversampled rate
            Stereo sample = g_supersaw.tick();

            // Write to downsampler
            g_downsampler.write(sample);
        }

        // Read one downsampled output sample
        Stereo output = g_downsampler.read();

        // Output to both channels
        outL[i] = output.left;
        outR[i] = output.right;
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

    std::cout << "JACK sample rate: " << nframes << " Hz" << std::endl;
    std::cout << "Internal (oversampled) rate: " << static_cast<int>(internalRate) << " Hz" << std::endl;

    // Initialize SuperSaw at the oversampled rate
    g_supersaw.init(internalRate);

    // Initialize Lag filters at the oversampled rate (they run in the inner loop)
    g_cutoffLag.init(internalRate, 0.2f);
    g_driveLag.init(internalRate, 0.2f);

    // Initialize downsampler
    g_downsampler.init(outputRate, OVERSAMPLE_FACTOR);

    // Re-apply settings after init
    g_supersaw.setFrequency(55.0f);
    g_supersaw.setDetune(0.2f);
    g_supersaw.setVibratoRate(6.0f);
    g_supersaw.setVibratoDepth(0.3f);
    g_supersaw.setDrive(driveFromNormalized(g_drive.load(std::memory_order_relaxed)));
    g_supersaw.setSpread(0.6f);
    g_supersaw.setCutoff(5000.0f);
    g_supersaw.filter.setResonance(RESONANCE);
    g_supersaw.setLpEnv(0.0f);
    g_supersaw.setLpAttack(0.0f);
    g_supersaw.setAttack(0.01f);
    g_supersaw.setDecay(0.1f);
    g_supersaw.setSustain(0.7f);
    g_supersaw.setRelease(0.3f);

    // Re-trigger the gate after init (which resets the envelope)
    g_supersaw.gate(1.0f);

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
    jack_client_t* client = jack_client_open("subcollider_supersaw", JackNoStartServer, &status);

    if (client == nullptr) {
        std::cerr << "Failed to open JACK client. Is JACK server running?" << std::endl;
        if (status & JackServerFailed) {
            std::cerr << "Unable to connect to JACK server" << std::endl;
        }
        XCloseDisplay(display);
        return 1;
    }

    std::cout << "Connected to JACK server" << std::endl;

    // Get sample rate and calculate oversampled rate
    jack_nframes_t sampleRate = jack_get_sample_rate(client);
    float outputRate = static_cast<float>(sampleRate);
    float internalRate = outputRate * OVERSAMPLE_FACTOR;

    std::cout << "JACK sample rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "Internal (oversampled) rate: " << static_cast<int>(internalRate) << " Hz" << std::endl;
    std::cout << "Oversampling factor: " << OVERSAMPLE_FACTOR << "x" << std::endl;

    // Initialize SuperSaw at the oversampled rate
    g_supersaw.init(internalRate, 42);  // seed=42
    g_supersaw.setFrequency(55.0f);
    g_supersaw.setDetune(0.2f);           // 0.2 semitones detune
    g_supersaw.setVibratoRate(6.0f);      // 6 Hz vibrato
    g_supersaw.setVibratoDepth(0.3f);     // 0.3 semitones vibrato depth
    g_supersaw.setDrive(driveFromNormalized(g_drive.load(std::memory_order_relaxed))); // Drive from normalized control
    g_supersaw.setSpread(0.6f);           // 60% stereo spread
    g_supersaw.setCutoff(5000.0f);        // 5kHz initial cutoff
    g_supersaw.filter.setResonance(RESONANCE); // Fixed resonance
    g_supersaw.setLpEnv(0.0f);            // No envelope modulation
    g_supersaw.setLpAttack(0.0f);         // No attack

    // Set ADSR envelope for gate on/off
    g_supersaw.setAttack(0.01f);          // 10ms attack
    g_supersaw.setDecay(0.1f);            // 100ms decay
    g_supersaw.setSustain(0.7f);          // 70% sustain
    g_supersaw.setRelease(0.3f);          // 300ms release

    // Trigger the note (gate on)
    g_supersaw.gate(1.0f);

    // Initialize Lag filters at the oversampled rate for smooth parameter changes
    g_cutoffLag.init(internalRate, 0.2f);
    g_driveLag.init(internalRate, 0.2f);
    // Set initial values to prevent initial transient
    g_cutoffLag.setValue(5000.0f);
    g_driveLag.setValue(g_drive.load(std::memory_order_relaxed));

    // Initialize the stereo downsampler
    g_downsampler.init(outputRate, OVERSAMPLE_FACTOR);

    // Set callbacks
    jack_set_process_callback(client, jackProcessCallback, nullptr);
    jack_set_sample_rate_callback(client, jackSampleRateCallback, nullptr);
    jack_on_shutdown(client, jackShutdownCallback, nullptr);

    // Create stereo output ports
    g_outputPortL = jack_port_register(client, "output_L",
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsOutput, 0);
    g_outputPortR = jack_port_register(client, "output_R",
                                        JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsOutput, 0);

    if (g_outputPortL == nullptr || g_outputPortR == nullptr) {
        std::cerr << "Failed to create JACK output ports" << std::endl;
        jack_client_close(client);
        XCloseDisplay(display);
        return 1;
    }

    // Activate client
    if (jack_activate(client) != 0) {
        std::cerr << "Failed to activate JACK client" << std::endl;
        jack_client_close(client);
        XCloseDisplay(display);
        return 1;
    }

    std::cout << "JACK client activated" << std::endl;
    std::cout << std::endl;
    std::cout << "Playing SuperSaw at 55 Hz (7 detuned voices with vibrato)" << std::endl;
    std::cout << "Using " << OVERSAMPLE_FACTOR << "x oversampling for improved audio quality" << std::endl;
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
            std::cout << "\rCutoff: " << static_cast<int>(cutoff) << " Hz   "
                     << "Drive: " << driveGain << "   " << std::flush;
        }

        // Sleep to avoid polling too fast
        usleep(50000);  // 50ms = 20 updates per second
    }

    // Cleanup
    std::cout << std::endl << std::endl << "Shutting down..." << std::endl;
    jack_deactivate(client);
    jack_client_close(client);
    XCloseDisplay(display);

    std::cout << "Done." << std::endl;
    return 0;
}
