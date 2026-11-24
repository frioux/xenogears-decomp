#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/libgpu.h"
#include "system/controller.h"
#include "battle/actor.h"

/* The battle text-list record drawn by this unit. main/misc8.c carries the full
   definition (BattleTextEntry/BattleTextFrame/BattleTextList); this is the view
   of the fields the option screen touches, with the frame coordinates s16
   because retail reads them with `lh`. Owed a shared header. */
typedef struct {
    /* 0x0 */ u8 flags;
    /* 0x1 */ u8 unk1[0x13];
} BattleTextEntry; // Size: 0x14

typedef struct {
    /* 0x0 */ u8 unk0[8];
    /* 0x8 */ s16 x;
    /* 0xA */ s16 y;
    /* 0xC */ s16 w;
    /* 0xE */ u16 h;
} BattleTextFrame; // Size: 0x10

typedef struct {
    /* 0x00 */ u8 extraWidth;
    /* 0x01 */ u8 unk1[3];
    /* 0x04 */ BattleTextEntry* pEntries;
    /* 0x08 */ s16 count;
    /* 0x0A */ s16 nextListIndex;
    /* 0x0C */ u8 unkC[6];
    /* 0x12 */ s16 row;
    /* 0x14 */ s16 y;
    /* 0x16 */ u8 unk16[6];
    /* 0x1C */ BattleTextFrame frames[2]; /* current, saved */
} BattleTextList; // Size: 0x3C

/* One row of the option screen's text list: a text pointer and the handler
   that reacts to the row, plus the text sprite id shown beside it. Two rows
   per page; the pages themselves are data, at g_BattleOptionPages. */
typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u8* pText;
    /* 0x08 */ void (*pHandler)(void);
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 unk10;
} BattleOptionRow; // Size: 0x14

typedef struct {
    /* 0x00 */ BattleOptionRow rows[2];
} BattleOptionPage; // Size: 0x28

extern BattleOptionPage g_BattleOptionPages[];

/* The nine flag bytes below are declared as open arrays on purpose: -G8
   classifies a 1-byte extern as small data, which makes its address one
   gp-relative insn, so gcc rematerialises it at each use instead of holding
   it in a saved register across the call the way the original does. */
extern u8 g_BattleDifficultyLevel[];
extern u8 g_BattleFrameRateLevel[];
extern u8 g_BattleMotionSpeedLevel[];
extern u8 g_BattlePlayer1Vibration[];
extern u8 g_BattlePlayer2Vibration[];
extern u8 g_BattlePlayer1ComControlled[];
extern u8 g_BattlePlayer2ComControlled[];
extern u8 g_BattleMatchCount[];
extern u8 g_BattleAiBehaviorLevel[];
extern u32 g_BattleDifficultyNames[];
extern s32* g_BattleCurrentTextList;
extern u8 g_BattleOptionOn;
extern s32 g_BattleScreenCopyRequest;
extern s32 D_80098800[];
extern s32 D_800970E4[];
extern s32 g_BattleRoundNumber;
extern u8* g_BattleAiBehaviorNames[];
extern u8 g_BattleFrameRateTable[];
extern u8 g_StrEmpty[];
extern char g_StrFpsFormat[];
extern s32 g_BattleButtonStatePressedOnce;
extern s32 g_BattleButtonStateReleased;
extern u8* g_BattleSpriteRects;
extern s32 g_BattleCursorIndex0;
extern s32 g_BattleCursorIndex1;
extern s32 g_BattleSlotConfirmFlags;
extern s32 g_BattlePrevCursorIndex0;
extern s32 g_BattlePrevCursorIndex1;
extern s32 g_BattleCurrentTextPadIndex;
extern s32 g_BattleEntryCount;
extern s32 g_BattleMode;
extern s32 g_BattleButtonState;
extern s32** g_BattleEntries;
extern u8 g_StrIntFormat[];
extern u8 g_StrNumberPlaceholder[];
extern u8 g_StrOn[];
extern u8 g_StrOff[];
extern u8 g_StrCom[];
extern u8 g_StrUser1[];
extern u8 g_StrUser2[];
extern s32 g_BattleMotionSpeedTable[];
extern s32 g_BattleMotionSpeedScale;
extern u16 g_BattleMotionSpeedScaleOption[];
extern u8* g_BattleSpriteArchiveBuffer;
extern s32 g_BattleSpriteArchiveFreed;
extern s32 g_BattleCursor0Step;
extern s32 g_BattleCursor0Offset;
extern s32 g_BattleCursor1Step;
extern s32 g_BattleCursor1Offset;
extern BattleTextList g_BattleOptionTextList[];
/* Three fields of the option pages keep their own symbol because retail forms
   each address with a fresh lui/%lo: naming them off the pPad base makes gcc
   use the base register instead and BattleDrawVibrationOptions comes out 16
   bytes short (588 against retail's 604). D_800916DC/D_80091718 are
   g_BattleOptionTextList[0]/[1].pEntries, D_800916E2 is page 0's
   nextListIndex (its `[0]` spelling emits an extra `la`). */
