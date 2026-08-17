/* =====================================================================
 * pipeline.c -- Orquestador del pipeline de 5 etapas.
 *
 * Puntos criticos del diseno (los detecto el rol Critico en la Parte 1):
 *
 *  1) Las etapas se ejecutan en ORDEN INVERSO (WB, MEM, EX, ID, IF).
 *     Si se llamaran en orden directo actualizando el estado sobre la
 *     marcha, una misma instruccion atravesaria las cinco etapas en un
 *     solo ciclo y esto dejaria de ser un pipeline.
 *
 *  2) WB escribe el banco ANTES de que ID lea, asi que una dependencia
 *     a distancia 3 se resuelve sola (escritura en la primera mitad del
 *     ciclo, lectura en la segunda).
 *
 *  3) Sin forwarding: los riesgos de datos se resuelven con burbujas.
 *     Se compara el registro que ID va a leer contra los destinos de las
 *     instrucciones que estan en EX y en MEM.
 *
 *  4) Los saltos se resuelven en EX, con dos ciclos de penalidad: al
 *     tomarse hay que descartar lo que quedo en IF/ID y en ID/EX.
 * ===================================================================== */
#include <string.h>
#include "mips.h"

/* ---------------------------------------------------------------------
 * Que registros lee la instruccion que esta en la etapa ID.
 * --------------------------------------------------------------------- */
static void id_reads(uint32_t instr, int *use_rs, int *use_rt,
                     uint8_t *rs, uint8_t *rt)
{
    uint8_t op    = (uint8_t)((instr >> 26) & 0x3Fu);
    uint8_t funct = (uint8_t)( instr        & 0x3Fu);

    *rs = (uint8_t)((instr >> 21) & 0x1Fu);
    *rt = (uint8_t)((instr >> 16) & 0x1Fu);
    *use_rs = 0;
    *use_rt = 0;

    if (op == OP_RTYPE) {
        if (funct == F_JR) {
            *use_rs = 1;    /* jr solo necesita $rs */
        } else {
            *use_rs = 1;
            *use_rt = 1;
        }
    } else if (op == OP_ADDI || op == OP_LW) {
        *use_rs = 1;
    } else if (op == OP_SW || op == OP_BEQ || op == OP_BNE) {
        *use_rs = 1;
        *use_rt = 1;
    }
    /* j no lee ningun registro */
}

/* Destino de la instruccion que esta en EX (el mux regDst todavia no se
 * aplico, hay que replicarlo aqui). Devuelve 0 si no escribe. */
static uint8_t idex_dest(const IDEX *x)
{
    if (x->valid == 0 || x->c.reg_write == 0u) {
        return 0u;
    }
    if (x->c.reg_dst != 0u) {
        return x->rd;
    }
    return x->rt;
}

/* Destino de la instruccion que esta en MEM. Devuelve 0 si no escribe. */
static uint8_t exmem_dest(const EXMEM *x)
{
    if (x->valid == 0 || x->c.reg_write == 0u) {
        return 0u;
    }
    return x->write_reg;
}

/* Devuelve 1 si hay que insertar una burbuja. */
int hazard_detect(const IFID *ifid, const IDEX *idex, const EXMEM *exmem)
{
    int     use_rs = 0, use_rt = 0;
    uint8_t rs = 0, rt = 0;
    uint8_t d_ex, d_mem;

    if (ifid == NULL || idex == NULL || exmem == NULL) {
        return 0;
    }
    if (ifid->valid == 0) {
        return 0;
    }

    id_reads(ifid->instr, &use_rs, &use_rt, &rs, &rt);
    d_ex  = idex_dest(idex);
    d_mem = exmem_dest(exmem);

    if (d_ex != 0u) {
        if ((use_rs && rs == d_ex) || (use_rt && rt == d_ex)) {
            return 1;
        }
    }
    if (d_mem != 0u) {
        if ((use_rs && rs == d_mem) || (use_rt && rt == d_mem)) {
            return 1;
        }
    }
    return 0;
}

void pipeline_init(Pipeline *p, uint32_t start_pc)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->pc    = start_pc;
    p->error = MIPS_OK;
}

