cmake_minimum_required(VERSION 3.21)

project(CTapsExampleProject C)

include(FetchContent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FetchContent_Declare(
        CTaps 
        GIT_REPOSITORY https://github.com/ikhovind/ctaps.git
        GIT_TAG v0.1.0
)

FetchContent_MakeAvailable(CTaps)

add_executable(example_client example_client.c)
target_link_libraries(example_client PRIVATE CTaps)

add_executable(example_quic_server example_quic_server.c)
target_link_libraries(example_quic_server PRIVATE CTaps)

add_executable(example_tcp_server example_tcp_server.c)
target_link_libraries(example_tcp_server PRIVATE CTaps)

configure_file(resources/cert.pem resources/cert.pem COPYONLY)
configure_file(resources/key.pem resources/key.pem COPYONLY)
