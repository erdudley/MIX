#ifndef MIX_IO_H
#define MIX_IO_H

#include "mix/types.h"

#include <stddef.h>
#include <stdint.h>

typedef struct mix_interpreter mix_interpreter;

#define MIX_IO_OUTPUT_CAP 4096U

typedef struct mix_io {
    char output[MIX_IO_OUTPUT_CAP];
    size_t length;
} mix_io;

void mix_io_init(mix_io *io);
void mix_io_reset(mix_io *io);
void mix_io_emit(mix_io *io, char ch);
const char *mix_io_cstr(const mix_io *io);

mix_run_result mix_io_out(mix_interpreter *vm, uint8_t device, mix_address addr);

#endif /* MIX_IO_H */