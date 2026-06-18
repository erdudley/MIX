#include "mix/assembler.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIX_ASM_MAX_SYMBOLS 512
#define MIX_ASM_MAX_LINES 1024
#define MIX_ASM_LINE_LEN 256
#define MIX_ASM_MAX_LITERALS 256
#define MIX_ASM_LITERAL_POOL 600U

#define MIX_ASM_SYMBOL_LEN 32

typedef struct mix_asm_symbol {
    char name[MIX_ASM_SYMBOL_LEN];
    int32_t value;
    bool defined;
} mix_asm_symbol;

typedef struct mix_asm_literal {
    int32_t value;
    uint16_t address;
} mix_asm_literal;

typedef struct mix_asm_state {
    mix_interpreter *vm;
    mix_asm_symbol symbols[MIX_ASM_MAX_SYMBOLS];
    size_t symbol_count;
    mix_asm_literal literals[MIX_ASM_MAX_LITERALS];
    size_t literal_count;
    uint16_t pool_loc;
    uint16_t loc;
    mix_address entry;
    bool entry_set;
    char error[256];
} mix_asm_state;

typedef struct mix_asm_opcode {
    const char *name;
    uint8_t c;
    uint8_t f;
} mix_asm_opcode;

static const mix_asm_opcode g_mix_opcodes[] = {
    {"NOP", 0, 0},   {"ADD", 1, 5},   {"SUB", 2, 5},   {"MUL", 3, 5},
    {"DIV", 4, 5},   {"NUM", 5, 0},   {"CHAR", 5, 1},  {"HLT", 5, 2},
    {"SLA", 6, 1},   {"SRA", 6, 2},   {"SLAX", 6, 5},  {"SRAX", 6, 6},
    {"MOVE", 7, 0},
    {"LDA", 8, 5},   {"LD1", 9, 5},   {"LD2", 10, 5},  {"LD3", 11, 5},
    {"LD4", 12, 5},  {"LD5", 13, 5},  {"LD6", 14, 5},  {"LDX", 15, 5},
    {"LDAN", 16, 5}, {"LD1N", 17, 5}, {"LD2N", 18, 5}, {"LD3N", 19, 5},
    {"LD4N", 20, 5}, {"LD5N", 21, 5}, {"LD6N", 22, 5}, {"LDXN", 23, 5},
    {"STA", 24, 5},  {"ST1", 25, 5},  {"ST2", 26, 5},  {"ST3", 27, 5},
    {"ST4", 28, 5},  {"ST5", 29, 5},  {"ST6", 30, 5},  {"STX", 31, 5},
    {"CMPA", 32, 5}, {"CMP1", 33, 5}, {"CMP2", 34, 5}, {"CMP3", 35, 5},
    {"CMP4", 36, 5}, {"CMP5", 37, 5}, {"CMP6", 38, 5},
    {"JOV", 39, 0},  {"JNOV", 39, 1}, {"JL", 39, 2},   {"JE", 39, 3},
    {"JG", 39, 4},   {"JGE", 39, 5},  {"JNE", 39, 6},  {"JLE", 39, 7},
    {"JMP", 39, 8},  {"JSJ", 39, 9},
    {"JAN", 40, 0},  {"JAZ", 40, 1},  {"JAP", 40, 2},  {"JANN", 40, 3},
    {"JANZ", 40, 4}, {"JANP", 40, 5},
    {"J1N", 41, 0},  {"J1Z", 41, 1},  {"J1P", 41, 2},  {"J1NN", 41, 3},
    {"J1NZ", 41, 4}, {"J1NP", 41, 5},
    {"J2N", 42, 0},  {"J2Z", 42, 1},  {"J2P", 42, 2},
    {"J3N", 43, 0},  {"J3Z", 43, 1},  {"J3P", 43, 2},
    {"J4N", 44, 0},  {"J4Z", 44, 1},  {"J4P", 44, 2},
    {"J5N", 45, 0},  {"J5Z", 45, 1},  {"J5P", 45, 2},
    {"J6N", 46, 0},  {"J6Z", 46, 1},  {"J6P", 46, 2},
    {"JXN", 47, 0},  {"JXZ", 47, 1},  {"JXP", 47, 2},
    {"INCA", 48, 1}, {"DECA", 48, 2}, {"ENTA", 48, 3}, {"ENNA", 48, 4},
    {"INC1", 49, 1}, {"DEC1", 49, 2}, {"ENT1", 49, 3}, {"ENN1", 49, 4},
    {"INC2", 50, 1}, {"DEC2", 50, 2}, {"ENT2", 50, 3}, {"ENN2", 50, 4},
    {"INC3", 51, 1}, {"DEC3", 51, 2}, {"ENT3", 51, 3}, {"ENN3", 51, 4},
    {"INC4", 52, 1}, {"DEC4", 52, 2}, {"ENT4", 52, 3}, {"ENN4", 52, 4},
    {"INC5", 53, 1}, {"DEC5", 53, 2}, {"ENT5", 53, 3}, {"ENN5", 53, 4},
    {"INC6", 54, 1}, {"DEC6", 54, 2}, {"ENT6", 54, 3}, {"ENN6", 54, 4},
    {"INCX", 55, 1}, {"DECX", 55, 2}, {"ENTX", 55, 3}, {"ENNX", 55, 4},
    {"STJ", 56, 2},  {"IN", 60, 0},   {"OUT", 61, 0},  {"IOC", 62, 0},
    {"JBUS", 63, 0},
};

