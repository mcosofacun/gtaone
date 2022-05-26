#pragma once

#include "GameDefs.h"
#include "PhysicsDefs.h"

class Convert
{
public:
    // Convert map units to meters
    static float MapUnitsToMeters(float units) { return (units * METERS_PER_MAP_UNIT); }

    // Convert meters to map units
    static float MetersToMapUnits(float units) { return (units / METERS_PER_MAP_UNIT); }

    static float MetersToAudioUnits(float units) { return (units / METERS_PER_AUDIO_UNIT); }

    // Convert map units to meters
    static glm::vec2 MapUnitsToMeters(const glm::vec2& units) 
    {  
        return {
            MapUnitsToMeters(units.x),
            MapUnitsToMeters(units.y)
        };
    }
    static glm::vec3 MapUnitsToMeters(const glm::vec3& units) 
    {  
        return {
            MapUnitsToMeters(units.x),
            MapUnitsToMeters(units.y),
            MapUnitsToMeters(units.z)
        };
    }
    // Convert meters to map units
    static glm::vec2 MetersToMapUnits(const glm::vec2& units) 
    {  
        return {
            MetersToMapUnits(units.x),
            MetersToMapUnits(units.y)
        };
    }
    static glm::vec3 MetersToMapUnits(const glm::vec3& units) 
    {  
        return {
            MetersToMapUnits(units.x),
            MetersToMapUnits(units.y + 0.5f), // extra offset for height to get things work
            MetersToMapUnits(units.z)
        };
    }

    // Convert fix16 angle
    static cxx::angle_t Fix16ToAngle(unsigned short ang16)
    {
        return cxx::angle_t::from_degrees((ang16 / 1024.0f) * 360.0f - SPRITE_ZERO_ANGLE);
    }

    // Convert pixels to map units
    static float PixelsToMapUnits(int pixels) { return (1.0f * pixels) / PIXELS_PER_MAP_UNIT; }

    // Convert map units to pixels
    static int MapUnitsToPixels(float units) { return (int) (units * PIXELS_PER_MAP_UNIT); }

    // Convert pixels to map units
    static glm::vec2 PixelsToMapUnits(const glm::ivec2& pixels)
    {
        return {
            PixelsToMapUnits(pixels.x),
            PixelsToMapUnits(pixels.y)
        };
    }
    static glm::vec3 PixelsToMapUnits(const glm::ivec3& pixels)
    {
        return {
            PixelsToMapUnits(pixels.x),
            PixelsToMapUnits(pixels.y),
            PixelsToMapUnits(pixels.z)
        };
    }

    // Convert map units to pixels
    static glm::ivec2 MapUnitsToPixels(const glm::vec2& units)
    {
        return {
            MapUnitsToPixels(units.x),
            MapUnitsToPixels(units.y)
        };
    }
    static glm::ivec3 MapUnitsToPixels(const glm::vec3& units)
    {
        return {
            MapUnitsToPixels(units.x),
            MapUnitsToPixels(units.y),
            MapUnitsToPixels(units.z)
        };
    }

    // Convert pixels directly to meters
    static float PixelsToMeters(int pixels)
    {
        return ((1.0f * pixels) / PIXELS_PER_MAP_UNIT) * METERS_PER_MAP_UNIT;
    }

    // Convert meters directly to pixels
    static int MetersToPixels(float meters)
    {
        return (int) ((meters / METERS_PER_MAP_UNIT) * PIXELS_PER_MAP_UNIT);
    }

    // Convert pixels directly to meters
    static glm::vec2 PixelsToMeters(const glm::ivec2& pixels)
    {
        return {
            PixelsToMeters(pixels.x),
            PixelsToMeters(pixels.y)
        };
    }
    static glm::vec3 PixelsToMeters(const glm::ivec3& pixels)
    {
        return {
            PixelsToMeters(pixels.x),
            PixelsToMeters(pixels.y),
            PixelsToMeters(pixels.z)
        };
    }

    // Convert meters directly to pixels
    static glm::ivec2 MetersToPixels(const glm::vec2& meters)
    {
        return {
            MetersToPixels(meters.x),
            MetersToPixels(meters.y)
        };
    }
    static glm::ivec3 MetersToPixels(const glm::vec3& meters)
    {
        return {
            MetersToPixels(meters.x),
            MetersToPixels(meters.y),
            MetersToPixels(meters.z)
        };
    }
};
