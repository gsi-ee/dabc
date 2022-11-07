if(hadaq-debug)
  set(_def HADAQ_DEBUG)
endif()

dabc_link_library(
  DabcHadaq
  SOURCES src/api.cxx
          src/BnetMasterModule.cxx
          src/CombinerModule.cxx
          src/defines.cxx
          src/Factory.cxx
          src/FilterModule.cxx
          src/HadaqTypeDefs.cxx
          src/HldFile.cxx
          src/HldInput.cxx
          src/HldOutput.cxx
          src/Iterator.cxx
          src/MonitorModule.cxx
          src/SorterModule.cxx
          src/TerminalModule.cxx
          src/UdpTransport.cxx
  HEADERS hadaq/api.h
          hadaq/BnetMasterModule.h
          hadaq/CombinerModule.h
          hadaq/defines.h
          hadaq/Factory.h
          hadaq/FilterModule.h
          hadaq/HadaqTypeDefs.h
          hadaq/HldFile.h
          hadaq/HldInput.h
          hadaq/HldOutput.h
          hadaq/Iterator.h
          hadaq/MonitorModule.h
          hadaq/SorterModule.h
          hadaq/TerminalModule.h
          hadaq/UdpTransport.h
  INCLUDE_SUBDIR hadaq
  LIBRARIES dabc::DabcBase dabc::DabcMbs
  DEFINITIONS ${_def}
  DABC_INSTALL)

dabc_executable(
  hldprint
  SOURCES hldprint.cxx
  LIBRARIES dabc::DabcHadaq)

file(COPY app hades spill start tdcmon test
     DESTINATION ${PROJECT_BINARY_DIR}/plugins/hadaq)
