add_executable(mix-tests
    test_main.c
    test_memory.c
    test_registers.c
    test_interpreter.c
    test_josephus.c
    test_benchmark.c
    test_benchmark3.c
)

target_compile_definitions(mix-tests PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(mix-tests PRIVATE mix::mix)
mix_set_warnings(mix-tests)

add_test(NAME unit COMMAND mix-tests)
set_tests_properties(unit PROPERTIES LABELS "unit")