#include "common.h"
#include "main/game.h"

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ s16 unk04;
    /* 0x06 */ s16 unk06;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ undefined16 unk26;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ s16 unk48;
    /* 0x4A */ s16 unk4A;
    /* 0x4C */ void *unk4C;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ void *unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ undefined32 unk60[8]; // 0x60..0x7F
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x000 */ undefined32 unk000[40]; // 0x000..0x09F
    /* 0x0A0 */ s16 unk0A0;
    /* 0x0A2 */ s16 unk0A2;
    /* 0x0A4 */ s16 unk0A4;
    /* 0x0A6 */ undefined16 unk0A6;
    /* 0x0A8 */ s16 unk0A8;
    /* 0x0AA */ s16 unk0AA;
    /* 0x0AC */ s16 unk0AC;
    /* 0x0AE */ undefined16 unk0AE;
} WorldMapScreenPos; // Size: 0xB0

typedef struct {
    /* 0x00 */ s16 unk00;
    /* 0x02 */ undefined16 unk02;
    /* 0x04 */ s32 unk04;
    /* 0x08 */ s32 unk08;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ undefined16 unk16;
} WorldMapEntry; // Size: 0x18

typedef struct {
    /* 0x0 */ s32 unk00;
    /* 0x4 */ s32 unk04;
    /* 0x8 */ s32 unk08;
    /* 0xC */ s32 unk0C;
} WorldMapVec; // Size: 0x10

typedef struct {
    /* 0x00 */ WorldMapVec vec;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 pad12;
} WorldMapTrail; // Size: 0x14

// 12.4 fixed point world map position; the range test below uses the middle axis too.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 unk4;
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
extern void *D_8009BDF8[];
extern void *D_8009CD38;
extern void *D_8009CD3C;
extern void WorldMapWrapDeltaScaled(WorldMapPos *);
extern s32 SquareRoot0(s32);

extern s32 func_8008C364(WorldMapActor *, s32);
extern void func_80089160(s32, void *, void *);
extern void WorldMapDeactivateSpriteGroup(s32);
extern s16 WorldMapGetTileHalfIndex(void *);
extern s32 WorldMapGetTilePlaneHeight(s32, s32);
extern void *D_8009BDFC[];
extern void *D_8009BE00[];
extern void *SpriteAllocFromAnimFile(void *, s16, s16, s16, s16, s16);
extern void SpritePlayAnimation(void *, s32);
extern void SpriteSetScale(void *, s16);
extern void WorldMapWrapPosition(void *);
extern s32 rcos(s32);
extern s32 rsin(s32);
extern void WorldMapBuildDirectionFromPositions(WorldMapPos *, WorldMapPos *, s32 *, s16 *);
extern void WorldMapAddSlotPosition(s16, s32 *);
extern s32 WorldMapStartSlot(s32, s32);
extern s32 g_WorldMapPlayerPosition;
extern s32 D_8009C5B4;
extern s32 g_WorldMapPlayerRotation;
extern WorldMapVec g_WorldMapScrollTargetX;
extern WorldMapTrail g_WorldMapTrail[];
extern u16 g_WorldMapPlayerAngle;
extern s16 g_WorldMapTrailCursor;

