#include "common.h"
#include "psyq/libgte.h"
#include "system/memory.h"

typedef struct {
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ u16 pad6;
} Vector16; // Size: 0x8

extern u8* D_8009BE14;
extern u8* D_8009BE18;
extern s32 D_8009BE38;
extern u8* D_8009D30C;
extern u8* D_8009D780;
extern u8* D_8009D7D0;

void func_8007474C(void) {
    HeapFree(D_8009BE18);
    HeapFree(D_8009BE14);
    HeapFree(D_8009D30C);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80074794);

/* 98.89%: the only difference is the allocator's choice of a2/a3 for the loaded
   D_8009D30C pointer and the index*8 temporary.
void func_80074794(s16 arg0, VECTOR* pVec) {
    Vector16* pEntry;
    s32 index;

    index = D_8009BE38;
    pEntry = (Vector16*)D_8009D30C + index;
    pEntry->vx = pVec->vx >> 12;
    pEntry->vz = pVec->vz >> 12;
    pEntry->vy = arg0;
    D_8009BE38 = (index + 1) & 0xF;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_800747DC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80074E58);

void func_80074F04(void) {
    HeapFree(D_8009D780);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80074F2C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075030);

void func_800750DC(void) {
    HeapFree(D_8009D7D0);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075104);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075228);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007528C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075460);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007565C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_800758C0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075B58);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075D4C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075E7C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076098);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_800762FC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007634C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076594);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_800767D4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076858);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076954);

int func_80076A14(void) {
    return 1;
}
