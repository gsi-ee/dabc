find_package(GOSIP QUIET)

if(GOSIP_FOUND)
   DABC_LINK_LIBRARY(DabcGosip
                     LIBRARIES DabcBase DabcMbs ${GOSIP_LIBRARY}
                     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${GOSIP_INCLUDE_DIR})

   file(COPY htm DESTINATION ${CMAKE_BINARY_DIR}/plugins/gosip)
endif()
