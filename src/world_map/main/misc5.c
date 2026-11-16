#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x4];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ u8 unk34[0x4];
    /* 0x38 */ u32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ u8 unk44[0xC];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ u8 unk60[0x20];
} WorldMapPlayer; // Size: 0x80

// sound-effect bank header: +0x14 is the bank id sound commands are played from
typedef struct {
    /* 0x00 */ u8 unk0[0x14];
    /* 0x14 */ u16 unk14;
} SoundSedsBank; // Size: unknown, only +0x14 is read

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
} WorldMapTrackPoint; // Size: 0x8

typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 w;
} WorldMapPos; // Size: 0x10

// 0x54-byte scene-layer entry. g_WorldMapGroups is an array of them: the group 4..11
// layers sit at indexes 4..11 (file offsets 0x150..0x39C), layer 0 at offset 0.
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 unk2[0x6];
    /* 0x08 */ WorldMapPos pos;
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ u8 *unk40; // header whose +0x4 is the prim count
    /* 0x44 */ u8 unk44[0x4];
    /* 0x48 */ u8 *unk48; // vertex buffer
    /* 0x4C */ u8 *unk4C; // copy destination for unk48
    /* 0x50 */ u8 unk50[0x4];
} WorldMapSceneLayer; // Size: 0x54

extern WorldMapPlayer *g_WorldMapSlots;
extern WorldMapSceneLayer *g_WorldMapGroups;
extern u16 g_WorldMapSlotStateIds[];
extern u16 g_WorldMapSlotStateFrames[];
extern WorldMapTrackPoint D_8009A490[];
extern u16 g_WorldMapActorParamStateIds;
extern u16 g_WorldMapActorParamStateFrames[];
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 D_8009BD3C;
// camera position / rotation halfwords, accumulated by the random jitter below
extern u16 g_WorldMapPathFrame;
extern u16 D_8009BD42;
extern u16 D_8009BD44;
extern u16 D_8009BD48;
extern u16 D_8009BD4A;
extern u16 D_8009BD4C;
extern MATRIX g_WorldMapViewMatrix;
extern void func_80096F18(void *, void *, s32, void *);
extern void func_80097244(s32 *pPos);
extern int rand(void);
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapCameraTarget;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 g_WorldMapPlayerPosition;
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s32 g_WorldMapCameraHeight;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;
extern s32 g_WorldMapRotationMatrixValid;
extern SoundSedsBank *g_MenuSedsFile;
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 D_8009BBC4;
extern void WorldMapSetSpriteSetParent(int, int);
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
extern void WorldMapWrapPosition(void *p);
extern s32 func_80093A5C(s32, s32);
extern void WorldMapDeactivateSpriteGroup(s32);
extern void func_80089160(s32, void *, s32);
extern void WorldMapStartSlot(s32, s32);
extern void WorldMapInterpolateQuadraticVector(s32, s16 *, s16 *, s16 *, s32 *);
extern void WorldMapExtractRotationFromMatrix(MATRIX *, SVECTOR *);
extern void SoundPlaySoundEffectOnFreePair(s32);
extern void SoundSendCommandConstant(s32);
extern s16 g_WorldMapSpriteGroup9Rotation[];
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX D_8009BE4C;
extern s32 g_WorldMapTpageAbr;
extern s32 D_8009D3CC;
extern s32 D_8009D554;
extern s32 g_WorldMapAreaMode;
extern s32 D_8009D804;
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern s32 ArchiveCdDataSync(s32 arg0);
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
extern void SoundAddSedsEntry(void *pSeds);
extern s32 ArchiveSetIndex(s32 arg0, s32 arg1);
extern void func_80097BC0(void *pPos);
extern s32 WorldMapGetQueuedLoadCount(void);
extern void WorldMapUpdateLoadQueue(void);
extern s32 Vsync(s32 arg0);
extern void WorldMapAddSlot(void *p0, void *p1);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void WorldMapInitRandomTimers(void);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern int WorldMapInitActorParamsFromTable(int idx);
extern int func_8007C3B8(int idx);
extern int func_8007C724(int idx);
extern int func_8007C7D8(int idx);
extern int WorldMapInitGroup4Actor(int idx);
extern int WorldMapUpdateGroup4Actor(int idx);
extern int WorldMapInitGroup5Actor(int idx);
extern int func_8007CF18(int idx);
extern int WorldMapInitGroup9Actor(int idx);
extern int WorldMapUpdateGroup9Actor(int idx);
extern int WorldMapInitGroup10Actor(int idx);
extern int WorldMapUpdateGroup10Actor(int idx);
extern void WorldMapInitGroup12Actor(void);
extern void WorldMapUpdateGroup12Actor(void);
extern void WorldMapInitGroup13Actor(void);
extern void WorldMapUpdateGroup13Actor(void);
extern void WorldMapInitGroup16Actor(void);
extern void WorldMapUpdateGroup16Actor(void);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern void WorldMapSetupPrimTemplates(void *pBlock, s32 arg, s32 arg2);

