cmake_minimum_required(VERSION 3.30)
project(neuro21_cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

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
add_subdirectory(google_tests/lib)

################################################
################################################
################################################
# Utils tests
add_executable(utils_tests
        google_tests/test_utils.cpp
        src/utils.cpp
)
target_link_libraries(utils_tests gtest gtest_main)
add_test(NAME UtilsTests COMMAND utils_tests)

################################################
################################################
################################################
# compression tests
add_executable(gorilla_compression_tests
        google_tests/test_gorilla_compression.cpp
        src/gorilla_compression.cpp
        src/utils.cpp
)
target_link_libraries(gorilla_compression_tests gtest gtest_main)
add_test(NAME GorillaCompressionTests COMMAND gorilla_compression_tests)

################################################
################################################
################################################
add_executable(data_handlers_tests
        google_tests/test_data_handlers.cpp
        src/data_handlers.cpp
)
target_link_libraries(data_handlers_tests gtest gtest_main)
add_test(NAME DataHandlersTests COMMAND data_handlers_tests)


# Optional: Ensure RPATH settings for macOS to locate gtest libraries at runtime
set_target_properties(gorilla_compression_tests PROPERTIES INSTALL_RPATH "${CMAKE_BINARY_DIR}/google_tests/lib")
set_target_properties(utils_tests PROPERTIES INSTALL_RPATH "${CMAKE_BINARY_DIR}/google_tests/lib")
set_target_properties(data_handlers_tests PROPERTIES INSTALL_RPATH "${CMAKE_BINARY_DIR}/google_tests/lib")








