dabc_link_library(
  DabcUser
  SOURCES src/Input.cxx src/Factory.cxx
  HEADERS user/Factory.h user/Input.h
  INCDIR user
  LIBRARIES dabc::DabcBase dabc::DabcMbs
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
