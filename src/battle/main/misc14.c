#include "common.h"
#include "psyq/libgpu.h"
#include "libgear/main.h"
#include "battle/actor.h"

// Partial view of the per-frame battle work block (g_BattleDisplayState). The shared
// include/battle/battle_work.h owns the type other units read; this view names
// only the sub-image upload state this unit advances each frame.
typedef struct {
    /* 0x000 */ u8 unk0[0x30];
    /* 0x030 */ u32 sourceWordOffset; // word offset into each sub-image source, 4 words per upload
    /* 0x034 */ u8 unk34[0x6C];
    /* 0x0A0 */ u8 unkA0; // func_8008A684 returns unless set
    /* 0x0A1 */ u8 unkA1[0x8];
    /* 0x0A9 */ u8 moveSteps; // position interpolation steps accumulated, 6 per frame
    /* 0x0AA */ u8 frameCount; // incremented every update; the upload runs on odd values
    /* 0x0AB */ u8 sizeSteps; // size increment steps accumulated, 1 per frame
    /* 0x0AC */ u8 unkAC;
    /* 0x0AD */ u8 actorDrawActive; // func_80088B80 returns unless set
    /* 0x0AE */ u8 unkAE[0x1C];
    /* 0x0CA */ u8 slotTimersActive; // set to tick the 0x10 prim pool timers down
    /* 0x0CB */ u8 unkCB[0x4];
    /* 0x0CF */ u8 unkCF;
} BattleUploadState; // Size: 0xD0 (lower bound: other units reach 0xFC)

// Partial view of the battle work block (g_BattleGraphicsWork): the sub-image descriptors
// LoadImage uploads and the scroll BattleUpdateImageUpload animates.
typedef struct {
    /* 0x0000 */ u8 unk0[0x6410];
    /* 0x6410 */ s32 scrollOffset; // sub-image scroll, steps by 4 between 0x7C and 0x81
    /* 0x6414 */ u8 unk6414;
    /* 0x6415 */ u8 scrollActive; // while set, scrollOffset steps every update
    /* 0x6416 */ u8 scrollDir; // 0 counts scrollOffset up to 0x81, 1 counts down
    /* 0x6417 */ u8 unk6417[0x2539];
    /* 0x8950 */ RECT uploadRect[4];
} BattleUploadWork; // Size: 0x8970 (lower bound: other units reach 0xA2C8)

// The prim pool's head is a 0x38-stride table of per-slot records; only the two
// fields this unit touches are named (see BattlePrimPool for the pool tail).
typedef struct {
    /* 0x00 */ u8 unk0[0x26];
    /* 0x26 */ s16 timer; // counts down to 0 while timerActive
    /* 0x28 */ u8 timerActive;
    /* 0x29 */ u8 unk29[0xF];
} BattlePrimPoolSlot; // Size: 0x38

// Partial view of the per-slot battle actor reached through D_800D32F8: only the
// four animation-done flags at 0x15F8 are named here.
typedef struct {
    /* 0x0000 */ u8 unk0[0x15F8];
    /* 0x15F8 */ u8 unk15F8; // set while the slot has two counters to step, not one
    /* 0x15F9 */ u8 unk15F9; // cleared once both of the slot's counters are done
    /* 0x15FA */ u8 unk15FA; // first counter done
    /* 0x15FB */ u8 unk15FB; // second counter done
} BattleActorAnimFlags; // Size: 0x15FC (lower bound, the actor continues past it)

