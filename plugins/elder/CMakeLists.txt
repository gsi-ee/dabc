find_package(ELDER QUIET)

if(ELDER_FOUND)
  dabc_link_library(
    DabcElder
    SOURCES src/RunModule.cxx src/Factory.cxx src/VisConDabc.cxx
    HEADERS elderdabc/RunModule.h elderdabc/VisConDabc.h elderdabc/Factory.h
    INCDIR elderdabc
    LIBRARIES dabc::DabcBase dabc::DabcMbs ${ELDER_LIBRARY} ${ELDER-STD_LIBRARY} ${ELDER-GSI_LIBRARY} ${ELDER-EEL_LIBRARY} 
    INCLUDES ${ELDER_INCLUDE_DIR}
    )
    
    dabc_install_plugin_data(
    DabcElder
    DIRECTORIES icons app htm
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/elder
    )
    
endif()
