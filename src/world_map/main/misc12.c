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
extern s32 D_8009C170;
extern s32 WorldMapGetTilePlaneHeight(s32 x, s32 z);
extern s32 g_WorldMapCameraHeight;
extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationZ[];
extern u16 g_WorldMapPlayerAngle;
extern u32 g_WorldMapScrollTargetX[];
extern s16 g_WorldMapCameraPathAnglesModes1To5[];
extern s16 g_WorldMapCameraPathAnglesMode7[];
extern s16 g_WorldMapCameraPathHeightsModes1To5[];
extern s16 g_WorldMapCameraPathHeightsMode7[];
extern s32 g_WorldMapCameraTargetY;
extern SVECTOR D_8009BD48;
extern s32 g_WorldMapRotationMatrixValid;
extern s32 func_80091FF8(s32 nLimit, s16 *pIn, s16 *pOfs);

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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E190);
/* best: 87.34%, 864 bytes against retail's 868 (one instruction short). Decoded
   structure and every relocation is right. Measured this round, so a later agent
   does not redo it:
   - The scratchpad stores do NOT need symbol names. Retail's `addiu $s0,$s2,%lo(D_1F8000A0)`
     and `sh $v0,(0x1F8000A0 & 0xFFFF)($s2)` are the same instructions as this body's
     `$s2 + 0xA0` offset stores: the %lo addend comes from a linker reloc in the original
     object, the linked bytes are identical. objdiff only shows the reloc name.
   - What DOES matter there is the ORDER: gcc hoists the `lhu g_WorldMapViewRotationZ`
     above the 0xA0 store unless the three tail stores are volatile, which is worth
     4 bytes (860 - 864). Retail keeps source order, this body now does too.
   - LEFT: (1) the `WorldMapGetGameStatePosition` call sequence: retail is
     [addiu a0,s1,0x28][jal][sh zero,0x24(s1) in the slot][li s3,1], this body gets
     reorg to steal `addiu a0` into the slot instead. (2) the case 4/5 arm: retail emits
     `addiu v0,3 / sh v0,0x20(s1)` first, then `lui/lw g_WorldMapPlayerPosition`,
     `nop`, `sw v0,0x28(s1)`, `lui/lw g_WorldMapPlayerPositionZ`, and the store of z in
     the jal slot; this body hoists both global loads above the state store and loses
     retail's load-delay nop (that nop is the missing instruction). (3) the same shape
     in case 7. The two jtbl_ operands are the only other diff.
   PARKED FOR A LAYOUT REASON TOO: this stub owns jtbl_800709F8/jtbl_80070A10 at
   .rodata+0/+0x18 and func_8008E4F4 owns jtbl_80070A30 at +0x38, so the whole
   switch family (E190, E4F4, both at retail's +0/+0x18/+0x38 which is 8-byte
   aligned throughout) has to be converted in address order in one go.
s32 func_8008E190(s32 idx) {
    WorldMapPlayer *pPlayer;
    u8 *pScratch;
    s32 nRet;
    u16 nAngle;

    pPlayer = g_WorldMapSlots + idx;
    WorldMapGetGameStatePosition(&pPlayer->x);
    pPlayer->hidden = 0;
    nRet = 1;
    nAngle = g_GameState.scriptAngle;
    pPlayer->dirZ = 0;
    pPlayer->dirY = 0;
    pPlayer->dirX = 0;
    pPlayer->pCameraPathAngles = 0;
    pPlayer->unk60 = 0;
    pPlayer->unk74 = 0;
    pPlayer->pCameraPathHeights = (s16 *)0xFFD80000;
    pPlayer->unk70 = 0;
    pPlayer->unk6C = 0;
    pPlayer->angle = nAngle;
    pScratch = (u8 *)0x1F800000;
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
        pPlayer->x = g_WorldMapPlayerPosition;
        pPlayer->z = g_WorldMapPlayerPositionZ;
        pPlayer->y = WorldMapGetTilePlaneHeight(pPlayer->x, pPlayer->z) + 0xFFFC0000;
        pPlayer->angle = g_WorldMapPlayerRotation;
        pPlayer->unk74 = D_8009C170;
        *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
        nAngle = pPlayer->angle;
        g_WorldMapGroups->entry[3].hidden = 1;
        g_WorldMapGroups->entry[2].hidden = 1;
        g_WorldMapGroups->entry[1].hidden = 1;
        g_WorldMapPlayerAngle = nAngle;
        break;
    case 7:
        pPlayer->state = 2;
        pPlayer->x = g_WorldMapPlayerPosition;
        pPlayer->z = g_WorldMapPlayerPositionZ;
        pPlayer->angle = g_WorldMapPlayerRotation;
        pPlayer->unk74 = D_8009C170;
        pPlayer->y = g_WorldMapPlayerPositionY;
        *(Quad *)g_WorldMapScrollTargetX = *(Quad *)&pPlayer->x;
        g_WorldMapPlayerAngle = pPlayer->angle;
        break;
    }
    *(Quad *)g_WorldMapGroups->entry[0].pos = *(Quad *)&pPlayer->x;
    g_WorldMapGroups->entry[0].hidden = pPlayer->hidden;
    *(volatile s16 *)(pScratch + 0xA0) = -(pPlayer->unk70 >> 12);
    *(volatile s16 *)(pScratch + 0xA4) = g_WorldMapViewRotationZ;
    *(volatile s16 *)(pScratch + 0xA2) = pPlayer->angle;
    RotMatrixYXZ((SVECTOR *)(pScratch + 0xA0), &g_WorldMapGroups->entry[0].matrix);
    RotMatrixYXZ((SVECTOR *)(pScratch + 0xA0), &g_WorldMapGroups->entry[1].matrix);
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
    return nRet;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E4F4);
/* best: 96.21%, 396 bytes = retail's exact size, every instruction and relocation
   right except the switch table operand (jtbl_80070A30 vs .rodata) and the position of
   the g_WorldMapGroups load in the scratchpad tail: retail emits `lui a1,%hi / lw a1` BEFORE
   the three `sh` stores and loads g_WorldMapViewRotationZ after `sh 0xA0`, this body emits the
   pointer load last and hoists g_WorldMapViewRotationZ to the top of the block. Tried: x/z/y store
   order (93.59), x/y/z (93.59), a MATRIX *pMat local for the first RotMatrixYXZ
   argument (no change, gcc sinks the address to the argument anyway), and the three
   tail stores made volatile (96.66 but 400 bytes, one instruction LONG: retail has a
   `nop` between `lui %hi` and `lhu %lo` of g_WorldMapViewRotationZ that this body does
   not emit, and the load is still hoisted above the 0xA0 store). PARKED FOR A
   LAYOUT REASON: E190 (parked just above, 87.06) owns jtbl_800709F8/jtbl_80070A10 at
   .rodata+0/+0x18, and C tables are emitted before asm-owned ones, so converting this
   function alone puts its table at +0 and shifts the whole overlay (check_bin MISMATCH).
   The two must be converted together, in address order.
s32 func_8008E4F4(s32 idx) {
    WorldMapPlayer *pPlayer;
    u8 *pScratch;
    s32 nRet;

    pPlayer = g_WorldMapSlots + idx;
    *(Quad *)g_WorldMapGroups->entry[0].pos = *(Quad *)&pPlayer->x;
    g_WorldMapGroups->entry[0].hidden = pPlayer->hidden;
    nRet = 1;
    pScratch = (u8 *)0x1F800000;
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
    *(s16 *)(pScratch + 0xA0) = -(pPlayer->unk70 >> 12);
    *(s16 *)(pScratch + 0xA4) = g_WorldMapViewRotationZ;
    *(s16 *)(pScratch + 0xA2) = pPlayer->angle;
    RotMatrixYXZ((SVECTOR *)(pScratch + 0xA0), &g_WorldMapGroups->entry[0].matrix);
    RotMatrixYXZ((SVECTOR *)(pScratch + 0xA0), &g_WorldMapGroups->entry[1].matrix);
    return nRet;
}
*/
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

