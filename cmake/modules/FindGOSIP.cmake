# Find GOSIP (mbspex) library and includes

set(GOSIP_FOUND 0)

if(NOT "$ENV{GSI_OS_VERSION}" STREQUAL "")
	set(GOSIPHOME /mbs/driv/mbspex_$ENV{GSI_OS_VERSION}_DEB)
	find_library(GOSIP_LIBRARY NAMES libmbspex.so PATHS ${GOSIPHOME}/lib DOC "Searching mbspex library")
#	find_library(GOSIP_LIBRARY NAMES /mbs/driv/mbspex_5.10-64_DEB/lib/libmbspex.so PATHS / DOC "Searching mbspex library")

	find_path(GOSIP_INCLUDE_DIR mbspex/libmbspex.h  ${GOSIPHOME}/include)

    if (GOSIP_LIBRARY AND GOSIP_INCLUDE_DIR)
      set(GOSIP_FOUND 1)
      #set(GOSIP_INCLUDE_DIR ${GOSIPHOME}/include)
      message(STATUS "Found mbspex includes ${GOSIP_INCLUDE_DIR} and library ${GOSIP_LIBRARY}")
   	endif()
endif()

mark_as_advanced(GOSIP_FOUND GOSIP_LIBRARY GOSIP_INCLUDE_DIR)

