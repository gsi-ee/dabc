find_package(LTSM QUIET)

if(LTSM_FOUND)
  dabc_link_library(
    DabcLtsm
    SOURCES src/Factory.cxx src/FileInterface.cxx
    HEADERS ltsm/Factory.h ltsm/FileInterface.h
    INCLUDE_SUBDIR ltsm
    LIBRARIES dabc::DabcBase ${LTSM_LIBRARY} ${FSQ_LIBRARY}
    INCLUDES ${LTSM_INCLUDE_DIR} ${LTSM_INCLUDE_DIR} ${TSM_INCLUDE_DIR}
    DEFINITIONS SYSTEM64 HAVE_CONFIG_H)

  install(DIRECTORY app
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dabc/plugins/ltsm)
endif()
