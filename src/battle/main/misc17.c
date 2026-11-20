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
    /* 0x09D */ u8 menuColumnPending;
    /* 0x09E */ u8 unk9E[0x7];
    /* 0x0A5 */ u8 spriteColumnPrimStart;
    /* 0x0A6 */ u8 menuColumnPrimStart;
    /* 0x0A7 */ u8 unkA7[0x9];
    /* 0x0B0 */ u8 unkB0;
    /* 0x0B1 */ u8 unkB1[0x6];
    /* 0x0B7 */ u8 unkB7;
    /* 0x0B8 */ u8 unkB8[0x40];
    /* 0x0F8 */ s32 spriteColumnPackageCount;
    /* 0x0FC */ s32 menuColumnPackageCount;
} BattleSpriteWork;

// The battle packet pool that g_BattleGraphicsWork->pPacketPrims points at, as far as
// this unit reaches into it: the two Gouraud quads the frame border is drawn into and
// the two flat quads its blanking rects use. The pool is 0x670 bytes.
typedef struct {
    /* 0x000 */ u8 unk0[0x5F0];
    /* 0x5F0 */ POLY_G4 gouraudQuads[2];
    /* 0x638 */ POLY_F4 flatQuads[2];
    /* 0x668 */ u8 unk668[0x8];
} BattlePacketPool; // Size: 0x670

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
extern u8 D_800C33B4[];  // 16 sprite ids, one per battle party column
extern s16 D_800C33C4[][2];
extern s16 D_800C3404[][2];

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
extern s32 BattleRollHitResult(void);
extern u16 func_80096FBC(void);
extern u16 BattleComputeSlotActionAmount(void);
extern void func_80096494(u16 *, u16 *, u8 *);
extern void BattleAllocPacketPrimitives(void);
extern void func_80076EA4(void);
extern s32 GetStringEntryFromBank(s32, s32);
extern void *GetStringEntryFromBank34(s32);
extern u_long *GetStringEntryFromBank12(s32);
extern u_long *g_BattleDigitImageBuffers[];
void func_80092298(u8, u8 *);
extern u_long *g_BattleTextWorkspace;
extern void bzero(void *, s32);
extern void HeapFree(void *);
extern u8 *g_BattleCommandQueue;
extern u8 D_800C3DE1[];
extern u8 *g_BattleCurSlotState;
extern BattleSlotState *g_BattleTargetSlotState;
extern u8 *D_800C3D60;
extern u16 g_BattleActionSlotMask;
extern u8 D_800CCD3E[];
extern void *D_800C348C[];
extern u8 D_800D2DB8;
extern u8 D_800C34AE;
void BattleClearAllSlotStates(void);
void BattleExecuteActionRow(void);
void BattleSetSlotEntryFieldsTo1(u8);
void BattleAccumulateSlotValueByRowFlag(void);
void BattleRollPercentAndSetState38(void);
void BattleRollPercentAndSwapActiveActor(void);
void BattleApplyActorConditionTimer(void);
void BattleApplyScriptConditionTimer(void);
void BattleSaveScriptCommandOperandsFromActor(void);
void BattleClearStatusWordsOnAction6E(void);
void func_800946F4(void);
extern void func_8008FC1C(s32, s32, s32, s32, u8);
extern void func_80090E7C(u8);
extern void BattleUpdateFrame(void);
extern void *GetStringEntryFromBank14(s32);
extern void *GetStringEntryFromBank30(s32);
extern u8 D_800CDD53[];
extern u8 D_800CEF23[];

// The largest amount the current action may deal; the tail of BattleComputeActionAmount clamps to it.
#define BATTLE_ACTION_AMOUNT_MAX 9999

// Number of action rows of one battle slot's status panel.
#define BATTLE_STATUS_ROW_COUNT 16

// The sprite packages BattleBuildSpriteColumn8 draws down one column, each 8 pixels below the last.
#define BATTLE_SPRITE_COLUMN_PACKAGE_COUNT 8

// The row's power is in twentieths: `power * amount / this` leaves the action's amount unchanged,
// and BattleComputeActionAmount forces exactly that rate on a row whose actionFlags has 0x400.
#define BATTLE_ACTION_POWER_FULL 20


/* 91.38% (1444 bytes against retail's 1440) under this unit's gcc 2.6.0. Every
   call, rectangle and store is retail's and the tail from func_80090E7C onwards is
   byte-identical; the residual is four bytes of register scheduling plus one hoist.
   Levers that were needed to get here:
   - the two 16-row scans read a byte of a 0x28-byte action row: the index must be
     built from two live locals (`rowOff` starting at 0x370 / 0x348 and stepped by
     0x28, `slotOff` = arg0 * 0x5F0 / arg0 * 0x690) summed into a third (`off`)
     before the load. That reproduces retail's `li s3,0x370`, `addu v0,s3,s5` and
     the `lui $at / addu $at,at,v0 / lbu %lo(at)` macro form. Any single-expression
     index, and `(i + 0x16) * 0x28`, makes gcc fold 0x370 into the symbol instead
     (it names D_800CE0C3 / D_800CF26B and loads with displacement 0): 89-91%.
   - the six upload rectangles must be six separate RECT locals, in this slot order:
     nameRect 0x18, tensRect 0x20, onesRect 0x28, tileRect 0x30, stripRect 0x38,
     blankRect 0x40. A `RECT rect[6]` array makes gcc keep `&rect[0]` in a register.
   - `(u16)BattleMaskHasStatusBit(mask, i) != 0` is what emits the `andi 0xffff`
     followed by `beqz` (a bare test of the s32 return gives sltu/bnez).
   What is left, all measured:
   - in both row scans gcc schedules the sum `off = rowOff + slotOff` one slot
     earlier and into $v1, where retail computes it after the `sb active[i]` into
     $v0; that shifts `move s1,zero`, `addiu s3,s3,40` and `addiu s0,s0,1` by one.
   - gcc hoists `&D_800CCD3E[arg0 * 0x170]` out of the third loop into a register
     (two extra instructions) where retail re-forms it per iteration through $at.
     Routing the index through a variable does fix the address form but needs $s7
     (89.48%).
   - gcc derives the `bufs` base from `&nameRect` (`addiu s3,sp,24; move s0,s3;
     lw a1,48(s0)`) where retail forms `addiu v0,sp,72; addu s0,v0,s2`.
   Tried as well: all five index orders, the cast and the array spelling of both row
   loads, a two-statement pointer split (pRow = base; pRow = pRow + idx), u8/u32
   index types, and a 3-minute permuter run whose best candidate hoists `arg0 * 8`
   into a `short` in the third loop and writes `i <= 16 - 1` (not ported). */
