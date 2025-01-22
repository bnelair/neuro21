cmake_minimum_required(VERSION 3.30)
project(neuro21_cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

function(add_google_test TARGET_NAME TEST_SRC)
    add_executable(${TARGET_NAME} ${TEST_SRC})
    target_link_libraries(${TARGET_NAME} gtest gtest_main)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()

# Add the main application
add_executable(neuro21_cpp
        main.cpp
        src/utils.cpp
        src/gorilla_compression.cpp
        src/data_handlers.cpp
)

# Enable testing
enable_testing()

# Add the Google Test subdirectory
#add_subdirectory(google_tests/lib)
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

################################################
################################################
################################################

add_google_test(utils_tests
        google_tests/test_utils.cpp
        src/utils.cpp
)

add_google_test(gorilla_compression_tests
        google_tests/test_gorilla_compression.cpp
        src/gorilla_compression.cpp
        src/utils.cpp
)

add_google_test(data_handlers_tests
        google_tests/test_data_handlers.cpp
        src/data_handlers.cpp
)







