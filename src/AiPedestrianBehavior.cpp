#include "stdafx.h"
#include "AiPedestrianBehavior.h"
#include "Pedestrian.h"
#include "AiCharacterController.h"
#include "MapDirection2D.h"
#include "GtaOneGame.h"

//////////////////////////////////////////////////////////////////////////

AiPedestrianBehavior::AiActivity::AiActivity(AiPedestrianBehavior* aiBehavior)
    : mAiBehavior(aiBehavior)
{
    cxx_assert(mAiBehavior);
}

AiPedestrianBehavior::AiActivity::~AiActivity()
{
    // do nothing
}

void AiPedestrianBehavior::AiActivity::StartActivity()
{
    if (!IsStatusInProgress())
    {
        ClearChildActivity(); // just in case

        SetActivityStatus(eAiActivityStatus_InProgress);
        OnActivityStart();
    }
    else
    {
        cxx_assert(false);
    }
}

void AiPedestrianBehavior::AiActivity::UpdateActivity()
{
    if (IsStatusInProgress())
    {
        // update child activity first
        if (mChildActivity && mChildActivity->IsStatusInProgress())
        {
            mChildActivity->UpdateActivity();
        }
        OnActivityUpdate();

        if (!IsStatusInProgress())
        {
            ClearChildActivity();
        }
    }
}

void AiPedestrianBehavior::AiActivity::CancelActivity()
{
    if (IsStatusInProgress())
    {
        SetActivityStatus(eAiActivityStatus_Cancelled);
        OnActivityCancelled();

        ClearChildActivity();
    }
}

bool AiPedestrianBehavior::AiActivity::IsStatusInProgress() const
{
    return mActivityStatus == eAiActivityStatus_InProgress;
}

bool AiPedestrianBehavior::AiActivity::IsStatusSuccess() const
{
    return mActivityStatus == eAiActivityStatus_Success;
}

bool AiPedestrianBehavior::AiActivity::IsStatusFailed() const
{
    return mActivityStatus == eAiActivityStatus_Failed;
}

bool AiPedestrianBehavior::AiActivity::IsStatusCancelled() const
{
    return mActivityStatus == eAiActivityStatus_Cancelled;
}

void AiPedestrianBehavior::AiActivity::SetActivityStatus(eAiActivityStatus newStatus)
{
    mActivityStatus = newStatus;
}

void AiPedestrianBehavior::AiActivity::StartChildActivity(AiActivity* childActivity)
{
    cxx_assert(childActivity);
    cxx_assert(childActivity != this);

    if (childActivity)
    {
        cxx_assert(!childActivity->IsStatusInProgress());
        ClearChildActivity();

        mChildActivity = childActivity;
        mChildActivity->StartActivity();
    }
}

void AiPedestrianBehavior::AiActivity::ClearChildActivity()
{
    if (mChildActivity)
    {
        if (mChildActivity->IsStatusInProgress())
        {
            mChildActivity->CancelActivity();
        }
        mChildActivity = nullptr;
    }
}

bool AiPedestrianBehavior::AiActivity::IsChildActivityInProgress() const
{
    return mChildActivity && mChildActivity->IsStatusInProgress();
}

bool AiPedestrianBehavior::AiActivity::IsChildActivityInProgress(AiActivity* matchActivity) const
{
    if (matchActivity)
    {
        return (mChildActivity == matchActivity) && mChildActivity->IsStatusInProgress();
    }
    return false;
}

bool AiPedestrianBehavior::AiActivity::IsChildActivitySuccess() const
{
    return mChildActivity && mChildActivity->IsStatusSuccess();
}

bool AiPedestrianBehavior::AiActivity::IsChildActivityFailed() const
{
    return mChildActivity && mChildActivity->IsStatusFailed();
}

//////////////////////////////////////////////////////////////////////////

AiPedestrianBehavior::AiActiviy_Wander::AiActiviy_Wander(AiPedestrianBehavior* aiBehavior)
    : AiActivity(aiBehavior)
{
}

void AiPedestrianBehavior::AiActiviy_Wander::OnActivityUpdate()
{
    // shortcuts
    AiActivity_WalkToPoint* act_walk = &mAiBehavior->mActivity_WalkToPoint;

    // decide what to do
    if (!IsChildActivityInProgress(act_walk))
    {
        // try walk somewhere
        if (!ChooseDesiredPoint(eGroundType_Pawement))
        {
            SetActivityStatus(eAiActivityStatus_Failed);
            return;
        }
        const float ArriveDistance = gGame.mParams.mPedestrianBoundsSphereRadius * 2.0f;
        act_walk->SetRunning(false);
        act_walk->SetArriveDistance(ArriveDistance);
        StartChildActivity(act_walk);
    }
}

