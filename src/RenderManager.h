#pragma once

#include "GraphicsDefs.h"
#include "RenderProgram.h"
#include "ParticleEffect.h"

// master render system, it is intended to manage rendering pipeline of the game
class RenderManager final: public cxx::noncopyable
{
public:
    RenderProgram mDefaultTexColorProgram;
    RenderProgram mCityMeshProgram;
    RenderProgram mGuiTexColorProgram;
    RenderProgram mSpritesProgram;
    RenderProgram mDebugProgram;
    RenderProgram mParticleProgram;

public:
    RenderManager();

    bool Initialize();
    void Deinit();

    void RenderParticleEffects(GameCamera& renderview);

    // Force reload all render programs
    void ReloadRenderPrograms();
    
    // Register particle effect for rendering
    void RegisterParticleEffect(ParticleEffect* particleEffect);
    void UnregisterParticleEffect(ParticleEffect* particleEffect);

private:
    void RenderParticleEffect(GameCamera& renderview, ParticleEffect* particleEffect);

private:
    bool InitRenderPrograms();
    void FreeRenderPrograms();
};