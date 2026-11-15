#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/memory.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} WorldMapPos4; // Size: 0x10

typedef struct {
    /* 0x00 */ u8 pad0[0x4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 pad6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u16 unk22;
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
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ WorldMapPos4 pos;
    /* 0x18 */ u8 unk18[0x8];
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ u32 *unk40;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ u8 *unk48;
    /* 0x4C */ u32 unk4C;
    /* 0x50 */ u32 unk50;
} WorldMapSpriteEntry; // Size: 0x54

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

// The world map frame render context D_8009BE3C points at: partial view of the
// two ordering table heads it fills each frame.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long *pOt[2];
} WorldMapRenderContext; // partial view

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
extern void func_80089160(s32, void *, s32);
extern void WorldMapSetupPrimTemplates(s32, u8 *, s32);
extern u8 *D_8006259C;
extern void WorldMapWrapPosition(void *);
extern s32 func_80093A5C(s32, s32);
extern void SoundPlaySoundEffectOnFreePair(s32);

extern void HeapFree(void *pMem);
extern void SoundStopAllElements(void);
extern void SoundRemoveSedsEntry(void *p);
extern void WorldMapFreeSpriteSets(void);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeGridImages(void);
extern void WorldMapFreeSlotPool(void);
extern s32 D_8009BC38;
extern s32 D_8009BCB0;
extern s32 D_8009BC3C;
extern s32 D_8009BCB4;
extern s32 D_8009C180;
extern s16 D_8006F94E;
extern s16 D_8006F950;
extern s16 D_8006F954;
extern s32 D_8009BBC4;
extern s32 D_8009BD40;
extern s32 D_8009BBB4;
extern s16 D_8009D558;
extern s32 D_8009BE28;
extern WorldMapRenderContext *D_8009BE3C;
extern s32 D_8009C5BC;
extern WorldMapPos4 D_8009C5AC;
extern void WorldMapBuildRotationMatrix(s32 *p);
extern void func_80097244(s32 *p);
extern void func_80089748(void);
extern void func_80089C78(void);
extern void func_800848F4(void);
extern void func_800980D4(s32 *p);
extern void func_800981C8(s32 *p);
extern void WorldMapWaitForLoadSlotFree(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32 *p);
extern void func_8009932C(u_long *pOt0, u_long *pOt1, s32 *pState);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void WorldMapDeactivateSpriteGroup(s32 arg0);
extern void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1);
extern MATRIX D_8009A180;
extern MATRIX D_8009BE4C;
extern s32 D_8009CCA4;
extern s32 D_8009D3CC;
extern s32 D_8009D804;
extern s32 D_8009CD40;
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(s32, s32, s32, s32);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void func_80098044(void);
extern void ArchiveCdDataSync(s32);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadTextureClut(void);
extern void func_800979C8(void);
extern void func_800736DC(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void SoundAddSedsEntry(u8 *);
extern void ArchiveSetIndex(s32, s32);
extern void func_80097BC0(WorldMapPos4 *);
extern s32 WorldMapGetQueuedLoadCount(void);
extern void WorldMapUpdateLoadQueue(void);
extern s32 Vsync(s32);
extern void WorldMapAddSlot();
extern s32 WorldMapStartActorScript(s32);
extern void WorldMapUpdateActorState(void);
extern void func_8007ADD4(void);
extern void WorldMapUpdateGroup4And5Scale(void);
extern void WorldMapUpdateGroup6And7Scale(void);
extern void func_8007BBEC(void);
extern s32 WorldMapInitActorScrollScale(s32);
extern s32 WorldMapSetupGroup4And5ScaledPrims(s32);
extern s32 WorldMapSetupGroup6And7ScaledPrims(s32);
extern s32 WorldMapInitReturnThree(s32);
extern s32 WorldMapInitGroup0Actor(s32);
extern s32 WorldMapActivateSpriteGroup9Timed(s32);
extern s32 WorldMapInitActorSlotTimer40(s32 index);
extern void func_80078EA4(void);
extern void func_80079778(void);
extern void func_8007A1B4(void);
extern void func_8007A430(void);
extern s32 WorldMapInitActorAtScrollPositionWithSound(s32);
extern s32 WorldMapSetupGroup14And15Prims(s32);
extern s32 WorldMapSetActorTimer60(s32);
extern s32 WorldMapReturnThree(void);
extern s32 WorldMapUpdateObjectAPosition(s32);
extern s32 WorldMapInitActorRotationMatrix(s32);
extern s32 WorldMapUpdateActorTerrainHeight(s32);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void WorldMapInitRandomTimers(void);

void WorldMapExitToMap10E(void) {
    u16 v;

    SoundStopAllElements();
    SoundRemoveSedsEntry(D_8006259C);
    HeapFree(D_8006259C);
    WorldMapFreeSpriteSets();
    WorldMapFreeTileVertexBuffers();
    WorldMapFreeSemiTransPrimBuffers();
    WorldMapFreeTileAnimEntries();
    WorldMapFreeFrameAnimEntries();
    WorldMapFreeSpriteGroupTable();
    WorldMapFreePrimTemplateBuffers();
    WorldMapFreeGridImages();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    WorldMapFreeSlotPool();
    v = *(u16 *)&D_8009BD3A;
    D_8006F94E = 0x10E;
    D_8006F954 = 0;
    D_8009BBC4 = 1;
    D_8006F950 = v;
}
s32 WorldMapResetActorSlotWithSound(s32 index) {
    WorldMapActor *p;
    u8 *q;

    p = D_8009BE24;
    D_8009BE0C = 0x78;
    D_8009D3F0 = 0x200000;
    q = D_8006259C;
    p = p + index;
    p->unk20 = 0;
    p->unk58 = 0;
    // do/while block makes cc1 schedule these stores after the actor cluster
    do { p->unk54 = 0; p->unk50 = 0; D_8009BD3C = 0; D_8009BD3A = 0; } while (0);
    D_8009BD38 = 0;
    D_8009D144 = 1;
    SoundPlaySoundEffectOnFreePair((((u16 *)q)[0xA] << 16) | 0xA4);
    p->unk22 = 0x18;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80077E68);
s32 WorldMapInitActorAtScrollPosition(s32 index) {
    WorldMapActor *p;
    s32 *pSlot;

    WorldMapSetSpriteSetParent(0, 1);
    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
    WorldMapSetSpriteSetParent(0, 4);
    WorldMapSetSpriteSetParent(0, 5);
    WorldMapSetSpriteSetParent(0, 6);
    WorldMapSetSpriteSetParent(0, 7);
    WorldMapSetSpriteSetParent(0, 8);
    WorldMapSetSpriteSetParent(0, 9);
    WorldMapSetSpriteSetParent(0, 10);
    WorldMapSetSpriteSetParent(0, 11);
    WorldMapSetSpriteSetParent(0, 12);
    WorldMapSetSpriteSetParent(0, 13);
    p = D_8009BE24 + index;
    p->unk20 = 0;
    *(WorldMapPos4 *)&p->unk28 = D_8009C5AC;
    pSlot = (s32 *)p;
    pSlot[0x15] = 0x40;
    pSlot[0x16] = 0x200;
    pSlot[0x17] = 0x60;
    pSlot[0x18] = 0x300;
    pSlot[0x14] = 0;
    pSlot[0x19] = 0x50;
    *(WorldMapPos4 *)&D_8009BE28 = *(WorldMapPos4 *)&p->unk28;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_800783E8);

s32 WorldMapEnterNop(void) {
    return 1;
}

s32 WorldMapUpdateFrameWithParticles(void) {
    s32 *pState;

    if (D_8009D144 == 0) {
        WorldMapBuildRotationMatrix(&D_8009BD40);
    } else {
        func_80097244(&D_8009BD40);
    }
    func_80089748();
    func_80089C78();
    func_800848F4();
    func_800980D4(&D_8009BBB4);
    if (D_8009D558 != 0) {
        func_800981C8(&D_8009BE28);
        WorldMapWaitForLoadSlotFree();
        func_80098CC0();
    }
    pState = &D_8009BE28;
    func_800983A0(pState);
    func_8009932C(D_8009BE3C->pOt[0], D_8009BE3C->pOt[1], pState);
    D_8009C5BC = D_8009C5BC + 0x40;
    func_80073B04();
    func_800737EC();
    func_80086798();
    return 1;
}
void WorldMapInitializeModeWithTerrainActors(void) {
    RECT rect;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    D_8009BE4C = D_8009A180;
    D_8009CCA4 = 2;
    D_8009D3CC = 0x10;
    D_8009D804 = 0;
    D_8009D144 = 0;
    D_8009CD40 = (s32)WorldMapWrapTileVertexPositions;
    func_80098044();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    D_8009C5AC.x = 0x2000000;
    D_8009C5B0 = -0x300000;
    D_8009C5B4 = 0x2000000;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    func_800979C8();
    func_800736DC();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(D_8006259C);
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(&D_8009C5AC);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapInitActorSlotTimer40, func_80078EA4);
    WorldMapAddSlot(WorldMapInitActorAtScrollPositionWithSound, func_80079778);
    WorldMapAddSlot(WorldMapSetupGroup14And15Prims, func_8007A1B4);
    WorldMapAddSlot(WorldMapSetActorTimer60, func_8007A430);
    WorldMapAddSlot(WorldMapReturnThree, WorldMapUpdateObjectAPosition);
    WorldMapAddSlot(WorldMapInitActorRotationMatrix, WorldMapUpdateActorTerrainHeight);
    WorldMapAddSlot(WorldMapEnterNop, WorldMapUpdateFrameWithParticles);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    WorldMapInitRandomTimers();
}
void WorldMapExitToMap110(void) {
    u16 v;

    SoundStopAllElements();
    SoundRemoveSedsEntry(D_8006259C);
    HeapFree(D_8006259C);
    WorldMapFreeSpriteSets();
    WorldMapFreeTileVertexBuffers();
    WorldMapFreeSemiTransPrimBuffers();
    WorldMapFreeTileAnimEntries();
    WorldMapFreeFrameAnimEntries();
    WorldMapFreeSpriteGroupTable();
    WorldMapFreePrimTemplateBuffers();
    WorldMapFreeGridImages();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    WorldMapFreeSlotPool();
    v = *(u16 *)&D_8009BD3A;
    D_8006F94E = 0x110;
    D_8006F954 = 0;
    D_8009BBC4 = 1;
    D_8006F950 = v;
}