static uint32_t mix_asm_error_line;

static void mix_asm_fail(mix_asm_state *st, const char *message)
{
    if (mix_asm_error_line > 0U) {
        snprintf(st->error, sizeof(st->error), "line %u: %s",
                 mix_asm_error_line, message);
    } else {
        snprintf(st->error, sizeof(st->error), "%s", message);
    }
}

static mix_asm_symbol *mix_asm_find_symbol(mix_asm_state *st, const char *name)
{
    for (size_t i = 0; i < st->symbol_count; ++i) {
        if (strcmp(st->symbols[i].name, name) == 0) {
            return &st->symbols[i];
        }
    }
    return NULL;
}

static mix_asm_symbol *mix_asm_define_symbol(
    mix_asm_state *st,
    const char *name,
    int32_t value,
    bool replace_ok)
{
    mix_asm_symbol *sym = mix_asm_find_symbol(st, name);

    if (sym != NULL) {
        if (sym->defined && !replace_ok) {
            mix_asm_fail(st, "duplicate symbol");
            return NULL;
        }
        sym->value = value;
        sym->defined = true;
        return sym;
    }

    if (st->symbol_count >= MIX_ASM_MAX_SYMBOLS) {
        mix_asm_fail(st, "symbol table full");
        return NULL;
    }

    sym = &st->symbols[st->symbol_count++];
    strncpy(sym->name, name, sizeof(sym->name) - 1U);
    sym->name[sizeof(sym->name) - 1U] = '\0';
    sym->value = value;
    sym->defined = true;
    return sym;
}

static const mix_asm_opcode *mix_asm_lookup_opcode(const char *name)
{
    for (size_t i = 0; i < sizeof(g_mix_opcodes) / sizeof(g_mix_opcodes[0]); ++i) {
        if (strcmp(g_mix_opcodes[i].name, name) == 0) {
            return &g_mix_opcodes[i];
        }
    }
    return NULL;
}

static void mix_asm_trim(char *line)
{
    size_t start = 0U;
    size_t len = strlen(line);

    while (start < len && isspace((unsigned char)line[start])) {
        ++start;
    }

    if (start > 0U) {
        memmove(line, line + start, len - start + 1U);
        len -= start;
    }

    while (len > 0U && isspace((unsigned char)line[len - 1U])) {
        line[--len] = '\0';
    }
}

static void mix_asm_strip_comment(char *line)
{
    const char *cursor = line;

    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        ++cursor;
    }

    if (*cursor == '*') {
        line[0] = '\0';
    }
}

static void mix_asm_strip_operand_comment(char *operand)
{
    char *comment;

    if (operand == NULL) {
        return;
    }

    mix_asm_trim(operand);
    if (operand[0] == '\0') {
        return;
    }

    if (operand[0] == '*') {
        const char *next = operand + 1;

        while (*next != '\0' && isspace((unsigned char)*next)) {
            ++next;
        }

        if (*next == '\0' || *next == '*') {
            operand[0] = '*';
            operand[1] = '\0';
            return;
        }
    }

    comment = strchr(operand, '*');
    if (comment != NULL) {
        *comment = '\0';
        mix_asm_trim(operand);
    }
}

