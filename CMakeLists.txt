cmake_minimum_required(VERSION 3.25.2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
    args
    VERSION 0.0.1
    DESCRIPTION "Command Line Argument Parser"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
add_subdirectory(demo)
