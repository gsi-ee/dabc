find_package(GOSIP QUIET)

if(GOSIP_FOUND)
  dabc_link_library(
    DabcGosip
    SOURCES src/Factory.cxx src/Player.cxx
    HEADERS gosip/Factory.h gosip/Player.h
    INCLUDE_SUBDIR gosip
    LIBRARIES dabc::DabcBase dabc::DabcMbs ${GOSIP_LIBRARY}
    INCLUDES ${GOSIP_INCLUDE_DIR})

  dabc_install_plugin_data(
    DabcGosip
    FILES htm
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/gosip)

endif()
