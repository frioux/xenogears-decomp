#include "common.h"
#include "battle/actor.h"
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
// The last step index g_BattleCommandQueue's step counter walks through.
#define BATTLE_COMMAND_STEP_MAX 8
// The digit sprites BattleParseNumberToDigits fills g_BattleValueDigitSpriteIds
// with, and the id that marks a digit position left blank.
#define BATTLE_VALUE_DIGIT_COUNT 4
// The battle slots func_80085618 steps and how many of them are character slots
// (BattleReleaseCharSlot is called for the rest).
#define BATTLE_ACTIVE_SLOT_COUNT 11
#define BATTLE_CHAR_SLOT_COUNT 3

extern u8 *g_BattleCommandQueue;
extern u8 *g_BattleCommandSlotRows[];
extern u8 *g_BattleCommandStatusBytes[];
extern volatile u8 *g_BattleDisplayState;
extern u8 g_BattleSlotRecords[];
extern u8 g_BattleCharSlotUsage[];
extern u16 g_BattleStateFlags;
extern u8 g_BattleSlotEnabledFlags[];
extern u8 g_BattleSlotCurveRunning;
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
    /* 0x0E */ u8 unkE[0x16];
    /* 0x24 */ u16 param; // the queued command's argument word, read per the command byte at +0x31
    /* 0x26 */ u8 unk26[0xB];
    /* 0x31 */ u8 command;
    /* 0x32 */ u8 unk32[0x16];
} BattleCommandRecord; // Size: 0x48

// The word at +0 of one 0x170-byte command row (D_800CCD1C): bit 0x800 marks a row
// whose action is already queued.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk2[0x16E];
} BattleCommandRow; // Size: 0x170

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
    /* 0x004 */ u16 unk4;
    /* 0x006 */ u16 unk6;
    /* 0x008 */ u8 unk8[0x28];
    /* 0x030 */ BattleSlotEffectFlags activeEffects[5];
    /* 0x044 */ u8 unk44[0x4C];
    /* 0x090 */ u16 unk90;
    /* 0x092 */ u16 unk92;
    /* 0x094 */ u8 unk94[0x24];
    /* 0x0B8 */ s32 unkB8;
    /* 0x0BC */ s32 unkBC;
    /* 0x0C0 */ u8 unkC0[0x14];
    /* 0x0D4 */ u16 unkD4;
    /* 0x0D6 */ u8 unkD6[0x9A];
} BattleSlotStatEntry; // Size: 0x170

extern BattleCommandRecord g_BattleCommandRecords[];
extern BattleSlotStatEntry g_BattleSlotEntriesBase[];
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
extern s32 BattleSlotBitMask(u8);
extern void BattleDrawCommandRowSprites(void);
extern s32 func_800877E0(s32, s32);
extern s32 BattleFindMinValueOtherSlot(s32);
extern void BattleInitActorSlotAnimation(s32, s32, s32, s32);
extern u8 BattleSlotBitMaskInverted(u8);
extern s32 BattleFillSlotEffectAmounts(u8, s32 *);
extern s32 func_80086028(u8, s32, s32, u8, u32 **, u8);
extern void BattleClearSlotCurveEntry(void);
extern void BattleAttachActorAnimScripts(u8, s32, s32, s32);
extern u8 g_BattleActorCommandIds[];
extern u16 g_BattleCommandRows[];
extern u8 *g_BattleGraphicsWork;
extern u8 g_BattlePrimStartIndex[];
extern u8 *g_BattleSpriteColumnWork;
extern u8 g_BattleSlotSpriteColumnIds[];
extern void BattleSetPrimGrey80BlendAdd(u8 *);
extern void BattleSetPrimGrey80BlendSub(u8 *);
extern s32 GetStringEntryFromBank(s32, s32);
extern void func_80076D58(u8 *, u8, u8);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *, u16, u16, u8, u8, u8);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern s32 BattleDrawSpritePackage(s32, s32, s16, s16);
extern u8 g_BattleSlotTextIndex[];
extern u8 g_BattleValueDigitSpriteIds[];
extern u8 *g_BattleDigitImageBuffers[];
extern void *GetStringEntryFromBank1C(s32, s32);
extern void BattleParseNumberToDigits(s32);
extern void BattleLoadImageAndSync(RECT *, u_long *);
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
extern u8 g_BattleCommandCodes[];
extern u8 g_BattleCommandSlotIds[];
extern u8 g_BattleCommandStepTable[][3];
extern BattleCommandRow D_800CCD1C[];
extern BattleTableEntry g_BattleWorkBlock[];
extern void func_80087EDC(u8, u8);
extern void func_800881B8(u8, u8);
extern void BattleSetSlotRegistersFromActor(u8, u8);
extern s32 BattleScriptScanForOpcode62(u8);
extern u8 D_800C3A70[];
extern u8 g_BattleSlotConditions[];
extern void *BattleAllocTextBuffer(s32);
extern s32 func_80086C88(u8, s32, s32, u8, u_long **);
extern void HeapFree(void *);
extern s32 BattleUpdateFrame(void);

/* PARKNOTE 60.72%, 1172 bytes (retail 1196 = 299 insns, 6 short): every arm's shape,
   load widths, call sites and the two queue-store groupings below are retail's; the
   residual is the loop's INDUCTION VARIABLES. Retail keeps six (s0 = i*0x170 as a byte
   offset for the gp-relative per-slot symbols, s2 = i, s3 = g_BattleCommandRows +
   arg0*0x48 + i*2 as a u16 pointer with displacement 0, s4 = g_BattleCommandRows -
   0x138, s5 = g_BattleCommandRows - 0x130 + i*0x1C for the 0x1C-byte per-slot record,
   s6 = D_800CCDC4 + i*0x170 for the +0x90 word); this body gets four. Evidence that
   retail had ONE SYMBOL PER PER-SLOT FIELD, not one struct: the original object relocs
   D_800CCD36 (+2), D_800CCD38/3A (+4/+6), D_800CCDC4/C6 (+0x90/+0x92), D_800CCDEC
   (+0xB8), D_800CCDF0 (+0xBC), D_800CCE08 (+0xD4) with zero addend, plus
   g_BattleSlotEntries for the +0x30 word. Trying `extern u16 D_800CCDC4[]` +
   `D_800CCDC4[i * 0xB8]` is the untried lever: that strength-reduces to retail's s6
   pointer, while `g_BattleSlotEntriesBase[i].unk90` (this body) does not.
   Two other things retail shows that this body cannot spell from one struct:
   - the +0 and +4 words are read SIGNED (lh) in arms 0/5/7/8 and 1/9 but UNSIGNED
     (lhu) in cases 2, 3, 10 and 11, and the duration word likewise - the original
     must have had two typed aliases of each address; here that is the (s16 *) casts
     at the signed sites, and those casts are not the source's shape either.
   - the four stores at +0/+4/+0x90/+0xB8 go through s4 = g_BattleCommandRows - 0x138
     as `lui at,1 / addu at,s4,at / sh -0x71xx(at)` (base + 0x8E84..0x8F3C). That is
     gcc forming the address from an already-live nearby symbol; no plain
     `g_BattleSlotEntriesBase[i].field` reproduces it, so those 12 instructions differ
     however the fields are spelled.
   Verified while decoding, do not re-derive: cases 1/9 only decrement and do NOT store
   to the queue; case 3 only clamps and does NOT store to the queue (its two exits jump
   past .L80085A5C to the loop tail); cases 2/10/11 and both halves of 0/5/7/8 do;
   the 0x8000 test and the enabled test both continue without touching anything.
   Best body (60.72%); the arms' arm-order/queue-store split is right, so a rewrite only
   needs to change how the per-slot words are named and how the duration row is indexed. */
/* PARKNOTE 64.20%, 1140 bytes (retail 1196 = 299 insns; this body is 14 insns SHORT).
   Round 428 measured, from the 60.72% body below: hoisting the duration-row read into a
   walked pointer (`u16 *pRows = &g_BattleCommandRows[arg0 * 0x24]`, read pRows[i] /
   ((s16 *)pRows)[i]) took it to 63.97% / 1156 bytes, and hoisting the command id the same
   way (`u8 *pCmd = &g_BattleActorCommandIds[arg0 * 0x48]`, switch (pCmd[i])) to 64.20% /
   1140. That is retail own shape: s3 = rows + arg0*0x48 walked by 2 with `lh/lhu 0(s3)`,
   and fp = arg0*0x48 hoisted for the id read - without the pointer gcc re-forms the whole
   `(u8)arg0 * 0x48` per iteration (5 insns) and the row address per arm site (2 insns).
   The remaining 14 insns are three spelling problems, none of them tried:
   1. WIDTHS. This body still reads the two hp words as the struct u16 field and converts
      (lhu + sll 0x10 + sra 0x10, 4 extra insns at the arm 0 and arm 1 sites); retail has
      `lh` there and `lhu` in cases 2/3/10/11 - the same word. The fix is to declare
      BattleSlotStatEntry.hp and .unk4 as `s16` (arm 0/1 then load lh directly) and read
      them as `*(u16 *)&...hp` at the unsigned sites (a cast lvalue loads lhu with no
      conversion). Not done: it also feeds the matched BattleResetActorStatDisplay, so it
      needs a check_bin after the change.
   2. The unk90 word still gets a `la g_BattleSlotEntriesBase+144` base pointer in a saved
      register (retail reads it through s6 = D_800CCDC4 + i*0x170, a walked pointer, and
      STORES it through the s0 byte offset, i.e. two different bases for one address).
   3. retail derives the record byte and the three far stores from ONE base s4 = rows -
      0x138 (stepped by 0x170), with the record at 8(s4) stepped by 0x1C; this body uses a
      second `la g_BattleSlotRecords+4` pointer and %hi/%lo for the far stores.
   Everything else in the arms (queue-store split, case 1/9 and case 3 not storing, the
   two ori 0x8000 chains in retail order, both BattleReleaseCharSlot paths) is retail. */
