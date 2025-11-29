/**
 * @file jack_playback_example.cpp
 * @brief JACK audio file playback example using Phasor and BufRd.
 *
 * This example demonstrates loading an audio file using libsndfile and playing
 * it back in a loop using the Phasor and BufRd UGens. The playback rate is
 * automatically scaled to match the difference between the server sample rate
 * (with 2x oversampling at 96kHz) and the file's original sample rate.
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
#include <subcollider/ugens/Downsampler.h>
#include <jack/jack.h>
#include <sndfile.h>
#include <iostream>
#include <atomic>
#include <csignal>
#include <cstring>
#include <ctime>

using namespace subcollider;
using namespace subcollider::ugens;

// Configuration
static constexpr float INTERNAL_SAMPLE_RATE = 96000.0f;  // 2x oversampling

// Global state for JACK callback
static BufferAllocator<> g_allocator;
static Buffer g_audioBuffer;
static Phasor g_phasor;
static BufRd g_bufRd;
static Downsampler g_downsamplerL;
static Downsampler g_downsamplerR;
static jack_port_t* g_outputPortL = nullptr;
static jack_port_t* g_outputPortR = nullptr;
static std::atomic<bool> g_running{true};
static float g_fileSampleRate = 0.0f;

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
 * Processes audio at 96kHz internally, then downsamples to JACK's rate.
 */
int jackProcessCallback(jack_nframes_t nframes, void*) {
    // Get output buffers from JACK (stereo)
    jack_default_audio_sample_t* outL =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortL, nframes));
    jack_default_audio_sample_t* outR =
        static_cast<jack_default_audio_sample_t*>(
            jack_port_get_buffer(g_outputPortR, nframes));

    // Process at 2x oversampling rate (96kHz)
    // For each output frame, we need to generate 2 internal frames
    constexpr size_t oversampleFactor = 2;
    const size_t internalFrames = nframes * oversampleFactor;

    // Temporary buffers for oversampled audio (max 2048 frames at 2x = 4096 samples)
    constexpr size_t MAX_BUFFER_SIZE = 4096;
    static float tempL[MAX_BUFFER_SIZE];
    static float tempR[MAX_BUFFER_SIZE];

    if (internalFrames > MAX_BUFFER_SIZE) {
        std::cerr << "Warning: Buffer size exceeds maximum!" << std::endl;
        return 0;
    }

    // Generate audio at 96kHz using Phasor and BufRd
    for (size_t i = 0; i < internalFrames; ++i) {
        // Get phase from Phasor
        float phase = g_phasor.tick();

        // Read from buffer at that phase
        Stereo sample = g_bufRd.tickStereo(phase);
        tempL[i] = sample.left;
        tempR[i] = sample.right;
    }

    // Downsample back to JACK's sample rate
    g_downsamplerL.process(tempL, outL, nframes);
    g_downsamplerR.process(tempR, outR, nframes);

    return 0;
}

/**
 * @brief JACK sample rate callback.
 */
int jackSampleRateCallback(jack_nframes_t nframes, void*) {
    std::cout << "JACK sample rate changed to: " << nframes << " Hz" << std::endl;

    // Initialize downsamplers for the new rate (outputRate, oversampleFactor)
    g_downsamplerL.init(static_cast<float>(nframes), 2);
    g_downsamplerR.init(static_cast<float>(nframes), 2);

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

    // Initialize buffer allocator
    g_allocator.init(INTERNAL_SAMPLE_RATE);

    // Load audio file
    if (!loadAudioFile(audioFile)) {
        return 1;
    }

    // Open JACK client
    jack_status_t status;
    jack_client_t* client = jack_client_open("subcollider_playback", JackNoStartServer, &status);

    if (client == nullptr) {
        std::cerr << "Failed to open JACK client. Is JACK server running?" << std::endl;
        if (status & JackServerFailed) {
            std::cerr << "Unable to connect to JACK server" << std::endl;
        }
        return 1;
    }

    std::cout << std::endl << "Connected to JACK server" << std::endl;

    // Get JACK sample rate
    jack_nframes_t jackSampleRate = jack_get_sample_rate(client);
    std::cout << "JACK sample rate: " << jackSampleRate << " Hz" << std::endl;
    std::cout << "Internal processing rate: " << INTERNAL_SAMPLE_RATE << " Hz (2x oversampling)" << std::endl;

    // Initialize Phasor for playback
    // The Phasor will generate indices from 0 to numSamples
    // We need to scale the rate to account for:
    // 1. The internal sample rate (96kHz)
    // 2. The file's original sample rate
    g_phasor.init(INTERNAL_SAMPLE_RATE);

    // Calculate playback rate
    // rate = (end - start) * playbackFrequency / sampleRate
    // playbackFrequency = fileSampleRate / bufferDuration
    // Since we want to play at the original speed:
    // rate = numSamples * (fileSampleRate / numSamples) / INTERNAL_SAMPLE_RATE
    // rate = fileSampleRate / INTERNAL_SAMPLE_RATE
    float playbackRate = g_fileSampleRate / INTERNAL_SAMPLE_RATE;
    float numSamplesFloat = static_cast<float>(g_audioBuffer.numSamples);

    g_phasor.set(playbackRate * numSamplesFloat, 0.0f, numSamplesFloat, 0.0f);

    std::cout << "Playback rate scaling: " << playbackRate << std::endl;
    std::cout << "Buffer length: " << g_audioBuffer.numSamples << " samples" << std::endl;

    // Initialize BufRd
    g_bufRd.init(&g_audioBuffer);
    g_bufRd.setLoop(true);
    g_bufRd.setInterpolation(2);  // Linear interpolation

    // Initialize downsamplers (outputRate, oversampleFactor)
    g_downsamplerL.init(static_cast<float>(jackSampleRate), 2);
    g_downsamplerR.init(static_cast<float>(jackSampleRate), 2);

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
        jack_nframes_t sleepTime = 100000;  // 100ms in microseconds
        jack_time_t sleepNanos = sleepTime * 1000;
        struct timespec ts;
        ts.tv_sec = sleepNanos / 1000000000;
        ts.tv_nsec = sleepNanos % 1000000000;
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
