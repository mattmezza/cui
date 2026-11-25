cmake_minimum_required(VERSION 3.20)
project(cui_node_bindings LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(CUI_BUILD_NATIVE "Build SDL3 backend" ON)
add_subdirectory(../bridge cui-bridge-build)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
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

add_test(NAME node_notifications_v1 COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/notifications-v1.test.cjs)

# Synthetic protocol boundaries stay in a Linux test-only preload library.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
 add_library(cui_notification_v1_fixture SHARED ../tests/notification_v1_fixture.c)
 target_link_libraries(cui_notification_v1_fixture PRIVATE cui_bridge)
 target_compile_options(cui_notification_v1_fixture PRIVATE -Wall -Wextra -Wpedantic -Werror)
 foreach(mode edges capacity limit zero repeat size unavailable)
  add_test(NAME node_notifications_v1_protocol_${mode} COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings>
   LD_PRELOAD=$<TARGET_FILE:cui_notification_v1_fixture> CUI_NOTIFICATION_FIXTURE=${mode}
   ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/notifications-v1-protocol.test.cjs)
 endforeach()
endif()

add_test(NAME node_value_notifications_v1 COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/value-notifications-v1.test.cjs)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
 add_library(cui_value_notification_fixture SHARED test/value-notifications-provider.c)
 target_include_directories(cui_value_notification_fixture PRIVATE ../../include)
 target_compile_options(cui_value_notification_fixture PRIVATE -Wall -Wextra -Wpedantic -Werror)
 foreach(mode edges capacity size unavailable limit zero repeat tag kind utf8 nul flags)
  add_test(NAME node_value_notifications_protocol_${mode} COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings>
   LD_PRELOAD=$<TARGET_FILE:cui_value_notification_fixture> CUI_VALUE_FIXTURE=${mode}
   ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/value-notifications-protocol.test.cjs)
 endforeach()
 set(CUI_NODE_VALUE_OLD_LIBRARY "" CACHE FILEPATH "Optional actual pre-value bridge for compatibility tests")
 if(CUI_NODE_VALUE_OLD_LIBRARY)
  if(NOT EXISTS "${CUI_NODE_VALUE_OLD_LIBRARY}")
   message(FATAL_ERROR "CUI_NODE_VALUE_OLD_LIBRARY must name an existing actual older bridge")
  endif()
  add_library(cui_node_old_value_bindings MODULE addon.c)
  target_include_directories(cui_node_old_value_bindings PRIVATE ${NODE_API_INCLUDE_DIR} ../../include)
  target_link_libraries(cui_node_old_value_bindings PRIVATE "${CUI_NODE_VALUE_OLD_LIBRARY}" m ${CMAKE_DL_LIBS})
  target_compile_options(cui_node_old_value_bindings PRIVATE -Wall -Wextra -Wpedantic -Werror)
  set_target_properties(cui_node_old_value_bindings PROPERTIES PREFIX "" OUTPUT_NAME "cui_old_values" SUFFIX ".node")
  add_library(cui_value_notification_missing_fixture SHARED test/value-notifications-provider.c)
  target_include_directories(cui_value_notification_missing_fixture PRIVATE ../../include)
  target_compile_definitions(cui_value_notification_missing_fixture PRIVATE CUI_VALUE_FIXTURE_MISSING)
  target_compile_options(cui_value_notification_missing_fixture PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME node_value_notifications_old_library COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_value_bindings> CUI_EXPECT_OLD=1
   ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/value-notifications-v1.test.cjs)
  add_test(NAME node_value_notifications_protocol_missing COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_value_bindings>
   LD_PRELOAD=$<TARGET_FILE:cui_value_notification_missing_fixture> CUI_VALUE_FIXTURE=missing
   ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/value-notifications-protocol.test.cjs)
 endif()
endif()

add_test(NAME node_shadows COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/shadows.test.cjs)

add_test(NAME node_part_shadows COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/part-shadows.test.cjs)

# Controlled Slider is independent of the legacy scalar route and notification queues.
add_test(NAME node_slider_model COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/slider-model.test.cjs)
add_test(NAME node_slider_model_protocol COMMAND ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/slider-model-protocol.test.cjs)
foreach(mode accept reject_vertical single)
 if(mode STREQUAL "reject_vertical")
  set(slider_model_args --reject --vertical)
 elseif(mode STREQUAL "single")
  set(slider_model_args --single)
 else()
  set(slider_model_args)
 endif()
 add_test(NAME node_slider_model_example_${mode} COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/slider-model.cjs ${slider_model_args})
endforeach()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
 set(CUI_NODE_SLIDER_OLD_LIBRARY "" CACHE FILEPATH "Optional actual pre-typed-Slider bridge")
 if(CUI_NODE_SLIDER_OLD_LIBRARY)
  if(NOT EXISTS "${CUI_NODE_SLIDER_OLD_LIBRARY}")
   message(FATAL_ERROR "CUI_NODE_SLIDER_OLD_LIBRARY must name an existing actual older bridge")
  endif()
  add_library(cui_node_old_slider_bindings MODULE addon.c)
  target_include_directories(cui_node_old_slider_bindings PRIVATE ${NODE_API_INCLUDE_DIR} ../../include)
  target_link_libraries(cui_node_old_slider_bindings PRIVATE "${CUI_NODE_SLIDER_OLD_LIBRARY}" m ${CMAKE_DL_LIBS})
  target_compile_options(cui_node_old_slider_bindings PRIVATE -Wall -Wextra -Wpedantic -Werror)
  set_target_properties(cui_node_old_slider_bindings PROPERTIES PREFIX "" OUTPUT_NAME "cui_old_slider" SUFFIX ".node")
  add_test(NAME node_slider_model_old_library COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_slider_bindings> CUI_SLIDER_FIXTURE=missing
   ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/slider-model-optional.test.cjs)
  foreach(missing RANGE 0 16)
   add_library(cui_slider_optional_${missing} SHARED test/slider-model-provider.c)
   target_include_directories(cui_slider_optional_${missing} PRIVATE ../../include)
   target_compile_definitions(cui_slider_optional_${missing} PRIVATE CUI_SLIDER_OMIT=${missing})
   target_compile_options(cui_slider_optional_${missing} PRIVATE -Wall -Wextra -Wpedantic -Werror)
   add_test(NAME node_slider_model_missing_${missing} COMMAND ${CMAKE_COMMAND} -E env
    CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_slider_bindings> LD_PRELOAD=$<TARGET_FILE:cui_slider_optional_${missing}>
    CUI_SLIDER_FIXTURE=$<IF:$<EQUAL:${missing},16>,complete,missing>
    ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/slider-model-optional.test.cjs)
  endforeach()
  add_test(NAME node_slider_model_native_config COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_slider_bindings> LD_PRELOAD=$<TARGET_FILE:cui_slider_optional_16>
   CUI_SLIDER_FIXTURE=config_parent
   ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/slider-model-native-config.test.cjs)
  foreach(mode unavailable config_size value_size receipt_size request_size presentation_size)
   add_test(NAME node_slider_model_${mode} COMMAND ${CMAKE_COMMAND} -E env
    CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_slider_bindings> LD_PRELOAD=$<TARGET_FILE:cui_slider_optional_16>
    CUI_SLIDER_FIXTURE=${mode}
    ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/slider-model-optional.test.cjs)
  endforeach()
 endif()
endif()
find_program(CUI_NODE_TSC_EXECUTABLE tsc)
if(CUI_NODE_TSC_EXECUTABLE)
 add_test(NAME node_slider_model_types_runtime COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/test/run-slider-model-types.py
  --tsc ${CUI_NODE_TSC_EXECUTABLE} --node ${NODE_EXECUTABLE}
  --output ${CMAKE_CURRENT_BINARY_DIR}/slider-model-types)
endif()

# Localized TimeField uses an independent applied-change FIFO, never Slider receipts.
add_test(NAME node_time_field_localized COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/time-field-localized.test.cjs)
add_test(NAME node_time_field_localized_protocol COMMAND ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/time-field-localized-protocol.test.cjs)
add_test(NAME node_time_field_localized_example COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/examples/time-field-localized.cjs)
if(TARGET cui_node_old_slider_bindings)
 add_test(NAME node_time_field_localized_actual_old COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_slider_bindings> CUI_TIME_FIXTURE=missing
  ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/time-field-localized-optional.test.cjs)
 foreach(missing RANGE 0 10)
  add_library(cui_time_optional_${missing} SHARED test/time-field-localized-provider.c)
  target_include_directories(cui_time_optional_${missing} PRIVATE ../../include)
  target_compile_definitions(cui_time_optional_${missing} PRIVATE CUI_TIME_OMIT=${missing})
  target_compile_options(cui_time_optional_${missing} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME node_time_field_localized_missing_${missing} COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_slider_bindings> LD_PRELOAD=$<TARGET_FILE:cui_time_optional_${missing}>
   CUI_TIME_FIXTURE=$<IF:$<EQUAL:${missing},10>,complete,missing>
   ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/time-field-localized-optional.test.cjs)
 endforeach()
 foreach(mode unavailable config_size result_size change_size)
  add_test(NAME node_time_field_localized_${mode} COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_slider_bindings> LD_PRELOAD=$<TARGET_FILE:cui_time_optional_10>
   CUI_TIME_FIXTURE=${mode} ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/time-field-localized-optional.test.cjs)
 endforeach()
endif()
if(CUI_NODE_TSC_EXECUTABLE)
 add_test(NAME node_time_field_localized_types_runtime COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/test/run-time-field-localized-types.py
  --tsc ${CUI_NODE_TSC_EXECUTABLE} --node ${NODE_EXECUTABLE}
  --output ${CMAKE_CURRENT_BINARY_DIR}/time-field-localized-types)
endif()

add_test(NAME node_skeleton_presentation COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/test/skeleton-presentation.test.cjs)
add_test(NAME node_skeleton_example COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/examples/skeleton-profile.cjs)
if(CUI_BRIDGE_NATIVE_TESTS)
 add_test(NAME node_skeleton_native COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/skeleton-profile.cjs "${CUI_TEST_FONT}" --smoke)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
 add_library(cui_node_skeleton_bad_size SHARED test/skeleton-size-provider.c)
 target_include_directories(cui_node_skeleton_bad_size PRIVATE ../../include)
 target_compile_options(cui_node_skeleton_bad_size PRIVATE -Wall -Wextra -Wpedantic -Werror)
 add_test(NAME node_skeleton_bad_size COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> LD_PRELOAD=$<TARGET_FILE:cui_node_skeleton_bad_size>
  CUI_SKELETON_UNSUPPORTED=1 ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/skeleton-presentation.test.cjs)
 foreach(provider legacy partial)
  string(TOUPPER "${provider}" kind)
  set(provider_var CUI_NODE_SKELETON_${kind}_LIBRARY)
  set(${provider_var} "" CACHE FILEPATH "Existing ${provider} bridge for Skeleton compatibility")
  if(${provider_var})
   if(NOT EXISTS "${${provider_var}}")
    message(FATAL_ERROR "${provider_var} must name an existing bridge library")
   endif()
   set(target cui_node_skeleton_${provider})
   add_library(${target} MODULE addon.c)
   target_include_directories(${target} PRIVATE ${NODE_API_INCLUDE_DIR} ../../include)
   target_link_libraries(${target} PRIVATE "${${provider_var}}" m ${CMAKE_DL_LIBS})
   target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
   set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".node")
   add_test(NAME node_skeleton_${provider} COMMAND ${CMAKE_COMMAND} -E env
    CUI_NODE_ADDON=$<TARGET_FILE:${target}> CUI_SKELETON_UNSUPPORTED=1 ${NODE_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/test/skeleton-presentation.test.cjs)
  endif()
 endforeach()
endif()
if(CUI_NODE_TSC_EXECUTABLE)
 add_test(NAME node_skeleton_types_runtime COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/test/run-skeleton-types.py
  --tsc ${CUI_NODE_TSC_EXECUTABLE} --node ${NODE_EXECUTABLE}
  --output ${CMAKE_CURRENT_BINARY_DIR}/skeleton-types)
endif()

add_test(NAME node_icon_resources COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} --test
 ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-resources.test.cjs)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
 add_library(cui_node_icon_bad_size SHARED test/icon-size-provider.c)
 target_include_directories(cui_node_icon_bad_size PRIVATE ../../include)
 target_compile_options(cui_node_icon_bad_size PRIVATE -Wall -Wextra -Wpedantic -Werror)
 add_test(NAME node_icon_resources_bad_size COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> LD_PRELOAD=$<TARGET_FILE:cui_node_icon_bad_size>
  CUI_ICON_UNSUPPORTED=1 ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-resources.test.cjs)
 set(CUI_NODE_ICON_ALPHA4_LIBRARY "" CACHE FILEPATH "Existing alpha.4 bridge with Icon resources")
 if(CUI_NODE_ICON_ALPHA4_LIBRARY)
  if(NOT EXISTS "${CUI_NODE_ICON_ALPHA4_LIBRARY}")
   message(FATAL_ERROR "CUI_NODE_ICON_ALPHA4_LIBRARY must name an existing bridge library")
  endif()
  add_library(cui_node_icon_alpha4 MODULE addon.c)
  target_include_directories(cui_node_icon_alpha4 PRIVATE ${NODE_API_INCLUDE_DIR} ../../include)
  target_link_libraries(cui_node_icon_alpha4 PRIVATE "${CUI_NODE_ICON_ALPHA4_LIBRARY}" m ${CMAKE_DL_LIBS})
  target_compile_options(cui_node_icon_alpha4 PRIVATE -Wall -Wextra -Wpedantic -Werror)
  set_target_properties(cui_node_icon_alpha4 PROPERTIES PREFIX "" SUFFIX ".node")
  add_test(NAME node_icon_resources_alpha4 COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_icon_alpha4>
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-resources.test.cjs)
 endif()
 if(TARGET cui_node_old_slider_bindings)
  add_test(NAME node_icon_resources_old COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_old_slider_bindings> CUI_ICON_UNSUPPORTED=1
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-resources.test.cjs)
 endif()
