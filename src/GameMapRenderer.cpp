#include "stdafx.h"
#include "GameMapRenderer.h"
#include "GpuBuffer.h"
#include "GtaOneGame.h"
#include "GpuTexture2D.h"
#include "GameCheatsWindow.h"
#include "PhysicsBody.h"
#include "Pedestrian.h"
#include "Vehicle.h"

//////////////////////////////////////////////////////////////////////////

void MapRenderStats::FrameBegin()
{
    mBlockChunksDrawnCount = 0;
    mSpritesDrawnCount = 0;

    ++mRenderFramesCounter;
}

void MapRenderStats::FrameEnd()
{
}

//////////////////////////////////////////////////////////////////////////

bool GameMapRenderer::Initialize()
{
    mCityMeshBufferV = gSystem.mGfxDevice.CreateBuffer(eBufferContent_Vertices);
    cxx_assert(mCityMeshBufferV);

    mCityMeshBufferI = gSystem.mGfxDevice.CreateBuffer(eBufferContent_Indices);
    cxx_assert(mCityMeshBufferI);

    if (mCityMeshBufferV == nullptr || mCityMeshBufferI == nullptr)
        return false;

    if (!mSpriteBatch.Initialize())
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot initialize sprites batch");
        return false;
    }

    return true;
}

void GameMapRenderer::Deinit()
{
    mSpriteBatch.Deinit();
    if (mCityMeshBufferV)
    {
        gSystem.mGfxDevice.DestroyBuffer(mCityMeshBufferV);
        mCityMeshBufferV = nullptr;
    }

    if (mCityMeshBufferI)
    {
        gSystem.mGfxDevice.DestroyBuffer(mCityMeshBufferI);
        mCityMeshBufferI = nullptr;
    }
}

void GameMapRenderer::RenderFrameBegin()
{
    mRenderStats.FrameBegin();

    // pre draw game objects
    for (GameObject* gameObject: gGame.mObjectsMng.mAllObjects)
    {
        if (gameObject->IsAttachedToObject())
            continue;

        PreDrawGameObject(gameObject);
    }
}

void GameMapRenderer::RenderFrameEnd()
{
    mRenderStats.FrameEnd();
}

void GameMapRenderer::PreDrawGameObject(GameObject* gameObject)
{
    if (gameObject->IsMarkedForDeletion() || gameObject->IsInvisibleFlag())
        return;

    // process attached objects
    for (GameObject* currAttachment: gameObject->mAttachedObjects)
    {
        PreDrawGameObject(currAttachment);
    }
}

void GameMapRenderer::RenderFrame(GameCamera& gameCamera)
{
    gSystem.mGfxDevice.BindTexture(eTextureUnit_3, gGame.mSpritesMng.mPalettesTable);
    gSystem.mGfxDevice.BindTexture(eTextureUnit_2, gGame.mSpritesMng.mPaletteIndicesTable);

    if (gGameCheatsWindow.mEnableDrawCityMesh)
    {
        DrawCityMesh(gameCamera);
    }

    mSpriteBatch.BeginBatch(SpriteBatch::DepthAxis_Y, eSpritesSortMode_HeightAndDrawOrder);

    // collect and render game objects sprites
    for (GameObject* gameObject: gGame.mObjectsMng.mAllObjects)
    {
        // attached objects must be drawn after the object to which they are attached
        if (gameObject->IsAttachedToObject())
            continue;

        DrawGameObject(gameCamera, gameObject);
    }

    gGame.mRenderMng.mSpritesProgram.Activate();
    gGame.mRenderMng.mSpritesProgram.UploadCameraTransformMatrices(gameCamera);

    RenderStates renderStates = RenderStates()
        .Disable(RenderStateFlags_FaceCulling)
        .Disable(RenderStateFlags_DepthWrite);
    gSystem.mGfxDevice.SetRenderStates(renderStates);

    mSpriteBatch.Flush();

    gGame.mRenderMng.mSpritesProgram.Deactivate();
}

void GameMapRenderer::DrawGameObject(GameCamera& gameCamera, GameObject* gameObject)
{
    if (gameObject->IsMarkedForDeletion() || gameObject->IsInvisibleFlag())
        return;

    bool debugSkipDraw = 
        (!gGameCheatsWindow.mEnableDrawPedestrians && gameObject->IsPedestrianClass()) ||
        (!gGameCheatsWindow.mEnableDrawVehicles && gameObject->IsVehicleClass()) ||
        (!gGameCheatsWindow.mEnableDrawObstacles && gameObject->IsObstacleClass()) ||
        (!gGameCheatsWindow.mEnableDrawDecorations && gameObject->IsDecorationClass());

    // detect if gameobject is visible on screen
    if (!debugSkipDraw && gameObject->IsOnScreen(gameCamera.mOnScreenMapArea))
    {
        mSpriteBatch.DrawSprite(gameObject->mDrawSprite);

        ++mRenderStats.mSpritesDrawnCount;
        gameObject->mLastRenderFrame = mRenderStats.mRenderFramesCounter;
    }

    // draw attached objects
    for (GameObject* currAttachment: gameObject->mAttachedObjects)
    {
        DrawGameObject(gameCamera, currAttachment);
    }
}

