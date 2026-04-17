# CMake generated Testfile for 
# Source directory: /workspace/tests/activation_code
# Build directory: /workspace/build/tests/activation_code
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[activation_code_test]=] "sh" "-c" "/workspace/build/tests/activation_code/activation_code_test > activation_code_test.out 2> activation_code_test.err")
set_tests_properties([=[activation_code_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/activation_code/CMakeLists.txt;16;add_test;/workspace/tests/activation_code/CMakeLists.txt;0;")
add_test([=[activation_code_compare_stdout]=] "/usr/bin/cmake" "-E" "compare_files" "/workspace/build/tests/activation_code/activation_code_test.out" "/workspace/tests/activation_code/activation_code_test.ok")
set_tests_properties([=[activation_code_compare_stdout]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/activation_code/CMakeLists.txt;19;add_test;/workspace/tests/activation_code/CMakeLists.txt;0;")
add_test([=[activation_code_compare_stderr]=] "/usr/bin/cmake" "-E" "compare_files" "/workspace/build/tests/activation_code/activation_code_test.err" "/workspace/tests/activation_code/activation_code_test.err")
set_tests_properties([=[activation_code_compare_stderr]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/activation_code/CMakeLists.txt;24;add_test;/workspace/tests/activation_code/CMakeLists.txt;0;")
