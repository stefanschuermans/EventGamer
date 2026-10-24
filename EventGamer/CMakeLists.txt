cmake_minimum_required(VERSION 3.20)
project(EventGamer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_executable(EventGamer
    src/main.cpp src/config.cpp src/config.h
    src/sip.cpp src/sip.h src/shutdown.cpp src/shutdown.h
)
target_compile_features(EventGamer PRIVATE cxx_std_17)
set_target_properties(EventGamer PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(EventGamer PRIVATE tomlplusplus::tomlplusplus eventgamer_pjproject)
if(MSVC)
    target_compile_options(EventGamer PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(EventGamer PRIVATE -Wall -Wextra -Wpedantic)
endif()
