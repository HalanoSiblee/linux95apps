#include "../include/audio.h"
#include <cstring>

#ifdef HAVE_PIPEWIRE
    #include <pipewire/pipewire.h>
    #include <pipewire/stream.h>
#else
    #include <pulse/simple.h>
    #include <pulse/error.h>
#endif

AudioOutput::AudioOutput() {}

AudioOutput::~AudioOutput() {
    close();
}

bool AudioOutput::init(int sampleRate, int channels, int format) {
    if (initialized) return true;
    
    currentSampleRate = sampleRate;
    currentChannels = channels;
    
#ifdef HAVE_PIPEWIRE
    // PipeWire initialization
    initialized = true;
#else
    // PulseAudio initialization
    pa_simple_format_t pa_format;
    switch (format) {
        case 0: pa_format = PA_SAMPLE_S16LE; break;
        case 1: pa_format = PA_SAMPLE_S32LE; break;
        case 2: pa_format = PA_SAMPLE_FLOAT32LE; break;
        default: pa_format = PA_SAMPLE_S16LE;
    }
    
    pa_sample_spec ss;
    ss.format = pa_format;
    ss.rate = sampleRate;
    ss.channels = channels;
    
    int error;
    handle = pa_simple_new(
        nullptr,
        nullptr,
        PA_STREAM_PLAYBACK,
        nullptr,
        "mediaplayer",
        &ss,
        nullptr,
        &error
    );
    
    if (!handle) {
        return false;
    }
    
    initialized = true;
#endif
    
    return true;
}

void AudioOutput::write(const uint8_t* data, size_t size) {
    if (!initialized) return;
    
#ifdef HAVE_PIPEWIRE
    // PipeWire write
#else
    if (handle) {
        int error;
        pa_simple_write(handle, data, size, &error);
    }
#endif
}

void AudioOutput::flush() {
    if (!initialized) return;
    
#ifdef HAVE_PIPEWIRE
    // PipeWire drain
#else
    if (handle) {
        int error;
        pa_simple_drain(handle, &error);
    }
#endif
}

void AudioOutput::close() {
    if (!initialized) return;
    
#ifdef HAVE_PIPEWIRE
    // PipeWire cleanup
#else
    if (handle) {
        pa_simple_free((pa_simple*)handle);
        handle = nullptr;
    }
#endif
    
    initialized = false;
}
