find_package(Stream QUIET PATHS $ENV{STREAMSYS})

if(Stream_FOUND)

  dabc_link_library(
    DabcStream
    SOURCES src/DabcProcMgr.cxx src/Factory.cxx src/RecalibrateModule.cxx
            src/RunModule.cxx src/TdcCalibrationModule.cxx
    HEADERS stream/DabcProcMgr.h stream/Factory.h stream/RecalibrateModule.h
            stream/RunModule.h stream/TdcCalibrationModule.h
    INCLUDE_SUBDIR stream
    LIBRARIES dabc::DabcBase dabc::DabcMbs dabc::DabcHadaq ${Stream_LIBRARY}
    INCLUDES ${STREAM_INCLUDE_DIR})

  # file used in code relative to $DABCSYS, keep path for a while
  configure_file(
    src/stream_engine.cpp
    ${CMAKE_BINARY_DIR}/plugins/stream/src/stream_engine.cpp COPYONLY)

  dabc_install_plugin_data(
    DabcStream
    FILES ${CMAKE_BINARY_DIR}/plugins/stream/src/stream_engine.cpp
    DIRECTORIES icons app
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/stream
    LEGACY_MODE ${DABC_LEGACY_BUILD})

endif()
