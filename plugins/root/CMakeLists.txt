if(ROOT_FOUND)
  include(${ROOT_USE_FILE})

  find_library(ROOT_RHTTP_LIBRARY RHTTP HINTS ${ROOT_LIBRARY_DIR})
  find_library(ROOT_XMLIO_LIBRARY XMLIO HINTS ${ROOT_LIBRARY_DIR})

  dabc_link_library(
    DabcRoot
    SOURCES src/Factory.cxx src/Monitor.cxx src/TDabcEngine.cxx
            src/TreeStore.cxx
    HEADERS root/Factory.h root/Monitor.h root/TDabcEngine.h root/TreeStore.h
    INCDIR root
    LIBRARIES dabc::DabcBase dabc::DabcHttp ROOT::RHTTP ROOT::XMLIO
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
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
    LIBRARIES DabcBase DabcRoot ${ROOT_LIBRARIES} ${ROOT_RHTTP_LIBRARY}
              ${ROOT_XMLIO_LIBRARY})

endif()

dabc_install_plugin_data(
  DabcRoot
  DIRECTORIES js
  DESTINATION ${DABC_INSTALL_PLUGINDIR}/root)
