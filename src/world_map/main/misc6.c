#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"
#include "psyq/memory.h"

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ s16 unk04;
    /* 0x06 */ u16 unk06;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ s16 unk20;
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
extern s32 D_8009D3D4;
extern u32 D_8009D55C;
extern u32 D_8009D560;
extern u32 D_8009D564;

extern void func_8007A06C(void *, void *, int);
extern void WorldMapSetSpriteSetParent(int, int);
extern void func_80089160(int, void *, int);
extern void func_80093354(VECTOR *);
extern int func_80093A5C(int, int);
extern void func_8003A89C(void *, int, int);
extern void func_80039FF8(void);
extern void func_8003852C(void *);
extern void WorldMapFreeSpriteSets(void);
extern void func_80086124(void);
extern void func_80086568(void);
extern void func_800866C8(void);
extern void func_80074F04(void);
extern void func_800750DC(void);
extern void func_80088FF4(void);
extern void func_80089128(void);
extern void func_80097D64(void);
extern void func_800976A0(void);
extern u8 *D_8006259C;
extern u8 *D_8009BC38;
extern u8 *D_8009BCB0;
extern u8 *D_8009BC3C;
extern u8 *D_8009BCB4;
extern u8 *D_8009C180;
extern u16 D_8009BD3A;
extern u16 D_8006F94E;
extern u16 D_8006F950;
extern u16 D_8006F954;
extern u32 D_8009BBC4;
extern u16 D_8009A5CC[];
extern void func_80072BB0(void);
extern void func_800894C8(s32);
extern void func_80072DB4(int, int, int, int);
extern int ArchiveDataSync(void);
extern void func_80076954(void);
extern void func_8009766C(void);
extern MATRIX D_8009A180;
extern MATRIX D_8009BE4C;
extern s32 D_8009CCA4;
extern s32 D_8009D3CC;
extern s32 D_8009D804;
extern void func_80086700(void);
extern void (*D_8009CD40)(void);
extern void func_80098044(void);
extern int ArchiveCdDataSync(int);
extern void func_8001B66C(void);
extern s16 D_8009A5B4[];
extern s16 D_8009A5B6[];
extern s16 D_8009A5B8[];
extern s16 D_8009A674[];
extern s16 D_8009A678[];
extern s32 D_8009D7F0;
extern void func_800809EC(u8 *, s32, s32, s32, s32);
extern void func_8007EBBC(WorldMapGroup *, void *, int, int);
extern void func_80084580(void);
extern void func_8008440C(void);
extern void func_800979C8(void);
extern void func_80072090(void);
extern void func_800736DC(void);
extern void func_800863E0(void);
extern void func_80074E58(void);
extern void func_80075030(void);
extern void func_800739B8(void);
extern void func_80088F64(void);
extern void *g_GameCurLoadedWDS;
extern void *SoundLoadWdsFile(void *, void *);
extern void *D_8009C88C;
extern void func_80097BC0(s32 *);
extern void func_800967E4(void);
extern int Vsync(int);
extern int func_80096668(void);
extern u16 g_SoundControlFlags;
extern void SoundAddSedsEntry(void *);
extern int ArchiveDecodeAlignedSize(u32);
extern s32 D_8009D3D0;
extern u8 D_80062648[];
extern void *D_8009C884;
extern void *func_80039850(void *);
extern s32 *D_80062528;
extern void func_80039A80(s32 *, int, int);
extern void func_80097718(void *, void *);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void func_80075228(void);
extern void func_800923A8(void);
extern void func_800925A0(void);
extern int WorldMapSetActorScriptTables(int);
extern void func_8007DE98(int);
extern int func_8007E450(int);
extern void func_8007E4E4(int);
extern int func_8007ECA4(int);
extern void func_8007EE34(int);
extern void func_8007F8AC(int);
extern void func_8007F968(int);
extern void func_8007FC8C(int);
extern int func_8007FD30(int);
extern void func_80078948(void);
extern void func_80078950(void);

