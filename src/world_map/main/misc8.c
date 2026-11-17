#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "psyq/memory.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/model.h"

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ s16 command;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 state;
    /* 0x22 */ u16 pad22;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ u16 pad26;
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 dirX;
    /* 0x3C */ s32 dirY;
    /* 0x40 */ s32 dirZ;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s16 angle;
    /* 0x4A */ u16 pad4A;
    /* 0x4C */ void *pSprite;
    /* 0x50 */ s32 unk50; // sprite-set slots' tint red; script slots keep their command cursor here
    /* 0x54 */ s32 green;
    /* 0x58 */ s32 blue;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 greenStep;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ s32 rotY;
    /* 0x6C */ s32 scaleAngle;
    /* 0x70 */ s32 scale;
    /* 0x74 */ s32 scaleStep;
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

// An actor's sprite keeps its world position as 16.16 fixed point: the wrap
// loop writes whole words, the GTE pass reads the integer half of each.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapSpritePos; // Size: 0xC, 16.16 fixed point

typedef struct {
    /* 0x0 */ s16 xFrac;
    /* 0x2 */ s16 x;
    /* 0x4 */ s16 yFrac;
    /* 0x6 */ s16 y;
    /* 0x8 */ s16 zFrac;
    /* 0xA */ s16 z;
} WorldMapSpritePosWhole; // Size: 0xC

// One triangle of a sprite set's mesh: three indices into WorldMapMesh::pVerts.
typedef struct {
    /* 0x0 */ s16 vert[3];
    /* 0x6 */ u8 unk6[0x8];
} WorldMapTriangle; // Size: 0xE

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ SVECTOR *pVerts;
    /* 0x8 */ WorldMapTriangle tris[1];
} WorldMapMesh; // Size: 0x16

typedef struct WorldMapEntry {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 partIndex;
    /* 0x04 */ u16 flags;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ s32 z;
    /* 0x14 */ u8 unk14[4];
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ ModelPart *pModel;
    /* 0x44 */ WorldMapMesh *pMesh;
    /* 0x48 */ void *pPackets[2]; // prim lists, [0] source and [1] destination; g_WorldMapPrimPacketIndex picks one
    /* 0x50 */ struct WorldMapEntry *pParent;
} WorldMapEntry; // Size: 0x54

extern WorldMapActor *g_WorldMapSlots; // actors, 0x80 apart
extern WorldMapEntry *g_WorldMapGroups; // sprite sets, 0x54 apart
extern s16 D_8009D7E0; // g_WorldMapGroups entry count
extern s32 g_WorldMapPrimPacketIndex;
extern s16 D_8009AABC[];
extern s16 D_8009AB48[];
extern s16 D_8009ABD4[];
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

// Partial view of the GTE work area in the scratchpad at 0x1F800000.
typedef struct {
    /* 0x00 */ VECTOR vert0; // first RotTrans output
    /* 0x10 */ VECTOR scale; // ScaleMatrix output, later the second RotTrans output
    /* 0x20 */ VECTOR vert2; // third RotTrans output
    /* 0x30 */ u8 unk30[0xC0];
    /* 0xF0 */ MATRIX mat;
} WorldMapScratch; // Size: 0x110

// One sprite set definition inside the block g_WorldMapSpriteSetTable points at.
typedef struct {
    /* 0x0 */ u16 partIndex; // index into ModelFileHeader::modelParts
    /* 0x2 */ u16 flags;
    /* 0x4 */ s16 x;
    /* 0x6 */ s16 y;
    /* 0x8 */ s16 z;
    /* 0xA */ u16 rot[3];
} WorldMapSpriteSetDef; // Size: 0x10

typedef struct {
    /* 0x0 */ u16 count;
    /* 0x2 */ WorldMapSpriteSetDef defs[0];
} WorldMapSpriteSetTable; // Size: 0x2

extern u8 D_8009AC60[];
extern u8 g_WorldMapPlayerPosition[];
extern ModelFileHeader *g_WorldMapModelData;
extern WorldMapSpriteSetTable *g_WorldMapSpriteSetTable;
extern s32 *g_WorldMapModelRelocs;
extern s16 D_8009BD28;
extern MATRIX D_8009A140;
extern MATRIX D_8009A160;
extern s32 g_GfxOTDepthShift;
extern s32 D_8009C16C;
extern s32 D_8009C840;

extern void ModelPartFreeLightData(void *pModelPart);
extern int Vsync(int mode);
extern void WorldMapFreeGridImages(void);
extern void WorldMapUpdateLoadQueue(void);
extern void func_80097BC0(void *p);
extern s32 WorldMapGetQueuedLoadCount(void);
extern s16 func_80084DB8(void *pArg, s16 arg1);
extern void WorldMapBuildSpritePrims(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 arg3);
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapCameraTarget;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;
extern s32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapCameraHeight;
extern s32 g_WorldMapFadeLevel;
extern void HeapFree(void *pMem);
extern void *HeapAlloc(u_int allocSize, u_int allocFlags);
extern void *g_MenuSedsFile;
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 g_WorldMapExitFlag;
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
extern void *g_WorldMapLoadedFileData;
extern u16 D_8009BCE0[];
extern FadeParams D_800704DC;

extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapMode4EntryTriggered;
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(int, int, int, int);
extern int ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern int ArchiveCdDataSync(int);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadClutAndTpageIds(void);
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
extern void WorldMapInitRandomTimers(void);
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
extern u8 *g_WorldMapRenderContext;
extern MATRIX g_WorldMapViewMatrix;
extern void WorldMapWrapDelta(void *pDelta);
extern void WorldMapWrapDeltaScaled(WorldMapPos *pDelta);
extern void ModelRenderPacket(ModelPart *pPart, void *pPacket, u_long *pOt, s32 mode);
extern s16 D_8009AD2C[];
extern s32 g_WorldMapCameraTarget;
extern s32 D_8009BE30;
extern s32 D_80050104;
extern s32 D_800595C0;
extern s32 g_BattleEmittedPacketCount;
extern void func_800223B0(void *p, s32 angle);
extern void SpriteDrawFrame(void *p, u32 *pTbl);
extern void GfxSetBaseMatrix(MATRIX *p);
extern void AnimScriptTick(void *p);

