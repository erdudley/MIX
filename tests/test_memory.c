#include "mix/memory.h"
#include "test.h"

MIX_TEST(test_memory_store_load_roundtrip)
{
    mix_memory mem;
    mix_word word = mix_word_from_value(12345);
    mix_address addr = {.value = 100};

    mix_memory_init(&mem);
    MIX_ASSERT_TRUE(mix_memory_store(&mem, addr, word));

    mix_word loaded = {0};
    MIX_ASSERT_TRUE(mix_memory_load(&mem, addr, &loaded));
    MIX_ASSERT_EQ_INT(12345, mix_word_to_value(loaded));
}

MIX_TEST(test_memory_rejects_out_of_range)
{
    mix_memory mem;
    mix_word word = mix_word_from_value(1);
    mix_address addr = {.value = MIX_MEMORY_SIZE};

    mix_memory_init(&mem);
    MIX_ASSERT_FALSE(mix_memory_store(&mem, addr, word));
}

MIX_TEST(test_word_field_roundtrip)
{
    mix_word word = {0};
    mix_field field = {.l = 3, .r = 4};
    mix_word patch = {0};

    word.bytes[2] = 10;
    word.bytes[3] = 20;
    patch.bytes[2] = 30;
    patch.bytes[3] = 40;

    word = mix_word_set_field(word, field, patch);
    mix_word extracted = mix_word_get_field(word, field);

    MIX_ASSERT_EQ_INT(30, extracted.bytes[2]);
    MIX_ASSERT_EQ_INT(40, extracted.bytes[3]);
}

MIX_TEST(test_word_negative_values)
{
    mix_word word = mix_word_from_value(-42);
    MIX_ASSERT_EQ_INT(-42, mix_word_to_value(word));
    MIX_ASSERT_TRUE(word.sign == MIX_NEGATIVE);
}

const mix_test_case mix_memory_tests[] = {
    {"store_load_roundtrip", test_memory_store_load_roundtrip},
    {"rejects_out_of_range", test_memory_rejects_out_of_range},
    {"field_roundtrip", test_word_field_roundtrip},
    {"negative_values", test_word_negative_values},
};

const size_t mix_memory_test_count = sizeof(mix_memory_tests) / sizeof(mix_memory_tests[0]);