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
extern BattleSpriteWork *g_BattleDisplayState;
extern u8 g_BattleCurSlotIndex;
extern BattleWorkBlock *g_BattleCurWorkBlock;
extern BattleGraphicsWork *g_BattleGraphicsWork;
extern s32 g_BattlePrimStartIndex;
extern BattleRow *g_BattleCurrentRow;
extern BattleTableEntry *D_800C3E34;
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleCurveTargets[];
extern u8 g_BattleCurveTargetModes[];
extern u16 D_800D2C9E;
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern BattleTableEntry *g_BattleActingSlot;
extern u8 g_BattleSlotFlags[];
extern u8 D_800C3CFA[];
extern u16 g_SystemPalette1;
extern u8 g_BattleSlotFlags[];
extern void *D_800D367C;

void BattleReleaseEffectSlotIfChar4(void);
void BattleSetSlotEntryFieldsTo2000(u8, s32);
void func_800916D4(u8, u8, s32);
void BattleParseNumberToDigits(u32);
void BattleSetPolyFT4RectH13(POLY_FT4 *, s16, s16, u8, u8, s16);
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
extern u8 D_800CCD88[];
extern u8 D_800C3E04;
extern u8 D_800D2DC4;
extern s32 func_80096AB8(void);
extern u16 func_80096FBC(void);
extern u16 func_80097610(void);
extern void func_80096494(u16 *, u16 *, u8 *);

// The largest amount the current action may deal; the tail of func_80094EE4 clamps to it.
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
    g_BattleDisplayState->spriteColumnPrimStart = *(u8 *)&g_BattlePrimStartIndex;
    g_BattleDisplayState->spriteColumnPending = 1;
}
/* 86.04%: the s/2 and s-s/2*2 expansion, both UV args and the pointer sum are
   insn-identical; what is left is expansion order (gcc hoists `li a1,0x20` /
   `li a2,0xA4` to the top here, the original materialises them after the UV
   temps) and the arg1 narrowing landing in v0 instead of in place in a1.
   Tried: %2 vs the explicit s-s/2*2 form, u32 vs u8 temps for the quotient,
   pointer sum in both operand orders.
   Re-screened: best 91.42 under gcc-2.6.0 (76.76 under cdk). There the left-
   overs are different and partly source level - 2.6.0 narrows arg1 with
   `sll/sra` where the original has `andi a1,s1,0xffff` (a signed-type hint),
   and it keeps `li a1,0x20`/`li a2,0xA4` before the s/2*2 chain, not after. */
/*
void func_800916D4(u8 arg0, u8 arg1, s32 arg2) {
    u32 s;
    u8 c;
    s32 i;
    u16 k;

    s = arg1 * 2 + arg0;
    BattleSetPolyFT4Rect(
        (POLY_FT4 *)(g_BattlePrimStartIndex * 0x28 + 0x140 +
                     (s32)(*(u8 **)(g_BattleGraphicsWork + 0xA230))),
        0x20, 0xA4, (s - s / 2 * 2) * 0x78 & 0xF8, s / 2 * 16, 0x60, 0x10);
    (*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x66E] = 0;
    if (g_BattleActorCharIds[(u8)arg2] == 1) {
        BattleParseNumberToDigits(func_8009A258((u8)arg2, s + 0x16));
        for (i = 0, k = 0x8C; i < 3; i++, k += 8) {
            c = D_800C3CFA[i];
            if (c == 0xFF) {
                continue;
            }
            BattleSetPolyFT4RectH13(
                (POLY_FT4 *)((*(u8 **)(g_BattleGraphicsWork + 0xA230)) +
                             (*(u8 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + 0x66E) *
                                  2 +
                              g_BattlePrimStartIndex) *
                                 0x28 +
                             0x190),
                k, 0xA6, (c * 8 + 0x78) & 0xF8, 0, 8);
            (*(u8 **)(g_BattleGraphicsWork + 0xA230))[0x66E]++;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800916D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009187C);
/* 87.45%: the whole shape (index branch, RECT, two string draws, two
   BattleSetPolyFT4RectH13 calls, HeapFree) reproduces; the leftovers are the
   &g_BattlePrimStartIndex wall (retail holds `lui s3/addiu s3` once and does `lw 0(s3)`
   twice, gcc rematerialises `lui/lw` per call) and the `andi s0,s0,0xffff`
   retail emits at index's first use (gcc proves the u8-derived range here and
   drops it). Also tried: the (arg2*2+arg1)*2 sum in a local before the branch
   (74.68 - it hoists the whole vofs computation to the top of the frame). */
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
    BattleSetPolyFT4RectH13((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x280), 0x30, 0xB6, 0, 0, len1 & 0xFF);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x2D0), 0x30, 0xC6, 0, 0, len2 & 0xFF);
    HeapFree(pBuf);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091B38);
