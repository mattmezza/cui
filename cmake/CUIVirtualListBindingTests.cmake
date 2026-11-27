option(CUI_VIRTUAL_LIST_BINDING_TESTS "Check Go and Rust live VirtualList rows on Linux" OFF)
if(NOT CUI_VIRTUAL_LIST_BINDING_TESTS)
 return()
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
 message(FATAL_ERROR "VirtualList binding provider tests require Linux")
endif()

find_program(CUI_VIRTUAL_LIST_GO go REQUIRED)
find_program(CUI_VIRTUAL_LIST_CARGO cargo REQUIRED)
set(CUI_VIRTUAL_LIST_LEGACY_LIBRARY "" CACHE FILEPATH "Optional released provider without live VirtualList rows")
function(cui_virtual_list_binding_test name directory)
 add_test(NAME bridge_${name} COMMAND ${CMAKE_COMMAND} -E env ${ARGN})
 set_tests_properties(bridge_${name} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/${directory}" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

set(_go_env GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local)
set(_go_test test ./cui -count=1 -run "^TestVirtualList")
set(_rust_env --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH --unset=LD_PRELOAD RUSTFLAGS=-Dwarnings)
set(_rust_test test --offline --locked -j1 --test virtual_list -- --test-threads=1)
set(_rust_example run --offline --locked -j1 --example virtual_list_inbox)

set(_current "$<TARGET_FILE:cui_bridge>")
set(_current_dir "$<TARGET_FILE_DIR:cui_bridge>")
set(_rust_shared "${CUI_RUST_SKELETON_TARGET_DIR}")
set(_rust_bundled "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}")

cui_virtual_list_binding_test(go_virtual_list_shared go ${_go_env}
 CUI_TEST_LEGACY_VIRTUAL_LIST=0 CUI_EXPECT_VIRTUAL_AXIS=1 CUI_BRIDGE_LIBRARY=${_current}
 ${CUI_VIRTUAL_LIST_GO} ${_go_test} -tags cui_dynamic)
cui_virtual_list_binding_test(go_virtual_list_bundled go ${_go_env}
 CUI_TEST_LEGACY_VIRTUAL_LIST=0 CUI_EXPECT_VIRTUAL_AXIS=1 CUI_BRIDGE_LIBRARY=
 ${CUI_VIRTUAL_LIST_GO} ${_go_test})
cui_virtual_list_binding_test(go_virtual_list_example go ${_go_env}
 CUI_BRIDGE_LIBRARY= ${CUI_VIRTUAL_LIST_GO} run ./examples/virtual_list_inbox)

cui_virtual_list_binding_test(rust_virtual_list_shared rust ${_rust_env}
 CUI_TEST_LEGACY_VIRTUAL_LIST=0 CUI_EXPECT_VIRTUAL_AXIS=1 CUI_BRIDGE_LIB_DIR=${_current_dir}
 CARGO_TARGET_DIR=${_rust_shared} ${CUI_VIRTUAL_LIST_CARGO} ${_rust_test})
cui_virtual_list_binding_test(rust_virtual_list_bundled rust ${_rust_env}
 CUI_TEST_LEGACY_VIRTUAL_LIST=0 CUI_EXPECT_VIRTUAL_AXIS=1
 CARGO_TARGET_DIR=${_rust_bundled} ${CUI_VIRTUAL_LIST_CARGO} ${_rust_test})
cui_virtual_list_binding_test(rust_virtual_list_example rust ${_rust_env}
 CARGO_TARGET_DIR=${_rust_bundled} ${CUI_VIRTUAL_LIST_CARGO} ${_rust_example})

if(CUI_VIRTUAL_LIST_LEGACY_LIBRARY)
 if(NOT EXISTS "${CUI_VIRTUAL_LIST_LEGACY_LIBRARY}" OR IS_DIRECTORY "${CUI_VIRTUAL_LIST_LEGACY_LIBRARY}")
  message(FATAL_ERROR "CUI_VIRTUAL_LIST_LEGACY_LIBRARY must name an existing shared provider")
 endif()
 set(_legacy "${CMAKE_CURRENT_BINARY_DIR}/virtual-list-legacy")
 file(MAKE_DIRECTORY "${_legacy}")
 file(CREATE_LINK "${CUI_VIRTUAL_LIST_LEGACY_LIBRARY}" "${_legacy}/libcui_bridge.so" SYMBOLIC)
 add_test(NAME bridge_python_virtual_list_legacy COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python
  CUI_BRIDGE_LIBRARY=${CUI_VIRTUAL_LIST_LEGACY_LIBRARY} CUI_EXPECT_VIRTUAL_LEGACY=1 CUI_EXPECT_VIRTUAL_AXIS=0
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_virtual_list_rows.py)
 cui_virtual_list_binding_test(go_virtual_list_legacy go ${_go_env}
  CUI_TEST_LEGACY_VIRTUAL_LIST=1 CUI_EXPECT_VIRTUAL_AXIS=0 CUI_BRIDGE_LIBRARY=${CUI_VIRTUAL_LIST_LEGACY_LIBRARY}
  ${CUI_VIRTUAL_LIST_GO} ${_go_test} -tags cui_dynamic)
 cui_virtual_list_binding_test(rust_virtual_list_legacy rust ${_rust_env}
  CUI_TEST_LEGACY_VIRTUAL_LIST=1 CUI_EXPECT_VIRTUAL_AXIS=0 CUI_BRIDGE_LIB_DIR=${_current_dir} LD_LIBRARY_PATH=${_legacy}
  CARGO_TARGET_DIR=${_rust_shared} ${CUI_VIRTUAL_LIST_CARGO} ${_rust_test})
endif()
