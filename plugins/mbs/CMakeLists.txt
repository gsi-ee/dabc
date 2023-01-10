DABC_INSTALL_HEADERS(mbs)

dabc_link_library(
  DabcMbs
  SOURCES src/api.cxx
          src/ClientTransport.cxx
          src/CombinerModule.cxx
          src/Factory.cxx
          src/GeneratorInput.cxx
          src/Iterator.cxx
          src/LmdInput.cxx
          src/LmdOutput.cxx
          src/MbsTypeDefs.cxx
          src/Monitor.cxx
          src/MonitorSlowControl.cxx
          src/ServerTransport.cxx
          src/TextInput.cxx
  HEADERS mbs/api.h
          mbs/ClientTransport.h
          mbs/CombinerModule.h
          mbs/Factory.h
          mbs/GeneratorInput.h
          mbs/Iterator.h
          mbs/LmdFile.h
          mbs/LmdInput.h
          mbs/LmdOutput.h
          mbs/LmdTypeDefs.h
          mbs/MbsTypeDefs.h
          mbs/Monitor.h
          mbs/MonitorSlowControl.h
          mbs/ServerTransport.h
          mbs/SlowControlData.h
          mbs/TextInput.h
  INCDIR mbs
  LIBRARIES dabc::DabcBase
  DABC_INSTALL)


DABC_EXECUTABLE(mbsprint
                SOURCES utils/mbsprint.cxx
                LIBRARIES DabcBase DabcMbs)

DABC_EXECUTABLE(mbscmd
                SOURCES utils/mbscmd.cxx
                LIBRARIES DabcBase DabcMbs)

dabc_install_plugin_data(
    DabcMbs
    DIRECTORIES app htm
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/mbs)
