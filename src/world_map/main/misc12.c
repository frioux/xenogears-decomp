#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern s32 func_80095414(s32 *pPos, s32 *pRot, s32 *pOut, s32 nSpeed, s32 nMode);
extern void WorldMapSetSpriteSetParent(int, int);
extern s16 g_WorldMapPathFrame;
extern s16 D_8009BD44;
extern s32 g_WorldMapCameraTarget;
extern s32 g_WorldMapCameraTargetZ;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapProjectionDistance;
extern s32 g_WorldMapPathPointHeights[];
extern u16 D_1F8000A8;
extern void func_80096F18(s16 *pOut, s32 *pSrc, s32 nHeight, u16 *pAngles);
extern void WorldMapWrapPosition(void *p);
extern u16 g_WorldMapPad1Released;
extern s16 g_WorldMapMode4AreaId;
extern s32 g_WorldMapMode4EntryTriggered;
extern u8 *WorldMapGetTileEntry(s32 x, s32 z);
extern u8 g_WorldMapZoneId;
extern s16 g_WorldMapAreaId;
extern u16 g_WorldMapViewRotationY[];
extern u8 g_WorldMapZoneProximity;
extern void *g_WorldMapAreaRect;
extern u32 g_WorldMapZoneAreaRects[3];
extern u16 D_8009B6D0;
extern u32 D_8009B6D4[3];
extern u16 D_8009B6E0;
extern u16 g_WorldMapPad1Held;
extern s32 g_WorldMapL2R2Pressed;
extern s32 g_WorldMapL2R2PressedLast;
extern s32 g_WorldMapL2R2PressEdge;
extern s32 g_WorldMapFadeLevel;
extern s32 g_WorldMapMode;
extern s32 D_8009C5A8;
extern s32 g_WorldMapPlayerPosition;
extern s32 g_WorldMapPlayerPositionY;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapPlayerRotation;
extern s32 g_WorldMapQueuedCharacterCount;
extern s32 WorldMapGetTilePlaneHeight(s32 x, s32 z);
extern s32 g_WorldMapCameraHeight;
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationZ[];
extern s16 g_WorldMapPlayerAngle;
extern u32 g_WorldMapScrollTargetX[];
extern s16 g_WorldMapCameraPathAnglesModes1To5[];
extern s16 g_WorldMapCameraPathAnglesMode7[];
extern s16 g_WorldMapCameraPathHeightsModes1To5[];
extern s16 g_WorldMapCameraPathHeightsMode7[];
extern s32 g_WorldMapCameraTargetY;
extern SVECTOR D_8009BD48;
extern s32 g_WorldMapRotationMatrixValid;
extern s32 func_80091FF8(s32 nLimit, u16 *pIn, s16 *pOfs);

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 command;
    /* 0x06 */ s16 targetSlot; // the slot index WorldMapStartSlot is called for
    /* 0x08 */ u8 pad8[0x10];
    /* 0x18 */ s32 (*pEnterHandler)(s32); // run once when the slot machine enters it; returns the next state
    /* 0x1C */ s32 (*pUpdateHandler)(s32); // run every frame; NULL while the slot is free
    /* 0x20 */ s16 state;
    /* 0x22 */ s16 stateTimer;
    /* 0x24 */ u16 hidden; // nonzero: the slot contributes no trail point
    /* 0x26 */ u8 unk26[0x2];
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 w; // fourth word of the x/y/z vector, copied whole with it
    /* 0x38 */ u32 dirX;
    /* 0x3C */ u32 dirY;
    /* 0x40 */ u32 dirZ;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ s16 angle;
    /* 0x4A */ s16 moveSpeed;
    /* 0x4C */ void *pSprite;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60;
    // 0x64 is reused per slot mode: the mode 1 update (WorldMapMode1UpdateSlot) keeps the
    // phase of its yaw ramp here and the camera-path modes store their angle track pointer
    // in it, so no single type fits; the pointer spelling is cast at those two sites.
    /* 0x64 */ s32 unk64;
    /* 0x68 */ s16 *pCameraPathHeights;
    /* 0x6C */ s32 unk6C;
    /* 0x70 */ s32 unk70;
    /* 0x74 */ u32 unk74;
    /* 0x78 */ s32 targetAngle;
    /* 0x7C */ s32 pathIndex;
} WorldMapPlayer; // Size: 0x80

typedef struct {
    /* 0x00 */ u16 hidden; // nonzero hides the entry: modes 4/5 set entries 1-3, modes 6/7 clear them
    /* 0x02 */ u8 unk2[0x6];
    /* 0x08 */ s32 pos[4];
    /* 0x18 */ u16 rotX;
    /* 0x1A */ u16 rotY;
    /* 0x1C */ u16 rotZ;
    /* 0x1E */ u8 pad1E[0x2];
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ u8 unk40[0x14];
} WorldMapRot; // Size: 0x54

typedef struct {
    /* 0x000 */ WorldMapRot entry[4];
} WorldMapSource; // Size: 0x150, the entry table at g_WorldMapGroups has stride 0x54

extern WorldMapPlayer *g_WorldMapSlots;
extern WorldMapSource *g_WorldMapGroups;

typedef struct {
    /* 0x0 */ u32 w[4];
} Quad; // Size: 0x10


void WorldMapGetGameStatePosition(s32 *pOut) {
    pOut[0] = g_GameState.scriptPosX << 12;
    pOut[1] = g_GameState.scriptPosY << 12;
    pOut[2] = g_GameState.scriptPosZ << 12;
}

void WorldMapSetGameStatePosition(s32 *pIn) {
    g_GameState.scriptPosX = pIn[0] >> 12;
    g_GameState.scriptPosY = pIn[1] >> 12;
    g_GameState.scriptPosZ = pIn[2] >> 12;
}

