if(NOT PROJECT_NAME STREQUAL DABC)
  cmake_minimum_required(VERSION 3.13)
  project(dabc-net-test)
  find_package(DABC)
endif()

add_library(DabcNetTest net-test.cxx)
target_link_libraries(DabcNetTest dabc::DabcBase)
