#include "common.h"
#include "main/game.h"
#include "battle/actor.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

// Sprite work buffer (g_BattleDisplayState). battle_work.h's BattleWork names the same
// block but stops at 0xA8, so the 0xB0/0xB7 flag bytes are declared here.
typedef struct {
    /* 0x000 */ u8 unk0[0x9C];
    /* 0x09C */ u8 spriteColumnPending;
    /* 0x09D */ u8 unk9D[0x8];
    /* 0x0A5 */ u8 spriteColumnPrimStart;
    /* 0x0A6 */ u8 unkA6[0xA];
    /* 0x0B0 */ u8 unkB0;
    /* 0x0B1 */ u8 unkB1[0x6];
    /* 0x0B7 */ u8 unkB7;
    /* 0x0B8 */ u8 unkB8[0x40];
    /* 0x0F8 */ s32 spriteColumnPackageCount;
} BattleSpriteWork;

extern u8 g_BattleActorCharIds[];
extern u16 g_BattleSavedSlotUnk7A[];
extern BattleSpriteWork *g_BattleDisplayState;
extern u8 g_BattleCurSlotIndex;
extern BattleWorkBlock *g_BattleCurWorkBlock;
extern BattleGraphicsWork *g_BattleGraphicsWork;
extern u8 g_BattlePrimStartIndex[];
extern BattleRow *g_BattleCurrentRow;
extern BattleTableEntry *g_BattleTargetSlotEntry;
extern BattleTableEntry g_BattleWorkBlock[];
extern s32 g_BattleCurveTargets[];
extern u8 g_BattleCurveTargetModes[];
extern u16 g_BattleCommandCellFlags;
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern BattleTableEntry *g_BattleActingSlot;
extern u8 g_BattleSlotFlags[];
extern u8 D_800C3CFA[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
extern u8 g_BattleActionRows[];
extern u8 g_BattleCellActionRows[];
extern u8 g_BattleActionRowIconSprites[];
extern u8 D_800D2F69[];
extern u8 D_800D2F6A[];
extern u8 D_800D2F6B[];
extern u8 g_BattleSlotFlags[];
extern void *D_800D367C;

void BattleReleaseEffectSlotIfChar4(void);
void BattleSetSlotEntryFieldsTo2000(u8, s32);
void func_800916D4(u8, u8, s32);
void BattleParseNumberToDigits(u32);
void BattleSetPolyFT4RectH13(POLY_FT4 *, u16, s16, u8, u8, s16);
void BattleSetPolyFT4Rect(POLY_FT4 *, s16, s16, u8, u8, s16, s16);
s32 func_8009A258(u8, s32);
void func_8009187C(u8, u8, u8);
void func_80091B38(u8, u8, u8);
void func_80093578(u8, u8);
void func_8009382C(u8, u8);
s32 BattleDrawSpritePackage(s32, u8 *, s32, s16);
s32 BattleMaskHasStatusBit(s32, u8);
extern void *BattleAllocTextBuffer(s32);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern void BattleLoadImageAndSync(RECT *, u_long *);
extern u8 *D_800C3DE8;
extern u8 g_BattleSlotTextIndex[];
extern u8 g_BattleTargetSlotIndex;
extern u8 g_BattleAmountThirdFlag;
extern s32 func_80096AB8(void);
extern u16 func_80096FBC(void);
extern u16 func_80097610(void);
extern void func_80096494(u16 *, u16 *, u8 *);
extern void BattleAllocPacketPrimitives(void);
extern void func_80076EA4(void);
extern s32 GetStringEntryFromBank(s32, s32);
extern u_long *GetStringEntryFromBank12(s32);
extern u_long *D_800C3E5C[];
void func_80092298(u8, u8 *);
extern u_long *D_800D2DB0;
extern void bzero(void *, s32);
extern void HeapFree(void *);

// The largest amount the current action may deal; the tail of BattleComputeActionAmount clamps to it.
#define BATTLE_ACTION_AMOUNT_MAX 9999

INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091064);
void BattleBuildSpriteColumn8(s32 arg0) {
    s32 i;
    s32 offset;

    g_BattleDisplayState->spriteColumnPackageCount = 0;
    for (i = 0; i < 8; i++) {
        offset = i * 8 + 0x38;
        g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(0x66,
            (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * 80, 0x20,
            (s16)(offset + arg0));
    }
    g_BattleDisplayState->spriteColumnPrimStart = g_BattlePrimStartIndex[0];
    g_BattleDisplayState->spriteColumnPending = 1;
}
/* 94.90% (424 bytes, retail's exact size) under this unit's gcc 2.6.0 (86.04
   under 2.7.2). The whole body now compiles against the typed externs and every
   instruction is retail's; the two residual blocks are (1) the position of
   `li a1,0x20` / `li a2,0xA4`, which gcc materialises straight after the
   `andi a1,a1,0xff` param mask where retail emits them after the u/v arithmetic,
   and (2) retail's `andi a3,a3,0xf8` sitting after the pointer load, not before.
   The u/v ORDER is the load-bearing part: retail computes `u = (s - s / 2 * 2) *
   0x78 & 0xF8` (a3) BEFORE `v = s / 2 * 16` (v1, the stack arg) and keeps s/2
   live in v1; with `u8 v` gcc hoists v's `sll` above the u chain instead, and
   declaring v `s32` with an explicit `& 0xFF` is what puts the pieces in retail's
   order. Tried: u8/u16/u32/s32 for both u and v, u/v as locals vs inline in the
   call, `% 2`, `(s >> 1) << 4`, a `half` local, a `rest` local, casts on the
   arguments (all either 94.15 or 94.90), and two 4-minute permuter runs (one
   reported score 0, which diff_func scores 94.15). Also needs the u16 second
   parameter on the BattleSetPolyFT4RectH13 declaration below: retail narrows the
   loop's k with `andi a1,s1,0xffff`, which an s16 parameter turns into sll/sra. */
/*
void func_800916D4(u8 arg0, u8 arg1, s32 arg2) {
    u32 s;
    s32 c;
    s32 i;
    u16 k;
    u8 u;
    s32 v;

    s = arg1 * 2 + arg0;
    u = (s - s / 2 * 2) * 0x78 & 0xF8;
    v = s / 2 * 16 & 0xFF;
    BattleSetPolyFT4Rect(
        (POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x140)),
        0x20, 0xA4, u, v, 0x60, 0x10);
    g_BattleGraphicsWork->pPacketPrims[0x66E] = 0;
    if (g_BattleActorCharIds[(u8)arg2] == 1) {
        BattleParseNumberToDigits(func_8009A258((u8)arg2, s + 0x16));
        for (i = 0, k = 0x8C; i < 3; k += 8) {
            c = D_800C3CFA[i++];
            if (c == 0xFF) {
                continue;
            }
            BattleSetPolyFT4RectH13(
                (POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                             ((g_BattleGraphicsWork->pPacketPrims[0x66E] * 2 +
                               ((s32 *)g_BattlePrimStartIndex)[0]) * 0x28 + 0x190)),
                k, 0xA6, (c * 8 + 0x78) & 0xF8, 0, 8);
            g_BattleGraphicsWork->pPacketPrims[0x66E]++;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800916D4);
/* 98.22% (704 bytes against retail's 700) under gcc 2.6.0 as well as 2.7.2, so
   this is now a one-instruction residual and not a preset question. The whole
   body reproduces retail: the two flag-row loads need the row offset in a local
   (`off = (arg2 * 2 + arg1 + 0x16) * 0x28 + arg0 * 0x5F0;` then
   `*(u16 *)(g_BattleCellActionRows + off)`), which is what makes gcc emit
   retail's symbol+register `lui $at` macro form; the row pick is 9 (0x4000) /
   7 (0x1000) / 8; the prim base needs a `u8 *pRec` local assigned before each
   palette if; and the switch case bodies must be listed 0, 2, default, 1 to get
   retail's layout (97.48 with 1, 0, 2, default).
   The residual is the FIRST beq's delay slot: retail keeps case 1's body out of
   line at the end of the switch and fills that slot with `slti v0,v1,2`, gcc
   fills it with a nop (and, for the ==2 test, inverts to `bne` with the next
   statement's `li a1,0xda` stolen into the slot where retail has
   `beq` + `li s2,0x15`). Listing the cases 0, 2, 1, default instead DOES give
   retail's exact 700 bytes (97.62) but puts case 1's body in that slot, so the
   tie is which instruction gcc parks there, not the tree shape. Tried: every
   case listing order, `s32 t = flags & 0xF; switch (t)`, `u8 t`, and a
   pre-initialised `col = 0xC` with a two-case switch. */
/*
void func_8009187C(u8 arg0, u8 arg1, u8 arg2) {
    u16 flags;
    u16 pal;
    u8 *pRec;
    s32 row;
    s32 col;
    u32 off;

    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        off = (arg2 * 2 + arg1 + 0x16) * 0x28 + arg0 * 0x5F0;
        flags = *(u16 *)(g_BattleCellActionRows + off);
    } else {
        off = (arg2 * 2 + arg1 + 0x15) * 0x28 + arg0 * 0x690;
        flags = *(u16 *)(g_BattleActionRows + off);
    }
    if (flags & 0x4000) {
        row = 9;
    } else if (flags & 0x1000) {
        row = 7;
    } else {
        row = 8;
    }
    switch (flags & 0xF) {
    case 0:
        col = 0xC;
        break;
    case 2:
        col = 0x15;
        break;
    default:
        col = 0xC;
        break;
    case 1:
        col = 0xD;
        break;
    }
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x320)),
                           0xDA, 0xAA, D_800D2F6A[row * 4], D_800D2F6B[row * 4], g_BattleActionRowIconSprites[row * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28;
    if (D_800D2F69[row * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x370)),
                           0xFE, 0xAA, D_800D2F6A[col * 4], D_800D2F6B[col * 4], g_BattleActionRowIconSprites[col * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28;
    if (D_800D2F69[col * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009187C);
/* 99.22% (508 bytes against retail's 512): the array-extern spelling for
   g_BattlePrimStartIndex plus `g_BattleGraphicsWork->pPacketPrims` reproduces
   retail's `lui s3/addiu s3` + `lw 0(s3)` per call, and every other instruction
   matches (geometry, RECT, both string draws, HeapFree). 87.45 -> 99.22 in one
   edit. Residual: the same single `andi s0,s0,0xffff` retail emits at index's
   first use that func_8009382C is missing -- gcc 2.6.0 proves the u16 range
   (u8 table load * 0x20 + (u8 * 2 + u8) * 2 fits) and drops the truncation, so
   the body is 4 bytes short. The two functions want the same answer: a spelling
   of a `u16 index` computed from byte loads whose range cc1 2.6.0 cannot prove
   (u16/s32/u32 locals, a u32 intermediate, a 3-statement form and a `u16`
   parameter on GetStringEntry were all measured dead in func_8009382C). */
/*
void func_80091B38(u8 arg0, u8 arg1, u8 arg2) {
    RECT rect;
    u8 *pBuf;
    u16 index;
    s32 len1;
    s32 len2;

    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        index = g_BattleActorCharIds[arg0] * 0x20 + (arg2 * 2 + arg1) * 2;
    } else {
        index = g_GameState.characters[g_BattleActorCharIds[arg0]].gearId * 0x20 + (arg2 * 2 + arg1) * 2;
    }
    pBuf = (u8 *)BattleAllocTextBuffer(0x39);
    bzero(pBuf, 0x618);
    len1 = SystemRenderStringEntry(GetStringEntry(D_800D367C, index), pBuf, 0x39, 0);
    len2 = SystemRenderStringEntry(GetStringEntry(D_800D367C, index | 1), pBuf, 0x39, 1);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, pBuf);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x280)), 0x30, 0xB6, 0, 0, len1 & 0xFF);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x2D0)), 0x30, 0xC6, 0, 0, len2 & 0xFF);
    HeapFree(pBuf);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091B38);
void BattleDrawCharStatusIfFlagged(u8 arg0, char arg1, u8 arg2) {
    u8 ret = 0;

    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        ret = (u16)BattleMaskHasStatusBit(
                  (u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].characterConditionMask,
                  arg1 + arg2 * 2) != 0;
    } else if ((u16)BattleMaskHasStatusBit(
                   (u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].gearConditionMask,
                   arg1 + arg2 * 2) != 0) {
        ret = 1;
    }
    if (ret != 0) {
        func_800916D4(arg1, arg2, arg0);
        func_8009187C(arg0, arg1, arg2);
        func_80091B38(arg0, arg1, arg2);
        g_BattleGraphicsWork->pPacketPrims[0x66A] = g_BattlePrimStartIndex[0];
        g_BattleGraphicsWork->pPacketPrims[0x66B] = 1;
    } else {
        g_BattleGraphicsWork->pPacketPrims[0x66B] = 0;
    }
}
void BattleDrawSlotSpriteQuads(u8 arg0) {
    u32 zero;

    zero = 0;
    *(u16 *)(g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[zero] * 0x28)) + 0xE) = g_SystemPalette1;
    *(u16 *)(g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[zero] * 0x28)) + 0x5E) = g_SystemPalette1;
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[zero] * 0x28), 0x34, 0x34, 0, arg0, 0x60, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0x50)), 0xB8, 0x34, 0x78, arg0, 0x60, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0xA0)), 0x9C, 0x34, 0, arg0, 0x10, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0xF0)), 0x120, 0x34, 0x40, arg0, 0x10, 0x60);
    g_BattleGraphicsWork->pPacketPrims[0x668] = g_BattlePrimStartIndex[0];
}
void BattleDrawSlotStatusPanel(void) {
    u32 zero;
    u8 **ppPrims;
    u8 *pPal;
    u8 *pPal2;

    zero = 0;
    ppPrims = &g_BattleGraphicsWork->pPacketPrims;
    pPal = g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[zero] * 0x28));
    *(u16 *)(pPal + 0xE) = g_SystemPalette1;
    pPal2 = *ppPrims - (-(((s32 *)g_BattlePrimStartIndex)[zero] * 0x28));
    *(u16 *)(pPal2 + 0x5E) = g_SystemPalette1;
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28), 0x36, 0x62, 0, 0, 0x60, 0x40);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x50)), 0xC2, 0x62, 0x78, 0, 0x60, 0x40);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0xA0)), 0x96, 0x62, 0, 0, 0x10, 0x40);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0xF0)), 0x122, 0x62, 0x40, 0, 0x10, 0x40);
    g_BattleGraphicsWork->pPacketPrims[0x668] = g_BattlePrimStartIndex[0];
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->unkB7 = 3;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092298);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092784);
// 70.57% under this unit's gcc 2.6.0, at objdiff's 1028 bytes -- the same count
// as the original, so the body is the right LENGTH and every difference is a
// frame offset. The
// whole body reproduces retail instruction for instruction -- the 0x618 text
// buffer, the eight 0x1B x 0x10 tiles, the glyph blit shared by the i == 7 and
// the arg1[i] arms, the even-tile load, the tens/ones digit blits with the
// D_800C3E5C digit-buffer table -- and the only residual is the FRAME: retail is
// 0x90 with 88 bytes of vars, this body is 0x98 because loop.c hoists the
// address of the tens rect (the one address used from two arms, since retail has
// a separate jal for the tens blit in each) and reload spills it (8 bytes at
// 0x68) instead of rematerialising it. Retail has no address in a register at
// all: every blit re-forms `addiu a0,sp,N` inline, which it can do because it
// holds exactly nine values (i, (i/2)*16, (i%2)*0x1E+0x380, arg1, arg2, 0xD,
// arg0, 6 and &pBufs[i]) in the nine saved registers. Handing the glyph and ones
// rects ONE use each (rect stores in the arms, a `goto nextTile` past the glyph
// blit, and a pData local feeding the ones blit both arms share) removes three of
// the four hoisted addresses and leaves only the tens one, but that spelling is
// 70.43% at 1012 bytes (4 bytes SHORT, so it cannot link); the body below keeps
// the blit duplicated in both arms, 70.57% at 1028.
// Tried: six vs one-at-a-time declaration orders (which DO decide the slots --
// retail's are glyph 0x10, tens 0x18, ones 0x20, tile 0x28, second 0x30, source
// rect 0x38, pBufs 0x40, i.e. that declaration order), block-scoped duplicate
// RECTs (gcc does not share stack slots), a RECT* local per site (worse), the
// free loop as a pointer walk (no change), which shows the extra biv is the
// first loop's `pBufs[i]` byte offset ($20 = i*4, live across both loops).
/*
void func_80092784(u8 arg0, u8 *arg1, u8 *arg2) {
    RECT glyphRect;
    RECT tensRect;
    RECT onesRect;
    RECT tileRect;
    RECT secondRect;
    RECT rect;
    u_long *pBufs[8];
    u_long *pData;
    u8 tensDigit;
    u8 onesDigit;
    s32 i;

    BattleAllocPacketPrimitives();
    func_80076EA4();
    D_800D2DB0 = BattleAllocTextBuffer(0x39);
    bzero(D_800D2DB0, 0x618);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, D_800D2DB0);
    for (i = 0; i < 8; i++) {
        pBufs[i] = BattleAllocTextBuffer(0x1B);
        bzero(pBufs[i], 0x30C);
        tileRect.x = (i % 2) * 0x1E + 0x380;
        tileRect.y = (i / 2) * 16 + 0x100;
        tileRect.w = 0x1B;
        tileRect.h = 0x10;
        BattleLoadImageAndSync(&tileRect, D_800D2DB0);
        if (i == 7) {
            SystemRenderStringEntry(GetStringEntryFromBank12(0xA), pBufs[i], 0x1B, 0);
            glyphRect.x = (i % 2) * 0x1E + 0x380;
            glyphRect.y = (i / 2) * 16 + 0x102;
            glyphRect.w = 0x1E;
            glyphRect.h = 0xD;
        } else if (arg1[i] != 0xFF) {
            SystemRenderStringEntry(GetStringEntryFromBank(g_BattleActorCharIds[arg0], arg1[i]), pBufs[i], 0x1B, 0);
            glyphRect.x = (i % 2) * 0x1E + 0x380;
            glyphRect.y = (i / 2) * 16 + 0x102;
            glyphRect.w = 0x1E;
            glyphRect.h = 0xD;
        } else {
            goto nextTile;
        }
        BattleLoadImageAndSync(&glyphRect, pBufs[i]);
nextTile:
        if ((i & 1) == 0) {
            secondRect.x = 0x3C0;
            secondRect.y = (i / 2) * 16 + 0x100;
            secondRect.w = 0x1B;
            secondRect.h = 0x10;
            BattleLoadImageAndSync(&secondRect, D_800D2DB0);
        }
        tensRect.x = (i % 2) * 16 + 0x3C0;
        tensRect.y = (i / 2) * 16 + 0x102;
        tensRect.w = 6;
        tensRect.h = 0xD;
        if (i == 7) {
            BattleLoadImageAndSync(&tensRect, D_800D2DB0);
            onesRect.x = (i % 2) * 16 + 0x3C2;
            onesRect.y = (i / 2) * 16 + 0x102;
            onesRect.w = 6;
            onesRect.h = 0xD;
            pData = D_800D2DB0;
        } else {
            tensDigit = arg2[i] / 10;
            BattleLoadImageAndSync(&tensRect, tensDigit != 0 ? D_800C3E5C[tensDigit] : D_800D2DB0);
            onesRect.x = (i % 2) * 16 + 0x3C2;
            onesRect.y = (i / 2) * 16 + 0x102;
            onesRect.w = 6;
            onesRect.h = 0xD;
            onesDigit = arg2[i] % 10;
            pData = arg1[i] != 0xFF ? D_800C3E5C[onesDigit] : D_800D2DB0;
        }
        BattleLoadImageAndSync(&onesRect, pData);
    }
    func_80092298(arg0, arg1);
    for (i = 0; i < 8; i++) {
        HeapFree(pBufs[i]);
    }
    HeapFree(D_800D2DB0);
    BattleDrawSlotStatusPanel();
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092B74);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800930AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093578);
/* 83.74%: the whole shape is derived and reproduces -- the leading
   BattleSetPolyFT4Rect at 0x140 (x 0x7C, y 0xA4, v = (u8)(arg1 * 0x10), dx 0x60,
   dy 0x10), the flags word g_BattleActionRows[arg0 * 0x690 + (arg1 + 0x25) * 0x28],
   the 0x4000/0x1000 row pick (9/8/7), the 3-case switch on flags & 0xF
   (0xC/0xD/0x15), the two BattleSetPolyFT4RectH13 blocks at 0x320 and 0x370 with
   u/v/h from D_800D2F6A/D_800D2F6B/g_BattleActionRowIconSprites[ofs] and the two palette stores at
   +0x32E/+0x37E selected by D_800D2F69[ofs]. Same family as misc16_r2's
   func_800904A0 (73.99 there, parked), which is where the structure came from.
   Leftovers, 720 bytes against retail's 692: (1) gcc folds 0x25 * 0x28 into the
   lhu's reloc addend (+0x5C8) where retail materialises arg1 + 0x25 first;
   (2) retail keeps arg0 in s1 and arg1 in s0, here the reverse; (3) the switch's
   compare tree is ==1 / <2 / ==2 in retail and a longer chain here.
   Not chased further: an 8-attempt body of this size is a permuter job.
   Un-parking needs no new externs (g_BattleActionRows, g_BattleActionRowIconSprites..6B,
   g_SystemPalette2 are already declared at the top of this file). */
/*
void func_80093578(u8 arg0, u8 arg1) {
    u8 *pRec;
    u16 flags;
    u16 pal;
    s32 row;
    s32 col;
    s32 ofs;

    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                     ((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x140),
                         0x7C, 0xA4, 0, (u8)(arg1 * 0x10), 0x60, 0x10);
    flags = *(u16 *)(g_BattleActionRows + arg0 * 0x690 + (arg1 + 0x25) * 0x28);
    if (flags & 0x4000) {
        row = 9;
    } else if (flags & 0x1000) {
        row = 8;
    } else {
        row = 7;
    }
    switch (flags & 0xF) {
    case 0:
        col = 0xC;
        break;
    case 1:
        col = 0xD;
        break;
    case 2:
        col = 0x15;
        break;
    }
    ofs = row * 4;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                         (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x320)),
                            0x24, 0xA6, D_800D2F6A[ofs], D_800D2F6B[ofs], g_BattleActionRowIconSprites[ofs]);
    ofs = col * 4;
    pRec = g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28;
    if (D_800D2F69[ofs] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                         (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x370)),
                            0x48, 0xA6, D_800D2F6A[ofs], D_800D2F6B[ofs], g_BattleActionRowIconSprites[ofs]);
    pRec = g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28;
    if (D_800D2F69[col * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}
*/
/* 99.04% (412 bytes against retail's 416): the array-extern spelling from
   func_8009209C closed the &g_BattlePrimStartIndex wall here too -- retail's
   `lui s3/addu s3` + `lw 0(s3)` per call is reproduced exactly, and every other
   instruction is identical. The whole remainder is the one `andi s0,s0,0xffff`
   retail emits at index's FIRST use (the truncation of the u16 local):
   gcc 2.6.0 proves the range (u8 load * 4 + u8, * 2 -> <= 2550) and drops the
   mask, so the body is 4 bytes short and cannot link. Tried: u16 vs s32 vs u32
   index, a `u32 value` intermediate, the 3-statement `index = ...*4; index +=
   arg1; index *= 2;` form (98.80), and declaring GetStringEntry's 2nd parameter
   `u16` (the second call's `ori a1,s0,1` proves it is not a parameter conversion,
   and that spelling masks neither call). */
/*
void func_8009382C(u8 arg0, u8 arg1) {
    RECT rect;
    u8 *pBuf;
    u16 index;
    s32 len1;
    s32 len2;

    index = (g_BattleSlotTextIndex[arg0 * 0x170] * 4 + arg1) * 2;
    pBuf = (u8 *)BattleAllocTextBuffer(0x39);
    bzero(pBuf, 0x618);
    len1 = SystemRenderStringEntry(GetStringEntry(D_800C3DE8, index), pBuf, 0x39, 0);
    len2 = SystemRenderStringEntry(GetStringEntry(D_800C3DE8, index | 1), pBuf, 0x39, 1);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, pBuf);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x280)), 0x88, 0xB6, 0, 0, len1 & 0xFF);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x2D0)), 0x88, 0xC6, 0, 0, len2 & 0xFF);
    HeapFree(pBuf);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009382C);
// 100.0% under gcc 2.6.0-psx (99.81/99.76/99.90 under this unit's 2.7.2): this
// function wants a 2.6.0 TU of its own; the unit as a whole is a 2.7.2 unit.
void BattleDrawCharStatusIfMaskUnset(u8 arg0, u8 arg1) {
    u16 tbl[4];

    tbl[0] = D_800C324E;
    tbl[1] = D_800C3250;
    tbl[2] = D_800C3252;
    tbl[3] = D_800C323A;
    if ((u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].statusFlags,
                           arg1)) {
        if ((g_BattleWorkBlock[arg0].conditionFlags & tbl[arg1]) == 0) {
            func_80093578(arg0, arg1);
            func_8009382C(arg0, arg1);
            g_BattleGraphicsWork->pPacketPrims[0x66A] = g_BattlePrimStartIndex[0];
            g_BattleGraphicsWork->pPacketPrims[0x66B] = 1;
        } else {
            g_BattleGraphicsWork->pPacketPrims[0x66B] = 0;
        }
    } else {
        g_BattleGraphicsWork->pPacketPrims[0x66B] = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093B08);
void BattleClearSlotFlagsIfChar4(u8 arg0, u8 arg1) {
    if (g_BattleActorCharIds[arg0] == 4) {
        g_BattleDisplayState->unkB7 = 0;
        g_BattleDisplayState->unkB0 = 0;
        if (arg1 != 0) {
            BattleReleaseEffectSlotIfChar4();
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800941A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800946F4);
void BattleAccumulateSlotValueByRowFlag(void) {
    BattleWorkBlock *pWork = g_BattleCurWorkBlock;
    u8 slot;

    for (slot = 0; slot < sizeof(pWork->slots) / sizeof(pWork->slots[0]); slot++) {
        if (pWork->slotStates[slot] != 0) {
            continue;
        }
        if (pWork->slots[slot].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
            pWork->gearAmounts[slot] += pWork->slotAmounts[g_BattleCurSlotIndex];
        } else {
            pWork->characterAmounts[slot] += pWork->slotAmounts[g_BattleCurSlotIndex];
        }
    }
}
void BattleRestoreSlotConditionFlags(void) {
    u8 i;
    u8 n1;
    u8 n2;

    n1 = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_BattleCurWorkBlock->slots[i].activeEffectFlags1 & 0xC000) {
            n1++;
        }
    }
    n2 = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_BattleCurWorkBlock->slots[i].activeEffectFlags1 & 2) {
            n2++;
        }
    }
    if (n1 == 1 && n2 == 2) {
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            if (g_BattleCurWorkBlock->slots[i].activeEffectFlags1 & 2) {
                g_BattleCurWorkBlock->slots[i].activeEffectFlags1 &= ~2;
                g_BattleCurWorkBlock->slots[i].conditionFlags = g_BattleSavedSlotUnk7A[i];
            }
        }
    }
    if (n1 == 2 && n2 == 1) {
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            if (g_BattleCurWorkBlock->slots[i].activeEffectFlags1 & 2) {
                g_BattleCurWorkBlock->slots[i].activeEffectFlags1 &= ~2;
                g_BattleCurWorkBlock->slots[i].conditionFlags = g_BattleSavedSlotUnk7A[i];
            }
        }
    }
}
void BattleComputeActionAmount(void) {
    BattleTableEntry *pAtk;
    BattleTableEntry *pDef;
    u16 atkValue;
    u16 defValue;
    s8 command;
    s32 amountSource;
    s32 rate;
    s32 amount;
    s32 mulAtk;
    s32 mulDef;
    u32 blocked;

    rate = g_BattleCurrentRow->power;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        blocked = g_BattleTargetSlotEntry->blockFlags & 0x4000;
    } else {
        blocked = g_BattleTargetSlotEntry->blockFlags & 0x8000;
    }
    if (blocked != 0) {
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        return;
    }
    command = func_80096AB8();
    atkValue = func_80096FBC();
    defValue = func_80097610();
    pAtk = g_BattleActingSlot;
    if ((pAtk->activeEffectFlags4 | pAtk->accessoryEffectFlags4) & 8) {
        atkValue += atkValue / 5;
    }
    if ((pAtk->activeEffectFlags4 | pAtk->accessoryEffectFlags4) & 2) {
        atkValue += atkValue / 10;
    }
    if ((pAtk->activeEffectFlags4 | pAtk->accessoryEffectFlags4) & 4) {
        atkValue -= atkValue / 5;
    }
    if ((pAtk->activeEffectFlags4 | pAtk->accessoryEffectFlags4) & 1) {
        atkValue -= atkValue / 10;
    }
    pDef = g_BattleTargetSlotEntry;
    if ((pDef->activeEffectFlags4 | pDef->accessoryEffectFlags4) & 4) {
        defValue += defValue / 5;
    }
    if ((pDef->activeEffectFlags4 | pDef->accessoryEffectFlags4) & 1) {
        defValue += defValue / 10;
    }
    if ((pDef->activeEffectFlags4 | pDef->accessoryEffectFlags4) & 8) {
        defValue -= defValue / 5;
    }
    if ((pDef->activeEffectFlags4 | pDef->accessoryEffectFlags4) & 2) {
        defValue -= defValue / 10;
    }
    if (g_BattleCurrentRow->element & 0x10) {
        if (!(g_BattleTargetSlotEntry->accessoryEffectFlags2 & 0x40)) {
            g_BattleTargetSlotEntry->activeEffectFlags2 |= 0x40;
        }
        if ((g_BattleActingSlot->activeEffectFlags7 | g_BattleActingSlot->accessoryEffectFlags7) & 0x4000) {
            g_BattleCurveTargetModes[g_BattleTargetSlotIndex] = 3;
            g_BattleCurveTargets[g_BattleTargetSlotIndex] = (u16)(g_BattleActingSlot->maxMp / 10) * 2;
        }
        if ((g_BattleActingSlot->activeEffectFlags7 | g_BattleActingSlot->accessoryEffectFlags7) & 0x1000) {
            g_BattleCurveTargetModes[g_BattleTargetSlotIndex] = 2;
            g_BattleCurveTargets[g_BattleTargetSlotIndex] = (u16)(g_BattleActingSlot->maxHp / 10) * 2;
        }
    }
    if (g_BattleCurrentRow->element & 0x20) {
        if (!(g_BattleTargetSlotEntry->accessoryEffectFlags2 & 0x80)) {
            g_BattleTargetSlotEntry->activeEffectFlags2 |= 0x80;
        }
    }
    if (g_BattleTargetSlotEntry->activeEffectFlags2 & 0x40) {
        defValue -= defValue >> 2;
        g_BattleTargetSlotEntry->activeEffectFlags2 &= ~0x40;
    }
    if (g_BattleActingSlot->activeEffectFlags2 & 0x80) {
        atkValue -= atkValue >> 2;
        g_BattleActingSlot->activeEffectFlags2 &= ~0x80;
    }
    if (g_BattleCurrentRow->actionFlags & 0x400) {
        rate = 0x14;
    }
    func_80096494(&atkValue, &defValue, &command);
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        mulAtk = 5;
        mulDef = 4;
    } else {
        mulAtk = 4;
        mulDef = 3;
    }
    if (defValue != 0) {
        amount = mulAtk * atkValue - mulDef * defValue;
    } else {
        amount = mulAtk * atkValue;
    }
    amountSource = g_BattleCurrentRow->amountSource;
    // Nested rather than "amountSource >= 0 && amountSource < 2": gcc folds a
    // && range test into a single unsigned compare.
    if (amountSource >= 0) {
        if (amountSource < 2) {
            amount = rate * amount / 20;
        }
    }
    if (amount <= 0) {
        amount = 0;
    } else if (amount < 10) {
        amount += rand() % 2;
    } else {
        amount += rand() % (amount / 10 + 2);
    }
    switch (command) {
    case 1:
        if (amount > 0) {
            g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        } else {
            amount = 1;
            g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        }
        break;
    case 2:
        amount /= 2;
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 5;
        break;
    case 3:
        amount = 0;
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 4;
        break;
    case 4:
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
        break;
    case 5:
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 7;
        break;
    }
    if (g_BattleAmountThirdFlag != 0) {
        if ((g_BattleCurrentRow->actionFlags & 0x100) && amount != 0) {
            amount /= 3;
        }
    }
    if (amount > BATTLE_ACTION_AMOUNT_MAX) {
        amount = BATTLE_ACTION_AMOUNT_MAX;
    }
    if (amount < 0) {
        amount = 0;
    }
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = amount;
}
void BattleSetSlotState2FromActorField5BWithModifiers(void) {
    BattleTableEntry *p = g_BattleActingSlot;
    BattleRow *q = g_BattleCurrentRow;
    BattleTableEntry *r;
    s16 v;
    u16 f;

    v = p->ether * q->power;
    if (p->accessoryEffectFlags4 & 0x2000) {
        v *= 2;
    }
    r = g_BattleTargetSlotEntry;
    f = r->activeEffectFlags7 | r->accessoryEffectFlags7;
    if (f & 0x100) {
        v = 7 * v / 10;
    }
    if (f & 0x200) {
        v = 13 * v / 10;
    }
    if (g_BattleWorkBlock[g_BattleCurSlotIndex].state.flags & 0x80) {
        v = 0;
    }
    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = v;
}
void BattleInitActiveSlotAction(void) {
    s32 t;
    BattleTableEntry *p;
    s32 *pCurveTarget;
    s32 v;

    if (g_BattleWorkBlock[g_BattleCurSlotIndex].characterId == 2) {
        BattleSetSlotEntryFieldsTo2000(g_BattleCurSlotIndex, 1);
    }
    t = g_BattleCurSlotIndex;
    g_BattleTargetSlotEntry->activeEffectFlags1 = 0;
    g_BattleTargetSlotEntry->activeEffectFlags2 = 0;
    g_BattleTargetSlotEntry->activeEffectFlags3 = 0;
    g_BattleTargetSlotEntry->activeEffectFlags4 = 0;
    g_BattleTargetSlotEntry->activeEffectFlags7 = 0;
    g_BattleCurveTargetModes[t] = 2;
    p = g_BattleTargetSlotEntry;
    v = p->maxHp * g_BattleCurrentRow->power;
    if (g_BattleCurSlotIndex) { // empty: makes gcc share this load with the one below ($v1, not $a1)
    }
    t = g_BattleCurSlotIndex;
    // A pointer, not g_BattleCurveTargets[t]: the indexed form schedules `1 << t` into the
    // load's delay slot, which drops retail's nop (4 bytes short).
    pCurveTarget = g_BattleCurveTargets + t;
    *pCurveTarget = v / 10;
    g_BattleCommandCellFlags |= 1 << t;
}
void BattleClearStatusWordsOnAction6E(void) {
    if (rand() % 100 > g_BattleCurrentRow->chance) {
        return;
    }
    if (g_BattleCurrentRow->conditionKind != 0x6E) {
        return;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x8000) {
        g_BattleTargetSlotEntry->activeEffectFlags3 = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x4000) {
        g_BattleTargetSlotEntry->accessoryEffectFlags3 = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x2000) {
        g_BattleTargetSlotEntry->activeEffectFlags4 = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x1000) {
        g_BattleTargetSlotEntry->accessoryEffectFlags4 = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x800) {
        g_BattleTargetSlotEntry->activeEffectFlags7 = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x400) {
        g_BattleTargetSlotEntry->accessoryEffectFlags7 = 0;
    }
    g_BattleCurWorkBlock->commandCode = 0x3A;
}
