#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/memory.h"
#include "system/model.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} WorldMapPos4; // Size: 0x10

// One control point of the camera path table at g_WorldMapCameraPathPoints (stride 8): halfword
// coordinates plus the -1 terminator that ends the table at +0x6.
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ s16 unk6;
} WorldMapPathPoint; // Size: 0x8

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
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ u8 pad68[0x4];
    /* 0x6C */ s32 unk6C;
    /* 0x70 */ s32 unk70;
    /* 0x74 */ s32 unk74;
    /* 0x78 */ u8 pad78[0x8];
} WorldMapActor; // Size: 0x80

// One entry of the g_WorldMapGroups table (stride 0x54). 0x48 and 0x4C are the source
// and destination prim lists (g_WorldMapGroups[14]/[15] feed WorldMapSetupPrimTemplates).
typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ WorldMapPos4 pos;
    /* 0x18 */ u8 unk18[0x8];
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ ModelPart *pModel;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ void *pPackets[2];
    /* 0x50 */ u32 unk50;
} WorldMapEntry; // Size: 0x54

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 pad14[0x484];
    /* 0x498 */ WorldMapEntry unk498;
    /* 0x4EC */ WorldMapEntry unk4EC;
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

// The world map frame render context g_WorldMapRenderContext points at: partial view of the
// two ordering table heads it fills each frame.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long *pOt[2];
} WorldMapRenderContext; // partial view

extern WorldMapActor *g_WorldMapSlots;
extern u16 g_WorldMapSlotStateIds;
extern u16 g_WorldMapSlotStateFrames[];
extern WorldMapState *g_WorldMapGroups;
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraHeight;
extern s32 g_WorldMapRotationMatrixValid;
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;
extern void func_80089160(s32, void *, s32);
extern void WorldMapSetupPrimTemplates(WorldMapEntry *, POLY_FT4 *, s32);
extern u8 *g_MenuSedsFile;
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
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 D_8009BBC4;
extern s16 g_WorldMapPathFrame;
extern s32 g_WorldMapScrollX;
extern s16 g_WorldMapSlotUpdateFlags;
extern s32 g_WorldMapCameraTarget;
extern s32 D_8009BBBC[];
extern WorldMapRenderContext *g_WorldMapRenderContext;
extern s32 g_WorldMapGridAngle;
extern WorldMapPos4 g_WorldMapPlayerPosition;
extern void WorldMapBuildRotationMatrix(void *p);
extern void func_80097244(void *p);
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
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX D_8009BE4C;
extern s32 g_WorldMapTpageAbr;
extern s32 D_8009D3CC;
extern s32 D_8009D804;
extern s32 g_WorldMapWrapPositionsCallback;
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(s32, s32, s32, s32);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern void ArchiveCdDataSync(s32);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapLoadClutAndTpageIds(void);
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
extern WorldMapPathPoint g_WorldMapCameraPathPoints[];
extern MATRIX g_WorldMapViewMatrix;
extern void WorldMapExtractRotationFromMatrix(MATRIX *, SVECTOR *);
extern void WorldMapInterpolateQuadraticVector(s32, s16 *, s16 *, s16 *, s32 *);
extern void WorldMapStartSlot(s32, s32);
extern void SoundFadeElementVolumeById(s32, s32, s32);
extern void SoundSetElementVolumeById(s32, s32);
extern s32 func_80094154(s32 *, s32 *);
extern s16 D_8009BD42;
extern s16 D_8009BD44;
extern s16 D_8009BD48;
extern s16 D_8009BD4A;
extern s16 D_8009BD4C;
extern s32 D_8009BD50;
extern s32 D_8009BD54;
extern s32 D_8009BD58;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 D_8009D554;
extern s32 g_WorldMapAreaMode;
extern s32 func_8007A1B4(s32);
extern s32 func_8007A430(s32);
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
    g_NextMapId = 0x10E;
    g_NextMapFlags = 0;
    D_8009BBC4 = 1;
    g_NextMapRotationY = v;
}
s32 WorldMapResetActorSlotWithSound(s32 index) {
    WorldMapActor *p;
    u8 *q;

    p = g_WorldMapSlots;
    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraHeight = 0x200000;
    q = g_MenuSedsFile;
    p = p + index;
    p->unk20 = 0;
    p->unk58 = 0;
    // do/while block makes cc1 schedule these stores after the actor cluster
    do { p->unk54 = 0; p->unk50 = 0; g_WorldMapViewRotationZ = 0; g_WorldMapViewRotationY = 0; } while (0);
    g_WorldMapCameraRot = 0;
    g_WorldMapRotationMatrixValid = 1;
    SoundPlaySoundEffectOnFreePair((((u16 *)q)[0xA] << 16) | 0xA4);
    p->unk22 = 0x18;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80077E68);
