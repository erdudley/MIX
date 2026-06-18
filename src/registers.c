#include "mix/registers.h"

#include <string.h>

static bool mix_register_id_valid(mix_register_id id)
{
    return id >= MIX_REG_A && id < MIX_REGISTER_COUNT;
}

void mix_registers_init(mix_registers *regs)
{
    mix_registers_clear(regs);
}

void mix_registers_clear(mix_registers *regs)
{
    memset(regs, 0, sizeof(*regs));
    regs->overflow_toggle = MIX_POSITIVE;
    regs->comparison_toggle = MIX_POSITIVE;
}

bool mix_registers_get(const mix_registers *regs, mix_register_id id, mix_word *out)
{
    if (regs == NULL || out == NULL || !mix_register_id_valid(id)) {
        return false;
    }
    *out = regs->r[id];
    return true;
}

bool mix_registers_set(mix_registers *regs, mix_register_id id, mix_word word)
{
    if (regs == NULL || !mix_register_id_valid(id)) {
        return false;
    }

    if (id >= MIX_REG_I1 && id <= MIX_REG_I6) {
        mix_word cleared = {0};
        cleared.sign = word.sign;
        cleared.bytes[3] = word.bytes[3];
        cleared.bytes[4] = word.bytes[4];
        regs->r[id] = cleared;
        return true;
    }

    regs->r[id] = word;
    return true;
}

void mix_registers_set_comparison(
    mix_registers *regs,
    int32_t left,
    int32_t right)
{
    if (regs == NULL) {
        return;
    }

    if (left < right) {
        regs->comparison.value = 1U;
    } else if (left == right) {
        regs->comparison.value = 0U;
    } else {
        regs->comparison.value = 2U;
    }
}

int mix_registers_comparison(const mix_registers *regs)
{
    if (regs == NULL) {
        return 0;
    }

    switch (regs->comparison.value) {
    case 1U:
        return -1;
    case 2U:
        return 1;
    default:
        return 0;
    }
}