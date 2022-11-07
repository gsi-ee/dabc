find_package(FESA QUIET)

if(FESA_FOUND)
  set(fesa_def "WITH_FESA")
endif()

dabc_link_library(
  DabcFesa
  SOURCES src/Factory.cxx src/Monitor.cxx src/Player.cxx
  HEADERS fesa/Factory.h fesa/Monitor.h fesa/Player.h
  INCLUDE_SUBDIR fesa
  DEFINITIONS ${fesa_def}
  LIBRARIES dabc::DabcBase dabc::DabcMbs ${FESA_ALL_LIBS}
  INCLUDES ${FESA_INCLUDE_DIR})

file(COPY example.xml fesa.js DESTINATION ${CMAKE_BINARY_DIR}/plugins/fesa)
