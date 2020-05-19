# Find DIM library and includes

set(DIM_FOUND 0)

if(NOT "$ENV{DIMDIR}" STREQUAL "")
   find_library(DIM_LIBRARY NAMES dim PATHS
      $ENV{DIMDIR}/linux
      DOC "Searching DIM library"
   )
   
   if (DIM_LIBRARY)
      set(DIM_FOUND 1)
      set(DIM_INCLUDE_DIR $ENV{DIMDIR}/dim)
      message(STATUS "Found DIM includes ${DIM_INCLUDE_DIR} and library ${DIM_LIBRARY}")
   endif()
endif()

mark_as_advanced(DIM_FOUND DIM_LIBRARY DIM_INCLUDE_DIR)