void BattleDrawCharStatusIfFlagged(u8 arg0, char arg1, u8 arg2) {
    u8 ret = 0;

    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        ret = (u16)BattleMaskHasStatusBit(
                  (u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].unk6,
                  arg1 + arg2 * 2) != 0;
    } else if ((u16)BattleMaskHasStatusBit(
                   (u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].unkA,
                   arg1 + arg2 * 2) != 0) {
        ret = 1;
    }
    if (ret != 0) {
        func_800916D4(arg1, arg2, arg0);
        func_8009187C(arg0, arg1, arg2);
        func_80091B38(arg0, arg1, arg2);
        g_BattleGraphicsWork->pPacketPrims[0x66A] = *(u8 *)&g_BattlePrimStartIndex;
        g_BattleGraphicsWork->pPacketPrims[0x66B] = 1;
    } else {
        g_BattleGraphicsWork->pPacketPrims[0x66B] = 0;
    }
}
/* 64.13%: shape is right (two palette stores, four BattleSetPolyFT4Rect calls with
   the arg0/0x60/0x10 stack args, trailing 0x668 store) but retail keeps
   `&g_BattlePrimStartIndex` in s0 (`lui/addiu` once) where gcc rematerialises
   `lui/lw g_BattlePrimStartIndex` at each use and a `lui/lbu` at the end; that also costs
   the 4th saved register (frame 56 vs 48) and swaps the s1/s2 roles of
   arg0/0x60. Tried: plain g_BattlePrimStartIndex (64.13), `s32 *pIndex = &g_BattlePrimStartIndex`,
   a `u8 *pPal` plus an s32 cache of it (59.64, and the body grows). This is
   misc16's documented &g_BattlePrimStartIndex wall. */
/*
void func_80091EC4(u8 arg0) {
    *(u16 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0xE) = g_SystemPalette1;
    *(u16 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x5E) = g_SystemPalette1;
    BattleSetPolyFT4Rect((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28), 0x34, 0x34, 0, arg0, 0x60, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x50), 0xB8, 0x34, 0x78, arg0, 0x60, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0xA0), 0x9C, 0x34, 0, arg0, 0x10, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0xF0), 0x120, 0x34, 0x40, arg0, 0x10, 0x60);
    *(u8 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + 0x668) = *(u8 *)&g_BattlePrimStartIndex;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091EC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009209C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092298);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092784);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092B74);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800930AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093578);
/* 72.03%: RECT/bzero/GetStringEntry/SystemRenderStringEntry order, both stack
   args and the frame all match. Same blocker as misc16's BattleDrawMenuEntryString
   (78.72%): the original loads *(g_BattleGraphicsWork+0xA230) FIRST, keeps &g_BattlePrimStartIndex
   in a saved reg (lw 0(s3)) and does `addiu a0,a0,0x280` before the base addu;
   gcc loads the g_BattlePrimStartIndex index first, re-forms its address per call and sinks
   the addiu into the jal delay slot. Improves to 74% with the index in a local.
   Re-screened: best 84.56 under gcc-2.6.0 (71.40 under cdk). There the frame
   is 56 vs 2.6.0's 48 - the original spills the &g_BattlePrimStartIndex index into s3,
   2.6.0 does not re-form it - and the addiu/addu order of the +0x280/+0x2D0
   adds is still swapped. */
