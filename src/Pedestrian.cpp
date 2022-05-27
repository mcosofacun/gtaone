#include "stdafx.h"
#include "Pedestrian.h"
#include "SpriteBatch.h"
#include "PedestrianStates.h"
#include "Vehicle.h"
#include "GtaOneGame.h"
#include "Collision.h"
#include "GameObjectHelpers.h"
#include "AiCharacterController.h"

Pedestrian::Pedestrian(GameObjectID id, ePedestrianType typeIdentifier) 
    : GameObject(eGameObjectClass_Pedestrian, id)
    , mCurrentAnimID(ePedestrianAnim_Null)
    , mRemapIndex(NO_REMAP)
    , mStatesManager(this)
    , mPedestrianType(typeIdentifier)
{
    mCurrentAnimState.SetListener(this);
}

Pedestrian::~Pedestrian()
{
    if (mAiController)
    {
        mAiController->SetCharacter(nullptr);
    }
}

void Pedestrian::HandleSpawn()
{
    int remapIndex = gGame.mStyleData.GetPedestrianRandomRemap(mPedestrianType);
    SetRemap(remapIndex);

    mCurrentStateTime = 0.0f;

    for (int currWeapon = 0; currWeapon < eWeapon_COUNT; ++currWeapon)
    {
        mWeapons[currWeapon].Setup((eWeaponID) currWeapon, 0);
    }

    mBurnStartTime = 0.0f;
    mStandingOnRailwaysTimer = 0.0f;

    mCurrentWeapon = eWeapon_Fists;
    mChangeWeapon = eWeapon_Fists;
    
    // create physical body
    CollisionShape collisionShape;
    collisionShape.SetAsCircle(gGame.mParams.mPedestrianBoundsSphereRadius);
    PhysicsMaterial physicsMaterial;
    physicsMaterial.mDensity = 0.3f; // todo: magic numbers
    PhysicsBody* physicsBody = gGame.mPhysicsMng.CreateBody(this, collisionShape, physicsMaterial, 
        CollisionGroup_Pedestrian, CollisionGroup_All, 
        ColliderFlags_None, 
        PhysicsBodyFlags_FixRotation);
    cxx_assert(physicsBody);
    SetPhysics(physicsBody);

    mDeathReason = ePedestrianDeathReason_null;
    mCurrentAnimID = ePedestrianAnim_Null;

    PedestrianStateEvent evData { ePedestrianStateEvent_Spawn };
    mStatesManager.ChangeState(ePedestrianState_StandingStill, evData); // force idle state
}

void Pedestrian::HandleDespawn()
{
    SetBurnEffectActive(false);
    SetCarExited();

    // reset weapon
    ClearAmmunition();

    // force stop sounds
    StopGameObjectSounds();

    GameObject::HandleDespawn();
}

bool Pedestrian::ShouldCollide(GameObject* otherObject) const
{
    return false;
}

void Pedestrian::HandleFallingStarts()
{
    cxx_assert(mPhysicsBody);

    glm::vec2 velocity = mPhysicsBody->GetLinearVelocity();
    if (glm::length2(velocity) > 0.0f)
    {
        velocity = glm::normalize(velocity);
        cxx_assert(!glm::isnan(velocity.x) && !glm::isnan(velocity.y));
    }
    mPhysicsBody->ClearForces();
    mPhysicsBody->SetLinearVelocity(velocity);

    // notify
    PedestrianStateEvent evData { ePedestrianStateEvent_FallFromHeightStart };
    mStatesManager.ProcessEvent(evData);
}

void Pedestrian::HandleFallsOnGround(float fallDistance)
{
    cxx_assert(mPhysicsBody);

    // notify
    PedestrianStateEvent evData { ePedestrianStateEvent_FallFromHeightEnd };
    mStatesManager.ProcessEvent(evData);

    // damage
    if (fallDistance > 0.0f)
    {
        DamageInfo damageInfo;
        damageInfo.SetFallDamage(fallDistance);
        ReceiveDamage(damageInfo);
    }
}

void Pedestrian::HandleFallsOnWater(float fallDistance)
{
    cxx_assert(mPhysicsBody);

    PedestrianStateEvent evData { ePedestrianStateEvent_WaterContact };
    mStatesManager.ProcessEvent(evData);
}

