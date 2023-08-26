cmake_minimum_required(VERSION 3.20)
project(MxCompiler)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_subdirectory(MxAntlr)

include_directories(/usr/include/antlr4-runtime/)
include_directories(MxAntlr/)
include_directories(src/)

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(code ${sources})

target_link_libraries(code MxAntlr)


set(TestdataPath "${PROJECT_SOURCE_DIR}/data")
set(SemaPath "${TestdataPath}/sema")
file(GLOB_RECURSE test_files RELATIVE ${SemaPath} ${SemaPath}/**.mx)
foreach (test_file ${test_files})
    string(REGEX REPLACE ".*/" "" filename "${test_file}")
    set(testname "sema|${filename}")
    if (${test_file} MATCHES ".*lambda.*")
        continue()
    endif ()
    execute_process(COMMAND grep "Verdict: " "${SemaPath}/${test_file}" COMMAND sed "s/Verdict: //"
            OUTPUT_VARIABLE verdict)
    add_test(NAME ${testname}
            COMMAND $<TARGET_FILE:code> ${SemaPath}/${test_file} -fsyntax-only
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/run")
    set_tests_properties(${testname} PROPERTIES LABELS "sema")
    if (NOT "${verdict}" STREQUAL "Success\n")
        set_tests_properties(${testname} PROPERTIES WILL_FAIL true)
    endif ()
endforeach ()

set(CodegenPath "${TestdataPath}/codegen")
file(GLOB_RECURSE test_files RELATIVE ${CodegenPath} ${CodegenPath}/**.mx)
foreach (test_file ${test_files})
    string(REGEX REPLACE ".*/" "" filename "${test_file}")

    set(testname "ir|${filename}")
    add_test(NAME ${testname}
            COMMAND "${PROJECT_SOURCE_DIR}/run-llvm-ir.bash" $<TARGET_FILE:code> ${CodegenPath}/${test_file}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/run")
    set_tests_properties(${testname} PROPERTIES LABELS "ir" TIMEOUT 3)

    set(testname "asm|${filename}")
    add_test(NAME ${testname}
            COMMAND "${PROJECT_SOURCE_DIR}/run-riscv32m.bash" $<TARGET_FILE:code> ${CodegenPath}/${test_file}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/run")
    set_tests_properties(${testname} PROPERTIES LABELS "asm" TIMEOUT 10)
endforeach ()

set(OptPath "${TestdataPath}/optim")
file(GLOB_RECURSE test_files RELATIVE ${OptPath} ${OptPath}/**.mx)
foreach (test_file ${test_files})
    string(REGEX REPLACE ".*/" "" filename "${test_file}")

    set(testname "ir-opt1|${filename}")
    add_test(NAME ${testname}
            COMMAND "${PROJECT_SOURCE_DIR}/run-llvm-ir.bash" $<TARGET_FILE:code> ${OptPath}/${test_file}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/run")
    set_tests_properties(${testname} PROPERTIES LABELS "ir" TIMEOUT 3)

    set(testname "opt1|${filename}")
    add_test(NAME ${testname}
            COMMAND "${PROJECT_SOURCE_DIR}/run-riscv32m.bash" $<TARGET_FILE:code> ${OptPath}/${test_file}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/run")
    set_tests_properties(${testname} PROPERTIES LABELS "opt1" TIMEOUT 40)
endforeach ()

enable_testing()