/* func_8008A72C, func_8008B2BC, func_8008BB40, func_8008C364, func_8008C844 and
   func_8008D678 each jr through a jtbl_ table of this unit; WorldMapUpdateActorSlot did too
   and is now C. The unit owns its .rodata (config/world_map.yaml), and cc1 emits a
   function's table where the function is in the file, so converting a stub keeps
   every table in place - only that function's own instructions and its table
   words change (proved with check_bin.sh for WorldMapUpdateActorSlot). */
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008A72C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008B2BC);
/* 97.39% (472 bytes, retail 476 - one instruction short). The head block is now
   byte-identical, including the unk48 sh/lh re-read that every earlier attempt
   folded into an sll/sra: it needs the assignments in RETAIL'S order, not the order
   the old parked body had -
     unk40/unk3C/unk38 = 0; unk48 = g_GameState.unk1824; mode = g_WorldMapMode;
     unk4A = 8; unk58 = 0xF; unk5C = pActor->unk48;
   (mode LAST of the two loads, and unk5C written as a re-read of the FIELD, which
   is what makes gcc emit sh 0x48 / lh 0x48 instead of sh 0x48 / sll+sra). The
   switch table is 14 words (13 cases, table at an 8-aligned unit offset, the pad
   comes from the next dlabel's .align 3, so a C switch keeps the layout).
   The remaining diff is ONE instruction, in the case 1-3 gear arm: retail is
     lw v0,g_WorldMapPlayerPosition; nop; sw v0,0x28(s0); lui a1,D_8009C5B4; lw a1,...
   and gcc emits
     lw v0,g_WorldMapPlayerPosition; lui a1,D_8009C5B4; lw a1,...; sw v0,0x28(s0)
   i.e. it hoists the second global load above the first store, so gas fills the
   load-delay slot with lui a1 instead of emitting retail's nop and the function is
   4 bytes short. Tried: the old parked source order (87.47), gearRide[2] variants,
   a u16 local for the angle, volatile unk48, do/while (0) around the stores (all
   worse or unchanged). NOTE the gear flag is g_GameState+0x22B2 = gearRide[1],
   NOT gearRide[2]: the old body read the wrong byte of gearRide and only reached
   87.47.
int func_8008B2BC(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 mode;
    s32 result = 1;

    if (g_GameState.partyMembers[1] != 0xFF) {
        pActor->unk4C = SpriteAllocFromAnimFile(D_8009CD38, 0x110, 0x1E0, 0x150, 0x100, 0x40);
        SpritePlayAnimation(pActor->unk4C, 0);
        SpriteSetScale(pActor->unk4C, 0x1800);
        *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
        pActor->unk24 = 0;
    } else {
        pActor->unk24 = 1;
        result = 3;
    }
    pActor->unk28 = g_GameState.unk1820 << 12;
    pActor->unk30 = g_GameState.unk1822 << 12;
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk48 = g_GameState.unk1824;
    mode = g_WorldMapMode;
    pActor->unk4A = 8;
    pActor->unk58 = 0xF;
    pActor->unk5C = pActor->unk48;
    switch (mode) {
    case 1:
    case 2:
    case 3:
        if (g_GameState.gearRide[1] == 0) {
            pActor->unk28 = g_WorldMapPlayerPosition;
            pActor->unk30 = D_8009C5B4;
            pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
            pActor->unk48 = g_WorldMapPlayerRotation;
        } else {
            pActor->unk20 = 1;
            pActor->unk24 = 1;
        }
        break;
    case 4:
    case 5:
    case 6:
        pActor->unk20 = 3;
        pActor->unk24 = 1;
        if (g_GameState.partyMembers[1] != 0xFF) {
            g_GameState.gearRide[1] = 1;
        }
        break;
    case 7:
        pActor->unk20 = 2;
        pActor->unk24 = 1;
        if (g_GameState.partyMembers[1] != 0xFF) {
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
*/
int WorldMapModeInitCharacterSprite1(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = 1;

    if (g_GameState.partyMembers[1] != 0xFF) {
        pActor->unk4C = SpriteAllocFromAnimFile(D_8009CD38, 0x110, 0x1E0, 0x150, 0x100, 0x40);
        SpritePlayAnimation(pActor->unk4C, 0);
        SpriteSetScale(pActor->unk4C, 0x1800);
        *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    } else {
        result = 3;
    }
    return result;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008B54C);
