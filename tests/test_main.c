#include "test.h"

#include <stddef.h>

int g_mix_test_failures = 0;
int g_mix_test_runs = 0;

extern const mix_test_case mix_memory_tests[];
extern const size_t mix_memory_test_count;

extern const mix_test_case mix_registers_tests[];
extern const size_t mix_registers_test_count;

extern const mix_test_case mix_interpreter_tests[];
extern const size_t mix_interpreter_test_count;

extern const mix_test_case mix_josephus_tests[];
extern const size_t mix_josephus_test_count;

extern const mix_test_case mix_benchmark_tests[];
extern const size_t mix_benchmark_test_count;

extern const mix_test_case mix_benchmark3_tests[];
extern const size_t mix_benchmark3_test_count;

void mix_test_run(const char *suite, const mix_test_case *cases, size_t count)
{
    printf("== %s ==\n", suite);

    for (size_t i = 0; i < count; ++i) {
        int failures_before = g_mix_test_failures;
        ++g_mix_test_runs;
        printf("  RUN  %s\n", cases[i].name);
        cases[i].fn();
        if (g_mix_test_failures == failures_before) {
            printf("  PASS %s\n", cases[i].name);
        }
    }
}

int main(void)
{
    mix_test_run("memory", mix_memory_tests, mix_memory_test_count);
    mix_test_run("registers", mix_registers_tests, mix_registers_test_count);
    mix_test_run("interpreter", mix_interpreter_tests, mix_interpreter_test_count);
    mix_test_run("josephus", mix_josephus_tests, mix_josephus_test_count);
    mix_test_run("benchmark", mix_benchmark_tests, mix_benchmark_test_count);
    mix_test_run("benchmark3", mix_benchmark3_tests, mix_benchmark3_test_count);

    printf("\n%d tests, %d failures\n", g_mix_test_runs, g_mix_test_failures);
    return (g_mix_test_failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}