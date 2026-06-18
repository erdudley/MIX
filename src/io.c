#include "mix/interpreter.h"
#include "mix/io.h"
#include "mix/memory.h"

#include <string.h>

#define MIX_LITERAL_POOL_BASE 600U

void mix_io_init(mix_io *io)
{
    mix_io_reset(io);
}

void mix_io_reset(mix_io *io)
{
    if (io == NULL) {
        return;
    }

    memset(io->output, 0, sizeof(io->output));
    io->length = 0U;
}

void mix_io_emit(mix_io *io, char ch)
{
    if (io == NULL || io->length >= MIX_IO_OUTPUT_CAP - 1U) {
        return;
    }

    io->output[io->length++] = ch;
    io->output[io->length] = '\0';
}

const char *mix_io_cstr(const mix_io *io)
{
    if (io == NULL) {
        return "";
    }

    return io->output;
}

static char mix_io_mix_byte_to_ascii(uint8_t byte)
{
    if (byte >= 30U && byte <= 39U) {
        return (char)('0' + (byte - 30U));
    }

    if (byte == 0U || byte == 36U || byte == 48U) {
        return ' ';
    }

    return (char)byte;
}

static void mix_io_emit_out_byte(mix_io *io, uint8_t byte, bool ascii_mode)
{
    if (byte == 0U) {
        return;
    }

    if (ascii_mode) {
        mix_io_emit(io, (char)byte);
        return;
    }

    mix_io_emit(io, mix_io_mix_byte_to_ascii(byte));
}

mix_run_result mix_io_out(mix_interpreter *vm, uint8_t device, mix_address addr)
{
    mix_address cursor = addr;
    mix_word word;
    bool ascii_mode = addr.value >= MIX_LITERAL_POOL_BASE;

    if (vm == NULL) {
        return MIX_RUN_INVALID_INSTRUCTION;
    }

    if (device == 0U) {
        device = 19U;
    }

    if (device != 18U && device != 19U) {
        return MIX_RUN_INVALID_INSTRUCTION;
    }

    while (cursor.value < MIX_MEMORY_SIZE) {
        if (!mix_memory_load(&vm->memory, cursor, &word)) {
            return MIX_RUN_MEMORY_FAULT;
        }

        if (word.bytes[0] == 0U && word.bytes[1] == 0U && word.bytes[2] == 0U
            && word.bytes[3] == 0U && word.bytes[4] == 0U) {
            break;
        }

        for (int i = 0; i < MIX_WORD_BYTES; ++i) {
            if (word.bytes[i] == 0U) {
                if (ascii_mode) {
                    return MIX_RUN_OK;
                }
                continue;
            }

            mix_io_emit_out_byte(&vm->io, word.bytes[i], ascii_mode);
        }

        cursor.value = (uint16_t)(cursor.value + 1U);
    }

    return MIX_RUN_OK;
}