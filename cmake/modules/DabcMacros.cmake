if(APPLE)
  set(libprefix ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(libsuffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
  set(libprefix ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(libsuffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

set(DABC_LIBRARY_PROPERTIES
    SUFFIX ${libsuffix}
    PREFIX ${libprefix}
    IMPORT_PREFIX ${libprefix})

#---------------------------------------------------------------------------------------------------
#---DABC_INSTALL_HEADERS([hdr1 hdr2 ...])
#---------------------------------------------------------------------------------------------------
function(DABC_INSTALL_HEADERS dir)
  file(GLOB headers "${dir}/*.h")
  string(REPLACE ${CMAKE_SOURCE_DIR} "" tgt ${CMAKE_CURRENT_SOURCE_DIR})
  string(MAKE_C_IDENTIFIER move_header${tgt} tgt)
  foreach (include_file ${headers})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" fname ${include_file})
    set (src ${CMAKE_CURRENT_SOURCE_DIR}/${fname})
    set (dst ${PROJECT_BINARY_DIR}/include/${fname})
    add_custom_command(
      OUTPUT ${dst}
      COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst}
      COMMENT "Copying header ${fname} to ${PROJECT_BINARY_DIR}/include"
      DEPENDS ${src})
    list(APPEND dst_list ${dst})
  endforeach()
  add_custom_target(${tgt} DEPENDS ${dst_list})
  set_property(GLOBAL APPEND PROPERTY DABC_HEADER_TARGETS ${tgt})
endfunction()

#---------------------------------------------------------------------------------------------------
#---DABC_LINK_LIBRARY(libname
#                     SOURCES src1 src2          : if not specified, taken "src/*.cxx"
#                     LIBRARIES lib1 lib2        : direct linked libraries
#                     DEFINITIONS def1 def2      : library definitions
#                     DEPENDENCIES dep1 dep2     : dependencies
#                     INCLUDES dir1 dir2         : include directories
#                     NOWARN                     : disable warnings for DABC libs
#)
function(DABC_LINK_LIBRARY libname)
   cmake_parse_arguments(ARG "NOWARN" "" "SOURCES;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES" ${ARGN})

   if(NOT ARG_SOURCES)
      file(GLOB ARG_SOURCES "src/*.cxx")
   endif()

   add_library(${libname} SHARED ${ARG_SOURCES})

   set_target_properties(${libname} PROPERTIES ${DABC_LIBRARY_PROPERTIES})

#   if(NOT CMAKE_CXX_STANDARD)
#     set_property(TARGET ${libname} PROPERTY CXX_STANDARD 11)
#   endif()

   target_compile_definitions(${libname} PRIVATE ${ARG_DEFINITIONS} ${DABC_DEFINES})

   target_link_libraries(${libname} ${ARG_LIBRARIES})

   if(CMAKE_PROJECT_NAME STREQUAL DABC)
     list(APPEND ARG_DEPENDENCIES move_headers)
     set(_main_incl ${CMAKE_BINARY_DIR}/include)
     set_property(GLOBAL APPEND PROPERTY DABC_LIBRARY_TARGETS ${libname})
     if(NOT ARG_NOWARN)
        target_compile_options(${libname} PRIVATE -Wall)
     endif()
   else()
     set(_main_incl ${DABC_INCLUDE_DIR})
   endif()

   target_include_directories(${libname} PRIVATE ${_main_incl} ${ARG_INCLUDES})

  if(ARG_DEPENDENCIES)
     add_dependencies(${libname} ${ARG_DEPENDENCIES})
  endif()

endfunction()


#---------------------------------------------------------------------------------------------------
#---DABC_EXECUTABLE(exename
#                   SOURCES src1 src2          :
#                   LIBRARIES lib1 lib2        : direct linked libraries
#                   DEFINITIONS def1 def2      : library definitions
#                   DEPENDENCIES dep1 dep2     : dependencies
#
#)
function(DABC_EXECUTABLE exename)
   cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES" ${ARGN})

   add_executable(${exename} ${ARG_SOURCES})

   target_compile_definitions(${exename} PRIVATE ${ARG_DEFINITIONS} ${DABC_DEFINES})

   target_link_libraries(${exename} ${ARG_LIBRARIES})

   if(CMAKE_PROJECT_NAME STREQUAL DABC)
     list(APPEND ARG_DEPENDENCIES move_headers)
     set(_main_incl ${CMAKE_BINARY_DIR}/include)
     target_compile_options(${exename} PRIVATE -Wall)
  else()
     set(_main_incl ${DABC_INCLUDE_DIR})
  endif()

   target_include_directories(${exename} PRIVATE ${_main_incl} ${ARG_INCLUDES})

  if(ARG_DEPENDENCIES)
     add_dependencies(${exename} ${ARG_DEPENDENCIES})
  endif()

endfunction()

