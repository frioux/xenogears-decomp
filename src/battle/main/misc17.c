#include "common.h"
#include "main/game.h"
#include "battle/actor.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

// Owned by include/system/menu.h, which this unit does not include.
#define MENU_ITEM_DESC_IMAGE_BUFFER_SIZE 0x618

// Bytes of one slot's text buffer: the size BattleAllocTextBuffer is asked for and the
// capacity SystemRenderStringEntry renders the string into. battle/main/misc3 names the
// same size for the same buffers.
#define BATTLE_TEXT_SLOT_BUFFER_SIZE 0x39

// The width of one digit glyph: the loop below steps the glyph's x by it, the glyph is drawn
// this wide, and its atlas x is the digit times it.
#define BATTLE_STATUS_DIGIT_WIDTH 8

// The digit positions of the number BattleDrawSlotStatusDigits draws: it draws one glyph
// per entry of D_800C3CFA, and an entry that is blank is skipped.
#define BATTLE_STATUS_DIGIT_COUNT 3

// The cap BattleResolveSlotActionState holds a slot's actionCount at: when the next action
// would take the counter past it, the increment is undone.
#define BATTLE_ACTION_COUNT_MAX 65000

// The stat weights BattleRunCurrentActionRow adds a row's bonuses to: GameCharacter's
// statWeights has eight, of which the dressing room reads the first seven (main/game.h);
// battle/main/misc19_r1 copies the same seven.
#define BATTLE_STAT_WEIGHT_COUNT 7

// The largest stat weight that loop adds to: a weight already at it is left alone.
#define BATTLE_STAT_WEIGHT_MAX 64999

// The row conditionKind BattleClearStatusWordsOnAction6E acts on: it zeroes the six effect
// words whose flag bits the row's conditionFlags carry.
#define BATTLE_CONDITION_KIND_CLEAR_STATUS 0x6E

// The accessoryEffectFlags5 bit the chance pair below is rolled for.
#define BATTLE_ACCESSORY_AMOUNT_HALVE_FLAG 0x80

// The percent chance BattleResolveSlotActionState halves an action's amount instead of
// adding half of it, when the target's accessoryEffectFlags5 has BATTLE_ACCESSORY_AMOUNT_HALVE_FLAG;
// a target whose characterId is 0 is rolled against the higher one. battle/main/misc20 names
// the same pair for the same roll.
#define BATTLE_AMOUNT_HALVE_CHANCE_PERCENT 60
#define BATTLE_AMOUNT_HALVE_CHANCE_PERCENT_CHARACTER_0 80

// The percent chance BattleResolveSlotActionState clears BATTLE_GEAR_EFFECT_FLAG off the
// target instead of keeping it.
#define BATTLE_GEAR_EFFECT_CLEAR_CHANCE_PERCENT 70

// The damage-modifier bits of a slot's activeEffectFlags4|accessoryEffectFlags4: BattleComputeActionAmount
// raises or lowers the value the acting slot deals by a fifth or a tenth, and does the same to the
// value the target subtracts, so one bit means the same bonus or malus whichever side holds it.
// battle/main/misc18 names the same nibble as the 0x5 and 0xA pairs.
#define BATTLE_EFFECT_DAMAGE_BONUS_20 0x8
#define BATTLE_EFFECT_DAMAGE_BONUS_10 0x2
#define BATTLE_EFFECT_DAMAGE_MALUS_20 0x4
#define BATTLE_EFFECT_DAMAGE_MALUS_10 0x1

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
// The packet-pool cursor the tail of func_80092B74 re-reads per store (parked below). splat
// rejects a second name at 0x800CCB34, so this alias can only live here, not in symbol_addrs.
extern volatile s32 g_BattlePrimIndexWord;
// The same word as g_BattlePrimStartIndex, read as the scalar it is: the packet pool cursor
// the border quad block indexes by. The original reloads it for every store.
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
extern void *g_BattleSlotStringTable;
extern u8 D_800C33B4[];  // 16 sprite ids, one per battle party column
extern s16 D_800C33C4[][2];
extern s16 D_800C3404[][2];

void BattleReleaseEffectSlotIfChar4(void);
void BattleSetSlotEntryFieldsTo2000(u8, u8);
void BattleDrawSlotStatusDigits(u8, u8, s32);
void BattleParseNumberToDigits(u32);
void BattleSetPolyFT4RectH13(POLY_FT4 *, u16, s16, u8, u8, s16);
void BattleSetPolyFT4Rect(POLY_FT4 *, s16, s16, u8, u8, s16, s16);
s32 func_8009A258(u8, s32);
void BattleDrawSlotStatusCellIcons(u8, u8, u8);
void BattleDrawSlotStatusText(u8, u8, u8);
void BattleDrawActionRowIcons(u8, u8);
void BattleDrawSlotStatusTextEntry3(u8, u8);
s32 BattleDrawSpritePackage(s32, u8 *, s32, s16);
s32 BattleMaskHasStatusBit(s32, u8);
extern void *BattleAllocTextBuffer(s32);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern void BattleLoadImageAndSync(RECT *, u_long *);
extern u8 *g_BattleEntry3StringTable;
extern u8 g_BattleSlotTextIndex[];
extern u8 g_BattleTargetSlotIndex;
extern u8 g_BattleAmountThirdFlag;
extern s32 BattleRollHitResult(void);
extern u16 BattleComputeActionAttackValue(void);
extern u16 BattleComputeSlotActionAmount(void);
extern void func_80096494(u16 *, u16 *, u8 *);
extern void BattleAllocPacketPrimitives(void);
extern void BattleLoadActionRowGraphics(void);
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
extern u16 g_BattleActionSlotMask[]; // the array view keeps its address in a register across both guard reads, as retail does
extern u8 D_800CCD3E[];
extern void *D_800C348C[];
extern volatile u8 D_800D2DB8; // volatile so the discarded read that ends BattleRunCurrentActionRow is not eliminated
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
void BattleResolveSlotActionState(void);
extern void func_8008FC1C(s32, s32, s32, s32, u8);
extern void BattleDrawCommandCellFrames(u8);
extern void BattleUpdateFrame(void);
extern void *GetStringEntryFromBank14(s32);
extern void *GetWeaponName(s32);
extern void BattleInitSlotGraphics(s32, u16, u16, u16, u16, u8, u8);
extern void *GetStringEntryFromBank30(s32);
extern u8 D_800CDD53[];
extern u8 D_800CEF23[];

// The largest amount the current action may deal; the tail of BattleComputeActionAmount clamps to it.

// Number of action rows of one battle slot's status panel.
#define BATTLE_STATUS_ROW_COUNT 16

// The sprite packages BattleBuildSpriteColumn8 draws down one column, each 8 pixels below the last.
#define BATTLE_SPRITE_COLUMN_PACKAGE_COUNT 8

// The pixels between those packages: each is drawn this far below the one before it.
#define BATTLE_SPRITE_COLUMN_ROW_STEP 8

// The bytes one drawn package occupies in the display state's package block: the block's
// package counter times this is where the next package is drawn.
#define BATTLE_SPRITE_PACKAGE_SIZE 80

// The row's power is in twentieths: `power * amount / this` leaves the action's amount unchanged,
// and BattleComputeActionAmount forces exactly that rate on a row whose actionFlags has 0x400.


