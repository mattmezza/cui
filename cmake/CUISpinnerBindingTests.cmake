set(CUI_SPINNER_LEGACY_LIBRARY "" CACHE FILEPATH "Optional released bridge without Spinner V1")
set(CUI_SPINNER_PARTIAL_LIBRARY "" CACHE FILEPATH "Optional shared bridge with incomplete Spinner V1")
foreach(kind LEGACY PARTIAL)
 if(CUI_SPINNER_${kind}_LIBRARY AND
    (NOT EXISTS "${CUI_SPINNER_${kind}_LIBRARY}" OR IS_DIRECTORY "${CUI_SPINNER_${kind}_LIBRARY}"))
  message(FATAL_ERROR "CUI_SPINNER_${kind}_LIBRARY must name an existing shared provider")
 endif()
endforeach()
add_test(NAME bridge_python_spinner COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 CUI_SPINNER_LEGACY_LIBRARY=${CUI_SPINNER_LEGACY_LIBRARY}
 CUI_SPINNER_PARTIAL_LIBRARY=${CUI_SPINNER_PARTIAL_LIBRARY}
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_spinner_presentation.py)
add_test(NAME bridge_python_spinner_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/spinner_status.py)

option(CUI_SPINNER_BINDING_TESTS "Check retained Spinner with Go and Rust on Linux" OFF)
if(NOT CUI_SPINNER_BINDING_TESTS)
 return()
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
 message(FATAL_ERROR "Spinner Go and Rust provider tests require Linux")
endif()
find_program(CUI_SPINNER_GO go REQUIRED)
find_program(CUI_SPINNER_CARGO cargo REQUIRED)

function(cui_spinner_go_test suffix library unsupported)
 add_test(NAME bridge_go_spinner_${suffix} COMMAND ${CMAKE_COMMAND} -E env
  GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
  CUI_BRIDGE_LIBRARY=${library} CUI_TEST_UNSUPPORTED_SPINNER=${unsupported}
  ${CUI_SPINNER_GO} ${ARGN})
 set_tests_properties(bridge_go_spinner_${suffix} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

function(cui_spinner_rust_test suffix target unsupported)
 add_test(NAME bridge_rust_spinner_${suffix} COMMAND ${CMAKE_COMMAND} -E env
  --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH --unset=LD_PRELOAD
  CARGO_TARGET_DIR=${target} RUSTFLAGS=-Dwarnings CUI_TEST_UNSUPPORTED_SPINNER=${unsupported} ${ARGN})
 set_tests_properties(bridge_rust_spinner_${suffix} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

set(_go_test test ./cui -count=1 -run "^Test(RotatingSpinner|SpinnerConfigurationValidation)$")
set(_rust_test ${CUI_SPINNER_CARGO} test --offline --locked -j1 --test spinner_presentation)
set(_rust_shared "${CUI_RUST_SKELETON_TARGET_DIR}")
set(_rust_bundled "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}")
set(_rust_current CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>)
cui_spinner_go_test(dynamic $<TARGET_FILE:cui_bridge> 0 ${_go_test} -tags cui_dynamic)
cui_spinner_go_test(bundled "" 0 ${_go_test})
cui_spinner_go_test(example "" 0 run ./examples/spinner_loading)
cui_spinner_rust_test(dynamic "${_rust_shared}" 0 ${_rust_current} ${_rust_test})
cui_spinner_rust_test(bundled "${_rust_bundled}" 0 ${_rust_test})
cui_spinner_rust_test(example "${_rust_bundled}" 0
 ${CUI_SPINNER_CARGO} run --offline --locked -j1 --example spinner_loading)

foreach(kind LEGACY PARTIAL)
 set(provider "${CUI_SPINNER_${kind}_LIBRARY}")
 if(provider)
  string(TOLOWER ${kind} suffix)
  set(provider_dir "${CMAKE_CURRENT_BINARY_DIR}/spinner-providers/${suffix}")
  file(MAKE_DIRECTORY "${provider_dir}")
  file(CREATE_LINK "${provider}" "${provider_dir}/libcui_bridge.so" SYMBOLIC)
  cui_spinner_go_test(${suffix} "${provider}" 1 ${_go_test} -tags cui_dynamic)
  cui_spinner_rust_test(${suffix} "${_rust_shared}" 1 ${_rust_current}
   LD_LIBRARY_PATH=${provider_dir} ${_rust_test})
 endif()
endforeach()
