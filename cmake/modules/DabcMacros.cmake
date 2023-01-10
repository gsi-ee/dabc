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

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_INSTALL_HEADERS([hdr1 hdr2 ...])
#---------------------------------------------------------------------------------------------------
# cmake-format: on
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

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_LINK_LIBRARY(libname                    : library name
#                     INCDIR dir                 : library's include dir
#                     SOURCES src1 src2          : if not specified, taken "src/*.cxx"
#                     HEADERS hdr1 hdr2          : public headers 9to be installed)
#                     PRIVATE_HEADERS hdr1 hdr2  : private headers (not to be installed)
#                     LIBRARIES lib1 lib2        : direct linked libraries
#                     DEFINITIONS def1 def2      : library definitions
#                     DEPENDENCIES dep1 dep2     : dependencies
#                     INCLUDES dir1 dir2         : include directories
#                     NOWARN                     : disable warnings for DABC libs
#)
# cmake-format: on
function(DABC_LINK_LIBRARY libname)
   cmake_parse_arguments(ARG "NOWARN;DABC_INSTALL" "INCDIR" "SOURCES;HEADERS;PRIVATE_HEADERS;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES" ${ARGN})

   add_library(${libname} SHARED ${ARG_SOURCES})
   add_library(dabc::${libname} ALIAS ${libname})

   set_target_properties(${libname} PROPERTIES ${DABC_LIBRARY_PROPERTIES}
       PUBLIC_HEADER
            "${ARG_HEADERS}")

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
        target_compile_options(${libname} PRIVATE -Wall $<$<CXX_COMPILER_ID:GNU>:-Wsuggest-override>)
     endif()
   else()
     set(_main_incl ${DABC_INCLUDE_DIR})
   endif()

   target_include_directories(${libname} PRIVATE ${_main_incl} ${ARG_INCLUDES})

  if(ARG_DEPENDENCIES)
     add_dependencies(${libname} ${ARG_DEPENDENCIES})
  endif()

  set_property(GLOBAL APPEND PROPERTY DABC_INSTALL_LIBRARY_TARGETS ${libname})
     install(TARGETS ${libname}
        EXPORT ${CMAKE_PROJECT_NAME}Targets
        LIBRARY
           DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER
           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dabc/${ARG_INCDIR})

endfunction()


# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_EXECUTABLE(exename
#                   SOURCES src1 src2          :
#                   LIBRARIES lib1 lib2        : direct linked libraries
#                   DEFINITIONS def1 def2      : library definitions
#                   DEPENDENCIES dep1 dep2     : dependencies
#                   CHECKSTD                   : check if libc++ should be linked for clang
#
#)
# cmake-format: on
function(DABC_EXECUTABLE exename)
   cmake_parse_arguments(ARG "CHECKSTD" "" "SOURCES;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES" ${ARGN})

   add_executable(${exename} ${ARG_SOURCES})

   target_compile_definitions(${exename} PRIVATE ${ARG_DEFINITIONS} ${DABC_DEFINES})

   if(NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL Clang AND ARG_CHECKSTD)
      find_library(cpp_LIBRARY "c++")
   endif()

   target_link_libraries(${exename} ${ARG_LIBRARIES} ${cpp_LIBRARY})

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

  install(TARGETS ${exename} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_INSTALL_PLUGIN_DATA(target
#                            LEGACY_MODE yes/no          : if YES, place file also in CMAKE_BINARY_DIR/plugins
#                            DESTINATION                 : installation location
#                            FILES file1 [file2...]      : files to install
#                            DIRECTORIES dir1 [dir2...]  : directories to install
#)
# This function install extra fiels and eventualy copies the mto CMAKE_BINARY_DIR for LEGACY_MODE.
#---------------------------------------------------------------------------------------------------
# cmake-format: on
function(DABC_INSTALL_PLUGIN_DATA target)
  cmake_parse_arguments(ARG "" "LEGACY_MODE;DESTINATION" "FILES;DIRECTORIES"
                        ${ARGN})

  get_filename_component(BASENAME ${ARG_DESTINATION} NAME)

  set(ARG_LEGACY_MODE YES) # force legacy mode

  if(DEFINED ARG_FILES)
    install(FILES ${ARG_FILES} DESTINATION ${ARG_DESTINATION})
    if(${ARG_LEGACY_MODE})
      dabc_install_plugin_data_source(${target} ${ARG_FILES} DESTINATION
                                      ${CMAKE_BINARY_DIR}/plugins/${BASENAME})
    endif()
  endif()

  if(DEFINED ARG_DIRECTORIES)
    install(DIRECTORY ${ARG_DIRECTORIES} DESTINATION ${ARG_DESTINATION})
    if(${ARG_LEGACY_MODE})
      dabc_install_plugin_data_source(${target} ${ARG_DIRECTORIES} DESTINATION
                                      ${CMAKE_BINARY_DIR}/plugins/${BASENAME})
    endif()
  endif()

endfunction()

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_INSTALL_PLUGIN_DATA_SOURCE(src1 [src2...]  : files or directories to copy
#                                   DESTINATION     : installation location
#)
# This function creates targets to copy files into CMAKE_BINARY_DIR for legacy mode
#---------------------------------------------------------------------------------------------------
# cmake-format: on
function(DABC_INSTALL_PLUGIN_DATA_SOURCE target)
  cmake_parse_arguments(ARG "" "DESTINATION" "" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    string(MAKE_C_IDENTIFIER copy_plugin_${BASENAME} tgt)

    foreach(plugin_file ${ARG_UNPARSED_ARGUMENTS})
      get_filename_component(src ${plugin_file} ABSOLUTE BASE_DIR
                             "${CMAKE_CURRENT_SOURCE_DIR}")
      if(IS_DIRECTORY ${src})
        set(COPY_ACTION "copy_directory")
      else()
        set(COPY_ACTION "copy")
      endif()
      set(dst ${ARG_DESTINATION}/${plugin_file})
      add_custom_command(
        OUTPUT ${dst}
        COMMAND ${CMAKE_COMMAND} -E ${COPY_ACTION} ${src} ${dst}
        COMMENT "Copying plugin ${plugin_file} to ${dst}"
        MAIN_DEPENDENCY ${src})
      list(APPEND dst_list ${dst})
      add_custom_target(${tgt}_${plugin_file} DEPENDS ${dst_list})
      add_dependencies(${target} ${tgt}_${plugin_file})
    endforeach()
  endif()
endfunction()
