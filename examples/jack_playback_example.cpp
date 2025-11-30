/**
 * @file jack_playback_example.cpp
 * @brief JACK audio file playback example using Phasor and BufRd.
 *
 * This example demonstrates loading an audio file using libsndfile and playing
 * it back in a loop using the Phasor and BufRd UGens. The playback rate is
 * automatically scaled to match the difference between the server sample rate
 * and the file's original sample rate.
 *
 * Compile with: -ljack -lsndfile
 *
 * Usage:
 *   ./example_jack_playback
 *
 * Press Ctrl+C to quit.
 */

#include <jack/jack.h>
#include <sndfile.h>
#include <subcollider.h>
#include <subcollider/BufferAllocator.h>
#include <subcollider/ugens/BufRd.h>
#include <subcollider/ugens/Phasor.h>
#include <X11/Xlib.h>

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cmath>
#include <cstring>
#include <ctime>
#include <iostream>
#include <unistd.h>

using namespace subcollider;
using namespace subcollider::ugens;

// Global state for JACK callback
static BufferAllocator<> g_allocator;
static Buffer g_audioBuffer;
static Phasor g_phasor;
static BufRd g_bufRd;
static Lag g_rateLag;
static jack_port_t* g_outputPortL = nullptr;
static jack_port_t* g_outputPortR = nullptr;
static std::atomic<bool> g_running{true};
static float g_fileSampleRate = 0.0f;
static float g_sampleRate = 48000.0f;
static std::atomic<float> g_basePlaybackRate{1.0f};
static std::atomic<float> g_rateControl{1.0f};
static constexpr float MIN_RATE = 0.25f;
static constexpr float MAX_RATE = 4.0f;
static constexpr float CENTER_DEADZONE = 0.02f;  // normalized distance from center