endif()
add_test(NAME node_icon_resources_example COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/examples/icon-resource.cjs)
if(CUI_NODE_TSC_EXECUTABLE)
 add_test(NAME node_icon_resources_types COMMAND "${CUI_NODE_TSC_EXECUTABLE}" --noEmit --strict
  --target ES2020 --module commonjs --skipLibCheck ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-resources.types.ts)
endif()

add_test(NAME node_icon_badge COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} --test
 ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-badge.test.cjs)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
 add_library(cui_node_icon_badge_mutation SHARED test/icon-badge-mutation-provider.c)
 target_include_directories(cui_node_icon_badge_mutation PRIVATE ../../include)
 target_link_libraries(cui_node_icon_badge_mutation PRIVATE ${CMAKE_DL_LIBS})
 target_compile_options(cui_node_icon_badge_mutation PRIVATE -Wall -Wextra -Wpedantic -Werror)
 foreach(mode invalid-copy invalid-result)
  add_test(NAME node_icon_badge_${mode} COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> LD_PRELOAD=$<TARGET_FILE:cui_node_icon_badge_mutation>
   CUI_ICON_BADGE_LIBRARY=$<TARGET_FILE:cui_bridge>
   CUI_ICON_BADGE_PROVIDER=${mode} ${NODE_EXECUTABLE} --test
   ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-badge.test.cjs)
 endforeach()
 if(TARGET cui_node_icon_alpha4)
  add_test(NAME node_icon_badge_alpha4 COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_icon_alpha4> CUI_ICON_BADGE_PROVIDER=alpha4
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-badge.test.cjs)
 endif()
 set(CUI_NODE_BADGE_PARTIAL_LIBRARY "" CACHE FILEPATH "Existing bridge with incomplete Badge symbols")
 if(CUI_NODE_BADGE_PARTIAL_LIBRARY)
  if(NOT EXISTS "${CUI_NODE_BADGE_PARTIAL_LIBRARY}")
   message(FATAL_ERROR "CUI_NODE_BADGE_PARTIAL_LIBRARY must name an existing bridge library")
  endif()
  add_library(cui_node_badge_partial MODULE addon.c)
  target_include_directories(cui_node_badge_partial PRIVATE ${NODE_API_INCLUDE_DIR} ../../include)
  target_link_libraries(cui_node_badge_partial PRIVATE "${CUI_NODE_BADGE_PARTIAL_LIBRARY}" m ${CMAKE_DL_LIBS})
  target_compile_options(cui_node_badge_partial PRIVATE -Wall -Wextra -Wpedantic -Werror)
  set_target_properties(cui_node_badge_partial PROPERTIES PREFIX "" SUFFIX ".node")
  add_test(NAME node_icon_badge_partial COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_badge_partial> CUI_ICON_BADGE_PROVIDER=partial-badge
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/icon-badge.test.cjs)
 endif()
