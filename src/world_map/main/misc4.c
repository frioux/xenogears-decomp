#include "common.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ u8 pad0[0x4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 pad6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ s16 unk22;
    /* 0x24 */ u8 pad24[0x4];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ u8 pad34[0x1C];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ u8 pad5C[0x10];
    /* 0x6C */ s32 unk6C;
    /* 0x70 */ s32 unk70;
    /* 0x74 */ s32 unk74;
    /* 0x78 */ u8 pad78[0x8];
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 pad14[0x484];
    /* 0x498 */ u8 unk498[0x40];
    /* 0x4D8 */ u32 *unk4D8;
    /* 0x4DC */ u32 unk4DC;
    /* 0x4E0 */ u8 *unk4E0;
    /* 0x4E4 */ u8 pad4E4[0x8];
    /* 0x4EC */ u8 unk4EC[0x40];
    /* 0x52C */ u32 *unk52C;
    /* 0x530 */ u32 unk530;
    /* 0x534 */ u8 *unk534;
    /* 0x538 */ u8 pad538[0x8];
    /* 0x540 */ s16 unk540;
    /* 0x542 */ u8 pad542[0x6];
    /* 0x548 */ s32 unk548;
    /* 0x54C */ s32 unk54C;
    /* 0x550 */ s32 unk550;
    /* 0x554 */ u8 pad554[0x4];
    /* 0x558 */ SVECTOR unk558;
    /* 0x560 */ MATRIX unk560;
    /* 0x580 */ u8 pad580[0x20];
} WorldMapState; // Size: 0x5A0

extern WorldMapActor *D_8009BE24;
extern u16 D_8009A450;
extern u16 D_8009A46C[];
extern WorldMapState *D_8009C620;
extern s32 D_8009BE0C;
extern s32 D_8009D3F0;
extern s32 D_8009D144;
extern s16 D_8009BD38;
extern s16 D_8009BD3A;
extern s16 D_8009BD3C;
extern void func_80089160(s32, s32, s32);
extern void func_8007A06C(s32, u8 *, u16);
extern u8 *D_8006259C;
extern void func_80093354(void *);
extern s32 func_80093A5C(s32, s32);
extern void func_80039E60(s32);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80077CC0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80077DC8);
/* 70.00%: identical instruction multiset and registers; only the position of the
   WorldMapActor store cluster differs (original puts addu/sh 0x20/sw 0x58/54/50
   before the D_8009BD3C/3A/38 stores, gcc puts them after D_8009BD38). Swapping
   the statement order in C changes nothing, so the scheduler is ordering by
   priority, not source order.
s32 func_80077DC8(s32 index) {
    WorldMapActor *p;
    u8 *q;

    D_8009BE0C = 0x78;
    D_8009D3F0 = 0x200000;
    q = D_8006259C;
    p = D_8009BE24 + index;
    p->unk20 = 0;
    p->unk58 = 0;
    p->unk54 = 0;
    p->unk50 = 0;
    D_8009BD3C = 0;
    D_8009BD3A = 0;
    D_8009BD38 = 0;
    D_8009D144 = 1;
    func_80039E60((((u16 *)q)[0xA] << 16) | 0xA4);
    p->unk22 = 0x18;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80077E68);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007828C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_800783E8);

s32 func_80078948(void) {
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80078950);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80078A60);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80078D24);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80078E2C);
/* 30.80%: instruction multiset is identical; source order already matches the
   original, but gcc's scheduler sinks the WorldMapActor stores to the end of the
   block (sw 0x58/0x54/0x50 land after `li v0,1`) and defers the D_8009BE24 load
   to just before its first use, while the original interleaves them with the
   global stores. `p = D_8009BE24 + index` instead of `index *= 0x80` drops it to
   19.97% (p goes to v0). -O1 scores 45.0.
s32 func_80078E2C(s32 index) {
    WorldMapActor *p;

    D_8009BE0C = 0x78;
    D_8009D3F0 = 0x200000;
    D_8009D144 = 0;
    p = D_8009BE24;
    index *= 0x80;
    p = (WorldMapActor *)((u8 *)p + index);
    p->unk20 = 0x10;
    p->unk58 = 0;
    p->unk54 = 0;
    p->unk50 = 0;
    D_8009BD38 = -0xC0;
    D_8009BD3A = 0x680;
    D_8009BD3C = 0;
    p->unk22 = 0x40;
    p->unk74 = 0;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80078EA4);
s32 func_800794D8(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;

    index *= 0x80;
    p = D_8009BE24;
    s = D_8009C620;
    p = (WorldMapActor *)((u8 *)p + index);
    p->unk28 = 0x02000000;
    p->unk30 = 0x01500000;
    p->unk20 = 0;
    s->unk558.vx = 0;
    s->unk558.vy = 0x780;
    s->unk558.vz = 0;
    RotMatrixYXZ(&s->unk558, &s->unk560);
    return 1;
}
s32 func_80079538(s32 index) {
    WorldMapActor *p;

    p = D_8009BE24 + index;
    if (p->unk4 != 0) {
        p->unk4 = 0;
        D_8009C620->unk540 = 1;
    }
    func_80093354(&p->unk28);
    p->unk2C = func_80093A5C(p->unk28, p->unk30) + 0x18000;
    D_8009C620->unk548 = p->unk28 >> 12;
    D_8009C620->unk54C = p->unk2C >> 12;
    D_8009C620->unk550 = p->unk30 >> 12;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_800795E4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80079778);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A06C);
s32 func_8007A144(s32 index) {
    WorldMapActor *p;

    p = D_8009BE24 + index;
    p->unk20 = 0;
    p->unk70 = 0;
    p->unk6C = 0;
    func_8007A06C((s32)D_8009C620 + 0x498, D_8009C620->unk4E0,
                  ((u16 *)D_8009C620->unk4D8)[2]);
    func_8007A06C((s32)D_8009C620 + 0x4EC, D_8009C620->unk534,
                  ((u16 *)D_8009C620->unk52C)[2]);
    return 3;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A1B4);

s32 func_8007A410(s32 index) {
    WorldMapActor *p;

    index *= 0x80;
    p = D_8009BE24;
    *(s16 *)((u8 *)p + index + 0x22) = 0x60;
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A430);

s32 func_8007A568(void) {
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A570);
/* 92.11%: a `s16 *dst = (s16 *)((u8 *)PSX_SCRATCH + 0xA0)` local (rather than
   three separate `*(s16 *)(PSX_SCRATCH + 0xA0/0xA2/0xA4)` casts, which scored
   67.74%) puts the lui/ori for $a1 where the original has them, and the stores
   reach the right addresses. Only difference left: gcc hoists the 2nd load
   before the 1st store (and puts the 1st value in $a0), so the three
   load/store pairs are not adjacent as in the original.
s32 func_8007A570(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    s16 *dst;

    p = D_8009BE24 + index;
    s = D_8009C620;
    p->unk4 = 0;
    dst = (s16 *)((u8 *)PSX_SCRATCH + 0xA0);
    dst[0] = s->unk8;
    dst[1] = s->unkC;
    dst[2] = s->unk10;
    func_80089160(0xA, (s32)dst, 0);
    return 3;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A5DC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A8AC);
s32 func_8007A9B4(s32 index) {
    WorldMapActor *p;

    p = D_8009BE24;
    index *= 0x80;
    p = (WorldMapActor *)((u8 *)p + index);
    p->unk50 = 0;
    p->unk20 = D_8009A450;
    p->unk22 = D_8009A46C[p->unk50];
    return 1;
}
