find_package(ROOT QUIET)

if(ROOT_FOUND)
   include(${ROOT_USE_FILE})

   DABC_LINK_LIBRARY(DabcMbsRoot
                    LIBRARIES DabcBase DabcMbs ${ROOT_LIBRARIES}
                    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

   ROOT_GENERATE_DICTIONARY(G__DabcMbsRoot mbs_root/DabcEvent.h mbs_root/DabcSubEvent.h
                            MODULE DabcMbsRoot
                            LINKDEF mbs_root/mbs_rootLinkDef.h
                            NOINSTALL)
endif()
