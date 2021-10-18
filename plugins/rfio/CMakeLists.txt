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

target_compile_options(DabcRfio PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wstringop-overflow=0>)