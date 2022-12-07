find_package(GOSIP)
# QUIET)

if(GOSIP_FOUND)
   DABC_LINK_LIBRARY(DabcGosip
	LIBRARIES DabcBase ${GOSIP_LIBRARY}
	     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${GOSIP_INCLUDE_DIR})
   #set_property(TARGET DabcGosip APPEND PROPERTY BUILD_RPATH "/mbs/driv/mbspex_5.10-64_DEB/lib/")  
   
#    DABC_LINK_LIBRARY(DabcGosip
#                     LIBRARIES DabcBase
#                     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${GOSIP_INCLUDE_DIR})           
   
   file(COPY app DESTINATION ${PROJECT_BINARY_DIR}/plugins/gosip)          
                     

   file(COPY htm DESTINATION ${CMAKE_BINARY_DIR}/plugins/gosip)
endif()

DABC_EXECUTABLE(rgoc
                SOURCES rgoc.cxx
                LIBRARIES DabcBase)