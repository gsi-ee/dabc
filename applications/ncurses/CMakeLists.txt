cmake_minimum_required(VERSION 3.9)
project(dabc-ncurses)
find_package(DABC)
include(${DABC_USE_FILE})

find_package(PkgConfig REQUIRED)
pkg_check_modules(ncurses REQUIRED ncurses menu IMPORTED_TARGET)

DABC_EXECUTABLE(dabc_shell
                SOURCES dabc_shell.cxx
                LIBRARIES ${DabcBase_LIBRARY} PkgConfig::ncurses
                CHECKSTD)