endif()
add_test(NAME node_icon_badge_example COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/examples/icon-badge-inbox.cjs)
if(CUI_BRIDGE_NATIVE_TESTS)
 add_test(NAME node_icon_badge_native COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> SDL_RENDER_DRIVER=software ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/icon-badge-inbox.cjs "${CUI_TEST_FONT}" --smoke)
endif()
if(CUI_NODE_TSC_EXECUTABLE)
 add_test(NAME node_icon_badge_types COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> "${Python3_EXECUTABLE}"
  ${CMAKE_CURRENT_SOURCE_DIR}/test/run-icon-badge-types.py --tsc "${CUI_NODE_TSC_EXECUTABLE}"
  --node "${NODE_EXECUTABLE}" --output ${CMAKE_CURRENT_BINARY_DIR}/icon-badge-types)
endif()

add_test(NAME node_keyed_data_table COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} --test
 ${CMAKE_CURRENT_SOURCE_DIR}/test/keyed-data-table.test.cjs)
if(TARGET cui_node_icon_alpha4)
 add_test(NAME node_keyed_data_table_alpha4 COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_icon_alpha4> CUI_KEYED_DATA_TABLE_LEGACY=1
  ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/keyed-data-table.test.cjs)
endif()
add_test(NAME node_keyed_data_table_example COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/examples/data-table-live.cjs)
if(CUI_BRIDGE_NATIVE_TESTS)
 add_test(NAME node_keyed_data_table_native COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> SDL_RENDER_DRIVER=software ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/data-table-live.cjs "${CUI_TEST_FONT}" --smoke)
