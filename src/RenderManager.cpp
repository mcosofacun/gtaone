#include "stdafx.h"
#include "RenderManager.h"
#include "GpuTexture2D.h"
#include "GpuProgram.h"
#include "GameCheatsWindow.h"
#include "ParticleRenderdata.h"
#include "GtaOneGame.h"

RenderManager::RenderManager()
    : mDefaultTexColorProgram("shaders/texture_color.glsl")
    , mGuiTexColorProgram("shaders/gui.glsl")
    , mCityMeshProgram("shaders/city_mesh.glsl")
    , mSpritesProgram("shaders/sprites.glsl")
    , mDebugProgram("shaders/debug.glsl")
    , mParticleProgram("shaders/particle.glsl")
{
}

bool RenderManager::Initialize()
{
    InitRenderPrograms();

    gSystem.mGfxDevice.SetClearColor(Color32_SkyBlue);

    return true;
}

void RenderManager::Deinit()
{
    FreeRenderPrograms();
}

void RenderManager::FreeRenderPrograms()
{
    mDefaultTexColorProgram.Deinit();
    mCityMeshProgram.Deinit();
    mGuiTexColorProgram.Deinit();
    mSpritesProgram.Deinit();
    mParticleProgram.Deinit();
    mDebugProgram.Deinit();
}

bool RenderManager::InitRenderPrograms()
{
    mDefaultTexColorProgram.Initialize();
    mGuiTexColorProgram.Initialize();
    mCityMeshProgram.Initialize(); 
    mSpritesProgram.Initialize();
    mParticleProgram.Initialize();
    mDebugProgram.Initialize();

    return true;
}

void RenderManager::ReloadRenderPrograms()
{
    gSystem.LogMessage(eLogMessage_Info, "Reloading render programs...");

    mDefaultTexColorProgram.Reinitialize();
    mGuiTexColorProgram.Reinitialize();
    mDebugProgram.Reinitialize();
    mSpritesProgram.Reinitialize();
    mParticleProgram.Reinitialize();
    mCityMeshProgram.Reinitialize();
}

void RenderManager::RegisterParticleEffect(ParticleEffect* particleEffect)
{
    cxx_assert(particleEffect);

    if (particleEffect == nullptr)
        return;

    if (particleEffect->mRenderdata)
    {
        cxx_assert(false);
        return;
    }

    ParticleRenderdata* renderdata = new ParticleRenderdata;
    particleEffect->SetRenderdata(renderdata);
}

void RenderManager::UnregisterParticleEffect(ParticleEffect* particleEffect)
{
    cxx_assert(particleEffect);

    if (particleEffect == nullptr)
        return;

    ParticleRenderdata* renderdata = particleEffect->mRenderdata;
    if (renderdata)
    {
        renderdata->DestroyVertexbuffer();
        delete renderdata;
    }
    particleEffect->SetRenderdata(nullptr);
}

void RenderManager::RenderParticleEffects(GameCamera& renderview)
{
    bool hasActiveParticleEffects = false;
    // check if there is something to draw
    for (ParticleEffect* currEffect: gGame.mParticlesMng.mParticleEffects)
    {
        if (currEffect->IsEffectActive())
        {
            hasActiveParticleEffects = true;
            break;
        }
    }

    if (!hasActiveParticleEffects)
        return;

    mParticleProgram.Activate();
    mParticleProgram.UploadCameraTransformMatrices(renderview);

    RenderStates renderStates = RenderStates()
        .Enable(RenderStateFlags_AlphaBlend)
        .Disable(RenderStateFlags_FaceCulling)
        .Disable(RenderStateFlags_DepthWrite);
    gSystem.mGfxDevice.SetRenderStates(renderStates);

    for (ParticleEffect* currEffect: gGame.mParticlesMng.mParticleEffects)
    {
        if (currEffect->IsEffectInactive())
            continue;

        RenderParticleEffect(renderview, currEffect);
    }

    mParticleProgram.Deactivate();
}

void RenderManager::RenderParticleEffect(GameCamera& renderview, ParticleEffect* particleEffect)
{
    ParticleRenderdata* renderdata = particleEffect->mRenderdata;
    if (renderdata == nullptr)
    {
        cxx_assert(false); // effect is not registered - something is wrong
        return; 
    }

    const int NumParticles = particleEffect->mAliveParticlesCount;
    if (NumParticles == 0)
        return;

    // update vertices
    if (renderdata->mIsInvalidated)
    {
        renderdata->ResetInvalidated();
        if (!renderdata->PrepareVertexbuffer(particleEffect->mParticles.size() * Sizeof_ParticleVertex))
        {
            cxx_assert(false);
            return;
        }

        GpuBuffer* vertexbuffer = renderdata->mVertexBuffer;
        cxx_assert(vertexbuffer);
        ParticleVertex* vertices = vertexbuffer->LockData<ParticleVertex>(BufferAccess_UnsynchronizedWrite | BufferAccess_InvalidateBuffer);
        if (vertices == nullptr)
        {
            cxx_assert(false);
            return;
        }

        for (int icurrParticle = 0; icurrParticle < NumParticles; ++icurrParticle)
        {
            ParticleVertex& particleVertex = vertices[icurrParticle];
            const Particle& srcParticle = particleEffect->mParticles[icurrParticle];
            particleVertex.mPositionSize.x = srcParticle.mPosition.x;
            particleVertex.mPositionSize.y = srcParticle.mPosition.y;
            particleVertex.mPositionSize.z = srcParticle.mPosition.z;
            particleVertex.mPositionSize.w = srcParticle.mSize;
            particleVertex.mColor = srcParticle.mColor;
        }

        if (!vertexbuffer->Unlock())
        {
            cxx_assert(false);
            return;
        }
    }

    if (renderdata->mVertexBuffer == nullptr)
    {
        cxx_assert(false);
        return;
    }

    ParticleVertex_Format vFormat;
    gSystem.mGfxDevice.BindVertexBuffer(renderdata->mVertexBuffer, vFormat);
    gSystem.mGfxDevice.RenderPrimitives(ePrimitiveType_Points, 0, NumParticles);
}