extern s16 D_800916E2;
extern BattleTextEntry* D_800916DC;
extern BattleTextEntry* D_80091718;
extern s32 g_BattleSlot0TextColor;
extern s32 g_BattleSlot1TextColor;
extern s32 g_BattleSlotConfirmLatched;
extern s32 g_BattleRootOrderingTable;
extern u8 g_StrVibrationOn[];
extern u8 g_StrVibrationOff[];

void BattleSetTextColor(s32 arg);
void BattleClearTextSpriteEntries(void);
void BattleStartTextScreen(s32 arg);
void BattleSetScreenMode(s32 mode);
void BattleSetTextCursor(s32 a, s32 b);
void BattleDrawTextRight(u8* pText);
void BattleSetTextSpriteEntry(s32 entry, s32 mode);
void BattlePlaySoundEffectById(s32 arg);
void ArchiveCdDataSync(int mode);
void BattleLoadSlotEntry(s32 index, s32 value);
void BattleFreeSlotEntry(s32 index);
s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags);
s32 WorldMapInitializeModeWithActors(s32 value, s32 max, s32 flags);
void BattleSetCurrentTextList(s32 arg);
void BattleByteRegTestReg(u8* pText);
void func_8007F258(void* pOt, s32 arg);
extern int ControllerGetType(int controllerIndex);

