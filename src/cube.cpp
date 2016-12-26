#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <bgfx/bgfx.h>
#include <common.h>

#include "core/config.h"
#include "core/gfx.h"
#include "core/gfx_types.h"

#include "core/prof/profiler.h"

#include "fs_pass.bin.h"
#include "vs_pass.bin.h"

struct Vertex {
    glm::vec3 vPos;
    uint32_t uiARGB;

    static void init() {
        scDecl
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
            .end();
    }

    static bgfx::VertexDecl scDecl;
};

bgfx::VertexDecl Vertex::scDecl;

static Vertex s_cubeVertices[8] =
{
    {{-1.0f,  1.0f, -1.0f}, 0xff000000 },
    {{ 1.0f,  1.0f, -1.0f}, 0xff0000ff },
    {{-1.0f, -1.0f, -1.0f}, 0xff00ff00 },
    {{ 1.0f, -1.0f, -1.0f}, 0xff00ffff },
    {{-1.0f,  1.0f,  1.0f}, 0xffff0000 },
    {{ 1.0f,  1.0f,  1.0f}, 0xffff00ff },
    {{-1.0f, -1.0f,  1.0f}, 0xffffff00 },
    {{ 1.0f, -1.0f,  1.0f}, 0xffffffff },
};

static const uint16_t s_cubeIndices[36] =
{
    0, 1, 2, // 0
    1, 3, 2,
    4, 6, 5, // 2
    5, 6, 7,
    0, 2, 4, // 4
    4, 2, 6,
    1, 5, 3, // 6
    5, 7, 3,
    0, 4, 1, // 8
    4, 5, 1,
    2, 3, 6, // 10
    6, 3, 7,
};

double delay() {
    int i, end;
    double j = 0;

    Profiler_ScopedSample(delay);
    for (i= 0, end = (rand() % 100); i < end; ++i) {
        j += sin(i);
    }

    return j;
}

Gfx *Gfx::create() {
    static Gfx sCube;
    return &sCube;
}

int Gfx::run(int argc, const char* const* argv) {

    uint32_t width = 1280;
    uint32_t height = 720;
    uint32_t debug = BGFX_DEBUG_MODE;
    uint32_t reset = BGFX_RESET_VSYNC;

    Profiler profiler;
    SL_UNUSED(profiler);

    entry::WindowHandle defaultWindow = {0};
    entry::setWindowTitle(defaultWindow, "Gfx Lab");

    bgfx::init();
    bgfx::reset(width, height, reset);
    bgfx::setDebug(debug);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

    Vertex::init();
    auto vbh = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), Vertex::scDecl);
    auto ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices)));
    //bgfx::ProgramHandle program  = loadProgram("vs_pass", "fs_pass");
    auto fsh = bgfx::createShader(getShaderMemory(fs_pass));
    auto vsh = bgfx::createShader(getShaderMemory(vs_pass));
    auto program = bgfx::createProgram(vsh, fsh);//, true /* destroy shaders when program is destroyed */);

    int64_t timeOffset = bx::getHPCounter();
    glm::mat4 model;
    while (!entry::processEvents(width, height, debug, reset)) {
        Profiler_Log("start profiling");
        int64_t now = bx::getHPCounter();
        static int64_t last = now;
        const int64_t frameTime = now - last;
        last = now;
        const double freq = double(bx::getHPFrequency());
        const double toMs = 1000.0 / freq;
        const float dt = float((now - timeOffset) / double(bx::getHPFrequency()));

#if defined(SL_GFX_DEBUG)
        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(0, 0, 0x3f, "Frame: %7.3fms", double(frameTime) * toMs);
#endif

        const glm::vec3 at = {0.0f, 0.0f, 0.0f};
        const glm::vec3 up = {0.0f, 1.0f, 0.0f};
        const glm::vec3 eye = {0.0f, 0.0f, 10.0f};
        const glm::mat4 view = glm::lookAt(eye, at, up);
        const glm::mat4 proj = glm::perspectiveFov(glm::radians(60.0f), float(width), float(height), 0.1f, 100.0f);
        bgfx::setViewTransform(0, &view[0][0], &proj[0][0]);
        bgfx::setViewRect(0, 0, 0, width, height);
        bgfx::submit(0, program); // dummy submit to do clear

        auto instance = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        instance = glm::rotate(instance, dt, glm::vec3(1.0f, 1.0f, 1.0f));
        bgfx::setTransform(&instance[0][0]);

        bgfx::setVertexBuffer(vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setState(BGFX_STATE_DEFAULT);
        bgfx::submit(0, program);

        bgfx::frame();

        delay();
        Profiler_Log("end profiling");
    }

    bgfx::destroyIndexBuffer(ibh);
    bgfx::destroyVertexBuffer(vbh);
    bgfx::destroyProgram(program);

    bgfx::shutdown();

    return 0;
}

