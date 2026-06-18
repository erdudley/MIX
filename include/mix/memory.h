#ifndef MIX_MEMORY_H
#define MIX_MEMORY_H

#include "mix/types.h"

typedef struct mix_memory {
    mix_word cells[MIX_MEMORY_SIZE];
} mix_memory;

void mix_memory_init(mix_memory *mem);
void mix_memory_clear(mix_memory *mem);

bool mix_memory_store(mix_memory *mem, mix_address addr, mix_word word);
bool mix_memory_load(const mix_memory *mem, mix_address addr, mix_word *out);

mix_word mix_word_from_value(int32_t value);
int32_t mix_word_to_value(mix_word word);

mix_word mix_word_get_field(mix_word word, mix_field field);
mix_word mix_word_set_field(mix_word word, mix_field field, mix_word value);

int32_t mix_word_to_index(mix_word word);
mix_word mix_word_from_index(int32_t value);
mix_word mix_word_shift_field_right(mix_word word, mix_field field);
mix_word mix_word_shift_field_left(mix_word word, mix_field field, mix_word value);

#endif /* MIX_MEMORY_H */