static bool mix_asm_parse_number(const char *text, int32_t *out)
{
    char *end = NULL;
    long value = strtol(text, &end, 10);

    if (end == text || (end != NULL && *end != '\0')) {
        return false;
    }

    *out = (int32_t)value;
    return true;
}

static bool mix_asm_eval_expr(mix_asm_state *st, const char *expr, int32_t *out)
{
    const char *cursor = expr;
    int32_t value = 0;
    int32_t term = 0;
    int sign = 1;

    while (isspace((unsigned char)*cursor)) {
        ++cursor;
    }

    if (*cursor == '+' || *cursor == '-') {
        sign = (*cursor == '-') ? -1 : 1;
        ++cursor;
    }

    while (isspace((unsigned char)*cursor)) {
        ++cursor;
    }

    if (*cursor == '*') {
        ++cursor;
        if (*cursor == '+') {
            ++cursor;
            if (!mix_asm_parse_number(cursor, &term)) {
                mix_asm_fail(st, "invalid star expression");
                return false;
            }
            term = (int32_t)st->loc + term;
        } else if (*cursor == '\0') {
            term = (int32_t)st->loc;
        } else {
            mix_asm_fail(st, "invalid star expression");
            return false;
        }
        value += sign * term;
        *out = value;
        return true;
    }

    if (isdigit((unsigned char)*cursor)) {
        if (!mix_asm_parse_number(cursor, &term)) {
            mix_asm_fail(st, "invalid number");
            return false;
        }
        value += sign * term;
        *out = value;
        return true;
    }

    {
        char name[MIX_ASM_SYMBOL_LEN];
        size_t i = 0U;
        while (cursor[i] != '\0' && cursor[i] != '+' && cursor[i] != '-'
               && i < MIX_ASM_SYMBOL_LEN - 1U) {
            name[i] = cursor[i];
            ++i;
        }
        name[i] = '\0';

        mix_asm_symbol *sym = mix_asm_find_symbol(st, name);
        if (sym == NULL || !sym->defined) {
            mix_asm_fail(st, "undefined symbol");
            return false;
        }

        value += sign * sym->value;
        cursor += (int)i;
    }

    while (*cursor != '\0') {
        sign = 1;
        if (*cursor == '+') {
            ++cursor;
        } else if (*cursor == '-') {
            sign = -1;
            ++cursor;
        } else {
            mix_asm_fail(st, "invalid expression");
            return false;
        }

        while (isspace((unsigned char)*cursor)) {
            ++cursor;
        }

        if (isdigit((unsigned char)*cursor)) {
            if (!mix_asm_parse_number(cursor, &term)) {
                mix_asm_fail(st, "invalid number");
                return false;
            }
            value += sign * term;
            char *end = NULL;
            strtol(cursor, &end, 10);
            cursor = end;
            continue;
        }

        {
            char name[MIX_ASM_SYMBOL_LEN];
            size_t i = 0U;
            while (cursor[i] != '\0' && cursor[i] != '+' && cursor[i] != '-'
                   && i < MIX_ASM_SYMBOL_LEN - 1U) {
                name[i] = cursor[i];
                ++i;
            }
            name[i] = '\0';
            mix_asm_symbol *sym = mix_asm_find_symbol(st, name);
            if (sym == NULL || !sym->defined) {
                mix_asm_fail(st, "undefined symbol");
                return false;
            }
            value += sign * sym->value;
            cursor += (int)i;
        }
    }

    *out = value;
    return true;
}

typedef struct mix_asm_operand {
    int32_t address;
    int32_t address_offset;
    int32_t literal_value;
    char string_literal[MIX_ASM_LINE_LEN];
    uint8_t index;
    uint8_t f;
    bool has_field;
    bool has_address_offset;
    bool is_literal;
    bool is_string_literal;
} mix_asm_operand;

static bool mix_asm_parse_index(const char *text, uint8_t *out_index)
{
    if (strcmp(text, "X") == 0) {
        *out_index = 7U;
        return true;
    }

    if (strcmp(text, "A") == 0) {
        *out_index = 8U;
        return true;
    }

    {
        int32_t idx = 0;
        if (!mix_asm_parse_number(text, &idx) || idx < 0 || idx > 6) {
            return false;
        }
        *out_index = (uint8_t)idx;
        return true;
    }
}

