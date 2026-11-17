#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "system/sound.h"

// One slot of the pool g_WorldMapSlots (stride 0x80), partial view: only the script
// cursor and the wait countdown are used here. The cursor is halfword
// addressed; 0x50 of a slot is a pointer to the current command word.
typedef struct {
    /* 0x00 */ u8 unk0[0x22];
    /* 0x22 */ s16 waitFrames;
    /* 0x24 */ u8 unk24[0x2C];
    /* 0x50 */ s16* pc;
    /* 0x54 */ u8 unk54[0x2C];
} WorldMapScriptSlot; // Size: 0x80

// The world map frame render context g_WorldMapRenderContext points at: the two ordering
// table heads it fills each frame.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long* pOt[2];
} WorldMapRenderContext; // partial view

extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;

extern void WorldMapDeactivateSpriteGroup(s32 arg0);
extern void WorldMapClearGroupEntryFlag(s32 arg0);
extern void WorldMapStartSlot(s32 arg0, s32 arg1);
extern s32* g_SoundAudioManager;

extern void SoundSetAudioManagerVolumeWithFade(s32* pManager);
extern void func_80089160(s32 arg0, void* arg1, s32 arg2);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);
extern void SoundFadeElementVolumeById(s32 arg0, s32 arg1, s32 arg2);

extern u8 g_WorldMapPlayerPosition[];
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapCameraHeight;
extern SoundFile* g_MenuSedsFile;

extern WorldMapScriptSlot* g_WorldMapSlots;
extern s32 g_WorldMapRotationMatrixValid;
extern s32 g_WorldMapPathFrame;
extern s16 g_WorldMapSlotUpdateFlags;
extern s32 g_WorldMapScrollX;
extern s32 g_WorldMapCameraTarget;
extern WorldMapRenderContext* g_WorldMapRenderContext;
extern s32 g_WorldMapGridAngle;
extern s32 g_WorldMapWorkBuffer1;
extern s32 g_WorldMapWorkBuffer2;
extern s32 g_WorldMapTilePrimBuffer;
extern s32 g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapDecompressedMapData;
extern s16 g_WorldMapViewRotationY;
extern s16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 g_NextMapFlags;
extern s32 g_WorldMapExitFlag;

extern void func_8009932C(u_long* pOt0, u_long* pOt1, s32* pState);

extern void WorldMapBuildRotationMatrix(s32* p);
extern void func_80097244(s32* p);
extern void func_80089748(void);
extern void func_80089C78(void);
extern void func_8008615C(void);
extern void func_800848F4(void);
extern void func_800980D4(s32* p);
extern void func_800981C8(s32* p);
extern void WorldMapWaitForLoadSlotFree(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32* p);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void WorldMapFreeSpriteSets(void);
extern void WorldMapFreeSpritePrimBuffers(void);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeGridImages(void);
extern void WorldMapFreeSlotPool(void);
extern void HeapFree(void* pMem);

typedef s32 (*WorldMapScriptHandler)(WorldMapScriptSlot* pSlot, s32 arg1, s16 arg2, s16 arg3);
extern WorldMapScriptHandler g_WorldMapScriptOpHandlers[];

// World map player (stride 0x80), the scroll and height fields
// func_80076DA4, func_80076FA8 and WorldMapApproachHeight touch. Same layout
// as misc12.c's local WorldMapPlayer.
typedef struct {
    /* 0x00 */ u8 unk0[0x28];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ u8 unk44[0xC];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
} WorldMapPlayer; // Size: 0x60

// Four-word position record; misc12.c copies the same bytes through a Quad.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} WorldMapPos4; // Size: 0x10

// Scroll delta buffer: the wrapped xyz offset at 0x00 and its 1/8 step at 0x10.
typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 pad0C;
    /* 0x10 */ s32 stepX;
    /* 0x14 */ s32 stepY;
    /* 0x18 */ s32 stepZ;
} WorldMapScrollDelta; // Size: 0x1C

// Eight name bytes swapped through the scratchpad by func_8007756C; the type
// is byte-aligned so gcc copies it with lwl/lwr.
typedef struct {
    /* 0x0 */ u8 name[8];
} WorldMapName8; // Size: 0x8


extern void WorldMapWrapDelta(WorldMapScrollDelta* pDelta);
extern void func_80096F18(void* pOut, s32* pSrc, s32 nHeight);
extern s32 g_WorldMapFadeLevel;
extern s32 D_8009BD48;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 g_WorldMapScrollTargetX;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapScrollTargetZ;
extern s32 D_8009BBBC;
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;

