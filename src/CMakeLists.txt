bgfx_include_compat()

fips_begin_app(cube cmdline)
    fips_files(cube.cpp)
    fips_src(core)

    fips_dir(core/shaders)
    fips_files(varying.def.sc)
    bgfx_shaders(FILES vs_pass.sc fs_pass.sc vs_tex0.sc fs_tex0.sc)

    fips_deps(Remotery)
    fips_deps(bgfx bgfx-examples-common bgfx-imgui bgfx-ib-compress)
fips_end_app()

fips_begin_app(viewer cmdline)
    fips_files(viewer.cpp)
    fips_src(core)

    fips_dir(core/shaders)
    fips_files(varying.def.sc)
    bgfx_shaders(FILES vs_pass.sc fs_pass.sc vs_tex0.sc fs_tex0.sc)

    fips_deps(Remotery assimp)
    fips_deps(bgfx bgfx-examples-common bgfx-imgui bgfx-ib-compress)
fips_end_app()