/* 92.28% (1456 bytes / 364 insns against retail's 1440 / 360) under this unit's gcc 2.6.0.
   Round 511 re-measured the older park unchanged at 91.38% (1444 bytes / 361 insns) and then
   bought 0.9 points with ONE change: the third loop's `bufs` addressing. Retail keeps an
   `i * 4` giv (`move s2,zero` in the third loop's preheader, `addiu s2,s2,4` in the latch's
   bnez slot, and `addiu v0,sp,0x48 / addu s0,v0,s2` re-forming &bufs[i]) while a plain
   `bufs[i]` body makes loop.c derive a POINTER biv (`addiu s0,s0,4` in the latch) off
   `&nameRect` (`addiu s3,sp,24; lw a1,48(s0)`). Declaring `u32 bufOff;` and writing the
   three sites as `*(void **)((u8 *)bufs + bufOff)` with `i++, bufOff += 4` in the for-clause
   reproduces retail's offset form and its `addiu v0,sp,72 / addu s0,v0,s2 / sw a0,0(s0)`;
   it costs 16 bytes (the extra is the still-hoisted D_800CCD3E base, below). Measured this
   round on the same body, all dead: a `volatile` qualifier on that bank byte (identical
   91.38, the hoisted thing is the ADDRESS, not the load), the `D_800CCD3E[arg0 * 0x170]`
   subscript spelling (identical), hoisting `i = 0` before the flags test so it lands in the
   bnez delay slot like retail's `move s1,zero` (91.73, and it moves the sum scheduling the
   wrong way), and `s32 slot = arg0;` used at every site (88.01 - an SI copy makes cc1
   rematerialise the whole address chain); `u8 slot = arg0;` inside each arm on top of
   `bufOff` is 92.35% / 1460 bytes / 365 insns with 115 differing lines instead of 124, but
   it buys that with a no-op `move s5,s6` and 4 more bytes, so it is not the parked body.
   ROUND 511 note on the size: neither body links (retail 1440). Every call, rectangle and store is retail's and the tail from BattleDrawCommandCellFrames onwards is
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
   into a `short` in the third loop and writes `i <= 16 - 1` (not ported).

   One cause behind every row above, measured: retail's third loop runs with SIX live
   callee-saved values (raw arg0, arg0 & 0xff, arg0 * 0x170, i, the byte offset i * 4 and
   &bufs[i]) and s5 dead, while this body needs SEVEN. The seventh is the hoisted
   D_800CCD3E base; it pushes raw arg0 from s6 into s5, and then every later site pays:
   a fresh `andi 0xff` inside both row scans where retail keeps one copy, `&nameRect` in a
   register instead of retail's per-call `addiu a0,sp,24`, and a pointer biv for `bufs`
   where retail keeps `sp + i * 4`. Kill that hoist and the diff collapses.
   Round 480 measured, on this same 1444-byte body:
   - `u32 arg0` parameter plus a `u8 slot = arg0;` assigned before the first flags test and
     used for the char-id index, both strides and both row bases: cc1 copy-propagates `slot`
     back to `(u8)arg0` and re-masks per site, so no register is saved (91.28%, 1444 bytes,
     361 insns).
   - the same `u8 slot` assigned inside EACH arm, which is retail's `move s4,a0` at
     0x9111C / 0x911B8: the hoist survives and grows, 1452 bytes (91.80% but 363 insns, so
     it cannot link).
   - the bank byte read into a local inside the bank14 arm: identical code.
   - presets: gcc-2.7.2 91.01, gcc-2.7.2-cdk 80.09, 2.6.0 with -G0 91.38 (unchanged), so the
     unit's XenoBattle260 preset is right and no preset switch is owed here. */
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
    u32 rowOff;
    u32 slotOff;
    u8 index;
    s32 i;
    u32 bufOff;

    BattleAllocPacketPrimitives();
    BattleLoadActionRowGraphics();
    func_8008FC1C(0x20, 0x30, 0x98, 0x38, 0xC);
    pBuf = BattleAllocTextBuffer(0x39);
    g_BattleTextWorkspace = pBuf;
    bzero(pBuf, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
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
                    (u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].characterConditionMask,
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
                    (u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].gearConditionMask,
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
    for (i = 0, bufOff = 0; i < BATTLE_STATUS_ROW_COUNT; i++, bufOff += 4) {
        pBuf = BattleAllocTextBuffer(0x1B);
        *(void **)((u8 *)bufs + bufOff) = pBuf;
        bzero(pBuf, 0x30C);
        tileRect.x = 0x380 + (i % 2) * 0x1E;
        tileRect.y = 0x100 + (i / 2) * 0x10;
        tileRect.w = 0x1B;
        tileRect.h = 0x10;
        BattleLoadImageAndSync(&tileRect, g_BattleTextWorkspace);
        if (active[i] != 0) {
            if (g_BattleSlotFlags[arg0 * 8] == 0) {
                pStr = GetStringEntryFromBank14(D_800CCD3E[arg0 * 0x170] * 0x10 + i);
            } else {
                pStr = GetStringEntryFromBank30(*(u8 *)(g_BattleSlotTextIndex + arg0 * 0x170) * 0x10 + i);
            }
            SystemRenderStringEntry(pStr, *(void **)((u8 *)bufs + bufOff), 0x1B, 0);
            nameRect.x = 0x380 + (i % 2) * 0x1E;
            nameRect.y = 0x102 + (i / 2) * 0x10;
            nameRect.w = 0x1E;
            nameRect.h = 0xD;
            BattleLoadImageAndSync(&nameRect, *(void **)((u8 *)bufs + bufOff));
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
    BattleDrawCommandCellFrames(arg0);
    for (i = 0; i < BATTLE_STATUS_ROW_COUNT; i++) {
        HeapFree(bufs[i]);
    }
    HeapFree(g_BattleTextWorkspace);
    BattleUpdateFrame();
    *(u16 *)(g_BattleGraphicsWork->pPacketPrims + 0x14E) = g_SystemPalette1;
    *(u16 *)(g_BattleGraphicsWork->pPacketPrims + 0x176) = g_SystemPalette1;
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->unkB7 = 1;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091064);

void BattleBuildSpriteColumn8(s32 arg0) {
    s32 i;
    s32 offset;

    g_BattleDisplayState->spriteColumnPackageCount = 0;
    for (i = 0; i < BATTLE_SPRITE_COLUMN_PACKAGE_COUNT; i++) {
        offset = i * BATTLE_SPRITE_COLUMN_ROW_STEP + 0x38;
        g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(0x66,
            (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * BATTLE_SPRITE_PACKAGE_SIZE, 0x20,
            (s16)(offset + arg0));
    }
    g_BattleDisplayState->spriteColumnPrimStart = g_BattlePrimStartIndex[0];
    g_BattleDisplayState->spriteColumnPending = 1;
}
void BattleDrawSlotStatusDigits(u8 arg0, u8 arg1, s32 arg2) {
    u32 s;
    s32 c;
    s32 i;
    u16 k;
    u8 u;
    u8 v;
    u32 half;

    s = arg1 * 2 + arg0;
    half = s / 2;
    u = (s - half * 2) * 0x78;
    v = half * 16;
    BattleSetPolyFT4Rect(
        (POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x140)),
        0x20, 0xA4, u, v, 0x60, 0x10);
    g_BattleGraphicsWork->pPacketPrims[0x66E] = 0;
    if (g_BattleActorCharIds[(u8)arg2] == 1) {
        BattleParseNumberToDigits(func_8009A258((u8)arg2, s + 0x16));
        for (i = 0, k = 0x8C; i < BATTLE_STATUS_DIGIT_COUNT; k += BATTLE_STATUS_DIGIT_WIDTH) {
            c = D_800C3CFA[i++];
            if (c == BATTLE_VALUE_DIGIT_NONE) {
                continue;
            }
            BattleSetPolyFT4RectH13(
                (POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                             ((g_BattleGraphicsWork->pPacketPrims[0x66E] * 2 +
                               ((s32 *)g_BattlePrimStartIndex)[0]) * 0x28 + 0x190)),
                k, 0xA6, (c * BATTLE_STATUS_DIGIT_WIDTH + 0x78) & 0xF8, 0, BATTLE_STATUS_DIGIT_WIDTH);
            g_BattleGraphicsWork->pPacketPrims[0x66E]++;
        }
    }
}
void BattleDrawSlotStatusCellIcons(u8 arg0, u8 arg1, u8 arg2) {
    u16 flags;
    u16 pal;
    u8 *pRec;
    s32 row;
    s32 col;
    s32 rowOff;

    // The row index stays a named local and the slot term is summed inside the address: as
    // one expression cc1 puts the address addu's destination on the slot register, retail's
    // is on the row one.
    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        rowOff = arg2 * 2 + arg1 + 0x16;
        flags = *(u16 *)(g_BattleCellActionRows + rowOff * 0x28 + arg0 * 0x5F0);
    } else {
        rowOff = arg2 * 2 + arg1 + 0x15;
        flags = *(u16 *)(g_BattleActionRows + rowOff * 0x28 + arg0 * 0x690);
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
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x320)),
                           0xDA, 0xAA, D_800D2F6A[row * 4], D_800D2F6B[row * 4], g_BattleActionRowIconSprites[row * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[0] * 0x28));
    if (D_800D2F69[row * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x370)),
                           0xFE, 0xAA, D_800D2F6A[col * 4], D_800D2F6B[col * 4], g_BattleActionRowIconSprites[col * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[0] * 0x28));
    if (D_800D2F69[col * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}
void BattleDrawSlotStatusText(u8 arg0, u8 arg1, u8 arg2) {
    RECT rect;
    u8 *pBuf;
    u16 index;
    s32 altEntry;
    s32 len1;
    s32 len2;

    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        index = g_BattleActorCharIds[arg0] * 0x20 + (arg2 * 2 + arg1) * 2;
    } else {
        index = g_GameState.characters[g_BattleActorCharIds[arg0]].gearId * 0x20 + (arg2 * 2 + arg1) * 2;
    }
    altEntry = 1;
    pBuf = (u8 *)BattleAllocTextBuffer(BATTLE_TEXT_SLOT_BUFFER_SIZE);
    bzero(pBuf, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
    len1 = SystemRenderStringEntry(GetStringEntry(g_BattleSlotStringTable, index), pBuf, BATTLE_TEXT_SLOT_BUFFER_SIZE, 0);
    // load-bearing s32: cc1 keeps the HImode index's zero-extension only when the
    // OR's right operand is a register, not the constant 1.
    len2 = SystemRenderStringEntry(GetStringEntry(g_BattleSlotStringTable, index | altEntry), pBuf, BATTLE_TEXT_SLOT_BUFFER_SIZE, 1);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, pBuf);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x280)), 0x30, 0xB6, 0, 0, len1 & 0xFF);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x2D0)), 0x30, 0xC6, 0, 0, len2 & 0xFF);
    HeapFree(pBuf);
}
void BattleDrawCharStatusIfFlagged(u8 arg0, u8 arg1, u8 arg2) {
    u8 ret = 0;

    if (g_BattleSlotFlags[arg0 * 8] == 0) {
        ret = (u16)BattleMaskHasStatusBit(
                  (u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].characterConditionMask,
                  arg1 + arg2 * 2) != 0;
    } else if ((u16)BattleMaskHasStatusBit(
                   (u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].gearConditionMask,
                   arg1 + arg2 * 2) != 0) {
        ret = 1;
    }
    if (ret != 0) {
        BattleDrawSlotStatusDigits(arg1, arg2, arg0);
        BattleDrawSlotStatusCellIcons(arg0, arg1, arg2);
        BattleDrawSlotStatusText(arg0, arg1, arg2);
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
   above are for this body.
   Re-measured and narrowed (round 405): the third loop is ONE mechanism, loop.c
   strength-reducing the height index `i * 4` into its own giv (`move s0,zero` in
   the preheader, `addiu s0,s0,4` in the latch) where retail computes
   `sll v0,s0,2` per iteration. The pColumns pointer alone does not help: 96.09%
   at 316 insns (1264), because the giv takes the byte-offset register and pushes
   pCursor to $s3. A QImode index pseudo DOES block the reduction - a `u_char
   column` counter, or a `u_char c = i;` copy - emits retail's `sll` plus
   `lh D_800C3404($v0)` with no giv (proved in cc1_probe), but it costs two
   `andi 0xff` (the index conversion and the loop test, which becomes
   `andi`+`sltiu`): 95.47% at 317 insns / 1268 bytes. Retail has neither an andi
   nor an sltiu, so its index is s32 and something other than register pressure
   kept its multiply alive. Also measured, all 96.09-96.28%: heights through
   `((s16 *)D_800C3404)[i * 2]` and through a flat `s16 D_800C3404[]` + `[i * 2]`
   (both hoist the table base into a register in the preheader, `lh 0($s0)`,
   `addiu s0,s0,4`, at retail's 1260/315), a separate `s32 col` counter, a
   `s16 height` local for the fourth argument, and a flat `s16 *pColumns`. The
   table-base-hoist levers (g-strength-reduce-table-base,
   k-array-base-pointer-local) point the wrong way here: this needs the multiply
   UNREDUCED while the base stays in the %hi/%lo relocation. // Re-measured round 430 by unparking and diffing: 96.97% at 1260 bytes / 315 insns,
// retail's exact size. The whole residual is still the third loop's one strength-reduced
// multiply: cc1 turns `i * 4` into a mult giv (`move s0,zero` in the preheader,
// `addiu s0,s0,4` in the latch) and reads D_800C3404 through `addu at,at,s0`, where
// retail recomputes `sll v0,s0,2` per iteration. Three more spellings measured this
// round, the giv in every one: `*(s16 *)((u8 *)D_800C3404 + (i << 2))` compiles exactly
// like `D_800C3404[i][0]` (the front end canonicalises the shift back into the multiply),
// 96.97 at 315/1260, so the keep-the-multiply family is closed for shift spellings too;
// a `s16 (*pColumns)[2]` local plus `column`/`height` locals does reproduce retail's
// pointer for D_800C33C4 and its `addiu s2,s2,4` but the giv then takes $s0, pushing
// `i` to $s1 and the cursor to $s3, 96.97 at 315/1260; a flat `s16 *pColumns` with
// `pColumns[i * 2]` is 96.03 at 316/1264. Retail has TWO givs - a base-combined pointer
// for D_800C33C4 and the unreduced multiply for D_800C3404 - and cse shares the single
// mult(i,4) tree between both addresses whatever the spelling, so loop.c only ever sees
// one candidate. Tried and measured before this: QImode counter (blocks the giv, costs
// andi+sltiu, 95.47), flat `s16 D_800C3404[]` + `[i * 2]` (base hoisted, 96.28).
// Round 443 addendum (third round on this body, re-measured from the unparked park):
// 96.97% / 1260 bytes / 315 insns, retail's exact size, 50 differing lines, all of them
// the third loop. The `.s` shows the difference is the ADDRESS FORM of the columns table:
// this body sends BOTH tables through the strength-reduced byte giv (`lui at,%hi(D_800C33C4)`
// / `addu at,at,s0` / `lh a2,0(at)` per iteration, `addiu s0,s0,4` in the latch) while
// retail hoists a pointer for the columns (`lui/addiu` in the preheader, `lh a2,0(s2)`,
// `addiu s2,s2,4` in the latch) and keeps `sll v0,s0,2` for the heights. The register
// evidence is that retail's giv is allocated LAST in the loop (i->$s0, cursor->$s1,
// derived pointer->$s2), so this is not a pressure tie: retail's loop.c made no mult giv
// at all. Re-confirmed this round that a MULT giv of a SINGLE use is still reduced (the
// `s16 (*pColumns)[2]` + `pColumns++` body leaves `$s2` as retail's pointer but puts the
// giv in `$s0`, i in `$s1` and the cursor in `$s3`, 96.09% at 316 insns / 1264 bytes), so
// the "one use is unprofitable" theory is dead and the sll has to come from a multiply
// loop.c never recorded as a giv - which no spelling tested across three rounds reaches.
// The lane's permuter rule does not apply (below 99%). See the two lever entries this
// body wrote: g-qimode-index-blocks-the-mult-giv, and g-scaled-record-index-is-a-giv.
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092298);

// 86.30% (988 bytes / 247 insns against retail's 1008 / 252), re-measured by un-parking
// and diffing in round 493 - unchanged, and the two residuals below are still the only ones.
// 70.43 -> 86.30 came from three changes, each measured:
// 1. THE TWO i==7 TESTS ARE WRITTEN `if (i != 7) {arg1 path} else {i==7 path}`: retail's
//    `beq s1,v0` and its block order (then-block inline, else out of line) only appear in
//    that orientation. 70.43 -> 84.62.
// 2. pBufs[i] IS STORED THROUGH A SOURCE POINTER, `*(pSlot = pBufs + i) = pBuf;` --
//    battle/main/misc3's BattleLoadActionRowGraphics, the same array-of-0x1B-buffers
//    idiom. That is what removes the 4*i BIV (`addu s3,s4,(sp+16)` plus `addiu s4,s4,4`
//    in the latch) and gives retail's fresh `sll v1,s1,2` / `addiu v0,sp,0x40` /
//    `addu s3,v1,v0`, base-first.
// 3. THE ONES DIGIT: retail tests arg1[i] BEFORE computing arg2[i] % 10, i.e. the modulus
//    lives inside the if, not in a preceding statement.
//
// RESIDUAL 1, THE FRAME (0x98 against retail's 0x90): retail rematerialises
// `addiu a0,sp,0x18` (the tens rect's address) in both tens blits; this body has loop.c
// HOIST it (`addiu t0,sp,0x18` into the preheader) and reload spills it to 0x68(sp), so
// with 10 live values for 9 registers `6` is rematerialised as three `li t0,6` and every
// s-register is shifted. MEASURED CAUSE, the cleanest experiment of the round: write the
// tens blit ONCE after the if/else with the source in a local (so the address has ONE
// use) and the hoist disappears -- frame comes out 0x90 with retail's slots and a
// rematerialised `addiu a0,sp,0x18` -- but the body collapses to 243 insns / 972 bytes,
// one jal where retail has two. So retail has two tens blits AND no hoist, and 2.6.0's
// loop.c move_movables hoists a 2-use invariant address. A pointer local for the second
// use (`pRect = &tensRect;` then passing pRect) is copy-propagated away: identical 988.
//
// RESIDUAL 2, THE GLYPH TAIL: retail keeps all four glyphRect stores in BOTH arms (8
// stores) and shares only the jal; this body cross-jumps the y/w/h stores plus the jal
// into one tail, which is where 3 of the 5 missing instructions went, and it also makes
// the i==7 arm fall into that shared tail instead of retail's own 4 stores + jal.
// Breaking the merge by reordering the 7-arm's stores deviates from retail's address
// order (x,y,w,h in both arms), so it was not taken.
//
// Tried and measured dead: pBufs[7] spelled literally in the i==7 arm (byte-identical;
// gcc already folds pBufs[i] to 0x5C there), gcc-2.7.2 (82.35) and gcc-2.7.2-cdk
// (74.51) on this body, so the preset is not the answer.
//
// ROUND 493 measurements on the same body:
// 4. THE i==7 ARM'S GLYPH RECT WRITTEN AS LITERALS (glyphRect.x = 0x39E; y = 0x132) does
//    break the x store's CSE (248 insns / 992 bytes) but the y/w/h stores STILL cross-jump
//    into one shared tail, and the whole allocation rotates: 85.60%. So retail's special
//    case is not literal-spelled, and the merge is not driven by the x store.
// 5. THE TENS HOIST IS PER-SITE, NOT ALL-OR-NOTHING (the shape round 438 called exclusive):
//    with both calls taking &tensRect the preheader gets `addu $8,sp,24` + `sw $8,104(sp)`,
//    yet the i==7 arm REMATERIALISES `addu $4,sp,24` (its pressure is one value lower - arg1
//    is dead there) while the then-arm does `lw $4,104($sp)`. Retail rematerialises at BOTH.
//    So remat is reachable and the residual is one site's reload choice plus the frame.
//    (Round 438 already measured the pointer-local spelling as byte-identical, so the only
//    untried axis left is a spelling that raises the then-arm's pressure argument.)
/*
void func_80092784(u8 arg0, u8 *arg1, u8 *arg2) {
    RECT glyphRect;
    RECT tensRect;
    RECT onesRect;
    RECT tileRect;
    RECT secondRect;
    RECT rect;
    u_long *pBufs[8];
    u_long *pBuf;
    u_long **pSlot;
    u_long *pData;
    u8 tensDigit;
    u8 onesDigit;
    s32 i;

    BattleAllocPacketPrimitives();
    BattleLoadActionRowGraphics();
    g_BattleTextWorkspace = BattleAllocTextBuffer(0x39);
    bzero(g_BattleTextWorkspace, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, g_BattleTextWorkspace);
    for (i = 0; i < 8; i++) {
        pBuf = BattleAllocTextBuffer(0x1B);
        *(pSlot = pBufs + i) = pBuf;
        bzero(pBuf, 0x30C);
        tileRect.x = (i % 2) * 0x1E + 0x380;
        tileRect.y = (i / 2) * 16 + 0x100;
        tileRect.w = 0x1B;
        tileRect.h = 0x10;
        BattleLoadImageAndSync(&tileRect, g_BattleTextWorkspace);
        if (i != 7) {
            if (arg1[i] == 0xFF) {
                goto nextTile;
            }
            SystemRenderStringEntry(GetStringEntryFromBank(g_BattleActorCharIds[arg0], arg1[i]), *pSlot, 0x1B, 0);
            glyphRect.x = (i % 2) * 0x1E + 0x380;
            glyphRect.y = (i / 2) * 16 + 0x102;
            glyphRect.w = 0x1E;
            glyphRect.h = 0xD;
        } else {
            SystemRenderStringEntry(GetStringEntryFromBank12(0xA), pBufs[i], 0x1B, 0);
            glyphRect.x = (i % 2) * 0x1E + 0x380;
            glyphRect.y = (i / 2) * 16 + 0x102;
            glyphRect.w = 0x1E;
            glyphRect.h = 0xD;
        }
        BattleLoadImageAndSync(&glyphRect, *pSlot);
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
        if (i != 7) {
            tensDigit = arg2[i] / 10;
            BattleLoadImageAndSync(&tensRect, tensDigit != 0 ? g_BattleDigitImageBuffers[tensDigit] : g_BattleTextWorkspace);
            onesRect.x = (i % 2) * 16 + 0x3C2;
            onesRect.y = (i / 2) * 16 + 0x102;
            onesRect.w = 6;
            onesRect.h = 0xD;
            onesDigit = arg2[i] % 10;
            pData = g_BattleTextWorkspace;
            if (arg1[i] != 0xFF) {
                pData = g_BattleDigitImageBuffers[onesDigit];
            }
        } else {
            BattleLoadImageAndSync(&tensRect, g_BattleTextWorkspace);
            onesRect.x = (i % 2) * 16 + 0x3C2;
            onesRect.y = (i / 2) * 16 + 0x102;
            onesRect.w = 6;
            onesRect.h = 0xD;
            pData = g_BattleTextWorkspace;
        }
        BattleLoadImageAndSync(&onesRect, pData);
    }
    func_80092298(arg0, arg1);
    for (i = 0; i < 8; i++) {
        HeapFree(pBufs[i]);
    }
    HeapFree(g_BattleTextWorkspace);
    BattleDrawSlotStatusPanel();
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092784);


/* 89.40% (1256 bytes against retail's 1336, 314 insns vs 334), frame 0x30 = retail's, four
//    callee-saved against retail's five. Re-measured this round by un-parking and diffing.
//    WON THIS ROUND (two levers worth having in levers.md):
//    1. THE HEAD AND BOTH PACKAGES ARE NOW INSTRUCTION-IDENTICAL. The digit split is
//       `u8 tens = arg1 / 10; digits[0] = tens; digits[1] = arg1 - tens * 10;` - one scalar
//       local. `arg1 % 10` puts the multiply on the ANDI'd copy of the quotient, and a bare
//       `arg1 - digits[0] * 10` makes cc1 RELOAD the element (`lbu v1,16(sp)`); only the
//       scalar gives retail's `sb q,16(sp)` / `andi v0,v0,0xff` / sll-addu-sll / `subu
//       a1,a1,v1` with arg1 RAW on the left.
//    2. Tail index: the per-store read of g_BattlePrimStartIndex has to be a volatile
//       LVALUE. A `volatile s32 *pPrimIndex` local scores 89.40 but pays one shared `la`
//       (a1) plus 16 `lw 0(a1)`, i.e. 14 instructions short of retail's 16
//       `lui v1,%hi / lw v1,%lo(v1)`. MEASURED DEAD: the same volatile cast spelled inline at
//       all 16 sites (identical - cc1 cse's the ADDRESS of a volatile MEM into one la); a
//       second symbol g_BattlePrimIndexWord at 0x800CCB34 declared plain s32 (cse folds all
//       16 reads into ONE: 65.97%, 1044 bytes, 261 insns); the same second symbol declared
//       volatile (this one DOES give retail's exact per-store lui+lw and 324 insns, but
//       sched2 then interleaves two store groups at a time - load,load,chain,chain,store,store
//       - for 70.71%, where retail's tail is in strict source order). So the last 16
//       instructions need a per-store two-instruction reload that sched2 leaves in order:
//       volatile is the only spelling that re-reads, and it reorders.
//    3. `n` as a second induction variable (`for (i = 0, n = -1; i < 7; i++, n++)` with
//       D_800C3DE1[n]): reproduces retail's `li s2,-1` and its `addiu s2,s2,1` in the bnez
//       delay slot plus the un-folded D_800C3DE1 reloc, but costs a sixth callee-saved
//       register (frame 0x38 against retail's 0x30) and measures 88.92%. Without it the -1
//       folds into the reloc (D_800C3DE0) and that read's addu takes s0.
//    4. The latch: retail re-materialises the pitch per iteration (`lui v0,0x20; addu s3,s3,v0;
//       addu s1,s1,v0`, 3 insns) where this body hoists 0x200000 into a callee-saved register
//       (1 insn). Same one-register story as (3): retail spends its five slots on i, x1, x2,
//       n and arg1 and rematerialises the constant.
//    Residual, all named: 16 (the tail index reads above), 2 (the folded D_800C3DE1 / -1 pair),
//    2 (the hoisted pitch), 1 (the queue byte index gives `addu v0,s0,v0` where retail is
//    base-first `addu v0,v0,s0`, and `move s0,zero` sits in the beqz delay slot in retail and
//    later here). */
// 93.92% (1336 bytes, 334 insns = EXACTLY retail's size and count), frame 0x38 against retail's 0x30.
//    WON THIS ROUND (89.40 -> 93.92, size now exact): the tail's 16 stores reproduce retail's
//    per-store lui %hi/lw %lo reload of the packet index AND its serial order when (a) the index
//    comes from a second volatile s32 extern at the same address 0x800CCB34 (g_BattlePrimIndexWord,
//    declared in this file only: splat rejects a second name at one address) and (b) the pool is written through a 'volatile
//    BattlePacketPool *'. Measured: a volatile POINTER local gives one hoisted la plus 16 lw 0(a1)
//    (89.40); a plain non-volatile symbol read is cse'd to ONE load (67.26); without the volatile
//    pool sched2 hoists the next volatile read above the previous store and interleaves two chains
//    (330 insns, 95.05, order right only inside each chain). So volatile store = scheduling
//    barrier, volatile symbol read = the per-store reload.
//    Loop: two counters (for (i = 0, n = -1; i < 7; i++, n++), D_800C3DE1[n]) give retail's
//    li s2,-1 plus addiu s2,s2,1 in the bnez delay slot and keep the reloc at D_800C3DE1 unfolded;
//    without n the -1 folds into the addend (%hi(D_800C3DE1-0x1)) and the bytes differ.
//    RESIDUAL, named: the pitch 0x200000 is hoisted into $s4 where retail rematerialises
//    lui v0,0x20 in the latch (+1 insn) and that hoist is the sixth callee-saved (retail is
//    s0=i, s1=x1, s2=n, s3=x2, s4=arg1 and nothing else, so arg1 takes $s5 and the frame is 0x38).
//    Measured dead: swapping the two increments, the 0x200000 decimal spelling (both 93.9x, still
//    hoisted); dropping n (frame 0x30, 330 insns, 95.05, but the D_800C3DE1 addend folds). Also
//    left: 16 addu v1,v1,v0 where retail is addu v0,v0,v1 (addu operand order lever, untried);
//    the queue byte gives addu v0,s0,v0 for retail's base-first addu v0,v0,s0 (1); retail re-reads
//    g_BattleGraphicsWork for the final sb 0x66F (2, tied to the register count); the tail's
//    li a2,0x80 / andi a1,s4,0xff sit after the two sb's here instead of before (ordering); and
//    the reloc NAME differs on all 16 stores (g_BattlePrimIndexWord vs g_BattlePrimStartIndex at
//    one address), so a perfect body still needs check_bin.sh, not objdiff.
//    RAW: g_BattleCommandQueue[i + 0x2C4] indexes a byte 0x2C4 into a BattleCommandQueueState
//    (misc1.c defines that struct in its own .c; a header move would fix the spelling).
/*
void func_80092B74(u8 arg0, u8 arg1) {
    BattleGraphicsWork *pWork;
    u8 digits[2];
    s32 x;
    s32 x1;
    s32 x2;
    s32 i;
    s32 n;
    s32 idx;
    u8 tens;

    tens = arg1 / 10;
    digits[0] = tens;
    digits[1] = arg1 - tens * 10;
    g_BattleDisplayState->spriteColumnPackageCount = 0;
    if (digits[0] != 0) {
        g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(digits[0] + 0x67,
            (u8 *)g_BattleGraphicsWork + 0xBA8, 0x56, 0x38);
    }
    g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(digits[1] + 0x67,
        (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * 0x50, 0x5E, 0x38);
    // x1/x2 are pixel positions kept left-justified by 16, the shape BattleDrawSpritePackage's
    // rows use in this overlay (battle/main/misc14 keeps its pitch the same way).
    x2 = 0x1A << 16;
    x1 = 0x2A << 16;
    for (i = 0, n = -1; i < 7; i++, n++) {
        if (g_BattleCommandQueue[i + 0x2C4] == 0xFF) {
            break;
        }
        g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(D_800C3DE1[n] + 0x39,
            (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * 0x50, x1 >> 16,
            0x4A);
        if (i != 0) {
            g_BattleDisplayState->spriteColumnPackageCount += BattleDrawSpritePackage(0xA,
                (u8 *)g_BattleGraphicsWork + 0xBA8 + g_BattleDisplayState->spriteColumnPackageCount * 0x50,
                x2 >> 16, 0x4A);
        }
        x2 += 0x20 << 16;
        x1 += 0x20 << 16;
    }
    pWork = g_BattleGraphicsWork;
    g_BattleDisplayState->spriteColumnPrimStart = g_BattlePrimStartIndex[0];
    g_BattleDisplayState->spriteColumnPending = 1;
    x = arg1 * 4 + 0x80;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->gouraudQuads[idx].x0 = 0x80;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->gouraudQuads[idx].y0 = 0x34;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->gouraudQuads[idx].x1 = x;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->gouraudQuads[idx].y1 = 0x34;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->gouraudQuads[idx].x2 = 0x80;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->gouraudQuads[idx].y2 = 0x3C;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->gouraudQuads[idx].x3 = x;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->gouraudQuads[idx].y3 = 0x3C;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->flatQuads[idx].x0 = x;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->flatQuads[idx].y0 = 0x34;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->flatQuads[idx].x1 = 0xF0;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->flatQuads[idx].y1 = 0x34;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->flatQuads[idx].x2 = x;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->flatQuads[idx].y2 = 0x3C;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->flatQuads[idx].x3 = 0xF0;
    idx = g_BattlePrimIndexWord;
    ((volatile BattlePacketPool *)pWork->pPacketPrims)->flatQuads[idx].y3 = 0x3C;
    pWork->pPacketPrims[0x66C] = g_BattlePrimStartIndex[0];
    pWork->pPacketPrims[0x66F] = 1;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092B74);



/* 94.01% (1224 bytes against retail's 1228, 306 vs 307 instructions) under this
// unit gcc 2.6.0. Frame 0xB8 against retail 0xB0 and the ten saved values are
// retail's; the four local slots 0x70/0x78/0x88 are 8 apart as retail's are.
// THE WIN this round (93.09 -> 94.01): move every read of the value pointer onto
// a per-iteration copy, pCur = pValue at the top of the outer loop, with all
// three *pCur uses and only pValue++ left on the original. That is the allocator
// lever: pValue refs drop, its pseudo loses the priority contest, and retail
// likewise reloads its value pointer once per outer iteration. Adding just the
// store copy (pCur only around "*-= digit * divisors[j]") is 93.52 / 1216.
// RESIDUAL: retail spills FOUR values (arg0 at 0x68, pValue 0x70, pId 0x78, the
// y counter 0x80) and recomputes &glyphRect as "addiu a0,sp,0x20" at its single
// use; this body spends a fifth slot on pGlyphRect (stored to 0x68 in the
// preheader, reloaded before that call) and keeps pCur in a register, which is
// the +8 frame byte. Retail also reads its y counter with lhu (a u16) where the
// i*16+0x100 induction variable comes out lw.
// Measured dead this round: inline &glyphRect instead of pGlyphRect with this
// body (87.13 / 1216 / 304); a real "u16 y" counter with "y += 0x10" at the loop
// end (83.91 / 1188 / 297, frame 0xA0 - gcc then keeps y in a register and the
// memory induction variable retail has never forms); modifying arg1/arg2 in
// place (arg1++, arg2++) with no pointer locals (87.71 / 1212 / 303, frame 0xB8).
// Permuter, third and fourth runs this round: base 1676 -> best 1547 (that is
// what produced pCur) -> 1482, but the 1482 candidate caches "new_var = &(*pValue)"
// before the loop and reads it as the current value, i.e. it reads slot 0 forever:
// WRONG CODE, rejected. Earlier rounds: 5255 -> 4281 -> 3657, plus try_flags
// 2.7.2 93.47, cdk 83.96, 2.6.0 -O3 93.09, -G0 93.09, so the preset is not it.
// */
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
    u8 *pSlotText;
    u16 *pCur;
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
    BattleLoadActionRowGraphics();
    g_BattleTextWorkspace = BattleAllocTextBuffer(0x39);
    bzero(g_BattleTextWorkspace, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, g_BattleTextWorkspace);
    pId = arg1;
    pValue = arg2;
    pGlyphRect = &glyphRect;
    for (i = 0; i < 4; i++) {
        pCur = pValue;
        pBufs[i] = BattleAllocTextBuffer(0x1B);
        bzero(pBufs[i], 0x30C);
        tileRect.x = 0x380;
        tileRect.y = i * 16 + 0x100;
        tileRect.w = 0x1B;
        tileRect.h = 0x10;
        BattleLoadImageAndSync(&tileRect, g_BattleTextWorkspace);
        started = 0;
        if (*pId != 0xFF) {
            pSlotText = &g_BattleSlotTextIndex[arg0 * 0x170];
            SystemRenderStringEntry(GetStringEntryFromBank34(*pSlotText * BATTLE_SLOT_COMMAND_COUNT + i),
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
            value = *pCur;
            digit = value / divisors[j];
            if (digit != 0 || started != 0) {
                BattleLoadImageAndSync(&tensRect, g_BattleDigitImageBuffers[digit]);
                started = 1;
                *pCur -= digit * divisors[j];
            } else {
                BattleLoadImageAndSync(&tensRect, g_BattleTextWorkspace);
            }
        }
        onesRect.x = 0x3C6;
        onesRect.y = i * 16 + 0x102;
        onesRect.w = 6;
        onesRect.h = 0xD;
        if (*pId != 0xFF) {
            value = *pCur;
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
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + *(volatile s32 *)g_BattlePrimStartIndex * 0x28),
                         0x94, 0x54, 0, 0, 0x60, 0x40);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                     (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0xA0)),
                         0xFC, 0x54, 0, 0, 0x20, 0x40);
    g_BattleGraphicsWork->pPacketPrims[0x668] = g_BattlePrimStartIndex[0];
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->unkB7 = 4;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800930AC);