/* 93.37% (also tried a void *pSprite temp assigned then used by both calls, as in
   WorldMapInitActorSpriteBySlot: no change, still 93.37 with the same three head insns). (re-diffed under the current XenoWorldMap flags; cdk 83.19%, plain -G0/-O
   variants lower). Everything is byte-identical except three insns in the prologue:
   the original computes index*0x80 in v1 BEFORE the 0x100 push and stores the
   0x40 stack argument just before the jal, while gcc emits the store right after
   li 0x40 and puts the shift in v0 after it. The SAME call shape with the SAME
   neighbours (WorldMapInitPartyActorSprite0, matched) reproduces the original prologue exactly, so
   the trigger is something in this function's tail, not the call: the tail below is
   itself byte-identical. Tried: flat tail without the unk48 temp (83.05), pointer
   built as (u8*)g_WorldMapSlots + index*0x80 (93.37), cdk (83.19).
   Round 56: re-diffed live, exactly the same 93.37 - the whole diff is the position
   of the "sw v0,0x14(sp)" for the 0x40 arg (retail: just before the jal, so v1 holds
   the sll and v0 the constant; ours: right after "li v0,0x40", sll into v0) plus the
   matching addu register. The matched WorldMapInitPartyActorSprite0 has retail's
   exact shape, so it is a scheduling/allocator artifact of this function's tail, not
   the call: no hand shape moved it. Permuter is blocked on this unit (see WorldMapFindZoneAtPosition).
int func_8008B54C(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    pActor->unk4C = SpriteAllocFromAnimFile(D_8009CD38, 0x110, 0x1E0, 0x150, 0x100, 0x40);
    SpritePlayAnimation(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x1800);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.unk1820 << 12;
    pActor->unk30 = g_GameState.unk1822 << 12;
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    {
        u16 unk48 = g_GameState.unk1824;
        pActor->unk24 = 1;
        pActor->unk4A = 8;
        pActor->unk20 = 2;
        pActor->unk40 = 0;
        pActor->unk3C = 0;
        pActor->unk38 = 0;
        pActor->unk58 = 0xF;
        pActor->unk48 = unk48;
    }
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

    switch (pActor->unk04) {
    case 1:
        pTarget = &g_WorldMapSlots[pActor->unk06];
        pActor->unk04 = 0;
        pActor->unk20 = 8;
        break;
    case 2:
        pActor->unk04 = 0;
        pActor->unk20 = 0x28;
        break;
    case 3:
        pActor->unk04 = 0;
        pActor->unk20 = 1;
        break;
    case 5:
        pActor->unk04 = 0;
        pActor->unk20 = 0x30;
        break;
    }
    switch (pActor->unk20) {
    case 0:
    case 1:
        if (((u8 *)&g_GameState.unk22B0)[index] == 0) {
            pTrail = &g_WorldMapTrail[(g_WorldMapTrailCursor - pActor->unk58) & 0x1F];
            if ((pActor->unk28 == pTrail->vec.unk00) & (pActor->unk2C == pTrail->vec.unk04) &
                (pActor->unk30 == pTrail->vec.unk08)) {
                if (((s8 *)pActor->unk4C)[0xAF] != 0) {
                    SpritePlayAnimation(pActor->unk4C, 0);
                    WorldMapDeactivateSpriteGroup(index + 0x2E);
                }
            } else {
                if (((s8 *)pActor->unk4C)[0xAF] != 1) {
                    SpritePlayAnimation(pActor->unk4C, 1);
                }
                WorldMapUpdateSpriteGroupFromActor(index + 0x2E, pActor, (WorldMapScreenPos *)pScratch);
            }
            pActor->unk28 = pTrail->vec.unk00;
            pActor->unk2C = pTrail->vec.unk04;
            pActor->unk30 = pTrail->vec.unk08;
            pActor->unk48 = pTrail->unk10;
            pActor->unk24 = 0;
        } else {
            pTarget = &g_WorldMapSlots[index + 3];
            pActor->unk28 = pTarget->unk28;
            pActor->unk2C = pTarget->unk2C;
            pActor->unk30 = pTarget->unk30;
            pActor->unk48 = pTarget->unk48;
            pActor->unk24 = 1;
            WorldMapDeactivateSpriteGroup(index + 0x2E);
        }
        break;
    case 2:
        pActor->unk28 = g_WorldMapSlots[7].unk28;
        pActor->unk2C = g_WorldMapSlots[7].unk2C;
        pActor->unk30 = g_WorldMapSlots[7].unk30;
        pActor->unk48 = g_WorldMapSlots[7].unk48;
        break;
    case 8:
        WorldMapBuildDirectionFromPositions(&pActor->unk28, &pTarget->unk28, &pActor->unk38, &pActor->unk48);
        pActor->unk50 = pTarget->unk28 >> 12;
        pActor->unk54 = pTarget->unk30 >> 12;
        SpritePlayAnimation(pActor->unk4C, 1);
        pActor->unk20++;
        /* falls through */
    case 9:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->unk20++;
        }
        WorldMapUpdateSpriteGroupFromActor(index + 0x2E, pActor, (WorldMapScreenPos *)pScratch);
        break;
    case 10:
        if (WorldMapStartSlot(pActor->unk06, 4)) {
            pActor->unk24 = 1;
            pActor->unk20 = 2;
            WorldMapDeactivateSpriteGroup(index + 0x2E);
        }
        break;
    case 40:
        pActor->unk28 = g_GameState.fieldScriptMemory[0x13 + index * 3] << 12;
        pActor->unk30 = g_GameState.fieldScriptMemory[0x14 + index * 3] << 12;
        pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
        angle = (&g_GameState.unk1824)[index];
        pActor->unk5C = angle;
        pActor->unk48 = angle;
        pScratch->x = pActor->unk28 + rcos((s16)angle) * 0x30;
        pScratch->z = pActor->unk30 + (-rsin(pActor->unk48)) * 0x30;
        WorldMapBuildDirectionFromPositions(&pActor->unk28, pScratch, &pActor->unk38, &pActor->unk48);
        pActor->unk50 = pScratch->x >> 12;
        pActor->unk54 = pScratch->z >> 12;
        pActor->unk24 = 0;
        SpritePlayAnimation(pActor->unk4C, 1);
        pActor->unk20++;
        /* falls through */
    case 41:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->unk20++;
        }
        break;
    case 42:
        ((u8 *)&g_GameState.unk22B0)[index] = 0;
        pActor->unk20 = 0x40;
        break;
    case 48:
        WorldMapBuildDirectionFromPositions(&pActor->unk28, &g_WorldMapSlots[1].unk28, &pActor->unk38,
                                           &pActor->unk48);
        pActor->unk50 = g_WorldMapSlots[1].unk28 >> 12;
        pActor->unk54 = g_WorldMapSlots[1].unk30 >> 12;
        SpritePlayAnimation(pActor->unk4C, 1);
        pActor->unk20++;
        /* falls through */
    case 49:
        if (WorldMapMoveActorTowardTarget(pActor) == 3) {
            pActor->unk20++;
        }
        break;
    case 50:
        if (WorldMapStartSlot(1, 6)) {
            pActor->unk20 = 0;
        }
        break;
    case 64:
        /* reached from case 42; nothing to do, it falls through to the tail */
        break;
    }
    if (pActor->unk24 == 0) {
        WorldMapAddSlotPosition(0, &pActor->unk28);
    }
    return result;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BB40);
