find_package(DIM QUIET)

if(DIM_FOUND)
  dabc_link_library(
    DabcDim
    SOURCES src/Factory.cxx src/Monitor.cxx
    HEADERS dim/Factory.h dim/Monitor.h
    INCLUDE_SUBDIR dim
    LIBRARIES dabc::DabcBase ${DIM_LIBRARY}
    INCLUDES ${DIM_INCLUDE_DIR})
endif()
