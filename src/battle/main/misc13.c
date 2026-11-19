#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

// The queue's command key: BattleClearCommandSlots clears these bytes and
// BattleCheckCommandKeyStatusBit compares them against a command row's.
#define BATTLE_COMMAND_KEY_LENGTH 7
// Command byte of a slot that holds no command: BattleClearCommandSlots and
// BattleInitSlotRowsForActor store it, BattleCheckActorCommandAvailable refuses to
// index through it (queued commands are 0xF4-0xFD, see battle/main/misc4.c).
#define BATTLE_COMMAND_NONE 0xFF
// The command slot rows BattleCheckCommandKeyStatusBit walks (g_BattleCommandSlotRows):
// one per key a queued command can match, the found-row ladder below counting down
// from the last of them.
#define BATTLE_COMMAND_SLOT_COUNT 13
// The rows of the 0x48-byte command record table BattleInitSlotRowsForActor clears.
#define BATTLE_COMMAND_RECORD_COUNT 32
// The most steps a linear move takes in one frame: BattleSetupLinearMove rolls
// g_BattleLerpStepsPerFrame from 1 to it.
#define BATTLE_LERP_STEPS_PER_FRAME_MAX 8

extern u8 *g_BattleCommandQueue;
extern u8 *g_BattleCommandSlotRows[];
extern u8 *g_BattleCommandStatusBytes[];
extern volatile u8 *g_BattleDisplayState;
extern u8 g_BattleSlotRecords[];
extern u8 g_BattleCharSlotUsage[];
extern u16 g_BattleStateFlags;
extern u16 g_BattleSlotFlag8000Mask;
extern u16 g_BattleActionSlotMask;
extern u16 g_BattleActionStateFlags;
extern u16 g_BattleActionParam;
extern u8 g_BattleActionActorSlot;
extern u8 g_BattleActionSlotIndex;
// One 0x48-byte row of the command record table at g_BattleCommandRecords: the
// queueing actor's slot at +0xD, the row's slot mask at +0 and the command byte at
// +0x31 (BATTLE_COMMAND_NONE while the row is empty).
typedef struct {
    /* 0x00 */ u16 slotMask;
    /* 0x02 */ u8 unk2[0xB];
    /* 0x0D */ u8 actorSlot;
    /* 0x0E */ u8 unkE[0x23];
    /* 0x31 */ u8 command;
    /* 0x32 */ u8 unk32[0x16];
} BattleCommandRecord; // Size: 0x48

// The active/accessory pair of effect flag words: one row of the five the slot
// entry carries, each active word at +0 and the accessory-granted copy of the same
// bits at +0x2 (include/battle/actor.h, BattleTableEntry's activeEffectFlags).
typedef struct {
    /* 0x0 */ u16 active;
    /* 0x2 */ u16 accessory;
} BattleSlotEffectFlags; // Size: 0x4

// The 0x170-byte battle slot entry as the stat display reads it: the hp pair at +0
// and the five active-effect words from +0x30. The rest of the entry is
// BattleSlotEntry in src/battle/main/misc8.c.
typedef struct {
    /* 0x000 */ u16 hp;
    /* 0x002 */ u16 maxHp;
    /* 0x004 */ u8 unk4[0x2C];
    /* 0x030 */ BattleSlotEffectFlags activeEffects[5];
    /* 0x044 */ u8 unk44[0x12C];
} BattleSlotStatEntry; // Size: 0x170

extern BattleCommandRecord g_BattleCommandRecords[];
extern BattleSlotStatEntry g_BattleSlotEntriesBase[];
extern u8 D_800CCD36[];
extern u8 g_BattleSoundEnabled;
extern u8 g_BattleSlotDisplayInitialized;
extern u8 g_BattleSlotFlags[];
extern u8 D_800C3EB5[];
extern u8 D_800D301D[];
extern u8 D_800C3EBE[];
extern u8 D_800C3EC0[];
extern u8 *g_BattleSlotParamTable;
extern u8 D_800C48EC[];
extern u8 D_800C48EE[];
extern u8 D_800C48F0[];

