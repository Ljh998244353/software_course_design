# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/ljh/project/soft_course_design/build/_deps/drogon-src"
  "/home/ljh/project/soft_course_design/build/_deps/drogon-build"
  "/home/ljh/project/soft_course_design/build/_deps/drogon-subbuild/drogon-populate-prefix"
  "/home/ljh/project/soft_course_design/build/_deps/drogon-subbuild/drogon-populate-prefix/tmp"
  "/home/ljh/project/soft_course_design/build/_deps/drogon-subbuild/drogon-populate-prefix/src/drogon-populate-stamp"
  "/home/ljh/project/soft_course_design/build/_deps/drogon-subbuild/drogon-populate-prefix/src"
  "/home/ljh/project/soft_course_design/build/_deps/drogon-subbuild/drogon-populate-prefix/src/drogon-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/ljh/project/soft_course_design/build/_deps/drogon-subbuild/drogon-populate-prefix/src/drogon-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/ljh/project/soft_course_design/build/_deps/drogon-subbuild/drogon-populate-prefix/src/drogon-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
