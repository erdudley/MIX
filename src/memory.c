#include "mix/memory.h"

#include <string.h>

static bool mix_address_valid(mix_address addr)
{
    return addr.value < MIX_MEMORY_SIZE;
}

void mix_memory_init(mix_memory *mem)
{
    mix_memory_clear(mem);
}

void mix_memory_clear(mix_memory *mem)
{
    memset(mem, 0, sizeof(*mem));
}

bool mix_memory_store(mix_memory *mem, mix_address addr, mix_word word)
{
    if (mem == NULL || !mix_address_valid(addr)) {
        return false;
    }
    mem->cells[addr.value] = word;
    return true;
}

bool mix_memory_load(const mix_memory *mem, mix_address addr, mix_word *out)
{
    if (mem == NULL || out == NULL || !mix_address_valid(addr)) {
        return false;
    }
    *out = mem->cells[addr.value];
    return true;
}

mix_word mix_word_from_value(int32_t value)
{
    mix_word word;
    uint32_t magnitude;

    word.sign = (value < 0) ? MIX_NEGATIVE : MIX_POSITIVE;
    magnitude = (value < 0) ? (uint32_t)(-value) : (uint32_t)value;

    for (int i = MIX_WORD_BYTES - 1; i >= 0; --i) {
        word.bytes[i] = (uint8_t)(magnitude % 64U);
        magnitude /= 64U;
    }

    return word;
}

int32_t mix_word_to_value(mix_word word)
{
    int32_t value = 0;

    for (int i = 0; i < MIX_WORD_BYTES; ++i) {
        value = (value * 64) + word.bytes[i];
    }

    return (word.sign == MIX_NEGATIVE) ? -value : value;
}

static uint8_t mix_word_byte_at(const mix_word *word, uint8_t mix_index)
{
    if (mix_index == 0U) {
        return 0U;
    }

    return word->bytes[mix_index - 1U];
}

static void mix_word_set_byte_at(mix_word *word, uint8_t mix_index, uint8_t value)
{
    if (mix_index == 0U) {
        return;
    }

    word->bytes[mix_index - 1U] = value;
}

mix_word mix_word_get_field(mix_word word, mix_field field)
{
    mix_word result = {0};

    if (field.l == 0U && field.r == 5U) {
        return word;
    }

    result.sign = (field.l == 0U) ? word.sign : MIX_POSITIVE;

    for (uint8_t mix_index = field.l; mix_index <= field.r; ++mix_index) {
        if (mix_index == 0U) {
            result.sign = word.sign;
            continue;
        }

        result.bytes[mix_index - 1U] = mix_word_byte_at(&word, mix_index);
    }

    return result;
}

mix_word mix_word_set_field(mix_word word, mix_field field, mix_word value)
{
    if (field.l == 0U && field.r == 5U) {
        return value;
    }

    for (uint8_t mix_index = field.l; mix_index <= field.r; ++mix_index) {
        if (mix_index == 0U) {
            word.sign = value.sign;
            continue;
        }

        mix_word_set_byte_at(&word, mix_index, value.bytes[mix_index - 1U]);
    }

    return word;
}

int32_t mix_word_to_index(mix_word word)
{
    int32_t value = (int32_t)word.bytes[3] * 64 + (int32_t)word.bytes[4];

    if (word.sign == MIX_NEGATIVE) {
        value = -value;
    }

    return value;
}

mix_word mix_word_from_index(int32_t value)
{
    mix_word word;
    uint32_t magnitude;

    word.sign = (value < 0) ? MIX_NEGATIVE : MIX_POSITIVE;
    magnitude = (value < 0) ? (uint32_t)(-value) : (uint32_t)value;

    memset(word.bytes, 0, sizeof(word.bytes));
    word.bytes[4] = (uint8_t)(magnitude % 64U);
    word.bytes[3] = (uint8_t)((magnitude / 64U) % 64U);

    return word;
}

mix_word mix_word_shift_field_right(mix_word word, mix_field field)
{
    mix_word result = {0};
    int width = (int)field.r - (int)field.l + 1;
    int dest = MIX_WORD_BYTES - width;

    if (field.l == 0U && field.r == 5U) {
        return word;
    }

    result.sign = (field.l == 0U) ? word.sign : MIX_POSITIVE;

    for (uint8_t mix_index = field.l; mix_index <= field.r; ++mix_index) {
        if (mix_index == 0U) {
            result.sign = word.sign;
            continue;
        }

        if (dest >= 0 && dest < MIX_WORD_BYTES) {
            result.bytes[dest++] = word.bytes[mix_index - 1U];
        }
    }

    return result;
}

mix_word mix_word_shift_field_left(mix_word word, mix_field field, mix_word value)
{
    mix_word result = {0};
    int width = (int)field.r - (int)field.l + 1;
    int src = MIX_WORD_BYTES - width;

    (void)word;

    if (field.l == 0U && field.r == 5U) {
        return value;
    }

    result.sign = (field.l == 0U) ? value.sign : MIX_POSITIVE;

    for (uint8_t mix_index = field.l; mix_index <= field.r; ++mix_index) {
        if (mix_index == 0U) {
            result.sign = value.sign;
            continue;
        }

        if (src >= 0 && src < MIX_WORD_BYTES) {
            result.bytes[mix_index - 1U] = value.bytes[src++];
        }
    }

    return result;
}