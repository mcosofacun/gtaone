#include "stdafx.h"
#include "SpriteManager.h"
#include "GpuTextureArray2D.h"
#include "GpuTexture2D.h"
#include "GraphicsDevice.h"
#include "GtaOneGame.h"
#include "stb_rect_pack.h"
#include "GameCheatsWindow.h"
#include "MemoryManager.h"

const int ObjectsTextureSizeX = 2048;
const int ObjectsTextureSizeY = 1024;
const int SpritesSpacing = 4;

SpriteManager gSpriteManager;

bool SpriteManager::InitLevelSprites()
{
    Cleanup();
    debug_assert(gGameMap.mStyleData.IsLoaded());

    if (!InitBlocksTexture())
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot create blocks texture");
        return false;
    }

    if (!InitBlocksIndicesTable())
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot initialize blocks indices table texture");
        return false;
    }

    if (!InitObjectsSpritesheet())
    {
        gConsole.LogMessage(eLogMessage_Warning, "Cannot create objects spritesheet");
        return false;
    }

    InitPalettesTable();
    InitBlocksAnimations();
    InitExplosionFrames();
    return true;
}

void SpriteManager::Cleanup()
{
    FlushSpritesCache();
    DestroySpriteTextures();
    FreeExplosionFrames();
    mIndicesTableChanged = false;
    if (mBlocksTextureArray)
    {
        gGraphicsDevice.DestroyTexture(mBlocksTextureArray);
        mBlocksTextureArray = nullptr;
    }

    if (mBlocksIndicesTable)
    {
        gGraphicsDevice.DestroyTexture(mBlocksIndicesTable);
        mBlocksIndicesTable = nullptr;
    }

    if (mObjectsSpritesheet.mSpritesheetTexture)
    {
        gGraphicsDevice.DestroyTexture(mObjectsSpritesheet.mSpritesheetTexture);
        mObjectsSpritesheet.mSpritesheetTexture = nullptr;
    }

    if (mPalettesTable)
    {
        gGraphicsDevice.DestroyTexture(mPalettesTable);
        mPalettesTable = nullptr;
    }

    if (mPaletteIndicesTable)
    {
        gGraphicsDevice.DestroyTexture(mPaletteIndicesTable);
        mPaletteIndicesTable = nullptr;
    }

    mBlocksIndices.clear();
    mBlocksAnimations.clear();
    mObjectsSpritesheet.mEntries.clear();
}

