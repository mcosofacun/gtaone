#pragma once

#include "MemoryManager.h"
#include "FileSystem.h"
#include "InputsManager.h"
#include "GraphicsDevice.h"
#include "AudioDevice.h"

// forwards
class Cvar;

// Common system specific stuff collected in System class
class System final: public cxx::noncopyable
{
public:
    // managers
    MemoryManager mMemoryMng;
    FileSystem mFiles;
    InputsManager mInputs;
    GraphicsDevice mGfxDevice;
    AudioDevice mSfxDevice;
    // console data
    std::deque<ConsoleLine> mConsoleLines;
    std::vector<Cvar*> mCvarsList;

public:
    // Initialize game subsystems and run main loop
    void Run(int argc, char *argv[]);

    // Abnormal application shutdown due to critical failure
    void Terminate();

    // Set application exit request flag, execution will be interrupted soon
    void QuitRequest();

    // Get real time seconds since system started
    double GetSystemSeconds() const;

    // Write text message in console
    void LogMessage(eLogMessage messageCat, const char* format, ...);

    // Clear all console text messages
    void FlushConsole();

    // parse and execute commands
    // @param commands: Commands string
    void ExecuteCommands(const char* commands);

    // Register or unregister console variable
    // @returns false on error
    bool RegisterCvar(Cvar* consoleVariable);
    bool UnregisterCvar(Cvar* consoleVariable);

private:
    void Initialize(int argc, char *argv[]);
    void Deinit(bool isTermination);
    void RegisterGlobalCvars();
    bool ExecuteFrame();
    void ParseStartupParams(int argc, char *argv[]);

    // Save/Load configuration to/from external file
    bool LoadConfiguration();
    bool SaveConfiguration();

    void InitMultimediaTimers();
    void DeinitMultimediaTimers();

private:
    bool mQuitRequested;
};

extern System gSystem;