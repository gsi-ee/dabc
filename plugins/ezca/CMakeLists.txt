find_package(EPICS QUIET)

if(EPICS_FOUND)
dabc_link_library(
    DabcEzca
    SOURCES src/EpicsInput.cxx src/Factory.cxx src/Monitor.cxx
    HEADERS ezca/Definitions.h ezca/EpicsInput.h ezca/Factory.h ezca/Monitor.h
    INCDIR ezca
    LIBRARIES dabc::DabcBase dabc::DabcMbs ${EPICS_LIBRARY}
    INCLUDES ${EPICS_INCLUDE_DIR})
endif()
