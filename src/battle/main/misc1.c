#include "common.h"
#include "psyq/libgpu.h"
#include "battle/actor.h"
#include "battle/battle_work.h"

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1[3];
} BattleStatusEntry; // Size: 0x4

// One 0x170-byte row of the per-slot tables (BattleTableEntry in
// include/battle/actor.h); only the words src/battle/main/misc1 reads are named.
typedef struct {
    /* 0x000 */ u16 flags; // 0xC000 = the slot's action is nullified, 0x8000 selects which half
    /* 0x002 */ u16 unk02;
    /* 0x004 */ u16 unk04; // 0x1000: the slot has a pending slot-action
    /* 0x006 */ u8 unk06[2];
    /* 0x008 */ u16 unk08; // the 0x8000 bit of this pair with unk0A doubles the countdown step
    /* 0x00A */ u16 unk0A;
    /* 0x00C */ u8 unk0C[0x7C];
    /* 0x088 */ u32 effectFlags4Pair; // activeEffectFlags4 and accessoryEffectFlags4 as one word
    /* 0x08C */ u8 unk8C[0x18];
    /* 0x0A4 */ u16 stateFlags; // the first word of the row's BattleSlotState, 0xC000 as above
    /* 0x0A6 */ u8 unkA6[0x12];
    // The hp pair in 1/256ths that the display value ratios are taken of, as in
    // misc8.c's BattleSlotEntry.
    /* 0x0B8 */ s32 scaledHp;
    /* 0x0BC */ s32 scaledMaxHp;
    /* 0x0C0 */ u8 unkC0[0x20];
    /* 0x0E0 */ u8 unkE0; // counted down while flags has 0x2000; the flag clears when it reaches 0
    /* 0x0E1 */ u8 unkE1[0x8F];
} BattleSlotRow; // Size: 0x170

// Digit sprite ids are indices into the glyph set that starts at this id.
#define BATTLE_DIGIT_SPRITE_BASE 0x67

// Width of one battle text slot's string buffer (0x39 in misc3.c and misc17.c too).
#define BATTLE_TEXT_SLOT_BUFFER_SIZE 0x39

// Frames BattleShowWaitSkipText keeps drawing after it puts the skip text up.
#define BATTLE_SKIP_WAIT_FRAMES 3

// The command queue block g_BattleCommandQueue points at (the 0x2F8 bytes
// BattleRunScene allocates for it), as in misc4.c's view of the same block.
typedef struct {
    /* 0x000 */ u8 unk000[0x2D3];
    /* 0x2D3 */ u8 actorIndex; // the slot the queued command is being built for
    /* 0x2D4 */ u8 unk2D4[6];
    /* 0x2DA */ u8 count; // queued commands
    /* 0x2DB */ u8 commandFinished; // 0 while a queued command runs
    /* 0x2DC */ u8 unk2DC[0xE];
    /* 0x2EA */ u8 unk2EA; // while set, func_80071B94 counts the battle clock up when it starts the slot's actor
    /* 0x2EB */ u8 charRedrawFlags[3]; // per party slot: copied into BattleWork.slotMarks
} BattleCommandQueueState; // Size: 0x2F8

// One 0x48-byte row of the command record table at g_BattleCommandRecords
// (0x800C4000), as in misc13.c's view of it.
typedef struct {
    /* 0x00 */ u8 unk00[0x31];
    /* 0x31 */ u8 command; // BATTLE_COMMAND_NONE when the row holds no command
    /* 0x32 */ u8 unk32[0x16];
} BattleCommandRecord; // Size: 0x48

extern u8 g_BattleOverlayEnabled;
extern u8 D_800C48EA;
// The same byte again as an array: its decay is a real address value, so cc1 keeps
// one `la` for the mode test and the write-back instead of a symbol access each.
extern u8 D_800C48EA_bytes[] __asm__("D_800C48EA");
extern s32 *g_BattleAudioManager;
extern BattleCommandQueueState *g_BattleCommandQueue;
extern BattleWork *g_BattleDisplayState;
extern u8 g_BattleTextSlotVisible[];
extern BattleCommandRecord g_BattleCommandRecords[];
extern s32 g_BattleOverlayHeapBlock1;
extern s32 g_BattleOverlayHeapBlock2;
extern s32 *g_DebuggerState;
extern u8 *g_BattleCurOT[];
extern u8 *g_BattleGraphicsWork;

extern void BattleSetArchive20Entry0(void);
extern s32 BattleAllocHeapBlock(s32, s32);
extern void BattleWaitForArchiveSync(void);
extern void func_801E5160(void);
extern s32 func_801E563C(void);
extern void func_801E879C(s32);
extern void func_8028022C(void);
extern void func_800BE790(void);
extern void SoundSetAudioManagerVolumeWithFade(s32 *, s32, s32);
extern void ArchiveReadFileToBuffer(s32, void *, u32, u32);
extern u8 g_BattleWaitSkipFlag;
extern u16 g_BattleActionSlotMask;
extern u16 g_BattleSlotFlag8000Mask;
extern u8 *g_BattleSkipStringBank;
// D_800D39C0 and D_800D39C6 are row 7 of the screen slot table at
// g_BattleTextSlotRects (0x800D39B8 - 0x800D3718 = 7 * 0x60): its pBuf at +0x8,
// with the VRAM rect 8 bytes ahead of it, and its textWidth at +0xE. Spelled
// through the row's fields because g_BattleTextSlotRects[7] puts the same
// instructions on a folded g_BattleTextSlotRects+0x2A0 relocation.
extern u_long *D_800D39C0[];
extern u8 D_800D39C6;
extern u8 *GetStringEntry(u8 *, s32);
extern u8 g_BattleMenuEvent;
extern u8 g_BattleActorCountdownState[];
extern u8 g_BattlePendingCharIndex;
extern u8 g_BattleTargetScan[];
extern u8 D_800D2DD8[];
extern u8 D_800D2DE3[];
extern u16 D_800D2E04[];
extern u16 g_BattleSlotDisplayValues[];
extern u16 g_BattleSlotCountdownTimers[];
extern u16 g_BattleCommandCellFlags;
extern u16 BattleMaskHasSlotBit(u16, u8);
extern u16 BattleSlotBitMask(u8);
extern u16 BattleSlotBitMaskInverted(u8);
extern u8 D_800C3EB7[][BATTLE_ACTOR_SLOT_STRIDE];
extern u8 g_BattleSlotReleasedFlags[][BATTLE_ACTOR_SLOT_STRIDE];
extern BattleStatusEntry D_800C3D1B[];
extern u16 g_BattleActorSlotDoneMask;
extern u16 g_BattlePendingSlotMask;
extern u8 g_BattleCommandSlotIds[];
extern u8 g_BattleCommandArgs;
extern u8 g_BattleCommandTiles;
extern void BattleResetActorStatDisplay(u8);
extern void func_80071B94(u8);
extern u16 BattleSelectActionRow(s32, s32);
extern void BattleSetSlotFlagB4(s32);
extern void BattleClearSlotFlagB4(s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern void BattleLoadImageAndSync(RECT *, u_long *);
extern void BattleClearSlotCurveState(void);
extern void BattleDecrementSlotConditionTimers(u8);
extern void BattleScriptResetActorState(u16);
extern void BattleApplySlotMaskToEffects(u16);
extern void BattleRunActiveSlotScripts(void);
extern void BattleResetActorCountdown(void);
extern void func_80079778(void);
extern void BattleShowWaitSkipText(void);
extern void BattleApplyCommandCellFlags(void);
extern void func_80080160(u8);
extern void func_80080C94(u8);
extern void func_800BFE48(void);
extern u8 BattleBuildTargetCandidateListByRow(u8);
extern s32 BattleActorPosXGreaterThan(u8, u8);
extern s32 BattleFindMinValueOtherSlot(u8);
extern void BattleLoadActorSlotAnimation(s32);
extern void BattleBeginActorStatAnimation(u8);
extern u8 g_BattleTextSlotCount;
extern u8 g_BattleSpriteColorTaskSlot;
extern u8 g_BattleTextSlotFlags; // row 0's text width, +0xE of g_BattleTextSlotRects
// The screen slot table at g_BattleTextSlotRects as misc3.c sees it: the
// buffer at +0x8 (its rect is the 0x18 bytes ahead of the pointer) and the
// width that came out of the render at +0xE.
extern u8 g_BattleTextSlotBuffers[];
extern u8 *g_BattleTextStringBank;
extern BattleTableEntry D_800CCD1C[];
extern u8 D_800C3E3D[];
extern u8 D_800C3D1A[];
extern u8 D_800C3EBA[];
extern u8 D_800D2CA4[];
extern u16 g_BattleClock;
extern u8 D_800D36C0;
extern s32 g_BattlePrimStartIndex;
extern u8 g_BattleStatDigitSpriteIds[];
extern u8 g_BattleSlotDigitSpriteIds[];
// The scene resource block pointer's word is followed by the scene's three
// optional digit sprite ids (0xFF when unused).
extern u8 g_BattleSceneResources[];
extern u16 g_BattleSlotDigitXOffsets[];
extern u8 g_BattleSlotPageIndex;
extern u16 g_BattleStatDigitXOffsets[];
extern u16 g_BattleSlotScreenOrigins[];
extern s32 BattleDrawSpritePackage(s32, s32, s16, s16);
extern s32 BattleDrawSpritePackageHalfScale(s32, s32, s16, s16);
extern void BattleSetPrimGrey40BlendSub(POLY_FT4 *);
extern void *g_BattleSpriteArchive;
extern u8 g_BattleSlotFlags[];
extern u8 g_BattleSlotActorIds[];
extern s32 func_80025FA8(void *pDir, s32 id, u8 *pPrim, s32 context, u16 x, u16 y, u16 u, u16 v, u16 w);
extern void BattleInitPanelMove(s32);
extern void func_80076710(s32);
extern void func_80072938(POLY_FT4 *, s32, s32, s32);
extern u8 g_BattleCountdownsEnabled;
extern u8 g_BattleSlotEnabledFlags[];
extern BattleSlotRow g_BattleSlotEntries[];
extern BattleSlotRow g_BattleSlotEntriesBase[];
extern BattleSlotRow D_800CCE08[];
extern u16 D_800D2D38; // hp display value, split into digits by func_80072F38
extern u16 D_800D2E58; // the same hp, kept for the gauge
extern u16 D_800D2FE0; // max hp display value, split into digits by func_80072F38
extern u16 D_800D3330; // the same max hp
extern s32 D_800D3018; // scaled hp display value, split into digits by func_80072F38
extern s32 D_800D333C; // the same scaled hp
extern s32 D_800D3668; // scaled max hp, only stored
extern u16 D_800CCD6C[];
extern u16 D_800CCD6E[];
extern u8 D_800CCE44[];
extern u16 D_800CCD68[];
extern u16 g_BattleSlotCountdownToggles[];
extern u8 g_BattleValueDigitSpriteIds[];
extern u8 g_BattleSlotDisplayValue[];
extern BattleTableEntry D_800CCDC4[]; // per-slot table family, stride 0x170
extern void BattleParseNumberToDigits(s32);

// The battle teardown reads the sub-system globals below; none of them has a
// header of its own.
extern s32 *g_SoundAudioManager;
extern s32 *g_SoundSedsBank;
extern s32 *g_SoundStreamReserveStart;
extern s32 *g_SoundStreamReserveBlock;
extern s32 *g_SystemStreamedFileData;
extern u8 *g_BattlePrimPool;
extern u8 g_SoundBankLoadBuffer[];
extern u8 g_GameResumeScene;
extern u8 g_GameSelectedSkinIndex;
extern u8 g_GameSelectedSkinLoadPending;
extern u8 g_GameDiscChangeRequest;
extern u8 g_SoundEffectBankIndex;
extern u8 g_BattleActorCharIds[];
extern volatile u16 g_BattleStateFlags;
extern u8 g_BattleLastMenuEvent;
extern u8 g_BattleSoundEnabled;
extern u8 g_BattleDisplayMode;
extern s32 *g_BattleSlotParamTable;
extern s32 *g_BattleStateWork;
extern u8 g_BattleResumeAudioFadePending;
extern u8 g_DebugPageIndex;
extern u8 D_800C3D44;
extern u8 g_BattleDiscChangePending;
extern s32 g_BattleStageDisplayId;
extern u8 g_BattleMenuEventHistory;
extern u8 g_BattleSceneRunning;
extern s32 g_BattleNextModeHeapBlock1;
extern u8 D_800D2D50;
extern s32 g_BattleNextModeHeapBlock2;
extern u8 D_800D2FC4;
extern u8 g_BattleSelectedSkinRecord[];
extern u8 D_8006F9DD;
extern u8 D_800658DC[];

// What the battle reset copies a character's two limit words into: one 4-byte
// pair per party slot.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
} BattleSlotLimits; // Size: 0x4