bool SpriteManager::InitObjectsSpritesheet()
{
    StyleData& cityStyle = gGameMap.mStyleData;

    int totalSprites = cityStyle.mSprites.size();
    debug_assert(totalSprites > 0);
    if (totalSprites == 0)
    {
        gConsole.LogMessage(eLogMessage_Warning, "Skip building objects atlas");
        return true;
    }

    debug_assert(ObjectsTextureSizeX > 0);
    debug_assert(ObjectsTextureSizeY > 0);

    mObjectsSpritesheet.mSpritesheetTexture = gGraphicsDevice.CreateTexture2D(eTextureFormat_R8UI, ObjectsTextureSizeX, ObjectsTextureSizeY, nullptr);
    debug_assert(mObjectsSpritesheet.mSpritesheetTexture);

    if (mObjectsSpritesheet.mSpritesheetTexture == nullptr)
        return false;

    mObjectsSpritesheet.mEntries.resize(totalSprites);

    // allocate temporary bitmap
    PixelsArray spritesBitmap;
    if (!spritesBitmap.Create(eTextureFormat_R8UI, ObjectsTextureSizeX, ObjectsTextureSizeY, gMemoryManager.mFrameHeapAllocator))
    {
        debug_assert(false);
        return false;
    }

    spritesBitmap.FillWithColor(0);

    // detect total layers count
    std::vector<stbrp_node> stbrp_nodes(ObjectsTextureSizeX);
    std::vector<stbrp_rect> stbrp_rects(totalSprites);

    // prepare sprites
    for (int isprite = 0, icurr = 0; isprite < totalSprites; ++isprite)
    {
        stbrp_rects[icurr].id = isprite;
        stbrp_rects[icurr].w = cityStyle.mSprites[isprite].mWidth + SpritesSpacing;
        stbrp_rects[icurr].h = cityStyle.mSprites[isprite].mHeight + SpritesSpacing;
        stbrp_rects[icurr].was_packed = 0;
        ++icurr;
    }

    float tcx = 1.0f / ObjectsTextureSizeX;
    float tcy = 1.0f / ObjectsTextureSizeY;

    // pack sprites
    bool all_done = false;
    {
		stbrp_context context;
		stbrp_init_target(&context, ObjectsTextureSizeX, ObjectsTextureSizeY, stbrp_nodes.data(), stbrp_nodes.size());
		all_done = stbrp_pack_rects(&context, stbrp_rects.data(), stbrp_rects.size()) > 0;

        // write sprites to temporary bitmap
        int numPacked = 0;
        for (const stbrp_rect& curr_rc: stbrp_rects)
        {
            if (curr_rc.was_packed == 0)
                continue;

            ++numPacked;
            if (!cityStyle.GetSpriteTexture(curr_rc.id, &spritesBitmap, curr_rc.x, curr_rc.y))
            {
                debug_assert(false);
                return false;
            }

            TextureRegion& spritesheetRecord = mObjectsSpritesheet.mEntries[curr_rc.id];
            spritesheetRecord.mRectangle.x = curr_rc.x;
            spritesheetRecord.mRectangle.y = curr_rc.y;
            spritesheetRecord.mRectangle.w = curr_rc.w - SpritesSpacing;
            spritesheetRecord.mRectangle.h = curr_rc.h - SpritesSpacing;
            spritesheetRecord.mU0 = spritesheetRecord.mRectangle.x * tcx;
            spritesheetRecord.mV0 = spritesheetRecord.mRectangle.y * tcy;
            spritesheetRecord.mU1 = (spritesheetRecord.mRectangle.x + spritesheetRecord.mRectangle.w) * tcx;
            spritesheetRecord.mV1 = (spritesheetRecord.mRectangle.y + spritesheetRecord.mRectangle.h) * tcy;
        }

        if (numPacked == 0)
        {
            debug_assert(false);
            return false;
        }

        // upload to texture
        if (!mObjectsSpritesheet.mSpritesheetTexture->Upload(spritesBitmap.mData))
        {
            debug_assert(false);
        }
    }
    debug_assert(all_done);
    return all_done;
}

bool SpriteManager::InitBlocksTexture()
{
    StyleData& cityStyle = gGameMap.mStyleData;
    // count textures
    const int totalTextures = cityStyle.GetBlockTexturesCount();
    assert(totalTextures > 0);
    if (totalTextures == 0)
    {
        gConsole.LogMessage(eLogMessage_Warning, "Skip building blocks atlas");
        return true;
    }

    // allocate temporary bitmap
    PixelsArray blockBitmap;
    if (!blockBitmap.Create(eTextureFormat_R8, MAP_BLOCK_TEXTURE_DIMS, MAP_BLOCK_TEXTURE_DIMS, gMemoryManager.mFrameHeapAllocator))
    {
        debug_assert(false);
        return false;
    }

    mBlocksTextureArray = gGraphicsDevice.CreateTextureArray2D(eTextureFormat_R8UI, blockBitmap.mSizex, blockBitmap.mSizey, totalTextures, nullptr);
    debug_assert(mBlocksTextureArray);
    
    int currentLayerIndex = 0;
    for (int iblockType = 0; iblockType < eBlockType_COUNT; ++iblockType)
    {
        int numTextures = cityStyle.GetBlockTexturesCount((eBlockType) iblockType);
        for (int itexture = 0; itexture < numTextures; ++itexture)
        {
            if (!cityStyle.GetBlockTexture((eBlockType) iblockType, itexture, &blockBitmap, 0, 0, 0))
            {
                gConsole.LogMessage(eLogMessage_Warning, "Cannot read block texture: %d %d", iblockType, itexture);
                return false;
            }

            // upload bitmap to gpu
            if (!mBlocksTextureArray->Upload(currentLayerIndex, 1, blockBitmap.mData))
            {
                debug_assert(false);
            }
            
            ++currentLayerIndex;
        }
    }
    return true;
}

