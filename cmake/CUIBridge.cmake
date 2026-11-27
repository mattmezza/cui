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
add_executable(test_bridge_kbd ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_kbd.c)
target_link_libraries(test_bridge_kbd PRIVATE cui)
cui_warnings(test_bridge_kbd)
add_test(NAME bridge_kbd COMMAND test_bridge_kbd)
add_executable(test_bridge_skeleton ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_skeleton.c)
target_link_libraries(test_bridge_skeleton PRIVATE cui_software)
cui_warnings(test_bridge_skeleton)
add_test(NAME bridge_skeleton COMMAND test_bridge_skeleton)
add_executable(test_bridge_spinner ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_spinner.c)
target_link_libraries(test_bridge_spinner PRIVATE cui_software)
cui_warnings(test_bridge_spinner)
add_test(NAME bridge_spinner COMMAND test_bridge_spinner)
add_executable(bridge_spinner_status_example
 ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/spinner_status.c)
target_link_libraries(bridge_spinner_status_example PRIVATE cui_bridge)
cui_warnings(bridge_spinner_status_example)
add_test(NAME bridge_spinner_status_example COMMAND bridge_spinner_status_example)
add_executable(test_bridge_pagination ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_pagination.c)
target_link_libraries(test_bridge_pagination PRIVATE cui)
cui_warnings(test_bridge_pagination)
add_test(NAME bridge_pagination COMMAND test_bridge_pagination)
add_executable(test_bridge_disclosure ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_disclosure.c)
target_link_libraries(test_bridge_disclosure PRIVATE cui_software)
cui_warnings(test_bridge_disclosure)
add_test(NAME bridge_disclosure COMMAND test_bridge_disclosure)
add_executable(test_bridge_attachment ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_attachment.c)
target_link_libraries(test_bridge_attachment PRIVATE cui_software)
cui_warnings(test_bridge_attachment)
add_test(NAME bridge_attachment COMMAND test_bridge_attachment)
add_executable(test_bridge_attachment_actions ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_attachment_actions.c)
target_link_libraries(test_bridge_attachment_actions PRIVATE cui)
cui_warnings(test_bridge_attachment_actions)
add_test(NAME bridge_attachment_actions COMMAND test_bridge_attachment_actions)
add_executable(test_bridge_attachment_slots ${CUI_BRIDGE_ROOT}/tests/unit/test_bridge_attachment_slots.c)
target_link_libraries(test_bridge_attachment_slots PRIVATE cui)
cui_warnings(test_bridge_attachment_slots)
add_test(NAME bridge_attachment_slots COMMAND test_bridge_attachment_slots)
add_executable(bridge_attachment_slots_example
 ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/attachment_slots.c)
target_link_libraries(bridge_attachment_slots_example PRIVATE cui_bridge)
cui_warnings(bridge_attachment_slots_example)
add_test(NAME bridge_attachment_slots_example COMMAND bridge_attachment_slots_example)
add_executable(bridge_disclosure_preferences_example
 ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/disclosure_preferences.c)
target_link_libraries(bridge_disclosure_preferences_example PRIVATE cui_bridge)
cui_warnings(bridge_disclosure_preferences_example)
add_test(NAME bridge_disclosure_preferences_example COMMAND bridge_disclosure_preferences_example)
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
add_executable(bridge_virtual_inbox_example ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/virtual_inbox.c)
target_link_libraries(bridge_virtual_inbox_example PRIVATE cui_bridge)
cui_warnings(bridge_virtual_inbox_example)
add_test(NAME bridge_virtual_inbox_example COMMAND bridge_virtual_inbox_example)
add_executable(test_bridge_data_table_capacity
 ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_data_table_capacity.c)
target_link_libraries(test_bridge_data_table_capacity PRIVATE cui_bridge)
cui_warnings(test_bridge_data_table_capacity)
add_test(NAME bridge_data_table_capacity COMMAND test_bridge_data_table_capacity)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
 add_executable(test_bridge_data_table_capacity_allocations
  ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_data_table_capacity.c)
 target_compile_definitions(test_bridge_data_table_capacity_allocations
  PRIVATE CUI_TEST_DATA_TABLE_ALLOCATIONS=1)
 target_link_libraries(test_bridge_data_table_capacity_allocations PRIVATE cui)
 target_link_options(test_bridge_data_table_capacity_allocations PRIVATE
  LINKER:--wrap=malloc LINKER:--wrap=calloc LINKER:--wrap=free)
 cui_warnings(test_bridge_data_table_capacity_allocations)
 add_test(NAME bridge_data_table_capacity_allocations
  COMMAND test_bridge_data_table_capacity_allocations)
endif()
add_executable(bridge_data_table_paging_example ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/data_table_paging.c)
target_link_libraries(bridge_data_table_paging_example PRIVATE cui_bridge)
cui_warnings(bridge_data_table_paging_example)
add_test(NAME bridge_data_table_paging_example COMMAND bridge_data_table_paging_example)
add_test(NAME bridge_data_table_capacity_example COMMAND bridge_data_table_paging_example --larger)
add_test(NAME bridge_python
 COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python
  CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_bridge.py)
