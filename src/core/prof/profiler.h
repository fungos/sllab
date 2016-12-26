#pragma once

#if defined(PROFILER_ENABLE)

#include <Remotery.h>

struct Profiler
{
    Profiler()
        : bInitialized(false)
    {
        if (RMT_ERROR_NONE == rmt_CreateGlobalInstance(&cRmt))
        {
            bInitialized = true;
        }
    }

    ~Profiler()
    {
        if (bInitialized)
        {
            rmt_DestroyGlobalInstance(cRmt);
        }
    }

    Remotery* cRmt;
    bool bInitialized;
};

#define Profiler_Log(text)              rmt_LogText(text)
#define Profiler_ScopedSample(name)     rmt_ScopedCPUSample(name)

#else

struct Profiler {};
#define Profiler_Log(text)
#define Profiler_ScopedSample(name)

#endif // PROFILER_ENABLE
