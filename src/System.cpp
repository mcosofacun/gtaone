#include "stdafx.h"
#include "System.h"
#include "GtaOneGame.h"
#include "cvars.h"

//////////////////////////////////////////////////////////////////////////
// cvars
//////////////////////////////////////////////////////////////////////////

// graphics
CvarPoint gCvarGraphicsScreenDims("r_screenDims", Point(1024, 768), "Screen dimensions", CvarFlags_Archive | CvarFlags_RequiresAppRestart);
CvarBoolean gCvarGraphicsFullscreen("r_fullscreen", false, "Is fullscreen mode enabled", CvarFlags_Archive);
CvarBoolean gCvarGraphicsVSync("r_vsync", true, "Is vertical synchronization enabled", CvarFlags_Archive);
CvarBoolean gCvarGraphicsTexFiltering("r_texFiltering", false, "Is texture filtering enabled", CvarFlags_Archive | CvarFlags_Readonly);

// physics
CvarFloat gCvarPhysicsFramerate("g_physicsFps", 60.0f, "Physical world update framerate", CvarFlags_Archive | CvarFlags_Init);

// memory
CvarBoolean gCvarMemEnableFrameHeapAllocator("mem_enableFrameHeapAllocator", true, "Enable frame heap allocator", CvarFlags_Archive | CvarFlags_Init);

// audio
CvarBoolean gCvarAudioActive("a_audioActive", true, "Enable audio system", CvarFlags_Archive | CvarFlags_Init);

// commands
CvarVoid gCvarSysQuit("quit", "Quit application", CvarFlags_None);
CvarVoid gCvarSysListCvars("print_cvars", "Print all registered console variables", CvarFlags_None);

//////////////////////////////////////////////////////////////////////////

static char ConsoleMessageBuffer[2048];

#define VA_SCOPE_OPEN(firstArg, vaName) \
    { \
        va_list vaName {}; \
        va_start(vaName, firstArg); \

#define VA_SCOPE_CLOSE(vaName) \
        va_end(vaName); \
    }

//////////////////////////////////////////////////////////////////////////

System gSystem;

//////////////////////////////////////////////////////////////////////////

void System::Initialize(int argc, char *argv[])
{
    mQuitRequested = false;

    LogMessage(eLogMessage_Info, GAME_TITLE);
    LogMessage(eLogMessage_Info, "System initialize");
    RegisterGlobalCvars();
    
    if (!mFiles.Initialize())
    {
        LogMessage(eLogMessage_Error, "Cannot initialize filesystem");
        Terminate();
    }

    LoadConfiguration();
    ParseStartupParams(argc, argv);

    mInputs.Initialize();

    if (!mMemoryMng.Initialize())
    {
        LogMessage(eLogMessage_Error, "Cannot initialize system memory manager");
        Terminate();
    }

    if (!mGfxDevice.Initialize())
    {
        LogMessage(eLogMessage_Error, "Cannot initialize graphics device");
        Terminate();
    }

    if (!mSfxDevice.Initialize())
    {
        LogMessage(eLogMessage_Warning, "Cannot initialize audio device");
    }

    InitMultimediaTimers();

    if (!mFiles.SetupGtaDataLocation())
    {
        LogMessage(eLogMessage_Warning, "Set valid gta gamedata location via sys config param 'g_gtadata'");
    }

    if (!gGame.Initialize())
    {
        LogMessage(eLogMessage_Error, "Cannot initialize game");
        Terminate();
    }
}

void System::Deinit(bool isTermination)
{
    LogMessage(eLogMessage_Info, "System shutdown");

    if (!isTermination)
    {
        SaveConfiguration();
    }

    DeinitMultimediaTimers();

    gGame.Deinit();

    mFiles.Deinit();
    mSfxDevice.Deinit();
    mGfxDevice.Deinit();
    mMemoryMng.Deinit();
}

void System::Run(int argc, char *argv[])
{
    Initialize(argc, argv);

    // main loop

#ifndef __EMSCRIPTEN__

    while (true)
    {
        bool continueExecution = ExecuteFrame();
        if (!continueExecution)
            break;
    }

    Deinit(false);

#else
    emscripten_set_main_loop([]()
    {
        bool continueExecution = gSystem.ExecuteFrame();
        if (!continueExecution)
        {
            gSystem.Deinit(false);
        }
    }, 
    0, false);
#endif // __EMSCRIPTEN__
}

