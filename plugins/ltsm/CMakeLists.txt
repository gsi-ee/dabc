find_package(LTSM)

if(LTSM_FOUND)
  dabc_link_library(
    DabcLtsm
    SOURCES src/Factory.cxx src/FileInterface.cxx
    HEADERS ltsm/Factory.h ltsm/FileInterface.h
    INCDIR ltsm
    LIBRARIES dabc::DabcBase ${LTSM_LIBRARY} ${FSQ_LIBRARY}
    INCLUDES ${LTSM_INCLUDE_DIR} ${LTSM_INCLUDE_DIR} ${TSM_INCLUDE_DIR}
    DEFINITIONS SYSTEM64 HAVE_CONFIG_H)

  dabc_install_plugin_data(
    DabcLtsm
    DIRECTORIES app
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/ltsm)
 endif()
