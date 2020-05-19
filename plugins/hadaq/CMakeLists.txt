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
