#include "common.h"
#include "main/game.h"

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
    /* 0x22 */ u16 unk22;
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
    /* 0x54 */ void *targetZ;
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
    /* 0x00 */ s16 id;
    /* 0x02 */ undefined16 pad02;
    /* 0x04 */ s32 x;
    /* 0x08 */ s32 y;
    /* 0x0C */ s32 z;
    /* 0x10 */ s32 height;
    /* 0x14 */ s16 radius;
    /* 0x16 */ undefined16 pad16;
} WorldMapEntry; // Size: 0x18

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
extern s16 D_8009BD04;
extern WorldMapEntry D_8009BE6C[];
extern s16 D_8009B18C[];
extern s16 D_8009B194[];
extern s16 D_8009B19C[];
extern s16 D_8009B1A4[];
extern void *g_WorldMapActorAnimFiles[];
extern void *g_WorldMapCharacterAnimFiles;
extern void *D_8009CD3C;
extern void WorldMapWrapDeltaScaled(WorldMapPos *);
extern s32 SquareRoot0(s32);

extern s32 func_8008C364(WorldMapActor *, s32);
extern void WorldMapInitSpriteGroupSprites(s32, void *, void *);
extern void WorldMapDeactivateSpriteGroup(s32);
extern s16 WorldMapGetTileHalfIndex(void *);
extern s32 WorldMapGetTilePlaneHeight(s32, s32);
extern void *D_8009BDFC[];
extern void *D_8009BE00[];
extern WorldMapSprite *SpriteAllocFromAnimFile(void *, s16, s16, s16, s16, s16);
extern void SpritePlayAnimation(void *, s32);
extern void SpriteSetScale(void *, s16);
extern void WorldMapWrapPosition(void *);
extern s32 rcos(s32);
extern s32 rsin(s32);
extern void WorldMapBuildDirectionFromPositions(WorldMapPos *, WorldMapPos *, s32 *, s16 *);
extern void WorldMapAddSlotPosition(s16, s32 *);
extern s32 WorldMapStartSlot(s32, s32);
extern s32 g_WorldMapPlayerPosition;
extern s32 g_WorldMapPlayerPositionZ;
extern s32 g_WorldMapPlayerRotation;
extern WorldMapVec g_WorldMapScrollTargetX;
extern WorldMapTrail g_WorldMapTrail[];
extern u16 g_WorldMapPlayerAngle;
extern s16 g_WorldMapTrailCursor;

/* func_8008A72C, WorldMapModeInitActorSlot, func_8008BB40, func_8008C364, func_8008C844 and
   func_8008D678 each jr through a jtbl_ table of this unit; WorldMapUpdateActorSlot did too
   and is now C. The unit owns its .rodata (config/world_map.yaml), and cc1 emits a
   function's table where the function is in the file, so converting a stub keeps
   every table in place - only that function's own instructions and its table
   words change (proved with check_bin.sh for WorldMapUpdateActorSlot). */
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008A72C);
int WorldMapModeInitActorSlot(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 mode;
    s32 result = 1;

    if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
        pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapCharacterAnimFiles, 0x110, 0x1E0, 0x150, 0x100, 0x40);
        SpritePlayAnimation(pActor->pSprite, 0);
        SpriteSetScale(pActor->pSprite, 0x1800);
        pActor->pSprite->flags &= ~4;
        pActor->unk24 = 0;
    } else {
        pActor->unk24 = 1;
        result = 3;
    }
    pActor->x = g_GameState.worldMapPosX << 12;
    pActor->z = g_GameState.worldMapPosZ << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->angle = g_GameState.worldMapPlayerRotation;
    mode = g_WorldMapMode;
    pActor->moveSpeed = 8;
    pActor->trailDelay = 0xF;
    pActor->drawAngle = pActor->angle;
    switch (mode) {
    case 1:
    case 2:
    case 3:
        if (g_GameState.gearRide[1] == 0) {
            // volatile: keeps the g_WorldMapPlayerPositionZ load below this store, as retail has it
            *(volatile s32 *)&pActor->x = g_WorldMapPlayerPosition;
            pActor->z = g_WorldMapPlayerPositionZ;
            pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
            pActor->angle = g_WorldMapPlayerRotation;
        } else {
            pActor->state = 1;
            pActor->unk24 = 1;
        }
        break;
    case 4:
    case 5:
    case 6:
        pActor->state = 3;
        pActor->unk24 = 1;
        if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
            g_GameState.gearRide[1] = 1;
        }
        break;
    case 7:
        pActor->state = 2;
        pActor->unk24 = 1;
        if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
            g_GameState.gearRide[1] = 1;
        }
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        break;
    }
    return result;
}
int WorldMapModeInitCharacterSprite1(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = 1;

    if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
        pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapCharacterAnimFiles, 0x110, 0x1E0, 0x150, 0x100, 0x40);
        SpritePlayAnimation(pActor->pSprite, 0);
        SpriteSetScale(pActor->pSprite, 0x1800);
        pActor->pSprite->flags &= ~4;
    } else {
        result = 3;
    }
    return result;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008B54C);
