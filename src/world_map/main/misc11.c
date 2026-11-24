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

extern s32 func_80090A84(WorldMapActor *);
extern s32 func_80090C68(WorldMapActor *);
extern s16 WorldMapGetTileKindValue(s16, s16);
extern s32 func_80095414(s32 *, s32 *, s32 *, s32, s32);
extern s32 WorldMapSelectAreaAtPosition(WorldMapPos *, s32);
extern void WorldMapUpdateRandomTimers(void);
extern s32 D_8009C170;
extern u16 D_8009B180[];
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern u8 g_WorldMapZoneId;
extern u8 g_WorldMapZoneProximity;
extern void WorldMapFindZoneAtPosition(WorldMapPos *, s32, s32, u8 *, u8 *);
extern void WorldMapUpdateSpriteGroupFromActor(s32, WorldMapActor *, WorldMapScreenPos *);
extern int WorldMapMoveActorTowardTarget(WorldMapActor *);

/* func_8008A72C, WorldMapModeInitActorSlot, WorldMapModeInitActorSlot2, func_8008C364 and func_8008C844
   each jr through a jtbl_ table of this unit; WorldMapUpdateActorSlot did too
   and is now C. The unit owns its .rodata (config/world_map.yaml), and cc1 emits a
   function's table where the function is in the file, so converting a stub keeps
   every table in place - only that function's own instructions and its table
   words change (proved with check_bin.sh for WorldMapUpdateActorSlot, whose table is
   8-aligned in the range). Check the parity first: func_8008C364's table is at
   range+0x294, i.e. 4 mod 8, and converting it pads 4 bytes and shifts the whole
   overlay (measured round 451, see its park note). The unit's two last tables
   (WorldMapUpdateGearActorSlot's) sit at range+0x3BC, i.e. 4 mod 8, so they cannot be
   cc1 tables from this range: that function and the tail after it are
   src/world_map/main/misc11_b.c, whose .rodata range starts at 0xDE4. */
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008A72C);
// 93.51% (752 insns / 3008 bytes against retail's 740 / 2960; 643 differing lines).
// Round 423, first body: full transcription of the command switch, the 65-entry state
// switch (table emitted, sltiu 0x41 matches), the nested func_80090A84 switch, the
// func_80095414 retry block, both 32-iteration trail fills, cases 13/14/15/16/17/18,
// 40/41/42/43/44/45 and the tail - every block is present and in retail's order.
// RESIDUALS, measured, in order of size:
//  (1) CALLEE-SAVED MIRROR: retail keeps pActor in $s0 and the index parameter in $s1,
//      the compile mirrors them (pActor $s1 / index $s0), which alone is most of the
//      differing lines. Declaring pActor before pScratch measures 92.09 (654 lines) vs
//      pScratch first 92.51, so declaration order does not flip that pair.
//  (2) +4 lw +3 lui vs retail: the dirX..unk44 write-back from the func_80095414 out
//      vector is interleaved (mine loads/stores one word at a time) where retail does
//      four lw then four sw, and the trailing beqz has a store in its delay slot where
//      retail has addiu a0,s0,0x28 (the WorldMapSelectAreaAtPosition argument hoisted
//      into the branch slot). Sched2 order, not source order - the same words are
//      loaded and stored.
//  (3) the `zone` index: retail spells it with two lbu (one per arm of the
//      g_WorldMapZoneId == 7 test) plus sll/sra (s16 index), so the if/else form above
//      is retail's (the `zone = proximity; if (...) zone += 3;` form scores 92.51 at
//      2992 bytes). Ours keeps one lbu and drops the sra because gcc knows the u8
//      range, i.e. one instruction and one load apart.
//  (4) the scratchpad uses a raw `s32 *pScratch = (s32 *)0x1F800000` with pScratch[0xC]
//      (case 18 staging), pScratch[0x24] (func_80095414 out) and *(u16 *)&pScratch[0x28]
//      (the staged angle) because retail reaches 0x30/0x90/0xA0 as displacements off one
//      base register - name those if the layout is understood.
// NEXT: the s0/s1 mirror is the one worth a -da/-dl allocno dump (levers
// a-global-allocno-priority-is / a-lreg-dump-gives-the) before any permuter run; the
// body is otherwise instruction-for-instruction retail's. No permuter run yet.
/*
int func_8008A72C(int index) {
    s32 *pScratch = (s32 *)0x1F800000;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapTrail *pTrail;
    s32 *pOut;
    s16 i;
    s16 zone;
    s16 moved;
    s32 result = 1;

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
        if (D_8009C170 == pActor->trailDelay) {
            pActor->state = 0;
            g_WorldMapMode = 1;
            D_8009BD04 = 0;
        }
        break;
    }
    switch (pActor->state) {
    case 0:
    case 1:
        if (((u8 *)&g_GameState.unk22B0)[1] == 0) {
            switch (func_80090A84(pActor)) {
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
                pOut = &pScratch[0x24];
                moved = func_80095414(&pActor->x, &pActor->dirX, pOut, pActor->moveSpeed << 12, g_WorldMapMode);
                if (moved == 0) {
                    *(WorldMapVec *)&pActor->dirX = *(WorldMapVec *)pOut;
                    moved = func_80095414(&pActor->x, &pActor->dirX, pOut, pActor->moveSpeed << 12, g_WorldMapMode);
                    if (moved == 0) {
                        pActor->dirZ = 0;
                        pActor->dirX = 0;
                    }
                }
                if (moved == 1) {
                    WorldMapFindZoneAtPosition((WorldMapPos *)pOut, 0x10, 0x20, &g_WorldMapZoneProximity,
                                               &g_WorldMapZoneId);
                    if (g_WorldMapZoneId == 7) {
                        zone = g_WorldMapZoneProximity + 3;
                    } else {
                        zone = g_WorldMapZoneProximity;
                    }
                    if (D_8009B180[zone] != 0) {
                        pActor->x = pOut[0];
                        pActor->y = pOut[1];
                        pActor->z = pOut[2];
                        pActor->unk34 = pOut[3];
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
            D_8009BD04 = 0;
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
                g_WorldMapSlots[1].targetSlot = g_WorldMapZoneId;
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
                g_WorldMapSlots[2].targetSlot = g_WorldMapZoneId;
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
                g_WorldMapSlots[1].targetSlot = 5;
                pActor->state++;
            }
        } else {
            pActor->state++;
        }
        break;
    case 17:
        if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
            if (WorldMapStartSlot(3, 1) != 0) {
                g_WorldMapSlots[2].targetSlot = 6;
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
        *(u16 *)&pScratch[0x28] = *(u16 *)&g_GameState.unk1826[0];
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
        pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
        pActor->drawAngle = *(u16 *)&g_GameState.unk1826[0];
        pActor->angle = *(u16 *)&g_GameState.unk1826[0];
        pScratch[0] = pActor->x + rcos((s16)pActor->angle) * 0x30;
        pScratch[2] = pActor->z + (-rsin(pActor->angle)) * 0x30;
        WorldMapBuildDirectionFromPositions(&pActor->x, (WorldMapPos *)pScratch, &pActor->dirX, &pActor->angle);
        pActor->targetX = pScratch[0] >> 12;
        pActor->unk24 = 0;
        pActor->targetZ = pScratch[2] >> 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        // falls through
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
        switch (D_8009C170) {
        case 1:
            pActor->state++;
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
    }
    g_GameState.worldMapPosX = pActor->x >> 12;
    g_GameState.worldMapPosZ = pActor->z >> 12;
    g_GameState.worldMapPlayerRotation = pActor->angle;
    if (pActor->unk24 == 0) {
        WorldMapAddSlotPosition(0, &pActor->x);
    }
    return result;
}
*/

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
// 99.87% (460 bytes / 115 insns = retail's exact size; every instruction, the block
//    order and the 8 table words are retail's - the only objdiff delta is
//    'jtbl_800707AC' vs '.rodata', which the linker resolves identically). IT CANNOT BE
//    COMMITTED: the unit's rodata range starts at 0x80070518 (yaml [0xA28, .rodata,
//    main/misc11]) and this table is retail's 0x800707AC, i.e. range+0x294 = 4 mod 8, so
//    cc1's '.align 3' pads 4 bytes before it: the object's .rodata is 0x3C0 against the
//    range's 0x3BC and 'check_bin.sh world_map' comes out 180426 bytes against retail's
//    180422 with 147513 differing bytes over 447 symbols (the whole-overlay shift).
//    Measured this round by converting it and running the gate, then reverting - the
//    park below is the body that produced that measurement. Needs a yaml split whose
//    new .rodata range starts at a parity-4 offset (lever g-parity-4-split-is-the-route
//    / SPLIT_JTBL_PARITY4), which is a worker+orchestrator job: for reference the
//    function's file offset is 0x1C874 and its table's is 0xCBC.
//    THE BODY (round 451): the shape that closed all of the above is the goto pair -
//    'case 0/2/4/6/default: setResultThree: result = 3; goto setZero;' and case 1
//    'if (gearId == GEAR_ID_NONE) goto setResultThree;' with 'setZero:' above the
//    four-store tail. Cross-jumping a duplicated 'result = 3' block puts the survivor
//    at the SECOND copy's position (inside case 1, 95.87%); the two labels pin
//    retail's layout [dispatch][result = 3][case 1][shared 4-store tail][case 3]
//    [case 5]. Case 5 and case 7 share a body (retail's table sends 7 to
//    .L8008C4BC, not to case 3's .L8008C464) - grouping 3/7 instead measures 95.87
//    with the same 460 bytes.
//    The two 'volatile' stores in case 5 are load-order pins measured to be needed:
//    without them sched2 raises both global loads above 'unk24 = 0' and stores x after
//    the Z load (93.26 / 456 bytes); with them every remaining insn matches.
//    Round 399 note, kept for the record: 95.87% (460 bytes = retail's 115 insns;
//    instruction multiset identical except one branch).
//    Reads the per-party-member init kind from three sources as a 3-bit
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
    setResultThree:
        result = 3;
        goto setZero;
    case 1:
        if (g_GameState.characters[g_GameState.partyMembers[index]].gearId == GEAR_ID_NONE) {
            goto setResultThree;
        }
        WorldMapInitActorSpriteBySlot(pActor, index);
    setZero:
        pActor->unk24 = 1;
        pActor->y = 0;
        pActor->z = 0;
        pActor->x = 0;
        break;
    case 3:
        WorldMapInitActorSpriteBySlot(pActor, index);
        pActor->unk24 = 0;
        pActor->x = g_GameState.fieldScriptMemory[0x16 + index * 3] << 12;
        pActor->z = g_GameState.fieldScriptMemory[0x17 + index * 3] << 12;
        pActor->y = WorldMapGetTilePlaneHeight(pActor->x, pActor->z);
        break;
    case 5:
    case 7:
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


