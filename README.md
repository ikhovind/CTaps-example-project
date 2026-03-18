# CTaps example project

A minimal example project showing how to use [CTaps](https://github.com/ikhovind/CTaps)
as a dependency in your projects.

CTaps provides an asynchronous, callback-based interface for network connections, abstracting over TCP, UDP, and QUIC protocols.

CTaps requires glib to be installed on your system:

```bash
sudo apt-get update && sudo apt-get install -y libglib2.0-dev
```

other dependencies are handled by CTaps itself and fetched automatically.
CTaps can be fetched using FetchContent like this in your cmake project:

```CMake
FetchContent_Declare(
        CTaps 
        GIT_REPOSITORY https://github.com/ikhovind/ctaps.git
        GIT_TAG 5823c4438cbd0d126e82d8c60bf773aa513ff7d5
)

FetchContent_MakeAvailable(CTaps)
```

See the [example-ctaps-downstream-build](.github/example-ctaps-downstream-build.yml)
workflow to see exactly how this project is built.