/* 94.17%, 1060 bytes = retail's exact size. Structure is fully decoded and every
   relocation matches; what is left is scheduler/allocator order in three places:
   (1) the entry puts the table base in $v1 and stores p->unk54 before the sll, retail
   keeps the base in $a0 and stores it between the sll and the addu; (2) case 0 keeps
   the p->unk20 increment after the g_WorldMapCameraTarget load where retail has it
   before, and the 0x1F8000A2 store lands after the shift, retail puts it between the
   two global loads; (3) the tail pairs the two sll/sw of each position quad, ours
   interleaves load/sll/sw per field. Three permuter runs from this body: PERMUTER_BETTER
   1950->1395->1319->920, and only two edits survived objdiff (the 'mode' copy before the
   switch and routing the third position quad's store through a temp, which measured
   worse here: 93.74). The copy block's 6+3 store order is retail's as written (x and z
   of each control point first, then the three y halfwords); the natural x,y,z order is
   91.53%. Note g_WorldMapPathFrame/D_8009BD42/D_8009BD44 are the SVECTOR at 0x8009BD40
   and D_8009BD48/4A/4C the one at 0x8009BD48; D_8009BD50/54/58 are the three words of
   the vector at 0x8009BD50. WorldMapInterpolateQuadraticVector takes the s32* out as
   its last (stack) argument, as misc5's caller does.
*/
/*
s32 func_80077E68(s32 index) {
    WorldMapActor *p;
    WorldMapPathPoint *pPoint;
    SVECTOR *pVec;
    s32 *pScratch;
    s32 grid;
    u16 mode;
    s32 dist;

    p = g_WorldMapSlots + index;
    pScratch = (s32 *)PSX_SCRATCH;
    grid = p->unk50 >> 12;
    pPoint = &g_WorldMapCameraPathPoints[grid];
    p->unk54 = grid;
    if (pPoint[2].unk6 != -1) {
        pVec = (SVECTOR *)((u8 *)pScratch + 0xA0);
        pVec[0].vx = pPoint[0].vx;
        pVec[0].vz = pPoint[0].vz;
        pVec[1].vx = pPoint[1].vx;
        pVec[1].vz = pPoint[1].vz;
        pVec[2].vx = pPoint[2].vx;
        pVec[2].vz = pPoint[2].vz;
        pVec[0].vy = pPoint[0].vy;
        pVec[1].vy = pPoint[1].vy;
        pVec[2].vy = pPoint[2].vy;
        WorldMapInterpolateQuadraticVector(p->unk50 & 0xFFF, (s16 *)pVec, (s16 *)&pVec[1],
                                           (s16 *)&pVec[2], pScratch);
        // the interpolated position becomes the path frame's vx/vy/vz (16.16 -> 12.4)
        D_8009BD4C = 0;
        D_8009BD48 = 0;
        D_8009BD58 = 0;
        D_8009BD50 = 0;
        D_8009BD54 = -0x1000;
        ((SVECTOR *)&g_WorldMapPathFrame)->vx = pScratch[0] >> 16;
        D_8009BD44 = -(pScratch[2] >> 16);
        D_8009BD42 = pScratch[1] >> 16;
        ((SVECTOR *)&D_8009BD48)->vy = D_8009BE2C >> 12;
    }
    func_80097244(&g_WorldMapPathFrame);
    WorldMapExtractRotationFromMatrix(&g_WorldMapViewMatrix, &g_WorldMapCameraRot);
    mode = p->unk20;
    switch (p->unk20) {
    case 0:
        p->unk58 = p->unk58 + 5;
        p->unk22 = p->unk22 - 1;
        if ((s16)p->unk22 >= 0) {
            break;
        }
        p->unk58 = 0x80;
        p->unk20 = mode + 1;
        pVec = (SVECTOR *)((u8 *)pScratch + 0xA0);
        pVec->vy = -0x20D;
        pVec->vx = (g_WorldMapCameraTarget >> 12) + 0x32;
        pVec->vz = (D_8009BE30 >> 12) - 0x70;
        func_80089160(8, pVec, 0);
        break;
    case 1:
        if (p->unk50 >= 0x6000) {
            p->unk20 = p->unk20 + 1;
        }
        break;
    case 2:
        p->unk58 = p->unk58 - 2;
        if (p->unk58 >= 0x19) {
            break;
        }
        p->unk58 = 0x18;
        p->unk20 = p->unk20 + 1;
        break;
    case 3:
        if (p->unk50 > 0x7FFF) {
            p->unk20 = p->unk20 + 1;
        }
        break;
    case 4:
        p->unk58 = p->unk58 - 1;
        if (p->unk58 >= 5) {
            break;
        }
        p->unk58 = 4;
        p->unk20 = p->unk20 + 1;
        break;
    case 5:
        if (p->unk50 > 0x83FF) {
            SoundFadeElementVolumeById((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0xA4, 0, 0x100);
            WorldMapStartSlot(0, 0xD);
            g_WorldMapTpageAbr = 2;
            D_8009D3CC = 4;
            p->unk22 = 0x50;
            p->unk58 = 0;
            p->unk20 = p->unk20 + 1;
        }
        break;
    case 6:
        p->unk22 = p->unk22 - 1;
        if ((s16)p->unk22 > 0) {
            break;
        }
        D_8009D554 = 0;
        g_WorldMapAreaMode = 0;
        break;
    }
    p->unk50 = p->unk50 + p->unk58;
    if ((s16)p->unk20 < 6) {
        pScratch[0] = D_8009BD48 << 12;
        pScratch[1] = D_8009BD4A << 12;
        pScratch[2] = D_8009BD4C << 12;
        pScratch[4] = g_WorldMapPathFrame << 12;
        pScratch[5] = D_8009BD42 << 12;
        pScratch[6] = D_8009BD44 << 12;
        dist = func_80094154(pScratch, pScratch + 4) >> 3;
        pScratch[8] = dist;
        SoundSetElementVolumeById((((u16 *)g_MenuSedsFile)[0xA] << 16) | 0xA4, 0x87 - dist);
    }
    return 1;
}
*/
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
    p = g_WorldMapSlots + index;
    p->unk20 = 0;
    *(WorldMapPos4 *)&p->unk28 = g_WorldMapPlayerPosition;
    pSlot = (s32 *)p;
    pSlot[0x15] = 0x40;
    pSlot[0x16] = 0x200;
    pSlot[0x17] = 0x60;
    pSlot[0x18] = 0x300;
    pSlot[0x14] = 0;
    pSlot[0x19] = 0x50;
    *(WorldMapPos4 *)&g_WorldMapCameraTarget = *(WorldMapPos4 *)&p->unk28;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_800783E8);