extern BattleSlotLimits g_BattlePartyConditionMasks[3];

extern void *bzero(u8 *, s32);
extern void *memmove(u8 *, u8 *, s32);
extern void ArchiveSetIndex(s32, s32);
extern void ArchiveCdDataSync(s32);
extern void HeapFree(void *);
extern void func_801E0A34(void);
extern void BattleLoadStage(s32);
extern void BattleInitStageDisplay(s32);
extern void BattleStartScreenFade(s32, u8, u8, u8, u8);
extern s32 *SoundCreateAndPlayAudioManager(u8 *, s32, s32);
extern void func_80077990(void);
extern void BattleBeginAnimTask(u16);
extern void BattleInitTextSlotGraphics(void);
extern void SoundStopElementsForSoundFile(s32 *);
extern void SoundRemoveSedsEntry(s32 *);
extern void BattleResetSlotConditionCounters(void);
extern void BattleFreeWdsEntry(void);
extern void BattleUnloadActorArchive(void);
extern void BattleStopSpritesAndReleaseActors(s32);
extern void func_801E252C(void);
extern void func_800723E0(void);
extern void func_8007252C(void);

void BattleLoadOverlay(void) {
    s32 first;
    s32 second;

    if (g_BattleOverlayEnabled != 0) {
        BattleSetArchive20Entry0();
        first = BattleAllocHeapBlock(4, 1);
        g_BattleOverlayHeapBlock1 = first;
        second = BattleAllocHeapBlock(first + 0x7FE1B000, 1);
        g_BattleOverlayHeapBlock2 = second;
        ArchiveReadFileToBuffer(1, (void *)0x801E5000, 0, 0x80);
        BattleWaitForArchiveSync();
        func_801E5160();
    }
}

void BattleSetOverlayArg(u8 arg0) {
    if (g_BattleOverlayEnabled != 0) {
        func_801E879C(arg0);
    }
}

void BattleFadeOutAudioIfOverlayIdle(void) {
    u8 result;

    result = 0;
    if (g_BattleOverlayEnabled != 0) {
        result = func_801E563C();
    }
    if (result == 0 && D_800C48EA == 0x81) {
        SoundSetAudioManagerVolumeWithFade(g_BattleAudioManager, 0, 0xF0);
    }
}

void BattleRunScene(void) {
    s32 skinBlock1;
    s32 skinBlock2;
    s32 block1;
    s32 block2;
    s32 nextMode;
    s32 *pAudioManager;
    s32 display;
    u16 animFlags;
    s32 value;
    u8 *pFlags;
    u8 *pMenuEvent;
    u16 *pPrimPool;
    u8 overlayFlags;
    u16 *pPoolFlags;
    u8 mode;
    u8 savedFlags;
    s32 offset;
    s32 i;

    nextMode = 1;
    g_BattleGraphicsWork = (u8 *)BattleAllocHeapBlock(0xA2B4, 0);
    g_BattleDisplayState = (BattleWork *)BattleAllocHeapBlock(0x10C, 0);
    g_BattleCommandQueue = (BattleCommandQueueState *)BattleAllocHeapBlock(0x2F8, 0);
    bzero(g_BattleGraphicsWork, 0xA2B4);
    bzero((u8 *)g_BattleDisplayState, 0x10C);
    bzero((u8 *)g_BattleCommandQueue, 0x2F8);

    pAudioManager = g_SoundAudioManager;
    g_DebugPageIndex = 0;
    g_BattleLastMenuEvent = 0xFF;
    g_BattleMenuEventHistory = 0xFF;
    g_BattleSoundEnabled = 0;
    g_BattleAudioManager = pAudioManager;

    if (g_GameResumeScene != 0) {
        g_GameSelectedSkinIndex = g_GameResumeScene - 1;
        if (g_BattleResumeAudioFadePending != 0) {
            g_BattleResumeAudioFadePending = 0;
            SoundSetAudioManagerVolumeWithFade(pAudioManager, 0x7F, 0x3C);
        }
        if (g_GameSelectedSkinLoadPending == 0) {
            g_GameResumeScene = 0;
        }
    }
    if (g_GameSelectedSkinLoadPending != 0) {
        ArchiveSetIndex(0x10, 2);
        skinBlock1 = BattleAllocHeapBlock(4, 1);
        skinBlock2 = BattleAllocHeapBlock(skinBlock1 + 0x7FE20000, 1);
        ArchiveReadFileToBuffer(1, (void *)0x801E0000, 0, 0x80);
        ArchiveCdDataSync(0);
        HeapFree((void *)skinBlock1);
        HeapFree((void *)skinBlock2);
        if (g_GameResumeScene == 0) {
            func_801E0A34();
        } else {
            g_GameSelectedSkinIndex = g_GameResumeScene - 1;
            g_GameResumeScene = 0;
        }
    }

    if (g_DebuggerState[0] != -1) {
        ArchiveSetIndex(0x10, 2);
        ArchiveReadFileToBuffer(6, (void *)0x80280000, 0, 0x80);
        ArchiveCdDataSync(0);
    }
    memmove(g_BattleSelectedSkinRecord, D_800658DC + g_GameSelectedSkinIndex * 0x20, 0x20);
    BattleLoadStage(g_SoundEffectBankIndex);
    func_8007252C();
    display = g_BattleStageDisplayId;
    g_BattleDisplayMode = 2;
    BattleInitStageDisplay(display);
    BattleStartScreenFade(0, 2, 0xFF, 0xFF, 0xFF);
    if (g_BattleOverlayEnabled == 0) {
        BattleStartScreenFade(0x14, 2, 0, 0, 0);
    }
    if (g_GameSelectedSkinLoadPending != 0) {
        g_BattleAudioManager = SoundCreateAndPlayAudioManager(g_SoundBankLoadBuffer, 0x7F, 0);
    }
    g_BattleSlotParamTable = g_SystemStreamedFileData;
    g_BattleStateWork = g_SystemStreamedFileData;
    func_80077990();
    animFlags = g_BattleStateFlags;
    g_BattleCountdownsEnabled = 1;
    BattleBeginAnimTask(animFlags);
    BattleUpdateFrame();
    BattleInitTextSlotGraphics();
    while (g_BattleSceneRunning == 0) {
        BattleUpdateFrame();
    }

    HeapFree(g_SoundSedsBank);
    if (g_SoundEffectBankIndex != 4) {
        SoundStopElementsForSoundFile(g_SoundSedsBank);
    }
    SoundRemoveSedsEntry(g_SoundSedsBank);
    HeapFree(g_SoundStreamReserveStart);
    HeapFree(g_SoundStreamReserveBlock);
    BattleLoadOverlay();
    BattleSetOverlayArg(1);
    if (D_800D2FC4 == 0) {
        g_BattleDisplayMode = 1;
    }
    BattleResetSlotConditionCounters();

    for (i = 0; i < 3; i++) {
        if (g_BattleActorCharIds[i] != 0x7F) {
            g_BattlePartyConditionMasks[i].unk0 = g_GameState.conditionRows[g_BattleActorCharIds[i]].commandMask;
            g_BattlePartyConditionMasks[i].unk2 = g_GameState.conditionRows[g_BattleActorCharIds[i]].characterConditionMask;
        }
    }

    while (D_800C48EA == 0 && D_800D2FC4 == 0) {
        if (g_BattleSceneRunning != 0) {
            func_800723E0();
        }
        BattleUpdateFrame();
    }

    pFlags = D_800C48EA_bytes;
    if ((*pFlags & 0xC0) == 0) {
        mode = 0;
    } else if ((*pFlags & 0x40) != 0) {
        mode = 1;
    } else if (g_BattleOverlayEnabled == 0) {
        mode = 2;
    } else if (g_BattleDiscChangePending != 0) {
        mode = 3;
        g_GameDiscChangeRequest = mode;
        *pFlags = 1;
    }

    switch (mode) {
    case 0:
        g_GameDiscChangeRequest = 0;
        nextMode = 0;
        // fall through
    case 3:
        if (g_BattleOverlayEnabled != 0) {
            pPrimPool = (u16 *)g_BattlePrimPool;
            overlayFlags = D_800C3D44;
            *(u16 *)((u8 *)pPrimPool + 0x394) = 0xFF;
            if (overlayFlags != 0 || D_800D2FC4 == 0) {
                i = 0;
                *(u8 *)((u8 *)pPrimPool + 0x800) = 0;
                pPoolFlags = (u16 *)g_BattlePrimPool;
                pMenuEvent = D_800C48EA_bytes;
                savedFlags = *pMenuEvent;
                offset = 0;
                *pMenuEvent = 0;
                do {
                    value = *(u16 *)((u8 *)g_BattleSlotEntries + offset);
                    value &= 0x8000;
                    // volatile: keeps cc1 from folding the 0x3B4 displacement into a
                    // second induction register, which is one instruction too many.
                    *(volatile u16 *)(pPoolFlags + 0x1DA) = value;
                    value |= *(u16 *)((u8 *)D_800CCE08 + offset) & 0x8000;
                    *(volatile u16 *)(pPoolFlags + 0x1DA) = value;
                    pPoolFlags++;
                    offset += 0x170;
                    i++;
                } while (i < 3);
                BattleFreeWdsEntry();
                BattleSetOverlayArg(1);
                D_800C48EA = savedFlags;
            }
        }
        break;
    case 1:
        g_GameDiscChangeRequest = 2;
        nextMode = 2;
        break;
    case 2:
        g_GameDiscChangeRequest = 1;
        nextMode = 1;
        break;
    }

    BattleUnloadActorArchive();
    if (D_800D2D50 != 0) {
        nextMode = 1;
    }
    ArchiveSetIndex(0x10, 0);
    block1 = BattleAllocHeapBlock(4, 1);
    g_BattleNextModeHeapBlock1 = block1;
    block2 = BattleAllocHeapBlock(block1 + 0x7FE22000, 1);
    g_BattleNextModeHeapBlock2 = block2;
    ArchiveReadFileToBuffer(4, (void *)0x801DE000, 0, 0x80);
    BattleStopSpritesAndReleaseActors(nextMode);
    while (g_BattleSceneRunning != 0) {
        BattleUpdateFrame();
    }
    if (g_BattleOverlayEnabled == 0) {
        if ((D_800C48EA & 0x40) == 0 && (D_8006F9DD & 8) == 0) {
            BattleStartScreenFade(0x40, 2, 0x40, 0x40, 0x40);
        }
    }
    BattleFadeOutAudioIfOverlayIdle();
    func_801E252C();
    HeapFree((void *)g_BattleNextModeHeapBlock1);
    HeapFree((void *)g_BattleNextModeHeapBlock2);
}



