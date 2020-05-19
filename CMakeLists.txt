cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(DABC)

#---Set paths where to put the libraries, executables and headers------------------------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#--- where to search for cmake modules ----
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

#--- Load some basic macros ---
include(DabcConfiguration)
include(DabcMacros)

add_subdirectory(base)

add_subdirectory(plugins)

add_subdirectory(applications)

# ================== Produce DABC headers ==========

get_property(__allHeaders GLOBAL PROPERTY DABC_HEADER_TARGETS)
add_custom_target(move_headers ALL DEPENDS ${__allHeaders})