extern u16 g_BattleStatusBitMasks[];
extern u16 g_BattleSlotBitMasks[];
extern u8 g_BattleDisplayMode;
extern u8 D_800CCDC4[];
extern u8 D_800CCDC6[];
extern u8 g_BattleValueDigitSpriteIds[];
extern s32 g_BattleLerpStepsPerFrame;
extern u8 g_BattleLerpDirX;
extern s32 g_BattleLerpStepX;
extern u8 g_BattleLerpDirY;
extern s32 g_BattleLerpStepY;
extern s32 g_BattleLerpFracX;
extern s32 g_BattleLerpFracY;
extern s32 g_BattleLerpStartX;
extern s32 g_BattleLerpTargetX;
extern s32 g_BattleLerpStartY;
extern s32 g_BattleLerpTargetY;
extern u8 g_BattleLerpDone;
extern BattleActorAnimFlags* D_800D32F8[];
extern u8 g_BattleStateWork[];
extern void func_801DF270(void);
extern void func_801DF4C0(void);
extern u8 D_800D2C36;
extern u8 D_800C3CFA[];
extern u8 D_800CDCB8[];
extern u8 D_800CDCBC[];
extern u8 D_800CDCD0[];
extern u8 D_800CDCD4[];
extern u8 D_800D2C38;
extern u16 g_BattleStatusFlags;
extern u16 D_800D2C3A;
extern s32 D_800C2054[];
extern u8 GameRandomByteRange(s32 arg0, u8 arg1);
extern void BattleSetupLinearMove(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 BattleMaskHasStatusBit(s32 mask, u8 idx);
extern u8 g_BattlePrimStartIndex;
extern u8* g_BattleSpriteColumnWork;
extern s32 BattleDrawSpritePackage(s32 arg0, s32 arg1, s16 arg2, s16 arg3);
extern void BattleSetPrimGrey80BlendAdd(u8* p);
extern Temp8* g_BattleSoundBankMain;
extern void SoundPlaySoundEffectOnLastPair(s32 arg0);
extern void BattleDrawSlotSpriteColumns(void);
extern void BattleBuildFlaggedSlotSpriteColumn(void);
extern void BattleBuildBlendedSprite(void);
extern void BattleBuildGrey80BlendSpriteColumnPair(void);
extern void BattleBuildGrey80BlendSpriteColumn5(void);
extern void BattleBuildGrey80BlendSpriteColumn6(void);
extern void BattleBuildGrey80BlendSpriteColumn7(void);
extern void BattleBuildGrey80BlendSpriteColumn2(void);
extern void BattleUpdatePauseMenu(u8 arg0);
extern void BattleUpdateImageUpload(u8 arg0);
extern void func_8008A684(u8 arg0);
extern int rand(void);
extern BattleUploadWork *g_BattleGraphicsWork;
extern BattleUploadState *g_BattleDisplayState;
extern u8 g_BattlePauseActive;
extern BattlePrimPoolSlot *g_BattlePrimPool;
extern u8 D_800C48EA;
extern u8 g_BattleMenuEvent;
extern s32 *g_DebuggerState;
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern int g_SpuChannelMask;
extern int ControllerGetType(int controllerIndex);
extern int ControllerIsStateStackFull(void);
extern void ControllerResetState(void);
extern int ControllerPopState(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
// The two bytes of the menu's event history: `last` is the event the menu had last
// frame (the shared header's g_BattleLastMenuEvent), `previous` the event before it.
typedef struct {
    /* 0x0 */ u8 previous;
    /* 0x1 */ u8 last;
} BattleMenuEventHistory; // Size: 0x2

// Partial view of the command queue header (BattleCommandQueue in src/battle/main/misc4.c):
// only the byte that says a queued command has run is named here.
typedef struct {
    /* 0x000 */ u8 unk0[0x2DB];
    /* 0x2DB */ u8 commandFinished;
} BattleCommandQueueTail; // Size: 0x2DC

extern BattleCommandQueueTail* g_BattleCommandQueue;
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleSlotDisplayValue;
extern BattleMenuEventHistory g_BattleMenuEventHistory;
extern u8 D_8005959C;
extern u8 g_BattleDebugFontLoaded;
extern u8 D_800D32A8;
extern u8 D_800D32B0;
extern void BattlePlaySoundIfEnabled(u8 arg0);
extern void FontSetExternalBuffer(void* pFont);
extern void* FontLoadFont(s32 startX, s32 startY, s32 width, s32 height, s32 maxLetters, u32 flags, s32 texpageX, s32 texpageY, s32 clutX, s32 clutY, void* pCompressedFontFile);
extern u8 D_800C3CF8[];
extern u8 D_800C3CFB[];
extern u8 D_800CCC58;
extern u8 g_BattleCommandCharIndex;
extern u8 D_800D2C35;
extern u8 g_BattleCursorScrollState;
extern s32 g_BattleCursorScrollY;
extern u16 D_800D2C2A;
extern void func_8007171C(void);
extern void BattlePollMenuEvent(s32 arg0);
extern void BattleParseNumberToDigits(s32 arg0);
extern void BattleSetPrimGrey80BlendSub(u8 *p);

void BattleStepTwoAxisInterpolation(void) {
    s32 i = 0;

    for (i = 0; i < g_BattleLerpStepsPerFrame; i++) {
        if (g_BattleLerpDirX != 0) {
            g_BattleLerpFracX -= g_BattleLerpStepX;
        } else {
            g_BattleLerpFracX += g_BattleLerpStepX;
        }
        if (g_BattleLerpDirY != 0) {
            g_BattleLerpFracY -= g_BattleLerpStepY;
        } else {
            g_BattleLerpFracY += g_BattleLerpStepY;
        }
    }
    if (g_BattleLerpStepX == 0x100) {
        if (g_BattleLerpDirX != 0) {
            if (g_BattleLerpFracX / 0x100 + g_BattleLerpStartX < g_BattleLerpTargetX) {
                g_BattleLerpDone = 1;
            }
        } else if (g_BattleLerpTargetX < g_BattleLerpFracX / 0x100 + g_BattleLerpStartX) {
            g_BattleLerpDone = 1;
        }
    } else if (g_BattleLerpDirY != 0) {
        if (g_BattleLerpFracY / 0x100 + g_BattleLerpStartY < g_BattleLerpTargetY) {
            g_BattleLerpDone = 1;
        }
    } else if (g_BattleLerpTargetY < g_BattleLerpFracY / 0x100 + g_BattleLerpStartY) {
        g_BattleLerpDone = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80088B80);
// Best attempt: objdiff 92.61% (retail 1240 bytes). Two classes of diff: the frame
// carries one extra callee-saved register (s6, so ra moves 0x48 -> 0x4C) and,
// downstream of that, the g_BattleSpriteColumnWork base sits in $a0 where retail has $a1 at the
// coordinate stores. The three sprite packages, the four prim loops and the 2x10
// grid already line up. The s16/u16 mix on 0x5D9C/0x5D9E and the word read of
// g_BattlePrimStartIndex inside the grid loop are casts retail really had: the same symbols are
// read the other way round in BattleStepTwoAxisInterpolation.
/*
void func_80088B80(void) {
    s32 i;
    s32 j;
    s32 sx;
    s32 sy;

    if (g_BattleDisplayState->actorDrawActive == 0) {
        return;
    }
    if (g_BattleLerpDone != 0) {
        if ((u8)GameRandomByteRange(0, 0x63) >= 0x60) {
            BattleSetupLinearMove(*(u16*)(g_BattleSpriteColumnWork + 0x5D9C), *(u16*)(g_BattleSpriteColumnWork + 0x5D9E),
                D_800C2054[(u8)GameRandomByteRange(0, 4)], D_800C2054[(u8)GameRandomByteRange(0, 4) + 5]);
        }
    }
    if (g_BattleLerpDone == 0) {
        BattleStepTwoAxisInterpolation();
        *(u16*)(g_BattleSpriteColumnWork + 0x5D9C) = *(u16*)&g_BattleLerpStartX + g_BattleLerpFracX / 0x100;
        *(u16*)(g_BattleSpriteColumnWork + 0x5D9E) = *(u16*)&g_BattleLerpStartY + g_BattleLerpFracY / 0x100;
    }
    g_BattleSpriteColumnWork[0x5D72] = BattleDrawSpritePackage(0xB9, (s32)(g_BattleSpriteColumnWork + 0xD20), *(s16*)(g_BattleSpriteColumnWork + 0x5D9C), *(s16*)(g_BattleSpriteColumnWork + 0x5D9E));
    g_BattleSpriteColumnWork[0x5D94] = g_BattlePrimStartIndex;
    g_BattleSpriteColumnWork[0x5D71] = BattleDrawSpritePackage((*(u16*)(g_BattleSpriteColumnWork + 0x5D9C) & 0xF) + 0xA9, (s32)(g_BattleSpriteColumnWork + 0x3C0), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D93] = g_BattlePrimStartIndex;
    g_BattleSpriteColumnWork[0x5D73] = BattleDrawSpritePackage(0x82, (s32)(g_BattleSpriteColumnWork + 0xFA0), *(s16*)(g_BattleSpriteColumnWork + 0x5D9C), *(s16*)(g_BattleSpriteColumnWork + 0x5D9E));
    g_BattleSpriteColumnWork[0x5D95] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D72]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((i * 2 + g_BattleSpriteColumnWork[0x5D94]) * 0x28 + 0xD20 + (s32)g_BattleSpriteColumnWork));
    }
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D73]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((i * 2 + g_BattleSpriteColumnWork[0x5D95]) * 0x28 + 0xFA0 + (s32)g_BattleSpriteColumnWork));
    }
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D71]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((i * 2 + g_BattleSpriteColumnWork[0x5D93]) * 0x28 + 0x3C0 + (s32)g_BattleSpriteColumnWork));
    }
    g_BattleSpriteColumnWork[0x5D96] = BattleDrawSpritePackage((*(u16*)(g_BattleSpriteColumnWork + 0x5D9E) & 0xF) + 0xC9, (s32)(g_BattleSpriteColumnWork + 0x32A0), *(s16*)(g_BattleSpriteColumnWork + 0x5D9C), *(s16*)(g_BattleSpriteColumnWork + 0x5D9E));
    g_BattleSpriteColumnWork[0x5D97] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D96]; i++) {
        BattleSetPrimGrey80BlendSub((u8*)((i * 2 + g_BattleSpriteColumnWork[0x5D97]) * 0x28 + 0x32A0 + (s32)g_BattleSpriteColumnWork));
    }
    if (g_BattleLerpDone == 0) {
        sy = 0xA << 16;
        for (i = 0; i < 2; i++) {
            sx = 0x82 << 16;
            for (j = 0; j < 0xA; j++) {
                BattleDrawSpritePackage(GameRandomByteRange(0, 9) + 0xBA, (s32)(((i * 0xA + j) * 0x50 + 0x46A0) + (s32)g_BattleSpriteColumnWork), sx >> 16, sy >> 16);
                BattleSetPrimGrey80BlendAdd((u8*)((((i * 0xA + j) * 2 + *(s32*)&g_BattlePrimStartIndex) * 0x28 + 0x46A0) + (s32)g_BattleSpriteColumnWork));
                sx += 0x60000;
            }
            sy += 0xBD << 16;
        }
        g_BattleSpriteColumnWork[0x5D98] = g_BattlePrimStartIndex;
    }
}
*/
void BattleBuildFlaggedSlotSpriteColumn(void) {
    s32 i = 0;
    s32 pitch = 0x6E << 16;
    s32 off;

    g_BattleSpriteColumnWork[0x5DA1] = 0;
    for (i = 0; i < 5; i++) {
        if ((u16)BattleMaskHasStatusBit(g_BattleStatusFlags, i) != 0) {
            off = g_BattleSpriteColumnWork[0x5DA1] * 0x50 + 0x4CE0;
            g_BattleSpriteColumnWork[0x5DA1] += BattleDrawSpritePackage(i + 0xC4, (s32)(off + (s32)g_BattleSpriteColumnWork), 0xE0, pitch >> 16);
            pitch += 0xA0000;
        }
    }
    g_BattleSpriteColumnWork[0x5DA0] = g_BattlePrimStartIndex;
    *(u16*)(g_BattleSpriteColumnWork + 0x5DA2) = 0;
}
void BattleBuildBlendedSprite(void) {
    s32 arg0;
    s32 pEntry;
    s32 i;

    arg0 = 0xA0;
    if (D_800D2C38 != 0) {
        arg0 = 0xA1;
    }
    g_BattleSpriteColumnWork[0x5D75] = BattleDrawSpritePackage(arg0, (s32)(g_BattleSpriteColumnWork + 0x3AC0), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D84] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D75]; i++) {
        pEntry = (i * 2 + g_BattleSpriteColumnWork[0x5D84]) * 0x28 + 0x3AC0;
        BattleSetPrimGrey80BlendAdd((u8*)(pEntry + (s32)g_BattleSpriteColumnWork));
    }
}
void BattleBuildGrey80BlendSpriteColumnPair(void) {
    u8 v;
    u8 slotA;
    u8 slotB;
    s32 i;

    v = g_BattleCommandCharIndex;
    slotA = v - 0x5D;
    slotB = v - 0x25;
    g_BattleSpriteColumnWork[0x5D77] = BattleDrawSpritePackage(slotA, (s32)(g_BattleSpriteColumnWork + 0x3E80), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D86] = g_BattlePrimStartIndex;
    g_BattleSpriteColumnWork[0x5D7F] = BattleDrawSpritePackage(slotB, (s32)(g_BattleSpriteColumnWork + 0x43D0), 0xA0, 0x64);
    g_BattleSpriteColumnWork[0x5D8E] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D77]; i++) {
        BattleSetPrimGrey80BlendAdd(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D86]) * 0x28 + 0x3E80));
    }
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D7F]; i++) {
        BattleSetPrimGrey80BlendSub(g_BattleSpriteColumnWork + ((i * 2 + g_BattleSpriteColumnWork[0x5D8E]) * 0x28 + 0x43D0));
    }
}
void BattleBuildGrey80BlendSpriteColumn5(void) {
    s32 i = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    BattleParseNumberToDigits(D_800D2C2A);
    while (i < 5) {
        s32 slot = D_800C3CF8[i++];
        if (slot != 0xFF) {
            off = g_BattleSpriteColumnWork[0x5D78] * 0x50 + 0x4E70;
            g_BattleSpriteColumnWork[0x5D78] += BattleDrawSpritePackage(slot + 0x92, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0x46);
            pitch += 0x60000;
        }
    }
    g_BattleSpriteColumnWork[0x5D87] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D78]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((off2 = (i * 2 + g_BattleSpriteColumnWork[0x5D87]) * 0x28 + 0x4E70) + (s32)g_BattleSpriteColumnWork));
    }
}
void BattleBuildGrey80BlendSpriteColumn6(void) {
    s32 i;
    s32 n;
    s32 pitch;
    s32 columnBase;
    s32 off;
    s32 off2;

    columnBase = 0x4FB0;
    if (g_BattleCommandCharIndex < 4) {
        BattleParseNumberToDigits(D_800D2C3A);
        i = 0;
        n = 0;
        pitch = 0x11A << 16;
        while (i < 2) {
            s32 slot = D_800C3CFB[i++];
            if (slot != 0xFF) {
                off = g_BattleSpriteColumnWork[0x5D79] * 0x50 + 0x4FB0;
                g_BattleSpriteColumnWork[0x5D79] += BattleDrawSpritePackage(slot + 0x92, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0x4E);
                pitch += 0x60000;
                n++;
            }
        }
        g_BattleSpriteColumnWork[0x5D79] += BattleDrawSpritePackage(0x9D, (s32)((g_BattleSpriteColumnWork[0x5D79] * 0x50 + columnBase) + (s32)g_BattleSpriteColumnWork), 0x11A + n * 6, 0x4E);
    } else {
        g_BattleSpriteColumnWork[0x5D79] = BattleDrawSpritePackage(0xA2, (s32)(g_BattleSpriteColumnWork + 0x4FB0), 0x11A, 0x4E);
    }
    g_BattleSpriteColumnWork[0x5D88] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D79]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((off2 = (i * 2 + g_BattleSpriteColumnWork[0x5D88]) * 0x28 + 0x4FB0) + (s32)g_BattleSpriteColumnWork));
    }
}
void BattleBuildGrey80BlendSpriteColumn7(void) {
    s32 i = 0;
    s32 columnBase;
    s32 n = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    columnBase = 0x50A0;
    BattleParseNumberToDigits(D_800D2C36);
    for (i = 0; i < 3; i++) {
        if (D_800C3CFA[i] != 0xFF) {
            off = g_BattleSpriteColumnWork[0x5D7A] * 0x50 + 0x50A0;
            g_BattleSpriteColumnWork[0x5D7A] += BattleDrawSpritePackage(D_800C3CFA[i] + 0x92, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0x56);
            pitch += 0x60000;
            n++;
        }
    }
    g_BattleSpriteColumnWork[0x5D7A] += BattleDrawSpritePackage(0x9D, (s32)((g_BattleSpriteColumnWork[0x5D7A] * 0x50 + columnBase) + (s32)g_BattleSpriteColumnWork), 0x11A + n * 6, 0x56);
    g_BattleSpriteColumnWork[0x5D89] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D7A]; i++) {
        off2 = (i * 2 + g_BattleSpriteColumnWork[0x5D89]) * 0x28 + 0x50A0;
        BattleSetPrimGrey80BlendAdd(&g_BattleSpriteColumnWork[off2]);
    }
}

