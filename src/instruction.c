#include "mix/instruction.h"

#include <string.h>

static mix_field mix_field_from_f(uint8_t f)
{
    mix_field field = {
        .l = (uint8_t)(f / 8U),
        .r = (uint8_t)(f % 8U),
    };

    return field;
}

mix_decoded mix_instruction_decode(mix_word word)
{
    mix_decoded inst;

    inst.sign = word.sign;
    inst.address = (int32_t)((word.bytes[0] * 64U) + word.bytes[1]);
    if (inst.sign == MIX_NEGATIVE) {
        inst.address = -inst.address;
    }
    inst.index = word.bytes[2] % 8U;
    inst.f = word.bytes[3];
    inst.c = word.bytes[4];
    inst.field = mix_field_from_f(inst.f);
    return inst;
}

int32_t mix_effective_address(const mix_interpreter *vm, const mix_decoded *inst)
{
    int32_t m = inst->address;

    if (inst->index > 0U && inst->index <= 6U) {
        mix_word index_reg;
        if (!mix_registers_get(
                &vm->registers,
                (mix_register_id)(MIX_REG_I1 + (int)inst->index - 1),
                &index_reg)) {
            return m;
        }
        m += mix_word_to_index(index_reg);
    } else if (inst->index == 7U) {
        m += mix_word_to_value(vm->registers.r[MIX_REG_X]);
    } else if (inst->index == 8U) {
        m += mix_word_to_value(vm->registers.r[MIX_REG_A]);
    }

    return m;
}

static bool mix_load_memory(
    const mix_interpreter *vm,
    int32_t m,
    mix_word *out)
{
    if (m < 0 || m >= MIX_MEMORY_SIZE) {
        return false;
    }

    return mix_memory_load(&vm->memory, (mix_address){.value = (uint16_t)m}, out);
}

static bool mix_store_memory(
    mix_interpreter *vm,
    int32_t m,
    mix_word word)
{
    if (m < 0 || m >= MIX_MEMORY_SIZE) {
        return false;
    }

    return mix_memory_store(
        &vm->memory, (mix_address){.value = (uint16_t)m}, word);
}

static mix_word mix_load_memory_field(
    const mix_interpreter *vm,
    int32_t m,
    mix_field field)
{
    mix_word memory_word;
    mix_word extracted;

    if (!mix_load_memory(vm, m, &memory_word)) {
        return (mix_word){0};
    }

    extracted = mix_word_get_field(memory_word, field);
    extracted.sign = (field.l == 5U) ? memory_word.sign : MIX_POSITIVE;
    return mix_word_shift_field_right(extracted, field);
}

static bool mix_store_memory_field(
    mix_interpreter *vm,
    int32_t m,
    mix_field field,
    mix_word value)
{
    mix_word memory_word;

    if (!mix_load_memory(vm, m, &memory_word)) {
        return false;
    }

    memory_word = mix_word_set_field(
        memory_word,
        field,
        mix_word_shift_field_left(memory_word, field, value));
    return mix_store_memory(vm, m, memory_word);
}

static mix_register_id mix_register_from_c(uint8_t c, uint8_t base)
{
    static const mix_register_id map[8] = {
        MIX_REG_A,
        MIX_REG_I1,
        MIX_REG_I2,
        MIX_REG_I3,
        MIX_REG_I4,
        MIX_REG_I5,
        MIX_REG_I6,
        MIX_REG_X,
    };

    return map[(c - base) % 8U];
}

static bool mix_jump(mix_interpreter *vm, int32_t m)
{
    if (m < 0 || m >= MIX_MEMORY_SIZE) {
        return false;
    }

    vm->registers.location_counter.value = (uint16_t)m;
    return true;
}

static void mix_set_jump_address(mix_interpreter *vm)
{
    mix_word jump = {0};
    uint32_t magnitude = vm->registers.location_counter.value;

    jump.sign = MIX_POSITIVE;
    jump.bytes[0] = (uint8_t)((magnitude / 64U) % 64U);
    jump.bytes[1] = (uint8_t)(magnitude % 64U);
    mix_registers_set(&vm->registers, MIX_REG_J, jump);
}

static bool mix_register_positive(mix_register_id id, mix_word word)
{
    if (id >= MIX_REG_I1 && id <= MIX_REG_I6) {
        return mix_word_to_index(word) > 0;
    }

    return mix_word_to_value(word) > 0;
}

static bool mix_register_zero(mix_register_id id, mix_word word)
{
    if (id >= MIX_REG_I1 && id <= MIX_REG_I6) {
        return mix_word_to_index(word) == 0;
    }

    return mix_word_to_value(word) == 0;
}

