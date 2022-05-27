#pragma once

#include "Pedestrian.h"
#include "AiPedestrianBehavior.h"

class AiCharacterController
{
public:
    // readonly
    AiPedestrianBehavior* mAiBehavior = nullptr;
    Pedestrian* mCharacter = nullptr; // controllable character
    PedestrianCtlState mCtlState;

public:
    AiCharacterController() = default;
    virtual ~AiCharacterController();

    void SetCharacter(Pedestrian* character);
    bool IsControllerActive() const;

    void UpdateFrame();
    void DebugDraw(DebugRenderer& debugRender);

    // objectives
    void FollowPedestrian(Pedestrian* pedestrian);
};