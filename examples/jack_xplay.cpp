/**
 * @file jack_xplay.cpp
 * @brief JACK example using XPlay to loop and crossfade a buffer segment.
 *
 * Loads data/amen_beats8_bpm172.wav and maps mouse X/Y to loop start/end
 * (both 0..1) with 200ms Lag smoothing. Uses XPlay for dual-head
 * crossfaded playback plus LFNoise modulation for balance/level.
 */

#include <jack/jack.h>
#include <sndfile.h>
#include <X11/Xlib.h>

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <subcollider.h>
#include <subcollider/BufferAllocator.h>
#include <subcollider/ugens/XPlay.h>
#include <subcollider/ugens/Lag.h>

using namespace subcollider;
using namespace subcollider::ugens;

// Globals
static BufferAllocator<> g_allocator;
static Buffer g_buffer;
static XPlay g_xplay;
static Lag g_startLag;
static Lag g_endLag;
static std::atomic<float> g_startTarget{0.0f};
static std::atomic<float> g_endTarget{1.0f};
static jack_port_t* g_outputPortL = nullptr;
static jack_port_t* g_outputPortR = nullptr;
static std::atomic<bool> g_running{true};
static float g_sampleRate = 48000.0f;

// Forward decls
bool loadAudioFile(const char* filename);
void configurePlayback(float sr);

/**
 * @brief JACK process callback.
 */
int jackProcessCallback(jack_nframes_t nframes, void*) {
    jack_default_audio_sample_t* outL = static_cast<jack_default_audio_sample_t*>(
        jack_port_get_buffer(g_outputPortL, nframes));
    jack_default_audio_sample_t* outR = static_cast<jack_default_audio_sample_t*>(
        jack_port_get_buffer(g_outputPortR, nframes));

    float startTarget = g_startTarget.load(std::memory_order_relaxed);
    float endTarget = g_endTarget.load(std::memory_order_relaxed);

    for (jack_nframes_t i = 0; i < nframes; ++i) {
        float smStart = g_startLag.tick(startTarget);
        float smEnd = g_endLag.tick(endTarget);
        g_xplay.setStartEnd(smStart, smEnd, true);

        Stereo s = g_xplay.tick();
        outL[i] = s.left;
        outR[i] = s.right;
    }

    return 0;
}

/**
 * @brief JACK sample-rate callback.
 */