static bool mix_register_negative(mix_register_id id, mix_word word)
{
    if (id >= MIX_REG_I1 && id <= MIX_REG_I6) {
        return mix_word_to_index(word) < 0;
    }

    return mix_word_to_value(word) < 0;
}

static void mix_char(mix_interpreter *vm)
{
    mix_word a;
    mix_word x = {0};
    uint32_t magnitude = (uint32_t)mix_word_to_value(vm->registers.r[MIX_REG_A]);

    mix_registers_get(&vm->registers, MIX_REG_A, &a);

    for (int i = MIX_WORD_BYTES - 1; i >= 0; --i) {
        x.bytes[i] = (uint8_t)((magnitude % 10U) + 30U);
        magnitude /= 10U;
        a.bytes[i] = (uint8_t)((magnitude % 10U) + 30U);
        magnitude /= 10U;
    }

    mix_registers_set(&vm->registers, MIX_REG_X, x);
    mix_registers_set(&vm->registers, MIX_REG_A, a);
}

static void mix_shift_bytes(uint8_t *bytes, size_t count, int shift, bool left)
{
    uint8_t temp[10];

    if (shift <= 0) {
        return;
    }

    if ((size_t)shift >= count) {
        memset(bytes, 0, count);
        return;
    }

    memcpy(temp, bytes, count);
    if (left) {
        memmove(bytes, temp + shift, count - (size_t)shift);
        memset(bytes + count - (size_t)shift, 0, (size_t)shift);
    } else {
        memmove(bytes + shift, temp, count - (size_t)shift);
        memset(bytes, 0, (size_t)shift);
    }
}

static void mix_shift_sla(mix_interpreter *vm, int shift)
{
    mix_word a;
    mix_word x;
    uint8_t bytes[10];

    mix_registers_get(&vm->registers, MIX_REG_A, &a);
    mix_registers_get(&vm->registers, MIX_REG_X, &x);
    memcpy(bytes, a.bytes, MIX_WORD_BYTES);
    memcpy(bytes + MIX_WORD_BYTES, x.bytes, MIX_WORD_BYTES);
    mix_shift_bytes(bytes, 10U, shift, true);
    memcpy(a.bytes, bytes, MIX_WORD_BYTES);
    memcpy(x.bytes, bytes + MIX_WORD_BYTES, MIX_WORD_BYTES);
    mix_registers_set(&vm->registers, MIX_REG_A, a);
    mix_registers_set(&vm->registers, MIX_REG_X, x);
}

static void mix_shift_sra(mix_interpreter *vm, int shift)
{
    mix_word a;
    mix_word x;
    uint8_t bytes[10];

    mix_registers_get(&vm->registers, MIX_REG_A, &a);
    mix_registers_get(&vm->registers, MIX_REG_X, &x);
    memcpy(bytes, a.bytes, MIX_WORD_BYTES);
    memcpy(bytes + MIX_WORD_BYTES, x.bytes, MIX_WORD_BYTES);
    mix_shift_bytes(bytes, 10U, shift, false);
    memcpy(a.bytes, bytes, MIX_WORD_BYTES);
    memcpy(x.bytes, bytes + MIX_WORD_BYTES, MIX_WORD_BYTES);
    mix_registers_set(&vm->registers, MIX_REG_A, a);
    mix_registers_set(&vm->registers, MIX_REG_X, x);
}

static bool mix_condition_jump(const mix_decoded *inst, int comparison)
{
    switch (inst->f) {
    case 2U:
        return comparison < 0;
    case 3U:
        return comparison == 0;
    case 4U:
        return comparison > 0;
    case 5U:
        return comparison >= 0;
    case 6U:
        return comparison != 0;
    case 7U:
        return comparison <= 0;
    default:
        return false;
    }
}

