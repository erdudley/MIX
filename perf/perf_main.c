#include "mix/mix.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct perf_result {
    const char *name;
    double seconds;
    uint64_t iterations;
    double ops_per_sec;
} perf_result;

static double perf_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static uint64_t bench_memory_store_load(uint64_t iterations)
{
    mix_memory mem;
    mix_word word = mix_word_from_value(12345);
    mix_address addr = {.value = 0};

    mix_memory_init(&mem);

    for (uint64_t i = 0; i < iterations; ++i) {
        addr.value = (uint16_t)(i % MIX_MEMORY_SIZE);
        mix_memory_store(&mem, addr, word);
        mix_memory_load(&mem, addr, &word);
    }

    return iterations * 2U;
}

static uint64_t bench_word_conversion(uint64_t iterations)
{
    int32_t value = 1;

    for (uint64_t i = 0; i < iterations; ++i) {
        mix_word word = mix_word_from_value(value);
        value = mix_word_to_value(word) + 1;
    }

    return iterations * 2U;
}

static uint64_t bench_interpreter_steps(uint64_t iterations)
{
    mix_interpreter vm;
    mix_word hlt = {0};

    mix_interpreter_init(&vm);
    hlt.bytes[4] = 5;

    for (uint64_t i = 0; i < iterations; ++i) {
        vm.registers.location_counter.value = 0;
        vm.steps_executed = 0;
        mix_memory_store(&vm.memory, (mix_address){.value = 0}, hlt);
        mix_interpreter_step(&vm);
    }

    return iterations;
}

static perf_result run_benchmark(
    const char *name,
    uint64_t (*fn)(uint64_t),
    uint64_t iterations)
{
    perf_result result;
    double start;
    double end;

    result.name = name;
    result.iterations = iterations;

    start = perf_now();
    result.ops_per_sec = (double)fn(iterations);
    end = perf_now();

    result.seconds = end - start;
    if (result.seconds > 0.0) {
        result.ops_per_sec /= result.seconds;
    } else {
        result.ops_per_sec = 0.0;
    }

    return result;
}

static void write_report(const char *path, perf_result *results, size_t count)
{
    FILE *fp = stdout;

    if (path != NULL && path[0] != '\0') {
        fp = fopen(path, "w");
        if (fp == NULL) {
            fprintf(stderr, "perf: cannot open report '%s'\n", path);
            fp = stdout;
        }
    }

    fprintf(fp, "MIX Performance Report\n");
    fprintf(fp, "======================\n\n");
    fprintf(fp, "%-28s %12s %14s %14s\n",
            "Benchmark", "Iterations", "Time (s)", "Ops/sec");
    fprintf(fp, "%-28s %12s %14s %14s\n",
            "---------", "----------", "--------", "-------");

    for (size_t i = 0; i < count; ++i) {
        fprintf(fp, "%-28s %12llu %14.6f %14.0f\n",
                results[i].name,
                (unsigned long long)results[i].iterations,
                results[i].seconds,
                results[i].ops_per_sec);
    }

    if (fp != stdout) {
        fclose(fp);
        printf("Performance report written to %s\n", path);
    }
}

static void print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s [--report PATH]\n", program);
    fprintf(stderr, "  Run MIX performance benchmarks.\n");
    fprintf(stderr, "  --report PATH   Write text report to PATH (default: stdout)\n");
}

int main(int argc, char **argv)
{
    const char *report_path = NULL;
    perf_result results[3];

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--report") == 0) {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            report_path = argv[++i];
        } else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    results[0] = run_benchmark("memory_store_load", bench_memory_store_load, 500000U);
    results[1] = run_benchmark("word_conversion", bench_word_conversion, 500000U);
    results[2] = run_benchmark("interpreter_step_hlt", bench_interpreter_steps, 200000U);

    write_report(report_path, results, 3);
    return EXIT_SUCCESS;
}