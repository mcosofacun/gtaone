#include "stdafx.h"
#include "GameParams.h"

GameParams::GameParams()
{
    SetToDefaults();
}

void GameParams::SetToDefaults()
{
    // todo: move to config

    // game - commons
    mGamePlayerRespawnTime = 6.0f;
    mGameRailwaysDamageDelay = 1.5f;

    // pedestrians
    mPedestrianBoundsSphereRadius = Convert::MapUnitsToMeters(0.10f);
    mPedestrianTurnSpeed = 260.0f;
    mPedestrianTurnSpeedSlideOnCar = 120.0f;
    mPedestrianSlideOnCarSpeed = Convert::MapUnitsToMeters(1.2f);
    mPedestrianWalkSpeed = Convert::MapUnitsToMeters(0.5f);
    mPedestrianRunSpeed = Convert::MapUnitsToMeters(1.5f);
    mPedestrianSpotTheCarDistance = Convert::MapUnitsToMeters(3.0f);
    mPedestrianKnockedDownTime = 3.0f;
    mPedestrianFallDeathHeight = Convert::MapUnitsToMeters(2.0f);
    mPedestrianDrowningTime = 0.05f;
    mPedestrianBurnDuration = 4.0f;
    mPedestrianMaxArmor = 3;
    // traffic - common
    mTrafficGenHareKrishnasTime = 60.0f;
    // traffic - pedestrians
    mTrafficGenMaxPeds = 20;
    mTrafficGenPedsChance = 50;
    mTrafficGenPedsMaxDistance = 2;
    mTrafficGenPedsCooldownTime = 1.5f;
    // traffic - cars
    mTrafficGenMaxCars = 12;
    mTrafficGenCarsChance = 65;
    mTrafficGenCarsMaxDistance = 4;
    mTrafficGenCarsCooldownTime = 3.0f;
    // explosion
    mExplosionRadius = Convert::MapUnitsToMeters(1.0f);
    // vehicles
    mCarBurnDuration = 20.0f;
    mCarSpeedPassengerCanEnter = 8.0f;
    mCarExplosionChainDelayTime = 0.5f;
    // broadcast events
    mBroadcastGunShotEventDuration = 1.0f;
    mBroadcastExplosionEventDuration = 1.5f;
    // ai
    mAiReactOnGunshotsDistance = Convert::MapUnitsToMeters(4.0f);
    mAiReactOnExplosionsDistance = Convert::MapUnitsToMeters(5.0f);
    // hud
    mHudBigFontMessageShowDuration = 3.0f;
    mHudCarNameShowDuration = 3.0f;
    mHudDistrictNameShowDuration = 3.0f;
    // collision
    mSparksOnCarsContactThreshold = 60.0f;
}