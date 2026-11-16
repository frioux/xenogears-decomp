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

extern u8 D_8005959C;
extern u8 *g_BattleGraphicsWork;
extern u8 g_BattleDisplayMode;
extern u8 D_800C492A;
extern u8 *g_BattleCurOT;
extern s32 D_800D2DAC;
extern BattlePrimPool *g_BattlePrimPool;
extern s32 g_BattlePrimStartIndex;
extern void *g_BattleSpriteArchive;
extern BattleWork *g_BattleDisplayState;
extern BattlePulseWork *g_BattlePulseTrianglePrims;
extern u16 g_BattleSlotScreenOrigins[];
extern u8 g_BattleSlotPageIndex;
extern u8 g_BattleActorCharIds[];
extern u8 *g_BattleSpriteColumnWork;
extern u8 D_800C3EB6;
extern u8 g_BattleSlotFlags;

extern u8 D_800D36C8;
extern u8 D_800D3725[];
extern u8 D_800D3726[];

extern s32 func_8002675C(void *pDir, s32 index, void *pPrims, s32 context, s16 x, s16 y, s16 scale);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *p, u16 x, s16 y, u8 u, u8 v, u8 dx);
extern void func_800728B8(u8 *pDst, s32 arg1, s32 arg2);
extern void func_80072A9C(s32 index, u8 arg1);
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
   0x1E0 strides, skipping entries whose D_800C3EB6[i*0x1C] is 0x7F, and the
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
        AddPrim(g_BattleCurOT + 4,
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
        if ((&D_800C3EB6)[off2] != 0x7F) {
            func_800728B8((u8 *)(g_BattleGraphicsWork + off), 1, g_BattleDisplayState[0x83]);
            if (*(u8 *)(g_BattleDisplayState + i + 0xCC) != 0) {
                func_800728B8((u8 *)(g_BattleGraphicsWork + off3),
                              *(u8 *)(g_BattleDisplayState + i + 0x78), g_BattleDisplayState[0xA2]);
                AddPrim(g_BattleCurOT + 4,
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

    if (g_BattleDisplayState->unkCB == 0) {
        return;
    }

    i = 0;
    offset = 0x641C;
    do {
        pWork = g_BattleDisplayState;
        func_800728B8((u8 *)(offset + (s32)g_BattleGraphicsWork), pWork->unkD0[i], pWork->unkA3);
        offset += 0xFA0;
        i++;
    } while (i < 2);
}

void BattleAddThreeFlaggedPrimLists(void) {
    if (g_BattleDisplayState->unk9C != 0) {
        func_800728B8(g_BattleGraphicsWork + 0xBA8, g_BattleDisplayState->unkF8, g_BattleDisplayState->unkA5);
    }

    if (g_BattleDisplayState->unk9E != 0) {
        func_800728B8(g_BattleGraphicsWork + 0x27C8, g_BattleDisplayState->unk100, g_BattleDisplayState->unkA7);
    }

    if (g_BattleDisplayState->unk9D != 0) {
        func_800728B8(g_BattleGraphicsWork + 0x1E68, g_BattleDisplayState->unkFC, g_BattleDisplayState->unkA6);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073FB8);
/* 87.42%: instruction multiset matches except the two g_BattleSlotFlags reads. The
   original forms `lui %hi(g_BattleSlotFlags); addu at,at,s1; lbu %lo(at)` from an
   integer index s1 = i*8 recomputed every iteration, and keeps &g_BattleCurOT in
   s0; gcc 2.7.2 strength-reduces the address into a pointer biv in s1 and
   rematerialises the link address with lui/lw. Every access shape tried
   (array subscript, integer cast of the symbol address, block-scoped base
   local) produces the biv. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800742A0);
/* void func_800742A0(void) {
    s32 i;
    s32 offset;
    s32 value;

    for (i = 0, offset = 0; i < 3; i++, offset += 0x1C) {
        if (*(u8 *)(g_BattleDisplayState + i + 0x7C) != 0) {
            *(s32 *)(g_BattleDisplayState + i * 4 + 0xE0) = 0;
            *(s32 *)(g_BattleDisplayState + i * 4 + 0xEC) = 0;
            if ((&D_800C3EB6)[offset] != 0x7F) {
                BattleRenderActorValueDigits(i);
                value = func_80072F38(i, *(u8 *)((s32)&g_BattleSlotFlags + i * 8));
                if (*(u8 *)((s32)&g_BattleSlotFlags + i * 8) != 0) {
                    BattleRenderActorStatDigits(i, value);
                } else {
                    func_80072A9C(i, value);
                }
                *(u8 *)(g_BattleDisplayState + i + 0x93) = *(u8 *)&g_BattlePrimStartIndex;
                *(u8 *)(g_BattleDisplayState + i + 0x7C) = 0;
            }
        }
    }
} */
/* 90.29%: the only differences are that the original keeps the address of
   g_BattleCurOT in s0 and reloads through it, and its `addu a1,a1,v1` (offset
   first) comes out as `addu a1,v1,a1` here. Same two problems as
   func_800744BC, but with four AddPrim uses of the link address still not
   enough to make gcc spill it to a callee-saved register. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800743A4);
/* void func_800743A4(void) {
    if ((*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x669] == 0) {
        return;
    }

    AddPrim(g_BattleCurOT + 4, (*(u8 **)(g_BattleGraphicsWork + 0xA230)) + (*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x668] * 40);
    AddPrim(g_BattleCurOT + 4, (*(u8 **)(g_BattleGraphicsWork + 0xA230)) + 0x50 + (*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x668] * 40);
    AddPrim(g_BattleCurOT + 4, (*(u8 **)(g_BattleGraphicsWork + 0xA230)) + 0xA0 + (*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x668] * 40);
    AddPrim(g_BattleCurOT + 4, (*(u8 **)(g_BattleGraphicsWork + 0xA230)) + 0xF0 + (*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x668] * 40);
} */
/* 79.68% (82.84% with the offset left inline). Splitting the offset into a
   named local and adding the prim pointer last fixes the final addu's operand
   order (addu a1,v0,a1, the original's addu a1,a1,v1) - the trick that matched
   BattleAddPrimAndRenderObject. What is left is allocation only: gcc puts p in a1 and the sum
   in v0 where the original builds the sum in a1 and keeps p in v1, and gcc
   re-forms %hi/%lo(g_BattleCurOT) per call instead of keeping &g_BattleCurOT in s0
   and reloading through it. func_800743A4 and func_80074554 are identical. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800744BC);
/* 79.68%: only allocation differs. Retail keeps p in $v1 and accumulates the
   offset chain directly in the argument register $a1 (addu $a1,$a1,$v1 in the
   jal slot) and keeps &g_BattleCurOT in a saved $s0 (`lw $a0,0($s0)` twice); gcc
   gives $a1 to p and builds the sum in $v0 (addu $a1,$v0,$a1). Also tried
   `u8 **pp = &g_BattleCurOT;` and AddPrim(*pp + 4, ...): same 79.68%, the address
   local is folded away and none of the $s0 save/restore is emitted. Retail's
   source clearly held the OT address in a local that survived the call, which
   reload_cse rematerialises for us. Allocator tie, not a source-shape problem. */
/* void func_800744BC(void) {
    u8 *p;
    s32 off;

    p = *(u8 **)(g_BattleGraphicsWork + 0xA230);
    off = p[0x66A] * 40 + 0x320;
    AddPrim(g_BattleCurOT + 4, (u8 *)(off + (s32)p));
    p = *(u8 **)(g_BattleGraphicsWork + 0xA230);
    off = p[0x66A] * 40 + 0x370;
    AddPrim(g_BattleCurOT + 4, (u8 *)(off + (s32)p));
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074554);
/* Same shape as func_800744BC with 0x280 and 0x2D0: identical result. */
/* void func_80074554(void) {
    AddPrim(g_BattleCurOT + 4, (*(u8 **)(g_BattleGraphicsWork + 0xA230)) + 0x280 + (*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x66A] * 40);
    AddPrim(g_BattleCurOT + 4, (*(u8 **)(g_BattleGraphicsWork + 0xA230)) + 0x2D0 + (*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x66A] * 40);
} */

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800745EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074AB8);
void BattleUpdateRotationBuffer(void) {
    s32 i;

    if (g_BattleDisplayState->unkC6 == 0) {
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
            AddPrim(g_BattleCurOT + 4,
                    (u8 *)((i * 2 + g_BattlePulseTrianglePrims->firstPrim) * sizeof(POLY_G3) +
                           (s32)g_BattlePulseTrianglePrims));
        }
        i++;
    } while (i < 4);
}
void BattleAddThreePrimLists(void) {
    if (g_BattleDisplayState->unkA8 == 0) {
        return;
    }

    func_800728B8(g_BattleSpriteColumnWork + 0x5550, g_BattleSpriteColumnWork[0x5D80], g_BattleSpriteColumnWork[0x5D8F]);
    func_800728B8(g_BattleSpriteColumnWork + 0x5640, g_BattleSpriteColumnWork[0x5D81], g_BattleSpriteColumnWork[0x5D90]);
    func_800728B8(g_BattleSpriteColumnWork + 0x5C80, g_BattleSpriteColumnWork[0x5D82], g_BattleSpriteColumnWork[0x5D91]);
}

void BattleAddPrimAndRenderObject(void) {
    BattlePrimPool *pPool;
    s32 off;

    if (g_BattleDisplayState->unkC8 != 0) {
        pPool = g_BattlePrimPool;
        off = pPool->unk7F4 * sizeof(POLY_FT4) + 0x7A4;
        AddPrim(g_BattleCurOT + 4, (u8 *)(off + (s32)pPool));
    }

    if (g_BattleDisplayState->unkC9 != 0) {
        func_80034888(D_800D2DAC, g_BattleCurOT + 4, g_BattlePrimStartIndex);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_8007500C);
/* 62.98%: the loop, the two argument sets, the u16/u8 conversions and the
   frame all match; gcc 2.7.2 hoists `la D_800D3726` (4 uses in the loop) into
   a callee-saved register, and strength-reduces the `off + base` prim address
   into a second biv, while the original keeps no address register at all: it
   re-forms `lui %hi(sym); addu $at,$at,off; lbu %lo($at)` at each use and
   `addu a0,off,base` per call (i.e. retail current loop.c reduced nothing).
   Also open: the original reaches g_BattlePrimStartIndex through `&g_BattleCurOT + 0x30`
   (`lw v0,0x30(s3)` with `la g_BattleCurOT` hoisted) in the AddPrim index only.
   Tried: explicit `off` counter, inline `i * 0x60`, `(s32)&sym + off` casts,
   `sym[off]`, literal `[0]` in the i==0 branch, plus cdk / -O1 / -O3 / -G0
   (try_flags: 44.94-62.98). */
/*
    void func_8007500C(void) {
        s32 i;
        s32 off;

        i = 0;
        off = 0;
        do {
            if (D_800D3725[off] != 0) {
                if (i == 0) {
                    BattleSetPolyFT4RectH13((POLY_FT4 *)((s32)&D_800D36C8 + g_BattlePrimStartIndex * 40),
                                  0x40 - (D_800D3726[off] >> 1), 0x2C, 0, 0, D_800D3726[off]);
                } else {
                    BattleSetPolyFT4RectH13((POLY_FT4 *)(off + ((s32)&D_800D36C8 + g_BattlePrimStartIndex * 40)),
                                  0x9A - (D_800D3726[off] >> 1), 0xCA, 0,
                                  (13 * (i / 2)) & 0xFF, D_800D3726[off]);
                }
                AddPrim(g_BattleCurOT + 4,
                        (u8 *)(off + ((s32)&D_800D36C8 + g_BattlePrimStartIndex * 40)));
            }
            i++;
            off += 0x60;
        } while (i < 8);
    }*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075168);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075938);
void BattleDrawFrameScreenMode1(void) {
    if (D_800C492A == 0) {
        func_8008FAD8();
        func_800742A0();
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
        func_80074AB8();
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
    func_80074AB8();
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800765C4);
/* 65.07% (316 bytes vs retail 320; the earlier attempt was 59.87%): the head,
   the two conditional stores and the final byte decrement now match, including
   the divu (height/divisor are u32) and the `addiu v1,v1,0x48` before the
   `addu v1,a1,v1`. Still wrong: gcc reassociates `arg0 * 0x60 + (base + 0x48)`
   into `(arg0 * 0x60 + 0x48) + base`, and it hoists the 0x104/0xA9 stores above
   the height/divisor loads, so retail's sw 0x54=height, load 0x4C/0x54, divu,
   mflo block comes out in a different order. Retail also re-reads g_BattleDisplayState
   into a fresh register for the 0xAB store.
void func_800765C4(s32 arg0) {
    u16 *pEntry;
    s32 base;
    u32 height;
    u32 divisor;

    pEntry = &g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + arg0];
    base = *pEntry;
    *(s32 *)(g_BattleDisplayState + 0x44) = 0x1C;
    *(s32 *)(g_BattleDisplayState + 0x34) = arg0 * 0x60 + (base + 0x48);
    if ((&g_BattleSlotFlags)[arg0 * 8] != 0 && g_BattleActorCharIds[arg0] != 7) {
        *(s32 *)(g_BattleDisplayState + 0x44) = 0x24;
        *(s32 *)(g_BattleDisplayState + 0x34) = arg0 * 0x60 + (base + 0x44);
    }

    *(s32 *)(g_BattleDisplayState + 0x3C) = 0x10;
    *(s32 *)(g_BattleDisplayState + 0x4C) = 0x98;
    height = *(s32 *)(g_BattleDisplayState + 0x34) - 5 - *(s32 *)(g_BattleDisplayState + 0x3C);
    *(s32 *)(g_BattleDisplayState + 0x54) = height;
    divisor = *(s32 *)(g_BattleDisplayState + 0x4C) - (*(s32 *)(g_BattleDisplayState + 0x44) + 5);
    *(u16 *)(g_BattleDisplayState + 0x104) = 0x800;
    g_BattleDisplayState[0xA9] = 6;
    *(s32 *)(g_BattleDisplayState + 0x5C) = divisor;
    *(s32 *)(g_BattleDisplayState + 0x5C) = 0x100;
    *(s32 *)(g_BattleDisplayState + 0x64) = 0;
    *(s32 *)(g_BattleDisplayState + 0x6C) = 0;
    *(u16 *)(g_BattleDisplayState + 0x106) = 0;
    *(s32 *)(g_BattleDisplayState + 0x54) = (*(u32 *)(g_BattleDisplayState + 0x54) << 8) / divisor;
    g_BattleDisplayState[0xAB] = 1;
    g_BattleDisplayState[0x90 + arg0]--;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076710);
/* 92.93%: everything structural matches (the 0xA9 loop, both func_80025FA8
   calls with their five stack arguments, the SetSemiTrans loop, the 0xAB loop,
   the two range tests, the 0x58 frame). Shapes that were needed: x and y are
   16-byte memory objects (`u16 v[8]`, only v[0] and v[4] used) - two u16 locals
   land at sp+0x28/0x2A and shrink the frame by 8 bytes; one `i` shared by the
   three loops keeps the middle loop's index in $s0 across the SetSemiTrans
   call, retail has it in $a3, so each loop needs its own variable; the
   SetSemiTrans offset needs its own `off` local to get retail's
   addu-in-the-jal-slot operand order. What is left is allocation only:
   (1) retail keeps &g_BattlePrimStartIndex in $s0 from just after the 0x77 store and does
   `lw a3,0(s0)` at both call sites, gcc 2.7.2 rematerialises
   `lui/lw %lo(g_BattlePrimStartIndex)` at each site (a `s32 *pCtx` local does not survive,
   it folds back and costs 8 points); (2) loop 1 emits `addu v1,v1,v0` where
   retail has `addu v0,v0,v1` - gcc canonicalises the commutative operands, so
   swapping the C operands changes nothing; (3) the second call's
   `idx * 0x50 + 0x3768 + (s32)g_BattleGraphicsWork` chain interleaves with the stack-arg
   stores (retail emits the three shifts contiguously and adds the constant to
   the product, not to the base) and `li a1,0x1` is not stolen into the loop's
   back-edge delay slot; (4) the 0xAB loop's index is $a1 here, $a3 in retail. */
/*
void func_80076710(s32 arg0) {
    u16 v[8];
    s32 i;
    s32 j;
    s32 k;
    s32 idx;
    s32 off;

    for (i = 0; i < g_BattleDisplayState[0xA9]; i++) {
        *(u32 *)(g_BattleDisplayState + 0x64) -= *(u32 *)(g_BattleDisplayState + 0x54);
        *(u32 *)(g_BattleDisplayState + 0x6C) += *(u32 *)(g_BattleDisplayState + 0x5C);
        v[0] = *(u16 *)(g_BattleDisplayState + 0x34) + (*(u32 *)(g_BattleDisplayState + 0x64) >> 8);
        v[4] = *(u16 *)(g_BattleDisplayState + 0x44) + (*(u32 *)(g_BattleDisplayState + 0x6C) >> 8);
    }

    g_BattleDisplayState[0xA9] = 0;
    g_BattleDisplayState[0x77] = 0;

    g_BattleDisplayState[0x77] = func_80025FA8(g_BattleSpriteArchive, 0x52, g_BattleGraphicsWork + 0x3768, g_BattlePrimStartIndex,
                                     v[0], v[4], *(u16 *)(g_BattleDisplayState + 0x104),
                                     *(u16 *)(g_BattleDisplayState + 0x104),
                                     *(u16 *)(g_BattleDisplayState + 0x106));

    idx = g_BattleDisplayState[0x77];
    g_BattleDisplayState[0x77] +=
        func_80025FA8(g_BattleSpriteArchive, 0x53, idx * 0x50 + 0x3768 + (s32)g_BattleGraphicsWork, g_BattlePrimStartIndex,
                      v[0], v[4], *(u16 *)(g_BattleDisplayState + 0x104), *(u16 *)(g_BattleDisplayState + 0x104),
                      *(u16 *)(g_BattleDisplayState + 0x106));

    for (j = idx * 2; j < g_BattleDisplayState[0x77] * 2; j += 2) {
        off = (j + g_BattlePrimStartIndex) * 40 + 0x3768;
        SetSemiTrans((u8 *)(off + (s32)g_BattleGraphicsWork), 1);
    }

    g_BattleDisplayState[0x87] = *(u8 *)&g_BattlePrimStartIndex;

    for (k = 0; k < g_BattleDisplayState[0xAB]; k++) {
        *(u16 *)(g_BattleDisplayState + 0x104) += 0x66;
        *(u16 *)(g_BattleDisplayState + 0x106) += 0x80;
    }
    g_BattleDisplayState[0xAB] = 0;

    if (v[0] <= *(u32 *)(g_BattleDisplayState + 0x3C)) {
        *(u8 *)(g_BattleDisplayState + arg0 + 0x90) = 1;
    }
    if (v[4] >= *(u32 *)(g_BattleDisplayState + 0x4C)) {
        *(u8 *)(g_BattleDisplayState + arg0 + 0x90) = 1;
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

