# Find GOSIP (mbspex) library and includes

set(GOSIP_FOUND 0)

if(NOT "$ENV{GSI_OS_VERSION}" STREQUAL "")
	set(GOSIPHOME /mbs/driv/mbspex_$ENV{GSI_OS_VERSION}_DEB)
	# build on debian10 gsi linux:
    #set(GOSIPHOME /lynx/Lynx/daq/usr/adamczew/workspace/drivers/mbspex)
	find_library(GOSIP_LIBRARY NAMES libmbspex.so PATHS ${GOSIPHOME}/lib DOC "Searching mbspex library")
	find_path(GOSIP_INCLUDE_DIR mbspex/libmbspex.h  ${GOSIPHOME}/include)

    if (GOSIP_LIBRARY AND GOSIP_INCLUDE_DIR)
      set(GOSIP_FOUND 1)
      message(STATUS "Found mbspex includes ${GOSIP_INCLUDE_DIR} and library ${GOSIP_LIBRARY}")
   	endif()
endif()

mark_as_advanced(GOSIP_FOUND GOSIP_LIBRARY GOSIP_INCLUDE_DIR)

