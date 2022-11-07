dabc_link_library(
  DabcBnet
  SOURCES src/Factory.cxx src/GeneratorModule.cxx src/ReceiverModule.cxx
          src/SenderModule.cxx src/testing.cxx
  HEADERS bnet/Factory.h bnet/GeneratorModule.h bnet/ReceiverModule.h
          bnet/SenderModule.h bnet/testing.h
  INCLUDE_SUBDIR bnet
  LIBRARIES dabc::DabcBase)
