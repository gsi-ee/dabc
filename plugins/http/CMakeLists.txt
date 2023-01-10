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

find_package(OpenSSL QUIET)

if(OPENSSL_FOUND)
  if(OPENSSL_VERSION)
    string(REPLACE "." ";" lst ${OPENSSL_VERSION})
    list(GET lst 0 ssl_major)
    list(GET lst 1 ssl_minor)
  endif()
  if((${ssl_major} EQUAL "1") AND (${ssl_minor} EQUAL "1"))
    MESSAGE(STATUS "Use SSL API VERSION 1.1 for civetweb")
    list(APPEND extra_defs OPENSSL_API_1_1)
    set(link_ssl ON)
  elseif((${ssl_major} EQUAL "3") AND (${ssl_minor} EQUAL "0"))
    MESSAGE(STATUS "Use SSL API VERSION 3.0 for civetweb")
    list(APPEND extra_defs OPENSSL_API_3_0)
    set(link_ssl ON)
  elseif((${ssl_major} EQUAL "1") AND (${ssl_minor} EQUAL "0"))
    MESSAGE(STATUS "Use SSL API VERSION 1.0 for civetweb")
    list(APPEND extra_defs OPENSSL_API_1_0)
    set(link_ssl ON)
  endif()
endif()

if(link_ssl)
   list(APPEND extra_defs NO_SSL_DL)
   list(APPEND extra_libs ${OPENSSL_LIBRARIES})
   list(APPEND extra_incl ${OPENSSL_INCLUDE_DIR})
   MESSAGE(STATUS "Use SSL ${OPENSSL_LIBRARIES} ${OPENSSL_INCLUDE_DIR}")
else()
   list(APPEND extra_defs NO_SSL)
endif()

dabc_link_library(
  DabcHttp
  SOURCES civetweb/civetweb.c src/Civetweb.cxx src/Factory.cxx src/FastCgi.cxx
          src/Server.cxx
  HEADERS http/Civetweb.h http/Factory.h http/FastCgi.h http/Server.h
  INCDIR http
  LIBRARIES dabc::DabcBase ${extra_libs}
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/civetweb ${extra_incl}
  DEFINITIONS ${extra_defs})

dabc_install_plugin_data(
  DabcHttp
  FILES example.xml master.xml
  DIRECTORIES scripts jq img
  DESTINATION ${DABC_INSTALL_PLUGINDIR}/http)
