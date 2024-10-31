set(DABC_DEFINES
    ""
    CACHE STRING "DABC definitions" FORCE)

# --- JAM22  try  for mbspex lib
cmake_policy(SET CMP0060 NEW)

# ====== check ROOT and its c++ standard ==========
if(root)
  find_package(ROOT QUIET)
endif()

if(ROOT_FOUND AND ROOT_CXX_STANDARD AND NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD ${ROOT_CXX_STANDARD})
  message(STATUS "FOUND ROOT with c++ standard ${CMAKE_CXX_STANDARD}")
endif()

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -W -Woverloaded-virtual -fsigned-char -Wzero-as-null-pointer-constant")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Woverloaded-virtual -fsigned-char")
endif()

set(DABC_CXX_STANDARD
    "${CMAKE_CXX_STANDARD}"
    CACHE STRING "DABC cxx standard" FORCE)

if(debug)
  set(DABC_DEBUGLEVEL ${debug})
  set(DABC_DEFINES DEBUGLEVEL=${debug})
else()
  set(DABC_DEBUGLEVEL 2)
endif()

if(extrachecks)
  set(DABC_EXTRA_CHECKS TRUE)
endif()

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/scripts/defines.h.in
  ${PROJECT_BINARY_DIR}/include/dabc/defines.h @ONLY NEWLINE_STYLE UNIX)

# --- for in-build use case ---
set(DABCSYS ${PROJECT_BINARY_DIR})
set(DABCLIBDIR lib)
if(APPLE)
  configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/dabclogin.mac.in
                 ${PROJECT_BINARY_DIR}/dabclogin @ONLY NEWLINE_STYLE UNIX)
else()
  configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/dabclogin.linux.in
                 ${PROJECT_BINARY_DIR}/dabclogin @ONLY NEWLINE_STYLE UNIX)
endif()
# -----------------------------

# --- for install use case ----
set(DABCSYS ${CMAKE_INSTALL_PREFIX})
set(DABCLIBDIR ${CMAKE_INSTALL_LIBDIR})
if(APPLE)
  configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/scripts/dabclogin.mac.in
    ${PROJECT_BINARY_DIR}/macros/dabclogin @ONLY NEWLINE_STYLE UNIX)
else()
  configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/scripts/dabclogin.linux.in
    ${PROJECT_BINARY_DIR}/macros/dabclogin @ONLY NEWLINE_STYLE UNIX)
endif()
# -----------------------------

foreach(lib pthread dl)
   find_library(DABC_${lib}_LIBRARY ${lib})
endforeach()

if(NOT APPLE)
  find_library(DABC_rt_LIBRARY rt)
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    find_library(DABC_m_LIBRARY m)
    find_library(DABC_cpp_LIBRARY "c++")
  endif()
endif()

configure_file(${PROJECT_SOURCE_DIR}/cmake/modules/DabcMacros.cmake
               ${PROJECT_BINARY_DIR}/DabcMacros.cmake COPYONLY)

configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/DABCUseFile.cmake.in
               ${PROJECT_BINARY_DIR}/DABCUseFile.cmake @ONLY NEWLINE_STYLE UNIX)
