if(ROOT_FOUND)
  include(${ROOT_USE_FILE})

  dabc_link_library(
    DabcMbsRoot
    SOURCES src/DabcEvent.cxx src/DabcSubEvent.cxx src/Factory.cxx
            src/RootTreeOutput.cxx
    HEADERS mbs_root/DabcEvent.h mbs_root/DabcSubEvent.h mbs_root/Factory.h
            mbs_root/mbs_rootLinkDef.h mbs_root/RootTreeOutput.h
    INCDIR mbs_root
    LIBRARIES dabc::DabcBase dabc::DabcMbs
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
    NOWARN)

  root_generate_dictionary(
    G__DabcMbsRoot
    mbs_root/DabcEvent.h mbs_root/DabcSubEvent.h
    MODULE DabcMbsRoot
    LINKDEF mbs_root/mbs_rootLinkDef.h
    NOINSTALL)
endif()
