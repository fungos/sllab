#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <bgfx/bgfx.h>
#include <common.h>
#include <bx/readerwriter.h>

#include "core/config.h"
#include "core/gfx.h"
#include "core/gfx_types.h"
#include "core/math/bbox.h"
#include "core/prof/profiler.h"
#include "core/resources/primitives.h"
#include "core/resources/model_loader.h"

#include "fs_tex0.bin.h"
#include "vs_tex0.bin.h"

/*
// should we do a triangle optimization pass over the one from assimp? do benchmark.
//#include <forsyth-too/forsythtriangleorderoptimizer.h>

void triangleReorder(u32* _indices, u32 _numIndices, u32 _numVertices, uint16_t _cacheSize)
{
    uint16_t* newIndexList = (u32 *)malloc(_numIndices * sizeof(u32));
    Forsyth::OptimizeFaces(_indices, _numIndices, _numVertices, newIndexList, _cacheSize);
    memcpy(_indices, newIndexList, _numIndices*2);
    free(newIndexList);
}
*/

template <typename T, typename E>
struct Result {
    T v;
    E e;
};

struct ProgramOptions {
    bool dumpModel;
    bool onlyDumpModel;
    u32 verbosity;
};

static Result<ProgramOptions, bool> parseCommandLine(int argc, const char* const* argv) {
    if (argc < 2) {
        fprintf(stdout, "%s <filename>\n", argv[0]);
        return Result<ProgramOptions, bool>{{}, true};
    }

    auto dumpModel = false;
    auto onlyDumpModel = false;
    auto verbosity = u32{0};
    for (auto i = u32{0}; i < argc; i++) {
        if (!stricmp(argv[i], "-v")) {
            verbosity++;
        }
        if (!stricmp(argv[i], "-dump")) {
            dumpModel = true;
        }
        if (!stricmp(argv[i], "-onlyDump")) {
            dumpModel = true;
            onlyDumpModel = true;
        }
    }

    return Result<ProgramOptions, bool>{ dumpModel, onlyDumpModel, verbosity, false };
}

Gfx *Gfx::create() {
    static Gfx sViewer;
    return &sViewer;
}

int Gfx::run(int argc, const char* const* argv) {
    auto options = parseCommandLine(argc, argv);
    if (options.e) {
        return -1;
    }

    //auto initPos = glm::vec3{500, 300, 5000};
    auto initPos = glm::vec3{0.0f, 0.0f, 0.0f};

    auto width = u32{1280};
    auto height = u32{720};
    auto debug = u32{BGFX_DEBUG_MODE};
    auto reset = u32{BGFX_RESET_VSYNC};

    Profiler profiler;
    SL_UNUSED(profiler);

    auto defaultWindow = entry::WindowHandle{0};
    entry::setWindowTitle(defaultWindow, "Gfx Lab");

    bgfx::init();
    bgfx::reset(width, height, reset);
    bgfx::setDebug(debug);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

    Vertex::init();

    ModelLoader loader;
    loader.init();
    if (!loader.load(argv[1])) {
        fprintf(stdout, "Could not open scene from file '%s'.\n", argv[1]);
        return -2;
    }

    auto model = &loader.pModels[0];
    auto mesh = &model->pMeshes[0];
    auto mat = &loader.pMaterials[0];

    if (options.v.dumpModel) {
        loader.dumpModel(model, options.v.verbosity);
    }

    if (options.v.onlyDumpModel) {
        loader.unload();
        return 0;
    }

    // technique
    auto fsh = bgfx::createShader(getShaderMemory(fs_tex0));
    auto vsh = bgfx::createShader(getShaderMemory(vs_tex0));
    auto program = bgfx::createProgram(vsh, fsh);//, true /* destroy shaders when program is destroyed */);
    auto uTexDiffuse = bgfx::createUniform("u_texDiffuse", bgfx::UniformType::Int1);

    // batch
    auto vbh = bgfx::createVertexBuffer(bgfx::makeRef(mesh->pVertexBuffer, mesh->uiVertexCount * sizeof(Vertex)), Vertex::scDecl);
    auto ibh = bgfx::createIndexBuffer(bgfx::makeRef(mesh->pIndexBuffer, mesh->uiIndexCount * sizeof(Index)));
    auto hTexDiffuse = mat->hDiffuseMap;

    auto timeOffset = bx::getHPCounter();
    glm::mat4 matModel;
    while (!entry::processEvents(width, height, debug, reset)) {
        Profiler_Log("start profiling");
        const auto now = bx::getHPCounter();
        static auto last = now;
        const auto frameTime = now - last;
        last = now;
        const auto freq = double(bx::getHPFrequency());
        const auto toMs = double(1000.0 / freq);
        const auto dt = float((now - timeOffset) / double(bx::getHPFrequency()));

#if defined(SL_GFX_DEBUG)
        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(0, 0, 0x3f, "Frame: %7.3fms", double(frameTime) * toMs);
#endif

        const auto at = glm::vec3{0.0f, 0.0f, 0.0f};
        const auto up = glm::vec3{0.0f, 1.0f, 0.0f};
        const auto eye = glm::vec3{0.0f, 0.0f, 10.0f};
        const auto matView = glm::lookAt(eye, at, up);
        const auto matProj = glm::perspectiveFov(glm::radians(60.0f), float(width), float(height), 0.1f, 100.0f);
        bgfx::setViewTransform(0, &matView[0][0], &matProj[0][0]);
        bgfx::setViewRect(0, 0, 0, width, height);
        bgfx::submit(0, program); // dummy submit to do clear

        auto instance = glm::translate(matModel, -initPos);//glm::vec3(0.0f, 0.0f, 0.0f));
        instance = glm::rotate(instance, dt, glm::vec3{1.0f, 1.0f, 1.0f});
        bgfx::setTransform(&instance[0][0]);

        bgfx::setVertexBuffer(vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setTexture(0, uTexDiffuse, hTexDiffuse);
        bgfx::setState(BGFX_STATE_DEFAULT);
        bgfx::submit(0, program);

        bgfx::frame();
        Profiler_Log("end profiling");
    }

    bgfx::destroyIndexBuffer(ibh);
    bgfx::destroyVertexBuffer(vbh);

    bgfx::destroyUniform(uTexDiffuse);
    bgfx::destroyProgram(program);
    bgfx::destroyShader(vsh);
    bgfx::destroyShader(fsh);

    cleanupSystemImages();
    loader.unload();

    bgfx::shutdown();

    return 0;
}

