#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/memory.h"

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

typedef struct {
    /* 0x00 */ u8 unk0[0x8];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[0x2C];
    /* 0x40 */ u8 *unk40;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ u8 *unk48;
    /* 0x4C */ u8 *unk4C;
    /* 0x50 */ u8 unk50[0x4];
} WorldMapEntry; // Size: 0x54

extern WorldMapSlot *D_8009BE24;
extern WorldMapEntry *D_8009C620;
extern void func_800816DC(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 abr);
extern u8 D_8009A758[];
// externs used by the parked attempts below
extern Vec3 D_8009C5AC;
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s16 D_1F8000A0[];
extern void func_80089160(s32, s16 *, s32);
extern u16 D_8009A6C0;
extern u16 D_8009A70C[];
extern s32 D_8009D144;
extern s32 D_8009BE0C;
extern s32 D_8009BE28;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 D_8009D55C;
extern s32 D_8009D560;
extern s32 D_8009D564;
extern void func_8007A06C(void *, s32, s32);

s32 func_80080900(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->pos.x = D_8009C5AC.x;
    pSlot->pos.y = D_8009C5AC.y;
    pSlot->pos.z = D_8009C5AC.z;
    return 1;
}
// Best: 76.71%. Structure is right (the branch, the three >>12 halfword
// stores and the three func_80089160 calls); the original addresses the
// scratchpad with raw absolute immediates (lui $a1,0x1F80 / ori ...0xA0, and
// `sh $v0,0xA0($at)` with no relocation), while naming D_1F8000A0 in C emits
// %hi/%lo relocations and makes gcc keep the base in a saved register.
// Reaching 100% therefore needs the literal 0x1F8000A0 in the source.
/*
s32 func_80080944(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];

    if (pSlot->unk4 != 1) {
        return 1;
    }
    pSlot->unk4 = 0;
    D_1F8000A0[0] = pSlot->pos.x >> 12;
    D_1F8000A0[1] = pSlot->pos.y >> 12;
    D_1F8000A0[2] = pSlot->pos.z >> 12;
    func_80089160(0x28, (u8 *)D_1F8000A0, 0);
    func_80089160(0x29, (u8 *)D_1F8000A0, 0);
    func_80089160(0x2A, (u8 *)D_1F8000A0, 0);
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080944);

void func_800809EC(u8 *p, s32 count, s32 a, s32 b, s32 c) {
    s32 i;

    for (i = 0; i < count; i++, p += 0x28) {
        p[4] = a;
        p[5] = b;
        p[6] = c;
    }
}

// Best: 92.64%. All instructions and relocations match; only the schedule of
// three insns differs (retail emits `move a0,s0` and keeps `sll a0,a0,7` on a0,
// ours defers both and hoists the D_8009C5B4 load two statements early).
/*
s32 func_80080A28(s32 index) {
    u8 *p = D_8009C620;
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->pos.x = D_8009C5AC.x;
    pSlot->pos.y = 0xFFF80000;
    pSlot->pos.z = D_8009C5B4;
    pSlot->unk54 = -0x800;
    pSlot->unk50 = 0;
    pSlot->unk58 = 0x80;
    func_8007A06C(p, *(s32 *)(p + 0x48), *(u16 *)(*(s32 *)(p + 0x40) + 4));
    p += 0x54;
    func_8007A06C(p, *(s32 *)(p + 0x48), *(u16 *)(*(s32 *)(p + 0x40) + 4));
    return 3;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080A28);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080AC4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080D00);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_8008106C);
// Best: 65.26%. Instruction multiset identical; the original forms the slot
// address (sll/addu/sw/lw) before loading D_8009A6C0, gcc hoists the global
// load above the pointer arithmetic. Decl order, an s32 index temp and an
// explicit `i + 1` store were all tried.
/*
s32 func_80081174(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->unk50 = 0;
    pSlot->unk20 = D_8009A6C0;
    pSlot->unk22 = D_8009A70C[pSlot->unk50];
    pSlot->unk50++;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081174);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800811C0);
// Best: 77.76%. All instructions present and the six global stores match
// exactly (relocations included); the original keeps the two trailing
// `sh` stores to the slot (unk4, unk20) after them, gcc's scheduler groups
// them with the `unk7C` store. Writing them first in the source raises the
// score to this shape but does not move them.
/*
s32 func_800813E8(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    s32 x;
    s32 y;
    s32 z;

    pSlot->unk4 = 1;
    pSlot->unk20 = 0;
    D_8009D144 = 0;
    pSlot->unk7C = 0x1000;
    x = D_8009C5AC.x;
    y = D_8009C5AC.y;
    z = D_8009C5AC.z;
    D_8009BE0C = 0x78;
    D_8009BE28 = x;
    D_8009D55C = x;
    D_8009BE2C = y;
    D_8009D560 = y;
    D_8009BE30 = z;
    D_8009D564 = z;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800813E8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081470);
void func_800816DC(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 abr) {
    s32 i;

    for (i = 0; i < count; i++) {
        pDst[3] = 9;
        pDst[7] = 0x2C;
        *(u16 *)(pDst + 0x16) = GetTPage(0, abr, 0x180, 0);
        pDst[7] |= 2;
        pDst += 0x28;
    }
    memcpy(pEntry->unk4C, pEntry->unk48, count * 0x28);
}

// Best: 66.00%. Structure, relocations and offsets are all right (the call,
// the three >>12 stores, the six slot stores); gcc's scheduler hoists the
// D_8009C5B0/D_8009C5B4 loads and the D_8009C620 load above the `sh 0x20`
// store and sinks the D_8009C5AC load to its store, retail keeps statement
// order. Locals x/y/z and a late pEntry assignment were no better.
/*
s32 func_800817A0(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    WorldMapEntry *pEntry = &D_8009C620[2];

    pSlot->unk20 = 0;
    pSlot->pos.x = D_8009C5AC.x;
    pSlot->pos.y = D_8009C5B0 - 0x100000;
    pSlot->pos.z = D_8009C5B4;
    pSlot->unk50 = 0;
    pSlot->unk54 = 0;
    pSlot->unk58 = 0;
    func_800816DC(pEntry, pEntry->unk48, *(u16 *)(pEntry->unk40 + 4), 1);
    pEntry->unk8 = pSlot->pos.x >> 12;
    pEntry->unkC = pSlot->pos.y >> 12;
    pEntry->unk10 = pSlot->pos.z >> 12;
    return 1;
}
*/
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

// Best: 51.67%. Instruction multiset is identical; the original computes the
// slot pointer (sll/addu) before forming the address of D_8009A758, gcc forms
// the address first. 46.11% for the slot-pointer-first order; declaring pData
// before pSlot puts the symbol address first and reaches 51.67%. Also tried:
// two separate statements, a s32 value temp, `D_8009BE24 + index`, and an
// explicit offset local.
/*
s32 func_800827C8(s32 index) {
    u8 *pData = D_8009A758;
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->unk50 = (s32)pData;
    return 1;
}
*/
s32 func_800827C8(s32 index) {
    D_8009BE24[index].unk50 = (u32)D_8009A758;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800827EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800828DC);
