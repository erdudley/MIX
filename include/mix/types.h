#ifndef MIX_TYPES_H
#define MIX_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define MIX_WORD_BYTES 5
#define MIX_MEMORY_SIZE 4000
#define MIX_REGISTER_COUNT 9

typedef enum mix_sign {
    MIX_NEGATIVE = 0,
    MIX_POSITIVE = 1
} mix_sign;

typedef struct mix_word {
    mix_sign sign;
    uint8_t bytes[MIX_WORD_BYTES];
} mix_word;

typedef struct mix_address {
    uint16_t value;
} mix_address;

typedef struct mix_field {
    uint8_t l;
    uint8_t r;
} mix_field;

typedef enum mix_run_result {
    MIX_RUN_OK = 0,
    MIX_RUN_HALT,
    MIX_RUN_INVALID_INSTRUCTION,
    MIX_RUN_MEMORY_FAULT,
    MIX_RUN_STEP_LIMIT
} mix_run_result;

#endif /* MIX_TYPES_H */