/* 87.64% (476 bytes, retail 476). The whole head, the if/else inversion (written
   as "if (gearRide[2] == 0) { gear position } else { unk20 = 1; unk24 = 1; }",
   which is what puts the short block out of line behind bnez like retail) and the
   whole switch (cases 1-3 gear, 4-6 unk20 = 3, 7 unk20 = 2, 8-13 empty: the empty
   cases are required, retail s table is 13 words = min 1 max 13) are byte-identical.
   What is left is one scheduler/sese tie in the block that sets up unk40..unk5C:
   retail emits lhu g_GameState+0x1824 and the g_WorldMapMode load ABOVE the unk4A/unk58
   constant stores and re-reads unk48 with lh for unk5C (sh 0x48 then lh 0x48), while
   gcc sinks both loads below the stores and folds the re-read into an sll/sra pair.
   A volatile u16 store for unk48 reproduces the lh but loses the rest (84.34), and
   reading unk48 into a volatile u32 local before the store gives 72.12. The permuter
   from this body found nothing better (base 1255 best 668, which is 72.12 on objdiff).
   Everything after the jr table is correct, including the shared unk20/unk24 tail and
   the cross-jumped case 4-6 body.
int func_8008BB40(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = 1;
    if (g_GameState.partyMembers[2] != 0xFF) {
        pActor->unk4C = SpriteAllocFromAnimFile(D_8009CD3C, 0x120, 0x1E0, 0x160, 0x100, 0x40);
        SpritePlayAnimation(pActor->unk4C, 0);
        SpriteSetScale(pActor->unk4C, 0x1800);
        *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
        pActor->unk24 = 0;
    } else {
        pActor->unk24 = 1;
        result = 3;
    }
    pActor->unk28 = g_GameState.unk1820 << 12;
    pActor->unk30 = g_GameState.unk1822 << 12;
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 8;
    pActor->unk58 = 0x1E;
    pActor->unk48 = g_GameState.unk1824;
    pActor->unk5C = pActor->unk48;
    switch (g_WorldMapMode) {
    case 1:
    case 2:
    case 3:
        if (g_GameState.gearRide[2] == 0) {
            pActor->unk28 = g_WorldMapPlayerPosition;
            pActor->unk30 = D_8009C5B4;
            pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
            pActor->unk48 = g_WorldMapPlayerRotation;
        } else {
            pActor->unk20 = 1;
            pActor->unk24 = 1;
        }
        break;
    case 4:
    case 5:
    case 6:
        pActor->unk20 = 3;
        pActor->unk24 = 1;
        if (g_GameState.partyMembers[2] != 0xFF) {
            g_GameState.gearRide[2] = 1;
        }
        break;
    case 7:
        pActor->unk20 = 2;
        pActor->unk24 = 1;
        if (g_GameState.partyMembers[2] != 0xFF) {
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
*/
int WorldMapModeInitCharacterSprite2(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = 1;

    if (g_GameState.partyMembers[2] != 0xFF) {
        pActor->unk4C = SpriteAllocFromAnimFile(D_8009CD3C, 0x120, 0x1E0, 0x160, 0x100, 0x40);
        SpritePlayAnimation(pActor->unk4C, 0);
        SpriteSetScale(pActor->unk4C, 0x1800);
        *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    } else {
        result = 3;
    }
    return result;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BDD0);
