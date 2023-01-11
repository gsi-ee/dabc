find_package(GOSIP)
# QUIET)

# JAM: remove commments of DEFINITIONS to test old plain C mode (direct copy of
# gosipcmd code)

if(GOSIP_FOUND)
  dabc_link_library(
    DabcGosip
    SOURCES src/Factory.cxx src/Player.cxx
    HEADERS gosip/Factory.h gosip/Player.h
    INCDIR gosip
    LIBRARIES dabc::DabcBase dabc::DabcMbs ${GOSIP_LIBRARY}
    INCLUDES ${GOSIP_INCLUDE_DIR})

  dabc_install_plugin_data(
    DabcGosip
    FILES htm
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/gosip)
endif()

dabc_executable(
  rgoc
  # DEFINITIONS GOSIP_COMMAND_PLAINC
  SOURCES rgoc.cxx
  LIBRARIES DabcBase)
