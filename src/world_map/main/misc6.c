#include "common.h"
#include "psyq/libgpu.h"
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
extern u16 D_8009A68C[];
extern WorldMapPath D_8009A65C[];
extern u8 D_8009A660[];
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

int func_8007DE14(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    RECT rect;

    pActor->unk54 = D_8009A65C[D_8009D3D4].unk0;
    /* the original object relocates the second word of the pair against its own
       symbol D_8009A660 (= D_8009A65C + 4) */
    pActor->unk58 = *(u16 **)&D_8009A660[D_8009D3D4 * sizeof(WorldMapPath)];
    pActor->unk50 = 0;
    pActor->unk20 = *(u16 *)pActor->unk54;
    pActor->unk22 = ((u16 *)pActor->unk58)[pActor->unk50];
    pActor->unk50++;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007DE98);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E450);
/* 41.32% (24.00% without the volatile casts). Same body as func_80080578 plus
   three stores. Retail is in strict source order; gcc 2.7.2's post-reload
   scheduler hoists every load (and `li` of a store constant) to the top of the
   block. Tried: locals for the three copies (24.00%), volatile-qualified loads
   + store (41.32%, the loads then land at the very top), -O1
   (75.00% on func_80080578), -O3 (27.21%), gcc 2.6.0 (27.03%), -mcpu=r8000
   (32.57%). PROVEN round 12: the unit IS scheduled, so no preset can fix this
   shape. -fno-schedule-insns2 keeps this at 49.68% (80080578) but drops every
   matched function in the file (D414 75.94, D600 75.94, D690 66.42, D774
   63.91, DE14 74.97, 8032C 76.47, vs 100.00 stock); -fno-schedule-insns2
   -fno-schedule-insns gives 80.59% on 80080578 with every register correct and
   only the addu two slots early (41.61-68.76 on the matched ones). Volatile on
   all eleven touched globals reproduces the exact master store order but moves
   the three sources to $5/$6/$7 (23.21%).
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
*/INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007E4E4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007EBBC);
/* 99.83%: byte-identical except that gcc picks the loop pointer base as
   pPrim + 7 where the original has pPrim + 6 (every displacement off it is
   then 0 vs 1, -3 vs -2, ...). Tried: setlen/setcode split, explicit field
   stores, u8 pointer with p[..] offsets, &pPoly[i] indexing (that one drops
   the bias to 0, 83.84%), pPoly++ in the body, code |= 2 instead of
   setSemiTrans - the bias never lands on 6.
void func_8007EBBC(WorldMapGroup *pGroup, u8 *pPrim, int count, int abr) {
    POLY_FT4 *pPoly;
    int i;

    for (i = 0, pPoly = (POLY_FT4 *)pPrim; i < count; i++, pPoly++) {
        setPolyFT4(pPoly);
        pPoly->tpage = GetTPage(0, abr, 0x300, 0x100);
        pPoly->clut = GetClut(0, 0x1FF);
        setRGB0(pPoly, 0x3C, 0x3C, 0xC0);
        setSemiTrans(pPoly, 1);
    }
    memcpy((u8 *)pGroup->unk4C, pGroup->unk48, count * sizeof(POLY_FT4));
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007ECA4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007EE34);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007F8AC);
/* 94.77%: instruction order matches except that gcc pins `li v0,1` (the
   return value) immediately after the jal, which then forces v1/a0 for the
   -0x85A / 0xDA6 / 0x3C constants and the D_8009A68C halfword; retail emits
   it between the 0x22 store and the 0x5C store. Writing the 0x22 store after
   the 0x5C store (the other order the allocator likes) moves li v0,1 to the
   right place (91.49%) but then the 0x22 store is hoisted above the 0x20 and
   0x3C stores. The store order below is already the master's (0x28, 0x2C,
   0x54, 0x20, 0x30, 0x50, 0x58); the deviation is only that gcc pulls the
   0x58 store up to follow 0x54 and materialises li v0,3 last. Group base is unk000[nType + 4], i.e. D_8009C620 + 84*nType +
   0x150, and pActor->unk5C[0] = D_8009A68C[nType].
int func_8007F8AC(int index) {
    int nType = index - 4;
    WorldMapActor *pActor = &D_8009BE24[index];
    WorldMapGroup *pGroup = &D_8009C620->unk000[nType + 4];
    u16 unkValue;
    int nAbr;

    if (nType == 4) {
        nAbr = 1;
    } else {
        nAbr = 3;
    }
    func_8007EBBC(pGroup, pGroup->unk48, *(u16 *)((u8 *)pGroup->unk40 + 4), nAbr);
    pActor->unk38 = -0x85A;
    pActor->unk40 = 0xDA6;
    pActor->unk20 = 0;
    pActor->unk3C = 0;
    unkValue = D_8009A68C[nType];
    pActor->unk22 = 0x3C;
    pActor->unk5C[0] = unkValue;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007F968);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FC8C);
/* 88.66%: instruction multiset identical. Retail materialises all the
   immediates first (lui/ori v1 = 0x1498000, lui/ori a0 = 0x4AF2000,
   li v0 = 3 for the return) and only then starts storing, and it defers
   the unk58 = 0x80 store to the very end; gcc reorders my stores the
   other way (0x58 right after 0x54). Tried literals and int locals for
   the two big constants - gcc folds the locals straight back into the
   stores. Group bases are unk000[9] (s1 + 0x2F4) and unk000[10]
   (s1 + 0x348).
int func_8007FC8C(int index) {
    WorldMapRoot *pRoot = D_8009C620;
    WorldMapActor *pActor = &D_8009BE24[index];
    int nX;
    int nZ;

    func_8007A06C(&pRoot->unk000[9], pRoot->unk000[9].unk48, *(u16 *)((u8 *)pRoot->unk000[9].unk40 + 4));
    func_8007A06C(&pRoot->unk000[10], pRoot->unk000[10].unk48, *(u16 *)((u8 *)pRoot->unk000[10].unk40 + 4));
    nX = 0x1498000;
    nZ = 0x4AF2000;
    pActor->unk28 = nX;
    pActor->unk2C = 0xFFF80000;
    pActor->unk54 = (void *)-0x800;
    pActor->unk20 = 0;
    pActor->unk30 = nZ;
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
/* 34.97%: same body as func_8007E450 minus the last three stores (see its
   note for the proof that the scheduler cannot be turned off for this unit).
   The
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
