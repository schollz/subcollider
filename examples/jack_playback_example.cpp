/**
 * @file jack_playback_example.cpp
 * @brief JACK audio file playback example using Phasor and BufRd.
 *
 * This example demonstrates loading an audio file using libsndfile and playing
 * it back in a loop using the Phasor and BufRd UGens.
 *
 * Compile with: -ljack -lsndfile
 *
 * Usage:
 *   ./example_jack_playback
 *
 * Press Ctrl+C to quit.
 */

#include <subcollider.h>
#include <subcollider/BufferAllocator.h>
#include <subcollider/ugens/Phasor.h>
#include <subcollider/ugens/BufRd.h>
#include <jack/jack.h>
#include <sndfile.h>
#include <iostream>
#include <atomic>
#include <csignal>
#include <cstring>
#include <ctime>

using namespace subcollider;
using namespace subcollider::ugens;

// Global state for JACK callback
static BufferAllocator<> g_allocator;
static Buffer g_audioBuffer;
static Phasor g_phasor;
static BufRd g_bufRd;
static jack_port_t* g_outputPortL = nullptr;
static jack_port_t* g_outputPortR = nullptr;
static std::atomic<bool> g_running{true};
static float g_fileSampleRate = 0.0f;
static float g_jackSampleRate = 0.0f;

/**
 * @brief Load audio file using libsndfile.
 * @param filename Path to audio file
 * @return true if successful, false otherwise
 */
bool loadAudioFile(const char* filename) {
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(sfInfo));

    SNDFILE* sndFile = sf_open(filename, SFM_READ, &sfInfo);
    if (!sndFile) {
        std::cerr << "Failed to open audio file: " << filename << std::endl;
        std::cerr << "Error: " << sf_strerror(nullptr) << std::endl;
        return false;
    }

    std::cout << "Loaded: " << filename << std::endl;
    std::cout << "  Sample rate: " << sfInfo.samplerate << " Hz" << std::endl;
    std::cout << "  Channels: " << sfInfo.channels << std::endl;
    std::cout << "  Frames: " << sfInfo.frames << std::endl;
    std::cout << "  Duration: " << (float)sfInfo.frames / sfInfo.samplerate << " seconds" << std::endl;

    g_fileSampleRate = static_cast<float>(sfInfo.samplerate);

    // Allocate buffer
    uint8_t channels = (sfInfo.channels >= 2) ? 2 : 1;
    g_audioBuffer = g_allocator.allocate(sfInfo.frames, channels);

    if (!g_audioBuffer.isValid()) {
        std::cerr << "Failed to allocate buffer" << std::endl;
        sf_close(sndFile);
        return false;
    }

    // Read audio data
    if (channels == 1) {
        // Mono file - read directly
        sf_count_t readCount = sf_read_float(sndFile, g_audioBuffer.data, sfInfo.frames);
        if (readCount != sfInfo.frames) {
            std::cerr << "Warning: Only read " << readCount << " of " << sfInfo.frames << " frames" << std::endl;
        }
    } else {
        // Stereo or multi-channel file
        // libsndfile reads interleaved data, which matches our stereo buffer format
        sf_count_t readCount = sf_readf_float(sndFile, g_audioBuffer.data, sfInfo.frames);
        if (readCount != sfInfo.frames) {
            std::cerr << "Warning: Only read " << readCount << " of " << sfInfo.frames << " frames" << std::endl;
        }
    }

    sf_close(sndFile);
    std::cout << "Audio file loaded successfully!" << std::endl;
    return true;
}

/**
 * @brief JACK process callback - ISR-safe audio processing.
 *
 * This function is called by JACK for each audio block.
 * Directly reads from buffer without oversampling for simplicity.
 */
