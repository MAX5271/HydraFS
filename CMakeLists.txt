cmake_minimum_required(VERSION 3.18)
project(HydraFS LANGUAGES CXX)

# Enforce modern C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the networking libraries
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

# Include the source directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Create the executable for the worker node
add_executable(hydra_worker src/worker/main.cpp)

# Link the libraries to the executable
target_link_libraries(hydra_worker Threads::Threads)