#
# project: sllab
#

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if (NOT FIPS_IMPORT)
    cmake_minimum_required(VERSION 2.8)
    get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
    include("${FIPS_ROOT_DIR}/cmake/fips.cmake")
	fips_setup(PROJECT sllab)
endif()

if (FIPS_MACOS)
    include_directories(../fips-bgfx/bx/include/compat/osx)
elseif (FIPS_IOS)
    include_directories(../fips-bgfx/bx/include/compat/ios)
elseif (FIPS_PNACL)
    include_directories(../fips-bgfx/bx/include/compat/nacl)
elseif (FIPS_WINDOWS)
    include_directories(../fips-bgfx/bx/include/compat/msvc)
endif()

fips_add_subdirectory(src)

if (NOT FIPS_IMPORT)
	fips_ide_group(Tests)
	fips_add_subdirectory(tests)
	fips_finish()
endif()