s32 WorldMapUpdateFrame(void) {
    s32* pState;

    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapBuildRotationMatrix(&g_WorldMapPathFrame);
    } else {
        func_80097244(&g_WorldMapPathFrame);
    }
    func_80089748();
    func_80089C78();
    func_8008615C();
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

s32 WorldMapRunSlotScript(s32 index) {
    WorldMapScriptSlot* pSlot = &g_WorldMapSlots[index];
    s16* pCmd;
    s32 cmd;
    s32 n = 0;
    s32 pad[2]; // unused: reproduces retail's 0x28 frame

    do {
        pCmd = pSlot->pc += n;
        cmd = *(s32*)pCmd;
        n = g_WorldMapScriptOpHandlers[cmd & 0xFFFF](pSlot, cmd >> 16, pCmd[2], pCmd[3]);
    } while (n != 0);
    return 1;
}

s32 WorldMapScriptOp00End(void) {
    g_WorldMapLoopActive = 0;
    g_WorldMapAreaMode = 0;
    return 0;
}

s32 WorldMapScriptOp01WaitFrames(WorldMapScriptSlot* pSlot, s32 arg1) {
    s16 timer = pSlot->waitFrames;

    if (timer == 0) {
        pSlot->waitFrames = arg1;
        return 0;
    }
    timer--;
    pSlot->waitFrames = timer;
    return (timer < 1) << 1;
}

s32 WorldMapScriptOp02StartSlot(s32 arg0, s32 arg1, s32 arg2) {
    WorldMapStartSlot(arg1, arg2);
    return 4;
}

s32 WorldMapScriptOp03SetPosition(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(s32*)g_WorldMapPlayerPosition = arg1 << 12;
    g_WorldMapPlayerPositionY = arg2 << 12;
    g_WorldMapPlayerPositionZ = arg3 << 12;
    return 4;
}

s32 WorldMapScriptOp04SetScratchPosition(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(u16*)0x1F8000A0 = arg1;
    *(u16*)0x1F8000A2 = arg2;
    *(u16*)0x1F8000A4 = arg3;
    return 4;
}

s32 WorldMapScriptOp05ActivateSpriteGroup(s32 arg0, s32 arg1) {
    func_80089160(arg1, (void*)0x1F8000A0, 0);
    return 2;
}

s32 WorldMapScriptOp06DeactivateSpriteGroup(s32 arg0, s32 arg1) {
    WorldMapDeactivateSpriteGroup(arg1);
    return 2;
}

s32 WorldMapScriptOp07ClearGroupEntries(s32 arg0, s32 arg1) {
    WorldMapClearGroupEntryFlag(arg1);
    return 2;
}

s32 WorldMapScriptOp08FadeMasterVolume(void) {
    SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager);
    return 4;
}

s32 WorldMapScriptOp09PlaySound(s32 arg0, s32 arg1) {
    SoundPlaySoundEffectOnFreePair((g_MenuSedsFile->sedId << 16) | arg1);
    return 2;
}

s32 WorldMapScriptOp10FadeSound(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    SoundFadeElementVolumeById((g_MenuSedsFile->sedId << 16) | arg1, arg2, arg3);
    return 4;
}

s32 WorldMapScriptOp11SetDrawMode(s32 arg0, s32 arg1, s32 arg2) {
    g_WorldMapTpageAbr = arg1;
    g_WorldMapDrawMode = arg2;
    return 4;
}

void func_80076DA4(WorldMapPlayer* pPlayer, WorldMapScrollDelta* pDelta) {
    s16 rotX;
    s16 rotY;
    s16 rotZ;

    if ((pPlayer->unk38 != pPlayer->unk50) | (pPlayer->unk3C != pPlayer->unk54) |
        (pPlayer->unk40 != pPlayer->unk58)) {
        pDelta->x = pPlayer->unk50 - pPlayer->unk38;
        pDelta->y = pPlayer->unk54 - pPlayer->unk3C;
        pDelta->z = pPlayer->unk58 - pPlayer->unk40;
        WorldMapWrapDelta(pDelta);
        pDelta->stepX = pDelta->x >> 3;
        pDelta->stepY = pDelta->y >> 3;
        pDelta->stepZ = pDelta->z >> 3;
        if ((pDelta->stepX < 0 ? -pDelta->stepX : pDelta->stepX) < 0x40) {
            pPlayer->unk38 = pPlayer->unk50;
        } else {
            pPlayer->unk38 += pDelta->stepX;
        }
        if ((pDelta->stepY < 0 ? -pDelta->stepY : pDelta->stepY) < 0x40) {
            pPlayer->unk3C = pPlayer->unk54;
        } else {
            pPlayer->unk3C += pDelta->stepY;
        }
        if ((pDelta->stepZ < 0 ? -pDelta->stepZ : pDelta->stepZ) < 0x40) {
            pPlayer->unk40 = pPlayer->unk58;
        } else {
            pPlayer->unk40 += pDelta->stepZ;
        }
    }
    g_WorldMapCameraRot = pPlayer->unk38 >> 12;
    // shortcut: the do/while block isolates this store so cc1 schedules it as a
    // unit; without it gcc hoists the next load one slot early and is 3 insns short
    do { g_WorldMapViewRotationY = pPlayer->unk3C >> 12; } while (0);
    g_WorldMapViewRotationZ = pPlayer->unk40 >> 12;

}


