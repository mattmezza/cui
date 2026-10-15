# SPDX-License-Identifier: MIT
include_guard(GLOBAL)
option(CUI_MACOS_BUNDLE "Package the native starter as a macOS application" "${APPLE}")
set(_cui_deploy_default OFF)
if(APPLE AND CMAKE_HOST_APPLE AND NOT CMAKE_CROSSCOMPILING)
  set(_cui_deploy_default ON)
endif()
option(CUI_MACOS_DEPLOY_DEPENDENCIES "Copy native macOS runtime dependencies into the app" ${_cui_deploy_default})
unset(_cui_deploy_default)

# FONT_DIR must contain the pinned Inter fonts and their license. This helper
# works at configure time for Darwin cross builds; deployment requires macOS.
function(cui_macos_bundle target)
  cmake_parse_arguments(BUNDLE "" "FONT_DIR" "" ${ARGN})
  if(NOT CUI_MACOS_BUNDLE)
    return()
  endif()
  if(NOT APPLE)
    message(FATAL_ERROR "CUI_MACOS_BUNDLE requires a Darwin target toolchain")
  endif()
  if(NOT BUNDLE_FONT_DIR)
    set(BUNDLE_FONT_DIR "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../third_party/inter")
  endif()
  foreach(asset Inter-Regular.otf Inter-Bold.otf LICENSE.txt manifest.json)
    set(path "${BUNDLE_FONT_DIR}/${asset}")
    if(NOT EXISTS "${path}")
      message(FATAL_ERROR "Missing pinned bundle resource: ${path}")
    endif()
    target_sources(${target} PRIVATE "${path}")
    set_source_files_properties("${path}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/inter")
  endforeach()
  set_target_properties(${target} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "CUI Native Starter"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.cui.native-starter"
    MACOSX_BUNDLE_BUNDLE_VERSION "1"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0")
  install(TARGETS ${target} BUNDLE DESTINATION .)
  if(CUI_MACOS_DEPLOY_DEPENDENCIES)
    if(CMAKE_CROSSCOMPILING OR NOT CMAKE_HOST_APPLE)
      message(FATAL_ERROR "Dependency deployment requires a native macOS build; cross builds must stage target dylibs and fix install names separately")
    endif()
    # Only run during installation, after the bundle and its resources exist.
    install(CODE "include(BundleUtilities)\nfixup_bundle(\"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/$<TARGET_BUNDLE_DIR_NAME:${target}>\" \"\" \"$<TARGET_FILE_DIR:${target}>\")")
  elseif(CMAKE_CROSSCOMPILING)
    message(STATUS "macOS cross bundle: runtime dylib deployment and signing are not performed")
  endif()
endfunction()