s32 WorldMapInitActorSlotTimer40(s32 index) {
    WorldMapActor *p;
    s32 scroll;
    s32 timer;

    p = D_8009BE24;
    D_8009BE0C = 0x78;
    D_8009D3F0 = 0x200000;
    D_8009D144 = 0;
    p = p + index;
    scroll = -0xC0;
    p->unk20 = 0x10;
    p->unk58 = 0;
    // the do/while blocks decide the store order and where the 0x40 lands
    do {
        p->unk54 = 0;
        p->unk50 = 0;
        D_8009BD38 = scroll;
        do { D_8009BD3A = 0x680; } while (0);
        timer = 0x40;
        D_8009BD3C = 0;
    } while (0);
    p->unk22 = timer;
    p->unk74 = 0;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80078EA4);
s32 WorldMapInitActorRotationMatrix(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;

    p = D_8009BE24 + index;
    s = D_8009C620;
    p->unk28 = 0x02000000;
    p->unk30 = 0x01500000;
    p->unk20 = 0;
    s->unk558.vx = 0;
    s->unk558.vy = 0x780;
    s->unk558.vz = 0;
    RotMatrixYXZ(&s->unk558, &s->unk560);
    return 1;
}
s32 WorldMapUpdateActorTerrainHeight(s32 index) {
    WorldMapActor *p;

    p = D_8009BE24 + index;
    if (p->unk4 != 0) {
        p->unk4 = 0;
        D_8009C620->unk540 = 1;
    }
    WorldMapWrapPosition(&p->unk28);
    p->unk2C = func_80093A5C(p->unk28, p->unk30) + 0x18000;
    D_8009C620->unk548 = p->unk28 >> 12;
    D_8009C620->unk54C = p->unk2C >> 12;
    D_8009C620->unk550 = p->unk30 >> 12;
    return 1;
}
s32 WorldMapInitActorAtScrollPositionWithSound(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    u16 *q;
    s32 *pSlot;

    WorldMapSetSpriteSetParent(0, 1);
    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
    WorldMapSetSpriteSetParent(0, 4);
    WorldMapSetSpriteSetParent(0, 5);
    WorldMapSetSpriteSetParent(0, 6);
    WorldMapSetSpriteSetParent(0, 7);
    WorldMapSetSpriteSetParent(0, 8);
    WorldMapSetSpriteSetParent(0, 9);
    WorldMapSetSpriteSetParent(0, 10);
    WorldMapSetSpriteSetParent(0, 11);
    WorldMapSetSpriteSetParent(0, 12);
    WorldMapSetSpriteSetParent(0, 13);
    p = D_8009BE24 + index;
    p->unk20 = 0;
    *(WorldMapPos4 *)&p->unk28 = D_8009C5AC;
    pSlot = (s32 *)p;
    pSlot[0x15] = 0x40;
    pSlot[0x16] = 0x200;
    pSlot[0x17] = 0x60;
    pSlot[0x18] = 0x300;
    pSlot[0x14] = 0;
    pSlot[0x19] = 0x50;
    *(WorldMapPos4 *)&D_8009BE28 = *(WorldMapPos4 *)&p->unk28;
    s = D_8009C620;
    q = D_8006259C;
    ((s16 *)s)[0xC] = -0x100;
    ((s16 *)s)[0xD] = 0;
    ((s16 *)s)[0xE] = -0x40;
    SoundPlaySoundEffectOnFreePair((q[0xA] << 16) | 0x36);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80079778);
