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
add_executable(test_bridge_slider_model ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_slider_model.c)
target_link_libraries(test_bridge_slider_model PRIVATE cui_bridge)
cui_warnings(test_bridge_slider_model)
add_test(NAME bridge_slider_model COMMAND test_bridge_slider_model)
add_executable(test_bridge_slider_model_abi ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_slider_model_abi.c)
cui_warnings(test_bridge_slider_model_abi)
target_include_directories(test_bridge_slider_model_abi PRIVATE ${CUI_BRIDGE_ROOT}/include)
add_test(NAME bridge_slider_model_abi COMMAND test_bridge_slider_model_abi)
add_executable(test_bridge_slider_model_internal ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_slider_model_internal.c)
target_include_directories(test_bridge_slider_model_internal PRIVATE ${CUI_BRIDGE_ROOT}/include)
target_link_libraries(test_bridge_slider_model_internal PRIVATE cui)
cui_warnings(test_bridge_slider_model_internal)
add_test(NAME bridge_slider_model_internal COMMAND test_bridge_slider_model_internal)
add_executable(test_bridge_slider_model_unsupported ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_slider_model_unsupported.c)
target_include_directories(test_bridge_slider_model_unsupported PRIVATE ${CUI_BRIDGE_ROOT}/include)
target_link_libraries(test_bridge_slider_model_unsupported PRIVATE cui)
cui_warnings(test_bridge_slider_model_unsupported)
add_test(NAME bridge_slider_model_unsupported COMMAND test_bridge_slider_model_unsupported)
add_executable(test_bridge_slider_legacy_trace ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_slider_legacy_trace.c)
target_link_libraries(test_bridge_slider_legacy_trace PRIVATE cui_bridge)
cui_warnings(test_bridge_slider_legacy_trace)
add_test(NAME bridge_slider_legacy_trace COMMAND test_bridge_slider_legacy_trace)
add_test(NAME bridge_slider_legacy_frozen COMMAND ${Python3_EXECUTABLE}
 ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_slider_legacy_frozen.py $<TARGET_FILE:test_bridge_slider_legacy_trace>)
add_test(NAME bridge_slider_model_thread COMMAND ${Python3_EXECUTABLE}
 ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_slider_model_thread.py $<TARGET_FILE:cui_bridge>)
add_executable(bridge_slider_model_example ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/slider_model.c)
target_link_libraries(bridge_slider_model_example PRIVATE cui_bridge)
cui_warnings(bridge_slider_model_example)
add_test(NAME bridge_slider_model_example_accept COMMAND bridge_slider_model_example)
add_test(NAME bridge_slider_model_example_reject COMMAND bridge_slider_model_example --reject)
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

add_test(NAME bridge_python_slider_model COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_slider_model.py)
foreach(mode accept reject_vertical single)
 if(mode STREQUAL "reject_vertical")
  set(slider_example_args --reject --vertical)
 elseif(mode STREQUAL "single")
  set(slider_example_args --single)
 else()
  set(slider_example_args)
 endif()
 add_test(NAME bridge_python_slider_example_${mode} COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/slider_model.py ${slider_example_args})
endforeach()

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

add_test(NAME bridge_python_value_notifications COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_value_notifications_v1.py)

add_executable(test_bridge_window_visibility ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_window_visibility.c)
target_link_libraries(test_bridge_window_visibility PRIVATE cui_bridge)
cui_warnings(test_bridge_window_visibility)
add_test(NAME bridge_window_visibility COMMAND test_bridge_window_visibility)
if(TARGET cui_sdl3)
 add_executable(test_bridge_window_visibility_sdl ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_window_visibility_sdl.c)
 target_link_libraries(test_bridge_window_visibility_sdl PRIVATE cui_bridge ${CUI_SDL_TARGET})
 cui_warnings(test_bridge_window_visibility_sdl)
 if(CUI_BRIDGE_NATIVE_TESTS)
  add_test(NAME bridge_window_visibility_sdl COMMAND ${CMAKE_COMMAND} -E env CUI_TEST_FONT=${CUI_TEST_FONT} $<TARGET_FILE:test_bridge_window_visibility_sdl>)
  add_test(NAME bridge_python_window_visibility COMMAND ${CMAKE_COMMAND} -E env
   PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
   CUI_TEST_FONT=${CUI_TEST_FONT} ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_window_visibility_native_virtual.py)
 endif()
