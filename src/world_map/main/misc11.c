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
extern s16 g_WorldMapZoneEntryCount;
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

extern s32 WorldMapInitPartyActorSlot(WorldMapActor *, s32);
extern void WorldMapInitSpriteGroupSprites(s32, void *, void *);
extern void WorldMapGetGameStatePosition(void *);
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
extern s32 ratan2(s32, s32);
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

extern s32 WorldMapUpdateSlotTurnAndAreaEntry(WorldMapActor *);
extern s32 WorldMapUpdateSlotTurnAndZoneEntry(WorldMapActor *);
extern s16 WorldMapGetTileKindValue(s16, s16);
extern s32 func_80095414(s32 *, s32 *, s32 *, s32, s32);
extern s32 WorldMapSelectAreaAtPosition(WorldMapPos *, s32);
extern void WorldMapUpdateRandomTimers(void);
extern s32 g_WorldMapQueuedCharacterCount;
extern u16 g_WorldMapZoneTrailFlags[];
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern u8 g_WorldMapZoneId;
extern u8 g_WorldMapZoneProximity;
extern void WorldMapFindZoneAtPosition(WorldMapPos *, s32, s32, u8 *, u8 *);
extern void WorldMapUpdateSpriteGroupFromActor(s32, WorldMapActor *, WorldMapScreenPos *);
extern int WorldMapMoveActorTowardTarget(WorldMapActor *);

/* WorldMapModeUpdateActorSlot, WorldMapModeInitActorSlot, WorldMapModeInitActorSlot2, WorldMapInitPartyActorSlot and WorldMapUpdatePartyActorTrail
   each jr through a jtbl_ table of this unit; WorldMapUpdateActorSlot did too
   and is now C. The unit owns its .rodata (config/world_map.yaml), and cc1 emits a
   function's table where the function is in the file, so converting a stub keeps
   every table in place - only that function's own instructions and its table
   words change (proved with check_bin.sh for WorldMapUpdateActorSlot, whose table is
   8-aligned in the range). Check the parity first: WorldMapInitPartyActorSlot's table is at
   range+0x294, i.e. 4 mod 8, and converting it pads 4 bytes and shifts the whole
   overlay (measured round 451, see its park note). The unit's two last tables
   (WorldMapUpdateGearActorSlot's) sit at range+0x3BC, i.e. 4 mod 8, so they cannot be
   cc1 tables from this range: that function and the tail after it are
   src/world_map/main/misc11_b.c, whose .rodata range starts at 0xDE4. */