void GameMapRenderer::DebugDraw(DebugRenderer& debugRender)
{
    for (GameObject* gameObject: gGame.mObjectsMng.mAllObjects)
    {
        // check if gameobject was on screen in current frame
        if (gameObject->mLastRenderFrame != mRenderStats.mRenderFramesCounter)
            continue;

        gameObject->DebugDraw(debugRender);
    }
}

void GameMapRenderer::DrawCityMesh(GameCamera& gameCamera)
{
    RenderStates cityMeshRenderStates;

    gSystem.mGfxDevice.SetRenderStates(cityMeshRenderStates);

    gGame.mRenderMng.mCityMeshProgram.Activate();
    gGame.mRenderMng.mCityMeshProgram.UploadCameraTransformMatrices(gameCamera);

    if (mCityMeshBufferV && mCityMeshBufferI)
    {
        gSystem.mGfxDevice.BindVertexBuffer(mCityMeshBufferV, CityVertex3D_Format::Get());
        gSystem.mGfxDevice.BindIndexBuffer(mCityMeshBufferI);
        gSystem.mGfxDevice.BindTexture(eTextureUnit_0, gGame.mSpritesMng.mBlocksTextureArray);
        gSystem.mGfxDevice.BindTexture(eTextureUnit_1, gGame.mSpritesMng.mBlocksIndicesTable);

        for (const MapBlocksChunk& currChunk: mMapBlocksChunks)
        {
            if (!gameCamera.mFrustum.contains(currChunk.mBounds))
                continue;

            gSystem.mGfxDevice.RenderIndexedPrimitives(ePrimitiveType_Triangles, eIndicesType_i32, 
                currChunk.mIndicesStart * Sizeof_DrawIndex, currChunk.mIndicesCount);

            ++mRenderStats.mBlockChunksDrawnCount;
        }
    }
    gGame.mRenderMng.mCityMeshProgram.Deactivate();
}

void GameMapRenderer::BuildMapMesh()
{
    CityMeshData blocksMesh;
    for (int batchy = 0; batchy < BlocksBatchesPerSide; ++batchy)
    {
        for (int batchx = 0; batchx < BlocksBatchesPerSide; ++batchx)
        {
            Rect mapArea { 
                batchx * BlocksBatchDims - ExtraBlocksPerSide, 
                batchy * BlocksBatchDims - ExtraBlocksPerSide,
                BlocksBatchDims,
                BlocksBatchDims };

            unsigned int prevVerticesCount = blocksMesh.mBlocksVertices.size();
            unsigned int prevIndicesCount = blocksMesh.mBlocksIndices.size();

            MapBlocksChunk& currChunk = mMapBlocksChunks[batchy * BlocksBatchesPerSide + batchx];
            currChunk.mBounds.mMin = glm::vec3 { mapArea.x * METERS_PER_MAP_UNIT, 0.0f, mapArea.y * METERS_PER_MAP_UNIT };
            currChunk.mBounds.mMax = glm::vec3 { 
                (mapArea.x + mapArea.w) * METERS_PER_MAP_UNIT, MAP_LAYERS_COUNT * METERS_PER_MAP_UNIT, 
                (mapArea.y + mapArea.h) * METERS_PER_MAP_UNIT};

            currChunk.mVerticesStart = prevVerticesCount;
            currChunk.mIndicesStart = prevIndicesCount;
            
            // append new geometry
            GameMapHelpers::BuildMapMesh(gGame.mMap, gGame.mStyleData, mapArea, blocksMesh);
            
            currChunk.mVerticesCount = blocksMesh.mBlocksVertices.size() - prevVerticesCount;
            currChunk.mIndicesCount = blocksMesh.mBlocksIndices.size() - prevIndicesCount;
        }
    }

    // upload map geometry to video memory
    int totalVertexDataBytes = blocksMesh.mBlocksVertices.size() * Sizeof_CityVertex3D;
    int totalIndexDataBytes = blocksMesh.mBlocksIndices.size() * Sizeof_DrawIndex;

    // upload vertex data
    mCityMeshBufferV->Setup(eBufferUsage_Static, totalVertexDataBytes, nullptr);
    if (void* pdata = mCityMeshBufferV->Lock(BufferAccess_Write))
    {
        memcpy(pdata, blocksMesh.mBlocksVertices.data(), totalVertexDataBytes);
        mCityMeshBufferV->Unlock();
    }

    // upload index data
    mCityMeshBufferI->Setup(eBufferUsage_Static, totalIndexDataBytes, nullptr);
    if (void* pdata = mCityMeshBufferI->Lock(BufferAccess_Write))
    {
        memcpy(pdata, blocksMesh.mBlocksIndices.data(), totalIndexDataBytes);
        mCityMeshBufferI->Unlock();
    }
}