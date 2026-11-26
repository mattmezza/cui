option(CUI_RATING_BINDING_TESTS "Check retained Rating providers with Go and Rust on Linux" OFF)
if(NOT CUI_RATING_BINDING_TESTS)
 return()
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
 message(FATAL_ERROR "Rating binding provider tests require Linux")
endif()

find_program(CUI_RATING_GO go REQUIRED)
find_program(CUI_RATING_CARGO cargo REQUIRED)
set(CUI_RATING_LEGACY_LIBRARY "" CACHE FILEPATH "Optional released provider without Rating V1")
function(cui_go_rating_test suffix library unsupported)
 add_test(NAME bridge_go_rating_${suffix} COMMAND ${CMAKE_COMMAND} -E env
  GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
  CUI_BRIDGE_LIBRARY=${library} CUI_TEST_UNSUPPORTED_RATING=${unsupported}
  ${CUI_RATING_GO} ${ARGN})
 set_tests_properties(bridge_go_rating_${suffix} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

function(cui_rust_rating_test suffix target unsupported)
 add_test(NAME bridge_rust_rating_${suffix} COMMAND ${CMAKE_COMMAND} -E env
  --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH --unset=LD_PRELOAD
  CARGO_TARGET_DIR=${target} RUSTFLAGS=-Dwarnings CUI_TEST_UNSUPPORTED_RATING=${unsupported} ${ARGN})
 set_tests_properties(bridge_rust_rating_${suffix} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

set(_go_rating_test test ./cui -count=1 -run "^TestRating")
set(_rust_rating_test ${CUI_RATING_CARGO} test --offline --locked -j1 --test rating_options -- --test-threads=1)
set(_rust_rating_example ${CUI_RATING_CARGO} run --offline --locked -j1 --example rating_options)
set(_rust_rating_shared "${CUI_RUST_SKELETON_TARGET_DIR}")
set(_rust_rating_static "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}")
set(_rust_rating_current CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>)

cui_go_rating_test(dynamic $<TARGET_FILE:cui_bridge> 0 ${_go_rating_test} -tags cui_dynamic)
cui_go_rating_test(bundled "" 0 ${_go_rating_test})
cui_go_rating_test(example "" 0 run ./examples/rating_options)
cui_rust_rating_test(dynamic "${_rust_rating_shared}" 0 ${_rust_rating_current} ${_rust_rating_test})
cui_rust_rating_test(bundled "${_rust_rating_static}" 0 ${_rust_rating_test})
cui_rust_rating_test(example "${_rust_rating_static}" 0 ${_rust_rating_example})

if(CUI_RATING_LEGACY_LIBRARY)
 if(NOT EXISTS "${CUI_RATING_LEGACY_LIBRARY}" OR IS_DIRECTORY "${CUI_RATING_LEGACY_LIBRARY}")
  message(FATAL_ERROR "CUI_RATING_LEGACY_LIBRARY must name an existing shared provider")
 endif()
 set(_rating_legacy "${CMAKE_CURRENT_BINARY_DIR}/rating-legacy")
 file(MAKE_DIRECTORY "${_rating_legacy}")
 file(CREATE_LINK "${CUI_RATING_LEGACY_LIBRARY}" "${_rating_legacy}/libcui_bridge.so" SYMBOLIC)
 cui_go_rating_test(legacy "${CUI_RATING_LEGACY_LIBRARY}" 1 ${_go_rating_test} -tags cui_dynamic)
 cui_go_rating_test(example_legacy "${CUI_RATING_LEGACY_LIBRARY}" 1 run -tags cui_dynamic ./examples/rating_options)
 cui_rust_rating_test(legacy "${_rust_rating_shared}" 1
  ${_rust_rating_current} LD_LIBRARY_PATH=${_rating_legacy} ${_rust_rating_test})
 cui_rust_rating_test(example_legacy "${_rust_rating_shared}" 1
  ${_rust_rating_current} LD_LIBRARY_PATH=${_rating_legacy} ${_rust_rating_example})

 add_test(NAME bridge_python_rating_legacy COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=${CUI_RATING_LEGACY_LIBRARY}
  CUI_RATING_UNSUPPORTED=1 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_rating_options.py)
 add_test(NAME bridge_python_rating_example_legacy COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=${CUI_RATING_LEGACY_LIBRARY}
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/examples/rating_options.py)
endif()
