#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern s32 func_80095414(s32 *pPos, s32 *pRot, s32 *pOut, s32 nSpeed, s32 nMode);
extern void WorldMapSetSpriteSetParent(int, int);
extern s16 g_WorldMapPathFrame;
extern s16 D_8009BD44;
extern s32 g_WorldMapCameraTarget;
extern s32 D_8009BE30;
extern s32 g_WorldMapScrollTargetY;
extern s32 g_WorldMapProjectionDistance;
extern s32 g_WorldMapPathPointHeights[];
extern u16 D_1F8000A8;
extern void func_80096F18(s16 *pOut, s32 *pSrc, s32 nHeight, u16 *pAngles);
extern void WorldMapWrapPosition(void *p);
extern u16 g_WorldMapPad1Released;
extern s16 g_WorldMapMode4AreaId;
extern s32 D_8009D804;
extern u8 *WorldMapGetTileEntry(s32 x, s32 z);
extern u8 g_WorldMapZoneId;
extern s16 g_WorldMapAreaId;
extern u16 g_WorldMapViewRotationY;
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
extern s16 g_WorldMapViewRotationZ;
extern u16 g_WorldMapPlayerAngle;
extern u32 g_WorldMapScrollTargetX[];
extern s16 g_WorldMapCameraPathAnglesModes1To5[];
extern s16 g_WorldMapCameraPathAnglesMode7[];
extern s16 g_WorldMapCameraPathHeightsModes1To5[];
extern s16 g_WorldMapCameraPathHeightsMode7[];
extern s32 D_8009BE2C;
extern SVECTOR D_8009BD48;
extern s32 g_WorldMapRotationMatrixValid;
extern s32 func_80091FF8(s32 nLimit, s16 *pIn, s16 *pOfs);

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 command;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 state;
    /* 0x22 */ s16 stateTimer;
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u8 unk26[0x2];
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ u32 dirX;
    /* 0x3C */ u32 dirY;
    /* 0x40 */ u32 dirZ;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ s16 angle;
    /* 0x4A */ u16 moveSpeed;
    /* 0x4C */ void *pSprite;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s16 *pCameraPathAngles;
    /* 0x68 */ s16 *pCameraPathHeights;
    /* 0x6C */ u32 unk6C;
    /* 0x70 */ s32 unk70;
    /* 0x74 */ u32 unk74;
    /* 0x78 */ u8 unk78[0x4];
    /* 0x7C */ s32 unk7C;
} WorldMapPlayer; // Size: 0x80

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 unk2[0x6];
    /* 0x08 */ s32 pos[4];
    /* 0x18 */ u8 unk18[0x4];
    /* 0x1C */ u16 rotZ;
    /* 0x1E */ u8 unk1E[0x2];
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

