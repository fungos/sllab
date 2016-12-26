/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "common.h"
#include "bgfx_utils.h"
#include "vs_bump.bin.h"
#include "fs_bump.bin.h"

#if BX_PLATFORM_WINDOWS
#define getShaderMemory(x) ( bgfx::makeRef(x##_dx11, sizeof(x##_dx11)) )
#else
#define getShaderMemory(x) ( bgfx::makeRef(x##_glsl, sizeof(x##_glsl)) )
#endif

struct PosTexcoordVertex
{
    float m_x;
    float m_y;
    float m_z;
    float m_u;
    float m_v;

    static void init()
    {
        ms_decl
            .begin()
            .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float, true, true)
            .end();
    }

    static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosTexcoordVertex::ms_decl;

static PosTexcoordVertex s_cubeVertices[24] =
{
    {-1.0f,  1.0f,  1.0f,      0.0f,      0.0f },
    { 1.0f,  1.0f,  1.0f, 1.0f,      0.0f },
    {-1.0f, -1.0f,  1.0f,      0.0f, 1.0f },
    { 1.0f, -1.0f,  1.0f, 1.0f, 1.0f },
    {-1.0f,  1.0f, -1.0f,      0.0f,      0.0f },
    { 1.0f,  1.0f, -1.0f, 1.0f,      0.0f },
    {-1.0f, -1.0f, -1.0f,      0.0f, 1.0f },
    { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f },
    {-1.0f,  1.0f,  1.0f,      0.0f,      0.0f },
    { 1.0f,  1.0f,  1.0f, 1.0f,      0.0f },
    {-1.0f,  1.0f, -1.0f,      0.0f, 1.0f },
    { 1.0f,  1.0f, -1.0f, 1.0f, 1.0f },
    {-1.0f, -1.0f,  1.0f,      0.0f,      0.0f },
    { 1.0f, -1.0f,  1.0f, 1.0f,      0.0f },
    {-1.0f, -1.0f, -1.0f,      0.0f, 1.0f },
    { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f },
    { 1.0f, -1.0f,  1.0f,      0.0f,      0.0f },
    { 1.0f,  1.0f,  1.0f, 1.0f,      0.0f },
    { 1.0f, -1.0f, -1.0f,      0.0f, 1.0f },
    { 1.0f,  1.0f, -1.0f, 1.0f, 1.0f },
    {-1.0f, -1.0f,  1.0f,      0.0f,      0.0f },
    {-1.0f,  1.0f,  1.0f, 1.0f,      0.0f },
    {-1.0f, -1.0f, -1.0f,      0.0f, 1.0f },
    {-1.0f,  1.0f, -1.0f, 1.0f, 1.0f },
};
/*
static PosTexcoordVertex s_cubeVertices[24] =
{
    {-1.0f,  1.0f,  1.0f,      0,      0 },
    { 1.0f,  1.0f,  1.0f, 0x7fff,      0 },
    {-1.0f, -1.0f,  1.0f,      0, 0x7fff },
    { 1.0f, -1.0f,  1.0f, 0x7fff, 0x7fff },
    {-1.0f,  1.0f, -1.0f,      0,      0 },
    { 1.0f,  1.0f, -1.0f, 0x7fff,      0 },
    {-1.0f, -1.0f, -1.0f,      0, 0x7fff },
    { 1.0f, -1.0f, -1.0f, 0x7fff, 0x7fff },
    {-1.0f,  1.0f,  1.0f,      0,      0 },
    { 1.0f,  1.0f,  1.0f, 0x7fff,      0 },
    {-1.0f,  1.0f, -1.0f,      0, 0x7fff },
    { 1.0f,  1.0f, -1.0f, 0x7fff, 0x7fff },
    {-1.0f, -1.0f,  1.0f,      0,      0 },
    { 1.0f, -1.0f,  1.0f, 0x7fff,      0 },
    {-1.0f, -1.0f, -1.0f,      0, 0x7fff },
    { 1.0f, -1.0f, -1.0f, 0x7fff, 0x7fff },
    { 1.0f, -1.0f,  1.0f,      0,      0 },
    { 1.0f,  1.0f,  1.0f, 0x7fff,      0 },
    { 1.0f, -1.0f, -1.0f,      0, 0x7fff },
    { 1.0f,  1.0f, -1.0f, 0x7fff, 0x7fff },
    {-1.0f, -1.0f,  1.0f,      0,      0 },
    {-1.0f,  1.0f,  1.0f, 0x7fff,      0 },
    {-1.0f, -1.0f, -1.0f,      0, 0x7fff },
    {-1.0f,  1.0f, -1.0f, 0x7fff, 0x7fff },
};
*/
static const uint16_t s_cubeIndices[36] =
{
     0,  2,  1,
     1,  2,  3,
     4,  5,  6,
     5,  7,  6,

     8, 10,  9,
     9, 10, 11,
    12, 13, 14,
    13, 15, 14,

    16, 18, 17,
    17, 18, 19,
    20, 21, 22,
    21, 23, 22,
};

