#include "mix/mix.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIX_BENCHMARK3_PATH SOURCE_DIR "/tests/data/3.mix"
#define MIX_BENCHMARK3_EXPECTED_PATH SOURCE_DIR "/tests/data/3.expected"

static char *mix_read_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    char *buffer = NULL;
    long size = 0;

    if (fp == NULL) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    rewind(fp);
    buffer = (char *)malloc((size_t)size + 1U);
    if (buffer == NULL) {
        fclose(fp);
        return NULL;
    }

    if (fread(buffer, 1U, (size_t)size, fp) != (size_t)size) {
        free(buffer);
        fclose(fp);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(fp);
    return buffer;
}

MIX_TEST(test_benchmark3_program_output)
{
    mix_interpreter vm;
    mix_assemble_result assembled;
    mix_run_result run_result;
    const mix_io *io;
    char *expected = mix_read_file(MIX_BENCHMARK3_EXPECTED_PATH);

    if (expected == NULL) {
        fprintf(stderr, "cannot read expected output file\n");
        MIX_ASSERT_TRUE(expected != NULL);
        return;
    }

    assembled = mix_assemble_file(&vm, MIX_BENCHMARK3_PATH);
    if (!assembled.ok) {
        fprintf(stderr, "assemble failed: %s\n", assembled.error);
        free(expected);
        MIX_ASSERT_TRUE(assembled.ok);
        return;
    }

    run_result = mix_interpreter_run(&vm);

    MIX_ASSERT_EQ_INT(MIX_RUN_HALT, run_result);

    io = mix_interpreter_io(&vm);
    MIX_ASSERT_TRUE(io != NULL);
    MIX_ASSERT_TRUE(strcmp(expected, mix_io_cstr(io)) == 0);
    free(expected);
}

const mix_test_case mix_benchmark3_tests[] = {
    {"program_output", test_benchmark3_program_output},
};

const size_t mix_benchmark3_test_count =
    sizeof(mix_benchmark3_tests) / sizeof(mix_benchmark3_tests[0]);