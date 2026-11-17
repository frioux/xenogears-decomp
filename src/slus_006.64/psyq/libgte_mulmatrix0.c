#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// MulMatrix0 (LIBGTE/MTX_000.OBJ) is its own translation unit only because the
// shipped .OBJ pads its tail to a 16-byte multiple: the nop at 0x39B18 is the
// `asm` subsegment in config/slus_006.64.yaml, so psyq/libgte_tail starts at the
// next function. Nothing else separates the two files.
//
// Transcribed as the asm Sony wrote, like CompMatrix in psyq/libgte_tail: it is
// gte_MulMatrix0 with the 3.x macros -- SetRotMatrix's five ctc2s, then three
// `ldclmv`/`rtir`/`stclmv` rounds whose store side packs the columns into whole
// words instead of the 4.x macro's three `sh`s.

MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2) {
    register MATRIX *pRet __asm__("$2");

    __asm__ volatile (".set\tnoreorder\n\t"
                      "lw\t$8, 0($4)\n\t"
                      "lw\t$9, 4($4)\n\t"
                      "lw\t$10, 8($4)\n\t"
                      "lw\t$11, 12($4)\n\t"
                      "lw\t$12, 16($4)\n\t"
                      "ctc2\t$8, $0\n\t"
                      "ctc2\t$9, $1\n\t"
                      "ctc2\t$10, $2\n\t"
                      "ctc2\t$11, $3\n\t"
                      "ctc2\t$12, $4\n\t"
                      "lhu\t$8, 0($5)\n\t"
                      "lw\t$9, 4($5)\n\t"
                      "lw\t$10, 12($5)\n\t"
                      "lui\t$1, 65535\n\t"
                      "and\t$9, $9, $1\n\t"
                      "or\t$8, $8, $9\n\t"
                      "mtc2\t$8, $0\n\t"
                      "mtc2\t$10, $1\n\t"
                      "nop\n\t"
                      ".word\t0x4a486012\n\t"
                      "lhu\t$8, 2($5)\n\t"
                      "lw\t$9, 8($5)\n\t"
                      "lh\t$10, 14($5)\n\t"
                      "sll\t$9, $9, 16\n\t"
                      "or\t$8, $8, $9\n\t"
                      "mfc2\t$11, $9\n\t"
                      "mfc2\t$12, $10\n\t"
                      "mfc2\t$13, $11\n\t"
                      "mtc2\t$8, $0\n\t"
                      "mtc2\t$10, $1\n\t"
                      "nop\n\t"
                      ".word\t0x4a486012\n\t"
                      "lhu\t$8, 4($5)\n\t"
                      "lw\t$9, 8($5)\n\t"
                      "lw\t$10, 16($5)\n\t"
                      "lui\t$1, 65535\n\t"
                      "and\t$9, $9, $1\n\t"
                      "or\t$8, $8, $9\n\t"
                      "mfc2\t$14, $9\n\t"
                      "mfc2\t$15, $10\n\t"
                      "mfc2\t$24, $11\n\t"
                      "mtc2\t$8, $0\n\t"
                      "mtc2\t$10, $1\n\t"
                      "nop\n\t"
                      ".word\t0x4a486012\n\t"
                      "andi\t$11, $11, 65535\n\t"
                      "sll\t$14, $14, 16\n\t"
                      "or\t$14, $14, $11\n\t"
                      "sw\t$14, 0($6)\n\t"
                      "andi\t$13, $13, 65535\n\t"
                      "sll\t$24, $24, 16\n\t"
                      "or\t$24, $24, $13\n\t"
                      "sw\t$24, 12($6)\n\t"
                      "mfc2\t$8, $9\n\t"
                      "mfc2\t$9, $10\n\t"
                      "andi\t$8, $8, 65535\n\t"
                      "sll\t$12, $12, 16\n\t"
                      "or\t$8, $8, $12\n\t"
                      "sw\t$8, 4($6)\n\t"
                      "andi\t$15, $15, 65535\n\t"
                      "sll\t$9, $9, 16\n\t"
                      "or\t$9, $9, $15\n\t"
                      "sw\t$9, 8($6)\n\t"
                      "swc2\t$11, 16($6)\n\t"
                      "addu\t$2, $6, $0\n\t"
                      ".set\treorder"
                      : "=r"(pRet) : : "$1", "$8", "$9", "$10", "$11", "$12",
                          "$13", "$14", "$15", "$24", "memory");
    return pRet;
}