/*

void func_80091064(u8 arg0) {
    RECT nameRect;
    RECT tensRect;
    RECT onesRect;
    RECT tileRect;
    RECT stripRect;
    RECT blankRect;
    void *bufs[BATTLE_STATUS_ROW_COUNT];
    u8 active[BATTLE_STATUS_ROW_COUNT];
    u8 values[BATTLE_STATUS_ROW_COUNT];
    void *pBuf;
    u8 *pStr;
    u32 off;
    s32 rowOff;
    s32 slotOff;
    u8 index;
    s32 i;

    BattleAllocPacketPrimitives();
    func_80076EA4();
    func_8008FC1C(0x20, 0x30, 0x98, 0x38, 0xC);
    pBuf = BattleAllocTextBuffer(0x39);
    g_BattleTextWorkspace = pBuf;
    bzero(pBuf, 0x618);
    blankRect.x = 0x3C0;
    blankRect.y = 0;
    blankRect.w = 0x3C;
    blankRect.h = 0xD;
    BattleLoadImageAndSync(&blankRect, g_BattleTextWorkspace);
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        slotOff = arg0 * 0x5F0;
        rowOff = 0x370;
        for (i = 0; i < BATTLE_STATUS_ROW_COUNT; i++) {
            if ((u16)BattleMaskHasStatusBit(
                    (u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].characterConditionMask,
                    i) != 0) {
                active[i] = 1;
                off = rowOff + slotOff;
                values[i] = D_800CDD53[off];
            } else {
                active[i] = 0;
                values[i] = 0;
            }
            rowOff += 0x28;
        }
    } else {
        slotOff = arg0 * 0x690;
        rowOff = 0x348;
        for (i = 0; i < BATTLE_STATUS_ROW_COUNT; i++) {
            if ((u16)BattleMaskHasStatusBit(
                    (u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].gearConditionMask,
                    i) != 0) {
                active[i] = 1;
                off = rowOff + slotOff;
                values[i] = D_800CEF23[off];
            } else {
                active[i] = 0;
                values[i] = 0;
            }
            rowOff += 0x28;
        }
    }
    for (i = 0; i < BATTLE_STATUS_ROW_COUNT; i++) {
        pBuf = BattleAllocTextBuffer(0x1B);
        bufs[i] = pBuf;
        bzero(pBuf, 0x30C);
        tileRect.x = 0x380 + (i % 2) * 0x1E;
        tileRect.y = 0x100 + (i / 2) * 0x10;
        tileRect.w = 0x1B;
        tileRect.h = 0x10;
        BattleLoadImageAndSync(&tileRect, g_BattleTextWorkspace);
        if (active[i] != 0) {
            if (g_BattleSlotFlags[arg0 * 8] == 0) {
                pStr = GetStringEntryFromBank14(*(u8 *)(D_800CCD3E + arg0 * 0x170) * 0x10 + i);
            } else {
                pStr = GetStringEntryFromBank30(*(u8 *)(g_BattleSlotTextIndex + arg0 * 0x170) * 0x10 + i);
            }
            SystemRenderStringEntry(pStr, bufs[i], 0x1B, 0);
            nameRect.x = 0x380 + (i % 2) * 0x1E;
            nameRect.y = 0x102 + (i / 2) * 0x10;
            nameRect.w = 0x1E;
            nameRect.h = 0xD;
            BattleLoadImageAndSync(&nameRect, bufs[i]);
        }
        if (i % 2 == 0) {
            stripRect.x = 0x3C0;
            stripRect.y = 0x100 + (i / 2) * 0x10;
            stripRect.w = 0x1B;
            stripRect.h = 0x10;
            BattleLoadImageAndSync(&stripRect, g_BattleTextWorkspace);
        }
        tensRect.x = 0x3C0 + (i % 2) * 0x10;
        tensRect.y = 0x102 + (i / 2) * 0x10;
        tensRect.w = 6;
        tensRect.h = 0xD;
        index = values[i] / 10;
        BattleLoadImageAndSync(&tensRect, index != 0 ? g_BattleDigitImageBuffers[index] : g_BattleTextWorkspace);
        onesRect.x = 0x3C2 + (i % 2) * 0x10;
        onesRect.y = 0x102 + (i / 2) * 0x10;
        onesRect.w = 6;
        onesRect.h = 0xD;
        BattleLoadImageAndSync(&onesRect, active[i] != 0 ? g_BattleDigitImageBuffers[values[i] % 10] : g_BattleTextWorkspace);
    }
    func_80090E7C(arg0);
    for (i = 0; i < BATTLE_STATUS_ROW_COUNT; i++) {
        HeapFree(bufs[i]);
    }
    HeapFree(g_BattleTextWorkspace);
    BattleUpdateFrame();
    *(u16 *)(g_BattleGraphicsWork->pPacketPrims + 0x14E) = g_SystemPalette1;
    *(u16 *)(g_BattleGraphicsWork->pPacketPrims + 0x176) = g_SystemPalette1;
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->unkB7 = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091064);
void BattleBuildSpriteColumn8(s32 arg0) {
    s32 i;
    s32 offset;

    g_BattleDisplayState->spriteColumnPackageCount = 0;
    for (i = 0; i < BATTLE_SPRITE_COLUMN_PACKAGE_COUNT; i++) {
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
    s32 rowOff;
    s32 slotOff;

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
   the body is 4 bytes short. Round 323 probed this to a dead end (see the note
   under func_8009382C): 35 spellings of a bounded `u16 index` all fold, and the
   only operands that keep the mask are unbounded ones that cost an extra
   instruction. */
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
void BattleDrawCharStatusIfFlagged(u8 arg0, u8 arg1, u8 arg2) {
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
    // `zero` is a variable index and the two sums are written base - (-offset) on purpose: with a
    // literal [0] gcc folds the negation pair into negu/subu (83.45%) and with `prims + offset` it
    // swaps the addu operands (99.45%), both 100.00% only as written.
    u32 zero;

    zero = 0;
    ((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[zero] * 0x28))))->clut = g_SystemPalette1;
    ((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[zero] * 0x28)) + 0x50))->clut = g_SystemPalette1;
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[zero] * 0x28), 0x34, 0x34, 0, arg0, 0x60, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0x50)), 0xB8, 0x34, 0x78, arg0, 0x60, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0xA0)), 0x9C, 0x34, 0, arg0, 0x10, 0x60);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0xF0)), 0x120, 0x34, 0x40, arg0, 0x10, 0x60);
    g_BattleGraphicsWork->pPacketPrims[0x668] = g_BattlePrimStartIndex[0];
}
void BattleDrawSlotStatusPanel(void) {
    // `zero`, the two-view spelling of pPacketPrims (`ppPrims` for the second store only) and the
    // base - (-offset) sums are all measured: a literal index gives 87.61%, one view of the pool
    // pointer 91.00% and `prims + offset` 99.49%, 100.00% only as written.
    u32 zero;
    u8 **ppPrims;

    zero = 0;
    ppPrims = &g_BattleGraphicsWork->pPacketPrims;
    ((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[zero] * 0x28))))->clut = g_SystemPalette1;
    ((POLY_FT4 *)(*ppPrims - (-(((s32 *)g_BattlePrimStartIndex)[zero] * 0x28)) + 0x50))->clut = g_SystemPalette1;
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[zero] * 0x28), 0x36, 0x62, 0, 0, 0x60, 0x40);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0x50)), 0xC2, 0x62, 0x78, 0, 0x60, 0x40);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0xA0)), 0x96, 0x62, 0, 0, 0x10, 0x40);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[zero] * 0x28 + 0xF0)), 0x122, 0x62, 0x40, 0, 0x10, 0x40);
    g_BattleGraphicsWork->pPacketPrims[0x668] = g_BattlePrimStartIndex[0];
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->unkB7 = 3;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092298);
/* 96.97% at objdiff's 1260 bytes -- retail's EXACT size, and every instruction of
   the two quad-initialisation loops (the two Gouraud quads and the two flat quads)
   is retail's, including the per-store reload of g_BattleGraphicsWork->pPacketPrims
   and the `addu a0,s2,a0` of the SetPolyG4 argument. The whole remainder is the
   third loop's three registers: retail assigns $s0 to `i`, $s1 to the token cursor
   and $s2 to the &D_800C33C4 induction pointer, this body takes $s1 to `i`, $s2 to
   the cursor and $s0 to the pointer, which also puts retail's `addu at,at,s0` and
   `sll v0,s0,2` on the wrong registers. Measured: making D_800C33C4 a
   `s16 (*pColumns)[2]` local does reproduce retail's hoisted `lui/addiu` pointer
   shape and `i` in $s0, but costs 4 bytes (1264, 96.03%) and moves the cursor to
   $s3; the preheader order of `count = 0`, `pCursor = arg1` and an `i = 0;`
   statement before the for, and both the `(u8 *)work + (count * 0x50 + 0x1E68)`
   and `count * 0x50 + (0x1E68 + (s32)work)` spellings of the package base, all
   measure the same. What does reproduce loops 1/2: the store goes through
   `(i + <pool>->gouraudQuads)->r0` with the pool expression re-spelled for every
   store (a `pPool` local shares one register, loads into $a0 instead of $v0 and
   also breaks the SetPolyG4 argument's operand order), colours r0/g0/b0 and
   r1/g1/b1 are 0xFF,0,0 while r2/r3 are 0, then the eight x/y halves follow; the
   flat-quad loop sets only r0/g0/b0 and its eight x/y halves. The third loop reads
   g_BattleDisplayState->menuColumnPackageCount (0xFC), draws through
   BattleDrawSpritePackage into the 0x1E68 + count*0x50 package and sets
   menuColumnPrimStart/menuColumnPending (0xA6/0x9D) at the end; `if (*pCursor++ ==
   0xFF) continue;` is what produces retail's cursor bump in the beq delay slot,
   and the test skips only the draw, it does not leave the loop. The
   BattlePacketPool view struct and the D_800C33B4/D_800C33C4/D_800C3404 externs
   above are for this body. */
