cmake_minimum_required(VERSION 3.9)
project(dabc-core-test)
find_package(DABC)
include(${DABC_USE_FILE})

dabc_link_library(
  DabcCoreTest
  SOURCES core-test.cxx
  LIBRARIES dabc::DabcBase)
