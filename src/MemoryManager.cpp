#include "stdafx.h"
#include "MemoryManager.h"
#include "cvars.h"

//////////////////////////////////////////////////////////////////////////

const int SysMemoryFrameHeapSize = 12 * 1024 * 1024;

//////////////////////////////////////////////////////////////////////////

bool MemoryManager::Initialize()
{
    gSystem.LogMessage(eLogMessage_Info, "Init MemoryManager");

    if (gCvarMemEnableFrameHeapAllocator.mValue)
    {
        gSystem.LogMessage(eLogMessage_Info, "Frame heap memory size: %d", SysMemoryFrameHeapSize);

        mFrameHeapAllocator = new cxx::linear_memory_allocator;
        if (!mFrameHeapAllocator->init_allocator(SysMemoryFrameHeapSize))
        {
            gSystem.LogMessage(eLogMessage_Warning, "Fail to allocate frame heap memory buffer");
            SafeDelete(mFrameHeapAllocator);
        }
        else
        {
            // setup out of memory handler
            mFrameHeapAllocator->mOutOfMemoryProc = [](unsigned int allocateBytes)
            {
                gSystem.LogMessage(eLogMessage_Warning, "Cannot allocate %d bytes on frame heap", allocateBytes);
                cxx_assert(false);
            };
        }
    }
    else
    {
        gSystem.LogMessage(eLogMessage_Info, "Frame heap memory disabled");
    }

    mHeapAllocator = new cxx::heap_memory_allocator;
    mHeapAllocator->init_allocator(0);

    mHeapAllocator->mOutOfMemoryProc = [](unsigned int allocateBytes)
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot allocate %d bytes", allocateBytes);
        cxx_assert(false);
    };

    return true;
}

void MemoryManager::Deinit()
{
    SafeDelete(mFrameHeapAllocator);
    SafeDelete(mHeapAllocator);
}

void MemoryManager::FlushFrameHeapMemory()
{
    if (mFrameHeapAllocator)
    {
        mFrameHeapAllocator->reset();
    }
}