/*
void func_80092298(u8 arg0, u8 *arg1) {
    s32 i;
    s32 size;
    u8 *pCursor;

    for (i = 0; i < 2; i++) {
        SetPolyG4((i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads));
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->r0 = 0xFF;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->g0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->b0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->r1 = 0xFF;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->g1 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->b1 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->r2 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->g2 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->b2 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->r3 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->g3 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->b3 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->x0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->y0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->x1 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->y1 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->x2 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->y2 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->x3 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->gouraudQuads)->y3 = 0;
    }
    for (i = 0; i < 2; i++) {
        SetPolyF4((i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads));
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->r0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->g0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->b0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->x0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->y0 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->x1 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->y1 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->x2 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->y2 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->x3 = 0;
        (i + ((BattlePacketPool *)g_BattleGraphicsWork->pPacketPrims)->flatQuads)->y3 = 0;
    }
    g_BattleDisplayState->menuColumnPackageCount = 0;
    pCursor = arg1;
    i = 0;
    for (; i < 0x10; i++) {
        if (*pCursor++ == 0xFF) {
            continue;
        }
        g_BattleDisplayState->menuColumnPackageCount += BattleDrawSpritePackage(D_800C33B4[i],
            (u8 *)g_BattleGraphicsWork + (g_BattleDisplayState->menuColumnPackageCount * 0x50 + 0x1E68),
            D_800C33C4[i][0], D_800C3404[i][0]);
    }
    g_BattleDisplayState->menuColumnPrimStart = g_BattlePrimStartIndex[0];
    g_BattleDisplayState->menuColumnPending = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092784);
// 70.57% under this unit's gcc 2.6.0, at objdiff's 1028 bytes -- the same count
// as the original, so the body is the right LENGTH and every difference is a
// frame offset. The
// whole body reproduces retail instruction for instruction -- the 0x618 text
// buffer, the eight 0x1B x 0x10 tiles, the glyph blit shared by the i == 7 and
// the arg1[i] arms, the even-tile load, the tens/ones digit blits with the
// g_BattleDigitImageBuffers digit-buffer table -- and the only residual is the FRAME: retail is
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

    divisors[0] = 1000;
    divisors[1] = 100;
    divisors[2] = 10;
    BattleAllocPacketPrimitives();
    func_80076EA4();
    g_BattleTextWorkspace = BattleAllocTextBuffer(0x39);
    bzero(g_BattleTextWorkspace, 0x618);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, g_BattleTextWorkspace);
    for (i = 0; i < 8; i++) {
        pBufs[i] = BattleAllocTextBuffer(0x1B);
        bzero(pBufs[i], 0x30C);
        tileRect.x = (i % 2) * 0x1E + 0x380;
        tileRect.y = (i / 2) * 16 + 0x100;
        tileRect.w = 0x1B;
        tileRect.h = 0x10;
        BattleLoadImageAndSync(&tileRect, g_BattleTextWorkspace);
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
            BattleLoadImageAndSync(&secondRect, g_BattleTextWorkspace);
        }
        tensRect.x = (i % 2) * 16 + 0x3C0;
        tensRect.y = (i / 2) * 16 + 0x102;
        tensRect.w = 6;
        tensRect.h = 0xD;
        if (i == 7) {
            BattleLoadImageAndSync(&tensRect, g_BattleTextWorkspace);
            onesRect.x = (i % 2) * 16 + 0x3C2;
            onesRect.y = (i / 2) * 16 + 0x102;
            onesRect.w = 6;
            onesRect.h = 0xD;
            pData = g_BattleTextWorkspace;
        } else {
            tensDigit = arg2[i] / 10;
            BattleLoadImageAndSync(&tensRect, tensDigit != 0 ? g_BattleDigitImageBuffers[tensDigit] : g_BattleTextWorkspace);
            onesRect.x = (i % 2) * 16 + 0x3C2;
            onesRect.y = (i / 2) * 16 + 0x102;
            onesRect.w = 6;
            onesRect.h = 0xD;
            onesDigit = arg2[i] % 10;
            pData = arg1[i] != 0xFF ? g_BattleDigitImageBuffers[onesDigit] : g_BattleTextWorkspace;
        }
        BattleLoadImageAndSync(&onesRect, pData);
    }
    func_80092298(arg0, arg1);
    for (i = 0; i < 8; i++) {
        HeapFree(pBufs[i]);
    }
    HeapFree(g_BattleTextWorkspace);
    BattleDrawSlotStatusPanel();
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092B74);
/* 64.13% (1004 bytes against retail's 1336). Everything up to the quad geometry
   at the end is structurally right and measured: the two digit packages
   (0x67+tens behind `if (tens != 0)`, 0x67+ones), the seven-row loop
   (`g_BattleCommandQueue[i + 0x2C4] == 0xFF` breaks; `D_800C3DE1[i - 1] + 0x39`
   for the first sprite and a bare 0xA for i != 0, 0x4A as the fourth argument),
   the two flag stores, and the 16 quad halfword stores in retail's exact order.
   TWO measured findings for whoever picks this up:
   (1) THE QUADS ARE INDEXED BY THE PRIM INDEX, not by a constant. Retail scales
   the pool base by 0x24 = sizeof(POLY_G4) for the gouraud pair and by 0x18 =
   sizeof(POLY_F4) for the flat pair (`sll v0,v1,3 / addu / sll 2` and
   `sll v0,v1,1 / addu / sll 3`), so the source is `...->gouraudQuads[idx].x0` /
   `...->flatQuads[idx].x0` with idx = the word at 0x800CCB34; the store
   displacements 0x5F8/0x640 are that struct's field offsets, and the 0x28 stride
   this parked body uses is wrong.
   (2) THE TAIL NEEDS A SCALAR DECLARATION OF 0x800CCB34, WHICH THIS UNIT CANNOT
   HAVE. Retail re-reads that word once per store as `lui $v1,%hi(sym)` +
   `lw $v1,%lo(sym)($v1)` (16 times, 2 instructions each, the address destroyed by
   the load, i.e. a `(mem (symbol_ref))` RTL). Measured with cc1_probe.sh: only a
   NON-ARRAY (scalar) declaration emits that form; `extern u8 g_X[]` (and an
   s32/u32/volatile array, and `((s32 *)g_X)[0]`, `*(s32 *)&g_X`,
   `*(s32 *)(g_X + 0)`, a pointer local) all emit ONE shared `la` + `lw 0(reg)`,
   which is 16 instructions short and cannot link. The byte read earlier in the
   tail (`lbu $v0, 0($t2)` after a `la`) is the ARRAY form, so retail's TU had
   both views of the address (misc11.c in this overlay declares
   `extern volatile s32 g_BattlePrimStartIndex;` and gets the scalar form). This
   file's `extern u8 g_BattlePrimStartIndex[]` exists because the five matched
   functions around this stub need the `la` form, so the two cannot coexist here
   without a second symbol at 0x800CCB34 (symbol_addrs has no duplicate address
   anywhere, and splat names the disassembly by address, so that is not obviously
   safe) or converting those five to a scalar declaration plus pointer locals.
   Also open: retail's `i * 0x20 + 0x2A` / `i * 0x20 + 0x1A` third arguments are
   a SHIFTED induction variable (`lui s1,0x2A0000` / `sra $a2,s1,0x10`,
   +0x200000 per row) where this body gives `li`/`sll`/`ori`; and retail's
   `x = (arg1 & 0xFF) * 4 + 0x80` is emitted as andi / sll / addiu spread over
   the first three stores. Un-parking needs no new externs beyond
   `extern u8 *g_BattleCommandQueue;` and `extern u8 D_800C3DE1[];`, both now at
   the top of this file. */