extern void BattleApplySlotCurve(u8);
extern void func_80085618(u8);
extern void BattleInitActiveSlotActions(u16);
extern void func_800941A4(void);
extern u16 BattleSlotBitMask(u8);
extern void func_80085D34(void);
extern s32 func_800877E0(s32, s32);
extern s32 BattleFindMinValueOtherSlot(s32);
extern void func_800B89FC(s32, s32, s32, s32);
extern u8 BattleSlotBitMaskInverted(u8);
extern s32 func_8009ADA0(u8, s32 *);
extern void BattleClearSlotCurveEntry(void);
extern void BattleAttachActorAnimScripts(u8, s32, s32, s32);
extern u8 g_BattleActorCommandIds[];
extern u16 g_BattleCommandRows[];
extern u8 *g_BattleGraphicsWork;
extern u8 g_BattlePrimStartIndex[];
extern u8 *g_BattleSpriteColumnWork;
extern u8 D_800C33B0[];
extern void BattleSetPrimGrey80BlendAdd(u8 *);
extern void BattleSetPrimGrey80BlendSub(u8 *);
extern s32 GetStringEntryFromBank(s32, s32);
extern void func_80076D58(u8 *, u8, u8);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *, u16, u16, u8, u8, u8);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern s32 BattleDrawSpritePackage(s32, s32, s16, s16);
extern u8 g_BattleCommandCharIndex[];
extern u8 g_BattleActorCharIds[];
extern u8 g_BattleCommandStatusMasks[];
extern s32 BattleMaskHasStatusBit(s32, u8);
extern s32 g_BattleLerpStartX;
extern s32 g_BattleLerpStartY;
extern s32 g_BattleLerpTargetX;
extern s32 g_BattleLerpTargetY;
extern s32 g_BattleLerpStepX;
extern s32 g_BattleLerpStepY;
extern u8 g_BattleLerpDirX;
extern u8 g_BattleLerpDirY;
extern s32 g_BattleLerpStepsPerFrame;
extern s32 g_BattleLerpFracX;
extern s32 g_BattleLerpFracY;
extern u8 g_BattleLerpDone;
extern u8 GameRandomByteRange(s32, u8);
extern s32 BattleMaskHasSlotBit(s32, u8);