s32 WorldMapUpdateActorState(s32 idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;

    switch (pPlayer->unk20) {
    case 0:
        break;
    case 1:
        if ((s16)--pPlayer->unk22 < 0) {
            pPlayer->unk20 = g_WorldMapSlotStateIds[pPlayer->unk50];
            pPlayer->unk22 = g_WorldMapSlotStateFrames[pPlayer->unk50];
            pPlayer->unk50++;
        }
        break;
    case 2:
        func_80089160(0x11, NULL, 0);
        pPlayer->unk20 = 1;
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->unk14 << 16) | 1);
        break;
    case 3:
        WorldMapStartSlot(2, 2);
        pPlayer->unk20 = 1;
        func_80089160(0xF, NULL, 0);
        func_80089160(0x10, NULL, 0);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->unk14 << 16) | 4);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->unk14 << 16) | 5);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->unk14 << 16) | 6);
        break;
    case 4:
        WorldMapStartSlot(2, 3);
        WorldMapStartSlot(3, 1);
        pPlayer->unk20 = 1;
        break;
    case 5:
        WorldMapStartSlot(2, 6);
        WorldMapStartSlot(4, 1);
        WorldMapStartSlot(5, 1);
        pPlayer->unk20 = 1;
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->unk14 << 16) | 7);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->unk14 << 16) | 8);
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->unk14 << 16) | 9);
        break;
    case 6:
        WorldMapStartSlot(2, 4);
        pPlayer->unk20 = 1;
        SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->unk14 << 16) | 0xA);
        SoundSendCommandConstant((g_MenuSedsFile->unk14 << 16) | 0xB);
        SoundSendCommandConstant((g_MenuSedsFile->unk14 << 16) | 0xC);
        break;
    case 7:
        WorldMapStartSlot(2, 5);
        pPlayer->unk20 = 1;
        break;
    case 8:
        WorldMapStartSlot(2, 1);
        pPlayer->unk20 = 1;
        break;
    case 9:
        WorldMapStartSlot(6, 1);
        WorldMapStartSlot(2, 7);
        pPlayer->unk20 = 1;
        break;
    case 10:
        WorldMapStartSlot(0, 0xD);
        D_8009D3CC = 4;
        pPlayer->unk20 = 1;
        break;
    case 11:
        D_8009D554 = 0;
        g_WorldMapAreaMode = 0;
        pPlayer->unk20 = 1;
        break;
    }
    return 1;
}

int WorldMapInitActorScrollScale(int idx) {
    WorldMapPlayer *pPlayer;
    s32 a;
    s32 b;
    s32 c;

    a = g_WorldMapPlayerPosition;
    b = D_8009C5B0;
    c = D_8009C5B4;
    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraHeight = 0x1C0000;
    g_WorldMapCameraRot = 0x40;
    g_WorldMapViewRotationY = 0x480;
    D_8009BD3C = 0;
    pPlayer = g_WorldMapSlots + idx;
    g_WorldMapCameraTarget = a;
    g_WorldMapScrollTargetX = a;
    D_8009BE2C = b;
    g_WorldMapScrollTargetY = b;
    D_8009BE30 = c;
    g_WorldMapScrollTargetZ = c;
    // volatile store pins the sw after the six copies, as retail has it
    *(volatile s32 *)&pPlayer->unk50 = 0x1000;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007ADD4);
