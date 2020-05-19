cmake_minimum_required(VERSION 3.9)
project(dabc-ib-test)
find_package(DABC)
include(${DABC_USE_FILE})

find_path(VERBS_INCLUDE_DIR infiniband/verbs.h)
find_library(VERBS_LIBRARIES ibverbs)

if(VERBS_LIBRARIES AND VERBS_INCLUDE_DIR AND DabcVerbs_LIBRARY)
   set(VERBS_DEFS WITH_VERBS)
endif()

DABC_LINK_LIBRARY(DabcIbTest
                   SOURCES 
                      IbTestApplication.cxx
                      IbTestDefines.cxx
                      IbTestGPU.cxx
                      IbTestSchedule.cxx
                      IbTestWorkerModule.cxx
                   LIBRARIES ${DabcBase_LIBRARY} ${DabcVerbs_LIBRARY} ${VERBS_LIBRARIES}
                   INCLUDES ${VERBS_INCLUDE_DIR}
                   DEFINITIONS ${VERBS_DEFS})
