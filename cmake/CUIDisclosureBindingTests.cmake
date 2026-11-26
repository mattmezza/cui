add_test(NAME bridge_python_disclosure COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_disclosure.py)
add_test(NAME bridge_python_disclosure_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/examples/workspace_preferences.py)

option(CUI_DISCLOSURE_BINDING_TESTS "Check Go and Rust retained disclosure on Linux" OFF)
if(NOT CUI_DISCLOSURE_BINDING_TESTS)
 return()
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
 message(FATAL_ERROR "Compiled disclosure binding tests require Linux")
endif()
find_program(CUI_DISCLOSURE_GO go REQUIRED)
find_program(CUI_DISCLOSURE_CARGO cargo REQUIRED)
set(CUI_DISCLOSURE_LEGACY_LIBRARY "" CACHE FILEPATH "Optional released provider without Disclosure V1")

function(cui_go_disclosure_test name library legacy)
 add_test(NAME bridge_go_disclosure_${name} COMMAND ${CMAKE_COMMAND} -E env
  GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
  CUI_BRIDGE_LIBRARY=${library} CUI_TEST_LEGACY_DISCLOSURE=${legacy}
  ${CUI_DISCLOSURE_GO} ${ARGN})
 set_tests_properties(bridge_go_disclosure_${name} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

function(cui_rust_disclosure_test name target legacy)
 add_test(NAME bridge_rust_disclosure_${name} COMMAND ${CMAKE_COMMAND} -E env
  --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH --unset=LD_PRELOAD
  CARGO_TARGET_DIR=${target} RUSTFLAGS=-Dwarnings CUI_TEST_LEGACY_DISCLOSURE=${legacy} ${ARGN})
 set_tests_properties(bridge_rust_disclosure_${name} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

set(_disclosure_go_test test ./cui -count=1 -run "^TestDisclosure(Presentation|HeadingAndDivider)$")
set(_disclosure_rust_test ${CUI_DISCLOSURE_CARGO} test --offline --locked -j1 --test disclosure)
set(_disclosure_rust_shared CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>)
set(_disclosure_rust_dynamic "${CUI_RUST_SKELETON_TARGET_DIR}")
set(_disclosure_rust_bundled "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}")
cui_go_disclosure_test(dynamic $<TARGET_FILE:cui_bridge> 0 ${_disclosure_go_test} -tags cui_dynamic)
cui_go_disclosure_test(bundled "" 0 ${_disclosure_go_test})
cui_rust_disclosure_test(dynamic "${_disclosure_rust_dynamic}" 0 ${_disclosure_rust_shared} ${_disclosure_rust_test})
cui_rust_disclosure_test(bundled "${_disclosure_rust_bundled}" 0 ${_disclosure_rust_test})
cui_go_disclosure_test(example "" 0 run ./examples/disclosure_preferences)
cui_rust_disclosure_test(example "${_disclosure_rust_bundled}" 0
 ${CUI_DISCLOSURE_CARGO} run --offline --locked -j1 --example disclosure_preferences)

if(CUI_DISCLOSURE_LEGACY_LIBRARY)
 if(NOT EXISTS "${CUI_DISCLOSURE_LEGACY_LIBRARY}" OR IS_DIRECTORY "${CUI_DISCLOSURE_LEGACY_LIBRARY}")
  message(FATAL_ERROR "CUI_DISCLOSURE_LEGACY_LIBRARY must name an existing shared provider")
 endif()
 set(_disclosure_legacy "${CMAKE_CURRENT_BINARY_DIR}/disclosure-legacy")
 file(MAKE_DIRECTORY "${_disclosure_legacy}")
 file(CREATE_LINK "${CUI_DISCLOSURE_LEGACY_LIBRARY}" "${_disclosure_legacy}/libcui_bridge.so" SYMBOLIC)
 cui_go_disclosure_test(legacy "${CUI_DISCLOSURE_LEGACY_LIBRARY}" 1 ${_disclosure_go_test} -tags cui_dynamic)
 cui_rust_disclosure_test(legacy "${_disclosure_rust_dynamic}" 1
  ${_disclosure_rust_shared} LD_LIBRARY_PATH=${_disclosure_legacy} ${_disclosure_rust_test})
 add_test(NAME bridge_python_disclosure_legacy COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=${CUI_DISCLOSURE_LEGACY_LIBRARY}
  CUI_TEST_LEGACY_DISCLOSURE=1 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_disclosure.py)
endif()
