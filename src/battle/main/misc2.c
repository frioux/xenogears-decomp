#include "common.h"
#include "psyq/libgpu.h"
#include "battle/battle_work.h"

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
    /* 0x66B */ u8 unk66B[5];
} BattlePacketPrims; // Size: 0x670

// Partial view of the sprite column draw buffer g_BattleSpriteColumnWork points at
// (0x5DA4 bytes, allocated by BattleAllocDrawBuffer): the fields of the 0x4CE0 list
// and the counter that gates it.
typedef struct {
    /* 0x0000 */ u8 unk0[0x5DA0];
    /* 0x5DA0 */ u8 listPrimStart;    // prim index the 0x4CE0 list was built at
    /* 0x5DA1 */ u8 listPackageCount; // packages that list holds
    /* 0x5DA2 */ u16 columnCounter;   // frames the 0x4CE0 list has been drawn for
} BattleSpriteColumnWork; // Size: 0x5DA4

extern u8 D_8005959C;
extern u8 *g_BattleGraphicsWork;
extern u8 g_BattleDisplayMode;
extern u8 g_BattleFrameDrawSuppressed;
extern u8 *g_BattleCurOT[];
extern s32 g_BattleRenderString;
extern BattlePrimPool *g_BattlePrimPool;
extern s32 g_BattlePrimStartIndex;
extern void *g_BattleSpriteArchive;
extern BattleWork *g_BattleDisplayState;
extern BattlePulseWork *g_BattlePulseTrianglePrims;
extern u16 g_BattleSlotScreenOrigins[];
extern u8 g_BattleSlotPageIndex;
extern u8 g_BattleActorCharIds[];
extern u8 *g_BattleSpriteColumnWork;
extern u8 g_BattleSlotActorIds[];
extern u8 g_BattleSlotFlags[];
extern BattleSlotGraphics *g_BattleSlotGraphics[];

extern u8 g_BattlePrimBuffer[];
extern volatile u8 D_800D3725[];
extern volatile u8 g_BattleTextSlotFlags[];

extern s32 func_8002675C(void *pDir, s32 index, void *pPrims, s32 context, s16 x, s16 y, s16 scale);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *p, u16 x, s16 y, u8 u, u8 v, u8 dx);
extern void func_800728B8(u8 *pDst, s32 arg1, s32 arg2);
extern void BattleRenderActorSplitDigitPanel(s32 index, u8 arg1);
extern void BattleRenderActorStatDigits(s32 index, u8 arg1);
extern s32 func_80072F38(s32 index, u8 arg1);
extern void BattleRenderActorValueDigits(s32 index);
extern s32 func_80025FA8(void *pDir, s32 id, u8 *pPrim, s32 context, u16 x, u16 y, u16 u, u16 v, u16 w);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073B64);
/* 86.09%: all five call loops are recovered and their bodies match
   instruction-for-instruction - (1) `for (i = 0; i < g_BattleDisplayState[0x97]; i++)`
   AddPrim of a 0x10-stride prim at g_BattleGraphicsWork + 0x908 indexed by
   g_BattleDisplayState[0x98] + i*2; (2) four func_800728B8 at g_BattleGraphicsWork + 0x2E08 +
   i*0x320 with g_BattleDisplayState[i+0x74]/[i+0x84]; (3) three entries at 0x1E4 stride
   with a `switch` on the byte at base+0x852D (case 1 -> atlas + i*0x1E4 +
   0x835C with bytes +0x852E/+0x852C, case 2 -> the same pointer + 0xA0 with
   +0x852F/+0x852C; the code below folds case 2 into +0x83FC, but retail
   adds the 0xA0 to the case-1 pointer LAST, so `pAtlas + 0xA0` is the shape to
   try)
   plus the 0xEC/[0x99] draw at 0x6008 + i*0x140; (4) the optional 0xAF draw at
   0x9C8 and three draws at 0x3A88 + i*0xC80; (5) three entries at 0x1C/0x50/
   0x1E0 strides, skipping entries whose g_BattleSlotActorIds[i*0x1C] is 0x7F, and the
   AddPrim whose index is `(i * 2 + ctx) * 36` where ctx is g_BattlePrimStartIndex reached
   as `&g_BattleCurOT + 0x30` (that is how retail gets it here:
   `lw v1,0x30(s4)` off the hoisted `la g_BattleCurOT`). Remaining differences are
   all allocation: retail keeps &g_BattleCurOT in $s4 (saving it in the prologue;
   we rematerialise `lui/lw` at each AddPrim and lose the s4 slot), the
   `off + (s32)g_BattleGraphicsWork` adds come out base-first here and offset-first in
   retail (`addu a0,a1,v0` vs `addu a0,v0,a1`, and the AddPrim prim argument
   likewise), and loop 3's four big-displacement byte loads make gcc hoist
   `0x852C` into a register (`li s1,0x852C`) instead of keeping retail's
   base+off3 register with 0x852C..0x852F displacements. Two `u16 v[8]`-style
   tricks were not needed: this frame (0x30, s0-s4+ra) matches as written. */