void WorldMapSetupPrimTemplates(s32 arg0, u8 *pDst, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        pDst[3] = 9;
        pDst[7] = 0x2C;
        *(u16 *)(pDst + 0x16) = GetTPage(1, 3, 0x340, 0x100);
        *(u16 *)(pDst + 0xE) = GetClut(0x100, 0x1FF);
        pDst[4] = 0x80;
        pDst[5] = 0x80;
        pDst[7] |= 2;
        pDst[6] = 0x80;
        pDst += 0x28;
    }
    memcpy(*(u8 **)(arg0 + 0x4C), *(u8 **)(arg0 + 0x48), count * 0x28);
}

s32 WorldMapSetupGroup14And15Prims(s32 index) {
    WorldMapActor *p;

    p = D_8009BE24 + index;
    p->unk20 = 0;
    p->unk70 = 0;
    p->unk6C = 0;
    WorldMapSetupPrimTemplates((s32)D_8009C620 + 0x498, D_8009C620->unk4E0,
                  ((u16 *)D_8009C620->unk4D8)[2]);
    WorldMapSetupPrimTemplates((s32)D_8009C620 + 0x4EC, D_8009C620->unk534,
                  ((u16 *)D_8009C620->unk52C)[2]);
    return 3;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A1B4);
/* 99.90%: 151 instructions, identical stream; the only delta is the register
   of the first scale value (retail `lw v0,0x6C` + `sw v0` twice, ours `lw
   v1`). A single `scale` local reused for both scratchpad vectors gives this;
   two locals (declared at the top, or in a block at the second use) give two
   live value pseudos and gcc then reuses the dead actor pointer in $a2
   (97.16%), and spelling the pairs as repeated `p->unk6C`/`p->unk70` lets gcc
   CSE them and moves p to $a3 (96.17%). Same allocator class as misc5
   WorldMapUpdateGroup4And5Scale.
s32 func_8007A1B4(s32 index) {
    WorldMapSpriteEntry *pEntry;
    MATRIX *pMtxA;
    MATRIX *pMtxB;
    WorldMapActor *p;
    s32 result;
    s32 scale;
    s32 scaleY;

    pEntry = (WorldMapSpriteEntry *)D_8009C620;
    p = D_8009BE24 + index;
    result = 1;
    pEntry[14].pos = pEntry[0].pos;
    pEntry[15].pos = pEntry[14].pos;
    p->unk6C += 0xC0;
    if (p->unk6C >= 0x800) {
        p->unk70 = (p->unk70 + 0x100) & 0x7FFF;
    }
    if (p->unk70 >= 0x6000) {
        result = 3;
        p->unk4 = 0;
        p->unk70 = 0;
        p->unk6C = 0;
    }
    pMtxA = (MATRIX *)0x1F8000F0;
    *pMtxA = D_8009A180;
    pMtxB = (MATRIX *)0x1F800110;
    *pMtxB = *pMtxA;
    scale = p->unk6C;
    scaleY = p->unk70;
    *(u32 *)0x1F800008 = scale;
    *(u32 *)0x1F800000 = scale;
    *(u32 *)0x1F800014 = 0x1000;
    *(u32 *)0x1F800004 = 0x1000;
    *(u32 *)0x1F800018 = scaleY;
    *(u32 *)0x1F800010 = scaleY;
    ScaleMatrix(pMtxA, (VECTOR *)0x1F800000);
    ScaleMatrix(pMtxB, (VECTOR *)0x1F800010);
    pEntry[14].matrix = *pMtxA;
    pEntry[15].matrix = *pMtxB;
    return result;
}
*/

