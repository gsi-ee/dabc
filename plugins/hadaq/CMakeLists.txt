DABC_INSTALL_HEADERS(hadaq)

if(hadaq-debug)
  set(_def HADAQ_DEBUG)
endif()

DABC_LINK_LIBRARY(DabcHadaq
                  LIBRARIES DabcBase DabcMbs
                  DEFINITIONS ${_def})

DABC_EXECUTABLE(hldprint
                SOURCES hldprint.cxx
                LIBRARIES DabcBase DabcMbs DabcHadaq)

file(COPY app hades spill start tdcmon test DESTINATION ${CMAKE_BINARY_DIR}/plugins/hadaq)
