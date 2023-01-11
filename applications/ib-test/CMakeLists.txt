cmake_minimum_required(VERSION 3.9)
project(dabc-ib-test)
find_package(DABC)

if(DabcVerbs_LIBRARY)
  find_path(VERBS_INCLUDE_DIR infiniband/verbs.h)
  find_library(VERBS_LIBRARIES ibverbs)
endif()

if(VERBS_LIBRARIES AND VERBS_INCLUDE_DIR)
  set(VERBS_DEFS WITH_VERBS)
  set(_lib dabc::DabcVerbs)
else()
  set(VERBS_LIBRARIES)
  set(VERBS_INCLUDE_DIR)
endif()

add_library(DabcIbTest IbTestApplication.cxx IbTestDefines.cxx IbTestGPU.cxx
                       IbTestSchedule.cxx IbTestWorkerModule.cxx)
target_link_libraries(DabcIbTest dabc::DabcBase ${_lib} ${VERBS_LIBRARIES})

# DEFINITIONS ${VERBS_DEFS} #TODO