/* PARKED 65.63%, 1140 bytes / 285 insns (retail 1196 / 299; 14 insns short).
//    Round 441 redo: un-parked and re-measured the 64.20% body, then took it to 65.63 in two
//    measured steps, both now in the body below.
//    1. 64.20 -> 65.51: the row base and the command-id read as byte arithmetic off the
//       symbol - `s32 ofs = arg0 * 0x48; pRows = (u16 *)((u8 *)g_BattleCommandRows + ofs);`
//       and `switch (*(u8 *)((u8 *)g_BattleActorCommandIds + ofs + i))`. That is retail's own
//       shape: fp/s8 holds arg0*0x48, s3 = ofs + rows, and the id read re-forms
//       lui/addiu of the symbol every iteration and then emits `addu v0,fp,v0; addu v0,v0,s2`
//       (tree sym+ofs then +i). A plain `arr[ofs + i]` emits the two addus the other way
//       round and through $at.
//    2. 65.51 -> 65.63: the two flag-word updates in the arm 0/5/7/8 sub-branches written as
//       `*(u16 *)((u8 *)g_BattleCommandRows + 0x900) |= BattleSlotBitMask(i)`. That IS retail:
//       the object has `lhu v1,0x900(s7); or; sh v1,0x900(s7)` there with NO relocation, while
//       the 0x8000 test above it reads the same word through a proper %hi/%lo reloc of
//       g_BattleSlotFlag8000Mask. One word, two spellings in one function.
//    NEW EVIDENCE, read off the OBJECT this round (objdump -dr expected/asm/battle/main/misc13.s.o,
//    not the .s): retail's four far stores carry NO relocation either, so no symbol spelling
//    can reproduce them - `lui at,0x1; addu at,s4,at; sh/sw -0x717C/-0x7178/-0x70EC/-0x70C4(at)`
//    are the hp/unk4/unk90/unkB8 stores at +0x8E84/+0x8E88/+0x8F14/+0x8F3C off s4, and
//    0x10000-0x70C4 is gcc's %hi/%lo split of a CONSTANT, i.e. the 1998 source added
//    compile-time offsets to a pointer it had derived from g_BattleCommandRows. s4 itself is
//    g_BattleCommandRows - 0x138 and s5 is - 0x130, i.e. -0x9C and -0x98 in u16 units: three
//    negative-index/offset spellings off that one anchor. 12 instructions are unreachable
//    with any symbol spelling and the frame/saved set follows from the rest.
//    Still unexplained, and the only path to 100: retail keeps the rows base in s7 across the
//    whole loop (frame 0x58, 10 saved registers) and forms every per-slot address from it,
//    where this body saves 8 (frame 0x48 today, 0x40 before step 2) and forms the flag word's
//    address once in the prologue (`addiu s6,v1,0x900`) instead of as a 0x900 displacement.
//    A spelling that keeps s7 live as the base (rather than a constant pointer) is what would
//    re-rank the allocator; nothing shippable found for it this round.
//    Also re-decoded, unchanged: the widths (hp/unk4 read `lh` in arms 0/5/7/8 and 1/9 and
//    `lhu` in 2/3/10/11 - the same words, so the original had two typed aliases), the
//    per-slot fields as separate %hi(sym)+s0 accesses with s0 = i*0x170 walked, and
//    `((u8 *)g_BattleSlotRecords + 4)[i * 0x1C]` scoring identical (1136 bytes, 65.51) to
//    `g_BattleSlotRecords[i * 0x1C + 4]`, which is kept because gcc already walks that
//    pointer by 0x1C. */
/*
void func_80085618(u8 arg0) {
    s32 i;
    u16 v;
    u32 u;
    s32 c;
    s16 a;
    s16 d;
    u16 *pRows;
    s32 ofs;

    ofs = arg0 * 0x48;
    pRows = (u16 *)((u8 *)g_BattleCommandRows + ofs);
    for (i = 0; i < BATTLE_ACTIVE_SLOT_COUNT; i++) {
        if (g_BattleSlotEnabledFlags[i] == 0) {
            continue;
        }
        if (g_BattleSlotEntriesBase[i].activeEffects[0].active & 0x8000) {
            g_BattleSlotFlag8000Mask |= BattleSlotBitMask(i);
            continue;
        }
        switch (*(u8 *)((u8 *)g_BattleActorCommandIds + ofs + i)) {
        case 0:
        case 5:
        case 7:
        case 8:
            a = *(s16 *)&g_BattleSlotEntriesBase[i].hp;
            d = ((s16 *)pRows)[i];
            if ((g_BattleSlotRecords[i * 0x1C + 4] == 0)) {
                if (a - d > 0) {
                    *(s16 *)&g_BattleSlotEntriesBase[i].hp = a - d;
                } else {
                    *(s16 *)&g_BattleSlotEntriesBase[i].hp = 0;
                    *(u16 *)((u8 *)g_BattleCommandRows + 0x900) |= BattleSlotBitMask(i);
                    g_BattleSlotEntriesBase[i].activeEffects[0].active |= 0x8000;
                    if (i >= BATTLE_CHAR_SLOT_COUNT) {
                        BattleReleaseCharSlot(i);
                    }
                }
            } else {
                c = g_BattleSlotEntriesBase[i].unkB8 - pRows[i];
                if (c > 0) {
                    g_BattleSlotEntriesBase[i].unkB8 = c;
                } else {
                    g_BattleSlotEntriesBase[i].unkB8 = 0;
                    *(u16 *)((u8 *)g_BattleCommandRows + 0x900) |= BattleSlotBitMask(i);
                    g_BattleSlotEntriesBase[i].unkD4 |= 0x8000;
                    g_BattleSlotEntriesBase[i].activeEffects[0].active |= 0x8000;
                    if (i >= BATTLE_CHAR_SLOT_COUNT) {
                        BattleReleaseCharSlot(i);
                    }
                }
            }
            g_BattleCommandQueue[i + 0x2EB] = 1;
            break;
        case 1:
        case 9:
            a = *(s16 *)&g_BattleSlotEntriesBase[i].unk4;
            d = ((s16 *)pRows)[i];
            if (a - d > 0) {
                *(s16 *)&g_BattleSlotEntriesBase[i].unk4 = a - d;
            } else {
                *(s16 *)&g_BattleSlotEntriesBase[i].unk4 = 0;
            }
            break;
        case 2:
            if ((g_BattleSlotRecords[i * 0x1C + 4] != 0) && (g_BattleSlotCurveRunning == 0)) {
                u = g_BattleSlotEntriesBase[i].unkB8 + pRows[i];
                g_BattleSlotEntriesBase[i].unkB8 = u;
                if ((u32)g_BattleSlotEntriesBase[i].unkBC < u) {
                    g_BattleSlotEntriesBase[i].unkB8 = g_BattleSlotEntriesBase[i].unkBC;
                }
            } else {
                v = g_BattleSlotEntriesBase[i].hp + pRows[i];
                g_BattleSlotEntriesBase[i].hp = v;
                if (g_BattleSlotEntriesBase[i].maxHp < v) {
                    g_BattleSlotEntriesBase[i].hp = g_BattleSlotEntriesBase[i].maxHp;
                }
            }
            g_BattleCommandQueue[i + 0x2EB] = 1;
            break;
        case 3:
            if ((g_BattleSlotRecords[i * 0x1C + 4] == 0) || (g_BattleSlotCurveRunning != 0)) {
                v = g_BattleSlotEntriesBase[i].unk4 + pRows[i];
                g_BattleSlotEntriesBase[i].unk4 = v;
                if (g_BattleSlotEntriesBase[i].unk6 < v) {
                    g_BattleSlotEntriesBase[i].unk4 = g_BattleSlotEntriesBase[i].unk6;
                }
            }
            break;
        case 10:
            if (g_BattleSlotEntriesBase[i].unk90 - pRows[i] > 0) {
                g_BattleSlotEntriesBase[i].unk90 = g_BattleSlotEntriesBase[i].unk90 - pRows[i];
            } else {
                g_BattleSlotEntriesBase[i].unk90 = 0;
            }
            g_BattleCommandQueue[i + 0x2EB] = 1;
            break;
        case 11:
            v = g_BattleSlotEntriesBase[i].unk90 + pRows[i];
            g_BattleSlotEntriesBase[i].unk90 = v;
            if (g_BattleSlotEntriesBase[i].unk92 < v) {
                g_BattleSlotEntriesBase[i].unk90 = g_BattleSlotEntriesBase[i].unk92;
            }
            g_BattleCommandQueue[i + 0x2EB] = 1;
            break;
        }
    }
}*/
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
    if (BattleFillSlotEffectAmounts((u8)arg0, buf) != 0) {
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
// The base's assignment has to sit inside the `0x9C8 +` sum: as a separate local
// gcc reassociates the constant onto the base register and the delay-slot addu
// comes out base-first (70.44%).
void BattleDrawCommandRowSprites(void) {
    s32 v;
    u8 *pWork;
    u8 *pWork2;
    u8 *pWork3;

    g_BattleDisplayState[0x7B] = 0;
    v = BattleDrawSpritePackage(g_BattleCommandQueue[0x2D4] + 0xF, (s32)((g_BattleDisplayState[0x7B] * 0x50) + (0x9C8 + (s32)(pWork = g_BattleGraphicsWork))), 0x2A, 0xD0);
    g_BattleDisplayState[0x7B] += v;
    v = BattleDrawSpritePackage(0x19, (s32)((g_BattleDisplayState[0x7B] * 0x50) + (0x9C8 + (s32)(pWork2 = g_BattleGraphicsWork))), 0x32, 0xD0);
    g_BattleDisplayState[0x7B] += v;
    v = BattleDrawSpritePackage(g_BattleCommandQueue[0x2D5] + 0xF, (s32)((g_BattleDisplayState[0x7B] * 0x50) + (0x9C8 + (s32)(pWork3 = g_BattleGraphicsWork))), 0x3A, 0xD0);
    g_BattleDisplayState[0x7B] += v;
    g_BattleDisplayState[0xA4] = g_BattlePrimStartIndex[0];
}
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
   in the preheader. The status-row index is `g_BattleWorkBlock[arg1].characterId`
   (retail relocs D_800CCD3E, the same address with the +0x56 addend in the load
   displacement instead; the linked bytes are identical, verified with check_bin).
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
        pEntry = g_BattleCommandStatusBytes[g_BattleWorkBlock[arg1].characterId];
        if ((u16)BattleMaskHasStatusBit(g_GameState.conditionRows[g_BattleActorCharIds[arg1]].commandMask,
                                        pEntry[0xC - j]) != 0) {
            result = 1;
        }
    }
