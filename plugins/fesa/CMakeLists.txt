find_package(FESA QUIET)

if(FESA_FOUND)
   set(fesa_def "WITH_FESA")
endif()

DABC_LINK_LIBRARY(DabcFesa
                  DEFINITIONS ${fesa_def}
                  LIBRARIES DabcBase DabcMbs ${FESA_ALL_LIBS}
                  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${FESA_INCLUDE_DIR})

file(COPY example.xml fesa.js DESTINATION ${CMAKE_BINARY_DIR}/plugins/fesa)
