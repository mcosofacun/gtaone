#pragma once

#include "GameDefs.h"

// player-related state
class PlayerState final
{
    friend class GameplayGamestate;

public:
    glm::vec3 mSpawnPosition;
    Pedestrian* mCharacter = nullptr;
    PedestrianCtlState mCtlState;
   
public:
    void SetCharacter(Pedestrian* character);

    void SetWantedLevel(int wantedLevel);
    void SetRespawnTimer();

    int GetWantedLevel() const;

    // cheats
    void Cheat_GiveAllAmmunitions();

private:
    int mLastDistrictIndex = 0; // district index where character was last time
    int mWantedLevel = 0; // current police attention level
    float mRespawnTime = 0.0f;
    bool mUpdateInputs = false;
};