static bool mix_asm_parse_operand(
    mix_asm_state *st,
    const char *text,
    mix_asm_operand *op)
{
    char buffer[128];
    char *index_part = NULL;
    char *field_part = NULL;
    const char *cursor = text;

    memset(op, 0, sizeof(*op));
    op->f = 5U;

    while (isspace((unsigned char)*cursor)) {
        ++cursor;
    }

    if (*cursor == '=') {
        size_t literal_len = strlen(cursor);

        if (literal_len < 2U || cursor[literal_len - 1U] != '=') {
            mix_asm_fail(st, "invalid literal");
            return false;
        }

        ++cursor;
        literal_len -= 2U;
        if (literal_len >= sizeof(buffer)) {
            mix_asm_fail(st, "invalid literal");
            return false;
        }

        memcpy(buffer, cursor, literal_len);
        buffer[literal_len] = '\0';

        st->error[0] = '\0';
        if (!mix_asm_eval_expr(st, buffer, &op->literal_value)) {
            strncpy(op->string_literal, buffer, sizeof(op->string_literal) - 1U);
            op->string_literal[sizeof(op->string_literal) - 1U] = '\0';
            op->is_string_literal = true;
            op->is_literal = true;
            st->error[0] = '\0';
            return true;
        }
        op->is_literal = true;
        return true;
    }

    if (*cursor == '\'' && cursor[1] != '\0' && cursor[2] == '\'') {
        op->address = (int32_t)(unsigned char)cursor[1];
        return true;
    }

    strncpy(buffer, cursor, sizeof(buffer) - 1U);
    buffer[sizeof(buffer) - 1U] = '\0';

    field_part = strchr(buffer, '(');
    if (field_part != NULL) {
        *field_part = '\0';
        ++field_part;
        char *close = strchr(field_part, ')');
        if (close == NULL) {
            mix_asm_fail(st, "invalid field");
            return false;
        }
        *close = '\0';

        if (strchr(field_part, ':') != NULL) {
            int32_t left = 0;
            int32_t right = 0;
            char *colon = strchr(field_part, ':');
            *colon = '\0';
            if (!mix_asm_parse_number(field_part, &left)
                || !mix_asm_parse_number(colon + 1, &right)) {
                mix_asm_fail(st, "invalid field");
                return false;
            }
            op->f = (uint8_t)(left + 8 * right);
            op->has_field = true;
        } else {
            int32_t device = 0;
            if (mix_asm_parse_number(field_part, &device)) {
                op->f = (uint8_t)device;
                op->has_field = true;
            } else if (!mix_asm_parse_index(field_part, &op->index)) {
                mix_asm_fail(st, "invalid field/index");
                return false;
            }
        }
    }

    index_part = strchr(buffer, ',');
    if (index_part != NULL) {
        char *second_comma = strchr(index_part + 1, ',');

        *index_part = '\0';
        if (second_comma != NULL) {
            *second_comma = '\0';
            if (!mix_asm_parse_number(index_part + 1, &op->address_offset)) {
                mix_asm_fail(st, "invalid address offset");
                return false;
            }
            op->has_address_offset = true;
            index_part = second_comma + 1;
        } else {
            index_part = index_part + 1;
        }

        if (!mix_asm_parse_index(index_part, &op->index)) {
            mix_asm_fail(st, "invalid index");
            return false;
        }
    }

    mix_asm_trim(buffer);

    if (buffer[0] == '\0') {
        op->address = op->has_address_offset ? op->address_offset : 0;
        return true;
    }

    if (strcmp(buffer, "*") == 0) {
        op->address = (int32_t)st->loc;
        if (op->has_address_offset) {
            op->address += op->address_offset;
        }
        return true;
    }

    if (!mix_asm_eval_expr(st, buffer, &op->address)) {
        return false;
    }

    if (op->has_address_offset) {
        op->address += op->address_offset;
    }

    return true;
}

static mix_word mix_asm_encode_word(
    int32_t address,
    uint8_t index,
    uint8_t f,
    uint8_t c)
{
    mix_word word = {0};
    int32_t magnitude = address;
    mix_sign sign = MIX_POSITIVE;

    if (magnitude < 0) {
        sign = MIX_NEGATIVE;
        magnitude = -magnitude;
    }

    word.sign = sign;
    word.bytes[0] = (uint8_t)((magnitude / 64) % 64);
    word.bytes[1] = (uint8_t)(magnitude % 64);
    word.bytes[2] = index;
    word.bytes[3] = f;
    word.bytes[4] = c;
    return word;
}

