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

extern s32 WorldMapUpdateSlotTurnAndPressEntry(WorldMapPlayer *pPlayer);
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
        nTmp = WorldMapUpdateSlotTurnAndPressEntry(pPlayer);
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
        if (WorldMapUpdateSlotTurnAndPressEntry(pPlayer) == 1) {
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
