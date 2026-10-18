# Shared by the optional root target and standalone language projects.
get_filename_component(CUI_BRIDGE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(cui_bridge SHARED ${CUI_BRIDGE_ROOT}/src/bridge/cui_bridge.c)
target_compile_definitions(cui_bridge PRIVATE CUI_BRIDGE_BUILD=1)
target_include_directories(cui_bridge PUBLIC ${CUI_BRIDGE_ROOT}/include PRIVATE ${CUI_BRIDGE_ROOT}/src/bridge)
target_link_libraries(cui_bridge PRIVATE cui)
if(TARGET cui_sdl3)
 target_sources(cui_bridge PRIVATE ${CUI_BRIDGE_ROOT}/src/bridge/cui_bridge_sdl.c)
 target_compile_definitions(cui_bridge PRIVATE CUI_BRIDGE_WITH_SDL=1)
 target_link_libraries(cui_bridge PRIVATE cui_sdl3)
endif()
cui_warnings(cui_bridge)
add_executable(test_bridge_core ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_core.c)
target_link_libraries(test_bridge_core PRIVATE cui_bridge)
cui_warnings(test_bridge_core)
add_test(NAME bridge_core COMMAND test_bridge_core)
add_executable(test_bridge_components ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_components.c)
target_link_libraries(test_bridge_components PRIVATE cui_bridge)
cui_warnings(test_bridge_components)
add_test(NAME bridge_components COMMAND test_bridge_components)
add_test(NAME bridge_python
 COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python
  CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_bridge.py)
option(CUI_BRIDGE_NATIVE_TESTS "Register opt-in Linux invisible native binding checks" OFF)
if(CUI_BRIDGE_NATIVE_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT TARGET cui_sdl3 OR NOT CUI_TEST_FONT)
  message(FATAL_ERROR "Binding native checks require Linux, SDL/text adapters and CUI_TEST_FONT")
 endif()
 foreach(tool xwininfo xwd xdotool convert)
  find_program(CUI_BRIDGE_${tool} ${tool} REQUIRED)
 endforeach()
 add_test(NAME bridge_python_native COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python
  CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge> CUI_TEST_FONT=${CUI_TEST_FONT}
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_native_virtual.py)
endif()

add_test(NAME bridge_python_components COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_components.py)

# ASan instrumentation has a separate stack budget; native Windows defaults to 1 MB.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CUI_SANITIZE)
 add_test(NAME bridge_small_stack COMMAND ${Python3_EXECUTABLE}
  ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_small_stack.py
  $<TARGET_FILE:test_bridge_core> $<TARGET_FILE:test_bridge_components>)
endif()
add_test(NAME bridge_python_extended COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_extended_components.py)
add_test(NAME bridge_python_models COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_model_components.py)
add_test(NAME bridge_python_workspace COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/workspace.py --headless)

if(TARGET cui_sdl3)
 add_executable(test_bridge_components_sdl ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_components_sdl.c)
 target_link_libraries(test_bridge_components_sdl PRIVATE cui_bridge)
 cui_warnings(test_bridge_components_sdl)
 if(CUI_BRIDGE_NATIVE_TESTS)
  add_test(NAME bridge_components_sdl COMMAND test_bridge_components_sdl)
  set_tests_properties(bridge_components_sdl PROPERTIES SKIP_RETURN_CODE 77)
 endif()
endif()
if(CUI_BRIDGE_NATIVE_TESTS)
 add_test(NAME bridge_python_image_native COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  CUI_TEST_FONT=${CUI_TEST_FONT}
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_image_native_virtual.py)
endif()

if(CUI_BRIDGE_NATIVE_TESTS)
 add_test(NAME bridge_python_clipboard_native COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  CUI_TEST_FONT=${CUI_TEST_FONT}
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_clipboard_native_virtual.py)
endif()

add_test(NAME bridge_python_sidebar_workspace COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/sidebar_workspace.py --headless)

add_test(NAME bridge_python_notifications_v1 COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_notifications_v1.py)