done:
    return result;
}
/* 95.45%, 424 = retail's exact size (106 insns; read the ".size", the old
   "432" was a phantom 8-byte gap five rounds chased). Two residuals, size exact:
   1. the callee-saved pair is MIRRORED: retail holds arg1 in $s3 and r in $s4, this
      body the other way round (every use mirrors: srl/addu/subu/andi/addiu/return).
   2. "andi a1,s2,0xfffc" is scheduled one slot EARLY: retail groups the three
      argument masks after the three chains, so its "sll v0,s3,4" comes first.
   Dead this round: q*0x78 into an s32 local then &0xF8 (94.89); a char holding
   (q*0x78)&0xF8 hoisted above "arg2 = arg2 + rows" (unchanged); an "n = arg1" live
   copy, "arg1 = arg1;" and "r = arg1 - q*2" (all unchanged - gcc coalesces them).
   Permuter 6 min from this body: base 355 -> 260, but both finds were those q*0x78
   splits, so its score does not transfer to objdiff.
   Presets: gcc-2.6.0 99.23, gcc-2.7.2-cdk 89.0, -O3 compile-failed. The unit cannot
   switch to 2.6.0 (BattleBeginActorStatAnimation 93.18, BattleReleaseCharSlot 92.28,
   four more at 99.1-99.8), so this body would need a unit split to benefit.
   The register pair reads like a local_alloc tie: one basic block, the two values are
   the same in both compiles, only their order in the callee-saved pool differs. */