/*
void func_80092B74(u8 arg0, u8 arg1) {
    u8 digits[2];
    s16 x;
    s32 i;

    digits[0] = arg1 / 10;
    digits[1] = arg1 % 10;
    g_BattleDisplayState->spriteColumnPackageCount = 0;
    if (digits[0] != 0) {
        g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(digits[0] + 0x67,
            (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * 0x50, 0x56, 0x38);
    }
    g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(digits[1] + 0x67,
        (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * 0x50, 0x5E, 0x38);
    for (i = 0; i < 7; i++) {
        if (g_BattleCommandQueue[i + 0x2C4] == 0xFF) {
            break;
        }
        g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(D_800C3DE1[i - 1] + 0x39,
            (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * 0x50, i * 0x20 + 0x2A,
            0x4A);
        if (i != 0) {
            g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(0xA,
                (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * 0x50,
                i * 0x20 + 0x1A, 0x4A);
        }
    }
    g_BattleDisplayState->spriteColumnPrimStart = g_BattlePrimStartIndex[0];
    g_BattleDisplayState->spriteColumnPending = 1;
    x = arg1 * 4 + 0x80;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->gouraudQuads[0].x0 = 0x80;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->gouraudQuads[0].y0 = 0x34;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->gouraudQuads[0].x1 = x;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->gouraudQuads[0].y1 = 0x34;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->gouraudQuads[0].x2 = 0x80;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->gouraudQuads[0].y2 = 0x3C;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->gouraudQuads[0].x3 = x;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->gouraudQuads[0].y3 = 0x3C;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->flatQuads[0].x0 = x;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->flatQuads[0].y0 = 0x34;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->flatQuads[0].x1 = 0xF0;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->flatQuads[0].y1 = 0x34;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->flatQuads[0].x2 = x;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->flatQuads[0].y2 = 0x3C;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->flatQuads[0].x3 = 0xF0;
    ((BattlePacketPool *)(g_BattleGraphicsWork->pPacketPrims +
                          ((s32 *)g_BattlePrimStartIndex)[0] * 0x28))->flatQuads[0].y3 = 0x3C;
    g_BattleGraphicsWork->pPacketPrims[0x66C] = g_BattlePrimStartIndex[0];
    g_BattleGraphicsWork->pPacketPrims[0x66F] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800930AC);
/* 90.96% (1204 bytes against retail's 1228) under this unit's gcc 2.6.0. The FRAME
   is retail's exactly (0xB0, vars 0x68, ten saved registers) and the body is the
   whole function: the 0x618 text buffer, the four-tile row (tile x 0x380, text
   x 0x380/0x3C0+2j/0x3C6), the three-digit split with divisors {1000,100,10} and
   the ones digit from the remainder, then the four pBufs frees, the two flat
   quads at 0x94/0xFC and the 0x668/0x669/0xB7 flags.
   Load-bearing shapes found by measurement: (1) the divisors array must be filled
   with three assignments, not an initialiser -- `= {1000,100,10}` makes cc1 copy
   from .rodata with lwl/lwr (worth 80.83 vs 76.49); (2) the row ys are the INLINE
   expressions (i * 16 + 0x100 for the tile and second rects, i * 16 + 0x102 for
   the glyph, the three tens slots and the ones slot) -- naming them y/textY is
   87.29%, and it is the inline form that puts the 0x80 slot back and the frame at
   retail's 0xB0; (3) the glyph RECT needs a hoisted `pGlyphRect = &glyphRect`
   pointer (90.96 vs 86.08 inline): gcc otherwise spills the sp+0x20 base into a
   var slot and then also re-bases pBufs on sp+0x20, which costs the arg0/pValue
   slots; (4) the ones-digit arm is written as `if (*pId != 0xFF) {draw} else
   {blank}` (87.27 -> 87.29 either way, but 85.74 with the arms the other way
   round); (5) `u8 started`/`u8 digit` are the narrow types retail masks with
   andi 0xFF at each use, and `u32 value` is what makes the divide/mult divu and
   multu; (6) `u16 onesDigit` is the local whose use emits retail's `andi 0xFFFF`.
   Residual (6 instructions, all allocator): retail folds the two table bases into
   the assembler macro (lui $at,%hi / addu $at,$at,v0 / lbu|lw %lo($at)) where this
   body materialises them (lui/addiu into a scratch + addu, one instruction more
   each), and retail's row-y induction variable is HImode (lhu from 0x80) where the
   inline-expression giv is SImode (lw). Tried: index/offset locals for both tables
   (82.20 for both, and 80.x for one), `u16 y`/`textY` variables, declaration
   orders, and two permuter runs (5255 -> 4281 -> 3657; its only real finds are the
   pGlyphRect pointer and the arm order above). The 0x68 slot in this body holds
   the spilled pGlyphRect where retail keeps arg0 there, which is the register
   permutation the rest of the diff shows. */
