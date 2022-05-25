#include "stdafx.h"
#include "AudioManager.h"
#include "GameMapManager.h"
#include "AudioDevice.h"
#include "GtaOneGame.h"
#include "cvars.h"

AudioManager gAudioManager;

//////////////////////////////////////////////////////////////////////////
// cvars

CvarEnum<eGameMusicMode> gCvarGameMusicMode("g_musicMode", eGameMusicMode_Radio, "Game music mode", CvarFlags_Archive);

CvarInt gCvarMusicVolume("g_musicVolume", 3, "Game music volume in range 0-7", CvarFlags_Archive | CvarFlags_RequiresAppRestart);
CvarInt gCvarSoundsVolume("g_soundsVolume", 3, "Audio effects volume in range 0-7", CvarFlags_Archive | CvarFlags_RequiresAppRestart);

//////////////////////////////////////////////////////////////////////////

bool AudioManager::Initialize()
{
    InitSoundsAndMusicGainValue();
    if (!PrepareAudioResources())
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot allocate audio resources");
        return false;
    }
    
    return true;
}

void AudioManager::Deinit()
{
    StopMusic();
    StopAllSounds();

    ReleaseActiveEmitters();
    ReleaseLevelSounds();

    ShutdownAudioResources();
}

void AudioManager::UpdateFrame()
{
    UpdateActiveEmitters();

    UpdateMusic();
}

bool AudioManager::PreloadLevelSounds()
{
    ReleaseLevelSounds();

    gConsole.LogMessage(eLogMessage_Debug, "Loading level sounds...");
    if (!mVoiceSounds.LoadArchive("AUDIO/VOCALCOM"))
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot load Voice sounds");
    }

    std::string audioBankFileName = cxx::va("AUDIO/LEVEL%03d", gGameMap.mAudioFileNumber); 
    if (!mLevelSounds.LoadArchive(audioBankFileName))
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot load Level sounds");
    }

    mLevelSfxSamples.resize(mLevelSounds.GetEntriesCount());
    mVoiceSfxSamples.resize(mVoiceSounds.GetEntriesCount());

    return true;
}

void AudioManager::ReleaseLevelSounds()
{
    // stop all sources and detach buffers
    for (AudioSource* source: mSfxAudioSources)
    {
        source->Stop();
        source->SetSampleBuffer(nullptr);
    }

    mLevelSounds.FreeArchive();
    mVoiceSounds.FreeArchive();

    for (SfxSample* currSample: mLevelSfxSamples)
    {
        SafeDelete(currSample);
    }

    for (SfxSample* currSample: mVoiceSfxSamples)
    {
        SafeDelete(currSample);
    }

    mLevelSfxSamples.clear();
    mVoiceSfxSamples.clear();
}

void AudioManager::ShutdownAudioResources()
{
    for (AudioSource* currSource: mSfxAudioSources)
    {
        gAudioDevice.DestroyAudioSource(currSource);
    }
    mSfxAudioSources.clear();

    if (mMusicAudioSource)
    {
        gAudioDevice.DestroyAudioSource(mMusicAudioSource);
        mMusicAudioSource = nullptr;
    }

    for (AudioSampleBuffer* currBuffer: mMusicSampleBuffers)
    {
        gAudioDevice.DestroySampleBuffer(currBuffer);
    }
    cxx_assert(MaxMusicSampleBuffers == mMusicSampleBuffers.size()); // check for leaks
    mMusicSampleBuffers.clear();
}

AudioSource* AudioManager::GetFreeAudioSource() const
{
    for (AudioSource* currSource: mSfxAudioSources)
    {
        if (currSource && !currSource->IsPlaying() && !currSource->IsPaused())
            return currSource;
    }

    return nullptr;
}

