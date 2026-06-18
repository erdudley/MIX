#ifndef MIX_TEST_H
#define MIX_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*mix_test_fn)(void);

typedef struct mix_test_case {
    const char *name;
    mix_test_fn fn;
} mix_test_case;

#define MIX_TEST(name) static void name(void)

#define MIX_ASSERT(expr)                                                     \
    do {                                                                     \
        if (!(expr)) {                                                       \
            fprintf(stderr, "  FAIL %s:%d: assertion failed: %s\n",          \
                    __FILE__, __LINE__, #expr);                              \
            ++g_mix_test_failures;                                           \
            return;                                                          \
        }                                                                    \
    } while (0)

#define MIX_ASSERT_EQ_INT(expected, actual)                                  \
    do {                                                                     \
        const long long _exp = (long long)(expected);                        \
        const long long _act = (long long)(actual);                          \
        if (_exp != _act) {                                                  \
            fprintf(stderr,                                                 \
                    "  FAIL %s:%d: expected %lld, got %lld\n",               \
                    __FILE__, __LINE__, _exp, _act);                         \
            ++g_mix_test_failures;                                           \
            return;                                                          \
        }                                                                    \
    } while (0)

#define MIX_ASSERT_TRUE(expr) MIX_ASSERT(expr)
#define MIX_ASSERT_FALSE(expr) MIX_ASSERT(!(expr))

extern int g_mix_test_failures;
extern int g_mix_test_runs;

void mix_test_run(const char *suite, const mix_test_case *cases, size_t count);

#endif /* MIX_TEST_H */