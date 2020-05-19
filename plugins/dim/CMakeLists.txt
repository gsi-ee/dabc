find_package(DIM QUIET)

if(DIM_FOUND)
   DABC_LINK_LIBRARY(DabcDim 
                     LIBRARIES DabcBase ${DIM_LIBRARY}
                     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${DIM_INCLUDE_DIR})
endif()
