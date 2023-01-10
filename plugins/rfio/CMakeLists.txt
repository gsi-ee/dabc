dabc_link_library(
  DabcRfio
  SOURCES rawapi/rawapin.c
          rawapi/rawCliProcn.c
          rawapi/rawProcn.c
          rawapi/rawProcUn.c
          rawapi/rconnect.c
          src/Factory.cxx
          src/FileInterface.cxx
  HEADERS rfio/Factory.h rfio/FileInterface.h
  INCDIR rfio
  LIBRARIES dabc::DabcBase
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/rawapi ${CMAKE_CURRENT_SOURCE_DIR}
  DEFINITIONS Linux SYSTEM64
  NOWARN)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 10)
   target_compile_options(DabcRfio PRIVATE -Wstringop-overflow=0)
endif()
