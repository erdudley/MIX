#ifndef MIX_INTERPRETER_H
#define MIX_INTERPRETER_H

#include "mix/memory.h"
#include "mix/registers.h"

#include <stddef.h>

#include "mix/io.h"

typedef struct mix_interpreter {
    mix_memory memory;
    mix_registers registers;
    mix_io io;
    size_t step_limit;
    size_t steps_executed;
} mix_interpreter;

const mix_io *mix_interpreter_io(const mix_interpreter *vm);

void mix_interpreter_init(mix_interpreter *vm);
void mix_interpreter_reset(mix_interpreter *vm);

mix_run_result mix_interpreter_step(mix_interpreter *vm);
mix_run_result mix_interpreter_run(mix_interpreter *vm);

#endif /* MIX_INTERPRETER_H */