/* NOT ATTEMPTED - decoded map only (no body written, nothing measured).
   311 insns, owns the FIRST jump table of the unit (jtbl_80070250, 12 entries,
   arms shared: 0/5/7/8 -> .L8008570C, 1/9 -> .L80085908, 4/6 -> .L80085A70,
   2 -> .L8008583C, 3 -> .L80085954, 10 -> .L800859CC, 11 -> .L80085A18).
   Shape: for (i = 0; i < 11; i++) over battle slots, early skip when
   g_BattleSlotEnabledFlags[i] == 0, then a switch on
   g_BattleActorCommandIds[arg0 * 0x48 + i] (u8, lbu, bounds sltiu 0xC, no index
   copy so a plain switch already produces retail's operands). i*0x170 is held in
   $s0 as a byte offset for the gp-relative per-slot tables (g_BattleSlotEntries,
   g_BattleSlotEntriesBase, D_800CCD36, D_800CCDEC, D_800CCDF0, D_800CCE08,
   D_800CCD38/3A, D_800CCDC4/C6); $s3 = g_BattleCommandRows + arg0*0x48 + i*2;
   $s5 = g_BattleCommandRows - 0x130 + i*0x1C; $s4 = g_BattleCommandRows - 0x138
   + i*0x170; $s6 = D_800CCDC4 + i*0x170. The four stores that go through
   $s4 + 0x8F3C / +0x8E84 / +0x8E88 / +0x8F14 are absolute per-slot symbols that
   fit neither addiu nor a plain lui pair, so gcc emitted lui 0x1 + addu + %lo
   with a negative displacement - expect those to need the exact D_ address.
   Loop tail: s3 += 2, s6/s4/s0 += 0x170, s2 += 1, slti 0xB, s5 += 0x1C.
   Every case ends by storing 1 to g_BattleCommandQueue[i + 0x2EB]. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085618);
void BattleResetActorStatDisplay(s32 arg0) {
    s32 i = 2;
    BattleSlotEffectFlags *pFlags;

    g_BattleSlotEntriesBase[(u8)arg0].hp = g_BattleSlotEntriesBase[(u8)arg0].maxHp;

    pFlags = &g_BattleSlotEntriesBase[(u8)arg0].activeEffects[1];
    for (; i >= 0; i -= 2) {
        pFlags->active = 0;
        pFlags--;
    }

    i = 4;
    pFlags = &g_BattleSlotEntriesBase[(u8)arg0].activeEffects[4];
    for (; i >= 0; i -= 2) {
        pFlags->active = 0;
        pFlags--;
    }
}
void BattleBeginActorStatAnimation(s32 arg0) {
    s32 buf[3];
    s32 i;
    u16 v;
    u8 *p;

    buf[0] = buf[1] = buf[2] = 0;
    if (func_8009ADA0((u8)arg0, buf) != 0) {
        for (i = 0; i < 3; i++) {
            if (buf[i] != 0) {
                g_BattleCommandQueue[0x2DA] = 0;
                BattleClearSlotCurveEntry();
                g_BattleActorCommandIds[(u8)arg0] = i + 8;
                v = buf[i];
                p = g_BattleCommandQueue;
                g_BattleCommandRows[(u8)arg0] = v;
                func_80085618(p[0x2DA]);
            }
        }
        BattleAttachActorAnimScripts((u8)arg0, buf[0], buf[1], buf[2]);
    }
}
void BattleBeginAction(s32 arg0, u16 arg1, u16 arg2) {
    u16 v = g_BattleStateFlags;

    g_BattleSlotFlag8000Mask = 0;
    g_BattleActionSlotMask = arg1;
    g_BattleActionStateFlags = v;
    BattleInitActiveSlotActions(arg2);
}
void BattleApplySlotCurveAndStep(u8 arg0) {
    u8 v = arg0;

    BattleApplySlotCurve(v);
    func_80085618(v);
    g_BattleDisplayState[0xAD] = 0;
}
void BattleBeginActorAction(u8 arg0, u16 arg1, u16 arg2) {
    u8 *p = g_BattleCommandQueue;
    u16 v = g_BattleStateFlags;
    u8 c;

    g_BattleSlotFlag8000Mask = 0;
    g_BattleActionActorSlot = arg0;
    c = p[0x2DC];
    g_BattleActionSlotMask = arg1;
    g_BattleActionParam = arg2;
    g_BattleActionStateFlags = v;
    g_BattleActionSlotIndex = c + 0xFF;
    func_800941A4();
}
// 99.63%: 324 bytes (= retail), identical instruction multiset, identical
// schedule, identical registers; the ONLY difference is the operand slots of the
// three addu that reorg places in the BattleDrawSpritePackage jal delay slots:
// retail `addu a1,a1,v0` (chain in src1, base in src2), gcc `addu a1,v0,a1`.
// The register split is already retail's (chain a1, base v0) in the pointer form;
// only the RTL operand order differs, so the source order of a `base + int` pointer
// sum cannot steer it.
// Tried and measured, all 99.63% or worse: ofs + base; base + ofs; &base[ofs];
// base + 0x9C8 + ofs; (s32)base + ofs (75.54%, 320 bytes); ofs + (s32)base (74.25%);
// u32 casts (74.25%); int sums with an embedded assignment to an s32 local, base
// first (75.98%) and base last (83.19%); a volatile re-read of the base combined
// with the chain-first int sum (82.94%: right operand order, mirrored registers);
// `u8 * const volatile` g_BattleGraphicsWork; a separate ofs local per call (70.57%, gcc then
// evaluates it before the first-argument lbu).
// Round 79 concluded the integer forms reproduce retail's operand order - they do
// not: every int-typed sum lets gcc's reassociation move 0x9C8 onto the BASE
// register (addiu v0,v0,2504) and the addu still comes out base-first, which is
// what costs the 4 bytes. Only the pointer form keeps 0x9C8 on the offset chain.
// Also probed: gcc-2.7.2-cdk and -O3 both give 99.63% (tools/scripts/try_flags.sh),
// so this is not a preset question. Permuter: 1154 iterations in round 79, 2398
// more in round 98, 3503 more in round m103-p13, base=30 best=30 every time.
// g_BattleDisplayState is declared `volatile u8 *`: that was the permuter's own best edit
// (base 250 -> 30) and the only change it ever found.
// Reads g_BattlePrimStartIndex with lbu, so it must stay u8 (s32 gives 94.94%). The SECOND
// call's first argument is the constant 0x19, not g_BattleCommandQueue[0x2D4] + 0xF.
/*
void func_80085D34(void) {
    s32 v;

    g_BattleDisplayState[0x7B] = 0;
    v = BattleDrawSpritePackage(g_BattleCommandQueue[0x2D4] + 0xF, (s32)(g_BattleGraphicsWork + (g_BattleDisplayState[0x7B] * 0x50 + 0x9C8)), 0x2A, 0xD0);
    g_BattleDisplayState[0x7B] += v;
    v = BattleDrawSpritePackage(0x19, (s32)(g_BattleGraphicsWork + (g_BattleDisplayState[0x7B] * 0x50 + 0x9C8)), 0x32, 0xD0);
    g_BattleDisplayState[0x7B] += v;
    v = BattleDrawSpritePackage(g_BattleCommandQueue[0x2D5] + 0xF, (s32)(g_BattleGraphicsWork + (g_BattleDisplayState[0x7B] * 0x50 + 0x9C8)), 0x3A, 0xD0);
    g_BattleDisplayState[0x7B] += v;
    g_BattleDisplayState[0xA4] = g_BattlePrimStartIndex;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085D34);
void BattleClearCommandSlots(void) {
    s32 i;

    for (i = 0; i < BATTLE_COMMAND_KEY_LENGTH; i++) {
        u8 *p = g_BattleCommandQueue + i;

        p[0x2CC] = BATTLE_COMMAND_NONE;
    }
    g_BattleCommandQueue[0x2D6] = 0;
}
/* PARKNOTE 99.30%, 372 bytes (= retail): the instruction stream is retail's except the
   register that holds the switch index. Retail copies the row counter into a
   CALLER-SAVED temp (`addu v1,a3` in the delay slot of `bnez matched`) and uses
   that temp for both `sltiu v0,v1,13` and `sll v0,v1,2`; every plain local we
   write is copy-propagated into a3 (368 bytes, 97.74%, one instruction short --
   that size can never link). Reusing `result` as the switch scrutinee does
   reproduce the copy and the 372-byte size (this body) but it is NOT the real
   source: it clobbers the return value so the default path returns i, and the
   copy lands in s0 because result crosses the jal. `matched = i; switch
   (matched)` puts the copy in v1 (retail's register) but gcc still folds the
   scrutinee back to a3 and drops the inner loop's per-iteration `j = 0` reset
   (98.92%, 372 bytes). So the open lever is one variable that is a distinct
   pseudo at the switch (a plain copy is coalesced) and is dead across the jal;
   a permuter run from the plain `switch (i)` body (base=326) found only the
   `result = i` edit (best=37).
   Also note the ladder: gcc 2.7.2 emits the 12 increment arms in source order
   with the fall-through arm first-listed, so cases 0..11 must be listed with
   `j++` and no break, case 12 holds the body.
   `i = 0` is assigned before `pSlots = pQueue` (retail's order: a3=0 then
   t1=a2); the second copy of the queue pointer is what produces `move t1,a2`
   in the preheader. `D_800CCD36[arg1 * 0x170 + 8]` is retail's D_800CCD3E and
   folds to the same bytes.
*/
// Walks the 13 command slots looking for the row whose 7-byte key matches the
// queue's; the arm ladder below counts the found row DOWN from 12 into j, which
// is why each case falls through into the next.
s32 BattleCheckCommandKeyStatusBit(u8 arg0, u8 arg1) {
    s32 i;
    s32 j;
    s32 result = 0;
    s32 matched;
    u8 *pQueue = g_BattleCommandQueue;
    u8 *pSlots;
    u8 *pRow;
    u8 *pKey;
    u8 *pEntry;

    if (pQueue[0x2D6] == 0) {
        goto done;
    }
    if (arg0 != 4) {
        goto done;
    }
    i = 0;
    pSlots = pQueue;
    for (; i < BATTLE_COMMAND_SLOT_COUNT; i++) {
        pRow = g_BattleCommandSlotRows[i];
        for (j = 0; j < BATTLE_COMMAND_KEY_LENGTH; j++) {
            pKey = pSlots + j;
            if (pKey[0x2CC] != pRow[j]) {
                matched = 0;
                break;
            }
            matched = 1;
        }
        j = 0;
        if (matched) {
            break;
        }
    }
    matched = i;
    switch (matched) {
    case 0:
        j++;
    case 1:
        j++;
    case 2:
        j++;
    case 3:
        j++;
    case 4:
        j++;
    case 5:
        j++;
    case 6:
        j++;
    case 7:
        j++;
    case 8:
        j++;
    case 9:
        j++;
    case 10:
        j++;
    case 11:
        j++;
    case 12:
        pEntry = g_BattleCommandStatusBytes[D_800CCD36[arg1 * 0x170 + 8]];
        if ((u16)BattleMaskHasStatusBit(*(u16 *)&g_GameState.unk16BC[g_BattleActorCharIds[arg1]].unk4,
                                        pEntry[0xC - j]) != 0) {
            result = 1;
        }
    }
done:
    return result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086028);
