#include "common.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ u16 unk04;
    /* 0x06 */ u16 unk06;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ undefined32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ undefined32 unk48;
    /* 0x4C */ undefined32 unk4C;
    /* 0x50 */ int unk50;
    /* 0x54 */ void *unk54;
    /* 0x58 */ void *unk58;
    /* 0x5C */ undefined32 unk5C[8]; // 0x5C..0x7B
    /* 0x7C */ u32 unk7C;
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x00 */ s16 unk00;
    /* 0x02 */ s16 unk02;
    /* 0x04 */ undefined16 unk04[2]; // 0x04..0x07
    /* 0x08 */ s32 unk08;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ undefined32 unk14;
    /* 0x18 */ SVECTOR unk18;
    /* 0x20 */ MATRIX unk20;
    /* 0x40 */ void *unk40;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ void *unk48;
    /* 0x4C */ undefined32 unk4C;
    /* 0x50 */ undefined32 unk50;
} WorldMapGroup; // Size: 0x54

typedef struct {
    /* 0x000 */ WorldMapGroup unk000[17]; // 0x000..0x593
} WorldMapRoot;

typedef struct {
    /* 0x0 */ u16 *unk0;
    /* 0x4 */ u16 *unk4;
} WorldMapPath; // Size: 0x8

extern WorldMapActor *D_8009BE24;
extern WorldMapRoot *D_8009C620;
extern u32 D_8009C5AC;
extern u32 D_8009C5B0;
extern u32 D_8009C5B4;
extern u16 D_8009A698;
extern u16 D_8009A6AC[];
extern WorldMapPath D_8009A65C[];
extern u32 D_8009BE0C;
extern u32 D_8009BE28;
extern u32 D_8009BE2C;
extern u32 D_8009BE30;
extern u32 D_8009D144;
extern u32 D_8009D3D4;
extern u32 D_8009D55C;
extern u32 D_8009D560;
extern u32 D_8009D564;

extern void func_8007A06C(void *, void *, int);
extern void func_800848B4(int, int);
extern void func_80089160(int, void *, int);
extern void func_80093354(VECTOR *);
extern int func_80093A5C(int, int);

int func_8007D414(int index) {
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    WorldMapActor *pActor;

    func_800848B4(0xC, 0xF);
    pGroup = &D_8009C620->unk000[12];
    pRot = &pGroup->unk18;
    pMatrix = &pGroup->unk20;
    pGroup->unk00 = 0;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    pActor = &D_8009BE24[index];
    pActor->unk40 = -0x4000;
    pActor->unk28 = 0xE80000;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk2C = 0;
    pActor->unk30 = 0x280000;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007D4A4);

int func_8007D600(int index) {
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    WorldMapActor *pActor;

    func_800848B4(0xD, 0xE);
    pGroup = &D_8009C620->unk000[13];
    pRot = &pGroup->unk18;
    pMatrix = &pGroup->unk20;
    pGroup->unk00 = 0;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    pActor = &D_8009BE24[index];
    pActor->unk40 = -0x4000;
    pActor->unk28 = 0xF80000;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk2C = 0;
    pActor->unk30 = 0x380000;
    return 1;
}

int func_8007D690(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    WorldMapGroup *pGroup = &D_8009C620->unk000[13];
    SVECTOR *pScratch = (SVECTOR *)((u8 *)PSX_SCRATCH + 0xA0);

    pActor->unk30 += pActor->unk40;
    func_80093354((VECTOR *)&pActor->unk28);
    pActor->unk2C = func_80093A5C(pActor->unk28, pActor->unk30) - 0x4000;
    pGroup->unk08 = pActor->unk28 >> 12;
    pGroup->unk0C = pActor->unk2C >> 12;
    pGroup->unk10 = pActor->unk30 >> 12;
    pScratch->vx = pActor->unk28 >> 12;
    pScratch->vy = pActor->unk2C >> 12;
    pScratch->vz = pActor->unk30 >> 12;
    func_80089160(0x19, pScratch, 0);
    return 1;
}

