cmake_minimum_required(VERSION 3.20)
project(cui_node_bindings LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(CUI_BUILD_NATIVE "Build SDL3 backend" ON)
add_subdirectory(../bridge cui-bridge-build)
find_program(NODE_EXECUTABLE node REQUIRED)
get_filename_component(CUI_NODE_EXECUTABLE_REAL "${NODE_EXECUTABLE}" REALPATH)
get_filename_component(CUI_NODE_BIN_DIR "${CUI_NODE_EXECUTABLE_REAL}" DIRECTORY)
find_path(NODE_API_INCLUDE_DIR node_api.h
 HINTS "${CUI_NODE_BIN_DIR}/../include/node" PATHS /usr/include/node REQUIRED)
add_library(cui_node_bindings MODULE addon.c)
target_include_directories(cui_node_bindings PRIVATE ${NODE_API_INCLUDE_DIR})
target_link_libraries(cui_node_bindings PRIVATE cui_bridge)
if(WIN32)
  find_library(NODE_IMPORT_LIBRARY NAMES node libnode PATHS ${NODE_API_INCLUDE_DIR}/../lib ${NODE_API_INCLUDE_DIR}/..)
  if(NOT NODE_IMPORT_LIBRARY)
    message(FATAL_ERROR "Node import library (node.lib) required for Windows N-API addon")
  endif()
  target_link_libraries(cui_node_bindings PRIVATE ${NODE_IMPORT_LIBRARY})
  add_custom_command(TARGET cui_node_bindings POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:cui_bridge>
    $<TARGET_FILE_DIR:cui_node_bindings>)
elseif(APPLE)
  target_link_options(cui_node_bindings PRIVATE "LINKER:-undefined,dynamic_lookup")
else()
  target_link_libraries(cui_node_bindings PRIVATE m)
endif()
set_target_properties(cui_node_bindings PROPERTIES PREFIX "" OUTPUT_NAME "cui" SUFFIX ".node")
if(MSVC)
  target_compile_options(cui_node_bindings PRIVATE /W4 /WX)
else()
  target_compile_options(cui_node_bindings PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_test(NAME node_binding COMMAND ${CMAKE_COMMAND} -E env CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} --expose-gc ${CMAKE_CURRENT_SOURCE_DIR}/test/binding.test.cjs)
add_test(NAME node_addon_safety COMMAND ${CMAKE_COMMAND} -E env CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} --expose-gc ${CMAKE_CURRENT_SOURCE_DIR}/test/addon.test.cjs)
set_tests_properties(node_binding node_addon_safety PROPERTIES DEPENDS bridge_core)
if(CUI_BRIDGE_NATIVE_TESTS)
 add_test(NAME node_native COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> CUI_FONT_PATH=${CUI_TEST_FONT}
  ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/native.test.cjs)
endif()

add_test(NAME node_components COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/components.test.cjs)
add_test(NAME node_extended COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/extended.test.cjs)
add_test(NAME node_models COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/models.test.cjs)
add_test(NAME node_workspace COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/examples/workspace.cjs --headless)

if(CUI_BRIDGE_NATIVE_TESTS)
 add_test(NAME node_clipboard_native COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> CUI_TEST_FONT=${CUI_TEST_FONT}
  ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/clipboard-native.test.cjs)
endif()

add_test(NAME node_sidebar_workspace COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/examples/sidebar-workspace.cjs --headless)