/* 93.37% (248 bytes, retail 248 - same size, same multiset). Round 186 un-parked
   this body and re-measured it live against the current field names: still 93.37,
   so the number is real and the body builds; the -O2 sched2 prologue order below is
   the only difference. Everything is
   byte-identical except three insns of the prologue: retail does the index shift
   FIRST and into v1 (sll v1,a0,7 before the 0x100 push) and stores the 0x40 stack
   argument last, right before the jal, so v0 holds that constant and the
   "addu s0,s0,v1" lands in the jal delay slot; gcc stores the 0x40 right after
   "li v0,0x40" and puts the shift in v0 afterwards.
   Round 167: the parked body used to carry the OLD field names (unk4C/unk28/...),
   so it no longer compiled and every percentage in the older notes came from a
   stale object - it is re-measured at 93.37 with the current field names (pSprite,
   x, y, z, state, dirX/Y/Z, angle, moveSpeed, trailDelay). The permuter now reads
   it and found PERMUTER_BETTER 410 -> 315 in 1608 iterations, but the winning edit
   is a DEAD END / wrong code: it casts the 0x100 argument to (unsigned char),
   which truncates it to 0, i.e. it buys the score by changing what the call is
   passed (same trap as round 155's WorldMapModeInitActorSlot candidate). Do not port it.
   Hand shapes already tried and measured no better: a pSprite local used by all
   three calls (WorldMapInitActorSpriteBySlot style), the pointer built as
   (u8 *)g_WorldMapSlots + index * 0x80, a flat tail without the angle local
   (83.05), cdk (83.19). WorldMapInitPartyActorSprite0 (matched) has this exact
   call shape and does reproduce retail's prologue, so the trigger is this
   function's tail, not the call.
int func_8008B54C(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    u16 angle;

    pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapCharacterAnimFiles, 0x110, 0x1E0, 0x150, 0x100, 0x40);
    SpritePlayAnimation(pActor->pSprite, 0);
    SpriteSetScale(pActor->pSprite, 0x1800);
    pActor->pSprite->flags &= ~4;
    pActor->x = g_GameState.worldMapPosX << 12;
    pActor->z = g_GameState.worldMapPosZ << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    angle = g_GameState.worldMapPlayerRotation;
    pActor->unk24 = 1;
    pActor->moveSpeed = 8;
    pActor->state = 2;
    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->trailDelay = 0xF;
    pActor->angle = angle;
    return 1;
}
*/
int WorldMapUpdateActorSlot(int index) {
    s32 result = 1;
    WorldMapPos *pScratch = (WorldMapPos *)0x1F800000;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapActor *pTarget;
    WorldMapTrail *pTrail;
    u16 angle;

    switch (pActor->command) {
    case 1:
        pTarget = &g_WorldMapSlots[pActor->targetSlot];
        pActor->command = 0;
        pActor->state = 8;
        break;
    case 2:
        pActor->command = 0;
        pActor->state = 0x28;
        break;
    case 3:
        pActor->command = 0;
        pActor->state = 1;
        break;
    case 5:
        pActor->command = 0;
        pActor->state = 0x30;
        break;
    }
    switch (pActor->state) {
    case 0:
    case 1:
        if (((u8 *)&g_GameState.unk22B0)[index] == 0) {
            pTrail = &g_WorldMapTrail[(g_WorldMapTrailCursor - pActor->trailDelay) & 0x1F];
            if ((pActor->x == pTrail->vec.x) & (pActor->y == pTrail->vec.y) &
                (pActor->z == pTrail->vec.z)) {
                if (pActor->pSprite->animId != 0) {
                    SpritePlayAnimation(pActor->pSprite, 0);
                    WorldMapDeactivateSpriteGroup(index + 0x2E);
                }
            } else {
                if (pActor->pSprite->animId != 1) {
                    SpritePlayAnimation(pActor->pSprite, 1);
                }
                WorldMapUpdateSpriteGroupFromActor(index + 0x2E, pActor, (WorldMapScreenPos *)pScratch);
            }
            pActor->x = pTrail->vec.x;
            pActor->y = pTrail->vec.y;
            pActor->z = pTrail->vec.z;
            pActor->angle = pTrail->angle;
            pActor->unk24 = 0;
        } else {
            pTarget = &g_WorldMapSlots[index + 3];
            pActor->x = pTarget->x;
            pActor->y = pTarget->y;
            pActor->z = pTarget->z;
            pActor->angle = pTarget->angle;
            pActor->unk24 = 1;
            WorldMapDeactivateSpriteGroup(index + 0x2E);
        }
        break;
    case 2:
        pActor->x = g_WorldMapSlots[7].x;
        pActor->y = g_WorldMapSlots[7].y;
        pActor->z = g_WorldMapSlots[7].z;
        pActor->angle = g_WorldMapSlots[7].angle;
        break;
    case 8:
        WorldMapBuildDirectionFromPositions(&pActor->x, &pTarget->x, &pActor->dirX, &pActor->angle);
        pActor->targetX = pTarget->x >> 12;
        pActor->targetZ = pTarget->z >> 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        /* falls through */
    case 9:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->state++;
        }
        WorldMapUpdateSpriteGroupFromActor(index + 0x2E, pActor, (WorldMapScreenPos *)pScratch);
        break;
    case 10:
        if (WorldMapStartSlot(pActor->targetSlot, 4)) {
            pActor->unk24 = 1;
            pActor->state = 2;
            WorldMapDeactivateSpriteGroup(index + 0x2E);
        }
        break;
    case 40:
        pActor->x = g_GameState.fieldScriptMemory[0x13 + index * 3] << 12;
        pActor->z = g_GameState.fieldScriptMemory[0x14 + index * 3] << 12;
        pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
        angle = (&g_GameState.worldMapPlayerRotation)[index];
        pActor->drawAngle = angle;
        pActor->angle = angle;
        pScratch->x = pActor->x + rcos((s16)angle) * 0x30;
        pScratch->z = pActor->z + (-rsin(pActor->angle)) * 0x30;
        WorldMapBuildDirectionFromPositions(&pActor->x, pScratch, &pActor->dirX, &pActor->angle);
        pActor->targetX = pScratch->x >> 12;
        pActor->targetZ = pScratch->z >> 12;
        pActor->unk24 = 0;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        /* falls through */
    case 41:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->state++;
        }
        break;
    case 42:
        ((u8 *)&g_GameState.unk22B0)[index] = 0;
        pActor->state = 0x40;
        break;
    case 48:
        WorldMapBuildDirectionFromPositions(&pActor->x, &g_WorldMapSlots[1].x, &pActor->dirX,
                                           &pActor->angle);
        pActor->targetX = g_WorldMapSlots[1].x >> 12;
        pActor->targetZ = g_WorldMapSlots[1].z >> 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        /* falls through */
    case 49:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->state++;
        }
        break;
    case 50:
        if (WorldMapStartSlot(1, 6)) {
            pActor->state = 0;
        }
        break;
    case 64:
        /* reached from case 42; nothing to do, it falls through to the tail */
        break;
    }
    if (pActor->unk24 == 0) {
        WorldMapAddSlotPosition(0, &pActor->x);
    }
    return result;
}
// 87.64% (476 bytes, retail 476). Round 186 un-parked this body and rebuilt it against
// the current field names (pSprite, x, y, z, dirX/Y/Z, angle, moveSpeed, trailDelay,
// state): it compiles and still measures 87.64, so the number is real, not a stale
// object. The whole head before the tile-height call, the if/else inversion
// ("if (gearRide[2] == 0) { gear position } else { state = 1; unk24 = 1; }", which puts
// the short block out of line behind bnez as retail has it) and the whole switch
// (cases 1-3 gear, 4-6 state = 3, 7 state = 2, 8-13 empty - the empty cases are
// required, retail's table is 13 words) are byte-identical.
// What is left is one sched2 tie in the block that sets up dirX/dirY/dirZ..drawAngle:
// retail emits the three zero stores, the lhu of g_GameState+0x1824 (worldMapPlayerRotation)
// and the g_WorldMapMode load ABOVE the moveSpeed/trailDelay constant stores, and
// re-reads angle with lh for drawAngle (sh 0x48 then lh 0x48); gcc sinks those loads
// below the stores and folds the re-read into an sll/sra pair. C store order does not
// move it (sched2 runs post-reload). Tried and measured worse: volatile u16 store for
// angle (reproduces the lh, loses the rest, 84.34), reading angle into a volatile u32
// local before the store (72.12). The permuter from this body found nothing better.
/*
int func_8008BB40(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = 1;
    if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
        pActor->pSprite = SpriteAllocFromAnimFile(D_8009CD3C, 0x120, 0x1E0, 0x160, 0x100, 0x40);
        SpritePlayAnimation(pActor->pSprite, 0);
        SpriteSetScale(pActor->pSprite, 0x1800);
        pActor->pSprite->flags &= ~4;
        pActor->unk24 = 0;
    } else {
        pActor->unk24 = 1;
        result = 3;
    }
    pActor->x = g_GameState.worldMapPosX << 12;
    pActor->z = g_GameState.worldMapPosZ << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->moveSpeed = 8;
    pActor->trailDelay = 0x1E;
    pActor->angle = g_GameState.worldMapPlayerRotation;
    pActor->drawAngle = pActor->angle;
    switch (g_WorldMapMode) {
    case 1:
    case 2:
    case 3:
        if (g_GameState.gearRide[2] == 0) {
            pActor->x = g_WorldMapPlayerPosition;
            pActor->z = g_WorldMapPlayerPositionZ;
            pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
            pActor->angle = g_WorldMapPlayerRotation;
        } else {
            pActor->state = 1;
            pActor->unk24 = 1;
        }
        break;
    case 4:
    case 5:
    case 6:
        pActor->state = 3;
        pActor->unk24 = 1;
        if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
            g_GameState.gearRide[2] = 1;
        }
        break;
    case 7:
        pActor->state = 2;
        pActor->unk24 = 1;
        if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
            g_GameState.gearRide[2] = 1;
        }
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        break;
    }
    return result;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BB40);

int WorldMapModeInitCharacterSprite2(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = 1;

    if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
        pActor->pSprite = SpriteAllocFromAnimFile(D_8009CD3C, 0x120, 0x1E0, 0x160, 0x100, 0x40);
        SpritePlayAnimation(pActor->pSprite, 0);
        SpriteSetScale(pActor->pSprite, 0x1800);
        pActor->pSprite->flags &= ~4;
    } else {
        result = 3;
    }
    return result;
}
// 93.37% (248 bytes, retail 248). Round 186: re-parked with the current field names
// (pSprite, x, y, z, state, moveSpeed, trailDelay, angle) - it now builds, and
// re-measured live it is still 93.37, so the number is real (the old park text used
// unk4C/unk28/... and did not compile). Byte-identical apart from the same three
// prologue insns as func_8008B54C: retail shifts the index into v1 between the 0x120
// and 0x1E0 argument setup and keeps 0x40 in v0 until the sw in front of the jal; gcc
// stores 0x40 right after li 0x40 and shifts into v0 afterwards. The tail is
// byte-identical; -O2 sched2 post-reload makes this a whole-function context effect.
/*
int func_8008BDD0(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    pActor->pSprite = SpriteAllocFromAnimFile(D_8009CD3C, 0x120, 0x1E0, 0x160, 0x100, 0x40);
    SpritePlayAnimation(pActor->pSprite, 0);
    SpriteSetScale(pActor->pSprite, 0x1800);
    pActor->pSprite->flags &= ~4;
    pActor->x = g_GameState.worldMapPosX << 12;
    pActor->z = g_GameState.worldMapPosZ << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    {
        u16 unk48 = g_GameState.worldMapPlayerRotation;
        pActor->unk24 = 1;
        pActor->moveSpeed = 8;
        pActor->state = 2;
        pActor->dirZ = 0;
        pActor->dirY = 0;
        pActor->dirX = 0;
        pActor->trailDelay = 0x1E;
        pActor->angle = unk48;
    }
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BDD0);

int WorldMapMoveActorTowardTarget(WorldMapActor *pActor) {
    s32 dx;
    s32 dz;
    s32 result = 0;

    dx = pActor->targetX - (pActor->x >> 12);
    if ((dx < 0 ? -dx : dx) >= 5) {
        pActor->x += pActor->dirX * (pActor->moveSpeed / 2);
    } else {
        result = 1;
    }
    dz = pActor->targetZ - (pActor->z >> 12);
    if ((dz < 0 ? -dz : dz) >= 5) {
        pActor->z += pActor->dirZ * (pActor->moveSpeed / 2);
    } else {
        result |= 2;
    }
    WorldMapWrapPosition(&pActor->x);
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    return result;
}
void WorldMapQueueZoneEntry(s16 arg0, s32 *pArray, s16 arg2, s16 arg3) {
    WorldMapEntry *pEntry = &D_8009BE6C[D_8009BD04];

    pEntry->id = arg0;
    pEntry->x = pArray[0];
    pEntry->y = pArray[1];
    pEntry->z = pArray[2];
    pEntry->height = arg3;
    pEntry->radius = arg2;
    D_8009BD04 = (D_8009BD04 + 1) & 0x1F;
}
void WorldMapFindZoneAtPosition(WorldMapPos *pPos, s32 arg1, s32 arg2, u8 *pOut, u8 *pOut2) {
    WorldMapEntry *pEntry = D_8009BE6C;
    s32 i;
    WorldMapPos diff;
    s32 tmp[4]; /* unread; its stack slot is what gives retail's 96-byte frame */
    s32 pos;
    s32 base;
    s32 dist;

    *pOut = 0;
    *pOut2 = 0;
    if (D_8009BD04 > 0) {
        for (i = 0; i < D_8009BD04; i++, pEntry++) {
            pos = pPos->y;
            if (pos < pEntry->y) {
                base = pos - arg2 * 0x1000;
                pos = pEntry->y;
            } else if (pEntry->radius || pEntry->id) {
                base = pEntry->y - pEntry->height * 0x1000;
            } else {
                /* same value as the arm above; retail keeps the branch, so must we */
                base = pEntry->y - pEntry->height * 0x1000;
            }
            if (((pos - base) >> 12) < arg2 + pEntry->height) {
                diff.x = (pEntry->x - pPos->x) >> 12;
                diff.z = (pEntry->z - pPos->z) >> 12;
                WorldMapWrapDeltaScaled(&diff);
                dist = SquareRoot0(diff.x * diff.x + diff.z * diff.z);
                if (dist < pEntry->radius + arg1) {
                    *pOut = 2;
                } else if (dist < pEntry->radius + arg1 + 0x10) {
                    *pOut = 1;
                } else {
                    continue;
                }
                *pOut2 = pEntry->id;
                break;
            }
        }
    }
}
void WorldMapUpdateSpriteGroupFromActor(s32 arg0, WorldMapActor *pActor, WorldMapScreenPos *pPos) {
    if (WorldMapGetTileHalfIndex(&pActor->x) == 3) {
        pPos->x = pActor->x >> 12;
        pPos->y = pActor->y >> 12;
        pPos->z = pActor->z >> 12;
        pPos->rotZ = 0;
        pPos->rotX = 0;
        pPos->rotY = pActor->drawAngle;
        WorldMapInitSpriteGroupSprites(arg0, &pPos->x, &pPos->rotX);
    } else {
        WorldMapDeactivateSpriteGroup(arg0);
    }
}
void WorldMapInitActorSpriteBySlot(WorldMapActor *pActor, int index) {
    WorldMapSprite *pSprite;

    pSprite = SpriteAllocFromAnimFile(g_WorldMapActorAnimFiles[index], D_8009B18C[index], D_8009B194[index], D_8009B19C[index],
                            D_8009B1A4[index], 0x40);
    pActor->pSprite = pSprite;
    if (g_GameState.gearRide[index] == 1) {
        SpritePlayAnimation(pSprite, 0);
    } else {
        SpritePlayAnimation(pSprite, 3);
    }
    SpriteSetScale(pActor->pSprite, 0x2000);
    pActor->pSprite->flags &= ~4;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C364);