bool SpriteManager::InitBlocksIndicesTable()
{
    StyleData& cityStyle = gGameMap.mStyleData;

    // count textures
    const int totalTextures = cityStyle.GetBlockTexturesCount();
    assert(totalTextures > 0);
    if (totalTextures == 0)
    {
        gConsole.LogMessage(eLogMessage_Warning, "Skip building blocks indices table");
        return true;
    }

    mBlocksIndices.resize(totalTextures);

    // reset to default order
    for (int i = 0; i < totalTextures; ++i)
    {
        mBlocksIndices[i] = i;
    }

    int textureWidth = cxx::get_next_pot(mBlocksIndices.size());
    mBlocksIndicesTable = gGraphicsDevice.CreateTexture2D(eTextureFormat_R16UI, textureWidth, 1, nullptr);
    debug_assert(mBlocksIndicesTable);
    if (mBlocksIndicesTable)
    {
        mBlocksIndicesTable->Upload(0, 0, 0, mBlocksIndices.size(), 1, mBlocksIndices.data());
    }
    return true;
}

void SpriteManager::InitPalettesTable()
{
    StyleData& cityStyle = gGameMap.mStyleData;

    int textureHeight = cxx::get_next_pot(cityStyle.mPalettes.size());

    mPalettesTable = gGraphicsDevice.CreateTexture2D(eTextureFormat_RGBA8, 256, textureHeight, nullptr); 
    debug_assert(mPalettesTable);
    // upload texels
    mPalettesTable->Upload(0, 0, 0, 256, cityStyle.mPalettes.size(), cityStyle.mPalettes.data());

    int textureWidth = cxx::get_next_pot(cityStyle.mPaletteIndices.size());
    mPaletteIndicesTable = gGraphicsDevice.CreateTexture2D(eTextureFormat_R16UI, textureWidth, 1, nullptr);
    debug_assert(mPaletteIndicesTable);
    if (mPaletteIndicesTable)
    {
        mPaletteIndicesTable->Upload(0, 0, 0, 
            cityStyle.mPaletteIndices.size(), 1, 
            cityStyle.mPaletteIndices.data());
    }
}

void SpriteManager::RenderFrameBegin()
{

}

void SpriteManager::RenderFrameEnd()
{
    if (mIndicesTableChanged)
    {
        // upload indices table
        debug_assert(mBlocksIndicesTable);
        mIndicesTableChanged = false;
        mBlocksIndicesTable->Upload(0, 0, 0, mBlocksIndices.size(), 1, mBlocksIndices.data());
    }
}

void SpriteManager::InitBlocksAnimations()
{
    StyleData& cityStyle = gGameMap.mStyleData;

    mBlocksAnimations.clear();
    for (const BlockAnimationInfo& currAnim: cityStyle.mBlocksAnimations)
    {
        BlockAnimation animData;
        animData.mBlockIndex = cityStyle.GetBlockTextureLinearIndex((currAnim.mWhich == 0 ? eBlockType_Side : eBlockType_Lid), currAnim.mBlock);
        animData.mAnimDesc.mFrameRate = (GTA_CYCLES_PER_FRAME * 1.0f) / currAnim.mSpeed;
        animData.mAnimDesc.SetFrames(animData.mBlockIndex, currAnim.mFrameCount + 1);
        for (int iframe = 0; iframe < currAnim.mFrameCount; ++iframe)
        {   
            // convert to linear indices
            SpriteAnimFrame& animFrame = animData.mAnimDesc.mFrames[iframe + 1];
            animFrame.mSprite = cityStyle.GetBlockTextureLinearIndex(eBlockType_Aux, currAnim.mFrames[iframe]);
        }
        animData.PlayAnimation(eSpriteAnimLoop_FromStart);
        mBlocksAnimations.push_back(animData);
    }
}