void Pedestrian::UpdateFrame()
{
    float deltaTime = gGame.mTimeMng.mGameFrameDelta;
    if (mCurrentAnimState.UpdateFrame(deltaTime))
    {
        SetSprite(mCurrentAnimState.GetSpriteIndex());
    }

    // update weapons state
    for (Weapon& currWeapon: mWeapons)
    {
        currWeapon.UpdateFrame();
    }

    // change weapon
    if (mCurrentWeapon != mChangeWeapon)
    {
        if (GetWeapon().IsOutOfAmmunition() || GetWeapon().IsReadyToFire())
        {
            mCurrentWeapon = mChangeWeapon;
            // notify current state
            PedestrianStateEvent evData { ePedestrianStateEvent_WeaponChange };
            mStatesManager.ProcessEvent(evData);
        }
    }

    mCurrentStateTime += deltaTime;

    UpdateDamageFromRailways();

    // update current state logic
    mStatesManager.ProcessFrame();
    
    UpdateBurnEffect();
    UpdateDrawOrder();
}

void Pedestrian::SimulationStep()
{
    mContactingOtherPeds = false;
    mContactingCars = false;

    mStatesManager.ProcessSimulationFrame();

    Vehicle* hitCarObject = nullptr;
    float maxCarSpeed = 0.0f;

    // inspect current contacts
    for (const Contact& currContact: mObjectsContacts)
    {
        if (Pedestrian* otherPedestrian = ToPedestrian(currContact.mThatObject))
        {
            mContactingOtherPeds = otherPedestrian->IsOnTheGround() && 
                !otherPedestrian->IsDead() && 
                !otherPedestrian->IsStunned() && 
                !otherPedestrian->IsDies(); // slowdown
        }

        // check car hit
        if (Vehicle* contactCar = ToVehicle(currContact.mThatObject))
        {
            mContactingCars = true;
            if (currContact.mContactPoints->mSeparation > 0.0f)
                continue;

            float penetration = fabsf(currContact.mContactPoints->mSeparation);
            if (penetration < gGame.mParams.mPedestrianBoundsSphereRadius * 0.9f) // todo: magic numbers
                continue;

            float carSpeed = fabsf(contactCar->GetCurrentSpeed());
            if (carSpeed < maxCarSpeed)
                continue;

            hitCarObject = contactCar;
            maxCarSpeed = carSpeed;
        }
    }

    if (hitCarObject)
    {
        DamageInfo damageInfo;
        damageInfo.SetCarHitDamage(hitCarObject);
        ReceiveDamage(damageInfo);
    }

    UpdateRotation();
    UpdateLocomotion();
}

void Pedestrian::UpdateDrawOrder()
{
    eSpriteDrawOrder newDrawOrder = eSpriteDrawOrder_Pedestrian;

    if (mCurrentCar)
    {
        newDrawOrder = eSpriteDrawOrder_CarPassenger;
        if (!mCurrentCar->HasHardTop())
        {
            newDrawOrder = eSpriteDrawOrder_ConvetibleCarPassenger;
        }
    }
    else
    {
        if (GetCurrentStateID() == ePedestrianState_SlideOnCar)
        {
            newDrawOrder = eSpriteDrawOrder_JumpingPedestrian;
        }

        if (IsStunned() || IsDead())
        {
            newDrawOrder = eSpriteDrawOrder_Corpse;
        }
    }

    if (mFireEffect)
    {
        mFireEffect->SetDrawOrder(newDrawOrder);
    }

    mDrawSprite.mDrawOrder = newDrawOrder;
}

void Pedestrian::UpdateLocomotion()
{
    cxx_assert(mPhysicsBody);

    float walkingSpeed = 0.0f;

    bool inverseDirection = false;
    bool isSlideOverCar = (ePedestrianState_SlideOnCar == GetCurrentStateID());

    if (isSlideOverCar)
    {
        walkingSpeed = gGame.mParams.mPedestrianSlideOnCarSpeed;
    }
    else if (IsIdle())
    {
        const PedestrianCtlState& ctlState = GetCtlState();
        // generic case
        if (ctlState.mWalkForward || ctlState.mWalkBackward || ctlState.mRun)
        {
            if (ctlState.mRun && CanRun())
            {
                walkingSpeed = gGame.mParams.mPedestrianRunSpeed;
            }
            else
            {
                walkingSpeed = gGame.mParams.mPedestrianWalkSpeed;
                inverseDirection = ctlState.mWalkBackward;
            }
        }
    }

    glm::vec2 desiredVelocity (0.0f, 0.0f);
    if (walkingSpeed)
    {
        glm::vec2 moveDirection = mPhysicsBody->GetSignVector() * (inverseDirection ? -1.0f : 1.0f);
        desiredVelocity = moveDirection * walkingSpeed;
        
        if (!isSlideOverCar)
        {
            // prevent walking through cars
            for (const Contact& currContact: mObjectsContacts)
            {
                if (!currContact.mThatObject->IsVehicleClass())
                    continue;

                const ContactPoint& cp = currContact.mContactPoints[0];
                if (glm::dot(-cp.mNormal, moveDirection) > 0.0f)
                {
                    desiredVelocity = glm::vec2(0.0f, 0.0f);
                }
            }
        }
    }

    mPhysicsBody->SetLinearVelocity(desiredVelocity);
}