/*
s32 func_80086028(u8 arg0, s32 arg1, s32 arg2, u8 arg3, u32 **arg4, u8 arg5) {
    s32 y;
    s32 q;
    s32 r;
    s32 len;
    s32 rows;
    u8 *pColumnWork;
    RECT rect;

    q = arg1 / 2;
    r = arg1 % 2;
    func_80076D58((u8 *)(s32)(arg1 * 0x50 + (0x5550 + (s32)g_BattleSpriteColumnWork)), r, 3);
    len = SystemRenderStringEntry((u8 *)GetStringEntryFromBank(g_BattleActorCharIds[arg0], arg3), *arg4, 0x1B, r);
    rect.x = q * 0x1E + 0x3C0;
    rect.y = 0x1A;
    rect.w = 0x1E;
    rect.h = 0xD;
    LoadImage(&rect, *arg4);
    rows = arg5 + 1;
    arg2 = arg2 + rows;
    y = arg2 * 0x10 + 0x50;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(s32)((arg1 * 2 + ((s32 *)g_BattlePrimStartIndex)[0]) * 0x28 + (0x5550 + (s32)(pColumnWork = g_BattleSpriteColumnWork))),
                  (y + arg1 * 4) & 0xFFFC,
                  (0xC8 - arg1 * 0x10) & 0xFFF8,
                  (q * 0x78) & 0xF8,
                  0x1A,
                  len);
    g_BattleSpriteColumnWork[0x5D80]++;
    return arg1 + 1;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086028);

// PARKED 98.56%, 2496 bytes / 624 insns (retail 2488 / 622). The whole 20-byte
//    arg1 SPILL is gone: retail's frame 88, saved set s0-s7+fp and 'fp = arg1'
//    are now exact, and every 'andi X,fp,0xff' reload is retail's.
//    WHAT CLOSED IT (new, round 460): in the case-8 and case-3 arms the two draw
//    calls' x arguments shared 'i * 0x10', so gcc CSEd that multiply across the
//    first BattleDrawSpritePackage call; the CSEd pseudo took one of the ten
//    callee-saved registers and the raw arg1 lost its home. Retail re-forms the
//    multiply from i after each call ('sll a2,s2,0x4' in the jal delay slot).
//    Writing the second icon's x as '(i + 1) * 0x10 + 0x54' (same value as
//    'i * 0x10 + 0x64', a different tree) breaks the CSE: 95.22 -> 98.56, and it
//    is also the more readable spelling ("the column after the first icon").
//    Note the naive attempts do NOT work because gcc canonicalises RTL:
//    '(i << 4) + 0x64' is the same insn as 'i * 0x10 + 0x64' (measured, 95.22,
//    unchanged).
//    REMAINING 8 bytes, all measured, none source-shaped:
//    1. Two instructions of delay-slot/reorg placement: retail leaves
//       'move s0,zero' (o = 0 for the next free loop) in the 'beqz s7' delay slot
//       and puts the 'g_BattleDisplayState[0xA8] = 1' store in the 'jal
//       BattleUpdateFrame' delay slot; this build takes the beqz slot for
//       'sltiu s3,9' and the jal slot for 'move s0,zero', which also leaves one
//       real 'nop' (lbu-use hazard) before 'sb ...,0x5D8F'.
//    2. The register rotation is now one slot: the arg1*0x170 temporary lands in
//       $s6 here and $s7 in retail (and retail's $s7 'drawn' is dead by then).
//    3. 'lbu 86(at)' vs retail's 'lbu 0(at)' is the typed g_BattleWorkBlock[arg1]
//       access (same linked bytes, objdiff cannot fold it).
//    Tried and measured this round, all worse or equal: '(i << 4) + 0x64' /
//    '(i << 4) + 0x68' (95.22, identical object - RTL canonicalisation). Earlier
//    rounds, do not redo: removing the BattleDrawSpritePackage return temp
//    (94.97), dropping the pQueue local (95.22/2504), permuter 4 min (best edit
//    clobbered the ladder counter - wrong code), try_flags 2.6.0 95.22 and
//    cdk 78.03, and declaration reordering (no instruction changes, only names).
//    Next lever if anyone picks this up: the two delay-slot placements, i.e. what
//    makes reorg prefer the loop init over the pending store. Both switch tables
//    (jtbl_800702B8, jtbl_800702F0) still diff as .rodata with byte counts that
//    match retail's layout (13 words + 4 pad, 9 words), so the switch C is right.
/*
void func_800861D0(u8 arg0, u8 arg1) {
    s32 i;
    s32 j;
    s32 matched;
    s32 o;
    s32 row;
    s32 drawn;
    u8 spriteId;
    u8 *pQueue;
    u8 *pRow;
    u8 *pKey;

    row = 0;
    drawn = 0;
    for (o = 0; o < 12; o += 4) {
        *(void **)(D_800C3A70 + o) = BattleAllocTextBuffer(0x1E);
    }
    g_BattleCommandQueue[0x2CC + g_BattleCommandQueue[0x2D6]] = arg0 + 0xFC;
    g_BattleCommandQueue[0x2D6] = g_BattleCommandQueue[0x2D6] + 1;
    if (g_BattleCommandQueue[0x2E4] != 0) {
        g_BattleCommandQueue[0x2CB + g_BattleCommandQueue[0x2D6]] = BATTLE_COMMAND_NONE;
    }
    pQueue = g_BattleCommandQueue;
    for (i = 0; i < BATTLE_COMMAND_SLOT_COUNT; i++) {
        pRow = g_BattleCommandSlotRows[i];
        for (j = 0; j < BATTLE_COMMAND_KEY_LENGTH; j++) {
            pKey = g_BattleCommandQueue + j;
            if (pKey[0x2CC] != pRow[j]) {
                matched = 0;
                break;
            }
            matched = 1;
        }
        if (matched) {
            break;
        }
    }
    matched = i;
    if (g_BattleCommandQueue[0x2E4] != 0) {
        g_BattleCommandQueue[0x2CB + g_BattleCommandQueue[0x2D6]] = arg0 + 0xFC;
    }
    g_BattleSpriteColumnWork[0x5D81] = 0;
    g_BattleSpriteColumnWork[0x5D80] = 0;
    for (i = 0; i < g_BattleCommandQueue[0x2D6]; i++) {
        switch (g_BattleCommandQueue[0x2CC + i]) {
        case 0:
            spriteId = 0x5D;
            break;
        case 2:
            spriteId = 0x5E;
            break;
        case 3:
            spriteId = 0x5F;
            break;
        }
        g_BattleSpriteColumnWork[0x5D81] += BattleDrawSpritePackage(spriteId,
                                    (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                          (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                    i * 0x10 + 0x50, 0xD0 - row * 0x10);
    }
    j = 0;
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
        if ((u16)BattleMaskHasStatusBit(g_GameState.conditionRows[g_BattleActorCharIds[arg1]].commandMask,
                                        g_BattleCommandStatusBytes[g_BattleWorkBlock[arg1].characterId][0xC - j]) != 0) {
            if (*(u16 *)(g_BattleCommandQueue + arg1 * 0x40 + 0x20) == 0) {
                if (g_BattleCommandQueue[0x2E4] == 0) {
                    g_BattleSpriteColumnWork[0x5D81] += BattleDrawSpritePackage(7,
                                                (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                                      (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                                i * 0x10 + 0x50, 0xD0 - row * 0x10);
                } else {
                    i -= 1;
                }
                drawn = 1;
                row = func_80086028(arg1, row, i,
                                    g_BattleCommandStatusBytes[g_BattleWorkBlock[arg1].characterId][0xC - j],
                                    (u_long **)(D_800C3A70 + (row / 2) * 4), 0);
            }
        }
        break;
    }
    if (g_BattleCommandQueue[0x2E4] != 0 && drawn != 0) {
        g_BattleSpriteColumnWork[0x5D90] = g_BattlePrimStartIndex[0];
        g_BattleCommandQueue[0x2DC] = g_BattleCommandStatusBytes[g_BattleWorkBlock[arg1].characterId][0xC - j] + 8;
        g_BattleSpriteColumnWork[0x5D8F] = g_BattlePrimStartIndex[0];
        g_BattleDisplayState[0xA8] = 1;
        BattleUpdateFrame();
        for (o = 0; o < 12; o += 4) {
            HeapFree(*(void **)(D_800C3A70 + o));
        }
        return;
    }
    j = 0;
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
    case 8:
        if ((u16)BattleMaskHasStatusBit(g_GameState.conditionRows[g_BattleActorCharIds[arg1]].commandMask,
                                        g_BattleCommandStatusBytes[g_BattleWorkBlock[arg1].characterId][0x13 - j]) != 0) {
            if (*(u16 *)(g_BattleCommandQueue + arg1 * 0x40 + 0x1C) == 0) {
                if (*(u16 *)(g_BattleCommandQueue + arg1 * 0x40 + 0x20) == 0) {
                    g_BattleSpriteColumnWork[0x5D81] += BattleDrawSpritePackage(8,
                                                (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                                      (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                                i * 0x10 + 0x54, 0xD0 - row * 0x10);
                    g_BattleSpriteColumnWork[0x5D81] += BattleDrawSpritePackage(7,
                                                (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                                      (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                                (i + 1) * 0x10 + 0x54, 0xD0 - row * 0x10);
                    row = func_80086028(arg1, row, i,
                                        g_BattleCommandStatusBytes[g_BattleWorkBlock[arg1].characterId][0x13 - j],
                                        (u_long **)(D_800C3A70 + (row / 2) * 4), 1);
                }
            }
        }
        break;
    }
    j = 0;
    switch (matched) {
    case 0:
        j++;
    case 1:
        j++;
    case 3:
        if ((u16)BattleMaskHasStatusBit(g_GameState.conditionRows[g_BattleActorCharIds[arg1]].commandMask,
                                        g_BattleCommandStatusBytes[g_BattleWorkBlock[arg1].characterId][0x16 - j]) != 0) {
            if (*(u16 *)(g_BattleCommandQueue + arg1 * 0x40 + 0x1E) == 0) {
                if (*(u16 *)(g_BattleCommandQueue + arg1 * 0x40 + 0x20) == 0) {
                    g_BattleSpriteColumnWork[0x5D81] += BattleDrawSpritePackage(9,
                                                (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                                      (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                                i * 0x10 + 0x58, 0xD0 - row * 0x10);
                    g_BattleSpriteColumnWork[0x5D81] += BattleDrawSpritePackage(7,
                                                (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                                      (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                                (i + 1) * 0x10 + 0x58, 0xD0 - row * 0x10);
                    func_80086028(arg1, row, i,
                                  g_BattleCommandStatusBytes[g_BattleWorkBlock[arg1].characterId][0x16 - j],
                                  (u_long **)(D_800C3A70 + (row / 2) * 4), 1);
                }
            }
        }
        break;
    }
    g_BattleSpriteColumnWork[0x5D90] = g_BattlePrimStartIndex[0];
    g_BattleSpriteColumnWork[0x5D8F] = g_BattlePrimStartIndex[0];
    g_BattleDisplayState[0xA8] = 1;
    BattleUpdateFrame();
    for (o = 0; o < 12; o += 4) {
        HeapFree(*(void **)(D_800C3A70 + o));
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800861D0);


u8 BattleCheckActorCommandAvailable(s32 arg0, u8 arg1) {
    s32 ok = 1;
    s32 idx;

    idx = arg0 + (g_BattleCommandQueue[0x2CC] + 1) * 3;
    if (g_BattleCommandQueue[0x2D6] != 0) {
        if (g_BattleCommandCharIndex[0] == 4) {
            idx = arg0 + 0xC;
        }
        if ((u16)BattleMaskHasStatusBit(g_GameState.conditionRows[g_BattleActorCharIds[arg1]].gearCommandMask,
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
/* 87.15%, 784 bytes (= retail). Structure and instruction multiset are retail's:
   same call set and order, 0x78 frame, the same ten saved registers, `count`
   spilled to 0x48 and the incoming stack parameter arg4 kept in s6.
   Two measured levers, both worth several points and both real source: the first
   two parameters are `u8` (declaring arg0 s32 is 80.99%, arg3 s32 84.69% - retail
   masks each once, in place, and reuses the masked value, which only happens when
   the parameter itself is narrow).
   The header rect must be its own `RECT rect;` beside `RECT rects[4]`: with a single
   `RECT rects[5]` and rects[count + 1] indexing gcc CSEs the array base with the
   LoadImage argument, hoists it above the first call, spills it and rematerialises
   arg4 (796 bytes, 81.88%); splitting the declaration frees the register that keeps
   arg4 in s6. `arg3` needs the (u8) spelling at its second use
   (g_BattlePrimStartIndex + arg3 * 2) or gcc masks twice and s4/s5 swap.
   Residual, all measured:
   1. Two register pairs are swapped: retail has arg1 in s7 and the
      g_BattleValueDigitSpriteIds base in fp, this build the other way round.
   2. The digit loop. Retail reloads `count` from 0x48 and re-forms both bases every
      iteration (`addiu a0,sp,0x20`, `addiu v0,sp,0x18`, `sll`, two `addu`, stores at
      8/0xA/0xC/0xE off the rect base); this build keeps a bumped pointer in s0 and
      stores at 0/2/4/6. That pointer costs the register retail keeps
      `g_BattleValueDigitSpriteIds + 4` in, so the loop bound here is re-formed with
      lui/addiu each iteration, and x lands in s1 instead of s0.
   3. `g_BattleDigitImageBuffers[id]` comes out la + addu + lw where retail has the
      lui $at / addu $at / lw %lo form (the array-extern and cast spellings were
      measured dead in round 311; misc16_r2's func_8008FE18 reports the same form).
   Permuter from this body: 1548 iterations base=3486 best=2661 (found u8 arg0),
   1251 iterations base=2466 best=1991 (found u8 arg3).
   ROUND 313, the residual is now ONE register choice, and it is a swap of which
   loop invariant gcc hoists: retail keeps the loop BOUND in $s1 (`addiu s1,fp,4`
   in the BattleParseNumberToDigits delay slot, `slt v0,fp,s1`) and re-forms the
   rect base inside the loop (`addiu v0,sp,0x18` + `addu`), while this build keeps
   a bumped rect pointer in $s0 (`addiu s0,s0,8`, stores at 0/2/4/6, the callee's
   argument passed from it) and rematerialises the bound with lui/addiu every
   iteration. Same three loop registers (ids base, x, one more), chosen differently.
   Measured this round, all worse than the 784-byte body above: one `RECT rects[5]`
   with `rects[0]` as the header and `rects[count + 1]` for the digits (796, 81.88 -
   gcc keeps `&rects[0]` live across the four calls and spills it into the 0x48 slot,
   which then promotes count into $s6 and makes the index `(count + 1) * 8`); the same
   array with the four stores written as `*(s16 *)((u8 *)rects + count * 8 + k)` (800,
   80.14); a `RECT *pRect = &rects[count]` local for the stores with `&rects[count]` at
   the call (788, 84.40 - gcc still strength-reduces, and pRect costs the register s1);
   `volatile s32 count` (852, 70.82 - the reloads come back but so does a 68-byte body).
   cc1_probe on a self-contained replica of the loop shows the same hoisting on a
   nearly empty function, so this is loop.c's move_movables preferring the 2-use base
   over the 1-use bound, not a register limit: no source spelling found that makes it
   hoist the bound instead.
   ROUND 328: the real residual is that loop.c creates a strength-reduced BIV for
   `rects[count]` (5 uses in the taken path) and that BIV's base takes the register
   retail spends on `g_BattleValueDigitSpriteIds + 4`; retail has no BIV at all, it
   re-forms `&rects[0]` and `&rect` from $sp every iteration (one `addiu` each) and
   keeps only the ids pointer, x and the bound. Measured this round, all at retail's
   784 bytes and all still IV-ing: a source pointer walk (add `u8 *pDigit` /
   `u8 *pDigitEnd` locals and `for (pDigit = ids; pDigit < pDigitEnd; pDigit++)`)
   makes gcc duplicate the exit test (`sltu`+`beqz` guard on top of the rotated loop)
   and keep the element pointer, 79.19%; `volatile RECT rects[4]` keeps the IV too and
   loses retail's store order, 85.47%; writing the four stores in retail's own order
   (w, x, y, h instead of x, y, w, h) changes nothing, 87.15 - gcc reorders them to
   x, 6, 0, 0xD in both spellings where retail emits 6, x, 0, 0xD. Presets screened:
   gcc-2.7.2-cdk 79.01, -G0 87.15, gcc-2.6.0 91.87 (PRESET_260 reported; the unit
   cannot switch - 2.6.0 drops BattleResetActorStatDisplay to 99.19,
   BattleCheckCommandKeyStatusBit to 99.78 and BattleInitSlotRowsForActor to 99.44).
   ROUND 400: un-parked and re-measured unchanged (87.15%, 784 bytes, 196 insns =
   retail's). m2c_draft decodes the digit loop to the same shape this body has (an
   ids pointer walked to g_BattleValueDigitSpriteIds + 4, the four stores sharing one
   count*8 off the rect base), so there is no structural port left: the residual is
   the BIV/mirror, not a body.
   ROUND 426: un-parked and re-measured unchanged, 87.15% / 784 bytes / 196 insns
   (retail 784 / 196), so byte and instruction counts are already exact. Read whole,
   the diff is ONE shift: retail has s7 = arg1 and s8 = the g_BattleValueDigitSpriteIds
   cursor with its bound (ids + 4) in s1; this body has s8 = arg1 and s7 = the cursor
   with the bound rematerialised (lui/addiu) every iteration and s1 holding the loop's
   exit test. Everything else that differs follows from that: this body keeps the
   strength-reduced rects[count] base in s0 and bumps it by 8, retail has no BIV and
   re-forms sp+24+count*8 each iteration, so the saved-register order is a symptom of
   the BIV rather than a second bug. Nothing new was tried this round. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086C88);
/*
s32 func_80086C88(u8 arg0, s32 arg1, s32 arg2, u8 arg3, u_long **arg4) {
    RECT rect;
    RECT rects[BATTLE_VALUE_DIGIT_COUNT];
    s32 count;
    s32 q;
    s32 r;
    s32 quad;
    s32 ofs;
    s32 x;
    s32 len;
    s32 primStart;
    s32 i;

    count = 0;
    q = arg1 / 2;
    r = arg1 % 2;
    quad = arg1 * 4;
    ofs = arg1 * 0x50;
    func_80076D58((u8 *)(s32)(ofs + (0x5550 + (s32)g_BattleSpriteColumnWork)), r, 3);
    len = SystemRenderStringEntry(
        (u8 *)GetStringEntryFromBank1C(g_BattleSlotTextIndex[(u8)arg0 * 0x170],
                                       g_BattleCommandStatusMasks[(u8)arg3]),
        *arg4, 0x1B, r);
    rect.x = q * 0x1E + 0x3C0;
    rect.y = 0x1A;
    rect.w = 0x1E;
    rect.h = 0xD;
    LoadImage(&rect, *arg4);
    primStart = ((s32 *)g_BattlePrimStartIndex)[0];
    BattleSetPolyFT4RectH13((POLY_FT4 *)(s32)((arg1 * 2 + primStart) * 0x28 +
                                              (0x5550 + (s32)g_BattleSpriteColumnWork)),
                            (quad + (arg2 + 1) * 0x10 + 0x86) & 0xFFFE,
                            (0xC8 - arg1 * 0x10) & 0xFFF8, (q * 0x78) & 0xF8, 0x1A, len & 0xFF);
    func_80076D58((u8 *)(s32)(ofs + (0x5C80 + (s32)g_BattleSpriteColumnWork)), 0, 3);
    BattleParseNumberToDigits(*(u16 *)((u8 *)g_BattlePrimStartIndex + (u8)arg3 * 2 + 0x60D8));
    x = arg1 * 8 + 0x3DE;
    for (i = 0; i < BATTLE_VALUE_DIGIT_COUNT; i++) {
        if (g_BattleValueDigitSpriteIds[i] != BATTLE_VALUE_DIGIT_NONE) {
            rects[count].x = x;
            rects[count].y = 0;
            rects[count].w = 6;
            rects[count].h = 0xD;
            x += 2;
            BattleLoadImageAndSync(&rects[count], g_BattleDigitImageBuffers[g_BattleValueDigitSpriteIds[i]]);
            count++;
        }
    }
    primStart = ((s32 *)g_BattlePrimStartIndex)[0];
    BattleSetPolyFT4RectH13((POLY_FT4 *)(s32)((arg1 * 2 + primStart) * 0x28 +
                                              (0x5C80 + (s32)g_BattleSpriteColumnWork)),
                            (arg1 * 4 + (arg2 + 1) * 0x10 + 0xEA) & 0xFFFE,
                            (0xC8 - arg1 * 0x10) & 0xFFF8, (arg1 * 0x20 + 0x78) & 0xF8, 0,
                            (count * 8) & 0xFF);
    g_BattleSpriteColumnWork[0x5D80]++;
    g_BattleSpriteColumnWork[0x5D82]++;
    return arg1 + 1;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086F98);
/* PARKED 93.71%, 2128 bytes / 532 insns (retail 2120 / 530). Round 445 redo.
   The switch's `continue` arm was the residual everyone was looking past: with
   `case BATTLE_COMMAND_NONE: continue;` INSIDE the switch, gcc tests 0xFF LAST
   and adds two instructions (91.49% / 152 lines); writing that arm as an
   `if (...) continue;` BEFORE the switch reproduces retail's whole compare chain
   (0xFF first with `ori v0,1` in its delay slot, then 1, `slti 2`, then 0 and 2)
   and scores 93.69 at the same 2128 bytes. Only the two instructions are still
   over: see residual 1.
   Load-bearing, each measured from this body:
   - the double-negated offset at the three queue-index sites,
     `(g_BattleCommandQueue - -(g_BattleCommandQueue[0x2D6]))[0x2CC]` and `[0x2CB]`
     (91.38 -> 91.49): retail's base-first `addu` and its v0/v1 pair.
   - `idx = arg0 + 0xC; if ((arg0 & 0xFF) == BATTLE_COMMAND_NONE) idx = 0xC;`
     inside the `charIndex == 4` arm (default, then override): retail's shape,
   - the s32 byte-offset local `o` for the two D_800C3A70 loops (it is what gives
     retail's `lui at,%hi / addu at,at,o / op %lo(at)`), `u8 maskIdx` (one
     `andi 0xFF` per use) and a separate `u8 spriteId` for the sprite loop -
     before that split the loop index and the mask index were mirrored s1/s2.
     93.71.
   Measured WORSE this round, do not redo:
   - the loop read as `(g_BattleCommandQueue - -i)[0x2CC]`: 92.58, 2136 bytes - a
     negated loop index becomes its own accumulator.
   - maskIdx blocks 2/3 as "default then override"
     (`maskIdx = (arg0 + 1) * 3 + 1; if (charIndex == 4) maskIdx = 0xD;`): retail's
     exact 2120 bytes / 530 insns but 90.29 - gcc hoists the default chain into the
     delay slots and keeps 0xD in a register instead of retail's `j` convergence.
   - the `else` arm's 5th argument to func_80086C88 as `(row / 2) * 4`: 93.46 /
     2132. (The old note here claimed retail had `((row + queue[0x2E4]) >> 1) * 4`
     on that path; retail's .s is `srl v0,s3,0x1f / addu / sra 1 / sll 2` = row/2,
     so the old spelling was not just slow but wrong.)
   - `i = 0;` hoisted above the `queue[0x2E4]` test (retail's init sits in that
     test's beqz delay slot): 93.01 / 178 lines.
   - switch spellings: `default: continue;` + cases 0/1/2 89.83 (2092 bytes),
     `default:` on the 0x5D arm 89.46 (2092), source order NONE/1/0/2 91.48.
   Residual:
   1. The second early return (`queue[0x2E7] == 0` -> store arg0, return 1) is
      emitted at the FRONT here and at the very END in retail (.L800877A0, after
      the free loop, falling into the epilogue), which is the 2-instruction
      surplus and the +8 shift of every branch target in the diff. Retail's front
      is a plain `beqz` to that far block; every source shape tried so far
      (nested if, if-restructure) keeps it inline.
   2. Scheduler/allocator, not source: `li s5,0x1` (result = 1) placement in the
      three icon blocks, the y argument's `sll 0x10` vs `sra 0x10` order, and
      `sb v0,168(v1)` vs the BattleUpdateFrame delay slot.
   3. `addiu s6,s4,12` (raw arg0) comes out `addiu s6,a0,12` off the masked
      pseudo the neighbouring `(arg0 & 0xFF)` compare creates.
   Intended name: BattleDrawActorCommandColumn (allocates the three text buffers
   at D_800C3A70, appends the actor's slot to the queue, draws a sprite per queued
   command, then draws the three status icons its condition words allow and frees
   the buffers; returns 1 when nothing was drawn).
/*
s32 func_80086F98(u8 arg0, u8 arg1) {
    s32 i;
    u8 maskIdx;
    u8 spriteId;
    s32 o;
    s32 row;
    s32 result;
    s32 idx;
    s32 off;
    s32 v;

    idx = (arg0 + 1) * 3;
    row = 0;
    result = 0;
    off = (g_BattleCommandQueue - -(arg1 * 0x40))[0x3C] * 0x170;
    if ((*(u8 *)(off + (u32)g_BattleSlotConditions) & 0x80) == 0 && g_BattleCommandCharIndex[0] != 4) {
        g_BattleCommandQueue[0x2DC] = g_BattleCommandStatusMasks[arg0];
        return 1;
    }
    if (g_BattleCommandCharIndex[0] == 0 || g_BattleCommandCharIndex[0] - 1 < arg0) {
        if (g_BattleCommandQueue[0x2E7] == 0) {
            g_BattleCommandQueue[0x2DC] = arg0;
            return 1;
        }
    }
    for (o = 0; o < 12; o += 4) {
        *(void **)(D_800C3A70 + o) = BattleAllocTextBuffer(0x1E);
    }
    (g_BattleCommandQueue - -(g_BattleCommandQueue[0x2D6]))[0x2CC] = arg0;
    g_BattleCommandQueue[0x2D6] = g_BattleCommandQueue[0x2D6] + 1;
    if (g_BattleCommandQueue[0x2E4] != 0) {
        (g_BattleCommandQueue - -(g_BattleCommandQueue[0x2D6]))[0x2CB] = BATTLE_COMMAND_NONE;
        (g_BattleCommandQueue - -(g_BattleCommandQueue[0x2D6]))[0x2CB] = arg0;
        idx = arg0 + (g_BattleCommandQueue[0x2CC] + 1) * 3;
    }
    g_BattleSpriteColumnWork[0x5D81] = 0;
    g_BattleSpriteColumnWork[0x5D80] = 0;
    g_BattleSpriteColumnWork[0x5D82] = 0;
    for (i = 0; i < g_BattleCommandQueue[0x2D6]; i++) {
        if (g_BattleCommandQueue[0x2CC + i] == BATTLE_COMMAND_NONE) {
            continue;
        }
        switch (g_BattleCommandQueue[0x2CC + i]) {
        case 0:
            spriteId = 0x5E;
            break;
        case 1:
            spriteId = 0x5F;
            break;
        case 2:
            spriteId = 0x5D;
            break;
            break;
        }
        v = BattleDrawSpritePackage(spriteId,
                                    (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                         (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                    i * 0x10 + 0x80, 0xD0 - row * 0x10);
        g_BattleSpriteColumnWork[0x5D81] += v;
    }
    if (g_BattleCommandCharIndex[0] == 4) {
        idx = arg0 + 0xC;
        if ((arg0 & 0xFF) == BATTLE_COMMAND_NONE) {
            idx = 0xC;
        }
    }
    maskIdx = idx;
    if ((u16)BattleMaskHasStatusBit(g_GameState.conditionRows[g_BattleActorCharIds[arg1]].gearCommandMask,
                                    g_BattleCommandStatusMasks[maskIdx]) != 0) {
        if (g_BattleCommandQueue[0x2E4] == 0) {
            result = 1;
            if (*(u16 *)(g_BattleCommandQueue - -(arg1 * 0x40) + 0x1C) == 0) {
                v = BattleDrawSpritePackage(8,
                                            (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                                 (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                            i * 0x10 + 0x80, 0xD0 - row * 0x10);
                g_BattleSpriteColumnWork[0x5D81] += v;
                row = func_80086C88(arg1, row, i - 1, maskIdx,
                                    (u_long **)(D_800C3A70 + (row / 2) * 4));
            }
        } else {
            result = 1;
            row = func_80086C88(arg1, row, i - 1, maskIdx,
                                (u_long **)(D_800C3A70 + ((row + g_BattleCommandQueue[0x2E4]) >> 1) * 4));
        }
    }
    if (g_BattleCommandQueue[0x2E4] != 0 && result != 0) {
        result = 0;
        if (g_BattleCommandQueue[0x2CC] == BATTLE_COMMAND_NONE) {
            idx = arg0 + 0xC;
        }
        g_BattleSpriteColumnWork[0x5D90] = g_BattlePrimStartIndex[0];
        g_BattleCommandQueue[0x2DC] = g_BattleCommandStatusMasks[idx & 0xFF];
        g_BattleSpriteColumnWork[0x5D8F] = g_BattlePrimStartIndex[0];
        g_BattleSpriteColumnWork[0x5D91] = g_BattlePrimStartIndex[0];
        g_BattleDisplayState[0xA8] = 1;
        BattleUpdateFrame();
        for (o = 0; o < 12; o += 4) {
            HeapFree(*(void **)(D_800C3A70 + o));
        }
        return result;
    }
    g_BattleCommandQueue[0x2DC] = arg0;
    if (g_BattleCommandCharIndex[0] == 4) {
        maskIdx = 0xD;
    } else {
        maskIdx = (arg0 + 1) * 3 + 1;
    }
    if ((u16)BattleMaskHasStatusBit(g_GameState.conditionRows[g_BattleActorCharIds[arg1]].gearCommandMask,
                                    g_BattleCommandStatusMasks[maskIdx]) != 0 &&
        *(u16 *)(g_BattleCommandQueue - -(arg1 * 0x40) + 0x1E) == 0) {
        result = 1;
        v = BattleDrawSpritePackage(9,
                                    (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                         (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                    i * 0x10 + 0x84, 0xD0 - row * 0x10);
        g_BattleSpriteColumnWork[0x5D81] += v;
        row = func_80086C88(arg1, row, i, maskIdx, (u_long **)(D_800C3A70 + (row / 2) * 4));
    }
    if (g_BattleCommandCharIndex[0] == 4) {
        maskIdx = 0xE;
    } else {
        maskIdx = (arg0 + 1) * 3 + 2;
    }
    if ((u16)BattleMaskHasStatusBit(g_GameState.conditionRows[g_BattleActorCharIds[arg1]].gearCommandMask,
                                    g_BattleCommandStatusMasks[maskIdx]) != 0 &&
        *(u16 *)(g_BattleCommandQueue - -(arg1 * 0x40) + 0x20) == 0) {
        result = 1;
        v = BattleDrawSpritePackage(7,
                                    (s32)((g_BattleSpriteColumnWork[0x5D81] * 0x50) +
                                         (0x5640 + (s32)g_BattleSpriteColumnWork)),
                                    i * 0x10 + 0x88, 0xD0 - row * 0x10);
        g_BattleSpriteColumnWork[0x5D81] += v;
        func_80086C88(arg1, row, i, maskIdx, (u_long **)(D_800C3A70 + (row / 2) * 4));
    }
    g_BattleSpriteColumnWork[0x5D90] = g_BattlePrimStartIndex[0];
    g_BattleSpriteColumnWork[0x5D8F] = g_BattlePrimStartIndex[0];
    g_BattleSpriteColumnWork[0x5D91] = g_BattlePrimStartIndex[0];
    g_BattleDisplayState[0xA8] = 1;
    BattleUpdateFrame();
    for (o = 0; o < 12; o += 4) {
        HeapFree(*(void **)(D_800C3A70 + o));
    }
    return result;
}
*/

