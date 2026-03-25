# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/ljh/project/soft_course_design/build/_deps/hiredis-src"
  "/home/ljh/project/soft_course_design/build/_deps/hiredis-build"
  "/home/ljh/project/soft_course_design/build/_deps/hiredis-subbuild/hiredis-populate-prefix"
  "/home/ljh/project/soft_course_design/build/_deps/hiredis-subbuild/hiredis-populate-prefix/tmp"
  "/home/ljh/project/soft_course_design/build/_deps/hiredis-subbuild/hiredis-populate-prefix/src/hiredis-populate-stamp"
  "/home/ljh/project/soft_course_design/build/_deps/hiredis-subbuild/hiredis-populate-prefix/src"
  "/home/ljh/project/soft_course_design/build/_deps/hiredis-subbuild/hiredis-populate-prefix/src/hiredis-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/ljh/project/soft_course_design/build/_deps/hiredis-subbuild/hiredis-populate-prefix/src/hiredis-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/ljh/project/soft_course_design/build/_deps/hiredis-subbuild/hiredis-populate-prefix/src/hiredis-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
