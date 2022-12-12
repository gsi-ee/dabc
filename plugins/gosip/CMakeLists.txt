find_package(GOSIP)
# QUIET)

# JAM: remove commments of DEFINITIONS to test old plain C mode
# (direct copy of gosipcmd code)

if(GOSIP_FOUND)
   DABC_LINK_LIBRARY(DabcGosip
	LIBRARIES DabcBase ${GOSIP_LIBRARY}
	     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${GOSIP_INCLUDE_DIR}
              #DEFINITIONS GOSIP_COMMAND_PLAINC
              )
   file(COPY app DESTINATION ${PROJECT_BINARY_DIR}/plugins/gosip)          
                     

   file(COPY htm DESTINATION ${CMAKE_BINARY_DIR}/plugins/gosip)
endif()

DABC_EXECUTABLE(rgoc
#			 DEFINITIONS GOSIP_COMMAND_PLAINC
                SOURCES rgoc.cxx               
                LIBRARIES DabcBase)