static uint16_t mix_asm_literal_address(mix_asm_state *st, int32_t value)
{
    for (size_t i = 0; i < st->literal_count; ++i) {
        if (st->literals[i].value == value) {
            return st->literals[i].address;
        }
    }

    if (st->literal_count >= MIX_ASM_MAX_LITERALS
        || st->pool_loc >= MIX_MEMORY_SIZE) {
        mix_asm_fail(st, "literal pool overflow");
        return 0U;
    }

    st->literals[st->literal_count].value = value;
    st->literals[st->literal_count].address = st->pool_loc;
    ++st->literal_count;

    if (!mix_memory_store(
            &st->vm->memory,
            (mix_address){.value = st->pool_loc},
            mix_word_from_value(value))) {
        mix_asm_fail(st, "literal pool store failed");
        return 0U;
    }

    return st->pool_loc++;
}

static uint16_t mix_asm_literal_string(mix_asm_state *st, const char *text)
{
    char buffer[MIX_ASM_LINE_LEN];
    size_t len;
    uint16_t start = st->pool_loc;

    snprintf(buffer, sizeof(buffer), "%s\n", text);
    len = strlen(buffer);

    for (size_t i = 0U; i < len; i += (size_t)MIX_WORD_BYTES) {
        mix_word word = {0};

        for (int j = 0; j < MIX_WORD_BYTES; ++j) {
            if (i + (size_t)j < len) {
                word.bytes[j] = (uint8_t)buffer[i + (size_t)j];
            }
        }

        if (!mix_memory_store(
                &st->vm->memory,
                (mix_address){.value = st->pool_loc},
                word)) {
            mix_asm_fail(st, "literal pool store failed");
            return 0U;
        }

        ++st->pool_loc;
    }

    if (!mix_memory_store(
            &st->vm->memory,
            (mix_address){.value = st->pool_loc},
            (mix_word){0})) {
        mix_asm_fail(st, "literal pool store failed");
        return 0U;
    }

    ++st->pool_loc;
    return start;
}

static bool mix_asm_store_word(mix_asm_state *st, mix_word word)
{
    if (st->loc >= MIX_MEMORY_SIZE) {
        mix_asm_fail(st, "memory overflow");
        return false;
    }

    if (!mix_memory_store(
            &st->vm->memory,
            (mix_address){.value = st->loc},
            word)) {
        mix_asm_fail(st, "memory store failed");
        return false;
    }

    ++st->loc;
    return true;
}

static size_t mix_asm_count_con_values(const char *rest)
{
    size_t count = 0U;

    if (rest == NULL || rest[0] == '\0') {
        return 0U;
    }

    ++count;
    for (const char *cursor = rest; *cursor != '\0'; ++cursor) {
        if (*cursor == ',') {
            ++count;
        }
    }

    return count;
}

static bool mix_asm_store_con_values(mix_asm_state *st, const char *rest)
{
    char buffer[MIX_ASM_LINE_LEN];
    char *cursor;

    if (rest == NULL) {
        mix_asm_fail(st, "invalid CON");
        return false;
    }

    strncpy(buffer, rest, sizeof(buffer) - 1U);
    buffer[sizeof(buffer) - 1U] = '\0';

    cursor = buffer;
    while (cursor != NULL && *cursor != '\0') {
        int32_t value = 0;
        char *comma = strchr(cursor, ',');

        if (comma != NULL) {
            *comma = '\0';
        }

        mix_asm_trim(cursor);
        if (cursor[0] == '\0') {
            mix_asm_fail(st, "invalid CON");
            return false;
        }

        if (!mix_asm_eval_expr(st, cursor, &value)) {
            return false;
        }

        if (!mix_asm_store_word(st, mix_word_from_value(value))) {
            return false;
        }

        cursor = (comma == NULL) ? NULL : comma + 1;
    }

    return true;
}