// 89.86% (436 bytes / 109 insns, retail 444 / 111, so exactly the 8 bytes of a
// 'j + nop'). Round 451, re-measured from the round-399 body with the mode chain
// 'if (mode > 0) { if (mode < 4) { gearRide[0] == 1 -> the trail setup } else if
// (mode < 8) { state = 2; unk24 = 1; } }'. The four volatile pins are this file's own
// idiom and are load-bearing (round 399 measured 86.43 -> 89.86 for them); with them
// the head, the angle/drawAngle lh pair, the position block, the scroll-target
// 16-byte copy, the 0x20-iteration trail loop and the tail are byte-identical.
// RESIDUAL (1), the 8 missing bytes: retail's dispatch carries BOTH tests -
// 'slti v0,v1,4 / bnez v0,<gear>' then 'slti v0,v1,8 / beqz v0,<tail> / li v0,2' -
// and reaches the out-of-line 'sh state / li 1 / sh unk24' block with 'j + nop'.
// cc1 2.7.2 emits that arm's test next to its body, after the gear block
// ('beq v0,$0,<arm>'), which saves the jump and the second dispatch test. Measured
// this round: the natural else-if and the nested 'else { if }' both 89.86; writing
// the arm first as 'if (mode >= 4 && mode < 8)' 85.08 / 436 (block inlined before the
// tests). So the block POSITION is what is missing, not the body.
// NEXT: the sibling func_8008C364 in this file closed for exactly this reason by
// scooting a shared block to a label ('goto setResultThree' / 'goto setZero'), which
// pins a block at the label's source position instead of letting cc1 move it - try
// that here: keep 'else if (mode < 8) goto <label>' and put the label (holding
// 'state = 2; unk24 = 1;') after the gear block, with the gear arm ending in a goto
// to the tail. That jump already exists in retail as the trail loop's exit.
// RESIDUAL (2): the loop preheader - retail emits 'move a3,zero' (i = 0) before
// 'sw y / sh angle' and forms &g_WorldMapTrail after them, cc1 swaps both pairs
// (same instruction count, so worth 0 bytes).
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
// 91.19% (2988 bytes / 747 insns against retail's 3148 / 787; 479 differing lines).
// Round 435, first real body: the twin is func_8008A72C (parked at 93.51% ten lines up) -
// same 65-entry state machine for a different slot (sprite group 0x2C vs 0x2F, zone test
// 0x18/0x30 vs 0x10/0x20, func_80090C68 vs func_80090A84, SelectArea arg 1 vs 0). Every
// case body, both trail fills and the tail are present and in retail's order.
// RESIDUALS, measured:
//  (1) 40 INSTRUCTIONS MISSING (160 bytes). Not located yet; the differences visible in
//      the head are branch layout, not missing work, so the deficit is inside one of the
//      later arms - find it before touching the dispatch shapes.
//  (2) the func_80090C68 return dispatch: retail tests `beq v1,3` first, then `slti 4`,
//      then `beq v1,$s0` - i.e. it compares the return against the LOCAL that holds
//      g_GameState.gearRide[0], not against a literal 1, and lays the four arms out as a
//      switch tree. This body's if-chain emits `bne v1,3 / li 8 / j / sh` instead.
//      Writing `ret == gear` (done here via the `gear` local) is what puts $s0 in the
//      compare, but the tree order is still the if-chain's.
//  (3) the 0x18 arm's else path stages x/y/z >> 12 as three `sh` at 0xA0/0xA2/0xA4 and
//      passes that address - spelled with ((s16*)pScratch)[0x50..0x52]; retail reaches
//      them off $s4 too, so this is probably right and untested.
//  (4) `case 64: break;` was added because retail's table has 65 entries (sltiu v1,0x41)
//      and entry 64 points at the switch exit; that fixed the bound but is a guess at the
//      source's shape.
// NEXT: locate the 40 missing instructions first (diff the tail arms one at a time), then
// the dispatch tree in (2). No permuter run.
/*
int func_8008C844(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 *pScratch = (s32 *)0x1F800000;
    WorldMapTrail *pTrail;
    s32 *pOut;
    s16 zone;
    s16 i;
    s32 gear;
    s32 moved;
    s32 ret;
    s32 result = 1;

    switch (pActor->command) {
    case 4:
        pActor->state = 0x10;
        pActor->command = 0;
        pActor->trailDelay = 1;
        g_GameState.gearRide[0] = 1;
        break;
    case 3:
        pActor->command = 0;
        pActor->state = 0x30;
        break;
    case 7:
        pActor->command = 0;
        pActor->trailDelay++;
        if (D_8009C170 == pActor->trailDelay) {
            pActor->state = 1;
            g_WorldMapMode = 2;
            D_8009BD04 = 0;
        }
        break;
    case 8:
        pActor->command = 0;
        pActor->state = 0x18;
        break;
    }

    switch (pActor->state) {
    case 0:
    case 1:
        gear = g_GameState.gearRide[0];
        if (gear == 1) {
            ret = func_80090C68(pActor);
            if (ret == 3) {
                pActor->state = 8;
            } else if (ret == gear) {
                g_WorldMapLoopActive = 0;
                g_WorldMapAreaMode = 0;
            } else if (ret == 4) {
                if ((WorldMapGetTileKindValue(1, WorldMapGetTileHalfIndex(&pActor->x)) << 16) != 0) {
                    pActor->state = 0x20;
                }
            } else {
                if ((pActor->dirX == 0) & (pActor->dirY == 0) & (pActor->dirZ == 0)) {
                    if (pActor->pSprite->animId != 0) {
                        SpritePlayAnimation(pActor->pSprite, 0);
                        WorldMapDeactivateSpriteGroup(0x2C);
                    }
                } else {
                    if (pActor->pSprite->animId != 1) {
                        SpritePlayAnimation(pActor->pSprite, 1);
                    }
                    WorldMapUpdateSpriteGroupFromActor(0x2C, pActor, (WorldMapScreenPos *)pScratch);
                }
                pOut = &pScratch[0x24];
                moved = func_80095414(&pActor->x, &pActor->dirX, pOut, pActor->moveSpeed << 12, g_WorldMapMode);
                if (moved == 0) {
                    *(WorldMapVec *)&pActor->dirX = *(WorldMapVec *)pOut;
                    moved = func_80095414(&pActor->x, &pActor->dirX, pOut, pActor->moveSpeed << 12, g_WorldMapMode);
                    if (moved == 0) {
                        pActor->dirZ = 0;
                        pActor->dirX = 0;
                    }
                }
                if (moved == 1) {
                    WorldMapFindZoneAtPosition((WorldMapPos *)pOut, 0x18, 0x30, &g_WorldMapZoneProximity,
                                               &g_WorldMapZoneId);
                    if (g_WorldMapZoneId == 7) {
                        zone = g_WorldMapZoneProximity + 3;
                    } else {
                        zone = g_WorldMapZoneProximity;
                    }
                    if (D_8009B180[zone] != 0) {
                        pActor->x = pOut[0];
                        pActor->y = pOut[1];
                        pActor->z = pOut[2];
                        pActor->unk34 = pOut[3];
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
                    WorldMapFindZoneAtPosition((WorldMapPos *)&pActor->x, 0x18, 0x30, &g_WorldMapZoneProximity,
                                               &g_WorldMapZoneId);
                }
                WorldMapSelectAreaAtPosition((WorldMapPos *)&pActor->x, 1);
                pActor->dirZ = 0;
                pActor->dirY = 0;
                pActor->dirX = 0;
                g_WorldMapScrollTargetX = *(WorldMapVec *)&pActor->x;
                g_WorldMapPlayerAngle = pActor->angle;
            }
            D_8009BD04 = 0;
        } else {
            if (pActor->pSprite->animId != 3) {
                SpritePlayAnimation(pActor->pSprite, 3);
                WorldMapDeactivateSpriteGroup(0x2C);
            }
        }
        break;
    case 2:
        pActor->x = g_WorldMapSlots[7].x;
        pActor->z = g_WorldMapSlots[7].z;
        pActor->angle = g_WorldMapSlots[7].angle;
        break;
    case 8:
        if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
            if (g_GameState.gearRide[1] == 1) {
                if (WorldMapStartSlot(5, 1) != 0) {
                    g_WorldMapSlots[index + 1].targetSlot = g_WorldMapZoneId;
                    pActor->state++;
                }
            } else {
                WorldMapStartSlot(2, 1);
                g_WorldMapSlots[2].targetSlot = g_WorldMapZoneId;
                WorldMapStartSlot(5, 8);
                pActor->state++;
            }
        } else {
            pActor->state++;
        }
        break;
    case 9:
        if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
            if (g_GameState.gearRide[2] == 1) {
                if (WorldMapStartSlot(6, 1) != 0) {
                    g_WorldMapSlots[index + 2].targetSlot = g_WorldMapZoneId;
                    pActor->state++;
                }
            } else {
                WorldMapStartSlot(3, 1);
                g_WorldMapSlots[3].targetSlot = g_WorldMapZoneId;
                WorldMapStartSlot(6, 8);
                pActor->state++;
            }
        } else {
            pActor->state++;
        }
        break;
    case 10:
        WorldMapBuildDirectionFromPositions(&pActor->x, &g_WorldMapSlots[g_WorldMapZoneId].x, &pActor->dirX,
                                            &pActor->angle);
        pActor->targetX = g_WorldMapSlots[g_WorldMapZoneId].x >> 12;
        pActor->targetZ = g_WorldMapSlots[g_WorldMapZoneId].z >> 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        // falls through
    case 11:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->state++;
        }
        g_WorldMapScrollTargetX = *(WorldMapVec *)&pActor->x;
        g_WorldMapPlayerAngle = pActor->angle;
        WorldMapUpdateSpriteGroupFromActor(0x2C, pActor, (WorldMapScreenPos *)pScratch);
        break;
    case 12:
        if (WorldMapStartSlot(g_WorldMapZoneId, 4) != 0) {
            pActor->unk24 = 1;
            pActor->state = 2;
            WorldMapDeactivateSpriteGroup(0x2C);
        }
        break;
    case 16:
        switch (D_8009C170) {
        case 1:
            pActor->state = 1;
            break;
        case 2:
            if (g_GameState.gearRide[1] != 0) {
                pActor->state++;
            }
            break;
        case 3:
            if (g_GameState.gearRide[1] & g_GameState.gearRide[2]) {
                pActor->state++;
            }
            break;
        }
        break;
    case 17:
        if (g_GameState.partyMembers[1] == CHARACTER_ID_NONE) {
            pActor->state++;
        } else if (WorldMapStartSlot(5, 5) != 0) {
            pActor->state++;
        }
        break;
    case 18:
        if ((g_GameState.partyMembers[2] == CHARACTER_ID_NONE) || (WorldMapStartSlot(6, 5) == 0)) {
            pActor->state = 0x40;
        }
        break;
    case 24:
        if (((u8 *)&g_GameState.characterFlags)[index] == 7) {
            pActor->unk22 = 1;
            pActor->state = 0x1A;
        } else {
            ((s16 *)pScratch)[0x50] = pActor->x >> 12;
            ((s16 *)pScratch)[0x51] = pActor->y >> 12;
            ((s16 *)pScratch)[0x52] = pActor->z >> 12;
            WorldMapInitSpriteGroupSprites(5, &((s16 *)pScratch)[0x50], NULL);
            pActor->unk22 = 8;
            pActor->state++;
        }
        break;
    case 25:
        pActor->unk22--;
        if ((s16)pActor->unk22 <= 0) {
            pActor->unk24 = 1;
            pActor->unk22 = 0x10;
            pActor->state++;
        }
        break;
    case 26:
        pActor->unk22--;
        if ((s16)pActor->unk22 <= 0) {
            SpritePlayAnimation(pActor->pSprite, 3);
            pActor->unk24 = 1;
            pActor->state = 2;
        }
        break;
    case 32:
        SpritePlayAnimation(pActor->pSprite, 0);
        if (g_GameState.partyMembers[1] != CHARACTER_ID_NONE) {
            WorldMapStartSlot(5, 2);
        }
        if (g_GameState.partyMembers[2] != CHARACTER_ID_NONE) {
            WorldMapStartSlot(6, 2);
        }
        pActor->dirZ = 0;
        pActor->dirY = 0;
        pActor->dirX = 0;
        pActor->state++;
        // falls through
    case 33:
        if (WorldMapStartSlot(1, 2) != 0) {
            g_WorldMapSlots[1].targetSlot = index;
            pActor->state++;
        }
        break;
    case 34:
        pActor->state = 0;
        break;
    case 48:
        WorldMapGetGameStatePosition(&pActor->x);
        pActor->unk24 = 0;
        pActor->angle = (s16)g_WorldMapSlots[7].unk60[6];
        pScratch[0] = pActor->x + rcos((s16)g_WorldMapSlots[7].unk60[6]) * 0x60;
        pScratch[2] = pActor->z - rsin(pActor->angle) * 0x60;
        WorldMapBuildDirectionFromPositions(&pActor->x, (WorldMapPos *)pScratch, &pActor->dirX, &pActor->angle);
        pActor->targetX = pScratch[0] >> 12;
        pActor->targetZ = pScratch[2] >> 12;
        SpritePlayAnimation(pActor->pSprite, 1);
        pActor->state++;
        // falls through
    case 49:
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
    case 64:
        break;
    case 50:
        pTrail = g_WorldMapTrail;
        g_WorldMapTrailCursor = 0;
        *(WorldMapVec *)&pScratch[0xC] = *(WorldMapVec *)&pActor->x;
        pScratch[0xF] = pActor->unk34;
        *(u16 *)&pScratch[0x28] = pActor->angle;
        for (i = 0x1F; i != -1; i--) {
            pTrail->vec = *(WorldMapVec *)&pScratch[0xC];
            pTrail->angle = *(u16 *)&pScratch[0x28];
            pTrail++;
        }
        pActor->state = 1;
        g_WorldMapMode = 2;
        break;
    }

    if ((pActor->state != 2) && ((g_WorldMapMode == 2) || (g_GameState.partyMembers[0] != 7))) {
        WorldMapAddSlotPosition(1, &pActor->x);
    }
    g_GameState.fieldScriptMemory[0x16] = pActor->x >> 12;
    g_GameState.fieldScriptMemory[0x17] = pActor->z >> 12;
    *(u16 *)&g_GameState.unk1826[0] = pActor->angle;
    return result;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D3F0);
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
