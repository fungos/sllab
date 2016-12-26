#pragma once

#if BX_PLATFORM_WINDOWS
#define getShaderMemory(x) ( bgfx::makeRef(x##_dx11, sizeof(x##_dx11)) )
#else
#define getShaderMemory(x) ( bgfx::makeRef(x##_glsl, sizeof(x##_glsl)) )
#endif

struct Gfx {
    int run(int argc, const char* const* argv);
    static Gfx *create();
};


