cmake_minimum_required(VERSION 3.9)
project(dabc-hadaq-test)
find_package(DABC)

add_executable(hadaq-example example.cxx)
target_link_libraries(hadaq-example dabc::DabcBase dabc::DabcHadaq)
