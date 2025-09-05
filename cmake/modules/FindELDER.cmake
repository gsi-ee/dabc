# Find ELDER library and includes
 message(STATUS "Looking for ELDER...")
set(ELDER_FOUND 0)

#set(ELDERHOME /u/adamczew/go4work/go4-app/elder/installation)
set(ELDERHOME $ENV{ELDERSYS})
find_library(ELDER_LIBRARY NAMES libelderpt-0.1.so  PATHS ${ELDERHOME}/lib DOC "Searching ELDER library")
find_library(ELDER-STD_LIBRARY NAMES libelderptstd.so  PATHS ${ELDERHOME}/lib DOC "Searching ELDER-STD library")
#find_library(ELDER-GSI_LIBRARY NAMES libelderptgsi_gs.so  PATHS ${ELDERHOME}/lib DOC "Searching ELDER-GSI library")
find_library(ELDER-EEL_LIBRARY NAMES libelderpteel.so  PATHS ${ELDERHOME}/lib DOC "Searching ELDER-EEL library")



find_path(ELDER_INCLUDE_DIR elderpt.hpp ${ELDERHOME}/include/elderpt-0.1)

if (ELDER_LIBRARY AND ELDER_INCLUDE_DIR)
  set(ELDER_FOUND 1)
  message(STATUS "Found ELDER includes ${ELDER_INCLUDE_DIR} and library ${ELDER_LIBRARY}")
endif()

mark_as_advanced(ELDER_FOUND ELDER_LIBRARY ELDER_INCLUDE_DIR)

if(ELDER-STD_LIBRARY)
message(STATUS "Found ELDER std library ${ELDER-STD_LIBRARY}")
mark_as_advanced(ELDER-STD_LIBRARY)
endif()

#if(ELDER-GSI_LIBRARY)
#message(STATUS "Found ELDER gsi_gs library ${ELDER-GSI_LIBRARY}")
#mark_as_advanced(ELDER-GSI_LIBRARY)
#endif()

if(ELDER-EEL_LIBRARY)
message(STATUS "Found ELDER EEL library ${ELDER-EEL_LIBRARY}")
mark_as_advanced(ELDER-EEL_LIBRARY)
endif()