/* PARKED 62.91%, 448 bytes / 112 insns (retail 456 = 114). Structure, both loops,
   their bounds, the store order, the addu operand orders and the $at-form stores
   are retail's. Round 452 find, kept: each loop body assigns `o = i * 6` instead
   of accumulating `o += 6`. With a NAMED s32 offset the store keeps retail's
   `lui at,%hi / addu at,at,o / sh %lo(at)` form AND the offset lands in a2 with
   the counter in a3, in BOTH loops; the accumulator form gives the reverse
   (i in a2) and 60.72. 448 bytes means the only missing pair is the loop-2 setup:
   this body loads g_BattleSlotParamTable into a2, so the offset o falls to t0 and
   ofsA/ofsB to t1/t2 and res to t5, where retail has pTable a1, o a2, ofsA t0,
   ofsB t1, res t3. Everything else in the diff is downstream of that one register,
   plus: retail keeps the masked arg0 in t2 across the loop (this body remasks it
   in the exit branch's delay slot, retail has nop there) and retail materialises
   the first element address whole (addiu 0x140 then addu i, then two lbu 0(a0))
   where this body folds 0x140 into the load displacement.
   ROUND 452 measurements, all new:
   - loop2 spelled `o = 6` before the loop plus `o += 6` in the body: 62.91, the
     SAME object as the `o = i * 6` form, so loop2's spelling is free.
   - the typed table: `BattleSlotParamEntry entries[10]` (0x20 each, so 0x140 is
     exactly the ten of them) followed by `u8 rows[16][8][8]`, read as
     rows[recA][recB][i] - 59.85% / 452 bytes / 113 insns. The typed chain changes
     the emission, so the raw offset arithmetic is kept. The 0x40/8/i strides do
     say the blob is a 0x140-byte header plus a [recA][8][8] byte cube.
   - gcc-2.6.0 screen of the whole unit: this function unchanged at 62.91 while
     BattleResetActorStatDisplay 99.19, BattleClearCommandSlots 99.33,
     BattleCheckCommandKeyStatusBit 99.78, BattleInitSlotRowsForActor 99.44,
     BattleAdvanceCommandStep 99.80 and BattleReleaseCharSlot 92.28 all drop, so
     the unit is not a 2.6.0 unit and no preset change is owed.
   - permute.sh 4 min -j 2 from this body: PERMUTER_NONE, best 3118 against a base
     near 5677, and the best candidate only aliased ofsA into a new local - no
     structural lead.
   Invariants already proved (do not re-derive): the four index chains must be
   written out in full at every site because the stores kill the loads (a hoisted
   `u8 *pParam` scores 43.58%), and the element byte must be a volatile load
   because retail reads the same byte twice at site 1 (59.15% without it).
   Lead for the next round: the residual is which pseudo wins a1/a2, i.e. where the
   loaded g_BattleSlotParamTable pointer is born. Untried: a per-site local
   `u8 *pTable = g_BattleSlotParamTable;` at each of the four sites (only the
   hoisted-out-of-loop form has been measured), and keeping `a` alive by one extra
   use after loop2 so gcc cannot remask arg0. */

INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800877E0);
/* PARKED 63.01%, 448 bytes / 112 insns (retail 456 = 114). Round 465 redo.
   PROVEN DIAGNOSIS, new and measured: with the final `g_BattleActorCharIds[a]`
   use REMOVED the same body compiles to retail's allocation exactly - pTable in
   $a1, the volatile entry pointer in $a0 (both loads off it), ofsA $t0, ofsB $t1,
   o $a2, i $a3, the base-first addu at every element site - 440 bytes, 65.52%
   (with that use deleted the code is wrong; it is a diagnostic, not a candidate).
   Adding the use back costs ONE register: gcc rematerialises `(u8)arg0`
   (`andi v0,a0,0xff`, which it puts in the beqz delay slot) and keeps arg0 live
   in $a0, so every other allocno sits one register higher (elem $a1, pTable $a2,
   o $t0, ofsA $t1, ofsB $t2) while retail copies the masked value to $t2
   (`andi v0,a0,0xff; addu t2,v0,zero`) and frees $a0. So the residual is NOT
   "which pseudo wins $a1/$a2" (round 452 note) but "the masked arg0 has no
   home"; the two instructions of the size gap are retail's `nop` in the beqz
   delay slot plus that copy.
   Round 465 measurements (all 448 bytes unless noted):
   - a NAMED `u8 *pEntry` local for the first chain's two volatile reads: 63.01,
     the best body, and the realistic spelling (retail reads $a0 twice); kept.
   - `s32 a; a = arg0 & 0xFF;`: masks IN PLACE in $a0 (`andi a0,a0,0xff`), so $a0
     stays blocked and the same cascade remains - dead.
   - `o` dropped, stores spelled `D_800C48EC + i * 6`: retail's offset register
     becomes a strength-reduced POINTER (`la` + `sh 0(v0)`), not the $at form -
     dead, a named `o` is required.
   - inline `(u8)arg1 * 0x1C` with no `b` local: part of the 63.01 body.
   Do not redo (round 452): typed 0x140 + [recA][8][8] table 59.85, hoisted
   `u8 *pParam` 43.58, non-volatile entry read 59.15, accumulator `o += 6` 60.72,
   gcc-2.6.0 screen of the unit (it cannot switch), permuter 4 min (no find).
   NEXT: the whole residual is one rematerialisation. Anything that stops gcc
   recomputing arg0 & 0xFF - or an `-dl` allocator dump showing global_alloc
   never allocates it - is the only thing left; the register order, the operand
   orders and the byte shape all follow from it.

s32 func_800877E0(s32 arg0, s32 arg1) {
    s32 o;
    s32 i;
    u8 a;
    u8 *pEntry;
    s32 ofsA;
    s32 ofsB;
    s32 res;

    res = 0;
    for (i = 0; i < 9; i++) {
        o = i * 6;
        *(u16 *)(D_800C48EC + o) = 0xFFFF;
        *(u16 *)(D_800C48EE + o) = 0xFFFF;
    }
    a = arg0;
    ofsA = a * 0x1C;
    ofsB = (u8)arg1 * 0x1C;
    *(u16 *)(D_800C48EC + 0) = *(u16 *)(D_800C3EBE + ofsA);
    *(u16 *)(D_800C48EE + 0) = *(u16 *)(D_800C3EC0 + ofsA);
    D_800C48F0[0] = 0;
    for (i = 1; i < 8; i++) {
        o = i * 6;
        pEntry = g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 0x40 + g_BattleSlotRecords[ofsB] * 8 + 0x140 + i;
        if (*(volatile u8 *)pEntry == 0xFF) {
            break;
        }
        *(u16 *)(D_800C48EC + o) =
            *(u16 *)(g_BattleSlotParamTable + ((*(volatile u8 *)pEntry & 7) << 5));
        *(u16 *)(D_800C48EE + o) =
            *(u16 *)(g_BattleSlotParamTable + ((*(volatile u8 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 0x40 + g_BattleSlotRecords[ofsB] * 8 + 0x140 + i) & 7) << 5) + 2);
        D_800C48F0[o] = *(volatile u8 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofsA] * 0x40 + g_BattleSlotRecords[ofsB] * 8 + 0x140 + i) & 0x80;
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

    BattleDrawCommandRowSprites();
    idx = arg0 * 0x40;
    BattleInitSlotRowsForActor(arg0, (g_BattleCommandQueue + idx)[0x3C]);
    g_BattleSoundEnabled = 0;
    r = func_800877E0(arg0, (g_BattleCommandQueue + idx)[0x3C]);
    BattleInitActorSlotAnimation(r, arg0, (g_BattleCommandQueue + idx)[0x3C], BattleFindMinValueOtherSlot(arg0));
    g_BattleSoundEnabled = 1;
    g_BattleSlotDisplayInitialized = 0;
}
/* 99.96%, 1004 bytes / 251 insns (= retail); `check_bin.sh battle` prints OK, so the
   linked bytes are identical. The only objdiff row left is the spelling of the
   g_BattleCommandRecords store: the typed `[idx].param` folds the +0x24 addend into the
   instruction (`sh $v0,36($at)` off %hi(g_BattleCommandRecords)) where retail relocs
   g_BattleCommandSlotMasks with displacement 0; both link to the same word, and the
   `extern u8 g_BattleCommandSlotMasks[]` spelling costs a queue-pointer reload
   (1016 bytes, 96.89), so the typed one stays.
   Round 427 closed the three register mirrors with two levers:
   - the negated offset (`g_BattleCommandQueue - -(slot * 0x40))[0x3C]`) at the two
     first-block sites and at D_800CCD1C; it is what makes cc1 put the loaded queue in
     $v0 and the scaled slot in $v1, base first (99.60 -> 99.68 -> 99.76 -> 99.80).
   - `u16 step;` (not s32) for the `>= 8` arm's step byte; its own variable, so the arm
     no longer shares a pseudo with the BattleScriptScanForOpcode62 result, and u16 is
     what makes gcc keep it in $a1 and mask in place (99.80 -> 99.96). Found by the
     permuter (base=130 best=10).
   Load-bearing (each measured at 1004 bytes): the u8 parameters; `s32 commandNo;`
   holding `arg1 & 0xFF`; the five later queue reads as an EMBEDDED ASSIGNMENT
   `(pQueueRow = g_BattleCommandQueue + slot * 0x40)[0x3C]`; `(u16)BattleMaskHasStatusBit`;
   `*(u8 *)&g_BattleActionParam`; `BattleSlotBitMask` returning s32;
   `count = *(volatile u8 *)&g_BattleCommandQueue[0x2DA]`; the two record stores as
   plain u8 arrays `g_BattleCommandSlotIds[... * 0x48]`.
   Dead spellings: `*(g_BattleCommandQueue + i + K)`, `(s32)g_BattleCommandQueue[0x2DC] < 8`,
   the non-volatile step read (1000 bytes), `*arg0`-style base locals, the embedded
   assignment at D_800CCD1C (97.37), `charId`/`commandNo` locals at the arm (identical),
   gcc-2.6.0 (99.96 identical) and gcc-2.7.2-cdk (compile-failed). */
