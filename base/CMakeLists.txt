set(DABC_BASESUB_S src/threads.cxx src/timing.cxx src/logging.cxx
                   src/string.cxx src/XmlEngine.cxx src/ConfigBase.cxx)
dabc_executable(
  dabc_xml
  SOURCES run/dabc_xml.cxx ${DABC_BASESUB_S}
  INCLUDES ${CMAKE_SOURCE_DIR}/base ${CMAKE_BINARY_DIR}/include)

dabc_link_library(
  DabcBase
  SOURCES src/api.cxx
          src/Application.cxx
          src/BinaryFile.cxx
          src/BinaryFileIO.cxx
          src/Buffer.cxx
          src/BuffersQueue.cxx
          src/Command.cxx
          src/CommandsQueue.cxx
          src/ConfigBase.cxx
          src/ConfigIO.cxx
          src/Configuration.cxx
          src/ConnectionManager.cxx
          src/ConnectionRequest.cxx
          src/CpuInfoModule.cxx
          src/DataIO.cxx
          src/DataTransport.cxx
          src/Device.cxx
          src/eventsapi.cxx
          src/Exception.cxx
          src/Factory.cxx
          src/Hierarchy.cxx
          src/HierarchyStore.cxx
          src/Iterator.cxx
          src/LocalTransport.cxx
          src/logging.cxx
          src/Manager.cxx
          src/MemoryPool.cxx
          src/ModuleAsync.cxx
          src/Module.cxx
          src/ModuleItem.cxx
          src/ModuleSync.cxx
          src/MultiplexerModule.cxx
          src/NetworkTransport.cxx
          src/Object.cxx
          src/Parameter.cxx
          src/Pointer.cxx
          src/Port.cxx
          src/Profiler.cxx
          src/Publisher.cxx
          src/Record.cxx
          src/Reference.cxx
          src/ReferencesVector.cxx
          src/SocketCommandChannel.cxx
          src/SocketDevice.cxx
          src/SocketFactory.cxx
          src/SocketThread.cxx
          src/SocketTransport.cxx
          src/statistic.cxx
          src/string.cxx
          src/Thread.cxx
          src/threads.cxx
          src/timing.cxx
          src/Transport.cxx
          src/Url.cxx
          src/Worker.cxx
          src/XmlEngine.cxx
  HEADERS dabc/api.h
          dabc/Application.h
          dabc/BinaryFile.h
          dabc/BinaryFileIO.h
          dabc/Buffer.h
          dabc/BuffersQueue.h
          dabc/Command.h
          dabc/CommandsQueue.h
          dabc/ConfigBase.h
          dabc/ConfigIO.h
          dabc/Configuration.h
          dabc/ConnectionManager.h
          dabc/ConnectionRequest.h
          dabc/CpuInfoModule.h
          dabc/DataIO.h
          dabc/DataTransport.h
          dabc/Device.h
          dabc/eventsapi.h
          dabc/Exception.h
          dabc/Factory.h
          dabc/Hierarchy.h
          dabc/HierarchyStore.h
          dabc/Iterator.h
          dabc/LocalTransport.h
          dabc/logging.h
          dabc/Manager.h
          dabc/MemoryPool.h
          dabc/ModuleAsync.h
          dabc/Module.h
          dabc/ModuleItem.h
          dabc/ModuleSync.h
          dabc/MultiplexerModule.h
          dabc/NetworkTransport.h
          dabc/Object.h
          dabc/Parameter.h
          dabc/Pointer.h
          dabc/Port.h
          dabc/Profiler.h
          dabc/Publisher.h
          dabc/Queue.h
          dabc/Record.h
          dabc/Reference.h
          dabc/ReferencesVector.h
          dabc/SocketCommandChannel.h
          dabc/SocketDevice.h
          dabc/SocketFactory.h
          dabc/SocketThread.h
          dabc/SocketTransport.h
          dabc/statistic.h
          dabc/string.h
          dabc/Thread.h
          dabc/threads.h
          dabc/timing.h
          dabc/Transport.h
          dabc/Url.h
          dabc/version.h
          dabc/Worker.h
          dabc/XmlEngine.h
          ${CMAKE_BINARY_DIR}/include/dabc/defines.h
  INCLUDE_SUBDIR dabc
  LIBRARIES ${DABC_pthread_LIBRARY} ${DABC_rt_LIBRARY} ${DABC_dl_LIBRARY}
            ${DABC_m_LIBRARY} ${DABC_cpp_LIBRARY}
  DABC_INSTALL)

dabc_executable(
  dabc_exe
  SOURCES run/dabc_exe.cxx
  LIBRARIES DabcBase)

configure_file(run/dabc_run ${PROJECT_BINARY_DIR}/bin/dabc_run COPYONLY)