bool AudioManager::PrepareAudioResources()
{
    for (int icurr = 0; icurr < MaxSfxAudioSources; ++icurr)
    {
        AudioSource* audioSource = gAudioDevice.CreateAudioSource();
        if (audioSource == nullptr)
            break;

        mSfxAudioSources.push_back(audioSource);
    }

    // allocate additional music source
    mMusicAudioSource = gAudioDevice.CreateAudioSource();
    cxx_assert(mMusicAudioSource);

    if (mMusicAudioSource)
    {
        mMusicAudioSource->SetGain(mMusicGain);
    }

    // allocate music sample buffers
    for (int icurr = 0; icurr < MaxMusicSampleBuffers; ++icurr)
    {
        AudioSampleBuffer* sampleBuffer = gAudioDevice.CreateSampleBuffer();
        if (sampleBuffer == nullptr)
            break;

        mMusicSampleBuffers.push_back(sampleBuffer);
    }

    return true;
}

void AudioManager::StopAllSounds()
{
    for (AudioSource* currSource: mSfxAudioSources)
    {
        if (currSource->IsPlaying() || currSource->IsPaused())
        {
            currSource->Stop();
        }
    }
}

void AudioManager::PauseAllSounds()
{
    for (AudioSource* currSource: mSfxAudioSources)
    {
        if (currSource->IsPlaying())
        {
            currSource->Pause();
        }
    }
}

void AudioManager::ResumeAllSounds()
{
    for (AudioSource* currSource: mSfxAudioSources)
    {
        if (currSource->IsPaused())
        {
            currSource->Resume();
        }
    }
}

SfxSample* AudioManager::GetSound(eSfxSampleType sfxType, SfxSampleIndex sfxIndex)
{
    AudioSampleArchive& sampleArchive = (sfxType == eSfxSampleType_Level) ? mLevelSounds : mVoiceSounds;
    if ((int) sfxIndex >= sampleArchive.GetEntriesCount())
    {
        cxx_assert(false);
        return nullptr;
    }

    std::vector<SfxSample*>& samples = (sfxType == eSfxSampleType_Level) ? 
        mLevelSfxSamples : 
        mVoiceSfxSamples;

    if (samples[sfxIndex] == nullptr)
    {
        AudioSampleArchive::SampleEntry archiveEntry;
        if (!sampleArchive.GetEntryData(sfxIndex, archiveEntry))
        {
            cxx_assert(false);
            return nullptr;
        }
        // upload audio data
        AudioSampleBuffer* audioBuffer = gAudioDevice.CreateSampleBuffer(
            archiveEntry.mSampleRate,
            archiveEntry.mBitsPerSample,
            archiveEntry.mChannelsCount,
            archiveEntry.mDataLength,
            archiveEntry.mData);
        cxx_assert(audioBuffer && !audioBuffer->IsBufferError());

        // free source data
        sampleArchive.FreeEntryData(sfxIndex);

        if (audioBuffer)
        {
            samples[sfxIndex] = new SfxSample(sfxType, sfxIndex, audioBuffer);
        }
    }
    return samples[sfxIndex];
}

SfxEmitter* AudioManager::CreateEmitter(GameObject* gameObject, const glm::vec3& emitterPosition, SfxEmitterFlags emitterFlags)
{
    SfxEmitter* emitter = mEmittersPool.create(gameObject, emitterFlags);
    cxx_assert(emitter);
    emitter->UpdateEmitterParams(emitterPosition);
    return emitter;
}

void AudioManager::DestroyEmitter(SfxEmitter* sfxEmitter)
{
    if (sfxEmitter == nullptr)
    {
        cxx_assert(false);
        return;
    }

    mEmittersPool.destroy(sfxEmitter);
    cxx::erase_elements(mActiveEmitters, sfxEmitter);
}

