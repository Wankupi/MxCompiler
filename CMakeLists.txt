cmake_minimum_required(VERSION 3.20)
project(MxCompiler)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(MxAntlr)

include_directories(/usr/include/antlr4-runtime/)
include_directories(MxAntlr/)
include_directories(src/)

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(code ${sources})

target_link_libraries(code MxAntlr)

set(TestRootPath "${PROJECT_SOURCE_DIR}/data/sema")
file(GLOB_RECURSE test_files RELATIVE ${TestRootPath} ${TestRootPath}/**.mx)
foreach (test_file ${test_files})
    if (${test_file} MATCHES ".*lambda.*")
        continue()
    endif ()
    execute_process(COMMAND grep "Verdict: " "${TestRootPath}/${test_file}" COMMAND sed "s/Verdict: //"
            OUTPUT_VARIABLE verdict)
    add_test(NAME ${test_file}
            COMMAND $<TARGET_FILE:code> ${TestRootPath}/${test_file}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/run")
    if (NOT "${verdict}" STREQUAL "Success\n")
        set_tests_properties(${test_file} PROPERTIES WILL_FAIL true)
    endif ()
endforeach ()

enable_testing()