s32 func_800907F4(s32 idx) {
    WorldMapPlayer *pPlayer;
    WorldMapRot *pRotA;
    WorldMapRot *pRotB;
    u16 *pScratchPos;

    pScratchPos = (u16 *)0x1F800000;
    pPlayer = g_WorldMapSlots + idx;
    pRotA = (WorldMapRot *)((u8 *)g_WorldMapGroups + 0xA8);
    pRotB = (WorldMapRot *)((u8 *)g_WorldMapGroups + 0xFC);
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
        pPlayer->unk58 += 4;
        if (pPlayer->unk58 >= 0x11) {
            pPlayer->unk5C += 4;
        }
        if (pPlayer->unk58 >= 0x80) {
            pPlayer->unk58 = 0x80;
        }
        if (pPlayer->unk5C >= 0x80) {
            pPlayer->unk5C = 0x80;
        }
        if ((pPlayer->unk58 >= 0x80) & (pPlayer->unk5C >= 0x80)) {
            pPlayer->state = 3;
        }
        break;
    case 2:
        pPlayer->unk58 -= 4;
        if (pPlayer->unk58 < 0x70) {
            pPlayer->unk5C -= 4;
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090A84);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090C68);
/* best: 99.91%, 428 bytes = retail's exact size, every instruction identical; the only
   residual is the switch table operand (jtbl_80070B84 against .rodata) and the .L label
   relocs, which the linker resolves to the same addresses.
   PARKED FOR A LAYOUT REASON, re-measured and re-proved this round. Converting this stub
   makes world_map.bin 4 bytes too long (check_bin: got 180426 against disc's 180422; the
   first differing word is a .text pointer in misc1's rodata, i.e. TEXT_START moved by 4).
   Cause: cc1 emits `.align 3` (8 bytes) before every C jump table - hardcoded in the MIPS
   port, measured identical in 2.6.0, 2.7.2 and 2.7.2-cdk, and 2.7.2 rejects
   -falign-jumps - while retail packs this unit's tables 4-byte aligned. The unit's rodata
   (0x800709F8, exactly 0x258 bytes) holds nine tables at +0, +0x18, +0x38, +0x58, +0x15C,
   +0x18C, +0x1BC, +0x1EC, +0x214; the last five are 4 mod 8, and the tables tile the range
   with no slack, so the first converted table of that run lands at retail's offset +4 and
   nothing later shrinks back. Converting func_80090E14 as well (its table follows this one)
   leaves the same +4: 0x25C against 0x258.
   A split unit whose .rodata range begins at 0x18C does not fix it either: the `.align 3`
   sets the object section's sh_addralign to 8, so the linker pads the section start 0x18C
   up to 0x190 and the table still lands 4 late.
   The old text of this park did not compile, so its percentage came from a stale object:
   `g_WorldMapViewRotationY` has to be read as `g_WorldMapViewRotationY[0]` with the u16
   array extern at the top of the file - every case body reads it with lhu. angle is s16
   (the rcos/rsin argument reads it with lh) and 0x48 is the field func_80090E14 also writes.
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
        if (g_WorldMapAreaId != -1) {
            return 1;
        }
    }
    if (g_WorldMapPad1Released & 0x10) {
        if ((g_WorldMapMode4AreaId == -1) && (g_WorldMapAreaId == g_WorldMapMode4AreaId)) {
            g_WorldMapMode4EntryTriggered = 1;
        }
    }
    WorldMapUpdateL2R2PressEdge();
    return 0;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090E14);
/* best: 99.90%, 416 bytes = retail's exact size, every instruction identical; the only
   residual is the switch table operand (jtbl_80070BB4 against .rodata) and the .L label
   relocs. PARKED FOR THE SAME LAYOUT REASON as func_80090C68 just above (its table, at
   .rodata +0x1BC, is 4 mod 8; that note has the measurement and the arithmetic).
   Its tail differs from func_80090C68's: a released 0x20 returns 1 only when
   g_WorldMapAreaId != -1, a released 0x40 returns 4, and it does not call
   WorldMapUpdateL2R2PressEdge.
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
        if (g_WorldMapAreaId != -1) {
            return 1;
        }
    }
    if (g_WorldMapPad1Released & 0x40) {
        return 4;
    }
    if (g_WorldMapPad1Released & 0x10) {
        if ((g_WorldMapMode4AreaId == -1) && (g_WorldMapAreaId == g_WorldMapMode4AreaId)) {
            g_WorldMapMode4EntryTriggered = 1;
        }
    }
    return 0;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090FB4);
/* best: 98.03%, 1148 bytes = retail's exact size, every relocation right, frame and
   saved set exact (0x28 with ra/s1/s0, pPlayer in s1, &g_WorldMapViewRotationY across the
   two rcos calls in s0). Decoded: the 0x64 phase dispatch, the four pad arms that build a
   turn target and a 16.16 velocity target, the two "ramp toward the target by at most a
   step, clamped to the remaining distance" blocks, the 0x6C pitch state machine, the 0x80
   move-speed ramp with its +/-moveSpeed<<12 clamp, then the trig tail and the return ladder
   1/4/0. Left is pure register naming plus one constant position:
   - retail keeps pad in a0 and nTurn in v1 (this body v1 and a1); nTol is t2 and pRotZ t1
     here t1 and t2. Eight declaration-order and statement-order permutations measured,
     all 98.03%.
   - the abs in both ramp blocks: retail is bgez / move a1,v0 / negu a1,a1, this body
     bgez / slt (a duplicate that reorg steals into the slot) / negu. An if/else and a
     second-variable spelling both measured the same; the permuter's best candidate scored
     better in its own metric but compiled to the identical object.
   - the `li 0x1000` for nTol: retail emits it before `subu`, reorg puts it in the `bgez`
     delay slot here (retail has a nop there).
   Spellings that ARE load-bearing, measured one at a time from 91.2% to here:
   - `extern s16 g_WorldMapViewRotationY[];` and `pRotY[0]` for both rcos arguments. With a
     scalar extern gcc uses the %hi/%lo macro form twice and the function loses a saved
     register (1144 bytes, s1 never used).
   - `pRotZ = g_WorldMapViewRotationZ;` (array decay, as a statement, not `&global`): that is
     what makes retail's lui+addiu into t1 with the lh and the sh both through it.
   - inside case 1 the FIRST initialisation is `nVel = 0`, which is the one reorg puts in
     the dispatch's branch delay slot, as retail does. nTurn = 0 first puts nTurn there.
   - the tail is `pPlayer->angle = (pPlayer->unk58 >> 12) & 0xFFF;` followed by
     `g_WorldMapViewRotationY[0] = pPlayer->angle;`. Reading the field back keeps the shift
     on the sum's register after the 0x58 store; a separate angle local puts `sra` into a
     fresh register before the store (97.4%).
   - the ramp's add/sub is a conditional expression, `(x < t) ? (x + s) : (x - s)`, which is
     what emits retail's bnez / addu / subu / shared store.
   - the 0x80 block really does test g_WorldMapPad1Held & 0x80 twice (retail re-reads the
     global between the step and the clamp).
   Field evidence for the struct: 0x64 is a phase counter here (0 then 1), not the camera-path
   pointer the camera modes store in it; 0x4A moveSpeed is read with `lh` (so s16); 0x6C is
   compared with slti (s32).
   Dead ends measured: volatile s16* pRotY (adds sll/sra sign extension per read), the pad
   passed inline instead of through a local, an if/else or two-variable abs, and a 4-minute
   permuter run. Re-measured and added in round 392, all at 98.03 / 287 insns / 1148 bytes:
   `u32 nPad` (byte-identical), swapping the `nTol = 0x1000` and `nDelta = g_WorldMapViewRotationZ[0]
   - nTurn` statements (byte-identical, so the li placement is reorg's, not statement order), and
   spelling both ramp abs as a duplicated `(x - y < 0) ? -(x - y) : (x - y)` instead of two
   locals - 95.73%, because gcc then folds the negate into `subu y,x` where retail has `negu`.
   The residual really is ONE allocator choice: this body coalesces nDelta into nAbs, so reorg
   fills the bgez slot with a duplicate `slt`, while retail keeps the copy (`addu a1,v0,zero`)
   and case 1's whole allocation follows from it - retail pad a0 / nTurn v1 / nAngle a0 with its
   copy in a2, here v1 / a1 / a2 with its copy in a0. gcc-2.7.2-cdk scores 93.66 on this body
   and drops three already-matched neighbours off 100, so the preset is right (retail forms
   g_WorldMapViewRotationY's address through `$at`, the 2.7.2 macro form).
   Not tried: block-scoped locals inside `case 1:` - the permutations above are declaration and
   statement order, not scope, and moving the arm's locals into its own block renumbers every
   pseudo in it, which is the cheapest remaining knob for this allocation.
   No jtbl in this stub, so parking it costs no layout work.
// Mode 1's per-frame slot update: ramps the slot's yaw (0x58, a 12.12 fixed-point angle,
// with 0x5C as its angular velocity) toward the direction the pad asks for, tilts the pitch
// accumulator 0x70 up/down, and rebuilds the slot's forward vector from both. The command
// byte 0x64 is the phase: 0 seeds the ramp from the current angle, 1 runs it.
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
    s32 nAbs;
    u16 nPad;

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
            nTol = 0x1000;
            nDelta = g_WorldMapViewRotationZ[0] - nTurn;
            if (nDelta < 0) {
                nDelta = -nDelta;
            }
            nStep = nDelta >> 4;
        }
        pRotZ = g_WorldMapViewRotationZ;
        nAngle = *pRotZ;
        if (nAngle != nTurn) {
            nDelta = nAngle - nTurn;
            nAbs = (nDelta < 0) ? -nDelta : nDelta;
            if (nAbs < nStep) {
                nStep = nAbs;
            }
            nAngle = (nAngle < nTurn) ? (nAngle + nStep) : (nAngle - nStep);
            *pRotZ = nAngle;
        }
        if (pPlayer->unk5C != nVel) {
            nDelta = pPlayer->unk5C - nVel;
            nAbs = (nDelta < 0) ? -nDelta : nDelta;
            if (nAbs < nTol) {
                nTol = nAbs;
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
            nLimit = pPlayer->moveSpeed << 12;
            if (nLimit < pPlayer->unk60) {
                pPlayer->unk60 = nLimit;
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
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091430);
/* best: 91.20%, 160-byte body at retail's size and every branch offset identical.
   Retail keeps &g_WorldMapViewRotationY (s16) in $v1 across the `sh` store and the
   `lh` re-read that feeds unk58 = angle << 12, and loads g_WorldMapMode only after
   that block; this gcc rematerialises the address ($at for the store, a fresh
   lui for the load) because it hoisted the g_WorldMapMode load into the register
   the address would have used. The `lh` re-read itself needs a volatile store
   (plain s16 pointer store is forwarded, 89.60); with the volatile store the
   register tie above is all that is left. Tried: plain global store (89.60),
   s16 pointer local + volatile cast store (91.20, kept), inline g_WorldMapMode
   in the two compares (same), gcc-2.7.2-cdk (78.97), permuter 3 min -j2 (only
   found a broken candidate that used pRotY uninitialised).
s32 func_80091430(s32 idx) {
    WorldMapPlayer *pPlayer;
    s16 *pRotY;
    s32 nMode;
    u16 nAngle;

    pPlayer = g_WorldMapSlots + idx;
    *(Quad *)&pPlayer->x = *(Quad *)g_WorldMapScrollTargetX;
    nAngle = g_WorldMapPlayerAngle;
    pRotY = g_WorldMapViewRotationY;
    g_WorldMapViewRotationZ = 0;
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091C18);
/* best: 87.81%, 996 bytes against retail's 992 (one instruction long). The whole
   state machine is decoded and every relocation is right; what is left is gcc's
   block and delay-slot ordering, not structure:
   - cases 9/10: retail's shared tail is [bnez unk50][sw v0,0x68][li 1][sw v0,0x50]
     with `sh zero,4(s0)` (command = 0) in the `j` delay slot, i.e. the pCameraPathHeights store was
     cross-jumped and takes its value in v0. This body emits the pCameraPathHeights store in the
     j delay and `sh zero,4` in the bnez delay instead.
   - case 0: retail emits [sh v0,0x20][sll a0,2][sh zero,0x22][sw a0,0x50]
     [lui/addu/lw g_WorldMapPathPointHeights][lw v0,0x50][nop][sll 1][addu][sw v1,0x54][lh][nop]
     [sw v0,0x58]; this body hoists the unk50 store and its reload above the two
     `sh` stores and drops the two load-delay nops.
   - camera-height ramp: retail keeps unk54 and g_WorldMapCameraHeight in a1/a0 and
     never reloads them; this body reloads both.
   - `nStep = 0x40000 >> 3` constant-folds to li 0x8000 here, retail has
     `lui v1,4` plus `sra v1,v1,3`, so the shift of the clamp constant was not folded
     in the original; no source spelling tried so far reproduces that.
   Listing the cases 14, 17, 9, 10 (not 9, 10, 14, 17) was worth 13 points - block
   emission order is the lever in this switch, so a permuter run from this body is
   the next step, not more hand shapes. Field types this body proved: command/state/
   stateTimer s16 (lh, slti, sh), unk54/unk60 s32 (blez, sra), pCameraPathAngles/pCameraPathHeights s16* (the angle
   and height tracks indexed by the path point). pRot = &g_WorldMapCameraRot is what
   makes retail hold the address in a3 across the compare and the func_80096F18 call;
   without it the three cases lose their separate arg-setup blocks.
s32 func_80091C18(s32 idx) {
    WorldMapPlayer *pPlayer;
    s16 *pPath;
    s16 *pRot;
    s32 nPoint;
    s32 nDelta;
    s32 nStep;
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
        pPlayer->pCameraPathAngles = g_WorldMapCameraPathAnglesMode7;
        pPlayer->pCameraPathHeights = g_WorldMapCameraPathHeightsMode7;
        pPlayer->command = 0;
        if (pPlayer->unk50 == 0) {
            pPlayer->unk50 = 1;
        }
        break;
    case 10:
        pPlayer->pCameraPathAngles = g_WorldMapCameraPathAnglesModes1To5;
        pPlayer->pCameraPathHeights = g_WorldMapCameraPathHeightsModes1To5;
        pPlayer->command = 0;
        if (pPlayer->unk50 == 0) {
            pPlayer->unk50 = 1;
        }
        break;
    }
    pPath = pPlayer->pCameraPathAngles;
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
        if (pPlayer->unk54 != g_WorldMapCameraHeight) {
            nDelta = pPlayer->unk54 - g_WorldMapCameraHeight;
            if (nDelta > 0) {
                if (nDelta > 0x40000) {
                    nStep = 0x40000 >> 3;
                } else {
                    nStep = nDelta >> 3;
                }
                if (nStep < 0x40) {
                    g_WorldMapCameraHeight = pPlayer->unk54;
                } else {
                    g_WorldMapCameraHeight += nStep;
                }
            } else {
                g_WorldMapCameraHeight -= 0x2000;
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
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091FF8);
/* best: 92.10%, same size (580 bytes). The whole structure is decoded and every
   relocation is right; what is left is gcc allocation/scheduling:
   - the three callee-saved roles are permuted (retail nX=s3, nBest=s4, i=s5;
     this compile nBest=s3, i=s4, nX=s5 - the loop counter lands in a different
     register, so the tail `move v0,s5`/`lw s5,16(sp)` pair differs),
   - `lui v0,0xfff8` (the 0xFFF80000 mask) is hoisted to the top of the block
     here and used as an early t0, retail emits it late and uses it twice,
   - retail fills the `beqz` delay slot of the inner `if (nZ < nBest)` with the
     `sw v1,4(s1)`; this compile leaves a nop there,
   - the final distance is `tbl + nBase - (nBest*8 - 0x50)`; gcc folds the 0x50
     onto the minuend here (`addiu v0,v0,80`) unless the subtrahend is held in a
     local, and then the second `slti` is duplicated into a delay slot.
   The 4th call argument must be `&D_1F8000A8` (a symbol reloc) - writing
   `pScratch + 0xA8` gives the same bytes with no reloc, and retail keeps the
   symbol *and* reuses $s1 as the high half.

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
        func_80096F18(&g_WorldMapPathFrame, &g_WorldMapCameraTarget, *pTrack - (g_WorldMapProjectionDistance / 2 << 12), (u16 *)&D_1F8000A8);
        nX = (g_WorldMapCameraTarget + ((g_WorldMapPathFrame - 0x180) << 12)) & 0xFFF80000;
        nY = (g_WorldMapCameraTargetZ - ((D_8009BD44 + 0x180) << 12)) & 0xFFF80000;
        *(s32 *)(pScratch + 8) = nY;
        for (j = 0; j < 6; j++) {
            *(s32 *)pScratch = nX;
            for (k = 0; k < 6; k++) {
                WorldMapWrapPosition(pScratch);
                nZ = *(s8 *)WorldMapGetTileEntry(*(s32 *)pScratch, *(s32 *)(pScratch + 8));
                if (nZ < nBest) {
                    *(s32 *)(pScratch + 4) = nZ;
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
        s32 nDist = (nBest << 3) - 0x50;
        nZ = ((s16 *)g_WorldMapCameraPathHeightsModes1To5)[nLimit] + nBase - nDist;
        if (nZ < 0) {
            nZ = -nZ;
        }
        if (nZ < 0x41) {
            i = nLimit;
        }
    }
    return i;
}
*/

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