void WorldMapApproachHeight(WorldMapPlayer* pPlayer) {
    s32 cur = pPlayer->unk5C;
    s32 idx;

    if (cur != g_WorldMapCameraHeight) {
        idx = (cur - g_WorldMapCameraHeight) >> 3;
        if ((idx < 0 ? -idx : idx) < 0x40) {
            g_WorldMapCameraHeight = cur;
        } else {
            g_WorldMapCameraHeight = g_WorldMapCameraHeight + idx;
        }
    }
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80076FA8);
/* Best attempt: 75.16%. Same skeleton as func_80076DA4 (compare the three
   player fields against the g_WorldMapScrollTargetX/560/564 goal, wrap the delta through
   WorldMapWrapDelta, 1/8 step, |step| < 0x200 snaps to the goal). Remaining diff:
   inside the if-block gcc cse reuses the g_WorldMapScrollTargetX/560/564 values it loaded
   for the compare (in a2/a3/a1), while the original reloads g_WorldMapScrollTargetY and
   g_WorldMapScrollTargetZ plus the player fields, so everything after shifts by two
   instructions. The four-word copy at the end is WorldMapPos4.
void func_80076FA8(WorldMapPlayer* pPlayer, WorldMapScrollDelta* pDelta) {
    if ((pPlayer->unk28 != g_WorldMapScrollTargetX) | (pPlayer->unk2C != g_WorldMapScrollTargetY) |
        (pPlayer->unk30 != g_WorldMapScrollTargetZ)) {
        pDelta->x = g_WorldMapScrollTargetX - pPlayer->unk28;
        pDelta->y = g_WorldMapScrollTargetY - pPlayer->unk2C;
        pDelta->z = g_WorldMapScrollTargetZ - pPlayer->unk30;
        WorldMapWrapDelta(pDelta);
        pDelta->stepX = pDelta->x >> 3;
        pDelta->stepY = pDelta->y >> 3;
        pDelta->stepZ = pDelta->z >> 3;
        if ((pDelta->stepX < 0 ? -pDelta->stepX : pDelta->stepX) < 0x200) {
            g_WorldMapScrollX += pDelta->x;
            pPlayer->unk28 = g_WorldMapScrollTargetX;
        } else {
            g_WorldMapScrollX += pDelta->stepX;
            pPlayer->unk28 += pDelta->stepX;
        }
        if ((pDelta->stepY < 0 ? -pDelta->stepY : pDelta->stepY) < 0x200) {
            pPlayer->unk2C = g_WorldMapScrollTargetY;
        } else {
            pPlayer->unk2C += pDelta->stepY;
        }
        if ((pDelta->stepZ < 0 ? -pDelta->stepZ : pDelta->stepZ) < 0x200) {
            D_8009BBBC += pDelta->z;
            pPlayer->unk30 = g_WorldMapScrollTargetZ;
        } else {
            D_8009BBBC += pDelta->stepZ;
            pPlayer->unk30 += pDelta->stepZ;
        }
    }
    *(WorldMapPos4*)&g_WorldMapCameraTarget = *(WorldMapPos4*)&pPlayer->unk28;
}
*/

