#include "common.h"
#include "main/game.h"

extern void* D_800C34B0;
extern u8 D_800CCE30[];
extern u8 D_800CCE4A[];

extern int rand(void);
extern void func_8009BE0C(void);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A0DC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A1AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A258);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A2D4);

u8 func_8009A7B8(u8 arg0) {
    return g_GameState.characters[arg0].unk0[5];
}

s32 func_8009A7E4(u8 arg0) {
    u8 id = *(u8*)((u8*)D_800C34B0 + arg0 * 16 + 0x581E);

    s32 ret;

    if (g_GameState.characters[4].unk0[6] == id) {
        ret = 1;
    } else if (g_GameState.characters[4].unk0[0xE] == id) {
        ret = 1;
    } else if (g_GameState.characters[4].unk0[0x16] == id) {
        ret = 1;
    } else {
        ret = g_GameState.characters[4].unk0[0x1E] == id;
    }
    return ret;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A854);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A9D0);
/* Best attempt: 99.66%. Only difference: the mfhi temp of the `% 100` expansion
   lands in $a1 instead of the original's $a0 (allocator tie-break, 2 insns).
s32 func_8009A9D0(void) {
    *(u8*)((u8*)D_800C34B0 + 0x5FC2) = 0;
    if (rand() % 100 < 50) {
        func_8009BE0C();
        return 1;
    }
    return 0;
}
*/


INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AA44);

u8 func_8009AB00(u8 arg0) {
    u8* p = (u8*)D_800C34B0 + arg0 * 0x170;
    u8 flags = p[0x15A] & 0xFE;

    p[0x15A] = flags;
    return flags;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AB38);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AC48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009ADA0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AEFC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AFD8);

void func_8009B098(void) {
    u8 i;

    for (i = 1; i < 3; i++) {
        u8* p = (u8*)(i * 0x170 + (u32)D_800C34B0);

        *(u16*)(p + 0x4C) = 0x64;
        *(u16*)(p + 0x4E) = 0x64;
        p[0x5E] = 0x14;
        p[0x5F] = 0xF;
        *(u16*)(p + 0x7A) = 0x1FBF;
        p[0x149] = 0;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B104);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B1E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B46C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B684);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BAC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BD94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BE0C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009C050);

void func_8009C0E0(u8 arg0) {
    s32 off = arg0 * 0x170;

    D_800CCE30[off] = 4;
    D_800CCE4A[off] = 3;
    g_GameState.unk16BC[0].unk1E |= 0x4000;
}