#define WORLD_MAP_FADE_RECT_TPAGE_X 0x380
#define WORLD_MAP_FADE_RECT_TPAGE_Y 0x100
#define WORLD_MAP_FADE_LEVEL_MAX 0xFF

// Fades the two fade-rectangle primitives in or out by one draw-mode step per frame:
// command 0xC fades the level down and 0xD fades it up, and the return value drops to 3
// once the fade has finished.  The primitives are re-linked into the ordering table
// every frame because the level is a Gouraud colour.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800925A0);
/* best: 92.73%, 320 bytes = retail's exact size, frame reproduced.
   Reproduced exactly: the command dispatch (CASE ORDER IS THE LEVER - the switch lists
   0xD before 0xC, which is what makes gcc 2.7.2 emit the 0xD arm first and test 0xC with
   a forward beq as retail does; an if/else chain and the ascending case order both come
   out with the two arms swapped), the twelve Gouraud colour stores (each one re-reads
   pPlayer->unk50, so each right-hand side has to be a volatile read - a plain read folds
   to a single load), the two addPrim pairs, both fade-step clamps, and the shared
   fade-done tail (reached with a goto, which reproduces retail's out-of-line g_WorldMapFadeActive
   store block; duplicating that store inside both arms makes gcc emit it twice).
   FRAME: retail's frame is 0xE0 and only ra/s0/s1 are saved, so 0xC0 of stack is
   reserved and no instruction touches it; the unusedFrame local below is what reproduces
   it (0xC8 gives 0xE8, nothing gives 0x20). It is worth 0.02% in objdiff and is still
   required for check_bin.
   LEFT, all register allocation and no structural difference: the two addPrim mask
   constants land in a1/a2 instead of retail's a2/a3; g_WorldMapRenderContext is kept in
   t0 instead of a1; &g_WorldMapFadeTpage is reached through the %hi/%lo macro form for
   its first store where retail keeps the address in a0 (gcc folds a constant address,
   and a pointer local does not stop it); the prim base lui/addiu is scheduled after the
   index loads where retail does it before them; and the tail's move v0,s1 is stolen into
   the stateTimer lhu delay slot where retail leaves a nop (retail loads the timer into
   v0, which blocks that slot). A 4-minute permuter run from this body found only
   candidates that make the colour stores share one temp instead of reloading - wrong
   code, not ported.
   The colour stores name g_WorldMapFadePrims+4/+5/+6/+0xC.. where splat named
   D_8009CE70..D_8009CE8A; the linked bytes are identical.
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
        pTpage = &g_WorldMapFadeTpage;
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
        pTpage = &g_WorldMapFadeTpage;
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
}
*/
