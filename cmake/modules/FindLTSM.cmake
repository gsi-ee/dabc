# Find LTSM library and includes

set(LTSM_FOUND 0)

find_library(LTSM_LIBRARY NAMES libltsmapi.so PATHS /home/hadaq/ltsm/install/lib/ DOC "Searching LTSM library")
#find_library(LTSM_LIBRARY NAMES libltsmapi.so PATHS /mbs/storage/PCx86_Linux_5.10-64_Deb/lib DOC "Searching LTSM library")

find_library(FSQ_LIBRARY NAMES libfsqapi.so  PATHS /home/hadaq/ltsm/install/lib/ DOC "Searching FSQ library")
#find_library(FSQ_LIBRARY NAMES libfsqapi.so  PATHS /mbs/storage/PCx86_Linux_5.10-64_Deb/lib DOC "Searching FSQ library")


find_path(LTSM_INCLUDE_DIR ltsmapi.h /usr/include/ltsm /home/hadaq/ltsm/install/include/ltsm)
#find_path(LTSM_INCLUDE_DIR ltsmapi.h /mbs/storage/PCx86_Linux_5.10-64_Deb/include)

find_path(FSQ_INCLUDE_DIR fsqapi.h /usr/include/fsq /home/hadaq/ltsm/install/include/fsq)

find_path(TSM_INCLUDE_DIR dsmapitd.h /usr/include /opt/tivoli/tsm/client/api/bin64/sample)
#find_path(TSM_INCLUDE_DIR dsmapitd.h /mbs/storage/PCx86_Linux_5.10-64_Deb/include)

if (LTSM_LIBRARY AND FSQ_LIBRARY AND LTSM_INCLUDE_DIR AND FSQ_INCLUDE_DIR AND TSM_INCLUDE_DIR)
  set(LTSM_FOUND 1)
  message(STATUS "Found LTSM includes ${LTSM_INCLUDE_DIR} ${FSQ_INCLUDE_DIR} ${TSM_INCLUDE_DIR} and libraries ${LTSM_LIBRARY} ${FSQ_LIBRARY}")
endif()

mark_as_advanced(LTSM_FOUND LTSM_LIBRARY FSQ_LIBRARY LTSM_INCLUDE_DIR FSQ_INCLUDE_DIR TSM_INCLUDE_DIR)

