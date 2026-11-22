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
extern void *g_WorldMapCharacter2AnimFiles;
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

/* func_8008A72C, WorldMapModeInitActorSlot, WorldMapModeInitActorSlot2, func_8008C364, func_8008C844 and
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
/* 93.37% (248 bytes, retail 248 - same instruction multiset, 6 differing lines).
   Round 389: the body below is the honest tail source order (unk24, moveSpeed, state,
   dirZ/dirY/dirX, trailDelay, angle) and it reproduces retail's whole tail insn for insn.
   The ONLY residual is the entry block's sched2 order, two moves:
   retail  "li a1,0x110 / sll v1,a0,7 / li v0,0x100 / sw v0,0x10(sp) / li v0,0x40 /
            ... / sw ra,0x1C(sp) / sw v0,0x14(sp) / jal / addu s0,s0,v1"
           keeps 0x40 in v0 to the last store and shifts into v1,
   gcc     "li a1,0x110 / li v0,0x100 / sw v0,0x10(sp) / li v0,0x40 / sw v0,0x14(sp) /
            sll v0,a0,7 / ... / jal / addu s0,s0,v0".
   The whole body is ONE basic block (no branch before the last call), so sched2's
   order depends on the far tail, not on the call - WorldMapInitPartyActorSprite0/1/2
   (matched, same 6-argument call, same retail prologue) get it right with a 4-statement
   tail, so the trigger is this function's tail and not the call.
   Round 389 measured and no better: s32 angle local (93.37, identical), direct
   g_GameState.worldMapPlayerRotation in the last store with no local (83.05), all uses
   spelled g_WorldMapSlots[index].field with no pActor local (61.27, 300 bytes).
   Round 167 permuter: PERMUTER_BETTER 410 -> 315 but the winning edit casts the 0x100
   argument to unsigned char (truncates it to 0) - wrong code, not ported.
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
int WorldMapModeInitActorSlot2(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = 1;
    s32 mode;

    if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
        pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapCharacter2AnimFiles, 0x120, 0x1E0, 0x160, 0x100, 0x40);
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
    pActor->trailDelay = 0x1E;
    pActor->drawAngle = pActor->angle;
    switch (mode) {
    case 1:
    case 2:
    case 3:
        if (g_GameState.gearRide[2] == 0) {
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
}


int WorldMapModeInitCharacterSprite2(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = 1;

    if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
        pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapCharacter2AnimFiles, 0x120, 0x1E0, 0x160, 0x100, 0x40);
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

    pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapCharacter2AnimFiles, 0x120, 0x1E0, 0x160, 0x100, 0x40);
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
// 95.87% (460 bytes = retail's 115 insns; instruction multiset IDENTICAL except one branch).
//    Round 399. Reads the per-party-member init kind from three sources as a 3-bit
//    switch value (bit0 partyMembers[index] != CHARACTER_ID_NONE, bit1 the per-actor
//    script angle fieldScriptMemory[0x15+index*3] & 0x3FFF >= 0x400, bit2
//    gearRide[index] == 1). The switch needs the even case values listed explicitly
//    (case 0/2/4/6 share the default body, density 8/8) - with only 1/3/5/7 + default
//    cc1 2.7.2 emits a comparison chain and objdiff reads 60.90.
//    THE ONLY RESIDUALS: (1) retail branches to the shared 'result = 3' block with
//    'beq v1,v0,.L8008C400' and falls through to the jal; cc1 emits the mirror
//    'bne v1,v0' and puts that same 2-insn block (j + li s2,3 delay) between case 1's
//    test and its jal. So retail's merged block sits BEFORE case 1's test and ours
//    sits inside it: block placement, not a spelling (measured: even-cases block
//    first in the source 91.52 at 456, tail written once after the switch 82.74, tail
//    duplicated in both arms 91.52/456 - see lever d-crossjump-tail-merge-stops).
//    (2) the switch table operand: retail names jtbl_800707AC, ours .rodata+0x178
//    (the unit owns its rodata; expected in diff_func.sh, resolved by the linker).
//    The two 'volatile' stores in case 5 are load-order pins measured to be needed:
//    without them sched2 raises both global loads above 'unk24 = 0' and stores x
//    after the Z load (93.26 / 456 bytes); with them every remaining insn matches.
/*
s32 func_8008C364(WorldMapActor *pActor, s32 index) {
    s32 kind = g_GameState.partyMembers[index] != CHARACTER_ID_NONE;
    s32 result = 1;

    if ((g_GameState.fieldScriptMemory[0x15 + index * 3] & 0x3FFF) >= 0x400) {
        kind |= 2;
    }
    if (g_GameState.gearRide[index] == 1) {
        kind |= 4;
    }
    switch (kind) {
    case 0:
    case 2:
    case 4:
    case 6:
    default:
        result = 3;
        pActor->unk24 = 1;
        pActor->y = 0;
        pActor->z = 0;
        pActor->x = 0;
        break;
    case 1:
        if (g_GameState.characters[g_GameState.partyMembers[index]].gearId == GEAR_ID_NONE) {
            result = 3;
        } else {
            WorldMapInitActorSpriteBySlot(pActor, index);
        }
        pActor->unk24 = 1;
        pActor->y = 0;
        pActor->z = 0;
        pActor->x = 0;
        break;
    case 3:
    case 7:
        WorldMapInitActorSpriteBySlot(pActor, index);
        pActor->unk24 = 0;
        pActor->x = g_GameState.fieldScriptMemory[0x16 + index * 3] << 12;
        pActor->z = g_GameState.fieldScriptMemory[0x17 + index * 3] << 12;
        pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
        break;
    case 5:
        WorldMapInitActorSpriteBySlot(pActor, index);
        // volatile: keeps the two global loads below it, as retail has them
        *(volatile s16 *)&pActor->unk24 = 0;
        // volatile: keeps the g_WorldMapPlayerPositionZ load below this store, as retail has it
        *(volatile s32 *)&pActor->x = g_WorldMapPlayerPosition;
        pActor->z = g_WorldMapPlayerPositionZ;
        pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
        g_GameState.fieldScriptMemory[0x15 + index * 3] = 0x400;
        break;
    }
    return result;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C364);

// 89.86% (436 bytes / 109 insns, retail 444 / 111). Round 399, rebuilt from the
// assembly with the natural mode chain - if (mode > 0) { if (mode < 4) { gearRide[0] == 1
// -> the trail setup } else if (mode < 8) { state = 2; unk24 = 1; } } - which scores
// 86.43 before the volatile pins below; the round-186 inverted chain scores 78.83.
// The four volatile pins are the file's own idiom (see WorldMapModeInitActorSlot) and
// each was measured: the three dirX/dirY/dirZ zero stores (sched2 sank them below the
// angle and mode loads) and 'state = 1' (sank below the position loads, 86.43 -> 89.86).
// With those, the head, the angle/drawAngle lh pair, the position block, the
// scroll-target 16-byte copy, the 0x20-iteration trail loop and the tail are ALL
// byte-identical to retail.
// TWO residuals left, both are compiler block choices, not spellings:
// (1) the mode<8 arm: retail keeps 'slti v0,v1,8 / beqz v0,tail / li v0,2' at the
//     dispatch and reaches the out-of-line 'sh state / li 1 / sh unk24' block with
//     'j + nop' (that nop plus the j are 8 of the 8 missing bytes); cc1 folds test and
//     body into one block emitted after the trail loop, so it needs no jump. Measured
//     identical (89.86) with 'else if (mode < 8)' and with a nested 'else { if }'.
// (2) the loop preheader: retail emits 'move a3,zero' (i = 0) before 'sw y / sh angle'
//     and forms &g_WorldMapTrail after them; cc1 swaps exactly those two (same count).
// NEXT for whoever picks this up: try the same body under gcc-2.7.2-cdk (-O2 -G8 scores
// 70.17 on the sibling func_8008C364, so the unit preset is right) or permute; the
// remaining 8 bytes are one jump plus the dispatch test, which no spelling has moved.
/*
s32 func_8008C530(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 0);
    WorldMapTrail *pTrail;
    s32 mode;
    s32 i;

    // volatile: keeps these three above the angle and mode loads, as retail has them
    *(volatile s32 *)&pActor->dirZ = 0;
    *(volatile s32 *)&pActor->dirY = 0;
    *(volatile s32 *)&pActor->dirX = 0;
    pActor->angle = *(u16 *)&g_GameState.unk1826[0];
    pActor->moveSpeed = 0xC;
    pActor->drawAngle = pActor->angle;
    mode = g_WorldMapMode;
    if (mode > 0) {
        if (mode < 4) {
            if (g_GameState.gearRide[0] == 1) {
                // volatile: keeps this above the position loads, as retail has it
                *(volatile s16 *)&pActor->state = 1;
                // volatile: keeps the g_WorldMapPlayerPositionZ load below this store, as retail has it
                *(volatile s32 *)&pActor->x = g_WorldMapPlayerPosition;
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
        } else if (mode < 8) {
            pActor->state = 2;
            pActor->unk24 = 1;
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