void Pedestrian::UpdateRotation()
{
    cxx::angle_t angularVelocity;

    bool isSlideOverCar = (ePedestrianState_SlideOnCar == GetCurrentStateID());
    if (isSlideOverCar || IsIdle())
    {
        const PedestrianCtlState& ctlState = GetCtlState();
        if (ctlState.mTurnLeft || ctlState.mTurnRight || ctlState.mRotateToDesiredAngle)
        {
            if (ctlState.mRotateToDesiredAngle)
            {
                static const float instantTurnSpeed = 360.0f * 10.0f;

                float turnSpeed = instantTurnSpeed;

                const float angleDelta = cxx::wrap_angle_neg_180((ctlState.mDesiredRotationAngle - mPhysicsBody->GetOrientation()).to_degrees());
                const float desiredSpeedPerTurn = gGame.mPhysicsMng.GetSimulationStepTime() * turnSpeed;
                if (angleDelta < desiredSpeedPerTurn)
                {
                    turnSpeed = angleDelta / gGame.mPhysicsMng.GetSimulationStepTime();
                }

                if (::fabs(turnSpeed) > 1.0f)
                {
                    angularVelocity.set_angle(turnSpeed, cxx::angle_t::units::degrees);
                }
            }
            else
            {
                float turnSpeed = isSlideOverCar ? 
                    gGame.mParams.mPedestrianTurnSpeedSlideOnCar : 
                    gGame.mParams.mPedestrianTurnSpeed;

                angularVelocity = cxx::angle_t::from_degrees(turnSpeed * (ctlState.mTurnLeft ? -1.0f : 1.0f));
            }
        }
    }
        
    mPhysicsBody->SetAngularVelocity(angularVelocity);
}

void Pedestrian::DebugDraw(DebugRenderer& debugRender)
{
    if (mCurrentCar == nullptr)
    {
        glm::vec3 position = mPhysicsBody->GetPosition();

        WeaponInfo& meleeWeapon = gGame.mStyleData.mWeaponTypes[eWeaponFireType_Melee];

        glm::vec2 signVector = mPhysicsBody->GetSignVector();
        debugRender.DrawLine(position, position + 
            glm::vec3(signVector.x * meleeWeapon.mBaseHitRange, 0.0f, signVector.y * meleeWeapon.mBaseHitRange), Color32_White, false);

        cxx::bounding_sphere_t bsphere (mPhysicsBody->GetPosition(), gGame.mParams.mPedestrianBoundsSphereRadius);
        debugRender.DrawSphere(bsphere, Color32_Orange, false);
    }
}

Weapon& Pedestrian::GetWeapon()
{
    return mWeapons[mCurrentWeapon];
}

Weapon& Pedestrian::GetWeapon(eWeaponID weapon)
{
    cxx_assert(weapon < eWeapon_COUNT);
    return mWeapons[weapon];
}

void Pedestrian::ChangeWeapon(eWeaponID weapon)
{
    mChangeWeapon = weapon;
}

void Pedestrian::EnterCar(Vehicle* targetCar, eCarSeat targetSeat)
{
    cxx_assert(targetSeat != eCarSeat_Any);
    cxx_assert(targetCar);

    if (targetCar->IsWrecked())
        return;

    float currentSpeed = targetCar->GetCurrentSpeed();
    if (currentSpeed >= gGame.mParams.mCarSpeedPassengerCanEnter)
        return;

    if (IsBurn()) // cannot enter vehicle while burn
        return;

    if (IsIdle())
    {
        PedestrianStateEvent evData { ePedestrianStateEvent_EnterCar };
        evData.mTargetCar = targetCar;
        evData.mTargetSeat = targetSeat;
        mStatesManager.ChangeState(ePedestrianState_EnteringCar, evData);
    }
}

