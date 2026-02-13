find_package(PEX)
# QUIET)


if(PEX_FOUND)
  dabc_link_library(
    DabcPex
    SOURCES src/Factory.cxx src/Device.cxx src/Input.cxx  src/Transport.cxx src/Player.cxx src/ReadoutModule.cxx
    HEADERS pex/Factory.h pex/Device.h pex/Input.h  pex/Transport.h pex/Player.h pex/ReadoutModule.h 
    INCDIR pex
    LIBRARIES DabcBase DabcMbs ${PEX_LIBRARY}
    INCLUDES ${PEX_INCLUDE_DIR})

  dabc_install_plugin_data(
    DabcPex
    DIRECTORIES htm app
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/pex)
endif()

