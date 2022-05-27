#include "stdafx.h"
#include "GtaOneGame.h"
#include "ConsoleWindow.h"
#include "GameCheatsWindow.h"
#include "Pedestrian.h"
#include "cvars.h"

//////////////////////////////////////////////////////////////////////////
// cvars
//////////////////////////////////////////////////////////////////////////

CvarBoolean gCvarMouseAiming("mouse_aiming", false, "Enable mouse aiming", CvarFlags_Archive | CvarFlags_RequiresAppRestart);

CvarString gCvarMapname("g_mapname", "", "Current map name", CvarFlags_Init);
CvarString gCvarCurrentBaseDir("g_basedir", "", "Current gta data location", CvarFlags_Init);
CvarEnum<eGtaGameVersion> gCvarGameVersion("g_gamever", eGtaGameVersion_Unknown, "Current gta game version", CvarFlags_Init);
CvarString gCvarGameLanguage("g_gamelang", "en", "Current game language", CvarFlags_Init);

// debug
CvarVoid gCvarDbgDumpSpriteDeltas("dbg_dumpSpriteDeltas", "Dump sprite deltas", CvarFlags_None);
CvarVoid gCvarDbgDumpBlockTextures("dbg_dumpBlocks", "Dump block textures", CvarFlags_None);
CvarVoid gCvarDbgDumpSprites("dbg_dumpSprites", "Dump all sprites", CvarFlags_None);
CvarVoid gCvarDbgDumpCarSprites("dbg_dumpCarSprites", "Dump car sprites", CvarFlags_None);

//////////////////////////////////////////////////////////////////////////

GtaOneGame gGame;

//////////////////////////////////////////////////////////////////////////

bool GtaOneGame::Initialize()
{
    cxx_assert(mCurrentGamestate == nullptr);

    // init randomizer
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    mRandom.set_seed((unsigned int) ms.count());

    mParams.SetToDefaults();

    if (!DetectGameVersion())
    {
        gSystem.LogMessage(eLogMessage_Debug, "Fail to detect game version");
    }

    if (!mRenderMng.Initialize())
    {
        gSystem.LogMessage(eLogMessage_Error, "Cannot initialize render manager");
        return false;
    }

    if (!mDebugRenderer.Initialize())
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot initialize debug renderer");
    }

    if (!mMapRenderer.Initialize())
    {
        gSystem.LogMessage(eLogMessage_Error, "Cannot initialize map renderer");
        return false;
    }

    gGameCheatsWindow.mWindowShown = true; // show by default

    if (gCvarMapname.mValue.empty())
    {
        // try load first found map
        if (!gSystem.mFiles.mGameMapsList.empty())
        {
            gCvarMapname.mValue = gSystem.mFiles.mGameMapsList[0];
        }
    }
    gCvarMapname.ClearModified();

    // init texts
    mTextsMng.Initialize();

    if (!mAudioMng.Initialize())
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot initialize audio manager");
    }

    mTimeMng.Initialize();

    if (!mGuiMng.Initialize())
    {
        gSystem.LogMessage(eLogMessage_Error, "Cannot initialize gui manager");
    }

    if (!mImGuiMng.Initialize())
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot initialize debug ui manager");
    }

    // init scenario
    if (!StartScenario(gCvarMapname.mValue))
    {
        ShutdownCurrentScenario();
        gSystem.LogMessage(eLogMessage_Warning, "Fail to start game"); 
        return false;
    }
    return true;
}

void GtaOneGame::Deinit()
{
    ShutdownCurrentScenario();

    mTextsMng.Deinit();
    mAudioMng.Deinit();
    mImGuiMng.Deinit();
    mGuiMng.Deinit();
    mSpritesMng.Cleanup();
    mMapRenderer.Deinit();
    mDebugRenderer.Deinit();
    mRenderMng.Deinit();

    cxx_assert(mCurrentGamestate == nullptr);
}

bool GtaOneGame::IsMenuGameState() const
{
    return mCurrentGamestate == &mMainMenuGamestate;
}

bool GtaOneGame::IsInGameState() const
{
    return mCurrentGamestate == &mGameplayGamestate;
}

void GtaOneGame::UpdateFrame()
{
    mTimeMng.UpdateFrame();
    mGuiMng.UpdateFrame();
    mImGuiMng.UpdateFrame();

    if (mCameraController)
    {
        mCameraController->UpdateFrame();
    }
    mCamera.ComputeViewBounds2();

    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateFrame();
    }

    ProcessBroadcastEvents();

    mAudioMng.UpdateFrame();
}

