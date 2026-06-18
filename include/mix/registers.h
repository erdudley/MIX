#ifndef MIX_REGISTERS_H
#define MIX_REGISTERS_H

#include "mix/types.h"

typedef enum mix_register_id {
    MIX_REG_A = 0,
    MIX_REG_X,
    MIX_REG_I1,
    MIX_REG_I2,
    MIX_REG_I3,
    MIX_REG_I4,
    MIX_REG_I5,
    MIX_REG_I6,
    MIX_REG_J
} mix_register_id;

typedef struct mix_registers {
    mix_word r[MIX_REGISTER_COUNT];
    mix_address location_counter;
    mix_address comparison;
    mix_sign overflow_toggle;
    mix_sign comparison_toggle;
} mix_registers;

void mix_registers_init(mix_registers *regs);
void mix_registers_clear(mix_registers *regs);

bool mix_registers_get(const mix_registers *regs, mix_register_id id, mix_word *out);
bool mix_registers_set(mix_registers *regs, mix_register_id id, mix_word word);

void mix_registers_set_comparison(
    mix_registers *regs,
    int32_t left,
    int32_t right);
int mix_registers_comparison(const mix_registers *regs);

#endif /* MIX_REGISTERS_H */