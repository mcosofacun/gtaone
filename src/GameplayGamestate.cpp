#include "stdafx.h"
#include "GameplayGamestate.h"
#include "GameObjectHelpers.h"
#include "GtaOneGame.h"
#include "Pedestrian.h"
#include "TimeManager.h"
#include "SpriteManager.h"
#include "PhysicsManager.h"
#include "WeatherManager.h"
#include "ParticleEffectsManager.h"
#include "TrafficManager.h"
#include "AiManager.h"

void GameplayGamestate::OnGamestateEnter()
{
    // todo
}

void GameplayGamestate::OnGamestateLeave()
{
    // todo
}

void GameplayGamestate::OnGamestateFrame()
{
    float deltaTime = gTimeManager.mGameFrameDelta;
    gGame.ProcessDebugCvars();

    // update player
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;
    if (humanPlayer)
    {
        humanPlayer->UpdateFrame();
    }

    // advance game state
    gSpriteManager.UpdateBlocksAnimations(deltaTime);
    gPhysics.UpdateFrame();
    gGameObjectsManager.UpdateFrame();
    gWeatherManager.UpdateFrame();
    gParticleManager.UpdateFrame();
    gTrafficManager.UpdateFrame();
    gAiManager.UpdateFrame();
    gBroadcastEvents.UpdateFrame();
}

void GameplayGamestate::OnGamestateInputEvent(KeyInputEvent& inputEvent)
{
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;
    if (humanPlayer)
    {
        humanPlayer->InputEvent(inputEvent);
    }
}

void GameplayGamestate::OnGamestateInputEvent(MouseButtonInputEvent& inputEvent)
{
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;
    if (humanPlayer)
    {
        humanPlayer->InputEvent(inputEvent);
    }
}

void GameplayGamestate::OnGamestateInputEvent(MouseMovedInputEvent& inputEvent)
{
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;
    if (humanPlayer)
    {
        humanPlayer->InputEvent(inputEvent);
    }
}

void GameplayGamestate::OnGamestateInputEvent(MouseScrollInputEvent& inputEvent)
{
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;
    if (humanPlayer)
    {
        humanPlayer->InputEvent(inputEvent);
    }
}

void GameplayGamestate::OnGamestateInputEvent(KeyCharEvent& inputEvent)
{
    // do nothing
}

void GameplayGamestate::OnGamestateInputEvent(GamepadInputEvent& inputEvent)
{
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;
    if (humanPlayer)
    {
        humanPlayer->InputEvent(inputEvent);
    }
}

void GameplayGamestate::OnGamestateInputEventLost()
{
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;
    if (humanPlayer)
    {
        humanPlayer->InputEventLost();
    }
}

void GameplayGamestate::OnGamestateBroadcastEvent(const BroadcastEvent& broadcastEvent)
{
    if (broadcastEvent.mEventType == eBroadcastEvent_PedestrianDead)
    {
        Pedestrian* pedestrian = ToPedestrian(broadcastEvent.mSubject);
        if (pedestrian && pedestrian->IsHumanPlayerCharacter())
        {
            OnHumanPlayerDie();
        }
        return;
    }

    if (broadcastEvent.mEventType == eBroadcastEvent_StartDriveCar)
    {
        Pedestrian* pedestrian = broadcastEvent.mCharacter;
        if (pedestrian && pedestrian->IsHumanPlayerCharacter())
        {
            OnHumanPlayerStartDriveCar();
        }
        return;
    }
}

void GameplayGamestate::OnHumanPlayerDie()
{
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;

    cxx_assert(humanPlayer);
    cxx_assert(humanPlayer->mCharacter);

    humanPlayer->mCharacter->StartGameObjectSound(ePedSfxChannelIndex_Voice, eSfxSampleType_Voice, SfxVoice_PlayerDies, SfxFlags_None);

    // todo: check lives left
    humanPlayer->SetRespawnTimer();

    gConsole.LogMessage(eLogMessage_Info, "Player died (%s)", cxx::enum_to_string(humanPlayer->mCharacter->mDeathReason));

    humanPlayer->mHUD.ShowBigFontMessage(eHUDBigFontMessage_Wasted);
}

void GameplayGamestate::OnHumanPlayerStartDriveCar()
{
    HumanPlayer* humanPlayer = gGame.mHumanPlayer;

    cxx_assert(humanPlayer);
    cxx_assert(humanPlayer->mCharacter);

    Vehicle* currentCar = humanPlayer->mCharacter->mCurrentCar;
    cxx_assert(currentCar);
    if (currentCar)
    {
        eVehicleModel carModel = currentCar->mCarInfo->mModelID;
        humanPlayer->mHUD.ShowCarNameMessage(carModel);
    }   
}