s32 WorldMapApproachValue(s32 arg0, s32 arg1, s32 arg2) {
    s32 delta;
    s32 limit;

    if (arg0 == arg1) {
        return arg0;
    }
    delta = arg1 - arg0;
    if (delta < 0) {
        delta = -delta;
    }
    limit = arg2;
    if (limit < 0) {
        // shortcut: the no-op redefinitions keep cse from negating the parameter
        // (negu v1,a2) instead of the copy; without them this is 99.67%
        arg2++;
        arg2--;
        limit = -limit;
    }
    if (delta < limit) {
        arg0 = arg1;
    } else {
        arg0 += arg2;
    }
    return arg0;
}
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapModeMatrix;
extern s32 g_WorldMapMode4EntryTriggered;
extern void (*g_WorldMapWrapPositionsCallback)(void);
extern void WorldMapWrapTileVertexPositions(void);
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void WorldMapDecompressMapTables(void);
extern void WorldMapAllocSlotPool(void);
extern void WorldMapComputeClipPlaneVectors(void);
extern s32 ArchiveCdDataSync(s32 arg0);
extern void WorldMapLoadSedsBank(void);
extern void WorldMapLoadTextureClut(void);
extern void WorldMapInitSpriteSets(void);
extern void WorldMapLoadClutAndTpageIds(void);
extern void func_800736DC(void);
extern void WorldMapRelocateMapEntryOffsets(void);
extern void func_80085FE0(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void WorldMapInitSpriteGroupTables(void);
extern void SoundAddSedsEntry(void* pSeds);
extern s32 ArchiveSetIndex(s32 arg0, s32 arg1);
extern void WorldMapAddSlot(void* p0, void* p1);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void WorldMapInitRandomTimers(void);
extern void WorldMapModeInitFadeRect(void);
extern void func_800925A0(void);
extern void WorldMapInitSlotAngleStep8(void);
extern void WorldMapUpdateSlotRotationMatrices(void);
extern void WorldMapModeInitNop(void);
extern void WorldMapUpdateFrameWithSlotProjection(void);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern void WorldMapInitActorAtScrollPosition(void);
extern void func_800783E8(void);
extern void WorldMapResetActorSlotWithSound(void);
extern void func_80077E68(void);
extern void func_80097BC0(void* p);
extern void WorldMapUpdateLoadQueue(void);
extern s32 WorldMapGetQueuedLoadCount(void);
extern s32 func_8007756C(s32 arg0);
extern s32 func_800776E0(s32 arg0);

void WorldMapInitializeMode(void) {
    RECT rect;
    s32* pPos;

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
    WorldMapDecompressMapTables();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 2;
    g_WorldMapDrawMode = 4;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    pPos = (s32*)g_WorldMapPlayerPosition;
    *pPos = 0x7702000;
    g_WorldMapPlayerPositionY = 0xFFD00000;
    g_WorldMapPlayerPositionZ = 0x27C0000;
    WorldMapInitSpriteSets();
    WorldMapLoadTextureClut();
    WorldMapLoadClutAndTpageIds();
    func_800736DC();
    WorldMapRelocateMapEntryOffsets();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    WorldMapInitSpriteGroupTables();
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot((void*)WorldMapModeInitFadeRect, (void*)func_800925A0);
    WorldMapAddSlot((void*)func_8007756C, (void*)func_800776E0);
    WorldMapAddSlot((void*)WorldMapInitSlotAngleStep8, (void*)WorldMapUpdateSlotRotationMatrices);
    WorldMapAddSlot((void*)WorldMapModeInitNop, (void*)WorldMapUpdateFrameWithSlotProjection);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    func_80085FE0();
    WorldMapInitRandomTimers();
    func_80089160(0xE, NULL, 0);
}

s32 WorldMapExitMode(void) {
    u16 v;

    WorldMapFreeSpriteSets();
    WorldMapFreeSpritePrimBuffers();
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
    v = *(u16*)&g_WorldMapViewRotationY;
    g_NextMapId = 0x11;
    g_NextMapFlags = 7;
    g_WorldMapExitFlag = 1;
    g_NextMapRotationY = v;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_8007756C);
/* Best attempt: 74.59% (380 bytes vs 368). Structure decoded and right: the
   16-byte copy pSlot+0x28 -> pSlot+0x38 is one struct assignment (lw x4 then
   sw x4) and the three 8-byte copies around 0x1F8000A0 swap g_WorldMapPathFrame with
   D_8009BD48 through the scratchpad (needs a 1-byte-aligned struct, which is
   what gives lwl/lwr). g_WorldMapViewRotationY must be s16 (the original does lh). Four
   remaining diffs: (1) the original keeps &g_WorldMapPathFrame in $s0 across the call
   and both copies, gcc rematerialises lui/addiu, and a WorldMapName8 pointer
   local is folded straight back to the symbol; (2) retail reloads
   g_WorldMapViewRotationY after storing 0x200 to it, gcc forwards the constant (a
   volatile cast forces the load but then emits lhu plus sll/sra for the
   shift and costs 12 bytes); (3) the single g_WorldMapCameraRot store goes through
   $a3 (lui+addiu) in the original and through $at here; (4) retail stores
   g_WorldMapCameraTarget through the $a1 that is also the call argument, then BE30,
   BE2C, where gcc uses $at for all three and orders BE30 before BE28.
   Preset screen: gcc-2.7.2 -G0 scores 85.30 and leaves all 18 already
   matched functions in this unit at 100.00 (nothing in this unit is
   gp-relative: every symbol is an absolute from undefined_syms_auto), so a
   -G0 switch would not break the unit but still does not close it.
s32 func_8007756C(s32 arg0) {
    u8* pSlot = (u8*)g_WorldMapSlots + (arg0 << 7);
    WorldMapName8* pName = (WorldMapName8*)&g_WorldMapPathFrame;

    g_WorldMapCameraRot = -0x80;
    g_WorldMapViewRotationY = 0x200;
    g_WorldMapCameraHeight = 0x400000;
    g_WorldMapViewRotationZ = 0;
    *(s32*)(pSlot + 0x28) = 0xFFF80000;
    g_WorldMapFadeLevel = 0x78;
    *(s32*)(pSlot + 0x2C) = *(volatile s16*)&g_WorldMapViewRotationY << 12;
    *(WorldMapPos4*)(pSlot + 0x38) = *(WorldMapPos4*)(pSlot + 0x28);
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapCameraTarget = g_WorldMapPlayerPosition;
    D_8009BE2C = g_WorldMapPlayerPositionY;
    D_8009BE30 = g_WorldMapPlayerPositionZ;
    func_80096F18(pName, &g_WorldMapCameraTarget, g_WorldMapCameraHeight);
    *(WorldMapName8*)0x1F8000A0 = *pName;
    *pName = *(WorldMapName8*)&D_8009BD48;
    *(WorldMapName8*)&D_8009BD48 = *(WorldMapName8*)0x1F8000A0;
    return 1;
}
*/

extern u16 g_WorldMapPad1Released;
extern u16 g_WorldMapPad1Held;
extern s32 g_WorldMapProjectionDistance;
extern s32 D_8009BD48;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_800776E0);
/* Best attempt: 92.93% (632 bytes; retail is 628). Everything is decoded and
   every relocation is right. Retail keeps TWO callee-saved roles: s1 =
   (u8*)0x1F800000 (set by the first insn of the prologue, used for the two
   scratch deltas AND the two 0x1F8000A0 name words after the call) and s0 =
   &g_WorldMapPathFrame (loaded mid-function just before func_80096F18, then used for
   both lwl/lwr+swl/swr name swaps). Notes from the attempts below:
   - gcc only keeps 0x1F800000 in a saved register when the 0x1F8000A0 name
     access is spelled (pScratch + 0xA0) - a literal (WorldMapName8*)0x1F8000A0
     makes gcc rematerialise lui/ori and the base lands in t0.
   - the clamp is `if (nPos < -0x180000) ... else if (nPos > 0x18000) ...`,
     both stores through one register (retail reuses v1), and nPos must be a
     local (a0) - it also reproduces the branch-delay subu.
   - the four key tests reload g_WorldMapPad1Held each time (u16; 0x1000 add 0x8000,
     0x4000 sub 0x8000 on +0x28; 0x8004 add 0xFFFF0000 and 0x2008 add 0x10000
     on +0x2C). 0x8000 must stay a literal so gcc materialises it with
     ori v1,zero,0x8000 + addu.
   - the compare is `(unk38 != unk28) | (unk3C != unk2C)` (xor/sltu/or) and the
     body reloads 0x1F800000/0x1F800004 for the >>3 steps.
   - retail keeps &g_WorldMapCameraRot in a3 for the single `sh v0,0(a3)` while the
     g_WorldMapViewRotationY store goes through %lo($at); not reproduced yet.
   - preset screen: 2 other functions in this unit match at 100 under the
     unit's XenoWorldMap preset, so the compiler is right.
   Open problem: with pName assigned right before the call gcc drops s1 (only
   s0 is saved, sp-24 frame); with pName initialised at the top it saves s1 but
   assigns the scratch base s0 and pName s1, the opposite of retail, and puts
   the BD40 la in the prologue. Nothing has produced s1 = scratch with s0 =
   pName.
*/

