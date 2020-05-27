set(DABC_VERSION "2.10.0" CACHE STRING "DABC version" FORCE)

set(DABC_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include" CACHE STRING "DABC include dir" FORCE)

set(DABC_LIBRARY_DIR "${CMAKE_BINARY_DIR}/lib" CACHE STRING "DABC include dir" FORCE)

set(DABC_LIBRARY "libDabcBase" CACHE STRING "DABC main library" FORCE)

set(DABC_DEFINES "" CACHE STRING "DABC definitions" FORCE)

# ====== check ROOT and its c++ standard ==========
if(root)
   find_package(ROOT QUIET)
endif()

if(ROOT_FOUND AND NOT CMAKE_CXX_STANDARD)
   if(APPLE)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++11" _check_c11)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++1y" _check_c14)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++1z" _check_c17)
   else()
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++11" _check_c11)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++14" _check_c14)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++17" _check_c17)
   endif()
   if(NOT ${_check_c11} EQUAL -1)
      set(CMAKE_CXX_STANDARD 11)
   elseif(NOT ${_check_c14} EQUAL -1)
      set(CMAKE_CXX_STANDARD 14)
   elseif(NOT ${_check_c17} EQUAL -1)
      set(CMAKE_CXX_STANDARD 17)
   endif()
   message("FOUND ROOT with flags ${ROOT_CXX_FLAGS} DABC take standard ${CMAKE_CXX_STANDARD}")
endif()

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()

set(DABC_CXX_STANDARD "${CMAKE_CXX_STANDARD}" CACHE STRING "DABC cxx standard" FORCE)

if(debug)
  set(DABC_DEBUGLEVEL ${debug})
  set(DABC_DEFINES DEBUGLEVEL=${debug})
else()
  set(DABC_DEBUGLEVEL 2)
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/DABCConfig.cmake.in
               ${CMAKE_BINARY_DIR}/DABCConfig.cmake @ONLY NEWLINE_STYLE UNIX)

if(extrachecks)
  set(DABC_EXTRA_CHECKS "#define DABC_EXTRA_CHECKS")
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/defines.h.in
               ${CMAKE_BINARY_DIR}/include/dabc/defines.h @ONLY NEWLINE_STYLE UNIX)

if(APPLE)
   configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/dabclogin.mac.in
                  ${CMAKE_BINARY_DIR}/dabclogin @ONLY NEWLINE_STYLE UNIX)
else()
   configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/dabclogin.linux.in
                  ${CMAKE_BINARY_DIR}/dabclogin @ONLY NEWLINE_STYLE UNIX)
endif()

foreach(lib pthread dl)
   find_library(DABC_${lib}_LIBRARY ${lib})
endforeach()

if(NOT APPLE)
   find_library(DABC_RT_LIBRARY rt)
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/modules/DabcMacros.cmake
               ${CMAKE_BINARY_DIR}/DabcMacros.cmake COPYONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/DABCUseFile.cmake.in
               ${CMAKE_BINARY_DIR}/DABCUseFile.cmake @ONLY NEWLINE_STYLE UNIX)

