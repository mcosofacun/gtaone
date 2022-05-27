#include "stdafx.h"
#include "GameplayGamestate.h"
#include "GameObjectHelpers.h"
#include "GtaOneGame.h"
#include "Pedestrian.h"
#include "cvars.h"

void GameplayGamestate::OnGamestateEnter()
{
    gGame.SetFollowCameraControlMode(gGame.mPlayerState.mCharacter);
}

void GameplayGamestate::OnGamestateLeave()
{
    // todo
}

void GameplayGamestate::OnGamestateFrame()
{
    float deltaTime = gGame.mTimeMng.mGameFrameDelta;
    gGame.ProcessDebugCvars();

    // advance game state
    UpdatePlayer();

    gGame.mSpritesMng.UpdateBlocksAnimations(deltaTime);
    gGame.mPhysicsMng.UpdateFrame();
    gGame.mObjectsMng.UpdateFrame();
    gGame.mWeatherMng.UpdateFrame();
    gGame.mParticlesMng.UpdateFrame();
    gGame.mTrafficMng.UpdateFrame();
    gGame.mAiMng.UpdateFrame();
}

void GameplayGamestate::OnGamestateInputEvent(KeyInputEvent& inputEvent)
{
    eInputActionsGroup actionGroup = gGame.mPlayerState.mCharacter->IsCarPassenger() ? 
        eInputActionsGroup_InCar : 
        eInputActionsGroup_OnFoot;
    eInputAction action = gSystem.mInputs.mActionsMapping.GetAction(actionGroup, inputEvent.mKeycode);
    if (action == eInputAction_null)
    {
        // try commons
        action = gSystem.mInputs.mActionsMapping.GetAction(eInputActionsGroup_Common, inputEvent.mKeycode);
    }

    if (action != eInputAction_null)
    {
        ProcessInputAction(action, inputEvent.mPressed);
        inputEvent.SetConsumed();
    }

    // handle special keys
    if (inputEvent.HasPressed(eKeycode_F9))
    {
        ShowLastDistrictLocation();
        inputEvent.SetConsumed();
    }
}

void GameplayGamestate::OnGamestateInputEvent(MouseButtonInputEvent& inputEvent)
{

}

void GameplayGamestate::OnGamestateInputEvent(MouseMovedInputEvent& inputEvent)
{

}

void GameplayGamestate::OnGamestateInputEvent(MouseScrollInputEvent& inputEvent)
{

}

void GameplayGamestate::OnGamestateInputEvent(KeyCharEvent& inputEvent)
{
    // do nothing
}

void GameplayGamestate::OnGamestateInputEvent(GamepadInputEvent& inputEvent)
{
    if (inputEvent.mGamepad != gSystem.mInputs.mActionsMapping.mGamepadID)
        return;

    eInputActionsGroup actionGroup = gGame.mPlayerState.mCharacter->IsCarPassenger() ? 
        eInputActionsGroup_InCar : 
        eInputActionsGroup_OnFoot;
    eInputAction action = gSystem.mInputs.mActionsMapping.GetAction(actionGroup, inputEvent.mButton);
    if (action == eInputAction_null)
    {
        // try commons
        action = gSystem.mInputs.mActionsMapping.GetAction(eInputActionsGroup_Common, inputEvent.mButton);
    }

    if (action != eInputAction_null)
    {
        ProcessInputAction(action, inputEvent.mPressed);
        inputEvent.SetConsumed();
    }
}

void GameplayGamestate::OnGamestateInputEventLost()
{
    // reset actions
    gGame.mPlayerState.mCtlState.Clear();
    gGame.mPlayerState.mUpdateInputs = false;
}

void GameplayGamestate::OnGamestateBroadcastEvent(const BroadcastEvent& broadcastEvent)
{
    if (broadcastEvent.mEventType == eBroadcastEvent_PedestrianDead)
    {
        Pedestrian* pedestrian = ToPedestrian(broadcastEvent.mSubject);
        if (pedestrian && pedestrian->IsPlayerCharacter())
        {
            OnHumanPlayerDie();
        }
        return;
    }

    if (broadcastEvent.mEventType == eBroadcastEvent_StartDriveCar)
    {
        Pedestrian* pedestrian = broadcastEvent.mCharacter;
        if (pedestrian && pedestrian->IsPlayerCharacter())
        {
            OnHumanPlayerStartDriveCar();
        }
        return;
    }
}

