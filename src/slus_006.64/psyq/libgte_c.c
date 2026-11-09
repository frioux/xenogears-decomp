#include "common.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgte_c", RotMatrix);

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
