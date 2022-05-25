#include "stdafx.h"
#include "GuiManager.h"
#include "RenderingManager.h"
#include "GpuProgram.h"
#include "SpriteManager.h"
#include "GuiContext.h"
#include "GtaOneGame.h"
#include "ImGuiManager.h"
#include "FontManager.h"
#include "ConsoleVar.h"

//////////////////////////////////////////////////////////////////////////
// cvars
//////////////////////////////////////////////////////////////////////////

CvarFloat gCvarUiScale("g_uiScale", 1.0f, "Ui elements scale factor", CvarFlags_Archive);

//////////////////////////////////////////////////////////////////////////

GuiManager gGuiManager;

//////////////////////////////////////////////////////////////////////////

bool GuiManager::Initialize()
{
    if (!mSpriteBatch.Initialize())
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot initialize sprites batch");
        return false;
    }

    gFontManager.Initialize();

    mCamera2D.SetIdentity();
    return true;
}

void GuiManager::Deinit()
{
    mSpriteBatch.Deinit();
    gFontManager.Deinit();
    // drop screens
    mScreensList.clear();
}

void GuiManager::RenderFrame()
{
    mSpriteBatch.BeginBatch(SpriteBatch::DepthAxis_Z, eSpritesSortMode_None);

    Rect prevScreenRect = gGraphicsDevice.mViewportRect;
    Rect prevScissorsBox = gGraphicsDevice.mScissorBox;

    // draw renderviews
    {
        gGraphicsDevice.BindTexture(eTextureUnit_3, gSpriteManager.mPalettesTable);
        gGraphicsDevice.BindTexture(eTextureUnit_2, gSpriteManager.mPaletteIndicesTable);

        gRenderManager.mSpritesProgram.Activate();

        RenderStates guiRenderStates = RenderStates()
            .Disable(RenderStateFlags_FaceCulling)
            .Disable(RenderStateFlags_DepthTest);
        gGraphicsDevice.SetRenderStates(guiRenderStates);

        for (GuiScreen* currScreen: mScreensList)
        {
            mCamera2D.SetIdentity();
            mCamera2D.mViewportRect = currScreen->mScreenArea;
            mCamera2D.SetProjection(0.0f, mCamera2D.mViewportRect.w * 1.0f, mCamera2D.mViewportRect.h * 1.0f, 0.0f);

            gGraphicsDevice.SetViewportRect(mCamera2D.mViewportRect);
            gRenderManager.mSpritesProgram.UploadCameraTransformMatrices(mCamera2D);

            GuiContext uiContext ( mCamera2D, mSpriteBatch );
            Rect clipRect { 0, 0, mCamera2D.mViewportRect.w, mCamera2D.mViewportRect.h };
            if (uiContext.EnterChildClipArea(clipRect))
            {
                currScreen->DrawScreen(uiContext);
                uiContext.LeaveChildClipArea();
            }
            mSpriteBatch.Flush();   
        }

        gRenderManager.mSpritesProgram.Deactivate();
    }

    { // draw imgui

        mCamera2D.SetIdentity();
        mCamera2D.mViewportRect = prevScreenRect;
        mCamera2D.SetProjection(0.0f, mCamera2D.mViewportRect.w * 1.0f, mCamera2D.mViewportRect.h * 1.0f, 0.0f);

        gRenderManager.mGuiTexColorProgram.Activate();
        gRenderManager.mGuiTexColorProgram.UploadCameraTransformMatrices(mCamera2D);
    
        RenderStates guiRenderStates = RenderStates()
            .Disable(RenderStateFlags_FaceCulling)
            .Disable(RenderStateFlags_DepthTest)
            .SetAlphaBlend(eBlendMode_Alpha);
        gGraphicsDevice.SetRenderStates(guiRenderStates);

        gGraphicsDevice.SetScissorRect(prevScissorsBox);
        gGraphicsDevice.SetViewportRect(prevScreenRect);

        gImGuiManager.RenderFrame();

        gRenderManager.mGuiTexColorProgram.Deactivate();
    }
}

void GuiManager::UpdateFrame()
{
    for (GuiScreen* currScreen: mScreensList)
    {
        currScreen->UpdateScreen();
    }
}

void GuiManager::InputEvent(MouseMovedInputEvent& inputEvent)
{
    // do nothing
}

void GuiManager::InputEvent(MouseScrollInputEvent& inputEvent)
{
    // do nothing
}

void GuiManager::InputEvent(MouseButtonInputEvent& inputEvent)
{
    // do nothing
}

void GuiManager::InputEvent(KeyInputEvent& inputEvent)
{
    // do nothing
}

void GuiManager::InputEvent(KeyCharEvent& inputEvent)
{
    // do nothing
}

void GuiManager::InputEvent(GamepadInputEvent& inputEvent)
{
    // do nothing
}

void GuiManager::AttachScreen(GuiScreen* screen)
{
    debug_assert(screen);
    if (cxx::contains(mScreensList, screen))
        return;

    mScreensList.push_back(screen);
}

void GuiManager::DetachScreen(GuiScreen* screen)
{
    debug_assert(screen);
    cxx::erase_elements(mScreensList, screen);
}