cmake_minimum_required(VERSION 3.16)

project(lab2_zirolio_ctrl)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OPT_LEVEL "3" CACHE STRING "Optimization level: 0 or 3")

if(NOT OPT_LEVEL MATCHES "^(0|3)$")
    message(FATAL_ERROR "OPT_LEVEL must be 0 or 3")
endif()

add_executable(main
    src/main.cpp
    src/data.cpp
    src/benchmark.cpp
    src/color.hpp
)