void WorldMapBuildActorModelMatrix(WorldMapActor *pActor, WorldMapModelOut *pOut, WorldMapModel *pModel) {
    if (pActor->state == 0) {
        return;
    }
    if (pActor->state != 1) {
        return;
    }
    {
        pModel->rot.vz = 0;
        pModel->rot.vx = 0;
        pModel->rot.vy = pActor->rotY;
        pModel->scale.vx = pActor->scale & 0x7FFF;
        pModel->scale.vy = 0x1000;
        pModel->scale.vz = rcos(pActor->scaleAngle) * 6;
        RotMatrix(&pModel->rot, &pModel->mat);
        ScaleMatrix(&pModel->mat, &pModel->scale);
        pOut->mat = pModel->mat;
        pActor->unk50 += pActor->unk5C;
        pActor->green += pActor->greenStep;
        if (pActor->unk50 >= 0x100) {
            pActor->unk50 = 0xFF;
            pActor->unk5C = -pActor->unk5C;
        } else if (pActor->unk50 < 0x40) {
            pActor->unk50 = 0x40;
            pActor->unk5C = -pActor->unk5C;
        }
        if (pActor->green >= 0x100) {
            pActor->green = 0xFF;
            pActor->greenStep = -pActor->greenStep;
        } else if (pActor->green < 0x80) {
            pActor->green = 0x80;
            pActor->greenStep = -pActor->greenStep;
        }
        pActor->scaleAngle = (pActor->scaleAngle + 8) & 0xFFF;
        pActor->scale += pActor->scaleStep;
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
    if (pActor->state == 0) {
        return;
    }
    if (pActor->state != 1) {
        return;
    }
    {
        pModel->rot.vz = 0;
        pModel->rot.vx = 0;
        pModel->rot.vy = pActor->rotY;
        pModel->scale.vx = pActor->scale & 0x7FFF;
        pModel->scale.vy = 0x1000;
        pModel->scale.vz = rcos(pActor->scaleAngle) * 6;
        RotMatrix(&pModel->rot, &pModel->mat);
        ScaleMatrix(&pModel->mat, &pModel->scale);
        pOut->mat = pModel->mat;
        pActor->unk50 += pActor->unk5C;
        pActor->green += pActor->greenStep;
        if (pActor->unk50 >= 0x100) {
            pActor->unk50 = 0xFF;
            pActor->unk5C = -pActor->unk5C;
        } else if (pActor->unk50 < 0x40) {
            pActor->unk50 = 0x40;
            pActor->unk5C = -pActor->unk5C;
        }
        if (pActor->green >= 0x100) {
            pActor->green = 0xFF;
            pActor->greenStep = -pActor->greenStep;
        } else if (pActor->green < 0x80) {
            pActor->green = 0x80;
            pActor->greenStep = -pActor->greenStep;
        }
        pActor->scaleAngle = (pActor->scaleAngle + 8) & 0xFFF;
        pActor->scale += pActor->scaleStep;
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
    memcpy(pEntry->pPackets[1], pEntry->pPackets[0], count << 5);
}

void WorldMapSetSpritePrimColor(u8 *pData, s32 count, s32 r, s32 g, s32 b) {
    s32 i;

    for (i = count - 1; i != -1; i--) {
        pData[4] = r;
        pData[5] = g;
        pData[6] = b;
        pData += 0x20;
    }
}

s32 WorldMapSetupSpriteSetPrims(s32 arg0) {
    WorldMapEntry *pEntry = &g_WorldMapGroups[arg0 + 78];

    WorldMapBuildSpritePrims(pEntry, pEntry->pPackets[0], pEntry->pModel->polyCount, 3);
    return 1;
}
s32 WorldMapUpdateSpriteSetPrims(s32 arg0) {
    s32 row = arg0 - 3;
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];
    WorldMapEntry *pEntry = &g_WorldMapGroups[row + 81];
    s32 *pSlots;
    s32 i;

    switch (pActor->command) {
    case 0:
        break;
    case 1:
        pSlots = &pActor->unk50;
        pActor->state = 1;
        pActor->command = 0;
        pEntry->x = D_8009AABC[row * 14];
        pEntry->y = D_8009AABC[row * 14 + 1];
        pEntry->z = D_8009AABC[row * 14 + 2];
        for (i = 3; i < 0xE; i++) {
            *pSlots++ = D_8009AABC[row * 14 + i];
        }
        break;
    case 2:
        pSlots = &pActor->unk50;
        pActor->state = 1;
        pActor->command = 0;
        pEntry->x = D_8009AB48[row * 14];
        pEntry->y = D_8009AB48[row * 14 + 1];
        pEntry->z = D_8009AB48[row * 14 + 2];
        for (i = 3; i < 0xE; i++) {
            *pSlots++ = D_8009AB48[row * 14 + i];
        }
        break;
    case 3:
        pActor->command = 0;
        pActor->state = 0;
        break;
    case 4:
        pSlots = &pActor->unk50;
        pActor->state = 1;
        pActor->command = 0;
        pEntry->x = D_8009ABD4[row * 14];
        pEntry->y = D_8009ABD4[row * 14 + 1];
        pEntry->z = D_8009ABD4[row * 14 + 2];
        for (i = 3; i < 0xE; i++) {
            *pSlots++ = D_8009ABD4[row * 14 + i];
        }
        break;
    }
    WorldMapBuildActorModelMatrix(pActor, pEntry, (WorldMapModel *)PSX_SCRATCH);
    WorldMapSetSpritePrimColor(pEntry->pPackets[g_WorldMapPrimPacketIndex],
                               pEntry->pModel->polyCount,
                               pActor->unk50, pActor->green, pActor->blue);
    return 1;
}

s32 WorldMapReturnSuccess(void) {
    return 1;
}

s32 WorldMapWaitForQueueDrain(s32 arg0) {
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];

    if (pActor->command == 1) {
        pActor->command = 0;
        DrawSync(0);
        Vsync(0);
        WorldMapFreeGridImages();
        func_80097BC0(g_WorldMapPlayerPosition);
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
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    g_WorldMapDrawMode = 0x10;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    pVec = (s32 *)g_WorldMapPlayerPosition;
    *pVec = 0x1800000;
    g_WorldMapPlayerPositionY = 0xFFF00000;
    g_WorldMapPlayerPositionZ = 0x1A00000;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    WorldMapLoadClutAndTpageIds();
    func_800736DC();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(g_MenuSedsFile);
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
    WorldMapInitRandomTimers();
}
s32 WorldMapShutdownScene(void) {
    u16 v;

    SoundStopAllElements();
    SoundRemoveSedsEntry(g_MenuSedsFile);
    HeapFree(g_MenuSedsFile);
    WorldMapFreeSpriteSets();
    WorldMapFreeTileVertexBuffers();
    WorldMapFreeSemiTransPrimBuffers();
    WorldMapFreeTileAnimEntries();
    WorldMapFreeFrameAnimEntries();
    WorldMapFreeSpriteGroupTable();
    WorldMapFreePrimTemplateBuffers();
    WorldMapFreeGridImages();
    HeapFree(g_WorldMapWorkBuffer1);
    HeapFree(g_WorldMapWorkBuffer2);
    HeapFree(g_WorldMapTilePrimBuffer);
    HeapFree(g_WorldMapTilePrimBufferCopy);
    HeapFree(g_WorldMapDecompressedMapData);
    WorldMapFreeSlotPool();
    v = *(u16 *)&g_WorldMapViewRotationY;
    g_NextMapId = 0x269;
    g_NextMapFlags = 4;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
    return 1;
}

s32 WorldMapSetActorParams(s32 arg0) {
    g_WorldMapSlots[arg0].unk50 = (u32)D_8009AC60;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008390C);
// Round 164: re-measured. The raw body below builds at 84.75% but is 224 bytes
// against retail's 244 (61 insns), so the store-to-load fold of
// g_WorldMapViewRotationY/Z also drops the two `lui`/`lh`/`sll` triples. The
// typed-struct form re-measures 30.52% today (unchanged); a `volatile s16` on
// the two externs brings both `lh` back but compiles to lhu+sll+sra and 252
// bytes. Nothing tried keeps the two `lh` AND 244 bytes.
// Round 111 triage: this body did NOT compile - it was defined void while the
// unit's own extern at the top of the file says s32 (retail returns 1), so every
// attempt to unpark it died before it produced bytes. With the s32 return below
// it builds and scores 84.75%, 224 bytes = retail's exact size, so round 64's
// numbers are real. New this round: '*(volatile s16 *)&g_WorldMapViewRotationY' does force
// retail's reload but gcc emits lhu + sll 0x10 + sra 0x4 for it, 252 bytes,
// 72.84% - gcc's volatile short load is unsigned, so the fold stays open.
// Best: 84.75% with the raw byte-pointer form below; the typed-struct form
// (pActor->unk7C = 0x1000; g_WorldMapViewRotationY = 0x400; ... v = g_WorldMapViewRotationY;) scores
// 30.52% because gcc reorders the whole straight-line block once it folds the
// store-to-load pair into the constant. Every instruction and relocation of
// the raw form matches except that folding: retail re-loads g_WorldMapViewRotationY (lh)
// and g_WorldMapViewRotationZ (lh) after storing them, we get lui $v0,0x40 instead.
// Round-49 update: a volatile *store* (*(volatile s16 *)&g_WorldMapViewRotationY = 0x400)
// does stop the fold and restores both lh's, but the extra live value shifts
// the allocator one register over (temps to a1, 0x960000 to a0; 244 vs 224
// bytes, 77%); the same happens spelling the angle globals as one array
// (extern s16 g_WorldMapCameraRot[]) with the reads through g_WorldMapCameraRot[1]/[2]: no fold,
// 79.41%, registers and store order off. My third shape - struct fields for the
// stores, plain externs for the reads - is 79.57% and also loses the allocator.
// -O1 scores 67.46. A volatile load forces the reload but emits
// lhu + sll 0x10 + sra 0x4 instead of lh + sll 0xc (72.84%); volatile on the
// stores alone changes nothing.
// Round 64: two more shapes, both keep the fold away but not the scheduler.
// (1) stores scalar (exact retail relocations) + reads spelled
//     g_WorldMapCameraRot[1]/[2] with `extern s16 g_WorldMapCameraRot[]`: no fold, the two `lh`
//     come back, but 51.59% / 244 bytes vs retail's 224 - cc1 hoists every
//     li+store pair past the $0 stores and lands the WorldMapActor cluster at
//     the end, exactly the misc4 WorldMapInitActorSlotTimer40 / misc7 func_800827EC class.
// (2) try_flags --sweep over that body: best is gcc-2.7.2-cdk -O1 74.33,
//     2.7.2 -O1 74.08, every -O2/-O3 row 43-52. So the store order is a -O2
//     scheduler property, not a preset question; the unit's already-matched
//     functions keep their flags.
/*
s32 func_8008390C(s32 arg0) {
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];
    u8 *p = (u8 *)pActor;
    s32 v;

    *(s32 *)(p + 0x7C) = 0x1000;
    v = *(s32 *)g_WorldMapPlayerPosition;
    g_WorldMapCameraTarget = v;
    g_WorldMapScrollTargetX = v;
    pActor->x = v;
    v = g_WorldMapPlayerPositionY;
    D_8009BE2C = v;
    g_WorldMapScrollTargetY = v;
    pActor->y = v;
    v = g_WorldMapPlayerPositionZ;
    D_8009BE30 = v;
    g_WorldMapScrollTargetZ = v;
    pActor->z = v;
    pActor->state = 0;
    pActor->command = 0;
    *(s32 *)(p + 0x5C) = 0x960000;
    g_WorldMapCameraRot = -0x20;
    g_WorldMapViewRotationY = 0x400;
    g_WorldMapViewRotationZ = 0;
    *(s32 *)(p + 0x50) = -0x20000;
    *(s32 *)(p + 0x38) = -0x20000;
    v = g_WorldMapViewRotationY;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapCameraHeight = 0x960000;
    *(s32 *)(p + 0x54) = v << 12;
    *(s32 *)(p + 0x3C) = v << 12;
    v = g_WorldMapViewRotationZ;
    g_WorldMapFadeLevel = 0x78;
    *(s32 *)(p + 0x58) = v << 12;
    *(s32 *)(p + 0x40) = v << 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083A00);

s32 WorldMapSetActorPosition(s32 arg0) {
    WorldMapActor *pActor = &g_WorldMapSlots[arg0];
    s32 x;
    s32 y;
    s32 z;

    pActor->x = 0x1800000;
    pActor->y = 0x80000;
    pActor->z = 0x1A00000;
    pActor->state = 0;
    g_WorldMapGroups[8].unk0 = 1;
    g_WorldMapGroups[7].unk0 = 1;
    x = pActor->x >> 12;
    g_WorldMapGroups[8].x = x;
    g_WorldMapGroups[7].x = x;
    y = pActor->y >> 12;
    g_WorldMapGroups[8].y = y;
    g_WorldMapGroups[7].y = y;
    z = pActor->z >> 12;
    g_WorldMapGroups[8].z = z;
    g_WorldMapGroups[7].z = z;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084068);
void WorldMapLoadTextureClut(void) {
    RECT rect;
    FadeParams factors = D_800704DC;
    u8 *pData;
    u8 *pDst;
    u16 *pClut;
    s32 i;

    pData = LZSSHeapDecompress(g_WorldMapLoadedFileData, 1);
    ModelLoadTextureBlock(pData);
    DrawSync(0);
    HeapFree(pData);
    HeapFree(g_WorldMapLoadedFileData);
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
void WorldMapInitSpriteSets(void) {
    WorldMapSpriteSetTable *pSetTable;
    WorldMapSpriteSetDef *pDef;
    WorldMapMesh *pMesh;
    ModelPart *pPart;
    s32 *pRelocs;
    s32 *pCur;
    s16 count;
    s32 i;

    count = ModelResolvePointers(g_WorldMapModelData);
    D_8009BD28 = count;
    pRelocs = g_WorldMapModelRelocs;
    pCur = pRelocs + 1;
    g_WorldMapModelRelocs = pCur;
    for (i = 0; i < count; i++) {
        *pCur++ = (s32)pRelocs + *pCur;
    }
    count = g_WorldMapSpriteSetTable->count;
    D_8009D7E0 = count;
    g_WorldMapGroups = HeapAlloc(count * sizeof(WorldMapEntry), 0);
    pSetTable = g_WorldMapSpriteSetTable;
    SetColorMatrix(&D_8009A140);
    SetLightMatrix(&D_8009A160);
    pDef = pSetTable->defs;
    for (i = 0; i < D_8009D7E0; i++, pDef++) {
        g_WorldMapGroups[i].unk0 = 0;
        g_WorldMapGroups[i].partIndex = pDef->partIndex;
        g_WorldMapGroups[i].flags = pDef->flags;
        g_WorldMapGroups[i].x = pDef->x;
        g_WorldMapGroups[i].y = pDef->y;
        g_WorldMapGroups[i].z = -pDef->z;
        g_WorldMapGroups[i].rot.vx = pDef->rot[0];
        g_WorldMapGroups[i].rot.vy = pDef->rot[1];
        g_WorldMapGroups[i].rot.vz = pDef->rot[2];
        RotMatrixYXZ(&g_WorldMapGroups[i].rot, &g_WorldMapGroups[i].mat);
        pPart = &((ModelFileHeader *)g_WorldMapModelData)->modelParts[g_WorldMapGroups[i].partIndex];
        g_WorldMapGroups[i].pModel = pPart;
        ModelAllocatePackets(pPart, &g_WorldMapGroups[i].pPackets[0], &g_WorldMapGroups[i].pPackets[1]);
        ModelInitializePacket(g_WorldMapGroups[i].pModel, g_WorldMapGroups[i].pPackets[0], 1);
        memcpy(g_WorldMapGroups[i].pPackets[1], g_WorldMapGroups[i].pPackets[0],
               g_WorldMapGroups[i].pModel->unk20[1].modelPacketSize);
        pMesh = ((WorldMapMesh **)g_WorldMapModelRelocs)[g_WorldMapGroups[i].partIndex];
        g_WorldMapGroups[i].pMesh = pMesh;
        pMesh->pVerts = (SVECTOR *)((u8 *)pMesh + (s32)pMesh->pVerts);
        g_WorldMapGroups[i].pParent = 0;
    }
    g_GfxOTDepthShift = 2;
    D_8009C16C = -1;
    D_8009C840 = -1;
    g_WorldMapGroups[4].unk0 = 1;
}

void WorldMapFreeSpriteSets(void) {
    s32 i;

    for (i = 0; i < D_8009D7E0; i++) {
        HeapFree(g_WorldMapGroups[i].pPackets[0]);
        ModelPartFreeLightData(g_WorldMapGroups[i].pModel);
    }
    HeapFree(g_WorldMapGroups);
}

void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1) {
    g_WorldMapGroups[arg1].pParent = &g_WorldMapGroups[arg0];
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_800848F4);
// Round 164: un-parked and repaired - the park did not compile (pScale and pB had
// no declarations; both are the scale VECTOR at 0x1F800010, pB was pScale's
// second name). It builds now and still measures 76.46%, 1060 bytes.
// Best: 76.46% (1060 bytes vs 1036). Structure and every call/offset in the
// head, the sprite-set copy and the tail (WrapDeltaScaled, ScaleMatrix,
// CompMatrix, SetRot/TransMatrix, RTPS, FLAG, SZ3, ModelRenderPacket) already
// agree; what is left is (a) gcc hoisting each scratchpad pointer into its own
// saved register (s0=0xF0, s7=0xA0, s8=0x10, s6=0x110) where retail keeps ONE
// base ($s1 = 0x1F800000) and derives 0xF0/0xF2/0xF4 from it, and (b) the GTE
// halves of the chain loop: retail splits the rotation-matrix load around the
// three column transforms and interleaves the translation ctc2s through
// $12-$14 (gte_SetRotMatrix_gpr order), and its first matrix load reaches the
// node through $t4-$t6, so gte_SetRotMatrix + gte_SetTransVector cannot
// reproduce it as two macro calls.
/*
void func_800848F4(void) {
    MATRIX *pMat = (MATRIX *)0x1F8000F0;
    MATRIX *pOut = (MATRIX *)0x1F800110;
    SVECTOR *pRot = (SVECTOR *)0x1F8000A0;
    VECTOR *pScale = (VECTOR *)0x1F800010;
    WorldMapPos *pDelta = (WorldMapPos *)0x1F800000;
    WorldMapEntry *pChain;
    s32 camX;
    s32 camZ;
    s32 scale;
    s32 i;
    long flag;

    scale = 0x800;
    *(s32 *)&pScale->vz = scale;
    *(s32 *)&pScale->vy = scale;
    *(s32 *)&pScale->vx = scale;
    D_80050104 = 3;
    D_800595C0 = 0;
    g_BattleEmittedPacketCount = 0;
    pRot->vz = 0;
    pRot->vy = 0;
    pRot->vx = 0;
    camX = g_WorldMapCameraTarget >> 12;
    camZ = D_8009BE30 >> 12;
    for (i = 0; i < D_8009D7E0; i++) {
        if (g_WorldMapGroups[i].unk0 != 0) {
            continue;
        }
        *pMat = g_WorldMapGroups[i].mat;
        pMat->t[1] = g_WorldMapGroups[i].x;
        pMat->t[2] = g_WorldMapGroups[i].y;
        pMat->t[3] = -g_WorldMapGroups[i].z;
        pChain = g_WorldMapGroups[i].pParent;
        while (pChain != 0) {
            pChain->mat.t[1] = pChain->x;
            pChain->mat.t[2] = pChain->y;
            pChain->mat.t[3] = -pChain->z;
            gte_SetRotMatrix(&pChain->mat);
            gte_ldclmv(&pMat->m[0][0]);
            gte_mvmva(1, 0, 3, 3, 0);
            gte_stclmv(&pMat->m[0][0]);
            gte_ldclmv(&pMat->m[0][1]);
            gte_mvmva(1, 0, 3, 3, 0);
            gte_stclmv(&pMat->m[0][1]);
            gte_ldclmv(&pMat->m[0][2]);
            gte_mvmva(1, 0, 3, 3, 0);
            gte_stclmv(&pMat->m[0][2]);
            gte_SetTransVector(&pChain->mat);
            gte_ldlv0(&pMat->t[1]);
            gte_rt();
            gte_stlvnl(&pMat->t[1]);
            pChain = pChain->pParent;
        }
        pDelta->x = pMat->t[1] - camX;
        pDelta->z = -pMat->t[3] - camZ;
        WorldMapWrapDeltaScaled(pDelta);
        pMat->t[1] = pDelta->x;
        pMat->t[3] = -pDelta->z;
        ScaleMatrix(pMat, pScale);
        CompMatrix(&g_WorldMapViewMatrix, pMat, pOut);
        SetRotMatrix(pOut);
        SetTransMatrix(pOut);
        gte_ldv0(pRot);
        gte_rtps();
        gte_stflg_val(flag);
        *(s32 *)0x1F800020 = flag;
        if (*(s32 *)0x1F800020 < 0) {
            continue;
        }
        gte_stsz((s32 *)0x1F800028);
        if (*(s32 *)0x1F800028 >= 0xD80) {
            continue;
        }
        ModelRenderPacket(g_WorldMapGroups[i].pModel, g_WorldMapGroups[i].pPackets[g_WorldMapPrimPacketIndex],
                          *(u_long **)(g_WorldMapRenderContext + 0x70), D_8009AD2C[g_WorldMapGroups[i].flags]);
    }
}
*/
// Not attempted: 259 instructions, no C written, no percentage. splat already
// labels the stub "Handwritten function" and the body is one GTE pipeline, so
// this is the class AGENTS.md calls unmatchable blind; it needs the whole body
// written before the first diff tells you anything.
// Decode, for whoever picks it up (addresses are the scratchpad, all reached
// through $s1 = 0x1F800028, i.e. write them as byte offsets off one base):
//   head: 0x1F800018/0x14/0x10 = 0x800 (scale, vz/vy/vx order), D_80050104 = 3,
//         D_800595C0 = 0, g_BattleEmittedPacketCount = 0, SVECTOR at 0x1F8000A0 = {0,0,0}.
//   per sprite set i (s4 = i, s2 = i*0x54): skip if g_WorldMapGroups[i].unk0 != 0;
//         copy g_WorldMapGroups[i].mat (0x20..0x3C) to 0x1F8000F0..0x10C, then
//         overwrite 0x1F800104/108/10C = unk8/unkC/-unk10;
//   chain: while (p->unk50) { p = p->unk50; p->unk34/38/3C = unk8/unkC/-unk10;
//         gte matrix load of p->unk20 into $0-$4, three MVMVA 1,0,3,3,0 rotations
//         of the 3 shorts at 0x1F8000F0/F2/F4 (lh->mtc2 $9-$11, mfc2 $12-$14 stored
//         back as shorts at stride 6), gte load of p->0x34..0x3C into $5-$7, then
//         mtc2 $0 + lwc2 $1 8(...) and MVMVA 1,0,0,0,0 with swc2 $25/$26/$27 into
//         0x1F800104..10C; }  -- the tail call is WorldMapWrapDeltaScaled.
//   then: 0x1F800000 = 0x1F800104 - (g_WorldMapCameraTarget >> 12),
//         0x1F800008 = -0x1F80010C - (D_8009BE30 >> 12);
//         ScaleMatrix(0x1F8000F0, 0x1F800010); CompMatrix(&g_WorldMapViewMatrix,
//         0x1F8000F0, 0x1F800110); SetRotMatrix/SetTransMatrix(0x1F800110);
//         RTPS on 0x1F8000A0, flag to 0x1F800020 (bltz skips), SZ3 to 0x1F800028
//         (>= 0xD80 skips), then
//         ModelRenderPacket(g_WorldMapGroups[i].pModel,
//             *(u32 *)((u8 *)g_WorldMapGroups + g_WorldMapPrimPacketIndex * 4 + 0x48),
//             *(u32 *)(g_WorldMapRenderContext + 0x70), D_8009AD2C[g_WorldMapGroups[i].flags]).
// The GTE encodings are the DMPSX 4.3 spellings (interleaved ctc2 of $12-$14 for
// the matrix load, nop;nop;.word for MVMVA), which inline_c.h still carries, so
// this is a body to write, not a wall -- but it is a two-hour job, not a stub.


s32 WorldMapFindSpriteSet(void *pArg, s16 *pOut) {
    WorldMapEntry *pEntry = g_WorldMapGroups;
    s16 result;
    s16 i;

    for (i = 0; i < D_8009D7E0; i++) {
        if ((pEntry->flags & 1) && (result = func_80084DB8(pArg, i)) != 0) {
            *pOut = i;
            return result;
        }
        pEntry++;
    }
    return 0;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084DB8);


void WorldMapProjectTriangleNormal(WorldMapPos *pPos, WorldMapPos *pOut, WorldMapPos *pNormal, u16 setIndex,
                   u16 triIndex) {
    WorldMapEntry *pEntry;
    WorldMapMesh *pMesh;
    WorldMapTriangle *pTri;
    SVECTOR *pVerts;
    MATRIX *pMat = (MATRIX *)0x1F8000F0;
    VECTOR *pA = (VECTOR *)0x1F800000;
    VECTOR *pB;
    WorldMapScratch *pWork = (WorldMapScratch *)0x1F800000;
    long flag;

    pEntry = &g_WorldMapGroups[setIndex];
    pOut->x = (pPos->x >> 12) - pEntry->x;
    pOut->z = pEntry->z - (pPos->z >> 12);
    *pMat = pEntry->mat;
    pMat->t[2] = 0;
    pMat->t[0] = 0;
    pMat->t[1] = pEntry->y;
    pWork->scale.vz = 0x800;
    pWork->scale.vy = 0x800;
    pWork->scale.vx = 0x800;
    pB = (VECTOR *)0x1F800010;
    ScaleMatrix(pMat, pB);
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);

    pMesh = pEntry->pMesh;
    pTri = pMesh->tris;
    pTri += triIndex;
    pVerts = pMesh->pVerts;
    RotTrans(&pVerts[pTri->vert[0]], pA, &flag);
    RotTrans(&pVerts[pTri->vert[1]], pB, &flag);
    RotTrans(&pVerts[pTri->vert[2]], &pWork->vert2, &flag);
    pB->vx -= pA->vx;
    pB->vy -= pA->vy;
    pB->vz -= pA->vz;
    pWork->vert2.vx -= pA->vx;
    pWork->vert2.vy -= pA->vy;
    pWork->vert2.vz -= pA->vz;
    OuterProduct0(&pWork->vert2, pB, &pWork->vert2);
    pWork->vert2.vx >>= 2;
    pWork->vert2.vy >>= 2;
    pWork->vert2.vz >>= 2;
    VectorNormal(&pWork->vert2, (VECTOR *)pNormal);
    WorldMapInterpolatePlaneHeight(pOut, (WorldMapPos *)pA, pNormal);
}
s32 WorldMapCheckTriangleEdgeSide(WorldMapPos *pPos, s32 yOffset, u16 setIndex, u16 triIndex) {
    WorldMapEntry *pEntry;
    WorldMapMesh *pMesh;
    WorldMapTriangle *pTri;
    SVECTOR *pVerts;
    MATRIX *pMat = (MATRIX *)0x1F8000F0;
    VECTOR *pA = (VECTOR *)0x1F800000;
    VECTOR *pB;
    MATRIX *pLine = (MATRIX *)0x1F800110;
    VECTOR *pNormal = (VECTOR *)0x1F800030;
    VECTOR *pDst = (VECTOR *)0x1F800040;
    WorldMapScratch *pWork = (WorldMapScratch *)0x1F800000;
    long flag;
    s32 x;
    s32 y;
    s32 z;
    s32 entryZ;

    pEntry = &g_WorldMapGroups[setIndex];
    *pMat = pEntry->mat;
    pMat->t[2] = 0;
    pMat->t[0] = 0;
    pMat->t[1] = pEntry->y;
    pWork->scale.vz = 0x800;
    pWork->scale.vy = 0x800;
    pWork->scale.vx = 0x800;
    pB = (VECTOR *)0x1F800010;
    ScaleMatrix(pMat, pB);
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);
    pMesh = pEntry->pMesh;
    pTri = pMesh->tris;
    pTri += triIndex;
    pVerts = pMesh->pVerts;
    RotTrans(&pVerts[pTri->vert[0]], pA, &flag);
    RotTrans(&pVerts[pTri->vert[1]], pB, &flag);
    RotTrans(&pVerts[pTri->vert[2]], &pWork->vert2, &flag);
    pB->vx -= pA->vx;
    pB->vy -= pA->vy;
    pB->vz -= pA->vz;
    pWork->vert2.vx -= pA->vx;
    pWork->vert2.vy -= pA->vy;
    pWork->vert2.vz -= pA->vz;
    OuterProduct0(&pWork->vert2, pB, &pWork->vert2);
    pWork->vert2.vx >>= 2;
    pWork->vert2.vy >>= 2;
    pWork->vert2.vz >>= 2;
    VectorNormal(&pWork->vert2, pNormal);
    x = (pPos->x >> 12) - pEntry->x - pA->vx;
    pLine->m[0][0] = x;
    pLine->m[1][0] = x;
    y = (pPos->y >> 12) - pA->vy;
    pLine->m[0][1] = y;
    entryZ = pEntry->z;
    z = entryZ - (pPos->z >> 12);
    pLine->m[1][1] = y - yOffset;
    z -= pWork->vert0.vz;
    pLine->m[0][2] = z;
    pLine->m[1][2] = z;
    ApplyMatrixLV(pLine, pNormal, pDst);
    return (pDst->vx ^ pDst->vy) >> 31;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085760);
