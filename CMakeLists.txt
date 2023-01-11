cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(
  DABC
  VERSION 2.11.0
  LANGUAGES C CXX)

#cmake_policy(SET CMP0060 NEW) 

#---Set paths where to put the libraries, executables and headers------------------------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

#--- JAM 6-12-22: try for rpath problems with libmbspex:
#set(CMAKE_SKIP_BUILD_RPATH FALSE)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_INSTALL_RPATH "${ORIGIN}")
#--------------- end JAM

#--- where to search for cmake modules ----
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

#--- Set install dirs ---
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

#--- Load some basic macros ---
include(DabcBuildOptions)
include(DabcConfiguration)
include(DabcMacros)

# Make applications an optional build.
# Allow to install apps only if they are build.
include(CMakeDependentOption)

option(DABC_LEGACY_INSTALL "Install plugins out of tree" OFF)
option(DABC_BUILD_APPLICATIONS "Build example applications" ON)
cmake_dependent_option(DABC_INSTALL_APPLICATIONS "Install example applications" ON "DABC_BUILD_APPLICATIONS" OFF)

if (DABC_LEGACY_INSTALL)
  set(DABC_INSTALL_PLUGINDIR ${CMAKE_INSTALL_PREFIX}/plugins)
  set(DABC_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})
else()
  set(DABC_INSTALL_PLUGINDIR ${CMAKE_INSTALL_DATAROOTDIR}/dabc/plugins)
  set(DABC_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/dabc)
endif()

add_subdirectory(base)

add_subdirectory(plugins)

if(DABC_BUILD_APPLICATIONS)
  add_subdirectory(applications)
endif()

# ================== Produce DABC headers ==========

get_property(__allHeaders GLOBAL PROPERTY DABC_HEADER_TARGETS)
add_custom_target(copy_headers ALL DEPENDS ${__allHeaders})

DABC_SHOW_ENABLED_OPTIONS()

# ================== Export DABC targets ==========

# Export the package for use from the build-tree (this registers the build-tree
# with a global CMake-registry)
export(PACKAGE ${PROJECT_NAME})

get_property(PROJECT_TARGETS GLOBAL PROPERTY DABC_INSTALL_LIBRARY_TARGETS)

# Add all targets to the build-tree export set
export(
  TARGETS ${PROJECT_TARGETS}
  NAMESPACE dabc::
  FILE ${PROJECT_NAME}Targets.cmake)

install(
  EXPORT ${PROJECT_NAME}Targets
  NAMESPACE dabc::
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/scripts/DABCConfig.cmake.in
  ${PROJECT_BINARY_DIR}/DABCConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

write_basic_package_version_file(
  ${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(
    FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}UseFile.cmake
        ${PROJECT_BINARY_DIR}/DabcMacros.cmake
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

if (${DABC_LEGACY_INSTALL})
  install(
    PROGRAMS ${PROJECT_BINARY_DIR}/macros/dabclogin
    DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
  install(
    PROGRAMS ${PROJECT_BINARY_DIR}/macros/dabclogin
    DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