int _main_(int /*_argc*/, char** /*_argv*/)
{
    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t debug = BGFX_DEBUG_TEXT;
    uint32_t reset = BGFX_RESET_VSYNC;

    bgfx::init();
    bgfx::reset(width, height, reset);

    // Enable debug text.
    bgfx::setDebug(debug);

    // Set view 0 clear state.
    bgfx::setViewClear(0
        , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
        , 0x303030ff
        , 1.0f
        , 0
        );

    // Create vertex stream declaration.
    PosTexcoordVertex::init();

    // Create static vertex buffer.
    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
          bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices) )
        , PosTexcoordVertex::ms_decl
        );

    // Create static index buffer.
    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices) ) );

    // Create texture sampler uniforms.
    bgfx::UniformHandle u_texColor  = bgfx::createUniform("u_texColor",  bgfx::UniformType::Int1);

    // Create program from shaders.
    auto fsh = bgfx::createShader(getShaderMemory(fs_bump));
	auto vsh = bgfx::createShader(getShaderMemory(vs_bump));
    auto program = bgfx::createProgram(vsh, fsh);//, true /* destroy shaders when program is destroyed */);

    // Load diffuse texture.
    bgfx::TextureHandle textureColor = loadTexture("fieldstone-rgba.dds");
    int64_t timeOffset = bx::getHPCounter();

    while (!entry::processEvents(width, height, debug, reset) )
    {
        // Set view 0 default viewport.
        bgfx::setViewRect(0, 0, 0, width, height);

        // This dummy draw call is here to make sure that view 0 is cleared
        // if no other draw calls are submitted to view 0.
        bgfx::submit(0, program);

        int64_t now = bx::getHPCounter();
        static int64_t last = now;
        const int64_t frameTime = now - last;
        last = now;
        const double freq = double(bx::getHPFrequency() );
        const double toMs = 1000.0/freq;

        float time = (float)( (now-timeOffset)/freq);

        // Use debug font to print information about this example.
        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/06-bump");
        bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Loading textures.");
        bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

        float at[3]  = { 0.0f, 0.0f,  0.0f };
        float eye[3] = { 0.0f, 0.0f, -7.0f };

        float view[16];
        bx::mtxLookAt(view, eye, at);

        float proj[16];
        bx::mtxProj(proj, 60.0f, float(width)/float(height), 0.1f, 100.0f);
        bgfx::setViewTransform(0, view, proj);

        // Set view 0 default viewport.
        bgfx::setViewRect(0, 0, 0, width, height);

        float mtx[16];
        bx::mtxRotateXY(mtx, time*0.023f + 0.21f, time*0.03f + 0.37f);
        mtx[12] = 0.0f;
        mtx[13] = 0.0f;
        mtx[14] = 0.0f;

        // Set transform for draw call.
        bgfx::setTransform(mtx);

        // Set vertex and index buffer.
        bgfx::setVertexBuffer(vbh);
        bgfx::setIndexBuffer(ibh);

        // Bind textures.
        bgfx::setTexture(0, u_texColor, textureColor);

        // Set render states.
        bgfx::setState(0
            | BGFX_STATE_RGB_WRITE
            | BGFX_STATE_ALPHA_WRITE
            | BGFX_STATE_DEPTH_WRITE
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_MSAA
            );

        // Submit primitive for rendering to view 0.
        bgfx::submit(0, program);

        // Advance to next frame. Rendering thread will be kicked to
        // process submitted rendering primitives.
        bgfx::frame();
    }

    // Cleanup.
    bgfx::destroyIndexBuffer(ibh);
    bgfx::destroyVertexBuffer(vbh);
    bgfx::destroyProgram(program);
    bgfx::destroyShader(vsh);
    bgfx::destroyShader(fsh);
    bgfx::destroyTexture(textureColor);
    bgfx::destroyUniform(u_texColor);

    // Shutdown bgfx.
    bgfx::shutdown();

    return 0;
}