/*
void func_8009382C(u8 arg0, u8 arg1) {
    RECT rect;
    u8 *pBuf;
    u16 index;
    s32 len1;
    s32 len2;

    index = (D_800CCD88[arg0 * 0x170] * 4 + arg1) * 2;
    pBuf = (u8 *)BattleAllocTextBuffer(0x39);
    bzero(pBuf, 0x618);
    len1 = SystemRenderStringEntry(GetStringEntry(D_800C3DE8, index), pBuf, 0x39, 0);
    len2 = SystemRenderStringEntry(GetStringEntry(D_800C3DE8, index | 1), pBuf, 0x39, 1);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, pBuf);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x280),
                  0x88, 0xB6, 0, 0, len1 & 0xFF);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x2D0),
                  0x88, 0xC6, 0, 0, len2 & 0xFF);
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
        if ((g_BattleWorkBlock[arg0].unk7A & tbl[arg1]) == 0) {
            func_80093578(arg0, arg1);
            func_8009382C(arg0, arg1);
            g_BattleGraphicsWork->pPacketPrims[0x66A] = *(u8 *)&g_BattlePrimStartIndex;
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
/* 99.77%: instruction multiset identical; the only difference is the operand
   order of one addu for i*0x170 + pBase (original base-first, gcc index-first).
   Tried: array-ref, pointer-plus (this version), integer base and a pointer
   local for that sum; also 2.7.2-cdk -> 99.77. */
void BattleAccumulateSlotValueByRowFlag(void) {
    u8 *pBase = (u8 *)g_BattleCurWorkBlock;
    u8 *pOther = (u8 *)(g_BattleCurSlotIndex * 4 + (u32)pBase);
    u8 *pRow;
    u8 i;

    for (i = 0; i < 3; i++) {
        if (((u8 *)(pBase + i))[0x5FA0] != 0) {
            continue;
        }
        pRow = pBase + i * 0x170;
        if (pRow[0x15A] & 0x80) {
            *(u32 *)(i * 4 + (s32)pBase + 0x5F60) += *(u32 *)(pOther + 0x5F6C);
        } else {
            *(u32 *)(i * 4 + (s32)pBase + 0x5F54) += *(u32 *)(pOther + 0x5F6C);
        }
    }
}
/* 91.61% first attempt: the whole shape is right (two count loops, the
   if/else-if pair, the two duplicated fix loops, u8 counters masked with
   0xFF). Left to close: the beqz delay slot (the original puts `i++` there,
   gcc leaves a nop and emits it after the counter increment) and the
   base/index order of the row-pointer addu in the two fix loops
   (original `addu a0,t1,v1` base-first, gcc `addu v1,a3,v1`). Making the
   count loops use a row pointer fixed their addu order (91.47 -> 91.61).
   Tried: p[0x3E] vs the full expression, one u16 *p reused vs re-declared
   per loop. Un-parking needs `extern u16 D_800C3AA4[];`.
   Re-screened: best 96.96 under gcc-2.6.0 (90.04 under cdk). There only the
   two count loops' registers differ (n1 in a2 vs t0, base in a0 vs a3), both
   row addu's are operand-swapped, and one `j`/`nop` pair is left over. */
