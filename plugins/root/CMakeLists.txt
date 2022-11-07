if(ROOT_FOUND)
  dabc_link_library(
    DabcRoot
    SOURCES src/Factory.cxx src/Monitor.cxx src/TDabcEngine.cxx
            src/TreeStore.cxx
    HEADERS root/Factory.h root/Monitor.h root/TDabcEngine.h root/TreeStore.h
    INCLUDE_SUBDIR root
    LIBRARIES dabc::DabcBase dabc::DabcHttp ROOT::RHTTP ROOT::XMLIO
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
    LIBRARIES dabc::DabcRoot ROOT::Tree)

endif()

file(COPY js DESTINATION ${CMAKE_BINARY_DIR}/plugins/root)
