DABC_INSTALL_HEADERS(http)

find_package(ZLIB QUIET)

if(ZLIB_FOUND)
   list(APPEND extra_libs ${ZLIB_LIBRARIES})
   list(APPEND extra_incl ${ZLIB_INCLUDE_DIRS})
else()
   list(APPEND extra_defs DABC_WITHOUT_ZLIB)
endif()

find_package(FCGI QUIET)

if(FCGI_FOUND)
   list(APPEND extra_libs ${FCGI_LIBRARY})
   list(APPEND extra_incl ${FCGI_INCLUDE_DIR})
else()
   list(APPEND extra_defs DABC_WITHOUT_FASTCGI)
endif()


DABC_LINK_LIBRARY(DabcHttp
                  SOURCES
                    civetweb/civetweb.c
                    src/Civetweb.cxx
                    src/Factory.cxx
                    src/FastCgi.cxx
                    src/Server.cxx
                 LIBRARIES 
                    DabcBase
                    ${extra_libs}
                 INCLUDES
                    ${CMAKE_CURRENT_SOURCE_DIR}/civetweb
                    ${extra_incl}
                 DEFINITIONS
                    ${extra_defs})

file(COPY scripts DESTINATION ${CMAKE_BINARY_DIR}/plugins/http)
file(COPY img DESTINATION ${CMAKE_BINARY_DIR}/plugins/http)
