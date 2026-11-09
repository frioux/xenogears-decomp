#include "common.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} Vec3; // Size: 0xC

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x4];
    /* 0x28 */ Vec3 pos;
    /* 0x34 */ u8 unk34[0x1C];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ u8 unk5C[0x20];
    /* 0x7C */ s32 unk7C;
} WorldMapSlot; // Size: 0x80

extern WorldMapSlot *D_8009BE24;
extern u8 D_8009A758[];

// Best: 98.82%. Byte-identical to the original except that the 2nd and 3rd
// stores relocate against D_8009C5AC+4/+8 where the original names D_8009C5B0
// and D_8009C5B4; the linker resolves both to the same address, so this is
// worth a check_bin.sh test. Needs a local Vec3 extern:
//   extern Vec3 D_8009C5AC;
/*
s32 func_80080900(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->pos.x = D_8009C5AC.x;
    pSlot->pos.y = D_8009C5AC.y;
    pSlot->pos.z = D_8009C5AC.z;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080900);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080944);

// Best: 66.40%. The 5th argument is word-sized (lw from 0x18(sp)) and the
// counter is incremented at the top of the loop body in the original; this
// shape gets the increment into the right place but turns slt into slt+beqz.
/*
void func_800809EC(u8 *p, s32 count, s32 a, s32 b, s32 c) {
    s32 i;

    for (i = 1; i <= count; i++) {
        p[4] = a;
        p[5] = b;
        p[6] = c;
        p += 0x28;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800809EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080A28);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080AC4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080D00);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_8008106C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081174);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800811C0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800813E8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081470);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800816DC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800817A0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081868);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800819C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081B24);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081C3C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081D80);

s32 func_80081FB4(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->unk20 = 0;
    pSlot->unk50 = 1;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081FD8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80082324);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800826B4);

// Best: 46.11%. Instruction multiset is identical; the original computes the
// slot pointer (sll/addu) before forming the address of D_8009A758, gcc forms
// the address first. Decl/locals order, a u8 *pData temp and an explicit
// (u8 *)D_8009BE24 + index * 0x80 all give the same 46.11%.
/*
s32 func_800827C8(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    u8 *pData = D_8009A758;

    pSlot->unk50 = (s32)pData;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800827C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800827EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800828DC);
