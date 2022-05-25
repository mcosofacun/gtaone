#pragma once

#include "GameMapManager.h"
#include "GameObjectsManager.h"
#include "HumanPlayer.h"
#include "GameplayGamestate.h"
#include "MainMenuGamestate.h"

// top level game application controller
class GtaOneGame final: public InputEventsHandler
{
    friend class GameplayGamestate;
    friend class MainMenuGamestate;

public:
    cxx::randomizer mGameRand;

    // readonly
    GenericGamestate* mCurrentGamestate = nullptr;
    HumanPlayer* mHumanPlayer = nullptr;

public:
    // Setup resources and switch to initial game state
    bool Initialize();

    // Cleanup current state and finish game
    void Deinit();

    // Common processing
    void UpdateFrame();

    // override InputEventsHandler
    void InputEvent(KeyInputEvent& inputEvent) override;
    void InputEvent(MouseButtonInputEvent& inputEvent) override;
    void InputEvent(MouseMovedInputEvent& inputEvent) override;
    void InputEvent(MouseScrollInputEvent& inputEvent) override;
    void InputEvent(KeyCharEvent& inputEvent) override;
    void InputEvent(GamepadInputEvent& inputEvent) override;
    void InputEventLost() override;

    // Current game state
    bool IsMenuGameState() const;
    bool IsInGameState() const;

    // Initialize player data
    void SetupHumanPlayer(Pedestrian* pedestrian);
    void DeleteHumanPlayer();

private:
    bool SetInputActionsFromConfig();
    bool DetectGameVersion();

    std::string GetTextsLanguageFileName(const std::string& languageID) const;

    bool StartScenario(const std::string& mapName);
    void ShutdownCurrentScenario();

    void SetCurrentGamestate(GenericGamestate* gamestate);

    void ProcessDebugCvars();

private:
    GameplayGamestate mGameplayGamestate;
    MainMenuGamestate mMainMenuGamestate;
};

extern GtaOneGame gGame;