void AudioManager::ReleaseActiveEmitters()
{
    if (mActiveEmitters.empty())
        return;

    std::vector<SfxEmitter*> activeEmitters;
    std::vector<SfxEmitter*> deleteEmitters;
    activeEmitters.swap(mActiveEmitters);

    for (SfxEmitter* currEmitter: activeEmitters)
    {
        currEmitter->StopAllSounds();
        if (currEmitter->IsAutoreleaseEmitter())
        {
            deleteEmitters.push_back(currEmitter);
        }
    }
    // destroy autorelease emitters
    for (SfxEmitter* currEmitter: deleteEmitters)
    {
        mEmittersPool.destroy(currEmitter);
    }
}

void AudioManager::UpdateActiveEmitters()
{
    if (mActiveEmitters.empty())
        return;

    std::vector<SfxEmitter*> inactiveEmitters;
    for (SfxEmitter* currEmitter: mActiveEmitters)
    {
        if (currEmitter->mGameObject) // sync audio params
        {
            glm::vec3 gameObjectPosition = currEmitter->mGameObject->mTransform.mPosition;
            currEmitter->UpdateEmitterParams(gameObjectPosition);
        }

        currEmitter->UpdateSounds();
        if (!currEmitter->IsActiveEmitter())
        {
            inactiveEmitters.push_back(currEmitter);
        }
    }

    for (SfxEmitter* currEmitter: inactiveEmitters)
    {
        cxx::erase_elements(mActiveEmitters, currEmitter);
        if (currEmitter->IsAutoreleaseEmitter())
        {
            mEmittersPool.destroy(currEmitter);
        }
    }
}

bool AudioManager::StartSound(eSfxSampleType sfxType, SfxSampleIndex sfxIndex, SfxFlags sfxFlags, const glm::vec3& emitterPosition)
{
    SfxSample* audioSample = GetSound(sfxType, sfxIndex);
    if (audioSample == nullptr)
        return false;

    SfxEmitter* autoreleaseEmitter = CreateEmitter(nullptr, emitterPosition, SfxEmitterFlags_Autorelease);
    cxx_assert(autoreleaseEmitter);
    if (autoreleaseEmitter)
    {
        if (autoreleaseEmitter->StartSound(0, audioSample, sfxFlags))
            return true;

        DestroyEmitter(autoreleaseEmitter);
    }
    return false;
}

float AudioManager::NextRandomPitch()
{
    static const float _pitchValues[] = {0.95f, 1.0f, 1.1f};

    int randomIndex = gGame.mGameRand.generate_int() % CountOf(_pitchValues);
    return _pitchValues[randomIndex];
}

void AudioManager::RegisterActiveEmitter(SfxEmitter* emitter)
{
    if (emitter == nullptr)
    {
        cxx_assert(false);
        return;
    }

    if (!cxx::contains(mActiveEmitters, emitter))
    {
        mActiveEmitters.push_back(emitter);
    }
}

void AudioManager::UpdateMusic()
{
    if ((gCvarGameMusicMode.mValue == eGameMusicMode_Disabled) || (mMusicStatus == eMusicStatus_Stopped))
        return;

    if (mMusicStatus == eMusicStatus_NextTrackRequest)
    {
        StartMusic("MUSIC/Track11"); // ambient 
        return;
    }

    if (mMusicStatus == eMusicStatus_Playing)
    {
        // process buffers
        std::vector<AudioSampleBuffer*> sampleBuffers;
        if (!mMusicAudioSource->ProcessBuffersQueue(sampleBuffers))
        {
            StopMusic();
            return;
        }
        if (!sampleBuffers.empty())
        {
            mMusicSampleBuffers.insert(mMusicSampleBuffers.end(), sampleBuffers.begin(), sampleBuffers.end());
        }

        if (!QueueMusicSampleBuffers())
        {
            if (mMusicDataStream->EndOfStream())
            {
                StopMusic();
                mMusicStatus = eMusicStatus_NextTrackRequest;
                return;
            }
        }

        if (!mMusicAudioSource->IsPlaying() && !mMusicAudioSource->Start())
        {
            StopMusic();
            return;
        }

        return;
    }
}

