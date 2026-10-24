cmake_minimum_required(VERSION 3.20)
project(EventGamer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/OSCDependencies.cmake)

find_package(tomlplusplus 3.4 CONFIG QUIET)
if(NOT tomlplusplus_FOUND)
    include(FetchContent)
    FetchContent_Declare(tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()

# Native Windows builds may supply the PJPROJECT SDK and its full library list.
set(PJPROJECT_INCLUDE_DIR "" CACHE PATH "PJPROJECT SDK include directory")
set(PJPROJECT_LIBRARIES "" CACHE STRING "PJPROJECT SDK libraries, including dependencies")
add_library(eventgamer_pjproject INTERFACE)
if(PJPROJECT_INCLUDE_DIR AND PJPROJECT_LIBRARIES)
    target_include_directories(eventgamer_pjproject INTERFACE "${PJPROJECT_INCLUDE_DIR}")
    target_link_libraries(eventgamer_pjproject INTERFACE ${PJPROJECT_LIBRARIES})
else()
    set(PJPROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.deps/pjproject/install"
        CACHE PATH "Local PJPROJECT installation prefix")
    # FindPkgConfig searches CMAKE_PREFIX_PATH as well as system installations.
    if(EXISTS "${PJPROJECT_ROOT}/lib/pkgconfig/libpjproject.pc")
        list(PREPEND CMAKE_PREFIX_PATH "${PJPROJECT_ROOT}")
        message(STATUS "Using local PJPROJECT prefix: ${PJPROJECT_ROOT}")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PJPROJECT REQUIRED libpjproject)
    target_include_directories(eventgamer_pjproject INTERFACE ${PJPROJECT_INCLUDE_DIRS})
    target_compile_options(eventgamer_pjproject INTERFACE ${PJPROJECT_CFLAGS_OTHER})
    # PJPROJECT commonly installs static libraries, so include private dependencies.
    target_link_directories(eventgamer_pjproject INTERFACE ${PJPROJECT_STATIC_LIBRARY_DIRS})
    target_link_libraries(eventgamer_pjproject INTERFACE ${PJPROJECT_STATIC_LIBRARIES})
    target_link_options(eventgamer_pjproject INTERFACE ${PJPROJECT_STATIC_LDFLAGS_OTHER})
endif()

add_library(eventgamer_osc STATIC
    src/osc_transport.cpp src/osc_transport.h
    src/game_protocol.cpp src/game_protocol.h
)
target_compile_features(eventgamer_osc PUBLIC cxx_std_17)
set_target_properties(eventgamer_osc PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(eventgamer_osc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(eventgamer_osc PRIVATE eventgamer_asio eventgamer_oscpack)

add_executable(EventGamer
    src/main.cpp src/config.cpp src/config.h
    src/sip.cpp src/sip.h src/shutdown.cpp src/shutdown.h
    src/call.cpp src/call.h
    src/client.h src/logic.h
    src/queue_logic.cpp src/queue_logic.h
    src/game_logic.cpp src/game_logic.h
)
target_compile_features(EventGamer PRIVATE cxx_std_17)
set_target_properties(EventGamer PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(EventGamer PRIVATE tomlplusplus::tomlplusplus eventgamer_pjproject
    eventgamer_osc)

include(CTest)
if(BUILD_TESTING)
    add_executable(osc_protocol_test tests/osc_protocol_test.cpp)
    target_link_libraries(osc_protocol_test PRIVATE eventgamer_osc eventgamer_asio eventgamer_oscpack)
    add_test(NAME osc_protocol_ipv4 COMMAND osc_protocol_test ipv4)
    add_test(NAME osc_protocol_ipv6 COMMAND osc_protocol_test ipv6)
    add_test(NAME osc_protocol_auto COMMAND osc_protocol_test auto)
    set_tests_properties(osc_protocol_ipv6 PROPERTIES SKIP_RETURN_CODE 77)
    set_tests_properties(osc_protocol_ipv4 osc_protocol_ipv6 osc_protocol_auto PROPERTIES TIMEOUT 10)
endif()
if(MSVC)
    target_compile_options(EventGamer PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(EventGamer PRIVATE -Wall -Wextra -Wpedantic)
endif()
