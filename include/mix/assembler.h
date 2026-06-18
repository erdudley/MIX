#ifndef MIX_ASSEMBLER_H
#define MIX_ASSEMBLER_H

#include "mix/interpreter.h"

#include <stddef.h>

typedef struct mix_assemble_result {
    bool ok;
    char error[256];
    mix_address entry;
} mix_assemble_result;

mix_assemble_result mix_assemble_file(mix_interpreter *vm, const char *path);
mix_assemble_result mix_assemble_string(mix_interpreter *vm, const char *source);

#endif /* MIX_ASSEMBLER_H */