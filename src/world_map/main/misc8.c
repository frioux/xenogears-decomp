#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "psyq/memory.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u16 pad22;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ u16 pad26;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s16 unk48;
    /* 0x4A */ u16 pad4A;
    /* 0x4C */ void *unk4C;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ s32 unk68;
    /* 0x6C */ s32 unk6C;
    /* 0x70 */ s32 unk70;
    /* 0x74 */ s32 unk74;
    /* 0x78 */ u32 unk78;
    /* 0x7C */ s32 unk7C;
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 a;
} FadeParams; // Size: 0x4, alignment 1

// 12.4 fixed point world map position; a normal (x, y, z) shares this layout.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

// One triangle of a sprite set's mesh: three indices into WorldMapMesh::unk4.
typedef struct {
    /* 0x0 */ s16 vert[3];
    /* 0x6 */ u8 unk6[0x8];
} WorldMapTriangle; // Size: 0xE

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ SVECTOR *unk4;
    /* 0x8 */ WorldMapTriangle tris[1];
} WorldMapMesh; // Size: 0x16

typedef struct WorldMapEntry {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u16 pad2;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[0xC];
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ u8 *unk40;
    /* 0x44 */ WorldMapMesh *unk44;
    /* 0x48 */ u8 *unk48;
    /* 0x4C */ u8 *unk4C;
    /* 0x50 */ struct WorldMapEntry *unk50;
} WorldMapEntry; // Size: 0x54

extern WorldMapActor *D_8009BE24; // actors, 0x80 apart
extern WorldMapEntry *D_8009C620; // sprite sets, 0x54 apart
extern s16 D_8009D7E0; // D_8009C620 entry count
typedef struct {
    /* 0x00 */ VECTOR scale;
    /* 0x10 */ u8 unk10[0x90];
    /* 0xA0 */ SVECTOR rot;
    /* 0xA8 */ u8 unkA8[0x48];
    /* 0xF0 */ MATRIX mat;
} WorldMapModel; // Size: 0x110

typedef struct {
    /* 0x00 */ u8 unk0[0x20];
    /* 0x20 */ MATRIX mat;
} WorldMapModelOut; // Size: 0x40

extern u8 D_8009AC60[];
extern u8 D_8009C5AC[];

extern void ModelPartFreeLightData(void *pModelPart);
extern int Vsync(int mode);
extern void WorldMapFreeGridImages(void);
extern void WorldMapUpdateLoadQueue(void);
extern void func_80097BC0(void *p);
extern s32 WorldMapGetQueuedLoadCount(void);
extern s16 func_80084DB8(void *pArg, s16 arg1);
extern void WorldMapBuildSpritePrims(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 arg3);
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s32 D_8009BE28;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 D_8009D55C;
extern s32 D_8009D560;
extern s32 D_8009D564;
extern s16 D_8009BD38;
extern s16 D_8009BD3A;
extern s16 D_8009BD3C;
extern s32 D_8009D144;
extern s32 D_8009D3F0;
extern s32 D_8009BE0C;
extern void HeapFree(void *pMem);
extern void *HeapAlloc(u_int allocSize, u_int allocFlags);
extern void *D_8006259C;
extern s32 D_8009BC38;
extern s32 D_8009BCB0;
extern s32 D_8009BC3C;
extern s32 D_8009BCB4;
extern s32 D_8009C180;
extern s16 D_8006F94E;
extern s16 D_8006F950;
extern s16 D_8006F954;
extern s32 D_8009BBC4;
extern void SoundStopAllElements(void);
extern void SoundRemoveSedsEntry(void *p);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeSlotPool(void);
extern void WorldMapFreeSpriteSets(void);
extern void ModelLoadTextureBlock(void *pBlock);
extern void func_800931D8(u8 *pSrc, u8 *pDst, s32 count, u8 *pFactors);
extern void *D_8009BD20;
extern u16 D_8009BCE0[];
extern FadeParams D_800704DC;