s32 BattleUpdateFrame(void) {
    if (g_DebuggerState[0] != -1) {
        func_8028022C();
    }
    func_800BE790();
    return 0;
}

void BattleTickSlotCountdowns(void) {
    u16 v;
    s32 dec;
    s32 i;

    if (g_BattleCountdownsEnabled == 0) {
        return;
    }
    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (g_BattleSlotEnabledFlags[i] == 0) {
            continue;
        }
        if (g_BattleActorCountdownState[i] != 0) {
            continue;
        }
        dec = 1;
        if ((g_BattleSlotEntries[i].unk08 | g_BattleSlotEntries[i].unk0A) & 0x8000) {
            dec = 2;
        }
        if (g_BattleSlotEntries[i].flags & 0x1000) {
            v = g_BattleSlotCountdownToggles[i] ^ 1;
            g_BattleSlotCountdownToggles[i] = v;
            if (v != 0) {
                continue;
            }
        }
        if (g_BattleSlotEntries[i].flags & 0x2000) {
            if ((g_BattleSlotEntries[i].unkE0 -= dec) != 0) {
                continue;
            }
            g_BattleSlotEntries[i].unkE0 = 0;
            g_BattleSlotEntries[i].flags &= 0xDFFF;
            continue;
        }
        if (g_BattleSlotEntries[i].flags & 0x80) {
            continue;
        }
        if (g_BattleSlotEntries[i].unk04 & 0x1000) {
            continue;
        }
        g_BattleSlotCountdownTimers[i] -= dec;
        if ((s16)g_BattleSlotCountdownTimers[i] > 0) {
            continue;
        }
        g_BattleActorCountdownState[i] = 1;
        g_BattleSlotCountdownTimers[i] = 0;
    }
}

void BattleResetActorCountdown(void) {
    u8 index;

    index = g_BattleCommandQueue->actorIndex;
    if (g_BattleActorCountdownState[index] != 0xFF) {
        g_BattleActorCountdownState[index] = 0;
    }
    g_BattleSlotCountdownTimers[g_BattleCommandQueue->actorIndex] = BattleSelectActionRow(g_BattleCommandQueue->actorIndex, 0);
    g_BattleSlotCountdownTimers[g_BattleCommandQueue->actorIndex - 0xB] = g_BattleSlotCountdownTimers[g_BattleCommandQueue->actorIndex];
}

void BattleShowWaitSkipText(void) {
    u8 *pEntry;
    s32 i;

    if (g_BattleWaitSkipFlag != 0 && (g_BattleActionSlotMask & g_BattleSlotFlag8000Mask) == 0) {
        pEntry = GetStringEntry(g_BattleSkipStringBank, g_BattleWaitSkipFlag);
        D_800D39C6 = SystemRenderStringEntry(pEntry, D_800D39C0[0], BATTLE_TEXT_SLOT_BUFFER_SIZE, 1);
        LoadImage((RECT *)&D_800D39C0[-2], D_800D39C0[0]);
        for (i = BATTLE_SKIP_WAIT_FRAMES; i != 0; i--) {
            BattleUpdateFrame();
        }
    }
}

void BattleClearCharRedrawFlags(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        g_BattleCommandQueue->charRedrawFlags[i] = 0;
    }
}

void BattleApplyCharRedrawFlags(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        g_BattleDisplayState->slotMarks[i] = g_BattleCommandQueue->charRedrawFlags[i];
    }
    BattleUpdateFrame();
}

void BattleClearAllSlotFlagB4(void) {
    s32 i;

    for (i = 0x2A0; i >= 0; i -= 0x60) {
        g_BattleTextSlotVisible[i] = 0;
    }
    g_BattleDisplayState->textSlotStringDrawn = 0;
    g_BattleDisplayState->textSlotEnabled = 0;
    BattleUpdateFrame();
}

void BattleWaitWhileStateIs8(void) {
    s32 i;

    if (g_BattleWaitSkipFlag != 0 && (g_BattleActionSlotMask & g_BattleSlotFlag8000Mask) == 0) {
        BattleSetSlotFlagB4(7);
        g_BattleCommandQueue->commandFinished = 0;
        for (i = 0x3B; i != 0; i--) {
            BattleUpdateFrame();
            if (g_BattleMenuEvent != 8) {
                break;
            }
        }
        g_BattleCommandQueue->commandFinished = 1;
        BattleClearSlotFlagB4(7);
        BattleUpdateFrame();
    }
}


