find_package(Stream QUIET)

if(Stream_FOUND)
   DABC_LINK_LIBRARY(DabcStream
                     SOURCES
                        src/DabcProcMgr.cxx
                        src/Factory.cxx
                        src/RecalibrateModule.cxx
                        src/RunModule.cxx
                        src/TdcCalibrationModule.cxx 
                     LIBRARIES 
                        DabcBase 
                        DabcMbs 
                        DabcHadaq 
                        ${Stream_LIBRARY}
                     INCLUDES 
                        ${CMAKE_CURRENT_SOURCE_DIR} 
                        ${STREAM_INCLUDE_DIR})
                     
   # file used in code relative to $DABCSYS, keep path for a while                  
   configure_file(src/stream_engine.cpp ${CMAKE_BINARY_DIR}/plugins/stream/src/stream_engine.cpp COPYONLY)
endif()
