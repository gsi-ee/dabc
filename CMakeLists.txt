cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(
  DABC
  VERSION 2.11.0
  LANGUAGES C CXX)

# ---Set paths where to put the libraries, executables and
# headers------------------------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# --- where to search for cmake modules ----
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

include(CMakeDependentOption)
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# --- Load some basic macros ---
include(DabcBuildOptions)
include(DabcConfiguration)
include(DabcMacros)

option(DABC_LEGACY_BUILD "Copy files files in build dir" OFF)
option(DABC_LEGACY_INSTALL "Install plugins out of tree" OFF)
cmake_dependent_option(DABC_BUILD_APPLICATIONS "Build example applications" ON "DABC_LEGACY_BUILD" OFF)
cmake_dependent_option(DABC_INSTALL_APPLICATIONS "Install example applications" ON "DABC_BUILD_APPLICATIONS" OFF)

if (DABC_LEGACY_INSTALL)
set(DABC_INSTALL_PLUGINDIR ${CMAKE_INSTALL_PREFIX}/plugins)
else()
set(DABC_INSTALL_PLUGINDIR ${CMAKE_INSTALL_DATAROOTDIR}/dabc/plugins)
endif()

add_subdirectory(base)
add_subdirectory(plugins)

if(DABC_BUILD_APPLICATIONS)
  add_subdirectory(applications)
endif()

dabc_show_enabled_options()

# ================== Export DABC targets ==========

# Export the package for use from the build-tree (this registers the build-tree
# with a global CMake-registry)
export(PACKAGE ${CMAKE_PROJECT_NAME})

get_property(PROJECT_TARGETS GLOBAL PROPERTY DABC_INSTALL_LIBRARY_TARGETS
                                             ${libname})

# Add all targets to the build-tree export set
export(
  TARGETS ${PROJECT_TARGETS}
  NAMESPACE dabc::
  FILE ${CMAKE_PROJECT_NAME}Targets.cmake)

install(
  EXPORT ${CMAKE_PROJECT_NAME}Targets
  NAMESPACE dabc::
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/scripts/DABCConfig.cmake.in
  ${PROJECT_BINARY_DIR}/DABCConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_BINDIR)

write_basic_package_version_file(
  ${CMAKE_PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}UseFile.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/DabcMacros.cmake
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR})
