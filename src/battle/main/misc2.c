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
    /* 0x66B */ u8 unk66B[5];
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

extern u8 g_DebugPageIndex;
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
extern BattleSpriteColumnWork *g_BattleSpriteColumnWork;
extern u8 g_BattleSlotActorIds[];
extern u8 g_BattleSlotFlags[];
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073B64);
/* 86.09%: all five call loops are recovered and their bodies match
   instruction-for-instruction - (1) `for (i = 0; i < g_BattleDisplayState[0x97]; i++)`
   AddPrim of a 0x10-stride prim at g_BattleGraphicsWork + 0x908 indexed by
   g_BattleDisplayState[0x98] + i*2; (2) four BattleAddPrimListToOT at g_BattleGraphicsWork + 0x2E08 +
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
        BattleAddPrimListToOT((u8 *)(g_BattleGraphicsWork + off),
                      *(u8 *)(g_BattleDisplayState + i + 0x74),
                      *(u8 *)(g_BattleDisplayState + i + 0x84));
    }

    for (i = 0, off = 0x6008, off3 = 0; i < 3; i++, off += 0x140, off3 += 0x1E4) {
        if (*(u8 *)(g_BattleDisplayState + i + 0xCC) != 0) {
            p = (u8 *)(g_BattleGraphicsWork + off3);
            switch (*(u8 *)(p + 0x852D)) {
            case 1:
                BattleAddPrimListToOT((u8 *)(g_BattleGraphicsWork + off3 + 0x835C),
                              *(u8 *)(p + 0x852E), *(u8 *)(p + 0x852C));
                break;
            case 2:
                BattleAddPrimListToOT((u8 *)(g_BattleGraphicsWork + off3 + 0x83FC),
                              *(u8 *)(p + 0x852F), *(u8 *)(p + 0x852C));
                break;
            }
            BattleAddPrimListToOT((u8 *)(g_BattleGraphicsWork + off),
                          *(s32 *)(g_BattleDisplayState + i * 4 + 0xEC),
                          *(u8 *)(g_BattleDisplayState + i + 0x99));
        }
    }

    if (g_BattleDisplayState[0xAF] != 0) {
        BattleAddPrimListToOT(g_BattleGraphicsWork + 0x9C8, g_BattleDisplayState[0x7B], g_BattleDisplayState[0xA4]);
    }

    for (i = 0, off = 0x3A88; i < 3; i++, off += 0xC80) {
        BattleAddPrimListToOT((u8 *)(g_BattleGraphicsWork + off),
                      *(s32 *)(g_BattleDisplayState + i * 4 + 0xE0),
                      *(u8 *)(g_BattleDisplayState + i + 0x93));
    }

    for (i = 0, off = 0x818, off2 = 0, off3 = 0; i < 3;
         i++, off += 0x50, off2 += 0x1C, off3 += 0x1E0) {
        if ((&g_BattleSlotActorIds)[off2] != 0x7F) {
            BattleAddPrimListToOT((u8 *)(g_BattleGraphicsWork + off), 1, g_BattleDisplayState[0x83]);
            if (*(u8 *)(g_BattleDisplayState + i + 0xCC) != 0) {
                BattleAddPrimListToOT((u8 *)(g_BattleGraphicsWork + off3),
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800745EC);
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

/* 85.91% at retail's exact 344 bytes, frame 0x30 and mask s0-s3+ra exact, and
   the instruction multiset is now identical - what is left is a register
   permutation inside the two arms (retail keeps the 13*(i/2) chain in $v1, the
   flags byte in $v0 and the x argument in $a1, and puts the $a5c delay slot's
   `srl v0,s1,0x1f` where this body puts a speculative `li a2,0xca`), plus the
   two `addu a0,s0,a0` / `addu a1,s0,a1` operand orders (the `off + (prod + base)`
   spelling that would produce them makes loop.c create a second accumulator).
   Three body shapes had to be right to get here from 57: (1) `off` is an
   accumulator local, NOT an inline `i * 0x60` - the inline form makes loop.c
   build an address biv for every use site (four accumulators, 376 bytes, 57.11);
   (2) the x argument gets its own `s32 x` local assigned per arm, which is what
   keeps retail's duplicated `subu a1,a1,v0` in each arm instead of one shared
   copy in the join block; (3) the 13*(i/2) value is carried in a `u8 v` through
   an embedded assignment at the argument (permuter find, 79.52 -> 85.91).
   A 5-minute permuter run from this body returned only candidates that reuse
   `off` for x (wrong code, rejected).
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_8007500C);
/*
void func_8007500C(void) {
    s32 i;
    s32 off;
    s32 x;
    u8 v;

    i = 0;
    off = 0;
    do {
        if (g_BattleTextSlotVisible[off] != 0) {
            if (i == 0) {
                x = 0x40 - (g_BattleTextSlotFlags[off] >> 1);
                BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattlePrimStartIndex * 40 + (s32)g_BattlePrimBuffer),
                                        x, 0x2C, 0, 0, g_BattleTextSlotFlags[off]);
            } else {
                x = 0x9A - (g_BattleTextSlotFlags[off] >> 1);
                BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattlePrimStartIndex * 40 + ((s32)g_BattlePrimBuffer + off)),
                                        x, 0xCA, 0,
                                        v = (13 * (i / 2)) & 0xFF, g_BattleTextSlotFlags[off]);
            }
            AddPrim(g_BattleCurOT[0] + 4,
                    (u8 *)(((s32 *)g_BattleCurOT)[12] * 40 + ((s32)g_BattlePrimBuffer + off)));
        }
        i++;
        off += 0x60;
    } while (i < 8);
}*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075168);
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
        func_800745EC();
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

void func_800765C4(s32 slot) {
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