bool AiPedestrianBehavior::AiActiviy_Wander::ChooseDesiredPoint(eGroundType groundType)
{
    Pedestrian* character = mAiBehavior->GetCharacter();
    cxx_assert(character);

    cxx::angle_t currHeading = character->mTransform.mOrientation;

    // choose new block ir next order: forward, left, right, backward
    const eMapDirection2D currentMapDirection = GetStraightMapDirectionFromHeading(currHeading);
    const eMapDirection2D moveDirs[] =
    {
        currentMapDirection,
        GetStraightMapDirectionCCW(currentMapDirection),
        GetStraightMapDirectionCW(currentMapDirection),
        GetStraightMapDirectionOpposite(currentMapDirection)
    };

    glm::ivec3 logPosition = Convert::MetersToMapUnits(character->mTransform.mPosition);
    const MapBlockInfo* currentBlock = gGame.mMap.GetBlockInfo(logPosition.x, logPosition.z, logPosition.y);
    eMapDirection2D bestDirection = eMapDirection2D_None;
    for (eMapDirection2D directionCandidate: moveDirs)
    {
        const MapBlockInfo* neighbourBlock = gGame.mMap.GetNeighbourBlock(logPosition.x, logPosition.z, logPosition.y, directionCandidate);
        if ((neighbourBlock->mGroundType == groundType) || (neighbourBlock->mGroundType == currentBlock->mGroundType))
        {
            bestDirection = directionCandidate;
            break;
        }
    }

    // nothing found
    if (bestDirection == eMapDirection2D_None)
        return false;

    cxx_assert(IsMapDirectionStraight(bestDirection));

    switch (bestDirection)
    {
        case eMapDirection2D_N: logPosition.z -= 1; break;
        case eMapDirection2D_E: logPosition.x += 1; break;
        case eMapDirection2D_S: logPosition.z += 1; break;
        case eMapDirection2D_W: logPosition.x -= 1; break;
    }

    // choose random point within block
    float randomSubPosx = gGame.mRandom.generate_float(0.1f, 0.9f);
    float randomSubPosy = gGame.mRandom.generate_float(0.1f, 0.9f);
    mAiBehavior->mDesiredPoint.x = Convert::MapUnitsToMeters(logPosition.x * 1.0f) + Convert::MapUnitsToMeters(randomSubPosx);
    mAiBehavior->mDesiredPoint.y = Convert::MapUnitsToMeters(logPosition.z * 1.0f) + Convert::MapUnitsToMeters(randomSubPosy);
    return true;
}

//////////////////////////////////////////////////////////////////////////

AiPedestrianBehavior::AiActivity_Runaway::AiActivity_Runaway(AiPedestrianBehavior* aiBehavior)
    : AiActiviy_Wander(aiBehavior)
{
}

void AiPedestrianBehavior::AiActivity_Runaway::OnActivityUpdate()
{
    // shortcuts
    AiActivity_WalkToPoint* act_walk = &mAiBehavior->mActivity_WalkToPoint;

    // decide what to do
    if (!IsChildActivityInProgress(act_walk))
    {
        // try walk somewhere
        if (!ChooseRunawayPoint())
        {
            SetActivityStatus(eAiActivityStatus_Failed);
            return;
        }
        const float ArriveDistance = gGame.mParams.mPedestrianBoundsSphereRadius * 2.0f;
        act_walk->SetRunning(true);
        act_walk->SetArriveDistance(ArriveDistance);
        StartChildActivity(act_walk);
    }
}

