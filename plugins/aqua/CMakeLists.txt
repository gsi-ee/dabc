dabc_link_library(
  DabcAqua
  SOURCES src/ClientOutput.cxx src/Factory.cxx
  HEADERS aqua/ClientOutput.h aqua/Factory.h
  INCDIR aqua
  LIBRARIES dabc::DabcBase dabc::DabcMbs
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(aquatest aquatest.c)
