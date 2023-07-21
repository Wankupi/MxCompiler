cmake_minimum_required(VERSION 3.20)
project(MxCompiler)

# set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/out")

include_directories(/usr/include/antlr4-runtime/)
include_directories(MxAntlr/)

add_subdirectory(MxAntlr)

file(GLOB_RECURSE sources src/*.cpp)
add_executable(code ${sources})

target_link_libraries(code MxAntlr)