// Best: 78.64% in 1368 bytes vs retail's 1400 (32 bytes / 8 insns short). The
// body below is structurally right -- it is the register frame that is wrong:
// retail's frame is 0x30 and it keeps only three scratchpad bases live
// ($s1 = 0x1F800000, $s3 = 0x1F800010, $s0 = 0x1F800020, the gte_stlvnl
// destinations), so 0x1F8000F0/0x30/0x40/0x50 are ABSOLUTE constant
// expressions in the original (`sw $v0,64($at)`, `lui $a0,0x1F80; ori 0xF0`),
// and the pack's vz reads are absolute too (`lui $a0,0x1F800008; lw 0($a0)`)
// while its vx reads reuse the base register (`lhu 0($s1)`) -- i.e. the
// original's vx and vz reads were *different expressions*, which is why the
// pointer locals here push the frame to 0x40 and cost the 8 insns.
// Everything else is mapped: a0 = a WorldMapPos used only by the switch arms,
// a1 = the WorldMapPos the clip origin comes from, a2 = setIndex as s32 (no
// andi before the *0x54 chain, unlike WorldMapCheckTriangleEdgeSide whose is u16), a3 =
// triIndex. After the three MVMVA passes it packs (vz<<16)|(u16)vx for the
// three vertices and the clip origin into the three longs at 0x1F800030 and
// calls NormalClip(v0,v1,origin) -> bit 1, (v1,v2) -> bit 2, (v2,v0) -> bit 4;
// switch (clip) { case 3: re-pack the origin from a0 (& 0xFFFF on x),
// NormalClip(origin, v1, origin) != 0 -> clip = 1; case 5: same with v0 ->
// clip = 4; case 6: same with v2 -> clip = 2; } then three edge normals at
// 0x1F800030/40/50 each VectorNormal(n, n) and `return clip`. gte_stflg(&flag)
// is already exact (cfc2 $12 = $t4, nop, sw through the address register).
// The one clue to the original's spelling of those constants: the
// VectorNormal argument is built as `addiu $a0,$s1,0x30` (and 0x40, 0x50), i.e.
// pVert0 + 3/4/5 in VECTOR strides, while the clip origin's stores to
// 0x1F800040 are absolute -- so the original mixed two typed views of the same
// scratchpad slots and gcc kept them as separate pseudos. Reproduce that and
// the saved registers come down to retail's five.
/*
s32 func_80085760(WorldMapPos *pPos, WorldMapPos *pRef, s32 setIndex, s32 triIndex) {
    WorldMapEntry *pEntry;
    WorldMapMesh *pMesh;
    WorldMapTriangle *pTri;
    SVECTOR *pVerts;
    VECTOR *pVert0 = (VECTOR *)0x1F800000;
    VECTOR *pVert1 = (VECTOR *)0x1F800010;
    VECTOR *pVert2 = (VECTOR *)0x1F800020;
    VECTOR *pArgs = (VECTOR *)0x1F800030;
    VECTOR *pPosVec = (VECTOR *)0x1F800040;
    VECTOR *pNormal2 = (VECTOR *)0x1F800050;
    MATRIX *pMat = (MATRIX *)0x1F8000F0;
    long flag;
    s32 clip;

    pEntry = &g_WorldMapGroups[setIndex];
    pPosVec->vx = (pRef->x >> 12) - pEntry->x;
    pPosVec->vz = pEntry->z - (pRef->z >> 12);
    *pMat = pEntry->mat;
    pMat->t[2] = 0;
    pMat->t[0] = 0;
    pMat->t[1] = pEntry->y;
    pVert0->vz = 0x800;
    pVert0->vy = 0x800;
    pVert0->vx = 0x800;
    ScaleMatrix(pMat, pVert0);
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);
    pMesh = pEntry->pMesh;
    pTri = &pMesh->tris[triIndex];
    pVerts = pMesh->pVerts;
    gte_ldv0(&pVerts[pTri->vert[0]]);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(pVert0);
    gte_stflg(&flag);
    gte_ldv0(&pVerts[pTri->vert[1]]);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(pVert1);
    gte_stflg(&flag);
    gte_ldv0(&pVerts[pTri->vert[2]]);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(pVert2);
    gte_stflg(&flag);
    clip = 0;
    pArgs->vx = (pVert0->vz << 16) | ((u16 *)pVert0)[0];
    pArgs->vy = (pVert1->vz << 16) | ((u16 *)pVert1)[0];
    pArgs->vz = (pPosVec->vz << 16) | ((u16 *)pPosVec)[0];
    if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) > 0) {
        clip |= 1;
    }
    pArgs->vx = (pVert1->vz << 16) | ((u16 *)pVert1)[0];
    pArgs->vy = (pVert2->vz << 16) | ((u16 *)pVert2)[0];
    if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) > 0) {
        clip |= 2;
    }
    pArgs->vx = (pVert2->vz << 16) | ((u16 *)pVert2)[0];
    pArgs->vy = (pVert0->vz << 16) | ((u16 *)pVert0)[0];
    if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) > 0) {
        clip |= 4;
    }
    switch (clip) {
    case 3:
        pArgs->vx = ((pEntry->z - (pPos->z >> 12)) << 16) | (((pPos->x >> 12) - pEntry->x) & 0xFFFF);
        pArgs->vy = (pVert1->vz << 16) | ((u16 *)pVert1)[0];
        if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) != 0) {
            clip = 1;
        }
        break;
    case 5:
        pArgs->vx = ((pEntry->z - (pPos->z >> 12)) << 16) | (((pPos->x >> 12) - pEntry->x) & 0xFFFF);
        pArgs->vy = (pVert0->vz << 16) | ((u16 *)pVert0)[0];
        if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) != 0) {
            clip = 4;
        }
        break;
    case 6:
        pArgs->vx = ((pEntry->z - (pPos->z >> 12)) << 16) | (((pPos->x >> 12) - pEntry->x) & 0xFFFF);
        pArgs->vy = (pVert2->vz << 16) | ((u16 *)pVert2)[0];
        if (NormalClip(pArgs->vx, pArgs->vy, pArgs->vz) != 0) {
            clip = 2;
        }
        break;
    }
    pArgs->vx = pVert1->vx - pVert0->vx;
    pArgs->vy = 0;
    pArgs->vz = pVert1->vz - pVert0->vz;
    VectorNormal(pArgs, pArgs);
    pPosVec->vx = pVert2->vx - pVert1->vx;
    pPosVec->vy = 0;
    pPosVec->vz = pVert2->vz - pVert1->vz;
    VectorNormal(pPosVec, pPosVec);
    pNormal2->vx = pVert0->vx - pVert2->vx;
    pNormal2->vy = 0;
    pNormal2->vz = pVert0->vz - pVert2->vz;
    VectorNormal(pNormal2, pNormal2);
    return clip;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085CDC);
// Best: 97.70% at retail's exact size (636 bytes), instruction multiset
// identical. Everything else is now exact: the prologue save order needs `i = 0`
// assigned BEFORE pCam is materialised (that is delta 1, fixed); the pScratch[i+6]
// load in loop 3 must be read INLINE in the condition and in the SpriteDrawFrame
// index, not through a local, or sched2 hoists it above the lh (delta 2, fixed);
// and the slew arms assign to `d` (the ang-cur delta) rather than to cur, which
// puts the result in $v1 as retail has it. The only delta left is an allocator
// tie: retail holds ang in $a1 and cur in $a0, this body holds ang in $a0 and cur
// in $a1, which also flips the two load orders and the two arm registers.
// Declaration order does not move it (4 orders measured); an earlier permuter run
// (base 770, best 555) only "improved" it by making the body semantically wrong.
// Two relocation-only diffs remain and are byte identical: the WrapDelta argument
// names D_1F800008 and the loop-3 tail D_1F800004 where this body folds to 8/4.
/*
void func_80085CDC(void) {
    WorldMapActor *pActor;
    MATRIX *pMat;
    s32 *pScratch;
    WorldMapPos *pCam;
    s32 i;
    s32 cur;
    s16 ang;
    s32 d;
    s32 v;

    pScratch = (s32 *)0x1F800000;
    i = 0;
    pCam = (WorldMapPos *)&g_WorldMapCameraTarget;
    pActor = g_WorldMapSlots;
    for (; i < 0x40; i++, pActor++) {
        if ((pActor->unk24 == 0) & (pActor->pSprite != 0)) {
            pScratch[2] = pActor->x - pCam->x;
            pScratch[4] = pActor->z - pCam->z;
            WorldMapWrapDelta(&pScratch[2]);
            ((WorldMapSpritePos *)pActor->pSprite)->x = pScratch[2] << 4;
            ((WorldMapSpritePos *)pActor->pSprite)->z = -pScratch[4] << 4;
            ((WorldMapSpritePos *)pActor->pSprite)->y = pActor->y << 4;
        }
    }
    pMat = &g_WorldMapViewMatrix;
    SetRotMatrix(pMat);
    SetTransMatrix(pMat);
    pActor = g_WorldMapSlots;
    for (i = 0; i < 0x40; i++, pActor++) {
        if ((pActor->unk24 == 0) & (pActor->pSprite != 0)) {
            v = ((WorldMapSpritePosWhole *)pActor->pSprite)->x;
            ((SVECTOR *)pScratch)->vx = v;
            v = ((WorldMapSpritePosWhole *)pActor->pSprite)->y;
            ((SVECTOR *)pScratch)->vy = v;
            v = ((WorldMapSpritePosWhole *)pActor->pSprite)->z;
            ((SVECTOR *)pScratch)->vz = v;
            gte_ldv0((SVECTOR *)pScratch);
            gte_rtps();
            gte_stsz(&pScratch[i + 6]);
        }
    }
    GfxSetBaseMatrix(&g_WorldMapViewMatrix);
    i = 0;
    pActor = g_WorldMapSlots;
    for (; i < 0x40; i++, pActor++) {
        if ((pActor->unk24 == 0) & (pActor->pSprite != 0) & (pScratch[i + 6] < 0xB00)) {
            SpriteDrawFrame(pActor->pSprite, (u32 *)(((u32 *)g_WorldMapRenderContext)[0x1C]) + (pScratch[i + 6] >> 4));
            ang = pActor->angle;
            cur = pActor->unk5C;
            d = ang - cur;
            if (d < 0) {
                d += 0x1000;
            }
            if (d < 0x801) {
                if (d >= 0x100) {
                    d = cur + 0x100;
                } else {
                    d = ang;
                }
            } else {
                d -= 0x1000;
                if (d < -0xFF) {
                    d = cur - 0x100;
                } else {
                    d = ang;
                }
            }
            pActor->unk5C = d;
            func_800223B0(pActor->pSprite, (d - *(u16 *)&g_WorldMapViewRotationY - 0x400) & 0xFFF);
            AnimScriptTick(pActor->pSprite);
        }
    }
}
*/
