find_package(SAFT QUIET)

if(SAFT_FOUND)
  dabc_link_library(
    DabcSaft
    SOURCES src/Device.cxx src/Factory.cxx src/Input.cxx
    HEADERS saftdabc/Definitions.h saftdabc/Device.h saftdabc/Factory.h
            saftdabc/Input.h
    INCLUDE_SUBDIR saftdabc
    LIBRARIES dabc::DabcBase dabc::DabcMbs ${SAFT_LIBRARY}
    INCLUDES ${SAFT_INCLUDE_DIR})
endif()
