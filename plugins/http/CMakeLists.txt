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

find_package(OpenSSL)

if(OPENSSL_FOUND)
  # cmake-format: off
  # Fix for cmake < 3.17.3 and OpenSSL >= 3.0
  # Taken from https://github.com/Kitware/CMake/blob/5676593be9bd3a26995f35b9db8fe5b9af8ea32e/Modules/FindOpenSSL.cmake#L562-L582
  if (NOT OPENSSL_VERSION)
    # Since OpenSSL 3.0.0, the new version format is MAJOR.MINOR.PATCH and
    # a new OPENSSL_VERSION_STR macro contains exactly that
    file(STRINGS "${OPENSSL_INCLUDE_DIR}/openssl/opensslv.h" OPENSSL_VERSION_STR
         REGEX "^#[\t ]*define[\t ]+OPENSSL_VERSION_STR[\t ]+\"([0-9])+\\.([0-9])+\\.([0-9])+\".*")
    string(REGEX REPLACE "^.*OPENSSL_VERSION_STR[\t ]+\"([0-9]+\\.[0-9]+\\.[0-9]+)\".*$"
           "\\1" OPENSSL_VERSION_STR "${OPENSSL_VERSION_STR}")

    set(OPENSSL_VERSION "${OPENSSL_VERSION_STR}")

    # Setting OPENSSL_VERSION_MAJOR OPENSSL_VERSION_MINOR and OPENSSL_VERSION_FIX
    string(REGEX MATCHALL "([0-9])+" OPENSSL_VERSION_NUMBER "${OPENSSL_VERSION}")
    # FIXME requires cmake-3.15 or newer, not working for 3.13
    if(CMAKE_VERSION VERSION_LESS 3.15.0)
      # fix for < cmake-3.15
      list(GET OPENSSL_VERSION_NUMBER 0 OPENSSL_VERSION_MAJOR)
      list(GET OPENSSL_VERSION_NUMBER 1 OPENSSL_VERSION_MINOR)
      list(GET OPENSSL_VERSION_NUMBER 2 OPENSSL_VERSION_FIX)
    else()
      list(POP_FRONT OPENSSL_VERSION_NUMBER
        OPENSSL_VERSION_MAJOR
        OPENSSL_VERSION_MINOR
        OPENSSL_VERSION_FIX)
    endif()

    unset(OPENSSL_VERSION_NUMBER)
    unset(OPENSSL_VERSION_STR)
  endif ()
  # -- end of fix
  # cmake-format: on

  if(${OPENSSL_VERSION} VERSION_GREATER_EQUAL 3.1.0)
    message(STATUS "Use SSL API VERSION 3.1 for civetweb")
    list(APPEND extra_defs OPENSSL_API_3_1)
    set(link_ssl ON)
  elseif(${OPENSSL_VERSION} VERSION_GREATER_EQUAL 3.0.0)
    message(STATUS "Use SSL API VERSION 3.0 for civetweb")
    list(APPEND extra_defs OPENSSL_API_3_0)
    set(link_ssl ON)
  elseif(${OPENSSL_VERSION} VERSION_GREATER_EQUAL 1.1.0)
    message(STATUS "Use SSL API VERSION 1.1 for civetweb")
    list(APPEND extra_defs OPENSSL_API_1_1)
    set(link_ssl ON)
  elseif(${OPENSSL_VERSION} VERSION_GREATER_EQUAL 1.0.0)
    message(STATUS "Use SSL API VERSION 1.0 for civetweb")
    list(APPEND extra_defs OPENSSL_API_1_0)
    set(link_ssl ON)
  endif()
endif()

if(link_ssl)
  list(APPEND extra_defs NO_SSL_DL)
  list(APPEND extra_libs ${OPENSSL_LIBRARIES})
  list(APPEND extra_incl ${OPENSSL_INCLUDE_DIR})
  message(STATUS "Use SSL ${OPENSSL_LIBRARIES} ${OPENSSL_INCLUDE_DIR}")
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
  DEFINITIONS ${extra_defs}
  COPY_HEADERS)

dabc_install_plugin_data(
  DabcHttp
  FILES example.xml master.xml
  DIRECTORIES js scripts jq img
  DESTINATION ${DABC_INSTALL_PLUGINDIR}/http)