endif()

add_test(NAME bridge_python_shadows COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_shadows.py)

add_test(NAME bridge_python_part_shadows COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_part_shadows.py)

# Optional actual-old-module proof. No historical binary is downloaded or vendored.
set(CUI_GO_SLIDER_OLD_LIBRARY "" CACHE FILEPATH "Actual pre-typed-Slider library for optional Go protocol checks")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CUI_GO_SLIDER_OLD_LIBRARY)
 if(NOT EXISTS "${CUI_GO_SLIDER_OLD_LIBRARY}")
  message(FATAL_ERROR "CUI_GO_SLIDER_OLD_LIBRARY must name an existing actual older bridge")
 endif()
 find_program(CUI_GO_SLIDER_EXECUTABLE go REQUIRED)
 add_test(NAME bridge_go_slider_protocol COMMAND ${Python3_EXECUTABLE}
  ${CUI_BRIDGE_ROOT}/bindings/go/tests/test_slider_protocol.py
  --build-dir ${CMAKE_BINARY_DIR} --library $<TARGET_FILE:cui_bridge>
  --old-library ${CUI_GO_SLIDER_OLD_LIBRARY}
  --go ${CUI_GO_SLIDER_EXECUTABLE} --cc ${CMAKE_C_COMPILER})
endif()