int jackSampleRateCallback(jack_nframes_t nframes, void*) {
    std::cout << "JACK sample rate changed to: " << nframes << " Hz" << std::endl;
    configurePlayback(static_cast<float>(nframes));
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
void signalHandler(int) { g_running.store(false); }

/**
 * @brief Load audio file via libsndfile.
 */
bool loadAudioFile(const char* filename) {
    SF_INFO info;
    std::memset(&info, 0, sizeof(info));
    SNDFILE* snd = sf_open(filename, SFM_READ, &info);
    if (!snd) {
        std::cerr << "Failed to open audio file: " << filename << "\n"
                  << sf_strerror(nullptr) << std::endl;
        return false;
    }

    std::cout << "Loaded file: " << filename << std::endl;
    std::cout << "  Sample rate: " << info.samplerate << " Hz" << std::endl;
    std::cout << "  Channels: " << info.channels << std::endl;
    std::cout << "  Frames: " << info.frames << std::endl;

    uint8_t channels = (info.channels >= 2) ? 2 : 1;
    g_buffer = g_allocator.allocate(info.frames, channels);
    if (!g_buffer.isValid()) {
        std::cerr << "Failed to allocate buffer" << std::endl;
        sf_close(snd);
        return false;
    }

    if (channels == 1) {
        sf_count_t read = sf_read_float(snd, g_buffer.data, info.frames);
        if (read != info.frames) {
            std::cerr << "Warning: read " << read << " of " << info.frames << " frames" << std::endl;
        }
    } else {
        sf_count_t read = sf_readf_float(snd, g_buffer.data, info.frames);
        if (read != info.frames) {
            std::cerr << "Warning: read " << read << " of " << info.frames << " frames" << std::endl;
        }
    }

    g_buffer.sampleRate = static_cast<float>(info.samplerate);
    sf_close(snd);
    return true;
}

/**
 * @brief Configure UGens after sample-rate changes.
 */
void configurePlayback(float sr) {
    g_sampleRate = sr;
    g_startLag.init(sr, 0.2f);
    g_endLag.init(sr, 0.2f);
    g_startLag.setValue(g_startTarget.load(std::memory_order_relaxed));
    g_endLag.setValue(g_endTarget.load(std::memory_order_relaxed));

    g_xplay.init(sr);
    g_xplay.setBuffer(&g_buffer);
    g_xplay.reader.setInterpolation(4);
    g_xplay.reader.setLoop(true);
    g_xplay.setFadeTime(0.05f);
    g_xplay.setGate(1.0f);
    g_xplay.setRate(1.0f);
    g_xplay.setStartEnd(g_startTarget.load(std::memory_order_relaxed),
                        g_endTarget.load(std::memory_order_relaxed));
}

int main() {
    std::cout << "SubCollider XPlay JACK Example" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "Mouse X -> start (0..1), Mouse Y -> end (0..1) with 200ms Lag smoothing."
              << std::endl;
    std::cout << "Loading data/amen_beats8_bpm172.wav" << std::endl;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // X11 for mouse tracking
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X11 display" << std::endl;
        return 1;
    }
    Window root = DefaultRootWindow(display);
    int screenWidth = DisplayWidth(display, DefaultScreen(display));
    int screenHeight = DisplayHeight(display, DefaultScreen(display));

    // JACK client
    jack_status_t status;
    jack_client_t* client =
        jack_client_open("subcollider_xplay", JackNoStartServer, &status);
    if (!client) {
        std::cerr << "Failed to open JACK client" << std::endl;
        XCloseDisplay(display);
        return 1;
    }

    // Allocator at JACK rate
    jack_nframes_t jackRate = jack_get_sample_rate(client);
    g_allocator.init(static_cast<float>(jackRate));

    // Load file
    if (!loadAudioFile("data/amen_beats8_bpm172.wav")) {
        jack_client_close(client);
        XCloseDisplay(display);
        return 1;
    }

    configurePlayback(static_cast<float>(jackRate));

    // Ports and callbacks
    jack_set_process_callback(client, jackProcessCallback, nullptr);
    jack_set_sample_rate_callback(client, jackSampleRateCallback, nullptr);
    jack_on_shutdown(client, jackShutdownCallback, nullptr);

    g_outputPortL = jack_port_register(
        client, "output_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    g_outputPortR = jack_port_register(
        client, "output_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!g_outputPortL || !g_outputPortR) {
        std::cerr << "Failed to create JACK output ports" << std::endl;
        jack_client_close(client);
        XCloseDisplay(display);
        return 1;
    }

    if (jack_activate(client) != 0) {
        std::cerr << "Failed to activate JACK client" << std::endl;
        jack_client_close(client);
        XCloseDisplay(display);
        return 1;
    }

    std::cout << "JACK client activated. Press Ctrl+C to quit." << std::endl;

    // Main loop: poll mouse for start/end targets
    while (g_running.load()) {
        Window returnedRoot, returnedChild;
        int rootX, rootY, winX, winY;
        unsigned int mask;
        if (XQueryPointer(display, root, &returnedRoot, &returnedChild,
                          &rootX, &rootY, &winX, &winY, &mask)) {
            float normX = static_cast<float>(rootX) / static_cast<float>(screenWidth);
            float normY = static_cast<float>(rootY) / static_cast<float>(screenHeight);
            normX = std::max(0.0f, std::min(1.0f, normX));
            normY = std::max(0.0f, std::min(1.0f, normY));
            g_startTarget.store(normX, std::memory_order_relaxed);
            g_endTarget.store(normY, std::memory_order_relaxed);
            std::cout << "\rStart: " << normX << "  End: " << normY << std::flush;
        }
        usleep(50000); // 50ms
    }

    std::cout << std::endl << "Shutting down..." << std::endl;
    jack_deactivate(client);
    jack_client_close(client);
    XCloseDisplay(display);
    g_allocator.release(g_buffer);
    std::cout << "Done." << std::endl;
    return 0;
}