bool AiPedestrianBehavior::AiActivity_Runaway::ChooseRunawayPoint()
{
    if (ChooseDesiredPoint(eGroundType_Pawement) || ChooseDesiredPoint(eGroundType_Field) ||
        ChooseDesiredPoint(eGroundType_Road) || ChooseDesiredPoint(eGroundType_Air))
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////

AiPedestrianBehavior::AiActivity_FollowLeader::AiActivity_FollowLeader(AiPedestrianBehavior* aiBehavior)
    : AiActivity(aiBehavior)
{
}

void AiPedestrianBehavior::AiActivity_FollowLeader::OnActivityStart()
{
    if (!CheckCanFollowTheLeader())
    {
        SetActivityStatus(eAiActivityStatus_Failed);
        return;
    }
    
    StartChildActivity(&mAiBehavior->mActivity_Wait);
}

void AiPedestrianBehavior::AiActivity_FollowLeader::OnActivityUpdate()
{
    // shortcuts
    AiActivity_WalkToPoint* act_walk = &mAiBehavior->mActivity_WalkToPoint;
    AiActivity_Wait* act_wait = &mAiBehavior->mActivity_Wait;

    if (!CheckCanFollowTheLeader())
    {
        SetActivityStatus(eAiActivityStatus_Failed);
        return;
    }

    const float IdleDistance = gGame.mParams.mPedestrianBoundsSphereRadius * 3.0f;
    const float ApproachDistance = gGame.mParams.mPedestrianBoundsSphereRadius * 1.5f;
    const float RushDistance = gGame.mParams.mPedestrianBoundsSphereRadius * 2.0f * 4.0f;
    const float StepDistance = gGame.mParams.mPedestrianBoundsSphereRadius * 2.0f * 3.0f;

    Pedestrian* currCharacter = mAiBehavior->GetCharacter();
    Pedestrian* leadCharacter = mAiBehavior->GetLeader();

    glm::vec2 charPosition = currCharacter->mTransform.GetPosition2();
    glm::vec2 leaderPosition = leadCharacter->mTransform.GetPosition2();
    float distanceToLeader2 = glm::distance2(charPosition, leaderPosition);

    if (IsChildActivityInProgress(act_walk))
    {
        if (distanceToLeader2 < (ApproachDistance * ApproachDistance))
        {
            act_wait->SetWaitSeconds(1.0f);
            StartChildActivity(act_wait);
            return;
        }
    }
    else
    {
        // check if should start walk
        if (distanceToLeader2 < (IdleDistance * IdleDistance))
        {
            // wait a bit
            if (!IsChildActivityInProgress(act_wait))
            {
                act_wait->SetWaitSeconds(1.0f);
                StartChildActivity(act_wait);
            }
            return;
        }

        mAiBehavior->mDesiredPoint = charPosition + glm::normalize(leaderPosition - charPosition) * StepDistance;
        act_walk->SetArriveDistance(ApproachDistance);
        StartChildActivity(act_walk);
    }

    // check if in rush
    if (IsChildActivityInProgress(act_walk))
    {
        float distanceToTarget2 = glm::distance2(mAiBehavior->mDesiredPoint, charPosition);
        bool inRush = leadCharacter->IsRunning() || (distanceToTarget2 > (RushDistance * RushDistance));
        act_walk->SetRunning(inRush);
    }
}

bool AiPedestrianBehavior::AiActivity_FollowLeader::CheckCanFollowTheLeader() const
{
    Pedestrian* character = mAiBehavior->GetCharacter();
    cxx_assert(character);

    Pedestrian* leaderCharacter = mAiBehavior->GetLeader();
    if ((leaderCharacter == nullptr) || leaderCharacter->IsDead() || leaderCharacter->IsDies())
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////

AiPedestrianBehavior::AiActivity_WalkToPoint::AiActivity_WalkToPoint(AiPedestrianBehavior* aiBehavior)
    : AiActivity(aiBehavior)
{
}

void AiPedestrianBehavior::AiActivity_WalkToPoint::SetRunning(bool setRunning)
{
    mSetRunning = setRunning;
}

void AiPedestrianBehavior::AiActivity_WalkToPoint::SetArriveDistance(float arriveDistance)
{
    mArriveDistance = arriveDistance;
}

void AiPedestrianBehavior::AiActivity_WalkToPoint::OnActivityStart()
{
    if (ContinueWalk())
    {
        SetActivityStatus(eAiActivityStatus_Success);
    }
}

void AiPedestrianBehavior::AiActivity_WalkToPoint::OnActivityUpdate()
{
    if (ContinueWalk())
    {
        SetActivityStatus(eAiActivityStatus_Success);
    }
}

void AiPedestrianBehavior::AiActivity_WalkToPoint::OnActivityCancelled()
{
    PedestrianCtlState& ctlState = mAiBehavior->mAiController->mCtlState;
    ctlState.Clear();
}

bool AiPedestrianBehavior::AiActivity_WalkToPoint::ContinueWalk()
{
    Pedestrian* character = mAiBehavior->GetCharacter();
    cxx_assert(character);

    PedestrianCtlState& ctlState = mAiBehavior->mAiController->mCtlState;
    ctlState.Clear();

    float tolerance2 = glm::pow(mArriveDistance * mArriveDistance, 2.0f);

    glm::vec2 currentPos2 = character->mTransform.GetPosition2();
    if (glm::distance2(currentPos2, mAiBehavior->mDesiredPoint) <= tolerance2)
        return true; // done

    // setup sign direction
    glm::vec2 toTarget = glm::normalize(mAiBehavior->mDesiredPoint - currentPos2);
    ctlState.mDesiredRotationAngle = cxx::angle_t::from_radians(::atan2f(toTarget.y, toTarget.x));
    ctlState.mRotateToDesiredAngle = true;
    ctlState.mWalkForward = true;
    ctlState.mRun = mSetRunning;

    return false;
}

//////////////////////////////////////////////////////////////////////////

AiPedestrianBehavior::AiActivity_Wait::AiActivity_Wait(AiPedestrianBehavior* aiBehavior)
    : AiActivity(aiBehavior)
{
}

void AiPedestrianBehavior::AiActivity_Wait::SetWaitSeconds(float waitSeconds)
{
    mWaitSeconds = waitSeconds;
}

void AiPedestrianBehavior::AiActivity_Wait::OnActivityStart()
{
    PedestrianCtlState& ctlState = mAiBehavior->mAiController->mCtlState;
    ctlState.Clear();

    mStartTime = gGame.mTimeMng.mGameTime;
}

void AiPedestrianBehavior::AiActivity_Wait::OnActivityUpdate()
{
    PedestrianCtlState& ctlState = mAiBehavior->mAiController->mCtlState;
    ctlState.Clear();

    // check wait time
    if (mWaitSeconds > 0.0f)
    {
        float currentTime = gGame.mTimeMng.mGameTime;
        if (currentTime > (mStartTime += mWaitSeconds))
        {
            mWaitSeconds = 0.0f;
            SetActivityStatus(eAiActivityStatus_Success);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

AiPedestrianBehavior::AiPedestrianBehavior(AiCharacterController* aiController, eAiPedestrianBehavior behaviorID)
    : mAiController(aiController)
    , mBehaviorID(behaviorID)
    , mDesiredPoint()
    , mActivity_Wander(this)
    , mActivity_Runaway(this)
    , mActivity_FollowLeader(this)
    , mActivity_WalkToPoint(this)
    , mActivity_Wait(this)
{
    cxx_assert(mAiController);
}

AiPedestrianBehavior::~AiPedestrianBehavior()
{
}

void AiPedestrianBehavior::ActivateBehavior()
{
    cxx_assert(mAiController);

    OnActivateBehavior();

    cxx_assert(mCurrentActivity == nullptr);
    mDesiredActivity = nullptr;
}

void AiPedestrianBehavior::ShutdownBehavior()
{
    if (mCurrentActivity && mCurrentActivity->IsStatusInProgress())
    {
        mCurrentActivity->CancelActivity();
    }

    OnShutdownBehavior();

    // reset state
    mCurrentActivity = nullptr;
    mDesiredActivity = nullptr;
}

void AiPedestrianBehavior::UpdateBehavior()
{
    ScanForThreats();
    ScanForLeader();

    ChooseDesiredActivity();

    if (mDesiredActivity && (mCurrentActivity != mDesiredActivity))
    {
        if (mCurrentActivity && mCurrentActivity->IsStatusInProgress())
        {
            mCurrentActivity->CancelActivity();
            mCurrentActivity = nullptr;
        }
        mCurrentActivity = mDesiredActivity;
        mDesiredActivity = nullptr;
        mCurrentActivity->StartActivity();
    }
    else if (mCurrentActivity)
    {
        mCurrentActivity->UpdateActivity();
    }

    OnUpdateBehavior();
}

void AiPedestrianBehavior::ChangeMemoryBits(AiBehaviorMemoryBits enableBits, AiBehaviorMemoryBits disableBits)
{
    mMemoryBits = (mMemoryBits | enableBits) & ~disableBits;
}

bool AiPedestrianBehavior::CheckMemoryBits(AiBehaviorMemoryBits memoryBits) const
{
    return (mMemoryBits & memoryBits) > 0;
}

bool AiPedestrianBehavior::CheckMemoryBitsAll(AiBehaviorMemoryBits memoryBits) const
{
    return (mMemoryBits & memoryBits) == memoryBits;
}

void AiPedestrianBehavior::ChangeBehaviorBits(AiBehaviorBits enableBits, AiBehaviorBits disableBits)
{
    mBehaviorBits = (mBehaviorBits | enableBits) & ~disableBits;
}

bool AiPedestrianBehavior::CheckBehaviorBits(AiBehaviorBits bits) const
{
    return (mBehaviorBits & bits) > 0;
}

bool AiPedestrianBehavior::CheckBehaviorBitsAll(AiBehaviorBits bits) const
{
    return (mBehaviorBits & bits) == bits;
}

Pedestrian* AiPedestrianBehavior::GetCharacter() const
{
    if (mAiController)
        return mAiController->mCharacter;

    return nullptr;
}

Pedestrian* AiPedestrianBehavior::GetLeader() const
{
    return mLeader;
}

void AiPedestrianBehavior::SetLeader(Pedestrian* pedestrian)
{
    // sanity checks
    if ((pedestrian == nullptr) || (pedestrian == GetCharacter()))
    {
        cxx_assert(false);
        return;
    }

    mLeader = pedestrian;
}

void AiPedestrianBehavior::ChooseDesiredActivity()
{
    if (CheckMemoryBits(AiBehaviorMemoryBits_InPanic))
    {
        mDesiredActivity = &mActivity_Runaway;
        return;
    }

    Pedestrian* character = GetCharacter();
    cxx_assert(character);
    if (character->HasFear_Explosions() && CheckMemoryBits(AiBehaviorMemoryBits_HearExplosion))
    {
        ChangeMemoryBits(AiBehaviorMemoryBits_InPanic, AiBehaviorMemoryBits_None);
        mDesiredActivity = &mActivity_Runaway;
        return;
    }

    if (character->HasFear_GunShots() && CheckMemoryBits(AiBehaviorMemoryBits_HearGunShots))
    {
        ChangeMemoryBits(AiBehaviorMemoryBits_InPanic, AiBehaviorMemoryBits_None);
        mDesiredActivity = &mActivity_Runaway;
        return;
    }

    if (mLeader)
    {
        mDesiredActivity = &mActivity_FollowLeader;
        return;
    }

    mDesiredActivity = &mActivity_Wander;
}

void AiPedestrianBehavior::ScanForThreats()
{
    // reset previous state
    ChangeMemoryBits(AiBehaviorMemoryBits_None, AiBehaviorMemoryBits_HearExplosion | AiBehaviorMemoryBits_HearGunShots);

    Pedestrian* character = GetCharacter();
    if (CheckMemoryBits(AiBehaviorMemoryBits_InPanic) || character->IsDead() || character->IsDies())
        return;

    AiBehaviorMemoryBits enableMemoryBits = AiBehaviorMemoryBits_None;
    BroadcastEvent eventData;
    glm::vec2 characterPos2 = character->mTransform.GetPosition2();
    // check gunshots
    {
        for (BroadcastEventsIterator eventsIter;;)
        {
            if (!eventsIter.NextEventInDistance(eBroadcastEvent_GunShot, characterPos2, gGame.mParams.mAiReactOnGunshotsDistance, eventData))
                break;

            if (eventData.mCharacter == character)// hear own gunshots
                continue;

            enableMemoryBits = (enableMemoryBits | AiBehaviorMemoryBits_HearGunShots);
            break;
        }
    }

    // check explosions
    {
        for (BroadcastEventsIterator eventsIter;;)
        {
            if (!eventsIter.NextEventInDistance(eBroadcastEvent_Explosion, characterPos2, gGame.mParams.mAiReactOnExplosionsDistance, eventData))
                break;

            enableMemoryBits = (enableMemoryBits | AiBehaviorMemoryBits_HearExplosion);
            break;
        }
    }

    if (enableMemoryBits)
    {
        ChangeMemoryBits(enableMemoryBits, AiBehaviorMemoryBits_None);
    }
}

void AiPedestrianBehavior::ScanForLeader()
{
    if (mLeader)
    {
        // check if leader still alive
        if (!mLeader->IsDead() && !mLeader->IsDies())
            return;

        mLeader.reset();
    }

    Pedestrian* character = GetCharacter();
    if (CheckMemoryBits(AiBehaviorMemoryBits_InPanic) || character->IsDead() || character->IsDies())
        return;

    if (!CheckBehaviorBits(AiBehaviorBits_PlayerFollower))
        return;

    glm::vec2 currPosition2 = character->mTransform.GetPosition2();

    // try follow human character Nearby
    float maxSignDistance = Convert::MapUnitsToMeters(0.5f);
    float bestDistance2 = glm::pow(maxSignDistance, 2.0f);
    if (Pedestrian* playerCharacter = gGame.mPlayerState.mCharacter)
    {
        if ((playerCharacter == nullptr) || (character == playerCharacter))
            return;

        if (!playerCharacter->IsStanding())
            return;

        float currDistance2 = glm::distance2(playerCharacter->mTransform.GetPosition2(), currPosition2);
        if (currDistance2 > bestDistance2)
            return;

        mLeader = playerCharacter;
    }
}
