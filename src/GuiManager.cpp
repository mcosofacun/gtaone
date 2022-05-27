#include "stdafx.h"
#include "GuiManager.h"
#include "GpuProgram.h"
#include "GuiContext.h"
#include "GtaOneGame.h"
#include "ConsoleVar.h"

//////////////////////////////////////////////////////////////////////////
// cvars
//////////////////////////////////////////////////////////////////////////

CvarFloat gCvarUiScale("g_uiScale", 1.0f, "Ui elements scale factor", CvarFlags_Archive);

//////////////////////////////////////////////////////////////////////////

bool GuiManager::Initialize()
{
    if (!mSpriteBatch.Initialize())
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot initialize sprites batch");
        return false;
    }

    mCamera2D.SetIdentity();
    return true;
}

void GuiManager::Deinit()
{
    mSpriteBatch.Deinit();
    FreeFonts();
    // drop screens
    mScreensList.clear();
}

void GuiManager::RenderFrame()
{
    mSpriteBatch.BeginBatch(SpriteBatch::DepthAxis_Z, eSpritesSortMode_None);

    Rect prevScreenRect = gSystem.mGfxDevice.mViewportRect;
    Rect prevScissorsBox = gSystem.mGfxDevice.mScissorBox;

    // draw renderviews
    {
        gSystem.mGfxDevice.BindTexture(eTextureUnit_3, gGame.mSpritesMng.mPalettesTable);
        gSystem.mGfxDevice.BindTexture(eTextureUnit_2, gGame.mSpritesMng.mPaletteIndicesTable);

        gGame.mRenderMng.mSpritesProgram.Activate();

        RenderStates guiRenderStates = RenderStates()
            .Disable(RenderStateFlags_FaceCulling)
            .Disable(RenderStateFlags_DepthTest);
        gSystem.mGfxDevice.SetRenderStates(guiRenderStates);

        for (GuiScreen* currScreen: mScreensList)
        {
            mCamera2D.SetIdentity();
            mCamera2D.mViewportRect = currScreen->mScreenArea;
            mCamera2D.SetProjection(0.0f, mCamera2D.mViewportRect.w * 1.0f, mCamera2D.mViewportRect.h * 1.0f, 0.0f);

            gSystem.mGfxDevice.SetViewportRect(mCamera2D.mViewportRect);
            gGame.mRenderMng.mSpritesProgram.UploadCameraTransformMatrices(mCamera2D);

            GuiContext uiContext ( mCamera2D, mSpriteBatch );
            Rect clipRect { 0, 0, mCamera2D.mViewportRect.w, mCamera2D.mViewportRect.h };
            if (uiContext.EnterChildClipArea(clipRect))
            {
                currScreen->DrawScreen(uiContext);
                uiContext.LeaveChildClipArea();
            }
            mSpriteBatch.Flush();   
        }

        gGame.mRenderMng.mSpritesProgram.Deactivate();
    }

    { // draw imgui

        mCamera2D.SetIdentity();
        mCamera2D.mViewportRect = prevScreenRect;
        mCamera2D.SetProjection(0.0f, mCamera2D.mViewportRect.w * 1.0f, mCamera2D.mViewportRect.h * 1.0f, 0.0f);

        gGame.mRenderMng.mGuiTexColorProgram.Activate();
        gGame.mRenderMng.mGuiTexColorProgram.UploadCameraTransformMatrices(mCamera2D);
    
        RenderStates guiRenderStates = RenderStates()
            .Disable(RenderStateFlags_FaceCulling)
            .Disable(RenderStateFlags_DepthTest)
            .SetAlphaBlend(eBlendMode_Alpha);
        gSystem.mGfxDevice.SetRenderStates(guiRenderStates);
        gSystem.mGfxDevice.SetScissorRect(prevScissorsBox);
        gSystem.mGfxDevice.SetViewportRect(prevScreenRect);

        gGame.mImGuiMng.RenderFrame();

        gGame.mRenderMng.mGuiTexColorProgram.Deactivate();
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
    cxx_assert(screen);
    if (cxx::contains(mScreensList, screen))
        return;

    mScreensList.push_back(screen);
}

void GuiManager::DetachScreen(GuiScreen* screen)
{
    cxx_assert(screen);
    cxx::erase_elements(mScreensList, screen);
}

void GuiManager::FreeFonts()
{
    for (auto& currFont: mFontsCache)
    {
        delete currFont.second;
    }
    mFontsCache.clear();
}

void GuiManager::FlushAllFonts()
{
    for (auto& currFont: mFontsCache)
    {
        currFont.second->Unload();
    }
}

Font* GuiManager::GetFont(const std::string& fontName)
{
    Font* fontInstance = nullptr;

    auto find_iterator = mFontsCache.find(fontName);
    if (find_iterator != mFontsCache.end())
    {
        fontInstance = find_iterator->second;
    }

    if (fontInstance == nullptr) // cache miss
    {
        fontInstance = new Font(fontName);
        mFontsCache[fontName] = fontInstance;
    }

    cxx_assert(fontInstance);
    if (!fontInstance->IsLoaded() && !fontInstance->LoadFromFile())
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot load font '%s'", fontName.c_str());
    }

    return fontInstance;
}
