# CMake generated Testfile for 
# Source directory: /home/ljh/project/soft_course_design
# Build directory: /home/ljh/project/soft_course_design/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[BidEngineTest]=] "/home/ljh/project/soft_course_design/build/bin/bid_engine_test")
set_tests_properties([=[BidEngineTest]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ljh/project/soft_course_design/CMakeLists.txt;106;add_test;/home/ljh/project/soft_course_design/CMakeLists.txt;0;")
add_test([=[TimingWheelTest]=] "/home/ljh/project/soft_course_design/build/bin/timing_wheel_test")
set_tests_properties([=[TimingWheelTest]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ljh/project/soft_course_design/CMakeLists.txt;110;add_test;/home/ljh/project/soft_course_design/CMakeLists.txt;0;")
subdirs("_deps/drogon-build")
subdirs("_deps/jwt_cpp-build")
subdirs("_deps/hiredis-build")
subdirs("_deps/googletest-build")