endif()
if(CUI_NODE_TSC_EXECUTABLE)
 add_test(NAME node_keyed_data_table_types COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> "${Python3_EXECUTABLE}"
  ${CMAKE_CURRENT_SOURCE_DIR}/test/run-data-table-types.py --case keyed --tsc "${CUI_NODE_TSC_EXECUTABLE}"
  --node "${NODE_EXECUTABLE}" --output ${CMAKE_CURRENT_BINARY_DIR}/keyed-data-table-types)
endif()

add_test(NAME node_data_table_paging COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} --test
 ${CMAKE_CURRENT_SOURCE_DIR}/test/data-table-paging.test.cjs)
add_test(NAME node_data_table_paging_example COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/examples/data-table-paging.cjs)
add_test(NAME node_data_table_capacity COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE} --test
 ${CMAKE_CURRENT_SOURCE_DIR}/test/data-table-capacity.test.cjs)
add_test(NAME node_data_table_capacity_example COMMAND ${CMAKE_COMMAND} -E env
 CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> ${NODE_EXECUTABLE}
 ${CMAKE_CURRENT_SOURCE_DIR}/examples/data-table-paging.cjs --larger)
if(CUI_BRIDGE_NATIVE_TESTS)
 add_test(NAME node_data_table_paging_native COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> SDL_RENDER_DRIVER=software ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/data-table-paging.cjs "${CUI_TEST_FONT}" --smoke)
 add_test(NAME node_data_table_capacity_native COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> SDL_RENDER_DRIVER=software ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/data-table-paging.cjs "${CUI_TEST_FONT}" --smoke --larger)
 add_test(NAME node_native_run COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> CUI_TEST_FONT=${CUI_TEST_FONT}
  SDL_RENDER_DRIVER=software ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/run-native.test.cjs)
