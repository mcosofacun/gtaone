#include "stdafx.h"
#include "SpriteBatch.h"
#include "RenderManager.h"
#include "SpriteManager.h"
#include "GpuTexture2D.h"

const unsigned int NumVerticesPerSprite = 4;
const unsigned int NumIndicesPerSprite = 6;

bool SpriteBatch::Initialize()
{
    mSpritesList.reserve(1024);
    return true;
}

void SpriteBatch::Deinit()
{
    mTrimeshBuffer.Deinit();
    Clear();
}

void SpriteBatch::Clear()
{
    mSpritesList.clear();
    mDrawVertices.clear();
    mDrawIndices.clear();
    mBatchesList.clear();
}

void SpriteBatch::DrawSprite(const Sprite2D& sourceSprite)
{
    if (sourceSprite.mTexture == nullptr)
    {
        cxx_assert(false);
        return;
    }
    mSpritesList.push_back(sourceSprite);
}

void SpriteBatch::Flush()
{
    if (!mSpritesList.empty())
    {
        SortSprites();
        GenerateSpritesBatches();
        RenderSpritesBatches();
    }
    Clear();
}

void SpriteBatch::GenerateSpritesBatches()
{
    int numSprites = mSpritesList.size();

    int totalVertexCount = numSprites * NumVerticesPerSprite; 
    cxx_assert(totalVertexCount > 0);

    int totalIndexCount = numSprites * NumIndicesPerSprite; 
    cxx_assert(totalIndexCount > 0);

    // allocate memory for mesh data
    mDrawVertices.resize(totalVertexCount);
    SpriteVertex3D* vertexData = mDrawVertices.data();

    mDrawIndices.resize(totalIndexCount);
    DrawIndex* indexData = mDrawIndices.data();

    // initial batch
    mBatchesList.clear();
    mBatchesList.emplace_back();
    DrawSpriteBatch* currentBatch = &mBatchesList.back();
    currentBatch->mFirstVertex = 0;
    currentBatch->mFirstIndex = 0;
    currentBatch->mVertexCount = 0;
    currentBatch->mIndexCount = 0;
    currentBatch->mSpriteTexture = mSpritesList[0].mTexture;

    for (int isprite = 0; isprite < numSprites; ++isprite)
    {
        const Sprite2D& sprite = mSpritesList[isprite];
        // start new batch
        if (sprite.mTexture != currentBatch->mSpriteTexture)
        {
            DrawSpriteBatch newBatch;
            newBatch.mFirstVertex = currentBatch->mVertexCount + currentBatch->mFirstVertex;
            newBatch.mFirstIndex = currentBatch->mIndexCount + currentBatch->mFirstIndex;
            newBatch.mVertexCount = 0;
            newBatch.mIndexCount = 0;
            newBatch.mSpriteTexture = sprite.mTexture;
            mBatchesList.push_back(newBatch);
            currentBatch = &mBatchesList.back();
        }

        currentBatch->mVertexCount += NumVerticesPerSprite;   
        currentBatch->mIndexCount += NumIndicesPerSprite;

        int vertexOffset = isprite * NumVerticesPerSprite;

        vertexData[vertexOffset + 0].mTexcoord.x = sprite.mTextureRegion.mU0;
        vertexData[vertexOffset + 0].mTexcoord.y = sprite.mTextureRegion.mV0;

        vertexData[vertexOffset + 1].mTexcoord.x = sprite.mTextureRegion.mU1;
        vertexData[vertexOffset + 1].mTexcoord.y = sprite.mTextureRegion.mV0;

        vertexData[vertexOffset + 2].mTexcoord.x = sprite.mTextureRegion.mU0;
        vertexData[vertexOffset + 2].mTexcoord.y = sprite.mTextureRegion.mV1;

        vertexData[vertexOffset + 3].mTexcoord.x = sprite.mTextureRegion.mU1;
        vertexData[vertexOffset + 3].mTexcoord.y = sprite.mTextureRegion.mV1;

        glm::vec2 positions[4];
        sprite.GetCorners(positions);

        if (mDepthAxis == DepthAxis_Y)
        {
            for (int i = 0; i < 4; ++i)
            {
                vertexData[vertexOffset + i].mPosition.x = positions[i].x;
                vertexData[vertexOffset + i].mPosition.z = positions[i].y;
                vertexData[vertexOffset + i].mPosition.y = sprite.mHeight;
                // common part
                vertexData[vertexOffset + i].mClutIndex = sprite.mPaletteIndex;
                vertexData[vertexOffset + i].mTextureSize[0] = sprite.mTexture->mSize.x;
                vertexData[vertexOffset + i].mTextureSize[1] = sprite.mTexture->mSize.y;
            }
        }
        else
        {
            for (int i = 0; i < 4; ++i)
            {
                vertexData[vertexOffset + i].mPosition.x = positions[i].x;
                vertexData[vertexOffset + i].mPosition.y = positions[i].y;
                vertexData[vertexOffset + i].mPosition.z = sprite.mHeight;
                // common part
                vertexData[vertexOffset + i].mClutIndex = sprite.mPaletteIndex;
                vertexData[vertexOffset + i].mTextureSize[0] = sprite.mTexture->mSize.x;
                vertexData[vertexOffset + i].mTextureSize[1] = sprite.mTexture->mSize.y;
            }
        }

        // setup indices
        int indexOffset = isprite * NumIndicesPerSprite;
        indexData[indexOffset + 0] = vertexOffset + 0;
        indexData[indexOffset + 1] = vertexOffset + 1;
        indexData[indexOffset + 2] = vertexOffset + 2;
        indexData[indexOffset + 3] = vertexOffset + 1;
        indexData[indexOffset + 4] = vertexOffset + 2;
        indexData[indexOffset + 5] = vertexOffset + 3;
    }
}

