find_package(EPICS QUIET)

if(EPICS_FOUND)
   DABC_LINK_LIBRARY(DabcEzca
                     LIBRARIES DabcBase DabcMbs ${EPICS_LIBRARY}
                     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${EPICS_INCLUDE_DIR})
endif()
