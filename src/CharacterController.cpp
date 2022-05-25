#include "stdafx.h"
#include "CharacterController.h"
#include "Pedestrian.h"
#include "Vehicle.h"
#include "DebugRenderer.h"

CharacterController::CharacterController(eCharacterControllerType controllerType)
    : mControllerType(controllerType)
{
}

CharacterController::~CharacterController()
{
    SetCharacter(nullptr);
}

void CharacterController::SetCharacter(Pedestrian* character)
{
    if (mCharacter == character)
        return;

    if (mCharacter)
    {
        cxx_assert(mCharacter->mController == this);
        mCharacter->mController = nullptr;
    }
    mCharacter = character;
    if (mCharacter)
    {
        cxx_assert(mCharacter->mController == nullptr);
        mCharacter->mController = this;
    }
    mCtlState.Clear();
}

bool CharacterController::IsControllerActive() const
{
    return mCharacter != nullptr;
}

bool CharacterController::IsControllerTypeAi() const 
{ 
    return mControllerType == eCharacterControllerType_Ai; 
}

bool CharacterController::IsControllerTypeHuman() const 
{ 
    return mControllerType == eCharacterControllerType_Human; 
}

void CharacterController::StartController(Pedestrian* character)
{
    cxx_assert(character);
    if ((character == nullptr) || (character == mCharacter))
        return;

    if (IsControllerActive())
    {
        cxx_assert(false);
        StopController();
    }

    SetCharacter(character);
    OnControllerStart();
}

void CharacterController::StopController()
{
    if (mCharacter == nullptr)
        return;

    OnControllerStop();
    SetCharacter(nullptr);
}

void CharacterController::OnControllerStart()
{
    // do nothing
}

void CharacterController::OnControllerStop()
{
    // do nothing
}