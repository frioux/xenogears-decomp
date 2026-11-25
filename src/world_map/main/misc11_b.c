#include "common.h"
#include "main/game.h"
#include "psyq/libgte.h"

// Tail of world_map/main/misc11, split into its own unit (config/world_map.yaml) because this
// object's .rodata range starts at file 0xDE4 (4 mod 8) while misc11's starts at 0xA28 (0 mod 8):
// cc1 hardcodes .align 3 before a switch table, so WorldMapUpdateGearActorSlot's two tables only land on retail's
// 0xDE4/0xE04 at internal +0/+0x20, i.e. from a range of that parity.

// World map view of the sprite object SpriteAllocFromAnimFile returns (SpriteData in
// include/field/actor.h). Only the two fields this unit touches are named: it clears
// bit 2 of the 0x3C flag word as a whole word, which that header's bitfields cannot
// express, and compares the current animation id, byte 3 of the header's 0xAC word.
typedef struct {
    /* 0x00 */ undefined32 unk00[15]; // 0x00..0x3B
    /* 0x3C */ u32 flags;
    /* 0x40 */ undefined8 unk40[0x6F]; // 0x40..0xAE
    /* 0xAF */ s8 animId;
} WorldMapSprite; // Size: 0xB0

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ s16 command;
    /* 0x06 */ s16 targetSlot;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ s16 state;
    /* 0x22 */ u16 stateTimer;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ undefined16 pad26;
    /* 0x28 */ s32 x;
    /* 0x2C */ s32 y;
    /* 0x30 */ s32 z;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ s32 dirX;
    /* 0x3C */ u32 dirY;
    /* 0x40 */ s32 dirZ;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ s16 angle;
    /* 0x4A */ s16 moveSpeed;
    /* 0x4C */ WorldMapSprite *pSprite;
    /* 0x50 */ s32 targetX;
    /* 0x54 */ s32 targetZ;
    /* 0x58 */ s32 trailDelay;
    /* 0x5C */ s32 drawAngle;
    /* 0x60 */ undefined32 unk60[8]; // 0x60..0x7F
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x000 */ undefined32 unk000[40]; // 0x000..0x09F
    /* 0x0A0 */ s16 x;
    /* 0x0A2 */ s16 y;
    /* 0x0A4 */ s16 z;
    /* 0x0A6 */ undefined16 pad0A6;
    /* 0x0A8 */ s16 rotX;
    /* 0x0AA */ s16 rotY;
    /* 0x0AC */ s16 rotZ;
    /* 0x0AE */ undefined16 pad0AE;
} WorldMapScreenPos; // Size: 0xB0

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 unk0C;
} WorldMapVec; // Size: 0x10

typedef struct {
    /* 0x00 */ WorldMapVec vec;
    /* 0x10 */ u16 angle;
    /* 0x12 */ u16 pad12;
} WorldMapTrail; // Size: 0x14

// 12.4 fixed point world map position; the range test below uses the middle axis too.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

extern WorldMapActor *g_WorldMapSlots;
extern s32 g_WorldMapMode;
extern WorldMapTrail g_WorldMapTrail[];
extern s16 g_WorldMapTrailCursor;
extern s32 g_WorldMapPlayerPosition;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapPlayerRotation;
extern void *D_8009BE00[];

extern s32 func_8008C364(WorldMapActor *, s32);
extern void WorldMapInitSpriteGroupSprites(s32, void *, void *);
extern void WorldMapGetGameStatePosition(void *);
extern void WorldMapDeactivateSpriteGroup(s32);
extern s32 WorldMapGetTilePlaneHeight(s32, s32);
extern WorldMapSprite *SpriteAllocFromAnimFile(void *, s16, s16, s16, s16, s16);
extern void SpritePlayAnimation(void *, s32);
extern void SpriteSetScale(void *, s16);
extern void WorldMapBuildDirectionFromPositions(WorldMapPos *, WorldMapPos *, s32 *, s16 *);
extern void WorldMapAddSlotPosition(s16, s32 *);
extern s32 WorldMapStartSlot(s32, s32);
extern void WorldMapUpdateSpriteGroupFromActor(s32, WorldMapActor *, WorldMapScreenPos *);
extern s32 WorldMapMoveActorTowardTarget(WorldMapActor *);

