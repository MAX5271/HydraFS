cmake_minimum_required(VERSION 3.18)
project(HydraFS LANGUAGES CXX)

# Enforce modern C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the required libraries
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED) # NEW: Find Protobuf

# NEW: Automatically compile our .proto blueprint into C++ code
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/proto/hydra.proto)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # NEW: Include the generated protobuf files

# Create the executable, compiling BOTH our main.cpp AND the generated protobuf code
add_executable(hydra_worker src/worker/main.cpp ${PROTO_SRCS})

# Link all libraries (Notice we added protobuf::libprotobuf)
target_link_libraries(hydra_worker Threads::Threads protobuf::libprotobuf)