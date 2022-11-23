# ====== check ROOT and its c++ standard ==========
if(root)
  find_package(ROOT QUIET COMPONENTS RHTTP XMLIO Tree)
endif()

if(NOT ROOT_FOUND)
  set(CMAKE_CXX_STANDARD 11)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(-Wshadow -W -Woverloaded-virtual -fsigned-char)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  add_compile_options(-stdlib=libc++ -W -Woverloaded-virtual -fsigned-char)
endif()

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
  ${PROJECT_BINARY_DIR}/inc/dabc/defines.h @ONLY NEWLINE_STYLE UNIX)

if(APPLE)
  configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/dabclogin.mac.in
                 ${PROJECT_BINARY_DIR}/dabclogin @ONLY NEWLINE_STYLE UNIX)
else()
  configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/dabclogin.linux.in
                 ${PROJECT_BINARY_DIR}/dabclogin @ONLY NEWLINE_STYLE UNIX)
endif()

if(NOT APPLE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    find_library(DABC_cpp_LIBRARY "c++")
  endif()
endif()

configure_file(${PROJECT_SOURCE_DIR}/cmake/modules/DabcMacros.cmake
               ${PROJECT_BINARY_DIR}/DabcMacros.cmake COPYONLY)

configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/DABCUseFile.cmake.in
               ${PROJECT_BINARY_DIR}/DABCUseFile.cmake @ONLY NEWLINE_STYLE UNIX)