// 83.82% under 2.6.0 (1728 bytes / 432 insns against retail's 1756 / 439). Round 536
// measured four new finds, all kept in the body below:
// 1. The skip flag is reached through an ARRAY VIEW of its symbol:
//      extern u8 g_BattleWaitSkipFlag_bytes[] __asm__("g_BattleWaitSkipFlag");
//    an array's decay is a real address value, so cc1 keeps ONE la s0 and emits
//    retail's sb zero,0(s0); the plain u8 extern folds the constant store into the
//    symbol store (lui at,%hi / sb zero,%lo(at)) and the later la lands elsewhere.
//    The clear loop then derives its biv from that same register, as retail does.
//    81.13 -> 83.02. Same trick as D_800C48EA_bytes, 60 lines up.
// 2. The countdown clear as a decreasing pointer: pClear = pFlags - 7;
//    for (i = 4; i >= 0; i--) *pClear-- = 0;  gives retail's addiu x,s0,-7 plus
//    addiu x,x,-1 in the back-edge delay slot; pFlags[i - 0xB] = 0 recomputes the
//    index with addu instead. 83.02 -> 83.36, left with a0 where retail keeps v1.
// 3. The two +0x10 quad stores want the constant added to the SCREEN ORIGIN first:
//    origins[...] + 0x10 + actorIndex * 0x60 emits retail's addiu before the addu.
//    83.29 -> 83.82. The +0x28 pair already has retail's order.
// 4. BattleFindMinValueOtherSlot returns s32 (its definition, misc11.c:786) and
//    BattleLoadActorSlotAnimation takes s32 (misc29.c:1056); with the real
//    prototypes the call is retail's move a0,v0 instead of andi a0,v0,0xffff.
// REMAINING, in the order they cost:
// A. CLOSED by find 1.
// B. The two tail loops (D_800C3D1A[i * 4], D_800C3EBA[i * 0x1C]): retail keeps the
//    SCALED INDEX in a register and re-forms the absolute base with lui/$at every
//    iteration; cc1 2.6.0 strength-reduces the base into a pointer accumulator
//    (la s0 in the preheader, addiu s0,s0,4 in the latch). AGENTS 3.4's unsolved class.
// C. The prim quad block: retail interleaves the prim-index load, the row sum and the
//    origin load in another order and loads g_BattleCommandQueue into t0 BEFORE
//    g_BattleGraphicsWork. Dropping the pDraw local for the inline typed subscript is
//    byte-identical (83.36 either way), so the local is not the cause.
// D. The queue loop's addu order: retail addu v1,v1,a1, the body addu v1,a1,v1.
//    MEASURED WORSE: the two-statement pSlot = g_BattleCommandQueue; pSlot += i * 0x40;
//    form (lever b-inplace-flips-pointer-base-but-hoists-mem-base) gives 80.32 / 1736.
// E. CLOSED by find 4.
// F. Retail hoists the else-arm loop init (li v0,0x1C) into the bnez delay slot of the
//    sltiu test and reuses that register as the loop counter; the body emits li s1,0x1c
//    at the loop head.
// Also tried and worse: plain g_BattleWaitSkipFlag = 0 with a separate D_800D2CA4[]
// clear (81.05), volatile DECLARATION of g_BattlePrimStartIndex (79.70, breaks a
// neighbour), -O1 (77.11), -O3 (compile fails), gcc 2.7.2 (80.54), cdk (73.77), -G0
// (compile fails) - the preset XenoBattle260 is right.
/*
// The same byte as an array: its decay is a real address value, so cc1 keeps one
// `la` in a register instead of folding the constant store into a symbol store.
extern u8 g_BattleWaitSkipFlag_bytes[] __asm__("g_BattleWaitSkipFlag");
// The eight vertex words of the action row's quad at +0x63D0 of the battle draw
// work block, per prim start index (the block's prim rows are 0x18 bytes apart).
typedef struct {
    u16 x0; // 0x00
    u16 y0; // 0x02
    u16 x1; // 0x04
    u16 y1; // 0x06
    u16 x2; // 0x08
    u16 y2; // 0x0A
    u16 x3; // 0x0C
    u16 y3; // 0x0E
    u8 unk10[8]; // 0x10
} BattleScrollRowQuad; // Size: 0x18, the stride of a prim row

// The action row quads of the battle draw work block.
typedef struct {
    u8 unk0000[0x63D0]; // 0x0000
    BattleScrollRowQuad rows[8]; // 0x63D0, one per action row index
} BattleDrawWorkBlock; // Size: 0x6418

// The 0xC-byte script flag block at D_800D2CA4: the five countdown bytes, then
// the skip flag g_BattleWaitSkipFlag.
typedef struct {
     u8 countdown[5]; // 0x0
     u8 unk5[6]; // 0x5
     u8 skip; // g_BattleWaitSkipFlag // 0xB
} BattleScriptFlagBlock; // Size: 0xC

void func_80071B94(u8 arg0) {
    BattleDrawWorkBlock *pDraw;
    u8 *pClear;
    u8 *pFlags;
    s32 i;

    g_BattleTextSlotCount = 1;
    pFlags = g_BattleWaitSkipFlag_bytes;
    *pFlags = 0;
    if (g_BattleCommandQueue->actorIndex == 0) {
        return;
    }
    BattleClearSlotCurveState();
    BattleClearCharRedrawFlags();
    g_BattleCountdownsEnabled = 0;
    g_BattleCommandQueue->actorIndex--;
    g_BattleSpriteColorTaskSlot = g_BattleCommandQueue->actorIndex;
    g_BattleCommandQueue->count = 0;
    g_BattleCommandQueue->commandFinished = 0;
    pClear = pFlags - 7;
    for (i = 4; i >= 0; i--) {
        *pClear-- = 0;
    }
    g_BattleActionSlotMask = 0;
    BattleDecrementSlotConditionTimers(g_BattleCommandQueue->actorIndex);
    if (g_BattleCommandQueue->actorIndex >= 3) {
        if (arg0 == 0) {
            BattleScriptResetActorState(g_BattleSlotEntries[g_BattleCommandQueue->actorIndex].unk04 & 0x2000);
            if ((D_800C3EB7[g_BattleCommandQueue->actorIndex][0] & 0x80) == 0 &&
                (D_800CCD1C[g_BattleCommandQueue->actorIndex].flags & 0x400) == 0) {
                g_BattleDisplayState->textSlotStringDrawn = 1;
                g_BattleTextSlotFlags = SystemRenderStringEntry(
                    GetStringEntry(g_BattleTextStringBank, D_800C3E3D[g_BattleCommandQueue->actorIndex]),
                    *(u8 **)g_BattleTextSlotBuffers, 0x39, 0);
                BattleLoadImageAndSync((RECT *)(g_BattleTextSlotBuffers - 8), *(u_long **)g_BattleTextSlotBuffers);
                g_BattleTextSlotVisible[0] = 1;
            }
        }
        if ((g_BattleSlotEntries[g_BattleCommandQueue->actorIndex].flags & 0x2080) == 0 &&
            (g_BattleSlotEntries[g_BattleCommandQueue->actorIndex].unk04 & 0x1000) == 0) {
            func_80079778();
            BattleClearAllSlotFlagB4();
            BattleShowWaitSkipText();
            BattleWaitWhileStateIs8();
        }
        BattleClearAllSlotFlagB4();
        BattleSetOverlayArg(0);
    } else {
        for (i = 0x1C; i >= 0; i -= 4) {
            D_800C3D1A[i] = 0;
        }
        pDraw = (BattleDrawWorkBlock *)g_BattleGraphicsWork;
        pDraw->rows[*(volatile s32 *)&g_BattlePrimStartIndex].x0 =
            g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + g_BattleCommandQueue->actorIndex] + 0x10 +
            g_BattleCommandQueue->actorIndex * 0x60;
        pDraw->rows[*(volatile s32 *)&g_BattlePrimStartIndex].y0 = 8;
        pDraw->rows[*(volatile s32 *)&g_BattlePrimStartIndex].x1 =
            g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + g_BattleCommandQueue->actorIndex] +
            g_BattleCommandQueue->actorIndex * 0x60 + 0x28;
        pDraw->rows[*(volatile s32 *)&g_BattlePrimStartIndex].y1 = 8;
        pDraw->rows[*(volatile s32 *)&g_BattlePrimStartIndex].x2 =
            g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + g_BattleCommandQueue->actorIndex] +
            0x10 + g_BattleCommandQueue->actorIndex * 0x60;
        pDraw->rows[*(volatile s32 *)&g_BattlePrimStartIndex].y2 = 0x20;
        pDraw->rows[*(volatile s32 *)&g_BattlePrimStartIndex].x3 =
            g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + g_BattleCommandQueue->actorIndex] +
            g_BattleCommandQueue->actorIndex * 0x60 + 0x28;
        pDraw->rows[*(volatile s32 *)&g_BattlePrimStartIndex].y3 = 0x20;
        g_BattleGraphicsWork[0x6414] = g_BattlePrimStartIndex;
        g_BattleGraphicsWork[0x6415] = 1;
        if ((g_BattleSlotEntries[g_BattleCommandQueue->actorIndex].flags & 0x2080) == 0 &&
            (g_BattleSlotEntries[g_BattleCommandQueue->actorIndex].unk04 & 0x1000) == 0) {
            if ((g_BattleSlotEntries[g_BattleCommandQueue->actorIndex].unk04 & 0x2000) == 0) {
                func_80080160(g_BattleCommandQueue->actorIndex);
            } else {
                func_80080C94(g_BattleCommandQueue->actorIndex);
            }
            if (g_BattleCommandQueue->unk2EA != 0) {
                g_BattleClock++;
            }
            D_800D36C0 = g_BattleCommandQueue->actorIndex;
            g_BattleDisplayState->primRowCount = 0;
            BattleApplySlotMaskToEffects(0);
        }
        BattleShowWaitSkipText();
        BattleWaitWhileStateIs8();
        BattleClearAllSlotFlagB4();
        BattleSetOverlayArg(0);
        for (i = 0; i < 8; i++) {
            if (BattleMaskHasSlotBit(g_BattleActionSlotMask, (u8)(i + 3)) != 0) {
                D_800C3D1A[i * 4] = 1;
            }
        }
        BattleRunActiveSlotScripts();
    }
    BattleApplyCharRedrawFlags();
    BattleApplyCommandCellFlags();
    g_BattleGraphicsWork[0x6415] = 0;
    for (i = 0; i < 0xB; i++) {
        ((u8 *)g_BattleCommandQueue)[i * 0x40 + 0x3C] = BattleBuildTargetCandidateListByRow(i);
        D_800C3EBA[i * 0x1C] =
            BattleActorPosXGreaterThan(i, ((u8 *)g_BattleCommandQueue)[i * 0x40 + 0x3C]);
    }
    BattleLoadActorSlotAnimation(BattleFindMinValueOtherSlot(g_BattleCommandQueue->actorIndex));
    BattleClearCharRedrawFlags();
    func_8007252C();
    if (D_800C48EA == 0) {
        BattleBeginActorStatAnimation(g_BattleCommandQueue->actorIndex);
    }
    func_8007252C();
    BattleApplyCharRedrawFlags();
    func_800BFE48();
    BattleResetActorCountdown();
    g_BattleCountdownsEnabled = 1;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071B94);



/* 72.24% - body is exact except the flag store: gcc hoists `li s3,1` out of the
   loop (loop.c invariant) and then schedules the g_BattleDisplayState store before the sh,
   costing a callee-saved reg and a 40-byte frame vs the original 32; the original
   re-materialises `li v0,1` inside the block. Tried: plain g_BattleDisplayState[i + 0x7C]
   (index-first addu), g_BattleDisplayState->slotMarks[i] (base-first addu, now
   identical - the cast is needed since g_BattleDisplayState became a typed pointer),
   store order before/after *pDst = *pSrc (no effect), pFlag pointer local.
   The pointer pair pDst = g_BattleSlotCountdownTimers, pSrc = pDst - 0xB reproduces the original
   addiu s2,s1,-0x16 exactly. */
