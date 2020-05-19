find_package(FESA QUIET)

if(FESA_FOUND)
   DABC_LINK_LIBRARY(DabcFesa
                     LIBRARIES DabcBase DabcMbs ${FESA_ALL_LIBS}
                     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${FESA_INCLUDE_DIR})
endif()
