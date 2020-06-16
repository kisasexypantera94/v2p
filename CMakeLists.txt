cmake_minimum_required(VERSION 3.15)
project(v2p C)

set(CMAKE_C_STANDARD 11)

add_library(v2p src/v2p.c src/utils.c)
target_include_directories(
        v2p

        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


add_subdirectory(tests)