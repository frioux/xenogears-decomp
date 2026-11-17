#include "common.h"
#include "psyq/libgte.h"

extern long D_800523F0[]; // one entry per 1/4096 turn, (cos << 16) | sin

/* Sony's vector.s, transcribed like RotMatrixX/Y/Z (psyq/libgte_average_*):
   gcc emits `mult` where the original has `multu` and reads the trig table
   through its own registers, so a C body cannot match. RotMatrix builds the
   whole rotation matrix from the two sines and two cosines of the three
   angles packed in r: vx|vy<<16 at 0(r), vz at 4(r). */
MATRIX *RotMatrix(SVECTOR *r, MATRIX *m) {
    __asm__ volatile (".set\tnoreorder\n\t"
                      "lw\t$9, 0($4)\n\t"      /* vx | vy << 16 */
                      "lh\t$11, 4($4)\n\t"     /* vz */
                      "srl\t$10, $9, 16\n\t"
                      "andi\t$9, $9, 0xFFF\n\t"
                      "andi\t$10, $10, 0xFFF\n\t"
                      "sll\t$8, $10, 2\n\t"
                      "lui\t$1, %%hi(D_800523F0)\n\t"
                      "addu\t$1, $1, $8\n\t"
                      "lw\t$13, %%lo(D_800523F0)($1)\n\t"
                      "andi\t$11, $11, 0xFFF\n\t"
                      "sll\t$24, $13, 16\n\t"
                      "sll\t$8, $11, 2\n\t"
                      "lui\t$1, %%hi(D_800523F0)\n\t"
                      "addu\t$1, $1, $8\n\t"
                      "lw\t$14, %%lo(D_800523F0)($1)\n\t"
                      "sra\t$13, $13, 16\n\t"
                      "sll\t$25, $14, 16\n\t"
                      "sra\t$25, $25, 16\n\t"
                      "sra\t$14, $14, 16\n\t"
                      "mult\t$14, $13\n\t"
                      "sll\t$10, $9, 2\n\t"
                      "lui\t$1, %%hi(D_800523F0)\n\t"
                      "addu\t$1, $1, $10\n\t"
                      "lw\t$12, %%lo(D_800523F0)($1)\n\t"
                      "mflo\t$8\n\t"
                      "sll\t$15, $12, 16\n\t"
                      "sra\t$12, $12, 16\n\t"
                      "mult\t$25, $12\n\t"
                      "sra\t$24, $24, 16\n\t"
                      "neg\t$24, $24\n\t"
                      "mflo\t$9\n\t"
                      "sra\t$8, $8, 12\n\t"
                      "sh\t$8, 0($5)\n\t"
                      "mult\t$14, $24\n\t"
                      "sra\t$15, $15, 16\n\t"
                      "mflo\t$8\n\t"
                      "sra\t$9, $9, 12\n\t"
                      "sra\t$8, $8, 12\n\t"
                      "mult\t$8, $15\n\t"
                      "mflo\t$10\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mult\t$25, $15\n\t"
                      "sra\t$10, $10, 12\n\t"
                      "mflo\t$11\n\t"
                      "sub\t$9, $9, $10\n\t"
                      "sh\t$9, 6($5)\n\t"
                      "mult\t$8, $12\n\t"
                      "mflo\t$10\n\t"
                      "sra\t$11, $11, 12\n\t"
                      "nop\n\t"
                      "mult\t$25, $13\n\t"
                      "sra\t$10, $10, 12\n\t"
                      "mflo\t$8\n\t"
                      "add\t$10, $10, $11\n\t"
                      "sh\t$10, 12($5)\n\t"
                      "mult\t$14, $12\n\t"
                      "neg\t$8, $8\n\t"
                      "mflo\t$10\n\t"
                      "sra\t$8, $8, 12\n\t"
                      "sh\t$8, 2($5)\n\t"
                      "mult\t$25, $24\n\t"
                      "mflo\t$8\n\t"
                      "sra\t$10, $10, 12\n\t"
                      "sra\t$8, $8, 12\n\t"
                      "mult\t$8, $15\n\t"
                      "mflo\t$11\n\t"
                      "nop\n\t"
                      "nop\n\t"
                      "mult\t$14, $15\n\t"
                      "sra\t$11, $11, 12\n\t"
                      "mflo\t$9\n\t"
                      "add\t$10, $10, $11\n\t"
                      "sh\t$10, 8($5)\n\t"
                      "mult\t$8, $12\n\t"
                      "mflo\t$10\n\t"
                      "sra\t$9, $9, 12\n\t"
                      "nop\n\t"
                      "mult\t$13, $15\n\t"
                      "sra\t$10, $10, 12\n\t"
                      "sub\t$9, $9, $10\n\t"
                      "sh\t$9, 14($5)\n\t"
                      "mflo\t$8\n\t"
                      "neg\t$11, $24\n\t"
                      "sh\t$11, 4($5)\n\t"
                      "mult\t$13, $12\n\t"
                      "neg\t$8, $8\n\t"
                      "mflo\t$9\n\t"
                      "sra\t$8, $8, 12\n\t"
                      "sh\t$8, 10($5)\n\t"
                      "sra\t$9, $9, 12\n\t"
                      "sh\t$9, 16($5)\n\t"
                      ".set\treorder"
                      : : : "$1", "$2", "$8", "$9", "$10", "$11", "$12", "$13",
                          "$14", "$15", "$24", "$25", "memory");
    return m;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_c", rcos);
/* best attempt 98.57%. One instruction differs: the original shifts the masked
   angle into a fresh register (sll t0,a0,2) while gcc 2.7.2 always folds it
   (sll a0,a0,2), i.e. it treats the andi result as dead. Eight spellings of the
   index (<<2, *4, u_int temp, [][2] subscript, pointer local, a % 0x1000,
   a &= 0xfff statement) all produce the in-place form, at -O2/-O3 alike.
   D_800523F2 is the auto-generated undefined symbol at D_800523F0 + 2, the sin
   half of each entry.
extern long D_800523F0[];
extern long D_800523F2[];

int rcos(int a) {
    return *(short*)((char*)D_800523F0 + (a & 0xFFF) * 4);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_c", rsin);
/* best attempt 98.57%, same one-instruction difference as rcos.
int rsin(int a) {
    return *(short*)((char*)D_800523F2 + (a & 0xFFF) * 4);
}
*/