void Pedestrian::LeaveCar()
{
    if (IsCarPassenger())
    {
        float currentSpeed = mCurrentCar->GetCurrentSpeed();
        if (currentSpeed >= gGame.mParams.mCarSpeedPassengerCanEnter)
            return;

        PedestrianStateEvent evData { ePedestrianStateEvent_ExitCar };
        mStatesManager.ChangeState(ePedestrianState_ExitingCar, evData);
    }
}

bool Pedestrian::ReceiveDamage(const DamageInfo& damageInfo)
{
    PedestrianStateEvent evData { ePedestrianStateEvent_ReceiveDamage };
    evData.mDamageInfo = damageInfo;
    return mStatesManager.ProcessEvent(evData);
}

void Pedestrian::SetAnimation(ePedestrianAnimID animation, eSpriteAnimLoop loopMode)
{
    if (mCurrentAnimID == animation)
    {
        if (mCurrentAnimState.IsActive())
        {
            mCurrentAnimState.SetCurrentLoop(loopMode);
            return;
        }
    }
    else
    {
        mCurrentAnimState.Clear();
        if (!gGame.mStyleData.GetPedestrianAnimation(animation, mCurrentAnimState.mAnimDesc))
        {
            cxx_assert(false);
        }
        mCurrentAnimID = animation;
    }

    mCurrentAnimState.PlayAnimation(loopMode);
    SetSprite(mCurrentAnimState.GetSpriteIndex());
}

ePedestrianState Pedestrian::GetCurrentStateID() const
{
    return mStatesManager.GetCurrentStateID();
}

bool Pedestrian::IsCarPassenger() const
{
    ePedestrianState currState = GetCurrentStateID();
    if (currState == ePedestrianState_DrivingCar)
    {
        return true; // includes driver
    }
    return false;
}

bool Pedestrian::IsCarDriver() const
{
    ePedestrianState currState = GetCurrentStateID();
    if (currState == ePedestrianState_DrivingCar)
    {
        return mCurrentSeat == eCarSeat_Driver;
    }
    return false;
}

bool Pedestrian::IsEnteringOrExitingCar() const
{
    ePedestrianState currState = GetCurrentStateID();
    if (currState == ePedestrianState_EnteringCar || currState == ePedestrianState_ExitingCar)
    {
        return true;
    }
    return false;
}

bool Pedestrian::IsIdle() const
{
    return IsStanding() || IsWalking();
}

bool Pedestrian::IsStanding() const
{
    ePedestrianState currState = GetCurrentStateID();
    return (currState == ePedestrianState_StandingStill);
}

bool Pedestrian::IsWalking() const
{
    ePedestrianState currState = GetCurrentStateID();
    return (currState == ePedestrianState_Walks || currState == ePedestrianState_Runs);
}

bool Pedestrian::IsRunning() const
{
    ePedestrianState currState = GetCurrentStateID();
    return (currState == ePedestrianState_Runs);
}

bool Pedestrian::IsStunned() const
{
    ePedestrianState currState = GetCurrentStateID();
    return currState == ePedestrianState_Stunned;
}

bool Pedestrian::IsDead() const
{
    ePedestrianState currState = GetCurrentStateID();
    return currState == ePedestrianState_Dead;
}

bool Pedestrian::IsDies() const
{
    ePedestrianState currState = GetCurrentStateID();
    return (currState == ePedestrianState_Electrocuted);
}

void Pedestrian::SetCarEntered(Vehicle* targetCar, eCarSeat targetSeat)
{
    cxx_assert(mCurrentCar == nullptr);
    cxx_assert(targetCar && targetSeat != eCarSeat_Any);

    mCurrentCar = targetCar;
    mCurrentSeat = targetSeat;
    mPhysicsBody->ClearForces();
    mCurrentCar->RegisterPassenger(this, mCurrentSeat);
}

void Pedestrian::SetCarExited()
{
    if (mCurrentCar == nullptr)
        return;

    mCurrentCar->UnregisterPassenger(this);
    mCurrentCar = nullptr;
}

void Pedestrian::SetDead(ePedestrianDeathReason deathReason)
{
    cxx_assert(mDeathReason == ePedestrianDeathReason_null);
    cxx_assert(deathReason != ePedestrianDeathReason_null);
    mDeathReason = deathReason;
}

