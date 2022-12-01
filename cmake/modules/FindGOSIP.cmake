# Find GOSIP library and includes

set(GOSIP_FOUND 1)

# JAM22 todo: here look for mbspex libs when on x86l node
if(NOT "$ENV{GOSIPDIR}" STREQUAL "")
   find_library(GOSIP_LIBRARY NAMES gosip PATHS
      $ENV{GOSIPDIR}/linux
      DOC "Searching GOSIP library"
   )
   
   if (GOSIP_LIBRARY)
      set(GOSIP_FOUND 1)
      set(GOSIP_INCLUDE_DIR $ENV{GOSIPDIR}/include)
      message(STATUS "Found GOSIP includes ${GOSIP_INCLUDE_DIR} and library ${GOSIP_LIBRARY}")
   endif()
endif()

mark_as_advanced(GOSIP_FOUND GOSIP_LIBRARY GOSIP_INCLUDE_DIR)

