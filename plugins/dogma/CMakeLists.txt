dabc_link_library(
  DabcDogma
  SOURCES src/CommandModule.cxx
          src/CombinerModule.cxx
          src/Factory.cxx
          src/DogmaFile.cxx
          src/Iterator.cxx
          src/defines.cxx
          src/UdpTransport.cxx
          src/TypeDefs.cxx
          src/api.cxx
  HEADERS dogma/CommandModule.h
          dogma/CombinerModule.h
          dogma/Factory.h
          dogma/DogmaFile.h
          dogma/Iterator.h
          dogma/defines.h
          dogma/UdpTransport.h
          dogma/TypeDefs.h
          dogma/api.h
  INCDIR dogma
  LIBRARIES dabc::DabcBase dabc::DabcMbs
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

dabc_executable(
  dogmacmd
  SOURCES dogmacmd.cxx
  LIBRARIES DabcBase)

dabc_executable(
  dogmaprint
  SOURCES dogmaprint.cxx
  LIBRARIES DabcBase DabcMbs DabcDogma)

dabc_install_plugin_data(
  DabcDogma
  DIRECTORIES app
  DESTINATION ${DABC_INSTALL_PLUGINDIR}/dogma)
