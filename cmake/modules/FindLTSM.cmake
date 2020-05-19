# Find LTSM library and includes

set(LTSM_FOUND 0)

find_library(LTSM_LIBRARY NAMES ApiTSM64 DOC "Searching LTSM library")

find_path(LTSM_INCLUDE_DIR tsmapi.h /usr/include /home/hadaq/ltsm/ltsm_ts/src/lib)

if (LTSM_LIBRARY AND LTSM_INCLUDE_DIR)
  set(LTSM_FOUND 1)
  message(STATUS "Found LTSM includes ${LTSM_INCLUDE_DIR} and library ${LTSM_LIBRARY}")
endif()

mark_as_advanced(LTSM_FOUND LTSM_LIBRARY LTSM_INCLUDE_DIR)

