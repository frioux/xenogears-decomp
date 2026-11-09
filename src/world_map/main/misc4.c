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
    /* 0x2C */ u8 pad2C[0x4];
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
    /* 0x538 */ u8 pad538[0x20];
    /* 0x558 */ SVECTOR unk558;
    /* 0x560 */ MATRIX unk560;
    /* 0x580 */ u8 pad580[0x20];
} WorldMapState; // Size: 0x5A0

extern WorldMapActor *D_8009BE24;
extern u16 D_8009A450;
extern u16 D_8009A46C[];
extern WorldMapState *D_8009C620;
extern void func_80089160(s32, s32, s32);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80077CC0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80077DC8);
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80079538);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_800795E4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80079778);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A06C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A144);
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
/* 67.74%: the whole shape is right (scratchpad halfword stores off
   PSX_SCRATCH + 0xA0/0xA2/0xA4, then func_80089160(0xA, 0x1F8000A0, 0)), but the
   original splits the `li a1,0x1F8000A0` into lui a1,0x1F80 ... ori a1,a1,0xA0
   around the s->unk8 load and interleaves each s->unk8/0xC/0x10 load with its
   store; gcc here hoists the loads and keeps lui/ori adjacent.
s32 func_8007A570(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;

    index *= 0x80;
    p = D_8009BE24;
    s = D_8009C620;
    p = (WorldMapActor *)((u8 *)p + index);
    p->unk4 = 0;
    *(s16 *)((u8 *)PSX_SCRATCH + 0xA0) = s->unk8;
    *(s16 *)((u8 *)PSX_SCRATCH + 0xA2) = s->unkC;
    *(s16 *)((u8 *)PSX_SCRATCH + 0xA4) = s->unk10;
    func_80089160(0xA, (s32)PSX_SCRATCH + 0xA0, 0);
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
