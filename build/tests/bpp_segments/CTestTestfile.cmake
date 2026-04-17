# CMake generated Testfile for 
# Source directory: /workspace/tests/bpp_segments
# Build directory: /workspace/build/tests/bpp_segments
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[bpp_segments_test]=] "sh" "-c" "/workspace/build/tests/bpp_segments/bpp_segments_test ../../../tests/bpp_segments/bpp.ber > bpp_segments_test.out 2> bpp_segments_test.err")
set_tests_properties([=[bpp_segments_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/bpp_segments/CMakeLists.txt;16;add_test;/workspace/tests/bpp_segments/CMakeLists.txt;0;")
add_test([=[bpp_segments_compare_stdout]=] "/usr/bin/cmake" "-E" "compare_files" "/workspace/build/tests/bpp_segments/bpp_segments_test.out" "/workspace/tests/bpp_segments/bpp_segments_test.ok")
set_tests_properties([=[bpp_segments_compare_stdout]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/bpp_segments/CMakeLists.txt;19;add_test;/workspace/tests/bpp_segments/CMakeLists.txt;0;")
add_test([=[bpp_segments_compare_stderr]=] "/usr/bin/cmake" "-E" "compare_files" "/workspace/build/tests/bpp_segments/bpp_segments_test.err" "/workspace/tests/bpp_segments/bpp_segments_test.err")
set_tests_properties([=[bpp_segments_compare_stderr]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/bpp_segments/CMakeLists.txt;24;add_test;/workspace/tests/bpp_segments/CMakeLists.txt;0;")