/* 91.53%, 1376 bytes = retail's exact size, instruction multiset identical.
   The entry table g_WorldMapGroups is a WorldMapEntry[] (stride 0x54): 0x11C/0x1C4/
   0x410/0x464 are entry 3/5/12/13's `matrix`, and the copies walk them down
   (13->9->11->7, 12->8->10->6, 5->4, 3->2->1). Two forms matter:
   - `pEntries[i] = *(MATRIX *)0x1F8001xx` gives gcc's 3+3+2 block move, while
     `pEntries[a].matrix = pEntries[b].matrix` gives retail's 4+4 one.
   - the global must be re-assigned per copy run (`pEntries = (WorldMapEntry *)
     g_WorldMapGroups;` four times) or gcc CSEs the load into one register instead of
     retail's four reloads.
   `D_8009BBBC[0] -= 0x4000` needs the array declaration (extern s32 D_8009BBBC[];)
   to make gcc put the address in a register instead of the $at macro form.
   Remaining delta: the four scratchpad rotation vectors. Retail clears
   0xB4/0xB0/0xAC/0xA8/0xA4/0xA0 and then emits the value stores in the order
   0xA2, 0xAA, 0xBA, 0xB2, 0xB8, 0xBC with each p->unkNN load left next to its
   store; we get the clears right (they need the do/while below, without it they
   move above the angle cluster) but gcc then hoists the three angle loads and
   the two remaining constant clears, so the last six stores come out in a
   different order (a=0xA0, b=0xA8, c=0xB0, d=0xB8 are the four SVECTORs).
   permute.sh from this exact body: PERMUTER_BETTER base=3195 best=2015 in 3 min,
   and the only edit it found that helped was routing two of the four
   `pEntries = (WorldMapEntry *)g_WorldMapGroups;` loads through a `WorldMapState *`
   temporary (permuter/func_800783E8/output-2015-1), i.e. this last stretch is a
   register-allocation question: run the permuter again from the improved body.
s32 func_800783E8(s32 index) {
    WorldMapActor *p;
    WorldMapEntry *pEntries;

    p = g_WorldMapSlots + index;
    p->unk30 -= 0x4000;
    D_8009BBBC[0] -= 0x4000;
    WorldMapWrapPosition(&p->unk28);
    *(u32 *)0x1F800000 = p->unk28 >> 12;
    *(u32 *)0x1F800004 = p->unk2C >> 12;
    *(u32 *)0x1F800008 = p->unk30 >> 12;
    ((WorldMapEntry *)g_WorldMapGroups)[0].pos = *(WorldMapPos4 *)0x1F800000;
    *(WorldMapPos4 *)&g_WorldMapCameraTarget = *(WorldMapPos4 *)&p->unk28;
    p->unk50 = (p->unk50 + p->unk54) & 0xFFF;
    p->unk58 = (p->unk58 + p->unk5C) & 0xFFF;
    p->unk60 = (p->unk60 + p->unk64) & 0xFFF;
    // do/while keeps cc1 from hoisting the clears above the angle cluster
    do {
        *(s16 *)0x1F8000B4 = 0;
        *(s16 *)0x1F8000B0 = 0;
        *(s16 *)0x1F8000AC = 0;
        *(s16 *)0x1F8000A8 = 0;
        *(s16 *)0x1F8000A4 = 0;
        *(s16 *)0x1F8000A0 = 0;
    } while (0);
    *(s16 *)0x1F8000A2 = p->unk50;
    *(s16 *)0x1F8000AA = p->unk58;
    *(s16 *)0x1F8000BA = 0;
    *(s16 *)0x1F8000B2 = p->unk60;
    *(s16 *)0x1F8000B8 = 0;
    *(s16 *)0x1F8000BC = p->unk60;
    RotMatrixYXZ((SVECTOR *)0x1F8000A0, (MATRIX *)0x1F8000F0);
    RotMatrixYXZ((SVECTOR *)0x1F8000A8, (MATRIX *)0x1F800110);
    RotMatrixYXZ((SVECTOR *)0x1F8000B0, (MATRIX *)0x1F800130);
    RotMatrixYXZ((SVECTOR *)0x1F8000B8, (MATRIX *)0x1F800150);
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    pEntries[3].matrix = *(MATRIX *)0x1F800150;
    pEntries[2].matrix = pEntries[3].matrix;
    pEntries[1].matrix = pEntries[2].matrix;
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    pEntries[5].matrix = *(MATRIX *)0x1F800130;
    pEntries[4].matrix = pEntries[5].matrix;
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    pEntries[12].matrix = *(MATRIX *)0x1F8000F0;
    pEntries[8].matrix = pEntries[12].matrix;
    pEntries[10].matrix = pEntries[8].matrix;
    pEntries[6].matrix = pEntries[10].matrix;
    pEntries = (WorldMapEntry *)g_WorldMapGroups;
    pEntries[13].matrix = *(MATRIX *)0x1F800110;
    pEntries[9].matrix = pEntries[13].matrix;
    pEntries[11].matrix = pEntries[9].matrix;
    pEntries[7].matrix = pEntries[11].matrix;
    return 1;
} */

