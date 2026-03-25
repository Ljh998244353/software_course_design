# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-src"
  "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-build"
  "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-subbuild/jwt_cpp-populate-prefix"
  "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-subbuild/jwt_cpp-populate-prefix/tmp"
  "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-subbuild/jwt_cpp-populate-prefix/src/jwt_cpp-populate-stamp"
  "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-subbuild/jwt_cpp-populate-prefix/src"
  "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-subbuild/jwt_cpp-populate-prefix/src/jwt_cpp-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-subbuild/jwt_cpp-populate-prefix/src/jwt_cpp-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/ljh/project/soft_course_design/build/_deps/jwt_cpp-subbuild/jwt_cpp-populate-prefix/src/jwt_cpp-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