mix_run_result mix_instruction_execute(mix_interpreter *vm, const mix_decoded *inst)
{
    int32_t m = mix_effective_address(vm, inst);
    mix_word reg_word;
    mix_word mem_word;
    mix_register_id reg_id;
    int32_t a_val;
    int32_t x_val;
    int32_t quotient;
    int32_t remainder;

    if (inst->c >= 8U && inst->c <= 15U) {
        reg_id = mix_register_from_c(inst->c, 8U);
        mem_word = mix_load_memory_field(vm, m, inst->field);
        mix_registers_set(&vm->registers, reg_id, mem_word);
        return MIX_RUN_OK;
    }

    if (inst->c >= 16U && inst->c <= 23U) {
        reg_id = mix_register_from_c(inst->c, 16U);
        mem_word = mix_load_memory_field(vm, m, inst->field);
        mem_word.sign = (mem_word.sign == MIX_POSITIVE) ? MIX_NEGATIVE : MIX_POSITIVE;
        mix_registers_set(&vm->registers, reg_id, mem_word);
        return MIX_RUN_OK;
    }

    if (inst->c >= 24U && inst->c <= 31U) {
        reg_id = mix_register_from_c(inst->c, 24U);
        mix_registers_get(&vm->registers, reg_id, &reg_word);
        if (!mix_store_memory_field(vm, m, inst->field, reg_word)) {
            return MIX_RUN_MEMORY_FAULT;
        }
        return MIX_RUN_OK;
    }

    if (inst->c >= 32U && inst->c <= 38U) {
        mix_word memory_word;

        reg_id = mix_register_from_c(inst->c, 32U);
        mix_registers_get(&vm->registers, reg_id, &reg_word);
        if (!mix_load_memory(vm, m, &memory_word)) {
            return MIX_RUN_MEMORY_FAULT;
        }

        if (reg_id >= MIX_REG_I1 && reg_id <= MIX_REG_I6) {
            mix_registers_set_comparison(
                &vm->registers,
                mix_word_to_index(mix_word_get_field(reg_word, inst->field)),
                mix_word_to_index(mix_word_get_field(memory_word, inst->field)));
        } else {
            mix_registers_set_comparison(
                &vm->registers,
                mix_word_to_value(mix_word_get_field(reg_word, inst->field)),
                mix_word_to_value(mix_word_get_field(memory_word, inst->field)));
        }
        return MIX_RUN_OK;
    }

    if (inst->c == 39U) {
        if (inst->f == 8U) {
            mix_set_jump_address(vm);
            return mix_jump(vm, m) ? MIX_RUN_OK : MIX_RUN_MEMORY_FAULT;
        }
        if (inst->f >= 2U && inst->f <= 7U) {
            if (mix_condition_jump(inst, mix_registers_comparison(&vm->registers))) {
                mix_set_jump_address(vm);
                return mix_jump(vm, m) ? MIX_RUN_OK : MIX_RUN_MEMORY_FAULT;
            }
            return MIX_RUN_OK;
        }
        return MIX_RUN_INVALID_INSTRUCTION;
    }

    if (inst->c >= 40U && inst->c <= 47U) {
        reg_id = mix_register_from_c(inst->c, 40U);
        mix_registers_get(&vm->registers, reg_id, &reg_word);
        bool take = false;

        switch (inst->f) {
        case 0U:
            take = mix_register_negative(reg_id, reg_word);
            break;
        case 1U:
            take = mix_register_zero(reg_id, reg_word);
            break;
        case 2U:
            take = mix_register_positive(reg_id, reg_word);
            break;
        case 3U:
            take = !mix_register_negative(reg_id, reg_word);
            break;
        case 4U:
            take = !mix_register_zero(reg_id, reg_word);
            break;
        case 5U:
            take = !mix_register_positive(reg_id, reg_word);
            break;
        default:
            return MIX_RUN_INVALID_INSTRUCTION;
        }

        if (take) {
            mix_set_jump_address(vm);
            return mix_jump(vm, m) ? MIX_RUN_OK : MIX_RUN_MEMORY_FAULT;
        }
        return MIX_RUN_OK;
    }

    if (inst->c >= 48U && inst->c <= 55U) {
        reg_id = mix_register_from_c(inst->c, 48U);
        mix_registers_get(&vm->registers, reg_id, &reg_word);

        switch (inst->f) {
        case 1U:
            if (reg_id >= MIX_REG_I1 && reg_id <= MIX_REG_I6) {
                mix_registers_set(
                    &vm->registers,
                    reg_id,
                    mix_word_from_index(mix_word_to_index(reg_word) + m));
            } else {
                mix_registers_set(
                    &vm->registers,
                    reg_id,
                    mix_word_from_value(mix_word_to_value(reg_word) + m));
            }
            return MIX_RUN_OK;
        case 2U:
            if (reg_id >= MIX_REG_I1 && reg_id <= MIX_REG_I6) {
                mix_registers_set(
                    &vm->registers,
                    reg_id,
                    mix_word_from_index(mix_word_to_index(reg_word) - m));
            } else {
                mix_registers_set(
                    &vm->registers,
                    reg_id,
                    mix_word_from_value(mix_word_to_value(reg_word) - m));
            }
            return MIX_RUN_OK;
        case 3U:
            if (reg_id >= MIX_REG_I1 && reg_id <= MIX_REG_I6) {
                mix_registers_set(&vm->registers, reg_id, mix_word_from_index(m));
            } else {
                mix_registers_set(&vm->registers, reg_id, mix_word_from_value(m));
            }
            return MIX_RUN_OK;
        case 4U:
            if (reg_id >= MIX_REG_I1 && reg_id <= MIX_REG_I6) {
                mix_registers_set(
                    &vm->registers,
                    reg_id,
                    mix_word_from_index(-m));
            } else {
                mix_registers_set(
                    &vm->registers,
                    reg_id,
                    mix_word_from_value(-m));
            }
            return MIX_RUN_OK;
        default:
            return MIX_RUN_INVALID_INSTRUCTION;
        }
    }

    switch (inst->c) {
    case 1U:
        mix_registers_get(&vm->registers, MIX_REG_A, &reg_word);
        mem_word = mix_load_memory_field(vm, m, inst->field);
        mix_registers_set(
            &vm->registers,
            MIX_REG_A,
            mix_word_from_value(mix_word_to_value(reg_word) + mix_word_to_value(mem_word)));
        return MIX_RUN_OK;
    case 2U:
        mix_registers_get(&vm->registers, MIX_REG_A, &reg_word);
        mem_word = mix_load_memory_field(vm, m, inst->field);
        mix_registers_set(
            &vm->registers,
            MIX_REG_A,
            mix_word_from_value(mix_word_to_value(reg_word) - mix_word_to_value(mem_word)));
        return MIX_RUN_OK;
    case 3U:
        a_val = mix_word_to_value(vm->registers.r[MIX_REG_A]);
        mem_word = mix_load_memory_field(vm, m, inst->field);
        x_val = (int32_t)((uint64_t)(uint32_t)a_val * (uint64_t)(uint32_t)mix_word_to_value(mem_word));
        mix_registers_set(&vm->registers, MIX_REG_A, mix_word_from_value((int32_t)(x_val / 1073741824LL)));
        mix_registers_set(&vm->registers, MIX_REG_X, mix_word_from_value((int32_t)(x_val % 1073741824LL)));
        return MIX_RUN_OK;
    case 4U:
        a_val = mix_word_to_value(vm->registers.r[MIX_REG_A]);
        mem_word = mix_load_memory_field(vm, m, inst->field);
        {
            int32_t divisor = mix_word_to_value(mem_word);
            if (divisor == 0) {
                return MIX_RUN_INVALID_INSTRUCTION;
            }
            quotient = a_val / divisor;
            remainder = a_val % divisor;
            mix_registers_set(&vm->registers, MIX_REG_A, mix_word_from_value(quotient));
            mix_registers_set(&vm->registers, MIX_REG_X, mix_word_from_value(remainder));
        }
        return MIX_RUN_OK;
    case 6U: {
        int shift = (m < 0) ? -m : m;

        if (shift > 10) {
            shift = 10;
        }

        switch (inst->f) {
        case 1U:
            mix_shift_sla(vm, shift);
            return MIX_RUN_OK;
        case 2U:
            mix_shift_sra(vm, shift);
            return MIX_RUN_OK;
        case 5U:
            mix_shift_sla(vm, shift);
            return MIX_RUN_OK;
        case 6U:
            mix_shift_sra(vm, shift);
            return MIX_RUN_OK;
        default:
            return MIX_RUN_INVALID_INSTRUCTION;
        }
    }
    case 5U:
        if (inst->f == 1U) {
            mix_char(vm);
            return MIX_RUN_OK;
        }
        if (inst->f == 2U) {
            return MIX_RUN_HALT;
        }
        return MIX_RUN_INVALID_INSTRUCTION;
    case 56U: {
        mix_word memory_word;

        mix_registers_get(&vm->registers, MIX_REG_J, &reg_word);
        if (!mix_load_memory(vm, m, &memory_word)) {
            return MIX_RUN_MEMORY_FAULT;
        }

        memory_word.sign = reg_word.sign;
        memory_word.bytes[0] = reg_word.bytes[0];
        memory_word.bytes[1] = reg_word.bytes[1];
        memory_word.bytes[2] = reg_word.bytes[2];

        if (!mix_store_memory(vm, m, memory_word)) {
            return MIX_RUN_MEMORY_FAULT;
        }
        return MIX_RUN_OK;
    }
    case 61U:
        return mix_io_out(vm, inst->f, (mix_address){.value = (uint16_t)m});
    default:
        return MIX_RUN_INVALID_INSTRUCTION;
    }
}