endif()
if(CUI_NODE_TSC_EXECUTABLE)
 add_test(NAME node_data_table_paging_types COMMAND ${CMAKE_COMMAND} -E env
  CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> "${Python3_EXECUTABLE}"
  ${CMAKE_CURRENT_SOURCE_DIR}/test/run-data-table-types.py --case paging --tsc "${CUI_NODE_TSC_EXECUTABLE}"
  --node "${NODE_EXECUTABLE}" --output ${CMAKE_CURRENT_BINARY_DIR}/data-table-paging-types)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
 add_library(cui_node_capacity_query_fixture SHARED ../tests/data_table_capacity_provider.c)
 target_include_directories(cui_node_capacity_query_fixture PRIVATE ../../include)
 target_compile_options(cui_node_capacity_query_fixture PRIVATE -Wall -Wextra -Wpedantic -Werror)
 foreach(mode limited zero large)
  add_test(NAME node_data_table_capacity_${mode} COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> CUI_DATA_TABLE_CAPACITY_FIXTURE=${mode}
   LD_PRELOAD=$<TARGET_FILE:cui_node_capacity_query_fixture>
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/data-table-capacity.test.cjs)
 endforeach()
 add_library(cui_node_data_table_paging_fixture SHARED ../tests/data_table_paging_provider.c)
 target_include_directories(cui_node_data_table_paging_fixture PRIVATE ../../include)
 target_compile_options(cui_node_data_table_paging_fixture PRIVATE -Wall -Wextra -Wpedantic -Werror)
 foreach(mode size exact reserved frame window visible capacity key)
  add_test(NAME node_data_table_paging_${mode} COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_bindings> CUI_DATA_TABLE_PAGING_FIXTURE=${mode}
   LD_PRELOAD=$<TARGET_FILE:cui_node_data_table_paging_fixture>
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/data-table-paging.test.cjs)
 endforeach()
 if(TARGET cui_node_icon_alpha4)
  add_library(cui_node_capacity_setter_fixture SHARED ../tests/data_table_capacity_provider.c)
  target_include_directories(cui_node_capacity_setter_fixture PRIVATE ../../include)
  target_compile_definitions(cui_node_capacity_setter_fixture PRIVATE CUI_TEST_CAPACITY_SETTER_ONLY)
  target_compile_options(cui_node_capacity_setter_fixture PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME node_data_table_capacity_alpha4 COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_icon_alpha4> CUI_DATA_TABLE_CAPACITY_FIXTURE=legacy
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/data-table-capacity.test.cjs)
  foreach(missing query setter)
   if(missing STREQUAL "query")
    set(capacity_fixture cui_node_capacity_setter_fixture)
   else()
    set(capacity_fixture cui_node_capacity_query_fixture)
   endif()
   add_test(NAME node_data_table_capacity_missing_${missing} COMMAND ${CMAKE_COMMAND} -E env
    CUI_NODE_ADDON=$<TARGET_FILE:cui_node_icon_alpha4> CUI_DATA_TABLE_CAPACITY_FIXTURE=missing_${missing}
    LD_PRELOAD=$<TARGET_FILE:${capacity_fixture}>
    ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/data-table-capacity.test.cjs)
  endforeach()
  add_library(cui_node_partial_paging_fixture SHARED ../tests/data_table_paging_provider.c)
  target_include_directories(cui_node_partial_paging_fixture PRIVATE ../../include)
  target_compile_definitions(cui_node_partial_paging_fixture PRIVATE CUI_TEST_PARTIAL_PAGING_PROVIDER)
  target_compile_options(cui_node_partial_paging_fixture PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME node_data_table_paging_alpha4 COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_icon_alpha4> CUI_DATA_TABLE_PAGING_FIXTURE=legacy
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/data-table-paging.test.cjs)
  add_test(NAME node_data_table_paging_partial COMMAND ${CMAKE_COMMAND} -E env
   CUI_NODE_ADDON=$<TARGET_FILE:cui_node_icon_alpha4> CUI_DATA_TABLE_PAGING_FIXTURE=partial
   LD_PRELOAD=$<TARGET_FILE:cui_node_partial_paging_fixture>
   ${NODE_EXECUTABLE} --test ${CMAKE_CURRENT_SOURCE_DIR}/test/data-table-paging.test.cjs)
 endif()
endif()
