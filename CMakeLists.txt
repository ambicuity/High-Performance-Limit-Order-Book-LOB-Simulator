cmake_minimum_required(VERSION 3.15)
project(lobsim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -march=native)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
elseif(MSVC)
    add_compile_options(/W4 /O2)
endif()

# Core library
add_library(lob_core STATIC
    cpp/src/Price.cpp
    cpp/src/LimitBook.cpp
    cpp/src/MatchingEngine.cpp
    cpp/src/MarketDataFeed.cpp
)

target_include_directories(lob_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include
)

# Python bindings
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
if(BUILD_PYTHON_BINDINGS)
    find_package(Python3 COMPONENTS Interpreter Development)
    
    # Try to find pybind11
    find_package(pybind11 CONFIG QUIET)
    if(NOT pybind11_FOUND)
        # If not found, try to use Python's pybind11
        if(Python3_FOUND)
            execute_process(
                COMMAND "${Python3_EXECUTABLE}" -c "import pybind11; print(pybind11.get_cmake_dir())"
                OUTPUT_VARIABLE pybind11_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(pybind11_DIR)
                find_package(pybind11 CONFIG PATHS "${pybind11_DIR}")
            endif()
        endif()
    endif()
    
    if(pybind11_FOUND)
        pybind11_add_module(lobsim cpp/src/pybind_module.cpp)
        target_link_libraries(lobsim PRIVATE lob_core)
        target_include_directories(lobsim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include)
        
        # Install target
        install(TARGETS lobsim DESTINATION bindings/python)
    else()
        message(WARNING "pybind11 not found, skipping Python bindings")
    endif()
endif()

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/cpp)
endif()

# Benchmarks
option(BUILD_BENCHMARKS "Build benchmarks" ON)
if(BUILD_BENCHMARKS)
    add_executable(bench_match_engine benchmarks/bench_match_engine.cpp)
    target_link_libraries(bench_match_engine PRIVATE lob_core)
    target_include_directories(bench_match_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include)
endif()
