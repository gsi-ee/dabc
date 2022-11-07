dabc_link_library(
  DabcUser
  SOURCES src/Input.cxx src/Factory.cxx
  HEADERS user/Factory.h user/Input.h
  INCLUDE_SUBDIR user
  LIBRARIES dabc::DabcBase dabc::DabcMbs)