void Pedestrian::DieFromDamage(const DamageInfo& damageInfo)
{
    PedestrianStateEvent evData { ePedestrianStateEvent_Die };
    evData.mDeathReason = ePedestrianDeathReason_Unknown;
    evData.mDamageInfo = damageInfo;

    switch (damageInfo.mDamageCause)
    {
        case eDamageCause_Gravity: 
            evData.mDeathReason = ePedestrianDeathReason_FallFromHeight;
        break;
        case eDamageCause_Electricity: 
            evData.mDeathReason = ePedestrianDeathReason_Electrocuted;
        break;
        case eDamageCause_Flame: 
            evData.mDeathReason = ePedestrianDeathReason_Fried;
        break;
        case eDamageCause_Water: 
            evData.mDeathReason = ePedestrianDeathReason_Drowned;
        break;
        case eDamageCause_CarHit: 
            evData.mDeathReason = ePedestrianDeathReason_Smashed;
        break;
        case eDamageCause_Explosion: 
            evData.mDeathReason = ePedestrianDeathReason_BlownUp;
        break;
        case eDamageCause_Bullet: 
            evData.mDeathReason = ePedestrianDeathReason_Shot;
        break;
        case eDamageCause_Punch: 
            evData.mDeathReason = ePedestrianDeathReason_Beaten;
        break;
    }

    mStatesManager.ChangeState(ePedestrianState_Dead, evData);
}

void Pedestrian::SetBurnEffectActive(bool activate)
{
    if (activate == IsBurn())
        return;

    if (activate)
    {
        cxx_assert(mFireEffect == nullptr);
        GameObjectInfo& objectInfo = gGame.mStyleData.mObjects[GameObjectType_LFire];

        mFireEffect = gGame.mObjectsMng.CreateDecoration(mTransform.mPosition, mTransform.mOrientation, &objectInfo);
        cxx_assert(mFireEffect);
        if (mFireEffect)
        {
            mFireEffect->SetLifeDuration(0);
            AttachObject(mFireEffect);
        }
        mBurnStartTime = gGame.mTimeMng.mGameTime;
    }
    else
    {
        cxx_assert(mFireEffect);
        if (mFireEffect)
        {   
            DetachObject(mFireEffect);
        }
        mFireEffect->MarkForDeletion();
        mFireEffect = nullptr;
    }
}

void Pedestrian::UpdateBurnEffect()
{
    if (mFireEffect == nullptr)
        return;

    if (IsDead() || !IsOnTheGround())
        return;

    if (gGame.mTimeMng.mGameTime > (mBurnStartTime + gGame.mParams.mPedestrianBurnDuration))
    {
        DamageInfo damageInfo;
        damageInfo.SetFireDamage(nullptr);
        DieFromDamage(damageInfo);
        return;
    }
}

bool Pedestrian::IsBurn() const
{
    return (mFireEffect != nullptr);
}

bool Pedestrian::IsOnTheGround() const
{
    return !mPhysicsBody->mFalling;
}

bool Pedestrian::IsInWater() const
{
    return mPhysicsBody->mWaterContact;
}

void Pedestrian::PutInsideCar(Vehicle* car, eCarSeat carSeat)
{
    cxx_assert(car);
    cxx_assert(carSeat < eCarSeat_Any);

    if ((car == nullptr) || (mCurrentCar == car))
        return;

    if (mCurrentCar)
    {
        SetCarExited();
    }

    SetCarEntered(car, carSeat);

    if (!IsDead())
    {
        PedestrianStateEvent evData { ePedestrianStateEvent_None };
        mStatesManager.ChangeState(ePedestrianState_DrivingCar, evData);
    }
}

void Pedestrian::PutOnFoot()
{
    if (mCurrentCar == nullptr)
        return;

    if (!IsDead())
    {
        PedestrianStateEvent evData { ePedestrianStateEvent_None };
        mStatesManager.ChangeState(ePedestrianState_StandingStill, evData);
    }
    SetCarExited();
}

bool Pedestrian::IsPlayerCharacter() const
{
    return gGame.mPlayerState.mCharacter == this;
}

bool Pedestrian::IsAiCharacter() const
{
    return (mAiController != nullptr);
}

bool Pedestrian::HasFear_Players() const
{
    bool hasFear = false;
    if (IsAiCharacter())
    {
        hasFear = mAiController->mAiBehavior && mAiController->mAiBehavior->CheckBehaviorBits(AiBehaviorBits_Fear_Player);
    }
    return hasFear;
}

bool Pedestrian::HasFear_Police() const
{
    bool hasFear = false;
    if (IsAiCharacter())
    {
        hasFear = mAiController->mAiBehavior && mAiController->mAiBehavior->CheckBehaviorBits(AiBehaviorBits_Fear_Police);
    }
    return hasFear;
}