// 100% (445 insns / 1780 bytes = retail's exact size; the only differing rows are the two
// jtbl_/.rodata table operands, which is this overlay's owned-table form - check_bin OK).
// Ported from the matched twin WorldMapUpdateActorSlot: same command/state machine family.
// Measured constraints: the state switch is 65 entries only because case 64 (state 0x40) is a
// live case whose body is the exit; the 0x30 arm (case 48) does NOT fall through into 49
// (retail jumps to the shared state store); the countdowns are (s16)--pActor->stateTimer <= 0 (u16
// field read with lhu, so the test is signed); slot = index - 4 must stay an s32 local
// (folding it into the constant costs the addiu and adds an sll/sra pair). The scratchpad is
// the one argument area: +0x0 is the WorldMapPos and WorldMapScreenPos the calls read, +0xA0
// is the SVECTOR WorldMapInitSpriteGroupSprites takes.
int WorldMapUpdateGearActorSlot(int index) {
    s32 *pScratch = (s32 *)PSX_SCRATCH;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapActor *pTarget;
    WorldMapTrail *pTrail;
    s16 angle;
    s32 slot;

    switch (pActor->command) {
    case 1:
        pActor->state = 8;
        pTarget = &g_WorldMapSlots[pActor->targetSlot];
        pActor->command = 0;
        break;
    case 4:
        pActor->state = 0x40;
        pActor->command = 0;
        g_GameState.gearRide[index - 4] = 1;
        break;
    case 5:
        pActor->command = 0;
        pActor->state = 0x10;
        break;
    case 2:
        pActor->command = 0;
        pActor->state = 0x20;
        break;
    case 8:
        pActor->command = 0;
        pActor->state = 0x18;
        break;
    case 3:
        pActor->command = 0;
        pActor->state = 0x30;
        break;
    }
    switch (pActor->state) {
    case 0:
    case 1:
        if (g_GameState.gearRide[index - 4] == 1) {
            pTrail = &g_WorldMapTrail[(g_WorldMapTrailCursor - pActor->trailDelay) & 0x1F];
            if ((pActor->x == pTrail->vec.x) & (pActor->y == pTrail->vec.y) &
                (pActor->z == pTrail->vec.z)) {
                if (pActor->pSprite->animId != 0) {
                    SpritePlayAnimation(pActor->pSprite, 0);
                    WorldMapDeactivateSpriteGroup(index + 0x28);
                }
            } else {
                if (pActor->pSprite->animId != 1) {
                    SpritePlayAnimation(pActor->pSprite, 1);
                }
                WorldMapUpdateSpriteGroupFromActor(index + 0x28, pActor, (WorldMapScreenPos *)pScratch);
            }
            pActor->x = pTrail->vec.x;
            pActor->y = pTrail->vec.y;
            pActor->z = pTrail->vec.z;
            pActor->angle = pTrail->angle;
        } else {
            if (pActor->pSprite->animId != 3) {
                SpritePlayAnimation(pActor->pSprite, 3);
                WorldMapDeactivateSpriteGroup(index + 0x28);
            }
        }
        break;
    case 2:
        pActor->x = g_WorldMapSlots[7].x;
        pActor->z = g_WorldMapSlots[7].z;
        pActor->angle = g_WorldMapSlots[7].angle;
        break;
    case 8:
        WorldMapBuildDirectionFromPositions(&pActor->x, &pTarget->x, &pActor->dirX, &pActor->angle);
        pActor->targetX = pTarget->x >> 12;
        pActor->targetZ = pTarget->z >> 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        // falls through
    case 9:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->state++;
        }
        WorldMapUpdateSpriteGroupFromActor(index + 0x28, pActor, (WorldMapScreenPos *)pScratch);
        break;
    case 10:
        if (WorldMapStartSlot(pActor->targetSlot, 4)) {
            pActor->unk24 = 1;
            pActor->state = 2;
            WorldMapDeactivateSpriteGroup(index + 0x28);
        }
        break;
    case 16:
        pActor->targetX = g_WorldMapSlots[4].x;
        pActor->targetZ = g_WorldMapSlots[4].z;
        pScratch[0] = pActor->targetX - pActor->x;
        pScratch[2] = pActor->targetZ - pActor->z;
        pActor->angle = (ratan2(pScratch[2], pScratch[0]) + 0x400) & 0xFFF;
        pActor->dirX = rcos(pActor->angle);
        pActor->dirZ = -rsin(pActor->angle);
        pActor->targetX >>= 12;
        pActor->targetZ >>= 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        // falls through
    case 17:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->state++;
        }
        break;
    case 18:
        if (WorldMapStartSlot(4, 7)) {
            pActor->state = 1;
        }
        break;
    case 24:
        if (g_GameState.partyMembers[index - 4] == 7) {
            pActor->stateTimer = 1;
            pActor->state = 0x1A;
        } else {
            SVECTOR *pSpritePos = (SVECTOR *)&pScratch[0x28];

            pSpritePos->vx = pActor->x >> 12;
            pSpritePos->vy = pActor->y >> 12;
            pSpritePos->vz = pActor->z >> 12;
            WorldMapInitSpriteGroupSprites(index + 1, pSpritePos, 0);
            pActor->stateTimer = 8;
            pActor->state++;
        }
        break;
    case 25:
        if ((s16)--pActor->stateTimer <= 0) {
            pActor->unk24 = 1;
            pActor->stateTimer = 0x10;
            pActor->state++;
        }
        break;
    case 26:
        if ((s16)--pActor->stateTimer <= 0) {
            SpritePlayAnimation(pActor->pSprite, 3);
            pActor->unk24 = 1;
            pActor->state = 2;
        }
        break;
    case 32:
        SpritePlayAnimation(pActor->pSprite, 0);
        pActor->dirZ = 0;
        pActor->dirY = 0;
        pActor->dirX = 0;
        pActor->state++;
        break;
    case 33:
        if (WorldMapStartSlot(index - 3, 2)) {
            g_WorldMapSlots[index - 3].targetSlot = index;
            pActor->state++;
        }
        break;
    case 34:
        pActor->state = 0;
        break;
    case 48:
        WorldMapGetGameStatePosition(&pActor->x);
        pActor->unk24 = 0;
        angle = g_WorldMapSlots[7].unk60[6];
        pActor->angle = angle;
        pScratch[0] = pActor->x + rcos(angle) * 0x60;
        pScratch[2] = pActor->z + (-rsin(pActor->angle)) * 0x60;
        WorldMapBuildDirectionFromPositions(&pActor->x, (WorldMapPos *)pScratch, &pActor->dirX, &pActor->angle);
        pActor->targetX = pScratch[0] >> 12;
        pActor->targetZ = pScratch[2] >> 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        break;
    case 49:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            SpritePlayAnimation(pActor->pSprite, 0);
            pActor->dirZ = 0;
            pActor->dirY = 0;
            pActor->dirX = 0;
            pActor->state++;
        }
        break;
    case 50:
        pActor->state = 1;
        break;
    case 64:
        break;
    }
    if (pActor->state != 2 &&
        (g_WorldMapMode == 2 || g_GameState.partyMembers[index - 4] != 7)) {
        WorldMapAddSlotPosition(1, &pActor->x);
    }
    slot = index - 4;
    g_GameState.fieldScriptMemory[0x16 + slot * 3] = pActor->x >> 12;
    g_GameState.fieldScriptMemory[0x17 + slot * 3] = pActor->z >> 12;
    ((u16 *)&g_GameState.unk1826)[index - 4] = pActor->angle;
    return 1;
}


