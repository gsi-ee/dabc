set(DABC_VERSION "2.10.0" CACHE STRING "DABC version")

set(DABC_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include" CACHE STRING "DABC include dir")

set(DABC_LIBRARY "libDabcBase" CACHE STRING "DABC main library")

configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/DABCConfig.cmake.in
               ${CMAKE_BINARY_DIR}/DABCConfig.cmake @ONLY NEWLINE_STYLE UNIX)

# configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/DABCUseFile.cmake.in
#                ${CMAKE_BINARY_DIR}/DABCUseFile.cmake @ONLY NEWLINE_STYLE UNIX)

if(APPLE)
   configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/dabclogin.mac.in
                  ${CMAKE_BINARY_DIR}/dabclogin @ONLY NEWLINE_STYLE UNIX)
else()
   configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/dabclogin.linux.in
                  ${CMAKE_BINARY_DIR}/dabclogin @ONLY NEWLINE_STYLE UNIX)
endif()