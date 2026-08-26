/* =====================================================================
 * minitest.h -- Framework minimo de pruebas: dos macros y un contador.
 * Cada binario de prueba devuelve 0 si todo paso, 1 si hubo alguna falla,
 * para que el make lo pueda encadenar.
 * ===================================================================== */
#ifndef MINITEST_H
#define MINITEST_H

#include <stdio.h>
#include <stdint.h>

static int mt_pass = 0;
static int mt_fail = 0;

#define MT_BEGIN(nombre) printf("== %s ==\n", (nombre))

#define CHECK(cond, msg)                                                   \
    do {                                                                   \
        if (cond) {                                                        \
            mt_pass++;                                                     \
        } else {                                                           \
            mt_fail++;                                                     \
            printf("  [FALLA] %s  (%s:%d)\n", (msg), __FILE__, __LINE__);  \
        }                                                                  \
    } while (0)

#define CHECK_EQ(got, exp, msg)                                            \
    do {                                                                   \
        uint32_t g_ = (uint32_t)(got);                                     \
        uint32_t e_ = (uint32_t)(exp);                                     \
        if (g_ == e_) {                                                    \
            mt_pass++;                                                     \
        } else {                                                           \
            mt_fail++;                                                     \
            printf("  [FALLA] %s: esperado 0x%08X, obtenido 0x%08X (%s:%d)\n", \
                   (msg), e_, g_, __FILE__, __LINE__);                     \
        }                                                                  \
    } while (0)

#define MT_END()                                                           \
    (printf("  %d comprobaciones ok, %d fallas\n\n", mt_pass, mt_fail),    \
     (mt_fail == 0) ? 0 : 1)

#endif /* MINITEST_H */
