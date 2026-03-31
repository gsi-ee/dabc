find_package(PEX)
# QUIET)


if(PEX_FOUND)
  dabc_link_library(
    DabcPex
    SOURCES src/Factory.cxx src/Device.cxx src/Input.cxx  src/Transport.cxx src/Player.cxx src/ReadoutModule.cxx src/FrontendBoard.cxx src/Febex3.cxx src/Poland.cxx src/Tamex.cxx
    HEADERS pex/Factory.h pex/Device.h pex/Input.h  pex/Transport.h pex/Player.h pex/ReadoutModule.h pex/FrontendBoard.h pex/Poland.h pex/Febex3.h pex/Tamex.h
    INCDIR pex
    LIBRARIES DabcBase DabcMbs ${PEX_LIBRARY}
    INCLUDES ${PEX_INCLUDE_DIR}
    COPY_HEADERS)

  dabc_install_plugin_data(
    DabcPex
    DIRECTORIES htm app
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/pex)
endif()