/* 72.36% park note kept below:
   gcc's loop.c hoists `li s3,1` for the flag store into a callee-saved register,
   so the frame is 0x28 instead of 0x20 and the g_BattleDisplayState load lands after
   the sh instead of before the lhu. */
void BattleApplyCommandCellFlags(void) {
    s32 i;

    if (g_BattleCommandCellFlags & 7) {
        for (i = 0; i < 3; i++) {
            if (BattleMaskHasSlotBit(g_BattleCommandCellFlags, i) != 0) {
                g_BattleActorCountdownState[i] = 0;
                g_BattleSlotCountdownTimers[i] = g_BattleSlotCountdownTimers[i - 0xB];
                g_BattleDisplayState->slotMarks[i] = 1;
            }
        }
    }
    g_BattleCommandCellFlags = 0;
}

void BattleResetActorCommandSlots(void) {
    u8 *pBuf;
    u8 *p;
    u8 *pEnd;
    s32 i;
    s32 j;

    for (i = 3; i < 0xB; i++) {
        if (BattleMaskHasSlotBit(g_BattlePendingSlotMask, i) != 0) {
            p = pBuf;
            pEnd = p + 0x100;
            do {
                *p++ = 0;
            } while (p < pEnd);
            for (j = 0x8B8; j >= 0; j -= 0x48) {
                g_BattleCommandSlotIds[j] = 0xFF;
            }
            g_BattleCommandArgs = 4;
            g_BattleCommandTiles = 0x17;
            BattleResetActorStatDisplay(i);
            func_80071B94(1);
        }
    }
}