s32 WorldMapEnterNop(void) {
    return 1;
}

s32 WorldMapUpdateFrameWithParticles(void) {
    s32 *pState;

    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapBuildRotationMatrix(&g_WorldMapPathFrame);
    } else {
        func_80097244(&g_WorldMapPathFrame);
    }
    func_80089748();
    func_80089C78();
    func_800848F4();
    func_800980D4(&g_WorldMapScrollX);
    if (g_WorldMapSlotUpdateFlags != 0) {
        func_800981C8(&g_WorldMapCameraTarget);
        WorldMapWaitForLoadSlotFree();
        func_80098CC0();
    }
    pState = &g_WorldMapCameraTarget;
    func_800983A0(pState);
    func_8009932C(g_WorldMapRenderContext->pOt[0], g_WorldMapRenderContext->pOt[1], pState);
    g_WorldMapGridAngle = g_WorldMapGridAngle + 0x40;
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
    D_8009BE4C = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    D_8009D3CC = 0x10;
    D_8009D804 = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = (s32)WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    g_WorldMapPlayerPosition.x = 0x2000000;
    g_WorldMapPlayerPositionY = -0x300000;
    g_WorldMapPlayerPositionZ = 0x2000000;
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
    func_80097BC0(&g_WorldMapPlayerPosition);
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
    g_NextMapId = 0x110;
    g_NextMapFlags = 0;
    D_8009BBC4 = 1;
    g_NextMapRotationY = v;
}