// MATCH: byte-identical (check_bin.sh world_map OK), objdiff 99.97% - the only differing
// row left is the second jump table's operand (retail names jtbl_80070620, this object
// says .rodata+0x108: same address, and 0x108 is where retail's second table sits).
// Two residuals closed here on top of the round-484/497 body (99.48%):
//   (a) the shared s16 scratch is the SAME variable as the case-18/42 loop counter: using
//       the existing "s16 i" for the func_80095414 result / zone index (no "zone" local at
//       all) puts retail's $a2 at every site, 99.54%. A separate s16 local gets $v1 - the
//       allocno's refs/live-length over the whole function decide its hand-out position.
//   (b) case 40 spells the stores targetX, targetZ, unk24, then SpritePlayAnimation, which
//       is what makes cc1 hoist the pSprite load (lw a0,0x4c(s0)) above the targetX store
//       as retail has it; unk24 before targetZ moves it 4 slots late (99.54%).
// Also required, and still standing: "s32 result = 1;" declared FIRST (retail materialises
// it at instruction 4 and pScratch's 0x1F800000 in the beq delay slot); pScratch + 0x24
// written inline at each use (no pOut local) for retail's ori a0,s2,0x90 / lw v0,0x90(s2);
// *(volatile s32 *)&pActor->y = WorldMapGetTilePlaneHeight() in case 40 for the y store's
// load-delay nop; pActor[1].targetSlot / pActor[2].targetSlot in cases 8/9/16/17; the
// trailing "case 0x40: break;" that gives the state table retail's 65 entries; the
// WorldMapVec write-back for func_80095414's output.
int WorldMapModeUpdateActorSlot(int index) {
    s32 result = 1;
    s32 *pScratch = (s32 *)0x1F800000;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapTrail *pTrail;
    s16 i;

    switch (pActor->command) {
    case 3:
        pActor->command = 0;
        pActor->state = 1;
        break;
    case 2:
        pActor->command = 0;
        pActor->state = 0x28;
        break;
    case 6:
        pActor->command = 0;
        pActor->trailDelay++;
        if (g_WorldMapQueuedCharacterCount == pActor->trailDelay) {
            pActor->state = 0;
            g_WorldMapMode = 1;
            g_WorldMapZoneEntryCount = 0;
        }
        break;
    }
    switch (pActor->state) {
    case 0:
    case 1:
        if (((u8 *)&g_GameState.unk22B0)[1] == 0) {
            switch (WorldMapUpdateSlotTurnAndAreaEntry(pActor)) {
            case 1:
                pActor->state = 0x40;
                g_WorldMapLoopActive = 0;
                g_WorldMapAreaMode = 0;
                break;
            case 3:
                if (g_WorldMapZoneId == 7) {
                    pActor->state = 8;
                } else {
                    g_WorldMapZoneId = 4;
                    pActor->state = 0x10;
                }
                break;
            case 2:
            case 4:
            case 5:
                break;
            default:
                if ((pActor->dirX == 0) & (pActor->dirY == 0) & (pActor->dirZ == 0)) {
                    if (pActor->pSprite->animId != 0) {
                        SpritePlayAnimation(pActor->pSprite, 0);
                        WorldMapDeactivateSpriteGroup(0x2F);
                    }
                } else {
                    if (pActor->pSprite->animId != 1) {
                        SpritePlayAnimation(pActor->pSprite, 1);
                    }
                    WorldMapUpdateSpriteGroupFromActor(0x2F, pActor, (WorldMapScreenPos *)pScratch);
                }
                i = func_80095414(&pActor->x, &pActor->dirX, pScratch + 0x24, pActor->moveSpeed << 12,
                                     g_WorldMapMode);
                if (i == 0) {
                    *(WorldMapVec *)&pActor->dirX = *(WorldMapVec *)(pScratch + 0x24);
                    i = func_80095414(&pActor->x, &pActor->dirX, pScratch + 0x24,
                                         pActor->moveSpeed << 12, g_WorldMapMode);
                    if (i == 0) {
                        pActor->dirZ = 0;
                        pActor->dirX = 0;
                    }
                }
                if (i == 1) {
                    WorldMapFindZoneAtPosition((WorldMapPos *)(pScratch + 0x24), 0x10, 0x20,
                                               &g_WorldMapZoneProximity, &g_WorldMapZoneId);
                    if (g_WorldMapZoneId == 7) {
                        i = g_WorldMapZoneProximity + 3;
                    } else {
                        i = g_WorldMapZoneProximity;
                    }
                    if (g_WorldMapZoneTrailFlags[i] != 0) {
                        *(WorldMapVec *)&pActor->x = *(WorldMapVec *)(pScratch + 0x24);
                        if ((pActor->dirX | pActor->dirZ) != 0) {
                            i = (g_WorldMapTrailCursor + 1) & 0x1F;
                            pTrail = &g_WorldMapTrail[i];
                            g_WorldMapTrailCursor = i;
                            pTrail->vec = *(WorldMapVec *)&pActor->x;
                            pTrail->angle = pActor->angle;
                            WorldMapUpdateRandomTimers();
                        }
                    }
                } else {
                    WorldMapFindZoneAtPosition((WorldMapPos *)&pActor->x, 0x10, 0x20, &g_WorldMapZoneProximity,
                                               &g_WorldMapZoneId);
                }
                WorldMapSelectAreaAtPosition(&pActor->x, 0);
                pActor->dirZ = 0;
                pActor->dirY = 0;
                pActor->dirX = 0;
                g_WorldMapScrollTargetX = *(WorldMapVec *)&pActor->x;
                g_WorldMapPlayerAngle = pActor->angle;
                break;
            }
            g_WorldMapZoneEntryCount = 0;
            pActor->unk24 = 0;
        } else {
            pActor->unk24 = 1;
            pActor->x = g_WorldMapSlots[4].x;
            pActor->y = g_WorldMapSlots[4].y;
            pActor->z = g_WorldMapSlots[4].z;
            pActor->angle = g_WorldMapSlots[4].angle;
            WorldMapDeactivateSpriteGroup(0x2F);
        }
        break;
    case 2:
    case 3:
        pActor->x = g_WorldMapSlots[7].x;
        pActor->y = g_WorldMapSlots[7].y;
        pActor->z = g_WorldMapSlots[7].z;
        pActor->angle = g_WorldMapSlots[7].angle;
        break;
    case 8:
        if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
            if (((u8 *)&g_GameState.unk22B0)[2] == 0) {
                WorldMapStartSlot(2, 1);
                pActor[1].targetSlot = g_WorldMapZoneId;
                WorldMapStartSlot(5, 8);
            } else {
                WorldMapStartSlot(5, 1);
                g_WorldMapSlots[5].targetSlot = g_WorldMapZoneId;
            }
        }
        pActor->state++;
        break;
    case 9:
        if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
            if (((u8 *)&g_GameState.unk22B0)[3] == 0) {
                WorldMapStartSlot(3, 1);
                pActor[2].targetSlot = g_WorldMapZoneId;
                WorldMapStartSlot(6, 8);
            } else {
                WorldMapStartSlot(6, 1);
                g_WorldMapSlots[6].targetSlot = g_WorldMapZoneId;
            }
        }
        pActor->state++;
        break;
    case 10:
        if (g_GameState.partyMembers[0] != CHARACTER_ID_NONE) {
            if (WorldMapStartSlot(4, 8) != 0) {
                pActor->state = 0xD;
            }
        } else {
            pActor->state = 0xD;
        }
        break;
    case 13:
        WorldMapBuildDirectionFromPositions(&pActor->x, &g_WorldMapSlots[g_WorldMapZoneId].x, &pActor->dirX,
                                           &pActor->angle);
        pActor->targetX = g_WorldMapSlots[g_WorldMapZoneId].x >> 12;
        pActor->targetZ = g_WorldMapSlots[g_WorldMapZoneId].z >> 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        // falls through
    case 14:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->state++;
        }
        g_WorldMapScrollTargetX = *(WorldMapVec *)&pActor->x;
        g_WorldMapPlayerAngle = pActor->angle;
        WorldMapUpdateSpriteGroupFromActor(index + 0x2E, pActor, (WorldMapScreenPos *)pScratch);
        break;
    case 15:
        if (WorldMapStartSlot(g_WorldMapZoneId, 4) != 0) {
            pActor->unk24 = 1;
            pActor->state = 2;
            WorldMapDeactivateSpriteGroup(0x2F);
        }
        break;
    case 16:
        if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
            if (WorldMapStartSlot(2, 1) != 0) {
                pActor[1].targetSlot = 5;
                pActor->state++;
            }
        } else {
            pActor->state++;
        }
        break;
    case 17:
        if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
            if (WorldMapStartSlot(3, 1) != 0) {
                pActor[2].targetSlot = 6;
                pActor->state++;
            }
        } else {
            pActor->state++;
        }
        break;
    case 18:
        pTrail = g_WorldMapTrail;
        g_WorldMapTrailCursor = 0;
        pScratch[0xC] = g_GameState.fieldScriptMemory[0x16] << 12;
        pScratch[0xE] = g_GameState.fieldScriptMemory[0x17] << 12;
        pScratch[0xD] = WorldMapGetTilePlaneHeight(pScratch[0xC], pScratch[0xE]);
        *(u16 *)&pScratch[0x28] = *(u16 *)&g_GameState.partyActorAngles[0];
        for (i = 0x1F; i != -1; i--) {
            pTrail->vec = *(WorldMapVec *)&pScratch[0xC];
            pTrail->angle = *(u16 *)&pScratch[0x28];
            pTrail++;
        }
        pActor->state = 0xD;
        break;
    case 40:
        pActor->x = g_GameState.fieldScriptMemory[0x16] << 12;
        pActor->z = g_GameState.fieldScriptMemory[0x17] << 12;
        // volatile: keeps the y store above the drawAngle load, as retail has it
        *(volatile s32 *)&pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
        pActor->drawAngle = *(u16 *)&g_GameState.partyActorAngles[0];
        pActor->angle = *(u16 *)&g_GameState.partyActorAngles[0];
        pScratch[0] = pActor->x + rcos((s16)pActor->angle) * 0x30;
        pScratch[2] = pActor->z + (-rsin(pActor->angle)) * 0x30;
        WorldMapBuildDirectionFromPositions(&pActor->x, (WorldMapPos *)pScratch, &pActor->dirX, &pActor->angle);
        pActor->targetX = pScratch[0] >> 12;
        pActor->targetZ = pScratch[2] >> 12;
        pActor->unk24 = 0;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        break;
    case 41:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            SpritePlayAnimation(pActor->pSprite, 0);
            pActor->dirZ = 0;
            pActor->dirY = 0;
            pActor->dirX = 0;
            pActor->state++;
        }
        g_WorldMapScrollTargetX = *(WorldMapVec *)&pActor->x;
        g_WorldMapPlayerAngle = pActor->angle;
        break;
    case 42:
        pTrail = g_WorldMapTrail;
        ((u8 *)&g_GameState.unk22B0)[1] = 0;
        pActor->trailDelay = 1;
        g_WorldMapTrailCursor = 0;
        *(WorldMapVec *)pScratch = *(WorldMapVec *)&pActor->x;
        *(u16 *)&pScratch[0x28] = pActor->angle;
        for (i = 0x1F; i != -1; i--) {
            pTrail->vec = *(WorldMapVec *)pScratch;
            pTrail->angle = *(u16 *)&pScratch[0x28];
            pTrail++;
        }
        pActor->state++;
        // falls through
    case 43:
        switch (g_WorldMapQueuedCharacterCount) {
        case 1:
            pActor->state = 1;
            break;
        case 2:
            if (((u8 *)&g_GameState.unk22B0)[2] == 0) {
                pActor->state++;
            }
            break;
        case 3:
            if ((((u8 *)&g_GameState.unk22B0)[2] | ((u8 *)&g_GameState.unk22B0)[3]) == 0) {
                pActor->state++;
            }
            break;
        }
        break;
    case 44:
        if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
            if (WorldMapStartSlot(2, 5) != 0) {
                pActor->state++;
            }
        } else {
            pActor->state++;
        }
        break;
    case 45:
        if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
            if (WorldMapStartSlot(3, 5) != 0) {
                pActor->state = 0x40;
            }
        } else {
            pActor->state = 0x40;
        }
        break;
    case 0x40:
        break;
    }
    g_GameState.worldMapPosX = pActor->x >> 12;
    g_GameState.worldMapPosZ = pActor->z >> 12;
    g_GameState.worldMapPlayerRotation = pActor->angle;
    if (pActor->unk24 == 0) {
        WorldMapAddSlotPosition(0, &pActor->x);
    }
    return result;
}



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
/* 93.37% (248 bytes = retail's 248, 62 insns = retail's 62; identical instruction multiset,
   tail byte-exact). The residual is three entry-block instructions:
   retail  li a1,0x110 / sll v1,a0,7 / li v0,0x100 / sw v0,0x10(sp) / li v0,0x40 / ... /
           sw ra,0x1C(sp) / sw v0,0x14(sp) / jal / addu s0,s0,v1
   gcc     li a1,0x110 / li v0,0x100 / sw v0,0x10(sp) / li v0,0x40 / sw v0,0x14(sp) /
           sll v0,a0,7 / ... / jal / addu s0,s0,v0
   i.e. retail's index shift lives in $v1 and its last stack-arg store sinks below sw ra,
   while ours reuses $v0 after that store. Measured dead: the tail in the twin's source
   order (93.37 - sched2 puts retail's order back), an s32 angle local (93.37), no angle
   local at all (83.05), every use spelled g_WorldMapSlots[index].field with no pActor
   (61.27, 300 bytes), try_flags --sweep (2.6.0 93.03, cdk 83.19, -O1 49.74), permuter
   round 167 (base 410 best 315, its only find truncates the 0x100 argument to u8 = wrong
   code) and round 402 (base 410 best 395, its only find swaps two calls = wrong code).
   Round 417: `s32 offset = index * 0x80;` with the base built from it compiles to the same
   body (93.37) and `pActor = g_WorldMapSlots; pActor += index;` reads 85.84 (it also moves
   the anim-file load after the slots load), so the shift's allocno cannot be forced to
   conflict with the 0x100/0x40 constants from source. See lever
   h-stack-arg-store-sink-is-a-war-tie: the matched twin WorldMapInitPartyActorSprite2 has the
   same retail prologue and a 4-statement tail and does get sll v1, so a longer tail loses a
   pre-reload scheduling contest, not a spelling.
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
// Round 402 measured the same wall from the other side: the residual is a WAR dependency - gcc
// puts the index shift in v0 and that pins sw v0,0x14(sp) before it, where retail's shift is in
// v1 and its store sits last. Re-measured live with a top-declared u16 angle (not the nested
// block below): 93.37%, 62 insns, retail's 248 bytes. Park it.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BDD0);
/* 93.37% (248 bytes = retail's 248, 62 insns = retail's 62, tail byte-exact). Same three
   prologue instructions as func_8008B54C: retail keeps the last stack argument (0x40) in $v0
   until the store in front of the jal and shifts the index into $v1 before the 0x120 setup,
   ours stores 0x40 immediately and reuses $v0 for the shift. Round 186 re-parked it compiling
   (the older text used unk4C/unk28 and did not build) at the same 93.37; round 402 re-measured
   it live with a top-declared u16 angle and got 93.37. Round 417: the twin's body (retype of
   WorldMapInitPartyActorSprite2, only the anim file / 0x120 / 0x160 / 0x1E constant changes)
   measures 93.37 here too. See func_8008B54C's park for every dead end and lever
   h-stack-arg-store-sink-is-a-war-tie - it is the same pre-reload scheduling contest.
int func_8008BDD0(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    u16 angle;

    pActor->pSprite = SpriteAllocFromAnimFile(g_WorldMapCharacter2AnimFiles, 0x120, 0x1E0, 0x160, 0x100, 0x40);
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
    pActor->trailDelay = 0x1E;
    pActor->angle = angle;
    return 1;
}
*/

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
    WorldMapEntry *pEntry = &D_8009BE6C[g_WorldMapZoneEntryCount];

    pEntry->id = arg0;
    pEntry->x = pArray[0];
    pEntry->y = pArray[1];
    pEntry->z = pArray[2];
    pEntry->height = arg3;
    pEntry->radius = arg2;
    g_WorldMapZoneEntryCount = (g_WorldMapZoneEntryCount + 1) & 0x1F;
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
    if (g_WorldMapZoneEntryCount > 0) {
        for (i = 0; i < g_WorldMapZoneEntryCount; i++, pEntry++) {
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
