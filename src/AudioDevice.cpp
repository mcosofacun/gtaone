#include "stdafx.h"
#include "AudioDevice.h"
#include "OpenALDefs.h"

AudioDevice gAudioDevice;

AudioDevice::~AudioDevice()
{
    mBuffersPool.cleanup();
    mSourcesPool.cleanup();
}

bool AudioDevice::Initialize()
{
    gConsole.LogMessage(eLogMessage_Debug, "Audio device initialization...");

    mDevice = ::alcOpenDevice(nullptr);
    if (mDevice == nullptr)
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot open audio device");
        return false;
    }

    gConsole.LogMessage(eLogMessage_Info, "Audio device description: %s", alcGetString(mDevice, ALC_DEVICE_SPECIFIER));

    mContext = ::alcCreateContext(mDevice, nullptr);
    if (mContext == nullptr)
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot create audio device context");

        Deinit();
        return false;
    }

    //alClearError();
    if (!::alcMakeContextCurrent(mContext))
    {
        gConsole.LogMessage(eLogMessage_Warning, "Audio device context error");

        Deinit();
        return false;
    }

    // setup default listener params
    ::alListener3f(AL_POSITION, 0.0f, 0.0f, 1.0f);
    alCheckError();

    ::alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alCheckError();

    ::alListenerf(AL_GAIN, 1.0f);
    alCheckError();

    float orientation_at_up[] = {0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f};
    ::alListenerfv(AL_ORIENTATION, orientation_at_up);
    alCheckError();

    ::alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    alCheckError();

    QueryAudioDeviceCaps();
    return true;
}

void AudioDevice::Deinit()
{
    // destroy sources
    for (AudioSource* currSource: mAllSources)
    {
        mSourcesPool.destroy(currSource);
    }
    mAllSources.clear();

    // destroy buffers
    for (AudioSampleBuffer* currBuffer: mAllBuffers)
    {
        mBuffersPool.destroy(currBuffer);
    }
    mAllBuffers.clear();

    // destroy context
    if (mContext)
    {
        ::alcMakeContextCurrent(nullptr);
        ::alcDestroyContext(mContext);
        mContext = nullptr;
    }

    // shutdown device
    if (mDevice)
    {
        ::alcCloseDevice(mDevice);
        mDevice = nullptr;
    }
}

bool AudioDevice::IsInitialized() const
{
    return mDevice != nullptr;
}

bool AudioDevice::SetMasterVolume(float gainValue)
{
    if (IsInitialized())
    {
        ::alListenerf(AL_GAIN, gainValue);
        alCheckError();
        return true;
    }
    return false;
}

AudioSampleBuffer* AudioDevice::CreateSampleBuffer()
{
    AudioSampleBuffer* audioBuffer = nullptr;
    if (IsInitialized())
    {
        audioBuffer = mBuffersPool.create();
        cxx_assert(audioBuffer);
        if (audioBuffer)
        {
            mAllBuffers.push_back(audioBuffer);
        }
    }
    return audioBuffer;
}

AudioSampleBuffer* AudioDevice::CreateSampleBuffer(int sampleRate, int bitsPerSample, int channelsCount, int dataLength, const void* bufferData)
{
    AudioSampleBuffer* audioBuffer = nullptr;
    if (IsInitialized())
    {
        audioBuffer = CreateSampleBuffer();
        cxx_assert(audioBuffer);

        if (audioBuffer)
        {
            if (!audioBuffer->SetupBufferData(sampleRate, bitsPerSample, channelsCount, dataLength, bufferData))
            {
                cxx_assert(false);
            }
        }
    }
    return audioBuffer;
}

void AudioDevice::DestroySampleBuffer(AudioSampleBuffer* audioBuffer)
{
    if (audioBuffer)
    {
        cxx::erase_elements(mAllBuffers, audioBuffer);
        mBuffersPool.destroy(audioBuffer);
    }
}

AudioSource* AudioDevice::CreateAudioSource()
{
    AudioSource* audioSource = nullptr;
    if (IsInitialized())
    {
        audioSource = mSourcesPool.create();
        cxx_assert(audioSource);
        if (audioSource)
        {
            mAllSources.push_back(audioSource);
        }
    }
    return audioSource;
}

void AudioDevice::DestroyAudioSource(AudioSource* audioSource)
{
    if (audioSource)
    {
        cxx::erase_elements(mAllSources, audioSource);
        mSourcesPool.destroy(audioSource);
    }
}

AudioListener* AudioDevice::CreateAudioListener()
{
    AudioListener* instance = new AudioListener;

    mAllListeners.push_back(instance);
    return instance;
}

void AudioDevice::DestroyAudioListener(AudioListener* audioListener)
{
    if (audioListener)
    {
        cxx::erase_elements(mAllListeners, audioListener);
        delete audioListener;
    }
}

void AudioDevice::UpdateFrame()
{
    if (IsInitialized())
    {
        UpdateSourcesPositions();
    }
}

void AudioDevice::UpdateSourcesPositions()
{
    for (AudioSource* currSource: mAllSources)
    {
        if (!currSource->IsPlaying())
            continue;

        const glm::vec3& sourceLocation = currSource->mSourceLocation;

        AudioListener* nearestListener = nullptr;
        float distanceToListener2 = 0.0f;
        // find the nearest listener
        for (AudioListener* currListener: mAllListeners)
        {
            if (nearestListener == nullptr)
            {
                nearestListener = currListener;
                distanceToListener2 = glm::distance2(sourceLocation, currListener->mPosition);
                continue;
            }
            float dist2 = glm::distance2(sourceLocation, currListener->mPosition);
            if (dist2 < distanceToListener2)
            {
                nearestListener = currListener;
                distanceToListener2 = dist2;
            }
        }

        glm::vec3 listenerLocation {0.0f, 0.0f, 1.0f};
        if (nearestListener)
        {
            listenerLocation = nearestListener->mPosition;
        }

        // relative position
        float posx = sourceLocation.x - listenerLocation.x;
        float posz = sourceLocation.z - listenerLocation.z;

        ::alSource3f(currSource->mSourceID, AL_POSITION, posx, sourceLocation.y, posz);
        alCheckError();
    }
}

void AudioDevice::QueryAudioDeviceCaps()
{
    ::alcGetIntegerv(mDevice, ALC_MONO_SOURCES, 1, &mDeviceCaps.mMaxSourcesMono);
    ::alcGetIntegerv(mDevice, ALC_STEREO_SOURCES, 1, &mDeviceCaps.mMaxSourcesStereo);

    gConsole.LogMessage(eLogMessage_Info, "Audio Device caps:");
    gConsole.LogMessage(eLogMessage_Info, " - max sources mono: %d", mDeviceCaps.mMaxSourcesMono);
    gConsole.LogMessage(eLogMessage_Info, " - max sources stereo: %d", mDeviceCaps.mMaxSourcesStereo);
}

AudioSampleBuffer* AudioDevice::GetSampleBufferWithID(unsigned int bufferID) const
{
    for (AudioSampleBuffer* currBuffer: mAllBuffers)
    {
        if (currBuffer->mBufferID == bufferID)
            return currBuffer;
    }
    cxx_assert(false);
    return nullptr;
}

AudioSource* AudioDevice::GetAudioSourceWithID(unsigned int sourceID) const
{
    for (AudioSource* currSource: mAllSources)
    {
        if (currSource->mSourceID == sourceID)
            return currSource;
    }
    cxx_assert(false);
    return nullptr;
}
