add_test(NAME bridge_python_attachment_options COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_attachment_options.py)

add_test(NAME bridge_python_attachment_actions COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_attachment_actions.py)
add_test(NAME bridge_python_attachment_actions_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/examples/attachment_actions.py)
add_test(NAME bridge_python_attachment_slots COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_attachment_slots.py)
add_test(NAME bridge_python_attachment_slots_example COMMAND ${CMAKE_COMMAND} -E env
 PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge>
 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/examples/attachment_slots.py)

option(CUI_ATTACHMENT_BINDING_TESTS "Check Go and Rust retained Attachment on Linux" OFF)
if(NOT CUI_ATTACHMENT_BINDING_TESTS)
 return()
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
 message(FATAL_ERROR "Compiled Attachment binding tests require Linux")
endif()
find_program(CUI_ATTACHMENT_GO go REQUIRED)
find_program(CUI_ATTACHMENT_CARGO cargo REQUIRED)
set(CUI_ATTACHMENT_LEGACY_LIBRARY "" CACHE FILEPATH "Optional released bridge without Attachment V1")

function(cui_attachment_binding_tests mode go_provider rust_target legacy)
 set(go_tags)
 if(go_provider)
  set(go_tags -tags cui_dynamic)
 endif()
 add_test(NAME bridge_go_attachment_${mode} COMMAND ${CMAKE_COMMAND} -E env
  GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
  CUI_BRIDGE_LIBRARY=${go_provider} CUI_TEST_LEGACY_ATTACHMENT=${legacy}
  ${CUI_ATTACHMENT_GO} test ${go_tags} ./cui -count=1 -run "^TestAttachment(Presentation|Actions)?$")
 add_test(NAME bridge_rust_attachment_${mode} COMMAND ${CMAKE_COMMAND} -E env
  --unset=CUI_BRIDGE_LIB_DIR --unset=LD_LIBRARY_PATH --unset=LD_PRELOAD
  CARGO_TARGET_DIR=${rust_target} RUSTFLAGS=-Dwarnings CUI_TEST_LEGACY_ATTACHMENT=${legacy}
  ${ARGN} ${CUI_ATTACHMENT_CARGO} test --offline --locked -j1 --test attachment)
 set_tests_properties(bridge_go_attachment_${mode} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
 set_tests_properties(bridge_rust_attachment_${mode} PROPERTIES
  WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
endfunction()

set(attachment_shared CUI_BRIDGE_LIB_DIR=$<TARGET_FILE_DIR:cui_bridge>)
add_test(NAME bridge_go_attachment_example COMMAND ${CMAKE_COMMAND} -E env
 GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
 CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge> ${CUI_ATTACHMENT_GO} run -tags cui_dynamic ./examples/attachment_upload)
set_tests_properties(bridge_go_attachment_example PROPERTIES
 WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
add_test(NAME bridge_rust_attachment_example COMMAND ${CMAKE_COMMAND} -E env
 ${attachment_shared} CARGO_TARGET_DIR=${CUI_RUST_SKELETON_TARGET_DIR} RUSTFLAGS=-Dwarnings
 ${CUI_ATTACHMENT_CARGO} run --offline --locked -j1 --example attachment_upload)
set_tests_properties(bridge_rust_attachment_example PROPERTIES
 WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
add_test(NAME bridge_go_attachment_actions_example COMMAND ${CMAKE_COMMAND} -E env
 GOMAXPROCS=1 GOFLAGS=-p=1 GOPROXY=off GOSUMDB=off GOTOOLCHAIN=local
 CUI_BRIDGE_LIBRARY=$<TARGET_FILE:cui_bridge> ${CUI_ATTACHMENT_GO} run -tags cui_dynamic ./examples/attachment_actions)
set_tests_properties(bridge_go_attachment_actions_example PROPERTIES
 WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/go" RUN_SERIAL TRUE TIMEOUT 600)
add_test(NAME bridge_rust_attachment_actions_example COMMAND ${CMAKE_COMMAND} -E env
 ${attachment_shared} CARGO_TARGET_DIR=${CUI_RUST_SKELETON_TARGET_DIR} RUSTFLAGS=-Dwarnings
 ${CUI_ATTACHMENT_CARGO} run --offline --locked -j1 --example attachment_actions)
set_tests_properties(bridge_rust_attachment_actions_example PROPERTIES
 WORKING_DIRECTORY "${CUI_BRIDGE_ROOT}/bindings/rust" RUN_SERIAL TRUE TIMEOUT 600)
cui_attachment_binding_tests(dynamic $<TARGET_FILE:cui_bridge>
 "${CUI_RUST_SKELETON_TARGET_DIR}" 0 ${attachment_shared})
cui_attachment_binding_tests(bundled "" "${CUI_RUST_SKELETON_STATIC_TARGET_DIR}" 0)
if(CUI_ATTACHMENT_LEGACY_LIBRARY)
 if(NOT EXISTS "${CUI_ATTACHMENT_LEGACY_LIBRARY}" OR IS_DIRECTORY "${CUI_ATTACHMENT_LEGACY_LIBRARY}")
  message(FATAL_ERROR "CUI_ATTACHMENT_LEGACY_LIBRARY must name an existing shared bridge")
 endif()
 set(attachment_legacy "${CMAKE_CURRENT_BINARY_DIR}/attachment-legacy")
 file(MAKE_DIRECTORY "${attachment_legacy}")
 file(CREATE_LINK "${CUI_ATTACHMENT_LEGACY_LIBRARY}" "${attachment_legacy}/libcui_bridge.so" SYMBOLIC)
 cui_attachment_binding_tests(legacy "${CUI_ATTACHMENT_LEGACY_LIBRARY}"
  "${CUI_RUST_SKELETON_TARGET_DIR}" 1 ${attachment_shared} LD_LIBRARY_PATH=${attachment_legacy})
 add_test(NAME bridge_python_attachment_legacy COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=${CUI_ATTACHMENT_LEGACY_LIBRARY}
  CUI_TEST_LEGACY_ATTACHMENT=1 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_attachment_options.py)
 add_test(NAME bridge_python_attachment_actions_legacy COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=${CUI_ATTACHMENT_LEGACY_LIBRARY}
  CUI_TEST_LEGACY_ATTACHMENT_ACTIONS=1 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_attachment_actions.py)
 add_test(NAME bridge_python_attachment_slots_legacy COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${CUI_BRIDGE_ROOT}/bindings/python CUI_BRIDGE_LIBRARY=${CUI_ATTACHMENT_LEGACY_LIBRARY}
  CUI_TEST_LEGACY_ATTACHMENT_SLOTS=1 ${Python3_EXECUTABLE} ${CUI_BRIDGE_ROOT}/bindings/python/test_attachment_slots.py)
endif()
