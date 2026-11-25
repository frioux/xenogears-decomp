#include "common.h"
#include "psyq/libgpu.h"
#include "battle/battle_work.h"
#include "battle/actor.h"

// The colour ramp the pulse triangles fade along: the whole word is stepped,
// the low byte is the shade written into the triangle vertices.
typedef union {
    /* 0x0 */ s32 value;
    /* 0x0 */ u8 level;
} BattleColorRamp;

// g_BattlePulseTrianglePrims: the eight POLY_G3 triangles BattleInitPulseTriangles
// builds, then the ramp and flags BattleUpdateRotationBuffer pulses them with.
typedef struct {
    /* 0x00 */ POLY_G3 prims[8];
    /* 0xE0 */ BattleColorRamp fade;
    /* 0xE4 */ u8 firstPrim; // index of the first triangle drawn this frame
    /* 0xE5 */ u8 fading;    // 1 while fading down, 0 while fading up
    /* 0xE6 */ u8 enabled[4];
} BattlePulseWork; // Size: 0xEA

// Partial view of the packet prim pool g_BattleGraphicsWork->pPacketPrims points at
// (BATTLE_PACKET_PRIM_POOL_SIZE bytes): the prim index each list of POLY_FT4s is
// drawn at, and the flag gating the four blocks at +0x0.
typedef struct {
    /* 0x000 */ POLY_FT4 prims[0x29];
    /* 0x668 */ u8 ft4PrimIndex;  // prim index of the blocks at +0x0/+0x50/+0xA0/+0xF0
    /* 0x669 */ u8 pending;       // while set, the four blocks at +0x0 have prims to add
    /* 0x66A */ u8 pairPrimIndex; // prim index of the lists at +0x280/+0x2D0/+0x320/+0x370
    /* 0x66B */ u8 pairPrimsPending; // while set, the pair lists and the menu cell list are added
    /* 0x66C */ u8 cellPrimIndex;  // prim index the cell prims were built at
    /* 0x66D */ u8 labelDrawn;     // set once the cell drew its label string
    /* 0x66E */ u8 digitCount;     // digits written into that bank
    /* 0x66F */ u8 numberPrimsPending; // while set, the cell's number prims are added
} BattlePacketPrims; // Size: 0x670

// Partial view of the sprite column draw buffer g_BattleSpriteColumnWork points at
// (0x5DA4 bytes, allocated by BattleAllocDrawBuffer). 0x0000-0x5D70 is prim data:
// one area per prim list, 0x50 bytes per package of that list, in the order the
// lists were laid out. Each list's package count and prim start are then read from
// a pair at 0x5D70 (the 0x4CE0 list and the 20-package panel list have their own),
// plus the counter that gates the 0x4CE0 list. A list's count and start are indexed
// independently: the lists are appended in a different order than declared here.
typedef struct {
    /* 0x0000 */ u8 unk0[0x3C0];
    /* 0x03C0 */ u8 unk3C0[0x960];
    /* 0x0D20 */ u8 unkD20[0x280];
    /* 0x0FA0 */ u8 unkFA0[0x780];
    /* 0x1720 */ u8 unk1720[0xE10];
    /* 0x2530 */ u8 unk2530[0xD70];
    /* 0x32A0 */ u8 unk32A0[0x820];
    /* 0x3AC0 */ u8 unk3AC0[0x370];
    /* 0x3E30 */ u8 unk3E30[0x50];
    /* 0x3E80 */ u8 unk3E80[0x550];
    /* 0x43D0 */ u8 unk43D0[0x2D0];
    /* 0x46A0 */ u8 unk46A0[0x640];
    /* 0x4CE0 */ u8 unk4CE0[0x190];
    /* 0x4E70 */ u8 unk4E70[0x140];
    /* 0x4FB0 */ u8 unk4FB0[0xF0];
    /* 0x50A0 */ u8 unk50A0[0x140];
    /* 0x51E0 */ u8 unk51E0[0xA0];
    /* 0x5280 */ u8 unk5280[0x140];
    /* 0x53C0 */ u8 unk53C0[0x190];
    /* 0x5550 */ u8 unk5550[0xF0];
    /* 0x5640 */ u8 unk5640[0x640];
    /* 0x5C80 */ u8 unk5C80[0xF0];
    /* 0x5D70 */ u8 packageCount[0x13]; // packages each list holds
    /* 0x5D83 */ u8 primStart[0x13];    // prim index each list was built at
    /* 0x5D96 */ u8 animPackageCount;   // package count of the 0x32A0 list
    /* 0x5D97 */ u8 animPrimStart;      // prim index of the 0x32A0 list
    /* 0x5D98 */ u8 panelPrimStart;     // prim index of the 20-package 0x46A0 list
    /* 0x5D99 */ u8 unk5D99[0x7];
    /* 0x5DA0 */ u8 listPrimStart;      // prim index the 0x4CE0 list was built at
    /* 0x5DA1 */ u8 listPackageCount;   // packages that list holds
    /* 0x5DA2 */ u16 columnCounter;     // frames the 0x4CE0 list has been drawn for
} BattleSpriteColumnWork; // Size: 0x5DA4

// the slot's display value, one 8-byte record per slot
typedef struct {
    /* 0x0 */ u8 value;
    /* 0x1 */ u8 unk1[7];
} BattleSlotDisplayValue; // Size: 0x8

extern u8 g_DebugPageIndex;
extern u8 *g_BattleGraphicsWork;
extern u8 g_BattleDisplayMode;
extern u8 g_BattleFrameDrawSuppressed;
extern u8 *g_BattleCurOT[];
extern s32 g_BattleRenderString;
extern BattlePrimPool *g_BattlePrimPool;
extern s32 g_BattlePrimStartIndex;
// the same object as g_BattlePrimStartIndex, declared as an array so cc1 2.6.0 holds
// its address in a register across a call (a scalar extern is rematerialised).
extern s32 g_BattlePrimStartIndexTable[] __asm__("g_BattlePrimStartIndex");
extern void *g_BattleSpriteArchive;
extern BattleWork *g_BattleDisplayState;
extern BattlePulseWork *g_BattlePulseTrianglePrims;
extern u16 g_BattleSlotScreenOrigins[];
extern u8 g_BattleSlotPageIndex;
extern u8 g_BattleActorCharIds[];
extern BattleSpriteColumnWork *g_BattleSpriteColumnWork;
extern u8 g_BattleSlotActorIds[];
extern u8 g_BattleSlotFlags[];
extern BattleSlotDisplayValue g_BattleSlotDisplayValue[];
// the per-slot table family (stride 0x170) seen from the slot's value pair:
// D_800CCDC4 is the slot's current value, D_800CCDC6 the value it is filled to
// the same object, read through a volatile lvalue: retail re-reads it (and so
// re-derives the whole prim address) before every store
extern volatile s32 g_BattlePrimStartIndexVolatile __asm__("g_BattlePrimStartIndex");
extern BattleTableEntry D_800CCDC4[];
extern BattleTableEntry D_800CCDC6[];
// the same table family seen from its other fields: the entry's +0x50 word and the
// two +0xA8 words, indexed 0xB8 words apart (the 0x170-byte stride)
extern u16 g_BattleSlotEntries[];
extern u16 D_800CCD6C[];
extern u16 D_800CCD6E[];
extern s16 g_BattleSlotCountdownTimers[];
extern BattleSlotGraphics *g_BattleSlotGraphics[];

extern u8 g_BattlePrimBuffer[];
extern volatile u8 g_BattleTextSlotVisible[];
extern volatile u8 g_BattleTextSlotFlags[];