/* 91.62%, 424 bytes (retail 432). Instruction multiset and schedule match; every
   remaining difference is the s-register NUMBER each call-crossing value gets.
   Retail: s0=q s1=arg0 s2=arg2 s3=arg1 s4=r s5=arg3 s6=arg5 s7=arg4 fp=arg1*4;
   ours: s0=arg5 s1=q s2=arg0 s3=r s4=arg1 s5=arg3 s6=arg1*4 s7=arg4 s8=arg2 - an
   allocator priority tie, and we come out 8 bytes short because the two stack
   parameters are read into different registers. Permuter: 1663 iterations, base=1530
   best=1125 (not zero).
   Three spellings in this body are measured, not style: `((s32 *)g_BattlePrimStartIndex)[0]`
   reads the counter with retail's `lw` (a plain u8 spelling is lbu, 88.08%);
   `arg2 += arg5 + 1;` reproduces retail's `addiu s6,s6,1 / addu s2,s2,s6`
   (folding it back into the call argument is 88.08%); and the callee's first two
   rect arguments are declared u16 because retail passes `& 0xFFFC`/`& 0xFFF8` with no
   sign extension (s16 makes gcc add `sll 0x10`/`sra 0x10`, 89.45% - note misc16_r1/r2
   declare the same callee with s16, which is worth re-checking against its definition).
   Also tried and measured: the permuter's `(arg1 * 2) + counter` operand swap.
/*
s32 func_80086028(u8 arg0, s32 arg1, s32 arg2, u8 arg3, u32 **arg4, u8 arg5) {
    s32 q;
    s32 r;
    s32 len;
    RECT rect;

    q = arg1 / 2;
    r = arg1 % 2;
    func_80076D58((u8 *)(arg1 * 0x50 + 0x5550 + g_BattleSpriteColumnWork), r, 3);
    len = SystemRenderStringEntry((u8 *)GetStringEntryFromBank(g_BattleActorCharIds[arg0], arg3), *arg4, 0x1B, r);
    rect.x = q * 0x1E + 0x3C0;
    rect.y = 0x1A;
    rect.w = 0x1E;
    rect.h = 0xD;
    LoadImage(&rect, *arg4);
    arg2 += arg5 + 1;
    BattleSetPolyFT4RectH13((POLY_FT4 *)((((s32 *)g_BattlePrimStartIndex)[0] + arg1 * 2) * 0x28 + 0x5550 + g_BattleSpriteColumnWork),
                  (arg2 * 0x10 + 0x50 + arg1 * 4) & 0xFFFC,
                  (0xC8 - arg1 * 0x10) & 0xFFF8,
                  (q * 0x78) & 0xF8,
                  0x1A,
                  len);
    g_BattleSpriteColumnWork[0x5D80]++;
    return arg1 + 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800861D0);
u8 BattleCheckActorCommandAvailable(s32 arg0, u8 arg1) {
    s32 ok = 1;
    s32 idx;

    idx = arg0 + (g_BattleCommandQueue[0x2CC] + 1) * 3;
    if (g_BattleCommandQueue[0x2D6] != 0) {
        if (g_BattleCommandCharIndex[0] == 4) {
            idx = arg0 + 0xC;
        }
        if ((u16)BattleMaskHasStatusBit(*(u16 *)&g_GameState.unk16BC[g_BattleActorCharIds[arg1]].unk8,
                               g_BattleCommandStatusMasks[idx & 0xFF]) != 0) {
            if (g_BattleCommandQueue[0x2CC] != BATTLE_COMMAND_NONE && g_BattleCommandCharIndex[0] != 4 && g_BattleCommandCharIndex[0] < g_BattleCommandQueue[0x2CC] + 1) {
                ok = 0;
            }
        } else {
            ok = 0;
        }
    }
    return ok;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086C88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086F98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800877E0);
/* 59.15%: retail recomputes `g_BattleSlotRecords[a*0x1C] * 0x40` and `[b*0x1C] * 8`
   inside the loop on every one of the three blocks, while gcc hoists them into
   the loop preheader as loop invariants (the difference is ~60 bytes). Spelling
   the index inline (`g_BattleSlotRecords[a * 0x1C]`, no ofsA/ofsB locals, 62.57%) makes
   gcc form `la g_BattleSlotRecords` base pointers instead of retail's per-access
   lui `%hi` + addu + `%lo`. Also open: retail materialises `+ 0x140` with an
   explicit addiu before the `+ i` in the first block (it re-reads through the
   same $a0) while ours folds 0x140 into the load displacement.
s32 func_800877E0(s32 arg0, s32 arg1) {
    s32 i;
    s32 o;
    u8 a;
    u8 b;
    s32 ofsA;
    s32 ofsB;
    s32 res;

    res = 0;
    for (i = 0, o = 0; i < 9; i++, o += 6) {
        *(u16 *)(D_800C48EC + o) = 0xFFFF;
        *(u16 *)(D_800C48EE + o) = 0xFFFF;
    }
    a = arg0;
    ofsA = a * 0x1C;
    b = arg1;
    ofsB = b * 0x1C;
    *(u16 *)(D_800C48EC + 0) = *(u16 *)(D_800C3EBE + ofsA);
    *(u16 *)(D_800C48EE + 0) = *(u16 *)(D_800C3EC0 + ofsA);
    D_800C48F0[0] = 0;
    o = 6;
    for (i = 1; i < 8; i++, o += 6) {
        if (*(u8 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 0x40 + g_BattleSlotRecords[ofsB] * 8 + 0x140 + i) == 0xFF) {
            break;
        }
        *(u16 *)(D_800C48EC + o) =
            *(u16 *)(g_BattleSlotParamTable + ((*(u8 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 0x40 + g_BattleSlotRecords[ofsB] * 8 + 0x140 + i) & 7) << 5));
        *(u16 *)(D_800C48EE + o) =
            *(u16 *)(g_BattleSlotParamTable + ((*(u8 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 0x40 + g_BattleSlotRecords[ofsB] * 8 + 0x140 + i) & 7) << 5) + 2);
        D_800C48F0[o] = *(u8 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 0x40 + g_BattleSlotRecords[ofsB] * 8 + 0x140 + i) & 0x80;
    }
    if (g_BattleActorCharIds[a] == 4) {
        res = 1;
    }
    return res;
}
*/
void BattleInitSlotRowsForActor(s32 arg0, s32 arg1) {
    s32 i;

    for (i = 0; i < BATTLE_COMMAND_RECORD_COUNT; i++) {
        g_BattleCommandRecords[i].command = BATTLE_COMMAND_NONE;
        g_BattleCommandRecords[i].actorSlot = arg0;
        g_BattleCommandRecords[i].slotMask = BattleSlotBitMask(arg1);
    }
}
void BattleSetupActorSlotDisplay(u8 arg0) {
    s32 idx;
    s32 r;

    func_80085D34();
    idx = arg0 * 0x40;
    BattleInitSlotRowsForActor(arg0, (g_BattleCommandQueue + idx)[0x3C]);
    g_BattleSoundEnabled = 0;
    r = func_800877E0(arg0, (g_BattleCommandQueue + idx)[0x3C]);
    func_800B89FC(r, arg0, (g_BattleCommandQueue + idx)[0x3C], BattleFindMinValueOtherSlot(arg0));
    g_BattleSoundEnabled = 1;
    g_BattleSlotDisplayInitialized = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80087AF0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80087EDC);
