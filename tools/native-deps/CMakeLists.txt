cmake_minimum_required(VERSION 3.20)
project(cui_native_dependencies LANGUAGES NONE)
include(ExternalProject)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# A separate build keeps upstream options out of CUI's cache. This prefix is
# consumed through pkg-config by the ordinary CUI configuration.
set(CUI_DEPS_PREFIX "${CMAKE_BINARY_DIR}/prefix" CACHE PATH "Native dependency install prefix")
option(BUILD_SHARED_LIBS "Build deployable shared native dependencies" ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Dependency build configuration" FORCE)
endif()
get_filename_component(CUI_DEPS_PREFIX "${CUI_DEPS_PREFIX}" ABSOLUTE)
set(dependency_args
  "-DCMAKE_INSTALL_PREFIX:PATH=${CUI_DEPS_PREFIX}"
  "-DCMAKE_INSTALL_LIBDIR:STRING=lib"
  "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
  "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
  "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
  "-DCMAKE_PREFIX_PATH:PATH=${CUI_DEPS_PREFIX}")
# Forward the target toolchain, SDK, architecture and deployment target to each
# independently configured upstream project (including Linux -> macOS builds).
foreach(variable CMAKE_TOOLCHAIN_FILE CMAKE_OSX_SYSROOT CMAKE_OSX_ARCHITECTURES
    CMAKE_OSX_DEPLOYMENT_TARGET CMAKE_C_COMPILER CMAKE_CXX_COMPILER
    CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS
    CUI_MACOS_CC CUI_MACOS_CXX CUI_MACOS_SDK CUI_MACOS_PREFIX CUI_MACOS_ARCH
    CUI_MACOS_DEPLOYMENT_TARGET)
  if(DEFINED ${variable} AND NOT "${${variable}}" STREQUAL "")
    string(REPLACE ";" "|" value "${${variable}}")
    list(APPEND dependency_args "-D${variable}:STRING=${value}")
  endif()
endforeach()

function(dependency_license target directory)
  ExternalProject_Add_Step(${target} license
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CUI_DEPS_PREFIX}/share/licenses/${target}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${ARGN}
      "${CUI_DEPS_PREFIX}/share/licenses/${target}/"
    WORKING_DIRECTORY "<SOURCE_DIR>/${directory}"
    DEPENDEES install)
endfunction()

set(sdl_static OFF)
if(NOT BUILD_SHARED_LIBS)
  set(sdl_static ON)
endif()
ExternalProject_Add(sdl3
  URL https://codeload.github.com/libsdl-org/SDL/tar.gz/7f3ae3d57459e59943a4ecfefc8f6277ec6bf540
  URL_HASH SHA256=7a8347c770b90b33daac2352858ca03f9c9a2ccc8ce711054870361d1a6b32e5
  LIST_SEPARATOR |
  CMAKE_ARGS ${dependency_args}
    -DSDL_SHARED:BOOL=${BUILD_SHARED_LIBS}
    -DSDL_STATIC:BOOL=${sdl_static}
    -DSDL_TEST_LIBRARY:BOOL=OFF -DSDL_TESTS:BOOL=OFF
    -DSDL_EXAMPLES:BOOL=OFF -DSDL_INSTALL:BOOL=ON
  INSTALL_COMMAND "${CMAKE_COMMAND}" --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE})
dependency_license(sdl3 . LICENSE.txt)

ExternalProject_Add(freetype
  DEPENDS sdl3
  URL https://codeload.github.com/freetype/freetype/tar.gz/42608f77f20749dd6ddc9e0536788eaad70ea4b5
  URL_HASH SHA256=68ce87bb59ea209eb7350f41a94a27519ce16b37011b475a1e62d4abee154b66
  LIST_SEPARATOR |
  CMAKE_ARGS ${dependency_args}
    -DFT_DISABLE_HARFBUZZ:BOOL=ON -DFT_DISABLE_ZLIB:BOOL=ON
    -DFT_DISABLE_BZIP2:BOOL=ON -DFT_DISABLE_PNG:BOOL=ON
    -DFT_DISABLE_BROTLI:BOOL=ON
  INSTALL_COMMAND "${CMAKE_COMMAND}" --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE})
dependency_license(freetype . docs/FTL.TXT docs/GPLv2.TXT LICENSE.TXT)

ExternalProject_Add(harfbuzz
  DEPENDS freetype
  URL https://codeload.github.com/harfbuzz/harfbuzz/tar.gz/43a778492219032b9d747112e412dec2201d2667
  URL_HASH SHA256=fd4adecafebea87b72c515bd5d069a60673552da69b3052f9f64033b1af0528d
  LIST_SEPARATOR |
  CMAKE_ARGS ${dependency_args}
    -DHB_HAVE_FREETYPE:BOOL=ON -DHB_HAVE_GLIB:BOOL=OFF
    -DHB_HAVE_GOBJECT:BOOL=OFF -DHB_HAVE_ICU:BOOL=OFF
    -DHB_HAVE_CAIRO:BOOL=OFF -DHB_BUILD_UTILS:BOOL=OFF
    -DHB_BUILD_SUBSET:BOOL=OFF
    -DFREETYPE_INCLUDE_DIR_freetype2:PATH=${CUI_DEPS_PREFIX}/include/freetype2
    -DFREETYPE_INCLUDE_DIR_ft2build:PATH=${CUI_DEPS_PREFIX}/include/freetype2
  INSTALL_COMMAND "${CMAKE_COMMAND}" --install <BINARY_DIR> --config ${CMAKE_BUILD_TYPE})
dependency_license(harfbuzz . COPYING)