void BattleBuildGrey80BlendSpriteColumn2(void) {
    s32 i = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    BattleParseNumberToDigits(D_800D2C35);
    while (i < 2) {
        s32 slot = D_800C3CFB[i++];
        if (slot != 0xFF) {
            off = g_BattleSpriteColumnWork[0x5D7B] * 0x50 + 0x51E0;
            g_BattleSpriteColumnWork[0x5D7B] += BattleDrawSpritePackage(slot + 0x92, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0x5E);
            pitch += 0x60000;
        }
    }
    g_BattleSpriteColumnWork[0x5D8A] = g_BattlePrimStartIndex;
    for (i = 0; i < g_BattleSpriteColumnWork[0x5D7B]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((off2 = (i * 2 + g_BattleSpriteColumnWork[0x5D8A]) * 0x28 + 0x51E0) + (s32)g_BattleSpriteColumnWork));
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800898F0);
// Best attempt: objdiff 91.38% (528 bytes, the original size). Only two classes of diff,
// both shared with the rest of the BattleDrawSpritePackage family in this unit: every call site puts
// the g_BattleSpriteColumnWork base in $a1 and the offset chain in $v0 where the original has base $v1 /
// chain $a1 (the final addu is the mirror image), and the second loop's "pitch = 0x48 << 16"
// plus its 0x80000 increment are scheduled a few instructions differently. Pitch values are
// 0x20<<16 / 0x48<<16 with a +0x80000 step, the step is OUTSIDE the if, and the table
// lookup index is arg0 * 0x170 into a u8 table at D_800CCDC4 / D_800CCDC6.
/*
void func_800898F0(u8 arg0) {
    s32 i = 0;
    s32 pitch = 0x20 << 16;
    s32 off;

    g_BattleSpriteColumnWork[0x5D7C] = 0;
    BattleParseNumberToDigits(*(u16*)(D_800CCDC4 + arg0 * 0x170));
    for (i = 0; i < 4; i++) {
        if (g_BattleValueDigitSpriteIds[i] != 0xFF) {
            off = g_BattleSpriteColumnWork[0x5D7C] * 0x50 + 0x5280;
            g_BattleSpriteColumnWork[0x5D7C] += BattleDrawSpritePackage(g_BattleValueDigitSpriteIds[i] + 0x92, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0xCC);
        }
        pitch += 0x80000;
    }
    g_BattleSpriteColumnWork[0x5D8B] = g_BattlePrimStartIndex;
    g_BattleSpriteColumnWork[0x5D7D] = 0;
    BattleParseNumberToDigits(*(u16*)(D_800CCDC6 + arg0 * 0x170));
    pitch = 0x48 << 16;
    for (i = 0; i < 4; i++) {
        if (g_BattleValueDigitSpriteIds[i] != 0xFF) {
            off = g_BattleSpriteColumnWork[0x5D7D] * 0x50 + 0x53C0;
            g_BattleSpriteColumnWork[0x5D7D] += BattleDrawSpritePackage(g_BattleValueDigitSpriteIds[i] + 0x92, (s32)(off + (s32)g_BattleSpriteColumnWork), pitch >> 16, 0xCC);
        }
        pitch += 0x80000;
    }
    off = g_BattleSpriteColumnWork[0x5D7D] * 0x50 + 0x53C0;
    g_BattleSpriteColumnWork[0x5D7D] += BattleDrawSpritePackage(0x9C, (s32)(off + (s32)g_BattleSpriteColumnWork), 0x41, 0xCC);
    g_BattleSpriteColumnWork[0x5D8C] = g_BattlePrimStartIndex;
}
*/
void BattleBuildSpritePrimLists(void) {
    BattleDrawSlotSpriteColumns();
    BattleBuildFlaggedSlotSpriteColumn();
    BattleBuildBlendedSprite();
    BattleBuildGrey80BlendSpriteColumnPair();
    BattleBuildGrey80BlendSpriteColumn5();
    BattleBuildGrey80BlendSpriteColumn6();
    BattleBuildGrey80BlendSpriteColumn7();
    BattleBuildGrey80BlendSpriteColumn2();
}
u16 BattleRandomRange(u16 arg0, u16 arg1) {
    s32 rem;
    s32 range;

    if (arg0 == 0xFFFF) {
        return 0xFFFF;
    }
    if (arg1 == 0) {
        return 0;
    }
    if (arg0 == arg1) {
        return arg0;
    }
    range = arg1 - arg0;
    if (range > 0xFFFE) {
        return rand();
    }
    rem = (u16)rand() % (range + 1);
    return (u16)(arg0 + rem);
}
u16 BattleStatusBitMask(u8 idx) {
    return g_BattleStatusBitMasks[idx];
}
u16 BattleSlotBitMask(u8 idx) {
    return g_BattleSlotBitMasks[idx];
}
u16 BattleStatusBitMaskInverted(u8 idx) {
    return ~g_BattleStatusBitMasks[idx];
}
u16 BattleSlotBitMaskInverted(u8 idx) {
    return ~g_BattleSlotBitMasks[idx];
}
s32 BattleMaskHasStatusBit(s32 mask, u8 idx) {
    s32 flags;

    if (idx < 0x10) {
        flags = g_BattleStatusBitMasks[idx] & mask;
    } else {
        flags = 0;
    }
    return flags;
}
s32 BattleMaskHasSlotBit(s32 mask, u8 idx) {
    s32 flags;

    if (idx < 0x10) {
        flags = g_BattleSlotBitMasks[idx] & mask;
    } else {
        flags = 0;
    }
    return flags;
}
void BattlePollMenuEvent(s32 arg0) {
    s32 channelMask;
    BattleMenuEventHistory* pEvent;
    u8* pMenuExit;
    u8* pSlotState;
    u8 event;
    u8 waiting;
    u8 muted;
    u8 prevEvent;

    event = 8;
    waiting = 1;
    muted = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (muted == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                muted++;
                channelMask = g_SpuChannelMask;
            }
        } else {
            if (muted != 0) {
                SoundEnableAllSpuChannels();
                g_SpuChannelMask = channelMask;
            }
            waiting = 0;
        }
    } while (waiting != 0);

    pEvent = &g_BattleMenuEventHistory;
    pMenuExit = &D_800C48EA;
    pSlotState = &g_BattleWorkBlock[0].state.commandCounter;
    do {
        if (ControllerIsStateStackFull() != 0) {
            ControllerResetState();
        } else {
            while (ControllerPopState() != 0) {
                if (*pMenuExit != 0 || g_BattleCommandQueue->commandFinished != 0) {
                    event = 0xFF;
                    break;
                }
                if (g_BattlePauseActive != 0) {
                    if (*g_DebuggerState != -1) {
                        if ((g_C1ButtonStatePressedOnce & 4) != 0) {
                            if ((g_C1ButtonStatePressedOnce & 8) != 0) {
                                *pMenuExit = 1;
                                SoundEnableAllSpuChannels();
                                g_SpuChannelMask = channelMask;
                                g_BattlePauseActive = 0;
                                break;
                            }
                        }
                    }
                } else if ((g_C1ButtonStatePressedOnce & 0x2000) != 0) {
                    BattlePlaySoundIfEnabled(0x4C);
                    prevEvent = pEvent->last;
                    event = 0;
                    pEvent->last = event;
                    pEvent->previous = prevEvent;
                    break;
                } else if ((g_C1ButtonStatePressedOnce & 0x4000) != 0) {
                    BattlePlaySoundIfEnabled(0x4C);
                    prevEvent = pEvent->last;
                    event = 1;
                    pEvent->last = event;
                    pEvent->previous = prevEvent;
                    break;
                } else if ((g_C1ButtonStatePressedOnce & 0x8000) != 0) {
                    BattlePlaySoundIfEnabled(0x4C);
                    prevEvent = pEvent->last;
                    event = 2;
                    pEvent->last = event;
                    pEvent->previous = prevEvent;
                    break;
                } else if ((g_C1ButtonStatePressedOnce & 0x1000) != 0) {
                    BattlePlaySoundIfEnabled(0x4C);
                    prevEvent = pEvent->last;
                    event = 3;
                    pEvent->last = event;
                    pEvent->previous = prevEvent;
                    break;
                } else if ((g_C1ButtonStateReleased & 0x20) != 0) {
                    event = 4;
                    BattlePlaySoundIfEnabled(0x4D);
                    break;
                } else if ((g_C1ButtonStateReleased & 0x40) != 0) {
                    event = 5;
                    BattlePlaySoundIfEnabled(0x4E);
                    break;
                } else if ((g_C1ButtonStateReleased & 0x80) != 0) {
                    event = 6;
                    BattlePlaySoundIfEnabled(0x4D);
                    break;
                } else if ((g_C1ButtonStateReleased & 0x10) != 0) {
                    event = 7;
                    BattlePlaySoundIfEnabled(0x4D);
                    break;
                } else if ((g_C1ButtonStateReleased & 1) != 0) {
                    event = 0xC;
                    if (*g_DebuggerState != -1) {
                        g_BattleSlotDisplayValue = 0x1C;
                        D_800D32A8 = 0x1C;
                        D_800D32B0 = 0x1C;
                        /* the three per-slot records run 0x170 apart, the stride of
                           g_BattleWorkBlock's slots (BattleSetSlotFields148And162 in
                           src/battle/main/misc19.c writes these same two bytes) */
                        pSlotState[0] = 4;
                        pSlotState[0x170] = 4;
                        pSlotState[0x2E0] = 4;
                        pSlotState[0x1A] = 0xFF;
                        pSlotState[0x18A] = 0xFF;
                        pSlotState[0x2FA] = 0xFF;
                    }
                    break;
                } else if ((g_C1ButtonStateReleased & 2) != 0) {
                    if (*g_DebuggerState != -1) {
                        if (++D_8005959C >= 5) {
                            D_8005959C = 0;
                        }
                        event = 0xB;
                        if (g_BattleDebugFontLoaded == 0) {
                            FontSetExternalBuffer((void*)0x80200000);
                            FontLoadFont(0x10, 0x10, 0x140, 0x100, 0x3E8, 0, 0x340, 0, 0x340, 0x20, 0);
                            g_BattleDebugFontLoaded += 1;
                        }
                    }
                    break;
                } else if ((g_C1ButtonStateReleased & 0x100) != 0) {
                    event = 0xD;
                    break;
                }
                if ((g_C1ButtonStateReleased & 0x800) == 0) {
                    continue;
                }
                event = 0xE;
                if (D_800CCC58 != 0) {
                    if (g_BattlePauseActive == 0) {
                        GraphicsDrawPauseLetters(0x88, 0x64);
                        GraphicsDrawPauseLetters(0x88, 0x144);
                        SoundMuteAllSpuChannels();
                        channelMask = g_SpuChannelMask;
                        g_BattlePauseActive = 1;
                    } else {
                        SoundEnableAllSpuChannels();
                        g_SpuChannelMask = channelMask;
                        g_BattlePauseActive = 0;
                    }
                }
                break;
            }
        }
    } while (g_BattlePauseActive != 0);
    g_BattleMenuEvent = event;
}