void System::Terminate()
{    
    Deinit(true); // leave gracefully

#ifdef __EMSCRIPTEN__
    emscripten_force_exit(EXIT_FAILURE);
#else
    exit(EXIT_FAILURE);
#endif // __EMSCRIPTEN__
}

void System::QuitRequest()
{
    mQuitRequested = true;
}

bool System::LoadConfiguration()
{
    LogMessage(eLogMessage_Debug, "Loading system configuration");

    cxx::json_document configDocument;
    if (!mFiles.ReadConfig(mFiles.mSysConfigPath, configDocument))
        return false;

    cxx::json_document_node configRootNode = configDocument.get_root_node();

    // load archive cvars
    for (Cvar* currCvar: mCvarsList)
    {
        if (!currCvar->IsArchive())
            continue;

        currCvar->LoadCvar(configRootNode);
    }

    return true;
}

bool System::SaveConfiguration()
{
    LogMessage(eLogMessage_Debug, "Saving system configuration");

    cxx::json_document configDocument;
    configDocument.create_document();

    cxx::json_document_node configRootNode = configDocument.get_root_node();

    // save archive cvars
    for (Cvar* currCvar: mCvarsList)
    {
        if (!currCvar->IsArchive())
            continue;

        currCvar->SaveCvar(configRootNode);
    }

    if (!mFiles.SaveConfig(mFiles.mSysConfigPath, configDocument))
        return false;

    return true;
}

double System::GetSystemSeconds() const
{
    double currentTime = ::glfwGetTime();
    return currentTime;
}

bool System::ExecuteFrame()
{
    if (mQuitRequested)
        return false;

    mInputs.UpdateFrame();
    mMemoryMng.FlushFrameHeapMemory();

    gGame.UpdateFrame();

    // process quit command
    if (gCvarSysQuit.IsModified())
    {
        gCvarSysQuit.ClearModified();
        QuitRequest();
    }

    // process list cvars command
    if (gCvarSysListCvars.IsModified())
    {
        gCvarSysListCvars.ClearModified();
        for (Cvar* currCvar: mCvarsList)
        {
            if (currCvar->IsHidden())
                continue;

            currCvar->PrintInfo();
        };
    }

    // update screen params
    if (gCvarGraphicsFullscreen.IsModified() || gCvarGraphicsVSync.IsModified())
    {
        mGfxDevice.EnableFullscreen(gCvarGraphicsFullscreen.mValue);
        gCvarGraphicsFullscreen.ClearModified();

        mGfxDevice.EnableVSync(gCvarGraphicsVSync.mValue);
        gCvarGraphicsVSync.ClearModified();
    }

    mGfxDevice.ClearScreen();
    gGame.RenderFrame();
    mGfxDevice.Present();

    return true;
}

void System::ParseStartupParams(int argc, char *argv[])
{
    for (int iarg = 0; iarg < argc; )
    {
        if (cxx_stricmp(argv[iarg], "-mapname") == 0 && (argc > iarg + 1))
        {
            gCvarMapname.SetFromString(argv[iarg + 1], eCvarSetMethod_CommandLine);
            iarg += 2;
            continue;
        }
        if (cxx_stricmp(argv[iarg], "-gtadata") == 0 && (argc > iarg + 1))
        {
            gCvarCurrentBaseDir.SetFromString(argv[iarg + 1], eCvarSetMethod_CommandLine);
            iarg += 2;
            continue;
        }
        if (cxx_stricmp(argv[iarg], "-gtaversion") == 0 && (argc > iarg + 1))
        {
            gCvarGameVersion.SetFromString(argv[iarg + 1], eCvarSetMethod_CommandLine);
            iarg += 2;
            continue;
        }
        if (cxx_stricmp(argv[iarg], "-lang") == 0 && (argc > iarg + 1))
        {
            gCvarGameLanguage.SetFromString(argv[iarg + 1], eCvarSetMethod_CommandLine);
            iarg += 2;
            continue;
        }
        if (cxx_stricmp(argv[iarg], "-weather") == 0)
        {
            gCvarWeatherActive.SetFromString("true", eCvarSetMethod_CommandLine);
            iarg += 1;
            continue;
        }
        LogMessage(eLogMessage_Warning, "Unknown arg '%s'", argv[iarg]);
        ++iarg;
    }
}

