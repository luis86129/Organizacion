#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>

/* Framework de testing casero: sin dependencias externas.
 * Cada TEST_CASE cuenta como 1 assert lógico (puede contener varios
 * CHECK internos, pero se reporta como una sola prueba pass/fail). */

static int tests_run    = 0;
static int tests_passed = 0;
static int current_ok    = 1;

#define TEST_CASE(name) \
    do { \
        current_ok = 1; \
        printf("  [ ] %s ... ", name); \
    } while (0)

#define CHECK(cond) \
    do { \
        tests_run++; \
        if (cond) { \
            tests_passed++; \
        } else { \
            current_ok = 0; \
            printf("\n      FALLÓ: %s (línea %d)", #cond, __LINE__); \
        } \
    } while (0)

#define TEST_END() \
    do { \
        printf(current_ok ? "OK\n" : "\n"); \
    } while (0)

#define TEST_SUMMARY(suite_name) \
    do { \
        printf("\n%s: %d/%d checks pasaron\n", suite_name, tests_passed, tests_run); \
        if (tests_passed != tests_run) return 1; \
        return 0; \
    } while (0)

#endif /* TEST_COMMON_H */
