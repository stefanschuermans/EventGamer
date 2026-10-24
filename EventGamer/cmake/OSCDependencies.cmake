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
