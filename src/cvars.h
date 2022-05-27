#pragma once

#include "ConsoleVar.h"
#include "GameDefs.h"

//////////////////////////////////////////////////////////////////////////
// all console variables declarations
//////////////////////////////////////////////////////////////////////////

// inputs
extern CvarBoolean gCvarMouseAiming; // aim with mouse

// graphics
extern CvarPoint gCvarGraphicsScreenDims; // screen dimensions
extern CvarBoolean gCvarGraphicsFullscreen; // is fullscreen mode enabled
extern CvarBoolean gCvarGraphicsVSync; // is vertical synchronization enabled
extern CvarBoolean gCvarGraphicsTexFiltering; // is texture filtering enabled

// physics
extern CvarFloat gCvarPhysicsFramerate; // physical world update framerate

// memory
extern CvarBoolean gCvarMemEnableFrameHeapAllocator; // enable frame heap allocator

// audio
extern CvarBoolean gCvarAudioActive; // enable audio system
extern CvarEnum<eGameMusicMode> gCvarGameMusicMode; // ingame music mode
extern CvarInt gCvarMusicVolume; // ingame music volume in range [0-7]
extern CvarInt gCvarSoundsVolume; // ingame effects volume in range [0-7]

// game
extern CvarString gCvarGtaDataPath; // config gta data location
extern CvarString gCvarMapname; // current map name
extern CvarString gCvarCurrentBaseDir; // current gta data location
extern CvarEnum<eGtaGameVersion> gCvarGameVersion; // current gta game version
extern CvarString gCvarGameLanguage; // current game language
extern CvarBoolean gCvarWeatherActive; // whether weather effects enabled
extern CvarEnum<eWeatherEffect> gCvarWeatherEffect; // currently active weather
extern CvarBoolean gCvarCarSparksActive; // enable car sparks effect

// ui
extern CvarFloat gCvarUiScale; // ui elements scale factor

//////////////////////////////////////////////////////////////////////////
// console commands
//////////////////////////////////////////////////////////////////////////

extern CvarVoid gCvarSysQuit; // quit application
extern CvarVoid gCvarSysListCvars; // print all non-hidden cvars to console

// debug commands
extern CvarVoid gCvarDbgDumpSpriteDeltas; // dump sprite deltas
extern CvarVoid gCvarDbgDumpBlockTextures; // dump block textures
extern CvarVoid gCvarDbgDumpSprites; // dump all sprites
extern CvarVoid gCvarDbgDumpCarSprites; // dump car sprites
