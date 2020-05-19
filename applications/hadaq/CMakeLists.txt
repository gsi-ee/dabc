cmake_minimum_required(VERSION 3.9)
project(dabc-hadaq-test)
find_package(DABC)
include(${DABC_USE_FILE})

DABC_EXECUTABLE(hadaq-example SOURCES example.cxx LIBRARIES ${DabcBase_LIBRARY} ${DabcHadaq_LIBRARY})
