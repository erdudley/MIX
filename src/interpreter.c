#include "mix/interpreter.h"
#include "mix/instruction.h"

void mix_interpreter_init(mix_interpreter *vm)
{
    mix_interpreter_reset(vm);
}

void mix_interpreter_reset(mix_interpreter *vm)
{
    mix_memory_init(&vm->memory);
    mix_registers_init(&vm->registers);
    mix_io_init(&vm->io);
    vm->step_limit = 0;
    vm->steps_executed = 0;
}

const mix_io *mix_interpreter_io(const mix_interpreter *vm)
{
    if (vm == NULL) {
        return NULL;
    }

    return &vm->io;
}

static mix_run_result mix_interpreter_fetch(
    mix_interpreter *vm,
    mix_word *instruction)
{
    mix_address pc = vm->registers.location_counter;

    if (!mix_memory_load(&vm->memory, pc, instruction)) {
        return MIX_RUN_MEMORY_FAULT;
    }

    vm->registers.location_counter.value = (uint16_t)(pc.value + 1U);
    return MIX_RUN_OK;
}

mix_run_result mix_interpreter_step(mix_interpreter *vm)
{
    mix_word instruction;
    mix_decoded decoded;
    mix_run_result result;

    if (vm == NULL) {
        return MIX_RUN_INVALID_INSTRUCTION;
    }

    if (vm->step_limit > 0 && vm->steps_executed >= vm->step_limit) {
        return MIX_RUN_STEP_LIMIT;
    }

    result = mix_interpreter_fetch(vm, &instruction);
    if (result != MIX_RUN_OK) {
        return result;
    }

    decoded = mix_instruction_decode(instruction);
    ++vm->steps_executed;
    return mix_instruction_execute(vm, &decoded);
}

mix_run_result mix_interpreter_run(mix_interpreter *vm)
{
    mix_run_result result = MIX_RUN_OK;

    if (vm == NULL) {
        return MIX_RUN_INVALID_INSTRUCTION;
    }

    while (result == MIX_RUN_OK) {
        result = mix_interpreter_step(vm);
    }

    if (result == MIX_RUN_HALT || result == MIX_RUN_STEP_LIMIT) {
        return result;
    }

    return result;
}