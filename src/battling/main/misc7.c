#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/libgpu.h"
#include "system/controller.h"

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
extern s32 D_8009274C;
extern s32 D_80092750;
extern u8* g_BattleSpriteRects;
extern s32 g_BattleCursorIndex0;
extern s32 g_BattleCursorIndex1;
extern s32 g_BattleSlotConfirmFlags;
extern s32 g_BattlePrevCursorIndex0;
extern s32 g_BattlePrevCursorIndex1;
extern s32 D_80091364;
extern s32 g_BattleEntryCount;
extern s32 g_BattleMode;
extern s32 D_80092748;
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
extern s16 D_800916E2;
extern s16 D_8009171E;
extern u8 D_80091369;
extern u8 D_80091391;
extern s32 g_BattleCursor0Step;
extern s32 g_BattleCursor0Offset;
extern s32 g_BattleCursor1Step;
extern s32 g_BattleCursor1Offset;
extern u8 D_800916D8[];
extern u8* D_800916DC;
extern s16 D_800916EA;
extern u8 D_80091714[];
extern u8* D_80091718;
extern s16 D_80091726;
extern s32 g_BattleSlot0TextColor;
extern s32 g_BattleSlot1TextColor;
extern s32 D_80092754;
extern s32 D_80092938;
extern u8 g_StrVibrationOn[];
extern u8 g_StrVibrationOff[];

void BattleSetTextColor(s32 arg);
void BattleClearTextSpriteEntries(void);
void func_80080C48(s32 arg);
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F854);

/* 96.25%: the register assignment is NOT mirrored - D_80098800 is (a1,v1) and
   D_800970E4 is (a0,v0) in both the original and our compile. The only
   difference is group order: retail runs the D_80098800 load/ori/store first
   and the D_800970E4 one second, ours always schedules the v0 chain (D_800970E4)
   first even with the statements written the other way round, and `lw ra` sits
   between the two stores in both. sched2 priority tie, not a source shape.
   Also tried: statements reversed (95.42), no tmp[] (96.08), two s32 locals
   for the loads (96.08, and it also drops the 8-byte frame slot). */

/* void func_8007F854(void) {
    s32 tmp[2];

    g_BattleScreenCopyRequest = 1;
    BattleSetScreenMode(1);
    g_BattleCurrentTextList = 0;
    BattleClearTextSpriteEntries();
    D_80098800[0] |= 0xC;
    D_800970E4[0] |= 0xC;
} */

void BattleResetTextListState(void) {
    s32 tmp[2]; // reproduces the original's unused 8-byte stack slot

    BattleSetScreenMode(1);
    g_BattleCurrentTextList = 0;
    BattleClearTextSpriteEntries();
}

void BattleExitOptionsScreen(void) {
    s32 mode;
    s32 current;

    func_80080C48(0);
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

typedef struct {
    /* 0x0 */ u8 unk0[0x12];
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ u8 unk16[0xE];
    /* 0x24 */ s16 unk24;
    /* 0x26 */ u8 unk26[2];
    /* 0x28 */ s16 unk28;
} Misc7Struct; // Size: 0x2A

void BattleSetRowTextColor(Misc7Struct* p, s32 arg) {
    if (p->unk12 == arg) {
        BattleSetTextColor(1);
    } else {
        BattleSetTextColor(0);
    }
}

u32 BattleGetDifficultyText(void) {
    return g_BattleDifficultyNames[g_BattleDifficultyLevel[0]];
}

void BattleDrawOptionRows0To4(Misc7Struct* p) {
    u8 buf[0x8];
    u8* pText;
    s32 value;

    BattleSetTextCursor(p->unk24 + p->unk28 - 0xA, p->unk14);
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

void BattleDrawOptionRows0To2(Misc7Struct* p) {
    u8 buf[0x8];

    BattleSetTextCursor(p->unk24 + p->unk28 - 0xA, p->unk14);
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
    Misc7Struct* pPad;
    s32 flag;

    BattleSetTextCursor(0xA0, 0x8C);
    pPad = (Misc7Struct*)D_800916D8;
    flag = !(g_BattleSlotConfirmFlags & 1);
    if (flag && D_800916EA == 0) {
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
        D_800916DC[0x14] &= ~4;
    } else {
        D_800916DC[0x14] |= 4;
    }
    BattleSetTextColor(0);
    flag = !(g_BattleSlotConfirmFlags & 2);
    if (flag && D_80092754 == 0) {
        flag = 0;
    }
    pPad = (Misc7Struct*)D_80091714;
    if (flag && D_80091726 == 0) {
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
        D_80091718[0x14] &= ~4;
    } else {
        D_80091718[0x14] |= 4;
    }
    BattleSetTextColor(0);
    func_8007F258(D_80092938, 1);
}

void BattleDrawOptionRows4To6(Misc7Struct* p) {
    u8 buf[0x10];
    u8* pText;
    u8 flag;

    BattleSetTextColor(0);
    BattleSetTextCursor(p->unk24 + p->unk28 - 0xA, p->unk14);
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
        status = D_8009274C;
    } else {
        status = D_80092750;
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

    if (D_80091364 == 0) {
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
        } while (g_BattleCursorIndex0 == g_BattleCursorIndex1 && (D_80092748 & 1) == 0 && !same);
        if (D_80091364 == 0) {
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

    if (D_80092750 & 0x40) {
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
        } while (g_BattleCursorIndex0 == g_BattleCursorIndex1 && (D_80092748 & 1) == 0 && !same);
        if (D_80092750 & 0x20) {
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
        for (i = 0; i < 0x31; i++) {
            LoadImage((RECT*)pSprite, (u_long*)(g_BattleSpriteArchiveBuffer + i * 0x1000));
            LoadImage((RECT*)(pSprite + 8), (u_long*)(g_BattleSpriteArchiveBuffer + i * 0x1000 + 0x100));
            pSprite += 0x14;
        }
        HeapFree(g_BattleSpriteArchiveBuffer);
        g_BattleSpriteArchiveFreed = 1;
    }
    g_BattleMode = mode;
    D_800916E2 = (mode == 4) ? 3 : 4;
    D_8009171E = 5;
    if (mode == 3) {
        D_80091369 = 0x27;
        D_80091391 = 0x28;
    } else {
        D_80091369 = 0x25;
        D_80091391 = 0x26;
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