void WorldMapSelectAreaRectByState(void) {
    u16 nEntry;
    void *pRect;

    if (g_WorldMapZoneProximity == 0) {
        return;
    }
    switch (g_WorldMapZoneId) {
    case 0xF:
        nEntry = D_8009B6D0;
        pRect = g_WorldMapZoneAreaRects;
        break;
    case 0x10:
        nEntry = D_8009B6E0;
        pRect = D_8009B6D4;
        break;
    default:
        return;
    }
    g_WorldMapAreaRect = pRect;
    g_WorldMapAreaId = nEntry;
}
s32 WorldMapFindStepDirection(s32 *pPos, s32 *pDir, s32 nSpeed) {
    s32 nAngle;
    s32 out[4];
    s32 rot[4];

    for (nAngle = 0; nAngle < 0x1000; nAngle += 0x100) {
        rot[0] = rcos(nAngle);
        rot[2] = -rsin(nAngle);
        if (func_80095414(pPos, rot, out, nSpeed, 2) == 1) {
            return nAngle;
        }
    }
    return -1;
}
// best: 99.79%, 868 bytes = retail's exact size, 217 insns. The body below is INSTRUCTION-
// IDENTICAL to retail except the head's scriptAngle register ($a0 here, retail $v1); the
// other differing rows are spellings (both jtbl_ operands, `addiu s0,s2,%lo(D_1F8000A0)`
// written as the folded literal 160, and the `j .L8008E3D8` arms, all check_bin-identical).
// Round 516 CLOSED arm 7: three named loads do the work the barriers could not - `nRotation`
// holds g_WorldMapPlayerRotation from the x block to the tail (that is what puts rot's load
// in retail's first block and its `sh` two blocks later), and `nY`/`nQueued` name the two
// tail loads so their `lw`s come out above the `sh` with retail's $v0/$a0 pair. The last
// 0.2% came from extending nY's life to the return (`nY = nRet; return nY;`, permuter
// PERMUTER_BETTER base=65 best=25): that one edit re-ranks the tail block's local hand-out
// and swaps rot/posY from $a0/$v1 to retail's $v1/$a0, with no other instruction moving.
// History of the same arm, for anyone retrying here. Retail is [state store][pos load]
//    [rot load][sw x][posZ load][nop][sw z]
//    [posY load][queued load][sh angle][sw unk74][sw y]; this body loads rot first and
//    stores angle in that first block. The `do { } while (0);` barriers after state/x/z
//    (a measured lever, h-an-empty-do-while-pins-a-run-of-stores) are what bought 91.93 ->
//    98.59 and retail's byte count; without them the arm batches all five loads and is two
//    nops short (860 bytes). Retail's angle store sits TWO blocks below its load, which no
//    statement order reaches; only an early `nRotation = g_WorldMapPlayerRotation` local
//    stored late would, and that is what finally worked (see above). Tried and measured then:
//    no barriers (91.93), barriers after x and z with angle last (94.76, 868 but batching),
//    angle after x with barriers (97.23), angle first with barriers (98.59 = best).
// 2. Head: nAngle (g_GameState.scriptAngle) sits in $v1 in retail, $a0 here (4 lines).
//    Moving `hidden = 0` above the WorldMapGetGameStatePosition call is what puts the
//    jal delay slot right (retail does, 88.15 -> 89.24).
// 3. Known-good spelling differences: the two jtbl_ operands (this unit owns its tables,
//    .rodata vs jtbl_800709F8/jtbl_80070A10) and `addiu s0,s2,%lo(D_1F8000A0)` written as
//    the folded literal 160. Naming D_1F8000A0 instead emits a separate lui/halfword
//    store per field (87.35) because gcc will not share the symbol's high half across
//    .vx/.vy/.vz; the u16* pScratchPos spelling is the sibling
//    WorldMapModeInitGroupRotations' and reproduces its store order.
// Presets measured on this body: cdk -O2 71.14, 2.6.0 -O2 86.81, -O1 73.35, -O3 76.35,
// -fno-schedule-insns2 86.24 - the unit's XenoWorldMap flags are right.
// Round 516 re-measured and closed the rest of the routes: a 4-minute permuter run reported
// PERMUTER_NONE (base=295 best=295, 1697 iterations), so arm 7 is not a statement-order or
// declaration edit. Dropping the nAngle local (head and both arms read pPlayer->angle
// directly, which is what retail's two `lhu 72(s1)` reloads look like) is 93.12 / 852 bytes:
// the local is load-bearing. Writing arm 7 as retail's own store order with no barriers
// (state, x, z, angle, unk74, y) reproduces retail's STORE order and per-field registers
// exactly but costs the nop (864 bytes, 95.05), because cc1 then batches all five loads above
// the state store where retail interleaves [lw Position][lw Rotation][sw x][lw PositionZ]
// [nop][sw z]; retail's PositionZ/PositionY/Queued loads all reuse $v0, so its serial
// schedule follows from register reuse and no source order reproduced it (sched1 wall).
// Round 529 re-measured this body: 99.79% / 868 bytes = retail's size, and the WHOLE residual
// is three lines - the head's scriptAngle value uses $a0 here and $v1 in retail (`lui/lhu/sh
// a0` against `lui/lhu/sh v1`). The first switch's `andi` dispatch takes $v1 in BOTH, so
// retail simply reused $v1 for the dying load; the emitted instructions are otherwise
// identical, which means the qty structure is identical too and only the allocator's
// processing order differs (the switch temp is live out of the entry block, the load is not).
// Measured this round, every one at retail's 868 bytes: `u32 nAngle` 99.52 (worse, 74 lines);
// nAngle declared first among the locals 99.79 (identical); an explicit
// `s32 nState = g_GameState.worldMapState & 0x1FFF;` local in front of the first switch
// 99.79 (identical); 3-minute permuter PERMUTER_NONE base=25 best=25, 1545 iterations.
// parked: 99.79% (868 bytes = retail's exact size); residual is 3 register lines, above.
/*
s32 func_8008E190(s32 idx) {
    WorldMapPlayer *pPlayer;
    u16 *pScratchPos;
    s32 nRet;
    u16 nAngle;
    s32 nY;
    s32 nQueued;
    s32 nRotation;
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->hidden = 0;
    WorldMapGetGameStatePosition(&pPlayer->x);
    nRet = 1;
    nAngle = g_GameState.scriptAngle;
    pPlayer->dirZ = 0;
    pPlayer->dirY = 0;
    pPlayer->dirX = 0;
    pPlayer->unk64 = 0;
    pPlayer->unk60 = 0;
    pPlayer->unk74 = 0;
    pPlayer->pCameraPathHeights = (s16 *)0xFFD80000;
    pPlayer->unk70 = 0;
    pPlayer->unk6C = 0;
    pPlayer->angle = nAngle;
    pScratchPos = (u16 *)0x1F800000;
    switch (g_GameState.worldMapState & 0x1FFF) {
    case 0:
        nRet = 3;
        break;
    case 1:
    case 2:
        pPlayer->moveSpeed = 0xC;
        break;
    case 3:
    case 4:
        pPlayer->moveSpeed = 0x20;
        break;
    }
    switch (g_WorldMapMode) {
    case 1:
        pPlayer->y = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z);
        break;
    case 2:
    case 3:
        pPlayer->state = 1;
        pPlayer->y = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z);
        break;
    case 4:
    case 5:
        pPlayer->state = 3;
        do { } while (0);
        pPlayer->x = g_WorldMapPlayerPosition;
        do { } while (0);
        pPlayer->z = g_WorldMapPlayerPositionZ;
        pPlayer->y = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z) + 0xFFFC0000;
        pPlayer->angle = g_WorldMapPlayerRotation;
        pPlayer->unk74 = g_WorldMapQueuedCharacterCount;
        *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
        nAngle = pPlayer->angle;
        g_WorldMapGroups->entry[3].hidden = 1;
        g_WorldMapGroups->entry[2].hidden = 1;
        g_WorldMapGroups->entry[1].hidden = 1;
        g_WorldMapPlayerAngle = nAngle;
        break;
    case 7:
        pPlayer->state = 2;
        do { } while (0);
        pPlayer->x = g_WorldMapPlayerPosition;
        nRotation = g_WorldMapPlayerRotation;
        do { } while (0);
        pPlayer->z = g_WorldMapPlayerPositionZ;
        do { } while (0);
        nY = g_WorldMapPlayerPositionY;
        nQueued = g_WorldMapQueuedCharacterCount;
        pPlayer->angle = nRotation;
        pPlayer->unk74 = nQueued;
        pPlayer->y = nY;
        *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
        g_WorldMapPlayerAngle = pPlayer->angle;
        break;
    }
    *(Quad *)g_WorldMapGroups->entry[0].pos = *(Quad *)&pPlayer->x;
    g_WorldMapGroups->entry[0].hidden = pPlayer->hidden;
    pScratchPos[0x50] = -(pPlayer->unk70 >> 12);
    pScratchPos[0x51] = pPlayer->angle;
    pScratchPos[0x52] = g_WorldMapViewRotationZ[0];
    RotMatrixYXZ((SVECTOR *)&pScratchPos[0x50], &g_WorldMapGroups->entry[0].matrix);
    RotMatrixYXZ((SVECTOR *)&pScratchPos[0x50], &g_WorldMapGroups->entry[1].matrix);
    WorldMapSetGameStatePosition(&pPlayer->x);
    g_GameState.scriptAngle = pPlayer->angle;
    switch (D_8009C5A8) {
    case 2:
        pPlayer->state = 0x24;
        break;
    case 3:
        pPlayer->state = 0x28;
        pPlayer->pathIndex = 0;
        break;
    case 4:
        pPlayer->state = 0x30;
        pPlayer->pathIndex = 0;
        break;
    case 5:
        pPlayer->state = 0x34;
        pPlayer->pathIndex = 0;
        break;
    }
    nY = nRet;
    return nY;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E190);



// objdiff reads 99.80% here and always will: the two differences are both spellings of
// retail's bytes, not different bytes (check_bin OK). One is the switch's table operand
// (`jtbl_80070A30` against `.rodata`), the other is `addiu $s0,$s2,%lo(D_1F8000A0)`, which
// this body writes as the folded literal 0xA0 off the same base register. Reaching the
// reloc by naming the symbol costs the shared base (388 bytes, 93.12).
s32 WorldMapModeInitGroupRotations(s32 idx) {
    WorldMapPlayer *pPlayer;
    u16 *pScratchPos;
    s32 nRet;

    pPlayer = g_WorldMapSlots + idx;
    *(Quad *)g_WorldMapGroups->entry[0].pos = *(Quad *)&pPlayer->x;
    g_WorldMapGroups->entry[0].hidden = pPlayer->hidden;
    nRet = 1;
    pScratchPos = (u16 *)0x1F800000;
    if ((g_GameState.worldMapState & 0x1FFF) == 0) {
        nRet = 3;
    }
    switch (g_WorldMapMode) {
    case 1:
        pPlayer->y = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z);
        break;
    case 2:
    case 3:
        pPlayer->state = 1;
        pPlayer->y = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z);
        break;
    case 4:
    case 5:
        g_WorldMapGroups->entry[3].hidden = 1;
        g_WorldMapGroups->entry[2].hidden = 1;
        g_WorldMapGroups->entry[1].hidden = 1;
        break;
    case 6:
    case 7:
        g_WorldMapGroups->entry[3].hidden = 0;
        g_WorldMapGroups->entry[2].hidden = 0;
        g_WorldMapGroups->entry[1].hidden = 0;
        break;
    }
    pScratchPos[0x50] = -(pPlayer->unk70 >> 12);
    pScratchPos[0x51] = pPlayer->angle;
    pScratchPos[0x52] = g_WorldMapViewRotationZ[0];
    RotMatrixYXZ((SVECTOR *)&pScratchPos[0x50], &g_WorldMapGroups->entry[0].matrix);
    RotMatrixYXZ((SVECTOR *)&pScratchPos[0x50], &g_WorldMapGroups->entry[1].matrix);
    return nRet;
}

s32 WorldMapInitActorAtGameStatePosition(s32 idx) {
    WorldMapPlayer *pPlayer;
    u16 nAngle;

    pPlayer = g_WorldMapSlots + idx;
    WorldMapGetGameStatePosition(&pPlayer->x);
    pPlayer->y = 0x80000;
    nAngle = g_GameState.scriptAngle;
    pPlayer->moveSpeed = 0x20;
    pPlayer->unk74 = 3;
    pPlayer->pCameraPathHeights = (s16 *)0xFFD80000;
    pPlayer->dirZ = 0;
    pPlayer->dirY = 0;
    pPlayer->dirX = 0;
    pPlayer->hidden = 0;
    pPlayer->unk60 = 0;
    pPlayer->unk64 = 0;
    pPlayer->unk6C = 0;
    pPlayer->state = 8;
    pPlayer->angle = nAngle;
    *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
    g_WorldMapPlayerAngle = pPlayer->angle;
    *(Quad *)g_WorldMapGroups->entry[0].pos = *(Quad *)&pPlayer->x;
    return 1;
}
extern void WorldMapInitSpriteGroupSprites(s32, void *, void *);
extern void WorldMapDeactivateSpriteGroup(s32);
extern s16 WorldMapGetTileHalfIndex(void *);
extern s32 WorldMapGetTileKindValue(s32, s16);
extern s32 WorldMapFindStepDirection(s32 *, s32 *, s32);
extern void WorldMapFindZoneAtPosition(void *, s32, s32, u8 *, u8 *);
extern s32 WorldMapSelectAreaAtPosition(void *, s32);
extern void WorldMapQueueZoneEntry(s32, s32 *, s32, s32);
extern void WorldMapAddSlotPosition(s16, s32 *);
extern void WorldMapInitRandomTimers(void);
extern s32 WorldMapMoveActorTowardTarget(WorldMapPlayer *pPlayer);
extern void WorldMapBuildDirectionFromPositions(s32 *, s32 *, u32 *, s16 *);
extern void WorldMapPlayBgmFromBuffer(s32, s32);
extern s32 SoundSetAudioManagerVolumeWithFade(s32, s32, s32);
extern s32 SoundPlaySoundEffectOnFreePair(s32);
extern s32 func_80093A5C(s32, s32);
extern s32 func_80095CD4(s32 *, u32 *, s32 *, s32, s32);
extern s32 g_SoundAudioManager;
extern s16 D_8009BD04;
extern void *g_MenuSedsFile;
extern s32 g_WorldMapSoundBankBuffer;
extern s32 g_WorldMapSoundBankFileIndex;
extern s32 g_WorldMapMode7SoundBankBuffer;
extern s32 g_WorldMapMode7SoundBankFileIndex;
extern s16 g_NextMapId;
extern s16 g_NextMapFlags;
extern u16 g_NextMapRotationY;
extern s32 g_WorldMapExitFlag;
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern s16 D_8009B1AC[];
extern s16 D_8009B1B0[];
extern s16 D_8009B1D4[];
extern s16 D_8009B1D8[];

extern s32 func_80090E14(WorldMapPlayer *pPlayer);
extern s32 func_80090FB4(WorldMapPlayer *pPlayer);
extern void WorldMapUpdateRandomTimers(void);
extern s32 g_WorldMapTpageAbr;
extern s32 g_WorldMapDrawMode;

// best 65.57% (8304 bytes / 2076 insns against retail's 8312 / 2067). REDO round: un-parked, re-measured 65.57 (identical to the note), preset screened, one structural probe tried and rejected. Established this round: (1) the callee-saved difference is a COUNT as well as an order - retail saves 8 words (s0-s6 plus ra at 0x28-0x44) in a 0x48 frame whose outgoing-args area is 0x28, the compile saves 9 ($16-$23 plus ra) in a 0x40 frame, so every sp offset AND every s-name differs. Retail's s-roles are RANGE-specific, not one-per-value: the case-2 arm re-materialises the constant 1 into $s1 (used by `nTmp != 1` and by `pPlayer->pathIndex = 1`) and -1 into $s0, and `&pPlayer->x` is $s1 in the entry block but $s0 in the case-2 arm - so do not try to reach the map by assigning one local per register. (2) The shared `nTmp` is live across calls in many arms (one pseudo in $16 for the whole function) where retail keeps arm temps in $v0/$v1: splitting nTmp into per-arm block-scoped temps is the most promising untried step. (3) REJECTED probe: moving `D_8009BD04 = 0;` out of the three entry-switch arms into one store after the switch (retail does share that tail at .L8008EA90) measures WORSE - 64.29% / 8160 bytes / 2040 insns - because gcc then cross-jumps the near-identical case 1 and case 2 bodies, which retail keeps as two copies. Keep the three copies. (4) Preset is right: cdk 57.84, -O3 60.15, -G0 compile-failed, unit flags 65.57. (5) Entry-block order residual, exactly: retail is [WorldMapStartSlot(8,9)] then [lui+addiu g_WorldMapScrollTargetX, lw,lw,lw,sw,sw,sw, lw, nop, sw] then [lhu g_GameState.worldMapState] then [ori 0x4000] then [andi 0x1FFF into s0] then [sh]; the compile hoists the lhu above the four-word copy. Retail's switch tree from that block: test 2 first, then slti 3, then 1, then 3, default jumps past the D_8009BD04 store.
/*
s32 func_8008E76C(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 *pScratch;
    s32 nState;
    s32 nRet;
    s32 nTmp;
    s32 nStep;
    s16 nCommand;
    s16 *pHeight;

    nRet = 1;
    pScratch = (s32 *)0x1F800000;
    pPlayer = g_WorldMapSlots + idx;
    nCommand = pPlayer->command;
    if (nCommand == 4) {
        pPlayer->command = 0;
        pPlayer->unk74++;
        if (g_WorldMapQueuedCharacterCount == pPlayer->unk74) {
            WorldMapStartSlot(8, 9);
            *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
            nTmp = g_GameState.worldMapState | 0x4000;
            nState = nTmp & 0x1FFF;
            g_GameState.worldMapState = nTmp;
            g_WorldMapPlayerAngle = pPlayer->angle;
            switch (nState) {
            case 1:
                pPlayer->state = 0xC;
                g_WorldMapMode = nCommand;
                pPlayer->pCameraPathHeights = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z) + 0x18000;
                *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
                *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
                *(u16 *)0x1F8000A2 = pPlayer->angle;
                *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
                *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
                *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
                WorldMapInitSpriteGroupSprites(2, (void *)0x1F8000A8, (void *)0x1F8000A0);
                D_8009BD04 = 0;
                break;
            case 2:
                pPlayer->state = 0xC;
                g_WorldMapMode = 5;
                pPlayer->pCameraPathHeights = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z) + 0x18000;
                *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
                *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
                *(u16 *)0x1F8000A2 = pPlayer->angle;
                *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
                *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
                *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
                WorldMapInitSpriteGroupSprites(2, (void *)0x1F8000A8, (void *)0x1F8000A0);
                D_8009BD04 = 0;
                break;
            case 3:
                SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, 0xF0);
                *(s16 *)0x1F8000A0 = pPlayer->x >> 0xC;
                *(u16 *)0x1F8000A2 = pPlayer->y >> 0xC;
                *(u16 *)0x1F8000A4 = pPlayer->z >> 0xC;
                nTmp = 0x3D;
                if (WorldMapGetTileHalfIndex(&pPlayer->x) == nState) {
                    nTmp = 0x3E;
                }
                WorldMapInitSpriteGroupSprites(nTmp, (void *)0x1F8000A0, NULL);
                WorldMapStartSlot(9, 9);
                WorldMapStartSlot(0xA, 9);
                WorldMapStartSlot(0xB, 9);
                nRet = 2;
                pPlayer->state = 8;
                *(s16 *)&pPlayer->unk0[2] = 0x3C;
                g_WorldMapMode = 7;
                D_8009BD04 = 0;
                break;
            }
            g_GameState.gearRide[0] = 1;
            if (g_GameState.partyMembers[1] != 0xFF) {
                g_GameState.gearRide[1] = 1;
            }
            if (g_GameState.partyMembers[2] != 0xFF) {
                g_GameState.gearRide[2] = 1;
            }
            WorldMapInitRandomTimers();
            g_WorldMapAreaRect = (void *)-1;
            g_WorldMapAreaId = -1;
            g_WorldMapMode4AreaId = -1;
        }
    }
    switch (pPlayer->state) {
    case 0:
    case 1:
        pPlayer->y = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z) - 0x1000;
        WorldMapQueueZoneEntry(idx, &pPlayer->x, 0x40, 0x400);
        break;
    case 2:
        nTmp = func_80090FB4(pPlayer);
        if (nTmp != 1) {
            if (nTmp == 4) {
                if ((WorldMapGetTileKindValue(2, WorldMapGetTileHalfIndex(&pPlayer->x)) << 0x10) != 0) {
                    nStep = WorldMapFindStepDirection(&pPlayer->x, (s32 *)&pPlayer->dirX, 0x68000);
                    if (nStep != -1) {
                        pPlayer->state = 0x10;
                        pPlayer->targetAngle = nStep;
                        pPlayer->pCameraPathHeights = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z);
                        WorldMapStartSlot(0xB, 0xA);
                        SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, 0xF0);
                        pPlayer->pathIndex = 1;
                        WorldMapDeactivateSpriteGroup(0x3C);
                        WorldMapDeactivateSpriteGroup(0x3F);
                        g_WorldMapAreaRect = (void *)-1;
                        g_WorldMapAreaId = -1;
                        g_WorldMapMode4AreaId = -1;
                        g_WorldMapZoneId = 0;
                        g_WorldMapZoneProximity = 0;
                        goto clearSpriteFlag;
                    }
                }
            } else {
                nTmp = func_80095CD4(&pPlayer->x, &pPlayer->dirX, pScratch + 0x24, pPlayer->unk60, g_WorldMapMode);
                if (nTmp == 0) {
                    pPlayer->unk60 = 0;
                    pPlayer->dirZ = 0;
                    pPlayer->dirX = 0;
                }
                if (nTmp == 1) {
                    WorldMapFindZoneAtPosition(pScratch + 0x24, 0x40, 0x20, &g_WorldMapZoneProximity, &g_WorldMapZoneId);
                    if (g_WorldMapZoneProximity != 2) {
                        pPlayer->x = pScratch[0x24];
                        pPlayer->y = pScratch[0x25];
                        pPlayer->z = pScratch[0x26];
                        pPlayer->w = pScratch[0x27];
                    } else {
                        pPlayer->unk60 = 0;
                    }
                }
                *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
                g_WorldMapPlayerAngle = pPlayer->angle;
                WorldMapSelectAreaAtPosition(&pPlayer->x, 2);
                WorldMapSelectAreaRectByState();
                *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
                *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
                *(u16 *)0x1F8000A2 = pPlayer->angle;
                RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[0].matrix);
                RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[1].matrix);
                nStep = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z) - pPlayer->y;
                if (nStep < 0) {
                    nStep = -nStep;
                }
                if ((nStep <= 0x5FFFF) && (pPlayer->unk60 != 0)) {
                    *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
                    *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
                    *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
                    nTmp = WorldMapGetTileHalfIndex(&pPlayer->x);
                    if (nTmp == 2) {
                        WorldMapInitSpriteGroupSprites(0x3F, (void *)0x1F8000A8, (void *)0x1F8000A0);
                        nStep = 0x3C;
                    } else if (nTmp == 3) {
                        WorldMapInitSpriteGroupSprites(0x3C, (void *)0x1F8000A8, (void *)0x1F8000A0);
                        nStep = 0x3F;
                    } else {
                        goto deactivateGroups;
                    }
                } else {
                deactivateGroups:
                    WorldMapDeactivateSpriteGroup(0x3C);
                    nStep = 0x3F;
                }
                WorldMapDeactivateSpriteGroup(nStep);
                pPlayer->dirZ = 0;
                pPlayer->dirY = 0;
                pPlayer->dirX = 0;
            clearSpriteFlag:
                D_8009BD04 = 0;
            }
        } else {
        idleMode:
            g_WorldMapLoopActive = 0;
            g_WorldMapAreaMode = 0;
        }
        break;
    case 3:
        nTmp = func_80090E14(pPlayer);
        switch (nTmp) {
        case 1:
            if (((s16 *)g_WorldMapAreaRect)[7] != 2) {
                goto idleMode;
            }
        enterState32:
            pPlayer->state = 0x20;
            pPlayer->pCameraPathHeights = (s16 *)0x30000;
            *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
            *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
            *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
            WorldMapInitSpriteGroupSprites(2, (void *)0x1F8000A8, (void *)0x1F8000A0);
            break;
        case 4:
            nTmp = WorldMapFindStepDirection(&pPlayer->x, (s32 *)&pPlayer->dirX, 0x68000);
            if (nTmp != -1) {
                pPlayer->state = 0x14;
                pPlayer->targetAngle = nTmp;
                pPlayer->pCameraPathHeights = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z);
                *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
                *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
                *(u16 *)0x1F8000A2 = pPlayer->angle;
                *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
                *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
                *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
                WorldMapInitSpriteGroupSprites(2, (void *)0x1F8000A8, (void *)0x1F8000A0);
                WorldMapDeactivateSpriteGroup(1);
                g_WorldMapAreaRect = (void *)-1;
                g_WorldMapAreaId = -1;
                g_WorldMapMode4AreaId = -1;
            }
            break;
        default:
            nStep = pPlayer->angle - g_GameState.scriptAngle;
            nTmp = nStep;
            if (nStep < 0) {
                nTmp = -nTmp;
            }
            if (nTmp >= 0x801) {
                if (nStep < 0) {
                    nStep += 0x1000;
                } else {
                    nStep -= 0x1000;
                }
            }
            nTmp = nStep;
            if (nStep < 0) {
                nTmp = -nTmp;
            }
            if (nTmp >= 0x81) {
                if (nStep < 0) {
                    nTmp = g_GameState.scriptAngle - 0x80;
                } else {
                    nTmp = g_GameState.scriptAngle + 0x80;
                }
                pPlayer->angle = nTmp;
            }
            pPlayer->angle &= 0xFFF;
            nStep = func_80095414(&pPlayer->x, (s32 *)&pPlayer->dirX, pScratch + 0x24, pPlayer->moveSpeed << 0xC, g_WorldMapMode);
            if (nStep == 0) {
                pPlayer->dirX = pScratch[0x24];
                pPlayer->dirY = pScratch[0x25];
                pPlayer->dirZ = pScratch[0x26];
                pPlayer->unk44 = pScratch[0x27];
                nStep = func_80095414(&pPlayer->x, (s32 *)&pPlayer->dirX, pScratch + 0x24, pPlayer->moveSpeed << 0xC, g_WorldMapMode);
                if (nStep == 0) {
                    pPlayer->dirZ = 0;
                    pPlayer->dirX = 0;
                }
            }
            if (nStep == 1) {
                pScratch[0x25] = WorldMapGetTilePlaneHeight(pScratch[0x24], pScratch[0x26]) + 0x18000;
                WorldMapFindZoneAtPosition(pScratch + 0x24, 0x40, 0x20, &g_WorldMapZoneProximity, &g_WorldMapZoneId);
                if (g_WorldMapZoneProximity != 2) {
                    pPlayer->x = pScratch[0x24];
                    pPlayer->y = pScratch[0x25];
                    pPlayer->z = pScratch[0x26];
                    pPlayer->w = pScratch[0x27];
                } else {
                    pPlayer->unk60 = 0;
                }
            }
            *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
            g_WorldMapPlayerAngle = pPlayer->angle;
            WorldMapSelectAreaAtPosition(&pPlayer->x, 2);
            *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
            *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
            *(u16 *)0x1F8000A2 = pPlayer->angle;
            RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[0].matrix);
            RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[1].matrix);
            if ((pPlayer->dirX | pPlayer->dirZ) != 0) {
                *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
                *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
                *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
                WorldMapInitSpriteGroupSprites(1, (void *)0x1F8000A8, (void *)0x1F8000A0);
            } else {
                WorldMapDeactivateSpriteGroup(1);
            }
            pPlayer->dirZ = 0;
            pPlayer->dirY = 0;
            pPlayer->dirX = 0;
            if (WorldMapGetTileHalfIndex(&pPlayer->x) != 3) {
                WorldMapDeactivateSpriteGroup(1);
                WorldMapInitSpriteGroupSprites(3, (void *)0x1F8000A8, (void *)0x1F8000A0);
                pPlayer->state = 4;
                pPlayer->stateTimer = 0x28;
            }
            break;
        }
        break;
    case 4:
        if (func_80090E14(pPlayer) == 1) {
            if (((s16 *)g_WorldMapAreaRect)[7] == 2) {
                goto enterState32;
            }
            goto idleMode;
        }
        nStep = pPlayer->angle - g_GameState.scriptAngle;
        nTmp = nStep;
        if (nStep < 0) {
            nTmp = -nTmp;
        }
        if (nTmp >= 0x801) {
            if (nStep < 0) {
                nStep += 0x1000;
            } else {
                nStep -= 0x1000;
            }
        }
        nTmp = nStep;
        if (nStep < 0) {
            nTmp = -nTmp;
        }
        if (nTmp >= 0x81) {
            if (nStep < 0) {
                nTmp = g_GameState.scriptAngle - 0x80;
            } else {
                nTmp = g_GameState.scriptAngle + 0x80;
            }
            pPlayer->angle = nTmp;
        }
        pPlayer->angle &= 0xFFF;
        nStep = func_80095414(&pPlayer->x, (s32 *)&pPlayer->dirX, pScratch + 0x24, pPlayer->moveSpeed << 0xC, g_WorldMapMode);
        if (nStep == 0) {
            pPlayer->dirX = pScratch[0x24];
            pPlayer->dirY = pScratch[0x25];
            pPlayer->dirZ = pScratch[0x26];
            pPlayer->unk44 = pScratch[0x27];
            nStep = func_80095414(&pPlayer->x, (s32 *)&pPlayer->dirX, pScratch + 0x24, pPlayer->moveSpeed << 0xC, g_WorldMapMode);
            if (nStep == 0) {
                pPlayer->dirZ = 0;
                pPlayer->dirX = 0;
            }
        }
        if (nStep == 1) {
            pScratch[0x25] = func_80093A5C(pScratch[0x24], pScratch[0x26]) + 0x18000;
            WorldMapFindZoneAtPosition(pScratch + 0x24, 0x40, 0x20, &g_WorldMapZoneProximity, &g_WorldMapZoneId);
            if (g_WorldMapZoneProximity != 2) {
                pPlayer->x = pScratch[0x24];
                pPlayer->y = pScratch[0x25];
                pPlayer->z = pScratch[0x26];
                pPlayer->w = pScratch[0x27];
                if ((pPlayer->dirX | pPlayer->dirZ) != 0) {
                    WorldMapUpdateRandomTimers();
                }
            } else {
                pPlayer->y = func_80093A5C(pPlayer->x, pPlayer->z) + 0x18000;
                pPlayer->unk60 = 0;
            }
        } else {
            pPlayer->y = func_80093A5C(pPlayer->x, pPlayer->z) + 0x18000;
        }
        *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
        g_WorldMapPlayerAngle = pPlayer->angle;
        WorldMapSelectAreaAtPosition(&pPlayer->x, 2);
        WorldMapSelectAreaRectByState();
        *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
        *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
        *(u16 *)0x1F8000A2 = pPlayer->angle;
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[0].matrix);
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[1].matrix);
        nTmp = pPlayer->stateTimer - 1;
        pPlayer->stateTimer = nTmp;
        if ((nTmp << 0x10) > 0) {
            *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
            *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
            *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
            WorldMapInitSpriteGroupSprites(3, (void *)0x1F8000A8, (void *)0x1F8000A0);
        } else {
            pPlayer->stateTimer = 0;
            WorldMapDeactivateSpriteGroup(3);
        }
        if ((pPlayer->dirX | pPlayer->dirZ) != 0) {
            *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
            *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
            *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
            WorldMapInitSpriteGroupSprites(4, (void *)0x1F8000A8, (void *)0x1F8000A0);
        } else {
            WorldMapDeactivateSpriteGroup(4);
            pPlayer->stateTimer = 0;
        }
        pPlayer->dirZ = 0;
        pPlayer->dirY = 0;
        pPlayer->dirX = 0;
        if (WorldMapGetTileHalfIndex(pScratch + 0x24) == 3) {
            WorldMapDeactivateSpriteGroup(3);
            WorldMapDeactivateSpriteGroup(4);
            pPlayer->state = 3;
        }
        D_8009BD04 = 0;
        break;
    case 8:
        nTmp = pPlayer->y - 0x8000;
        pPlayer->y = nTmp;
        if (nTmp <= 0xFFE20000) {
            pPlayer->y = -0x1E0000;
            if (pPlayer->command == 0xB) {
                pPlayer->unk64 = 0;
                pPlayer->state = 2;
                pPlayer->command = 0;
                WorldMapPlayBgmFromBuffer(g_WorldMapMode7SoundBankBuffer, g_WorldMapMode7SoundBankFileIndex);
            }
        }
    storeScroll:
        *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
        g_WorldMapPlayerAngle = pPlayer->angle;
        break;
    case 0xC:
        pHeight = pPlayer->pCameraPathHeights;
        nTmp = pPlayer->y + 0x800;
        pPlayer->y = nTmp;
        if (nTmp >= (s32)pHeight) {
            pPlayer->y = (s32)pHeight;
            pPlayer->state = 3;
            g_WorldMapGroups->entry[3].hidden = 1;
            g_WorldMapGroups->entry[2].hidden = 1;
            g_WorldMapGroups->entry[1].hidden = 1;
        }
        goto storeScroll;
    case 0x10:
        pHeight = pPlayer->pCameraPathHeights;
        nTmp = pPlayer->y + 0x8000;
        pPlayer->y = nTmp;
        if (nTmp >= (s32)pHeight) {
            pPlayer->y = (s32)pHeight;
            pPlayer->state = 1;
            WorldMapPlayBgmFromBuffer(g_WorldMapSoundBankBuffer, g_WorldMapSoundBankFileIndex);
            WorldMapStartSlot(8, 0xA);
            WorldMapStartSlot(9, 0xA);
            WorldMapStartSlot(0xA, 0xA);
            WorldMapStartSlot(4, 3);
            WorldMapStartSlot(1, 3);
            g_GameState.gearRide[0] = 1;
            if (g_GameState.partyMembers[1] != 0xFF) {
                WorldMapStartSlot(5, 3);
                WorldMapStartSlot(2, 3);
                g_GameState.gearRide[1] = 1;
            }
            if (g_GameState.partyMembers[2] != 0xFF) {
                WorldMapStartSlot(6, 3);
                WorldMapStartSlot(3, 3);
                g_GameState.gearRide[2] = 1;
            }
            pPlayer->dirZ = 0;
            pPlayer->dirY = 0;
            pPlayer->dirX = 0;
            pPlayer->unk74 = 0;
            g_GameState.worldMapState &= 0x3FFF;
            WorldMapDeactivateSpriteGroup(0);
            WorldMapDeactivateSpriteGroup(1);
        initRandomTimers:
            WorldMapInitRandomTimers();
        } else if ((nTmp > 0xFFE00000) && (pPlayer->pathIndex != 0)) {
            pPlayer->pathIndex = 0;
            *(s16 *)0x1F8000A0 = pPlayer->x >> 0xC;
            *(u16 *)0x1F8000A2 = (s32)pPlayer->pCameraPathHeights >> 0xC;
            *(u16 *)0x1F8000A4 = pPlayer->z >> 0xC;
            nTmp = 0x3D;
            if (WorldMapGetTileHalfIndex(&pPlayer->x) == 3) {
                nTmp = 0x3E;
            }
            WorldMapInitSpriteGroupSprites(nTmp, (void *)0x1F8000A0, NULL);
        }
    storeScrollY:
        g_WorldMapScrollTargetY = pPlayer->y;
        break;
    case 0x14:
        pHeight = pPlayer->pCameraPathHeights;
        nTmp = pPlayer->y - 0x800;
        pPlayer->y = nTmp;
        if ((s32)pHeight >= nTmp) {
            pPlayer->y = (s32)pHeight;
            pPlayer->state = 1;
            WorldMapStartSlot(8, 0xA);
            WorldMapStartSlot(1, 3);
            WorldMapStartSlot(4, 3);
            g_GameState.gearRide[0] = 1;
            if (g_GameState.partyMembers[1] != 0xFF) {
                WorldMapStartSlot(2, 3);
                WorldMapStartSlot(5, 3);
                g_GameState.gearRide[1] = 1;
            }
            if (g_GameState.partyMembers[2] != 0xFF) {
                WorldMapStartSlot(3, 3);
                WorldMapStartSlot(6, 3);
                g_GameState.gearRide[2] = 1;
            }
            pPlayer->dirZ = 0;
            pPlayer->dirY = 0;
            pPlayer->dirX = 0;
            pPlayer->unk74 = 0;
            g_GameState.worldMapState &= 0x3FFF;
            g_WorldMapGroups->entry[3].hidden = 0;
            g_WorldMapGroups->entry[2].hidden = 0;
            g_WorldMapGroups->entry[1].hidden = 0;
            goto initRandomTimers;
        }
        goto storeScrollY;
    case 0x20:
        pHeight = pPlayer->pCameraPathHeights;
        nTmp = pPlayer->y + 0x2000;
        pPlayer->y = nTmp;
        if (nTmp >= (s32)pHeight) {
            pPlayer->y = (s32)pHeight;
            WorldMapDeactivateSpriteGroup(2);
            g_WorldMapGroups->entry[3].hidden = 1;
            g_WorldMapGroups->entry[2].hidden = 1;
            g_WorldMapGroups->entry[1].hidden = 1;
            g_WorldMapGroups->entry[0].hidden = 1;
            pPlayer->state++;
            pScratch[0] = 0x04B40000;
            pScratch[2] = 0x01C00000;
            pScratch[1] = (s32)pPlayer->pCameraPathHeights;
            WorldMapBuildDirectionFromPositions(&pPlayer->x, pScratch, &pPlayer->dirX, &pPlayer->angle);
            pPlayer->unk50 = pScratch[0] >> 0xC;
            pPlayer->unk54 = pScratch[2] >> 0xC;
        }
        goto storeScroll;
    case 0x21:
        if (WorldMapMoveActorTowardTarget(pPlayer) == 3) {
            pPlayer->state++;
        }
        pPlayer->y = (s32)pPlayer->pCameraPathHeights;
        goto storeScroll;
    case 0x22:
        pPlayer->state = 0x40;
        goto idleMode;
    case 0x24:
        pPlayer->y = 0x30000;
        pPlayer->state++;
        pScratch[0] = 0x04937000;
        pScratch[2] = 0x021E2000;
        pScratch[1] = 0x30000;
        WorldMapBuildDirectionFromPositions(&pPlayer->x, pScratch, &pPlayer->dirX, &pPlayer->angle);
        pPlayer->unk50 = pScratch[0] >> 0xC;
        pPlayer->unk54 = pScratch[2] >> 0xC;
        *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
        g_WorldMapGroups->entry[3].hidden = 1;
        g_WorldMapGroups->entry[2].hidden = 1;
        g_WorldMapGroups->entry[1].hidden = 1;
        g_WorldMapGroups->entry[0].hidden = 1;
        *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
        g_WorldMapPlayerAngle = pPlayer->angle;
        *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
        *(u16 *)0x1F8000A2 = pPlayer->angle;
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[0].matrix);
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[1].matrix);
        break;
    case 0x25:
        if (WorldMapMoveActorTowardTarget(pPlayer) == 3) {
            pPlayer->y = 0x30000;
            pPlayer->state++;
            pPlayer->pCameraPathHeights = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z) + 0x18000;
            *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
            *(s16 *)0x1F8000AA = (s32)pPlayer->pCameraPathHeights >> 0xC;
            *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
            WorldMapInitSpriteGroupSprites(2, (void *)0x1F8000A8, (void *)0x1F8000A0);
            g_WorldMapGroups->entry[0].hidden = 0;
        }
        pPlayer->y = 0x30000;
        goto storeScroll;
    case 0x26:
        pHeight = pPlayer->pCameraPathHeights;
        nTmp = pPlayer->y - 0x2000;
        pPlayer->y = nTmp;
        if ((s32)pHeight >= nTmp) {
            pPlayer->y = (s32)pHeight;
            WorldMapDeactivateSpriteGroup(2);
            pPlayer->state = 3;
        }
        goto storeScroll;
    case 0x28:
        pPlayer->state++;
        pPlayer->y = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z) + 0x18000;
        pScratch[0] = D_8009B1AC[pPlayer->pathIndex * 4] << 0xC;
        pScratch[2] = D_8009B1B0[pPlayer->pathIndex * 4] << 0xC;
        WorldMapBuildDirectionFromPositions(&pPlayer->x, pScratch, &pPlayer->dirX, &pPlayer->angle);
        pPlayer->unk50 = pScratch[0] >> 0xC;
        pPlayer->targetAngle = pPlayer->angle;
        nStep = pPlayer->pathIndex + 1;
        pPlayer->pathIndex = nStep;
        pPlayer->unk54 = pScratch[2] >> 0xC;
        if (nStep == 2) {
            WorldMapStartSlot(9, 0xF);
        }
        nStep = pPlayer->pathIndex;
        if (nStep == 4) {
            pPlayer->stateTimer = 0x40;
            pPlayer->state++;
            WorldMapStartSlot(0, 0xD);
            g_WorldMapTpageAbr = 2;
            g_WorldMapDrawMode = nStep;
        }
        // fallthrough
    case 0x29:
        if (WorldMapMoveActorTowardTarget(pPlayer) == 3) {
            pPlayer->state--;
        }
    turnToTarget:
        nStep = pPlayer->targetAngle - g_GameState.scriptAngle;
        nTmp = nStep;
        if (nStep < 0) {
            nTmp = -nTmp;
        }
        if (nTmp >= 0x801) {
            if (nStep < 0) {
                nStep += 0x1000;
            } else {
                nStep -= 0x1000;
            }
        }
        nTmp = nStep;
        if (nStep < 0) {
            nTmp = -nTmp;
        }
        if (nTmp >= 0x11) {
            if (nStep < 0) {
                nTmp = g_GameState.scriptAngle - 0x10;
            } else {
                nTmp = g_GameState.scriptAngle + 0x10;
            }
            pPlayer->angle = nTmp;
        }
        pPlayer->angle &= 0xFFF;
        *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
        *(u16 *)0x1F8000A2 = pPlayer->angle;
        *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
        *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
        *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
        *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
        WorldMapInitSpriteGroupSprites(1, (void *)0x1F8000A8, (void *)0x1F8000A0);
        *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
        *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
        *(u16 *)0x1F8000A2 = pPlayer->angle;
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[0].matrix);
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[1].matrix);
        goto storeScroll;
    case 0x2A:
        nTmp = pPlayer->stateTimer - 1;
        pPlayer->stateTimer = nTmp;
        if (nTmp & 0x8000) {
            g_NextMapId = 0x50;
            g_NextMapFlags = 1;
            g_WorldMapLoopActive = 0;
            g_WorldMapAreaMode = 0;
            g_WorldMapExitFlag = 1;
            g_NextMapRotationY = g_WorldMapViewRotationY[0];
        }
        goto turnToTarget;
    case 0x30:
        pPlayer->state++;
        pPlayer->y = func_80093A5C(pPlayer->x, pPlayer->z) + 0x18000;
        pScratch[0] = D_8009B1D4[pPlayer->pathIndex * 4] << 0xC;
        pScratch[2] = D_8009B1D8[pPlayer->pathIndex * 4] << 0xC;
        WorldMapBuildDirectionFromPositions(&pPlayer->x, pScratch, &pPlayer->dirX, &pPlayer->angle);
        pPlayer->unk50 = pScratch[0] >> 0xC;
        pPlayer->targetAngle = pPlayer->angle;
        nStep = pPlayer->pathIndex + 1;
        pPlayer->pathIndex = nStep;
        pPlayer->unk54 = pScratch[2] >> 0xC;
        if (nStep == 2) {
            WorldMapStartSlot(9, 0x10);
        }
        if (pPlayer->pathIndex == 7) {
            pPlayer->stateTimer = 0x40;
            pPlayer->state++;
            WorldMapStartSlot(0, 0xD);
            g_WorldMapTpageAbr = 2;
            g_WorldMapDrawMode = 4;
        }
        // fallthrough
    case 0x31:
    case31Tail:
        if (WorldMapMoveActorTowardTarget(pPlayer) == 3) {
            pPlayer->state--;
        }
        nStep = pPlayer->targetAngle - g_GameState.scriptAngle;
        nTmp = nStep;
        if (nStep < 0) {
            nTmp = -nTmp;
        }
        if (nTmp >= 0x801) {
            if (nStep < 0) {
                nStep += 0x1000;
            } else {
                nStep -= 0x1000;
            }
        }
        nTmp = nStep;
        if (nStep < 0) {
            nTmp = -nTmp;
        }
        if (nTmp >= 0x11) {
            if (nStep < 0) {
                nTmp = g_GameState.scriptAngle - 0x10;
            } else {
                nTmp = g_GameState.scriptAngle + 0x10;
            }
            pPlayer->angle = nTmp;
        }
        pPlayer->angle &= 0xFFF;
        pPlayer->y = func_80093A5C(pPlayer->x, pPlayer->z) + 0x18000;
        *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
        *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
        *(u16 *)0x1F8000A2 = pPlayer->angle;
        *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
        *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
        *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
        WorldMapInitSpriteGroupSprites(4, (void *)0x1F8000A8, (void *)0x1F8000A0);
        *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
        *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
        *(u16 *)0x1F8000A2 = pPlayer->angle;
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[0].matrix);
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[1].matrix);
        goto storeScroll;
    case 0x32:
        nTmp = pPlayer->stateTimer - 1;
        pPlayer->stateTimer = nTmp;
        if (nTmp & 0x8000) {
            g_NextMapId = 0x120;
            g_NextMapFlags = 6;
            g_WorldMapLoopActive = 0;
            g_WorldMapAreaMode = 0;
            g_WorldMapExitFlag = 1;
            g_NextMapRotationY = g_WorldMapViewRotationY[0];
        }
        goto case31Tail;
    case 0x34:
        pPlayer->state++;
        WorldMapStartSlot(0xA, 0x11);
        pPlayer->y = -0x100000;
        pPlayer->angle = 0x200;
        pPlayer->dirX = rcos(0x200);
        pPlayer->dirZ = -rsin(pPlayer->angle);
        pPlayer->stateTimer = 0x5A;
        pPlayer->targetAngle = 0;
        *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
        *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
        *(u16 *)0x1F8000A2 = pPlayer->angle;
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[0].matrix);
        RotMatrixYXZ((SVECTOR *)0x1F8000A0, &g_WorldMapGroups->entry[1].matrix);
        // fallthrough
    case 0x35:
        nTmp = pPlayer->stateTimer - 1;
        pPlayer->stateTimer = nTmp;
        if (nTmp & 0x8000) {
            pPlayer->stateTimer = 0xB4;
            pPlayer->state++;
            WorldMapStartSlot(9, 0x11);
        }
    moveAndRise:
        pPlayer->x += pPlayer->dirX * 0xC;
        pPlayer->z += pPlayer->dirZ * 0xC;
        WorldMapWrapPosition(&pPlayer->x);
        nTmp = pPlayer->y + 0x1000;
        pPlayer->y = nTmp;
        if (nTmp >= -1 && nTmp <= 0x1FFFD) {
            *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
            *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
            *(u16 *)0x1F8000A2 = pPlayer->angle;
            *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
            *(s16 *)0x1F8000AA = pPlayer->y >> 0xC;
            *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
            WorldMapInitSpriteGroupSprites(0x3F, (void *)0x1F8000A8, (void *)0x1F8000A0);
        } else {
            WorldMapDeactivateSpriteGroup(0x3F);
        }
        goto storeScroll;
    case 0x36:
        nTmp = pPlayer->stateTimer - 1;
        pPlayer->stateTimer = nTmp;
        if (nTmp & 0x8000) {
            pPlayer->stateTimer = 0x78;
            pPlayer->state++;
            SoundPlaySoundEffectOnFreePair((((u16 *)g_MenuSedsFile)[0xA] << 0x10) | 0x38);
        }
        goto moveAndRise;
    case 0x37:
        nTmp = pPlayer->stateTimer - 1;
        pPlayer->stateTimer = nTmp;
        if (nTmp & 0x8000) {
            pPlayer->stateTimer = 0x5A;
            pPlayer->state++;
            WorldMapDeactivateSpriteGroup(0x2B);
        }
        pPlayer->x += pPlayer->dirX * 8;
        pPlayer->z += pPlayer->dirZ * 8;
        WorldMapWrapPosition(&pPlayer->x);
        pPlayer->y += 0x1000;
        if (pPlayer->state == 0x38) {
            WorldMapDeactivateSpriteGroup(0x2B);
        } else {
            *(s16 *)0x1F8000A0 = -(pPlayer->unk70 >> 0xC);
            *(u16 *)0x1F8000A4 = g_WorldMapViewRotationZ[0];
            *(u16 *)0x1F8000A2 = pPlayer->angle;
            *(s16 *)0x1F8000AA = 0;
            *(s16 *)0x1F8000A8 = pPlayer->x >> 0xC;
            *(s16 *)0x1F8000AC = pPlayer->z >> 0xC;
            WorldMapInitSpriteGroupSprites(0x2B, (void *)0x1F8000A8, (void *)0x1F8000A0);
        }
        goto storeScroll;
    case 0x38:
        nTmp = pPlayer->stateTimer - 1;
        pPlayer->stateTimer = nTmp;
        if (nTmp & 0x8000) {
            pPlayer->stateTimer = 0x80;
            pPlayer->state++;
            WorldMapStartSlot(0, 0xD);
            g_WorldMapTpageAbr = 2;
            g_WorldMapDrawMode = 4;
        }
    move:
        pPlayer->x += pPlayer->dirX * 8;
        pPlayer->z += pPlayer->dirZ * 8;
        goto storeScroll;
    case 0x39:
        nTmp = pPlayer->stateTimer - 1;
        pPlayer->stateTimer = nTmp;
        if (nTmp & 0x8000) {
            g_NextMapId = 0x1F0;
            g_WorldMapLoopActive = 0;
            g_WorldMapAreaMode = 0;
            g_NextMapFlags = 0;
            g_WorldMapExitFlag = 1;
            g_NextMapRotationY = g_WorldMapViewRotationY[0];
        }
        goto move;
    case 0x40:
        break;
    }
    nTmp = pPlayer->x >> 0xC;
    g_WorldMapGroups->entry[1].pos[0] = nTmp;
    g_WorldMapGroups->entry[0].pos[0] = nTmp;
    nTmp = pPlayer->y >> 0xC;
    g_WorldMapGroups->entry[1].pos[1] = nTmp;
    g_WorldMapGroups->entry[0].pos[1] = nTmp;
    nTmp = pPlayer->z >> 0xC;
    g_WorldMapGroups->entry[1].pos[2] = nTmp;
    g_WorldMapGroups->entry[0].pos[2] = nTmp;
    WorldMapSetGameStatePosition(&pPlayer->x);
    g_GameState.scriptAngle = pPlayer->angle;
    nState = pPlayer->state;
    if (nState == 8 || nState == 0x10 || nState == 2) {
        WorldMapAddSlotPosition(2, &pPlayer->x);
    }
    return nRet;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E76C);


s32 WorldMapModeInitActorRotation(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
    pPlayer = g_WorldMapSlots + idx;
    pPlayer->state = 0;
    *(Quad *)&pPlayer->x = *(Quad *)g_WorldMapGroups->entry[2].pos;
    *(Quad *)&pPlayer->dirX = *(Quad *)g_WorldMapGroups->entry[3].pos;
    nMode = g_WorldMapMode;
    pPlayer->unk50 = 0;
    pPlayer->unk54 = 0x40;
    if (nMode < 8) {
        if (nMode >= 4) {
            pPlayer->state = 3;
            pPlayer->unk5C = 0x80;
            pPlayer->unk58 = 0x80;
        }
    }
    return 1;
}

int WorldMapModeInitSpriteSets2And3(void) {
    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
    return 1;
}

// the two ramp counters (unk58, unk5C) climb and fall by this much per frame and stop at
// WORLD_MAP_ROT_RAMP_MAX; they are the per-frame steps added to the YXZ group rotation
#define WORLD_MAP_ROT_RAMP_STEP 4
#define WORLD_MAP_ROT_RAMP_MAX 0x80

s32 WorldMapUpdateSlotRotationRamp(s32 idx) {
    WorldMapPlayer *pPlayer;
    WorldMapRot *pRotA;
    WorldMapRot *pRotB;
    u16 *pScratchPos;

    pScratchPos = (u16 *)0x1F800000;
    pPlayer = g_WorldMapSlots + idx;
    pRotA = &g_WorldMapGroups->entry[2];
    pRotB = &g_WorldMapGroups->entry[3];
    if (pPlayer->command == 9) {
        pPlayer->command = 0;
        pPlayer->state = 1;
    } else if (pPlayer->command == 0xA) {
        pPlayer->command = 0;
        pPlayer->state = 2;
    }
    switch (pPlayer->state) {
    case 0:
        pPlayer->unk58 = 0;
        pPlayer->unk5C = 0;
        break;
    case 1:
        pPlayer->unk58 += WORLD_MAP_ROT_RAMP_STEP;
        if (pPlayer->unk58 >= 0x11) {
            pPlayer->unk5C += WORLD_MAP_ROT_RAMP_STEP;
        }
        if (pPlayer->unk58 >= WORLD_MAP_ROT_RAMP_MAX) {
            pPlayer->unk58 = WORLD_MAP_ROT_RAMP_MAX;
        }
        if (pPlayer->unk5C >= WORLD_MAP_ROT_RAMP_MAX) {
            pPlayer->unk5C = WORLD_MAP_ROT_RAMP_MAX;
        }
        if ((pPlayer->unk58 >= WORLD_MAP_ROT_RAMP_MAX) & (pPlayer->unk5C >= WORLD_MAP_ROT_RAMP_MAX)) {
            pPlayer->state = 3;
        }
        break;
    case 2:
        pPlayer->unk58 -= WORLD_MAP_ROT_RAMP_STEP;
        if (pPlayer->unk58 < 0x70) {
            pPlayer->unk5C -= WORLD_MAP_ROT_RAMP_STEP;
        }
        if (pPlayer->unk58 < 0) {
            pPlayer->unk58 = 0;
        }
        if (pPlayer->unk5C < 0) {
            pPlayer->unk5C = 0;
        }
        if ((pPlayer->unk58 == 0) & (pPlayer->unk5C == 0)) {
            pPlayer->state = 0;
        }
        break;
    }
    pPlayer->unk50 += pPlayer->unk58;
    pPlayer->unk54 -= pPlayer->unk5C;
    pScratchPos[0x54] = 0;
    pScratchPos[0x50] = 0;
    pScratchPos[0x51] = pPlayer->unk50;
    pScratchPos[0x55] = pPlayer->unk54;
    pScratchPos[0x52] = pRotA->rotZ;
    pScratchPos[0x56] = pRotB->rotZ;
    RotMatrixZYX((SVECTOR *)&pScratchPos[0x50], &pRotA->matrix);
    RotMatrixZYX((SVECTOR *)&pScratchPos[0x54], &pRotB->matrix);
    return 1;
}
void WorldMapUpdateL2R2PressEdge(void) {
    u16 keys;
    s32 state;

    keys = g_WorldMapPad1Held;
    if ((keys & 1) && (keys & 2)) {
        g_WorldMapL2R2Pressed = 1;
    } else {
        g_WorldMapL2R2Pressed = 0;
    }
    state = g_WorldMapL2R2Pressed;
    g_WorldMapL2R2PressEdge = (state ^ g_WorldMapL2R2PressedLast) & state;
    g_WorldMapL2R2PressedLast = state;
}
/* best: 99.92% (objdiff), 484 bytes = retail's exact size, every instruction identical; the
   only residual is the table operand (jtbl_80070B54 against .rodata) and the .L relocs, which
   the linker resolves to the same address.
   RE-MEASURED round 430 from this body: 99.92% / 484 bytes, and permute.sh returns
   PERMUTER_ZERO base=0 on its first iteration - the residual is the table's RELOCATION, so no
   C edit reaches it (lever i-permuter-zero-on-a-switch). Two things measured this round:
   a FUNCTION-scope `static const void *const pCases[]` of label addresses hoists to the top of
   the object's .rodata exactly like the file-scope one (cc1_probe with this unit's cc1 prints
   `.rdata / .align 2 / t.2:` ahead of both test functions' bodies), so the const-table route
   cannot buy order from inside the function either; and a 4-mod-8 ABSOLUTE table is
   unproducible by any cc1 shape, carve included, because cc1's `.align 3` both pads the table
   to an 8-aligned offset inside its section AND raises that section's sh_addralign to 8, so ld
   places the section 8-aligned too - the table is 8-aligned absolutely whatever the yaml says.
   0x80070B54 is 4 mod 8, so this one and its four neighbours stay asm.
   THE WALL IS RODATA ORDER AS WELL AS ALIGNMENT - both were measured this round, do not redo.
   This unit's .rodata (0x800709F8, 0x258 bytes) is nine jump tables and nothing else: E190's two
   at +0x00/+0x18, E4F4's at +0x38, E76C's 0x104-byte one at +0x58, then five at +0x15C/+0x18C/
   +0x1BC/+0x1EC/+0x214, i.e. 4 mod 8.
   1. switch (the body below): cc1's hardcoded .align 3 makes every C jump table 8-aligned, so
      the pad in front of this one pushes the overlay to 180426 bytes against disc's 180422.
   2. static const void *const pCases[] = { &&label, ... } (the only rodata cc1 emits .align 2,
      which IS 4-aligned) keeps the total at retail's 0x258 and reproduces every instruction,
      but cc1 hoists file-scope constants to the TOP of the .s, ahead of the stubs' .include'd
      asm tables: with these two functions converted that way the object's .rodata was
      0000=A84's twelve words, 0030=C68's (retail has E190's at 0000), and check_bin failed on
      content at file offset 0xF08 with both sizes equal. An inline switch table, by contrast,
      does land at its own function's position. So order is right for switch and alignment is
      right for a const table, and never both for one table.
   The reachable fix is all-or-nothing: all nine tables as const label tables (every size is a
   multiple of 4, so .align 2 reproduces +0x00/0x18/0x38/0x58/0x15C/0x18C/0x1BC/0x1EC/0x214
   exactly), or the sub-range from +0x15C carved into a unit of its own. Both wait on
   func_8008E190 (parked 99.79%), func_8008E76C and func_800914D0 converting first.
   Body notes: it is func_80090C68's body plus one arm - the released-0x20 test has an
   else-if (g_WorldMapAreaId != -1) that returns 1 when the area record's mode (0xE) is 1, and
   gcc keeps the -1 in v0 across both compares as retail does. angle is s16 at 0x48 (rcos/rsin
   read it with lh). g_WorldMapViewRotationY must be indexed ([0]) through the u16 array extern
   at the top of the file - the bare name truncates the address. The WorldMapRect typedef this
   body needs (identical to misc14's) is commented out below it.
// The area record g_WorldMapAreaRect points at: the layout src/world_map/main/misc14 walks.
typedef struct {
    // 0x0
    s16 left;
    // 0x2
    s16 top;
    // 0x4
    s16 width;
    // 0x6
    s16 height;
    // 0x8
    s16 kind;
    // 0xA
    s16 unkA;
    // 0xC
    u16 value;
    // 0xE
    s16 mode;
} WorldMapRect; // Size: 0x10

s32 func_80090A84(WorldMapPlayer *pPlayer) {
    switch (g_WorldMapPad1Held >> 12) {
    case 1:
        pPlayer->angle = g_WorldMapViewRotationY[0];
        break;
    case 2:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x400) & 0xFFF;
        break;
    case 3:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x200) & 0xFFF;
        break;
    case 4:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x800) & 0xFFF;
        break;
    case 6:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x600) & 0xFFF;
        break;
    case 8:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x400) & 0xFFF;
        break;
    case 9:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x200) & 0xFFF;
        break;
    case 12:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x600) & 0xFFF;
        break;
    }
    if (g_WorldMapPad1Held & 0xF000) {
        pPlayer->dirX = rcos(pPlayer->angle);
        pPlayer->dirZ = -rsin(pPlayer->angle);
    }
    if (g_WorldMapPad1Released & 0x20) {
        if (g_WorldMapZoneProximity != 0) {
            return 3;
        }
        if (g_WorldMapAreaId != WORLD_MAP_AREA_NONE) {
            return 1;
        }
    } else if (g_WorldMapAreaId != WORLD_MAP_AREA_NONE) {
        if (((WorldMapRect *)g_WorldMapAreaRect)->mode == 1) {
            return 1;
        }
    }
    if (g_WorldMapPad1Released & 0x10) {
        if ((g_WorldMapMode4AreaId == WORLD_MAP_AREA_NONE) && (g_WorldMapAreaId == g_WorldMapMode4AreaId)) {
            g_WorldMapMode4EntryTriggered = 1;
        }
    }
    WorldMapUpdateL2R2PressEdge();
    return 0;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090A84);

/* best: 99.91% (objdiff), 428 bytes = retail's exact size, instruction diff limited to the
   table operand (jtbl_80070B84 against .rodata) and the .L relocs of the j instructions, which
   the linker resolves to the same addresses. Re-measured this round from the body below.
   PARKED FOR THE WALL func_80090A84's note above documents and measured twice: its table sits
   at .rodata+0x18C (4 mod 8, so cc1's .align 3 pad in front of a C switch table shifts the
   overlay +4, 180426 against disc's 180422), and the 4-aligned alternative - a const table of
   label addresses - is hoisted to .rodata+0 instead of +0x18C, which fails on content. Nothing
   about this function can dodge that: it is a sibling of func_80090A84 (0.921 shape match),
   same switch and same table size.
   Its tail differs from func_80090A84's: a released 0x20 returns 1 only when
   g_WorldMapAreaId != -1, there is no else-if on the held branch, and it does not return 4.
s32 func_80090C68(WorldMapPlayer *pPlayer) {
    switch (g_WorldMapPad1Held >> 12) {
    case 1:
        pPlayer->angle = g_WorldMapViewRotationY[0];
        break;
    case 2:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x400) & 0xFFF;
        break;
    case 3:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x200) & 0xFFF;
        break;
    case 4:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x800) & 0xFFF;
        break;
    case 6:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x600) & 0xFFF;
        break;
    case 8:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x400) & 0xFFF;
        break;
    case 9:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x200) & 0xFFF;
        break;
    case 12:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x600) & 0xFFF;
        break;
    }
    if (g_WorldMapPad1Held & 0xF000) {
        pPlayer->dirX = rcos(pPlayer->angle);
        pPlayer->dirZ = -rsin(pPlayer->angle);
    }
    if (g_WorldMapPad1Released & 0x20) {
        if (g_WorldMapZoneProximity != 0) {
            return 3;
        }
        if (g_WorldMapAreaId != WORLD_MAP_AREA_NONE) {
            return 1;
        }
    }
    if (g_WorldMapPad1Released & 0x10) {
        if ((g_WorldMapMode4AreaId == WORLD_MAP_AREA_NONE) && (g_WorldMapAreaId == g_WorldMapMode4AreaId)) {
            g_WorldMapMode4EntryTriggered = 1;
        }
    }
    WorldMapUpdateL2R2PressEdge();
    return 0;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090C68);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090E14);
/* best: 99.90% (objdiff), 416 bytes = retail's exact size (104 insns), and permute.sh reports
   PERMUTER_ZERO base=0 in one iteration: the body below is instruction-identical to retail,
   the only residual being the switch table operand (jtbl_80070BB4 against .rodata) and the .L
   relocs the linker resolves. NOT convertible as C: the table's ABSOLUTE address is
   0x80070BB4, which is 4 mod 8, cc1 hardcodes .align 3 in front of a C switch table, and
   misc12's .rodata starts 8-aligned at 0x800709F8, so the table can only land at
   .rodata+0x1C0 and the unit's rodata grows 4 bytes. Measured this round with the body live:
   check_bin.sh world_map gives build/out/world_map.bin 180426 against disc's 180422 with
   147397 differing bytes, the first at 0x4 (disc 0x80072878 against build 0x8007287C) because
   text start moved 4 and every rodata code pointer was rewritten. The arithmetic test in
   levers i-rodata-head-carve-moves and i-jtbl-align-3-hardcoded is "a table converts iff its
   absolute address is 8-aligned": misc12's eight jtbl dlables sit at 0x800709F8/0x80070A10/
   0x80070A50 (owned by stubs) and 0x80070B54/0x80070B84/0x80070BB4/0x80070BE4/0x80070C0C,
   i.e. the five tables from +0x15C are 4 mod 8 and are not convertible under any C spelling,
   so nothing more comes out of this unit's switch family. The const-label-table alternative (lever
   i-const-label-table-is-4-aligned-but-hoisted, round 417) is all-or-nothing: it needs every
   table of the unit converted, so it waits on func_8008E190, func_8008E76C and func_800914D0.
   Body notes: it is func_80090C68's switch with a different tail - a released 0x20 returns 1
   only when g_WorldMapAreaId is not WORLD_MAP_AREA_NONE, a released 0x40 returns 4, and it
   does not call WorldMapUpdateL2R2PressEdge. angle is s16 at 0x48 (rcos/rsin read it with lh)
   and g_WorldMapViewRotationY must be indexed through the u16 array extern at the top of the
   file.
s32 func_80090E14(WorldMapPlayer *pPlayer) {
    switch (g_WorldMapPad1Held >> 12) {
    case 1:
        pPlayer->angle = g_WorldMapViewRotationY[0];
        break;
    case 2:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x400) & 0xFFF;
        break;
    case 3:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x200) & 0xFFF;
        break;
    case 4:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x800) & 0xFFF;
        break;
    case 6:
        pPlayer->angle = (g_WorldMapViewRotationY[0] + 0x600) & 0xFFF;
        break;
    case 8:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x400) & 0xFFF;
        break;
    case 9:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x200) & 0xFFF;
        break;
    case 12:
        pPlayer->angle = (g_WorldMapViewRotationY[0] - 0x600) & 0xFFF;
        break;
    }
    if (g_WorldMapPad1Held & 0xF000) {
        pPlayer->dirX = rcos(pPlayer->angle);
        pPlayer->dirZ = -rsin(pPlayer->angle);
    }
    if (g_WorldMapPad1Released & 0x20) {
        if (g_WorldMapAreaId != WORLD_MAP_AREA_NONE) {
            return 1;
        }
    }
    if (g_WorldMapPad1Released & 0x40) {
        return 4;
    }
    if (g_WorldMapPad1Released & 0x10) {
        if ((g_WorldMapMode4AreaId == WORLD_MAP_AREA_NONE) && (g_WorldMapAreaId == g_WorldMapMode4AreaId)) {
            g_WorldMapMode4EntryTriggered = 1;
        }
    }
    return 0;
}
*/
// best: 98.71%, 1148 bytes / 287 insns = retail's exact size, every branch offset and every
// relocation right, frame and saved set exact (0x28 with ra/s1/s0, pPlayer in s1,
// &g_WorldMapViewRotationY across the two rcos calls in s0).
// Closed in round 405: the abs in the two ramp blocks. Retail's `bgez / move rd,rs /
// negu rd,rd` pair is what cc1 emits for `__builtin_abs(x)`, not for any hand-written
// ternary or if/else: every copy-variable spelling (`nAbs = (d < 0) ? -d : d;`, an if/else
// either polarity, `nAbs = d; if (...) nAbs = -nAbs;`) is coalesced into ONE pseudo before
// register allocation (a -dc dump shows `(reg 87) = 107 - 82` then `(reg 87) = -(reg 87)`),
// so reorg fills the bgez slot with a duplicate `slt` and the body is one instruction short
// of retail's shape. `__builtin_abs` keeps the copy: 98.03 -> 98.52. That also needs TWO abs
// locals, one per ramp block (`nAbs`, `nAbsVel`): a single shared one gives both blocks the
// same register where retail uses a1 then a0 - two locals, 98.52 -> 98.71, and the whole
// second ramp block (0x5C, the 16.16 velocity ramp) is then instruction-for-instruction
// retail's, `move a0,v0 / negu a0,a0` included.
// What is left is ONE register rotation inside `case 1:` and nothing else: retail keeps
// nTurn in $v1 (and the pad value in $a0), this body keeps nTurn in $a0 (pad in $v1), and
// the rest of the block's caller-saved names follow (nTol t2 vs t1, pRotZ t1 vs t2, nAngle
// a0 + copy a2 vs a1 + copy v1). It is a global-alloc priority contest for $a0 between the
// pad value and nTurn; measured and unchanged by: eight declaration orders, `u32 nPad`
// instead of `u16`, reading g_WorldMapPad1Held inline at all five tests instead of through
// the local (cse merges the loads either way), block-scoped locals inside `case 1:` (98.01),
// the `if (d >= 0)` if/else abs (95.78, 292 insns), gcc-2.7.2-cdk (93.66, and it drops three
// matched neighbours off 100, so the preset is right), and a 4-minute permuter run from the
// 98.03 body - not re-run on this one, so a fresh permute is a fair next attempt.
// Spellings that ARE load-bearing, measured one at a time from 91.2% to here:
// - `extern s16 g_WorldMapViewRotationY[];` and `pRotY[0]` for both rcos arguments. With a
//   scalar extern gcc uses the %hi/%lo macro form twice and the function loses a saved
//   register (1144 bytes, s1 never used).
// - `pRotZ = g_WorldMapViewRotationZ;` (array decay, as a statement, not `&global`): that is
//   what makes retail's lui+addiu into t1 with the lh and the sh both through it.
// - inside case 1 the FIRST initialisation is `nVel = 0`, which is the one reorg puts in
//   the dispatch's branch delay slot, as retail does. nTurn = 0 first puts nTurn there.
// - the tail is `pPlayer->angle = (pPlayer->unk58 >> 12) & 0xFFF;` followed by
//   `g_WorldMapViewRotationY[0] = pPlayer->angle;`. Reading the field back keeps the shift
//   on the sum's register after the 0x58 store; a separate angle local puts `sra` into a
//   fresh register before the store (97.4%).
// - the ramp's add/sub is a conditional expression, `(x < t) ? (x + s) : (x - s)`, which is
//   what emits retail's bnez / addu / subu / shared store.
// - the 0x80 block really does test g_WorldMapPad1Held & 0x80 twice (retail re-reads the
//   global between the step and the clamp).
// - the first ramp block keeps the delta in place (`if (nDelta < 0) nDelta = -nDelta;` then
//   `nStep = nDelta >> 4`), which is retail's bare `negu v0,v0` with no copy - only the two
//   later ramp blocks use __builtin_abs.
// Field evidence for the struct: 0x64 is a phase counter here (0 then 1), not the camera-path
// pointer the camera modes store in it; 0x4A moveSpeed is read with `lh` (so s16); 0x6C is
// compared with slti (s32).
// No jtbl in this stub, so parking it costs no layout work.
// Mode 1's per-frame slot update: ramps the slot's yaw (0x58, a 12.12 fixed-point angle,
// with 0x5C as its angular velocity) toward the direction the pad asks for, tilts the pitch
// accumulator 0x70 up/down, and rebuilds the slot's forward vector from both. The command
// byte 0x64 is the phase: 0 seeds the ramp from the current angle, 1 runs it.
// best: 99.56%, 1148 bytes / 287 insns = retail's exact size, frame and saved set exact.
// Round 476 took this from 98.71 to 99.56 with two edits, both measured:
// - write the yaw ramp's store as 'ptr = cond ? a+b : a-b;' instead of assigning the
//   same value to a local and then storing it (the ternary's result is what retail
//   stores, and the local was one pseudo too many for local-alloc): 92.10-era note's
//   'keep the shift on the sum register' hint was about a different arm.
// - the clamp pair at 0x60 needs TWO locals, nLimit and nRange: retail keeps the
//   negative limit in $v0 and the positive one in $v1, which one shared local cannot do
//   (a single nLimit gave both the same register). That edit alone took 99.46 -> 99.56.
// Left, and measured:
// (1) the 'nTol = 0x1000' constant: retail emits 'li t2,4096' in the lh load-delay
//     slot and a nop in the bgez slot, this body the reverse. Source order of the
//     assignment does not move it (tried before and after the nDelta computation).
// (2) ONE branch target: retail's second 'if (g_WorldMapPad1Held & 0x80)' (the clamp
//     guard, a re-read of the global after the 0x60 store) jumps to the else-if ramp
//     block at 0x340 when it fails; this body jumps past the ramp to 0x368. Every other
//     instruction, both other branch targets included, is retail's. Duplicating the ramp
//     as an 'else if (unk60 != 0)' inside the pad block does NOT get cross-jumped
//     (295 insns / 1180 bytes / 96.78), so retail's source had one ramp block reached
//     from both the outer and the inner test - not expressible as nested if/else.
// A 4-minute permuter run from the 98.71 body found the ramp-store edit above; a second
// run from this body is a fair next attempt. gcc-2.7.2-cdk drops three matched
// neighbours and scores 93.66, so the preset is right.
/*
s32 func_80090FB4(WorldMapPlayer *pPlayer) {
    s16 *pRotY;
    s16 *pRotZ;
    s16 nAngle;
    s32 nTurn;
    s32 nVel;
    s32 nStep;
    s32 nTol;
    s32 nDelta;
    s32 nLimit;
    s32 nRange;
    s32 nAbs;
    s32 nAbsVel;
    u32 nPad;

    switch (pPlayer->unk64) {
    case 0:
        pPlayer->unk64 = 1;
        pPlayer->unk5C = 0;
        pPlayer->unk58 = pPlayer->angle << 12;
        break;
    case 1:
        nVel = 0;
        nTurn = 0;
        nStep = 8;
        nTol = 0x1200;
        nPad = g_WorldMapPad1Held;
        if (nPad & 0x8000) {
            nTurn -= 0x60;
            nVel -= 0x10000;
        }
        if (nPad & 0x2000) {
            nTurn += 0x60;
            nVel += 0x10000;
        }
        if (nPad & 0x4) {
            nTurn -= 0x60;
            nVel -= 0x10000;
        }
        if (nPad & 0x8) {
            nTurn += 0x60;
            nVel += 0x10000;
        }
        if (nPad & 0xA00C) {
            nDelta = g_WorldMapViewRotationZ[0] - nTurn;
            nTol = 0x1000;
            if (nDelta < 0) {
                nDelta = -nDelta;
            }
            nStep = nDelta >> 4;
        }
        pRotZ = g_WorldMapViewRotationZ;
        nAngle = *pRotZ;
        if (nAngle != nTurn) {
            nDelta = nAngle - nTurn;
            nAbs = __builtin_abs(nDelta);
            if (nAbs < nStep) {
                nStep = nAbs;
            }
            *pRotZ = (nAngle < nTurn) ? (nAngle + nStep) : (nAngle - nStep);
        }
        if (pPlayer->unk5C != nVel) {
            nDelta = pPlayer->unk5C - nVel;
            nAbsVel = __builtin_abs(nDelta);
            if (nAbsVel < nTol) {
                nTol = nAbsVel;
            }
            if (pPlayer->unk5C < nVel) {
                pPlayer->unk5C += nTol;
            } else {
                pPlayer->unk5C -= nTol;
            }
        }
        break;
    }
    pPlayer->unk58 += pPlayer->unk5C;
    pPlayer->angle = (pPlayer->unk58 >> 12) & 0xFFF;
    g_WorldMapViewRotationY[0] = pPlayer->angle;
    switch (pPlayer->unk6C) {
    case 0:
        pPlayer->unk6C = 1;
        pPlayer->unk70 = 0;
        break;
    case 1:
        if (pPlayer->unk70 != 0) {
            if (pPlayer->unk70 < 0) {
                pPlayer->unk70 += 0x2000;
            } else {
                pPlayer->unk70 -= 0x2000;
            }
        }
        if (g_WorldMapPad1Held & 0x4000) {
            pPlayer->unk6C = 2;
        }
        if (g_WorldMapPad1Held & 0x1000) {
            pPlayer->unk6C = 3;
        }
        break;
    case 2:
        if (g_WorldMapPad1Held & 0x4000) {
            pPlayer->unk70 -= 0x4000;
            if (pPlayer->unk70 < -0x80000) {
                pPlayer->unk70 = -0x80000;
            }
        } else {
            pPlayer->unk6C = 1;
        }
        break;
    case 3:
        if (g_WorldMapPad1Held & 0x1000) {
            pPlayer->unk70 += 0x4000;
            if (pPlayer->unk70 > 0x80000) {
                pPlayer->unk70 = 0x80000;
            }
        } else {
            pPlayer->unk6C = 1;
        }
        break;
    }
    if (g_WorldMapPad1Held & 0x80) {
        if (g_WorldMapPad1Held & 0x2) {
            pPlayer->unk60 -= 0x1000;
        } else {
            pPlayer->unk60 += 0x1000;
        }
        if (g_WorldMapPad1Held & 0x80) {
            nLimit = -pPlayer->moveSpeed << 12;
            if (pPlayer->unk60 < nLimit) {
                pPlayer->unk60 = nLimit;
            }
            nRange = pPlayer->moveSpeed << 12;
            if (nRange < pPlayer->unk60) {
                pPlayer->unk60 = nRange;
            }
        }
    } else if (pPlayer->unk60 != 0) {
        if (pPlayer->unk60 < 0) {
            pPlayer->unk60 += 0x1000;
        } else {
            pPlayer->unk60 -= 0x1000;
        }
    }
    pRotY = g_WorldMapViewRotationY;
    pPlayer->dirX = rcos(pRotY[0]);
    pPlayer->dirY = rcos(pPlayer->unk70 >> 12);
    pPlayer->dirZ = -rsin(pRotY[0]);
    VectorNormal((VECTOR *)&pPlayer->dirX, (VECTOR *)&pPlayer->dirX);
    if (g_WorldMapPad1Released & 0x20) {
        if (g_WorldMapAreaId != -1) {
            return 1;
        }
    }
    if (g_WorldMapPad1Released & 0x40) {
        if ((pPlayer->unk60 == 0) & (g_WorldMapViewRotationZ[0] == 0)) {
            return 4;
        }
    }
    if (g_WorldMapPad1Released & 0x10) {
        if ((g_WorldMapMode4AreaId == -1) && (g_WorldMapAreaId == g_WorldMapMode4AreaId)) {
            g_WorldMapMode4EntryTriggered = 1;
        }
    }
    return 0;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090FB4);

s32 WorldMapModeInitSlotPositionAngle(s32 idx) {
    WorldMapPlayer *pPlayer;
    s16 *pRotY;
    s32 nMode;
    u16 nAngle;

    pPlayer = g_WorldMapSlots + idx;
    *(Quad *)&pPlayer->x = *(Quad *)g_WorldMapScrollTargetX;
    nAngle = g_WorldMapPlayerAngle;
    pRotY = g_WorldMapViewRotationY;
    g_WorldMapViewRotationZ[0] = 0;
    *(volatile s16 *)pRotY = nAngle;
    pPlayer->unk50 = (s16)nAngle;
    pPlayer->unk58 = *pRotY << 12;
    nMode = g_WorldMapMode;
    if (nMode < 8) {
        if (nMode >= 6) {
            pPlayer->state = 3;
        }
    }
    return 1;
}


// The scroll delta buffer WorldMapWrapDelta takes: the wrapped xyz offset at 0x00 and
// its 1/8 step at 0x10 (the type src/world_map/main/misc3.c uses for the same callee).
typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 pad0C;
    /* 0x10 */ s32 stepX;
    /* 0x14 */ s32 stepY;
    /* 0x18 */ s32 stepZ;
} WorldMapScrollDelta; // Size: 0x1C

