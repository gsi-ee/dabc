cmake_minimum_required(VERSION 3.9)
project(dabc-net-test)
find_package(DABC)

add_library(DabcNetTest net-test.cxx)
target_link_libraries(DabcNetTest dabc::DabcBase)
