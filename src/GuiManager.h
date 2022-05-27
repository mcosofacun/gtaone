#pragma once

#include "GameCamera.h"
#include "SpriteBatch.h"
#include "GuiScreen.h"
#include "Font.h"

// manages all graphical user interface operation
class GuiManager final: public InputEventsHandler
{
public:
    // setup/free internal resources
    bool Initialize();
    void Deinit();

    void RenderFrame();
    void UpdateFrame();

    // Flush all currently loaded fonts
    void FlushAllFonts();

    // Finds font instance within cache and force it to load
    // @param fontName: Font name
    // @returns font instance which might be not loaded in case of error
    Font* GetFont(const std::string& fontName);

    // manage gui screens
    void AttachScreen(GuiScreen* screen);
    void DetachScreen(GuiScreen* screen);

    // override InputEventsHandler
    void InputEvent(MouseMovedInputEvent& inputEvent) override;
    void InputEvent(MouseScrollInputEvent& inputEvent) override;
    void InputEvent(MouseButtonInputEvent& inputEvent) override;
    void InputEvent(KeyInputEvent& inputEvent) override;
    void InputEvent(KeyCharEvent& inputEvent) override;
    void InputEvent(GamepadInputEvent& inputEvent) override;

private:
    void FreeFonts();

private:
    SpriteBatch mSpriteBatch;
    GameCamera2D mCamera2D;
    std::vector<GuiScreen*> mScreensList;
    std::map<std::string, Font*> mFontsCache;
};