/* 85.48%: retail keeps neither table offset in a register across the first
   compare (both `arg * 0x1C` are scratch there) and reuses ONE saved register
   ($s1) for ofsB in the middle and ofsA at the end; with ofsA/ofsB as locals
   gcc keeps both live in $s1/$s3 from the top (692 vs 684 bytes, same
   instruction multiset). Spelling the offsets inline instead makes gcc hoist
   `la g_BattleSlotRecords` as a base pointer (67.96%, 0x30 frame). Also open: the sel
   test order - retail computes `sltiu(arg0,3)` in the compare's delay slot,
   `xori/sll 3`, then `sltiu(arg1,3)` + `beqz`/`move sel,0`/`move sel,v1`;
   `(arg1 < 3) ? (arg0 >= 3) * 8 : 0` (82.50%), `sel = (arg0>=3)*8; if
   (arg1>=3) sel = 0;` (85.48%) both put the arg1 test first. The tail's
   `slot*4 + char*0x20 g_BattleSlotParamTable` operand order and the `if (arg0 < 3)` layout
   (then = <3 at offsets 4/6, else = >=3 at 0x10/0x12) are right.
void func_80087EDC(u8 arg0, u8 arg1) {
    s32 ofsA;
    s32 ofsB;
    s32 sel;
    s32 i;

    ofsA = arg0 * 0x1C;
    ofsB = arg1 * 0x1C;
    if (g_BattleSlotRecords[ofsA] == g_BattleSlotRecords[ofsB]) {
        return;
    }
    sel = (arg0 >= 3) * 8;
    if (arg1 >= 3) {
        sel = 0;
    }
    if (g_BattleCharSlotUsage[(g_BattleSlotRecords[ofsB] + sel) << 2] >= 4) {
        return;
    }
    BattleReleaseCharSlot(arg0);
    g_BattleCharSlotUsage[(g_BattleSlotRecords[ofsB] + sel) << 2]++;
    for (i = 0; i < 4; i++) {
        if (BattleMaskHasSlotBit(D_800D301D[(g_BattleSlotRecords[ofsB] + sel) << 2], i) == 0) {
            break;
        }
    }
    D_800C3EB5[ofsA] = i;
    g_BattleSlotRecords[ofsA] = g_BattleSlotRecords[ofsB];
    D_800D301D[(g_BattleSlotRecords[ofsA] + sel) << 2] |= BattleSlotBitMask(D_800C3EB5[ofsA]);
    if (arg0 < 3) {
        *(u16 *)(D_800C3EBE + ofsA) = *(u16 *)(D_800C3EB5[ofsA] * 4 + g_BattleSlotRecords[ofsA] * 0x20 + g_BattleSlotParamTable + 4);
        *(u16 *)(D_800C3EC0 + ofsA) = *(u16 *)(D_800C3EB5[ofsA] * 4 + g_BattleSlotRecords[ofsA] * 0x20 + g_BattleSlotParamTable + 6);
    } else {
        *(u16 *)(D_800C3EBE + ofsA) = *(u16 *)(D_800C3EB5[ofsA] * 4 + g_BattleSlotRecords[ofsA] * 0x20 + g_BattleSlotParamTable + 0x10);
        *(u16 *)(D_800C3EC0 + ofsA) = *(u16 *)(D_800C3EB5[ofsA] * 4 + g_BattleSlotRecords[ofsA] * 0x20 + g_BattleSlotParamTable + 0x12);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800881B8);
/* 79.99%, 488 bytes (retail 500). NOTE the old 81.95% park note decoded `sel`
   WRONG: retail is `sel = (arg1 < 3 && arg0 < 3) ? 0x18 : 0x10` (the
   `ori $a2,0x18` in the second beqz's delay slot is skipped only when arg0 < 3),
   not `arg1 >= 3 && arg0 < 3`. With the right condition the shape is right and the
   residual is: retail keeps ofsB in the saved $s2 (reused after the
   BattleReleaseCharSlot call) and re-forms ofsA into $a1 after the call, while gcc
   wants both offsets live (472 bytes, 2 saved regs) or neither (508 bytes with an
   `la` base pointer for g_BattleSlotRecords, which is what spelling `arg0 * 0x1C`
   inline twice costs). Retail also re-loads g_BattleSlotRecords[ofsA] four times
   where gcc forwards the value it just stored.
   Spelling measured: both offsets as locals from the top (472, 81.02%), ofsB local +
   arg0 inline at the compare (488, this body), arg0 inline everywhere (508, 77.47%).
/*
void func_800881B8(u8 arg0, u8 arg1) {
    s32 ofsA;
    s32 ofsB;
    u8 sel;

    ofsB = arg1 * 0x1C;
    if (g_BattleSlotRecords[arg0 * 0x1C] == g_BattleSlotRecords[ofsB]) {
        return;
    }
    sel = 0x10;
    if (arg1 < 3 && arg0 < 3) {
        sel = 0x18;
    }
    if (g_BattleCharSlotUsage[(g_BattleSlotRecords[ofsB] + sel) << 2] != 0) {
        return;
    }
    BattleReleaseCharSlot(arg0);
    ofsA = arg0 * 0x1C;
    D_800C3EB5[ofsA] = 0;
    g_BattleSlotRecords[ofsA] = g_BattleSlotRecords[ofsB];
    g_BattleCharSlotUsage[(g_BattleSlotRecords[ofsA] + sel) << 2] = 1;
    D_800D301D[(g_BattleSlotRecords[ofsA] + sel) << 2] = 1;
    if (arg0 < 3) {
        *(u16 *)(D_800C3EBE + ofsA) = *(u16 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 8 + 0x100);
        *(u16 *)(D_800C3EC0 + ofsA) = *(u16 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 8 + 0x102);
    } else {
        *(u16 *)(D_800C3EBE + ofsA) = *(u16 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 8 + 0x104);
        *(u16 *)(D_800C3EC0 + ofsA) = *(u16 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 8 + 0x106);
    }
}
*/
void BattleReleaseCharSlot(u8 arg0) {
    s32 ofs;
    u8 sel;
    u8 invMask;

    sel = (arg0 >= 3) * 8;
    if (g_BattleSlotFlags[arg0 * 8] != 0) {
        sel |= 0x10;
    }
    ofs = arg0 * 0x1C;
    g_BattleCharSlotUsage[(g_BattleSlotRecords[ofs] + sel) << 2]--;
    /* The do/while keeps gcc from hoisting the mask argument above the store. */
    do {
        invMask = BattleSlotBitMaskInverted(D_800C3EB5[ofs]);
    } while (0);
    D_800D301D[(g_BattleSlotRecords[ofs] + sel) << 2] &= invMask;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80088490);
