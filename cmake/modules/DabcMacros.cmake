if(APPLE)
  set(libprefix ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(libsuffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
  set(libprefix ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(libsuffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

set(DABC_LIBRARY_PROPERTIES SUFFIX ${libsuffix} PREFIX ${libprefix}
                            IMPORT_PREFIX ${libprefix})

# let use MAIN_DEPENDENCY with newer cmake
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.31.0)
  cmake_policy(SET CMP0175 OLD)
endif()

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_INSTALL_HEADERS([hdr1 hdr2 ...])
#---------------------------------------------------------------------------------------------------
# cmake-format: on
function(DABC_INSTALL_HEADERS target)
  cmake_parse_arguments(ARG "" "INCDIR" "HEADERS;" ${ARGN})

  set(dst_list)

  string(MAKE_C_IDENTIFIER copy_header_${ARG_INCDIR} tgt)
  foreach(include_file ${ARG_HEADERS})
    get_filename_component(src ${include_file} ABSOLUTE BASE_DIR
                           "${CMAKE_CURRENT_SOURCE_DIR}")
    set(dst ${PROJECT_BINARY_DIR}/include/${include_file})
    add_custom_command(
      OUTPUT ${dst}
      COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst}
      MAIN_DEPENDENCY ${src}
      COMMENT "Copying header ${include_file} to ${dst}")
    list(APPEND dst_list ${dst})
  endforeach()
  add_custom_target(${tgt} DEPENDS ${dst_list})
  set_property(GLOBAL APPEND PROPERTY DABC_HEADER_TARGETS ${tgt})
  add_dependencies(${target} ${tgt})
endfunction()

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_LINK_LIBRARY(libname                    : library name
#                     INCDIR dir                 : library's include dir
#                     SOURCES src1 src2          : if not specified, taken "src/*.cxx"
#                     HEADERS hdr1 hdr2          : public headers 9to be installed)
#                     PRIVATE_HEADERS hdr1 hdr2  : private headers (not to be installed)
#                     EXTRA_HEADERS hdr1 hdr2    : extra headers (not source, e.g. generated, to be installed)
#                     LIBRARIES lib1 lib2        : direct linked libraries
#                     DEFINITIONS def1 def2      : library definitions
#                     DEPENDENCIES dep1 dep2     : dependencies
#                     INCLUDES dir1 dir2         : include directories
#                     NOWARN                     : disable warnings for DABC libs
#                     COPY_HEADERS               : copy headers to build tree
#)
# cmake-format: on
function(DABC_LINK_LIBRARY libname)
  cmake_parse_arguments(
    ARG
    "NOWARN;COPY_HEADERS"
    "INCDIR"
    "SOURCES;HEADERS;PRIVATE_HEADERS;EXTRA_HEADERS;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES"
    ${ARGN})

  add_library(${libname} SHARED ${ARG_SOURCES})
  add_library(dabc::${libname} ALIAS ${libname})

  if(${ARG_COPY_HEADERS})
    dabc_install_headers(${libname} HEADERS ${ARG_HEADERS} INCDIR ${ARG_INCDIR})
  endif()

  set_target_properties(
    ${libname} PROPERTIES ${DABC_LIBRARY_PROPERTIES} PUBLIC_HEADER
                          "${ARG_HEADERS};${ARG_EXTRA_HEADERS}")

  target_compile_definitions(${libname} PRIVATE ${ARG_DEFINITIONS}
                                                ${DABC_DEFINES})

  target_link_libraries(${libname} ${ARG_LIBRARIES})

  if(PROJECT_NAME STREQUAL DABC)
    list(APPEND ARG_DEPENDENCIES copy_headers)
    set(_main_incl ${PROJECT_BINARY_DIR}/include)
    set_property(GLOBAL APPEND PROPERTY DABC_LIBRARY_TARGETS ${libname})
    if(NOT ARG_NOWARN)
       target_compile_options(${libname} PRIVATE -Wall $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wsuggest-override>>)
    endif()
  else()
    set(_main_incl ${DABC_INCLUDE_DIR})
  endif()

  target_include_directories(
    ${libname}
    PUBLIC $<INSTALL_INTERFACE:include>
           $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE ${_main_incl} ${ARG_INCLUDES})

  if(ARG_DEPENDENCIES)
    add_dependencies(${libname} ${ARG_DEPENDENCIES})
  endif()

  set_property(GLOBAL APPEND PROPERTY DABC_INSTALL_LIBRARY_TARGETS ${libname})
  install(
    TARGETS ${libname}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${DABC_INSTALL_INCLUDEDIR}/${ARG_INCDIR})

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
  cmake_parse_arguments(
    ARG "CHECKSTD" "" "SOURCES;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES"
    ${ARGN})

  add_executable(${exename} ${ARG_SOURCES})

  target_compile_definitions(${exename} PRIVATE ${ARG_DEFINITIONS}
                                                ${DABC_DEFINES})

  if(NOT APPLE
     AND CMAKE_CXX_COMPILER_ID STREQUAL Clang
     AND ARG_CHECKSTD)
    find_library(cpp_LIBRARY "c++")
  endif()

  target_link_libraries(${exename} ${ARG_LIBRARIES} ${cpp_LIBRARY})

  if(PROJECT_NAME STREQUAL DABC)
    list(APPEND ARG_DEPENDENCIES copy_headers)
    set(_main_incl ${PROJECT_BINARY_DIR}/include)
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
#                            LEGACY_MODE yes/no          : if YES, place file also in PROJECT_BINARY_DIR/plugins
#                            DESTINATION                 : installation location
#                            FILES file1 [file2...]      : files to install
#                            DIRECTORIES dir1 [dir2...]  : directories to install
#)
# This function install extra fiels and eventualy copies the mto PROJECT_BINARY_DIR for LEGACY_MODE.
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
                                      ${PROJECT_BINARY_DIR}/plugins/${BASENAME})
    endif()
  endif()

  if(DEFINED ARG_DIRECTORIES)
    install(DIRECTORY ${ARG_DIRECTORIES} DESTINATION ${ARG_DESTINATION})
    if(${ARG_LEGACY_MODE})
      dabc_install_plugin_data_source(${target} ${ARG_DIRECTORIES} DESTINATION
                                      ${PROJECT_BINARY_DIR}/plugins/${BASENAME})
    endif()
  endif()

endfunction()

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_INSTALL_PLUGIN_DATA_SOURCE(src1 [src2...]  : files or directories to copy
#                                   DESTINATION     : installation location
#)
# This function creates targets to copy files into PROJECT_BINARY_DIR for legacy mode
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
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E ${COPY_ACTION} ${src} ${dst}
        MAIN_DEPENDENCY ${src}
        COMMENT "Copying plugin ${plugin_file} to ${dst}")

    endforeach()
  endif()
endfunction()