void BattleUploadImageFrame(void) {
    /* declared out of call order: gcc's register tie for the LoadImage arguments depends on it */
    s32 vramOffset3;
    s32 vramOffset0;
    s32 vramOffset1;
    s32 vramOffset2;
    g_BattleDisplayState->frameCount += 1;
    if ((g_BattleDisplayState->frameCount & 1) == 0) {
        return;
    }
    vramOffset0 = 0x8970;
    LoadImage(&g_BattleGraphicsWork->uploadRect[0], (u_long*)(g_BattleDisplayState->sourceWordOffset * 4 + vramOffset0 + (s32)g_BattleGraphicsWork));
    vramOffset1 = 0x8FA0;
    LoadImage(&g_BattleGraphicsWork->uploadRect[1], (u_long*)(g_BattleDisplayState->sourceWordOffset * 4 + vramOffset1 + (s32)g_BattleGraphicsWork));
    vramOffset2 = 0x95D0;
    LoadImage(&g_BattleGraphicsWork->uploadRect[2], (u_long*)(g_BattleDisplayState->sourceWordOffset * 4 + vramOffset2 + (s32)g_BattleGraphicsWork));
    vramOffset3 = 0x9C00;
    LoadImage(&g_BattleGraphicsWork->uploadRect[3], (u_long*)(g_BattleDisplayState->sourceWordOffset * 4 + vramOffset3 + (s32)g_BattleGraphicsWork));
    g_BattleDisplayState->sourceWordOffset += 4;
    if (g_BattleDisplayState->sourceWordOffset >= 0xC7) {
        g_BattleDisplayState->sourceWordOffset = 0;
    }
}
void BattleUpdateImageUpload(u8 arg0) {
    if (D_800CCC58 != 0) {
        func_8007171C();
    }
    if (arg0 == 0) {
        BattlePollMenuEvent(0);
    }
    g_BattleDisplayState->moveSteps += 6;
    g_BattleDisplayState->sizeSteps += 1;
    if (g_BattleGraphicsWork->scrollActive != 0) {
        if (g_BattleGraphicsWork->scrollDir == 0) {
            g_BattleGraphicsWork->scrollOffset += 4;
            if (g_BattleGraphicsWork->scrollOffset >= 0x81) {
                g_BattleGraphicsWork->scrollOffset = 0x7C;
                g_BattleGraphicsWork->scrollDir = 1;
            }
        } else {
            g_BattleGraphicsWork->scrollOffset -= 4;
            if (g_BattleGraphicsWork->scrollOffset < 0) {
                g_BattleGraphicsWork->scrollOffset = 4;
                g_BattleGraphicsWork->scrollDir = 0;
            }
        }
    }
    switch (g_BattleCursorScrollState) {
    case 1:
    case 3:
        g_BattleCursorScrollY += 1;
        break;
    case 2:
    case 4:
        g_BattleCursorScrollY -= 1;
        break;
    }
    BattleUploadImageFrame();
}