void SpriteManager::UpdateBlocksAnimations(float deltaTime)
{
    if (!gGameCheatsWindow.mEnableBlocksAnimation)
        return;

    for (BlockAnimation& currAnim: mBlocksAnimations)
    {
        if (!currAnim.UpdateFrame(deltaTime))
            continue;
        mBlocksIndices[currAnim.mBlockIndex] = currAnim.GetSpriteIndex(); // patch table
        mIndicesTableChanged = true;
    }
}

void SpriteManager::DumpBlocksTexture(const std::string& outputLocation)
{
    StyleData& cityStyle = gGameMap.mStyleData;

    debug_assert(cityStyle.IsLoaded());
    cxx::ensure_path_exists(outputLocation);
    // allocate temporary bitmap
    PixelsArray blockBitmap;
    if (!blockBitmap.Create(eTextureFormat_RGBA8, MAP_BLOCK_TEXTURE_DIMS, MAP_BLOCK_TEXTURE_DIMS, gMemoryManager.mFrameHeapAllocator))
    {
        debug_assert(false);
        return;
    }
    std::string pathBuffer;
    for (int iblockType = 0; iblockType < eBlockType_COUNT; ++iblockType)
    {
        eBlockType currentBlockType = (eBlockType) iblockType;

        int numTextures = cityStyle.GetBlockTexturesCount(currentBlockType);
        for (int itexture = 0; itexture < numTextures; ++itexture)
        {
            if (!cityStyle.GetBlockTexture(currentBlockType, itexture, &blockBitmap, 0, 0, 0))
            {
                gConsole.LogMessage(eLogMessage_Warning, "Cannot read block texture: %d %d", iblockType, itexture);
                continue;
            }
            
            // dump to file
            pathBuffer = cxx::va("%s/%s_%d.png", outputLocation.c_str(), cxx::enum_to_string(currentBlockType), itexture);
            if (!blockBitmap.SaveToFile(pathBuffer))
            {
                debug_assert(false);
            }
        }
    } // for
}

void SpriteManager::DumpSpriteTextures(const std::string& outputLocation)
{
    StyleData& cityStyle = gGameMap.mStyleData;

    debug_assert(cityStyle.IsLoaded());
    cxx::ensure_path_exists(outputLocation);
    std::string pathBuffer;
    for (int iSpriteType = 0; iSpriteType < eSpriteType_COUNT; ++iSpriteType)
    {
        eSpriteType sprite_type = (eSpriteType) iSpriteType;
        for (int iSpriteId = 0; iSpriteId < cityStyle.GetNumSprites(sprite_type); ++iSpriteId)
        {
            int sprite_index = cityStyle.GetSpriteIndex(sprite_type, iSpriteId);

            PixelsArray spriteBitmap;
            spriteBitmap.Create(eTextureFormat_RGBA8, 
                cityStyle.mSprites[sprite_index].mWidth, 
                cityStyle.mSprites[sprite_index].mHeight, gMemoryManager.mFrameHeapAllocator);
            cityStyle.GetSpriteTexture(sprite_index, &spriteBitmap, 0, 0);
            
            // dump to file
            pathBuffer = cxx::va("%s/%s_%d.png", outputLocation.c_str(), cxx::enum_to_string(sprite_type), iSpriteId);
            if (!spriteBitmap.SaveToFile(pathBuffer))
            {
                debug_assert(false);
            }
        }
    } // for
}