/* PARKED 87.56% (1064 bytes / 266 insns; retail 1068 / 267). Two switches plus a
   shared rand-jitter tail; every block is found and the ONE remaining defect is
   the 4th argument of the tail call.
   NEEDS these externs (the parked body below is spelled for them; the scalar
   D_8009BD3x/4x and g_WorldMapCameraTarget/2C/30 declarations give the same 87.56%):
       extern SVECTOR g_WorldMapCameraRot[];   // rotation, offset, target: 3 SVECTORs
       extern SVECTOR g_WorldMapPathFrame[];   // offset + target, same bytes
       extern WorldMapPos g_WorldMapCameraTarget[];
   Switching them makes the two already-matched neighbours read objdiff 99.00 /
   99.85 (MATCH, identical bytes: only reloc addends move), so only do it with
   this body in place.
   SOLVED (was 81.35%): the three read-modify-write globals retail addresses
   through a HELD register - case 4 D_8009BE30 in $a2, case 5 g_WorldMapViewRotationY in $a3,
   tail g_WorldMapPathFrame in $t1 - are UNBOUNDED ARRAY declarations. That is what makes
   gcc keep the base in a general register instead of the `sw $r,sym` macro form
   that maspsx expands through $at. And only the FIRST RMW of a basic block does
   it, which is exactly retail's tail (BD40 in $t1, then BD48/BD42/BD4A via $at).
   The held base only appears when the RMW is at offset 0 of its array symbol:
   g_WorldMapCameraRot[1].vx += x emits the $at macro form, g_WorldMapPathFrame[0].vx += x the
   held base. That is why g_WorldMapPathFrame needs its own declaration.
   REMAINING 4 BYTES, one decision: retail materialises &g_WorldMapPathFrame fresh in $a0
   for func_80096F18 and AGAIN in $t1 after the two rand() calls; with arg1 and
   the tail RMW both spelled &g_WorldMapPathFrame gcc hoists the address into the SAVED
   register $s0 before the calls (which also pushes the player pointer to $s1 and
   the scratch base to $s2, adding a save/restore pair): 1072 bytes, 85.93%.
   Spelling arg1 as &g_WorldMapCameraRot[1] kills the hoist (that is this 87.56% body) but
   gcc then derives the 4th argument as `addu $a3,$a0,-8` where retail emits a
   fresh `la`, so the function comes out 4 bytes SHORT. All three roles need
   values gcc cannot relate: arg1 g_WorldMapPathFrame, arg4 g_WorldMapCameraRot and the tail base
   g_WorldMapPathFrame -i.e. two spellings of the same address that differ where it
   matters. Tried: arg1 &D_8009BD48[-1] (the hoist returns, 1072), arg4
   &g_WorldMapPathFrame[-1] (81.91%, 1048), the whole tail in a do/while (0) block (80.44%,
   1072), arg4 through a pointer local. Permuter from this body twice, both
   PERMUTER_BETTER and both SHORTER than retail, so neither can link:
   `s32 nTargetZ = pPlayer->unk5C;` before the BD40[1] stores (92.26%, 1060) and
   `nAngle = g_WorldMapPlayerPosition;` between case 3 and case 4 with unk58 = nAngle + 0xC7C00
   (93.93%, 1052). The unused return value is not the lever here (tried void).
   Compiler is not the preset: -G0 85.93, gcc-2.6.0 85.89, cdk -G0 66.51.
   Do not convert func_8007BBEC/8007C3B8/8007C7D8 before this one: their tables
   must follow its two (jtbl_8006FBF4 at .rodata+0x30, jtbl_8006FC14 at +0x50,
   with 4 pad bytes between them from the align before the second table).
    int func_8007ADD4(s32 idx) {
        WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
        u16 *pJitter = (u16 *)PSX_SCRATCH;
        s32 nAngle;
        s16 nState;
        s16 nTimer;

        nState = pPlayer->unk4 - 1;
        switch (nState) {
        case 0:
            pPlayer->unk4 = 0;
            pPlayer->unk20 = 1;
            break;
        case 1:
            pPlayer->unk20 = 2;
            pPlayer->unk4 = 0;
            pPlayer->unk50 = 0x40000;
            break;
        case 2:
            g_WorldMapCameraHeight = 0x640000;
            pPlayer->unk4 = 0;
            pPlayer->unk20 = 0;
            g_WorldMapCameraRot[0].vx = -0x1E0;
            g_WorldMapCameraRot[0].vy = 0x480;
            g_WorldMapCameraRot[0].vz = 0;
            break;
        case 3:
            pPlayer->unk4 = 0;
            pPlayer->unk20 = 0;
            g_WorldMapCameraHeight = 0x1E0000;
            g_WorldMapCameraRot[0].vx = 0x40;
            g_WorldMapCameraRot[0].vy = 0x418;
            g_WorldMapCameraRot[0].vz = 0;
            g_WorldMapRotationMatrixValid = 0;
            g_WorldMapCameraTarget[0].z = D_8009C5B4;
            break;
        case 4:
            pPlayer->unk4 = 0;
            pPlayer->unk20 = 3;
            break;
        case 5:
            pPlayer->unk4 = 0;
            pPlayer->unk20 = 4;
            pPlayer->unk58 = g_WorldMapPlayerPosition + 0xC7C00;
            g_WorldMapRotationMatrixValid = 1;
            pPlayer->unk5C = D_8009C5B4 + 0x3EC400;
            break;
        case 6:
            pPlayer->unk4 = 0;
            pPlayer->unk20 = 5;
            break;
        }
        switch (pPlayer->unk20) {
        case 0:
            break;
        case 1:
            pPlayer->unk50 += 0x200;
            if (pPlayer->unk50 > 0x8000) {
                pPlayer->unk50 = 0x8000;
                pPlayer->unk20 = 0;
            }
            break;
        case 2:
            pPlayer->unk50 -= 0x200;
            if (pPlayer->unk50 < 0x8000) {
                pPlayer->unk50 = 0x8000;
                pPlayer->unk20 = 0;
            }
            break;
        case 3:
            pPlayer->unk50 -= 0x100;
            if (pPlayer->unk50 < 0x1000) {
                pPlayer->unk50 = 0x1000;
                pPlayer->unk20 = 0;
            }
            break;
        case 4:
            g_WorldMapCameraTarget[0].z += 0x3A000;
            if (g_WorldMapCameraTarget[0].z > 0x77FFFFF) {
                pPlayer->unk20 = 0;
            }
            g_WorldMapPathFrame[0].vx = (pPlayer->unk58 - g_WorldMapCameraTarget[0].x) >> 12;
            g_WorldMapPathFrame[1].vz = 0;
            g_WorldMapPathFrame[1].vx = 0;
            nAngle = g_WorldMapCameraTarget[0].y >> 12;
            g_WorldMapPathFrame[1].vy = nAngle;
            g_WorldMapPathFrame[0].vy = nAngle - 0x40;
            g_WorldMapPathFrame[0].vz = (g_WorldMapCameraTarget[0].z - pPlayer->unk5C) >> 12;
            func_80097244((s32 *)&g_WorldMapPathFrame[0]);
            WorldMapExtractRotationFromMatrix(&g_WorldMapViewMatrix, &g_WorldMapCameraRot[0]);
            break;
        case 5:
            g_WorldMapCameraRot[0].vy += 4;
            nTimer = g_WorldMapCameraRot[0].vy;
            if (nTimer > 0x600) {
                pPlayer->unk20 = 0;
            }
            break;
        }
        func_80096F18(&g_WorldMapCameraRot[1], &g_WorldMapCameraTarget[0], g_WorldMapCameraHeight, &g_WorldMapCameraRot[0]);
        pJitter[0x50] = rand() % (pPlayer->unk50 >> 12) - (pPlayer->unk50 >> 13);
        pJitter[0x51] = rand() % (pPlayer->unk50 >> 12) - (pPlayer->unk50 >> 13);
        g_WorldMapPathFrame[0].vx += pJitter[0x50];
        g_WorldMapPathFrame[1].vx += pJitter[0x50];
        g_WorldMapPathFrame[0].vy += pJitter[0x51];
        g_WorldMapPathFrame[1].vy += pJitter[0x51];
        return 1;
    }
*/

