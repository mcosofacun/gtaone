#include "stdafx.h"
#include "AiCharacterController.h"
#include "AiGangBehavior.h"

AiCharacterController::~AiCharacterController()
{
    SetCharacter(nullptr);
    SafeDelete(mAiBehavior);
}

bool AiCharacterController::IsControllerActive() const
{
    return mCharacter != nullptr;
}

void AiCharacterController::SetCharacter(Pedestrian* character)
{
    if (mCharacter == character)
        return;

    mCtlState.Clear();
    // destroy old ai behavior
    if (mAiBehavior)
    {
        mAiBehavior->ShutdownBehavior();
        SafeDelete(mAiBehavior);
    }

    if (mCharacter)
    {
        cxx_assert(mCharacter->mAiController == this);
        mCharacter->mAiController = nullptr;
        cxx_assert(mCharacter->mCtlState == &mCtlState);
        mCharacter->mCtlState = nullptr;
    }

    mCharacter = character;

    if (mCharacter)
    {
        cxx_assert(mCharacter->mAiController == nullptr);
        mCharacter->mAiController = this;
        mCharacter->mCtlState = &mCtlState;

        // create new ai behavior
        if ((mCharacter->mPedestrianType == ePedestrianType_Gang) || 
            (mCharacter->mPedestrianType == ePedestrianType_GangLeader))
        {
            mAiBehavior = new AiGangBehavior(this);
        }
        if (mAiBehavior == nullptr)
        {
            mAiBehavior = new AiPedestrianBehavior(this, eAiPedestrianBehavior_Civilian);
        }
        mAiBehavior->ActivateBehavior();
    }
}

void AiCharacterController::UpdateFrame()
{
    if (mAiBehavior)
    {
        mAiBehavior->UpdateBehavior();
    }

    // self detach
    if (mCharacter && mCharacter->IsDead())
    {
        SetCharacter(nullptr);
    }
}

void AiCharacterController::DebugDraw(DebugRenderer& debugRender)
{
}

void AiCharacterController::FollowPedestrian(Pedestrian* pedestrian)
{
    cxx_assert(pedestrian);
    if (mAiBehavior)
    {
        mAiBehavior->SetLeader(pedestrian);
    }
}