dabc_link_library(
  DabcBnet
  SOURCES src/Factory.cxx src/GeneratorModule.cxx src/ReceiverModule.cxx
          src/SenderModule.cxx src/testing.cxx
  HEADERS bnet/Factory.h bnet/GeneratorModule.h bnet/ReceiverModule.h
          bnet/SenderModule.h bnet/testing.h
  INCDIR bnet
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
  LIBRARIES dabc::DabcBase)