static bool mix_asm_pass(
    mix_asm_state *st,
    char lines[][MIX_ASM_LINE_LEN],
    size_t line_count,
    bool assemble)
{
    st->loc = 0U;

    for (size_t i = 0; i < line_count; ++i) {
        mix_asm_error_line = (uint32_t)(i + 1U);
        char line[MIX_ASM_LINE_LEN];
        char label[MIX_ASM_SYMBOL_LEN] = {0};
        char operand[128] = {0};
        char *token;
        char *rest;

        strncpy(line, lines[i], sizeof(line) - 1U);
        line[sizeof(line) - 1U] = '\0';
        mix_asm_strip_comment(line);
        mix_asm_trim(line);
        if (line[0] == '\0') {
            continue;
        }

        token = strtok(line, " \t");
        if (token == NULL) {
            continue;
        }

        rest = strtok(NULL, "");
        if (rest != NULL) {
            mix_asm_trim(rest);
        }

        if (rest != NULL && rest[0] != '\0'
            && mix_asm_lookup_opcode(token) == NULL
            && strcmp(token, "EQU") != 0
            && strcmp(token, "ORIG") != 0
            && strcmp(token, "CON") != 0
            && strcmp(token, "ALF") != 0
            && strcmp(token, "END") != 0) {
            strncpy(label, token, sizeof(label) - 1U);
            label[sizeof(label) - 1U] = '\0';
            token = strtok(rest, " \t");
            if (token != NULL) {
                rest = strtok(NULL, "");
                if (rest != NULL) {
                    mix_asm_trim(rest);
                }
            }
        }

        if (token == NULL) {
            continue;
        }

        if (rest != NULL) {
            mix_asm_strip_operand_comment(rest);
        }

        if (!assemble && label[0] != '\0') {
            if (mix_asm_define_symbol(st, label, (int32_t)st->loc, false) == NULL) {
                return false;
            }
        }

        if (strcmp(token, "EQU") == 0) {
            int32_t value = 0;
            if (rest == NULL || !mix_asm_eval_expr(st, rest, &value)) {
                mix_asm_fail(st, "invalid EQU");
                return false;
            }
            if (label[0] == '\0') {
                mix_asm_fail(st, "EQU requires label");
                return false;
            }
            if (mix_asm_define_symbol(st, label, value, true) == NULL) {
                return false;
            }
            continue;
        }

        if (strcmp(token, "ORIG") == 0) {
            int32_t value = 0;
            if (rest == NULL || !mix_asm_eval_expr(st, rest, &value)
                || value < 0 || value >= MIX_MEMORY_SIZE) {
                mix_asm_fail(st, "invalid ORIG");
                return false;
            }
            st->loc = (uint16_t)value;
            continue;
        }

        if (strcmp(token, "END") == 0) {
            int32_t value = 0;
            if (rest != NULL && rest[0] != '\0') {
                if (!mix_asm_eval_expr(st, rest, &value)) {
                    return false;
                }
                st->entry.value = (uint16_t)value;
                st->entry_set = true;
            }
            continue;
        }

        if (!assemble) {
            if (strcmp(token, "CON") == 0) {
                st->loc = (uint16_t)(st->loc + mix_asm_count_con_values(rest));
            } else if (strcmp(token, "ALF") == 0) {
                ++st->loc;
            } else if (mix_asm_lookup_opcode(token) != NULL) {
                st->loc = (uint16_t)(st->loc + 1U);
            } else {
                mix_asm_fail(st, "unknown opcode");
                return false;
            }
            continue;
        }

        if (strcmp(token, "CON") == 0) {
            if (!mix_asm_store_con_values(st, rest)) {
                return false;
            }
            continue;
        }

        if (strcmp(token, "ALF") == 0) {
            mix_word word = {0};
            const char *text = rest;
            if (text == NULL) {
                mix_asm_fail(st, "invalid ALF");
                return false;
            }
            while (*text != '\0' && !isalnum((unsigned char)*text) && *text != ' ') {
                ++text;
            }
            if (*text == '"') {
                ++text;
            }
            for (int i = 0; i < MIX_WORD_BYTES && text[i] != '\0' && text[i] != '"'; ++i) {
                word.bytes[i] = (uint8_t)text[i];
            }
            if (!mix_asm_store_word(st, word)) {
                return false;
            }
            continue;
        }

        {
            const mix_asm_opcode *opcode = mix_asm_lookup_opcode(token);
            mix_asm_operand operand;
            uint8_t f;

            if (opcode == NULL) {
                mix_asm_fail(st, "unknown opcode");
                return false;
            }

            if (rest == NULL) {
                rest = "";
            }

            if (!mix_asm_parse_operand(st, rest, &operand)) {
                return false;
            }

            f = operand.has_field ? operand.f : opcode->f;
            if (strcmp(token, "OUT") == 0 && operand.has_field) {
                f = operand.f;
            }

            if (operand.is_literal
                && opcode->c >= 48U
                && opcode->c <= 55U
                && opcode->f >= 1U
                && opcode->f <= 4U) {
                if (!mix_asm_store_word(
                        st,
                        mix_asm_encode_word(
                            operand.literal_value,
                            operand.index,
                            f,
                            opcode->c))) {
                    return false;
                }
            } else if (operand.is_literal) {
                uint16_t literal_addr = 0U;

                if (operand.is_string_literal) {
                    literal_addr = mix_asm_literal_string(st, operand.string_literal);
                } else {
                    literal_addr = mix_asm_literal_address(st, operand.literal_value);
                }

                if (literal_addr == 0U && st->error[0] != '\0') {
                    return false;
                }
                if (!mix_asm_store_word(
                        st,
                        mix_asm_encode_word(
                            (int32_t)literal_addr,
                            operand.index,
                            f,
                            opcode->c))) {
                    return false;
                }
            } else if (!mix_asm_store_word(
                           st,
                           mix_asm_encode_word(
                               operand.address,
                               operand.index,
                               f,
                               opcode->c))) {
                return false;
            }
        }
    }

    return true;
}