int func_8007D774(int index) {
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    WorldMapActor *pActor;

    pGroup = &D_8009C620->unk000[16];
    pRot = &pGroup->unk18;
    pMatrix = &pGroup->unk20;
    pGroup->unk00 = 1;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    pActor = &D_8009BE24[index];
    pActor->unk28 = 0xD00000;
    pActor->unk30 = 0x400000;
    pActor->unk2C = 0xFFD80000;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk40 = -0x6000;
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007D7FC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007D918);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007DCE0);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007DE14);
/* 82.21%: only difference is that the original keeps an 8-byte stack frame
   (addiu sp,sp,-8 mid-body) and stores unk50 before unk58; everything else,
   including the two loads off D_8009A65C and the reloads, is identical.
int func_8007DE14(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];

    pActor->unk54 = D_8009A65C[D_8009D3D4].unk0;
    pActor->unk50 = 0;
    pActor->unk58 = D_8009A65C[D_8009D3D4].unk4;
    pActor->unk20 = *(u16 *)pActor->unk54;
    pActor->unk22 = ((u16 *)pActor->unk58)[pActor->unk50];
    pActor->unk50++;
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007DE98);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E450);
/* 24.00%
int func_8007E450(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    int nScale = 0x1000;
    u32 unkAc;
    u32 unkB0;
    u32 unkB4;
    int nOne;

    D_8009D144 = 0;
    pActor->unk7C = nScale;
    unkAc = D_8009C5AC;
    unkB0 = D_8009C5B0;
    unkB4 = D_8009C5B4;
    D_8009BE0C = 0x78;
    nOne = 1;
    D_8009BE28 = unkAc;
    D_8009D55C = unkAc;
    D_8009BE2C = unkB0;
    D_8009D560 = unkB0;
    D_8009BE30 = unkB4;
    D_8009D564 = unkB4;
    pActor->unk04 = nOne;
    pActor->unk58 = 0x40;
    pActor->unk20 = 0;
    pActor->unk50 = 0;
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E4E4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007EBBC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007ECA4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007EE34);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007F8AC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007F968);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FC8C);
/* 88.66%: the two func_8007A06C calls and the store order match; the original
   materialises 0x1498000, 0x4AF2000 and the 3 into v1/a0/v0 before the first
   store, so those three come from locals, not literals.
int func_8007FC8C(int index) {
    WorldMapRoot *pRoot = D_8009C620;
    WorldMapActor *pActor = &D_8009BE24[index];

    func_8007A06C(&pRoot->unk000[9], pRoot->unk000[9].unk48, *(u16 *)((u8 *)pRoot->unk000[9].unk40 + 4));
    func_8007A06C(&pRoot->unk000[10], pRoot->unk000[10].unk48, *(u16 *)((u8 *)pRoot->unk000[10].unk40 + 4));
    pActor->unk28 = 0x1498000;
    pActor->unk2C = 0xFFF80000;
    pActor->unk54 = (void *)-0x800;
    pActor->unk20 = 0;
    pActor->unk30 = 0x4AF2000;
    pActor->unk50 = 0;
    pActor->unk58 = (void *)0x80;
    return 3;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FD30);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FF70);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080218);

int func_8008032C(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];

    pActor->unk50 = 0;
    pActor->unk20 = D_8009A698;
    pActor->unk22 = D_8009A6AC[pActor->unk50];
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080370);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080578);
/* 34.97%: same body as func_8007E450 minus the last three stores. The
   original emits everything in strict source order (sll, lw D_8009BE24,
   li 0x1000, sw D_8009D144, addu, sw 0x7C, then the three D_8009C5xx loads,
   li 0x78, sw D_8009BE0C, li 1, the six paired stores); gcc 2.7.2 reorders
   the loads here, so the shape is not reproducible from this source order.
int func_80080578(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    int nScale = 0x1000;
    u32 unkAc;
    u32 unkB0;
    u32 unkB4;
    int nOne;

    D_8009D144 = 0;
    pActor->unk7C = nScale;
    unkAc = D_8009C5AC;
    unkB0 = D_8009C5B0;
    unkB4 = D_8009C5B4;
    D_8009BE0C = 0x78;
    nOne = 1;
    D_8009BE28 = unkAc;
    D_8009D55C = unkAc;
    D_8009BE2C = unkB0;
    D_8009D560 = unkB0;
    D_8009BE30 = unkB4;
    D_8009D564 = unkB4;
    pActor->unk04 = nOne;
    pActor->unk20 = 0;
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_80080600);
