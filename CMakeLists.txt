cmake_minimum_required(VERSION 3.16)
project(stm32g474_fw_sim C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -Werror -O2)

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.c)
add_executable(stm32g474_fw_sim ${SOURCES})
