#include "mix/mix.h"

#include <stdio.h>
#include <stdlib.h>

static void mix_print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s <program.mix>\n", program);
    fprintf(stderr, "  Assemble and run a MIXAL program.\n");
}

int main(int argc, char **argv)
{
    mix_interpreter vm;
    mix_assemble_result assembled;
    mix_run_result run_result;
    const mix_io *io;

    if (argc != 2) {
        mix_print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    mix_interpreter_init(&vm);
    assembled = mix_assemble_file(&vm, argv[1]);
    if (!assembled.ok) {
        fprintf(stderr, "assembly error: %s\n", assembled.error);
        return EXIT_FAILURE;
    }

    run_result = mix_interpreter_run(&vm);
    if (run_result != MIX_RUN_HALT) {
        fprintf(stderr, "program stopped with code %d after %zu steps\n",
                (int)run_result, vm.steps_executed);
        return EXIT_FAILURE;
    }

    io = mix_interpreter_io(&vm);
    if (io != NULL && io->length > 0U) {
        fputs(mix_io_cstr(io), stdout);
    }

    return EXIT_SUCCESS;
}