int WorldMapSetupGroup4And5ScaledPrims(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pBlock = &g_WorldMapGroups[4];
    WorldMapSceneLayer *pLayer = &g_WorldMapGroups[4];
    u32 *pScratch = (u32 *)PSX_SCRATCH;
    s32 scale;
    s32 i;

    for (i = 0; i < 2; i++) {
        WorldMapSetupPrimTemplates(pBlock, (s32)pLayer->unk48,
                                   ((u16 *)pLayer->unk40)[2]);
        pLayer++;
        pBlock++;
    }
    pPlayer->unk54 = 0;
    pPlayer->unk50 = 0;
    *(MATRIX *)&pScratch[0x3C] = g_WorldMapIdentityMatrix;
    scale = pPlayer->unk50;
    pScratch[1] = 0x1000;
    pScratch[2] = scale;
    pScratch[0] = scale;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    pBlock->mat = *(MATRIX *)&pScratch[0x3C];
    pBlock[1].mat = *(MATRIX *)&pScratch[0x3C];
    pBlock[1].unk0 = 1;
    pBlock->unk0 = 1;
    return 3;
}

int WorldMapUpdateGroup4And5Scale(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pLayer = g_WorldMapGroups;
    // 0x158/0x15C/0x160 are layer 4's pos, 0x1AC/0x1B0/0x1B4 layer 5's; the six
    // pos stores stay untyped because the typed form lets gcc hoist the -0x40
    // pair above the g_WorldMapPlayerPosition load, which retail does not
    u8 *pWork = (u8 *)pLayer;
    u32 *pScratch = (u32 *)PSX_SCRATCH;
    // second layer's scale; retail keeps this field's address in a register,
    // which is what makes the player pointer land in $a2
    s32 *pScale2 = &pPlayer->unk54;
    s32 x;
    s32 y;
    s32 z;
    s32 w;

    if (pPlayer->unk4 != 0) {
        pPlayer->unk4 = 0;
        pLayer[5].unk0 = 0;
        pLayer[4].unk0 = 0;
    }
    *(s32 *)(pWork + 0x1AC) = g_WorldMapPlayerPosition >> 12;
    *(s32 *)(pWork + 0x158) = g_WorldMapPlayerPosition >> 12;
    y = -0x40;
    *(s32 *)(pWork + 0x1B0) = y;
    *(s32 *)(pWork + 0x15C) = y;
    z = D_8009C5B4 >> 12;
    *(s32 *)(pWork + 0x1B4) = z;
    *(s32 *)(pWork + 0x160) = z;
    pPlayer->unk50 += 0x180;
    if (pPlayer->unk50 > 0x800) {
        *pScale2 += 0x180;
    }
    if (pPlayer->unk50 > 0x7F00) {
        pPlayer->unk50 = 0x7F00;
    }
    if (*pScale2 > 0x7F00) {
        *pScale2 = 0x7F00;
    }
    *(MATRIX *)&pScratch[0x3C] = g_WorldMapIdentityMatrix;
    *(MATRIX *)&pScratch[0x44] = *(MATRIX *)&pScratch[0x3C];
    w = pPlayer->unk50;
    pScratch[2] = w;
    pScratch[0] = w;
    x = *pScale2;
    pScratch[5] = 0x1000;
    pScratch[1] = 0x1000;
    pScratch[6] = x;
    pScratch[4] = x;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    ScaleMatrix((MATRIX *)&pScratch[0x44], (VECTOR *)&pScratch[4]);
    pLayer[4].mat = *(MATRIX *)&pScratch[0x3C];
    pLayer[5].mat = *(MATRIX *)&pScratch[0x44];
    return 1;
}
int WorldMapSetupGroup6And7ScaledPrims(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pBlock = &g_WorldMapGroups[6];
    WorldMapSceneLayer *pLayer = &g_WorldMapGroups[6];
    u32 *pScratch = (u32 *)PSX_SCRATCH;
    s32 scale;
    s32 i;

    for (i = 0; i < 2; i++) {
        WorldMapSetupPrimTemplates(pBlock, (s32)pLayer->unk48,
                                   ((u16 *)pLayer->unk40)[2]);
        pLayer++;
        pBlock++;
    }
    pPlayer->unk54 = 0;
    pPlayer->unk50 = 0;
    *(MATRIX *)&pScratch[0x3C] = g_WorldMapIdentityMatrix;
    scale = pPlayer->unk50;
    pScratch[1] = 0x1000;
    pScratch[2] = scale;
    pScratch[0] = scale;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    pBlock->mat = *(MATRIX *)&pScratch[0x3C];
    pBlock[1].mat = *(MATRIX *)&pScratch[0x3C];
    pBlock[1].unk0 = 1;
    pBlock->unk0 = 1;
    return 3;
}

