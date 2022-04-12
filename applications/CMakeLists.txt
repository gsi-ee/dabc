include(ExternalProject)

set(applications core-test net-test hadaq ib-test)

if(NOT APPLE)
   list(APPEND applications ncurses)
endif()

get_property(_allLibs GLOBAL PROPERTY DABC_LIBRARY_TARGETS)

foreach(ex ${applications})
   ExternalProject_Add(${ex}
                       DEPENDS ${_allLibs}
                       CMAKE_ARGS -DCMAKE_PREFIX_PATH=${PROJECT_BINARY_DIR}
                       SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${ex}
                       BINARY_DIR ${ex}
                       INSTALL_COMMAND "")
endforeach()
