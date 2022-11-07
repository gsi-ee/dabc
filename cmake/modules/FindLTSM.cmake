# Find LTSM library and includes

set(LTSM_FOUND 0)

find_library(LTSM_LIBRARY NAMES libltsmapi.a  PATHS /home/hadaq/ltsm/install/lib/ DOC "Searching LTSM library")

find_path(LTSM_INCLUDE_DIR ltsmapi.h /usr/include /home/hadaq/ltsm/install/include)

find_path(TSM_INCLUDE_DIR dsmapitd.h /usr/include /opt/tivoli/tsm/client/api/bin64/sample)


if (LTSM_LIBRARY AND LTSM_INCLUDE_DIR AND TSM_INCLUDE_DIR)
  set(LTSM_FOUND 1)
  message(STATUS "Found LTSM includes ${LTSM_INCLUDE_DIR} ${TSM_INCLUDE_DIR} and library ${LTSM_LIBRARY}")
endif()

mark_as_advanced(LTSM_FOUND LTSM_LIBRARY LTSM_INCLUDE_DIR TSM_INCLUDE_DIR)

