include(ExternalProject)

set(applications core-test net-test hadaq ib-test)

if(NOT APPLE)
  list(APPEND applications ncurses)
endif()

get_property(_allLibs GLOBAL PROPERTY DABC_LIBRARY_TARGETS)

foreach(ex ${applications})
  add_subdirectory(${ex})
endforeach()
