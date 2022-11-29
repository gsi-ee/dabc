DABC_LINK_LIBRARY(DabcDogma
                  LIBRARIES DabcBase
                  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

DABC_EXECUTABLE(dogmacmd
                SOURCES dogmacmd.cxx
                LIBRARIES DabcBase)

file(COPY app DESTINATION ${PROJECT_BINARY_DIR}/plugins/dogma)
