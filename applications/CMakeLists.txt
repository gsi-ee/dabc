include(ExternalProject)

set(applications core-test)

foreach(ex ${applications})
   ExternalProject_Add(${ex}
                       DEPENDS DabcBase
                       CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_BINARY_DIR}
                       SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${ex}
                       BINARY_DIR ${ex}
                       INSTALL_COMMAND "")
endforeach()
