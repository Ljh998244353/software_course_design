# Install script for directory: /home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/llvm-objdump-20")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "lib" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ljh/project/soft_course_design/build/lib/libtrantor.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/trantor" TYPE FILE FILES "/home/ljh/project/soft_course_design/build/_deps/drogon-build/trantor/exports/trantor/exports.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/trantor/net" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/EventLoop.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/EventLoopThread.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/EventLoopThreadPool.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/InetAddress.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/TcpClient.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/TcpConnection.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/TcpServer.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/AsyncStream.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/callbacks.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/Resolver.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/Channel.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/Certificate.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/net/TLSPolicy.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/trantor/utils" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/AsyncFileLogger.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/ConcurrentTaskQueue.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/Date.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/Funcs.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/LockFreeQueue.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/LogStream.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/Logger.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/MsgBuffer.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/NonCopyable.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/ObjectPool.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/SerialTaskQueue.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/TaskQueue.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/TimingWheel.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/trantor/utils/Utilities.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Trantor" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-build/trantor/CMakeFiles/TrantorConfig.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-build/trantor/TrantorConfigVersion.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/cmake_modules/Findc-ares.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/trantor/cmake_modules/FindBotan.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Trantor/TrantorTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Trantor/TrantorTargets.cmake"
         "/home/ljh/project/soft_course_design/build/_deps/drogon-build/trantor/CMakeFiles/Export/e2741f8d78b158992c2d7ed6a282eae4/TrantorTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Trantor/TrantorTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Trantor/TrantorTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Trantor" TYPE FILE FILES "/home/ljh/project/soft_course_design/build/_deps/drogon-build/trantor/CMakeFiles/Export/e2741f8d78b158992c2d7ed6a282eae4/TrantorTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Trantor" TYPE FILE FILES "/home/ljh/project/soft_course_design/build/_deps/drogon-build/trantor/CMakeFiles/Export/e2741f8d78b158992c2d7ed6a282eae4/TrantorTargets-debug.cmake")
  endif()
endif()

