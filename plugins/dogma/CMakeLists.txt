dabc_link_library(
  DabcDogma
  SOURCES src/TerminalModule.cxx src/Factory.cxx
  HEADERS dogma/TerminalModule.h dogma/Factory.h
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