void SpriteManager::DumpCarsTextures(const std::string& outputLocation)
{
    StyleData& cityStyle = gGameMap.mStyleData;

    debug_assert(cityStyle.IsLoaded());
    cxx::ensure_path_exists(outputLocation);
    std::string pathBuffer;
    for (const VehicleInfo& currCar: cityStyle.mVehicles)
    {
        int sprite_index = currCar.mSpriteIndex;

        PixelsArray spriteBitmap;
        spriteBitmap.Create(eTextureFormat_RGBA8, 
            cityStyle.mSprites[sprite_index].mWidth, 
            cityStyle.mSprites[sprite_index].mHeight, gMemoryManager.mFrameHeapAllocator);
        cityStyle.GetSpriteTexture(sprite_index, &spriteBitmap, 0, 0);
            
        // dump to file
        pathBuffer = cxx::va("%s/%d-%s-%s.png", outputLocation.c_str(), currCar.mModelID, 
            cxx::enum_to_string(currCar.mModelID), 
            cxx::enum_to_string(currCar.mClassID));

        if (!spriteBitmap.SaveToFile(pathBuffer))
        {
            debug_assert(false);
        }
    } // for
}

void SpriteManager::DumpSpriteDeltas(const std::string& outputLocation)
{
    StyleData& cityStyle = gGameMap.mStyleData;

    debug_assert(cityStyle.IsLoaded());
    cxx::ensure_path_exists(outputLocation);
    std::string pathBuffer;

    for (int isprite = 0, Num = gGameMap.mStyleData.mSprites.size(); isprite < Num; ++isprite)
    {
        SpriteInfo& sprite = gGameMap.mStyleData.mSprites[isprite];

        PixelsArray spriteBitmap;
        spriteBitmap.Create(eTextureFormat_RGBA8, sprite.mWidth, sprite.mHeight, gMemoryManager.mFrameHeapAllocator);
        for (int idelta = 0; idelta < sprite.mDeltaCount; ++idelta)
        {
            if (!cityStyle.GetSpriteTexture(isprite, BIT(idelta), &spriteBitmap, 0, 0))
            {
                debug_assert(false);
            }

            // dump to file
            pathBuffer = cxx::va("%s/sprite_%d_delta_%d.png", outputLocation.c_str(), isprite, idelta);
            if (!spriteBitmap.SaveToFile(pathBuffer))
            {
                debug_assert(false);
            }
        }
    } // for
}

void SpriteManager::DumpSpriteDeltas(const std::string& outputLocation, int spriteIndex)
{
    StyleData& cityStyle = gGameMap.mStyleData;

    debug_assert(cityStyle.IsLoaded());
    cxx::ensure_path_exists(outputLocation);
    std::string pathBuffer;

    SpriteInfo& sprite = cityStyle.mSprites[spriteIndex];

    PixelsArray spriteBitmap;
    spriteBitmap.Create(eTextureFormat_RGBA8, sprite.mWidth, sprite.mHeight, gMemoryManager.mFrameHeapAllocator);
    for (int idelta = 0; idelta < sprite.mDeltaCount; ++idelta)
    {
        if (!cityStyle.GetSpriteTexture(spriteIndex, BIT(idelta), &spriteBitmap, 0, 0))
        {
            debug_assert(false);
        }

        // dump to file
        pathBuffer = cxx::va("%s/sprite_%d_delta_%d.png", outputLocation.c_str(), spriteIndex, idelta);
        if (!spriteBitmap.SaveToFile(pathBuffer))
        {
            debug_assert(false);
        }
    } // for
}

void SpriteManager::FlushSpritesCache()
{
    // move all textures to pool
    for (SpriteCacheElement& currElement: mSpritesCache)
    {
        mFreeSpriteTextures.push_back(currElement.mTexture);
    }

    mSpritesCache.clear();
}

void SpriteManager::FlushSpritesCache(GameObjectID objectID)
{
    for (auto icurrent = mSpritesCache.begin(); icurrent != mSpritesCache.end(); )
    {
        if (icurrent->mObjectID == objectID)
        {
            // move texture to pool
            mFreeSpriteTextures.push_back(icurrent->mTexture);

            icurrent = mSpritesCache.erase(icurrent);
            continue;
        }
        ++icurrent;
    }
}

void SpriteManager::DestroySpriteTextures()
{
    for (GpuTexture2D* currTexture: mFreeSpriteTextures)
    {
        gGraphicsDevice.DestroyTexture(currTexture);
    }
    mFreeSpriteTextures.clear();
}

