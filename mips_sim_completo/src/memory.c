/* =====================================================================
 * memory.c -- Memoria de instrucciones, memoria de datos y banco de
 *             registros. Los tres se cargan desde archivos de TEXTO
 *             (.txt) con una palabra de 32 bits por linea.
 *
 *   imem.txt -> una palabra por instruccion, en orden
 *   dmem.txt -> contenido inicial de la memoria de datos
 *   regs.txt -> 32 palabras con el valor inicial de $0..$31
 *
 * Formato: cada palabra se escribe como 32 caracteres 0/1, una por linea.
 * Eso es lo que se ve al abrir cualquiera de los tres archivos.
 *
 * El lector, ademas del binario, tolera:
 *   - hexadecimal, con o sin prefijo: 20010005  o  0x20010005
 *   - lineas en blanco y comentarios que empiecen con # o //
 *   - mas de una palabra por linea, separadas por espacios
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mips.h"

#define LINE_MAX_LEN 512

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
 * Convierte un token de texto en una palabra de 32 bits.
 * Devuelve 0 si el token es valido, -1 si no.
 * --------------------------------------------------------------------- */
static int parse_word(const char *tok, uint32_t *out)
{
    size_t        len = strlen(tok);
    char         *end = NULL;
    unsigned long v;

    if (len == 32u) {
        size_t i;
        int    solo_bits = 1;

        for (i = 0; i < 32u; i++) {
            if (tok[i] != '0' && tok[i] != '1') {
                solo_bits = 0;
            }
        }
        if (solo_bits) {
            v = strtoul(tok, &end, 2);
            *out = (uint32_t)v;
            return 0;
        }
    }

    v = strtoul(tok, &end, 16);
    if (end == tok || *end != '\0') {
        return -1;   /* no era un numero */
    }
    *out = (uint32_t)v;
    return 0;
}

/* Recorta el comentario de la linea: todo lo que siga a # o a // */
static void quitar_comentario(char *linea)
{
    char *p = strchr(linea, '#');

    if (p != NULL) {
        *p = '\0';
    }
    p = strstr(linea, "//");
    if (p != NULL) {
        *p = '\0';
    }
}

/* ---------------------------------------------------------------------
 * Lectura generica de un .txt a un arreglo de palabras.
 * Devuelve 0 si todo salio bien, -1 si hubo error.
 * --------------------------------------------------------------------- */
static int load_words(const char *path, uint32_t *dst, uint32_t max_words,
                      uint32_t *n_words_out)
{
    FILE    *f;
    char     linea[LINE_MAX_LEN];
    uint32_t n = 0;
    uint32_t nro_linea = 0;

    if (path == NULL || dst == NULL || n_words_out == NULL) {
        return -1;
    }

    f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "error: no se pudo abrir '%s'\n", path);
        return -1;
    }

    while (fgets(linea, (int)sizeof(linea), f) != NULL) {
        char *tok;

        nro_linea++;
        quitar_comentario(linea);

        tok = strtok(linea, " \t\r\n");
        while (tok != NULL) {
            uint32_t palabra = 0u;

            if (n >= max_words) {
                fprintf(stderr, "error: '%s' excede la capacidad (%u palabras)\n",
                        path, max_words);
                fclose(f);
                return -1;
            }
            if (parse_word(tok, &palabra) != 0) {
                fprintf(stderr, "error: '%s' linea %u: '%s' no es una palabra valida\n",
                        path, nro_linea, tok);
                fclose(f);
                return -1;
            }
            dst[n] = palabra;
            n++;
            tok = strtok(NULL, " \t\r\n");
        }
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
    rf->r[0] = 0u;   /* $0 siempre vale cero, pase lo que pase en el archivo */
    return rc;
}

/* Vuelca palabras a un .txt: 32 caracteres 0/1 por linea, del bit 31 al 0. */
int mem_dump_words(const uint32_t *w, uint32_t n_words, const char *path)
{
    FILE    *f;
    uint32_t i;
    int      b;
    char     linea[33];

    if (w == NULL || path == NULL) {
        return -1;
    }

    f = fopen(path, "w");
    if (f == NULL) {
        fprintf(stderr, "error: no se pudo escribir '%s'\n", path);
        return -1;
    }

    linea[32] = '\0';
    for (i = 0; i < n_words; i++) {
        for (b = 31; b >= 0; b--) {
            linea[31 - b] = ((w[i] >> b) & 1u) ? '1' : '0';
        }
        if (fprintf(f, "%s\n", linea) < 0) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}