s32 WorldMapSlotEnterNop(void) {
    return 1;
}

s32 WorldMapUpdateFrameWithoutEffects(void) {
    s32* pState;

    if (g_WorldMapRotationMatrixValid == 0) {
        WorldMapBuildRotationMatrix(&g_WorldMapPathFrame);
    } else {
        func_80097244(&g_WorldMapPathFrame);
    }
    func_8008615C();
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
void WorldMapInitializeModeWithSound(void) {
    RECT rect;
    s32* pPos;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 1);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    WorldMapAllocSlotPool();
    g_WorldMapModeMatrix = g_WorldMapIdentityMatrix;
    g_WorldMapTpageAbr = 1;
    g_WorldMapDrawMode = 4;
    g_WorldMapMode4EntryTriggered = 0;
    g_WorldMapRotationMatrixValid = 0;
    g_WorldMapWrapPositionsCallback = WorldMapWrapTileVertexPositions;
    WorldMapComputeClipPlaneVectors();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    pPos = (s32*)g_WorldMapPlayerPosition;
    *pPos = 0x2000000;
    g_WorldMapPlayerPositionY = 0xFFE00000;
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
    func_80097BC0(pPos);
    do {
        WorldMapUpdateLoadQueue();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    WorldMapAddSlot((void*)WorldMapModeInitFadeRect, (void*)func_800925A0);
    WorldMapAddSlot((void*)WorldMapResetActorSlotWithSound, (void*)func_80077E68);
    WorldMapAddSlot((void*)WorldMapInitActorAtScrollPosition, (void*)func_800783E8);
    WorldMapAddSlot((void*)WorldMapEnterNop, (void*)WorldMapUpdateFrameWithParticles);
    func_800978FC();
    func_8008901C();
    func_800865A0();
}

/*
s32 func_800776E0(s32 arg0) {
    u8* pScratch = (u8*)0x1F800000;
    u8* pSlot = (u8*)g_WorldMapSlots + (arg0 << 7);
    WorldMapName8* pName;
    s32 nPos;

    if (g_WorldMapPad1Released & 0x40) {
        g_WorldMapLoopActive = 0;
        g_WorldMapAreaMode = 0;
    }
    if (g_WorldMapPad1Held & 0x1000) {
        *(s32*)(pSlot + 0x28) += 0x8000;
    }
    if (g_WorldMapPad1Held & 0x4000) {
        *(s32*)(pSlot + 0x28) -= 0x8000;
    }
    nPos = *(s32*)(pSlot + 0x28);
    if (nPos < -0x180000) {
        *(s32*)(pSlot + 0x28) = -0x180000;
    } else if (nPos > 0x18000) {
        *(s32*)(pSlot + 0x28) = 0x18000;
    }
    if (g_WorldMapPad1Held & 0x8004) {
        *(s32*)(pSlot + 0x2C) += 0xFFFF0000;
    }
    if (g_WorldMapPad1Held & 0x2008) {
        *(s32*)(pSlot + 0x2C) += 0x10000;
    }
    if ((*(s32*)(pSlot + 0x38) != *(s32*)(pSlot + 0x28)) |
        (*(s32*)(pSlot + 0x3C) != *(s32*)(pSlot + 0x2C))) {
        *(s32*)pScratch = *(s32*)(pSlot + 0x28) - *(s32*)(pSlot + 0x38);
        *(s32*)(pScratch + 4) = *(s32*)(pSlot + 0x2C) - *(s32*)(pSlot + 0x3C);
        *(s32*)(pSlot + 0x38) += *(s32*)pScratch >> 3;
        *(s32*)(pSlot + 0x3C) += *(s32*)(pScratch + 4) >> 3;
    }
    g_WorldMapCameraRot = *(s32*)(pSlot + 0x38) >> 12;
    g_WorldMapViewRotationY = *(s32*)(pSlot + 0x3C) >> 12;
    pName = (WorldMapName8*)&g_WorldMapPathFrame;
    func_80096F18(pName, &g_WorldMapCameraTarget, g_WorldMapCameraHeight);
    *(WorldMapName8*)(pScratch + 0xA0) = *pName;
    *pName = *(WorldMapName8*)&D_8009BD48;
    *(WorldMapName8*)&D_8009BD48 = *(WorldMapName8*)(pScratch + 0xA0);
    g_WorldMapViewRotationY = (g_WorldMapViewRotationY + 0x800) & 0xFFF;
    SetGeomScreen(g_WorldMapProjectionDistance);
    return 1;
}
*/
