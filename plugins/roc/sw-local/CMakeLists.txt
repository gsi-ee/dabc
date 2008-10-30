project(KNUT)

SET(CMAKE_BUILD_TYPE own)
SET(CMAKE_CXX_FLAGS_OWN "-O3 -DNDEBUG")
SET(CMAKE_C_FLAGS_OWN "-O3 -DNDEBUG")

# ________________ CHECK FOR ROOT _________________________


OPTION(USE_ROOT "Should we use ROOT for dictionary generation" ON)


SET (KNUT_DICT)

IF (${USE_ROOT} STREQUAL ON)
IF (EXISTS $ENV{ROOTSYS})

MESSAGE(STATUS "Use ROOT instalation from $ENV{ROOTSYS}")

FIND_PROGRAM(ROOT_CINT_EXECUTABLE
               NAMES rootcint
               PATHS $ENV{ROOTSYS}/bin
               NO_DEFAULT_PATH)

FIND_PROGRAM(ROOT_RLIBMAP_EXECUTABLE
               NAMES rlibmap
               PATHS $ENV{ROOTSYS}/bin
               NO_DEFAULT_PATH)

SET (KNUT_LINKDEF Control/LinkDef.h)

SET (KNUT_DICT ${CMAKE_CURRENT_BINARY_DIR}/G__SysCore.cpp)

SET (KNUT_DICT_H ${CMAKE_CURRENT_BINARY_DIR}/G__SysCore.h)

SET (KNUT_MAP ${CMAKE_BINARY_DIR}/libKnut.rootmap)

SET_SOURCE_FILES_PROPERTIES(${KNUT_DICT} PROPERTIES
                            GENERATED true
                            COMPILE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}")

SET_SOURCE_FILES_PROPERTIES(${KNUT_DICT_H} PROPERTIES
                            GENERATED true)

SET_SOURCE_FILES_PROPERTIES(${KNUT_MAP} PROPERTIES
                            GENERATED true)

INCLUDE_DIRECTORIES($ENV{ROOTSYS}/include)

ELSE (EXISTS $ENV{ROOTSYS})
SET(USE_ROOT OFF)
ENDIF (EXISTS $ENV{ROOTSYS})
ENDIF (${USE_ROOT} STREQUAL ON)


# ________________ THIS IS CppSockets static clibrary ______________

FILE(GLOB CPPSOCKETS_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CppSockets/*.cpp)

add_library(CppSockets STATIC ${CPPSOCKETS_SRCS})

INCLUDE_DIRECTORIES(CppSockets)

# ________________ Simulator ______________


add_executable(simulator Simulator/SysCoreSimulatorApp.cpp Simulator/SysCoreSimulator.cpp)

TARGET_LINK_LIBRARIES(simulator CppSockets pthread)

# ________________ this is control static clibrary ______________

add_library(Knut_st STATIC Control/SysCoreData.cpp Control/SysCoreSorter.cpp Control/SysCoreBoard.cpp Control/SysCoreControl.cpp)

TARGET_LINK_LIBRARIES(Knut_st CppSockets pthread)

# ________________ Upload ________________________

add_executable(upload Control/SysCoreUploadApp.cpp)

TARGET_LINK_LIBRARIES(upload Knut_st)

# _________________ KNUT library _________________

IF (${USE_ROOT} STREQUAL ON)

add_library(Knut SHARED ${KNUT_DICT})

TARGET_LINK_LIBRARIES(Knut Knut_st)

FILE(GLOB KNUT_HDRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Control/SysCore*.h)

GET_TARGET_PROPERTY(KnutLocation Knut LOCATION)

MESSAGE(STATUS "Knut will be located in ${KnutLocation}")

ADD_CUSTOM_COMMAND(OUTPUT ${KNUT_DICT}
     COMMAND ${ROOT_CINT_EXECUTABLE}
     ARGS -f ${KNUT_DICT} -c -p ${KNUT_HDRS} ${KNUT_LINKDEF}
     DEPENDS ${KNUT_HDRS} ${KNUT_LINKDEF}
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

ADD_CUSTOM_COMMAND(TARGET Knut
                  POST_BUILD
                  COMMAND ${ROOT_RLIBMAP_EXECUTABLE}
                  ARGS -o ${KNUT_MAP} -l ${KnutLocation} -c ${KNUT_LINKDEF}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

ENDIF (${USE_ROOT} STREQUAL ON)
