if(ROOT_FOUND)
   include(${ROOT_USE_FILE})

   find_library(ROOT_RHTTP_LIBRARY RHTTP HINTS ${ROOT_LIBRARY_DIR})
   find_library(ROOT_XMLIO_LIBRARY XMLIO HINTS ${ROOT_LIBRARY_DIR})

   DABC_LINK_LIBRARY(DabcRoot
                    LIBRARIES DabcBase DabcHttp ${ROOT_LIBRARIES} ${ROOT_RHTTP_LIBRARY} ${ROOT_XMLIO_LIBRARY}
                    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

   ROOT_GENERATE_DICTIONARY(G__DabcRoot root/TDabcEngine.h
                            MODULE DabcRoot
                            LINKDEF LinkDef.h
                            NOINSTALL)

   message("ROOT LIBS ${ROOT_LIBRARIES}")

   DABC_EXECUTABLE(dabc_root
                SOURCES utils/dabc_root.cxx
                LIBRARIES DabcBase DabcRoot ${ROOT_LIBRARIES} ${ROOT_RHTTP_LIBRARY} ${ROOT_XMLIO_LIBRARY})

endif()

file(COPY js DESTINATION ${CMAKE_BINARY_DIR}/plugins/root)
