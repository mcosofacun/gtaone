#pragma once

#include "GameDefs.h"
#include "BroadcastEvent.h"

// Gamestate controller base class
class Gamestate
{
public:
    Gamestate() = default;
    virtual ~Gamestate();

    // enter or exit gamestate
    virtual void OnGamestateEnter();
    virtual void OnGamestateLeave();

    // process gamestate frame logic
    virtual void OnGamestateFrame();

    // process gamestate input events
    virtual void OnGamestateInputEvent(KeyInputEvent& inputEvent);
    virtual void OnGamestateInputEvent(MouseButtonInputEvent& inputEvent);
    virtual void OnGamestateInputEvent(MouseMovedInputEvent& inputEvent);
    virtual void OnGamestateInputEvent(MouseScrollInputEvent& inputEvent);
    virtual void OnGamestateInputEvent(KeyCharEvent& inputEvent);
    virtual void OnGamestateInputEvent(GamepadInputEvent& inputEvent);
    virtual void OnGamestateInputEventLost();

    // process game event
    virtual void OnGamestateBroadcastEvent(const BroadcastEvent& broadcastEvent);
};