s32 WorldMapInitActorSlotTimer40(s32 index) {
    WorldMapActor *p;
    s32 scroll;
    s32 timer;

    p = g_WorldMapSlots;
    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraHeight = 0x200000;
    g_WorldMapRotationMatrixValid = 0;
    p = p + index;
    scroll = -0xC0;
    p->unk20 = 0x10;
    p->unk58 = 0;
    // the do/while blocks decide the store order and where the 0x40 lands
    do {
        p->unk54 = 0;
        p->unk50 = 0;
        g_WorldMapCameraRot = scroll;
        do { g_WorldMapViewRotationY = 0x680; } while (0);
        timer = 0x40;
        g_WorldMapViewRotationZ = 0;
    } while (0);
    p->unk22 = timer;
    p->unk74 = 0;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80078EA4);
s32 WorldMapInitActorRotationMatrix(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;

    p = g_WorldMapSlots + index;
    s = g_WorldMapGroups;
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

    p = g_WorldMapSlots + index;
    if (p->unk4 != 0) {
        p->unk4 = 0;
        g_WorldMapGroups->unk540 = 1;
    }
    WorldMapWrapPosition(&p->unk28);
    p->unk2C = func_80093A5C(p->unk28, p->unk30) + 0x18000;
    g_WorldMapGroups->unk548 = p->unk28 >> 12;
    g_WorldMapGroups->unk54C = p->unk2C >> 12;
    g_WorldMapGroups->unk550 = p->unk30 >> 12;
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
    p = g_WorldMapSlots + index;
    p->unk20 = 0;
    *(WorldMapPos4 *)&p->unk28 = g_WorldMapPlayerPosition;
    pSlot = (s32 *)p;
    pSlot[0x15] = 0x40;
    pSlot[0x16] = 0x200;
    pSlot[0x17] = 0x60;
    pSlot[0x18] = 0x300;
    pSlot[0x14] = 0;
    pSlot[0x19] = 0x50;
    *(WorldMapPos4 *)&g_WorldMapCameraTarget = *(WorldMapPos4 *)&p->unk28;
    s = g_WorldMapGroups;
    q = g_MenuSedsFile;
    ((s16 *)s)[0xC] = -0x100;
    ((s16 *)s)[0xD] = 0;
    ((s16 *)s)[0xE] = -0x40;
    SoundPlaySoundEffectOnFreePair((q[0xA] << 16) | 0x36);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_80079778);
