#pragma once

#include "Gamestate.h"

// Main game
class GameplayGamestate: public Gamestate
{
public:
    GameplayGamestate() = default;

    // override GenericGamestate
    void OnGamestateEnter() override;
    void OnGamestateLeave() override;
    void OnGamestateFrame() override;
    void OnGamestateInputEvent(KeyInputEvent& inputEvent) override;
    void OnGamestateInputEvent(MouseButtonInputEvent& inputEvent) override;
    void OnGamestateInputEvent(MouseMovedInputEvent& inputEvent) override;
    void OnGamestateInputEvent(MouseScrollInputEvent& inputEvent) override;
    void OnGamestateInputEvent(KeyCharEvent& inputEvent) override;
    void OnGamestateInputEvent(GamepadInputEvent& inputEvent) override;
    void OnGamestateInputEventLost() override;
    void OnGamestateBroadcastEvent(const BroadcastEvent& broadcastEvent) override;

private:
    void OnHumanPlayerDie();
    void OnHumanPlayerStartDriveCar();

    void ProcessInputAction(eInputAction action, bool isActivated);
    void ProcessRepetitiveActions();

    void UpdateDistrictLocation();
    void UpdateMouseAiming();
    void UpdateRespawnTimer();

    void UpdatePlayer();
    void RespawnPlayer();

    bool GetActionState(eInputAction action) const;

    void ShowLastDistrictLocation();
    void SwitchNextWeapon();
    void SwitchPrevWeapon();
    void EnterOrExitCar(bool alternative);
};