void SpriteBatch::RenderSpritesBatches()
{
    SpriteVertex3D_Format vFormat;
    mTrimeshBuffer.SetVertices(Sizeof_SpriteVertex3D * mDrawVertices.size(), mDrawVertices.data());
    mTrimeshBuffer.SetIndices(Sizeof_DrawIndex * mDrawIndices.size(), mDrawIndices.data());
    mTrimeshBuffer.Bind(vFormat);

    for (const DrawSpriteBatch& currBatch: mBatchesList)
    {       
        unsigned int idxBufferOffset = Sizeof_DrawIndex * currBatch.mFirstIndex;
        gSystem.mGfxDevice.BindTexture(eTextureUnit_0, currBatch.mSpriteTexture);
        gSystem.mGfxDevice.RenderIndexedPrimitives(ePrimitiveType_Triangles, eIndicesType_i32, idxBufferOffset, currBatch.mIndexCount);
    }
}

void SpriteBatch::BeginBatch(DepthAxis depthAxis, eSpritesSortMode sortMode)
{
    Clear();

    mDepthAxis = depthAxis;
    mSortMode = sortMode;
}

void SpriteBatch::SortSprites()
{
    if (mSortMode == eSpritesSortMode_None)
        return;

    if (mSortMode == eSpritesSortMode_Height)
    {
        static auto SortProc = [](const Sprite2D& lhs, const Sprite2D& rhs)
        {
            return lhs.mHeight < rhs.mHeight;
        };
        std::stable_sort(mSpritesList.begin(), mSpritesList.end(), SortProc);
        return;
    }

    if (mSortMode == eSpritesSortMode_DrawOrder)
    {
        static auto SortProc = [](const Sprite2D& lhs, const Sprite2D& rhs)
        {
            return lhs.mDrawOrder < rhs.mDrawOrder;
        };
        std::stable_sort(mSpritesList.begin(), mSpritesList.end(), SortProc);
        return;
    }

    if (mSortMode == eSpritesSortMode_HeightAndDrawOrder)
    {
        static auto SortProc = [](const Sprite2D& lhs, const Sprite2D& rhs)
        {
            if (lhs.mHeight != rhs.mHeight)
            {
                return (lhs.mHeight < rhs.mHeight);
            }
            return (lhs.mDrawOrder < rhs.mDrawOrder);
        };  
        std::stable_sort(mSpritesList.begin(), mSpritesList.end(), SortProc);
        return;
    }
}