/*
void func_80073B64(void) {
    s32 i;
    s32 off;
    s32 off2;
    s32 off3;
    u8 *p;

    for (i = 0; i < g_BattleDisplayState[0x97]; i++) {
        AddPrim(g_BattleCurOT[0] + 4,
                (u8 *)(g_BattleGraphicsWork + 0x908 + (g_BattleDisplayState[0x98] + i * 2) * 0x10));
    }

    for (i = 0, off = 0x2E08; i < 4; i++, off += 0x320) {
        func_800728B8((u8 *)(g_BattleGraphicsWork + off),
                      *(u8 *)(g_BattleDisplayState + i + 0x74),
                      *(u8 *)(g_BattleDisplayState + i + 0x84));
    }

    for (i = 0, off = 0x6008, off3 = 0; i < 3; i++, off += 0x140, off3 += 0x1E4) {
        if (*(u8 *)(g_BattleDisplayState + i + 0xCC) != 0) {
            p = (u8 *)(g_BattleGraphicsWork + off3);
            switch (*(u8 *)(p + 0x852D)) {
            case 1:
                func_800728B8((u8 *)(g_BattleGraphicsWork + off3 + 0x835C),
                              *(u8 *)(p + 0x852E), *(u8 *)(p + 0x852C));
                break;
            case 2:
                func_800728B8((u8 *)(g_BattleGraphicsWork + off3 + 0x83FC),
                              *(u8 *)(p + 0x852F), *(u8 *)(p + 0x852C));
                break;
            }
            func_800728B8((u8 *)(g_BattleGraphicsWork + off),
                          *(s32 *)(g_BattleDisplayState + i * 4 + 0xEC),
                          *(u8 *)(g_BattleDisplayState + i + 0x99));
        }
    }

    if (g_BattleDisplayState[0xAF] != 0) {
        func_800728B8(g_BattleGraphicsWork + 0x9C8, g_BattleDisplayState[0x7B], g_BattleDisplayState[0xA4]);
    }

    for (i = 0, off = 0x3A88; i < 3; i++, off += 0xC80) {
        func_800728B8((u8 *)(g_BattleGraphicsWork + off),
                      *(s32 *)(g_BattleDisplayState + i * 4 + 0xE0),
                      *(u8 *)(g_BattleDisplayState + i + 0x93));
    }

    for (i = 0, off = 0x818, off2 = 0, off3 = 0; i < 3;
         i++, off += 0x50, off2 += 0x1C, off3 += 0x1E0) {
        if ((&g_BattleSlotActorIds)[off2] != 0x7F) {
            func_800728B8((u8 *)(g_BattleGraphicsWork + off), 1, g_BattleDisplayState[0x83]);
            if (*(u8 *)(g_BattleDisplayState + i + 0xCC) != 0) {
                func_800728B8((u8 *)(g_BattleGraphicsWork + off3),
                              *(u8 *)(g_BattleDisplayState + i + 0x78), g_BattleDisplayState[0xA2]);
                AddPrim(g_BattleCurOT[0] + 4,
                        (u8 *)(g_BattleGraphicsWork + 0x740 +
                               (i * 2 + ((s32 *)&g_BattleCurOT)[0xC]) * 36));
            }
        }
    }
}
 */

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
        func_800728B8((u8 *)(offset + (s32)g_BattleGraphicsWork), pWork->packageCounts[i], pWork->primStart);
        offset += 0xFA0;
        i++;
    } while (i < 2);
}