# Optional actual-old-module Rust protocol proof; uses existing tools offline.
set(CUI_RUST_SLIDER_OLD_LIBRARY "" CACHE FILEPATH "Actual pre-typed-Slider library for optional Rust protocol checks")
set(CUI_RUST_SLIDER_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-slider-target" CACHE PATH "Reused Cargo target directory for optional Rust protocol checks")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CUI_RUST_SLIDER_OLD_LIBRARY)
 if(NOT EXISTS "${CUI_RUST_SLIDER_OLD_LIBRARY}")
  message(FATAL_ERROR "CUI_RUST_SLIDER_OLD_LIBRARY must name an existing actual older bridge")
 endif()
 find_program(CUI_RUST_SLIDER_CARGO cargo REQUIRED)
 file(MAKE_DIRECTORY "${CUI_RUST_SLIDER_TARGET_DIR}")
 add_test(NAME bridge_rust_slider_protocol COMMAND ${Python3_EXECUTABLE}
  ${CUI_BRIDGE_ROOT}/bindings/rust/tests/test_slider_protocol.py
  --build-dir ${CMAKE_BINARY_DIR} --target-dir ${CUI_RUST_SLIDER_TARGET_DIR}
  --old-library ${CUI_RUST_SLIDER_OLD_LIBRARY}
  --cargo ${CUI_RUST_SLIDER_CARGO} --cc ${CMAKE_C_COMPILER})
endif()

# Additive localized kind55 route; tests use a simulated host, never a desktop window.
add_executable(test_bridge_time_field ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_time_field.c)
target_include_directories(test_bridge_time_field PRIVATE ${CUI_BRIDGE_ROOT}/include)
target_link_libraries(test_bridge_time_field PRIVATE cui)
cui_warnings(test_bridge_time_field)
add_test(NAME bridge_time_field COMMAND test_bridge_time_field)
add_executable(test_bridge_time_field_unsupported ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_time_field_unsupported.c)
target_include_directories(test_bridge_time_field_unsupported PRIVATE ${CUI_BRIDGE_ROOT}/include)
target_link_libraries(test_bridge_time_field_unsupported PRIVATE cui)
cui_warnings(test_bridge_time_field_unsupported)
add_test(NAME bridge_time_field_unsupported COMMAND test_bridge_time_field_unsupported)
add_executable(test_bridge_time_field_abi ${CUI_BRIDGE_ROOT}/tests/fixtures/time_field_bridge/abi.c)
target_include_directories(test_bridge_time_field_abi PRIVATE ${CUI_BRIDGE_ROOT}/include)
cui_warnings(test_bridge_time_field_abi)
add_test(NAME bridge_time_field_abi COMMAND test_bridge_time_field_abi)
add_test(NAME bridge_time_field_thread COMMAND ${Python3_EXECUTABLE}
 ${CUI_BRIDGE_ROOT}/tests/fixtures/time_field_bridge/thread.py $<TARGET_FILE:cui_bridge>)
add_executable(test_bridge_time_field_legacy ${CUI_BRIDGE_ROOT}/tests/fixtures/time_field_bridge/legacy.c)
target_link_libraries(test_bridge_time_field_legacy PRIVATE cui_bridge)
cui_warnings(test_bridge_time_field_legacy)
add_test(NAME bridge_time_field_legacy COMMAND test_bridge_time_field_legacy)
add_executable(bridge_time_field_example ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/time_field_localized.c)
target_link_libraries(bridge_time_field_example PRIVATE cui_bridge)
cui_warnings(bridge_time_field_example)
add_test(NAME bridge_time_field_example COMMAND bridge_time_field_example)
add_test(NAME bridge_time_field_source_closure COMMAND ${Python3_EXECUTABLE}
 ${CUI_BRIDGE_ROOT}/tests/fixtures/time_field_bridge/closure.py)
get_property(_cui_bridge_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if("CXX" IN_LIST _cui_bridge_languages)
 foreach(order bridge_first widget_first)
  add_executable(test_bridge_time_field_header_${order} ${CUI_BRIDGE_ROOT}/tests/fixtures/time_field_bridge/header.cpp)
  target_include_directories(test_bridge_time_field_header_${order} PRIVATE ${CUI_BRIDGE_ROOT}/include)
  set_property(TARGET test_bridge_time_field_header_${order} PROPERTY CXX_STANDARD 11)
  if(order STREQUAL "widget_first")
   target_compile_definitions(test_bridge_time_field_header_${order} PRIVATE CUI_TIME_BRIDGE_WIDGET_FIRST=1)
  endif()
  cui_warnings(test_bridge_time_field_header_${order})
  add_test(NAME bridge_time_field_header_${order} COMMAND test_bridge_time_field_header_${order})
 endforeach()
endif()
# Matching Linux static source closure, distinct from installed Go/Rust API proof.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
 add_executable(bridge_time_field_example_static
  ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/time_field_localized.c
  ${CUI_BRIDGE_ROOT}/src/bridge/cui_bridge.c)
 target_include_directories(bridge_time_field_example_static PRIVATE ${CUI_BRIDGE_ROOT}/include)
 target_compile_definitions(bridge_time_field_example_static PRIVATE CUI_BRIDGE_BUILD=1)
 target_link_libraries(bridge_time_field_example_static PRIVATE cui)
 cui_warnings(bridge_time_field_example_static)
 add_test(NAME bridge_time_field_example_static COMMAND bridge_time_field_example_static)
endif()
# Optional independent pre-feature trace: build from the bounded freezer first.
set(CUI_TIME_FIELD_FROZEN_LEGACY_EXECUTABLE "" CACHE FILEPATH "Pre-localized bridge legacy trace executable")
set(CUI_TIME_FIELD_FROZEN_LEGACY_LIBRARY_DIR "" CACHE PATH "Directory of matching old libcui_bridge.so")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CUI_TIME_FIELD_FROZEN_LEGACY_EXECUTABLE)
 if(NOT EXISTS "${CUI_TIME_FIELD_FROZEN_LEGACY_EXECUTABLE}" OR NOT EXISTS "${CUI_TIME_FIELD_FROZEN_LEGACY_LIBRARY_DIR}/libcui_bridge.so")
  message(FATAL_ERROR "Frozen TimeField trace needs existing old executable and matching library directory")
 endif()
 add_test(NAME bridge_time_field_frozen_wrapped COMMAND ${Python3_EXECUTABLE}
  ${CUI_BRIDGE_ROOT}/tests/fixtures/time_field_bridge/legacy_compare.py
  --old-executable ${CUI_TIME_FIELD_FROZEN_LEGACY_EXECUTABLE}
  --new-executable $<TARGET_FILE:test_bridge_time_field_legacy>
  --old-library-dir ${CUI_TIME_FIELD_FROZEN_LEGACY_LIBRARY_DIR}
  --new-library-dir $<TARGET_FILE_DIR:cui_bridge>)
endif()

# Localized TimeField remains an independent optional symbol group.
set(CUI_PYTHON_TIME_FIELD_OLD_LIBRARY "" CACHE FILEPATH "Optional pre-localized bridge for Python compatibility proof")
set(CUI_PYTHON_TIME_FIELD_PARTIAL_LIBRARY "" CACHE FILEPATH "Optional partial-symbol bridge for Python compatibility proof")
add_test(NAME bridge_python_time_field COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 CUI_TIME_FIELD_OLD_LIBRARY=${CUI_PYTHON_TIME_FIELD_OLD_LIBRARY}
 CUI_TIME_FIELD_PARTIAL_LIBRARY=${CUI_PYTHON_TIME_FIELD_PARTIAL_LIBRARY}
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_time_field.py)
add_test(NAME bridge_python_time_field_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/time_field_localized.py)

# Opt-in localized facade acceptance: supplied historical/partial providers only.
# Builds language consumers offline, never another C engine or a desktop window.
option(CUI_TIME_FIELD_BINDING_TESTS "Validate Go/Rust localized TimeField against existing providers" OFF)
set(CUI_TIME_FIELD_OLD_LIBRARY "" CACHE FILEPATH "Actual pre-localized-TimeField bridge")
set(CUI_TIME_FIELD_PARTIAL_LIBRARY "" CACHE FILEPATH "Actual bridge with incomplete localized optional group")
set(CUI_TIME_FIELD_RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-time-field-target" CACHE PATH "Reusable Cargo target directory")
if(CUI_TIME_FIELD_BINDING_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Localized facade provider acceptance currently supports Linux only")
 endif()
 foreach(provider CUI_TIME_FIELD_OLD_LIBRARY CUI_TIME_FIELD_PARTIAL_LIBRARY)
  if(NOT EXISTS "${${provider}}" OR IS_DIRECTORY "${${provider}}")
   message(FATAL_ERROR "${provider} must name an existing provider; no download is performed")
  endif()
 endforeach()
 find_program(CUI_TIME_FIELD_GO go REQUIRED)
 find_program(CUI_TIME_FIELD_CARGO cargo REQUIRED)
 foreach(language go rust)
  add_test(NAME bridge_${language}_time_localized COMMAND ${Python3_EXECUTABLE}
   ${CUI_BRIDGE_ROOT}/bindings/go/tests/test_time_localized.py
   --language ${language} --build-dir ${CMAKE_BINARY_DIR}
   --library $<TARGET_FILE:cui_bridge>
   --old-library ${CUI_TIME_FIELD_OLD_LIBRARY}
   --partial-library ${CUI_TIME_FIELD_PARTIAL_LIBRARY}
   --target-dir ${CUI_TIME_FIELD_RUST_TARGET_DIR}
   --go ${CUI_TIME_FIELD_GO} --cargo ${CUI_TIME_FIELD_CARGO})
  set_tests_properties(bridge_${language}_time_localized PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
 endforeach()
endif()
add_test(NAME bridge_time_localized_runner_offline COMMAND ${Python3_EXECUTABLE}
 ${CUI_BRIDGE_ROOT}/bindings/go/tests/test_time_localized_runner.py)

option(CUI_TIME_FIELD_STATIC_BINDING_TESTS "Validate Go/Rust localized TimeField bundled source consumers" OFF)
set(CUI_TIME_FIELD_STATIC_RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-time-field-static-target" CACHE PATH "Reusable Cargo cache for bundled static localized consumers")
if(CUI_TIME_FIELD_STATIC_BINDING_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Static localized facade acceptance currently supports Linux only")
 endif()
 find_program(CUI_TIME_FIELD_GO go REQUIRED)
 find_program(CUI_TIME_FIELD_CARGO cargo REQUIRED)
 find_program(CUI_TIME_FIELD_READELF readelf REQUIRED)
 file(MAKE_DIRECTORY "${CUI_TIME_FIELD_STATIC_RUST_TARGET_DIR}")
 foreach(language go rust)
  add_test(NAME bridge_${language}_time_localized_static COMMAND ${Python3_EXECUTABLE}
   ${CUI_BRIDGE_ROOT}/bindings/go/tests/test_time_localized.py
   --language ${language} --linkage static --build-dir ${CMAKE_BINARY_DIR}
   --target-dir ${CUI_TIME_FIELD_STATIC_RUST_TARGET_DIR}
   --go ${CUI_TIME_FIELD_GO} --cargo ${CUI_TIME_FIELD_CARGO}
   --readelf ${CUI_TIME_FIELD_READELF})
  set_tests_properties(bridge_${language}_time_localized_static PROPERTIES RUN_SERIAL TRUE TIMEOUT 600)
 endforeach()
endif()
