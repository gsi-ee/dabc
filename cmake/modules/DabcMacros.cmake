if(APPLE)
  set(libprefix ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(libsuffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
  set(libprefix ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(libsuffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

set(DABC_LIBRARY_PROPERTIES SUFFIX ${libsuffix} PREFIX ${libprefix}
                            IMPORT_PREFIX ${libprefix})

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_INSTALL_HEADERS(target
#                        INCLUDE_SUBDIR dir   : place headers in extra subdirectory 'dir'
#                        HEADERS hdr1 hdr2    : public headers (to be installed)
#)
# This function install HEADERS for target
#---------------------------------------------------------------------------------------------------
# cmake-format: on
function(DABC_INSTALL_HEADERS target)
  cmake_parse_arguments(ARG "" "INCLUDE_SUBDIR" "HEADERS;" ${ARGN})

  string(MAKE_C_IDENTIFIER copy_header_${ARG_INCLUDE_SUBDIR} tgt)
  foreach(include_file ${ARG_HEADERS})
    get_filename_component(src ${include_file} ABSOLUTE BASE_DIR
                           "${CMAKE_CURRENT_SOURCE_DIR}")
    set(dst ${PROJECT_BINARY_DIR}/include/${include_file})
    add_custom_command(
      OUTPUT ${dst}
      COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst}
      COMMENT "Copying header ${include_file} to ${dst}"
      MAIN_DEPENDENCY ${src})
    list(APPEND dst_list ${dst})
  endforeach()
  add_custom_target(${tgt} DEPENDS ${dst_list})
  add_dependencies(${target} ${tgt})
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

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_LINK_LIBRARY(libname
#                     SOURCES src1 src2          : if not specified, taken "src/*.cxx"
#                     HEADERS hdr1 hdr2          : public headers (to be installed)
#                     PRIVATE_HEADERS hdr1 hdr2  : private headers (not to be installed)
#                     INCLUDE_SUBDIR dir         : subdir for includes
#                     LIBRARIES lib1 lib2        : direct linked libraries
#                     DEFINITIONS def1 def2      : library definitions
#                     DEPENDENCIES dep1 dep2     : dependencies
#                     INCLUDES dir1 dir2         : include directories
#                     NOWARN                     : disable warnings for DABC libs
#)
# cmake-format: on
function(DABC_LINK_LIBRARY libname)
  cmake_parse_arguments(
    ARG
    "NOWARN"
    "INCLUDE_SUBDIR"
    "SOURCES;HEADERS;PRIVATE_HEADERS;LIBRARIES;DEFINITIONS;DEPENDENCIES;INCLUDES"
    ${ARGN})

  add_library(${libname} SHARED ${ARG_SOURCES})
  add_library(dabc::${libname} ALIAS ${libname})

  set_target_properties(${libname} PROPERTIES ${DABC_LIBRARY_PROPERTIES}
                                              PUBLIC_HEADER "${ARG_HEADERS}")

  target_compile_definitions(${libname} PRIVATE ${ARG_DEFINITIONS}
                                                ${DABC_DEFINES})

  target_link_libraries(${libname} ${ARG_LIBRARIES})

  if(CMAKE_PROJECT_NAME STREQUAL DABC)
    set_property(GLOBAL APPEND PROPERTY DABC_LIBRARY_TARGETS ${libname})
    if(NOT ARG_NOWARN)
      target_compile_options(
        ${libname} PRIVATE -Wall $<$<CXX_COMPILER_ID:GNU>:-Wsuggest-override>)
    endif()

    set_property(GLOBAL APPEND PROPERTY DABC_INSTALL_LIBRARY_TARGETS ${libname})
    install(
      TARGETS ${libname}
      EXPORT ${CMAKE_PROJECT_NAME}Targets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${ARG_INCLUDE_SUBDIR})
  endif()

  target_include_directories(
    ${libname}
    PUBLIC $<INSTALL_INTERFACE:include>
           $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/inc>
           $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE ${ARG_INCLUDES})

  if(DABC_LEGACY_BUILD)
    dabc_install_headers(${libname} HEADERS ${ARG_HEADERS} INCLUDE_SUBDIR
                         ${ARG_INCLUDE_SUBDIR})
  endif()
endfunction()

# cmake-format: off
#---------------------------------------------------------------------------------------------------
#---DABC_EXECUTABLE(exename
#                   SOURCES src1 src2          :
#                   LIBRARIES lib1 lib2        : direct linked libraries
#                   DEFINITIONS def1 def2      : library definitions
#                   DEPENDENCIES dep1 dep2     : dependencies
#                   CHECKSTD                   : check if libc++ should be linked for clang
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

  if(CMAKE_PROJECT_NAME STREQUAL DABC)
    target_compile_options(${exename} PRIVATE -Wall)
  endif()

  target_include_directories(
    ${exename}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/inc>
    PRIVATE ${ARG_INCLUDES})

  if(ARG_DEPENDENCIES)
    add_dependencies(${exename} ${ARG_DEPENDENCIES})
  endif()

  install(TARGETS ${exename} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

endfunction()
