set(DABC_VERSION "2.10.0" CACHE STRING "DABC version")

set(DABC_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include" CACHE STRING "DABC include dir")

set(DABC_LIBRARY "libDabcBase" CACHE STRING "DABC main library")

set(DABC_DEFINES)

if(debug)
  set(DABC_DEBUGLEVEL ${debug})
  set(DABC_DEFINES DEBUGLEVEL=${debug})
else()
  set(DABC_DEBUGLEVEL 2)
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/DABCConfig.cmake.in
               ${CMAKE_BINARY_DIR}/DABCConfig.cmake @ONLY NEWLINE_STYLE UNIX)

# configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/DABCUseFile.cmake.in
#                ${CMAKE_BINARY_DIR}/DABCUseFile.cmake @ONLY NEWLINE_STYLE UNIX)


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