void SpriteManager::GetSpriteTexture(GameObjectID objectID, int spriteIndex, int remap, SpriteDeltaBits deltaBits, Sprite2D& sourceSprite)
{
    sourceSprite.mTexture = nullptr;
    if (deltaBits == 0)
    {
        GetSpriteTexture(objectID, spriteIndex, remap, sourceSprite);
        return;
    }

    debug_assert(spriteIndex < (int) mObjectsSpritesheet.mEntries.size());

    // filter out present delta bits
    SpriteInfo& spriteStyle = gGameMap.mStyleData.mSprites[spriteIndex];
    deltaBits &= spriteStyle.GetDeltaBits();

    debug_assert(remap >= 0);
    sourceSprite.mPaletteIndex = gGameMap.mStyleData.GetSpritePaletteIndex(spriteStyle.mClut, remap);

    if (deltaBits == 0)
    {
        GetSpriteTexture(objectID, spriteIndex, remap, sourceSprite);
        return;
    }

    // find sprite with deltas within cache
    for (SpriteCacheElement& currElement: mSpritesCache)
    {
        if (currElement.mObjectID == objectID && currElement.mSpriteIndex == spriteIndex)
        {
            if (currElement.mSpriteDeltaBits == deltaBits)
            {
                sourceSprite.mTexture = currElement.mTexture;
                sourceSprite.mTextureRegion = currElement.mTextureRegion;
                return;
            }
            currElement.mSpriteDeltaBits = deltaBits;

            // upload changes
            PixelsArray pixels;
            if (!pixels.Create(currElement.mTexture->mFormat, 
                currElement.mTexture->mSize.x, 
                currElement.mTexture->mSize.y, gMemoryManager.mFrameHeapAllocator))
            {
                debug_assert(false);
            }

            if (!gGameMap.mStyleData.GetSpriteTexture(spriteIndex, deltaBits, &pixels, 0, 0))
            {
                debug_assert(false);
            }
            sourceSprite.mTextureRegion = currElement.mTextureRegion;
            sourceSprite.mTexture = currElement.mTexture;
            sourceSprite.mTexture->Upload(pixels.mData);
            return;
        }
    }
    
    // cache miss
    Point dimensions;
    dimensions.x = cxx::get_next_pot(spriteStyle.mWidth);
    dimensions.y = cxx::get_next_pot(spriteStyle.mHeight);

    sourceSprite.mTexture = GetFreeSpriteTexture(dimensions, eTextureFormat_R8UI);
    if (sourceSprite.mTexture == nullptr)
    {
        debug_assert(false);
    }

    PixelsArray pixels;
    if (!pixels.Create(eTextureFormat_R8UI, dimensions.x, dimensions.y, 
        gMemoryManager.mFrameHeapAllocator))
    {
        debug_assert(false);
    }

    pixels.FillWithCheckerBoard();

    // combine soruce image with deltas
    if (!gGameMap.mStyleData.GetSpriteTexture(spriteIndex, deltaBits, &pixels, 0, 0))
    {
        debug_assert(false);
    }

    // upload to texture
    sourceSprite.mTexture->Upload(pixels.mData);

    Rect srcRect;
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = spriteStyle.mWidth;
    srcRect.h = spriteStyle.mHeight;

    sourceSprite.mTextureRegion.SetRegion(srcRect, dimensions);

    // add to sprites cache
    SpriteCacheElement spriteCacheElement;
    spriteCacheElement.mObjectID = objectID;
    spriteCacheElement.mSpriteIndex = spriteIndex;
    spriteCacheElement.mSpriteDeltaBits = deltaBits;
    spriteCacheElement.mTexture = sourceSprite.mTexture;
    spriteCacheElement.mTextureRegion = sourceSprite.mTextureRegion;

    mSpritesCache.push_back(spriteCacheElement);
}

