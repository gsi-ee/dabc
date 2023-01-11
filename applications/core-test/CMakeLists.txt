cmake_minimum_required(VERSION 3.9)
project(dabc-core-test)
find_package(DABC)

add_library(DabcCoreTest core-test.cxx)
target_link_libraries(DabcCoreTest dabc::DabcBase)