/* 68.06%, 316 bytes (retail 300): everything matches except (1) retail lays the
   skip path out as a `j .Lstore` / `addu a2,v1` trampoline BEFORE the find loop and
   BRANCHES INTO the loop (`bnez $v0,.Lloop` with `ori a1,0x40` - the loop's
   `(i + 0x10) << 2` induction - in its delay slot), while gcc puts the loop
   straight after the test (`beqz $v0,store`); (2) consequently the `g_BattleSlotRecords`
   offset lives in $v1 in retail (a0 in ours) and retail stores D_800D301D before
   g_BattleCharSlotUsage where we emit them the other way round (source order and the
   do/while block both measured no change - sched2 reorders). The first offset is
   spelled inline and the second assigned to `ofs` because retail recomputes
   `arg0 * 0x1C` at the store block (63.19% with a single live `ofs`).
   Tried: one live `ofs` for both (63.19%), `n = i; break` vs a separate counter
   (unchanged), swapping the two flag stores (unchanged), and 0x1C spelled inline at
   every site (56.81%).
/*
void func_80088490(u8 arg0) {
    s32 ofs;
    s32 i;
    s32 n;

    n = g_BattleSlotRecords[arg0 * 0x1C];
    if (g_BattleCharSlotUsage[(n + 0x10) << 2] != 0) {
        for (i = 0; i < 8; i++) {
            if (g_BattleCharSlotUsage[(i + 0x10) << 2] == 0) {
                n = i;
                break;
            }
        }
    }
    ofs = arg0 * 0x1C;
    g_BattleSlotRecords[ofs] = n;
    D_800C3EB5[ofs] = 0;
    D_800D301D[(g_BattleSlotRecords[ofs] + 0x10) << 2] = 1;
    g_BattleCharSlotUsage[(g_BattleSlotRecords[ofs] + 0x10) << 2] = 1;
    *(u16 *)(D_800C3EBE + ofs) = *(u16 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofs] * 8 + 0x100);
    *(u16 *)(D_800C3EC0 + ofs) = *(u16 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofs] * 8 + 0x102);
}
*/
u8 BattleIsCharSlot18InUse(u8 arg0) {
    return g_BattleCharSlotUsage[(g_BattleSlotRecords[arg0 * 0x1C] + 0x18) << 2];
}
// Draws the two sprite-column halves of the slot display: two package draws per half
// followed by the blend pass over the prims each one appended. pPrimStart keeps the
// prim counter's address in a register across the middle draw, as retail does.
void func_8008860C(void) {
    s32 i;
    u8 *pPrimStart;

    for (i = 0; i < 2; i++) {
        g_BattleSpriteColumnWork[0x5D74] += BattleDrawSpritePackage(D_800C33B0[i], (s32)(g_BattleSpriteColumnWork[0x5D74] * 0x50 + (0x1720 + (s32)g_BattleSpriteColumnWork)), 0xA0, 0x64);
    }
    pPrimStart = g_BattlePrimStartIndex;
    g_BattleSpriteColumnWork[0x5D83] = *pPrimStart;
    g_BattleSpriteColumnWork[0x5D70] = BattleDrawSpritePackage(0xA8, (s32)(g_BattleSpriteColumnWork[0x5D70] * 0x50 + (s32)g_BattleSpriteColumnWork), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D92] = *pPrimStart;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D74]; i++) {
        BattleSetPrimGrey80BlendAdd((u8 *)((i * 2 + g_BattleSpriteColumnWork[0x5D83]) * 0x28 + (0x1720 + (s32)g_BattleSpriteColumnWork)));
    }
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D70]; i++) {
        BattleSetPrimGrey80BlendAdd((u8 *)((i * 2 + g_BattleSpriteColumnWork[0x5D92]) * 0x28 + (s32)g_BattleSpriteColumnWork));
    }
    for (i = 2; i < 4; i++) {
        g_BattleSpriteColumnWork[0x5D7E] += BattleDrawSpritePackage(D_800C33B0[i], (s32)(g_BattleSpriteColumnWork[0x5D7E] * 0x50 + (0x2530 + (s32)g_BattleSpriteColumnWork)), 0xA0, 0x64);
    }
    g_BattleSpriteColumnWork[0x5D8D] = g_BattlePrimStartIndex[0];
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D7E]; i++) {
        BattleSetPrimGrey80BlendSub((u8 *)((i * 2 + g_BattleSpriteColumnWork[0x5D8D]) * 0x28 + (0x2530 + (s32)g_BattleSpriteColumnWork)));
    }
}
void BattleSetupLinearMove(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 dx;
    s32 dy;

    g_BattleLerpStartX = arg0;
    g_BattleLerpStartY = arg1;
    g_BattleLerpTargetX = arg2;
    g_BattleLerpTargetY = arg3;
    if (arg2 == arg0 || arg3 == arg1) {
        return;
    }
    if (arg2 < arg0) {
        dx = arg0 - arg2;
        g_BattleLerpDirX = 1;
    } else {
        dx = arg2 - arg0;
        g_BattleLerpDirX = 0;
    }
    if (arg3 < arg1) {
        dy = arg1 - arg3;
        g_BattleLerpDirY = 1;
    } else {
        dy = arg3 - arg1;
        g_BattleLerpDirY = 0;
    }
    if (dy <= dx) {
        g_BattleLerpStepX = 0x100;
        g_BattleLerpStepY = (dy << 8) / dx;
    } else {
        g_BattleLerpStepY = 0x100;
        g_BattleLerpStepX = (dx << 8) / dy;
    }
    g_BattleLerpFracX = 0;
    g_BattleLerpFracY = 0;
    g_BattleLerpStepsPerFrame = GameRandomByteRange(1, BATTLE_LERP_STEPS_PER_FRAME_MAX);
    g_BattleLerpDone = 0;
}