// 99.17% (96 bytes, retail 96, 24 insns). Residual is the ORDER of the two
// `|= 0xC` chains, not their registers: retail emits D_80098800's whole chain
// (la $a1, lw $v1, ori, sw $v1) then `lw ra,24(sp)` then D_800970E4's (la $a0,
// lw $v0, ori, sw $v0); this body emits the 970E4 chain first. In the body kept
// below the registers are already retail's ($a1 = D_80098800, $a0 = 970E4).
// The two are coupled through the allocator: whichever chain's base pseudo dies
// first in RTL (i.e. is stored first) takes $a0, so the source order that fixes
// the registers also fixes the emission order. Retail has the last-dying base in
// $a0, i.e. its post-reload scheduler swapped the two chains - no source order
// does that and the permuter cannot reach it either.
// Measured this round: plain `D_80098800[0] |= 0xC; D_800970E4[0] |= 0xC;` in
// that order 96.25 (registers mirrored: $a0 = 98800); the SAME two statements
// reversed 95.42 (registers right, both chains' emission order mirrored - the
// cleanest body, one scheduler decision from a match); the park below (value
// temporaries for the 98800 chain, its store last) 99.17, i.e. the two chains
// swapped. Older measurements: pointer locals A-first 96.25, 98800 store split
// 98.33, both chains via value temporaries 97.71, s32 return with
// `D_80098800[0] |= 0xC; return (D_800970E4[0] |= 0xC);` 97.92 (forces $v0/$v1 on
// the returned value, reproduces retail's ENTIRE order, only the two bases
// swapped), gcc-2.6.0 90.42/88.75, gcc-2.7.2-cdk 86.5, try_flags --sweep: max
// 97.5 at 2.7.2 -O2/-O3 x -G4/-G8.
// Permuter: three earlier runs (4/4/3 min) reached 97.50 -> 98.75 -> 99.17 -> no
// find, plus this round a fourth 3-min run from 99.17 (2022 iterations,
// base=20 best=20, PERMUTER_NONE). The useful body is the RETURN form above, so
// on a candidates list prefer it: its only residual is the two base registers.
// Evidence that retail returns the 970E4 flags word: no callers, and $v0 is
// forced for a returned value while retail has $v0 = 970E4.
// Round 422 re-measured the park at 99.17 (its residual is the two STORES
// swapped around the `lw ra` fill: 970E4 then 98800 against retail's 98800 then
// 970E4; every address, load and ori is already retail's) and tried two fresh
// shapes, both 96.25: loading BOTH words before storing either
// (`a = A[0]; b = B[0]; A[0] = a | 0xC; B[0] = b | 0xC;`) is byte-identical to
// the plain form, and a pointer local for the 98800 chain with the 970E4 chain
// spelled directly (`p = D_80098800; p[0] |= 0xC; D_800970E4[0] |= 0xC;`) is the
// plain form too - so neither moves the store order off the register pair.
/*
void func_8007F854(void) {
    s32* new_var;
    int new_var3;
    s32 tmp[2]; // reproduces the original's unused 8-byte stack slot
    s32* pA;
    int new_var2;
    s32* pB;

    g_BattleScreenCopyRequest = 1;
    BattleSetScreenMode(1);
    g_BattleCurrentTextList = 0;
    BattleClearTextSpriteEntries();
    pA = D_80098800;
    new_var = D_800970E4;
    pB = new_var;
    new_var3 = pA[0];
    new_var3 = new_var3 | 0xC;
    pB[0] |= 0xC;
    new_var2 = new_var3;
    pA[0] = new_var2;
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F854);


void BattleResetTextListState(void) {
    s32 tmp[2]; // reproduces the original's unused 8-byte stack slot

    BattleSetScreenMode(1);
    g_BattleCurrentTextList = 0;
    BattleClearTextSpriteEntries();
}

void BattleExitOptionsScreen(void) {
    s32 mode;
    s32 current;

    BattleStartTextScreen(0);
    if (g_BattleMatchCount[0] != 0) {
        mode = 3;
    } else {
        current = g_BattleRoundNumber;
        if (current == 1) {
            mode = 3;
        } else {
            g_BattleRoundNumber = current - 1;
            mode = 6;
        }
    }
    BattleSetScreenMode(mode);
}

void BattleSetRowTextColor(BattleTextList* p, s32 arg) {
    if (p->row == arg) {
        BattleSetTextColor(1);
    } else {
        BattleSetTextColor(0);
    }
}

u32 BattleGetDifficultyText(void) {
    return g_BattleDifficultyNames[g_BattleDifficultyLevel[0]];
}

void BattleDrawOptionRows0To4(BattleTextList* p) {
    u8 buf[0x8];
    u8* pText;
    s32 value;

    BattleSetTextCursor(p->frames[0].x + p->frames[0].w - 0xA, p->y);
    BattleSetTextColor(0);
    BattleSetRowTextColor(p, 0);
    BattleDrawTextRight((u8*)BattleGetDifficultyText());
    BattleSetRowTextColor(p, 1);
    Sprintf(buf, g_StrIntFormat, g_BattleMotionSpeedLevel[0] + 1);
    value = g_BattleMotionSpeedTable[g_BattleMotionSpeedLevel[0]];
    g_BattleMotionSpeedScale = value;
    g_BattleMotionSpeedScaleOption[0] = value;
    BattleDrawTextRight(buf);
    BattleSetRowTextColor(p, 2);
    Sprintf(buf, g_StrFpsFormat, g_BattleFrameRateTable[g_BattleFrameRateLevel[0]]);
    BattleDrawTextRight(buf);
    BattleSetRowTextColor(p, 3);
    pText = g_StrUser1;
    if (g_BattlePlayer1ComControlled[0] != 0) {
        pText = g_StrCom;
    }
    BattleDrawTextRight(pText);
    BattleSetRowTextColor(p, 4);
    pText = g_StrUser2;
    if (g_BattlePlayer2ComControlled[0] != 0) {
        pText = g_StrCom;
    }
    BattleDrawTextRight(pText);
    BattleSetTextColor(0);
}

void BattleDrawOptionRows0To2(BattleTextList* p) {
    u8 buf[0x8];

    BattleSetTextCursor(p->frames[0].x + p->frames[0].w - 0xA, p->y);
    BattleSetTextColor(0);
    BattleDrawTextRight(g_StrEmpty);
    BattleSetRowTextColor(p, 1);
    BattleDrawTextRight(g_BattleAiBehaviorNames[g_BattleAiBehaviorLevel[0]]);
    BattleSetTextSpriteEntry(g_BattleAiBehaviorLevel[0] + 0x15, 1);
    BattleSetRowTextColor(p, 2);
    Sprintf(buf, g_StrFpsFormat, g_BattleFrameRateTable[g_BattleFrameRateLevel[0]]);
    BattleDrawTextRight(buf);
    BattleSetTextColor(0);
}

void BattleDrawVibrationOptions(void) {
    s32 tmp[2]; // reproduces the original's unused 8-byte stack slot
    BattleTextList* pPad;
    s32 flag;

    BattleSetTextCursor(0xA0, 0x8C);
    pPad = &g_BattleOptionTextList[0];
    flag = !(g_BattleSlotConfirmFlags & 1);
    if (flag && pPad->row == 0) {
        g_BattleSlot0TextColor = 1;
    } else {
        g_BattleSlot0TextColor = 0;
    }
    BattleSetTextCursor(0x50, 0x8C);
    if (ControllerGetType(0) == 4 && g_BattlePlayer1ComControlled[0] == 0) {
        if (flag) {
            BattleSetRowTextColor(pPad, 1);
        }
        BattleByteRegTestReg((g_BattlePlayer1Vibration[0] & 1) ? g_StrVibrationOn : g_StrVibrationOff);
        D_800916DC[1].flags &= ~4;
    } else {
        D_800916DC[1].flags |= 4;
    }
    BattleSetTextColor(0);
    flag = !(g_BattleSlotConfirmFlags & 2);
    if (flag && g_BattleSlotConfirmLatched == 0) {
        flag = 0;
    }
    pPad = &g_BattleOptionTextList[1];
    if (flag && pPad->row == 0) {
        g_BattleSlot1TextColor = 1;
    } else {
        g_BattleSlot1TextColor = 0;
    }
    BattleSetTextCursor(0xF0, 0x8C);
    if (ControllerGetType(1) == 4 && g_BattlePlayer2ComControlled[0] == 0) {
        if (flag) {
            BattleSetRowTextColor(pPad, 1);
        }
        BattleByteRegTestReg((g_BattlePlayer2Vibration[0] & 1) ? g_StrVibrationOn : g_StrVibrationOff);
        D_80091718[1].flags &= ~4;
    } else {
        D_80091718[1].flags |= 4;
    }
    BattleSetTextColor(0);
    func_8007F258(g_BattleRootOrderingTable, 1);
}

void BattleDrawOptionRows4To6(BattleTextList* p) {
    u8 buf[0x10];
    u8* pText;
    u8 flag;

    BattleSetTextColor(0);
    BattleSetTextCursor(p->frames[0].x + p->frames[0].w - 0xA, p->y);
    BattleDrawTextRight(g_StrEmpty);
    BattleDrawTextRight(g_StrEmpty);
    BattleDrawTextRight(g_StrEmpty);
    BattleDrawTextRight(g_StrEmpty);
    BattleSetRowTextColor(p, 4);
    flag = g_BattleMatchCount[0];
    if (flag != 0) {
        Sprintf(buf, g_StrIntFormat, flag);
        pText = buf;
    } else {
        pText = g_StrNumberPlaceholder;
    }
    BattleDrawTextRight(pText);
    BattleSetRowTextColor(p, 5);
    BattleDrawTextRight((u8*)g_BattleDifficultyNames[g_BattleDifficultyLevel[0]]);
    BattleSetRowTextColor(p, 6);
    pText = g_StrOff;
    if (g_BattleOptionOn != 0) {
        pText = g_StrOn;
    }
    BattleDrawTextRight(pText);
    BattleSetTextColor(0);
}

s32 WorldMapInitializeModeWithActors(s32 value, s32 max, s32 flags) {
    s32 delta = 1;
    s32 pad;
    s32 status;

    if (flags & 4) {
        delta = -1;
    }
    pad = 0;
    if (flags & 2) {
        status = g_BattleButtonStatePressedOnce;
    } else {
        status = g_BattleButtonStateReleased;
    }
    if (status & 0x2000) {
        value += delta;
    }
    if (status & 0x8000) {
        value -= delta;
    }
    if (status & 0xA000) {
        pad = 1;
    }
    if (flags & 1) {
        if (value == -1) {
            value = max;
        }
        if (value > max) {
            value = 0;
        }
    } else {
        if (value == -1) {
            pad = 0;
            value = 0;
        }
        if (value > max) {
            pad = 0;
            value = max;
        }
    }
    if (pad) {
        BattlePlaySoundEffectById(0x20);
    }
    return value;
}

void BattleMoveDifficultyCursor(void) {
    g_BattleDifficultyLevel[0] = WorldMapInitializeModeWithActors(g_BattleDifficultyLevel[0], 2, 0);
}

void BattleMoveSpeedCursor(void) {
    g_BattleMotionSpeedLevel[0] = WorldMapInitializeModeWithActors(g_BattleMotionSpeedLevel[0], 7, 2);
}

void BattleMoveFrameRateCursor(void) {
    g_BattleFrameRateLevel[0] = WorldMapInitializeModeWithActors(g_BattleFrameRateLevel[0], 4, 2);
}

void BattleMoveVibrationCursorPad0(void) {
    g_BattlePlayer1Vibration[0] = WorldMapInitializeModeWithActors(g_BattlePlayer1Vibration[0], 1, 1);
}

void BattleMoveVibrationCursorPad1(void) {
    g_BattlePlayer2Vibration[0] = WorldMapInitializeModeWithActors(g_BattlePlayer2Vibration[0], 1, 1);
}

void BattleMoveComCursorPad0(void) {
    g_BattlePlayer1ComControlled[0] = WorldMapInitializeModeWithActors(g_BattlePlayer1ComControlled[0], 1, 1);
}

void BattleMoveComCursorPad1(void) {
    g_BattlePlayer2ComControlled[0] = WorldMapInitializeModeWithActors(g_BattlePlayer2ComControlled[0], 1, 1);
}

void BattleMoveValueOptionCursor(void) {
    g_BattleMatchCount[0] = WorldMapInitializeModeWithActors(g_BattleMatchCount[0], 3, 2);
}

s32 BattleMoveOnOffCursor(void) {
    s32 value = WorldMapInitializeModeWithActors(g_BattleOptionOn, 1, 1);

    g_BattleOptionOn = value;
    return value;
}

void BattleMoveComBehaviorCursor(void) {
    g_BattleAiBehaviorLevel[0] = WorldMapInitializeModeWithActors(g_BattleAiBehaviorLevel[0], 0xD, 3);
}

void BattleHandleSelectionInputPad0(void) {
    s32 same;

    if (g_BattleCurrentTextPadIndex == 0) {
        if (g_C1ButtonStateReleased & 0x40) {
            g_BattleSlotConfirmFlags &= -2;
            BattleFreeSlotEntry(0);
            BattlePlaySoundEffectById(0x22);
        }
    }
    if ((g_BattleSlotConfirmFlags & 1) == 0) {
        same = (g_BattleCursorIndex0 == g_BattleCursorIndex1);
        do {
            g_BattleCursorIndex0 = WorldMapInitializeModeWithActors(g_BattleCursorIndex0, g_BattleEntryCount - 1, 3);
        } while (g_BattleCursorIndex0 == g_BattleCursorIndex1 && (g_BattleButtonState & 1) == 0 && !same);
        if (g_BattleCurrentTextPadIndex == 0) {
            if (g_C1ButtonStateReleased & 0x20) {
                g_BattleSlotConfirmFlags |= 1;
                BattlePlaySoundEffectById(0x21);
                BattleLoadSlotEntry(0, *g_BattleEntries[g_BattleCursorIndex0]);
            }
        }
    }
}

void BattleHandleSelectionInputPad1(void) {
    s32 same;

    if (g_BattleButtonStateReleased & 0x40) {
        if (g_BattleMode == 2) {
            g_BattleSlotConfirmFlags &= -3;
        } else {
            g_BattleSlotConfirmFlags = 0;
            BattleFreeSlotEntry(1);
            BattlePlaySoundEffectById(0x22);
            return;
        }
    }
    if ((g_BattleSlotConfirmFlags & 2) == 0) {
        same = (g_BattleCursorIndex0 == g_BattleCursorIndex1);
        do {
            g_BattleCursorIndex1 = WorldMapInitializeModeWithActors(g_BattleCursorIndex1, g_BattleEntryCount - 1, 3);
        } while (g_BattleCursorIndex0 == g_BattleCursorIndex1 && (g_BattleButtonState & 1) == 0 && !same);
        if (g_BattleButtonStateReleased & 0x20) {
            g_BattleSlotConfirmFlags |= 2;
            BattlePlaySoundEffectById(0x21);
            BattleLoadSlotEntry(1, *g_BattleEntries[g_BattleCursorIndex1]);
        }
    }
}

/* The empty cases spell out sprite ids 4..31: without them cc1 compares the
   special ids one by one instead of emitting the jtbl_ dispatch. */
