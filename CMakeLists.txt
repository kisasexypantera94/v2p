cmake_minimum_required(VERSION 3.15)
project(v2p C)

set(CMAKE_C_STANDARD 11)

add_library(v2p v2p.c utils.c)

add_subdirectory(tests)