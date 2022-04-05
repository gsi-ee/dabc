DABC_INSTALL_HEADERS(mbs)

DABC_LINK_LIBRARY(DabcMbs LIBRARIES DabcBase)

DABC_EXECUTABLE(mbsprint
                SOURCES utils/mbsprint.cxx
                LIBRARIES DabcBase DabcMbs)

DABC_EXECUTABLE(mbscmd
                SOURCES utils/mbscmd.cxx
                LIBRARIES DabcBase DabcMbs)

file(COPY htm app DESTINATION ${PROJECT_BINARY_DIR}/plugins/mbs)
