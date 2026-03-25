# Install script for directory: /home/ljh/project/soft_course_design/build/_deps/drogon-src

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ljh/project/soft_course_design/build/lib/libdrogon.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/drogon" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/Attribute.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/CacheMap.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/Cookie.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/DrClassMap.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/DrObject.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/DrTemplate.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/DrTemplateBase.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpAppFramework.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpBinder.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpClient.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpController.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpFilter.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpRequest.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpResponse.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpSimpleController.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpTypes.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/HttpViewData.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/IntranetIpFilter.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/IOThreadStorage.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/LocalHostFilter.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/MultiPart.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/NotFound.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/Session.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/UploadFile.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/WebSocketClient.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/WebSocketConnection.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/WebSocketController.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/drogon.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/version.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/drogon_callbacks.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/PubSubService.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/drogon_test.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/RateLimiter.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-build/exports/drogon/exports.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/drogon/orm" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/ArrayParser.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/BaseBuilder.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/Criteria.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/DbClient.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/DbListener.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/DbTypes.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/Exception.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/Field.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/FunctionTraits.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/Mapper.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/CoroMapper.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/Result.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/ResultIterator.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/Row.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/RowIterator.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/SqlBinder.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/orm_lib/inc/drogon/orm/RestfulController.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/drogon/nosql" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/nosql_lib/redis/inc/drogon/nosql/RedisClient.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/nosql_lib/redis/inc/drogon/nosql/RedisResult.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/nosql_lib/redis/inc/drogon/nosql/RedisSubscriber.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/nosql_lib/redis/inc/drogon/nosql/RedisException.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/drogon/utils" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/coroutine.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/FunctionTraits.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/HttpConstraint.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/OStringStream.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/Utilities.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/monitoring.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/drogon/utils/monitoring" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/monitoring/Counter.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/monitoring/Metric.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/monitoring/Registry.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/monitoring/Collector.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/monitoring/Sample.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/monitoring/Gauge.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/utils/monitoring/Histogram.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/drogon/plugins" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/Plugin.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/Redirector.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/SecureSSLRedirector.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/AccessLogger.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/RealIpResolver.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/Hodor.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/SlashRemover.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/GlobalFilters.h"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/lib/inc/drogon/plugins/PromExporter.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Drogon" TYPE FILE FILES
    "/home/ljh/project/soft_course_design/build/_deps/drogon-build/CMakeFiles/DrogonConfig.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-build/DrogonConfigVersion.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/FindUUID.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/FindJsoncpp.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/FindSQLite3.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/FindMySQL.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/Findpg.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/FindBrotli.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/Findcoz-profiler.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/FindHiredis.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake_modules/FindFilesystem.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake/DrogonUtilities.cmake"
    "/home/ljh/project/soft_course_design/build/_deps/drogon-src/cmake/ParseAndAddDrogonTests.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "dev" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Drogon/DrogonTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Drogon/DrogonTargets.cmake"
         "/home/ljh/project/soft_course_design/build/_deps/drogon-build/CMakeFiles/Export/9fe51f2b716a6bd37518a903e3e9a4cf/DrogonTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Drogon/DrogonTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Drogon/DrogonTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Drogon" TYPE FILE FILES "/home/ljh/project/soft_course_design/build/_deps/drogon-build/CMakeFiles/Export/9fe51f2b716a6bd37518a903e3e9a4cf/DrogonTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Drogon" TYPE FILE FILES "/home/ljh/project/soft_course_design/build/_deps/drogon-build/CMakeFiles/Export/9fe51f2b716a6bd37518a903e3e9a4cf/DrogonTargets-debug.cmake")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/ljh/project/soft_course_design/build/_deps/drogon-build/trantor/cmake_install.cmake")
  include("/home/ljh/project/soft_course_design/build/_deps/drogon-build/examples/cmake_install.cmake")
  include("/home/ljh/project/soft_course_design/build/_deps/drogon-build/drogon_ctl/cmake_install.cmake")

endif()

