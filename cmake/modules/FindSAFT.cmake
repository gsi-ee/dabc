# Find SAFT library and includes

set(SAFT_FOUND 0)

find_library(SAFT_LIBRARY NAMES saftlib DOC "Searching SAFT library")

find_path(SAFT_INCLUDE_DIR saftlib/SAFTd.h /usr/include)

if (SAFT_LIBRARY AND SAFT_INCLUDE_DIR)
  set(SAFT_FOUND 1)
  message(STATUS "Found SAFT includes ${SAFT_INCLUDE_DIR} and library ${SAFT_LIBRARY}")
endif()

mark_as_advanced(SAFT_FOUND SAFT_LIBRARY SAFT_INCLUDE_DIR)

