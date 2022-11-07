dabc_link_library(
  DabcAqua
  SOURCES src/ClientOutput.cxx src/Factory.cxx
  HEADERS aqua/ClientOutput.h aqua/Factory.h
  INCLUDE_SUBDIR aqua
  LIBRARIES dabc::DabcBase dabc::DabcMbs)

add_executable(aquatest aquatest.c)