add_test(NAME bridge_python_pagination COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_pagination_options.py)
add_test(NAME bridge_python_pagination_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/examples/pagination.py)
add_test(NAME bridge_python_rating COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 CUI_RATING_UNSUPPORTED=0 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_rating_options.py)
add_test(NAME bridge_python_rating_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/examples/rating_options.py)
add_test(NAME bridge_python_virtual_list_rows COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_virtual_list_rows.py)
add_test(NAME bridge_python_virtual_list_inbox COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/examples/virtual_list_inbox.py)
set(CUI_PYTHON_SKELETON_LEGACY_LIBRARY "" CACHE FILEPATH "Optional published pre-presentation bridge")
set(CUI_PYTHON_SKELETON_PARTIAL_LIBRARY "" CACHE FILEPATH "Optional bridge with incomplete Skeleton symbols")
add_test(NAME bridge_python_skeleton COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 CUI_SKELETON_LEGACY_LIBRARY=${CUI_PYTHON_SKELETON_LEGACY_LIBRARY}
 CUI_SKELETON_PARTIAL_LIBRARY=${CUI_PYTHON_SKELETON_PARTIAL_LIBRARY}
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_skeleton_presentation.py)
add_test(NAME bridge_python_skeleton_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/skeleton_profile.py)
set(CUI_PYTHON_ICON_BADGE_LEGACY_LIBRARY "" CACHE FILEPATH "Optional published alpha.4 bridge")
set(CUI_PYTHON_ICON_BADGE_PARTIAL_LIBRARY "" CACHE FILEPATH "Optional bridge with incomplete Badge symbols")
add_test(NAME bridge_python_icons COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 CUI_ICON_BADGE_LEGACY_LIBRARY=${CUI_PYTHON_ICON_BADGE_LEGACY_LIBRARY}
 CUI_ICON_BADGE_PARTIAL_LIBRARY=${CUI_PYTHON_ICON_BADGE_PARTIAL_LIBRARY}
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_icons.py)
add_test(NAME bridge_python_icon_badge_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/icon_badge_inbox.py)
option(CUI_BRIDGE_NATIVE_TESTS "Register opt-in Linux invisible native binding checks" OFF)
option(CUI_GO_ICON_TESTS "Check owned Icon resources and retained Icon/Badge with Go on Linux" OFF)
if(CUI_GO_ICON_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Go Icon and Badge provider tests currently support Linux only")
 endif()
 find_program(CUI_GO_ICON_EXECUTABLE go REQUIRED)
 function(cui_go_icon_test suffix library unsupported presentation)
  add_test(NAME bridge_go_icon_${suffix} COMMAND ${CMAKE_COMMAND} -E env
   GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
   CUI_BRIDGE_LIBRARY=${library} CUI_TEST_ICON_RESOURCE_UNSUPPORTED=${unsupported}
   CUI_TEST_ICON_BADGE_PROVIDER=${presentation}
   ${CUI_GO_ICON_EXECUTABLE} ${ARGN})
  set_tests_properties(bridge_go_icon_${suffix} PROPERTIES
   WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
 endfunction()
 cui_go_icon_test(dynamic $<TARGET_FILE:cui_bridge> 0 current
  test -tags cui_dynamic ./cui -count=1 -run "^Test(Icon(Resource|AndMessage)|Configured(Icon|Badge))")
 cui_go_icon_test(static "" 0 current
  test ./cui -count=1 -run "^Test(Icon(Resource|AndMessage)|Configured(Icon|Badge))")
 cui_go_icon_test(example "" 0 current run ./examples/icon_resource)
 cui_go_icon_test(inbox "" 0 current run ./examples/icon_badge_inbox)
 set(CUI_GO_ICON_LEGACY_LIBRARY "" CACHE FILEPATH "Optional pre-resource Go bridge provider")
 if(CUI_GO_ICON_LEGACY_LIBRARY)
  cui_go_icon_test(legacy "${CUI_GO_ICON_LEGACY_LIBRARY}" 1 legacy
   test -tags cui_dynamic ./cui -count=1 -run "^Test(Icon(ResourceOptionalProvider|AndMessage)|ConfiguredIconBadgeProvider)$")
 endif()
 set(CUI_GO_ICON_ALPHA4_LIBRARY "" CACHE FILEPATH "Optional published alpha.4 Go bridge provider")
 if(CUI_GO_ICON_ALPHA4_LIBRARY)
  cui_go_icon_test(alpha4 "${CUI_GO_ICON_ALPHA4_LIBRARY}" 0 alpha4
   test -tags cui_dynamic ./cui -count=1 -run "^Test(Icon(Resource|AndMessage)|ConfiguredIconBadgeProvider)")
 endif()
 set(CUI_GO_ICON_PARTIAL_BADGE_LIBRARY "" CACHE FILEPATH "Optional Go bridge provider with an incomplete Badge group")
 if(CUI_GO_ICON_PARTIAL_BADGE_LIBRARY)
  cui_go_icon_test(partial_badge "${CUI_GO_ICON_PARTIAL_BADGE_LIBRARY}" 0 partial-badge
   test -tags cui_dynamic ./cui -count=1 -run "^TestConfiguredIconBadgeProvider$")
 endif()
 if(CUI_BRIDGE_NATIVE_TESTS)
  cui_go_icon_test(native $<TARGET_FILE:cui_bridge> 0 current
   run -tags cui_dynamic ./examples/icon_badge_inbox --font ${CUI_TEST_FONT} --smoke)
 endif()
endif()
option(CUI_GO_SKELETON_TESTS "Check responsive Skeleton authoring with Go on Linux" OFF)
if(CUI_GO_SKELETON_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Go Skeleton provider tests currently support Linux only")
 endif()
 find_program(CUI_GO_SKELETON_EXECUTABLE go REQUIRED)
 set(_cui_go_env GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local)
 function(cui_go_skeleton_test suffix library unsupported)
  add_test(NAME bridge_go_skeleton_${suffix} COMMAND ${CMAKE_COMMAND} -E env
   ${_cui_go_env} CUI_BRIDGE_LIBRARY=${library} CUI_TEST_UNSUPPORTED_SKELETON=${unsupported}
   ${CUI_GO_SKELETON_EXECUTABLE} ${ARGN})
  set_tests_properties(bridge_go_skeleton_${suffix} PROPERTIES
   WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
 endfunction()
 cui_go_skeleton_test(dynamic $<TARGET_FILE:cui_bridge> 0
  test -tags cui_dynamic ./cui -count=1 -run "^TestResponsiveSkeleton")
 cui_go_skeleton_test(static "" 0 test ./cui -count=1 -run "^TestResponsiveSkeleton")
 cui_go_skeleton_test(example "" 0 run ./examples/skeleton_profile)
 foreach(provider LEGACY PARTIAL)
  set(CUI_GO_SKELETON_${provider}_LIBRARY "" CACHE FILEPATH "Optional Go Skeleton ${provider} provider")
  if(CUI_GO_SKELETON_${provider}_LIBRARY)
   string(TOLOWER ${provider} suffix)
   cui_go_skeleton_test(${suffix} "${CUI_GO_SKELETON_${provider}_LIBRARY}" 1
    test -tags cui_dynamic ./cui -count=1 -run "^TestResponsiveSkeletonOptionalProvider$")
  endif()
 endforeach()
 if(CUI_BRIDGE_NATIVE_TESTS)
  cui_go_skeleton_test(native $<TARGET_FILE:cui_bridge> 0
   run -tags cui_dynamic ./examples/skeleton_profile --font ${CUI_TEST_FONT} --smoke)
 endif()
endif()
option(CUI_GO_KEYED_DATA_TABLE_TESTS "Check keyed DataTable authoring and paging with Go on Linux" OFF)
if(CUI_GO_KEYED_DATA_TABLE_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Go keyed DataTable provider tests currently support Linux only")
 endif()
 find_program(CUI_GO_KEYED_DATA_TABLE_EXECUTABLE go REQUIRED)
 set(CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY "" CACHE FILEPATH "Optional existing bridge without keyed tables")
 function(cui_go_data_table_test name library legacy)
  add_test(NAME bridge_go_${name} COMMAND ${CMAKE_COMMAND} -E env
   GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
   CUI_BRIDGE_LIBRARY=${library} CUI_KEYED_DATA_TABLE_LEGACY=${legacy} SDL_RENDER_DRIVER=software
   ${CUI_GO_KEYED_DATA_TABLE_EXECUTABLE} ${ARGN})
  set_tests_properties(bridge_go_${name} PROPERTIES
   WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
 endfunction()
 cui_go_data_table_test(keyed_data_table_dynamic $<TARGET_FILE:cui_bridge> 0
  test -tags cui_dynamic ./cui -count=1 -run "^Test(KeyedDataTable|DataTable)$")
 cui_go_data_table_test(keyed_data_table_static "" 0
  test ./cui -count=1 -run "^Test(KeyedDataTable|DataTable)$")
 cui_go_data_table_test(keyed_data_table_example "" 0 run ./examples/data_table_live)
 cui_go_data_table_test(data_table_paging_dynamic $<TARGET_FILE:cui_bridge> 0
  test -tags cui_dynamic ./cui -count=1 -run "^TestDataTablePaging(EmptyAndOwnerThread)?$")
 cui_go_data_table_test(data_table_paging_static "" 0
  test ./cui -count=1 -run "^TestDataTablePaging(EmptyAndOwnerThread)?$")
 cui_go_data_table_test(data_table_paging_example "" 0 run ./examples/data_table_paging)
 cui_go_data_table_test(data_table_capacity_dynamic $<TARGET_FILE:cui_bridge> 0
  test -tags cui_dynamic ./cui -count=1 -run "^TestDataTableCapacity(Limits)?$")
 cui_go_data_table_test(data_table_capacity_static "" 0
  test ./cui -count=1 -run "^TestDataTableCapacity(Limits)?$")
 cui_go_data_table_test(data_table_capacity_example "" 0
  run ./examples/data_table_paging --larger)

 function(cui_go_capacity_fixture target provider)
  add_library(${target} SHARED ${CUI_BRIDGE_ROOT}/bindings/tests/data_table_capacity_provider.c)
  target_include_directories(${target} PRIVATE ${CUI_BRIDGE_ROOT}/include)
  target_link_libraries(${target} PRIVATE "-Wl,--no-as-needed" ${provider} "-Wl,--as-needed")
  if(ARGN)
   target_compile_definitions(${target} PRIVATE ${ARGN})
  endif()
  cui_warnings(${target})
 endfunction()
 cui_go_capacity_fixture(cui_go_capacity_query_fixture cui_bridge)
 cui_go_capacity_fixture(cui_go_capacity_rejected_fixture cui_bridge CUI_TEST_CAPACITY_SETTER_ONLY)
 foreach(mode limited zero large rejected)
  if(mode STREQUAL "rejected")
   set(_go_capacity_fixture cui_go_capacity_rejected_fixture)
  else()
   set(_go_capacity_fixture cui_go_capacity_query_fixture)
  endif()
  cui_go_data_table_test(data_table_capacity_${mode} $<TARGET_FILE:${_go_capacity_fixture}> 0
   test -tags cui_dynamic ./cui -count=1 -run "^TestDataTableCapacityProvider$")
  set_property(TEST bridge_go_data_table_capacity_${mode} PROPERTY
   ENVIRONMENT "CUI_DATA_TABLE_CAPACITY_FIXTURE=${mode}")
 endforeach()

 add_library(cui_go_paging_fixture SHARED ${CUI_BRIDGE_ROOT}/bindings/tests/data_table_paging_provider.c)
 target_include_directories(cui_go_paging_fixture PRIVATE ${CUI_BRIDGE_ROOT}/include)
 target_link_libraries(cui_go_paging_fixture PRIVATE "-Wl,--no-as-needed" cui_bridge "-Wl,--as-needed")
 cui_warnings(cui_go_paging_fixture)
 foreach(mode exact frame reserved window visible capacity different_capacity explicit_capacity key size)
  cui_go_data_table_test(data_table_paging_${mode} $<TARGET_FILE:cui_go_paging_fixture> 0
   test -tags cui_dynamic ./cui -count=1 -run "^TestDataTablePaging(Fallback|Protocol)$")
  set_property(TEST bridge_go_data_table_paging_${mode} PROPERTY
   ENVIRONMENT "CUI_DATA_TABLE_PAGING_FIXTURE=${mode}")
 endforeach()
 if(CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY)
  if(NOT EXISTS "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}")
   message(FATAL_ERROR "CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY must name an existing bridge")
  endif()
  cui_go_data_table_test(keyed_data_table_legacy "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}" 1
   test -tags cui_dynamic ./cui -count=1 -run "^Test(KeyedDataTableLegacyProvider|DataTable)$")
  cui_go_data_table_test(keyed_data_table_legacy_bundled "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}" 1
   test ./cui -count=1 -run "^Test(KeyedDataTableLegacyProvider|DataTable)$")
  cui_go_data_table_test(data_table_paging_legacy "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}" 1
   test -tags cui_dynamic ./cui -count=1 -run "^TestDataTablePagingFallback$")
  cui_go_data_table_test(data_table_paging_legacy_bundled "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}" 1
   test ./cui -count=1 -run "^TestDataTablePagingFallback$")
  set_tests_properties(bridge_go_data_table_paging_legacy bridge_go_data_table_paging_legacy_bundled
   PROPERTIES ENVIRONMENT "CUI_DATA_TABLE_PAGING_FIXTURE=legacy")
  set(_go_paging_provider_dir "${CMAKE_CURRENT_BINARY_DIR}/go-data-table-paging-legacy")
  file(MAKE_DIRECTORY "${_go_paging_provider_dir}")
  file(CREATE_LINK "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}"
   "${_go_paging_provider_dir}/libcui_bridge.so" SYMBOLIC)
  add_library(cui_go_partial_paging_fixture SHARED ${CUI_BRIDGE_ROOT}/bindings/tests/data_table_paging_provider.c)
  target_include_directories(cui_go_partial_paging_fixture PRIVATE ${CUI_BRIDGE_ROOT}/include)
  target_compile_definitions(cui_go_partial_paging_fixture PRIVATE CUI_TEST_PARTIAL_PAGING_PROVIDER)
  target_link_libraries(cui_go_partial_paging_fixture PRIVATE "-Wl,--no-as-needed"
   "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}" "-Wl,--as-needed")
  set_target_properties(cui_go_partial_paging_fixture PROPERTIES BUILD_RPATH "${_go_paging_provider_dir}")
  cui_warnings(cui_go_partial_paging_fixture)
  cui_go_data_table_test(data_table_paging_partial $<TARGET_FILE:cui_go_partial_paging_fixture> 1
   test -tags cui_dynamic ./cui -count=1 -run "^TestDataTablePagingFallback$")
  set_property(TEST bridge_go_data_table_paging_partial PROPERTY
   ENVIRONMENT "CUI_DATA_TABLE_PAGING_FIXTURE=partial")
  cui_go_data_table_test(data_table_capacity_legacy "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}" 1
   test -tags cui_dynamic ./cui -count=1 -run "^TestDataTableCapacityProvider$")
  set_property(TEST bridge_go_data_table_capacity_legacy PROPERTY
   ENVIRONMENT "CUI_DATA_TABLE_CAPACITY_FIXTURE=legacy")
  cui_go_capacity_fixture(cui_go_capacity_query_legacy_fixture "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}")
  cui_go_capacity_fixture(cui_go_capacity_setter_legacy_fixture "${CUI_GO_KEYED_DATA_TABLE_LEGACY_LIBRARY}"
   CUI_TEST_CAPACITY_SETTER_ONLY)
  foreach(mode query setter)
   set_target_properties(cui_go_capacity_${mode}_legacy_fixture PROPERTIES BUILD_RPATH "${_go_paging_provider_dir}")
   cui_go_data_table_test(data_table_capacity_${mode}_only $<TARGET_FILE:cui_go_capacity_${mode}_legacy_fixture> 1
    test -tags cui_dynamic ./cui -count=1 -run "^TestDataTableCapacityProvider$")
   set_property(TEST bridge_go_data_table_capacity_${mode}_only PROPERTY
    ENVIRONMENT "CUI_DATA_TABLE_CAPACITY_FIXTURE=${mode}-only")
  endforeach()
 endif()
 if(CUI_BRIDGE_NATIVE_TESTS)
  cui_go_data_table_test(keyed_data_table_native $<TARGET_FILE:cui_bridge> 0
   run -tags cui_dynamic ./examples/data_table_live --font ${CUI_TEST_FONT} --smoke)
  cui_go_data_table_test(data_table_paging_native $<TARGET_FILE:cui_bridge> 0
   run -tags cui_dynamic ./examples/data_table_paging --font ${CUI_TEST_FONT} --smoke)
  cui_go_data_table_test(data_table_paging_native_bundled "" 0
   run -tags cui_native ./examples/data_table_paging --font ${CUI_TEST_FONT} --smoke)
  cui_go_data_table_test(data_table_capacity_native $<TARGET_FILE:cui_bridge> 0
   run -tags cui_dynamic ./examples/data_table_paging --font ${CUI_TEST_FONT} --smoke --larger)
  cui_go_data_table_test(data_table_capacity_native_bundled "" 0
   run -tags cui_native ./examples/data_table_paging --font ${CUI_TEST_FONT} --smoke --larger)
  cui_go_data_table_test(data_table_paging_run $<TARGET_FILE:cui_bridge> 0
   test -tags cui_dynamic ./cui -count=1 -run "^TestNativeRun(Lifecycle|AfterFrame)$")
  set_property(TEST bridge_go_data_table_paging_run PROPERTY ENVIRONMENT "CUI_TEST_FONT=${CUI_TEST_FONT}")
 endif()
endif()

option(CUI_RUST_SKELETON_TESTS "Check responsive Skeleton authoring with Rust on Linux" OFF)
set(CUI_RUST_SKELETON_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-skeleton-target" CACHE PATH "Reusable Cargo cache for dynamic Skeleton checks")
set(CUI_RUST_SKELETON_STATIC_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-skeleton-static-target" CACHE PATH "Reusable Cargo cache for bundled Skeleton checks")
include("${CMAKE_CURRENT_LIST_DIR}/CUIPaginationBindingTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CUIRatingBindingTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CUIDisclosureBindingTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CUIAttachmentBindingTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CUIVirtualListBindingTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CUISpinnerBindingTests.cmake")
if(CUI_RUST_SKELETON_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Rust Skeleton provider tests currently support Linux only")
 endif()
 find_program(CUI_RUST_SKELETON_CARGO cargo REQUIRED)
 function(cui_rust_skeleton_test suffix target library_dir unsupported)
  add_test(NAME bridge_rust_skeleton_${suffix} COMMAND ${CMAKE_COMMAND} -E env
   --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH
   CARGO_TARGET_DIR=${target} RUSTFLAGS=-Dwarnings
   CUI_TEST_UNSUPPORTED_SKELETON=${unsupported} ${library_dir}
   ${CUI_RUST_SKELETON_CARGO} ${ARGN})
  set_tests_properties(bridge_rust_skeleton_${suffix} PROPERTIES
   WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
 endfunction()
 set(_cui_rust_current CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>)
 cui_rust_skeleton_test(dynamic "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_current}" 0
  test --offline --locked -j1 --test skeleton_presentation -- --test-threads=1)
 cui_rust_skeleton_test(static "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  test --offline --locked -j1 --test skeleton_presentation -- --test-threads=1)
 cui_rust_skeleton_test(example "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  run --offline --locked -j1 --example skeleton_profile)
 foreach(provider LEGACY PARTIAL)
  set(CUI_RUST_SKELETON_${provider}_LIBRARY "" CACHE FILEPATH "Optional Rust Skeleton ${provider} provider")
  if(CUI_RUST_SKELETON_${provider}_LIBRARY)
   set(lib "${CUI_RUST_SKELETON_${provider}_LIBRARY}")
   if(NOT EXISTS "${lib}" OR IS_DIRECTORY "${lib}")
    message(FATAL_ERROR "CUI_RUST_SKELETON_${provider}_LIBRARY must name an existing provider")
   endif()
   string(TOLOWER ${provider} suffix)
   set(provider_dir "${CMAKE_CURRENT_BINARY_DIR}/rust-skeleton-providers/${suffix}")
   file(MAKE_DIRECTORY "${provider_dir}")
   file(CREATE_LINK "${lib}" "${provider_dir}/libcui_bridge.so" SYMBOLIC)
   cui_rust_skeleton_test(${suffix} "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_current};LD_LIBRARY_PATH=${provider_dir}" 1
    test --offline --locked -j1 --test skeleton_presentation -- --exact optional_provider_preserves_legacy)
  endif()
 endforeach()
 if(CUI_BRIDGE_NATIVE_TESTS)
  cui_rust_skeleton_test(native "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_current}" 0
   run --offline --locked -j1 --example skeleton_profile -- --font ${CUI_TEST_FONT} --smoke)
 endif()
endif()
option(CUI_RUST_ICON_TESTS "Check owned Icon resources and retained Icon/Badge with Rust on Linux" OFF)
if(CUI_RUST_ICON_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Rust Icon provider tests currently support Linux only")
 endif()
 find_program(CUI_RUST_ICON_CARGO cargo REQUIRED)
 set(CUI_RUST_ICON_TARGET_DIR "${CUI_RUST_SKELETON_TARGET_DIR}" CACHE PATH "Reusable Cargo cache for shared Rust Icon tests")
 set(CUI_RUST_ICON_STATIC_TARGET_DIR "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" CACHE PATH "Reusable Cargo cache for bundled Rust Icon tests")
 function(cui_rust_icon_test suffix target library_dir unsupported)
  string(REGEX REPLACE "^badge_" "" provider "${suffix}")
  add_test(NAME bridge_rust_icon_${suffix} COMMAND ${CMAKE_COMMAND} -E env
   --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH
   CARGO_TARGET_DIR=${target} RUSTFLAGS=-Dwarnings
   CUI_TEST_ICON_RESOURCE_UNSUPPORTED=${unsupported}
   CUI_TEST_ICON_BADGE_PROVIDER=${provider} ${library_dir}
   ${CUI_RUST_ICON_CARGO} ${ARGN})
  set_tests_properties(bridge_rust_icon_${suffix} PROPERTIES
   WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
 endfunction()
 set(_cui_rust_icon_shared CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>)
 cui_rust_icon_test(dynamic "${CUI_RUST_ICON_TARGET_DIR}" "${_cui_rust_icon_shared}" 0
  test --offline --locked -j1 --test icon_resources -- --test-threads=1)
 cui_rust_icon_test(static "${CUI_RUST_ICON_STATIC_TARGET_DIR}" "" 0
  test --offline --locked -j1 --test icon_resources -- --test-threads=1)
 cui_rust_icon_test(example "${CUI_RUST_ICON_STATIC_TARGET_DIR}" "" 0
  run --offline --locked -j1 --example icon_resource)
 cui_rust_icon_test(badge_dynamic "${CUI_RUST_ICON_TARGET_DIR}" "${_cui_rust_icon_shared}" 0
  test --offline --locked -j1 --test icon_badge -- --test-threads=1)
 cui_rust_icon_test(badge_static "${CUI_RUST_ICON_STATIC_TARGET_DIR}" "" 0
  test --offline --locked -j1 --test icon_badge -- --test-threads=1)
 cui_rust_icon_test(badge_inbox "${CUI_RUST_ICON_STATIC_TARGET_DIR}" "" 0
  run --offline --locked -j1 --example icon_badge_inbox)
 if(CUI_BRIDGE_NATIVE_TESTS)
  cui_rust_icon_test(badge_native "${CUI_RUST_ICON_TARGET_DIR}" "${_cui_rust_icon_shared}" 0
   run --offline --locked -j1 --example icon_badge_inbox -- --font ${CUI_TEST_FONT} --smoke)
 endif()
 foreach(provider LEGACY ALPHA4 PARTIAL_BADGE)
  set(CUI_RUST_ICON_${provider}_LIBRARY "" CACHE FILEPATH "Optional Rust Icon ${provider} provider")
  if(CUI_RUST_ICON_${provider}_LIBRARY)
   set(lib "${CUI_RUST_ICON_${provider}_LIBRARY}")
   if(NOT EXISTS "${lib}" OR IS_DIRECTORY "${lib}")
    message(FATAL_ERROR "CUI_RUST_ICON_${provider}_LIBRARY must name an existing provider")
   endif()
   string(TOLOWER ${provider} suffix)
   set(provider_dir "${CMAKE_CURRENT_BINARY_DIR}/rust-icon-providers/${suffix}")
   file(MAKE_DIRECTORY "${provider_dir}")
   file(CREATE_LINK "${lib}" "${provider_dir}/libcui_bridge.so" SYMBOLIC)
   set(selection)
   set(unsupported 0)
   if(provider STREQUAL "LEGACY")
    set(selection --exact optional_provider_preserves_legacy)
    set(unsupported 1)
   endif()
   set(library "${_cui_rust_icon_shared};LD_LIBRARY_PATH=${provider_dir}")
   if(NOT provider STREQUAL "PARTIAL_BADGE")
    cui_rust_icon_test(${suffix} "${CUI_RUST_ICON_TARGET_DIR}" "${library}" ${unsupported}
     test --offline --locked -j1 --test icon_resources -- --test-threads=1 ${selection})
   endif()
   cui_rust_icon_test(badge_${suffix} "${CUI_RUST_ICON_TARGET_DIR}" "${library}" ${unsupported}
    test --offline --locked -j1 --test icon_badge -- --exact optional_provider_preserves_legacy)
  endif()
 endforeach()
endif()
option(CUI_RUST_KEYED_DATA_TABLE_TESTS "Check keyed DataTable authoring and paging with Rust on Linux" OFF)
if(CUI_RUST_KEYED_DATA_TABLE_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Rust keyed DataTable provider tests currently support Linux only")
 endif()
 find_program(CUI_RUST_KEYED_DATA_TABLE_CARGO cargo REQUIRED)
 set(CUI_RUST_KEYED_DATA_TABLE_LEGACY_LIBRARY "" CACHE FILEPATH "Optional existing bridge without keyed tables")
 function(cui_rust_data_table_test name target library legacy)
  add_test(NAME bridge_rust_${name} COMMAND ${CMAKE_COMMAND} -E env
   --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH --unset=LD_PRELOAD
   CARGO_TARGET_DIR=${target} RUSTFLAGS=-Dwarnings SDL_RENDER_DRIVER=software
   CUI_RUST_KEYED_DATA_TABLE_LEGACY=${legacy} ${library}
   ${CUI_RUST_KEYED_DATA_TABLE_CARGO} ${ARGN})
  set_tests_properties(bridge_rust_${name} PROPERTIES
   WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
 endfunction()
 set(_cui_rust_table_shared CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>)
 cui_rust_data_table_test(keyed_data_table_dynamic "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_table_shared}" 0
  test --offline --locked -j1 --test data_table_keyed)
 cui_rust_data_table_test(keyed_data_table_static "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  test --offline --locked -j1 --test data_table_keyed)
 cui_rust_data_table_test(keyed_data_table_example "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  run --offline --locked -j1 --example data_table_live)
 cui_rust_data_table_test(data_table_paging_dynamic "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_table_shared}" 0
  test --offline --locked -j1 --test data_table_paging)
 cui_rust_data_table_test(data_table_paging_static "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  test --offline --locked -j1 --test data_table_paging)
 cui_rust_data_table_test(data_table_paging_example "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  run --offline --locked -j1 --example data_table_paging)
 cui_rust_data_table_test(data_table_capacity_dynamic "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_table_shared}" 0
  test --offline --locked -j1 --test data_table_capacity -- --test-threads=1)
 cui_rust_data_table_test(data_table_capacity_static "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  test --offline --locked -j1 --test data_table_capacity -- --test-threads=1)
 cui_rust_data_table_test(data_table_capacity_removal "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  test --offline --locked -j1 --lib -- --exact data_table_capacity_tests::removes_capacity_records_for_entire_subtrees)
 cui_rust_data_table_test(data_table_capacity_example "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
  run --offline --locked -j1 --example data_table_paging -- --larger)
 add_library(cui_rust_capacity_query_fixture SHARED ${CUI_BRIDGE_ROOT}/bindings/tests/data_table_capacity_provider.c)
 add_library(cui_rust_capacity_setter_fixture SHARED ${CUI_BRIDGE_ROOT}/bindings/tests/data_table_capacity_provider.c)
 target_compile_definitions(cui_rust_capacity_setter_fixture PRIVATE CUI_TEST_CAPACITY_SETTER_ONLY)
 foreach(export query setter)
  target_include_directories(cui_rust_capacity_${export}_fixture PRIVATE ${CUI_BRIDGE_ROOT}/include)
  cui_warnings(cui_rust_capacity_${export}_fixture)
 endforeach()
 foreach(mode limited zero large rejected)
  set(fixture cui_rust_capacity_query_fixture)
  if(mode STREQUAL "rejected")
   set(fixture cui_rust_capacity_setter_fixture)
  endif()
  cui_rust_data_table_test(data_table_capacity_${mode} "${CUI_RUST_SKELETON_TARGET_DIR}"
   "${_cui_rust_table_shared};LD_PRELOAD=$<TARGET_FILE:${fixture}>;CUI_DATA_TABLE_CAPACITY_FIXTURE=${mode}" 0
   test --offline --locked -j1 --test data_table_capacity -- --exact optional_provider_preserves_ordinary_tables)
 endforeach()
 add_library(cui_rust_paging_fixture SHARED ${CUI_BRIDGE_ROOT}/bindings/tests/data_table_paging_provider.c)
 target_include_directories(cui_rust_paging_fixture PRIVATE ${CUI_BRIDGE_ROOT}/include)
 cui_warnings(cui_rust_paging_fixture)
 foreach(mode exact frame reserved window visible capacity different_capacity explicit_capacity key size)
  set(selection copied_record_is_validated)
  if(mode STREQUAL "size")
   set(selection incompatible_provider_leaves_positional_tables_usable)
  endif()
  cui_rust_data_table_test(data_table_paging_${mode} "${CUI_RUST_SKELETON_TARGET_DIR}"
   "${_cui_rust_table_shared};LD_PRELOAD=$<TARGET_FILE:cui_rust_paging_fixture>;CUI_DATA_TABLE_PAGING_FIXTURE=${mode}" 0
   test --offline --locked -j1 --test data_table_paging -- --exact ${selection})
 endforeach()
 if(CUI_RUST_KEYED_DATA_TABLE_LEGACY_LIBRARY)
  if(NOT EXISTS "${CUI_RUST_KEYED_DATA_TABLE_LEGACY_LIBRARY}" OR IS_DIRECTORY "${CUI_RUST_KEYED_DATA_TABLE_LEGACY_LIBRARY}")
   message(FATAL_ERROR "CUI_RUST_KEYED_DATA_TABLE_LEGACY_LIBRARY must name an existing bridge")
  endif()
  set(provider_dir "${CMAKE_CURRENT_BINARY_DIR}/rust-keyed-data-table-legacy")
  file(MAKE_DIRECTORY "${provider_dir}")
  file(CREATE_LINK "${CUI_RUST_KEYED_DATA_TABLE_LEGACY_LIBRARY}" "${provider_dir}/libcui_bridge.so" SYMBOLIC)
  cui_rust_data_table_test(keyed_data_table_legacy "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_table_shared};LD_LIBRARY_PATH=${provider_dir}" 1
   test --offline --locked -j1 --test data_table_keyed -- --exact legacy_provider_keeps_positional_tables_usable)
  set(_cui_rust_table_older "CUI_BRIDGE_LIB_DIR=${provider_dir};LD_LIBRARY_PATH=${provider_dir}")
  cui_rust_data_table_test(data_table_paging_legacy "${CUI_RUST_SKELETON_TARGET_DIR}"
   "${_cui_rust_table_older};CUI_DATA_TABLE_PAGING_FIXTURE=legacy" 1
   test --offline --locked -j1 --test data_table_paging -- --exact incompatible_provider_leaves_positional_tables_usable)
  add_library(cui_rust_partial_paging_fixture SHARED ${CUI_BRIDGE_ROOT}/bindings/tests/data_table_paging_provider.c)
  target_include_directories(cui_rust_partial_paging_fixture PRIVATE ${CUI_BRIDGE_ROOT}/include)
  target_compile_definitions(cui_rust_partial_paging_fixture PRIVATE CUI_TEST_PARTIAL_PAGING_PROVIDER)
  cui_warnings(cui_rust_partial_paging_fixture)
  cui_rust_data_table_test(data_table_paging_partial "${CUI_RUST_SKELETON_TARGET_DIR}"
   "${_cui_rust_table_older};LD_PRELOAD=$<TARGET_FILE:cui_rust_partial_paging_fixture>;CUI_DATA_TABLE_PAGING_FIXTURE=partial" 1
   test --offline --locked -j1 --test data_table_paging -- --exact incompatible_provider_leaves_positional_tables_usable)
  cui_rust_data_table_test(data_table_capacity_legacy "${CUI_RUST_SKELETON_TARGET_DIR}"
   "${_cui_rust_table_older};CUI_DATA_TABLE_CAPACITY_FIXTURE=legacy" 1
   test --offline --locked -j1 --test data_table_capacity -- --exact optional_provider_preserves_ordinary_tables)
  foreach(export query setter)
   cui_rust_data_table_test(data_table_capacity_${export}_only "${CUI_RUST_SKELETON_TARGET_DIR}"
    "${_cui_rust_table_older};LD_PRELOAD=$<TARGET_FILE:cui_rust_capacity_${export}_fixture>;CUI_DATA_TABLE_CAPACITY_FIXTURE=${export}-only" 1
    test --offline --locked -j1 --test data_table_capacity -- --exact optional_provider_preserves_ordinary_tables)
  endforeach()
 endif()
 if(CUI_BRIDGE_NATIVE_TESTS)
  cui_rust_data_table_test(keyed_data_table_native "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_table_shared}" 0
   run --offline --locked -j1 --example data_table_live -- --font ${CUI_TEST_FONT} --smoke)
  cui_rust_data_table_test(data_table_paging_native "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_table_shared}" 0
   run --offline --locked -j1 --example data_table_paging -- --font ${CUI_TEST_FONT} --smoke)
  cui_rust_data_table_test(data_table_paging_native_bundled "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
   run --offline --locked -j1 --features native --example data_table_paging -- --font ${CUI_TEST_FONT} --smoke)
  cui_rust_data_table_test(data_table_capacity_native "${CUI_RUST_SKELETON_TARGET_DIR}" "${_cui_rust_table_shared}" 0
   run --offline --locked -j1 --example data_table_paging -- --font ${CUI_TEST_FONT} --smoke --larger)
  cui_rust_data_table_test(data_table_capacity_native_bundled "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" "" 0
   run --offline --locked -j1 --features native --example data_table_paging -- --font ${CUI_TEST_FONT} --smoke --larger)
 endif()
endif()
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
 add_test(NAME bridge_python_skeleton_native COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/skeleton_profile.py
  --font ${CUI_TEST_FONT} --smoke)
 add_test(NAME bridge_python_keyed_data_table_native COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  SDL_RENDER_DRIVER=software
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/data_table_live.py
  --font ${CUI_TEST_FONT} --smoke)
 add_test(NAME bridge_python_data_table_paging_native COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  SDL_RENDER_DRIVER=software
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/data_table_paging.py
  --font ${CUI_TEST_FONT} --smoke)
 set_tests_properties(bridge_python_data_table_paging_native PROPERTIES TIMEOUT 30)
 add_test(NAME bridge_python_data_table_capacity_native COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  SDL_RENDER_DRIVER=software
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/data_table_paging.py
  --font ${CUI_TEST_FONT} --smoke --larger)
 set_tests_properties(bridge_python_data_table_capacity_native PROPERTIES TIMEOUT 30)
 add_test(NAME bridge_python_icon_badge_native COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
  SDL_RENDER_DRIVER=software
  ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/icon_badge_inbox.py
  --font ${CUI_TEST_FONT} --smoke)
endif()

add_test(NAME bridge_python_components COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_components.py)
add_test(NAME bridge_python_radio_delivery_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/examples/radio_delivery.py)

option(CUI_GO_RADIO_MOTION_TESTS "Check retained Radio presentation with Go on Linux" OFF)
if(CUI_GO_RADIO_MOTION_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Go Radio motion provider tests currently support Linux only")
 endif()
 find_program(CUI_GO_RADIO_MOTION_EXECUTABLE go REQUIRED)
 function(cui_go_radio_motion_test suffix library legacy)
  add_test(NAME bridge_go_radio_motion_${suffix} COMMAND ${CMAKE_COMMAND} -E env
   CUI_BRIDGE_LIBRARY=${library} CUI_TEST_RADIO_MOTION_LEGACY=${legacy}
   CUI_TEST_RADIO_CONTENT_LEGACY=${legacy}
   CUI_TEST_RADIO_TOOLTIP_LEGACY=${legacy}
   CUI_TEST_RADIO_TAB_INDEX_LEGACY=${legacy} CUI_TEST_RADIO_PAINT_LEGACY=${legacy}
   SDL_RENDER_DRIVER=software
   ${CUI_GO_RADIO_MOTION_EXECUTABLE} ${ARGN})
  set_tests_properties(bridge_go_radio_motion_${suffix} PROPERTIES
   WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 300)
 endfunction()
 cui_go_radio_motion_test(dynamic $<TARGET_FILE:cui_bridge> 0
  test -p=1 -tags cui_dynamic ./cui -run ^TestRadio -count=1)
 cui_go_radio_motion_test(static "" 0
  test -p=1 ./cui -run ^TestRadio -count=1)
 cui_go_radio_motion_test(example "" 0 run -p=1 ./examples/radio_delivery)
 set(CUI_GO_RADIO_LEGACY_LIBRARY "" CACHE FILEPATH "Optional bridge without retained Radio motion")
 if(CUI_GO_RADIO_LEGACY_LIBRARY)
  if(NOT EXISTS "${CUI_GO_RADIO_LEGACY_LIBRARY}" OR IS_DIRECTORY "${CUI_GO_RADIO_LEGACY_LIBRARY}")
   message(FATAL_ERROR "CUI_GO_RADIO_LEGACY_LIBRARY must name an existing provider")
  endif()
  cui_go_radio_motion_test(legacy "${CUI_GO_RADIO_LEGACY_LIBRARY}" 1
   test -p=1 -tags cui_dynamic ./cui -run ^TestRadio -count=1)
  cui_go_radio_motion_test(legacy_bundled "${CUI_GO_RADIO_LEGACY_LIBRARY}" 1
   test -p=1 ./cui -run ^TestRadio -count=1)
 endif()
 if(CUI_BRIDGE_NATIVE_TESTS)
  cui_go_radio_motion_test(native $<TARGET_FILE:cui_bridge> 0
   run -p=1 -tags cui_dynamic ./examples/radio_delivery --font ${CUI_TEST_FONT} --smoke)
 endif()
endif()

option(CUI_RUST_RADIO_MOTION_TESTS "Check retained Radio presentation with Rust on Linux" OFF)
if(CUI_RUST_RADIO_MOTION_TESTS)
 if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Rust Radio motion provider tests currently support Linux only")
 endif()
 find_program(CUI_RUST_RADIO_MOTION_CARGO cargo REQUIRED)
 set(CUI_RUST_RADIO_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/rust-radio-shared" CACHE PATH "Cargo targets for shared Radio motion tests")
 set(CUI_RUST_RADIO_STATIC_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/rust-radio-static" CACHE PATH "Cargo targets for bundled Radio motion tests")
 function(cui_rust_radio_motion_test suffix target env supported)
  add_test(NAME bridge_rust_radio_motion_${suffix} COMMAND ${CMAKE_COMMAND} -E env
   --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH
   CARGO_TARGET_DIR=${target} RUSTFLAGS=-Dwarnings SDL_RENDER_DRIVER=software
   CUI_TEST_RADIO_MOTION=${supported} CUI_TEST_RADIO_PAINT=${supported} ${env}
   ${CUI_RUST_RADIO_MOTION_CARGO} ${ARGN})
  set_tests_properties(bridge_rust_radio_motion_${suffix} PROPERTIES
   WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
 endfunction()
 set(_radio_shared
  "CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:cui_bridge>")
 # The layout engine uses a process-wide current context.
 cui_rust_radio_motion_test(dynamic "${CUI_RUST_RADIO_TARGET_DIR}" "${_radio_shared}" 1
  test --offline --locked -j1 --test radio_motion --test radio_paint -- --test-threads=1)
 cui_rust_radio_motion_test(static "${CUI_RUST_RADIO_STATIC_TARGET_DIR}" "" 1
  test --offline --locked -j1 --test radio_motion --test radio_paint -- --test-threads=1)
 cui_rust_radio_motion_test(example "${CUI_RUST_RADIO_STATIC_TARGET_DIR}" "" 1
  run --offline --locked -j1 --example radio_delivery)
 set(CUI_RUST_RADIO_LEGACY_LIBRARY "" CACHE FILEPATH "Optional bridge without retained Radio motion")
 if(CUI_RUST_RADIO_LEGACY_LIBRARY)
  if(NOT EXISTS "${CUI_RUST_RADIO_LEGACY_LIBRARY}" OR IS_DIRECTORY "${CUI_RUST_RADIO_LEGACY_LIBRARY}")
   message(FATAL_ERROR "CUI_RUST_RADIO_LEGACY_LIBRARY must name an existing provider")
  endif()
  set(_radio_legacy "${CMAKE_CURRENT_BINARY_DIR}/rust-radio-legacy")
  file(MAKE_DIRECTORY "${_radio_legacy}")
  file(CREATE_LINK "${CUI_RUST_RADIO_LEGACY_LIBRARY}" "${_radio_legacy}/libcui_bridge.so" SYMBOLIC)
  cui_rust_radio_motion_test(legacy "${CUI_RUST_RADIO_TARGET_DIR}"
   "CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>;LD_LIBRARY_PATH=${_radio_legacy}" 0
   test --offline --locked -j1 --test radio_motion --test radio_paint -- --test-threads=1)
  cui_rust_radio_motion_test(legacy_example "${CUI_RUST_RADIO_TARGET_DIR}"
   "CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>;LD_LIBRARY_PATH=${_radio_legacy}" 0
   run --offline --locked -j1 --example radio_delivery)
 endif()
 if(CUI_BRIDGE_NATIVE_TESTS)
  cui_rust_radio_motion_test(native "${CUI_RUST_RADIO_TARGET_DIR}" "${_radio_shared}" 1
   run --offline --locked -j1 --example radio_delivery -- --font ${CUI_TEST_FONT} --smoke)
 endif()
endif()

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
add_test(NAME bridge_python_carousel_slots COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_carousel_slots.py)
add_test(NAME bridge_python_kbd_stroke COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge> CUI_EXPECT_KBD_STROKE=1
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_kbd_stroke.py)
add_test(NAME bridge_python_keyed_data_table_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/data_table_live.py)
set(CUI_PYTHON_DATA_TABLE_LEGACY_LIBRARY "" CACHE FILEPATH "Optional published pre-keyed DataTable Python bridge")
add_test(NAME bridge_python_data_table_paging COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 CUI_DATA_TABLE_LEGACY_LIBRARY=${CUI_PYTHON_DATA_TABLE_LEGACY_LIBRARY}
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_data_table_paging.py)
add_test(NAME bridge_python_data_table_paging_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/data_table_paging.py)
add_test(NAME bridge_python_data_table_capacity_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/data_table_paging.py --larger)
add_test(NAME bridge_python_run COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_run.py)
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
add_test(NAME bridge_python_responsive_notifications COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 CUI_EXPECT_RESPONSIVE_NOTIFICATIONS=1
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_responsive_notifications.py)
add_test(NAME bridge_python_notification_workspace COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/bridge/examples/notification_workspace.py)

add_executable(test_bridge_window_visibility ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_window_visibility.c)
target_link_libraries(test_bridge_window_visibility PRIVATE cui_bridge)
cui_warnings(test_bridge_window_visibility)
add_test(NAME bridge_window_visibility COMMAND test_bridge_window_visibility)
if(TARGET cui_sdl3)
 add_executable(test_bridge_window_visibility_sdl ${CUI_BRIDGE_ROOT}/bindings/bridge/tests/test_bridge_window_visibility_sdl.c)
 target_link_libraries(test_bridge_window_visibility_sdl PRIVATE cui_bridge cui_sdl3_dependency)
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
