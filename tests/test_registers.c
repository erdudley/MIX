#include "mix/memory.h"
#include "mix/registers.h"
#include "test.h"

MIX_TEST(test_registers_set_get_roundtrip)
{
    mix_registers regs;
    mix_word word = mix_word_from_value(999);

    mix_registers_init(&regs);
    MIX_ASSERT_TRUE(mix_registers_set(&regs, MIX_REG_A, word));

    mix_word loaded = {0};
    MIX_ASSERT_TRUE(mix_registers_get(&regs, MIX_REG_A, &loaded));
    MIX_ASSERT_EQ_INT(999, mix_word_to_value(loaded));
}

MIX_TEST(test_registers_reject_invalid_id)
{
    mix_registers regs;
    mix_word word = mix_word_from_value(1);

    mix_registers_init(&regs);
    MIX_ASSERT_FALSE(mix_registers_set(&regs, MIX_REGISTER_COUNT, word));
}

const mix_test_case mix_registers_tests[] = {
    {"set_get_roundtrip", test_registers_set_get_roundtrip},
    {"reject_invalid_id", test_registers_reject_invalid_id},
};

const size_t mix_registers_test_count =
    sizeof(mix_registers_tests) / sizeof(mix_registers_tests[0]);