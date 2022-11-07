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
#---DABC_LINK_LIBRARY(libname
#                     SOURCES src1 src2          : if not specified, taken "src/*.cxx"
#                     HEADERS hdr1 hdr2          : public headers 9to be installed)
#                     PRIVATE_HEADERS hdr1 hdr2  : private headers (not to be installed)
#                     INCLUDE_SUBDIR dir         : subdir for includes
#                     LIBRARIES lib1 lib2        : direct linked libraries
#                     DEFINITIONS def1 def2      : library definitions
#                     DEPENDENCIES dep1 dep2     : dependencies
#                     INCLUDES dir1 dir2         : include directories
#                     NOWARN                     : disable warnings for DABC libs
#                     DABC_INSTALL               : install if part of DABC project
#)
function(DABC_LINK_LIBRARY libname)
   cmake_parse_arguments(ARG "NOWARN;DABC_INSTALL" "" "SOURCES;HEADERS;PRIVATE_HEADERS;INCLUDE_SUBDIR;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES" ${ARGN})

   add_library(${libname} SHARED ${ARG_SOURCES})
   add_library(dabc::${libname} ALIAS ${libname})

   set_target_properties(${libname} PROPERTIES ${DABC_LIBRARY_PROPERTIES}
   PUBLIC_HEADER
            "${ARG_HEADERS}")

   target_compile_definitions(${libname} PRIVATE ${ARG_DEFINITIONS} ${DABC_DEFINES})

   target_link_libraries(${libname} ${ARG_LIBRARIES})

   if(CMAKE_PROJECT_NAME STREQUAL DABC)
     set_property(GLOBAL APPEND PROPERTY DABC_LIBRARY_TARGETS ${libname})
     if(NOT ARG_NOWARN)
        target_compile_options(${libname} PRIVATE -Wall $<$<CXX_COMPILER_ID:GNU>:-Wsuggest-override>)
     endif()
   endif()

   if (ARG_DABC_INSTALL AND CMAKE_PROJECT_NAME STREQUAL DABC)
     set_property(GLOBAL APPEND PROPERTY DABC_INSTALL_LIBRARY_TARGETS ${libname})
     install(TARGETS ${libname}
        EXPORT ${CMAKE_PROJECT_NAME}Targets
        LIBRARY
           DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER
           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${ARG_INCLUDE_SUBDIR})
   endif()

   target_include_directories(${libname}
      PUBLIC
         $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      PRIVATE
         ${ARG_INCLUDES}
   )
endfunction()


#---------------------------------------------------------------------------------------------------
#---DABC_EXECUTABLE(exename
#                   SOURCES src1 src2          :
#                   LIBRARIES lib1 lib2        : direct linked libraries
#                   DEFINITIONS def1 def2      : library definitions
#                   DEPENDENCIES dep1 dep2     : dependencies
#                   CHECKSTD                   : check if libc++ should be linked for clang
#
#)
function(DABC_EXECUTABLE exename)
   cmake_parse_arguments(ARG "CHECKSTD" "" "SOURCES;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES" ${ARGN})

   add_executable(${exename} ${ARG_SOURCES})

   target_compile_definitions(${exename} PRIVATE ${ARG_DEFINITIONS} ${DABC_DEFINES})

   if(NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL Clang AND ARG_CHECKSTD)
      find_library(cpp_LIBRARY "c++")
   endif()

   target_link_libraries(${exename} ${ARG_LIBRARIES} ${cpp_LIBRARY})

   if(CMAKE_PROJECT_NAME STREQUAL DABC)
     target_compile_options(${exename} PRIVATE -Wall)
  endif()

   target_include_directories(${exename} PRIVATE ${ARG_INCLUDES})

  if(ARG_DEPENDENCIES)
     add_dependencies(${exename} ${ARG_DEPENDENCIES})
  endif()

endfunction()

