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

#define CONCATENATE_DETAIL(x, y)	x##y
#define CONCATENATE(x, y)			CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x)				CONCATENATE(x, __LINE__)

template <typename T, typename E>
struct Result {
	T v;
	E e;

	T unwrap() {
		return std::move(v);
	}
};

#define Ok(value)			{value, {}}
#define Err(err)			{{}, err}
#define try(ret, fun)												\
							auto MAKE_UNIQUE(result) = fun;			\
							if (MAKE_UNIQUE(result).e != 0) {		\
								return {{}, MAKE_UNIQUE(result).e};	\
							}										\
							auto ret = MAKE_UNIQUE(result).unwrap();\

#define try_chain(ret, fun, chain_err)								\
						{											\
							auto MAKE_UNIQUE(result) = fun;			\
							if (MAKE_UNIQUE(result).e != 0) {		\
								return {{}, chain_err};				\
							}										\
							ret = MAKE_UNIQUE(result).unwrap();		\
						}

enum Error {
	None = 0,
	MissingArguments,
	FileNotFound,
};

const char *errorDescription[] = {
	"No error found",
	"Missing command line arguments",
	"File not found",
};

struct ProgramOptions {
	bool dumpModel = false;
	bool onlyDumpModel = false;
	u32 verbosity = 0;
	std::string filename;
};

typedef Result<ProgramOptions, Error> ArgsResult;
typedef Result<bool, Error> ProgramResult;


static auto parseCommandLine(int argc, const char* const* argv) {
	/*if (argc < 2) {
		fprintf(stdout, "%s <filename>\n", argv[0]);
		return std::move(ArgsResult{{}, Error::MissingArguments});
	}*/

	auto options = ProgramOptions{};
	for (auto i = u32{0}; i < argc; i++) {
		if (!stricmp(argv[i], "-v")) {
			options.verbosity++;
		}
		if (!stricmp(argv[i], "-dump")) {
			options.dumpModel = true;
		}
		if (!stricmp(argv[i], "-onlyDump")) {
			options.dumpModel = true;
			options.onlyDumpModel = true;
		}
		if (!stricmp(argv[i], "-load") && i + 1 < argc) {
			options.filename = argv[++i];
		}
	}

	return std::move(ArgsResult Ok(options));
}

Gfx *Gfx::create() {
	static Gfx sViewer;
	return &sViewer;
}

struct ModelInstance {
	ModelInstance(ModelData *data) {
		auto model = &data->pModels[0];
		auto mesh = &model->pMeshes[0];
		auto mat = &data->pMaterials[0];

		vbh = bgfx::createVertexBuffer(bgfx::makeRef(mesh->pVertexBuffer, mesh->uiVertexCount * sizeof(Vertex)), Vertex::scDecl);
		ibh = bgfx::createIndexBuffer(bgfx::makeRef(mesh->pIndexBuffer, mesh->uiIndexCount * sizeof(Index)));
		hTexDiffuse = mat->hDiffuseMap;
	}

	~ModelInstance() {
		assert(!isValid(vbh) && !isValid(ibh) && "destroy() not called");
	}

	void destroy() {
		bgfx::destroyIndexBuffer(ibh);
		bgfx::destroyVertexBuffer(vbh);
		vbh = BGFX_INVALID_HANDLE;
		ibh = BGFX_INVALID_HANDLE;
	}

	bgfx::VertexBufferHandle vbh;
	bgfx::IndexBufferHandle ibh;
	bgfx::TextureHandle hTexDiffuse;
};


