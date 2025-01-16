dabc_link_library(
  DabcDogma
  SOURCES src/CombinerModule.cxx
          src/TerminalModule.cxx
          src/Factory.cxx
          src/DogmaFile.cxx
          src/DogmaOutput.cxx
          src/DogmaInput.cxx
          src/Iterator.cxx
          src/defines.cxx
          src/UdpTransport.cxx
          src/TypeDefs.cxx
          src/api.cxx
  HEADERS dogma/CombinerModule.h
          dogma/TerminalModule.h
          dogma/Factory.h
          dogma/DogmaFile.h
          dogma/DogmaOutput.h
          dogma/DogmaInput.h
          dogma/Iterator.h
          dogma/defines.h
          dogma/UdpTransport.h
          dogma/TypeDefs.h
          dogma/api.h
  INCDIR dogma
  LIBRARIES dabc::DabcBase dabc::DabcMbs
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

dabc_executable(
  dldprint
  SOURCES dldprint.cxx
  LIBRARIES DabcBase DabcMbs DabcDogma)

dabc_install_plugin_data(
  DabcDogma
  DIRECTORIES app
  DESTINATION ${DABC_INSTALL_PLUGINDIR}/dogma)
