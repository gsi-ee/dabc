if(NOT PROJECT_NAME STREQUAL DABC)
  cmake_minimum_required(VERSION 3.13)
  project(dabc-hadaq-test)
  find_package(DABC)
endif()

add_executable(hadaq-example example.cxx)
target_link_libraries(hadaq-example dabc::DabcBase dabc::DabcHadaq)
