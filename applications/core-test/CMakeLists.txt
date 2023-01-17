if(NOT PROJECT_NAME STREQUAL DABC)
  cmake_minimum_required(VERSION 3.13)
  project(dabc-core-test)
  find_package(DABC)
endif()

add_library(DabcCoreTest core-test.cxx)
target_link_libraries(DabcCoreTest dabc::DabcBase)
