/**
 * @file jack_example.cpp
 * @brief JACK audio example for SubCollider DSP engine.
 *
 * This example demonstrates real-time audio output using JACK audio server.
 * Compile with: -ljack
 *
 * Usage:
 *   ./example_jack
 *
 * Press Enter to trigger/release notes, Ctrl+C to quit.
 */

#include <subcollider.h>
#include <jack/jack.h>
#include <iostream>
#include <atomic>
#include <csignal>
#include <cstring>

using namespace subcollider;

// Global state for JACK callback
static ExampleVoice g_voice;
static jack_port_t* g_outputPort = nullptr;
static std::atomic<bool> g_running{true};
static std::atomic<bool> g_noteOn{false};
static std::atomic<float> g_frequency{440.0f};

/**
 * @brief JACK process callback - ISR-safe audio processing.
 *
 * This function is called by JACK for each audio block.
 * No heap allocation, no blocking, no virtual calls.
 */
int jackProcessCallback(jack_nframes_t nframes, void*) {
    // Get output buffer from JACK
    jack_default_audio_sample_t* out =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPort, nframes));

    // Update frequency at control rate (once per block)
    g_voice.setFrequency(g_frequency.load(std::memory_order_relaxed));

    // Handle gate changes at control rate
    static bool lastNoteOn = false;
    bool currentNoteOn = g_noteOn.load(std::memory_order_relaxed);
    if (currentNoteOn != lastNoteOn) {
        if (currentNoteOn) {
            g_voice.trigger();
        } else {
            g_voice.release();
        }
        lastNoteOn = currentNoteOn;
    }

    // Process audio block
    g_voice.process(out, nframes);

    return 0;
}

/**
 * @brief JACK sample rate callback.
 */
int jackSampleRateCallback(jack_nframes_t nframes, void*) {
    std::cout << "Sample rate changed to: " << nframes << " Hz" << std::endl;
    g_voice.init(static_cast<float>(nframes));
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
    std::cout << "SubCollider JACK Example" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << std::endl;

    // Set up signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Open JACK client
    jack_status_t status;
    jack_client_t* client = jack_client_open("subcollider", JackNoStartServer, &status);

    if (client == nullptr) {
        std::cerr << "Failed to open JACK client. Is JACK server running?" << std::endl;
        if (status & JackServerFailed) {
            std::cerr << "Unable to connect to JACK server" << std::endl;
        }
        return 1;
    }

    std::cout << "Connected to JACK server" << std::endl;

    // Get sample rate and initialize voice
    jack_nframes_t sampleRate = jack_get_sample_rate(client);
    std::cout << "Sample rate: " << sampleRate << " Hz" << std::endl;

    g_voice.init(static_cast<float>(sampleRate));
    g_voice.setFrequency(440.0f);
    g_voice.setAttack(0.01f);
    g_voice.setRelease(0.3f);
    g_voice.setVibratoDepth(0.1f);
    g_voice.setVibratoRate(5.0f);
    g_voice.setAmplitude(0.5f);

    // Set callbacks
    jack_set_process_callback(client, jackProcessCallback, nullptr);
    jack_set_sample_rate_callback(client, jackSampleRateCallback, nullptr);
    jack_on_shutdown(client, jackShutdownCallback, nullptr);

    // Create output port
    g_outputPort = jack_port_register(client, "output",
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsOutput, 0);

    if (g_outputPort == nullptr) {
        std::cerr << "Failed to create JACK output port" << std::endl;
        jack_client_close(client);
        return 1;
    }

    // Activate client
    if (jack_activate(client) != 0) {
        std::cerr << "Failed to activate JACK client" << std::endl;
        jack_client_close(client);
        return 1;
    }

    std::cout << "JACK client activated" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Press Enter to toggle note on/off" << std::endl;
    std::cout << "  Type a number + Enter to change frequency (Hz)" << std::endl;
    std::cout << "  Press Ctrl+C to quit" << std::endl;
    std::cout << std::endl;

    // Main loop - handle user input
    std::string input;
    while (g_running.load()) {
        std::cout << (g_noteOn.load() ? "[ON] " : "[OFF]") << " Freq: " << g_frequency.load() << " Hz > ";
        std::cout.flush();

        if (!std::getline(std::cin, input)) {
            break;
        }

        if (input.empty()) {
            // Toggle note on/off
            bool current = g_noteOn.load();
            g_noteOn.store(!current);
        } else {
            // Try to parse as frequency
            try {
                float freq = std::stof(input);
                if (freq > 20.0f && freq < 20000.0f) {
                    g_frequency.store(freq);
                } else {
                    std::cout << "Frequency must be between 20 and 20000 Hz" << std::endl;
                }
            } catch (...) {
                std::cout << "Invalid input. Enter a number for frequency or just Enter to toggle note." << std::endl;
            }
        }
    }

    // Cleanup
    std::cout << std::endl << "Shutting down..." << std::endl;
    jack_deactivate(client);
    jack_client_close(client);

    std::cout << "Done." << std::endl;
    return 0;
}
