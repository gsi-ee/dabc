dabc_link_library(
  DabcDogma
  SOURCES src/TerminalModule.cxx
          src/Factory.cxx
          src/defines.cxx
          src/UdpTransport.cxx
          src/TypeDefs.cxx
  HEADERS dogma/TerminalModule.h
          dogma/Factory.h
          dogma/defines.h
          dogma/UdpTransport.h
          dogma/TypeDefs.h
  INCDIR dogma
  LIBRARIES DabcBase
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

dabc_executable(
  dogmacmd
  SOURCES dogmacmd.cxx
  LIBRARIES DabcBase)

dabc_install_plugin_data(
  DabcDogma
  DIRECTORIES app
  DESTINATION ${DABC_INSTALL_PLUGINDIR}/dogma)
