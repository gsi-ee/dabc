cmake_minimum_required(VERSION 3.9)
project(dabc-ncurses)
find_package(DABC)
include(${DABC_USE_FILE})

find_library(NCURSES_LIBRARY ncurses)
find_library(MENU_LIBRARY menu)

find_path(NCURSES_INCLUDE_DIR ncurses.h /usr/include/ncurses)
find_path(MENU_INCLUDE_DIR menu.h /usr/include/ncurses)

if(NCURSES_LIBRARY AND MENU_LIBRARY AND NCURSES_INCLUDE_DIR AND MENU_INCLUDE_DIR)
   DABC_EXECUTABLE(dabc_shell
                   SOURCES dabc_shell.cxx
                   LIBRARIES ${DabcBase_LIBRARY} ${NCURSES_LIBRARY} ${MENU_LIBRARY}
                   INCLUDES ${NCURSES_INCLUDE_DIR} ${MENU_INCLUDE_DIR}
                   CHECKSTD)
endif()
