cmake_minimum_required(VERSION 3.16)
project(mix
    VERSION 0.1.0
    DESCRIPTION "MIX interpreter"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MIX_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(MIX_ENABLE_SANITIZERS "Enable ASan/UBSan in debug builds" OFF)

include(cmake/CompilerWarnings.cmake)
include(cmake/Valgrind.cmake)

# ---------------------------------------------------------------------------
# Core library
# ---------------------------------------------------------------------------
add_library(mix STATIC
    src/memory.c
    src/registers.c
    src/instruction.c
    src/io.c
    src/assembler.c
    src/interpreter.c
)

add_library(mix::mix ALIAS mix)

target_include_directories(mix
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

mix_set_warnings(mix)

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(mix-run src/main.c)
target_link_libraries(mix-run PRIVATE mix::mix)
mix_set_warnings(mix-run)

# ---------------------------------------------------------------------------
# Tests, benchmarks, tooling
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
add_subdirectory(perf)

# Convenience targets (see README for usage)
add_custom_target(run
    COMMAND mix-run
    DEPENDS mix-run
    COMMENT "Run the MIX interpreter"
)

add_custom_target(perf-report
    COMMAND $<TARGET_FILE:mix-perf> --report ${CMAKE_BINARY_DIR}/perf_report.txt
    DEPENDS mix-perf
    COMMENT "Run performance benchmarks and write report"
)

mix_add_valgrind_target(memcheck mix-tests)