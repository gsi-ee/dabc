# Find PEX (pexor for dabc readout) library and includes


set(PEX_FOUND 0)

	#set(GOSIPHOME /mbs/driv/mbspex_$ENV{GSI_OS_VERSION}_DEB)
    set(PEXHOME /home/adamczew/workspace/pexlibs/pexor/user)
	find_library(PEX_LIBRARY NAMES libpexor.so PATHS ${PEXHOME}/lib DOC "Searching pexor  library")
	find_path(PEX_INCLUDE_DIR pexor/Board.h  ${PEXHOME}/include)

    if (PEX_LIBRARY AND PEX_INCLUDE_DIR)
      set(PEX_FOUND 1)
      message(STATUS "Found pexor includes ${PEX_INCLUDE_DIR} and library ${PEX_LIBRARY}")
   	endif()

mark_as_advanced(PEX_FOUND PEX_LIBRARY PEX_INCLUDE_DIR)