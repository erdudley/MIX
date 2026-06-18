#include "mix/interpreter.h"
#include "mix/memory.h"
#include "test.h"

MIX_TEST(test_interpreter_halt_on_hlt_opcode)
{
    mix_interpreter vm;
    mix_word hlt = {0};

    mix_interpreter_init(&vm);
    hlt.bytes[3] = 2;
    hlt.bytes[4] = 5;
    MIX_ASSERT_TRUE(mix_memory_store(
        &vm.memory, (mix_address){.value = 0}, hlt));

    MIX_ASSERT_EQ_INT(MIX_RUN_HALT, mix_interpreter_step(&vm));
    MIX_ASSERT_EQ_INT(1, (int)vm.steps_executed);
}

MIX_TEST(test_interpreter_step_limit)
{
    mix_interpreter vm;
    mix_word nop_like = {0};

    mix_interpreter_init(&vm);
    nop_like.bytes[4] = 99;
    MIX_ASSERT_TRUE(mix_memory_store(
        &vm.memory, (mix_address){.value = 0}, nop_like));

    vm.step_limit = 3;
    MIX_ASSERT_EQ_INT(MIX_RUN_INVALID_INSTRUCTION, mix_interpreter_step(&vm));
    MIX_ASSERT_EQ_INT(MIX_RUN_INVALID_INSTRUCTION, mix_interpreter_step(&vm));
    MIX_ASSERT_EQ_INT(MIX_RUN_INVALID_INSTRUCTION, mix_interpreter_step(&vm));
    MIX_ASSERT_EQ_INT(MIX_RUN_STEP_LIMIT, mix_interpreter_step(&vm));
}

const mix_test_case mix_interpreter_tests[] = {
    {"halt_on_hlt_opcode", test_interpreter_halt_on_hlt_opcode},
    {"step_limit", test_interpreter_step_limit},
};

const size_t mix_interpreter_test_count =
    sizeof(mix_interpreter_tests) / sizeof(mix_interpreter_tests[0]);