// 81.33% (304 bytes, retail 300). Round 186: re-parked with the current field names
// (pSprite, x, y, z, dirX/Y/Z, angle, moveSpeed, trailDelay, drawAngle, state) - it
// builds now and re-measures at 81.33. The same two head differences as WorldMapInitPartyActor1:
// retail stores the three zeros (0x40/0x3C/0x38) right after the func_8008C364 call and
// loads g_GameState+0x182A above them (gcc sinks them below the g_WorldMapMode load),
// and retail re-reads angle with lh for drawAngle where gcc keeps the register and
// emits an sll/sra pair - exactly the 4 extra bytes (304 vs 300). The whole gearRide[2]
// block, the mode<8 arm and the tail are byte-identical.
/*
int func_8008DD6C(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 2);
    s32 mode;

    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->moveSpeed = 0xC;
    pActor->trailDelay = 0x1F;
    pActor->angle = *(u16 *)&g_GameState.unk1826[4];
    pActor->drawAngle = pActor->angle;
    mode = g_WorldMapMode;
    if (mode > 0) {
        if (mode < 4) {
            if (g_GameState.gearRide[2] == 1) {
                pActor->state = 1;
                pActor->x = g_WorldMapPlayerPosition;
                pActor->z = g_WorldMapPlayerPositionZ;
                pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
                pActor->angle = g_WorldMapPlayerRotation;
            }
        } else if (mode < 8) {
            pActor->state = 2;
            pActor->unk24 = 1;
        }
    }
    g_GameState.fieldScriptMemory[0x1C] = pActor->x >> 12;
    g_GameState.fieldScriptMemory[0x1D] = pActor->z >> 12;
    *(u16 *)&g_GameState.unk1826[4] = pActor->angle;
    return result;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11_b", func_8008DD6C);

int WorldMapModeInitPartyActor2(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 2);
    s32 mode = g_WorldMapMode;

    if (mode > 0) {
        if (mode < 4) {
            return result;
        }
        if (mode < 8) {
            pActor->unk24 = 1;
        }
    }
    return result;
}
int WorldMapInitPartyActorSprite2(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    pActor->pSprite = SpriteAllocFromAnimFile(D_8009BE00[0], 0x100, 0x1FB, 0x280, 0x100, 0x40);
    SpritePlayAnimation(pActor->pSprite, 0);
    SpriteSetScale(pActor->pSprite, 0x2000);
    pActor->pSprite->flags &= ~4;
    pActor->x = g_GameState.fieldScriptMemory[0x1C] << 12;
    pActor->z = g_GameState.fieldScriptMemory[0x1D] << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    pActor->unk24 = 1;
    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->moveSpeed = 0xC;
    pActor->angle = *(u16 *)&g_GameState.unk1826[4];
    return 1;
}