void GtaOneGame::RenderFrame()
{
    mMapRenderer.RenderFrameBegin();
    mSpritesMng.RenderFrameBegin();

    mCamera.ComputeMatricesAndFrustum();
    gSystem.mGfxDevice.SetViewportRect(mCamera.mViewportRect);

    mMapRenderer.RenderFrame(mCamera);
    mRenderMng.RenderParticleEffects(mCamera);

    // debug draw
    if (mCamera.mDebugDrawFlags != GameCameraDebugDrawFlags_None)
    {
        mDebugRenderer.RenderFrameBegin(mCamera);

        if (mCamera.CheckDebugDrawFlags(GameCameraDebugDrawFlags_Map))
        {
            mMapRenderer.DebugDraw(mDebugRenderer);
        }

        if (mCamera.CheckDebugDrawFlags(GameCameraDebugDrawFlags_Traffic))
        {
            mTrafficMng.DebugDraw(mDebugRenderer);
        }

        if (mCamera.CheckDebugDrawFlags(GameCameraDebugDrawFlags_Ai))
        {
            mAiMng.DebugDraw(mDebugRenderer);
        }

        if (mCamera.CheckDebugDrawFlags(GameCameraDebugDrawFlags_Particles))
        {
            mParticlesMng.DebugDraw(mDebugRenderer);
        }

        mDebugRenderer.RenderFrameEnd();
    }

    mGuiMng.RenderFrame();

    mSpritesMng.RenderFrameEnd();
    mMapRenderer.RenderFrameEnd();
}

void GtaOneGame::InputEventLost()
{
    if (mCameraController)
    {
        mCameraController->InputEventLost();
    }

    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateInputEventLost();
    }
}

void GtaOneGame::InputEvent(KeyInputEvent& inputEvent)
{
    if (inputEvent.HasPressed(eKeycode_F3))
    {
        mRenderMng.ReloadRenderPrograms();
        return;
    }

    if (inputEvent.HasPressed(eKeycode_ESCAPE))
    {
        gSystem.QuitRequest();
        return;
    }

    if (inputEvent.HasPressed(eKeycode_C))
    {
        gGameCheatsWindow.mWindowShown = !gGameCheatsWindow.mWindowShown;
        return;
    }

    if (mCameraController)
    {
        mCameraController->InputEvent(inputEvent);
    }

    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateInputEvent(inputEvent);
    }
}

void GtaOneGame::InputEvent(MouseButtonInputEvent& inputEvent)
{
    if (mCameraController)
    {
        mCameraController->InputEvent(inputEvent);
    }

    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateInputEvent(inputEvent);
    }
}

void GtaOneGame::InputEvent(MouseMovedInputEvent& inputEvent)
{
    if (mCameraController)
    {
        mCameraController->InputEvent(inputEvent);
    }

    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateInputEvent(inputEvent);
    }
}

void GtaOneGame::InputEvent(MouseScrollInputEvent& inputEvent)
{
    if (mCameraController)
    {
        mCameraController->InputEvent(inputEvent);
    }

    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateInputEvent(inputEvent);
    }
}

void GtaOneGame::InputEvent(KeyCharEvent& inputEvent)
{
    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateInputEvent(inputEvent);
    }
}

void GtaOneGame::InputEvent(GamepadInputEvent& inputEvent)
{
    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateInputEvent(inputEvent);
    }
}

