find_package(DOFI)

if(DOFI_FOUND)
  dabc_link_library(
    DabcDofi
    SOURCES src/Factory.cxx src/Player.cxx src/TerminalModule.cxx
    HEADERS dofi/Factory.h dofi/Player.h dofi/TerminalModule.h dofi/Command.h
    INCDIR dofi
    LIBRARIES dabc::DabcBase )
    #INCLUDES ${DOFI_INCLUDE_DIR})

  dabc_install_plugin_data(
    DabcDofi
    DIRECTORIES app
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/dofi)
endif()

dabc_executable(
  rdoficom
  SOURCES rdoficom.cxx
  LIBRARIES DabcBase)