s32 BattleAdvanceCommandStep(u8 arg0, u8 arg1) {
    u8 *pQueueRow;
    u8 slot;
    u8 count;
    s32 commandNo;
    s32 stepNo;
    u16 step; // u16, not s32: keeps the step byte in $a1 and masks it in place
    u8 rawStep;
    s32 done;

    done = 0;
    if (g_BattleSlotDisplayInitialized == 0) {
        if (g_BattleSlotFlags[(u8)arg0 * 8] == 0) {
            if (g_BattleActorCharIds[(u8)arg0] != 4) {
                // The negated offset is what makes retail's addu put the queue base first.
                func_80087EDC((u8)arg0, (g_BattleCommandQueue - -((u8)arg0 * 0x40))[0x3C]);
            }
        } else {
            func_800881B8((u8)arg0, (g_BattleCommandQueue - -((u8)arg0 * 0x40))[0x3C]);
        }
        g_BattleSlotDisplayInitialized = 1;
    }
    BattleClearSlotCurveEntry();
    if (g_BattleSlotFlags[(u8)arg0 * 8] == 0) {
        if (g_BattleCommandQueue[0x2DC] < BATTLE_COMMAND_STEP_MAX) {
            commandNo = arg1 & 0xFF;
            g_BattleCommandQueue[0x2DC] = g_BattleCommandStepTable[g_BattleCommandQueue[0x2DC]][commandNo];
        } else {
            rawStep = *(volatile u8 *)&g_BattleCommandQueue[0x2DC];
            step = rawStep - BATTLE_COMMAND_STEP_MAX;
            if ((u16)BattleMaskHasStatusBit(
                           *(u16 *)((u8 *)&g_GameState + 0x16C0 + g_BattleActorCharIds[(u8)arg0] * 0x20),
                           step) != 0) {
                done = 1;
            } else {
                g_BattleCommandQueue[0x2DC] = BATTLE_COMMAND_STEP_MAX - 1;
            }
        }
    } else {
        g_BattleCommandQueue[0x2DC] = g_BattleCommandQueue[0x2DC] + 1;
    }
    slot = (u8)arg0;
    // Same negated offset: retail's addu is base-first at this site too.
    if (D_800CCD1C[(g_BattleCommandQueue - -(slot * 0x40))[0x3C]].flags & 0x800) {
        g_BattleCommandCodes[g_BattleCommandQueue[0x2DA] * 0x48] = arg0;
        g_BattleCommandSlotIds[g_BattleCommandQueue[0x2DA] * 0x48] = 0xF3;
        g_BattleCommandRecords[g_BattleCommandQueue[0x2DA]].param =
            BattleSlotBitMask((pQueueRow = g_BattleCommandQueue + slot * 0x40)[0x3C]);
        g_BattleCommandQueue[0x2DA] = g_BattleCommandQueue[0x2DA] + 1;
    }
    BattleBeginActorAction(slot, (u16)BattleSlotBitMask((pQueueRow = g_BattleCommandQueue + slot * 0x40)[0x3C]),
                           g_BattleCommandQueue[0x2DC] - 1);
    count = *(volatile u8 *)&g_BattleCommandQueue[0x2DA];
    g_BattleCommandCodes[g_BattleCommandQueue[0x2DA] * 0x48] = arg0;
    g_BattleCommandRecords[g_BattleCommandQueue[0x2DA]].slotMask = g_BattleActionSlotMask;
    g_BattleCommandSlotIds[g_BattleCommandQueue[0x2DA] * 0x48] = *(u8 *)&g_BattleActionParam;
    g_BattleCommandQueue[0x2DA] = g_BattleCommandQueue[0x2DA] + 1;
    BattleSetSlotRegistersFromActor(slot, (pQueueRow = g_BattleCommandQueue + slot * 0x40)[0x3C]);
    if ((pQueueRow = g_BattleCommandQueue + slot * 0x40)[0x3C] >= 3) {
        stepNo = (u8)BattleScriptScanForOpcode62((pQueueRow = g_BattleCommandQueue + slot * 0x40)[0x3C]);
        if (stepNo != 0) {
            done = 1;
        }
    }
    BattleApplySlotCurveAndStep(count);
    return done;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80087EDC);
