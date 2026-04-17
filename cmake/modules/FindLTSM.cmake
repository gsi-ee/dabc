# Find LTSM library and includes

set(LTSM_FOUND 0)

# check if we are on MBS cluster, or on HADES eventbuilders:
if(NOT "$ENV{GSI_OS_VERSION}" STREQUAL "")
	set(LTSMHOME /mbs/storage/$ENV{GSI_CPU_PLATFORM}_$ENV{GSI_OS}_$ENV{GSI_OS_VERSION}_$ENV{GSI_OS_TYPE})
	#set(LTSMHOME /mbs/storage/PCx86_Linux_6.12-64_Deb)
else()
	set(LTSMHOME /home/hadaq/ltsm/install)
endif()

find_library(LTSM_LIBRARY NAMES libltsmapi.so PATHS ${LTSMHOME}/lib/ DOC "Searching LTSM library")
find_library(FSQ_LIBRARY NAMES   libfsqapi.so  PATHS ${LTSMHOME}/lib/ DOC "Searching FSQ library")

find_path(LTSM_INCLUDE_DIR ltsmapi.h /usr/include/ltsm ${LTSMHOME}/include/ltsm)
find_path(FSQ_INCLUDE_DIR fsqapi.h /usr/include/fsq ${LTSMHOME}/include/fsq)

find_path(TSM_INCLUDE_DIR dsmapitd.h /usr/include /opt/tivoli/tsm/client/api/bin64/sample)



if (LTSM_LIBRARY AND FSQ_LIBRARY AND LTSM_INCLUDE_DIR AND FSQ_INCLUDE_DIR AND TSM_INCLUDE_DIR)
  set(LTSM_FOUND 1)
  message(STATUS "Found LTSM includes ${LTSM_INCLUDE_DIR} ${FSQ_INCLUDE_DIR} ${TSM_INCLUDE_DIR} and libraries ${LTSM_LIBRARY} ${FSQ_LIBRARY}")
endif()

mark_as_advanced(LTSM_FOUND LTSM_LIBRARY FSQ_LIBRARY LTSM_INCLUDE_DIR FSQ_INCLUDE_DIR TSM_INCLUDE_DIR)