void GameplayGamestate::OnHumanPlayerDie()
{
    cxx_assert(gGame.mPlayerState.mCharacter);

    gGame.mPlayerState.mCharacter->StartGameObjectSound(ePedSfxChannelIndex_Voice, eSfxSampleType_Voice, SfxVoice_PlayerDies, SfxFlags_None);

    // todo: check lives left
    gGame.mPlayerState.SetRespawnTimer();

    gSystem.LogMessage(eLogMessage_Info, "Player died (%s)", cxx::enum_to_string(gGame.mPlayerState.mCharacter->mDeathReason));

    gGame.mHUD.ShowBigFontMessage(eHUDBigFontMessage_Wasted);
}

void GameplayGamestate::OnHumanPlayerStartDriveCar()
{
    cxx_assert(gGame.mPlayerState.mCharacter);

    Vehicle* currentCar = gGame.mPlayerState.mCharacter->mCurrentCar;
    cxx_assert(currentCar);
    if (currentCar)
    {
        eVehicleModel carModel = currentCar->mCarInfo->mModelID;
        gGame.mHUD.ShowCarNameMessage(carModel);
    }   
}

void GameplayGamestate::ProcessInputAction(eInputAction action, bool isActivated)
{
    switch (action)
    {
        case eInputAction_SteerLeft:
        case eInputAction_TurnLeft:
        case eInputAction_SteerRight:
        case eInputAction_TurnRight:
        case eInputAction_Run:
        case eInputAction_Accelerate:
        case eInputAction_Reverse:
        case eInputAction_WalkBackward:
        case eInputAction_WalkForward:
        case eInputAction_Jump:
        case eInputAction_HandBrake:
        case eInputAction_Shoot:
            // those are updating in ProcessRepetitiveActions
        break;

        case eInputAction_Horn:
            if (gGame.mPlayerState.mCharacter->IsCarPassenger())
            {
                gGame.mPlayerState.mCtlState.mHorn = isActivated;
                if (gGame.mPlayerState.mCharacter->mCurrentCar->HasEmergencyLightsAnimation())
                {
                    gGame.mPlayerState.mCharacter->mCurrentCar->EnableEmergencyLights(isActivated);
                }
            }
        break;

        case eInputAction_NextWeapon:
            if (isActivated)
            {
                SwitchNextWeapon();
            }
        break;

        case eInputAction_PrevWeapon:
            if (isActivated)
            {
                SwitchPrevWeapon();
            }
        break;

        case eInputAction_Special:
            gGame.mPlayerState.mCtlState.mSpecial = isActivated;
        break;

        case eInputAction_EnterCar:
        case eInputAction_LeaveCar:
        case eInputAction_EnterCarAsPassenger:
            if (isActivated)
            {
                EnterOrExitCar(action == eInputAction_EnterCarAsPassenger);
            }
        break;

        default:
            cxx_assert(false);
        break;
    }

    gGame.mPlayerState.mUpdateInputs = true;
}

void GameplayGamestate::ShowLastDistrictLocation()
{
    gGame.mHUD.ShowDistrictNameMessage(gGame.mPlayerState.mLastDistrictIndex);
}