/*
void func_80094D24(void) {
    u8 i;
    u8 n1;
    u8 n2;
    u16 *p;

    for (i = 0, n1 = 0; i < 3; i++) {
        p = (u16 *)(g_BattleCurWorkBlock + i * 0x170);
        if (p[0x3E] & 0xC000) {
            n1++;
        }
    }
    for (i = 0, n2 = 0; i < 3; i++) {
        p = (u16 *)(g_BattleCurWorkBlock + i * 0x170);
        if (p[0x3E] & 2) {
            n2++;
        }
    }
    if (n1 == 1 && n2 == 2) {
        for (i = 0; i < 3; i++) {
            p = (u16 *)(g_BattleCurWorkBlock + i * 0x170);
            if (p[0x3E] & 2) {
                p[0x3E] &= ~2;
                p[0x3D] = D_800C3AA4[i];
            }
        }
    } else if (n1 == 2 && n2 == 1) {
        for (i = 0; i < 3; i++) {
            p = (u16 *)(g_BattleCurWorkBlock + i * 0x170);
            if (p[0x3E] & 2) {
                p[0x3E] &= ~2;
                p[0x3D] = D_800C3AA4[i];
            }
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094D24);
/* 99.86%, 1964 bytes = retail's size: the whole body (the action-flag block tests,
   the four +/- 1/5 and 1/10 status scalings on each side, the element/0x4000/0x1000
   curve-target writes, func_80096494, the 5/4 vs 4/3 rate pair, the two rand rolls,
   the five-case jump-table switch, the /3, the 9999 clamp and the slotAmounts store)
   reproduces instruction for instruction. Two things are left, both worth 4 bytes:
   1) retail loads amountSource with lbu and then compares it SIGNED (bltz + slti 2 +
      beqz, two jumps to the same label, 3 insns); every unsigned spelling folds to a
      single sltiu v0,v0,2 (-4 bytes) and every signed spelling (s8 n = field) gives
      retail's 3-insn shape but a sign-extending lb load instead of lbu. Tried the
      && form, nested ifs, > -1 && < 2, (s8) casts at each use and an s8 local.
   2) the switch table is 5 words plus a trailing 0 pad word (unit .rodata 0x880..0x898);
      C emits 20 bytes, so the pad needs the config/battle.yaml carve misc23 already has
      ( - [0x894, rodata] ) before this can link; the supervisor approved it, but it is
      moot until (1) closes, so the stub still owns the table.
   Un-parking needs these externs: D_800C3E04, D_800D2DC4, func_80096AB8, func_80096FBC,
   func_80097610, func_80096494 (u16 *, u16 *, u8 *). A const pad in this unit cannot work:
   -G8 puts a 4-byte const in .sdata and maspsx then fails on the object.
void func_80094EE4(void) {
    BattleTableEntry *pAtk;
    BattleTableEntry *pDef;
    u16 atkValue;
    u16 defValue;
    s8 command;
    s8 n;
    s32 rate;
    s32 amount;
    s32 mulAtk;
    s32 mulDef;
    u32 blocked;

    rate = g_BattleCurrentRow->power;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        blocked = *(u16 *)((u8 *)D_800C3E34 + 0x34) & 0x4000;
    } else {
        blocked = *(u16 *)((u8 *)D_800C3E34 + 0x34) & 0x8000;
    }
    if (blocked != 0) {
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        return;
    }
    command = func_80096AB8();
    atkValue = func_80096FBC();
    defValue = func_80097610();
    pAtk = g_BattleActingSlot;
    if ((pAtk->unk88 | pAtk->unk8A) & 8) {
        atkValue += atkValue / 5;
    }
    if ((pAtk->unk88 | pAtk->unk8A) & 2) {
        atkValue += atkValue / 10;
    }
    if ((pAtk->unk88 | pAtk->unk8A) & 4) {
        atkValue -= atkValue / 5;
    }
    if ((pAtk->unk88 | pAtk->unk8A) & 1) {
        atkValue -= atkValue / 10;
    }
    pDef = D_800C3E34;
    if ((pDef->unk88 | pDef->unk8A) & 4) {
        defValue += defValue / 5;
    }
    if ((pDef->unk88 | pDef->unk8A) & 1) {
        defValue += defValue / 10;
    }
    if ((pDef->unk88 | pDef->unk8A) & 8) {
        defValue -= defValue / 5;
    }
    if ((pDef->unk88 | pDef->unk8A) & 2) {
        defValue -= defValue / 10;
    }
    if (g_BattleCurrentRow->element & 0x10) {
        if (!(D_800C3E34->unk82 & 0x40)) {
            D_800C3E34->unk80 |= 0x40;
        }
        if ((g_BattleActingSlot->unk8C | g_BattleActingSlot->unk8E) & 0x4000) {
            g_BattleCurveTargetModes[D_800C3E04] = 3;
            *(s32 *)(D_800C3E04 * 4 + g_BattleCurveTargets) = (u16)(g_BattleActingSlot->maxMp / 10) * 2;
        }
        if ((g_BattleActingSlot->unk8C | g_BattleActingSlot->unk8E) & 0x1000) {
            g_BattleCurveTargetModes[D_800C3E04] = 2;
            *(s32 *)(D_800C3E04 * 4 + g_BattleCurveTargets) = (u16)(g_BattleActingSlot->maxHp / 10) * 2;
        }
    }
    if (g_BattleCurrentRow->element & 0x20) {
        if (!(D_800C3E34->unk82 & 0x80)) {
            D_800C3E34->unk80 |= 0x80;
        }
    }
    if (D_800C3E34->unk80 & 0x40) {
        defValue -= defValue >> 2;
        D_800C3E34->unk80 &= ~0x40;
    }
    if (g_BattleActingSlot->unk80 & 0x80) {
        atkValue -= atkValue >> 2;
        g_BattleActingSlot->unk80 &= ~0x80;
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
    n = g_BattleCurrentRow->amountSource;
    if (n >= 0) {
        if (n < 2) {
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
    if (D_800D2DC4 != 0) {
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
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094EE4);
void BattleSetSlotState2FromActorField5BWithModifiers(void) {
    BattleTableEntry *p = g_BattleActingSlot;
    BattleRow *q = g_BattleCurrentRow;
    BattleTableEntry *r;
    s16 v;
    u16 f;

    v = p->ether * q->power;
    if (p->unk8A & 0x2000) {
        v *= 2;
    }
    r = D_800C3E34;
    f = r->unk8C | r->unk8E;
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
    s32 v;

    if (g_BattleWorkBlock[g_BattleCurSlotIndex].characterId == 2) {
        BattleSetSlotEntryFieldsTo2000(g_BattleCurSlotIndex, 1);
    }
    t = g_BattleCurSlotIndex;
    D_800C3E34->unk7C = 0;
    D_800C3E34->unk80 = 0;
    D_800C3E34->unk84 = 0;
    D_800C3E34->unk88 = 0;
    D_800C3E34->unk8C = 0;
    g_BattleCurveTargetModes[t] = 2;
    p = D_800C3E34;
    v = p->maxHp * g_BattleCurrentRow->power;
    if (g_BattleCurSlotIndex) { // empty: makes gcc share this load with the one below ($v1, not $a1)
    }
    t = g_BattleCurSlotIndex;
    *(s32 *)(t * 4 + g_BattleCurveTargets) = v / 10;
    D_800D2C9E |= 1 << t;
}
void BattleClearStatusWordsOnAction6E(void) {
    if (rand() % 100 > g_BattleCurrentRow->chance) {
        return;
    }
    if (g_BattleCurrentRow->conditionKind != 0x6E) {
        return;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x8000) {
        D_800C3E34->unk84 = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x4000) {
        D_800C3E34->unk86 = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x2000) {
        D_800C3E34->unk88 = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x1000) {
        D_800C3E34->unk8A = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x800) {
        D_800C3E34->unk8C = 0;
    }
    if (g_BattleCurrentRow->conditionFlags & 0x400) {
        D_800C3E34->unk8E = 0;
    }
    g_BattleCurWorkBlock->commandCode = 0x3A;
}