bool GtaOneGame::StartScenario(const std::string& mapName)
{
    ShutdownCurrentScenario();

    // setup view
    mCamera.mViewportRect = gSystem.mGfxDevice.mViewportRect;

    if (mapName.empty())
    {
        gSystem.LogMessage(eLogMessage_Warning, "Map name is not specified");
        return false;
    }

    if (!mMap.LoadFromFile(mapName))
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot load map '%s'", mapName.c_str());
        return false;
    }

    // load corresponding style data
    std::string styleFileName = mMap.GetStyleFileName();

    gSystem.LogMessage(eLogMessage_Info, "Loading style data '%s'", styleFileName.c_str());
    if (!mStyleData.LoadFromFile(styleFileName))
        return false;

    if (!mAudioMng.PreloadLevelSounds())
    {
        // ignore
    }

    mSpritesMng.Cleanup();
    mMapRenderer.BuildMapMesh();
    if (!mSpritesMng.InitSprites(&mStyleData))
    {
        cxx_assert(false);
    }

    mPhysicsMng.EnterWorld();
    mParticlesMng.EnterWorld();
    mObjectsMng.EnterWorld();

    // temporary
    //glm::vec3 pos { 108.0f, 2.0f, 25.0f };
    //glm::vec3 pos { 14.0, 2.0f, 38.0f };
    //glm::vec3 pos { 91.0f, 2.0f, 236.0f };
    //glm::vec3 pos { 121.0f, 2.0f, 200.0f };
    //glm::vec3 pos { 174.0f, 2.0f, 230.0f };

    glm::vec3 playerSpawnPoint;

    // choose spawn point
    // it is temporary!
    for (int yBlock = 10; yBlock < 20; ++yBlock)
    {
        for (int xBlock = 10; xBlock < 20; ++xBlock)
        {
            for (int zBlock = MAP_LAYERS_COUNT - 1; zBlock > -1; --zBlock)
            {
                const MapBlockInfo* currBlock = mMap.GetBlockInfo(xBlock, yBlock, zBlock);
                if (currBlock->mGroundType == eGroundType_Field ||
                    currBlock->mGroundType == eGroundType_Pawement ||
                    currBlock->mGroundType == eGroundType_Road)
                {
                    playerSpawnPoint = Convert::MapUnitsToMeters(glm::vec3(xBlock + 0.5f, zBlock, yBlock + 0.5f));
                    yBlock = 20;
                    xBlock = 20;
                    break;
                }
            }
        }
    }

    // spawn player ped

    cxx::angle_t pedestrianHeading { 360.0f * mRandom.generate_float(), cxx::angle_t::units::degrees };

    Pedestrian* pedestrian = mObjectsMng.CreatePedestrian(playerSpawnPoint, pedestrianHeading, ePedestrianType_Player);
    mPlayerState.SetCharacter(pedestrian);
    mPlayerState.Cheat_GiveAllAmmunitions(); // temporary

    mHUD.InitHUD(mCamera.mViewportRect);
    mTrafficMng.StartupTraffic();
    mWeatherMng.EnterWorld();

    SetCurrentGamestate(&mGameplayGamestate);
    return true;
}

void GtaOneGame::ShutdownCurrentScenario()
{
    SetCurrentGamestate(nullptr);
    mPlayerState.SetCharacter(nullptr);
    ClearBroadcastEvents();

    mHUD.DeinitHUD();
    mAiMng.ReleaseAiControllers();
    mTrafficMng.CleanupTraffic();
    mWeatherMng.ClearWorld();
    mObjectsMng.ClearWorld();
    mPhysicsMng.ClearWorld();
    mStyleData.Cleanup();
    mMap.Cleanup();
    mAudioMng.ReleaseLevelSounds();
    mParticlesMng.ClearWorld();
}

bool GtaOneGame::DetectGameVersion()
{
    bool useAutoDetection = (gCvarGameVersion.mValue == eGtaGameVersion_Unknown);
    if (useAutoDetection)
    {
        const int GameMapsCount = (int) gSystem.mFiles.mGameMapsList.size();
        if (GameMapsCount == 0)
            return false;

        if (gSystem.mFiles.IsFileExists("MISSUK.INI"))
        {
            gCvarGameVersion.mValue = eGtaGameVersion_MissionPack1_London69;
        }
        else if (gSystem.mFiles.IsFileExists("missuke.ini"))
        {
            gCvarGameVersion.mValue = eGtaGameVersion_MissionPack2_London61;
        }
        else if (GameMapsCount < 3)
        {
            gCvarGameVersion.mValue = eGtaGameVersion_Demo;
        }
        else
        {
            gCvarGameVersion.mValue = eGtaGameVersion_Full;
        }
    }

    gSystem.LogMessage(eLogMessage_Debug, "Gta game version is '%s' (%s)", cxx::enum_to_string(gCvarGameVersion.mValue),
        useAutoDetection ? "autodetect" : "forced");
    
    return true;
}

void GtaOneGame::ProcessDebugCvars()
{
    if (gCvarDbgDumpSpriteDeltas.IsModified())
    {
        gCvarDbgDumpSpriteDeltas.ClearModified();
        std::string savePath = gSystem.mFiles.mExecutableDirectory + "/sprite_deltas";
        mSpritesMng.DumpSpriteDeltas(savePath);
        gSystem.LogMessage(eLogMessage_Info, "Sprite deltas path is '%s'", savePath.c_str());
    }

    if (gCvarDbgDumpBlockTextures.IsModified())
    {
        gCvarDbgDumpBlockTextures.ClearModified();
        std::string savePath = gSystem.mFiles.mExecutableDirectory + "/block_textures";
        mSpritesMng.DumpBlocksTexture(savePath);
        gSystem.LogMessage(eLogMessage_Info, "Blocks textures path is '%s'", savePath.c_str());
    }

    if (gCvarDbgDumpSprites.IsModified())
    {
        gCvarDbgDumpSprites.ClearModified();
        std::string savePath = gSystem.mFiles.mExecutableDirectory + "/sprites";
        mSpritesMng.DumpSpriteTextures(savePath);
        gSystem.LogMessage(eLogMessage_Info, "Sprites path is '%s'", savePath.c_str());
    }

    if (gCvarDbgDumpCarSprites.IsModified())
    {
        gCvarDbgDumpCarSprites.ClearModified();
        std::string savePath = gSystem.mFiles.mExecutableDirectory + "/car_sprites";
        mSpritesMng.DumpCarsTextures(savePath);
        gSystem.LogMessage(eLogMessage_Info, "Car sprites path is '%s'", savePath.c_str());
    }
}

