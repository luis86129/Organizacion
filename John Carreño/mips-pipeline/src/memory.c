/* =====================================================================
 * memory.c -- Memoria de instrucciones, memoria de datos y banco de
 *             registros. Los tres se cargan desde archivos .bin que
 *             contienen palabras de 32 bits en little-endian.
 *
 *   imem.bin -> una palabra por instruccion, en orden
 *   dmem.bin -> contenido inicial de la memoria de datos
 *   regs.bin -> 32 palabras con el valor inicial de $0..$31
 * ===================================================================== */
#include <stdio.h>
#include <string.h>
#include "mips.h"

const char *mips_error_str(MipsError e)
{
    switch (e) {
        case MIPS_OK:          return "ok";
        case MIPS_ERR_ALIGN:   return "direccion no alineada a 4 bytes";
        case MIPS_ERR_RANGE:   return "direccion fuera de rango";
        case MIPS_ERR_ILLEGAL: return "instruccion ilegal";
        case MIPS_ERR_NULL:    return "parametro nulo";
        default:               return "desconocido";
    }
}

void mem_init_imem(InstrMem *im)
{
    if (im != NULL) {
        memset(im, 0, sizeof(*im));
    }
}

void mem_init_dmem(DataMem *dm)
{
    if (dm != NULL) {
        memset(dm, 0, sizeof(*dm));
    }
}

void regfile_init(RegFile *rf)
{
    if (rf != NULL) {
        memset(rf, 0, sizeof(*rf));
    }
}

/* ---------------------------------------------------------------------
 * Lectura generica de un .bin a un arreglo de palabras. Se arma la
 * palabra byte por byte para no depender del endianness de la maquina.
 * Devuelve 0 si todo salio bien, -1 si hubo error.
 * --------------------------------------------------------------------- */
static int load_words(const char *path, uint32_t *dst, uint32_t max_words,
                      uint32_t *n_words_out)
{
    FILE    *f;
    uint8_t  b[4];
    uint32_t n = 0;
    size_t   rd;

    if (path == NULL || dst == NULL || n_words_out == NULL) {
        return -1;
    }

    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "error: no se pudo abrir '%s'\n", path);
        return -1;
    }

    rd = fread(b, 1, 4, f);
    while (rd == 4 && n < max_words) {
        dst[n] = (uint32_t)b[0]        | ((uint32_t)b[1] << 8) |
                 ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
        n++;
        rd = fread(b, 1, 4, f);
    }

    if (rd != 0 && rd != 4) {
        fprintf(stderr, "error: '%s' no contiene un multiplo de 4 bytes\n", path);
        fclose(f);
        return -1;
    }
    if (rd == 4 && n >= max_words) {
        fprintf(stderr, "error: '%s' excede la capacidad (%u palabras)\n",
                path, max_words);
        fclose(f);
        return -1;
    }

    fclose(f);
    *n_words_out = n;
    return 0;
}

int mem_load_imem(InstrMem *im, const char *path)
{
    if (im == NULL) {
        return -1;
    }
    mem_init_imem(im);
    return load_words(path, im->w, IMEM_WORDS, &im->n_words);
}

int mem_load_dmem(DataMem *dm, const char *path)
{
    if (dm == NULL) {
        return -1;
    }
    mem_init_dmem(dm);
    return load_words(path, dm->w, DMEM_WORDS, &dm->n_words);
}

int regfile_load(RegFile *rf, const char *path)
{
    uint32_t n = 0;
    int      rc;

    if (rf == NULL) {
        return -1;
    }
    regfile_init(rf);
    rc = load_words(path, rf->r, NUM_REGS, &n);
    rf->r[0] = 0u;   /* $0 siempre vale cero, pase lo que pase en el .bin */
    return rc;
}

/* Vuelca palabras a un .bin, tambien en little-endian. */
int mem_dump_words(const uint32_t *w, uint32_t n_words, const char *path)
{
    FILE    *f;
    uint32_t i;
    uint8_t  b[4];

    if (w == NULL || path == NULL) {
        return -1;
    }

    f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "error: no se pudo escribir '%s'\n", path);
        return -1;
    }

    for (i = 0; i < n_words; i++) {
        b[0] = (uint8_t)( w[i]        & 0xFFu);
        b[1] = (uint8_t)((w[i] >> 8)  & 0xFFu);
        b[2] = (uint8_t)((w[i] >> 16) & 0xFFu);
        b[3] = (uint8_t)((w[i] >> 24) & 0xFFu);
        if (fwrite(b, 1, 4, f) != 4) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}
