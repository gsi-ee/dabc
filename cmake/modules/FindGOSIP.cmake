# Find GOSIP (mbspex) library and includes

set(GOSIP_FOUND 0)

#if(NOT "$ENV{GSI_OS_VERSION}" STREQUAL "")
	set(GOSIPHOME /mbs/driv/mbspex_$ENV{GSI_OS_VERSION}_DEB)
# JAM 16-jan-26: use directly pexor libs instead of mbs, correct driver!
# set(GOSIPHOME /daq/usr/adamczew/workspace/drivers/pexor/user)
#set(GOSIPHOME /dabc/driv/$ENV{GSI_CPU_PLATFORM}_$ENV{GSI_OS}_$ENV{GSI_OS_VERSIONX}_$ENV{GSI_OS_TYPE}/head/pexor/user)
	find_library(GOSIP_LIBRARY NAMES libmbspex.so PATHS ${GOSIPHOME}/lib DOC "Searching mbspex library")
	find_path(GOSIP_INCLUDE_DIR mbspex/libmbspex.h  ${GOSIPHOME}/include)

    if (GOSIP_LIBRARY AND GOSIP_INCLUDE_DIR)
      set(GOSIP_FOUND 1)
      message(STATUS "Found mbspex includes ${GOSIP_INCLUDE_DIR} and library ${GOSIP_LIBRARY}")
   	endif()
#endif()

mark_as_advanced(GOSIP_FOUND GOSIP_LIBRARY GOSIP_INCLUDE_DIR)