/* Un ciclo de reloj. Devuelve 1 cuando el pipeline quedo detenido. */
int pipeline_step(Pipeline *p, const InstrMem *im, DataMem *dm, RegFile *rf)
{
    IFIn   ifin;
    IFOut  fe;
    IDOut  id;
    EXOut  ex;
    MEMOut me;
    WBOut  wb;
    int    stall;
    int    taken;

    /* Validaciones previas antes de empezar el algoritmo. */
    if (p == NULL || im == NULL || dm == NULL || rf == NULL) {
        return 1;
    }
    if (p->halted) {
        return 1;
    }

    /* ---- Etapa 5: Write Back (primero, para que ID lea lo recien escrito) */
    if (p->memwb.valid) {
        p->instructions++;
    }
    wb = stage_wb(&p->memwb, rf);
    (void)wb;   /* el resultado se usa solo para depurar/probar */

    /* ---- Etapa 4: Memory */
    me = stage_mem(&p->exmem, dm);

    /* ---- Etapa 3: ALU */
    ex = stage_ex(&p->idex);
    taken = ex.exmem.branch_taken;

    /* ---- Etapa 2: Decode */
    id = stage_id(&p->ifid, rf);

    /* Riesgo de datos contra las instrucciones que estan en EX y MEM.
     * Un salto tomado manda: lo que estaba en ID se va a descartar. */
    stall = hazard_detect(&p->ifid, &p->idex, &p->exmem);
    if (taken) {
        stall = 0;
    }

    /* ---- Etapa 1: Fetch */
    ifin.pc     = p->pc;
    ifin.taken  = taken;
    ifin.target = ex.exmem.branch_pc;
    fe = stage_if(&ifin, im);

    /* ---- Escritura de los registros de pipeline (fin del ciclo) */
    p->memwb = me.memwb;
    p->exmem = ex.exmem;

    if (stall) {
        memset(&p->idex, 0, sizeof(p->idex));   /* burbuja hacia EX */
        p->stalls++;                            /* IF/ID y PC se congelan */
    } else {
        p->idex = id.idex;
        p->ifid = fe.ifid;
        p->pc   = fe.next_pc;
    }

    if (taken) {
        /* Se descarta el camino no tomado: dos instrucciones en vuelo. */
        if (p->ifid.valid) {
            p->flushes++;
        }
        if (p->idex.valid) {
            p->flushes++;
        }
        memset(&p->ifid, 0, sizeof(p->ifid));
        memset(&p->idex, 0, sizeof(p->idex));
    }

    p->cycles++;

    /* ---- Errores. Si el salto se tomo, lo que decodifico ID venia del
     * camino falso y su error no cuenta. */
    if (fe.error != MIPS_OK) {
        p->error = fe.error;
    } else if (ex.error != MIPS_OK) {
        p->error = ex.error;
    } else if (me.error != MIPS_OK) {
        p->error = me.error;
    } else if (id.error != MIPS_OK && !taken) {
        p->error = id.error;
    }

    if (p->error != MIPS_OK) {
        p->halted = 1;
        return 1;
    }

    /* ---- Fin normal: no queda nada en vuelo y el PC ya salio del programa */
    if (p->ifid.valid == 0 && p->idex.valid == 0 &&
        p->exmem.valid == 0 && p->memwb.valid == 0 &&
        (p->pc / 4u) >= im->n_words) {
        p->halted = 1;
        return 1;
    }

    return 0;
}

/* Corre hasta terminar. Devuelve 0 si termino bien, -1 si hubo error o
 * si se agotaron los ciclos (programa que no termina). */
int pipeline_run(Pipeline *p, const InstrMem *im, DataMem *dm, RegFile *rf,
                 uint64_t max_cycles)
{
    int fin = 0;

    if (p == NULL || im == NULL || dm == NULL || rf == NULL) {
        return -1;
    }

    while (fin == 0 && p->cycles < max_cycles) {
        fin = pipeline_step(p, im, dm, rf);
    }

    if (p->error != MIPS_OK) {
        return -1;
    }
    if (p->halted == 0) {
        return -1;   /* se agotaron los ciclos */
    }
    return 0;
}