void BattleAddThreeFlaggedPrimLists(void) {
    if (g_BattleDisplayState->spriteColumnPending != 0) {
        func_800728B8(g_BattleGraphicsWork + 0xBA8, g_BattleDisplayState->spriteColumnPackageCount, g_BattleDisplayState->spriteColumnPrimStart);
    }

    if (g_BattleDisplayState->cursorPending != 0) {
        func_800728B8(g_BattleGraphicsWork + 0x27C8, g_BattleDisplayState->cursorPackageCount, g_BattleDisplayState->cursorPrimStart);
    }

    if (g_BattleDisplayState->menuColumnPending != 0) {
        func_800728B8(g_BattleGraphicsWork + 0x1E68, g_BattleDisplayState->menuColumnPackageCount, g_BattleDisplayState->menuColumnPrimStart);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073FB8);
/* 95.80% at retail's exact 186 instructions (744 bytes), frame 0x20 / s0-s2+ra
   exact. All twelve AddPrim sites and the two g_BattlePrimStartIndex chains are
   recovered; the 8 prim lists are called in retail's source order, which is
   0x280, 0x2D0, 0x320, 0x370 first and then 0x140, 0x190, 0x1E0, 0x230 - not
   ascending. What is left is two instruction-shape ties:
   (1) retail emits `addiu a1,a1,0x...` with a1 = count*40 (constant added to the
   PRODUCT) then `addu a1,s0,a1` (base last) in the jal delay slot; every
   spelling of `count * 40 + CONST + (s32)pSlot` is reassociated by gcc into
   `count*40 + (CONST + pSlot)` (`addiu v0,s0,CONST` + `addu a1,a1,v0`), and so
   is `(s32)pSlot + (count * 40 + CONST)` (86.90%). Giving the sum its own
   `s32 off` local does produce retail's `addiu a1,a1,CONST`, but gcc then hoists
   the `lbu 0x5A4(s0)` out of the call sequence and it drops to 80.72.
   (2) the two `g_BattlePrimStartIndex * 12` chains differ only in which
   registers gcc picks ($v0/$v1 swapped) and in the final `addu` operand order
   (`addu a1,a1,v1` against `addu a1,v0,a1`).
   Note the last two AddPrim offsets are `count * sizeof(POLY_G4)` (36) at 0x3C0
   and `count * sizeof(DR_MODE)` (12) at 0x408, which is what the BattleSlotGraphics
   header's prims[2]/drawModes[2] fields lay out.
*/
/*
void func_80073FB8(void) {
    BattleSlotGraphics *pSlot;
    s32 i;

    AddPrim(g_BattleCurOT[0] + 4,
            (u8 *)(g_BattlePrimStartIndex * 12 + 0x8920 + (s32)g_BattleGraphicsWork));

    for (i = 0; i < 4; i++) {
        if (*(u8 *)((u8 *)g_BattleDisplayState + i + 0x7F) != 0 && g_BattleDisplayMode == 1) {
            AddPrim(g_BattleCurOT[0] + 4,
                    (u8 *)((i * 2 + *(u8 *)((u8 *)g_BattleDisplayState + i + 0x84)) * 52 + 0x5A0 +
                           (s32)g_BattleGraphicsWork));
        }
    }

    AddPrim(g_BattleCurOT[0] + 4,
            (u8 *)(g_BattlePrimStartIndex * 12 + 0x8908 + (s32)g_BattleGraphicsWork));

    for (i = 0; i < 7; i++) {
        if (*(u8 *)((u8 *)g_BattleDisplayState + i + 0xB0) != 0) {
            pSlot = g_BattleSlotGraphics[i];
            func_800728B8((u8 *)pSlot, pSlot->packageCount, *(u8 *)((u8 *)pSlot + 0x5A4));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * 40 + 0x280 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * 40 + 0x2D0 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * 40 + 0x320 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * 40 + 0x370 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * 40 + 0x140 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * 40 + 0x190 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * 40 + 0x1E0 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * 40 + 0x230 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * sizeof(POLY_G4) + 0x3C0 + (s32)pSlot));
            AddPrim(g_BattleCurOT[0] + 4, (u8 *)(*(u8 *)((u8 *)pSlot + 0x5A4) * sizeof(DR_MODE) + 0x408 + (s32)pSlot));
        }
    }
}
*/
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
            if (g_BattleSlotActorIds[offset] != 0x7F) {
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800745EC);
void BattleAddSpriteColumnPrimLists(void) {
    u16 counter;

    if (g_BattleDisplayState->actorPrimListsPending != 0) {
        func_800728B8(g_BattleSpriteColumnWork + 0x3E80, g_BattleSpriteColumnWork[0x5D77],
                      g_BattleSpriteColumnWork[0x5D86]);
        func_800728B8(g_BattleSpriteColumnWork + 0x5280, g_BattleSpriteColumnWork[0x5D7C],
                      g_BattleSpriteColumnWork[0x5D8B]);
        func_800728B8(g_BattleSpriteColumnWork + 0x53C0, g_BattleSpriteColumnWork[0x5D7D],
                      g_BattleSpriteColumnWork[0x5D8C]);
        func_800728B8(g_BattleSpriteColumnWork + 0x43D0, g_BattleSpriteColumnWork[0x5D7F],
                      g_BattleSpriteColumnWork[0x5D8E]);
    }

    if (g_BattleDisplayState->actorDrawActive == 0) {
        return;
    }

    func_800728B8(g_BattleSpriteColumnWork + 0x46A0, 20, g_BattleSpriteColumnWork[0x5D98]);
    func_800728B8(g_BattleSpriteColumnWork, g_BattleSpriteColumnWork[0x5D70],
                  g_BattleSpriteColumnWork[0x5D92]);
    func_800728B8(g_BattleSpriteColumnWork + 0x1720, g_BattleSpriteColumnWork[0x5D74],
                  g_BattleSpriteColumnWork[0x5D83]);

    counter = ((BattleSpriteColumnWork *)g_BattleSpriteColumnWork)->columnCounter + 1;
    ((BattleSpriteColumnWork *)g_BattleSpriteColumnWork)->columnCounter = counter;
    if ((s16)counter < 15) {
        func_800728B8(g_BattleSpriteColumnWork + 0x4CE0,
                      ((BattleSpriteColumnWork *)g_BattleSpriteColumnWork)->listPackageCount,
                      ((BattleSpriteColumnWork *)g_BattleSpriteColumnWork)->listPrimStart);
    } else if ((s16)counter >= 21) {
        ((BattleSpriteColumnWork *)g_BattleSpriteColumnWork)->columnCounter = 0;
    }

    func_800728B8(g_BattleSpriteColumnWork + 0x3AC0, g_BattleSpriteColumnWork[0x5D75],
                  g_BattleSpriteColumnWork[0x5D84]);
    func_800728B8(g_BattleSpriteColumnWork + 0x3E30, g_BattleSpriteColumnWork[0x5D76],
                  g_BattleSpriteColumnWork[0x5D85]);
    func_800728B8(g_BattleSpriteColumnWork + 0x4E70, g_BattleSpriteColumnWork[0x5D78],
                  g_BattleSpriteColumnWork[0x5D87]);
    func_800728B8(g_BattleSpriteColumnWork + 0x4FB0, g_BattleSpriteColumnWork[0x5D79],
                  g_BattleSpriteColumnWork[0x5D88]);
    func_800728B8(g_BattleSpriteColumnWork + 0x50A0, g_BattleSpriteColumnWork[0x5D7A],
                  g_BattleSpriteColumnWork[0x5D89]);
    func_800728B8(g_BattleSpriteColumnWork + 0x51E0, g_BattleSpriteColumnWork[0x5D7B],
                  g_BattleSpriteColumnWork[0x5D8A]);
    func_800728B8(g_BattleSpriteColumnWork + 0x2530, g_BattleSpriteColumnWork[0x5D7E],
                  g_BattleSpriteColumnWork[0x5D8D]);
    func_800728B8(g_BattleSpriteColumnWork + 0x32A0, g_BattleSpriteColumnWork[0x5D96],
                  g_BattleSpriteColumnWork[0x5D97]);
    func_800728B8(g_BattleSpriteColumnWork + 0x3C0, g_BattleSpriteColumnWork[0x5D71],
                  g_BattleSpriteColumnWork[0x5D93]);
    func_800728B8(g_BattleSpriteColumnWork + 0xD20, g_BattleSpriteColumnWork[0x5D72],
                  g_BattleSpriteColumnWork[0x5D94]);
    func_800728B8(g_BattleSpriteColumnWork + 0xFA0, g_BattleSpriteColumnWork[0x5D73],
                  g_BattleSpriteColumnWork[0x5D95]);
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
            // a register, which only the explicit offset form above produces.
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

    func_800728B8(g_BattleSpriteColumnWork + 0x5550, g_BattleSpriteColumnWork[0x5D80], g_BattleSpriteColumnWork[0x5D8F]);
    func_800728B8(g_BattleSpriteColumnWork + 0x5640, g_BattleSpriteColumnWork[0x5D81], g_BattleSpriteColumnWork[0x5D90]);
    func_800728B8(g_BattleSpriteColumnWork + 0x5C80, g_BattleSpriteColumnWork[0x5D82], g_BattleSpriteColumnWork[0x5D91]);
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

/* 79.17% (340 bytes against retail 344, frame 0x30 and mask s0-s3+ra exact).
   Everything is in place except two instruction-level leftovers: gcc cross-jumps
   the shared `0x40 - w/2` / `0x9A - w/2` subu out of both arms (retail duplicates
   it, one more instruction) and the final `+ off` addu comes out
   `addu a0,a0,s0` where retail has `addu a0,s0,a0`. Three levers got here from
   62.98: (1) D_800D3725/g_BattleTextSlotFlags are VOLATILE - retail loads g_BattleTextSlotFlags[off]
   twice per arm instead of CSE-ing it, and volatile is also what stops loop.c
   from making an address biv for each (three bivs with a plain declaration);
   (2) g_BattleCurOT is declared as an array so its address is materialised in a
   register (retail keeps it in $s3 and reads 0($s3) and 0x30($s3) - the 0x30 is
   g_BattlePrimStartIndex, and retail reaches it through that base ONLY in the
   AddPrim index, the SetPolyFT4 call uses the symbol itself);
   (3) g_BattlePrimBuffer as an array, and the prim address associated as
   `prod + ((s32)g_BattlePrimBuffer + off)` (the `off + (prod + base)` spelling makes
   gcc extract a `base + off` biv and costs a register and 8 bytes).
   The source indexes with i*0x60: retail folds g_BattleTextSlotFlags[i*0x60] in the i==0
   arm to the bare symbol, so it knew the index was 0 there.
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_8007500C);
/*
void func_8007500C(void) {
    s32 i;
    s32 off;

    i = 0;
    off = 0;
    do {
        if (D_800D3725[off] != 0) {
            if (i == 0) {
                BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattlePrimStartIndex * 40 + (s32)g_BattlePrimBuffer),
                                        0x40 - (g_BattleTextSlotFlags[off] >> 1), 0x2C, 0, 0, g_BattleTextSlotFlags[off]);
            } else {
                BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattlePrimStartIndex * 40 + ((s32)g_BattlePrimBuffer + off)),
                                        0x9A - (g_BattleTextSlotFlags[off] >> 1), 0xCA, 0,
                                        (13 * (i / 2)) & 0xFF, g_BattleTextSlotFlags[off]);
            }
            AddPrim(g_BattleCurOT[0] + 4,
                    (u8 *)(((s32 *)g_BattleCurOT)[12] * 40 + ((s32)g_BattlePrimBuffer + off)));
        }
        i++;
        off += 0x60;
    } while (i < 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075168);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075938);
void BattleDrawFrameScreenMode1(void) {
    if (g_BattleFrameDrawSuppressed == 0) {
        func_8008FAD8();
        BattleRenderSlotStatDigits();
        func_80075938();
        func_80073538();
        func_80073A58();
        func_80073B64();
        BattleAddTwoPrimLists();
        BattleAddThreeFlaggedPrimLists();
        func_8007500C();
        BattleAddThreePrimLists();
        func_800745EC();
        BattleUpdateRotationBuffer();
        func_80073FB8();
        func_80088B80();
        BattleAddSpriteColumnPrimLists();
    }
}


void func_800764B4(void) {
    D_8005959C = 0;
    func_8008FAD8();
    func_801DE594();
    func_80073FB8();
}

void BattleDrawFrameScreenMode2(void) {
    func_8008FAD8();
    func_80073538();
    BattleAddThreeFlaggedPrimLists();
    func_8007500C();
    BattleAddPrimAndRenderObject();
    func_80073FB8();
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

/* 97.59%: 332 bytes, retail's exact size, one instruction out of place - gcc emits
   the `sll v0,v0,1` (the u16 scale in the g_BattleSlotScreenOrigins index) before the
   `lui %hi/addiu %lo` of the symbol, retail after it. A two-minute permuter run from
   this body returned PERMUTER_NONE at base=60 (one instruction). What the body needed:
   (1) every access is written as an inline `((u8 *)g_BattleDisplayState + off)` so gcc
   reloads the global per block like retail (four loads) instead of keeping one register
   live; (2) the second half of the 0x34..0xAB run goes through two pointer locals
   (`pWork`, and `pAb` for the 0xAB store) because retail reloads into $v0 and $v1 there;
   (3) the three zero stores are a do/while(0) unit so they keep retail's place after
   the 0x5C pair; (4) the 0x5C=divisor store is the embedded assignment of the divisor
   expression (permuter find); (5) `pTail[slot + 0x90] -= 1` inside an embedded
   assignment of the base is what gives retail's `addu v1,v1,a2` operand order.
void func_800765C4(s32 slot) {
    u16 *pOrigin;
    u8 *pTail;
    u8 *pWork;
    u8 *pAb;
    s32 base;
    s32 offset;
    s32 offset2;
    s32 height;
    u32 divisor;
    s32 scaled;

    pOrigin = &g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot];
    base = *pOrigin;
    *(s32 *)((u8 *)g_BattleDisplayState + 0x44) = 0x1C;
    offset = base + 0x48;
    *(s32 *)((u8 *)g_BattleDisplayState + 0x34) = slot * 0x60 + offset;
    if (g_BattleSlotFlags[slot * 8] != 0 && g_BattleActorCharIds[slot] != 7) {
        offset2 = *pOrigin + 0x44;
        *(s32 *)((u8 *)g_BattleDisplayState + 0x44) = 0x24;
        *(s32 *)((u8 *)g_BattleDisplayState + 0x34) = slot * 0x60 + offset2;
    }

    *(s32 *)((u8 *)g_BattleDisplayState + 0x3C) = 0x10;
    *(s32 *)((u8 *)g_BattleDisplayState + 0x4C) = 0x98;
    pWork = (u8 *)g_BattleDisplayState;
    height = *(s32 *)(pWork + 0x34) - 5;
    *(s32 *)(pWork + 0x54) = height - *(s32 *)(pWork + 0x3C);
    divisor = *(s32 *)(pWork + 0x44) + 5;
    *(s32 *)(pWork + 0x5C) = *(s32 *)(pWork + 0x4C) - divisor;
    divisor = *(s32 *)(pWork + 0x5C);
    scaled = (*(u32 *)(pWork + 0x54) << 8) / divisor;
    *(u16 *)(pWork + 0x104) = 0x800;
    *(u8 *)((u8 *)g_BattleDisplayState + 0xA9) = 6;
    *(s32 *)(pWork + 0x5C) = 0x100;
    pAb = (u8 *)g_BattleDisplayState;
    do { // a unit so the cluster keeps retail's place after the 0x5C pair
        *(s32 *)(pWork + 0x64) = 0;
        *(s32 *)(pWork + 0x6C) = 0;
        *(u16 *)(pWork + 0x106) = 0;
    } while (0);
    *(s32 *)(pWork + 0x54) = scaled;
    *(u8 *)(pAb + 0xAB) = 1;
    *((pTail = (u8 *)g_BattleDisplayState) + slot + 0x90) -= 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800765C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076710);
/* 95.68% at retail's exact 728 bytes; 4-minute permuter runs from 91.53 and from
   this body both returned BETTER on their own metric but the ported edits were
   worse under objdiff, so nothing was kept. What is left is allocation and
   scheduling only:
   (1) loop 1's two `sh` sums: retail accumulates into the SHIFTED value's
   register (`addu v0,v0,v1`, dest = the >>8 result, `sh v0,40(sp)`); every
   operand order in C gives dest = the lhu (`addu v1,v1,v0`), and giving the
   shift its own `u32 dx` local swaps that but then puts the reload in $v1 and
   drops to 93.45.
   (2) retail reaches g_BattlePrimStartIndex at both func_80025FA8 call sites as
   `lui s0,%hi; addiu s0,s0,%lo` once plus `lw a3,0(s0)` per call; a `s32 *pCtx`
   local (also spelled `mem_addr pCtx` with a cast) is folded back to
   `lui a3,%hi; lw a3,%lo(a3)` per site. Only `extern s32
   g_BattlePrimStartIndex[]` was not tried, because three already-matched
   functions in this file read it as a scalar.
   (3) the second call's `idx * 0x50 + 0x3768` chain: the `sll <<4` and
   `addiu 0x3768` come out in the other order and `sll s0,s0,1` (idx*2) lands in
   the `j` delay slot instead of before the call.
   (4) `li a1,0x1` (SetSemiTrans' second argument) sits at the top of the loop
   body instead of in the back-edge delay slot, and the 0xAB loop's counter is
   $a1 instead of retail's $a3.
   The shapes that DID have to be right: the two package-call statements and the
   0x104/0x106 updates through the first `if` are ONE statement unit
   (`do { ... } while (0)`), which the permuter found and is worth 2 points; the
   0x77 byte and the 0x90 byte are raw `*(u8 *)((u8 *)g_BattleDisplayState + N)`
   so the global is reloaded between them like retail; `off`/`off2` hold
   `product + 0x3768` so the base is added last, which is what puts retail's
   `addiu` before the `lui/lw` of g_BattleGraphicsWork; and the two 16-byte
   x/y objects are one `u16 v[8]` read as v[0]/v[4] (that is what puts them at
   sp+0x28/0x30).
*/
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
    pCtx = &g_BattlePrimStartIndex;
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
}
*/
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