int WorldMapUpdateGroup6And7Scale(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pLayer = g_WorldMapGroups;
    // 0x200/0x204/0x208 are layer 6's pos, 0x254/0x258/0x25C layer 7's; untyped
    // for the reason given in WorldMapUpdateGroup4And5Scale
    u8 *pWork = (u8 *)pLayer;
    u32 *pScratch = (u32 *)PSX_SCRATCH;
    // second layer's scale; retail keeps this field's address in a register,
    // which is what makes the player pointer land in $a2
    s32 *pScale2 = &pPlayer->unk54;
    s32 x;
    s32 y;
    s32 z;
    s32 w;

    if (pPlayer->unk4 != 0) {
        pPlayer->unk4 = 0;
        pLayer[7].unk0 = 0;
        pLayer[6].unk0 = 0;
    }
    *(s32 *)(pWork + 0x254) = g_WorldMapPlayerPosition >> 12;
    *(s32 *)(pWork + 0x200) = g_WorldMapPlayerPosition >> 12;
    y = -0x40;
    *(s32 *)(pWork + 0x258) = y;
    *(s32 *)(pWork + 0x204) = y;
    z = D_8009C5B4 >> 12;
    *(s32 *)(pWork + 0x25C) = z;
    *(s32 *)(pWork + 0x208) = z;
    pPlayer->unk50 += 0x180;
    if (pPlayer->unk50 > 0x800) {
        *pScale2 += 0x180;
    }
    if (pPlayer->unk50 > 0x7F00) {
        pPlayer->unk50 = 0x7F00;
    }
    if (*pScale2 > 0x7F00) {
        *pScale2 = 0x7F00;
    }
    *(MATRIX *)&pScratch[0x3C] = g_WorldMapIdentityMatrix;
    *(MATRIX *)&pScratch[0x44] = *(MATRIX *)&pScratch[0x3C];
    w = pPlayer->unk50;
    pScratch[2] = w;
    pScratch[0] = w;
    x = *pScale2;
    pScratch[5] = 0x1000;
    pScratch[1] = 0x1000;
    pScratch[6] = x;
    pScratch[4] = x;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    ScaleMatrix((MATRIX *)&pScratch[0x44], (VECTOR *)&pScratch[4]);
    pLayer[6].mat = *(MATRIX *)&pScratch[0x3C];
    pLayer[7].mat = *(MATRIX *)&pScratch[0x44];
    return 1;
}

int WorldMapInitReturnThree(void) {
    return 3;
}

int WorldMapActivateSpriteGroup9Timed(int idx) {
    // byte view of the player slot: through a typed WorldMapPlayer * gcc lifts the
    // three position words above the stores and settles on $a3 for the slot
    // (73.27%, 324 bytes) where retail interleaves load/store on $a2
    u8 *pPlayer = (u8 *)g_WorldMapSlots + (idx << 7);
    s16 timer;
    int nResult = 1;

    if (*(s16 *)(pPlayer + 4) == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pPlayer + 0x22) = 0x3C;
        *(s32 *)(pPlayer + 0x28) = g_WorldMapPlayerPosition;
        *(s32 *)(pPlayer + 0x2C) = D_8009C5B0;
        *(s32 *)(pPlayer + 0x30) = D_8009C5B4;
    }
    timer = *(u16 *)(pPlayer + 0x22);
    timer--;
    *(s16 *)(pPlayer + 0x22) = timer;
    if (timer > 0) {
        *(s32 *)(pPlayer + 0x28) += -0x95D0;
        *(s32 *)(pPlayer + 0x30) += 0x2F130;
        *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
        *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
        *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
        func_80089160(9, (void *)0x1F8000A0, (s32)g_WorldMapSpriteGroup9Rotation);
    } else {
        *(s16 *)(pPlayer + 0x22) = 0x3C;
        *(WorldMapPos *)(pPlayer + 0x28) = *(WorldMapPos *)&g_WorldMapPlayerPosition;
        WorldMapDeactivateSpriteGroup(9);
        nResult = 3;
    }
    return nResult;
}

int WorldMapInitGroup0Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(0, 1);
    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
    pLayer = g_WorldMapGroups;
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->unk50 = 0;
    pPlayer->unk54 = 0x100;
    pPlayer->unk58 = 0x100;
    pLayer->rot.vx = 0;
    pLayer->rot.vy = 0;
    pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007BBEC);