void SpriteManager::GetSpriteTexture(GameObjectID objectID, int spriteIndex, int remap, Sprite2D& sourceSprite)
{
    debug_assert(remap >= 0);

    debug_assert(spriteIndex < (int) mObjectsSpritesheet.mEntries.size());
    SpriteInfo& spriteStyle = gGameMap.mStyleData.mSprites[spriteIndex];

    sourceSprite.mPaletteIndex = gGameMap.mStyleData.GetSpritePaletteIndex(spriteStyle.mClut, remap);
    sourceSprite.mTexture = mObjectsSpritesheet.mSpritesheetTexture;
    sourceSprite.mTextureRegion = mObjectsSpritesheet.mEntries[spriteIndex];
}

GpuTexture2D* SpriteManager::GetFreeSpriteTexture(const Point& dimensions, eTextureFormat format)
{
    for (auto icurr = mFreeSpriteTextures.begin(); icurr != mFreeSpriteTextures.end(); )
    {
        GpuTexture2D* currTexture = *icurr;
        if (currTexture->mSize == dimensions && currTexture->mFormat == format)
        {
            mFreeSpriteTextures.erase(icurr);
            return currTexture;
        }
        ++icurr;
    }

    GpuTexture2D* texture = gGraphicsDevice.CreateTexture2D(format, dimensions.x, dimensions.y, nullptr);
    return texture;
}

void SpriteManager::InitExplosionFrames()
{
    StyleData& cityStyle = gGameMap.mStyleData;

    int explosionSpriteIndex = cityStyle.GetSpriteIndex(eSpriteType_Ex, 0);
    int explosionSpritesCount = cityStyle.GetNumSprites(eSpriteType_Ex);

    debug_assert(cxx::is_even(explosionSpritesCount));

    int framesCount = (explosionSpritesCount / 4); // single explosion frame consists of four smaller parts
    if (framesCount < 1)
        return;

    SpriteInfo& sprite = cityStyle.mSprites[explosionSpriteIndex];

    int textureSizex = sprite.mWidth * 2;
    int textureSizey = sprite.mHeight * 2;

    PixelsArray pixels;
    if (!pixels.Create(eTextureFormat_R8UI, textureSizex, textureSizey, 
        gMemoryManager.mFrameHeapAllocator))
    {
        debug_assert(false);
        return;
    }

    for (int iframe = 0; iframe < framesCount; ++iframe)
    {
        // copy data
        for (int ipiece = 0; ipiece < 4; ++ipiece)
        {
            int destx = (ipiece % 2);
            int desty = (ipiece / 2);

            int currSpriteIndex = explosionSpriteIndex + iframe + (ipiece * framesCount);
            if (!cityStyle.GetSpriteTexture(currSpriteIndex, &pixels, sprite.mWidth * destx, sprite.mHeight * desty))
            {
                debug_assert(false);
            }
        }

        // upload pixels to gpu
        GpuTexture2D* texture = gGraphicsDevice.CreateTexture2D(pixels.mFormat, pixels.mSizex, pixels.mSizey, pixels.mData);
        if (texture)
        {
            mExplosionFrames.push_back(texture);
            continue;
        }
        debug_assert(texture);
    }
    
    mExplosionPaletteIndex = cityStyle.GetSpritePaletteIndex(sprite.mClut, 0);
}

void SpriteManager::FreeExplosionFrames()
{
    for (GpuTexture2D* currTexure: mExplosionFrames)
    {
        gGraphicsDevice.DestroyTexture(currTexure);
    }
    mExplosionFrames.clear();
}

bool SpriteManager::GetExplosionTexture(int frameIndex, Sprite2D& sourceSprite) const
{
    int framesCount = GetExplosionFramesCount();
    if (frameIndex < framesCount)
    {
        sourceSprite.mPaletteIndex = mExplosionPaletteIndex;
        sourceSprite.mTexture = mExplosionFrames[frameIndex];
        sourceSprite.mTextureRegion.SetRegion(sourceSprite.mTexture->mSize);
        return true;
    }
    return false;
}

int SpriteManager::GetExplosionFramesCount() const
{
    return (int) mExplosionFrames.size();
}