// 81.43% under 2.6.0 (356 bytes against retail's 332, 89 vs 83 insns; round 425
// re-measured it, round 534 re-measured the same body unchanged). ROTATED_ENTRY and
// CLOSED: see lever g-rotated-entry-j-to-bottom-test (round 398). Retail inverts the
// loop with an entry jump - the preheader evaluates the state byte, then `j
// .L800724CC` with `ori a3,zero,0xB` in the delay slot - so ONE compare (bne v0,t0)
// at the bottom is shared by the entry and the back edge. gcc 2.6.0 duplicates the
// exit test inline instead (`li v0,1` + `beq v0,t0 -> found`), which (a) costs the
// extra constant, (b) makes the found handler a branch target and so costs it a fresh
// `li v0,0xB` and a `nop` in the wrap-branch delay slot where retail reuses a3 and
// the loop's `addiu a1,a1,1`. That is the whole 6-insn gap. cc1_probe.sh measured
// while, for, do-while, guarded-do-while and goto-check shapes under 2.6.0, 2.7.2 and
// 2.7.2-cdk: every one emits [preheader: eval; beq -> exit][body][eval; bne -> body]
// and never retail's [preheader: eval; j bottom-test]. The threading needs a compiler
// pass none of the three installed has (2.8+?); worth re-probing if a 2.8.x preset is
// added. Everything below the entry is retail's: both call sites (func_80071B94(0) on
// the wrapped-out path and after the scan, BattleResetActorCommandSlots on the
// non-zero-mask arm, the if/else chain with the reset arm last), the charIndex arm and
// every load/store shape. Folded since the round-411 park (77.55 -> 81.43): a `u8
// targetSlot` local for the actor id in the condition removes the found handler's
// D_800D2DD8[i] reload, and the embedded assignment `(*pCurrent = i + 1)` removes its
// *pCurrent reload (`andi v1,v1,0xff` reuses the stored value, as retail does). Also
// tried and worse: `i++; if (i == 0xB) i = 0; *pCurrent = i;` for the tail (79.51,
// 352 bytes), the old post-loop fixup form (77.55), -G0 / 2.7.2 / cdk presets.
/*
void func_800723E0(void) {
    u8 *pCurrent;
    u8 targetSlot;
    s32 i;

    if (g_BattlePendingSlotMask == 0) {
        if (g_BattlePendingCharIndex != 0) {
            g_BattleCommandQueue->actorIndex = g_BattlePendingCharIndex;
            D_800D2DE3[g_BattlePendingCharIndex] = 1;
            D_800D2E04[g_BattlePendingCharIndex] = 0;
            g_BattlePendingCharIndex = 0;
        } else {
            g_BattleCommandQueue->actorIndex = 0;
            pCurrent = g_BattleTargetScan;
            i = *pCurrent;
            while (g_BattleActorCountdownState[targetSlot = D_800D2DD8[i]] != 1) {
                i++;
                if (i == 0xB) {
                    i = 0;
                }
                if (i == *pCurrent) {
                    func_80071B94(0);
                    return;
                }
            }
            g_BattleCommandQueue->actorIndex = targetSlot + 1;
            if ((*pCurrent = i + 1) == 0xB) {
                *pCurrent = 0;
            }
        }
        func_80071B94(0);
    } else {
        BattleResetActorCommandSlots();
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800723E0);



// 93.02% under 2.6.0 (904 bytes against retail's 908, 226 vs 227 insns; round 534
// re-measured this body and reproduced the round-425 number). Control flow, both loops
// and every load/store shape are retail's. The declaration this body needs is the
// `extern volatile u16 g_BattleStateFlags;` at line 208: retail RELOADS the global for
// the 0x7F8 test, again for the 0x7 test and once more as the loop-3 `mask`, and the
// non-volatile global is cse'd into one load (that body is 91.63% / 892 bytes). The
// other user of the global in this unit is BattleRunScene (line ~377, one load); it was
// re-diffed with the volatile declaration in place and stays 99.98% / 1944 bytes, the
// same 6-line residual it has without it, so the change costs nothing there.
// RESIDUAL 1, the whole 4-byte gap - ONE callee-saved register. Retail keeps SEVEN
// (s0-s6, 0x30 frame, saves ra+s0..s6): s1 = i, s2 = i*0x170, s3 = (i-3)*0x1C, s4 = i-3,
// s5 = a walking pointer to D_800C3D1B (+4 per iteration), s6 = 0xFF. We keep six, all
// one register lower, and the prologue/epilogue are 2 instructions short. Retail's
// extra register is the `j = i - 3` biv: it spells the loop-2 enabled test and the
// countdown store through the SHIFTED bases (D_800D2DCF[j] and D_800D2DE7[j], i.e.
// g_BattleSlotEnabledFlags[i] and g_BattleActorCountdownState[i] three bytes along) and
// walks D_800C3D1B with a pointer. Our i-based bases fold the 3 into the address
// (`3(at)`) and need no j biv - same linked bytes (folded by diff_func), one objdiff
// point and one register. Measuring that respelling means writing
// `g_BattleSlotEnabledFlags[j + 3]` with a `j = i - 3` local, which is not the natural
// spelling of these arrays; not tried yet.
// RESIDUAL 2, one instruction: `mask = g_BattleStateFlags` is a volatile read, so cc1
// emits `lhu v0 / andi s2,v0,0xffff` where retail loads straight into the variable's
// register (`lui s2,%hi / lhu s2,%lo`). Spelling it `mask = *(u16 *)&g_BattleStateFlags;`
// to cast the volatile away gives retail's bare load - MEASURED 92.27% / 900 bytes / 225
// insns, i.e. the register hand-out it buys back costs more than the instruction it
// saves, so the volatile read is the better half.
// Tried and worse: `u8 slot = i;` at the top of the loop body (slot lands in a1,
// 90.13%), s32/u16 `mask` (s32/LONG reproduces retail's bare `bnez s2`), a u8 loop
// counter (gcc emits sltu plus an andi per iteration - wrong), three if/else arm
// orderings (retail's order is the best one and is what the body below uses). A 4 minute
// permuter run from an earlier body (base 2434 -> best 1584) only found bodies that hoist
// `BattleSlotBitMask(i)` out of the branches - a call retail does not make on the
// continue paths, so wrong code.
/*
void func_8007252C(void) {
    s32 i;
    s32 mask;

    g_BattleStateFlags = 0;
    for (i = 0; i < 3; i++) {
        if (g_BattleSlotEnabledFlags[i] == 0) {
            continue;
        }
        if ((g_BattleSlotEntries[i].flags & 0xC000) != 0) {
            if ((g_BattleSlotEntries[i].flags & 0x8000) != 0) {
                g_BattleSlotEntriesBase[i].flags = 0;
            }
            g_BattleActorCountdownState[i] = 0xFF;
        } else if (D_800C3EB7[i][0] == 0) {
            g_BattleStateFlags |= BattleSlotBitMask(i);
        }
    }
    for (i = 3; i < 0xB; i++) {
        if (g_BattleSlotEnabledFlags[i] == 0) {
            continue;
        }
        if (g_BattleSlotReleasedFlags[i][0] == 0) {
            if ((g_BattleSlotEntries[i].flags & 0xC000) != 0) {
                if (BattleMaskHasSlotBit(g_BattleActorSlotDoneMask, i) == 0) {
                    g_BattleSlotEntriesBase[i].flags = 0;
                    g_BattleActorCountdownState[i] = 0xFF;
                    continue;
                }
            } else if (D_800C3EB7[i][0] != 0 || D_800C3D1B[i - 3].unk0 != 0) {
                continue;
            }
        } else if ((g_BattleSlotEntries[i].stateFlags & 0xC000) != 0) {
            if (BattleMaskHasSlotBit(g_BattleActorSlotDoneMask, i) == 0) {
                g_BattleSlotEntries[i].effectFlags4Pair = 0;
                g_BattleActorCountdownState[i] = 0xFF;
                continue;
            }
        } else if (D_800C3EB7[i][0] != 0 || D_800C3D1B[i - 3].unk0 != 0) {
            continue;
        }
        g_BattleStateFlags |= BattleSlotBitMask(i);
    }
    if ((g_BattleStateFlags & 0x7F8) == 0) {
        D_800C48EA = 1;
    }
    if ((g_BattleStateFlags & 0x7) == 0) {
        D_800C48EA = 0x81;
    }
    if (D_800C48EA != 0) {
        return;
    }
    mask = g_BattleStateFlags;
    for (i = 0; i < 0xB; i++) {
        if (BattleMaskHasSlotBit(mask, i) != 0 && (g_BattleSlotEntries[i].unk04 & 0x1000) != 0) {
            mask &= BattleSlotBitMaskInverted(i);
        }
    }
    if (mask == 0) {
        for (i = 0; i < 0xB; i++) {
            if (BattleMaskHasSlotBit(g_BattleStateFlags, i) != 0 && (g_BattleSlotEntries[i].unk04 & 0x1000) != 0) {
                g_BattleSlotEntries[i].unk04 &= 0xEFFF;
                break;
            }
        }
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007252C);



// Adds every second prim of a run of packages to the current ordering table: each
// package holds one POLY_FT4 per render context, and primStart selects which.
void BattleAddPrimListToOT(POLY_FT4 *pPrim, s32 count, s32 primStart) {
    s32 i;

    for (i = 0; i < count; i++) {
        AddPrim(g_BattleCurOT[0] + 4, &pPrim[i * 2 + primStart]);
    }
}

/* PARKED 95.62% under 2.6.0, 348 bytes against retail's 352 - ONE instruction
   short, so it can never link. Body is otherwise instruction-for-instruction:
   0x30 frame, six saved regs, the in-loop `sll s0,s2,1`, SetShadeTex first, then
   r0/g0, then b0, and the `!= 1` spelling gives retail's out-of-line 0x80/0x80 arm.
   THE RESIDUAL is the loop latch. Retail: `... sb zero,6(v0) / addiu s2,s2,1 /
   slt / bnez / nop`, with a `nop` also in the load-delay slot after `lw
   g_BattlePrimStartIndex`. Ours emits the b0 address chain AFTER the increment, so
   reorg steals `addiu s2,s2,1` into that `lw`'s load-delay slot and `sb zero,6(v1)`
   into the `bnez` slot - two instructions instead of three, hence 4 bytes short.
   The increment is emitted before the b0 store because in this compile `i` dies at
   the loop body's first statement.
   MEASURED DEAD this round: the plain body without the `prim` local - 85.06 with
   `lbu` and 89.10 once the global is a word - because loop.c then strength-reduces
   `i * 2` into a `s0 += 2` biv (`sll v0,s2,1` in the loop test) and the frame drops
   to 0x28; the `prim = i * 2;` body's FIRST STATEMENT is what stops that reduction
   and buys 89.10 -> 95.62. Also dead: `s32` return type (95.62, unchanged, so the
   c-stolen-delay-slot-retail lever does not apply here), a while-loop with `i++`
   as the body's last statement (identical), index written `i * 2 + g` (equivalent,
   89.10 with the biv reduction), gcc 2.7.2 (89.26), cdk (compile fails). The
   permuter's best (3 min, base 531 -> 161) hoists the else arm's `g0 = 0x80` out of
   its arm - wrong code (it overwrites the other arm's g0 = 0), rejected.
   NOTE the eval order that matters: `g_BattlePrimStartIndex` must be typed s32 in
   this file for retail's `lw` (it is read as a byte only by func_80071B94 and
   func_80073538, both still stubs).
*/
// ROUND 442 re-measure: 95.62% / 348 bytes (retail 352), unchanged from the note
//    below - the park compiled clean and the body is still the best known. One
//    instruction SHORT, so it can never link.
//    NEW ATTEMPTS THIS ROUND (all measured, all kept or worse):
//    - volatile b0 store (`*(volatile u8 *)&pPrim[...].b0 = 0;`): 352 bytes, retail's
//      exact size and retail's bnez delay-slot nop, but the sb lands AFTER the slt and
//      the increment is still hoisted into the lw load-delay slot -> 95.17%. So the
//      volatile blocks reorg from sinking the store into the branch slot but does not
//      stop it hoisting `addiu s2,s2,1`.
//    - b0 store indexed with `i * 2` instead of `prim` (keeps i live to the store):
//      89.10% / 344 - as the lever predicts, cc1 strength-reduces i to a doubled biv.
//    CONCLUSION: the residual is *only* reorg moving the loop increment into the
//    `lw g_BattlePrimStartIndex` load-delay slot (cc1's own .s shows the addiu there,
//    before the assembler). Retail keeps both delay slots as nop and the increment
//    after the b0 store. Needs an idiom that stops that one hoist - not found.
// ROUND 455: re-measured on the freshest cc1 .s: 95.62% / 348 bytes / 87 insns
//    against retail's 352 / 88 - ONE instruction short, so it can never link.
//    cc1's own .s (not maspsx) has the latch hoist:
//      lw $2,g_BattlePrimStartIndex / addu $18,$18,1 / addu $2,$16,$2 / ...
//    i.e. reorg inserts the loop increment into the load-delay slot of the last
//    chain's global load, and sinks the b0 sb into the bnez slot.
//    MEASURED DEAD THIS ROUND (each in cc1_probe.sh on this unit's real compiler,
//    then the promising ones in the unit):
//    - `while (i < last) { ...; volatile b0 store; i++; }`: retail's SIZE (352)
//      and retail's bnez-slot nop, but the increment is still in the load slot and
//      the store lands after the slt - the same body the old note measured at
//      95.17, so the volatile buys the branch slot and nothing else.
//    - reading the last chain's global as *(volatile s32 *)&g_BattlePrimStartIndex
//      with a volatile b0 store: byte-identical to the volatile body above (a
//      volatile load still fills its delay slot).
//    - pPrim[g + (i << 1)] for the last store, and the function-scope (i + i):
//      both let loop.c derive the doubled biv (frame 0x28, 6 saved regs) = 89.10.
//    Conclusion: the load-delay-slot hoist is a wall in every spelling whose
//    increment cannot be emitted after the test, so this stays parked.
/*
void func_80072938(POLY_FT4 *pPrim, s32 first, s32 last, s32 mode) {
    s32 i;
    s32 prim;

    for (i = first; i < last; i++) {
        prim = i * 2;
        SetShadeTex(&pPrim[g_BattlePrimStartIndex + prim], 0);
        if ((mode & 0xFF) != 1) {
            pPrim[g_BattlePrimStartIndex + prim].r0 = 0x80;
            pPrim[g_BattlePrimStartIndex + prim].g0 = 0;
        } else {
            pPrim[g_BattlePrimStartIndex + prim].r0 = 0x80;
            pPrim[g_BattlePrimStartIndex + prim].g0 = 0x80;
        }
        pPrim[g_BattlePrimStartIndex + prim].b0 = 0;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072938);



void BattleRenderActorSplitDigitPanel(s32 slot, u8 arg1) {
    s32 first;
    s32 i;
    s32 xIndex;

    first = g_BattleDisplayState->statSpriteCount[slot];
    for (i = 0; i < 3; i++) {
        if (g_BattleSlotDigitSpriteIds[i] != 0xFF) {
            g_BattleDisplayState->statSpriteCount[slot] += BattleDrawSpritePackage(
                g_BattleSlotDigitSpriteIds[i] + BATTLE_DIGIT_SPRITE_BASE,
                (s32)(g_BattleGraphicsWork + (slot * 3200 + 0x3A88)) + g_BattleDisplayState->statSpriteCount[slot] * 80,
                (s16)(g_BattleSlotDigitXOffsets[slot * 24 + i] + g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot]),
                0x10);
        }
    }
    // The glyph after '9' separates the two groups.
    g_BattleDisplayState->statSpriteCount[slot] += BattleDrawSpritePackage(
        BATTLE_DIGIT_SPRITE_BASE + 10,
        (s32)(g_BattleGraphicsWork + (slot * 3200 + 0x3A88)) + g_BattleDisplayState->statSpriteCount[slot] * 80,
        (s16)(g_BattleSlotDigitXOffsets[slot * 24 + 3] + g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot]),
        0x10);
    // Unused scene sprite entries do not consume an x offset.
    for (i = 4, xIndex = 4; i < 7; i++) {
        if (g_BattleSceneResources[i] != 0xFF) {
            g_BattleDisplayState->statSpriteCount[slot] += BattleDrawSpritePackage(
                g_BattleSceneResources[i] + BATTLE_DIGIT_SPRITE_BASE,
                (s32)(g_BattleGraphicsWork + (slot * 3200 + 0x3A88)) + g_BattleDisplayState->statSpriteCount[slot] * 80,
                (s16)(g_BattleSlotDigitXOffsets[slot * 24 + xIndex] + g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot]),
                0x10);
            xIndex++;
        }
    }
    if (arg1 != 0) {
        func_80072938((POLY_FT4 *)(g_BattleGraphicsWork + (slot * 3200 + 0x3A88)), first,
                      g_BattleDisplayState->statSpriteCount[slot], arg1);
    }
}
void BattleRenderActorStatDigits(s32 slot, u8 arg1) {
    s32 first;
    s32 i;

    first = g_BattleDisplayState->statSpriteCount[slot];
    for (i = 0; i < 5; i++) {
        if (g_BattleStatDigitSpriteIds[i] != 0xFF) {
            g_BattleDisplayState->statSpriteCount[slot] += BattleDrawSpritePackage(
                g_BattleStatDigitSpriteIds[i] + 0x67,
                (s32)(g_BattleGraphicsWork + (slot * 3200 + 0x3A88)) + g_BattleDisplayState->statSpriteCount[slot] * 80,
                (s16)(g_BattleStatDigitXOffsets[slot * 24 + i] + g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot]),
                0x10);
        }
    }
    if (arg1 != 0) {
        func_80072938((POLY_FT4 *)(g_BattleGraphicsWork + (slot * 3200 + 0x3A88)), first,
                      g_BattleDisplayState->statSpriteCount[slot], arg1);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072F38);
/* 80.04% under 2.6.0 (1084 bytes against retail's 1096). ROUND 372 re-measured
   this park: the stored body had NEVER COMPILED (it reads a `blank` local that was
   never declared, so the 81.16% recorded below came from an earlier variant); with
   `u8 blank;` added it is 80.04%. The 7 elided stores, the 11 digit-array stores,
   the five-step scale division, both ratio ladders and the three zero-suppression
   loops are still exact. What is left is the frame and the constants: retail forms
   each division magic ONCE and holds them in s0 (0x51EB851F, the /100) and s1
   (0x66666667, the /10), keeps the blank 0xFF in s2 and `result` in t9 (nothing
   calls after the mode selection) - a 0x10 frame; ours rematerialises both magics,
   holds `result` in s1 and the blank in s0, and saves one register fewer.
   The original note follows.
   ---
   81.16% under 2.6.0 (1088 bytes / 272 insns against retail's 1096 / 274, that is
   a frame difference only): retail saves THREE callee-saved registers (s0 =
   0x51EB851F, s1 = 0x66666667, s2 = 0xFF, each formed once and live to the end)
   and ours saves two, because gcc rematerialises both division magics in
   caller-saved registers instead of holding them across the function. Everything
   else is exact: the 4 row loads, all 7 elided stores, all 11 digit-array stores,
   the five-step scale division, both ratio ladders (one shared result = 2 arm) and
   the three zero-suppression loops (their bound test must be an INDEXED for loop:
   'for (i = 0; i < 2; i++)' with a break compiles to retail's lbu/bnez/sb/slt/bnez
   shape, while a do-while over pointers gives sltu and an inverted branch).
   Two measured constraints on the digit chains: hp/maxHp are s16 (a signed /100
   and /10 with sll/sra pairs), the scaled value is s32; and the remainder must be
   written as 'value = value - digit * 10' - with '%' gcc keeps the untruncated
   quotient and drops the sll/sra pair. The intermediate stores to D_800D2D38 /
   D_800D2FE0 / D_800D3018 survive only because the three chains are interleaved in
   the source, so no two stores to the same global are adjacent (gcc deletes the
   earlier one otherwise; measured on both layouts: 1028 bytes with the chains one
   after another). Tried: pointer-biv loops, a do-while over pointers, a u8 blank
   local for 0xFF, digits stored inline vs held to the end.
s32 func_80072F38(s32 slot, u8 arg1) {
    s16 hp;
    s16 maxHp;
    s32 scaledHp;
    s32 scaledMaxHp;
    s16 value;
    s16 maxValue;
    s32 scaledValue;
    s16 hpDigit0;
    s16 hpDigit1;
    s16 maxHpDigit0;
    s16 maxHpDigit1;
    s32 scaledDigit0;
    s32 scaledDigit1;
    s32 scaledDigit2;
    s32 scaledDigit3;
    s32 result;
    s32 i;
    u8 blank;

    maxHp = g_BattleSlotEntriesBase[slot].unk02;
    hp = g_BattleSlotEntriesBase[slot].flags;
    scaledHp = g_BattleSlotEntriesBase[slot].scaledHp;
    scaledMaxHp = g_BattleSlotEntriesBase[slot].scaledMaxHp;
    D_800D3330 = maxHp;
    D_800D2FE0 = maxHp;
    D_800D2E58 = hp;
    D_800D2D38 = hp;
    D_800D333C = scaledHp;
    D_800D3018 = scaledHp;
    D_800D3668 = scaledMaxHp;

    // 2 when the slot's value has run well ahead of its maximum, 1 when it is a
    // quarter of the way there, 0 below that; the digit renderers take it as their
    // shade mode.
    result = 0;
    if (arg1 != 0) {
        if (scaledMaxHp / 8 >= scaledHp) {
            result = 2;
        } else if (scaledMaxHp / 4 >= scaledHp) {
            result = 1;
        }
    } else if (maxHp / 8 >= hp) {
        result = 2;
    } else if (maxHp / 4 >= hp) {
        result = 1;
    }

    value = D_800D2D38;
    hpDigit0 = value / 100;
    value = value - hpDigit0 * 100;
    D_800D2D38 = value;
    maxValue = D_800D2FE0;
    maxHpDigit0 = maxValue / 100;
    maxValue = maxValue - maxHpDigit0 * 100;
    D_800D2FE0 = maxValue;
    scaledValue = D_800D3018;
    scaledDigit0 = scaledValue / 10000;
    scaledValue = scaledValue - scaledDigit0 * 10000;
    D_800D3018 = scaledValue;
    hpDigit1 = value / 10;
    value = value - hpDigit1 * 10;
    D_800D2D38 = value;
    maxHpDigit1 = maxValue / 10;
    maxValue = maxValue - maxHpDigit1 * 10;
    D_800D2FE0 = maxValue;
    scaledDigit1 = scaledValue / 1000;
    scaledValue = scaledValue - scaledDigit1 * 1000;
    D_800D3018 = scaledValue;
    scaledDigit2 = scaledValue / 100;
    scaledValue = scaledValue - scaledDigit2 * 100;
    g_BattleStatDigitSpriteIds[2] = scaledDigit2;
    D_800D3018 = scaledValue;
    scaledDigit3 = scaledValue / 10;
    scaledValue = scaledValue - scaledDigit3 * 10;
    g_BattleStatDigitSpriteIds[3] = scaledDigit3;
    D_800D3018 = scaledValue;

    blank = 0xFF;
    g_BattleSlotDigitSpriteIds[0] = hpDigit0;
    g_BattleSlotDigitSpriteIds[1] = hpDigit1;
    g_BattleSceneResources[4] = maxHpDigit0;
    g_BattleSceneResources[5] = maxHpDigit1;
    g_BattleStatDigitSpriteIds[0] = scaledDigit0;
    g_BattleStatDigitSpriteIds[1] = scaledDigit1;
    g_BattleSlotDigitSpriteIds[2] = *(u8 *)&D_800D2D38;
    g_BattleSceneResources[6] = *(u8 *)&D_800D2FE0;
    g_BattleStatDigitSpriteIds[4] = *(u8 *)&D_800D3018;

    // Blank the leading zeros of each group; the units digit is never blanked.
    for (i = 0; i < 2; i++) {
        if (g_BattleSlotDigitSpriteIds[i] != 0) {
            break;
        }
        g_BattleSlotDigitSpriteIds[i] = blank;
    }
    for (i = 0; i < 2; i++) {
        if (g_BattleSceneResources[4 + i] != 0) {
            break;
        }
        g_BattleSceneResources[4 + i] = blank;
    }
    for (i = 0; i < 4; i++) {
        if (g_BattleStatDigitSpriteIds[i] != 0) {
            break;
        }
        g_BattleStatDigitSpriteIds[i] = blank;
    }

    return result;
}
*/
void BattleRenderActorValueDigits(s32 slot) {
    u8 *pSlotGraphics;
    s16 amount;
    s32 off;
    s32 i;

    off = slot * 484;
    pSlotGraphics = g_BattleGraphicsWork;
    pSlotGraphics = pSlotGraphics + off;
    // 0x853D is the slot's (0x1E4 stride) status byte in the graphics work block.
    if (pSlotGraphics[0x853D] == 1) {
        amount = g_BattleSlotDisplayValue[slot * 8];
    } else {
        amount = D_800CCDC4[slot].flags;
    }
    BattleParseNumberToDigits(amount);

    for (i = 0; i < 4; i++) {
        if (g_BattleValueDigitSpriteIds[i] != 0xFF) {
            g_BattleDisplayState->valueSpriteCount[slot] += BattleDrawSpritePackage(
                g_BattleValueDigitSpriteIds[i] + 0x83,
                (s32)(g_BattleGraphicsWork + (slot * 320 + 0x6008)) + g_BattleDisplayState->valueSpriteCount[slot] * 80,
                (s16)(slot * 96 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + slot] + 0x4A) + i * 6),
                0x25);
        }
    }
    g_BattleDisplayState->valueSpritePrimStart[slot] = g_BattlePrimStartIndex;
}