int jackProcessCallback(jack_nframes_t nframes, void*) {
    // Get output buffers from JACK (stereo)
    jack_default_audio_sample_t* outL =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortL, nframes));
    jack_default_audio_sample_t* outR =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortR, nframes));

    // Generate audio directly at JACK's sample rate
    for (jack_nframes_t i = 0; i < nframes; ++i) {
        // Get phase from Phasor (index into buffer)
        float phase = g_phasor.tick();

        // Read from buffer at that phase
        Stereo sample = g_bufRd.tickStereo(phase);
        outL[i] = sample.left;
        outR[i] = sample.right;
    }

    return 0;
}

/**
 * @brief JACK sample rate callback.
 */
int jackSampleRateCallback(jack_nframes_t nframes, void*) {
    std::cout << "JACK sample rate changed to: " << nframes << " Hz" << std::endl;
    g_jackSampleRate = static_cast<float>(nframes);
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

int main(int argc, char* argv[]) {
    std::cout << "SubCollider JACK Playback Example" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;

    // Determine audio file path
    const char* audioFile = "data/amen_beats8_bpm172.wav";
    if (argc > 1) {
        audioFile = argv[1];
    }

    // Set up signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Open JACK client first to get the sample rate
    jack_status_t status;
    jack_client_t* client = jack_client_open("subcollider_playback", JackNoStartServer, &status);

    if (client == nullptr) {
        std::cerr << "Failed to open JACK client. Is JACK server running?" << std::endl;
        if (status & JackServerFailed) {
            std::cerr << "Unable to connect to JACK server" << std::endl;
        }
        return 1;
    }

    std::cout << "Connected to JACK server" << std::endl;

    // Get JACK sample rate
    g_jackSampleRate = static_cast<float>(jack_get_sample_rate(client));
    std::cout << "JACK sample rate: " << g_jackSampleRate << " Hz" << std::endl;

    // Initialize buffer allocator with file's sample rate (will be set when loading)
    g_allocator.init(g_jackSampleRate);

    // Load audio file
    if (!loadAudioFile(audioFile)) {
        jack_client_close(client);
        return 1;
    }

    // Initialize Phasor for playback at JACK's sample rate
    // The Phasor generates indices from 0 to numSamples
    g_phasor.init(g_jackSampleRate);

    // Calculate playback rate
    // The Phasor rate is how many samples (in the file) to advance per tick.
    // If file sample rate matches JACK rate: rate = 1.0 (advance 1 sample per tick)
    // If file rate < JACK rate: rate < 1.0 (file is slower, we interpolate)
    // If file rate > JACK rate: rate > 1.0 (file is faster, we skip samples)
    float playbackRate = g_fileSampleRate / g_jackSampleRate;
    float numSamplesFloat = static_cast<float>(g_audioBuffer.numSamples);

    g_phasor.set(playbackRate, 0.0f, numSamplesFloat, 0.0f);

    std::cout << "File sample rate: " << g_fileSampleRate << " Hz" << std::endl;
    std::cout << "Playback rate: " << playbackRate << std::endl;
    std::cout << "Buffer length: " << g_audioBuffer.numSamples << " samples" << std::endl;

    // Initialize BufRd
    g_bufRd.init(&g_audioBuffer);
    g_bufRd.setLoop(true);
    g_bufRd.setInterpolation(2);  // Linear interpolation

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
        return 1;
    }

    // Activate client
    if (jack_activate(client) != 0) {
        std::cerr << "Failed to activate JACK client" << std::endl;
        jack_client_close(client);
        return 1;
    }

    std::cout << std::endl << "JACK client activated" << std::endl;
    std::cout << "Playing audio in loop... Press Ctrl+C to quit" << std::endl;
    std::cout << std::endl;

    // Main loop - just wait for Ctrl+C
    while (g_running.load()) {
        // Sleep to avoid busy-waiting
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 100000000;  // 100ms
        nanosleep(&ts, nullptr);
    }

    // Cleanup
    std::cout << std::endl << "Shutting down..." << std::endl;
    jack_deactivate(client);
    jack_client_close(client);

    // Release buffer
    g_allocator.release(g_audioBuffer);

    std::cout << "Done." << std::endl;
    return 0;
}
