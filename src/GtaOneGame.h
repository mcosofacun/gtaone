#pragma once

#include "GameMap.h"
#include "GameObjectsManager.h"
#include "PlayerState.h"
#include "GameplayGamestate.h"
#include "MainMenuGamestate.h"
#include "GameHUD.h"
#include "TrafficManager.h"
#include "SpriteManager.h"
#include "GameTextsManager.h"
#include "WeatherManager.h"
#include "ParticleEffectsManager.h"
#include "AiManager.h"
#include "BroadcastEvent.h"
#include "PhysicsManager.h"
#include "AudioManager.h"
#include "FollowCameraController.h"
#include "FreeLookCameraController.h"
#include "TimeManager.h"
#include "StyleData.h"
#include "GuiManager.h"
#include "ImGuiManager.h"
#include "RenderManager.h"
#include "GameMapRenderer.h"
#include "DebugRenderer.h"

// top level game application controller
class GtaOneGame final: public InputEventsHandler
{
    friend class GameplayGamestate;
    friend class MainMenuGamestate;
    friend class BroadcastEventsIterator;

public:
    cxx::randomizer mRandom;

    CameraController* mCameraController = nullptr;
    Gamestate* mCurrentGamestate = nullptr;

    GameCamera mCamera;
    GameParams mParams;
    GameMap mMap;
    GameHUD mHUD;
    StyleData mStyleData;
    PlayerState mPlayerState;

    // managers
    TrafficManager mTrafficMng;
    WeatherManager mWeatherMng;
    SpriteManager mSpritesMng;
    GameTextsManager mTextsMng;
    ParticleEffectsManager mParticlesMng;
    AiManager mAiMng;
    GuiManager mGuiMng;
    ImGuiManager mImGuiMng;
    TimeManager mTimeMng;
    AudioManager mAudioMng;
    PhysicsManager mPhysicsMng;
    GameObjectsManager mObjectsMng;
    RenderManager mRenderMng;
    GameMapRenderer mMapRenderer;
    DebugRenderer mDebugRenderer;

public:
    bool Initialize();
    void Deinit();

    void UpdateFrame();
    void RenderFrame();

    // override InputEventsHandler
    void InputEvent(KeyInputEvent& inputEvent) override;
    void InputEvent(MouseButtonInputEvent& inputEvent) override;
    void InputEvent(MouseMovedInputEvent& inputEvent) override;
    void InputEvent(MouseScrollInputEvent& inputEvent) override;
    void InputEvent(KeyCharEvent& inputEvent) override;
    void InputEvent(GamepadInputEvent& inputEvent) override;
    void InputEventLost() override;

    // Current game state
    bool IsMenuGameState() const;
    bool IsInGameState() const;

    // broadcast events
    void ReportEvent(eBroadcastEvent eventType, GameObject* subject, Pedestrian* character, float durationTime);
    void ReportEvent(eBroadcastEvent eventType, const glm::vec2& position, float durationTime);
    void ClearBroadcastEvents();
    void ProcessBroadcastEvents();

    // public for debug purposes
    void SetFollowCameraControlMode(Pedestrian* character);
    void SetFreeLookCameraControlMode();
    eGameCameraControlMode GetCameraControlMode() const;

private:
    bool DetectGameVersion();

    bool StartScenario(const std::string& mapName);
    void ShutdownCurrentScenario();

    void SetCurrentGamestate(Gamestate* gamestate);

    void ProcessDebugCvars();

private:
    GameplayGamestate mGameplayGamestate;
    MainMenuGamestate mMainMenuGamestate;

    std::vector<BroadcastEvent> mBroadcastEventsList;

    FollowCameraController mFollowCameraController;
    FreeLookCameraController mFreeLookCameraController;
};

extern GtaOneGame gGame;