static bool mix_asm_load_lines(
    const char *source,
    char lines[][MIX_ASM_LINE_LEN],
    size_t *line_count)
{
    const char *cursor = source;
    size_t count = 0U;

    while (*cursor != '\0' && count < MIX_ASM_MAX_LINES) {
        size_t i = 0U;
        while (*cursor != '\0' && *cursor != '\n' && i < MIX_ASM_LINE_LEN - 1U) {
            lines[count][i++] = *cursor++;
        }
        lines[count][i] = '\0';
        ++count;
        if (*cursor == '\n') {
            ++cursor;
        }
    }

    *line_count = count;
    return true;
}

mix_assemble_result mix_assemble_string(mix_interpreter *vm, const char *source)
{
    mix_assemble_result result = {0};
    mix_asm_state st;
    char lines[MIX_ASM_MAX_LINES][MIX_ASM_LINE_LEN];
    size_t line_count = 0U;

    if (vm == NULL || source == NULL) {
        snprintf(result.error, sizeof(result.error), "invalid arguments");
        return result;
    }

    memset(&st, 0, sizeof(st));
    st.vm = vm;
    st.pool_loc = MIX_ASM_LITERAL_POOL;
    mix_interpreter_reset(vm);

    if (!mix_asm_load_lines(source, lines, &line_count)) {
        snprintf(result.error, sizeof(result.error), "too many lines");
        return result;
    }

    if (!mix_asm_pass(&st, lines, line_count, false)) {
        snprintf(result.error, sizeof(result.error), "%s", st.error);
        return result;
    }

    if (!mix_asm_pass(&st, lines, line_count, true)) {
        snprintf(result.error, sizeof(result.error), "%s", st.error);
        return result;
    }

    if (!st.entry_set) {
        snprintf(result.error, sizeof(result.error), "missing END directive");
        return result;
    }

    vm->registers.location_counter = st.entry;
    result.ok = true;
    result.entry = st.entry;
    return result;
}

mix_assemble_result mix_assemble_file(mix_interpreter *vm, const char *path)
{
    mix_assemble_result result = {0};
    FILE *fp = fopen(path, "r");
    char *source = NULL;
    long size = 0;

    if (fp == NULL) {
        snprintf(result.error, sizeof(result.error), "cannot open '%s'", path);
        return result;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        snprintf(result.error, sizeof(result.error), "cannot seek '%s'", path);
        return result;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        snprintf(result.error, sizeof(result.error), "cannot size '%s'", path);
        return result;
    }

    rewind(fp);
    source = (char *)malloc((size_t)size + 1U);
    if (source == NULL) {
        fclose(fp);
        snprintf(result.error, sizeof(result.error), "out of memory");
        return result;
    }

    if (fread(source, 1U, (size_t)size, fp) != (size_t)size) {
        free(source);
        fclose(fp);
        snprintf(result.error, sizeof(result.error), "cannot read '%s'", path);
        return result;
    }

    source[size] = '\0';
    fclose(fp);

    result = mix_assemble_string(vm, source);
    free(source);
    return result;
}