#pragma once

#include "AudioSource.h"
#include "OpenALDefs.h"

// Hardware audio device
class AudioDevice final: public cxx::noncopyable
{
    friend class AudioSource;

public:
    ~AudioDevice();

    // Setup audio device internal resources
    bool Initialize();
    void Deinit();
    bool IsInitialized() const;

    // Setup device params
    bool SetMasterVolume(float gainValue);

    // change audiolistener position
    void SetListenerPosition(const glm::vec3& position);
    void SetListenerPosition(const glm::vec2& position2);

    // Create sample buffer instance
    AudioSampleBuffer* CreateSampleBuffer(int sampleRate, int bitsPerSample, int channelsCount, int dataLength, const void* bufferData);
    AudioSampleBuffer* CreateSampleBuffer();

    // Create audio source instance
    AudioSource* CreateAudioSource();

    // Free sample buffer instance, it should not be used by anyone at this point
    // @param audioBuffer: Pointer
    void DestroySampleBuffer(AudioSampleBuffer* audioBuffer);

    // Free audio source instance, does not destroys attached audio buffer
    // @param audioInstance: Pointer
    void DestroyAudioSource(AudioSource* audioSource);

private:
    void QueryAudioDeviceCaps();

    // Find sample buffer by internal identifier
    AudioSampleBuffer* GetSampleBufferWithID(unsigned int bufferID) const;

    // Find source by internal identifier
    AudioSource* GetAudioSourceWithID(unsigned int sourceID) const;

private:
    AudioDeviceCaps mDeviceCaps;
    ALCcontext* mContext = nullptr;
    ALCdevice* mDevice = nullptr;
    // allocated objects
    std::vector<AudioSource*> mAllSources;
    std::vector<AudioSampleBuffer*> mAllBuffers;
    // objects pools
    cxx::object_pool<AudioSampleBuffer> mBuffersPool;
    cxx::object_pool<AudioSource> mSourcesPool;
};