/*
void func_800930AC(u8 arg0, u8 *arg1, u16 *arg2) {
    RECT glyphRect;
    RECT *pGlyphRect;
    RECT onesRect;
    RECT tensRect;
    RECT tileRect;
    RECT secondRect;
    RECT rect;
    u_long *pBufs[4];
    u16 divisors[3];
    u8 *pId;
    u16 *pValue;
    u32 value;
    u8 started;
    u8 digit;
    u16 onesDigit;
    s32 i;
    s32 j;

    divisors[0] = 1000;
    divisors[1] = 100;
    divisors[2] = 10;
    BattleAllocPacketPrimitives();
    func_80076EA4();
    g_BattleTextWorkspace = BattleAllocTextBuffer(0x39);
    bzero(g_BattleTextWorkspace, 0x618);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, g_BattleTextWorkspace);
    pId = arg1;
    pValue = arg2;
    pGlyphRect = &glyphRect;
    for (i = 0; i < 4; i++) {
        pBufs[i] = BattleAllocTextBuffer(0x1B);
        bzero(pBufs[i], 0x30C);
        tileRect.x = 0x380;
        tileRect.y = i * 16 + 0x100;
        tileRect.w = 0x1B;
        tileRect.h = 0x10;
        BattleLoadImageAndSync(&tileRect, g_BattleTextWorkspace);
        started = 0;
        if (*pId != 0xFF) {
            SystemRenderStringEntry(GetStringEntryFromBank34(g_BattleSlotTextIndex[arg0 * 0x170] * 4 + i),
                                    pBufs[i], 0x1B, 0);
            glyphRect.x = 0x380;
            glyphRect.y = i * 16 + 0x102;
            glyphRect.w = 0x1E;
            glyphRect.h = 0xD;
            BattleLoadImageAndSync(pGlyphRect, pBufs[i]);
        }
        secondRect.x = 0x3C0;
        secondRect.y = i * 16 + 0x100;
        secondRect.w = 0x1B;
        secondRect.h = 0x10;
        BattleLoadImageAndSync(&secondRect, g_BattleTextWorkspace);
        for (j = 0; j < 3; j++) {
            tensRect.x = 0x3C0 + j * 2;
            tensRect.y = i * 16 + 0x102;
            tensRect.w = 6;
            tensRect.h = 0xD;
            value = *pValue;
            digit = value / divisors[j];
            if (digit != 0 || started != 0) {
                BattleLoadImageAndSync(&tensRect, g_BattleDigitImageBuffers[digit]);
                started = 1;
                *pValue -= digit * divisors[j];
            } else {
                BattleLoadImageAndSync(&tensRect, g_BattleTextWorkspace);
            }
        }
        onesRect.x = 0x3C6;
        onesRect.y = i * 16 + 0x102;
        onesRect.w = 6;
        onesRect.h = 0xD;
        if (*pId != 0xFF) {
            value = *pValue;
            onesDigit = value % 10;
            BattleLoadImageAndSync(&onesRect, g_BattleDigitImageBuffers[onesDigit]);
        } else {
            BattleLoadImageAndSync(&onesRect, g_BattleTextWorkspace);
        }
        pId++;
        pValue++;
    }
    for (i = 0; i < 4; i++) {
        HeapFree(pBufs[i]);
    }
    HeapFree(g_BattleTextWorkspace);
    *(u16 *)(g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0xE) = g_SystemPalette1;
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28),
                         0x94, 0x54, 0, 0, 0x60, 0x40);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                     (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0xA0)),
                         0xFC, 0x54, 0, 0, 0x20, 0x40);
    g_BattleGraphicsWork->pPacketPrims[0x668] = g_BattlePrimStartIndex[0];
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->unkB7 = 4;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093578);
/* 97.72% at objdiff's 692 bytes -- retail's EXACT size, and every instruction
   is retail's: the only difference left is which register three values land in
   (retail keeps `row` in $s0 where it holds the dying arg1, this body takes $a0;
   `g_BattlePrimStartIndex[0]` is $v1 in retail and $t0 here; `pRec` is formed
   with `addu v1,v1,v0` in place in retail). Structure that took it from 83.74:
   (1) the flags word wants a named index local -- `index = arg1 + 0x25;` then
   `row = index * 0x28;` before the load, otherwise gcc folds 0x25 * 0x28 into
   the lhu's reloc addend (+0x5C8) where retail emits `addiu s0,s0,37` first;
   (2) the row pick is 9 for flags & 0x4000, then 7 when 0x1000 is SET and 8 when
   it is clear -- the older 9/8/7 reading had the delay-slot arm inverted;
   (3) the switch needs `case 0`, `case 1`, `case 2` then `default: col = 0xC`,
   listed in that order (case 0 present is what makes gcc emit the
   `slti v0,v1,2` range test); listing them 0,2,1 or dropping case 0 loses the j;
   (4) `pRec = g_BattleGraphicsWork->pPacketPrims;` before the 0x320 call.
   Tried: declaration orders (six, no change), u16/s32 for row, index and ofs,
   an `ofs` local vs `row * 4` inline (ofs lands in $t1 and costs 4 bytes), and
   three 4-minute permuter runs (base 926 -> 655 -> 600 -> 385); the last one's
   only improvement swapped `D_800D2F69[col * 4]` for `D_800D2F69[0xC * 4]`,
   which is wrong code. Round 294 added an 11-shape sweep (one shared variable for
   the offset and the pick, 97.30 -- it does move the pick into an $s register but
   then the two parameters swap $s0/$s1; a separate `rowIdx` for the pick, 97.60;
   pick into `index`, 97.21; the offset inlined into the flags address, 96.56; u32
   index, identical; s16 row, 97.60; `row = index; row *= 0x28;`, 97.30; reversed
   product, identical; a `u16 *pFlags` local, 95.92 at 696 bytes; switch before the
   pick, 87.86) and a fourth permuter run (base 385 -> 375, its only find a
   `-(-(x))` double negation on the second pRec line that buys 0.05 and is not
   shippable code). Preset screen: gcc-2.7.2 96.21, gcc-2.7.2-cdk 57.83, so 2.6.0 is
   right. Only the allocator tie for `row` is left. Un-parking needs no new
   externs. */