/* 93.37% (was 83.03% before the preset change). Byte-identical apart from the same
   three prologue insns as func_8008B54C: original shifts index into v1 before the
   0x100 push and stores the 0x40 argument in front of the jal; gcc stores it right
   after li 0x40 and shifts into v0. Tail is byte-identical.
int func_8008BDD0(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];

    pActor->unk4C = SpriteAllocFromAnimFile(D_8009CD3C, 0x120, 0x1E0, 0x160, 0x100, 0x40);
    SpritePlayAnimation(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x1800);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.unk1820 << 12;
    pActor->unk30 = g_GameState.unk1822 << 12;
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    {
        u16 unk48 = g_GameState.unk1824;
        pActor->unk24 = 1;
        pActor->unk4A = 8;
        pActor->unk20 = 2;
        pActor->unk40 = 0;
        pActor->unk3C = 0;
        pActor->unk38 = 0;
        pActor->unk58 = 0x1E;
        pActor->unk48 = unk48;
    }
    return 1;
}
*/
int WorldMapMoveActorTowardTarget(WorldMapActor *pActor) {
    s32 dx;
    s32 dz;
    s32 result = 0;

    dx = pActor->unk50 - (pActor->unk28 >> 12);
    if ((dx < 0 ? -dx : dx) >= 5) {
        pActor->unk28 += pActor->unk38 * (pActor->unk4A / 2);
    } else {
        result = 1;
    }
    dz = pActor->unk54 - (pActor->unk30 >> 12);
    if ((dz < 0 ? -dz : dz) >= 5) {
        pActor->unk30 += pActor->unk40 * (pActor->unk4A / 2);
    } else {
        result |= 2;
    }
    WorldMapWrapPosition(&pActor->unk28);
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    return result;
}
void WorldMapQueueZoneEntry(s16 arg0, s32 *pArray, s16 arg2, s16 arg3) {
    WorldMapEntry *pEntry = &D_8009BE6C[D_8009BD04];

    pEntry->unk00 = arg0;
    pEntry->unk04 = pArray[0];
    pEntry->unk08 = pArray[1];
    pEntry->unk0C = pArray[2];
    pEntry->unk10 = arg3;
    pEntry->unk14 = arg2;
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
            pos = pPos->unk4;
            if (pos < pEntry->unk08) {
                base = pos - arg2 * 0x1000;
                pos = pEntry->unk08;
            } else if (pEntry->unk14 || pEntry->unk00) {
                base = pEntry->unk08 - pEntry->unk10 * 0x1000;
            } else {
                /* same value as the arm above; retail keeps the branch, so must we */
                base = pEntry->unk08 - pEntry->unk10 * 0x1000;
            }
            if (((pos - base) >> 12) < arg2 + pEntry->unk10) {
                diff.x = (pEntry->unk04 - pPos->x) >> 12;
                diff.z = (pEntry->unk0C - pPos->z) >> 12;
                WorldMapWrapDeltaScaled(&diff);
                dist = SquareRoot0(diff.x * diff.x + diff.z * diff.z);
                if (dist < pEntry->unk14 + arg1) {
                    *pOut = 2;
                } else if (dist < pEntry->unk14 + arg1 + 0x10) {
                    *pOut = 1;
                } else {
                    continue;
                }
                *pOut2 = pEntry->unk00;
                break;
            }
        }
    }
}
void WorldMapUpdateSpriteGroupFromActor(s32 arg0, WorldMapActor *pActor, WorldMapScreenPos *pPos) {
    if (WorldMapGetTileHalfIndex(&pActor->unk28) == 3) {
        pPos->unk0A0 = pActor->unk28 >> 12;
        pPos->unk0A2 = pActor->unk2C >> 12;
        pPos->unk0A4 = pActor->unk30 >> 12;
        pPos->unk0AC = 0;
        pPos->unk0A8 = 0;
        pPos->unk0AA = pActor->unk5C;
        func_80089160(arg0, &pPos->unk0A0, &pPos->unk0A8);
    } else {
        WorldMapDeactivateSpriteGroup(arg0);
    }
}
void WorldMapInitActorSpriteBySlot(WorldMapActor *pActor, int index) {
    void *pSprite;

    pSprite = SpriteAllocFromAnimFile(D_8009BDF8[index], D_8009B18C[index], D_8009B194[index], D_8009B19C[index],
                            D_8009B1A4[index], 0x40);
    pActor->unk4C = pSprite;
    if (g_GameState.gearRide[index] == 1) {
        SpritePlayAnimation(pSprite, 0);
    } else {
        SpritePlayAnimation(pSprite, 3);
    }
    SpriteSetScale(pActor->unk4C, 0x2000);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C364);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C530);