void BattleUpdatePauseMenu(u8 arg0) {
    s32 i;
    s32 channelMask;
    u8 muted;
    u8 waiting;

    waiting = 1;
    muted = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (muted == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                muted++;
                channelMask = g_SpuChannelMask;
            }
        } else {
            if (muted != 0) {
                SoundEnableAllSpuChannels();
                g_SpuChannelMask = channelMask;
            }
            waiting = 0;
        }
    } while (waiting != 0);

    BattleUploadImageFrame();
    if (g_BattleDisplayState->slotTimersActive != 0) {
        for (i = 0; i < 0x10; i++) {
            if (g_BattlePrimPool[i].timerActive != 0) {
                if (--g_BattlePrimPool[i].timer < 0) {
                    g_BattlePrimPool[i].timer = 0;
                }
            }
        }
    }

    muted = 1;
    do {
        if (g_BattleDisplayState->unkCF == 0) {
            g_BattleMenuEvent = 0xFF;
        }
        if (ControllerIsStateStackFull() != 0) {
            ControllerResetState();
        } else {
            while (ControllerPopState() != 0) {
                if (g_BattlePauseActive != 0) {
                    if (*g_DebuggerState != -1) {
                        if ((g_C1ButtonStatePressedOnce & 4) != 0) {
                            if ((g_C1ButtonStatePressedOnce & 8) != 0) {
                                D_800C48EA = muted;
                                SoundEnableAllSpuChannels();
                                g_SpuChannelMask = channelMask;
                                g_BattlePauseActive = 0;
                                break;
                            }
                        }
                    }
                } else if ((g_C1ButtonStateReleased & 0x20) != 0) {
                    g_BattleMenuEvent = 4;
                    break;
                }
                if ((g_C1ButtonStateReleased & 0x800) == 0) {
                    continue;
                }
                if (D_800CCC58 == 0) {
                    break;
                }
                if (g_BattlePauseActive == 0) {
                    SoundMuteAllSpuChannels();
                    GraphicsDrawPauseLetters(0x88, 0x64);
                    GraphicsDrawPauseLetters(0x88, 0x144);
                    channelMask = g_SpuChannelMask;
                    g_BattlePauseActive = muted;
                } else {
                    SoundEnableAllSpuChannels();
                    g_SpuChannelMask = channelMask;
                    g_BattlePauseActive = 0;
                }
                break;
            }
        }
    } while (g_BattlePauseActive != 0);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A684);