int WorldMapInitGroup12Actor(int index) {
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    WorldMapActor *pActor;

    WorldMapSetSpriteSetParent(0xC, 0xF);
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

int WorldMapUpdateGroup12Actor(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    WorldMapRoot *pRoot = D_8009C620;

    if (pActor->unk04 == 1) {
        SVECTOR *pScratch = (SVECTOR *)((u8 *)PSX_SCRATCH + 0xA0);

        pActor->unk04 = 0;
        pRoot->unk000[12].unk00 = 1;
        pRoot->unk000[15].unk00 = 1;
        func_800894C8(0x18);
        pScratch->vx = pActor->unk28 >> 12;
        pScratch->vy = pActor->unk2C >> 12;
        pScratch->vz = pActor->unk30 >> 12;
        func_80089160(0x1D, pScratch, 0);
        return 3;
    }
    {
    SVECTOR *pScratch = (SVECTOR *)((u8 *)PSX_SCRATCH + 0xA0);

    pActor->unk30 += pActor->unk40;
    func_80093354((VECTOR *)&pActor->unk28);
    pActor->unk2C = func_80093A5C(pActor->unk28, pActor->unk30) - 0x4000;
    pRoot->unk000[12].unk08 = pActor->unk28 >> 12;
    pRoot->unk000[12].unk0C = pActor->unk2C >> 12;
    pRoot->unk000[12].unk10 = pActor->unk30 >> 12;
    pScratch->vx = pActor->unk28 >> 12;
    pScratch->vy = pActor->unk2C >> 12;
    pScratch->vz = pActor->unk30 >> 12;
    func_80089160(0x18, pScratch, 0);
    }
    return 1;
}

int WorldMapInitGroup13Actor(int index) {
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;
    WorldMapActor *pActor;

    WorldMapSetSpriteSetParent(0xD, 0xE);
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

int WorldMapUpdateGroup13Actor(int index) {
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

int WorldMapInitGroup16Actor(int index) {
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

int WorldMapUpdateGroup16Actor(int index) {
    WorldMapGroup *pGroup = &D_8009C620->unk000[16];
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 nPos;

    switch (pActor->unk04) {
    case 1:
        // volatile: keeps the D_8009BE28 load below these two stores, as retail has it
        *(volatile s16 *)&pActor->unk04 = 0;
        *(volatile s16 *)&pGroup->unk00 = 0;
        pActor->unk28 = D_8009BE28;
        pActor->unk30 = D_8009BE30 + 0x400000;
        break;
    case 2:
        pActor->unk04 = 0;
        pActor->unk20 = 2;
        break;
    }
    pActor->unk30 += pActor->unk40;
    func_80093354((VECTOR *)&pActor->unk28);
    pGroup->unk08 = pActor->unk28 >> 12;
    pGroup->unk0C = pActor->unk2C >> 12;
    pGroup->unk10 = pActor->unk30 >> 12;
    if (pActor->unk20 == 2) {
        nPos = pActor->unk28;
        D_8009D55C = nPos;
        nPos = pActor->unk2C;
        D_8009D560 = nPos;
        nPos = pActor->unk30;
        D_8009D564 = nPos;
    }
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007D918);
/* 93.88%: byte-identical except the &D_8009C5AC group. Retail keeps the address
   in $s0 (lui/addiu before the first store, sw v0,0(s0), then move a0,s0 at the
   func_80097BC0 call); gcc folds every use to %hi/%lo. Tried: a pPos local used
   for the store and the call, pPos[0], and plain D_8009C5AC plus &D_8009C5AC
   (all three compile identically). Also note the ArchiveDataSync loop is
   while (ArchiveDataSync() >= 3), which is what gives the slti/beqz shape.
void func_8007D918(void) {
    RECT rect;
    s32 nIndex;

    func_80072BB0();
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    func_80076954();
    func_8009766C();
    D_8009BE4C = D_8009A180;
    D_8009CCA4 = 2;
    D_8009D3CC = 0x10;
    D_8009D804 = 0;
    D_8009D144 = 0;
    D_8009CD40 = func_80086700;
    func_80098044();
    ArchiveCdDataSync(0);
    func_8001B66C();
    nIndex = D_8009D3D4;
    D_8009C5AC = D_8009A5B4[nIndex * 4] << 12;
    D_8009C5B0 = D_8009A5B6[nIndex * 4] << 12;
    D_8009C5B4 = D_8009A5B8[nIndex * 4] << 12;
    func_80084580();
    func_8008440C();
    func_800979C8();
    func_80072090();
    func_800736DC();
    func_800863E0();
    func_80074E58();
    func_80075030();
    func_800739B8();
    func_80088F64();
    ArchiveCdDataSync(0);
    g_GameCurLoadedWDS = SoundLoadWdsFile(D_8009C88C, 0);
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(&D_8009C5AC);
    do {
        func_800967E4();
        Vsync(0);
    } while (func_80096668() > 0);
    while (g_SoundControlFlags & 0x10) {
    }
    HeapFree(D_8009C88C);
    SoundAddSedsEntry(D_8006259C);
    memcpy(D_80062648, D_8009C884, ArchiveDecodeAlignedSize(D_8009D3D0));
    D_80062528 = func_80039850(D_80062648);
    func_80039A80(D_80062528, 0x7F, 0);
    func_80097718(func_800923A8, func_800925A0);
    func_80097718(WorldMapSetActorScriptTables, func_8007DE98);
    func_80097718(func_8007E450, func_8007E4E4);
    func_80097718(func_8007ECA4, func_8007EE34);
    func_80097718(func_8007F8AC, func_8007F968);
    func_80097718(func_8007F8AC, func_8007F968);
    func_80097718(func_8007F8AC, func_8007F968);
    func_80097718(func_8007F8AC, func_8007F968);
    func_80097718(func_8007F8AC, func_8007F968);
    func_80097718(func_8007FC8C, func_8007FD30);
    func_80097718(func_80078948, func_80078950);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    func_80075228();
}
*/void WorldMapExitToMap1A1(void) {
    s32 nIndex;
    u16 unkWord;
    u16 unkValue;

    func_8003A89C(D_80062528, 0, 0xF0);
    func_80039FF8();
    func_8003852C(D_8006259C);
    HeapFree(D_8006259C);
    WorldMapFreeSpriteSets();
    func_80086568();
    func_800866C8();
    func_80074F04();
    func_800750DC();
    func_80088FF4();
    func_80089128();
    func_80097D64();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    func_800976A0();
    nIndex = D_8009D3D4;
    D_8006F94E = 0x1A1;
    unkValue = D_8009A5CC[nIndex];
    D_8009BBC4 = 1;
    D_8006F950 = D_8009BD3A;
    D_8006F954 = unkValue;
}

int WorldMapSetActorScriptTables(int index) {
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
/* 78.97%: instruction multiset identical (400 bytes both), only the tail block
   after RotMatrixYXZ differs in evaluation order and register choice. Retail
   evaluates the first 109*unk38 multiply after the D_8009A678 lh, keeps the
   index in $v1 and the return constant ($v0=1) in the middle of the block;
   gcc hoists the multiply and puts the return constant first. Tried: pActor
   assigned before/after the WorldMapSetSpriteSetParent calls (after is right - it is what
   puts the index in $s2 and moves addu into the first func_8007EBBC delay
   slot), nIndex declared first vs last.
int func_8007ECA4(int index) {
    int nIndex;
    WorldMapActor *pActor;
    WorldMapRoot *pRoot;
    WorldMapGroup *pGroup;
    SVECTOR *pRot;
    MATRIX *pMatrix;

    WorldMapSetSpriteSetParent(1, 2);
    WorldMapSetSpriteSetParent(1, 3);
    pRoot = D_8009C620;
    pActor = &D_8009BE24[index];
    func_8007EBBC(&pRoot->unk000[1], pRoot->unk000[1].unk48, *(u16 *)((u8 *)pRoot->unk000[1].unk40 + 4), 3);
    func_8007EBBC(&pRoot->unk000[2], pRoot->unk000[2].unk48, *(u16 *)((u8 *)pRoot->unk000[2].unk40 + 4), 3);
    func_8007EBBC(&pRoot->unk000[3], pRoot->unk000[3].unk48, *(u16 *)((u8 *)pRoot->unk000[3].unk40 + 4), 1);
    pGroup = &D_8009C620->unk000[1];
    pRot = &pGroup->unk18;
    pMatrix = &pGroup->unk20;
    pGroup->unk00 = 0;
    pRot->vx = pRot->vy = pRot->vz = 0;
    RotMatrixYXZ(pRot, pMatrix);
    nIndex = D_8009D3D4;
    pActor->unk38 = -0x85A;
    pActor->unk20 = 0;
    pActor->unk3C = 0;
    pActor->unk40 = 0xDA6;
    pActor->unk50 = D_8009A674[nIndex * 4] << 12;
    pActor->unk54 = (void *)(D_8009A678[nIndex * 4] << 12);
    pActor->unk28 = D_8009C5AC - pActor->unk38 * 13952;
    pActor->unk2C = D_8009C5B0;
    pActor->unk30 = D_8009C5B4 - pActor->unk40 * 13952;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007EE34);
// 100.00% in objdiff under -G0 (99.4 under the unit preset) but the C is 45 bytes
// the wrong length at link time, so -G0 is NOT the answer for this overlay (its
// globals are absolute symbols from undefined_syms_auto, and -G0 changes that
// addressing). Do not re-try -G0 here.

/*
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007F8AC);
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
/* 90.00%: every store, call and relocation is right and only 4 bytes short of
   retail's 0x240; the deltas are the two group pointers (retail keeps
   &D_8009C620->unk000[9] in $s2 - computed in the bne delay slot - and
   unk000[10] in $s3 - computed in the beqz delay slot - and stores through
   $s1 displacements), plus gcc hoisting the unk50/unk54 loads of the scratchpad
   block and picking different registers than retail's all-in-$v0. Note unk54
   and unk58 are void* in the struct: comparing them needs the (s32) casts below
   or gcc drops the clamp (sltu) and the whole unk58 branch.
int func_8007FD30(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    WorldMapRoot *pRoot = D_8009C620;
    WorldMapGroup *pGroup9 = &pRoot->unk000[9];
    WorldMapGroup *pGroup10 = &pRoot->unk000[10];
    s32 nX;
    s32 nY;
    s32 nZ;

    if (pActor->unk04 == 1) {
        pActor->unk04 = 0;
    }
    nX = pActor->unk28 >> 12;
    pGroup10->unk08 = nX;
    pGroup9->unk08 = nX;
    nY = pActor->unk2C >> 12;
    pGroup10->unk0C = nY;
    pGroup9->unk0C = nY;
    nZ = pActor->unk30 >> 12;
    pGroup10->unk10 = nZ;
    pGroup9->unk10 = nZ;
    pGroup10->unk20 = D_8009A180;
    pGroup9->unk20 = pGroup10->unk20;
    nX = pActor->unk50;
    *(s32 *)0x1F800008 = nX;
    *(s32 *)0x1F800000 = nX;
    *(s32 *)0x1F800014 = 0x1000;
    *(s32 *)0x1F800004 = 0x1000;
    nY = (s32)pActor->unk54;
    *(s32 *)0x1F800018 = nY;
    *(s32 *)0x1F800010 = nY;
    ScaleMatrix(&pGroup9->unk20, (VECTOR *)0x1F800000);
    ScaleMatrix(&pGroup10->unk20, (VECTOR *)0x1F800010);
    nX = pActor->unk50 + 0x180;
    pActor->unk50 = nX;
    if (nX > 0x7FFF) {
        pActor->unk50 = 0x7FFF;
    }
    nY = (s32)pActor->unk54 + 0x180;
    pActor->unk54 = (void *)nY;
    if (nY > 0x7FFF) {
        pActor->unk54 = (void *)0x7FFF;
    }
    func_800809EC(*(u8 **)((u8 *)pGroup9 + D_8009D7F0 * 4 + 0x48), *(u16 *)((u8 *)pGroup9->unk40 + 4),
                  pActor->unk58, pActor->unk58, pActor->unk58);
    func_800809EC(*(u8 **)((u8 *)pGroup10 + D_8009D7F0 * 4 + 0x48), *(u16 *)((u8 *)pGroup10->unk40 + 4),
                  pActor->unk58, pActor->unk58, pActor->unk58);
    nZ = (s32)pActor->unk58 - 3;
    pActor->unk58 = (void *)nZ;
    if (nZ < 0) {
        pActor->unk58 = NULL;
        return 3;
    }
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc6", func_8007FF70);
void WorldMapExitToMap84(void) {
    u16 unkValue;

    func_80039FF8();
    func_8003852C(D_8006259C);
    HeapFree(D_8006259C);
    WorldMapFreeSpriteSets();
    func_80086124();
    func_80086568();
    func_800866C8();
    func_80074F04();
    func_800750DC();
    func_80088FF4();
    func_80089128();
    func_80097D64();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    func_800976A0();
    unkValue = D_8009BD3A;
    D_8006F94E = 0x84;
    D_8006F954 = 2;
    D_8009BBC4 = 1;
    D_8006F950 = unkValue;
}

int WorldMapInitActorScript(int index) {
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
