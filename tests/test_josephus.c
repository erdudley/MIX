#include "mix/mix.h"
#include "test.h"

#include <stdio.h>
#include <string.h>

#define MIX_JOSEPHUS_PATH SOURCE_DIR "/tests/data/1.mix"
#define MIX_JOSEPHUS_EXPECTED \
    "15 12 22 8 16 11 23 21 3 5 1 17 10 7 24 19 20 18 9 14 4 2 13 6"

MIX_TEST(test_josephus_program_output)
{
    mix_interpreter vm;
    mix_assemble_result assembled;
    mix_run_result run_result;
    const mix_io *io;

    assembled = mix_assemble_file(&vm, MIX_JOSEPHUS_PATH);
    if (!assembled.ok) {
        fprintf(stderr, "assemble failed: %s\n", assembled.error);
        MIX_ASSERT_TRUE(assembled.ok);
        return;
    }

    run_result = mix_interpreter_run(&vm);
    MIX_ASSERT_EQ_INT(MIX_RUN_HALT, run_result);

    io = mix_interpreter_io(&vm);
    MIX_ASSERT_TRUE(io != NULL);
    MIX_ASSERT_TRUE(strcmp(MIX_JOSEPHUS_EXPECTED, mix_io_cstr(io)) == 0);
}

const mix_test_case mix_josephus_tests[] = {
    {"program_output", test_josephus_program_output},
};

const size_t mix_josephus_test_count =
    sizeof(mix_josephus_tests) / sizeof(mix_josephus_tests[0]);