/* PARKED 98.80% (218/218 insns) - cannot be converted on its own: the switch
   table would land at .rodata+0x30 but retail has func_8007ADD4's two tables
   (jtbl_8006FBF4/jtbl_8006FC14) there first; convert ADD4 and BBEC together.
   Only remaining diff: the 16.16 x read at 0x1F800002, retail "lh", ours
   "lhu;sll 16;sra 16" (tried ((s16*)pScratch)[1], *(s16*)((u8*)pScratch+2),
   ((SVECTOR*)pScratch)->vy).
    s32 func_8007BBEC(s32 idx) {
        WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
        WorldMapSceneLayer *pLayer;
        WorldMapTrackPoint *pPoint;
        u32 *pScratch = (u32 *)0x1F800000;
        VECTOR *pVec0;
        VECTOR *pVec1;
        VECTOR *pVec2;
        VECTOR *pVec3;
        SVECTOR *pRot;
        SVECTOR *pOutRot;
        MATRIX *pMat;
        s32 result = 1;

        switch (pPlayer->unk50 >> 12) {
        case 0:
        case 1:
            pPlayer->unk50 += pPlayer->unk58;
            pPlayer->unk54 += 4;
            break;
        case 2:
        case 3:
        case 4:
            pPlayer->unk50 += pPlayer->unk58;
            pPlayer->unk58 -= 8;
            if (pPlayer->unk58 < 0x80) {
                pPlayer->unk58 = 0x80;
            }
            pPlayer->unk54 += 4;
            break;
        case 5:
        case 6:
        case 7:
            pPlayer->unk50 += pPlayer->unk58;
            pPlayer->unk58 += 8;
            if (pPlayer->unk58 > 0x100) {
                pPlayer->unk58 = 0x100;
            }
            pPlayer->unk54 -= 0x10;
            break;
        case 8:
            result = 3;
            break;
        }
        pPoint = &D_8009A490[pPlayer->unk50 >> 12];
        if (pPoint[2].unk6 != -1) {
            WorldMapInterpolateQuadraticVector(pPlayer->unk50 & 0xFFF, (s16 *)pPoint,
                                               (s16 *)&pPoint[1], (s16 *)&pPoint[2],
                                               (s32 *)pScratch);
        }
        pVec1 = (VECTOR *)&pScratch[4];
        pPoint = &D_8009A490[(pPlayer->unk50 + 0x80) >> 12];
        WorldMapInterpolateQuadraticVector((pPlayer->unk50 + 0x80) & 0xFFF, (s16 *)pPoint,
                                           (s16 *)&pPoint[1], (s16 *)&pPoint[2], (s32 *)pVec1);
        pLayer = g_WorldMapGroups;
        pLayer->pos.x = ((s16 *)pScratch)[1];
        pLayer->pos.y = ((s32 *)pScratch)[1] >> 16;
        pLayer->pos.z = ((s32 *)pScratch)[2] >> 16;
        pVec0 = (VECTOR *)pScratch;
        pVec1->vx = (pVec1->vx - pVec0->vx) >> 12;
        pVec1->vy = (pVec1->vy - pVec0->vy) >> 12;
        pVec1->vz = -((pVec1->vz - pVec0->vz) >> 12);
        VectorNormal(pVec1, pVec0);
        pRot = (SVECTOR *)&pScratch[0x28];
        pRot->vx = 0;
        pRot->vy = ratan2(pVec0->vx, pVec0->vz) & 0xFFF;
        pRot->vz = pPlayer->unk54;
        pMat = (MATRIX *)&pScratch[0x3C];
        RotMatrixYXZ(pRot, pMat);
        pRot->vx = 0;
        pRot->vy = -0x1000;
        pRot->vz = 0;
        ApplyMatrix(pMat, pRot, pVec1);
        pVec3 = (VECTOR *)&pScratch[0xC];
        OuterProduct12(pVec0, pVec1, pVec3);
        pVec2 = (VECTOR *)&pScratch[8];
        VectorNormal(pVec3, pVec2);
        OuterProduct12(pVec0, pVec2, pVec3);
        VectorNormal(pVec3, pVec1);
        pMat->m[0][0] = pVec2->vx;
        pMat->m[0][1] = pVec2->vy;
        pMat->m[0][2] = pVec2->vz;
        pMat->m[1][0] = pVec1->vx;
        pMat->m[1][1] = pVec1->vy;
        pMat->m[1][2] = pVec1->vz;
        pMat->m[2][0] = pVec0->vx;
        pMat->m[2][1] = pVec0->vy;
        pMat->m[2][2] = pVec0->vz;
        TransposeMatrix(pMat, &pLayer->mat);
        pOutRot = (SVECTOR *)&pScratch[0x2A];
        WorldMapExtractRotationFromMatrix(pMat, pOutRot);
        if (pLayer->pos.y >= -0x7F) {
            pRot->vx = pLayer->pos.x;
            pRot->vy = pLayer->pos.y;
            pRot->vz = pLayer->pos.z;
            pOutRot->vz = -*(u16 *)&pOutRot->vz;
            func_80089160(0x12, pRot, (s32)pOutRot);
        }
        return result;
    }
*/
void WorldMapInitializeModeWithGroupActors(void) {
    RECT rect;
    s32 *pPos = &g_WorldMapPlayerPosition;
    s32 *pScratch; // shared: one register for the position store and the group-13 handler

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
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    pScratch = pPos;
    *pScratch = 0xD00000;
    D_8009C5B0 = 0xFFF60000;
    D_8009C5B4 = 0x400000;
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
    SoundAddSedsEntry(g_MenuSedsFile);
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot((void *)WorldMapModeInitFadeRect, (void *)func_800925A0);
    WorldMapAddSlot((void *)WorldMapInitActorParamsFromTable, (void *)func_8007C3B8);
    WorldMapAddSlot((void *)func_8007C724, (void *)func_8007C7D8);
    WorldMapAddSlot((void *)WorldMapInitGroup4Actor, (void *)WorldMapUpdateGroup4Actor);
    WorldMapAddSlot((void *)WorldMapInitGroup5Actor, (void *)func_8007CF18);
    WorldMapAddSlot((void *)WorldMapInitGroup9Actor, (void *)WorldMapUpdateGroup9Actor);
    WorldMapAddSlot((void *)WorldMapInitGroup10Actor, (void *)WorldMapUpdateGroup10Actor);
    WorldMapAddSlot((void *)WorldMapInitGroup12Actor, (void *)WorldMapUpdateGroup12Actor);
    pScratch = (void *)WorldMapUpdateGroup13Actor;
    WorldMapAddSlot((void *)WorldMapInitGroup13Actor, pScratch);
    WorldMapAddSlot((void *)WorldMapInitGroup16Actor, (void *)WorldMapUpdateGroup16Actor);
    WorldMapAddSlot((void *)WorldMapEnterNop, (void *)WorldMapUpdateFrameWithParticles);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    WorldMapInitRandomTimers();
}

