#pragma once

#include "GameDefs.h"

class PixelsArray;

// this class holds gta style data which get loaded from G24-files
class CityStyleData final
{
public: 
    CityStyleData();
    ~CityStyleData();

    // Load style data from specific file, returns false on error
    // @param stylesName: Target file name
    bool LoadFromFile(const char* stylesName);

    // Free currently loaded style data
    void Cleanup();

    // Test whether style data was loaded
    bool IsLoaded() const;

    // Read block bitmap to specific location at target texture
    // Block bitmap has fixed dimensions (GTA_BLOCK_TEXTURE_DIMS x GTA_BLOCK_TEXTURE_DIMS)
    // @param blockType: Source block area type
    // @param blockIndex: Source block index
    // @param pixelsArray: Target bitmap, must be created
    // @param destPositionX, destPositionY: Location within destination texture where block will be placed
    bool GetBlockTexture(eBlockType blockType, int blockIndex, PixelsArray* pixelsArray, int destPositionX, int destPositionY);

    // Get number of textures total or for specific block type only
    // @param blockType: Block type
    int GetBlockTexturesCount(eBlockType blockType) const;
    int GetBlockTexturesCount() const;

    // Convert block type and relative index to absolute block index
    // @param blockType: Block type
    // @param blockIndex: Source relative index
    int GetBlockTextureLinearIndex(eBlockType blockType, int blockIndex) const;
    
    // Read animation info for specified block or returns false if block does not have any
    // @param blockType: Source block area type
    // @param blockIndex: Target block index
    // @param animationInfo: Output info
    bool GetBlockAnimationInfo(eBlockType blockType, int blockIndex, BlockAnimationStyleData* animationInfo);

    // Read gta car info
    // @param index: Index of car, using to enumerate all available entries
    // @param carInfo: Output info
    bool GetCarClassInfo(int index, CarStyleData* carInfo);

    // Read gta object info
    // @param index: Index of object, using to enumerate all available entries
    // @param objectInfo: Output info
    bool GetObjectInfo(int index, MapObjectStyleData* objectInfo);

    // Read gta sprite info, actual bitmap does not included
    // @param index: Index of object, using to enumerate all available entries
    // @param spriteInfo: Output info
    bool GetSpriteInfo(int index, SpriteStyleData* spriteInfo);

    // Read sprite bitmap to specific location at target texture
    // @param spriteIndex: Sprite index
    // @param pixelsArray: Target bitmap, must be created
    // @param destPositionX, destPositionY: Location within destination texture where block will be placed
    bool GetSpriteTexture(int spriteIndex, PixelsArray* pixelsArray, int destPositionX, int destPositionY);

    // Map sprite type and id pair to sprite index
    // @param spriteType: Sprite type
    // @para spriteId: Sprite id
    int GetSpriteIndex(eSpriteType spriteType, int spriteId) const;

private:
    // Reading style data internals
    // @param file: Source stream
    bool ReadBlockTextures(std::ifstream& file);
    bool ReadCLUTs(std::ifstream& file, int dataLength);
    bool ReadPaletteIndices(std::ifstream& file, int dataLength);
    bool ReadAnimations(std::ifstream& file, int dataLength);
    bool ReadObjects(std::ifstream& file, int dataLength);
    bool ReadCars(std::ifstream& file, int dataLength);
    bool ReadSprites(std::ifstream& file, int dataLength);
    bool ReadSpriteGraphics(std::ifstream& file, int dataLength);
    bool ReadSpriteNumbers(std::ifstream& file, int dataLength);

private:
    std::vector<unsigned char> mBlockTexturesRaw;
    std::vector<unsigned char> mSpriteGraphicsRaw;
    std::vector<unsigned short> mPaletteIndices;
    std::vector<Palette256> mPalettes;
    std::vector<BlockAnimationStyleData> mBlocksAnimations;
    std::vector<CarStyleData> mCars;
    std::vector<MapObjectStyleData> mObjects;
    std::vector<SpriteStyleData> mSprites;
    int mTileClutSize, mSpriteClutSize, mRemapClutSize, mFontClutSize;
    int mSideBlocksCount, mLidBlocksCount, mAuxBlocksCount;
    int mSpriteNumbers[eSpriteType_COUNT];
};