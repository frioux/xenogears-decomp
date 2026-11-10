#include "common.h"
#include "main/game.h"
#include "psyq/libapi.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"

extern int Vsync(int mode);
extern void func_80039CC4(void);
extern void func_800399D4(s32* pFile);
extern void* func_80039850(void* pFile);
extern void func_80039A80(s32* pFile, s32 volume, s32 frames);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);

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
extern s16 D_8009C872[1];
extern s32 D_8009BCC4;
extern s32 D_8009BE40;
extern s32 D_8009D64C;
extern s32 D_8009D80C;
extern s32* D_80062528;
extern u8 D_80062648[];

void func_8007474C(void) {
    HeapFree(D_8009BE18);
    HeapFree(D_8009BE14);
    HeapFree(D_8009D30C);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80074794);

/* 98.89%: instructions and store order are identical; the allocator puts the
   loaded D_8009D30C pointer in a2 and the index*8 temporary in a3, retail does
   the reverse (also tried: pointer local, index*8 via (u8*) arithmetic,
   declaration order, store order vx/vy/vz vs vx/vz/vy).
void func_80074794(s16 arg0, s32* pVec) {
    Vector16* pEntry;
    s32 index;

    index = D_8009BE38;
    pEntry = (Vector16*)D_8009D30C + index;
    pEntry->vx = pVec[0] >> 12;
    pEntry->vz = pVec[2] >> 12;
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

/* 94.80%: the clear loop and the 0x300/0x180 selection match; upstream gcc CSEs
   the constant 1 into one register (li a0,1 reused for both D_8009D64C and
   D_8009BCC4) while retail re-materialises it (li v1,1 then li v0,1), which also
   drags the width value out of v0 into v1. Also tried: -O3 and gcc-2.7.2-cdk
   (both 85.8), ternary vs if/else, local vs direct store, decl order.
void func_80075228(void) {
    s32 i;
    s32 screenWidth;
    s16* p;

    i = 0xF;
    p = D_8009C872;
    for (; i >= 0; i--) {
        *p = 0;
        p--;
    }
    D_8009D64C = 1;
    screenWidth = (g_GameState.unk1834 & 0x4000) == 0 ? 0x180 : 0x300;
    D_8009BE40 = screenWidth;
    D_8009BCC4 = 1;
    D_8009D80C = 0;
}
*/

/* 91.92%: the loop's base symbol (D_8009C854+0x1E vs D_8009C872), gcc's
   li-constant CSE and the if/else branch polarity all differ.
void func_80075228(void) {
    s32 i;
    s32 screenWidth;

    for (i = 0xF; i >= 0; i--) {
        D_8009C854[i] = 0;
    }
    D_8009D64C = 1;
    if (((u16*)g_GameState.unk17FC)[0x1C] & 0x4000) {
        screenWidth = 0x300;
    } else {
        screenWidth = 0x180;
    }
    D_8009BE40 = screenWidth;
    D_8009BCC4 = 1;
    D_8009D80C = 0;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007528C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075460);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007565C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_800758C0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075B58);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075D4C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075E7C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076098);
void func_800762FC(void) {
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    DrawSync(0);
    Vsync(0);
    FlushCache();
    ExitCriticalSection();
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007634C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076594);
void func_800767D4(u8* pSrc, s32 index) {
    u8* pBuffer;

    func_80039CC4();
    func_800399D4(D_80062528);
    memcpy(D_80062648, pSrc, ArchiveDecodeAlignedSize(index));
    D_80062528 = func_80039850(D_80062648);
    func_80039A80(D_80062528, 0x7F, 0);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076858);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076954);

int func_80076A14(void) {
    return 1;
}