void configurePlayback(float sampleRate) {
  g_sampleRate = sampleRate;
  g_phasor.init(g_sampleRate);
  g_rateLag.init(g_sampleRate, 0.2f);

  if (g_audioBuffer.isValid()) {
    float basePlaybackRate =
        g_fileSampleRate / g_sampleRate;  // step in samples per tick
    g_basePlaybackRate.store(basePlaybackRate, std::memory_order_relaxed);
    float numSamplesFloat = static_cast<float>(g_audioBuffer.numSamples);
    float targetRate =
        basePlaybackRate * g_rateControl.load(std::memory_order_relaxed);
    g_rateLag.setValue(targetRate);
    g_phasor.set(targetRate, 0.0f, numSamplesFloat, 0.0f);

    std::cout << "Playback rate scaling: " << targetRate << std::endl;
    std::cout << "Buffer length: " << g_audioBuffer.numSamples << " samples"
              << std::endl;
  }
}

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
  std::cout << "  Duration: " << (float)sfInfo.frames / sfInfo.samplerate
            << " seconds" << std::endl;

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
    sf_count_t readCount =
        sf_read_float(sndFile, g_audioBuffer.data, sfInfo.frames);
    if (readCount != sfInfo.frames) {
      std::cerr << "Warning: Only read " << readCount << " of " << sfInfo.frames
                << " frames" << std::endl;
    }
  } else {
    // Stereo or multi-channel file
    // libsndfile reads interleaved data, which matches our stereo buffer format
    sf_count_t readCount =
        sf_readf_float(sndFile, g_audioBuffer.data, sfInfo.frames);
    if (readCount != sfInfo.frames) {
      std::cerr << "Warning: Only read " << readCount << " of " << sfInfo.frames
                << " frames" << std::endl;
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
 * Processes audio at the JACK sample rate.
 */
int jackProcessCallback(jack_nframes_t nframes, void*) {
  // Get output buffers from JACK (stereo)
  jack_default_audio_sample_t* outL = static_cast<jack_default_audio_sample_t*>(
      jack_port_get_buffer(g_outputPortL, nframes));
  jack_default_audio_sample_t* outR = static_cast<jack_default_audio_sample_t*>(
      jack_port_get_buffer(g_outputPortR, nframes));

  float targetRate = g_basePlaybackRate.load(std::memory_order_relaxed) *
                     g_rateControl.load(std::memory_order_relaxed);

  // Generate audio at JACK rate using Phasor and BufRd
  for (jack_nframes_t i = 0; i < nframes; ++i) {
    float smoothRate = g_rateLag.tick(targetRate);
    g_phasor.setRate(smoothRate);

    // Get phase from Phasor
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

int main(int argc, char* argv[]) {
  std::cout << "SubCollider JACK Playback Example" << std::endl;
  std::cout << "==================================" << std::endl;
  std::cout << std::endl;

  // Determine audio file path
  const char* audioFile = "data/amen_16_48000.wav";
  if (argc > 1) {
    audioFile = argv[1];
  }

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

  // Open JACK client
  jack_status_t status;
  jack_client_t* client =
      jack_client_open("subcollider_playback", JackNoStartServer, &status);

  if (client == nullptr) {
    std::cerr << "Failed to open JACK client. Is JACK server running?"
              << std::endl;
    if (status & JackServerFailed) {
      std::cerr << "Unable to connect to JACK server" << std::endl;
    }
    XCloseDisplay(display);
    return 1;
  }

  std::cout << std::endl << "Connected to JACK server" << std::endl;

  // Get JACK sample rate
  jack_nframes_t jackSampleRate = jack_get_sample_rate(client);
  std::cout << "JACK sample rate: " << jackSampleRate << " Hz" << std::endl;

  // Initialize buffer allocator at JACK rate
  g_allocator.init(static_cast<float>(jackSampleRate));

  // Load audio file
  if (!loadAudioFile(audioFile)) {
    jack_client_close(client);
    XCloseDisplay(display);
    return 1;
  }

  // Initialize Phasor for playback
  // The Phasor will generate indices from 0 to numSamples
  // We need to scale the rate to account for:
  // 1. The JACK sample rate
  // 2. The file's original sample rate
  configurePlayback(static_cast<float>(jackSampleRate));

  // Initialize BufRd
  g_bufRd.init(&g_audioBuffer);
  g_bufRd.setLoop(true);
  g_bufRd.setInterpolation(4);  // Cubic interpolation for best quality

  // Set callbacks
  jack_set_process_callback(client, jackProcessCallback, nullptr);
  jack_set_sample_rate_callback(client, jackSampleRateCallback, nullptr);
  jack_on_shutdown(client, jackShutdownCallback, nullptr);

  // Create stereo output ports
  g_outputPortL = jack_port_register(
      client, "output_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  g_outputPortR = jack_port_register(
      client, "output_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

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

  std::cout << std::endl << "JACK client activated" << std::endl;
  std::cout << "Playing audio in loop... Press Ctrl+C to quit" << std::endl;
  std::cout << "Move mouse horizontally to control playback rate (-4x..0..4x)"
            << std::endl;
  std::cout << std::endl;

  // Main loop - just wait for Ctrl+C
  while (g_running.load()) {
    Window returnedRoot, returnedChild;
    int rootX, rootY, winX, winY;
    unsigned int mask;

    if (XQueryPointer(display, root, &returnedRoot, &returnedChild, &rootX,
                     &rootY, &winX, &winY, &mask)) {
      (void)rootY;
      (void)winX;
      (void)winY;
      (void)mask;
      float normalizedX =
          static_cast<float>(rootX) / static_cast<float>(screenWidth);
      normalizedX = std::max(0.0f, std::min(1.0f, normalizedX));

      // Exponential mapping for musical-feeling rate control with direction.
      // Center pauses; left reverses; right forwards.
      float centered = (normalizedX - 0.5f) * 2.0f;  // -1..1
      float magNorm = std::abs(centered);
      float rate = 0.0f;
      if (magNorm > CENTER_DEADZONE) {
        float scaledNorm =
            (magNorm - CENTER_DEADZONE) / (1.0f - CENTER_DEADZONE);
        scaledNorm = std::max(0.0f, std::min(1.0f, scaledNorm));
        float logMin = std::log(MIN_RATE);
        float logMax = std::log(MAX_RATE);
        float magnitude = std::exp(logMin + scaledNorm * (logMax - logMin));
        rate = (centered > 0.0f ? magnitude : -magnitude);
      }

      g_rateControl.store(rate, std::memory_order_relaxed);
      std::cout << "\rPlayback rate: " << rate << "x" << std::flush;
    }

    usleep(50000);  // 50ms polling
  }

  // Cleanup
  std::cout << std::endl << "Shutting down..." << std::endl;
  jack_deactivate(client);
  jack_client_close(client);
  XCloseDisplay(display);

  // Release buffer
  g_allocator.release(g_audioBuffer);

  std::cout << "Done." << std::endl;
  return 0;
}