/* 78.83% (432 bytes; retail is 444). Structurally right and byte-identical from the
   WorldMapGetTilePlaneHeight jal onward, including the 32-iteration 0x14-stride copy to
   g_WorldMapTrail and the tail. Two source facts established:
   (1) the mode chain must be written inverted -- "if (mode >= 4) { if (mode < 8) {
       unk20 = 2; unk24 = 1; } } else { ...gearRide... }" -- because that is the only
       spelling that puts the gearRide block out of line behind "bnez v0, A" like
       retail; the natural "if (mode < 4) A else if (mode < 8) B" compiles to the
       mirror image (79.81% but with the two blocks swapped).
   (2) the 16-byte copies are struct assignments (WorldMapVec), which is what makes
       gcc emit the 3-load / 3-store / 1-load / 1-store block move retail has; four
       separate field stores interleave load/store instead.
   What still differs is the same store-sink wall as func_8008D3F0 and func_8008DD6C
   in this unit: retail keeps the three "unk40/unk3C/unk38 = 0" stores above the
   g_WorldMapMode load, gcc sinks them below it, and inside the gearRide block retail
   keeps "unk20 = 1" first while gcc sinks it below the two g_WorldMapPlayerPosition/D_8009C5B4
   global loads. Source order of the stores does not change it. Everything else
   (lh reload of unk48 for unk5C, the delay slot store, unk4A = 0xC, the tail) matches.
s32 func_8008C530(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 0);
    WorldMapTrail *pTrail;
    s32 mode;
    s32 i;

    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[0];
    pActor->unk4A = 0xC;
    pActor->unk5C = pActor->unk48;
    mode = g_WorldMapMode;
    if (mode > 0) {
        if (mode >= 4) {
            if (mode < 8) {
                pActor->unk20 = 2;
                pActor->unk24 = 1;
            }
        } else {
            if (g_GameState.gearRide[0] == 1) {
                pActor->unk20 = 1;
                pActor->unk28 = g_WorldMapPlayerPosition;
                pActor->unk30 = D_8009C5B4;
                pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
                pActor->unk48 = g_WorldMapPlayerRotation;
                g_WorldMapScrollTargetX = *(WorldMapVec *)&pActor->unk28;
                g_WorldMapPlayerAngle = pActor->unk48;
                g_WorldMapTrailCursor = 0;
                pTrail = g_WorldMapTrail;
                for (i = 0; i < 0x20; i++, pTrail++) {
                    pTrail->vec = *(WorldMapVec *)&pActor->unk28;
                    pTrail->unk10 = pActor->unk48;
                }
            }
        }
    }
    g_GameState.fieldScriptMemory[0x16] = pActor->unk28 >> 12;
    g_GameState.fieldScriptMemory[0x17] = pActor->unk30 >> 12;
    *(u16 *)&g_GameState.unk1826[0] = pActor->unk48;
    return result;
}

*/
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

    pActor->unk4C = SpriteAllocFromAnimFile(D_8009BDF8[0], 0x100, 0x1FD, 0x140, 0x140, 0x40);
    SpritePlayAnimation(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x2000);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.fieldScriptMemory[0x16] << 12;
    pActor->unk30 = g_GameState.fieldScriptMemory[0x17] << 12;
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    pActor->unk24 = 1;
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[0];
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C844);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D3F0);
/* 81.33%: everything after the func_8008C364 call is correct except the store
   order in the head: the original puts the three zero stores (0x40, 0x3C, 0x38)
   first, then hoists the g_GameState+0x1828 load above them, and re-reads
   unk48 with lh; gcc here sinks the zero stores below the sll/sra chain and cse
   folds the re-read into the lhu. The mode < 4 / mode < 8 / gearRide blocks and
   the whole tail are byte-identical. Also tried: unk5C assigned after the
   g_WorldMapMode load (identical output), ascending zero stores.
   Re-diffed under the current XenoWorldMap flags, still 81.33%. Two new facts:
   the source ORDER of the zero stores makes no difference - gcc's post-reload
   scheduler sinks all three to just before the branch and floats the sh/lh pair
   above the g_WorldMapMode load whatever the C order is, and hoisting the mode load
   to the top of the head only makes it worse. And storing unk48 through a
   volatile u16 pointer reproduces the original's lh reload (instead of an
   sll/sra pair, which makes the function 304 bytes instead of the original 300):
   that variant scores 80.07% but has the original's exact instruction multiset,
   with only the position of the two blocks differing, so it is the base to
   continue from rather than the text below.
   Round 56: re-diffed with the body below live, still 81.33%; the whole diff is
   the positions of the three zero stores and the sh/lh pair, nothing else.
int func_8008D3F0(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 1);
    s32 mode;

    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk58 = 0xF;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[2];
    pActor->unk5C = pActor->unk48;
    mode = g_WorldMapMode;
    if (mode > 0) {
        if (mode < 4) {
            if (g_GameState.gearRide[1] == 1) {
                pActor->unk20 = 1;
                pActor->unk28 = g_WorldMapPlayerPosition;
                pActor->unk30 = D_8009C5B4;
                pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
                pActor->unk48 = g_WorldMapPlayerRotation;
            }
        } else if (mode < 8) {
            pActor->unk20 = 2;
            pActor->unk24 = 1;
        }
    }
    g_GameState.fieldScriptMemory[0x19] = pActor->unk28 >> 12;
    g_GameState.fieldScriptMemory[0x1A] = pActor->unk30 >> 12;
    *(u16 *)&g_GameState.unk1826[2] = pActor->unk48;
    return result;
}
*/


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

    pActor->unk4C = SpriteAllocFromAnimFile(D_8009BDFC[0], 0x100, 0x1FC, 0x160, 0x140, 0x40);
    SpritePlayAnimation(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x2000);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.fieldScriptMemory[0x19] << 12;
    pActor->unk30 = g_GameState.fieldScriptMemory[0x1A] << 12;
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    pActor->unk24 = 1;
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[2];
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D678);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008DD6C);
/* 81.33%: structurally the same function as func_8008D3F0 (mode 2, gearRide[2],
   g_GameState+0x182A and unk1930[0x1C]/[0x1D]); the whole branch body and the tail
   are byte-identical. Two head differences remain: (1) the original emits the three
   zero stores to 0x40/0x3C/0x38 immediately after the func_8008C364 call and the
   lhu of g_GameState+0x182A right after them, while gcc sinks the zero stores below
   the g_WorldMapMode load; (2) the original re-reads unk48 with lh and sign-extends for
   unk5C, gcc keeps the value in a register (sll/sra pair). Also tried a u16 temp for
   the g_GameState load: same 81.33%, and it makes the unk5C store worse.
int func_8008DD6C(int index) {
    WorldMapActor *pActor = &g_WorldMapSlots[index];
    s32 result = func_8008C364(pActor, 2);
    s32 mode;

    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk58 = 0x1F;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[4];
    pActor->unk5C = pActor->unk48;
    mode = g_WorldMapMode;
    if (mode > 0) {
        if (mode < 4) {
            if (g_GameState.gearRide[2] == 1) {
                pActor->unk20 = 1;
                pActor->unk28 = g_WorldMapPlayerPosition;
                pActor->unk30 = D_8009C5B4;
                pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
                pActor->unk48 = g_WorldMapPlayerRotation;
            }
        } else if (mode < 8) {
            pActor->unk20 = 2;
            pActor->unk24 = 1;
        }
    }
    g_GameState.fieldScriptMemory[0x1C] = pActor->unk28 >> 12;
    g_GameState.fieldScriptMemory[0x1D] = pActor->unk30 >> 12;
    *(u16 *)&g_GameState.unk1826[4] = pActor->unk48;
    return result;
}
*/
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

    pActor->unk4C = SpriteAllocFromAnimFile(D_8009BE00[0], 0x100, 0x1FB, 0x280, 0x100, 0x40);
    SpritePlayAnimation(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x2000);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.fieldScriptMemory[0x1C] << 12;
    pActor->unk30 = g_GameState.fieldScriptMemory[0x1D] << 12;
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    pActor->unk24 = 1;
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[4];
    return 1;
}