void WorldMapExitToMap111(void) {
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
    v = g_WorldMapViewRotationY;
    g_NextMapId = 0x111;
    g_NextMapFlags = 2;
    D_8009BBC4 = 1;
    g_NextMapRotationY = v;
}

int WorldMapInitActorParamsFromTable(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots;
    s32 *pCount;

    pPlayer += idx;
    pCount = &pPlayer->unk50;
    *pCount = 0;
    pPlayer->unk20 = g_WorldMapActorParamStateIds;
    pPlayer->unk22 = g_WorldMapActorParamStateFrames[*pCount];
    *pCount = *pCount + 1;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C3B8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C724);
/* 44.62%: instruction multiset and size identical (180 bytes), only gcc sched2
   order differs. Retail puts the 0x7C store right after the addu and emits the
   three D_8009C5xx loads after it, and keeps li 0xffc0 late; gcc hoists the
   D_8009C5B0/B4 loads, sinks the 0x7C store next to the 0x58 store and hoists
   li 0xffc0 to the top. Tried: unk5C indexed vs a named unk7C field, s32
   locals for the three loads, plain globals (all three give the same 180-byte
   schedule), -O3 (41.22%). Same class as misc4 WorldMapResetActorSlotWithSound (70%) and misc6
   func_80080578 (34.97%).
int func_8007C724(int idx) {
    WorldMapPlayer *pPlayer;
    s32 a;
    s32 b;
    s32 c;

    g_WorldMapFadeLevel = 0x78;
    g_WorldMapCameraHeight = 0x400000;
    pPlayer = g_WorldMapSlots + idx;
    g_WorldMapRotationMatrixValid = 1;
    ((u32 *)&pPlayer->unk5C)[8] = 0x1000;
    a = g_WorldMapPlayerPosition;
    b = D_8009C5B0;
    c = D_8009C5B4;
    g_WorldMapCameraRot = -0x40;
    g_WorldMapViewRotationY = 0;
    D_8009BD3C = 0;
    g_WorldMapCameraTarget = a;
    g_WorldMapScrollTargetX = a;
    D_8009BE2C = b;
    g_WorldMapScrollTargetY = b;
    D_8009BE30 = c;
    g_WorldMapScrollTargetZ = c;
    pPlayer->unk58 = 0x40;
    pPlayer->unk50 = 0;
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C7D8);
int WorldMapInitGroup4Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(4, 0);
    WorldMapSetSpriteSetParent(4, 2);
    WorldMapSetSpriteSetParent(4, 1);
    WorldMapSetSpriteSetParent(4, 3);
    pLayer = &g_WorldMapGroups[4];
    pLayer->unk0 = 0;
    pLayer->rot.vx = pLayer->rot.vy = pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xD00000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x400000;
    return 1;
}
int WorldMapUpdateGroup4Actor(int idx) {
    WorldMapPlayer *pPlayer = g_WorldMapSlots + idx;
    WorldMapSceneLayer *pBlock = (WorldMapSceneLayer *)&((u8 *)g_WorldMapGroups)[0x150];
    u16 *pScratchPos = (u16 *)0x1F800000;
    s16 mode;

    mode = pPlayer->unk4;
    if (mode == 1) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 1;
    } else if (mode == 2) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 2;
    }
    pPlayer->unk30 += pPlayer->unk40;
    WorldMapWrapPosition(&pPlayer->unk28);
    pBlock->pos.x = pPlayer->unk28 >> 12;
    pBlock->pos.y = pPlayer->unk2C >> 12;
    pBlock->pos.z = pPlayer->unk30 >> 12;
    pScratchPos[0x50] = pPlayer->unk28 >> 12;
    pScratchPos[0x51] = pPlayer->unk2C >> 12;
    pScratchPos[0x52] = pPlayer->unk30 >> 12;
    func_80089160(0x13, (void *)&pScratchPos[0x50], 0);
    switch (pPlayer->unk20) {
    case 1:
        func_80089160(0x1F, (void *)&pScratchPos[0x50], 0);
        /* fallthrough */
    case 0:
        g_WorldMapScrollTargetZ = pPlayer->unk30;
        break;
    case 2:
        func_80089160(0x1F, (void *)&pScratchPos[0x50], 0);
        break;
    }
    return 1;
}
int WorldMapInitGroup5Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(5, 6);
    pLayer = &g_WorldMapGroups[5];
    pLayer->unk0 = 0;
    pLayer->rot.vx = pLayer->rot.vy = pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xB00000;
    pPlayer->unk20 = 0;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x200000;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007CF18);