typedef struct {
    /* 0x00 */ u32 w[9];
} WorldMapG4; // Size: 0x24, two POLY_G4-shaped primitives are not it

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
/* best: 87.06% (860 bytes, retail 868 - 2 insns short). Decoded structure and
   every relocation is right; residues: (1) gcc fills the WorldMapGetGameStatePosition
   jal slot with `addiu a0,s1,0x28` and then emits li s3,1 before the `sh zero,0x24(s1)`
   store where retail has `addiu a0` FIRST, the store in the slot and li s3,1 after it.
   (2) the D_8009C5A8 compare chain and the scratchpad tail are two instructions short.
   Parked rather than left in C: the unit owns these tables (E190 owns jtbl_800709F8 and
   jtbl_80070A10 at +0/+0x18), so a body that is 8 bytes short shifts the overlay.
s32 func_8008E190(s32 idx) {
    WorldMapPlayer *pPlayer;
    u8 *pScratch;
    s32 nRet;
    u16 nAngle;

    pPlayer = g_WorldMapSlots + idx;
    WorldMapGetGameStatePosition(&pPlayer->x);
    pPlayer->unk24 = 0;
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
        g_WorldMapGroups->entry[3].unk0 = 1;
        g_WorldMapGroups->entry[2].unk0 = 1;
        g_WorldMapGroups->entry[1].unk0 = 1;
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
    g_WorldMapGroups->entry[0].unk0 = pPlayer->unk24;
    *(s16 *)(pScratch + 0xA0) = -(pPlayer->unk70 >> 12);
    *(s16 *)(pScratch + 0xA4) = g_WorldMapViewRotationZ;
    *(s16 *)(pScratch + 0xA2) = pPlayer->angle;
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
        pPlayer->unk7C = 0;
        break;
    case 4:
        pPlayer->state = 0x30;
        pPlayer->unk7C = 0;
        break;
    case 5:
        pPlayer->state = 0x34;
        pPlayer->unk7C = 0;
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
   argument (no change, gcc sinks the address to the argument anyway). PARKED FOR A
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
    g_WorldMapGroups->entry[0].unk0 = pPlayer->unk24;
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
        g_WorldMapGroups->entry[3].unk0 = 1;
        g_WorldMapGroups->entry[2].unk0 = 1;
        g_WorldMapGroups->entry[1].unk0 = 1;
        break;
    case 6:
    case 7:
        g_WorldMapGroups->entry[3].unk0 = 0;
        g_WorldMapGroups->entry[2].unk0 = 0;
        g_WorldMapGroups->entry[1].unk0 = 0;
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
    pPlayer->unk24 = 0;
    pPlayer->unk60 = 0;
    pPlayer->pCameraPathAngles = 0;
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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800907F4);
/* best: 98.27%. Structurally identical (same frame, same registers, two pointers
   at g_WorldMapGroups+0xA8/+0xFC, one scratchpad base in $s0 live across both calls).
   The only delta is the value in $s0: retail holds 0x1F800000 and so stores at
   0xA0/0xA2/0xA4/0xA8/0xAA/0xAC and passes the two RotMatrixZYX vectors with
   `ori a0,s0,0xA0` / `ori a0,s0,0xA8`; gcc 2.7.2 rematerialises the one-insn
   constant and derives the vectors as base+0xA0 with `addiu` (98.27), while a
   base of 0x1F8000A0 keeps a register but shifts all eight offsets (98.27 too,
   mirrored). Literal 0x1F8000xx addresses make reload use $at per store (74.29);
   cdk 72.92, so the preset is not the answer.

s32 func_800907F4(s32 idx) {
    WorldMapPlayer *pPlayer;
    WorldMapRot *pRotA;
    WorldMapRot *pRotB;

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
    *(u16 *)0x1F8000A8 = 0;
    *(u16 *)0x1F8000A0 = 0;
    *(u16 *)0x1F8000A2 = pPlayer->unk50;
    *(u16 *)0x1F8000AA = pPlayer->unk54;
    *(u16 *)0x1F8000A4 = pRotA->rotZ;
    *(u16 *)0x1F8000AC = pRotB->rotZ;
    RotMatrixZYX((SVECTOR *)0x1F8000A0, &pRotA->matrix);
    RotMatrixZYX((SVECTOR *)0x1F8000A8, &pRotB->matrix);
    return 1;
}*/
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
/* best: 99.91%, same 428/416-byte size and every instruction identical; only the
   switch table operand differs (jtbl_ vs .rodata) plus the .L label relocs.
   PARKED FOR A LAYOUT REASON, not a codegen one: gcc emits the C jump table
   8-byte aligned, so it lands at .rodata+0x190 where retail packs it at
   .rodata+0x18C (4 bytes of pad at 0x18C), which shifts all of
   world_map.bin by 4 and fails check_bin. The tables before it belong to
   the BLOCKED stubs (func_8008E190/8008E4F4/8008E76C/80090A84), so the unit
   cannot be re-laid-out for one function: converting the whole switch family
   of this unit at once is the way in. diff_func.sh says 99.9%, the bytes
   match, and angle must be s16 (rcos argument reads lh) and g_WorldMapViewRotationY u16
   (lhu in every case body).

s32 func_80090C68(WorldMapPlayer *pPlayer) {
    switch (g_WorldMapPad1Held >> 12) {
    case 1:
        pPlayer->angle = g_WorldMapViewRotationY;
        break;
    case 2:
        pPlayer->angle = (g_WorldMapViewRotationY + 0x400) & 0xFFF;
        break;
    case 3:
        pPlayer->angle = (g_WorldMapViewRotationY + 0x200) & 0xFFF;
        break;
    case 4:
        pPlayer->angle = (g_WorldMapViewRotationY + 0x800) & 0xFFF;
        break;
    case 6:
        pPlayer->angle = (g_WorldMapViewRotationY + 0x600) & 0xFFF;
        break;
    case 8:
        pPlayer->angle = (g_WorldMapViewRotationY - 0x400) & 0xFFF;
        break;
    case 9:
        pPlayer->angle = (g_WorldMapViewRotationY - 0x200) & 0xFFF;
        break;
    case 12:
        pPlayer->angle = (g_WorldMapViewRotationY - 0x600) & 0xFFF;
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
            D_8009D804 = 1;
        }
    }
    WorldMapUpdateL2R2PressEdge();
    return 0;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090E14);
/* best: 99.90%, same 428/416-byte size and every instruction identical; only the
   switch table operand differs (jtbl_ vs .rodata) plus the .L label relocs.
   PARKED FOR A LAYOUT REASON, not a codegen one: gcc emits the C jump table
   8-byte aligned, so it lands at .rodata+0x190 where retail packs it at
   .rodata+0x18C (4 bytes of pad at 0x18C), which shifts all of
   world_map.bin by 4 and fails check_bin. The tables before it belong to
   the BLOCKED stubs (func_8008E190/8008E4F4/8008E76C/80090A84), so the unit
   cannot be re-laid-out for one function: converting the whole switch family
   of this unit at once is the way in. diff_func.sh says 99.9%, the bytes
   match, and angle must be s16 (rcos argument reads lh) and g_WorldMapViewRotationY u16
   (lhu in every case body).

s32 func_80090E14(WorldMapPlayer *pPlayer) {
    switch (g_WorldMapPad1Held >> 12) {
    case 1:
        pPlayer->angle = g_WorldMapViewRotationY;
        break;
    case 2:
        pPlayer->angle = (g_WorldMapViewRotationY + 0x400) & 0xFFF;
        break;
    case 3:
        pPlayer->angle = (g_WorldMapViewRotationY + 0x200) & 0xFFF;
        break;
    case 4:
        pPlayer->angle = (g_WorldMapViewRotationY + 0x800) & 0xFFF;
        break;
    case 6:
        pPlayer->angle = (g_WorldMapViewRotationY + 0x600) & 0xFFF;
        break;
    case 8:
        pPlayer->angle = (g_WorldMapViewRotationY - 0x400) & 0xFFF;
        break;
    case 9:
        pPlayer->angle = (g_WorldMapViewRotationY - 0x200) & 0xFFF;
        break;
    case 12:
        pPlayer->angle = (g_WorldMapViewRotationY - 0x600) & 0xFFF;
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
            D_8009D804 = 1;
        }
    }
    return 0;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090FB4);
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
    pRotY = &g_WorldMapViewRotationY;
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
        pPlayer->pCameraPathAngles = g_WorldMapCameraPathAnglesModes1To5;
        pPlayer->pCameraPathHeights = g_WorldMapCameraPathHeightsModes1To5;
        break;
    case 6:
        break;
    case 7:
        pPlayer->unk50 = 3;
        g_WorldMapCameraHeight = 0x280000;
        g_WorldMapCameraRot = -0x260;
        pPlayer->pCameraPathAngles = g_WorldMapCameraPathAnglesMode7;
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
        D_8009BD48.vy = D_8009BE2C >> 12;
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
        nY = (D_8009BE30 - ((D_8009BD44 + 0x180) << 12)) & 0xFFF80000;
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
// The original touches every field of the two primitives at 0x8009CE6C, so
// splat gave each field its own address symbol; naming them here keeps the
// %lo relocations identical to the original object.
extern WorldMapG4 g_WorldMapFadePrims;
extern u8 D_8009CE6F;
extern u8 D_8009CE70;
extern u8 D_8009CE71;
extern u8 D_8009CE72;
extern u8 D_8009CE73;
extern u16 D_8009CE74;
extern u16 D_8009CE76;
extern u8 D_8009CE78;
extern u8 D_8009CE79;
extern u8 D_8009CE7A;
extern u16 D_8009CE7C;
extern u16 D_8009CE7E;
extern u8 D_8009CE80;
extern u8 D_8009CE81;
extern u8 D_8009CE82;
extern u16 D_8009CE84;
extern u16 D_8009CE86;
extern u8 D_8009CE88;
extern u8 D_8009CE89;
extern u8 D_8009CE8A;
extern u16 D_8009CE8C;
extern u16 D_8009CE8E;
extern u16 D_8009CE98;
extern u16 D_8009CE9A;
extern u16 D_8009CEA0;
extern u16 D_8009CEA2;
extern u16 D_8009CEA8;
extern u16 D_8009CEAA;
extern u16 D_8009CEB0;
extern u16 D_8009CEB2;
extern DR_TPAGE g_WorldMapFadeTpage;
extern s32 g_WorldMapTpageAbr;

s32 WorldMapModeInitFadeRect(s32 idx) {
    WorldMapPlayer *pPlayer;
    WorldMapG4 *pG4;
    u_short nTpage;

    pPlayer = g_WorldMapSlots + idx;
    pPlayer->unk50 = 0xF8;
    nTpage = GetTPage(0, g_WorldMapTpageAbr, 0x380, 0x100);
    SetDrawTPage(&g_WorldMapFadeTpage, 1, 0, nTpage);
    pG4 = &g_WorldMapFadePrims;
    // the original re-reads unk50 for all twelve colour bytes; volatile stops
    // gcc folding them into one load.
    D_8009CE6F = 8;
    D_8009CE73 = 0x38;
    D_8009CE70 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE71 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE72 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE78 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE79 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE7A = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE80 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE81 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE82 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE88 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE89 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE8A = *(volatile s32 *)&pPlayer->unk50;
    SetSemiTrans(pG4, 1);
    pG4[1] = pG4[0];
    D_8009CE74 = 0;
    D_8009CE76 = 0;
    D_8009CE7C = 0x140;
    D_8009CE7E = 0;
    D_8009CE84 = 0;
    D_8009CE86 = 0xD8;
    D_8009CE8C = 0x140;
    D_8009CE8E = 0xD8;
    D_8009CE98 = 0;
    D_8009CE9A = 0;
    D_8009CEA0 = 0x140;
    D_8009CEA2 = 0;
    D_8009CEA8 = 0;
    D_8009CEAA = 0xD8;
    D_8009CEB0 = 0x140;
    D_8009CEB2 = 0xD8;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800925A0);
