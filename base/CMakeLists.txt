DABC_INSTALL_HEADERS(dabc)

set(DABC_BASESUB_S
       src/threads.cxx
       src/timing.cxx
       src/logging.cxx
       src/string.cxx
       src/XmlEngine.cxx
       src/ConfigBase.cxx
)
DABC_EXECUTABLE(dabc_xml
                SOURCES run/dabc_xml.cxx ${DABC_BASESUB_S}
                LIBRARIES ${DABC_pthread_LIBRARY} ${DABC_rt_LIBRARY})

DABC_LINK_LIBRARY(DabcBase LIBRARIES ${DABC_pthread_LIBRARY} ${DABC_rt_LIBRARY} ${DABC_dl_LIBRARY})

DABC_EXECUTABLE(dabc_exe
                SOURCES run/dabc_exe.cxx
                LIBRARIES DabcBase)

configure_file(run/dabc_run ${PROJECT_BINARY_DIR}/bin/dabc_run COPYONLY)