/* PARKED 96.74% under 2.6.0, 1316 bytes against retail's 1312 - ONE instruction
   long, so it can never link. Re-measured this round on the parked body (the
   older 97.12% note came from an equivalent loop shape; both are 96.74 now).
   1. THE SIZE GAP: the 9th argument (0xC00) of both func_80025FA8 calls. Retail
      emits `li s0,0xc00` ONCE inside case 1 (after the argument loads) and stores
      that same register for both calls; this cc1 rematerialises `li t1,0xc00` per
      call, which is exactly the extra instruction. cc1's .greg dump shows loop.c
      hoisting the 3072 pseudo to the function preheader, so its allocno conflicts
      with every other one and reload rematerialises it. Measured dead this round:
      a function-scope `s16 angle` with `angle = 0xC00;` as case 1's FIRST
      STATEMENT (identical - the assignment is still hoisted), the block-scope
      initialiser (identical), case 0/case 1 loops written both as
      `for (j = first; ...)` and as retail's `for (; first < count * 2; first += 2)`
      (identical, the parked body uses the latter), and the case-3 arm written as
      `*pSlotFlags != 0 && charIds != 7` with the pending store first (95.73%,
      1332 bytes - worse). Earlier rounds measured dead: the literal in both calls,
      per-case counter locals (92.24), a `do { } while (0)` around case 1 (93.95).
   2. the prologue's save order: retail emits `sw s7,68 / li s7,0x1000` and
      `sw s6,64 / move s6,zero` in a different position among the s5/s3/s4 IV
      initialisations than this compile.
   3. case 1's 0x10/0x98 stack immediates: retail emits them BEFORE the
      `lui a3,g_BattlePrimStartIndex` load, ours after.
   4. case 3's two stores sit in the opposite arms' order (ours bne, retail beq).
   Preset is right: 2.6.0 (2.7.2 is worse across this unit).
   The parked body casts g_BattlePrimStartIndex to s32 on purpose: retail reads it
   with `lbu` in this function (0x3D24) and with `lw` at every other site.
   Load-bearing shapes found in earlier rounds (each measured): `first` must be
   `<count> * 2` ASSIGNED BEFORE the following package call; the loop-invariant
   bases must be source locals assigned before the loop in the order `i = 0;`,
   `pOrigins = g_BattleSlotScreenOrigins;`, `scale = 0x1000;`, then the for-init;
   case 1 needs `off`/`off2` locals holding `product + 0x3768` so the
   g_BattleGraphicsWork base is added last; the flags need the walking pointer
   (`pSlotFlags += 8`) and g_BattleSlotActorIds needs its own for-clause IV
   (`actorOff += 0x1C`), else gcc derives 28i as 4*(8i-i) and shifts the frame.
*/
// ROUND 442: re-measured 96.74% / 1316 bytes (retail 1312) - still one instruction
//    long, so it can never link. Measured this round and dead: the 9th argument
//    declared `s32 angle` instead of `s16` (identical). The other residuals the note
//    lists (prologue save order, case 1's argument-load order, case 3's arm layout)
//    are orderings, not size. This is the same wall as lever
//    j-a-constant-assigned-inside-the: the constant is inside the guard block and
//    still rematerialises, because both uses are separated by the first call.
// ROUND 455: this body was NOT COMPILING - the declaration said "u16 *pPoolFlags"
//    while the body walked pSlotFlags, so the 96.74% in the notes came from a stale
//    object. With the name fixed the pointer is also a "u8 *", not "u16 *" (retail
//    reads it with lbu and walks it 8 bytes per iteration): that alone fixed 10 of
//    the 85 differing lines. Re-measured, still 96.74% at 1316 bytes / 329 insns
//    against retail's 1312 / 328, i.e. ONE instruction long, so it can never link.
//    THE SIZE RESIDUAL, and it is the only one: retail emits "li s0,0xc00" ONCE
//    inside case 1 and stores that same callee-saved register to both calls' 9th
//    argument slot (sw s0,32(sp) twice); this cc1 rematerialises "li t1,0xc00" per
//    call. NEW THIS ROUND: putting angle = 0xC00 among the source's pre-loop
//    invariants (next to scale = 0x1000) is BYTE-IDENTICAL to the block-scope
//    "s16 angle = 0xC00;" - the direct proof that loop.c has already hoisted the
//    constant into the loop preheader (as the -dg note below says) and that reload
//    rematerialises it from there; "u32 angle" is identical too. So this is lever
//    a-shared-const-argument-outranks-a-three-ref-byte-flag seen from the wanted
//    side (retail keeps the CONST_INT in s0), and a-li-constant-is-a-reload-scratch
//    says that class is BLOCKED: s0-s8 are all held by the seven loop invariants
//    plus the two case-local cursors in BOTH builds, and no spelling moves the
//    constant's allocno.
//    The other 3 residuals are orderings, not size, each measured: (a) the prologue
//    emits the sw s6 / move s6,zero pair before the sw s7 / li s7,0x1000 pair where
//    retail has the reverse; (b) case 1 pushes the 5th/6th argument stores before
//    the three base-register loads where retail interleaves them; (c) case 3's
//    first if emits the TRUE arm first (bne to the else store) where retail emits
//    the else arm first (beq to the shared TRUE store) - same instruction count,
//    opposite arm layout.
/*
void func_80073538(void) {
    s32 actorOff;
    s32 first;
    s32 i;
    s32 off;
    s32 off2;
    u8 *pSlotFlags;
    u16 *pOrigins;
    s16 scale;

    i = 0;
    pOrigins = g_BattleSlotScreenOrigins;
    scale = 0x1000;
    g_BattleDisplayState->scaledPanelPackageCount = 0;
    for (actorOff = 0, pSlotFlags = g_BattleSlotFlags; i < 3; i++, actorOff += 0x1C) {
        if (g_BattleSlotActorIds[actorOff] != 0x7F) {
            switch (g_BattleDisplayState->panelMode[i]) {
            case 0:
                g_BattleDisplayState->panelPackageCounts[i] = 0;
                g_BattleDisplayState->panelPackageCounts[i] += BattleDrawSpritePackage(0x8F,
                    (s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) + g_BattleDisplayState->panelPackageCounts[i] * 80,
                    (s16)(i * 96 + (pOrigins[g_BattleSlotPageIndex * 3 + i] + 0x44)), 0x1F);
                g_BattleDisplayState->panelPackageCounts[i] += BattleDrawSpritePackageHalfScale(0x52,
                    (s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) + g_BattleDisplayState->panelPackageCounts[i] * 80,
                    (s16)(i * 96 + (pOrigins[g_BattleSlotPageIndex * 3 + i] + 0x48)), 0x1C);
                first = g_BattleDisplayState->panelPackageCounts[i] * 2;
                g_BattleDisplayState->panelPackageCounts[i] += BattleDrawSpritePackageHalfScale(0x53,
                    (s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) + g_BattleDisplayState->panelPackageCounts[i] * 80,
                    (s16)(i * 96 + (pOrigins[g_BattleSlotPageIndex * 3 + i] + 0x48)), 0x1C);
                // Every package of the panel that is not a digit is blended grey.
                for (; first < g_BattleDisplayState->panelPackageCounts[i] * 2; first += 2) {
                    BattleSetPrimGrey40BlendSub((POLY_FT4 *)((s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) +
                                                             (*(s32 *)&g_BattlePrimStartIndex + first) * 40));
                }
                g_BattleDisplayState->panelPrimStart[i] = g_BattlePrimStartIndex;
                g_BattleDisplayState->panelPending[i] = 1;
                break;
            case 1: {
                s16 angle = 0xC00;
                g_BattleDisplayState->scaledPanelPackageCount = func_80025FA8(g_BattleSpriteArchive, 0x52,
                    g_BattleGraphicsWork + 0x3768, *(s32 *)&g_BattlePrimStartIndex, 0x10, 0x98, scale, scale, angle);
                first = g_BattleDisplayState->scaledPanelPackageCount * 2;
                off = g_BattleDisplayState->scaledPanelPackageCount * 80 + 0x3768;
                g_BattleDisplayState->scaledPanelPackageCount += func_80025FA8(g_BattleSpriteArchive, 0x53,
                    (u8 *)(off + (s32)g_BattleGraphicsWork),
                    *(s32 *)&g_BattlePrimStartIndex, 0x10, 0x98, scale, scale, angle);
                for (; first < g_BattleDisplayState->scaledPanelPackageCount * 2; first += 2) {
                    off2 = (first + *(s32 *)&g_BattlePrimStartIndex) * 40 + 0x3768;
                    BattleSetPrimGrey40BlendSub((POLY_FT4 *)(off2 + (s32)g_BattleGraphicsWork));
                }
                g_BattleDisplayState->scaledPanelPrimStart = g_BattlePrimStartIndex;
                break;
            }
            case 3:
                if (*pSlotFlags == 0 || g_BattleActorCharIds[i] == 7) {
                    g_BattleDisplayState->panelPackageCounts[i] = 0;
                } else {
                    g_BattleDisplayState->panelPending[i] = 0;
                }
                BattleInitPanelMove(i);
                // fall through
            case 2:
                func_80076710(i);
                if (*pSlotFlags == 0 || g_BattleActorCharIds[i] == 7) {
                    g_BattleDisplayState->panelActive[i] = 0;
                }
                break;
            }
        }
        pSlotFlags += 8;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073538);


// While the sub-image scroll is running (BattleGraphicsWork.scrollActive), the row the
// scroll index selects gets its three colour bytes written and the two prims it holds
// are added to the ordering table. Read through the block as a byte array: retail
// loads 0x6410 with lbu, so that is not the s32 BattleGraphicsWork names here.
void BattleAddScrollRowPrims(void) {
    if (((u8 *)g_BattleGraphicsWork)[0x6415] != 0) {
        ((u8 *)g_BattleGraphicsWork)[0x63CC + ((u8 *)g_BattleGraphicsWork)[0x6414] * 24] = ((u8 *)g_BattleGraphicsWork)[0x6410];
        ((u8 *)g_BattleGraphicsWork)[0x63CD + ((u8 *)g_BattleGraphicsWork)[0x6414] * 24] = ((u8 *)g_BattleGraphicsWork)[0x6410];
        ((u8 *)g_BattleGraphicsWork)[0x63CE + ((u8 *)g_BattleGraphicsWork)[0x6414] * 24] = ((u8 *)g_BattleGraphicsWork)[0x6410];
        AddPrim(g_BattleCurOT[0] + 4, (0x63C8 + ((u8 *)g_BattleGraphicsWork)[0x6414] * 24) + (u8 *)g_BattleGraphicsWork);
        AddPrim(g_BattleCurOT[0] + 4, (0x63F8 + ((u8 *)g_BattleGraphicsWork)[0x6414] * 12) + (u8 *)g_BattleGraphicsWork);
    }
}