// 78.83% (428 bytes, retail 444). Round 186: re-parked with the current field names
// (pSprite, x, y, z, dirX/Y/Z, angle, moveSpeed, trailDelay, drawAngle, state, and
// WorldMapTrail.angle for offset 0x10) - it builds now and re-measures at 78.83.
// Everything from the WorldMapGetTilePlaneHeight jal onward is byte-identical: the
// 16-byte copies must be WorldMapVec struct assignments (which copy x,y,z AND unk34,
// exactly retail's 4 loads/4 stores), and four separate field stores interleave
// load/store instead. Two differences remain, both established:
// (1) retail keeps the three zero stores (0x40/0x3C/0x38) above the g_WorldMapMode
//     load and re-reads angle with lh for drawAngle; gcc sinks the zero stores below
//     the mode load and folds the re-read into an sll/sra pair. C store order does not
//     move it (sched2 is post-reload).
// (2) block layout: retail reaches the mode<8 arm out of line (j + nop in front of
//     "sh state / li 1 / sh unk24" at .L8008C690), gcc inlines it and puts the second
//     store in the j delay slot, so this body is one j and three nops short.
// The mode chain must be written inverted - "if (mode >= 4) { if (mode < 8) B } else
// { gearRide }" - because that is what puts the gearRide block out of line behind
// "bnez v0, A" as retail has it; the natural "if (mode < 4) A else if (mode < 8) B"
// gives the mirror image (79.81 with the two blocks swapped, 4 bytes different).
/*
s32 func_8008C530(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 0);
    WorldMapTrail *pTrail;
    s32 mode;
    s32 i;

    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->angle = *(u16 *)&g_GameState.unk1826[0];
    pActor->moveSpeed = 0xC;
    pActor->drawAngle = pActor->angle;
    mode = g_WorldMapMode;
    if (mode > 0) {
        if (mode >= 4) {
            if (mode < 8) {
                pActor->state = 2;
                pActor->unk24 = 1;
            }
        } else {
            if (g_GameState.gearRide[0] == 1) {
                pActor->state = 1;
                pActor->x = g_WorldMapPlayerPosition;
                pActor->z = g_WorldMapPlayerPositionZ;
                pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
                pActor->angle = g_WorldMapPlayerRotation;
                g_WorldMapScrollTargetX = *(WorldMapVec *)&pActor->x;
                g_WorldMapPlayerAngle = pActor->angle;
                g_WorldMapTrailCursor = 0;
                pTrail = g_WorldMapTrail;
                for (i = 0; i < 0x20; i++, pTrail++) {
                    pTrail->vec = *(WorldMapVec *)&pActor->x;
                    pTrail->angle = pActor->angle;
                }
            }
        }
    }
    g_GameState.fieldScriptMemory[0x16] = pActor->x >> 12;
    g_GameState.fieldScriptMemory[0x17] = pActor->z >> 12;
    *(u16 *)&g_GameState.unk1826[0] = pActor->angle;
    return result;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C530);

int WorldMapModeInitPartyActor0(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 0);
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

int WorldMapInitPartyActorSprite0(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapActorAnimFiles[0], 0x100, 0x1FD, 0x140, 0x140, 0x40);
    SpritePlayAnimation(pActor->pSprite, 0);
    SpriteSetScale(pActor->pSprite, 0x2000);
    pActor->pSprite->flags &= ~4;
    pActor->x = g_GameState.fieldScriptMemory[0x16] << 12;
    pActor->z = g_GameState.fieldScriptMemory[0x17] << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    pActor->unk24 = 1;
    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->moveSpeed = 0xC;
    pActor->angle = *(u16 *)&g_GameState.unk1826[0];
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C844);
// 81.33% (304 bytes, retail 300). Round 186: re-parked with the current field names
// (pSprite, x, y, z, dirX/Y/Z, angle, moveSpeed, trailDelay, drawAngle, state) - it
// builds now and re-measures at 81.33. Two differences, both in the head:
// (1) retail stores the three zeros (0x40/0x3C/0x38) immediately after the
//     func_8008C364 call and loads g_GameState+0x1828 above them; gcc sinks them below
//     the g_WorldMapMode load whatever the C order is.
// (2) retail RE-READS angle with lh for drawAngle (sh 0x48 then lh 0x48); gcc keeps the
//     value in a register and emits an sll/sra pair, which is exactly the 4 extra bytes
//     (304 vs 300). Storing angle through a volatile u16 pointer reproduces retail's lh
//     reload and the exact 300-byte size but scores 80.07 - that variant, not this text,
//     is the base to continue from.
// Everything from the mode tests onward - the gearRide[1] block, the mode<8 arm and the
// tail - is byte-identical.
/*
int func_8008D3F0(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 1);
    s32 mode;

    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->moveSpeed = 0xC;
    pActor->trailDelay = 0xF;
    pActor->angle = *(u16 *)&g_GameState.unk1826[2];
    pActor->drawAngle = pActor->angle;
    mode = g_WorldMapMode;
    if (mode > 0) {
        if (mode < 4) {
            if (g_GameState.gearRide[1] == 1) {
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
    g_GameState.fieldScriptMemory[0x19] = pActor->x >> 12;
    g_GameState.fieldScriptMemory[0x1A] = pActor->z >> 12;
    *(u16 *)&g_GameState.unk1826[2] = pActor->angle;
    return result;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D3F0);



int WorldMapModeInitPartyActor1(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 1);
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

int WorldMapInitPartyActorSprite1(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    pActor->pSprite = SpriteAllocFromAnimFile(D_8009BDFC[0], 0x100, 0x1FC, 0x160, 0x140, 0x40);
    SpritePlayAnimation(pActor->pSprite, 0);
    SpriteSetScale(pActor->pSprite, 0x2000);
    pActor->pSprite->flags &= ~4;
    pActor->x = g_GameState.fieldScriptMemory[0x19] << 12;
    pActor->z = g_GameState.fieldScriptMemory[0x1A] << 12;
    pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
    pActor->unk24 = 1;
    pActor->dirZ = 0;
    pActor->dirY = 0;
    pActor->dirX = 0;
    pActor->moveSpeed = 0xC;
    pActor->angle = *(u16 *)&g_GameState.unk1826[2];
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D678);
// 81.33% (304 bytes, retail 300). Round 186: re-parked with the current field names
// (pSprite, x, y, z, dirX/Y/Z, angle, moveSpeed, trailDelay, drawAngle, state) - it
// builds now and re-measures at 81.33. The same two head differences as func_8008D3F0:
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008DD6C);

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
