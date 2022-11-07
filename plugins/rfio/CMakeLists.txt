DABC_LINK_LIBRARY(DabcRfio
                  SOURCES
                     rawapi/rawapin.c
                     rawapi/rawCliProcn.c
                     rawapi/rawProcn.c
                     rawapi/rawProcUn.c
                     rawapi/rconnect.c
                     src/Factory.cxx
                     src/FileInterface.cxx
                 LIBRARIES
                     DabcBase
                 INCLUDES
                    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/rawapi
                 DEFINITIONS
                    Linux SYSTEM64
                 NOWARN)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 10)
   target_compile_options(DabcRfio PRIVATE -Wstringop-overflow=0)
endif()