/* best 75.19% (356 bytes, size exact): the two `if` blocks and every store
   are right, but gcc CSEs the three scratchpad addresses (0x1F8000A0/A2/A4,
   each used once per block) into s3/s4/s2 across both calls -- retail
   re-materialises them as absolute `lui $at / sh` for all but one, so our
   frame is 0x30 not 0x28 and s5 replaces retail's single s3-held constant 1.
   Tried: volatile on the six stores (76.84%, 364 bytes), keeping the
   literal 1 in each comparison. The same six stores in WorldMapUpdateGroup10Actor (this
   file, matched) do compile to the $at form, so this is an allocator tie
   triggered by the extra `0x20` test, not a source-shape error.
int func_8007CF18(int idx) {
    u8 *pPlayer = (u8 *)g_WorldMapSlots + (idx << 7);
    u8 *pWork = (u8 *)g_WorldMapGroups;

    if (*(s16 *)(pPlayer + 4) == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pPlayer + 0x20) = 1;
    }
    *(s32 *)(pPlayer + 0x30) += *(s32 *)(pPlayer + 0x40);
    WorldMapWrapPosition(pPlayer + 0x28);
    *(s32 *)(pPlayer + 0x2C) = func_80093A5C(*(s32 *)(pPlayer + 0x28), *(s32 *)(pPlayer + 0x30)) - 0x4000;
    *(s32 *)(pWork + 0x1AC) = *(s32 *)(pPlayer + 0x28) >> 12;
    *(s32 *)(pWork + 0x1B0) = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(s32 *)(pWork + 0x1B4) = *(s32 *)(pPlayer + 0x30) >> 12;
    *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
    *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
    func_80089160(0x15, (void *)0x1F8000A0, 0);
    if (*(s16 *)(pPlayer + 0x20) == 1) {
        *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
        *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
        *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
        func_80089160(0x1E, (void *)0x1F8000A0, 0);
    }
    return 1;
}
*/
int WorldMapInitGroup9Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(9, 7);
    WorldMapSetSpriteSetParent(9, 8);
    pLayer = &g_WorldMapGroups[9];
    pLayer->unk0 = 0;
    pLayer->rot.vx = pLayer->rot.vy = pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk28 = 0xC00000;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0;
    return 1;
}
int WorldMapUpdateGroup9Actor(int idx) {
    // byte view of the player slot: with a typed pointer gcc reuses the actor's
    // position words instead of reloading them, losing retail's two load-delay
    // nops (62.97%, 276 bytes against retail's 280)
    u8 *pPlayer = (u8 *)g_WorldMapSlots + (idx << 7);
    WorldMapSceneLayer *pLayer = g_WorldMapGroups;

    if (*(s16 *)(pPlayer + 4) == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        pLayer[9].unk0 = 1;
        pLayer[7].unk0 = 1;
        pLayer[8].unk0 = 1;
        WorldMapDeactivateSpriteGroup(0x16);
        return 3;
    }
    *(s32 *)(pPlayer + 0x30) += *(s32 *)(pPlayer + 0x40);
    WorldMapWrapPosition(pPlayer + 0x28);
    *(s32 *)(pPlayer + 0x2C) = func_80093A5C(*(s32 *)(pPlayer + 0x28), *(s32 *)(pPlayer + 0x30)) - 0x4000;
    pLayer[9].pos.x = *(s32 *)(pPlayer + 0x28) >> 12;
    pLayer[9].pos.y = *(s32 *)(pPlayer + 0x2C) >> 12;
    pLayer[9].pos.z = *(s32 *)(pPlayer + 0x30) >> 12;
    *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
    *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
    func_80089160(0x16, (void *)0x1F8000A0, 0);
    return 1;
}

int WorldMapInitGroup10Actor(int idx) {
    WorldMapSceneLayer *pLayer;
    WorldMapPlayer *pPlayer;

    WorldMapSetSpriteSetParent(0xA, 0xB);
    pLayer = &g_WorldMapGroups[10];
    pLayer->unk0 = 0;
    pLayer->rot.vx = pLayer->rot.vy = pLayer->rot.vz = 0;
    RotMatrixYXZ(&pLayer->rot, &pLayer->mat);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xE00000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x100000;
    return 1;
}

int WorldMapUpdateGroup10Actor(int idx) {
    // byte view of the player slot, for the reason given in WorldMapUpdateGroup9Actor
    u8 *pPlayer = (u8 *)g_WorldMapSlots + (idx << 7);
    WorldMapSceneLayer *pLayer = g_WorldMapGroups;

    if (*(s16 *)(pPlayer + 4) == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        pLayer[10].unk0 = 1;
        pLayer[11].unk0 = 1;
        WorldMapDeactivateSpriteGroup(0x17);
        *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
        *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
        *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
        func_80089160(0x1C, (void *)0x1F8000A0, 0);
        return 3;
    }
    *(s32 *)(pPlayer + 0x30) += *(s32 *)(pPlayer + 0x40);
    WorldMapWrapPosition(pPlayer + 0x28);
    *(s32 *)(pPlayer + 0x2C) = func_80093A5C(*(s32 *)(pPlayer + 0x28), *(s32 *)(pPlayer + 0x30)) - 0x4000;
    pLayer[10].pos.x = *(s32 *)(pPlayer + 0x28) >> 12;
    pLayer[10].pos.y = *(s32 *)(pPlayer + 0x2C) >> 12;
    pLayer[10].pos.z = *(s32 *)(pPlayer + 0x30) >> 12;
    *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
    *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
    func_80089160(0x17, (void *)0x1F8000A0, 0);
    return 1;
}