void System::LogMessage(eLogMessage messageCat, const char* format, ...)
{
    VA_SCOPE_OPEN(format, vaList)
    vsnprintf(ConsoleMessageBuffer, sizeof(ConsoleMessageBuffer), format, vaList);
    VA_SCOPE_CLOSE(vaList)

    if (messageCat > eLogMessage_Debug)
    {
        printf("%s\n", ConsoleMessageBuffer);
    }
    ConsoleLine consoleLine;
    consoleLine.mLineType = eConsoleLineType_Message;
    consoleLine.mMessageCategory = messageCat;
    consoleLine.mString = ConsoleMessageBuffer;
    mConsoleLines.push_back(std::move(consoleLine));
}

void System::FlushConsole()
{
    mConsoleLines.clear();
}

void System::ExecuteCommands(const char* commands)
{
    LogMessage(eLogMessage_Debug, "%s", commands); // echo

    cxx::string_tokenizer tokenizer(commands);
    for (;;)
    {
        std::string commandName;
        if (!tokenizer.get_next(commandName, ' '))
            break;

        // find variable by name
        Cvar* consoleVariable = nullptr;
        for (Cvar* currCvar: mCvarsList)
        {
            if (currCvar->mName == commandName)
            {
                consoleVariable = currCvar;
                break;
            }
        }

        if (consoleVariable)
        {   
            std::string commandParams;
            if (tokenizer.get_next(commandParams, ';'))
            {
                cxx::trim(commandParams);
            }
            consoleVariable->CallWithParams(commandParams);
            break;
        }
        LogMessage(eLogMessage_Warning, "Unknown command %s", commandName.c_str());
    }
}

bool System::RegisterCvar(Cvar* consoleVariable)
{
    if (consoleVariable == nullptr)
    {
        cxx_assert(false);
        return false;
    }
    if (cxx::contains_if(mCvarsList, [consoleVariable](const Cvar* currCvar)
        {
            return (currCvar == consoleVariable) || (currCvar->mName == consoleVariable->mName);
        }))
    {
        cxx_assert(false);
        return false;
    }
    mCvarsList.push_back(consoleVariable);
    return true;
}

bool System::UnregisterCvar(Cvar* consoleVariable)
{
    if (consoleVariable == nullptr)
    {
        cxx_assert(false);
        return false;
    }
    cxx::erase_elements(mCvarsList, consoleVariable);
    return true;
}

void System::InitMultimediaTimers()
{
#if OS_NAME == OS_WINDOWS
    MMRESULT mmResult = ::timeBeginPeriod(1);
    if (mmResult != TIMERR_NOERROR)
    {
        LogMessage(eLogMessage_Debug, "Cannot setup multimedia timers");
    }
#endif
}

void System::DeinitMultimediaTimers()
{
#if OS_NAME == OS_WINDOWS
    ::timeEndPeriod(1); 
#endif
}

void System::RegisterGlobalCvars()
{
    // vars
    RegisterCvar(&gCvarGraphicsScreenDims);
    RegisterCvar(&gCvarGraphicsFullscreen);
    RegisterCvar(&gCvarGraphicsVSync);
    RegisterCvar(&gCvarGraphicsTexFiltering);
    RegisterCvar(&gCvarPhysicsFramerate);
    RegisterCvar(&gCvarMemEnableFrameHeapAllocator);
    RegisterCvar(&gCvarAudioActive);
    RegisterCvar(&gCvarGtaDataPath);
    RegisterCvar(&gCvarMapname);
    RegisterCvar(&gCvarCurrentBaseDir);
    RegisterCvar(&gCvarGameVersion);
    RegisterCvar(&gCvarGameLanguage);
    RegisterCvar(&gCvarWeatherActive);
    RegisterCvar(&gCvarWeatherEffect);
    RegisterCvar(&gCvarGameMusicMode);
    RegisterCvar(&gCvarCarSparksActive);
    RegisterCvar(&gCvarMouseAiming);
    RegisterCvar(&gCvarMusicVolume);
    RegisterCvar(&gCvarSoundsVolume);
    RegisterCvar(&gCvarUiScale);
    // commands
    RegisterCvar(&gCvarSysQuit);
    RegisterCvar(&gCvarSysListCvars);
    RegisterCvar(&gCvarDbgDumpSpriteDeltas);
    RegisterCvar(&gCvarDbgDumpBlockTextures);
    RegisterCvar(&gCvarDbgDumpSprites);
    RegisterCvar(&gCvarDbgDumpCarSprites);
}
