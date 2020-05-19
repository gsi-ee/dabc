find_path(VERBS_INCLUDE_DIR infiniband/verbs.h)
find_library(VERBS_LIBRARIES ibverbs)

if(VERBS_LIBRARIES AND VERBS_INCLUDE_DIR)
   DABC_INSTALL_HEADERS(verbs)

   DABC_LINK_LIBRARY(DabcVerbs
                     SOURCES 
                       src/ComplQueue.cxx
                       src/Context.cxx
                       src/Device.cxx
                       src/Factory.cxx
                       src/MemoryPool.cxx
                       src/QueuePair.cxx
                       src/Thread.cxx
                       src/Transport.cxx
                       src/Worker.cxx
                     LIBRARIES
                       DabcBase ${VERBS_LIBRARIES}
                     DEFINITIONS
                        __NO_MULTICAST__
                     INCLUDES
                        ${CMAKE_CURRENT_SOURCE_DIR}
                        ${VERBS_INCLUDE_DIR}
)

endif()