void BattleReloadSlotEntries(void) {
    u32 entry0;
    u32 entry1;

    entry0 = g_BattleEntries[g_BattleCursorIndex0][0];
    entry1 = g_BattleEntries[g_BattleCursorIndex1][0];
    switch (entry0) {
    case 4:
    case 7:
    case 11:
    case 30:
    case 31:
        entry0 = (entry1 == 0);
        break;
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
        break;
    }
    BattleLoadSlotEntry(0, entry0);
    switch (entry1) {
    case 4:
    case 7:
    case 11:
    case 30:
    case 31:
        entry1 = entry0 != 1;
        break;
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
        break;
    }
    BattleLoadSlotEntry(1, entry1);
}

void BattleLoadSelectionTextures(s32 idx0, s32 idx1) {
    u8* pBuf;
    u8* pSrc;
    u8* pSprite;

    pBuf = HeapAlloc(0x2000, 0);
    ArchiveReadFileFromCdSector(ArchiveDecodeSector(6) + idx0 * 2, pBuf, 0x1000, 0, 0);
    ArchiveReadFileFromCdSector(ArchiveDecodeSector(6) + idx1 * 2, pBuf + 0x1000, 0x1000, 0, 0);
    g_BattleCursorIndex0 = idx0;
    g_BattleCursorIndex1 = idx1;
    g_BattlePrevCursorIndex0 = idx0;
    g_BattlePrevCursorIndex1 = idx1;
    g_BattleSlotConfirmFlags = 3;
    ArchiveCdDataSync(0);
    pSrc = pBuf + 0x1000;
    pSprite = g_BattleSpriteRects + idx0 * 0x14;
    LoadImage((RECT*)pSprite, (u_long*)pBuf);
    LoadImage((RECT*)(pSprite + 8), (u_long*)(pBuf + 0x100));
    pSprite = g_BattleSpriteRects + idx1 * 0x14;
    LoadImage((RECT*)pSprite, (u_long*)pSrc);
    LoadImage((RECT*)(pSprite + 8), (u_long*)(pBuf + 0x1100));
    HeapDelayedFree(pBuf, 2);
}

