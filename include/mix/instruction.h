#ifndef MIX_INSTRUCTION_H
#define MIX_INSTRUCTION_H

#include "mix/interpreter.h"

typedef struct mix_decoded {
    mix_sign sign;
    int32_t address;
    uint8_t index;
    uint8_t f;
    uint8_t c;
    mix_field field;
} mix_decoded;

mix_decoded mix_instruction_decode(mix_word word);
int32_t mix_effective_address(const mix_interpreter *vm, const mix_decoded *inst);
mix_run_result mix_instruction_execute(mix_interpreter *vm, const mix_decoded *inst);

#endif /* MIX_INSTRUCTION_H */