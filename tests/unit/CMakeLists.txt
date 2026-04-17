# CMakeLists.txt for IPAd Unit Tests

cmake_minimum_required(VERSION 3.13...3.19 FATAL_ERROR)

# Build unit test for ipa_buf
add_executable(test_ipa_buf test_ipa_buf.c)
target_include_directories(test_ipa_buf PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/ipa/libipa
)
target_link_libraries(test_ipa_buf PRIVATE libipa http scard pcsclite curl)

# Add test
add_test(NAME test_ipa_buf COMMAND test_ipa_buf)

# Set properties for code coverage (optional)
if(ENABLE_COVERAGE)
    target_compile_options(test_ipa_buf PRIVATE --coverage)
    target_link_options(test_ipa_buf PRIVATE --coverage)
endif()