s32 WorldMapSetActorTimer60(s32 index) {
    WorldMapActor *p;

    p = D_8009BE24 + index;
    p->unk22 = 0x60;
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A430);
/* 77.29%: everything but the tail of the `unk22 > 0` branch matches. The
   original materialises the func_80089160 argument (0x1F8000A0) as lui a1 in
   the middle of the three scratch stores (lui a1,0x1F80; sh..; ori a1,a1,0xA0)
   and puts p->unk30 += 0x20000 in a1, so p is in a2. This shape (s16 *pDst
   for the stores and the call argument) gets p into a2 and 77.29%; the literal
   `*(s16 *)((u8 *)PSX_SCRATCH + 0xA0)` form gets the store order right but
   moves p to a3 (74.40%). gcc 2.7.2-cdk: 73.44%. Not a cdk/frame problem - the
   address constant and the 0x20000 swap registers between the two shapes.
s32 func_8007A430(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    WorldMapPos4 *pSrc;
    s16 *pDst;
    s32 result = 1;

    p = D_8009BE24 + index;
    if (p->unk4 == 1) {
        s = D_8009C620;
        p->unk4 = 0;
        p->unk28 = s->unk8 << 12;
        p->unk2C = s->unkC << 12;
        p->unk30 = s->unk10 << 12;
    }
    p->unk22 = p->unk22 - 1;
    if ((s16)p->unk22 > 0) {
        p->unk30 += 0x20000;
        pDst = (s16 *)((u8 *)PSX_SCRATCH + 0xA0);
        pDst[0] = p->unk28 >> 12;
        pDst[1] = p->unk2C >> 12;
        pDst[2] = p->unk30 >> 12;
        func_80089160(9, (s32)pDst, 0);
    } else {
        p->unk22 = 0x60;
        pSrc = &D_8009C5AC;
        *(WorldMapPos4 *)&p->unk28 = *pSrc;
        WorldMapDeactivateSpriteGroup(9);
        result = 3;
    }
    return result;
}
*/

