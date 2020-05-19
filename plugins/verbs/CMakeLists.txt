find_path(VERBS_INCLUDE_DIR infiniband/verbs.h)
find_library(VERBS_LIBRARIES ibverbs)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(verbs DEFAULT_MSG VERBS_LIBRARIES VERBS_INCLUDE_DIR)

if(VERBS_FOUND)
#   do not make verbs includes public
#   DABC_INSTALL_HEADERS(verbs)

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