/*
void func_80093578(u8 arg0, u8 arg1) {
    u8 *pRec;
    s32 index;
    u16 flags;
    u16 pal;
    s32 row;
    s32 col;

    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                     (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x140)),
                         0x7C, 0xA4, 0, (u8)(arg1 * 0x10), 0x60, 0x10);
    index = arg1 + 0x25;
    row = index * 0x28;
    flags = *(u16 *)(g_BattleActionRows + row + arg0 * 0x690);
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
    case 1:
        col = 0xD;
        break;
    case 2:
        col = 0x15;
        break;
    default:
        col = 0xC;
        break;
    }
    pRec = g_BattleGraphicsWork->pPacketPrims;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(pRec + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x320)),
                            0x24, 0xA6, D_800D2F6A[row * 4], D_800D2F6B[row * 4], g_BattleActionRowIconSprites[row * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28;
    if (D_800D2F69[col * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                         (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x370)),
                            0x48, 0xA6, D_800D2F6A[col * 4], D_800D2F6B[col * 4], g_BattleActionRowIconSprites[col * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims + ((s32 *)g_BattlePrimStartIndex)[0] * 0x28;
    if (D_800D2F69[col * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009382C);
/* 99.04% (412 bytes against retail's 416): the array-extern spelling from
   func_8009209C closed the &g_BattlePrimStartIndex wall here too -- retail's
   `lui s3/addu s3` + `lw 0(s3)` per call is reproduced exactly, and every other
   instruction is identical. The whole remainder is the one `andi s0,s0,0xffff`
   retail emits at index's FIRST use (the truncation of the u16 local):
   gcc 2.6.0 proves the range (u8 load * 4 + u8, * 2 -> <= 2550) and drops the
   mask, so the body is 4 bytes short and cannot link. The mask is a HImode
   value's zero-extension and cc1 keeps it only when nonzero_bits cannot bound
   the operand; round 323 measured 35 spellings (u16 / s16 / s32 / u32 local,
   volatile, bitfield, struct member, one-element array, declaration initialiser,
   second `u16` copy, `s16` round trip, unprototyped callee, `u16` parameter on
   the callee, `& 0xFFFF`, `| 0`, `index = index;`, `(u16)` at the use, the
   three-statement HImode form, both arms of an if/else) and every one emits
   retail's stream minus that instruction, under 2.6.0, 2.7.2 and 2.7.2-cdk
   alike. The forms that do keep it are a call result, an `s32` global load,
   `x % k` (func_80093B08's own mask on `x - (x/10)*10`) and a HImode compare,
   each of which adds an instruction here. Measured dead earlier too: an s32
   index with an explicit `(u16)(...)` initialiser, and gcc-2.7.2 (98.85). */
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
// objdiff stays at 99.87 because retail names the per-slot row table g_BattleSlotStatusRows where
// this body reaches the same word as g_BattleWorkBlock[slot].conditionFlags (reloc addend only).
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
/* 83.10% (1256 bytes against retail's 1360). The whole body is written and
   compiles: the four early-exit groups, the row-table pick (idx*0x5F0 + 0x1058 +
   rowIndex*0x28, else rowIndex*0x28 + 0x35D8), the two rand()-guarded blocks, the
   three timer calls, the 0xB-slot mask walk and the tail. Residual is register
   allocation, all of it one cause: this body keeps g_BattleCurWorkBlock in a
   CALLEE-SAVED register across the calls (frame 0x20 with s0 and s1 saved) where
   retail reloads the global after every call (frame 0x18, only s0 = the slot bit
   mask). Writing every use as `g_BattleCurWorkBlock->...` inline instead of
   through a `pWork` local was measured and changes nothing, so the value being
   kept live is a source-shape question the next worker should attack with the
   permuter (`tools/scripts/permute.sh battle/main/misc17 func_800941A4`), not by
   hand. Also open: retail's `mask <<= 1` sits in the loop-back delay slot and its
   `g_BattleCurSlotIndex++` is a global RMW at the loop tail (this body lets gcc
   cache the index), and the final `lbu v0, %lo(D_800D2DB8)` on the fall-through
   path (a return value or a volatile read - the early-exit path skips it) is not
   reproduced here. Externs and the callee prototypes are all at the top of this
   file already, so un-parking needs no new declarations. */