extern s32 func_8002675C(void *pDir, s32 index, void *pPrims, s32 context, s16 x, s16 y, s16 scale);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *p, u16 x, s16 y, u8 u, u8 v, u8 dx);
extern void BattleAddPrimListToOT(u8 *pDest, s32 packageCount, s32 primStart);
extern void BattleRenderActorSplitDigitPanel(s32 index, u8 arg1);
extern void BattleRenderActorStatDigits(s32 index, u8 arg1);
extern s32 func_80072F38(s32 index, u8 arg1);
extern void BattleRenderActorValueDigits(s32 index);
extern s32 func_80025FA8(void *pDir, s32 id, u8 *pPrim, s32 context, u16 x, u16 y, u16 u, u16 v, u16 w);

/*
89.56%, retail's EXACT size and instruction count (201 insns, 804 bytes = 0x4074..0x4398),
so nothing is missing from the body: the residual is register allocation across the six
loops plus three load-order/operand-order spots.
THE OLD NOTE'S OFFSETS WERE WRONG BY 0x10 and the old body did not compile. maspsx
renders an out-of-range displacement as `lui $at,1; addu $at,base,$at; lbu rt,-0x7AC3($at)`,
i.e. imm 0x853D sign-extended -0x7AC3, so retail's four bytes are 0x853C..0x853F
(= off3 + 0x835C + 0x1E0..0x1E3, the last four bytes of each 0x1E4-byte entry), NOT
0x852C..0x852F. And g_BattleDisplayState is a BattleWork pointer now, so the old
`g_BattleDisplayState[0x97]` park was a compile error: measured parks in this file must be
re-unparked and re-measured, never trusted.
What the body is now (all of it improved over the old park): typed BattleWork fields
(primRowCount/primRowStart, valueSpriteCount/valueSpritePrimStart, statSpriteCount,
statDigitPrimStart, actorSprite*, BATTLE_SLOT_CHARACTER_NONE); `pOT = g_BattleCurOT` as a
local for the LAST loop only, which reproduces retail's hoist of &g_BattleCurOT into $s4
(`lui/addiu s4` in the preheader, `lw a0,0(s4)` / `lw v1,0x30(s4)` in the body) - do not
"simplify" it back to g_BattleCurOT[0], that is what lost the s4 slot; `off + (u32)
g_BattleGraphicsWork` offset-first for the addu order; case 2 adds the 0xA0 to the case-1
pointer LAST (`... + gfx) + 0xA0`), which retail also does.
Residual, all allocation (nothing structural left):
The five call loops, as retail runs them: (1) primRowCount iterations of AddPrim into
g_BattleCurOT[0] + 4 of the 0x10-stride prim at gfx + 0x908 indexed by
(primRowStart + i * 2) * 0x10; (2) FOUR (not three) BattleAddPrimListToOT at
gfx + 0x2E08 + i * 0x320 with the byte run at state + i + 0x74 / + 0x84, which is the
three panels plus the 1.0-scale one (panelPackageCounts / panelPrimStart / then
scaledPanelPackageCount / scaledPanelPrimStart) - that is why the header's 3-long arrays
have to be walked with a byte pointer here; (3) three 0x1E4-stride entries, gated by
state + i + 0xCC, a switch on the byte at gfx + off3 + 0x853D into the atlas at
off3 + 0x835C (+ 0xA0 for case 2), then the draw at gfx + 0x6008 + i * 0x140 with
valueSpriteCount[i] / valueSpritePrimStart[i]; (4) the optional actorSpriteActive draw at
gfx + 0x9C8; (5) three draws at gfx + 0x3A88 + i * 0xC80 with statSpriteCount[i] /
statDigitPrimStart[i]; (6) three entries at 0x1C / 0x50 / 0x1E0 strides, skipping those
whose g_BattleSlotActorIds[i * 0x1C] is BATTLE_SLOT_CHARACTER_NONE, and the AddPrim whose
index is (i * 2 + pOT[0xC]) * 36 + 0x740 (0x30 bytes into g_BattleCurOT is the ctx).
(1)-(6) above are allocation-only residuals:
(1) retail's saved-register plan is loop1 i=s0; loop2 i=s0 off=s1; loop3 i=s0 off=s2
    off3=s1 0x835C=s3; loop5 i=s0 off=s1; loop6 i=s0 off=s2 off2=s1 off3=s3 pOT=s4.
    This body rotates off/off2/off3 through s1/s2/s3 differently in loops 3 and 6 (retail
    gives the FIRST-declared the HIGHER register there), which is what carries most of the
    92 differing lines.
(2) loop 1's `lbu` of primRowStart lands in $v1 here (reusing the state pointer's
    register) where retail puts it in $a1 and keeps state in $v1; base-first vs
    constant-first spelling of the same expression does not change it.
(3) loop 6's `state + i` addus come out index-first here (`addu v1,s0,a2`) vs retail's
    `addu v1,a2,s0`. Writing the byte reads as `*(u8 *)((u8 *)g_BattleDisplayState + i +
    0xCC)` instead of subscripts bought 0.1 (89.46 -> 89.56); it is the only lever that
    moved the number.
Tried and measured, do not repeat: the old base-first `g_BattleGraphicsWork + off` spelling
(89.43), loop-6 for-init reordered to off3/off/off2 (identical), p[0x852D] vs the correct
0x853D (that one is semantic, not a lever).
*/
// 89.56% (re-measured this round), retail's exact size and count: 201 insns / 804 bytes (0x4074..0x4398), frame 0x30, saves ra+s0-s4 exact. Nothing structural left: the 92 differing lines are register allocation across the six call loops plus the placement of the loop increments (retail puts each accumulator's addiu right after its last use and the OTHER increment in the jal delay slot; this body swaps that per loop).
// Re-measured with every remaining raw cast replaced by the typed BattleWork field (panelPackageCounts[i], panelPrimStart[i], panelPending[i], unk78[i], valueSpritePrimStart[i], statDigitPrimStart[i]): identical instruction stream, 89.56% - the typed form is what this park now holds.
// Root residual, exactly: in loop 3 retail has off3 in $s1 and off in $s2, in loop 6 off2 in $s1, off in $s2 and off3 in $s3; this body gives off $s1 in both, so the increments land in the wrong slots. Retail's loop-6 preheader emits the for-init as off3, off, off2 (this body emits off, off2, off3) - reordering the init to off3/off/off2 was tried and is identical, and so was the old base-first gfx spelling (89.43).
// A permuter run is the right next step (structure is right, all six loops present, exact size); it is out of scope for a lane that cannot exceed 99%.
// Re-measured this round at 201 insns / 804 bytes = retail's exact size and instruction count, frame 0x30, saves ra+s0-s4 exact: 89.56%, 92 differing lines, all register allocation in loops 3 and 6 as described above.
// Preset screen (new this round, closes that leg): gcc-2.7.2-cdk-psx 81.86 and gcc-2.7.2-psx does not compile the unit's own body, so the XenoLibrary 2.6.0 preset is right and the residual is not a compiler-version artefact.
// Also measured this round, no change: reversing the declaration order of i/off/off2/off3/p/pOT at the top of the body is byte-identical (89.56, same 92 lines), so allocno creation here follows statement order, not declaration order - do not spend a pass on that. Below 99% no permuter run is allowed, so this park is closed unless a lane gets the loop-3/loop-6 allocno assignment from another lever.
/*
void func_80073B64(void) {
    s32 i;
    s32 off;
    s32 off2;
    s32 off3;
    u8 *p;
    u8 **pOT;

    for (i = 0; i < g_BattleDisplayState->primRowCount; i++) {
        AddPrim(g_BattleCurOT[0] + 4,
                (u8 *)(0x908 + (g_BattleDisplayState->primRowStart + i * 2) * 0x10 +
                       (u32)g_BattleGraphicsWork));
    }

    for (i = 0, off = 0x2E08; i < 4; i++, off += 0x320) {
        BattleAddPrimListToOT((u8 *)(off + (u32)g_BattleGraphicsWork),
                              g_BattleDisplayState->panelPackageCounts[i],
                              g_BattleDisplayState->panelPrimStart[i]);
    }

    for (i = 0, off = 0x6008, off3 = 0; i < 3; i++, off += 0x140, off3 += 0x1E4) {
        if (g_BattleDisplayState->panelPending[i] != 0) {
            p = (u8 *)(off3 + (u32)g_BattleGraphicsWork);
            switch (p[0x853D]) {
            case 1:
                BattleAddPrimListToOT((u8 *)(off3 + 0x835C + (u32)g_BattleGraphicsWork),
                                      p[0x853E], p[0x853C]);
                break;
            case 2:
                BattleAddPrimListToOT((u8 *)(off3 + 0x835C + (u32)g_BattleGraphicsWork) + 0xA0,
                                      p[0x853F], p[0x853C]);
                break;
            }
            BattleAddPrimListToOT((u8 *)(off + (u32)g_BattleGraphicsWork),
                                  g_BattleDisplayState->valueSpriteCount[i],
                                  g_BattleDisplayState->valueSpritePrimStart[i]);
        }
    }

    if (g_BattleDisplayState->actorSpriteActive != 0) {
        BattleAddPrimListToOT((u8 *)(0x9C8 + (u32)g_BattleGraphicsWork),
                              g_BattleDisplayState->actorSpritePackageCount,
                              g_BattleDisplayState->actorSpritePrimStart);
    }

    for (i = 0, off = 0x3A88; i < 3; i++, off += 0xC80) {
        BattleAddPrimListToOT((u8 *)(off + (u32)g_BattleGraphicsWork),
                              g_BattleDisplayState->statSpriteCount[i],
                              g_BattleDisplayState->statDigitPrimStart[i]);
    }

    pOT = g_BattleCurOT;
    for (i = 0, off = 0x818, off2 = 0, off3 = 0; i < 3;
         i++, off += 0x50, off2 += 0x1C, off3 += 0x1E0) {
        if (g_BattleSlotActorIds[off2] != BATTLE_SLOT_CHARACTER_NONE) {
            BattleAddPrimListToOT((u8 *)(off + (u32)g_BattleGraphicsWork), 1,
                                  g_BattleDisplayState->unk83);
            if (g_BattleDisplayState->panelPending[i] != 0) {
                BattleAddPrimListToOT((u8 *)(off3 + (u32)g_BattleGraphicsWork),
                                      g_BattleDisplayState->unk78[i],
                                      g_BattleDisplayState->unkA2);
                AddPrim(pOT[0] + 4,
                        (u8 *)((i * 2 + ((s32 *)pOT)[0xC]) * 36 + 0x740 +
                               (u32)g_BattleGraphicsWork));
            }
        }
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073B64);


void BattleAddTwoPrimLists(void) {
    s32 i;
    s32 offset;
    BattleWork *pWork;

    if (g_BattleDisplayState->primListsPending == 0) {
        return;
    }

    i = 0;
    offset = 0x641C;
    do {
        pWork = g_BattleDisplayState;
        BattleAddPrimListToOT((u8 *)(offset + (s32)g_BattleGraphicsWork), pWork->packageCounts[i], pWork->primStart);
        offset += 0xFA0;
        i++;
    } while (i < 2);
}

void BattleAddThreeFlaggedPrimLists(void) {
    if (g_BattleDisplayState->spriteColumnPending != 0) {
        BattleAddPrimListToOT(g_BattleGraphicsWork + 0xBA8, g_BattleDisplayState->spriteColumnPackageCount, g_BattleDisplayState->spriteColumnPrimStart);
    }

    if (g_BattleDisplayState->cursorPending != 0) {
        BattleAddPrimListToOT(g_BattleGraphicsWork + 0x27C8, g_BattleDisplayState->cursorPackageCount, g_BattleDisplayState->cursorPrimStart);
    }

    if (g_BattleDisplayState->menuColumnPending != 0) {
        BattleAddPrimListToOT(g_BattleGraphicsWork + 0x1E68, g_BattleDisplayState->menuColumnPackageCount, g_BattleDisplayState->menuColumnPrimStart);
    }
}

void BattleAddPanelAndSlotPrimLists(void) {
    BattleSlotGraphics *pSlot;
    s32 i;
    u32 primOfs;
    u32 slotPrimOfs;
    u32 primOfs2;

    primOfs = g_BattlePrimStartIndex * 12 + 0x8920;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(primOfs + (u32)g_BattleGraphicsWork));

    for (i = 0; i < 4; i++) {
        if (g_BattleDisplayState->panelActive[i] != 0 && g_BattleDisplayMode == 1) {
            slotPrimOfs = (i * 2 + g_BattleDisplayState->panelPrimStart[i]) * 52 + 0x5A0;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(slotPrimOfs + (u32)g_BattleGraphicsWork));
        }
    }

    primOfs2 = g_BattlePrimStartIndex * 12 + 0x8908;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(primOfs2 + (u32)g_BattleGraphicsWork));

    for (i = 0; i < 7; i++) {
        if (g_BattleDisplayState->primFlags[i] != 0) {
            pSlot = g_BattleSlotGraphics[i];
            BattleAddPrimListToOT((u8 *)pSlot, pSlot->packageCount, pSlot->packagePrimStart);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_FT4) + 0x280]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_FT4) + 0x2D0]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_FT4) + 0x320]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_FT4) + 0x370]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_FT4) + 0x140]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_FT4) + 0x190]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_FT4) + 0x1E0]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_FT4) + 0x230]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(POLY_G4) + 0x3C0]);
            AddPrim(g_BattleCurOT[0] + 4, &((u8 *)pSlot)[pSlot->packagePrimStart * sizeof(DR_MODE) + 0x408]);
        }
    }
}

void BattleRenderSlotStatDigits(void) {
    s32 flagOfs;
    s32 i;
    s32 offset;
    s32 value;

    for (i = 0, offset = 0; i < 3; i++, offset += 0x1C) {
        if (g_BattleDisplayState->slotMarks[i] != 0) {
            g_BattleDisplayState->statSpriteCount[i] = 0;
            g_BattleDisplayState->valueSpriteCount[i] = 0;
            flagOfs = i * 8; // also what keeps retail's sll in the 0x7F test's delay slot
            if (g_BattleSlotActorIds[offset] != BATTLE_SLOT_CHARACTER_NONE) {
                BattleRenderActorValueDigits(i);
                value = func_80072F38(i, g_BattleSlotFlags[flagOfs]);
                if (g_BattleSlotFlags[flagOfs] != 0) {
                    BattleRenderActorStatDigits(i, value);
                } else {
                    BattleRenderActorSplitDigitPanel(i, value);
                }
                g_BattleDisplayState->statDigitPrimStart[i] = *(u8 *)&g_BattlePrimStartIndex;
                g_BattleDisplayState->slotMarks[i] = 0;
            }
        }
    }
}
void BattleAddFourSlotPrims(void) {
    BattlePacketPrims *p;

    p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
    if (p->pending == 0) {
        return;
    }

    // Each AddPrim re-reads pPacketPrims, as retail does. The integer sum below (not
    // &p->prims[i + n]) is also retail's: the subscript form puts the base first in the
    // addu and measures 99.43%.

    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->ft4PrimIndex * sizeof(POLY_FT4) + (s32)p));
    p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->ft4PrimIndex * sizeof(POLY_FT4) + (0x50 + (s32)p)));
    p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->ft4PrimIndex * sizeof(POLY_FT4) + (0xA0 + (s32)p)));
    p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->ft4PrimIndex * sizeof(POLY_FT4) + (0xF0 + (s32)p)));
}
void BattleAddPrimPair320(void) {
    BattlePacketPrims *p;

    p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->pairPrimIndex * sizeof(POLY_FT4) + (0x320 + (s32)p)));
    p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->pairPrimIndex * sizeof(POLY_FT4) + (0x370 + (s32)p)));
}
void BattleAddPrimPair280(void) {
    BattlePacketPrims *p;

    p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->pairPrimIndex * sizeof(POLY_FT4) + (0x280 + (s32)p)));
    p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
    AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->pairPrimIndex * sizeof(POLY_FT4) + (0x2D0 + (s32)p)));
}

void BattleAddDisplayStatePrims(void) {
    BattlePacketPrims *p;

    switch (g_BattleDisplayState->primState) {
    case 1:
        BattleAddFourSlotPrims();
        p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
        if (p->pending != 0) {
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * sizeof(POLY_FT4) + (0x410 + (s32)p)));
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * sizeof(POLY_FT4) + (0x460 + (s32)p)));
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * sizeof(POLY_FT4) + (0x4B0 + (s32)p)));
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * sizeof(POLY_FT4) + (0x500 + (s32)p)));
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * sizeof(POLY_FT4) + (0x550 + (s32)p)));
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * sizeof(POLY_FT4) + (0x5A0 + (s32)p)));
        }
        p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
        if (p->pairPrimsPending != 0) {
            if (p->digitCount != 0) {
                AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * sizeof(POLY_FT4) + (0x3C0 + (s32)p)));
            }
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->pairPrimIndex * sizeof(POLY_FT4) + (0x140 + (s32)p)));
            BattleAddPrimListToOT(
                (u8 *)((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims + 0x190,
                ((BattlePacketPrims *)((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims)->digitCount,
                ((BattlePacketPrims *)((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims)->pairPrimIndex);
            BattleAddPrimPair280();
            BattleAddPrimPair320();
        }
        break;
    case 2:
        BattleAddFourSlotPrims();
        p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
        if (p->pairPrimsPending != 0) {
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->pairPrimIndex * sizeof(POLY_FT4) + (0x140 + (s32)p)));
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->pairPrimIndex * sizeof(POLY_FT4) + (0x190 + (s32)p)));
        }
        if (((BattlePacketPrims *)((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims)->labelDrawn != 0) {
            BattleAddPrimPair320();
            BattleAddPrimPair280();
        }
        break;
    case 3:
        BattleAddFourSlotPrims();
        p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
        if (p->numberPrimsPending != 0) {
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * 36 + (0x5F0 + (s32)p)));
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * 24 + (0x638 + (s32)p)));
        }
        break;
    case 4:
        BattleAddFourSlotPrims();
        p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
        if (p->pairPrimsPending != 0) {
            if (p->digitCount != 0) {
                AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->cellPrimIndex * sizeof(POLY_FT4) + (0x3C0 + (s32)p)));
            }
            p = ((BattleGraphicsWork *)g_BattleGraphicsWork)->pPacketPrims;
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(p->pairPrimIndex * sizeof(POLY_FT4) + (0x140 + (s32)p)));
            BattleAddPrimPair280();
            BattleAddPrimPair320();
        }
        break;
    case 5:
        BattleAddFourSlotPrims();
        break;
    }
}

void BattleAddSpriteColumnPrimLists(void) {
    u16 counter;

    if (g_BattleDisplayState->actorPrimListsPending != 0) {
        BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk3E80, g_BattleSpriteColumnWork->packageCount[7],
                      g_BattleSpriteColumnWork->primStart[3]);
        BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk5280, g_BattleSpriteColumnWork->packageCount[12],
                      g_BattleSpriteColumnWork->primStart[8]);
        BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk53C0, g_BattleSpriteColumnWork->packageCount[13],
                      g_BattleSpriteColumnWork->primStart[9]);
        BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk43D0, g_BattleSpriteColumnWork->packageCount[15],
                      g_BattleSpriteColumnWork->primStart[11]);
    }

    if (g_BattleDisplayState->actorDrawActive == 0) {
        return;
    }

    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk46A0, 20, g_BattleSpriteColumnWork->panelPrimStart);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk0, g_BattleSpriteColumnWork->packageCount[0],
                  g_BattleSpriteColumnWork->primStart[15]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk1720, g_BattleSpriteColumnWork->packageCount[4],
                  g_BattleSpriteColumnWork->primStart[0]);

    counter = g_BattleSpriteColumnWork->columnCounter + 1;
    g_BattleSpriteColumnWork->columnCounter = counter;
    if ((s16)counter < 15) {
        BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk4CE0, g_BattleSpriteColumnWork->listPackageCount,
                      g_BattleSpriteColumnWork->listPrimStart);
    } else if ((s16)counter >= 21) {
        g_BattleSpriteColumnWork->columnCounter = 0;
    }

    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk3AC0, g_BattleSpriteColumnWork->packageCount[5],
                  g_BattleSpriteColumnWork->primStart[1]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk3E30, g_BattleSpriteColumnWork->packageCount[6],
                  g_BattleSpriteColumnWork->primStart[2]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk4E70, g_BattleSpriteColumnWork->packageCount[8],
                  g_BattleSpriteColumnWork->primStart[4]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk4FB0, g_BattleSpriteColumnWork->packageCount[9],
                  g_BattleSpriteColumnWork->primStart[5]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk50A0, g_BattleSpriteColumnWork->packageCount[10],
                  g_BattleSpriteColumnWork->primStart[6]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk51E0, g_BattleSpriteColumnWork->packageCount[11],
                  g_BattleSpriteColumnWork->primStart[7]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk2530, g_BattleSpriteColumnWork->packageCount[14],
                  g_BattleSpriteColumnWork->primStart[10]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk32A0, g_BattleSpriteColumnWork->animPackageCount,
                  g_BattleSpriteColumnWork->animPrimStart);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk3C0, g_BattleSpriteColumnWork->packageCount[1],
                  g_BattleSpriteColumnWork->primStart[16]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unkD20, g_BattleSpriteColumnWork->packageCount[2],
                  g_BattleSpriteColumnWork->primStart[17]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unkFA0, g_BattleSpriteColumnWork->packageCount[3],
                  g_BattleSpriteColumnWork->primStart[18]);
}
void BattleUpdateRotationBuffer(void) {
    s32 i;

    if (g_BattleDisplayState->pulseTriangleActive == 0) {
        return;
    }

    if (g_BattlePulseTrianglePrims->fading != 0) {
        g_BattlePulseTrianglePrims->fade.value -= 4;
        if (g_BattlePulseTrianglePrims->fade.value < 0x40) {
            g_BattlePulseTrianglePrims->fading = 0;
            g_BattlePulseTrianglePrims->fade.value = 0x40;
        }
    } else {
        g_BattlePulseTrianglePrims->fade.value += 4;
        if (g_BattlePulseTrianglePrims->fade.value >= 0x100) {
            g_BattlePulseTrianglePrims->fading = 1;
            g_BattlePulseTrianglePrims->fade.value = 0xFC;
        }
    }

    i = 0;
    do {
        if (g_BattlePulseTrianglePrims->enabled[i] != 0) {
            // Retail re-derives the packet address at each store instead of keeping it in
            // a register, which only the explicit offset form above produces; the typed
            // g_BattlePulseTrianglePrims->prims[i] form shares it and measures 91.78%.
            ((POLY_G3 *)((i * 2 + g_BattlePulseTrianglePrims->firstPrim) * sizeof(POLY_G3) +
                         (s32)g_BattlePulseTrianglePrims))
                ->r0 = g_BattlePulseTrianglePrims->fade.level;
            ((POLY_G3 *)((i * 2 + g_BattlePulseTrianglePrims->firstPrim) * sizeof(POLY_G3) +
                         (s32)g_BattlePulseTrianglePrims))
                ->g0 = 0;
            ((POLY_G3 *)((i * 2 + g_BattlePulseTrianglePrims->firstPrim) * sizeof(POLY_G3) +
                         (s32)g_BattlePulseTrianglePrims))
                ->b0 = 0;
            AddPrim(g_BattleCurOT[0] + 4,
                    (u8 *)((i * 2 + g_BattlePulseTrianglePrims->firstPrim) * sizeof(POLY_G3) +
                           (s32)g_BattlePulseTrianglePrims));
        }
        i++;
    } while (i < 4);
}
void BattleAddThreePrimLists(void) {
    if (g_BattleDisplayState->spriteWorkListsPending == 0) {
        return;
    }

    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk5550, g_BattleSpriteColumnWork->packageCount[16],
                  g_BattleSpriteColumnWork->primStart[12]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk5640, g_BattleSpriteColumnWork->packageCount[17],
                  g_BattleSpriteColumnWork->primStart[13]);
    BattleAddPrimListToOT(g_BattleSpriteColumnWork->unk5C80, g_BattleSpriteColumnWork->packageCount[18],
                  g_BattleSpriteColumnWork->primStart[14]);
}

void BattleAddPrimAndRenderObject(void) {
    BattlePrimPool *pPool;
    s32 off;

    if (g_BattleDisplayState->packagePending != 0) {
        pPool = g_BattlePrimPool;
        // the cast over the sum keeps retail's product-first addu; &pPool->prims[i] is base-first (99.74%)
        off = pPool->primIndex * sizeof(POLY_FT4) + 0x7A4;
        AddPrim(g_BattleCurOT[0] + 4, (u8 *)(off + (s32)pPool));
    }

    if (g_BattleDisplayState->objectRenderPending != 0) {
        func_80034888(g_BattleRenderString, g_BattleCurOT[0] + 4, g_BattlePrimStartIndex);
    }
}

/* History, still true: the body shapes that took this from 57 to 85.91 were (1) `off` as an
   accumulator local, NOT an inline `i * 0x60` (the inline form makes loop.c build an address biv
   per use site: four accumulators, 376 bytes, 57.11); (2) x as its own `s32` local assigned per
   arm, which is what keeps retail's duplicated `subu a1,a1,v0` in each arm rather than one shared
   copy at the join; (3) the 13*(i/2) value carried in a `u8 v` through an embedded assignment at
   the argument (permuter find, 79.52 -> 85.91). A 5-minute permuter run from that body returned
   only candidates that reuse `off` for x (wrong code, rejected).
*/
// 89.74% (this round; the old park measured 87.75 here, its 85.91 note was stale), 352 bytes / 88 insns against retail's 344 / 87.
// Measured this round: (1) the 0x30 frame and the s3 slot came back with 'u8 **pOT = g_BattleCurOT' live across the loop (pOT[0] and ((s32*)pOT)[12] = g_BattlePrimStartIndex); (2) the i==0 arm's two flags loads form their address with a bare 'lui v0' ONLY when the guard is written 'if (off == 0)' - with 'if (i == 0)' gcc cannot fold off to 0 and both loads pay an 'addu at,at,s0' (88.07 else).
// Residual, exactly: (a) the i==0 arm's 'andi a1,a1,0xffff' (the u16 conversion of x, which retail keeps ONCE at the join) is duplicated into that arm's jal delay slot, so its delay slot holds andi where retail holds 'addu a0,a0,s2' - one instruction and a placement; (b) the two 'addu a0,s0,a0' / 'addu a1,s0,a1' (offset first) where this cc1 emits base-first; (c) the prologue save order s1,s2,s3,s0,ra vs s3,s1,s0,s2,ra.
// Tried and worse, do not repeat: writing the sum offset-first ('off + (idx*40 + base)', the spelling that WOULD give the operand order) makes loop.c build a second address biv for the sum (s4 + its own 'addiu s2,s2,96', 87.01 / 360 / 90); 'off == 0' plus that spelling 87.01; assigning pOT after 'off = 0' instead of before it re-forms the else arm's base as 'addu v1,s0,s2' (84.06). Also measured: inline 'i * 0x60' for the flags index (creates a biv), g_BattleTextSlotFlags[0] literal (gcc hoists the address into s3, 83.48). The double-negation lever does not apply - off is a strength-reduced giv.
/*
void func_8007500C(void) {
    s32 i;
    s32 off;
    s32 x;
    u8 v;
    u8 **pOT;

    pOT = g_BattleCurOT;
    i = 0;
    off = 0;
    do {
        if (g_BattleTextSlotVisible[off] != 0) {
            if (off == 0) {
                x = 0x40 - (g_BattleTextSlotFlags[off] >> 1);
                BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattlePrimStartIndex * 40 + ((s32)g_BattlePrimBuffer + off)),
                                        x, 0x2C, 0, 0, g_BattleTextSlotFlags[off]);
            } else {
                x = 0x9A - (g_BattleTextSlotFlags[off] >> 1);
                BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattlePrimStartIndex * 40 + ((s32)g_BattlePrimBuffer + off)),
                                        x, 0xCA, 0,
                                        v = (13 * (i / 2)) & 0xFF, g_BattleTextSlotFlags[off]);
            }
            AddPrim(pOT[0] + 4,
                    (u8 *)(((s32 *)pOT)[12] * 40 + ((s32)g_BattlePrimBuffer + off)));
        }
        i++;
        off += 0x60;
    } while (i < 8);
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_8007500C);



void BattleBuildSlotGaugePrims(void) {
    s32 slot;
    u32 value;
    u32 full;
    u32 width;
    u8 *pGraphics;
    u8 *pSlotGraphics;

    for (slot = 0; slot < 3; slot++) {
        pGraphics = (u8 *)g_BattleGraphicsWork;
        pSlotGraphics = pGraphics + slot * 0x1E4;
        // 0x853D is the slot's (0x1E4 stride) status byte in the graphics work block.
        switch (pSlotGraphics[0x853D]) {
        case 1:
            // a value bar: two pixels per unit of the slot's display value, drawn green
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].x0 = slot * 0x60 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x28);
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].y0 = 0x22;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].x1 = slot * 0x60 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x28) + g_BattleSlotDisplayValue[slot].value * 2;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].y1 = 0x22;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].x2 = slot * 0x60 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x28);
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].y2 = 0x26;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].x3 = slot * 0x60 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x28) + g_BattleSlotDisplayValue[slot].value * 2;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].y3 = 0x26;
            *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].r0 = 0;
            *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].g0 = 0xFF;
            *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].b0 = 0;
            ((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].r1 = 0;
            *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].g1 = 0xFF;
            ((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].b1 = 0;
            break;
        case 2:
            value = D_800CCDC4[slot].flags;
            full = D_800CCDC6[slot].flags;
            // the gauge is 56 pixels wide at 100 percent
            width = value * 100 / full * 5600 / 10000;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].x0 = slot * 0x60 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x28);
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].y0 = 0x22;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].x1 = slot * 0x60 + 0x28 + g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + width;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].y1 = 0x22;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].x2 = slot * 0x60 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x28);
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].y2 = 0x26;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].x3 = slot * 0x60 + 0x28 + g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + width;
            *(volatile s16 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].y3 = 0x26;
            if (value >= full / 4) {
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].r0 = 0;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].g0 = 0;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].b0 = 0xFF;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].r1 = 0;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].g1 = 0;
                ((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].b1 = 0xFF;
            } else if (value >= full / 8) {
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].r0 = 0xFF;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].g0 = 0xFF;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].b0 = 0;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].r1 = 0xFF;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].g1 = 0xFF;
                ((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].b1 = 0;
            } else {
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].r0 = 0xFF;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].g0 = 0x7F;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].b0 = 0x7F;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].r1 = 0xFF;
                *(volatile u8 *)&((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].g1 = 0x7F;
                ((POLY_G4 *)((u32)g_BattleGraphicsWork + 0x740))[slot * 2 + g_BattlePrimStartIndexVolatile].b1 = 0x7F;
            }
            break;
        }
    }
}

// 92.72% at 2780 bytes / 695 insns against retail 696. Structurally COMPLETE: filtering the 201 differing diff lines for changed OPCODES leaves four (a delay-slot sll vs li+nop, one sll a1,t1,2 vs nop, and a hoisted li a1,0x14 that retail reloads), everything else is register naming. Residual: cc1 2.6.0 does not form retail biv for g_BattleSlotActorIds[slot * 0x1C] (this body emits sll/subu/sll from t1 every pass where retail walks t6 += 0x1C) and rotates the caller-saved set (the width array base is t0 in the panelMode==0 arm and a freshly computed a2 in the else arm, so the source took &width[slot] twice; the texture byte is in a0 and the width word in v0, this body has them swapped).
// LOAD-BEARING SPELLINGS: every prim store is a volatile lvalue and the index is read through g_BattlePrimStartIndexVolatile (same lever as BattleBuildSlotGaugePrims; without them the body collapses); the two quads are POLY_GT4s at gfx+0x5A0 indexed slot*2+primIndex, the panel quad at 6+primIndex; x0/x2 must be spelled origin + slot*0x60 + 0x2D, NOT slot-first (addu operand order, 90.61 with slot-first); the width term of x1/x3 wants (u16)width[slot] + 1, which keeps retail lhu+addiu.
// Tried: slot-first x spelling 90.61, (u16) width term 92.72. Next lever: permuter - structure is right, only allocation and four opcodes differ.
// 94.12% at 2780 bytes / 695 insns against retail 696 - ONE instruction SHORT, so it cannot link.
// WHAT IT TOOK THIS ROUND (92.72 -> 94.12, 201 -> 159 differing lines):
// (1) the actor-id byte offset needs its OWN for-clause IV: `for (slot = 0, actorOff = 0; slot < 3;
//     slot++, actorOff += 0x1C, ...)` with `g_BattleSlotActorIds[actorOff]`. cc1 2.6.0 will not
//     strength-reduce `slot * 0x1C` (it derives 28i as 4*(8i-i) from t1 inline - 3 insns) but it DOES
//     reduce an explicit IV, giving retail's `addiu t6,t6,0x1C` at the loop bottom. 92.72 -> 93.40.
//     Same shape and same reason as battle/main/misc1's parked func_80073538.
// (2) arm B's tail needs a `BattleWork *pState = g_BattleDisplayState;` local assigned BEFORE the
//     clut store and read after it (`if (pState->primListsPending != 0) pState->unk82 = 1;`). That is
//     what keeps the pointer in $a1 across the volatile clut store, as retail does; with two separate
//     `g_BattleDisplayState->` reads the load is emitted after the store with a nop in front of it.
//     93.40 -> 94.12, 181 -> 159 lines, and it also restored retail's prologue exactly (ra,s2,s1,s0
//     saved at 0x2C/0x28/0x24/0x20 and the $s2 slot for the constant 0xC).
// LOAD-BEARING SPELLINGS (all measured): every prim store is a volatile lvalue and the prim index is
// read through g_BattlePrimStartIndexVolatile (same lever as BattleBuildSlotGaugePrims; without them
// the body collapses); the two quads are POLY_GT4s at gfx+0x5A0 indexed slot*2+primIndex, the panel
// quad at 6+primIndex; x0/x2 must be spelled origin + slot*0x60 + 0x2D, NOT slot-first (addu operand
// order, 90.61 with slot-first); the width term of x1/x3 wants (u16)width[slot] + 1, which keeps
// retail's lhu+addiu.
// THE REMAINING ONE INSTRUCTION is the preheader: retail emits `lui v0,%hi(g_BattleSlotCountdownTimers)`
// / `addiu v0,v0,%lo` / `move t4,v0` (3 insns, the address pseudo copied into pTimer) where this body
// emits `lui t4` / `addiu t4` (2). Tried dead this round: `pLimit = g_BattleSlotCountdownTimers - 0xB;`
// in place of `pTimer - 0xB` (byte-identical). Also still off: retail does NOT hoist the 0x14 of the
// panel quad (it re-materialises `li a1,0x14` once per iteration in arm B) while this body keeps it in
// $t9, so retail's constant set is $s0=0x1A,$s1=0x1E,$s2=0xC,$t9=0xCE,$t8=the 0x68DB8BAD magic and
// this body's is $s0,$s1,$s2,$t8=0xCE,$t9=0x14,$t5=magic; and retail schedules the width/poly-index
// loads BELOW the mult/mfhi chain where this body fills that hole with them, and re-forms
// `&width[slot]` twice (t0 in the panelMode==0 arm, a freshly shifted a2 in the else arm).
// Next lever: permuter from this body - the structure is right (695 vs 696, same calls and branches),
// what is left is the constant set and that one preheader copy.
/*
/*
void func_80075938(void) {
    s32 slot;
    s32 actorOff;
    s32 width[4];
    BattleWork *pState;
    u16 clut;
    s16 *pTimer;
    s16 *pLimit;

    BattleBuildSlotGaugePrims();
    g_BattleDisplayState->unk82 = 0;
    pTimer = g_BattleSlotCountdownTimers;
    pLimit = pTimer - 0xB;
    for (slot = 0, actorOff = 0; slot < 3; slot++, actorOff += 0x1C, pTimer++, pLimit++) {
        if (g_BattleSlotActorIds[actorOff] != BATTLE_SLOT_CHARACTER_NONE) {
            if (width[slot] < 0) {
                width[slot] = 0;
            }
            if (g_BattleDisplayState->panelMode[slot] == 0) {
                width[slot] = (100 - (*pTimer * 100) / *pLimit) * 5600 / 10000;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].x0 =
                    g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + slot * 0x60 + 0x2D;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].y0 = 0x1A;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].x1 =
                    slot * 0x60 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x2C) + ((u16)width[slot] + 1);
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].y1 = 0x1A;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].x2 =
                    g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + slot * 0x60 + 0x2D;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].y2 = 0x1E;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].x3 =
                    slot * 0x60 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x2C) + ((u16)width[slot] + 1);
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].y3 = 0x1E;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].u0 =
                    (((u8 *)g_BattleGraphicsWork)[0xA244] & 0x3F) * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].v0 =
                    ((u8 *)g_BattleGraphicsWork)[0xA248];
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].u1 =
                    (width[slot] + (((u8 *)g_BattleGraphicsWork)[0xA244] & 0x3F)) * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].v1 =
                    ((u8 *)g_BattleGraphicsWork)[0xA248];
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].u2 =
                    (((u8 *)g_BattleGraphicsWork)[0xA244] & 0x3F) * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].v2 =
                    ((u8 *)g_BattleGraphicsWork)[0xA248] + 4;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].u3 =
                    (width[slot] + (((u8 *)g_BattleGraphicsWork)[0xA244] & 0x3F)) * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].v3 =
                    ((u8 *)g_BattleGraphicsWork)[0xA248] + 4;
                if (((D_800CCD6C[slot * 0xB8] | D_800CCD6E[slot * 0xB8]) & 0x8000) != 0) {
                    clut = ((u16 *)(g_BattleGraphicsWork + 0xA2AC))[3];
                } else if ((g_BattleSlotEntries[slot * 0xB8] & 0x1000) != 0) {
                    clut = ((u16 *)(g_BattleGraphicsWork + 0xA2AC))[2];
                } else {
                    clut = ((u16 *)(g_BattleGraphicsWork + 0xA2AC))[0];
                }
                *(volatile u16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [slot * 2 + g_BattlePrimStartIndexVolatile].clut = clut;
                g_BattleDisplayState->panelActive[slot] = 1;
            } else {
                width[slot] = (100 - *pTimer) * 5600 / 10000;
                if (g_BattleSlotFlags[slot * 8] != 0 && g_BattleActorCharIds[slot] != 7) {
                    width[slot] = (D_800CCDC4[slot].flags * 100 / D_800CCDC6[slot].flags) * 5600 / 10000;
                }
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].x0 = 0xC;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].y0 = 0xCE;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].x1 = 0xC;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].y1 = 0xCE - width[slot] * 2;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].x2 = 0x14;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].y2 = 0xCE;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].x3 = 0x14;
                *(volatile s16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].y3 = 0xCE - width[slot] * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].u0 =
                    (((u8 *)g_BattleGraphicsWork)[0xA244] & 0x3F) * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].v0 =
                    ((u8 *)g_BattleGraphicsWork)[0xA248];
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].u1 =
                    (width[slot] + (((u8 *)g_BattleGraphicsWork)[0xA244] & 0x3F)) * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].v1 =
                    ((u8 *)g_BattleGraphicsWork)[0xA248];
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].u2 =
                    (((u8 *)g_BattleGraphicsWork)[0xA244] & 0x3F) * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].v2 =
                    ((u8 *)g_BattleGraphicsWork)[0xA248] + 4;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].u3 =
                    (width[slot] + (((u8 *)g_BattleGraphicsWork)[0xA244] & 0x3F)) * 2;
                *(volatile u8 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].v3 =
                    ((u8 *)g_BattleGraphicsWork)[0xA248] + 4;
                pState = g_BattleDisplayState;
                *(volatile u16 *)&((POLY_GT4 *)((u32)g_BattleGraphicsWork + 0x5A0))
                     [6 + g_BattlePrimStartIndexVolatile].clut =
                    ((u16 *)(g_BattleGraphicsWork + 0xA2AC))[1];
                if (pState->primListsPending != 0) {
                    pState->unk82 = 1;
                }
            }
        } else {
            g_BattleDisplayState->panelActive[slot] = 0;
        }
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075938);

void BattleDrawFrameScreenMode1(void) {
    if (g_BattleFrameDrawSuppressed == 0) {
        BattleExpandSlotRects();
        BattleRenderSlotStatDigits();
        func_80075938();
        func_80073538();
        BattleAddScrollRowPrims();
        func_80073B64();
        BattleAddTwoPrimLists();
        BattleAddThreeFlaggedPrimLists();
        func_8007500C();
        BattleAddThreePrimLists();
        BattleAddDisplayStatePrims();
        BattleUpdateRotationBuffer();
        BattleAddPanelAndSlotPrimLists();
        func_80088B80();
        BattleAddSpriteColumnPrimLists();
    }
}


void func_800764B4(void) {
    g_DebugPageIndex = 0;
    BattleExpandSlotRects();
    func_801DE594();
    BattleAddPanelAndSlotPrimLists();
}

void BattleDrawFrameScreenMode2(void) {
    BattleExpandSlotRects();
    func_80073538();
    BattleAddThreeFlaggedPrimLists();
    func_8007500C();
    BattleAddPrimAndRenderObject();
    BattleAddPanelAndSlotPrimLists();
    func_80088B80();
    BattleAddSpriteColumnPrimLists();
}

void BattleDrawFrameByScreenMode(void) {
    switch (g_BattleDisplayMode) {
    case 0:
        func_800764B4();
        break;
    case 1:
        BattleDrawFrameScreenMode1();
        break;
    case 2:
        BattleDrawFrameScreenMode2();
        break;
    }
}

void BattleInitPanelMove(s32 slot) {
    BattleWork *pWork;
    u16 *pOrigin;
    mem_addr origins;
    s32 index;
    s32 base;
    s32 offset;
    s32 offset2;
    s32 height;
    u32 divisor;
    s32 scaled;

    index = g_BattleSlotPageIndex * 3 + slot;
    origins = (mem_addr)g_BattleSlotScreenOrigins;
    pOrigin = (u16 *)(index * 2 + origins);
    base = *pOrigin;
    g_BattleDisplayState->y = 0x1C;
    offset = base + 0x48;
    g_BattleDisplayState->x = slot * 0x60 + offset;
    if (g_BattleSlotFlags[slot * 8] != 0 && g_BattleActorCharIds[slot] != 7) {
        offset2 = *pOrigin + 0x44;
        g_BattleDisplayState->y = 0x24;
        g_BattleDisplayState->x = slot * 0x60 + offset2;
    }

    g_BattleDisplayState->xTarget = 0x10;
    g_BattleDisplayState->yTarget = 0x98;
    pWork = g_BattleDisplayState;
    height = pWork->x - 5;
    pWork->xStep = height - pWork->xTarget;
    divisor = pWork->y + 5;
    pWork->yStep = pWork->yTarget - divisor;
    divisor = pWork->yStep;
    scaled = (pWork->xStep << 8) / divisor;
    pWork->panelScale = 0x800;
    g_BattleDisplayState->moveSteps = 6;
    pWork->yStep = 0x100;
    do { // a unit so the cluster keeps retail's place after the 0x5C pair
        pWork->xOffset = 0;
        pWork->yOffset = 0;
        pWork->panelAngle = 0;
    } while (0);
    pWork->xStep = scaled;
    g_BattleDisplayState->sizeSteps = 1;
    g_BattleDisplayState->panelMode[slot]--;
}

/*
97.94% at retail's exact 728 bytes (the body compiles to 724 - ONE instruction short).
What it took, in order: (1) `extern s32 g_BattlePrimStartIndexTable[]
__asm__("g_BattlePrimStartIndex");` plus `pCtx = g_BattlePrimStartIndexTable;` is
what makes cc1 2.6.0 hold that address in $s0 across both func_80025FA8 calls
(retail: `lui s0,%hi` / `addiu s0,%lo`, then `lw a3,0(s0)` twice). A scalar extern,
a pointer local, `&g_...` through mem_addr and a `*(s32 *)` cast all rematerialise
`lui/lw` per call site (95.90 for the whole body). The array type has to be a
SEPARATE declaration with that asm label: retyping the shared scalar breaks the
other functions in this file. This bought 95.90 -> 97.94; add the declaration back
before un-parking.
Residual is exactly two coupled reorg decisions, and both are the lever
[g-rotated-entry-j-to-bottom-test]: retail evaluates the loop condition in the
PREHEADER and `j`s straight to the SINGLE bottom test, so `lbu v0,0x77(state)` /
`j .L80076908` / `sll v0,v0,1` (count*2 in the j delay slot) is the preheader's own
evaluation, and the bottom test body is `lbu / addiu s0,s0,2 / sll v0,v0,1 / slt /
bnez` with `ori a1,0x1` (SetSemiTrans' flag, re-materialised per iteration) in the
bnez delay slot. Every loop spelling gives the ROTATED form instead - the entry's
own inline branch, ONE count*2 (evaluated at the bottom only, which is the missing
4 bytes), the j init `sll s0,s0,1` parked in the j delay slot, and `li a1,0x1`
HOISTED into the preheader (where retail has `nop`). That lever records the shape as
unreachable from any spelling under gcc 2.6.0, 2.7.2 and 2.7.2-cdk (round 398,
battle/main/misc1 func_800723E0, probed with cc1_probe.sh), so the two residuals are
one blocker, not two:
(a) retail has the j init `sll s0,s0,1` BEFORE the second call (inside the arg
setup, right after the second `lhu v0,0x104(v1)`) and `sll v0,v0,1` (the entry
test's count*2) in the `j` delay slot; this body puts the j init in that delay slot
and shares ONE sll between the entry test and the loop bottom, hence 4 bytes short.
Writing `j = idx * 2;` as its own statement before the call puts the sll where
retail has it but takes $s0 earlier, pushes the state pointer into $t0 and scores
95.63 - retail's placement needs the insn at the END of the argument setup, which
no statement order reaches (no scheduler in this unit's preset).
(b) `li a1,0x1` (SetSemiTrans' flag) is re-materialised in the loop back-edge delay
slot in retail; this body hoists it above the loop (the same 4 bytes), which also
frees $a1 and makes the later sizeSteps counter $a1 instead of retail's $a3.
(b) NOTE the third residual, the 2nd call's arg setup: retail emits `sll a2,a2,0x4`
right after `addu a2,a2,s0` and `addiu a2,a2,14184` two slots later, with `sll
s0,s0,0x1` in the slot this body uses for `addiu a2,a2,14184`. Same insn count,
just interleaved - do not chase it before (a)/(b), which are 4 bytes short.
Also tried and measured: `mem_addr primStart = (mem_addr)&g_BattlePrimStartIndex;`
plus `*(s32 *)primStart` (95.90, identical - it folds), and `off` inlined into the
call (identical 97.94 / 724). Two earlier sessions ran 4-minute permuter runs from
91.53 and from this body; both were worthless under objdiff.
   The do/while(0) unit around the two package calls and the 0x104/0x106 updates
   is load-bearing too (permuter find, worth 2 points).
*/
// Re-measured this round after unpark: 97.94%, 724 bytes / 181 insns against retail's 728 / 182.
// The single missing instruction is retail's SECOND count*2 shift. Retail computes 'j = idx * 2' in
// the 2nd func_80025FA8 argument setup (sll s0,s0,1 before the jal) and then emits count*2 twice:
// once in the 'j .L80076908' delay slot at loop entry and once at the loop bottom (sll v0,v0,1
// before slt). This body emits the j init in the 'j' delay slot and the entry jumps INTO the
// bottom's sll, so the two count*2 computations CSE into one: 4 bytes short, and the second
// residual (retail's li a1,1 in the loop-back delay slot vs this body's hoisted copy) is the
// same 4 bytes. Recorded unreachable from any spelling under gcc 2.6.0 / 2.7.2 / 2.7.2-cdk
// (round 398 probe).
/*
void func_80076710(s32 slot) {
    u16 v[8];
    s32 i;
    s32 j;
    s32 k;
    s32 idx;
    s32 off;
    s32 off2;
    s32 *pCtx;

    for (i = 0; i < g_BattleDisplayState->moveSteps; i++) {
        *(s32 *)((u8 *)g_BattleDisplayState + 0x64) -= *(s32 *)((u8 *)g_BattleDisplayState + 0x54);
        *(s32 *)((u8 *)g_BattleDisplayState + 0x6C) += *(s32 *)((u8 *)g_BattleDisplayState + 0x5C);
        v[0] = (*(u32 *)((u8 *)g_BattleDisplayState + 0x64) >> 8) + *(u16 *)((u8 *)g_BattleDisplayState + 0x34);
        v[4] = (*(u32 *)((u8 *)g_BattleDisplayState + 0x6C) >> 8) + *(u16 *)((u8 *)g_BattleDisplayState + 0x44);
    }

    g_BattleDisplayState->moveSteps = 0;
    // a statement unit: this block is scheduled as one, which is what keeps retail's
    // ordering of the two package calls and the 0x104/0x106 updates.
    do {
    pCtx = g_BattlePrimStartIndexTable;
    *(u8 *)((u8 *)g_BattleDisplayState + 0x77) = 0;

    *(u8 *)((u8 *)g_BattleDisplayState + 0x77) =
        func_80025FA8(g_BattleSpriteArchive, 0x52, g_BattleGraphicsWork + 0x3768, *pCtx,
                      v[0], v[4], *(u16 *)((u8 *)g_BattleDisplayState + 0x104),
                      *(u16 *)((u8 *)g_BattleDisplayState + 0x104),
                      *(u16 *)((u8 *)g_BattleDisplayState + 0x106));

    idx = *(u8 *)((u8 *)g_BattleDisplayState + 0x77);
    off = idx * 0x50 + 0x3768;
    *(u8 *)((u8 *)g_BattleDisplayState + 0x77) +=
        func_80025FA8(g_BattleSpriteArchive, 0x53, off + (s32)g_BattleGraphicsWork, *pCtx,
                      v[0], v[4], *(u16 *)((u8 *)g_BattleDisplayState + 0x104),
                      *(u16 *)((u8 *)g_BattleDisplayState + 0x104),
                      *(u16 *)((u8 *)g_BattleDisplayState + 0x106));

    for (j = idx * 2; j < *(u8 *)((u8 *)g_BattleDisplayState + 0x77) * 2; j += 2) {
        off2 = (j + g_BattlePrimStartIndex) * 40 + 0x3768;
        SetSemiTrans((u8 *)(off2 + (s32)g_BattleGraphicsWork), 1);
    }

    *(u8 *)((u8 *)g_BattleDisplayState + 0x87) = *(u8 *)&g_BattlePrimStartIndex;

    for (k = 0; k < g_BattleDisplayState->sizeSteps; k++) {
        *(u16 *)((u8 *)g_BattleDisplayState + 0x104) += 0x66;
        *(u16 *)((u8 *)g_BattleDisplayState + 0x106) += 0x80;
    }
    g_BattleDisplayState->sizeSteps = 0;

    if (v[0] <= *(u32 *)((u8 *)g_BattleDisplayState + 0x3C)) {
        *(u8 *)((u8 *)g_BattleDisplayState + slot + 0x90) = 1;
    }
    } while (0);
    if (v[4] >= *(u32 *)((u8 *)g_BattleDisplayState + 0x4C)) {
        *(u8 *)((u8 *)g_BattleDisplayState + slot + 0x90) = 1;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076710);
void BattleLoadImageAndSync(RECT *pRect, u_long *pData) {
    LoadImage(pRect, pData);
    DrawSync(0);
}


void BattleDrawSpritePackage(s32 arg0, s32 arg1, s16 arg2, s16 arg3) {
    func_8002675C(g_BattleSpriteArchive, arg0, arg1, g_BattlePrimStartIndex, arg2, arg3, 0x1000);
}

void BattleDrawSpritePackageHalfScale(s32 arg0, s32 arg1, s16 arg2, s16 arg3) {
    func_8002675C(g_BattleSpriteArchive, arg0, arg1, g_BattlePrimStartIndex, arg2, arg3, 0x800);
}

