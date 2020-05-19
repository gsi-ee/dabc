# Find FESA library and includes

set(FESA_FOUND 0)

if(NOT "$ENV{RDA_HOME}" STREQUAL "")
   find_library(FESA_LIB1 NAMES omniORB4 PATHS
      $ENV{RDA_HOME}/lib/c86_64
      DOC "Searching FESA library"
   )

   find_library(FESA_LIB2 NAMES omnithread PATHS
      $ENV{RDA_HOME}/lib/c86_64
      DOC "Searching FESA library"
   )
   
   if (FESA_LIB1 AND FESA_LIB2)
      set(FESA_FOUND 1)
      set(FESA_INCLUDE_DIR  $ENV{RDA_HOME}/include)
      set(FESA_ALL_LIBS ${FESA_LIB1} ${FESA_LIB2}
                   crypto curl pthread
                   $ENV{RDA_HOME}/lib/x86_64/libcmw-rda.a
                   $ENV{RDA_HOME}/lib/x86_64/libcmw-rbac.a
                   $ENV{RDA_HOME}/lib/x86_64/libcmw-directory-client.a
                   $ENV{RDA_HOME}/lib/x86_64/libcmw-serializer.a
                   $ENV{RDA_HOME}/lib/x86_64/libcmw-log-stomp.a
                   $ENV{RDA_HOME}/lib/x86_64/libcmw-stomp.a
                   $ENV{RDA_HOME}/lib/x86_64/libcmw-log.a
                   $ENV{RDA_HOME}/lib/x86_64/libcmw-util.a
                   $ENV{RDA_HOME}/lib/x86_64/libiceutil.a)
      message(STATUS "Found FESA includes ${FESA_INCLUDE_DIR} and library ${FESA_LIB1}")
   endif()
endif()

mark_as_advanced(FESA_FOUND FESA_INCLUDE_DIR FESA_ALL_LIBS)