ProgramResult runLab(int argc, const char* const* argv) {
	try(options, parseCommandLine(argc, argv));

	//auto initPos = glm::vec3{500, 300, 5000};
	auto initPos = glm::vec3{ 0.0f, 0.0f, 0.0f };

	auto width = u32{ 1280 };
	auto height = u32{ 720 };
	auto debug = u32{ BGFX_DEBUG_MODE };
	auto reset = u32{ BGFX_RESET_VSYNC };

	Profiler profiler;
	SL_UNUSED(profiler);

	auto defaultWindow = entry::WindowHandle{ 0 };
	entry::setWindowTitle(defaultWindow, "Gfx Lab");

	bgfx::init();
	bgfx::reset(width, height, reset);
	bgfx::setDebug(debug);
	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

	Vertex::init();

	ModelLoader loader;
	loader.init();

	std::unique_ptr<ModelData> data;
	if (!options.filename.empty())
	{
		data = loader.load(options.filename.c_str());
		if (!data) {
			fprintf(stdout, "Could not open scene from file '%s'.\n", options.filename.c_str());
			return Err( Error::FileNotFound );
		}

		if (options.dumpModel) {
			dumpModel(&data.get()->pModels[0], options.verbosity);
		}

		if (options.onlyDumpModel) {
			loader.unload(data.get());
			return Ok(true);
		}
	}

	// technique
	auto fsh = bgfx::createShader(getShaderMemory(fs_tex0));
	auto vsh = bgfx::createShader(getShaderMemory(vs_tex0));
	auto program = bgfx::createProgram(vsh, fsh);//, true /* destroy shaders when program is destroyed */);
	auto uTexDiffuse = bgfx::createUniform("u_texDiffuse", bgfx::UniformType::Int1);

	// batch
	//auto vbh = bgfx::createVertexBuffer(bgfx::makeRef(mesh->pVertexBuffer, mesh->uiVertexCount * sizeof(Vertex)), Vertex::scDecl);
	//auto ibh = bgfx::createIndexBuffer(bgfx::makeRef(mesh->pIndexBuffer, mesh->uiIndexCount * sizeof(Index)));
	//auto hTexDiffuse = mat->hDiffuseMap;
	ModelInstance model(data.get());
	
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

		const auto at = glm::vec3{ 0.0f, 0.0f, 0.0f };
		const auto up = glm::vec3{ 0.0f, 1.0f, 0.0f };
		const auto eye = glm::vec3{ 0.0f, 0.0f, 10.0f };
		const auto matView = glm::lookAt(eye, at, up);
		const auto matProj = glm::perspectiveFov(glm::radians(60.0f), float(width), float(height), 0.1f, 100.0f);
		bgfx::setViewTransform(0, &matView[0][0], &matProj[0][0]);
		bgfx::setViewRect(0, 0, 0, width, height);
		bgfx::submit(0, program); // dummy submit to do clear

		auto instance = glm::translate(matModel, -initPos);//glm::vec3(0.0f, 0.0f, 0.0f));
		instance = glm::rotate(instance, dt, glm::vec3{ 1.0f, 1.0f, 1.0f });
		bgfx::setTransform(&instance[0][0]);
		
		bgfx::setVertexBuffer(model.vbh);
		bgfx::setIndexBuffer(model.ibh);
		bgfx::setTexture(0, uTexDiffuse, model.hTexDiffuse);
		bgfx::setState(BGFX_STATE_DEFAULT);
		bgfx::submit(0, program);

		bgfx::frame();
		Profiler_Log("end profiling");
	}

	//bgfx::destroyIndexBuffer(ibh);
	//bgfx::destroyVertexBuffer(vbh);
	model.destroy();

	bgfx::destroyUniform(uTexDiffuse);
	bgfx::destroyProgram(program);
	bgfx::destroyShader(vsh);
	bgfx::destroyShader(fsh);

	cleanupSystemImages();
	loader.unload(data.get());

	bgfx::shutdown();

	return Ok(true);
}

int Gfx::run(int argc, const char* const* argv) {
	auto result = runLab(argc, argv);
	if (result.e != 0) {
		fprintf(stdout, "Error: %s\n", errorDescription[result.e]);
		return -result.e;
	}
	return 0;
}

