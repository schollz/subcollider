/**
 * @file moog_example.cpp
 * @brief Interactive Moog filter example with mouse control.
 *
 * This example demonstrates the ImprovedMoogLadder filter with a SawDPW oscillator.
 * Mouse X controls cutoff frequency, Mouse Y controls resonance.
 *
 * Compile with: -ljack -lX11
 *
 * Usage:
 *   ./example_moog
 *
 * Move mouse to control filter (X = cutoff, Y = resonance)
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

// Global state for JACK callback
static SawDPW g_saw;
static ImprovedMoogLadder g_filter;
static Lag g_cutoffLag;
static Lag g_resonanceLag;
static jack_port_t* g_outputPortL = nullptr;
static jack_port_t* g_outputPortR = nullptr;
static std::atomic<bool> g_running{true};
static std::atomic<float> g_cutoff{1000.0f};
static std::atomic<float> g_resonance{0.3f};

// Mouse control parameters
static constexpr float MIN_CUTOFF = 100.0f;
static constexpr float MAX_CUTOFF = 8000.0f;
static constexpr float MIN_RESONANCE = 0.0f;
static constexpr float MAX_RESONANCE = 0.99f;

/**
 * @brief JACK process callback - ISR-safe audio processing.
 *
 * This function is called by JACK for each audio block.
 * No heap allocation, no blocking, no virtual calls.
 */
int jackProcessCallback(jack_nframes_t nframes, void*) {
    // Get output buffers from JACK (stereo)
    jack_default_audio_sample_t* outL =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortL, nframes));
    jack_default_audio_sample_t* outR =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortR, nframes));

    // Generate and filter audio
    for (jack_nframes_t i = 0; i < nframes; ++i) {
        // Smooth the cutoff and resonance values using Lag
        Sample smoothCutoff = g_cutoffLag.tick(g_cutoff.load(std::memory_order_relaxed));
        Sample smoothResonance = g_resonanceLag.tick(g_resonance.load(std::memory_order_relaxed));

        // Update filter parameters with smoothed values
        g_filter.setCutoff(smoothCutoff);
        g_filter.setResonance(smoothResonance);

        // Generate saw wave
        Sample sawSample = g_saw.tick();

        // Filter it
        Sample filtered = g_filter.tick(sawSample);

        // Output to both channels (mono)
        outL[i] = filtered * 0.3f;  // Reduce volume to prevent clipping
        outR[i] = filtered * 0.3f;
    }

    return 0;
}

/**
 * @brief JACK sample rate callback.
 */
int jackSampleRateCallback(jack_nframes_t nframes, void*) {
    std::cout << "Sample rate changed to: " << nframes << " Hz" << std::endl;
    g_saw.init(static_cast<float>(nframes));
    g_filter.init(static_cast<float>(nframes));
    g_cutoffLag.init(static_cast<float>(nframes), 0.2f);
    g_resonanceLag.init(static_cast<float>(nframes), 0.2f);
    g_saw.setFrequency(440.0f);
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
    std::cout << "SubCollider Moog Filter Example" << std::endl;
    std::cout << "================================" << std::endl;
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
    jack_client_t* client = jack_client_open("subcollider_moog", JackNoStartServer, &status);

    if (client == nullptr) {
        std::cerr << "Failed to open JACK client. Is JACK server running?" << std::endl;
        if (status & JackServerFailed) {
            std::cerr << "Unable to connect to JACK server" << std::endl;
        }
        XCloseDisplay(display);
        return 1;
    }

    std::cout << "Connected to JACK server" << std::endl;

    // Get sample rate and initialize UGens
    jack_nframes_t sampleRate = jack_get_sample_rate(client);
    std::cout << "Sample rate: " << sampleRate << " Hz" << std::endl;

    g_saw.init(static_cast<float>(sampleRate));
    g_saw.setFrequency(440.0f);

    g_filter.init(static_cast<float>(sampleRate));
    g_filter.setCutoff(1000.0f);
    g_filter.setResonance(0.3f);

    // Initialize Lag filters for smooth parameter changes (0.2 second lag time)
    g_cutoffLag.init(static_cast<float>(sampleRate), 0.2f);
    g_resonanceLag.init(static_cast<float>(sampleRate), 0.2f);
    // Set initial values to prevent initial transient
    g_cutoffLag.setValue(1000.0f);
    g_resonanceLag.setValue(0.3f);

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
    std::cout << "Controls:" << std::endl;
    std::cout << "  Move mouse horizontally (X) to control cutoff frequency" << std::endl;
    std::cout << "  Move mouse vertically (Y) to control resonance" << std::endl;
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

            // Map mouse Y to resonance (linear, inverted so top = high resonance)
            float normalizedY = 1.0f - (static_cast<float>(rootY) / static_cast<float>(screenHeight));
            normalizedY = std::max(0.0f, std::min(1.0f, normalizedY));
            float resonance = MIN_RESONANCE + normalizedY * (MAX_RESONANCE - MIN_RESONANCE);
            g_resonance.store(resonance, std::memory_order_relaxed);

            // Print current values
            std::cout << "\rCutoff: " << static_cast<int>(cutoff) << " Hz   "
                     << "Resonance: " << resonance << "   " << std::flush;
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