void BattleInitOptionsScreen(s32 mode) {
    u8* pSprite;
    s32 i;

    if (g_BattleSpriteArchiveFreed == 0) {
        ArchiveCdDataSync(0);
        pSprite = g_BattleSpriteRects;
        for (i = 0; i < BATTLE_SLOT_BIT_COUNT; i++) {
            LoadImage((RECT*)pSprite, (u_long*)(g_BattleSpriteArchiveBuffer + i * 0x1000));
            LoadImage((RECT*)(pSprite + 8), (u_long*)(g_BattleSpriteArchiveBuffer + i * 0x1000 + 0x100));
            pSprite += 0x14;
        }
        HeapFree(g_BattleSpriteArchiveBuffer);
        g_BattleSpriteArchiveFreed = 1;
    }
    g_BattleMode = mode;
    D_800916E2 = (mode == 4) ? 3 : 4;
    g_BattleOptionTextList[1].nextListIndex = 5;
    if (mode == 3) {
        g_BattleOptionPages[0].rows[0].unk1 = 0x27;
        g_BattleOptionPages[1].rows[0].unk1 = 0x28;
    } else {
        g_BattleOptionPages[0].rows[0].unk1 = 0x25;
        g_BattleOptionPages[1].rows[0].unk1 = 0x26;
    }
    BattleSetScreenMode(1);
    g_BattleSlotConfirmFlags = 0;
    g_BattleCursor1Offset = 0;
    g_BattleCursor1Step = 0;
    g_BattleCursor0Offset = 0;
    g_BattleCursor0Step = 0;
    g_BattlePrevCursorIndex0 = g_BattleCursorIndex0;
    g_BattlePrevCursorIndex1 = g_BattleCursorIndex1;
    BattleSetCurrentTextList(5);
}
