#pragma once

#define SL_UNUSED(x) (void)x

#if defined(FIPS_DEBUG)
#define SL_PROFILER_ENABLE  1
#define SL_GFX_DEBUG        1
#define BGFX_DEBUG_MODE     BGFX_DEBUG_TEXT
#elif defined(FIPS_PROFILING)
#define SL_PROFILER_ENABLE  1
#define SL_GFX_DEBUG        0
#define BGFX_DEBUG_MODE     BGFX_DEBUG_NONE
#else
#define SL_PROFILER_ENABLE  0
#define SL_GFX_DEBUG        0
#define BGFX_DEBUG_MODE     BGFX_DEBUG_NONE
#endif


