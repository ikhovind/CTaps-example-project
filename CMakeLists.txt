cmake_minimum_required(VERSION 3.21)

project(CTapsExampleProject C)

include(FetchContent)

FetchContent_Declare(
        CTaps 
        GIT_REPOSITORY https://github.com/ikhovind/ctaps.git
        GIT_TAG 5823c4438cbd0d126e82d8c60bf773aa513ff7d5
)

FetchContent_MakeAvailable(CTaps)

add_executable(example_client example_client.c)
target_link_libraries(example_client PRIVATE CTaps)

add_executable(example_server example_server.c)
target_link_libraries(example_server PRIVATE CTaps)
