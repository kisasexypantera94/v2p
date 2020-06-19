cmake_minimum_required(VERSION 3.15)
project(v2p C)

set(CMAKE_C_STANDARD 11)

add_library(v2p src/v2p.c src/legacy.c src/pae.c src/utils.c)
target_include_directories(
        v2p

        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_custom_command(OUTPUT v2p_single.h
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gen_header_only.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)
add_custom_target(
        v2p_single ALL
        DEPENDS v2p_single.h
)

add_subdirectory(tests)
add_subdirectory(examples)