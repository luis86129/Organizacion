/* =====================================================================
 * test_integration.c -- Test integral del sistema.
 *
 * Corre los ocho vectores del plan de verificacion (S1..S8) sobre el
 * pipeline completo y compara el estado final: banco de registros,
 * memoria de datos y contadores de burbujas/descartes.
 *
 * Ademas prueba el camino de archivos .txt (escribir, recargar, correr)
 * y los casos de robustez: escritura a $0 e instruccion ilegal.
 * ===================================================================== */
#include <stdio.h>
#include <string.h>
#include "minitest.h"
#include "mips.h"
#include "encode.h"
#include "programs.h"

#define MAX_CYCLES 1000u

/* Carga un Program en las memorias y lo corre hasta el final. */
static int correr(const Program *pr, Pipeline *p, DataMem *dm, RegFile *rf)
{
    InstrMem im;
    uint32_t i;
    int      rc;

    mem_init_imem(&im);
    for (i = 0; i < pr->n_code; i++) {
        im.w[i] = pr->code[i];
    }
    im.n_words = pr->n_code;

    mem_init_dmem(dm);
    for (i = 0; i < pr->n_data; i++) {
        dm->w[i] = pr->data[i];
    }
    dm->n_words = pr->n_data;

    regfile_init(rf);
    pipeline_init(p, 0u);

    rc = pipeline_run(p, &im, dm, rf, MAX_CYCLES);

    printf("  %s: %s -> %llu ciclos, %llu instr, %llu burbujas, %llu descartes\n",
           pr->id, pr->descripcion,
           (unsigned long long)p->cycles, (unsigned long long)p->instructions,
           (unsigned long long)p->stalls, (unsigned long long)p->flushes);
    return rc;
}

