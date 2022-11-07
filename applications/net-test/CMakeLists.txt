cmake_minimum_required(VERSION 3.9)
project(dabc-net-test)
find_package(DABC)
include(${DABC_USE_FILE})

dabc_link_library(
  DabcNetTest
  SOURCES net-test.cxx
  LIBRARIES dabc::DabcBase)
