find_package(SAFT QUIET)

if(SAFT_FOUND)
   DABC_LINK_LIBRARY(DabcSaft
                     LIBRARIES DabcBase DabcMbs ${SAFT_LIBRARY}
                     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${SAFT_INCLUDE_DIR})
endif()