bool AudioManager::StartMusic(const char* music)
{
    StopMusic();

    if (mMusicAudioSource == nullptr)
        return false;

    mMusicDataStream = OpenAudioFileStream(music);
    if (mMusicDataStream == nullptr)
        return false;

    if (!QueueMusicSampleBuffers())
    {
        SafeDelete(mMusicDataStream);
        return false;
    }

    if (!mMusicAudioSource->Start())
    {
        StopMusic();
        return false;
    }

    mMusicStatus = eMusicStatus_Playing;
    return true;
}

void AudioManager::StopMusic()
{
    mMusicStatus = eMusicStatus_Stopped;

    if (mMusicAudioSource)
    {
        std::vector<AudioSampleBuffer*> sampleBuffers;
        mMusicAudioSource->Stop();
        mMusicAudioSource->ProcessBuffersQueue(sampleBuffers);
        if (!sampleBuffers.empty())
        {
            mMusicSampleBuffers.insert(mMusicSampleBuffers.end(), sampleBuffers.begin(), sampleBuffers.end());
        }
    }

    SafeDelete(mMusicDataStream);
}

bool AudioManager::QueueMusicSampleBuffers()
{
    cxx_assert(mMusicDataStream);
    cxx_assert(mMusicAudioSource);

    int bytesPerSample = mMusicDataStream->GetChannelsCount() * (mMusicDataStream->GetSampleBits() / 8);
    int samplesPerBuffer = MusicSampleBufferSize / bytesPerSample;
    // fill sample buffers
    int totalSamplesProcessed = 0;
    for (;;)
    {
        if (mMusicSampleBuffers.empty())
            break;

        int numSamplesRead = mMusicDataStream->ReadPCMSamples(samplesPerBuffer, mMusicSampleData);
        if (numSamplesRead == 0)
            break;

        int dataLength = numSamplesRead * bytesPerSample;
        totalSamplesProcessed += numSamplesRead;

        AudioSampleBuffer* sampleBuffer = mMusicSampleBuffers.front();
        if (!sampleBuffer->SetupBufferData(mMusicDataStream->GetSampleRate(), 
            mMusicDataStream->GetSampleBits(), 
            mMusicDataStream->GetChannelsCount(), dataLength, mMusicSampleData))
        {
            cxx_assert(false);
            break;
        }
        if (!mMusicAudioSource->QueueSampleBuffer(sampleBuffer))
        {
            cxx_assert(false);
            break;
        }

        mMusicSampleBuffers.pop_front();
    }

    return totalSamplesProcessed > 0;
}

void AudioManager::InitSoundsAndMusicGainValue()
{
    int musicVolume = glm::clamp(gCvarMusicVolume.mValue, AudioMinVolume, AudioMaxVolume);
    if (gCvarMusicVolume.mValue != musicVolume)
    {
        gConsole.LogMessage(eLogMessage_Warning, "Music volume is '%d', must be in range [%d-%d]", gCvarMusicVolume.mValue, AudioMinVolume, AudioMaxVolume);
        gCvarMusicVolume.mValue = musicVolume;
        gCvarMusicVolume.ClearModified();
    }
    
    int soundsVolume = glm::clamp(gCvarSoundsVolume.mValue, AudioMinVolume, AudioMaxVolume);
    if (gCvarSoundsVolume.mValue != soundsVolume)
    {
        gConsole.LogMessage(eLogMessage_Warning, "Sounds volume is '%d', must be in range [%d-%d]", gCvarSoundsVolume.mValue, AudioMinVolume, AudioMaxVolume);
        gCvarSoundsVolume.mValue = soundsVolume;
        gCvarSoundsVolume.ClearModified();
    }

    mMusicGain = (1.0f * musicVolume) / (1.0f * AudioMaxVolume);
    mSoundsGain = (1.0f * soundsVolume) / (1.0f * AudioMaxVolume);
}
