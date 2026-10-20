cmake_minimum_required(VERSION 3.20)
project(cui_kbd_worker C)
enable_testing()
get_filename_component(CUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(CUI_BUILD_NATIVE OFF CACHE BOOL "Portable worker" FORCE)
add_subdirectory("${CUI_SOURCE_DIR}" cui EXCLUDE_FROM_ALL)
# Canonical targets own source registration and guide extraction.