void BattleDrawActionRowIcons(u8 arg0, u8 arg1) {
    u8 *pRec;
    u16 flags;
    u16 pal;
    s32 row;
    s32 col;
    u32 off;

    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                     (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x140)),
                         0x7C, 0xA4, 0, (u8)(arg1 * 0x10), 0x60, 0x10);
    off = (arg1 + 0x25) * 0x28 + arg0 * 0x690;
    flags = *(u16 *)(g_BattleActionRows + off);
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
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x320)),
                            0x24, 0xA6, D_800D2F6A[row * 4], D_800D2F6B[row * 4], g_BattleActionRowIconSprites[row * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[0] * 0x28));
    if (D_800D2F69[row * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x370)),
                            0x48, 0xA6, D_800D2F6A[col * 4], D_800D2F6B[col * 4], g_BattleActionRowIconSprites[col * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims - (-(((s32 *)g_BattlePrimStartIndex)[0] * 0x28));
    if (D_800D2F69[col * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}
void BattleDrawSlotStatusTextEntry3(u8 arg0, u8 arg1) {
    RECT rect;
    u8 *pBuf;
    u16 index;
    s32 altEntry;
    s32 len1;
    s32 len2;

    index = (g_BattleSlotTextIndex[arg0 * 0x170] * BATTLE_SLOT_COMMAND_COUNT + arg1) * 2;
    altEntry = 1;
    pBuf = (u8 *)BattleAllocTextBuffer(BATTLE_TEXT_SLOT_BUFFER_SIZE);
    bzero(pBuf, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
    len1 = SystemRenderStringEntry(GetStringEntry(g_BattleEntry3StringTable, index), pBuf, BATTLE_TEXT_SLOT_BUFFER_SIZE, 0);
    // load-bearing s32, as in BattleDrawSlotStatusText: cc1 keeps the HImode index's
    // zero-extension only when the OR's right operand is a register.
    len2 = SystemRenderStringEntry(GetStringEntry(g_BattleEntry3StringTable, index | altEntry), pBuf, BATTLE_TEXT_SLOT_BUFFER_SIZE, 1);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, pBuf);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x280)), 0x88, 0xB6, 0, 0, len1 & 0xFF);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0x2D0)), 0x88, 0xC6, 0, 0, len2 & 0xFF);
    HeapFree(pBuf);
}
// objdiff stays at 99.87 because retail names the per-slot row table g_BattleSlotStatusRows where
// this body reaches the same word as g_BattleWorkBlock[slot].conditionFlags (reloc addend only).
void BattleDrawCharStatusIfMaskUnset(u8 arg0, u8 arg1) {
    u16 tbl[BATTLE_SLOT_COMMAND_COUNT];

    tbl[0] = D_800C324E;
    tbl[1] = D_800C3250;
    tbl[2] = D_800C3252;
    tbl[3] = D_800C323A;
    if ((u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].statusFlags,
                           arg1)) {
        if ((g_BattleWorkBlock[arg0].conditionFlags & tbl[arg1]) == 0) {
            BattleDrawActionRowIcons(arg0, arg1);
            BattleDrawSlotStatusTextEntry3(arg0, arg1);
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
/* 92.06% (1560 bytes against retail's 1588, 390 insns against 397; vars 0x58
   against retail's 0x68, frame 0xA0 against retail's 0xB0). Everything
   structural is right and measured: the char-4/unkB7 guard, places[] = {100, 10},
   the character-vs-gear weapon-id pick (characters[].extraWeaponIds[0]/[3] with the
   0x2286 quantities, gears[characters[].gearId] with the 0x22B6 ones), the two
   text-buffer passes, both BattleLoadImageAndSync uploads per row, the slash and
   percent digit passes with the drawn latch, the free loop, and the tail written
   with the house pool idiom spelled inline at every use.
   ROUND 489 RESIDUAL, named: retail homes THREE values in memory and keeps SEVEN
   across the inner loop's calls - s7 textY, fp rowY, i, j, digitX, drawn and the
   bufs[i] pointer, with s1/s2 spent on the per-iteration places/counts addresses -
   while every body measured here keeps a spare register, so cc1 either spills rowY
   instead or merges the digit-loop offset into a walking counts pointer.
   Retail's three slots, in its own allocation order: digitY 0x70 (word store from
   s7 once per row, read back as lhu into digitRect.y - so it is an s32 copy of
   textY whose use is a u16 truncation), ofs 0x78 (word store from countOfs at the
   stripRect point, lw per j), and countOfs 0x80 (its home; += 2 at the loop tail).
   Retail's digit loop re-forms addu s2,ofs,base and addu s1,j*2,base per iteration
   off base sp+0x20 (nameRect's address, the +0x48 to counts folded into the load),
   and the tensRect read after the loop goes through countOfs at 0x80 with the
   0x68 displacement - not through ofs.
   MEASURED IN ROUND 489 (gcc-2.6.0 -O2, the unit preset):
   - adding s32 ofs and s32 digitY copies and reading the tensRect digit through
     ofs: 90.20, 1600 bytes, 400 insns, frame 0xB8. Spills countOfs 0x68, ofs 0x70,
     digitY 0x78 (right three slots, wrong order) PLUS two slots 0x80/0x88 holding
     &counts itself - the hoisted invariant and a strength-reduced copy of it.
   - the same with the tensRect digit read through countOfs (retail's own flow):
     92.06, 1560 bytes, 390 insns, frame 0xA0. The two &counts slots and ofs
     vanish; cc1 strengthens counts+countOfs into fp (addu fp,fp,2 at the tail) and
     spills rowY to 0x68 and digitY to 0x70. Layout is retail's shifted 8 low with
     rowY where retail has countOfs.
   - round 476 measured: preset is right (2.7.2 89.42, cdk 73.36, -O3 = -O2); the
     RECT declarations in retail's slot order (nameRect first) give 87.56 at
     retail's exact 1588 bytes/397 insns/0xB0 with the addresses of
     nameRect/tensRect/counts spilled instead of the values; that order plus a dead
     0x48 slot is 87.54 and one slot too many; a dead 0x48 slot in THIS body's order
     is 92.09.
   - retail also has an unused EIGHT-BYTE slot at 0x48 between stripRect and bufs
     (no 0x48(sp) access anywhere in its .s), which is why bufs/places/weaponIds/
     counts sit at 0x50/0x58/0x60/0x68 rather than 0x48/0x50/0x58/0x60. A dead
     u_long pad[2] does allocate that slot; it needs the three retail spills to be
     present first, or the frame overshoots.
   So the shape to attack is: get seven loop-carried values into s0-s7+fp, put ofs
   and a digitY copy in memory WITHOUT cc1 copy-propagating them into countOfs, and
   leave rowY in fp. Parking here because two rounds of declaration, order and
   copy-shape permutations have moved this delta around (87.5-92.1) without removing
   it, and the function is 397 instructions. */
/*
void func_80093B08(u8 arg0) {
    RECT tensRect;
    RECT nameRect;
    RECT digitRect;
    RECT tileRect;
    RECT stripRect;
    void *bufs[2];
    u16 places[2];
    u8 weaponIds[2];
    u16 counts[2];
    s32 textY;
    s32 rowY;
    s32 countOfs;
    s32 ofs;
    s32 digitY;
    s32 i;
    s32 j;
    s32 digit;
    s32 digitX;
    u8 drawn;

    if (g_BattleActorCharIds[arg0] == 4 && g_BattleDisplayState->unkB7 == 0) {
        places[0] = 100;
        places[1] = 10;
        if (g_BattleSlotFlags[arg0 * 8] == 0) {
            weaponIds[0] = g_GameState.characters[g_BattleActorCharIds[arg0]].extraWeaponIds[0];
            weaponIds[1] = g_GameState.characters[g_BattleActorCharIds[arg0]].extraWeaponIds[3];
            counts[0] = ((u8 *)&g_GameState)[0x2286 + weaponIds[0]];
            counts[1] = ((u8 *)&g_GameState)[0x2286 + weaponIds[1]];
        } else {
            weaponIds[0] = g_GameState.gears[g_GameState.characters[g_BattleActorCharIds[arg0]].gearId]
                               .extraWeaponIds[0];
            weaponIds[1] = g_GameState.gears[g_GameState.characters[g_BattleActorCharIds[arg0]].gearId]
                               .extraWeaponIds[3];
            counts[0] = ((u8 *)&g_GameState.unk22B6)[weaponIds[0]];
            counts[1] = ((u8 *)&g_GameState.unk22B6)[weaponIds[1]];
        }
        BattleAllocPacketPrimitives();
        BattleLoadActionRowGraphics();
        g_BattleTextWorkspace = BattleAllocTextBuffer(0x39);
        bzero(g_BattleTextWorkspace, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
        textY = 0x102;
        rowY = 0x100;
        countOfs = 0;
        for (i = 0; i < 2; i++) {
            bufs[i] = BattleAllocTextBuffer(0x1B);
            bzero(bufs[i], 0x30C);
            tileRect.x = 0x380;
            tileRect.y = rowY;
            tileRect.w = 0x1B;
            tileRect.h = 0x10;
            BattleLoadImageAndSync(&tileRect, g_BattleTextWorkspace);
            drawn = 0;
            if (weaponIds[i] != 0) {
                SystemRenderStringEntry(GetWeaponName(weaponIds[i]), bufs[i], 0x1B, 0);
                nameRect.x = 0x380;
                nameRect.y = textY;
                nameRect.w = 0x1E;
                nameRect.h = 0xD;
                BattleLoadImageAndSync(&nameRect, *(void **)((u8 *)bufs + bufOff));
            }
            digitX = 0x3C0;
            digitY = textY;
            stripRect.x = 0x3C0;
            stripRect.y = rowY;
            ofs = countOfs;
            stripRect.w = 0x1B;
            stripRect.h = 0x10;
            BattleLoadImageAndSync(&stripRect, g_BattleTextWorkspace);
            for (j = 0; j < 2; j++) {
                digitRect.x = digitX;
                digitRect.y = digitY;
                digitRect.w = 6;
                digitRect.h = 0xD;
                digit = *(u16 *)((u8 *)counts + ofs) / places[j];
                if ((u8)digit != 0 || drawn != 0) {
                    BattleLoadImageAndSync(&digitRect, g_BattleDigitImageBuffers[(u8)digit]);
                    *(u16 *)((u8 *)counts + ofs) -= (u8)digit * places[j];
                    drawn = 1;
                } else {
                    BattleLoadImageAndSync(&digitRect, g_BattleTextWorkspace);
                }
                digitX += 2;
            }
            tensRect.x = 0x3C4;
            tensRect.y = textY;
            tensRect.w = 6;
            tensRect.h = 0xD;
            BattleLoadImageAndSync(&tensRect, weaponIds[i] != 0
                                                    ? g_BattleDigitImageBuffers[*(u16 *)((u8 *)counts + countOfs) % 10]
                                                    : g_BattleTextWorkspace);
            textY += 0x10;
            rowY += 0x10;
            countOfs += 2;
        }
        for (i = 0; i < 2; i++) {
            HeapFree(bufs[i]);
        }
        HeapFree(g_BattleTextWorkspace);
        ((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + (((s32 *)g_BattlePrimStartIndex)[0] * 0x28)))->clut =
            g_SystemPalette1;
        BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                         ((s32 *)g_BattlePrimStartIndex)[0] * 0x28),
                             0xA0, 0xAC, 0, 0, 0x60, 0x20);
        BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                         (((s32 *)g_BattlePrimStartIndex)[0] * 0x28 + 0xA0)),
                             0x104, 0xAC, 0, 0, 0x18, 0x20);
        g_BattleGraphicsWork->pPacketPrims[0x668] = g_BattlePrimStartIndex[0];
        g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
        BattleInitSlotGraphics(0, 0x98, 0xA8, 0x8C, 0x28, 0, 1);
        g_BattleDisplayState->unkB7 = 5;
        ((u8 *)g_BattleDisplayState)[0x8E] = 1;
    }
}*/


void BattleClearSlotFlagsIfChar4(u8 arg0, u8 arg1) {
    if (g_BattleActorCharIds[arg0] == 4) {
        g_BattleDisplayState->unkB7 = 0;
        g_BattleDisplayState->unkB0 = 0;
        if (arg1 != 0) {
            BattleReleaseEffectSlotIfChar4();
        }
    }
}
void BattleRunCurrentActionRow(void) {
    BattleTableEntry *pSlot;
    u16 flags;
    u32 mask;
    GameCharacterSlot *pWeaponSlots;
    GameCharacterSlot *pWeaponSlot;
    s32 weaponSlot;
    BattleRow *pRow;
    u8 i;

    BattleClearAllSlotStates();
    D_800D2DB8 = 0;
    D_800C34AE = 0;
    g_BattleTargetSlotIndex = g_BattleCurWorkBlock->actorIndex;
    g_BattleActingSlot = &g_BattleCurWorkBlock->slots[g_BattleTargetSlotIndex];
    g_BattleTargetSlotState = &g_BattleActingSlot->state;
    if (g_BattleCurWorkBlock->slots[g_BattleCurWorkBlock->actorIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        BattleExecuteActionRow();
        BattleSetSlotEntryFieldsTo1(g_BattleTargetSlotIndex);
        return;
    }
    if (g_BattleTargetSlotIndex < MAX_PARTY_MEMBERS) {
        g_BattleCurrentRow = (BattleRow *)((u8 *)g_BattleCurWorkBlock +
                                          (g_BattleTargetSlotIndex * 0x5F0 + 0x1058) +
                                          g_BattleCurWorkBlock->rowIndex * 0x28);
    } else {
        g_BattleCurrentRow = (BattleRow *)((u8 *)g_BattleCurWorkBlock +
                                          (g_BattleCurWorkBlock->rowIndex * 0x28 + 0x35D8));
    }
    BattleSetSlotEntryFieldsTo2000(g_BattleTargetSlotIndex, 1);
    pRow = g_BattleCurrentRow;
    if (pRow->actionFlags & 0x10) {
        BattleExecuteActionRow();
        BattleSetSlotEntryFieldsTo1(g_BattleTargetSlotIndex);
        BattleAccumulateSlotValueByRowFlag();
        return;
    }
    g_BattleAmountThirdFlag = 0;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        if (g_BattleActingSlot->characterId == 1) {
            BattleRollPercentAndSetState38();
        }
    }
    if (g_BattleActingSlot->characterId == 4) {
        if ((u8)(g_BattleCurWorkBlock->rowIndex - 4) < 2) {
            weaponSlot = g_BattleCurWorkBlock->rowIndex - 3;
            pWeaponSlots = (GameCharacterSlot *)g_BattleActingSlot;
            pWeaponSlot = pWeaponSlots + weaponSlot;
            g_BattleCurrentRow->element = pWeaponSlot->attack;
        }
    }
    if (g_BattleActingSlot->activeEffectFlags2 & 0x20) {
        if ((g_BattleActionSlotMask[0] & BATTLE_PARTY_SLOT_MASK) != 0 &&
            g_BattleActionSlotMask[0] < BATTLE_PARTY_SLOT_MASK) {
            for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
                if (g_BattleWorkBlock[i].characterId == 3) {
                    g_BattleActionSlotMask[0] = 1 << i;
                }
            }
        }
    }
    mask = 1;
    g_BattleCurSlotIndex = 0;
    while (g_BattleCurSlotIndex < BATTLE_ACTOR_SLOT_COUNT) {
        if (mask & g_BattleCurWorkBlock->activeSlotMask) {
            pSlot = &g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex];
            g_BattleTargetSlotEntry = pSlot;
            g_BattleCurSlotState = (u8 *)&pSlot->state;
            D_800C3D60 = &pSlot->state.commandCounter;
            if (g_BattleCurSlotIndex < MAX_PARTY_MEMBERS) {
                BattleRollPercentAndSwapActiveActor();
            }
            ((void (*)(void))D_800C348C[g_BattleCurrentRow->opcode])();
            BattleResolveSlotActionState();
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
    if (g_BattleTargetSlotIndex < MAX_PARTY_MEMBERS &&
        g_BattleCurWorkBlock->rowIndex < BATTLE_STAT_WEIGHT_COUNT) {
        if (g_BattleActingSlot->statWeights[g_BattleCurWorkBlock->rowIndex] <= BATTLE_STAT_WEIGHT_MAX) {
            g_BattleActingSlot->statWeights[g_BattleCurWorkBlock->rowIndex] +=
                ((u8 *)&g_BattleActingSlot->unk54)[1] + g_BattleActingSlot->accessoryEffectFlags10;
        }
    }
    BattleAccumulateSlotValueByRowFlag();
    (void)D_800D2DB8;
}

void BattleResolveSlotActionState(void) {
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
                value = g_BattleActingSlot->actionCount + 1;
                g_BattleActingSlot->actionCount = value;
                if (value > BATTLE_ACTION_COUNT_MAX) {
                    g_BattleActingSlot->actionCount = value - 1;
                }
            }
        }
        flags = g_BattleTargetSlotEntry->activeEffectFlags2;
        g_BattleTargetSlotEntry->activeEffectFlags2 = flags & 0xEFFF;
        if (flags & BATTLE_GEAR_EFFECT_FLAG) {
            if (rand() % BATTLE_PERCENT_ROLL_RANGE < BATTLE_GEAR_EFFECT_CLEAR_CHANCE_PERCENT) {
                g_BattleTargetSlotEntry->activeEffectFlags2 &= ~BATTLE_GEAR_EFFECT_FLAG;
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
        if (g_BattleTargetSlotEntry->accessoryEffectFlags5 & BATTLE_ACCESSORY_AMOUNT_HALVE_FLAG) {
            limit = BATTLE_AMOUNT_HALVE_CHANCE_PERCENT;
            if (g_BattleTargetSlotEntry->characterId == 0) {
                limit = BATTLE_AMOUNT_HALVE_CHANCE_PERCENT_CHARACTER_0;
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
    atkValue = BattleComputeActionAttackValue();
    defValue = BattleComputeSlotActionAmount();
    pAtk = g_BattleActingSlot;
    if ((pAtk->activeEffectFlags4 | pAtk->accessoryEffectFlags4) & BATTLE_EFFECT_DAMAGE_BONUS_20) {
        atkValue += atkValue / 5;
    }
    if ((pAtk->activeEffectFlags4 | pAtk->accessoryEffectFlags4) & BATTLE_EFFECT_DAMAGE_BONUS_10) {
        atkValue += atkValue / 10;
    }
    if ((pAtk->activeEffectFlags4 | pAtk->accessoryEffectFlags4) & BATTLE_EFFECT_DAMAGE_MALUS_20) {
        atkValue -= atkValue / 5;
    }
    if ((pAtk->activeEffectFlags4 | pAtk->accessoryEffectFlags4) & BATTLE_EFFECT_DAMAGE_MALUS_10) {
        atkValue -= atkValue / 10;
    }
    pDef = g_BattleTargetSlotEntry;
    if ((pDef->activeEffectFlags4 | pDef->accessoryEffectFlags4) & BATTLE_EFFECT_DAMAGE_MALUS_20) {
        defValue += defValue / 5;
    }
    if ((pDef->activeEffectFlags4 | pDef->accessoryEffectFlags4) & BATTLE_EFFECT_DAMAGE_MALUS_10) {
        defValue += defValue / 10;
    }
    if ((pDef->activeEffectFlags4 | pDef->accessoryEffectFlags4) & BATTLE_EFFECT_DAMAGE_BONUS_20) {
        defValue -= defValue / 5;
    }
    if ((pDef->activeEffectFlags4 | pDef->accessoryEffectFlags4) & BATTLE_EFFECT_DAMAGE_BONUS_10) {
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
    if (pAtk->accessoryEffectFlags4 & BATTLE_GEAR_EFFECT_FLAG) {
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
    if (g_BattleWorkBlock[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
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
    if (g_BattleCurrentRow->conditionKind != BATTLE_CONDITION_KIND_CLEAR_STATUS) {
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