// Best attempt: objdiff 99.03%, 828 bytes = the original size, ONE instruction out of
// place. Everything matches except the position of `move a0,zero` (offset = 0): retail
// emits it inside the loop setup, between the D_800D32F8 base (lui/addiu a2) and
// `addiu t2,a2,12`, while a source-level `offset = 0;` always lands before both. That
// is the signature of a0 being a loop.c biv initialised with the other bivs, i.e. the
// source wrote the byte offset as an `i * 8`-shaped expression; every spelling of that
// (D_X[i * 2] on s32 arrays, *(s32 *)((s32)D_X + i * 8), offset = i * 8; in the body)
// makes gcc 2.6.0 strength-reduce one biv per D_ symbol instead of sharing one register,
// which costs 12-16 bytes. Measured, do not redo: done as a u8 local (not the literal 1,
// +0.5), active as u8 (not s32, +3.5, found by the permuter), pCounters = (u8 *)(offset +
// (s32)pWork) with the int arithmetic so the addu comes out index-first as retail has it,
// and the counter stores written through pCounters + 0x9E20/0x9E08/0x9E24/0x9E0C (a
// g_BattleStateWork base with a large constant is what produces retail's lui $at,1 /
// addu / sw -0x61E0 split; through the D_ symbol the store is a bare sw macro). Two
// 4-minute permuter runs closed the two type and width differences, then found nothing.
/*
void func_8008A684(u8 arg0) {
    u8* pWork;
    u8* pCounters;
    BattleActorAnimFlags** pSlot;
    s32 offset;
    s32 i;
    u8 active;
    u8 done;
    u8 waiting;
    u8 muted;
    s32 channelMask;

    active = 1;
    waiting = 1;
    muted = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (muted == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                muted++;
                channelMask = g_SpuChannelMask;
            }
        } else {
            if (muted != 0) {
                SoundEnableAllSpuChannels();
                g_SpuChannelMask = channelMask;
            }
            waiting = 0;
        }
    } while (waiting != 0);

    if (g_BattleDisplayState->unkCF == 0) {
        g_BattleMenuEvent = 0xFF;
    }
    do {
        if (ControllerIsStateStackFull() != 0) {
            ControllerResetState();
        } else {
            while (ControllerPopState() != 0) {
                if ((g_C1ButtonStateReleased & 0x20) != 0) {
                    g_BattleMenuEvent = 4;
                    break;
                }
                if ((g_C1ButtonStateReleased & 0x800) == 0) {
                    continue;
                }
                if (g_BattlePauseActive == 0) {
                    SoundMuteAllSpuChannels();
                    GraphicsDrawPauseLetters(0x88, 0x64);
                    GraphicsDrawPauseLetters(0x88, 0x144);
                    channelMask = g_SpuChannelMask;
                    g_BattlePauseActive = 1;
                } else {
                    SoundEnableAllSpuChannels();
                    g_SpuChannelMask = channelMask;
                    g_BattlePauseActive = 0;
                }
                break;
            }
        }
    } while (g_BattlePauseActive != 0);

    if (g_BattleDisplayState->unkA0 == 0) {
        return;
    }
    if (D_800D32F8[0]->unk15F9 == 0) {
        return;
    }
    done = 1;
    pWork = g_BattleStateWork;
    offset = 0;
    for (i = 0; i < 3; i++) {
        if (D_800D32F8[i]->unk15FA == 0) {
            if (*(s32*)(D_800CDCD0 + offset) == 0) {
                D_800D32F8[i]->unk15FA = done;
            } else {
                pCounters = (u8 *)(offset + (s32)pWork);
                *(s32*)(pCounters + 0x9E20) = *(s32*)(D_800CDCD0 + offset) - 1;
                *(s32*)(pCounters + 0x9E08) = *(s32*)(D_800CDCB8 + offset) + 1;
            }
        }
        if (D_800D32F8[i]->unk15FB == 0) {
            if (*(s32*)(D_800CDCD4 + offset) == 0) {
                D_800D32F8[i]->unk15FB = done;
            } else {
                pCounters = (u8 *)(offset + (s32)pWork);
                *(s32*)(pCounters + 0x9E24) = *(s32*)(D_800CDCD4 + offset) - 1;
                *(s32*)(pCounters + 0x9E0C) = *(s32*)(D_800CDCBC + offset) + 1;
            }
        }
        active &= D_800D32F8[i]->unk15FA;
        if (D_800D32F8[0]->unk15F8 != 0) {
            active &= D_800D32F8[i]->unk15FB;
        }
        offset += 8;
    }
    if (active == 0) {
        func_801DF270();
        func_801DF4C0();
    } else {
        D_800D32F8[0]->unk15F9 = 0;
    }
}
*/
void BattleUpdateEffectByScreenMode(u8 arg0) {
    switch (g_BattleDisplayMode) {
    case 0:
        func_8008A684(arg0);
        break;
    case 1:
        BattleUpdateImageUpload(arg0);
        break;
    case 2:
        BattleUpdatePauseMenu(arg0);
        break;
    }
}
void BattlePlaySoundWithBaseId(u8 arg0) {
    SoundPlaySoundEffectOnLastPair((g_BattleSoundBankMain->unk14 << 16) | arg0);
}
