cmake_minimum_required(VERSION 3.9)
project(dabc-ib-test)
find_package(DABC)
include(${DABC_USE_FILE})

if(DabcVerbs_LIBRARY)
   find_path(VERBS_INCLUDE_DIR infiniband/verbs.h)
   find_library(VERBS_LIBRARIES ibverbs)
endif()

if(VERBS_LIBRARIES AND VERBS_INCLUDE_DIR)
   set(VERBS_DEFS WITH_VERBS)
   set(_lib ${DabcVerbs_LIBRARY}) 
else()
   set(VERBS_LIBRARIES)
   set(VERBS_INCLUDE_DIR)
endif()

DABC_LINK_LIBRARY(DabcIbTest
                   SOURCES 
                      IbTestApplication.cxx
                      IbTestDefines.cxx
                      IbTestGPU.cxx
                      IbTestSchedule.cxx
                      IbTestWorkerModule.cxx
                   LIBRARIES ${DabcBase_LIBRARY} ${_lib} ${VERBS_LIBRARIES}
                   INCLUDES ${VERBS_INCLUDE_DIR}
                   DEFINITIONS ${VERBS_DEFS})
