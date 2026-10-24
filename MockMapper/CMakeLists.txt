cmake_minimum_required(VERSION 3.20)
project(MockMapper VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(../EventGamer/cmake/OSCDependencies.cmake)

add_executable(MockMapper
    src/main.cpp
    ../EventGamer/src/osc_transport.cpp
    ../EventGamer/src/shutdown.cpp
)
target_compile_features(MockMapper PRIVATE cxx_std_17)
set_target_properties(MockMapper PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(MockMapper PRIVATE ../EventGamer/src)
target_link_libraries(MockMapper PRIVATE eventgamer_asio eventgamer_oscpack)
if(MSVC)
    target_compile_options(MockMapper PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(MockMapper PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME mock_mapper_loopback
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/loopback.py"
                    "$<TARGET_FILE:MockMapper>")
        set_tests_properties(mock_mapper_loopback PROPERTIES TIMEOUT 30)
    endif()
endif()
