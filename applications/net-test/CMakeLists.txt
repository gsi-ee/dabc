cmake_minimum_required(VERSION 3.9)
project(dabc-net-test)
find_package(DABC)
include(${DABC_USE_FILE})

DABC_LINK_LIBRARY(DabcNetTest SOURCES net-test.cxx LIBRARIES ${DabcBase_LIBRARY})