/* 82.72%, 692 bytes (retail 684). The sel ladder is now retail's
   (`sel = 0; if (arg1 < 3) sel = (arg0 >= 3) * 8;`), the mask needs
   `(u16)BattleMaskHasSlotBit(...)` (without the cast the `andi v0,v0,0xFFFF` after
   the jal is missing) and the parameters are s32 with `(u8)` casts - u8 params
   lose the repeated `andi` retail emits at every use.
   Residual: retail recomputes `arg0 * 0x1C` after the battle of calls and reuses
   ONE saved register ($s1) for ofsB in the middle and ofsA at the tail, while gcc
   keeps both live from the top ($s1/$s3, +8 bytes); the tail also reads
   g_BattleSlotRecords[ofsA] before D_800C3EB5[ofsA] where retail reads the reverse.
   Dead spellings measured: one `ofs` variable reassigned after the loop (720),
   ofsA/ofsB inline at every site (652, 66.43%), ofsA assigned only at the tail
   (700 and the prologue scatters, 84.84%), a typed `BattleCharSlotRow` struct view
   (564, 60.05%: gcc merges the loads). Permuter 1679 iterations from this body,
   base=3246 best=2636, and its only find is a dead `if (D_800C3EC0)` around the
   sel assignment - not real source.
*/
/*
void func_80087EDC(s32 arg0, s32 arg1) {
    s32 ofsA;
    s32 ofsB;
    s32 sel;
    s32 i;

    ofsA = (u8)arg0 * 0x1C;
    ofsB = (u8)arg1 * 0x1C;
    if (g_BattleSlotRecords[ofsA] == g_BattleSlotRecords[ofsB]) {
        return;
    }
    sel = 0;
    if ((u8)arg1 < 3) {
        sel = ((u8)arg0 >= 3) * 8;
    }
    if (g_BattleCharSlotUsage[(g_BattleSlotRecords[ofsB] + sel) << 2] >= 4) {
        return;
    }
    BattleReleaseCharSlot((u8)arg0);
    g_BattleCharSlotUsage[(g_BattleSlotRecords[ofsB] + sel) << 2]++;
    for (i = 0; i < 4; i++) {
        if ((u16)BattleMaskHasSlotBit(D_800D301D[(g_BattleSlotRecords[ofsB] + sel) << 2], i) == 0) {
            break;
        }
    }
    D_800C3EB5[ofsA] = i;
    g_BattleSlotRecords[ofsA] = g_BattleSlotRecords[ofsB];
    D_800D301D[(g_BattleSlotRecords[ofsA] + sel) << 2] |= BattleSlotBitMask(D_800C3EB5[ofsA]);
    if ((u8)arg0 < 3) {
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

/* The body below reproduces retail's TRAMPOLINE, which plain spellings miss: the
   skip path is `j .Lstore` with `addu a2,v1` in its delay slot, placed BEFORE the
   find loop, and the loop's break branches back into it (cross-jumping of
   `sel = rec` from the else arm with `sel = i` from the break). It needs `sel` as
   a SEPARATE variable from `rec` that is NOT initialised before the if: on the
   loop-exhausted path the original leaves it undefined (retail's `sb $a2` there
   reads the same unset register), so this is the one body shape that matches - a
   well-defined `rec = i; break;` spelling has no second assignment for cse to
   merge and puts the loop straight after the test. */
void BattleAllocCharSlot(u8 arg0) {
    u8 rec;
    u8 sel;
    s32 i;
    s32 ofs;
    s32 stride;

    rec = g_BattleSlotRecords[(u8)arg0 * 0x1C];
    if (g_BattleCharSlotUsage[(rec + 0x10) << 2] == 0) {
        sel = rec;
    } else {
        for (i = 0; i < 8; i++) {
            if (g_BattleCharSlotUsage[(i + 0x10) << 2] == 0) {
                sel = i;
                break;
            }
        }
    }
    ofs = (u8)arg0 * 0x1C;
    g_BattleSlotRecords[ofs] = sel;
    D_800C3EB5[ofs] = 0;
    D_800D301D[(g_BattleSlotRecords[ofs] + 0x10) << 2] = 1;
    // retail runs the two usage chains one at a time; the empty loop notes keep
    // gcc's scheduler from hoisting the next record reload above the previous store
    do {
    } while (0);
    g_BattleCharSlotUsage[(g_BattleSlotRecords[ofs] + 0x10) << 2] = 1;
    // the param table holds one 8-byte row per character slot
    stride = 8;
    *(u16 *)(D_800C3EBE + ofs) = *(u16 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofs] * stride + 0x100);
    *(u16 *)(D_800C3EC0 + ofs) = *(u16 *)(g_BattleSlotParamTable + g_BattleSlotRecords[ofs] * stride + 0x102);
}


u8 BattleIsCharSlot18InUse(u8 arg0) {
    return g_BattleCharSlotUsage[(g_BattleSlotRecords[arg0 * 0x1C] + 0x18) << 2];
}
// Draws the two sprite-column halves of the slot display: two package draws per half
// followed by the blend pass over the prims each one appended. pPrimStart keeps the
// prim counter's address in a register across the middle draw, as retail does.
void BattleDrawSlotSpriteColumns(void) {
    s32 i;
    u8 *pPrimStart;

    for (i = 0; i < 2; i++) {
        g_BattleSpriteColumnWork[0x5D74] += BattleDrawSpritePackage(g_BattleSlotSpriteColumnIds[i], (s32)(g_BattleSpriteColumnWork[0x5D74] * 0x50 + (0x1720 + (s32)g_BattleSpriteColumnWork)), 0xA0, 0x64);
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
        g_BattleSpriteColumnWork[0x5D7E] += BattleDrawSpritePackage(g_BattleSlotSpriteColumnIds[i], (s32)(g_BattleSpriteColumnWork[0x5D7E] * 0x50 + (0x2530 + (s32)g_BattleSpriteColumnWork)), 0xA0, 0x64);
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
