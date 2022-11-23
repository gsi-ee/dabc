if(ROOT_FOUND)
  dabc_link_library(
    DabcRoot
    SOURCES src/Factory.cxx src/Monitor.cxx src/TDabcEngine.cxx
            src/TreeStore.cxx
    HEADERS root/Factory.h root/Monitor.h root/TDabcEngine.h root/TreeStore.h
    INCLUDE_SUBDIR root
    LIBRARIES dabc::DabcBase dabc::DabcHttp ROOT::RHTTP ROOT::XMLIO ROOT::Tree
    NOWARN)

  root_generate_dictionary(
    G__DabcRoot
    root/TDabcEngine.h
    MODULE DabcRoot
    LINKDEF LinkDef.h
    NOINSTALL)

  dabc_executable(
    dabc_root
    SOURCES utils/dabc_root.cxx
    LIBRARIES dabc::DabcRoot)

  dabc_install_plugin_data(
    DabcRoot
    DIRECTORIES js
    DESTINATION ${DABC_INSTALL_PLUGINDIR}/root
    LEGACY_MODE ${DABC_LEGACY_BUILD})

endif()
