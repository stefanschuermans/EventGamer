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

include(FetchContent)

find_path(ASIO_INCLUDE_DIR asio.hpp)
if(NOT ASIO_INCLUDE_DIR)
    FetchContent_Declare(asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR eventgamer-no-build
    )
    FetchContent_MakeAvailable(asio)
    set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include")
endif()
find_package(Threads REQUIRED)
add_library(eventgamer_asio INTERFACE)
target_include_directories(eventgamer_asio SYSTEM INTERFACE "${ASIO_INCLUDE_DIR}")
target_compile_definitions(eventgamer_asio INTERFACE ASIO_STANDALONE)
target_link_libraries(eventgamer_asio INTERFACE Threads::Threads)
if(WIN32)
    target_link_libraries(eventgamer_asio INTERFACE ws2_32 mswsock)
endif()

find_path(OSCPACK_INCLUDE_DIR osc/OscReceivedElements.h PATH_SUFFIXES oscpack)
find_library(OSCPACK_LIBRARY NAMES oscpack)
if(OSCPACK_INCLUDE_DIR AND OSCPACK_LIBRARY)
    add_library(eventgamer_oscpack INTERFACE)
    target_include_directories(eventgamer_oscpack SYSTEM INTERFACE "${OSCPACK_INCLUDE_DIR}")
    target_link_libraries(eventgamer_oscpack INTERFACE "${OSCPACK_LIBRARY}")
else()
    FetchContent_Declare(oscpack
        GIT_REPOSITORY https://github.com/RossBencina/oscpack.git
        GIT_TAG 3e7e3a102e316d75fe176885a6dd08f216f2eac0
        SOURCE_SUBDIR eventgamer-no-build
    )
    FetchContent_MakeAvailable(oscpack)
    # Only the codec is needed; UDP sockets are supplied by Asio.
    add_library(eventgamer_oscpack STATIC
        "${oscpack_SOURCE_DIR}/osc/OscTypes.cpp"
        "${oscpack_SOURCE_DIR}/osc/OscReceivedElements.cpp"
        "${oscpack_SOURCE_DIR}/osc/OscOutboundPacketStream.cpp"
        "${oscpack_SOURCE_DIR}/osc/OscPrintReceivedElements.cpp"
    )
    target_include_directories(eventgamer_oscpack SYSTEM PUBLIC "${oscpack_SOURCE_DIR}")
endif()