int main(void)
{
    Program  pr;
    Pipeline p;
    DataMem  dm;
    RegFile  rf;
    InstrMem im;
    uint32_t i;
    int      rc;

    MT_BEGIN("Test integral del sistema (vectores S1..S8)");

    /* ---------------- S1: aritmetica ---------------- */
    pr = prog_get(0);
    rc = correr(&pr, &p, &dm, &rf);
    CHECK(rc == 0, "S1 termina sin error");
    CHECK_EQ(rf.r[1], 10u, "S1: $1 = 10");
    CHECK_EQ(rf.r[2], 3u,  "S1: $2 = 3");
    CHECK_EQ(rf.r[3], 13u, "S1: $3 = $1 + $2");
    CHECK_EQ(rf.r[4], 7u,  "S1: $4 = $1 - $2");
    CHECK_EQ(p.instructions, 4u, "S1: se completan 4 instrucciones");

    /* ---------------- S2: logicas ---------------- */
    pr = prog_get(1);
    rc = correr(&pr, &p, &dm, &rf);
    CHECK(rc == 0, "S2 termina sin error");
    CHECK_EQ(rf.r[3], 0x0000000Fu, "S2: and");
    CHECK_EQ(rf.r[4], 0x000000FFu, "S2: or");
    CHECK_EQ(rf.r[5], 0x000000F0u, "S2: xor");
    CHECK_EQ(rf.r[6], 0xFFFFFF00u, "S2: nor");

    /* ---------------- S3: sw + lw ---------------- */
    pr = prog_get(2);
    rc = correr(&pr, &p, &dm, &rf);
    CHECK(rc == 0, "S3 termina sin error");
    CHECK_EQ(dm.w[4], 42u, "S3: sw dejo el dato en la direccion 16");
    CHECK_EQ(rf.r[3], 42u, "S3: lw recupera el mismo dato");

    /* ---------------- S4: riesgo load-use ---------------- */
    pr = prog_get(3);
    rc = correr(&pr, &p, &dm, &rf);
    CHECK(rc == 0, "S4 termina sin error");
    CHECK_EQ(rf.r[3], 7u,  "S4: lw cargo el dato");
    CHECK_EQ(rf.r[4], 14u, "S4: el add usa el valor recien cargado");
    CHECK(p.stalls > 0u, "S4: el riesgo load-use inserto burbujas");

    /* ---------------- S5: beq tomado y no tomado ---------------- */
    pr = prog_get(4);
    rc = correr(&pr, &p, &dm, &rf);
    CHECK(rc == 0, "S5 termina sin error");
    CHECK_EQ(rf.r[3], 0u, "S5: la instruccion saltada no se ejecuto");
    CHECK_EQ(rf.r[4], 0u, "S5: la segunda instruccion saltada tampoco");
    CHECK_EQ(rf.r[5], 7u, "S5: se ejecuta el destino del salto");
    CHECK_EQ(rf.r[6], 3u, "S5: el beq no tomado deja seguir de largo");
    CHECK(p.flushes > 0u, "S5: el salto tomado descarto instrucciones");

    /* ---------------- S6: bne ---------------- */
    pr = prog_get(5);
    rc = correr(&pr, &p, &dm, &rf);
    CHECK(rc == 0, "S6 termina sin error");
    CHECK_EQ(rf.r[3], 0u, "S6: bne tomado salta la instruccion");
    CHECK_EQ(rf.r[4], 1u, "S6: se ejecuta el destino");
    CHECK_EQ(rf.r[5], 2u, "S6: bne con iguales no salta");

    /* ---------------- S7: j y jr ---------------- */
    pr = prog_get(6);
    rc = correr(&pr, &p, &dm, &rf);
    CHECK(rc == 0, "S7 termina sin error");
    CHECK_EQ(rf.r[2], 0u, "S7: j salteo la instruccion 2");
    CHECK_EQ(rf.r[3], 0u, "S7: j salteo la instruccion 3");
    CHECK_EQ(rf.r[4], 7u, "S7: jr volvio a la direccion guardada");

    /* ---------------- S8: bucle y suma de arreglo ---------------- */
    pr = prog_get(7);
    rc = correr(&pr, &p, &dm, &rf);
    CHECK(rc == 0, "S8 termina sin error");
    CHECK_EQ(rf.r[1], 100u, "S8: suma del arreglo");
    CHECK_EQ(rf.r[2], 16u,  "S8: el puntero llego al limite");
    CHECK_EQ(dm.w[8], 100u, "S8: el total quedo guardado en la direccion 32");

    /* ---------------- camino de archivos .txt ---------------- */
    pr = prog_get(7);
    CHECK(mem_dump_words(pr.code, pr.n_code, "build/tmp_imem.txt") == 0,
          "se escribe el .txt de instrucciones");
    CHECK(mem_dump_words(pr.data, pr.n_data, "build/tmp_dmem.txt") == 0,
          "se escribe el .txt de datos");

    mem_init_imem(&im);
    mem_init_dmem(&dm);
    regfile_init(&rf);
    CHECK(mem_load_imem(&im, "build/tmp_imem.txt") == 0, "se recarga imem.txt");
    CHECK(mem_load_dmem(&dm, "build/tmp_dmem.txt") == 0, "se recarga dmem.txt");
    CHECK_EQ(im.n_words, pr.n_code, "imem.txt tiene todas las instrucciones");
    for (i = 0; i < pr.n_code; i++) {
        CHECK_EQ(im.w[i], pr.code[i], "la instruccion sobrevivio al .txt");
    }
    pipeline_init(&p, 0u);
    rc = pipeline_run(&p, &im, &dm, &rf, MAX_CYCLES);
    CHECK(rc == 0, "el programa cargado desde .txt corre igual");
    CHECK_EQ(rf.r[1], 100u, "mismo resultado que en memoria");

    /* ---------------- robustez: escritura a $0 ---------------- */
    mem_init_imem(&im);
    im.w[0] = ASM_ADDI(0, 0, 5);    /* intenta escribir $0 */
    im.w[1] = ASM_ADD(1, 0, 0);     /* $1 = $0 + $0        */
    im.n_words = 2u;
    mem_init_dmem(&dm);
    regfile_init(&rf);
    pipeline_init(&p, 0u);
    rc = pipeline_run(&p, &im, &dm, &rf, MAX_CYCLES);
    CHECK(rc == 0, "el programa con escritura a $0 termina");
    CHECK_EQ(rf.r[0], 0u, "$0 sigue valiendo cero");
    CHECK_EQ(rf.r[1], 0u, "$0 no quedo contaminado");

    /* ---------------- robustez: instruccion ilegal ---------------- */
    mem_init_imem(&im);
    im.w[0] = 0xFC000000u;          /* opcode 0x3F, no existe */
    im.n_words = 1u;
    mem_init_dmem(&dm);
    regfile_init(&rf);
    pipeline_init(&p, 0u);
    rc = pipeline_run(&p, &im, &dm, &rf, MAX_CYCLES);
    CHECK(rc == -1, "la instruccion ilegal detiene el simulador");
    CHECK(p.error == MIPS_ERR_ILLEGAL, "se reporta instruccion ilegal");

    /* ---------------- robustez: lw fuera de rango ---------------- */
    mem_init_imem(&im);
    im.w[0] = ASM_ADDI(1, 0, 0x4000);   /* direccion > DMEM */
    im.w[1] = ASM_LW(2, 0, 1);
    im.n_words = 2u;
    mem_init_dmem(&dm);
    regfile_init(&rf);
    pipeline_init(&p, 0u);
    rc = pipeline_run(&p, &im, &dm, &rf, MAX_CYCLES);
    CHECK(rc == -1, "el acceso fuera de rango detiene el simulador");
    CHECK(p.error == MIPS_ERR_RANGE, "se reporta direccion fuera de rango");

    return MT_END();
}
