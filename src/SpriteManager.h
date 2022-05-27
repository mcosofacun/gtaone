#pragma once

#include "GameDefs.h"
#include "Sprite2D.h"
#include "StyleData.h"

// This class implements caching mechanism for graphic resources

// Since engine uses original GTA assets, cache requires styledata to be provided
// Some textures, such as block tiles, may be combined into huge atlases for performance reasons 
class SpriteManager final: public cxx::noncopyable
{
public:
    // animating blocks texture indices table
    GpuTexture2D* mBlocksIndicesTable = nullptr;

    GpuTexture2D* mPalettesTable = nullptr;
    GpuTexture2D* mPaletteIndicesTable = nullptr; // index of palette in global palettes table

    // all blocks are packed into single texture array, where each level is single 64x64 bitmap
    GpuTextureArray2D* mBlocksTextureArray = nullptr;

    // all default objects bitmaps (with no deltas applied) are stored in single 2d texture
    Spritesheet mObjectsSpritesheet;

public:
    // preload sprite textures for current level
    bool InitSprites(StyleData* styleData);

    // flush all currently cached sprites
    void Cleanup();

    void RenderFrameBegin();
    void RenderFrameEnd();

    void UpdateBlocksAnimations(float deltaTime);

    // force drop cached sprites
    // @param objectID: Specific object identifier
    void FlushSpritesCache();
    void FlushSpritesCache(GameObjectID objectID);

    // Create sprite texture with deltas specified
    // @param objectID: Game object that owns sprite or GAMEOBJECT_ID_NULL
    // @param spriteIndex: Sprite index, linear
    // @param deltaBits: Sprite delta bits
    // @param sourceSprite: Output sprite data
    void GetSpriteTexture(GameObjectID objectID, int spriteIndex, int remap, SpriteDeltaBits deltaBits, Sprite2D& sourceSprite);
    void GetSpriteTexture(GameObjectID objectID, int spriteIndex, int remap, Sprite2D& sourceSprite);

    // Get explosion sprite texture
    // @param frameIndex: Frame index
    // @param sourceSprite: Output sprite data
    bool GetExplosionTexture(int frameIndex, Sprite2D& sourceSprite) const;

    // Get explosion frames count
    int GetExplosionFramesCount() const;

    // save all blocks textures to hard drive
    void DumpBlocksTexture(const std::string& outputLocation);
    void DumpSpriteTextures(const std::string& outputLocation);
    void DumpSpriteDeltas(const std::string& outputLocation);
    void DumpSpriteDeltas(const std::string& outputLocation, int spriteIndex);
    void DumpCarsTextures(const std::string& outputLocation);

private:
    bool InitBlocksIndicesTable();
    bool InitBlocksTexture();
    bool InitObjectsSpritesheet();
    void InitPalettesTable();
    void InitBlocksAnimations();

    void InitExplosionFrames();
    void FreeExplosionFrames();

    // find texture with required size and format or create new if nothing found
    GpuTexture2D* GetFreeSpriteTexture(const Point& dimensions, eTextureFormat format);
    void DestroySpriteTextures();

private:
    // animation state for blocks sharing specific texture
    struct BlockAnimation: public SpriteAnimation
    {
    public:
        int mBlockIndex; // linear
    };

    std::vector<BlockAnimation> mBlocksAnimations;
    std::vector<unsigned short> mBlocksIndices;
    bool mIndicesTableChanged;

    StyleData* mStyleData = nullptr;

    // usused sprite textures
    std::vector<GpuTexture2D*> mFreeSpriteTextures;

    // explosion sprite is huge and it was originally split into four pieces, 
    // so it must be assembled in one piece again before use
    std::vector<GpuTexture2D*> mExplosionFrames;
    int mExplosionPaletteIndex = 0;

    // cached sprite textures with deltas
    struct SpriteCacheElement
    {
    public:
        GameObjectID mObjectID; // object identifier which this sprite belongs to
        int mSpriteIndex;
        SpriteDeltaBits mSpriteDeltaBits; // all deltas applied to this sprite
        GpuTexture2D* mTexture;
        TextureRegion mTextureRegion;
    };
    std::vector<SpriteCacheElement> mSpritesCache;
};
