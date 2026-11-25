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
extern s32 g_WorldMapQueuedCharacterCount;
extern u16 D_8009B180[];
extern s32 g_WorldMapLoopActive;
extern s32 g_WorldMapAreaMode;
extern u8 g_WorldMapZoneId;
extern u8 g_WorldMapZoneProximity;
extern void WorldMapFindZoneAtPosition(WorldMapPos *, s32, s32, u8 *, u8 *);
extern void WorldMapUpdateSpriteGroupFromActor(s32, WorldMapActor *, WorldMapScreenPos *);
extern int WorldMapMoveActorTowardTarget(WorldMapActor *);


// Tail of world_map/main/misc11, split at file 0x1C874 (config/world_map.yaml) so that
// this run owns the .rodata range [0xCBC, 0xDE0) and its jump tables keep retail's
// offsets.
//
// The two gotos pin retail's block order [dispatch][result = 3][case 1][shared 4-store
// tail]; writing 'result = 3' inside case 1 instead lets cc1 cross-jump the tail and
// keep the survivor there (95.87%). Case 7 shares case 5's body: retail's table sends 7
// to the case-5 block, not to case 3's.
// The two volatile stores in case 7 are load-order pins - without them cc1's scheduler
// hoists both global loads above 'unk24 = 0' and stores x after the Z load.
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
}


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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11_c", func_8008C530);


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
// 99.69% (2988 bytes / 747 insns, retail's exact size; 288 differing lines, 26 of the 29 non-context diff rows are pure register names and 2 are the jtbl operand). Round 478 rewrote the body from retail's own shape, +8.5 points from the 91.19 park:
//  (1) THE 40 'MISSING INSTRUCTIONS' WERE PHANTOM: the stub's .size says 3148 but its .s is 2988 bytes (the next stub func_8008D3F0 starts at 8008D3F0); objdiff compares 2988 vs 2988. Never read 787/3148 from the header again.
//  (2) the func_80090C68 return dispatch is a real 'switch (ret)' with case 1, 3, 4 (retail's tree: beq v1,3 / slti v1,4 / beqz / beq v1,s0), not an if-chain. Also list case 1 BEFORE case 3 in source order: that places the state=8 block after the loopActive/areaMode block, which is retail's layout (+0.9 on its own).
//  (3) the frame 0x20 larger than the sibling func_8008A72C's is a DEAD local array: 'u_long unusedFrame[8];' never referenced takes vars=32 at expand time, which lands every save slot on retail's exact offsets and the frame on 0x58 (lever j-an-unused-local-array-is).
//  (4) 's32 result = 1;' and 's32 *pScratch = (s32 *)0x1F800000;' must be initialised BEFORE pActor so their save/lui pairs precede the g_WorldMapSlots load (levers j-local-declared-before-another, j-scratchpad-base-first).
//  (5) in case 8/9 the targetSlot store is pActor[1]/pActor[2] (retail sh v1,0x86(s1)), not g_WorldMapSlots[index+1]; case 16 keeps the table-base form because retail re-forms it there.
//  (6) zone = g_WorldMapZoneProximity; if (ZoneId == 7) zone += 3; loads the global ONCE, as retail's lbu a2 does.
//  (7) the four 0x90(s4) words are one '*(WorldMapVec *)&pActor->x = *(WorldMapVec *)&pScratch[0x24]' and pScratch[0x24] is spelled inline everywhere (no pOut local), which is what makes retail's 'ori a0,s4,0x90' and 0x90(s4) offsets appear.
//  (8) case 48 is misc11_b's matched WorldMapUpdateGearActorSlot case 48 verbatim: s16 angle local from g_WorldMapSlots[7].unk60[6], rcos(angle), and 'z + (-rsin(pActor->angle)) * 0x60' (the negu), plus 'pScratch[0x2C..0x38] 4-word copy' and an s32 loop counter (s16 emits a widening pair).
// RESIDUAL (all register ties, permuter 3 min = 360 -> 360 no find): moved lands in v1 where retail has a2; g_WorldMapZoneProximity in a0 vs retail a2; the trail pointer pTrail in a2 vs retail a3 in BOTH trail fills (retail's case-50 loop has pTrail=a3, counter=a2, mine swapped); and case 18's branch polarity (retail beqz to the exit, mine bnez to the arm). Also measured dead: s16 moved, the embedded assignment 'if ((moved = ...) == 0)', gcc-2.6.0 96.86, cdk does not compile.
/*
int func_8008C844(int index) {
    s32 result = 1;
    s32 *pScratch = (s32 *)0x1F800000;
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    WorldMapTrail *pTrail;
    s16 zone;
    s16 i;
    s32 j;
    s16 angle;
    u_long unusedFrame[8];
    s32 gear;
    s32 moved;
    s32 ret;

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
        if (g_WorldMapQueuedCharacterCount == pActor->trailDelay) {
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
            switch (ret) {
            case 1:
                g_WorldMapLoopActive = 0;
                g_WorldMapAreaMode = 0;
                break;
            case 3:
                pActor->state = 8;
                break;
            case 4:
                if ((WorldMapGetTileKindValue(1, WorldMapGetTileHalfIndex(&pActor->x)) << 16) != 0) {
                    pActor->state = 0x20;
                }
                break;
            default:
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
                moved = func_80095414(&pActor->x, &pActor->dirX, &pScratch[0x24], pActor->moveSpeed << 12,
                                      g_WorldMapMode);
                if (moved == 0) {
                    *(WorldMapVec *)&pActor->dirX = *(WorldMapVec *)&pScratch[0x24];
                    moved = func_80095414(&pActor->x, &pActor->dirX, &pScratch[0x24], pActor->moveSpeed << 12,
                                          g_WorldMapMode);
                    if (moved == 0) {
                        pActor->dirZ = 0;
                        pActor->dirX = 0;
                    }
                }
                if (moved == 1) {
                    WorldMapFindZoneAtPosition((WorldMapPos *)&pScratch[0x24], 0x18, 0x30,
                                               &g_WorldMapZoneProximity, &g_WorldMapZoneId);
                    zone = g_WorldMapZoneProximity;
                    if (g_WorldMapZoneId == 7) {
                        zone += 3;
                    }
                    if (D_8009B180[zone] != 0) {
                        *(WorldMapVec *)&pActor->x = *(WorldMapVec *)&pScratch[0x24];
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
                break;
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
                    pActor[1].targetSlot = g_WorldMapZoneId;
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
                    pActor[2].targetSlot = g_WorldMapZoneId;
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
        switch (g_WorldMapQueuedCharacterCount) {
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
        angle = g_WorldMapSlots[7].unk60[6];
        pActor->angle = angle;
        pScratch[0] = pActor->x + rcos(angle) * 0x60;
        pScratch[2] = pActor->z + (-rsin(pActor->angle)) * 0x60;
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
        *(u16 *)&pScratch[0x28] = pActor->angle;
        for (j = 0x1F; j != -1; j--) {
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11_c", func_8008C844);


// The four volatile pins are this file's idiom (see func_8008C530). Without the three
// zero clears' pins cc1 sinks them below the mode load (92.11%); without the state/x
// pins the gear arm's stores sink into the position loads (85.33%). Both keep retail's
// 304 bytes, so only the order tells.
int func_8008D3F0(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 1);
    s32 mode;

    // volatile: keeps these three above the angle and mode loads, as retail has them
    *(volatile s32 *)&pActor->dirZ = 0;
    *(volatile s32 *)&pActor->dirY = 0;
    *(volatile s32 *)&pActor->dirX = 0;
    pActor->angle = *(u16 *)&g_GameState.unk1826[2];
    pActor->moveSpeed = 0xC;
    pActor->trailDelay = 0xF;
    mode = g_WorldMapMode;
    pActor->drawAngle = pActor->angle;
    if (mode > 0) {
        if (mode < 4) {
            if (g_GameState.gearRide[1] == 1) {
                // volatile: keeps this above the position loads, as retail has it
                *(volatile s16 *)&pActor->state = 1;
                // volatile: keeps the g_WorldMapPlayerPositionZ load below this store, as retail has it
                *(volatile s32 *)&pActor->x = g_WorldMapPlayerPosition;
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
}




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