void WorldMapSetupPrimTemplates(WorldMapEntry *pEntry, POLY_FT4 *pPrim, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        setlen(pPrim, 9);
        pPrim->code = 0x2C;
        pPrim->tpage = GetTPage(1, 3, 0x340, 0x100);
        pPrim->clut = GetClut(0x100, 0x1FF);
        pPrim->r0 = 0x80;
        pPrim->g0 = 0x80;
        setSemiTrans(pPrim, 1);
        pPrim->b0 = 0x80; // last store sets gcc's induction base to 0x6 as retail's does
        pPrim++;
    }
    memcpy(pEntry->pPackets[1], pEntry->pPackets[0], count * sizeof(POLY_FT4));
}

s32 WorldMapSetupGroup14And15Prims(s32 index) {
    WorldMapActor *p;

    p = g_WorldMapSlots + index;
    p->unk20 = 0;
    p->unk70 = 0;
    p->unk6C = 0;
    WorldMapSetupPrimTemplates(&g_WorldMapGroups->unk498, g_WorldMapGroups->unk498.pPackets[0],
                  g_WorldMapGroups->unk498.pModel->polyCount);
    WorldMapSetupPrimTemplates(&g_WorldMapGroups->unk4EC, g_WorldMapGroups->unk4EC.pPackets[0],
                  g_WorldMapGroups->unk4EC.pModel->polyCount);
    return 3;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A1B4);
/* 99.90%, 604 bytes = retail's exact size. The whole residual is that retail
   loads p->unk6C into $v0 and stores it twice, while this body's single
   `scale` variable is $v1 for both values. Cause: local-alloc's combine_regs
   merges the two non-overlapping webs of the one variable, and their union
   contains the `li $v0,0x1000` plus its two stores, so $v0 is not free and
   both webs get $v1. Two separate locals break the merge and do give retail's
   $v0 then $v1, but then the p->unk70 load is moved up to just after the
   p->unk6C load (into the dead $a2): 97.16%. do/while containers (around the
   assignment, the stores, the whole pair) move that hoist rather than remove
   it, 94.5-97.2%, and drag the pMtxA address formation along. Volatile on one
   scratch store: 95.12%, 608 bytes. Plain memory stores instead of the local:
   cse reuses the 0x6000-test load and p leaves $a2, 95.5%. Embedded
   assignment for the 0x70 store: 97.16%. permute.sh from THIS body (third
   run; the first two ran against a body whose extern arity was wrong):
   PERMUTER_NONE base=15 best=15, 1219 iterations.
/*s32 func_8007A1B4(s32 index) {
    WorldMapEntry *pEntry;
    MATRIX *pMtxA;
    MATRIX *pMtxB;
    WorldMapActor *p;
    s32 result;
    s32 scale;

    pEntry = (WorldMapEntry *)g_WorldMapGroups;
    p = g_WorldMapSlots + index;
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
    *pMtxA = g_WorldMapIdentityMatrix;
    pMtxB = (MATRIX *)0x1F800110;
    *pMtxB = *pMtxA;
    scale = p->unk6C;
    *(u32 *)0x1F800008 = scale;
    *(u32 *)0x1F800000 = scale;
    *(u32 *)0x1F800014 = 0x1000;
    *(u32 *)0x1F800004 = 0x1000;
    scale = p->unk70;
    *(u32 *)0x1F800018 = scale;
    *(u32 *)0x1F800010 = scale;
    ScaleMatrix(pMtxA, (VECTOR *)0x1F800000);
    ScaleMatrix(pMtxB, (VECTOR *)0x1F800010);
    pEntry[14].matrix = *pMtxA;
    pEntry[15].matrix = *pMtxB;
    return result;
}
*/

