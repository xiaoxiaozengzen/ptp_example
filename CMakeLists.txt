cmake_minimum_required(VERSION 3.23)
project(ptp_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES
    src/*.cpp
)
add_executable(ptp_example ${SOURCES})