void GtaOneGame::SetCurrentGamestate(Gamestate* gamestate)
{
    if (mCurrentGamestate == gamestate)
        return;

    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateLeave();
    }

    mCurrentGamestate = gamestate;
    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateEnter();
    }
}

void GtaOneGame::ReportEvent(eBroadcastEvent eventType, GameObject* subject, Pedestrian* character, float durationTime)
{
    cxx_assert(subject);

    if (subject == nullptr)
        return;

    float currentGameTime = mTimeMng.mGameTime;

    const glm::vec2 position2 = subject->mTransform.GetPosition2();
    // check same event is exists
    for (BroadcastEvent& evData: mBroadcastEventsList)
    {
        if ((evData.mEventType == eventType) && (evData.mSubject == subject) && (evData.mPosition == position2))
        {
            evData.mEventTimestamp = currentGameTime;
            evData.mEventDurationTime = durationTime;
            return;
        }
    }

    mBroadcastEventsList.emplace_back();
    BroadcastEvent& evData = mBroadcastEventsList.back();
    // fill event data
    evData.mEventType = eventType;
    evData.mEventTimestamp = currentGameTime;
    evData.mEventDurationTime = durationTime;
    evData.mPosition = position2;
    evData.mSubject = subject;
    evData.mCharacter = character;

    // notify current gamestate
    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateBroadcastEvent(evData);
    }
}

void GtaOneGame::ReportEvent(eBroadcastEvent eventType, const glm::vec2& position, float durationTime)
{
    float currentGameTime = mTimeMng.mGameTime;

    // update time if same event is exists
    for (BroadcastEvent& evData: mBroadcastEventsList)
    {
        if ((evData.mEventType == eventType) && (evData.mPosition == position) && (evData.mSubject == nullptr))
        {
            evData.mEventTimestamp = currentGameTime;
            evData.mEventDurationTime = durationTime;
            return;
        }
    }

    mBroadcastEventsList.emplace_back();
    BroadcastEvent& evData = mBroadcastEventsList.back();
    // fill event data
    evData.mEventType = eventType;
    evData.mEventTimestamp = currentGameTime;
    evData.mEventDurationTime = durationTime;
    evData.mPosition = position;

    // notify current gamestate
    if (mCurrentGamestate)
    {
        mCurrentGamestate->OnGamestateBroadcastEvent(evData);
    }
}

void GtaOneGame::ClearBroadcastEvents()
{
    mBroadcastEventsList.clear();
}

void GtaOneGame::ProcessBroadcastEvents()
{
    float currentGameTime = mTimeMng.mGameTime;

    // remove obsolete events from list
    for (auto curr_event_iterator = mBroadcastEventsList.begin(); 
        curr_event_iterator != mBroadcastEventsList.end(); )
    {
        BroadcastEvent& eventData = *curr_event_iterator;
        if ((eventData.mStatus == BroadcastEvent::Status_Active) && 
            ((eventData.mEventTimestamp + eventData.mEventDurationTime) > currentGameTime))
        {
            ++curr_event_iterator;
            continue;
        }
        // remove expired event
        curr_event_iterator = mBroadcastEventsList.erase(curr_event_iterator);
    }
}

void GtaOneGame::SetFollowCameraControlMode(Pedestrian* character)
{
    cxx_assert(character);
    mCameraController = &mFollowCameraController;
    mFollowCameraController.SetFollowTarget(character);
    mCameraController->Setup(&mCamera);
}

void GtaOneGame::SetFreeLookCameraControlMode()
{
    if (mCameraController == &mFreeLookCameraController)
        return;

    mCameraController = &mFreeLookCameraController;
    mCameraController->Setup(&mCamera);
}

eGameCameraControlMode GtaOneGame::GetCameraControlMode() const
{
    if (mCameraController == &mFollowCameraController)
        return eGameCameraControlMode_Follow;

    cxx_assert(mCameraController == &mFreeLookCameraController);
    return eGameCameraControlMode_FreeLook;
}

