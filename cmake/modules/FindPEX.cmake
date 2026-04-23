# Find PEX (pexor for dabc readout) library and includes


set(PEX_FOUND 0)

    #set(PEXHOME /home/adamczew/workspace/pexlibs/pexor/user)
  	#set(PEXHOME /dabc/driv/PCx86_Linux_OSV_6_12_63_64_Deb/head/pexor/user)
    set(PEXHOME /dabc/driv/$ENV{GSI_CPU_PLATFORM}_$ENV{GSI_OS}_$ENV{GSI_OS_VERSIONX}_$ENV{GSI_OS_TYPE}/head/pexor/user)
    #set(PEXHOME /daq/usr/adamczew/workspace/drivers/pexor/user)
     
     
	find_library(PEX_LIBRARY NAMES libpexor.so PATHS ${PEXHOME}/lib DOC "Searching pexor  library")
	find_path(PEX_INCLUDE_DIR pexor/Board.h  ${PEXHOME}/include)

    if (PEX_LIBRARY AND PEX_INCLUDE_DIR)
      set(PEX_FOUND 1)
      message(STATUS "Found pexor includes ${PEX_INCLUDE_DIR} and library ${PEX_LIBRARY}")
   	endif()

mark_as_advanced(PEX_FOUND PEX_LIBRARY PEX_INCLUDE_DIR)