// The view yaw is read signed here (lh) and stored with sh.
extern s16 g_WorldMapViewRotationYAngle[] __asm__("g_WorldMapViewRotationY");
extern s32 g_WorldMapScrollX[];
extern s32 g_WorldMapScrollZ;
extern s32 g_WorldMapScrollTargetZ;
extern void WorldMapWrapDelta(WorldMapScrollDelta *pDelta);
extern void WorldMapStartSlot(s32, s32);

#define WORLD_MAP_SCROLL_SNAP_STEP 64

// best: 99.76% (objdiff), 1668 bytes / 417 insns = retail's exact size (nm -S on both
// objects: 0x684). Whole structure, both switches, both ramps and both walk tails decode;
// the residual is one allocator tie inside the two ramp blocks plus the two jtbl operands.
// CANNOT BE CONVERTED HERE (JTBL_LAYOUT, and the cause is ALIGNMENT, not placement: the
// old note here said cc1 puts C rodata before asm rodata, which is false for jump tables -
// cc1 emits each table in .rdata at its own function's point in the text order, and this
// object's tables do come out in retail's order). The unit's .rodata range is
// [0xF08,0x1160) and retail's two tables sit at +0x1EC / +0x214 (0x80070BE4 / 0x80070C0C),
// which are both 4 mod 8, while cc1 emits `align 3` (8-byte) before every switch table.
// Measured with this body live: build/src/world_map/main/misc12.c.o has .rodata 0x25C
// against retail's 0x258, a 4-byte zero pad at internal 0x1EC and the tables at 0x1F0 /
// 0x218; check_bin.sh world_map then prints MISMATCH (build/out/world_map.bin 180426 bytes
// against disc's 180422, every address table in the overlay off by +4) and culprit.sh
// reports 147395 differing bytes in 450 symbols. Converting the neighbours cannot fix it:
// retail's tables are packed 4-byte all the way down the chain (func_8008E76C's 0x104-byte
// table ends at +0x15C, itself 4 mod 8, so func_80090A84's table can never be a C table
// either). Not a preset question: gcc-2.6.0 and gcc-2.7.2-cdk emit the same `align 3`
// (cc1_probe.sh on a switch snippet). The route that IS open is the unit split sketched in
// tools/rounds/levers.md under i-jtbl-align-3-hardcoded: move func_80090A84/C68/E14/914D0
// and everything after them into a new main/misc12_b with [0x1064, .rodata] + [c] ranges,
// whose .rodata then starts at parity 4 and puts all five tail tables at retail's offsets.
// That is a config/world_map.yaml + gears.toml + new-file change, so it is not this lane's.
// The frame here is retail's exactly, so only these 4 bytes stand between this body and the
// link; no C spelling can move an assembler alignment pad, which is why no permuter run was
// made for the remaining 0.24% (the register tie below: both ramp blocks, retail's in-place
// `sll v1,v1,0xc / sra v1,v1,0x3` on nDelta against our fresh-`v0` shift).
// Facts a later agent should not re-derive:
// - The two walk tails were 2 insns short (1656, 96.66%) because cc1 hoists the final
//   `pPlayer->z = g_WorldMapScrollTargetZ` statement to the top of the arm (its load fills
//   the `lw v1,16(sp)` slot) while retail keeps `lui/lw/nop/sw` at the end. An empty
//   `do { } while (0);` between the `pPlayer->x = ...` store and that z store pins it in
//   both walk arms -> retail's exact 1668 bytes. (Being precise about which of the two is
//   barriered matters: a barrier after the scrollZ store instead batches the loads and
//   measures 95.98%.)
// - The two ramp arms' subtraction wants TWO statements, not one:
//   `nDelta = g_WorldMapViewRotationYAngle[0]; nDelta = pPlayer->unk50 - nDelta;` -> 99.76
//   (found by a 3-minute permuter run, base 140 -> 110). The one-line form is 99.62.
// - Rejected this round: `nDelta = (nDelta << 12) >> 3; nYaw = (pPlayer->unk58 + nDelta) &
//   0xFFFFFF;` to get retail's in-place `sll v1,v1,0xc` — costs an instruction (1672, 97.90).
// - `__builtin_abs` on the STRUCT FIELD (not on a local, not the in-place `if (x<0) x=-x`)
//   is what reproduces retail's `bgez / move / negu` in both ramp blocks, one abs local per
//   block (a shared local mirrors the two blocks' registers).
// - `extern s32 g_WorldMapScrollX[];` + `g_WorldMapScrollX[0] += ...` is retail's
//   base-register form at all three walk sites; a scalar extern gives the `lui at` macro.
//   Z (and the target Y/Z) really are scalars in retail's code.
// - Both switches must list EVERY case or gcc emits a decision tree: `command - 9` cases
//   0..8 (2-5 empty) and `state` cases 0..0x10 (3-0xF empty), with `s16 nCommand;
//   nCommand = pPlayer->command - 9;` giving retail's lhu + sll/sra pair. States 0 and 1
//   share ONE copy of the ramp via the `ramp:` label + `goto ramp;` (gcc 2.7.2 has no
//   cross-jumping). The position walk covers states 0,1,2,0x10; state 3 has its own y-only
//   walk. The view yaw is s16 at 0x48 (rcos/rsin read it with lh) and must be indexed
//   ([0]) through the u16 array extern at the top of the file — the bare name truncates.
// - The WorldMapScrollDelta typedef, the s16 alias for g_WorldMapViewRotationY and the
//   externs above this stub exist so the parked body below still compiles.
// ROUND 532 re-measured the stored body from the live C: 99.76% / 1668 bytes / 417 insns =
// retail's exact size, and the whole stream differs in exactly two places - the two
// `lw %lo(jtbl)` operands and five instructions in EACH ramp block, a pure caller-saved
// tie. Retail shifts nDelta in place in v1 and gives the unk58 load v0
// (`sll v1,v1,0xc / lw v0,88(s0) / sra v1,v1,0x3 / addu v0,v0,v1 / and v0,v0,a0`), ours
// makes a temp in v0 and lets the load take the freed v1
// (`sll v0,v1,0xc / lw v1,88(s0) / sra v0,v0,0x3 / addu v1,v1,v0 / and v0,v1,a0`).
// permute.sh 3 min / 1309 iterations: PERMUTER_NONE base=80 best=80, so no spelling of
// this body moves it. Re-tried and rejected here: the in-place form
// `nDelta = (nDelta << 12) >> 3; pPlayer->unk58 = (pPlayer->unk58 + nDelta) & 0xFFFFFF;`
// -> 1672 bytes / 97.81% (nDelta lands in a1 and the shift copies back through v0, +1
// insn) and the swapped sum `(((nDelta << 12) >> 3) + pPlayer->unk58)` -> 1672 / 99.21%.
// The state-0x10 block carries the same tie with `>> 5`, so one spelling has to close both.
// The +4 below was re-measured first-hand this round with the body live: check_bin.sh
// world_map gives build/out/world_map.bin 180426 against disc's 180422, first differing
// byte at file 0x4. The split that fixes it (i-jtbl-align-3-hardcoded, unchanged):
// `[0xF08, .rodata, main/misc12]` + `[0x1064, .rodata, main/misc12_b]` and
// `[0x1E504, c, main/misc12]` + `[0x20F94, c, main/misc12_b]` (func_80090A84's file
// offset), with func_80090A84/90FB4/90C68/90E14/914D0/91C18/91FF8/925A0 moved into
// src/world_map/main/misc12_b.c. misc12.c.o's rodata then tiles to exactly 0x15C - E190's
// two asm tables 0x18+0x20, E4F4's C table 0x20 at +0x38, E76C's asm 0x104 at +0x58 - so
// WorldMapModeInitGroupRotations must stay in misc12.c, and the new range starts at parity
// 4 with its five tables at internal 0/0x30/0x60/0x90/0xB8 = retail's 0x1064/0x1094/
// 0x10C4/0x10F4/0x111C and no pad. That un-parks four bodies at once (A84 99.92,
// C68 99.91, E14 99.90, 914D0 99.76).
/*
s32 func_800914D0(s32 idx) {
    WorldMapPlayer *pPlayer;
    WorldMapScrollDelta delta;
    s16 nCommand;
    s16 nRotY;
    s32 nDelta;
    s32 nAbs;
    s32 nYaw;
    u16 nTurn;

    pPlayer = g_WorldMapSlots + idx;
    nCommand = pPlayer->command - 9;
    switch (nCommand) {
    case 0:
        pPlayer->command = 0;
        pPlayer->state = 2;
        pPlayer->unk50 = g_WorldMapPlayerAngle;
        nRotY = g_WorldMapViewRotationYAngle[0];
        pPlayer->unk58 = nRotY << 12;
        break;
    case 1:
        pPlayer->command = 0;
        pPlayer->state = 0;
        nRotY = g_WorldMapViewRotationYAngle[0];
        pPlayer->unk50 = nRotY;
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        break;
    case 6:
        pPlayer->state = 0x10;
        pPlayer->command = 0;
        pPlayer->unk50 = 0x800;
        break;
    case 7:
        pPlayer->state = 0x10;
        pPlayer->command = 0;
        pPlayer->unk50 = 0xA00;
        break;
    case 8:
        pPlayer->state = 0x10;
        pPlayer->command = 0;
        pPlayer->unk50 = 0xC00;
        break;
    }
    switch (pPlayer->state) {
    case 0:
        nTurn = (g_WorldMapPad1Held >> 2) & 3;
        switch (nTurn) {
        case 1:
        case 3:
            pPlayer->state = 1;
            pPlayer->unk54 = -0x40;
            pPlayer->unk5C = (pPlayer->unk50 - 0x200) & 0xFFF;
            break;
        case 2:
            pPlayer->state = 1;
            pPlayer->unk54 = 0x40;
            pPlayer->unk5C = (pPlayer->unk50 + 0x200) & 0xFFF;
            break;
        }
    // States 0 and 1 both end by ramping the view yaw toward the slot's target yaw; the
    // label is the shared tail (retail has one copy of it, reached by a jump from state 1).
    ramp:
        nRotY = g_WorldMapViewRotationYAngle[0];
        if (nRotY != pPlayer->unk50) {
            nDelta = pPlayer->unk50 - nRotY;
            nAbs = __builtin_abs(nDelta);
            if (nAbs >= 0xC01) {
                if (nDelta < 0) {
                    nDelta += 0x1000;
                } else {
                    nDelta -= 0x1000;
                }
            }
            pPlayer->unk58 = (pPlayer->unk58 + ((nDelta << 12) >> 3)) & 0xFFFFFF;
            g_WorldMapViewRotationYAngle[0] = pPlayer->unk58 >> 12;
        } else {
            pPlayer->unk58 = nRotY << 12;
        }
        break;
    case 1:
        pPlayer->unk50 = (pPlayer->unk50 + pPlayer->unk54) & 0xFFF;
        if (pPlayer->unk50 == pPlayer->unk5C) {
            pPlayer->state = 0;
        }
        goto ramp;
    case 2:
        nDelta = g_WorldMapViewRotationYAngle[0];
        nDelta = pPlayer->unk50 - nDelta;
        nAbs = __builtin_abs(nDelta);
        if (nAbs >= 0x801) {
            if (nDelta < 0) {
                nDelta += 0x1000;
            } else {
                nDelta -= 0x1000;
            }
        }
        nAbs = __builtin_abs(nDelta);
        if (nAbs >= 0x181) {
            if (nDelta < 0) {
                nDelta = -0x180;
            } else {
                nDelta = 0x180;
            }
        }
        nYaw = (pPlayer->unk58 + ((nDelta << 12) >> 3)) & 0xFFFFFF;
        pPlayer->unk58 = nYaw;
        g_WorldMapViewRotationYAngle[0] = nYaw >> 12;
        if (((nYaw >> 12) == pPlayer->unk50) & (pPlayer->unk60 == 0)) {
            pPlayer->state = 3;
            WorldMapStartSlot(7, 0xB);
        }
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
        break;
    case 0x10:
        nDelta = g_WorldMapViewRotationYAngle[0];
        nDelta = pPlayer->unk50 - nDelta;
        nAbs = __builtin_abs(nDelta);
        if (nAbs >= 0x801) {
            if (nDelta < 0) {
                nDelta += 0x1000;
            } else {
                nDelta -= 0x1000;
            }
        }
        nAbs = __builtin_abs(nDelta);
        if (nAbs >= 0x181) {
            if (nDelta < 0) {
                nDelta = -0x180;
            } else {
                nDelta = 0x180;
            }
        }
        nYaw = (pPlayer->unk58 + ((nDelta << 12) >> 5)) & 0xFFFFFF;
        pPlayer->unk58 = nYaw;
        g_WorldMapViewRotationYAngle[0] = nYaw >> 12;
        if (((nYaw >> 12) == pPlayer->unk50) & (pPlayer->unk60 == 0)) {
            pPlayer->state = 3;
        }
        break;
    }
    switch (pPlayer->state) {
    case 0:
    case 1:
    case 2:
    case 0x10:
        if ((pPlayer->x != (s32)g_WorldMapScrollTargetX[0]) | (pPlayer->y != g_WorldMapScrollTargetY) |
            (pPlayer->z != g_WorldMapScrollTargetZ)) {
            delta.x = (s32)g_WorldMapScrollTargetX[0] - pPlayer->x;
            delta.y = g_WorldMapScrollTargetY - pPlayer->y;
            delta.z = g_WorldMapScrollTargetZ - pPlayer->z;
            WorldMapWrapDelta(&delta);
            delta.stepX = delta.x >> 3;
            delta.stepY = delta.y >> 3;
            delta.stepZ = delta.z >> 3;
            if ((__builtin_abs(delta.stepX) < WORLD_MAP_SCROLL_SNAP_STEP) & (__builtin_abs(delta.stepZ) < WORLD_MAP_SCROLL_SNAP_STEP)) {
                pPlayer->unk60 = 0;
                g_WorldMapScrollX[0] += delta.x;
                g_WorldMapScrollZ += delta.z;
                pPlayer->x = (s32)g_WorldMapScrollTargetX[0];
                do {
                } while (0);
                pPlayer->z = g_WorldMapScrollTargetZ;
            } else {
                pPlayer->unk60 = 1;
                pPlayer->x += delta.stepX;
                pPlayer->z += delta.stepZ;
                g_WorldMapScrollX[0] += delta.stepX;
                g_WorldMapScrollZ += delta.stepZ;
            }
            pPlayer->y += delta.stepY;
        }
        break;
    case 3:
        if ((pPlayer->x != (s32)g_WorldMapScrollTargetX[0]) | (pPlayer->y != g_WorldMapScrollTargetY) |
            (pPlayer->z != g_WorldMapScrollTargetZ)) {
            delta.x = (s32)g_WorldMapScrollTargetX[0] - pPlayer->x;
            delta.y = g_WorldMapScrollTargetY - pPlayer->y;
            delta.z = g_WorldMapScrollTargetZ - pPlayer->z;
            WorldMapWrapDelta(&delta);
            pPlayer->y += delta.y >> 4;
            g_WorldMapScrollX[0] += delta.x;
            g_WorldMapScrollZ += delta.z;
            pPlayer->x = (s32)g_WorldMapScrollTargetX[0];
            do {
            } while (0);
            pPlayer->z = g_WorldMapScrollTargetZ;
        }
        break;
    }
    WorldMapWrapPosition(&pPlayer->x);
    *(Quad *)&g_WorldMapCameraTarget = *(Quad *)&pPlayer->x;
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800914D0);





s32 WorldMapModeInitActorAngleSets(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    pPlayer = g_WorldMapSlots + idx;
    nMode = g_WorldMapMode;
    switch (nMode) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        pPlayer->unk50 = 3;
        g_WorldMapCameraHeight = 0x460000;
        g_WorldMapCameraRot = -0x260;
        pPlayer->unk64 = (s32)g_WorldMapCameraPathAnglesModes1To5;
        pPlayer->pCameraPathHeights = g_WorldMapCameraPathHeightsModes1To5;
        break;
    case 6:
        break;
    case 7:
        pPlayer->unk50 = 3;
        g_WorldMapCameraHeight = 0x280000;
        g_WorldMapCameraRot = -0x260;
        pPlayer->unk64 = (s32)g_WorldMapCameraPathAnglesMode7;
        pPlayer->pCameraPathHeights = g_WorldMapCameraPathHeightsMode7;
        break;
    }
    return 1;
}
// best: 91.99%, 988 bytes / 247 insns against retail's 992 / 248 (one instruction
// SHORT; round 502). Round 489's corrections stand: the angle track lives at 0x64
// ('pPlayer->unk64 = (s32)g_WorldMapCameraPathAngles*' and 'pPath = (s16 *)pPlayer->unk64'),
// the struct has no pCameraPathAngles member, and the command-switch arm order
// 14, 17, 9, 10 is worth 13 points (block emission order is the lever in that switch).
// ROUND 502 CLOSED THE CAMERA-HEIGHT RAMP (87.81 -> 91.99): name the two loaded values
// ('nTarget = pPlayer->unk54; nHeight = g_WorldMapCameraHeight;') and spell the step as a
// CLAMP followed by a shift - 'if (nDelta > 0x40000) nDelta = 0x40000; nStep = nDelta >> 3;'
// - instead of an if/else on '0x40000 >> 3'. Retail's 'lui v1,0x40000 / sra v1,v1,3' is
// those two statements (clamp, then shift), NOT a folded constant; with the locals the
// whole ramp region becomes instruction-identical (both arms, the addu in the beqz delay
// slot, the shared store) and the two reloads of unk54/g_WorldMapCameraHeight disappear.
// Measured dead the same round: moving 'pPlayer->command = 0;' above the
// pCameraPathHeights store in the case 9/10 arms is WORSE (91.73 / 996 / 249) - the two
// heights stores are cross-jumped into the shared tail exactly as written here.
// LEFT, in the order the diff shows it:
// - g_WorldMapCameraRot is read and written through a register in retail at three sites
//   (state-1 tail 'lh a0,0(a3)' + a3 passed to func_80096F18, case 3 'lh v1,0(a3)' and
//   'sh v0,0(a3)'), while a 's16 *pRot = &g_WorldMapCameraRot;' local folds to a bare
//   mem ref and re-forms the address at the call (+3 instructions here). That is lever
//   'a-pointer-local-to-a-scalar-global-always-folds': the address only survives if the
//   SYMBOL is declared as a struct field, and g_WorldMapCameraRot lives in a shared
//   header - an orchestrator decision, not a spelling.
// - the shared command tail: retail 'li v0,1 / sw v0,80(s0)' where this body reuses the
//   nRet=1 register ('sw s2,80(s0)'); the store's constant is cse'd with nRet's s2 here
//   and was not in retail.
// - state 0 keeps retail's statement order but sched2 hoists the unk50 store above the
//   two 'sh' stores and reloads it immediately (retail reloads it after the heights load
//   and indexes pPath with the reloaded value). No source order tried moves it.
// Field types this body proved: command/state/stateTimer s16 (lh, slti, sh), unk54/unk60
// s32 (blez, sra), pCameraPathHeights s16* and 0x64 s32 (the angle track pointer), both
// indexed by the path point.
/*
s32 func_80091C18(s32 idx) {
    WorldMapPlayer *pPlayer;
    s16 *pPath;
    s16 *pRot;
    s32 nPoint;
    s32 nDelta;
    s32 nStep;
    s32 nTarget;
    s32 nHeight;
    s32 nRet;

    pPlayer = g_WorldMapSlots + idx;
    nRet = 1;
    switch (pPlayer->command) {
    case 14:
        pPlayer->command = 0;
        pPlayer->state = 0;
        g_WorldMapRotationMatrixValid = 0;
        break;
    case 17:
        g_WorldMapCameraHeight = 0x400000;
        pPlayer->state = 3;
        pPlayer->command = 0;
        pPlayer->unk58 = -0x100;
        break;
    case 9:
        pPlayer->unk64 = (s32)g_WorldMapCameraPathAnglesMode7;
        pPlayer->pCameraPathHeights = g_WorldMapCameraPathHeightsMode7;
        pPlayer->command = 0;
        if (pPlayer->unk50 == 0) {
            pPlayer->unk50 = 1;
        }
        break;
    case 10:
        pPlayer->unk64 = (s32)g_WorldMapCameraPathAnglesModes1To5;
        pPlayer->pCameraPathHeights = g_WorldMapCameraPathHeightsModes1To5;
        pPlayer->command = 0;
        if (pPlayer->unk50 == 0) {
            pPlayer->unk50 = 1;
        }
        break;
    }
    pPath = (s16 *)pPlayer->unk64;
    switch (pPlayer->state) {
    case 0:
        nPoint = func_80091FF8(pPlayer->unk50, pPath, pPlayer->pCameraPathHeights);
        if (pPlayer->unk50 != nPoint) {
            pPlayer->state = 1;
            pPlayer->stateTimer = 0;
            pPlayer->unk50 = nPoint;
            pPlayer->unk54 = g_WorldMapPathPointHeights[nPoint];
            pPlayer->unk58 = pPath[pPlayer->unk50];
            pPlayer->unk60 = g_WorldMapCameraRot << 12;
        }
        func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight, &g_WorldMapCameraRot);
        break;
    case 1:
        if (pPlayer->stateTimer >= 5) {
            nPoint = func_80091FF8(pPlayer->unk50, pPath, pPlayer->pCameraPathHeights);
            if (pPlayer->unk50 != nPoint) {
                pPlayer->unk50 = nPoint;
                pPlayer->unk54 = g_WorldMapPathPointHeights[nPoint];
                pPlayer->unk58 = pPath[pPlayer->unk50];
            }
            pPlayer->stateTimer = 0;
        }
        nTarget = pPlayer->unk54;
        nHeight = g_WorldMapCameraHeight;
        if (nTarget != nHeight) {
            nDelta = nTarget - nHeight;
            if (nDelta > 0) {
                if (nDelta > 0x40000) {
                    nDelta = 0x40000;
                }
                nStep = nDelta >> 3;
                if (nStep < 0x40) {
                    g_WorldMapCameraHeight = nTarget;
                } else {
                    g_WorldMapCameraHeight = nHeight + nStep;
                }
            } else {
                g_WorldMapCameraHeight = nHeight - 0x2000;
            }
        }
        if (pPlayer->unk58 != g_WorldMapCameraRot) {
            nDelta = (pPlayer->unk58 - g_WorldMapCameraRot) << 12;
            if (nDelta < 0) {
                pPlayer->unk60 += nDelta >> 5;
            } else if (pPlayer->unk50 == 0) {
                pPlayer->unk60 += 0xF32;
            } else {
                pPlayer->unk60 += 0x799;
            }
            g_WorldMapCameraRot = pPlayer->unk60 >> 12;
        }
        pRot = &g_WorldMapCameraRot;
        if ((pPlayer->unk54 == g_WorldMapCameraHeight) & (pPlayer->unk58 == *pRot)) {
            pPlayer->state = 0;
        }
        func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight, pRot);
        break;
    case 2:
        D_8009BD48.vx = 0;
        D_8009BD48.vz = 0;
        D_8009BD48.vy = g_WorldMapCameraTargetY >> 12;
        break;
    case 3:
        pRot = &g_WorldMapCameraRot;
        if (pPlayer->unk58 != *pRot) {
            *pRot += 2;
        }
        func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, g_WorldMapCameraHeight, pRot);
        break;
    }
    pPlayer->stateTimer++;
    return nRet;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091C18);



// best: 92.80%, 576 bytes / 144 insns against retail's 572 / 143 (ONE instruction
// long). The whole structure is decoded and every relocation is right; what is left
// is gcc allocation and scheduling:
// - the three callee-saved roles are permuted: retail nX=s3, nBest=s4, i=s5, this body
//   rotates them (nBest=s3, i=s4, nX=s5), so the prologue save order, the tail 'move
//   v0,s5' / 'lw s5,16(sp)' pair and the outer loop's 'slti s5,3' all differ.
// - 'lui v0,0xfff8' (the 0xFFF80000 mask) is hoisted to the top of the block here and
//   used as an early t0; retail emits it late and uses it twice.
// - retail fills the 'beqz' delay slot of the inner 'if (nZ < nBest)' with the
//   'sw v1,4(s1)'; this body leaves a nop there.
// - the tail abs: retail is 'subu v0,v0,v1 / bgez v0 / nop / negu v0,v0 / slti'. This
//   body computes the difference in a fresh v1 and reorg COPIES the slti into the bgez
//   delay slot, which is the one extra instruction. Writing the tail as two statements
//   ('nZ = tbl[nLimit] + nBase; nZ -= nDist;') makes it worse: 145 insns / 90.49.
// Round 476 measured, from the 92.10 body:
// - the 4th argument to func_80096F18 must be a CONSTANT-derived address,
//   '(u16 *)(pScratch + 0xA8)': then cse merges its 'lui 0x1f80' with the one already
//   in s1 and emits the single 'addiu a3,s1,0xA8'. Retail's object shows that same
//   instruction with a %lo reloc on the D_1F8000A8 symbol, i.e. the linked bytes are
//   identical. Writing '&D_1F8000A8' costs a whole extra lui/addiu pair (92.10 -> 92.80,
//   145 -> 144 insns).
// - the extern for this function had to be fixed to 'u16 *pIn' (the body's type;
//   Ghidra agrees): with 's16 *' cc1 exits 33 with a conflicting-type error and
//   diff_func silently scores the stale object at the old percentage.
// A permuter run is NOT the next step here (below 99); the register rotation is the
// only handle and it needs a spelling that changes the allocno set.
// best: 94.16%, 572 bytes / 143 insns = retail's exact size (was 92.80/576/144).
// Round 489 closed two things and PROVED the rest shut with -da/-dl (recipe: cpp
// with -DSKIP_ASM, then tools/gcc-2.7.2-psx/cc1 with the unit cc_flags + -O2 -G8
// -da -dl; read the ';; Function func_80091FF8' section of m.i.lreg/.greg):
// - the inner-loop store is UNCONDITIONAL in retail - it sits in the beqz delay
//   slot, i.e. it executes on both paths, so the source had
//   'nZ = *(s8 *)WorldMapGetTileEntry(...); *(s32 *)(pScratch + 4) = nZ; if (nZ <
//   nBest) nBest = nZ;'. Keeping the store inside the if left a nop in that slot
//   and cost the extra instruction (144). Nothing in the function reads
//   scratch+4, so the unconditional store is safe to write.
// - the tail's abs needs its own BLOCK-LOCAL ('s32 nDiff;' at the top of the if
//   block, one expression): then it comes out as retail's
//   'subu v0,v0,v1 / bgez v0 / nop / negu v0,v0 / slti v0,v0,65'. Writing it into
//   the function-level nZ put the result in v1 and reorg duplicated the slti into
//   the bgez delay slot (144 insns).
// LEFT: the three-way callee-saved rotation (retail nX=s3, nBest=s4, i=s5; this
// body nBest=s3, i=s4, nX=s5) and nothing else. It is QUOTIENT-CLOSED, not a
// spelling: -dl gives pseudo 78 (nX) 5 refs / 31 insns = 2*5*4/31 = 1.29, pseudo
// 80 (nBest) 12 / 56 = 3*12*4/56 = 2.57, pseudo 82 (i) 10 / 86 = 3*10*4/86 = 1.40,
// and find_reg hands s3,s4,s5 out in ascending regno along the
// ';; 16 regs to allocate: 81 84 139 76 83 136 80 82 78 75 73 74 127 77 72 110'
// order, so retail needs nX first (>= 2.57, i.e. live <= 15 insns or 9 refs) where
// the stream pins it at 31/5. Both the refs term and the live length are fixed by
// retail's own instructions, which this body already reproduces one for one, so no
// source spelling reaches the order (declaration order and statement order both
// measure byte-identical). Do not re-brief this park.
// Also measured this round: the 'nBest = 0;' statement order (before or after the
// *pIn store) is byte-identical, try_flags cdk 81.45 / -O3 compile-fails / -G0
// 92.80 on the old body, so the preset is right.
// Round 502 re-measured this exact body: 94.16%, 572 bytes / 143 insns = retail's size,
// 63 differing lines and every one is the s3/s4/s5 rotation. Lever lookups ('regalloc
// callee-saved swapped rotation', 'do-while block barrier') name a three-way rotation
// quotient-closed: nX would need 8 refs (3 instructions retail does not have) or a
// 15-insn live range. Re-parked unchanged.
/*
s32 func_80091FF8(s32 nLimit, u16 *pIn, s16 *pOfs) {
    s32 *pTrack;
    u8 *pScratch;
    s32 nBase;
    s32 nX;
    s32 nY;
    s32 nBest;
    s32 nZ;
    s32 i;
    s32 j;
    s32 k;

    u16 v = *(u16 *)&g_WorldMapViewRotationY;
    u16 a = *(u16 *)&g_WorldMapViewRotationZ;

    nBase = g_WorldMapScrollTargetY >> 12;
    *(u16 *)0x1F8000AA = v;
    *(u16 *)0x1F8000AC = a;
    pScratch = (u8 *)0x1F800000;
    pTrack = g_WorldMapPathPointHeights;
    for (i = 0; i < 3; i++) {
        nBest = 0;
        *(u16 *)(pScratch + 0xA8) = *pIn;
        func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, *pTrack - (g_WorldMapProjectionDistance / 2 << 12), (u16 *)(pScratch + 0xA8));
        nX = (g_WorldMapCameraTarget + ((g_WorldMapPathFrame - 0x180) << 12)) & 0xFFF80000;
        nY = (g_WorldMapCameraTargetZ - ((D_8009BD44 + 0x180) << 12)) & 0xFFF80000;
        *(s32 *)(pScratch + 8) = nY;
        for (j = 0; j < 6; j++) {
            *(s32 *)pScratch = nX;
            for (k = 0; k < 6; k++) {
                WorldMapWrapPosition(pScratch);
                nZ = *(s8 *)WorldMapGetTileEntry(*(s32 *)pScratch, *(s32 *)(pScratch + 8));
                *(s32 *)(pScratch + 4) = nZ;
                if (nZ < nBest) {
                    nBest = nZ;
                }
                *(s32 *)pScratch += 0x80000;
            }
            *(s32 *)(pScratch + 8) += 0x80000;
        }
        if (*(s16 *)pOfs + nBase + 0x50 < (nBest << 3)) {
            break;
        }
        pIn++;
        pTrack++;
        pOfs++;
    }
    if (i < nLimit) {
        s32 nDiff;
        s32 nDist = (nBest << 3) - 0x50;
        nDiff = ((s16 *)g_WorldMapCameraPathHeightsModes1To5)[nLimit] + nBase - nDist;
        if (nDiff < 0) {
            nDiff = -nDiff;
        }
        if (nDiff < 0x41) {
            i = nLimit;
        }
    }
    return i;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091FF8);





s32 WorldMapModeInitValue50(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    pPlayer = g_WorldMapSlots + idx;
    nMode = g_WorldMapMode;
    if (nMode > 0) {
        if (nMode < 6) {
            g_WorldMapFadeLevel = 0x8C;
        } else if (nMode < 8) {
            g_WorldMapFadeLevel = 0x78;
            pPlayer->state = 1;
        }
    }
    pPlayer->unk50 = g_WorldMapFadeLevel << 12;
    return 1;
}
s32 WorldMapModeUpdateValue50(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nRet;

    pPlayer = g_WorldMapSlots + idx;
    nRet = 1;
    if (pPlayer->command == 9) {
        pPlayer->command = 0;
        pPlayer->state = 1;
    } else if (pPlayer->command == 0xA) {
        pPlayer->command = 0;
        pPlayer->state = 2;
    }
    switch (pPlayer->state) {
    case 0:
        nRet = 3;
        break;
    case 1:
        pPlayer->unk50 -= 0x1000;
        g_WorldMapFadeLevel = pPlayer->unk50 >> 12;
        if (g_WorldMapFadeLevel < 0x78) {
            g_WorldMapFadeLevel = 0x78;
            pPlayer->unk50 = 0x78000;
            pPlayer->state = 0;
        }
        break;
    case 2:
        pPlayer->unk50 += 0x1000;
        g_WorldMapFadeLevel = pPlayer->unk50 >> 12;
        if (g_WorldMapFadeLevel >= 0x8C) {
            g_WorldMapFadeLevel = 0x8C;
            pPlayer->unk50 = 0x8C000;
            pPlayer->state = 0;
        }
        break;
    }
    return nRet;
}
// The two fade-rectangle quads: prim 0 is the template this init fills in, prim 1
// its copy.  Retail reaches every field through its own absolute symbol, so the
// typed spelling below is reloc-only: objdiff reads 97.62% because the %lo addends
// differ, and check_bin world_map is the proof it links identically.
extern POLY_G4 g_WorldMapFadePrims[];
extern DR_TPAGE g_WorldMapFadeTpage;
extern s32 g_WorldMapTpageAbr;

s32 WorldMapModeInitFadeRect(s32 idx) {
    WorldMapPlayer *pPlayer;
    POLY_G4 *pG4;
    u_short nTpage;

    pPlayer = g_WorldMapSlots + idx;
    pPlayer->unk50 = 0xF8;
    nTpage = GetTPage(0, g_WorldMapTpageAbr, 0x380, 0x100);
    SetDrawTPage(&g_WorldMapFadeTpage, 1, 0, nTpage);
    pG4 = g_WorldMapFadePrims;
    setPolyG4(&g_WorldMapFadePrims[0]);
    // the original re-reads unk50 for all twelve colour bytes; volatile stops
    // gcc folding them into one load.
    pG4->r0 = *(volatile s32 *)&pPlayer->unk50;
    pG4->g0 = *(volatile s32 *)&pPlayer->unk50;
    pG4->b0 = *(volatile s32 *)&pPlayer->unk50;
    pG4->r1 = *(volatile s32 *)&pPlayer->unk50;
    pG4->g1 = *(volatile s32 *)&pPlayer->unk50;
    pG4->b1 = *(volatile s32 *)&pPlayer->unk50;
    pG4->r2 = *(volatile s32 *)&pPlayer->unk50;
    pG4->g2 = *(volatile s32 *)&pPlayer->unk50;
    pG4->b2 = *(volatile s32 *)&pPlayer->unk50;
    pG4->r3 = *(volatile s32 *)&pPlayer->unk50;
    pG4->g3 = *(volatile s32 *)&pPlayer->unk50;
    pG4->b3 = *(volatile s32 *)&pPlayer->unk50;
    SetSemiTrans(pG4, 1);
    pG4[1] = pG4[0];
    g_WorldMapFadePrims[0].x0 = 0;
    g_WorldMapFadePrims[0].y0 = 0;
    g_WorldMapFadePrims[0].x1 = 0x140;
    g_WorldMapFadePrims[0].y1 = 0;
    g_WorldMapFadePrims[0].x2 = 0;
    g_WorldMapFadePrims[0].y2 = 0xD8;
    g_WorldMapFadePrims[0].x3 = 0x140;
    g_WorldMapFadePrims[0].y3 = 0xD8;
    g_WorldMapFadePrims[1].x0 = 0;
    g_WorldMapFadePrims[1].y0 = 0;
    g_WorldMapFadePrims[1].x1 = 0x140;
    g_WorldMapFadePrims[1].y1 = 0;
    g_WorldMapFadePrims[1].x2 = 0;
    g_WorldMapFadePrims[1].y2 = 0xD8;
    g_WorldMapFadePrims[1].x3 = 0x140;
    g_WorldMapFadePrims[1].y3 = 0xD8;
    return 1;
}
// The frame render context g_WorldMapRenderContext points at (partial view: the head of
// the ordering table the fade primitives are linked into).
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long *pOt[2];
} WorldMapRenderContext; // partial view

extern WorldMapRenderContext *g_WorldMapRenderContext;
extern s32 g_WorldMapPrimPacketIndex;
extern s32 g_WorldMapDrawMode;
extern s32 g_WorldMapFadeActive;
// Address taken as array decay in WorldMapModeFadeRect (a pointer local keeps the address
// in a register; &global is folded to the %hi/%lo macro form).
extern DR_TPAGE g_WorldMapFadeTpageArr[] __asm__("g_WorldMapFadeTpage");

#define WORLD_MAP_FADE_RECT_TPAGE_X 0x380
#define WORLD_MAP_FADE_RECT_TPAGE_Y 0x100
#define WORLD_MAP_FADE_LEVEL_MAX 0xFF

// Fades the two fade-rectangle primitives in or out by one draw-mode step per frame:
// command 0xC fades the level down and 0xD fades it up, and the return value drops to 3
// once the fade has finished.  The primitives are re-linked into the ordering table
// every frame because the level is a Gouraud colour.
// best: 93.72% (objdiff), 1592 bytes / 398 insns against retail 1604 / 401, XenoWorldMap.
//    The whole structure is decoded and every relocation is right; what is left is gcc
//    allocation and delay-slot filling. (The twelve colour stores name
//    g_WorldMapFadePrims+4/+5/+6/+0xC.. where splat named D_8009CE70..D_8009CE8A; the
//    linked bytes are identical.)
//    Closed by the array-decay alias (round 474, +0.99 from 92.73 to 93.72 and -4 insns):
//    with `pTpage = &g_WorldMapFadeTpage` gcc re-forms the address inline for the first
//    store (`lui at`, then `lui/addiu` again for the second), one instruction over retail;
//    with `extern DR_TPAGE g_WorldMapFadeTpageArr[] __asm__("g_WorldMapFadeTpage");` and
//    `pTpage = g_WorldMapFadeTpageArr;` the address stays in a register as retail has it.
//    LEFT, each an allocator decision and none structural:
//    (1) the two addPrim masks land in $a1/$a2 where retail has $a2/$a3 (0xFF000000 in
//        $a1 against $a2, 0xFFFFFF in $a2 against $a3; retail keeps the render context in
//        $a1, we keep it in $t1). Moved by nothing measured: the misc13 tail shape
//        (pPrims/pPrim plus tag and primAddr locals for both pairs, 92.92), pPrim before
//        pCtx (identical), a pointer local for the constant address (folded), and a
//        POLY_G4-typed ctx->pOt[2] (same registers).
//    (2) the prim base lui/addiu is scheduled after the index math where retail does it
//        before the two global loads.
//    (3) retail leaves a nop in the load-delay slot of the second pOt[0] load of each pair
//        and we fill it with the tpage address: that is where the three missing
//        instructions are, so the size cannot come out at retail's 1604 while the
//        allocation differs.
//    (4) the tail: retail keeps the stateTimer value in $v0 (`lhu v0,34(s0) / nop /
//        addiu v0,v0,1 / sh v0`) and copies nRet into $v0 after it; we use $v1 and reorg
//        puts `move v0,s1` in the load delay slot.
//    ROUND 487: instruction-diffed both objects, and the whole 12-byte deficit is THREE
//    maspsx load-delay hazard `nop`s, one in each arm plus one in the tail. Each exists in
//    retail only because retail's loaded value lands in the register the next instruction
//    writes: the arm nop sits between `lw a0,112(a1)` (the OT node) and `lw v0,0(a0)`, so
//    the tpage address (retail: $a0, ours: $t0, a register the load is not using) can be
//    reorg-stolen into that gap on our side and not on retail's; the tail nop sits between
//    `lhu v0,34(s0)` and `addiu v0,v0,1`, where our load taking $v1 frees $v0 for the
//    `move v0,s1` return copy to take the gap. So (1) and (3) are ONE tie: the missing
//    instructions are a consequence of the register hand-out, not a separate residual, and
//    they cannot be added back by any statement.
//    Measured byte-identical this round on the same 93.72 / 1592 / 398 body: the
//    `pTpage = g_WorldMapFadeTpageArr;` statement moved above `pPrim = ...` in both arms,
//    the pTpage/pPrim declaration order swapped, and `pPlayer->stateTimer =
//    pPlayer->stateTimer + 1;` for `stateTimer++`. Preset screen: -O3 93.72 (identical),
//    gcc-2.7.2-cdk 82.71, gcc-2.6.0 compile-fails, so XenoWorldMap is right.
//    Trap: dropping the `pTpage = g_WorldMapFadeTpageArr;` assignment leaves pTpage
//    uninitialised (UB) and measures 93.90% at the same 1592 bytes - 0.18 higher than the
//    honest body. Re-measure after any edit that moves statements in this block.
//    FRAME: retail saves ra/s0/s1 in a 0xE0 frame and touches none of the 0xC0 below; the
//    unusedFrame local reproduces it (worth 0.02% and required for check_bin).
//    Reproduced exactly and load-bearing: the command switch lists 0xD before 0xC (an
//    if/else chain or the ascending order swaps the two arms), each of the twelve Gouraud
//    colour stores re-reads pPlayer->unk50 through a volatile s32 (a plain read folds to
//    one load), and the fade-done store g_WorldMapFadeActive = 0 is shared through a goto
//    (duplicating it in both arms emits it twice).
// best: 93.72% (objdiff), 1592 bytes / 398 insns against retail 1604 / 401, XenoWorldMap.
//   ROUND 496 RE-CONFIRMED the note below from a clean unpark: the parked body re-measures
//   93.72 / 1592 / 398 exactly. The 93.90% the round-474 note warns about is the UB body
//   (missing pTpage assignment), so 93.72 is the honest number and this is a park, not a
//   0.1% job.
//   Sharpened residual (round 496, instruction-diff of both objects): every one of the three
//   missing instructions is a tired load-delay nop, and all three come from ONE cause - the
//   addPrim block keeps 8 live values where retail keeps 6. Retail's pair uses only
//   v0,v1,a0,a1,a2,a3: pCtx in $a1, pPrim in the scratch $v1, the tpage address in $a0
//   (the very register that holds the loaded pOt), and the masks in $a2/$a3. Ours needs
//   a3 = pPrim, t0 = tpage address and t1 = pCtx on top of a1/a2 = masks, so
//   (a) in each arm the 'lui/addiu' of the tpage address fills the load-delay slot after
//   'lw a0,112(pCtx)' that retail must leave as a nop (retail's address register IS the
//   loaded one, so filling it would clobber the load), and
//   (b) in the tail the stateTimer temp lands in $v1, which frees $v0 for the 'move v0,s1'
//   return copy to take the 'lhu' delay slot where retail leaves its third nop.
//   So the size is a consequence of the register hand-out and cannot be added back by a
//   statement: any body that emits these 398 instructions ranks the two mask constants above
//   pCtx on refs x live_length (the masks span the 12 colour stores, pCtx spans only the two
//   addPrim pairs) and any body that ranks them the other way emits different instructions.
//   ROUND 496 permuter run (4 min, 1663 iterations): PERMUTER_BETTER base=4568 best=3093, and
//   its single edit - an empty 'do { } while (0);' before the state switch - compiles
//   BYTE-IDENTICALLY to this body in the real build (gcc deletes the label), so the
//   improvement is an artefact of its own text metric; nothing to port. Do not re-run it.
/*
s32 func_800925A0(s32 idx) {
    // retail's frame is 0xE0 with only ra/s0/s1 saved: it reserves 0xC0 of stack that no
    // instruction touches, which only an unused local reproduces.
    u8 unusedFrame[0xC0];
    WorldMapPlayer *pPlayer;
    WorldMapRenderContext *pCtx;
    POLY_G4 *pPrim;
    DR_TPAGE *pTpage;
    u_short nTpage;
    s32 nRet;

    pPlayer = g_WorldMapSlots + idx;
    nRet = 1;
    switch (pPlayer->command) {
    case 0xD:
        pPlayer->command = 0;
        pPlayer->state = 1;
        g_WorldMapFadeActive = 1;
        nTpage = GetTPage(0, g_WorldMapTpageAbr, WORLD_MAP_FADE_RECT_TPAGE_X, WORLD_MAP_FADE_RECT_TPAGE_Y);
        SetDrawTPage(&g_WorldMapFadeTpage, 1, 0, nTpage);
        break;
    case 0xC:
        pPlayer->command = 0;
        pPlayer->state = 0;
        g_WorldMapFadeActive = 1;
        nTpage = GetTPage(0, g_WorldMapTpageAbr, WORLD_MAP_FADE_RECT_TPAGE_X, WORLD_MAP_FADE_RECT_TPAGE_Y);
        SetDrawTPage(&g_WorldMapFadeTpage, 1, 0, nTpage);
        break;
    }
    switch (pPlayer->state) {
    case 0:
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].r0 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].g0 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].b0 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].r1 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].g1 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].b1 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].r2 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].g2 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].b2 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].r3 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].g3 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].b3 = *(volatile s32 *)&pPlayer->unk50;
        pCtx = g_WorldMapRenderContext;
        pPrim = &g_WorldMapFadePrims[g_WorldMapPrimPacketIndex];
        pPrim->tag = (pPrim->tag & 0xFF000000) | (*(u32 *)pCtx->pOt[0] & 0xFFFFFF);
        *(u32 *)pCtx->pOt[0] = (*(u32 *)pCtx->pOt[0] & 0xFF000000) | ((u32)pPrim & 0xFFFFFF);
        pTpage = g_WorldMapFadeTpageArr;
        pTpage->tag = (pTpage->tag & 0xFF000000) | (*(u32 *)pCtx->pOt[0] & 0xFFFFFF);
        *(u32 *)pCtx->pOt[0] = (*(u32 *)pCtx->pOt[0] & 0xFF000000) | ((u32)pTpage & 0xFFFFFF);
        pPlayer->unk50 -= g_WorldMapDrawMode;
        if (pPlayer->unk50 < 0) {
            nRet = 3;
            pPlayer->unk50 = 0;
            goto fadeDone;
        }
        break;
    case 1:
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].r0 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].g0 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].b0 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].r1 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].g1 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].b1 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].r2 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].g2 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].b2 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].r3 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].g3 = *(volatile s32 *)&pPlayer->unk50;
        g_WorldMapFadePrims[g_WorldMapPrimPacketIndex].b3 = *(volatile s32 *)&pPlayer->unk50;
        pCtx = g_WorldMapRenderContext;
        pPrim = &g_WorldMapFadePrims[g_WorldMapPrimPacketIndex];
        pPrim->tag = (pPrim->tag & 0xFF000000) | (*(u32 *)pCtx->pOt[0] & 0xFFFFFF);
        *(u32 *)pCtx->pOt[0] = (*(u32 *)pCtx->pOt[0] & 0xFF000000) | ((u32)pPrim & 0xFFFFFF);
        pTpage = g_WorldMapFadeTpageArr;
        pTpage->tag = (pTpage->tag & 0xFF000000) | (*(u32 *)pCtx->pOt[0] & 0xFFFFFF);
        *(u32 *)pCtx->pOt[0] = (*(u32 *)pCtx->pOt[0] & 0xFF000000) | ((u32)pTpage & 0xFFFFFF);
        pPlayer->unk50 += g_WorldMapDrawMode;
        if (pPlayer->unk50 >= WORLD_MAP_FADE_LEVEL_MAX) {
            pPlayer->unk50 = WORLD_MAP_FADE_LEVEL_MAX;
            goto fadeDone;
        }
        break;
    default:
        return nRet;
    }
    goto updateTimer;
fadeDone:
    g_WorldMapFadeActive = 0;
updateTimer:
    pPlayer->stateTimer++;
    return nRet;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800925A0);



