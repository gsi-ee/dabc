set(applications core-test net-test hadaq ib-test)

#if(NOT APPLE)
#  list(APPEND applications ncurses)
#endif()

get_property(_allLibs GLOBAL PROPERTY DABC_LIBRARY_TARGETS)

unset(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
unset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)

foreach(ex ${applications})
  add_subdirectory(${ex})
endforeach()