extern MATRIX D_8009A180;
extern MATRIX D_8009BE4C;
extern s32 D_8009CCA4;
extern s32 D_8009D3CC;
extern s32 D_8009D804;
extern void (*D_8009CD40)(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(int, int, int, int);
extern int ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern void func_80098044(void);
extern int ArchiveCdDataSync(int);
extern void WorldMapLoadSedsBank(void);
extern void func_8008440C(void);
extern void func_80084580(void);
extern void func_800979C8(void);
extern void func_800736DC(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void SoundAddSedsEntry(void *);
extern int ArchiveSetIndex(int, int);
extern void WorldMapAddSlot(void *, void *);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void func_80075228(void);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern void WorldMapRunSlotScript(void);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern s32 WorldMapSetActorParams(s32);
extern s32 func_8008390C(s32);
extern s32 WorldMapInterpolatePlaneHeight(WorldMapPos *p0, WorldMapPos *p1, WorldMapPos *p2);
extern s32 func_80083A00(s32);
extern s32 WorldMapSetActorPosition(s32);
extern s32 func_80084068(s32);
extern u8 *D_8009BE3C;
extern MATRIX D_8009C808;
extern void WorldMapWrapDelta(void *pDelta);
extern void func_800223B0(void *p, s32 angle);
extern void SpriteDrawFrame(void *p, u32 *pTbl);
extern void GfxSetBaseMatrix(MATRIX *p);
extern void AnimScriptTick(void *p);

void WorldMapBuildActorModelMatrix(WorldMapActor *pActor, WorldMapModelOut *pOut, WorldMapModel *pModel) {
    if (pActor->unk20 == 0) {
        return;
    }
    if (pActor->unk20 != 1) {
        return;
    }
    {
        pModel->rot.vz = 0;
        pModel->rot.vx = 0;
        pModel->rot.vy = pActor->unk68;
        pModel->scale.vx = pActor->unk70 & 0x7FFF;
        pModel->scale.vy = 0x1000;
        pModel->scale.vz = rcos(pActor->unk6C) * 6;
        RotMatrix(&pModel->rot, &pModel->mat);
        ScaleMatrix(&pModel->mat, &pModel->scale);
        pOut->mat = pModel->mat;
        pActor->unk50 += pActor->unk5C;
        pActor->unk54 += pActor->unk60;
        if (pActor->unk50 >= 0x100) {
            pActor->unk50 = 0xFF;
            pActor->unk5C = -pActor->unk5C;
        } else if (pActor->unk50 < 0x40) {
            pActor->unk50 = 0x40;
            pActor->unk5C = -pActor->unk5C;
        }
        if (pActor->unk54 >= 0x100) {
            pActor->unk54 = 0xFF;
            pActor->unk60 = -pActor->unk60;
        } else if (pActor->unk54 < 0x80) {
            pActor->unk54 = 0x80;
            pActor->unk60 = -pActor->unk60;
        }
        pActor->unk6C = (pActor->unk6C + 8) & 0xFFF;
        pActor->unk70 += pActor->unk74;
    }
}
// Best: 98.71%. Instruction multiset and registers are identical bar one
// store: `sw v?,4(s2)` (scale.vy = 0x1000) sits in the load-delay slot of
// `lw 0x68` here, while retail puts it in the slot of `lw 0x70`. Source order
// (vy before vx) is what fixes the store order; swapping them breaks it.
// The guard is two tests in retail (beqz x==0 then bne x!=1), i.e. two
// early returns, not one `if (x == 1)`.
/*
void WorldMapBuildActorModelMatrix(WorldMapActor *pActor, WorldMapModelOut *pOut, WorldMapModel *pModel) {
    if (pActor->unk20 == 0) {
        return;
    }
    if (pActor->unk20 != 1) {
        return;
    }
    {
        pModel->rot.vz = 0;
        pModel->rot.vx = 0;
        pModel->rot.vy = pActor->unk68;
        pModel->scale.vx = pActor->unk70 & 0x7FFF;
        pModel->scale.vy = 0x1000;
        pModel->scale.vz = rcos(pActor->unk6C) * 6;
        RotMatrix(&pModel->rot, &pModel->mat);
        ScaleMatrix(&pModel->mat, &pModel->scale);
        pOut->mat = pModel->mat;
        pActor->unk50 += pActor->unk5C;
        pActor->unk54 += pActor->unk60;
        if (pActor->unk50 >= 0x100) {
            pActor->unk50 = 0xFF;
            pActor->unk5C = -pActor->unk5C;
        } else if (pActor->unk50 < 0x40) {
            pActor->unk50 = 0x40;
            pActor->unk5C = -pActor->unk5C;
        }
        if (pActor->unk54 >= 0x100) {
            pActor->unk54 = 0xFF;
            pActor->unk60 = -pActor->unk60;
        } else if (pActor->unk54 < 0x80) {
            pActor->unk54 = 0x80;
            pActor->unk60 = -pActor->unk60;
        }
        pActor->unk6C = (pActor->unk6C + 8) & 0xFFF;
        pActor->unk70 += pActor->unk74;
    }
}
*/

void WorldMapBuildSpritePrims(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 arg3) {
    s32 i;

    for (i = count - 1; i != -1; i--) {
        pDst[3] = 7;
        pDst[7] = 0x24;
        *(u16 *)(pDst + 0x16) = GetTPage(0, arg3, 0x2C0, 0x100);
        pDst[4] = 0;
        pDst[5] = 0;
        pDst[6] = 0;
        pDst[7] |= 2;
        pDst += 0x20;
    }
    memcpy(pEntry->unk4C, pEntry->unk48, count << 5);
}

void WorldMapSetSpritePrimColor(u8 *pData, s32 count, u8 r, u8 g, s32 b) {
    s32 i;

    for (i = count - 1; i != -1; i--) {
        pData[4] = r;
        pData[5] = g;
        pData[6] = b;
        pData += 0x20;
    }
}

s32 WorldMapSetupSpriteSetPrims(s32 arg0) {
    WorldMapEntry *pEntry = &D_8009C620[arg0 + 78];

    WorldMapBuildSpritePrims(pEntry, pEntry->unk48, *(u16 *)(pEntry->unk40 + 4), 3);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083264);

s32 WorldMapReturnSuccess(void) {
    return 1;
}

s32 WorldMapWaitForQueueDrain(s32 arg0) {
    WorldMapActor *pActor = &D_8009BE24[arg0];

    if (pActor->unk4 == 1) {
        pActor->unk4 = 0;
        DrawSync(0);
        Vsync(0);
        WorldMapFreeGridImages();
        func_80097BC0(D_8009C5AC);
        do {
            WorldMapUpdateLoadQueue();
            Vsync(0);
        } while (WorldMapGetQueuedLoadCount() > 0);
    }
    return 1;
}
void WorldMapInitializeScene(void) {
    RECT rect;
    s32 *pVec;

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
    D_8009CD40 = WorldMapWrapTileVertexPositions;
    func_80098044();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    pVec = (s32 *)D_8009C5AC;
    *pVec = 0x1800000;
    D_8009C5B0 = 0xFFF00000;
    D_8009C5B4 = 0x1A00000;
    func_80084580();
    func_8008440C();
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
    func_80097BC0(pVec);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot((void *)WorldMapModeInitFadeRect, (void *)func_800925A0);
    WorldMapAddSlot((void *)WorldMapSetActorParams, (void *)WorldMapRunSlotScript);
    WorldMapAddSlot((void *)func_8008390C, (void *)func_80083A00);
    WorldMapAddSlot((void *)WorldMapSetActorPosition, (void *)func_80084068);
    WorldMapAddSlot((void *)WorldMapEnterNop, (void *)WorldMapUpdateFrameWithParticles);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    func_80075228();
}
s32 WorldMapShutdownScene(void) {
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
    D_8006F94E = 0x269;
    D_8006F954 = 4;
    D_8009BBC4 = 1;
    D_8006F950 = v;
    return 1;
}

s32 WorldMapSetActorParams(s32 arg0) {
    D_8009BE24[arg0].unk50 = (u32)D_8009AC60;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008390C);
// Best: 84.75% with the raw byte-pointer form below; the typed-struct form
// (pActor->unk7C = 0x1000; D_8009BD3A = 0x400; ... v = D_8009BD3A;) scores
// 30.52% because gcc reorders the whole straight-line block once it folds the
// store-to-load pair into the constant. Every instruction and relocation of
// the raw form matches except that folding: retail re-loads D_8009BD3A (lh)
// and D_8009BD3C (lh) after storing them, we get lui $v0,0x40 instead.
// Round-49 update: a volatile *store* (*(volatile s16 *)&D_8009BD3A = 0x400)
// does stop the fold and restores both lh's, but the extra live value shifts
// the allocator one register over (temps to a1, 0x960000 to a0; 244 vs 224
// bytes, 77%); the same happens spelling the angle globals as one array
// (extern s16 D_8009BD38[]) with the reads through D_8009BD38[1]/[2]: no fold,
// 79.41%, registers and store order off. My third shape - struct fields for the
// stores, plain externs for the reads - is 79.57% and also loses the allocator.
// -O1 scores 67.46. A volatile load forces the reload but emits
// lhu + sll 0x10 + sra 0x4 instead of lh + sll 0xc (72.84%); volatile on the
// stores alone changes nothing.
/*
void func_8008390C(s32 arg0) {
    WorldMapActor *pActor = &D_8009BE24[arg0];
    u8 *p = (u8 *)pActor;
    s32 v;

    *(s32 *)(p + 0x7C) = 0x1000;
    v = *(s32 *)D_8009C5AC;
    D_8009BE28 = v;
    D_8009D55C = v;
    pActor->unk28 = v;
    v = D_8009C5B0;
    D_8009BE2C = v;
    D_8009D560 = v;
    pActor->unk2C = v;
    v = D_8009C5B4;
    D_8009BE30 = v;
    D_8009D564 = v;
    pActor->unk30 = v;
    pActor->unk20 = 0;
    pActor->unk4 = 0;
    *(s32 *)(p + 0x5C) = 0x960000;
    D_8009BD38 = -0x20;
    D_8009BD3A = 0x400;
    D_8009BD3C = 0;
    *(s32 *)(p + 0x50) = -0x20000;
    *(s32 *)(p + 0x38) = -0x20000;
    v = D_8009BD3A;
    D_8009D144 = 0;
    D_8009D3F0 = 0x960000;
    *(s32 *)(p + 0x54) = v << 12;
    *(s32 *)(p + 0x3C) = v << 12;
    v = D_8009BD3C;
    D_8009BE0C = 0x78;
    *(s32 *)(p + 0x58) = v << 12;
    *(s32 *)(p + 0x40) = v << 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083A00);

s32 WorldMapSetActorPosition(s32 arg0) {
    WorldMapActor *pActor = &D_8009BE24[arg0];
    s32 x;
    s32 y;
    s32 z;

    pActor->unk28 = 0x1800000;
    pActor->unk2C = 0x80000;
    pActor->unk30 = 0x1A00000;
    pActor->unk20 = 0;
    D_8009C620[8].unk0 = 1;
    D_8009C620[7].unk0 = 1;
    x = pActor->unk28 >> 12;
    D_8009C620[8].unk8 = x;
    D_8009C620[7].unk8 = x;
    y = pActor->unk2C >> 12;
    D_8009C620[8].unkC = y;
    D_8009C620[7].unkC = y;
    z = pActor->unk30 >> 12;
    D_8009C620[8].unk10 = z;
    D_8009C620[7].unk10 = z;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084068);
void func_8008440C(void) {
    RECT rect;
    FadeParams factors = D_800704DC;
    u8 *pData;
    u8 *pDst;
    u16 *pClut;
    s32 i;

    pData = LZSSHeapDecompress(D_8009BD20, 1);
    ModelLoadTextureBlock(pData);
    DrawSync(0);
    HeapFree(pData);
    HeapFree(D_8009BD20);
    pData = HeapAlloc(0x200, 1);
    pDst = HeapAlloc(0x2000, 1);
    rect.x = 0;
    rect.y = 0x1F0;
    rect.w = 0x100;
    rect.h = 1;
    StoreImage(&rect, (u_long *)pData);
    DrawSync(0);
    func_800931D8(pData, pDst, 0x10, (u8 *)&factors);
    rect.x = 0;
    rect.y = 0x1F0;
    rect.w = 0x100;
    rect.h = 0xF;
    LoadImage(&rect, (u_long *)pDst);
    DrawSync(0);
    i = 0;
    pClut = D_8009BCE0;
    for (; i < 0x10; i++, pClut++) {
        *pClut = GetClut(rect.x, rect.y);
        rect.y++;
    }
    HeapFree(pDst);
    HeapFree(pData);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084580);

void WorldMapFreeSpriteSets(void) {
    s32 i;

    for (i = 0; i < D_8009D7E0; i++) {
        HeapFree(D_8009C620[i].unk48);
        ModelPartFreeLightData(D_8009C620[i].unk40);
    }
    HeapFree(D_8009C620);
}

void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1) {
    D_8009C620[arg1].unk50 = &D_8009C620[arg0];
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_800848F4);

s32 WorldMapFindSpriteSet(void *pArg, s16 *pOut) {
    WorldMapEntry *pEntry = D_8009C620;
    s16 result;
    s16 i;

    for (i = 0; i < D_8009D7E0; i++) {
        if ((pEntry->unk4 & 1) && (result = func_80084DB8(pArg, i)) != 0) {
            *pOut = i;
            return result;
        }
        pEntry++;
    }
    return 0;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084DB8);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085158);
// Best: 91.97% (708 bytes vs 716). Every call, argument, offset and the whole
// tail (three RotTrans, the two edge vectors, OuterProduct0, the three >>2, the
// VectorNormal and the WorldMapInterpolatePlaneHeight at the end) already match
// instruction for instruction; what is left is register allocation: retail
// keeps pEntry in s0 and the triangle index in s1 so the triangle pointer and
// the vertex base reuse those two, giving six saved registers, while gcc gives
// us pEntry s1 / index s2 and needs a seventh (s6) for pNormal. Tried: pVerts
// read once into a local (that fixed the repeated `lw 0x4(...)`), pTri before
// pVerts, the scale stores as three literal `*(s32*)0x1F8000xx = 0x800` (82%,
// gcc hoists all four scratchpad addresses into saved registers), the scale
// stores in retail's vz/vy/vx order, and pEntry assigned late. This is the
// allocator-tie class, not a source-shape problem.
/*
void func_80085158(WorldMapPos *pPos, WorldMapPos *pOut, WorldMapPos *pNormal, u16 setIndex,
                   u16 triIndex) {
    WorldMapEntry *pEntry;
    MATRIX *pMat = (MATRIX *)0x1F8000F0;
    VECTOR *pA = (VECTOR *)0x1F800000;
    VECTOR *pB = (VECTOR *)0x1F800010;
    VECTOR *pC = (VECTOR *)0x1F800020;
    WorldMapMesh *pMesh;
    WorldMapTriangle *pTri;
    SVECTOR *pVerts;
    long flag;

    pEntry = &D_8009C620[setIndex];
    pOut->x = (pPos->x >> 12) - pEntry->unk8;
    pOut->z = pEntry->unk10 - (pPos->z >> 12);
    *pMat = pEntry->mat;
    pMat->t[0] = 0;
    pMat->t[2] = 0;
    pB->vz = 0x800;
    pB->vy = 0x800;
    pB->vx = 0x800;
    pMat->t[1] = pEntry->unkC;
    ScaleMatrix(pMat, pB);
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);

    pMesh = pEntry->unk44;
    pTri = &pMesh->tris[triIndex];
    pVerts = pMesh->unk4;
    RotTrans(&pVerts[pTri->vert[0]], pA, &flag);
    RotTrans(&pVerts[pTri->vert[1]], pB, &flag);
    RotTrans(&pVerts[pTri->vert[2]], pC, &flag);
    pB->vx -= pA->vx;
    pB->vy -= pA->vy;
    pB->vz -= pA->vz;
    pC->vx -= pA->vx;
    pC->vy -= pA->vy;
    pC->vz -= pA->vz;
    OuterProduct0(pC, pB, pC);
    pC->vx >>= 2;
    pC->vy >>= 2;
    pC->vz >>= 2;
    VectorNormal(pC, (VECTOR *)pNormal);
    WorldMapInterpolatePlaneHeight(pOut, (WorldMapPos *)pA, pNormal);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085418);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085760);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085CDC);
// Best: 87.17% (620 bytes vs 636). Three loops over the 0x40 actors: project
// each active actor through the camera delta (WorldMapWrapDelta wraps it), run
// RTPS per actor and store SZ3 into the scratch slot at 0x1F800018+4*i, then
// depth-index SpriteDrawFrame and slew each actor's 0x5C angle toward its 0x48
// target. Diffs found but not yet fixed, in the order they appear:
//  1. retail keeps &D_8009BE28 in $s3 and reads D_8009BE30 as +8 off it, so
//     the frame is 0x28 not 0x20 (this also costs the s3 save/restore pair).
//  2. the delta passed to WorldMapWrapDelta is &pScratch[2] (0x1F800008), i.e.
//     addiu a0,$s2,8 - not $s2.
//  3. D_8009C808 is a MATRIX, not a MATRIX*: &D_8009C808 (lui+addiu) is what
//     retail holds in $s0 for both SetRotMatrix/SetTransMatrix.
//  4. the three scratch halfword writes read the actor's 0x4C pointer as
//     *s16 and must emit lh; a s16 array index gives lhu. Use byte casts.
//  5. in the third loop retail loads the 0x48 angle (lh a1,-4(s0)) BEFORE the
//     0x5C current angle (lw a0,16(s0)); the substraction is ang - cur.
//  6. retail's slew is `v1 = (d < 0x100) ? ang : cur + 0x100` with the THEN
//     value in the bnez delay slot and the ELSE after a j; and the `i = 0`
//     that starts loop 2 is emitted after the SetTransMatrix call, not before.
/*
void func_80085CDC(void) {
    WorldMapActor *pActor;
    MATRIX *pMat;
    s32 *pScratch;
    s32 i;
    s32 d;
    s32 cur;
    s16 ang;

    pScratch = (s32 *)0x1F800000;
    pActor = D_8009BE24;
    for (i = 0; i < 0x40; i++, pActor++) {
        if ((*(s16 *)((u8 *)pActor + 0x24) == 0) & (*(s32 *)((u8 *)pActor + 0x4C) != 0)) {
            pScratch[2] = *(s32 *)((u8 *)pActor + 0x28) - D_8009BE28;
            pScratch[4] = *(s32 *)((u8 *)pActor + 0x30) - D_8009BE30;
            WorldMapWrapDelta((WorldMapPos *)pScratch);
            ((s32 *)*(void **)((u8 *)pActor + 0x4C))[0] = pScratch[2] << 4;
            ((s32 *)*(void **)((u8 *)pActor + 0x4C))[2] = -(pScratch[4]) << 4;
            ((s32 *)*(void **)((u8 *)pActor + 0x4C))[1] = *(s32 *)((u8 *)pActor + 0x2C) << 4;
        }
    }
    pMat = &D_8009C808;
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);
    pActor = D_8009BE24;
    for (i = 0; i < 0x40; i++, pActor++) {
        if ((*(s16 *)((u8 *)pActor + 0x24) == 0) & (*(s32 *)((u8 *)pActor + 0x4C) != 0)) {
            ((s16 *)pScratch)[0] = ((s16 *)*(void **)((u8 *)pActor + 0x4C))[1];
            ((s16 *)pScratch)[1] = ((s16 *)*(void **)((u8 *)pActor + 0x4C))[3];
            ((s16 *)pScratch)[2] = ((s16 *)*(void **)((u8 *)pActor + 0x4C))[5];
            gte_ldv0(pScratch);
            gte_rtps();
            gte_stsz(&pScratch[i + 6]);
        }
    }
    GfxSetBaseMatrix(pMat);
    i = 0;
    pActor = D_8009BE24;
    for (; i < 0x40; i++, pActor++) {
        d = pScratch[i + 6];
        if ((*(s16 *)((u8 *)pActor + 0x24) == 0) & (*(s32 *)((u8 *)pActor + 0x4C) != 0) & (d < 0xB00)) {
            SpriteDrawFrame(*(void **)((u8 *)pActor + 0x4C),
                          (u32 *)(((u32 *)D_8009BE3C)[0x1C]) + (d >> 4));
            ang = *(s16 *)((u8 *)pActor + 0x48);
            cur = *(s32 *)((u8 *)pActor + 0x5C);
            d = ang - cur;
            if (d < 0) {
                d += 0x1000;
            }
            if (d < 0x801) {
                if (d < 0x100) {
                    cur = ang;
                } else {
                    cur = cur + 0x100;
                }
            } else {
                d -= 0x1000;
                if (d < -0xFF) {
                    cur = cur - 0x100;
                } else {
                    cur = ang;
                }
            }
            *(s32 *)((u8 *)pActor + 0x5C) = cur;
            func_800223B0(*(void **)((u8 *)pActor + 0x4C),
                          (cur - *(u16 *)&D_8009BD3A - 0x400) & 0xFFF);
            AnimScriptTick(*(void **)((u8 *)pActor + 0x4C));
        }
    }
}
*/
