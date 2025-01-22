cmake_minimum_required(VERSION 3.30)
project(Google_tests)

add_subdirectory(lib)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(Google_Tests_run test_gorilla_compression.cpp
        test_utils.cpp
        test_data_handlers.cpp
)
target_link_libraries(Google_Tests_run gtest gtest_main)