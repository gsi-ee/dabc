if(NOT PROJECT_NAME STREQUAL DABC)
  cmake_minimum_required(VERSION 3.13)
  project(dabc-ncurses)
  find_package(DABC)
endif()

find_package(PkgConfig)

if(PkgConfig_FOUND)
  pkg_check_modules(ncurses ncurses menu QUIET IMPORTED_TARGET)

  if(ncurses_FOUND)
    add_executable(dabc_shell dabc_shell.cxx)
    target_link_libraries(dabc_shell dabc::DabcBase PkgConfig::ncurses)
  endif()
endif()
