find_path(VERBS_INCLUDE_DIR infiniband/verbs.h)
find_library(VERBS_LIBRARIES ibverbs)

if(VERBS_LIBRARIES AND VERBS_INCLUDE_DIR)
  dabc_link_library(
    DabcVerbs
    SOURCES src/ComplQueue.cxx
            src/Context.cxx
            src/Device.cxx
            src/Factory.cxx
            src/MemoryPool.cxx
            src/QueuePair.cxx
            src/Thread.cxx
            src/Transport.cxx
            src/Worker.cxx
    HEADERS verbs/ComplQueue.h
            verbs/Context.h
            verbs/Device.h
            verbs/Factory.h
            verbs/MemoryPool.h
            verbs/OpenSM.h
            verbs/QueuePair.h
            verbs/Thread.h
            verbs/Transport.h
            verbs/Worker.h
    INCDIR verbs
    LIBRARIES DabcBase ${VERBS_LIBRARIES}
    DEFINITIONS __NO_MULTICAST__
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${VERBS_INCLUDE_DIR}
    COPY_HEADERS)

endif()
