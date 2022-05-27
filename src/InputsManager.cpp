#include "stdafx.h"
#include "InputsManager.h"
#include "GtaOneGame.h"
#include "ConsoleWindow.h"

InputsManager::InputsManager()
{
    Cleanup();
}

void InputsManager::InputEvent(MouseButtonInputEvent& inputEvent)
{
    mMouseButtons[inputEvent.mButton] = inputEvent.mPressed;

    for (InputEventsHandler* currentHandler: mInputHandlers)
    {
        currentHandler->InputEvent(inputEvent);
        if (inputEvent.mConsumed)
        {
            InputEventConsumed(currentHandler);
            break;
        }
    }
}

void InputsManager::InputEvent(MouseMovedInputEvent& inputEvent)
{
    mCursorPositionX = inputEvent.mCursorPositionX;
    mCursorPositionY = inputEvent.mCursorPositionY;

    for (InputEventsHandler* currentHandler: mInputHandlers)
    {
        currentHandler->InputEvent(inputEvent);
        if (inputEvent.mConsumed)
        {
            InputEventConsumed(currentHandler);
            break;
        }
    }
}

void InputsManager::InputEvent(MouseScrollInputEvent& inputEvent)
{
    for (InputEventsHandler* currentHandler: mInputHandlers)
    {
        currentHandler->InputEvent(inputEvent);
        if (inputEvent.mConsumed)
        {
            InputEventConsumed(currentHandler);
            break;
        }
    }
}

void InputsManager::InputEvent(GamepadInputEvent& inputEvent)
{
    cxx_assert(inputEvent.mGamepad < eGamepadID_COUNT);
    cxx_assert(inputEvent.mButton < eGamepadButton_COUNT);
    mGamepadsState[inputEvent.mGamepad].mButtons[inputEvent.mButton] = inputEvent.mPressed;

    for (InputEventsHandler* currentHandler: mInputHandlers)
    {
        currentHandler->InputEvent(inputEvent);
        if (inputEvent.mConsumed)
        {
            InputEventConsumed(currentHandler);
            break;
        }
    }
}

void InputsManager::InputEvent(KeyInputEvent& inputEvent)
{
    if (HandleDebugKeys(inputEvent))
    {
        InputEventConsumed(nullptr);
        return;
    }

    mKeyboardKeys[inputEvent.mKeycode] = inputEvent.mPressed;

    for (InputEventsHandler* currentHandler: mInputHandlers)
    {
        currentHandler->InputEvent(inputEvent);
        if (inputEvent.mConsumed)
        {
            InputEventConsumed(currentHandler);
            break;
        }
    }
}

void InputsManager::InputEvent(KeyCharEvent& inputEvent)
{
    for (InputEventsHandler* currentHandler: mInputHandlers)
    {
        currentHandler->InputEvent(inputEvent);
        if (inputEvent.mConsumed)
        {
            InputEventConsumed(currentHandler);
            break;
        }
    }
}

void InputsManager::Cleanup()
{
    ::memset(mMouseButtons, 0, sizeof(mMouseButtons));
    ::memset(mKeyboardKeys, 0, sizeof(mKeyboardKeys));
    ::memset(mGamepadsState, 0, sizeof(mGamepadsState));

    mLastInputsHandler = nullptr;
    mInputHandlers.clear();
}

void InputsManager::SetGamepadPresent(int gamepad, bool isPresent)
{
    cxx_assert(gamepad < eGamepadID_COUNT);

    mGamepadsState[gamepad].mPresent = isPresent;
    ::memset(mGamepadsState[gamepad].mButtons, 0, sizeof(mGamepadsState[gamepad].mButtons));
}

void InputsManager::InputEventConsumed(InputEventsHandler* handler)
{
    if (mLastInputsHandler == handler)
        return;

    if (mLastInputsHandler)
    {
        mLastInputsHandler->InputEventLost();
    }
    mLastInputsHandler = handler;
}

void InputsManager::UpdateFrame()
{
    // update input handlers

    mInputHandlers.clear();

    if (gGame.mImGuiMng.IsInitialized())
    {
        mInputHandlers.push_back(&gGame.mImGuiMng);
    }
    mInputHandlers.push_back(&gGame.mGuiMng);
    mInputHandlers.push_back(&gGame);
}

bool InputsManager::HandleDebugKeys(KeyInputEvent& inputEvent)
{
    // show/hide debug console window
    if (inputEvent.HasPressed(eKeycode_TILDE) || inputEvent.HasReleased(eKeycode_TILDE))
    {
        if (inputEvent.HasPressed(eKeycode_TILDE))
        {
            gDebugConsoleWindow.ToggleWindowShown();
        }
        inputEvent.SetConsumed();
        return true;
    }

    return false;
}

bool InputsManager::LoadInputActionsMapping()
{
    mActionsMapping.Clear();
    mActionsMapping.SetDefaults();

    // open config document
    cxx::json_document configDocument;
    if (!gSystem.mFiles.ReadConfig(gSystem.mFiles.mInputsConfigPath, configDocument))
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot load inputs config from '%s'", gSystem.mFiles.mInputsConfigPath.c_str());
        return false;
    }
    cxx::json_document_node rootNode = configDocument.get_root_node();
    mActionsMapping.LoadConfig(rootNode);
    return true;
}

void InputsManager::Initialize()
{
    LoadInputActionsMapping();
}