s32 WorldMapReturnThree(void) {
    return 3;
}

s32 WorldMapUpdateObjectAPosition(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    SVECTOR *pScratch;

    p = D_8009BE24 + index;
    s = D_8009C620;
    p->unk4 = 0;
    pScratch = (SVECTOR *)((u8 *)PSX_SCRATCH + 0xA0);
    pScratch->vx = s->unk8;
    pScratch->vy = s->unkC;
    pScratch->vz = s->unk10;
    func_80089160(0xA, pScratch, 0);
    return 3;
}

/* 92.11%: a `s16 *dst = (s16 *)((u8 *)PSX_SCRATCH + 0xA0)` local (rather than
   three separate `*(s16 *)(PSX_SCRATCH + 0xA0/0xA2/0xA4)` casts, which scored
   67.74%) puts the lui/ori for $a1 where the original has them, and the stores
   reach the right addresses. Only difference left: gcc hoists the 2nd load
   before the 1st store (and puts the 1st value in $a0), so the three
   load/store pairs are not adjacent as in the original.
s32 WorldMapUpdateObjectAPosition(s32 index) {
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
void WorldMapInitializeModeWithScaledGroups(void) {
    RECT rect;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    D_8009BE4C = D_8009A180;
    D_8009CCA4 = 2;
    D_8009D3CC = 0x10;
    D_8009D804 = 0;
    D_8009D144 = 0;
    D_8009CD40 = (s32)WorldMapWrapTileVertexPositions;
    func_80098044();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    D_8009C5AC.x = 0x5BED000;
    D_8009C5B0 = -0xA0000;
    D_8009C5B4 = 0x62A8000;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    func_800979C8();
    func_800736DC();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(D_8006259C);
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(&D_8009C5AC);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot(WorldMapModeInitFadeRect, func_800925A0);
    WorldMapAddSlot(WorldMapStartActorScript, WorldMapUpdateActorState);
    WorldMapAddSlot(WorldMapInitActorScrollScale, func_8007ADD4);
    WorldMapAddSlot(WorldMapSetupGroup4And5ScaledPrims, WorldMapUpdateGroup4And5Scale);
    WorldMapAddSlot(WorldMapSetupGroup6And7ScaledPrims, WorldMapUpdateGroup6And7Scale);
    WorldMapAddSlot(WorldMapInitReturnThree, WorldMapActivateSpriteGroup9Timed);
    WorldMapAddSlot(WorldMapInitGroup0Actor, func_8007BBEC);
    WorldMapAddSlot(WorldMapEnterNop, WorldMapUpdateFrameWithParticles);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    WorldMapInitRandomTimers();
}
void WorldMapExitToMap11A(void) {
    u16 v;

    SoundStopAllElements();
    SoundRemoveSedsEntry(D_8006259C);
    HeapFree(D_8006259C);
    WorldMapFreeSpriteSets();
    WorldMapFreeTileVertexBuffers();
    WorldMapFreeSemiTransPrimBuffers();
    WorldMapFreeTileAnimEntries();
    WorldMapFreeFrameAnimEntries();
    WorldMapFreeSpriteGroupTable();
    WorldMapFreePrimTemplateBuffers();
    WorldMapFreeGridImages();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    WorldMapFreeSlotPool();
    v = *(u16 *)&D_8009BD3A;
    D_8006F94E = 0x11A;
    D_8006F954 = 0;
    D_8009BBC4 = 1;
    D_8006F950 = v;
}
s32 WorldMapStartActorScript(s32 index) {
    WorldMapActor *p;

    p = D_8009BE24 + index;
    p->unk50 = 0;
    p->unk20 = D_8009A450;
    p->unk22 = D_8009A46C[p->unk50];
    return 1;
}
