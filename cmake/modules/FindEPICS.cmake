# Find EPICS library and includes

set(EPICS_FOUND 0)

if(NOT "$ENV{EPICS_BASE}" STREQUAL "" AND NOT "$ENV{EPICS_EXTENSIONS}" STREQUAL "")
   find_library(EPICS_LIBRARY NAMES ezca PATHS
      $ENV{EPICS_EXTENSIONS}/lib/linux-x86_64
      DOC "Searching EPICS library"
   )
   
   if (EPICS_LIBRARY)
      set(EPICS_FOUND 1)
      set(EPICS_INCLUDE_DIR $ENV{EPICS_BASE}/include $ENV{EPICS_BASE}/include/os/Linux $ENV{EPICS_EXTENSIONS}/include)
      message(STATUS "Found EPICS includes ${EPICS_INCLUDE_DIR} and library ${EPICS_LIBRARY}")
   endif()
endif()

mark_as_advanced(EPICS_FOUND EPICS_LIBRARY EPICS_INCLUDE_DIR)

