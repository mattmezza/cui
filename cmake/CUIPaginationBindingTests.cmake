option(CUI_PAGINATION_BINDING_TESTS "Check Go and Rust Pagination provider compatibility on Linux" OFF)
if(NOT CUI_PAGINATION_BINDING_TESTS)
 return()
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
 message(FATAL_ERROR "Pagination binding provider tests require Linux")
endif()

find_program(CUI_PAGINATION_GO go REQUIRED)
find_program(CUI_PAGINATION_CARGO cargo REQUIRED)
set(CUI_PAGINATION_LEGACY_LIBRARY "" CACHE FILEPATH "Optional released bridge without Pagination V1")

function(cui_go_pagination_test suffix library unsupported)
 add_test(NAME bridge_go_pagination_${suffix} COMMAND ${CMAKE_COMMAND} -E env
  GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
  CUI_BRIDGE_LIBRARY=${library} CUI_TEST_UNSUPPORTED_PAGINATION=${unsupported}
  ${CUI_PAGINATION_GO} ${ARGN})
 set_tests_properties(bridge_go_pagination_${suffix} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

function(cui_rust_pagination_test suffix target unsupported)
 add_test(NAME bridge_rust_pagination_${suffix} COMMAND ${CMAKE_COMMAND} -E env
  --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH --unset=LD_PRELOAD
  CARGO_TARGET_DIR=${target} RUSTFLAGS=-Dwarnings
  CUI_TEST_UNSUPPORTED_PAGINATION=${unsupported}
  ${ARGN})
 set_tests_properties(bridge_rust_pagination_${suffix} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

cui_go_pagination_test(dynamic $<TARGET_FILE:cui_bridge> 0
 test -tags cui_dynamic ./cui -count=1 -run "^TestPagination")
cui_go_pagination_test(static "" 0 test ./cui -count=1 -run "^TestPagination")
cui_go_pagination_test(example "" 0 run ./examples/pagination_options)

set(_cui_rust_pagination_dynamic "${CUI_RUST_SKELETON_TARGET_DIR}")
set(_cui_rust_pagination_static "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}")
set(_cui_rust_pagination_current CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>)
set(_cui_rust_pagination_test ${CUI_PAGINATION_CARGO} test --offline --locked -j1 --test pagination_options)
cui_rust_pagination_test(dynamic "${_cui_rust_pagination_dynamic}" 0
 ${_cui_rust_pagination_current} ${_cui_rust_pagination_test} -- --test-threads=1)
cui_rust_pagination_test(wire "${_cui_rust_pagination_dynamic}" 0
 ${_cui_rust_pagination_current} ${CUI_PAGINATION_CARGO} test --offline --locked -j1 --lib pagination_options)
cui_rust_pagination_test(static "${_cui_rust_pagination_static}" 0
 ${_cui_rust_pagination_test} -- --test-threads=1)
cui_rust_pagination_test(example "${_cui_rust_pagination_static}" 0
 ${CUI_PAGINATION_CARGO} run --offline --locked -j1 --example pagination_options)

function(cui_pagination_provider_fixture target provider)
 add_library(${target} SHARED ${CUI_BRIDGE_ROOT}/bindings/tests/pagination_provider.c)
 target_include_directories(${target} PRIVATE ${CUI_BRIDGE_ROOT}/include)
 target_link_libraries(${target} PRIVATE "-Wl,--no-as-needed" ${provider} "-Wl,--as-needed")
 target_compile_definitions(${target} PRIVATE ${ARGN})
 cui_warnings(${target})
endfunction()

cui_pagination_provider_fixture(cui_pagination_bad_size cui_bridge)
cui_pagination_provider_fixture(cui_pagination_bad_version cui_bridge CUI_TEST_PAGINATION_BAD_VERSION)
foreach(mode bad_size bad_version)
 set(fixture $<TARGET_FILE:cui_pagination_${mode}>)
 cui_go_pagination_test(${mode} ${fixture} 1
  test -tags cui_dynamic ./cui -count=1 -run "^TestPagination(OptionalProvider|CustomSizeOptionalProvider)$")
 cui_rust_pagination_test(${mode} "${_cui_rust_pagination_dynamic}" 1
  ${_cui_rust_pagination_current} LD_PRELOAD=${fixture}
  ${_cui_rust_pagination_test} -- optional_provider --test-threads=1)
endforeach()

# Exercise the optional height symbols independently of a working V1 wire ABI.
# Compile the same portable bridge once, hiding one or both symbols at link time.
add_library(cui_pagination_custom_provider_objects OBJECT
 "${CUI_BRIDGE_ROOT}/src/bridge/cui_bridge.c")
set_target_properties(cui_pagination_custom_provider_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(cui_pagination_custom_provider_objects PRIVATE CUI_BRIDGE_BUILD=1)
target_include_directories(cui_pagination_custom_provider_objects PRIVATE
 "${CUI_BRIDGE_ROOT}/include" "${CUI_BRIDGE_ROOT}/src/bridge")
target_link_libraries(cui_pagination_custom_provider_objects PRIVATE cui)
cui_warnings(cui_pagination_custom_provider_objects)

foreach(mode named_only custom_set_only)
 set(provider cui_pagination_${mode})
 set(directory "${CMAKE_CURRENT_BINARY_DIR}/pagination-providers/${mode}")
 set(map "${directory}/exports.map")
 set(hidden "cui_bridge_pagination_get_custom_size;")
 if(mode STREQUAL "named_only")
  string(APPEND hidden " cui_bridge_pagination_set_custom_size;")
 endif()
 file(GENERATE OUTPUT "${map}" CONTENT "{ local: ${hidden} };\n")
 add_library(${provider} SHARED)
 target_link_libraries(${provider} PRIVATE cui_pagination_custom_provider_objects cui)
 target_link_options(${provider} PRIVATE "LINKER:--version-script,${map}")
 set_target_properties(${provider} PROPERTIES
  OUTPUT_NAME cui_bridge LIBRARY_OUTPUT_DIRECTORY "${directory}" LINK_DEPENDS "${map}")

 cui_go_pagination_test(${mode} $<TARGET_FILE:${provider}> 0
  test -tags cui_dynamic ./cui -count=1 -run "^TestPagination")
 cui_rust_pagination_test(${mode} "${_cui_rust_pagination_dynamic}" 0
  ${_cui_rust_pagination_current} LD_LIBRARY_PATH=${directory}
  ${_cui_rust_pagination_test} -- --skip custom_size_geometry_and_activation --test-threads=1)
 set_tests_properties(bridge_go_pagination_${mode} bridge_rust_pagination_${mode} PROPERTIES
  ENVIRONMENT "CUI_PAGINATION_CUSTOM_UNSUPPORTED=1")

 add_test(NAME bridge_python_pagination_${mode} COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python
  CUI_BRIDGE_LIBRARY=$<TARGET_FILE:${provider}> CUI_PAGINATION_CUSTOM_UNSUPPORTED=1
  ${Python3_EXECUTABLE} "${CUI_BRIDGE_ROOT}/bindings/python/test_pagination_options.py")
endforeach()

if(CUI_PAGINATION_LEGACY_LIBRARY)
 if(NOT EXISTS "${CUI_PAGINATION_LEGACY_LIBRARY}" OR IS_DIRECTORY "${CUI_PAGINATION_LEGACY_LIBRARY}")
  message(FATAL_ERROR "CUI_PAGINATION_LEGACY_LIBRARY must name an existing provider")
 endif()
 set(provider_dir "${CMAKE_CURRENT_BINARY_DIR}/pagination-legacy")
 file(MAKE_DIRECTORY "${provider_dir}")
 file(CREATE_LINK "${CUI_PAGINATION_LEGACY_LIBRARY}" "${provider_dir}/libcui_bridge.so" SYMBOLIC)
 cui_pagination_provider_fixture(cui_pagination_partial_provider
  "${provider_dir}/libcui_bridge.so" CUI_TEST_PAGINATION_PARTIAL)
 set_target_properties(cui_pagination_partial_provider PROPERTIES BUILD_RPATH "${provider_dir}")

 cui_go_pagination_test(legacy "${CUI_PAGINATION_LEGACY_LIBRARY}" 1
  test -tags cui_dynamic ./cui -count=1 -run "^TestPagination(OptionalProvider|CustomSizeOptionalProvider)$")
 cui_go_pagination_test(partial $<TARGET_FILE:cui_pagination_partial_provider> 1
  test -tags cui_dynamic ./cui -count=1 -run "^TestPaginationOptionalProvider$")
 cui_rust_pagination_test(legacy "${_cui_rust_pagination_dynamic}" 1
  ${_cui_rust_pagination_current} LD_LIBRARY_PATH=${provider_dir}
  ${_cui_rust_pagination_test} -- optional_provider --test-threads=1)
 set_tests_properties(bridge_go_pagination_legacy bridge_rust_pagination_legacy PROPERTIES
  ENVIRONMENT "CUI_PAGINATION_CUSTOM_UNSUPPORTED=1")
 cui_rust_pagination_test(partial "${_cui_rust_pagination_dynamic}" 1
  ${_cui_rust_pagination_current} LD_LIBRARY_PATH=${provider_dir}
  LD_PRELOAD=$<TARGET_FILE:cui_pagination_partial_provider>
  ${_cui_rust_pagination_test} -- --exact optional_provider_preserves_legacy)
endif()