s32 WorldMapSetActorTimer60(s32 index) {
    WorldMapActor *p;

    p = g_WorldMapSlots + index;
    p->unk22 = 0x60;
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc4", func_8007A430);
/* 83.41% (retail 296 bytes, ours 300): structure right - same branch, same call,
   p in $a2 and the 0x20000 constant in $a1 as retail. The absolute scratch
   stores must be the literal `*(s16 *)((u8 *)PSX_SCRATCH + 0xA0)` form (a
   `s16 *pDst` local instead builds the address early into $a1, 77.29%), and
   wrapping the `p->unk30 += 0x20000` in a do/while is what keeps p in $a2 -
   without it p moves to $a3 (74.40%). Remaining delta: retail splits the
   call's address constant (`lui $a1,0x1F80` early, `ori` after the 0xA0
   store), reloads p->unk30 after the 0xA2 store, and has `li $a0,9` in the
   blez delay slot where ours has the 0x20000 lui. permute.sh from this body:
   PERMUTER_BETTER base=730 best=616, but that candidate (a pointer-to-member
   local feeding the 0xA4 store) measured 83.06% in objdiff, i.e. the
   permuter's metric and objdiff disagree here - porting it was a regression.
s32 func_8007A430(s32 index) {
    WorldMapActor *p;
    WorldMapState *s;
    WorldMapPos4 *pSrc;
    s32 result = 1;

    p = g_WorldMapSlots + index;
    if (p->unk4 == 1) {
        s = g_WorldMapGroups;
        p->unk4 = 0;
        p->unk28 = s->unk8 << 12;
        p->unk2C = s->unkC << 12;
        p->unk30 = s->unk10 << 12;
    }
    p->unk22 = p->unk22 - 1;
    if ((s16)p->unk22 > 0) {
        // the do/while keeps the 0x20000 constant in $a1 and p in $a2, as retail
        do { p->unk30 += 0x20000; } while (0);
        *(s16 *)((u8 *)PSX_SCRATCH + 0xA0) = p->unk28 >> 12;
        *(s16 *)((u8 *)PSX_SCRATCH + 0xA2) = p->unk2C >> 12;
        *(s16 *)((u8 *)PSX_SCRATCH + 0xA4) = p->unk30 >> 12;
        func_80089160(9, 0x1F8000A0, 0);
    } else {
        p->unk22 = 0x60;
        pSrc = &g_WorldMapPlayerPosition;
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

    p = g_WorldMapSlots + index;
    s = g_WorldMapGroups;
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

    p = g_WorldMapSlots + index;
    s = g_WorldMapGroups;
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
    D_8009BE4C = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    D_8009D3CC = 0x10;
    D_8009D804 = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = (s32)WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    g_WorldMapPlayerPosition.x = 0x5BED000;
    g_WorldMapPlayerPositionY = -0xA0000;
    g_WorldMapPlayerPositionZ = 0x62A8000;
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
    func_80097BC0(&g_WorldMapPlayerPosition);
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
    g_NextMapId = 0x11A;
    g_NextMapFlags = 0;
    D_8009BBC4 = 1;
    g_NextMapRotationY = v;
}
s32 WorldMapStartActorScript(s32 index) {
    WorldMapActor *p;

    p = g_WorldMapSlots + index;
    p->unk50 = 0;
    p->unk20 = g_WorldMapSlotStateIds;
    p->unk22 = g_WorldMapSlotStateFrames[p->unk50];
    return 1;
}
