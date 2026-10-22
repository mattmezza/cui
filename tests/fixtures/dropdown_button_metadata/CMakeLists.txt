cmake_minimum_required(VERSION 3.20)
project(cui_dropdown_button_metadata_worker C CXX)
enable_testing()
get_filename_component(CUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(CUI_BUILD_NATIVE OFF CACHE BOOL "Portable worker fixture" FORCE)
add_subdirectory("${CUI_SOURCE_DIR}" cui)
# All source/test/guide/header registrations belong to the canonical project.
