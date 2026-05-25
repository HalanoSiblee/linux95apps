#ifndef AUDIO_H
#define AUDIO_H

#include <cstdint>
#include <cstddef>

class AudioOutput {
public:
    AudioOutput();
    ~AudioOutput();
    
    bool init(int sampleRate, int channels, int format);
    void write(const uint8_t* data, size_t size);
    void flush();
    void close();
    bool isInitialized() const { return initialized; }
    
private:
    bool initialized = false;
    void* handle = nullptr;
    int currentSampleRate = 0;
    int currentChannels = 0;
};

#endif // AUDIO_H