void GameplayGamestate::ProcessRepetitiveActions()
{
    if (!gGame.mPlayerState.mUpdateInputs)
        return;

    gGame.mPlayerState.mCtlState.Clear();

    // update in car
    if (gGame.mPlayerState.mCharacter->IsCarPassenger())
    {
        gGame.mPlayerState.mCtlState.mSteerDirection = 0.0f;
        if (GetActionState(eInputAction_SteerRight))
        {
            gGame.mPlayerState.mCtlState.mSteerDirection += 1.0f;
        }
        if (GetActionState(eInputAction_SteerLeft))
        {
            gGame.mPlayerState.mCtlState.mSteerDirection -= 1.0f;
        }

        gGame.mPlayerState.mCtlState.mAcceleration = 0.0f;
        if (GetActionState(eInputAction_Accelerate))
        {
            gGame.mPlayerState.mCtlState.mAcceleration += 1.0f;
        }
        if (GetActionState(eInputAction_Reverse))
        {
            gGame.mPlayerState.mCtlState.mAcceleration -= 1.0f;
        }

        gGame.mPlayerState.mCtlState.mHandBrake = GetActionState(eInputAction_HandBrake);
    }
    // update on foot
    else
    {
        gGame.mPlayerState.mCtlState.mTurnLeft = GetActionState(eInputAction_TurnLeft);
        gGame.mPlayerState.mCtlState.mTurnRight = GetActionState(eInputAction_TurnRight);
        gGame.mPlayerState.mCtlState.mRun = GetActionState(eInputAction_Run);
        gGame.mPlayerState.mCtlState.mWalkBackward = GetActionState(eInputAction_WalkBackward);
        gGame.mPlayerState.mCtlState.mWalkForward = GetActionState(eInputAction_WalkForward);
        gGame.mPlayerState.mCtlState.mJump = GetActionState(eInputAction_Jump);
        gGame.mPlayerState.mCtlState.mShoot = GetActionState(eInputAction_Shoot);

        if ((gGame.mPlayerState.mCtlState.mTurnLeft == false) && 
            (gGame.mPlayerState.mCtlState.mTurnRight == false))
        {
            UpdateMouseAiming();
        }
    }
}

bool GameplayGamestate::GetActionState(eInputAction action) const
{
    const auto& mapping = gSystem.mInputs.mActionsMapping.mActionToKeys[action];
    if (mapping.mKeycode != eKeycode_null)
    {
        if (gSystem.mInputs.GetKeyState(mapping.mKeycode))
            return true;
    }
    if (mapping.mGpButton != eGamepadButton_null)
    {
        if (gSystem.mInputs.GetGamepadButtonState(gSystem.mInputs.mActionsMapping.mGamepadID, mapping.mGpButton))
            return true;
    }
    return false;
}

void GameplayGamestate::UpdateDistrictLocation()
{
    const DistrictInfo* currentDistrict = gGame.mMap.GetDistrictAtPosition2(gGame.mPlayerState.mCharacter->mTransform.GetPosition2());
    if (currentDistrict == nullptr)
        return;

    if (currentDistrict->mSampleIndex != gGame.mPlayerState.mLastDistrictIndex)
    {
        gGame.mPlayerState.mLastDistrictIndex = currentDistrict->mSampleIndex;
        gGame.mHUD.ShowDistrictNameMessage(gGame.mPlayerState.mLastDistrictIndex);
    }
}

void GameplayGamestate::UpdateMouseAiming()
{
    if (!gCvarMouseAiming.mValue)
        return;

    // get current mouse position in world space
    glm::ivec2 screenPosition(gSystem.mInputs.mCursorPositionX, gSystem.mInputs.mCursorPositionY);
    cxx::ray3d_t raycastResult;
    if (!gGame.mCamera.CastRayFromScreenPoint(screenPosition, raycastResult))
        return;

    float distanceFromCameraToCharacter = gGame.mCamera.mPosition.y - gGame.mPlayerState.mCharacter->mTransform.mPosition.y;
    glm::vec2 worldPosition
    (
        raycastResult.mOrigin.x + (raycastResult.mDirection.x * distanceFromCameraToCharacter), 
        raycastResult.mOrigin.z + (raycastResult.mDirection.z * distanceFromCameraToCharacter)
    );
    glm::vec2 toTarget = worldPosition - gGame.mPlayerState.mCharacter->mTransform.GetPosition2();
    gGame.mPlayerState.mCtlState.mRotateToDesiredAngle = true;
    gGame.mPlayerState.mCtlState.mDesiredRotationAngle = cxx::angle_t::from_radians(::atan2f(toTarget.y, toTarget.x));
}

void GameplayGamestate::UpdatePlayer()
{
    if (gGame.mPlayerState.mCharacter->GetWeapon().IsOutOfAmmunition())
    {
        SwitchPrevWeapon();
    }

    if (gGame.mPlayerState.mCharacter->IsDead())
    {
        UpdateRespawnTimer();
    }

    ProcessRepetitiveActions();

    UpdateDistrictLocation();
}

