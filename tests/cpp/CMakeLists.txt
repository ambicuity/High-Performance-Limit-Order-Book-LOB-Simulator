cmake_minimum_required(VERSION 3.15)

# Try to find GoogleTest
find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
    # If not found, fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Test executables
add_executable(test_book_basic test_book_basic.cpp)
target_link_libraries(test_book_basic PRIVATE lob_core GTest::gtest_main)
target_include_directories(test_book_basic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/include)

add_executable(test_engine_fills test_engine_fills.cpp)
target_link_libraries(test_engine_fills PRIVATE lob_core GTest::gtest_main)
target_include_directories(test_engine_fills PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/include)

# Add tests
include(GoogleTest)
gtest_discover_tests(test_book_basic)
gtest_discover_tests(test_engine_fills)
