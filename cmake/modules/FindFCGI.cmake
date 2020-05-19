# Find FCGI library and includes

set(FCGI_FOUND 0)

find_library(FCGI_LIBRARY NAMES fcgi DOC "Searching FCGI library")

find_path(FCGI_INCLUDE_DIR fcgiapp.h /usr/include /usr/include/fastcgi)

if (FCGI_LIBRARY AND FCGI_INCLUDE_DIR)
  set(FCGI_FOUND 1)
  message(STATUS "Found FCGI includes ${FCGI_INCLUDE_DIR} and library ${FCGI_LIBRARY}")
endif()

mark_as_advanced(FCGI_FOUND FCGI_LIBRARY FCGI_INCLUDE_DIR)