bool Pedestrian::HasFear_GunShots() const
{
    bool hasFear = false;
    if (IsAiCharacter())
    {
        hasFear = mAiController->mAiBehavior && mAiController->mAiBehavior->CheckBehaviorBits(AiBehaviorBits_Fear_GunShots);
    }
    return hasFear;
}

bool Pedestrian::HasFear_Explosions() const
{
    bool hasFear = false;
    if (IsAiCharacter())
    {
        hasFear = mAiController->mAiBehavior && mAiController->mAiBehavior->CheckBehaviorBits(AiBehaviorBits_Fear_Explosions);
    }
    return hasFear;
}

bool Pedestrian::HasFear_DeadPeds() const
{
    bool hasFear = false;
    if (IsAiCharacter())
    {
        hasFear = mAiController->mAiBehavior && mAiController->mAiBehavior->CheckBehaviorBits(AiBehaviorBits_Fear_DeadPeds);
    }
    return hasFear;
}

void Pedestrian::IncArmorMax()
{
    mArmorHitPoints = gGame.mParams.mPedestrianMaxArmor;
}

void Pedestrian::IncArmor()
{
    mArmorHitPoints = std::min(mArmorHitPoints + 1, gGame.mParams.mPedestrianMaxArmor);
}

void Pedestrian::DecArmor()
{
    mArmorHitPoints = std::max(mArmorHitPoints - 1, 0);
}

void Pedestrian::ClearAmmunition()
{
    for (int icurrent = 0; icurrent < eWeapon_COUNT; ++icurrent)
    {
        mWeapons[icurrent].SetAmmunition(0);
    }
    mArmorHitPoints = 0;
}

void Pedestrian::UpdateDamageFromRailways()
{
    if (IsDead() || IsDies())
        return;

    if (!IsOnTheGround())
    {
        mStandingOnRailwaysTimer = 0.0f;
        return;
    }

    glm::ivec3 logPosition = Convert::MetersToMapUnits(mTransform.mPosition);

    const MapBlockInfo* blockInfo = gGame.mMap.GetBlockInfo(logPosition.x, logPosition.z, logPosition.y);
    if ((blockInfo->mGroundType == eGroundType_Field) && blockInfo->mIsRailway)
    {
        mStandingOnRailwaysTimer += gGame.mTimeMng.mGameFrameDelta;
        if (mStandingOnRailwaysTimer > gGame.mParams.mGameRailwaysDamageDelay)
        {
            DamageInfo damageInfo;
            damageInfo.SetElectricityDamage();
            ReceiveDamage(damageInfo);
        }
    }
    else
    {
        mStandingOnRailwaysTimer = 0.0f;
    }
}

bool Pedestrian::OnAnimFrameAction(SpriteAnimation* animation, int frameIndex, eSpriteAnimAction actionID)
{
    cxx_assert(&mCurrentAnimState == animation);

    ePedestrianState stateID = GetCurrentStateID();
    if ((actionID == eSpriteAnimAction_Footstep) && IsPlayerCharacter())
    {
        if ((stateID == ePedestrianState_Runs) || (stateID == ePedestrianState_Walks))
        {
            SfxSampleIndex sfxIndex = (stateID == ePedestrianState_Runs) ? SfxLevel_FootStep2 : SfxLevel_FootStep1;
            StartGameObjectSound(ePedSfxChannelIndex_Misc, eSfxSampleType_Level, sfxIndex, SfxFlags_None);
        }
    }

    return true;
}

ePedestrianAnimID Pedestrian::GetCurrentAnimationID() const
{
    return mCurrentAnimID;
}

void Pedestrian::PushByPedestrian(Pedestrian* otherPedestrian)
{
    cxx_assert(otherPedestrian);
    cxx_assert(this != otherPedestrian);
    if ((otherPedestrian == nullptr) || (mPhysicsBody == nullptr))
        return;

    // todo: implement
}

bool Pedestrian::CanRun() const
{
    return !(mContactingOtherPeds && IsPlayerCharacter());
}

void Pedestrian::SetRemap(int remapIndex)
{
    mRemapIndex = remapIndex;
    mRemapClut = (mRemapIndex == NO_REMAP) ? 0 : mRemapIndex + gGame.mStyleData.GetPedestrianRemapsBaseIndex();
}

const PedestrianCtlState& Pedestrian::GetCtlState() const
{
    static const PedestrianCtlState nullCtlState;
    if (mCtlState)
        return *mCtlState;

    return nullCtlState;
}

