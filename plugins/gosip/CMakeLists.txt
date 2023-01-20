find_package(GOSIP)
# QUIET)


if(GOSIP_FOUND)
  dabc_link_library(
    DabcGosip
    SOURCES src/Factory.cxx src/Player.cxx src/TerminalModule.cxx
    HEADERS gosip/Factory.h gosip/Player.h gosip/TerminalModule.h gosip/Command.h
    INCDIR gosip
    LIBRARIES dabc::DabcBase dabc::DabcMbs ${GOSIP_LIBRARY}
    INCLUDES ${GOSIP_INCLUDE_DIR})

  dabc_install_plugin_data(
    DabcGosip
    DIRECTORIES htm app
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/gosip)
endif()

dabc_executable(
  rgoc
  SOURCES rgoc.cxx
  LIBRARIES DabcBase)
