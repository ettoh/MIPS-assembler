cmake_minimum_required(VERSION 3.13.0)
project(mips-assembler VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mips-assembler)

# source files
target_sources(mips-assembler
    PRIVATE
        main.cpp
)