/*
void func_800941A4(void) {
    BattleTableEntry *pSlot;
    u16 flags;
    u16 mask;
    u8 row;
    u8 i;

    BattleClearAllSlotStates();
    D_800D2DB8 = 0;
    D_800C34AE = 0;
    g_BattleTargetSlotIndex = g_BattleCurWorkBlock->actorIndex;
    g_BattleActingSlot = &g_BattleCurWorkBlock->slots[g_BattleTargetSlotIndex];
    g_BattleTargetSlotState = &g_BattleActingSlot->state;
    if (g_BattleCurWorkBlock->slots[g_BattleTargetSlotIndex].state.flags & 0x80) {
        BattleExecuteActionRow();
        BattleSetSlotEntryFieldsTo1(g_BattleTargetSlotIndex);
        return;
    }
    if (g_BattleTargetSlotIndex < 3) {
        g_BattleCurrentRow = (BattleRow *)((u8 *)pWork + 0x1058 + g_BattleTargetSlotIndex * 0x5F0 + g_BattleCurWorkBlock->rowIndex * 0x28);
    } else {
        g_BattleCurrentRow = (BattleRow *)((u8 *)pWork + g_BattleCurWorkBlock->rowIndex * 0x28 + 0x35D8);
    }
    BattleSetSlotEntryFieldsTo2000(g_BattleTargetSlotIndex, 1);
    flags = g_BattleCurrentRow->actionFlags;
    if (flags & 0x10) {
        BattleExecuteActionRow();
        BattleSetSlotEntryFieldsTo1(g_BattleTargetSlotIndex);
        BattleAccumulateSlotValueByRowFlag();
        return;
    }
    g_BattleAmountThirdFlag = 0;
    if ((flags & 0x100) && g_BattleActingSlot->characterId == 1) {
        BattleRollPercentAndSetState38();
    }
    row = g_BattleCurWorkBlock->rowIndex;
    if (g_BattleActingSlot->characterId == 4) {
        if ((u8)(row - 4) < 2) {
            g_BattleCurrentRow->element = ((u8 *)g_BattleActingSlot)[(row - 3) * 8 + 4];
        }
    }
    mask = 1;
    if (g_BattleActingSlot->activeEffectFlags2 & 0x20) {
        if ((g_BattleActionSlotMask & 7) != 0 && g_BattleActionSlotMask < 7) {
            for (i = 0; i < 3; i++) {
                if (((u8 *)D_800CCD3E)[i * 0x170] == 3) {
                    g_BattleActionSlotMask = 1 << i;
                }
            }
        }
    }
    g_BattleCurSlotIndex = 0;
    while (g_BattleCurSlotIndex < 0xB) {
            if (mask & g_BattleCurWorkBlock->activeSlotMask) {
            pSlot = &g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex];
            g_BattleTargetSlotEntry = pSlot;
            g_BattleCurSlotState = (u8 *)&pSlot->state;
            D_800C3D60 = (u8 *)pSlot + 0x148;
            if (g_BattleCurSlotIndex < 3) {
                BattleRollPercentAndSwapActiveActor();
            }
            ((void (*)(void))D_800C348C[g_BattleCurrentRow->opcode])();
            func_800946F4();
            if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 0) {
                flags = g_BattleCurrentRow->actionFlags;
                if (flags & 0x800) {
                    BattleApplyActorConditionTimer();
                } else if (flags & 0x4000) {
                    BattleApplyScriptConditionTimer();
                } else if (flags & 4) {
                    BattleClearStatusWordsOnAction6E();
                }
            }
            if (g_BattleCurrentRow->actionFlags & 1) {
                g_BattleCurWorkBlock->unk5FB0 = g_BattleCurrentRow->unk02;
            } else {
                g_BattleCurWorkBlock->unk5FB0 = g_BattleCurWorkBlock->rowIndex;
            }
        }
        g_BattleCurSlotIndex++;
        mask <<= 1;
    }
    BattleSaveScriptCommandOperandsFromActor();
    BattleSetSlotEntryFieldsTo1(g_BattleTargetSlotIndex);
    if (g_BattleActingSlot->characterId == 4 && D_800C34AE == 0) {
        BattleDecrementGearPartUse();
    }
    if (g_BattleTargetSlotIndex < 3 && g_BattleCurWorkBlock->rowIndex < 7) {
        if (!(0xFDE7 < *(u16 *)((u8 *)g_BattleActingSlot + g_BattleCurWorkBlock->rowIndex * 2 + 0x90))) {
            *(u16 *)((u8 *)g_BattleActingSlot + g_BattleCurWorkBlock->rowIndex * 2 + 0x90) +=
                ((u8 *)g_BattleActingSlot)[0x55] + ((u8 *)g_BattleActingSlot)[0xA1];
        }
    }
    BattleAccumulateSlotValueByRowFlag();
}
*/
void func_800946F4(void) {
    u32 amount;
    u32 sum;
    u16 value;
    u16 flags;
    s32 limit;
    u8 state;
    u8 i;

    if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 0) {
        if (g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] ==
            g_BattleTargetSlotEntry->hp) {
            if (g_BattleTargetSlotIndex < MAX_PARTY_MEMBERS) {
                value = *(u16 *)g_BattleActingSlot->unk3A + 1;
                *(u16 *)g_BattleActingSlot->unk3A = value;
                if (value > 0xFDE8) {
                    *(u16 *)g_BattleActingSlot->unk3A = value - 1;
                }
            }
        }
        flags = g_BattleTargetSlotEntry->activeEffectFlags2;
        g_BattleTargetSlotEntry->activeEffectFlags2 = flags & 0xEFFF;
        if (flags & BATTLE_GEAR_EFFECT_FLAG) {
            if (rand() % BATTLE_PERCENT_ROLL_RANGE < 70) {
                g_BattleTargetSlotEntry->activeEffectFlags2 &= 0xDFFF;
            }
        }
        if (g_BattleCurrentRow->actionFlags & 0x100) {
            if (g_BattleTargetSlotEntry->unk36 & 0x4000) {
                g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
            }
            if (g_BattleTargetSlotEntry->unk36 & 0x2000) {
                if ((g_BattleCurrentRow->element & 0xF) == 0) {
                    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
                }
            }
        }
        if (g_BattleTargetSlotEntry->accessoryEffectFlags5 & 0x80) {
            limit = 0x3C;
            if (g_BattleTargetSlotEntry->characterId == 0) {
                limit = 0x50;
            }
            if (rand() % BATTLE_PERCENT_ROLL_RANGE < limit) {
                g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] =
                    (u32)g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] >> 1;
            } else {
                amount = g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex];
                sum = amount >> 1;
                sum = sum + amount;
                g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = sum;
            }
        }
        if (g_BattleTargetSlotEntry->accessoryEffectFlags5 & 0x20) {
            g_BattleCurWorkBlock->slotStates[g_BattleTargetSlotIndex] = 0;
            g_BattleCurWorkBlock->slotAmounts[g_BattleTargetSlotIndex] =
                g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex];
        }
        if (g_BattleTargetSlotIndex < MAX_PARTY_MEMBERS &&
            g_BattleTargetSlotIndex != g_BattleCurSlotIndex) {
            if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].activeEffectFlags1 & 2) {
                g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].activeEffectFlags1 &= ~2;
                g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].conditionFlags =
                    g_BattleSavedSlotUnk7A[g_BattleCurSlotIndex];
            }
        }
        if (g_BattleTargetSlotEntry->characterId == 3) {
            if ((u32)g_BattleCurveTargets[g_BattleCurSlotIndex] >= g_BattleTargetSlotEntry->hp) {
                for (i = MAX_PARTY_MEMBERS; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
                    g_BattleWorkBlock[i].activeEffectFlags2 &= ~0x20;
                }
            }
        }
    }
    if (g_BattleTargetSlotEntry->unk36 & 0x8000) {
        state = g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex];
        switch (state) {
        case 0:
        case 5:
            g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
            break;
        case 2:
            g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
            break;
        }
    }
    if (g_BattleTargetSlotEntry->accessoryEffectFlags3 & 0x80) {
        if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 2) {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] <<= 1;
        }
    }
    if (g_BattleTargetSlotEntry->accessoryEffectFlags4 & 0x200) {
        if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 1) {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = 0;
        }
    }
}
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
    command = BattleRollHitResult();
    atkValue = func_80096FBC();
    defValue = BattleComputeSlotActionAmount();
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
            g_BattleCurveTargets[g_BattleTargetSlotIndex] = (u16)(g_BattleActingSlot->maxMp / BATTLE_MP_SCALE) * 2;
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
        rate = BATTLE_ACTION_POWER_FULL;
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
            amount = rate * amount / BATTLE_ACTION_POWER_FULL;
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
// 99.88 in objdiff for the reloc only: retail names g_BattleSlotConditions, this body
// g_BattleWorkBlock[slot].state.flags (same word).
void BattleSetSlotState2FromActorField5BWithModifiers(void) {
    BattleTableEntry *pAtk = g_BattleActingSlot;
    BattleRow *pRow = g_BattleCurrentRow;
    BattleTableEntry *pDef;
    s16 value;
    u16 flags;

    value = pAtk->ether * pRow->power;
    if (pAtk->accessoryEffectFlags4 & 0x2000) {
        value *= 2;
    }
    pDef = g_BattleTargetSlotEntry;
    flags = pDef->activeEffectFlags7 | pDef->accessoryEffectFlags7;
    if (flags & 0x100) {
        value = 7 * value / 10;
    }
    if (flags & 0x200) {
        value = 13 * value / 10;
    }
    if (g_BattleWorkBlock[g_BattleCurSlotIndex].state.flags & 0x80) {
        value = 0;
    }
    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = value;
}
// 99.84 in objdiff for the reloc only: retail names g_BattleCurveTargets[t], this body
// `g_BattleCurveTargets + slot` (same address).
void BattleInitActiveSlotAction(void) {
    s32 slot;
    BattleTableEntry *pTarget;
    s32 *pCurveTarget;
    s32 maxHp;

    if (g_BattleWorkBlock[g_BattleCurSlotIndex].characterId == 2) {
        BattleSetSlotEntryFieldsTo2000(g_BattleCurSlotIndex, 1);
    }
    slot = g_BattleCurSlotIndex;
    g_BattleTargetSlotEntry->activeEffectFlags1 = 0;
    g_BattleTargetSlotEntry->activeEffectFlags2 = 0;
    g_BattleTargetSlotEntry->activeEffectFlags3 = 0;
    g_BattleTargetSlotEntry->activeEffectFlags4 = 0;
    g_BattleTargetSlotEntry->activeEffectFlags7 = 0;
    g_BattleCurveTargetModes[slot] = 2;
    pTarget = g_BattleTargetSlotEntry;
    maxHp = pTarget->maxHp * g_BattleCurrentRow->power;
    if (g_BattleCurSlotIndex) { // empty: makes gcc share this load with the one below ($v1, not $a1)
    }
    slot = g_BattleCurSlotIndex;
    // A pointer, not g_BattleCurveTargets[slot]: the indexed form schedules `1 << slot` into the
    // load's delay slot, which drops retail's nop (4 bytes short).
    pCurveTarget = g_BattleCurveTargets + slot;
    *pCurveTarget = maxHp / 10;
    g_BattleCommandCellFlags |= 1 << slot;
}
void BattleClearStatusWordsOnAction6E(void) {
    if (rand() % BATTLE_PERCENT_ROLL_RANGE > g_BattleCurrentRow->chance) {
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
