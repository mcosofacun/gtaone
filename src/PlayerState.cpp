#include "stdafx.h"
#include "PlayerState.h"
#include "GtaOneGame.h"

int PlayerState::GetWantedLevel() const
{
    return mWantedLevel;
}

void PlayerState::SetWantedLevel(int wantedLevel)
{
    cxx_assert(wantedLevel <= GAME_MAX_WANTED_LEVEL);
    mWantedLevel = wantedLevel;
}

void PlayerState::Cheat_GiveAllAmmunitions()
{
    if (mCharacter)
    {
        for (int icurr = 0; icurr < eWeapon_COUNT; ++icurr)
        {
            mCharacter->mWeapons[icurr].AddAmmunition(99);
        }
        mCharacter->IncArmorMax();
    }
}

void PlayerState::SetRespawnTimer()
{
    mRespawnTime = gGame.mParams.mGamePlayerRespawnTime;
}

void PlayerState::SetCharacter(Pedestrian* character)
{
    mCtlState.Clear();
    if (mCharacter)
    {
        cxx_assert(mCharacter->mCtlState == &mCtlState);
        mCharacter->mCtlState = nullptr;
    }
    mCharacter = character;
    if (mCharacter)
    {
        mCharacter->mCtlState = &mCtlState;
        mSpawnPosition = mCharacter->mTransform.mPosition;
    }
}