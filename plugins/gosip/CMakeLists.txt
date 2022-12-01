find_package(GOSIP)
# QUIET)

if(GOSIP_FOUND)
   DABC_LINK_LIBRARY(DabcGosip
                     LIBRARIES DabcBase DabcMbs ${GOSIP_LIBRARY}
                     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${GOSIP_INCLUDE_DIR})
                     
   

   file(COPY app DESTINATION ${PROJECT_BINARY_DIR}/plugins/gosip)          
                     

   file(COPY htm DESTINATION ${CMAKE_BINARY_DIR}/plugins/gosip)
endif()

DABC_EXECUTABLE(rgoc
                SOURCES rgoc.cxx
                LIBRARIES DabcBase)