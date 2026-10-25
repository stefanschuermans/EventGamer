# EventGamer - telephone controls for Snake in MadMapper.
# Copyright (C) 2026 Stefan Schuermans
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

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
