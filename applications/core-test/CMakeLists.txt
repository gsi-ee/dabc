cmake_minimum_required(VERSION 3.9)
project(dabc-core-test)
find_package(DABC)
include(${DABC_USE_FILE})

DABC_LINK_LIBRARY(DabcCoreTest SOURCES core-test.cxx LIBRARIES ${DabcBase_LIBRARY})