void GameplayGamestate::EnterOrExitCar(bool alternative)
{
    if (gGame.mPlayerState.mCharacter->IsCarPassenger())
    {
        gGame.mPlayerState.mCharacter->LeaveCar();
        return;
    }

    PhysicsQueryResult queryResults;

    glm::vec3 pos = gGame.mPlayerState.mCharacter->mPhysicsBody->GetPosition();
    glm::vec2 posA { pos.x, pos.z };
    glm::vec2 posB = posA + (gGame.mPlayerState.mCharacter->mPhysicsBody->GetSignVector() * gGame.mParams.mPedestrianSpotTheCarDistance);

    gGame.mPhysicsMng.QueryObjectsLinecast(posA, posB, queryResults, CollisionGroup_Car);

    // process all cars
    for (int iElement = 0; iElement < queryResults.mElementsCount; ++iElement)
    {
        PhysicsBody* physicsBody = queryResults.mElements[iElement].mPhysicsObject;
        cxx_assert(physicsBody);
        cxx_assert(physicsBody->mGameObject);
        cxx_assert(physicsBody->mGameObject->IsVehicleClass());

        Vehicle* carObject = (Vehicle*) physicsBody->mGameObject;

        eCarSeat carSeat = alternative ? eCarSeat_Passenger : eCarSeat_Driver;
        if (carObject->IsSeatPresent(carSeat))
        {
            gGame.mPlayerState.mCharacter->EnterCar(carObject, carSeat);
        }
        return;
    }
}

void GameplayGamestate::SwitchPrevWeapon()
{
    int nextWeaponIndex = gGame.mPlayerState.mCharacter->mCurrentWeapon == 0 ? 
        (eWeapon_COUNT - 1) : (gGame.mPlayerState.mCharacter->mCurrentWeapon - 1);
    for (; nextWeaponIndex != gGame.mPlayerState.mCharacter->mCurrentWeapon; )
    {
        eWeaponID weaponID = (eWeaponID) nextWeaponIndex;
        if (!gGame.mPlayerState.mCharacter->mWeapons[weaponID].IsOutOfAmmunition())
        {
            gGame.mPlayerState.mCharacter->ChangeWeapon(weaponID);
            return;
        }
        nextWeaponIndex = nextWeaponIndex == 0 ? (eWeapon_COUNT - 1) : (nextWeaponIndex - 1);
    }
}

void GameplayGamestate::SwitchNextWeapon()
{
    int nextWeaponIndex = (gGame.mPlayerState.mCharacter->mCurrentWeapon + 1) % eWeapon_COUNT;
    for (; nextWeaponIndex != gGame.mPlayerState.mCharacter->mCurrentWeapon; )
    {
        eWeaponID weaponID = (eWeaponID) nextWeaponIndex;
        if (!gGame.mPlayerState.mCharacter->mWeapons[weaponID].IsOutOfAmmunition())
        {
            gGame.mPlayerState.mCharacter->ChangeWeapon(weaponID);
            return;
        }
        nextWeaponIndex = (nextWeaponIndex + 1) % eWeapon_COUNT;
    }
}

void GameplayGamestate::UpdateRespawnTimer()
{
    if (gGame.mPlayerState.mRespawnTime <= 0.0f)
        return;

    float deltaTime = gGame.mTimeMng.mGameFrameDelta;
    gGame.mPlayerState.mRespawnTime -= deltaTime;
    if (gGame.mPlayerState.mRespawnTime > 0.0f)
        return;

    RespawnPlayer();
}

void GameplayGamestate::RespawnPlayer()
{
    gGame.mPlayerState.SetWantedLevel(GAME_MAX_WANTED_LEVEL);//todo

    gGame.mPlayerState.mLastDistrictIndex = 0;
    gGame.mPlayerState.mRespawnTime = 0.0f;

    // todo : exit from car

    gGame.mPlayerState.mCharacter->HandleDespawn();
    gGame.mPlayerState.mCharacter->SetPosition(gGame.mPlayerState.mSpawnPosition);
    gGame.mPlayerState.mCharacter->HandleSpawn();

    gGame.mPlayerState.Cheat_GiveAllAmmunitions();
}