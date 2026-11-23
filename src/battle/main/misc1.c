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

// The command queue block g_BattleCommandQueue points at (the 0x2F8 bytes
// func_80070F40 allocates for it), as in misc4.c's view of the same block.
typedef struct {
    /* 0x000 */ u8 unk000[0x2D3];
    /* 0x2D3 */ u8 actorIndex; // the slot the queued command is being built for
    /* 0x2D4 */ u8 unk2D4[6];
    /* 0x2DA */ u8 count; // queued commands
    /* 0x2DB */ u8 commandFinished; // 0 while a queued command runs
    /* 0x2DC */ u8 unk2DC[0xF];
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
extern u8 D_800D2CAF;
extern u16 g_BattleActionSlotMask;
extern u16 g_BattleSlotFlag8000Mask;
extern u8 *D_800D39F0;
extern s32 D_800D39C0[];
extern u8 D_800D39C6;
extern u8 *GetStringEntry(u8 *);
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
extern u8 D_800C3EB8[][BATTLE_ACTOR_SLOT_STRIDE];
extern BattleStatusEntry D_800C3D1B[];
extern u16 g_BattleActorSlotDoneMask;
extern u16 g_BattlePendingSlotMask;
extern u8 g_BattleCommandSlotIds[];
extern u8 g_BattleCommandArgs;
extern u8 g_BattleCommandTiles;
extern void BattleResetActorStatDisplay(u8);
extern void func_80071B94(s32);
extern u16 BattleSelectActionRow(s32, s32);
extern void BattleSetSlotFlagB4(s32);
extern void BattleClearSlotFlagB4(s32);
extern s32 SystemRenderStringEntry(u8 *, s32, s32, s32);
extern u8 g_BattlePrimStartIndex;
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
extern void func_800765C4(s32);
extern void func_80076710(s32);
extern void func_80072938(POLY_FT4 *, s32, s32, s32);
extern u8 D_800D3298;
extern u8 g_BattleSlotEnabledFlags[];
extern BattleSlotRow g_BattleSlotEntries[];
extern BattleSlotRow g_BattleSlotEntriesBase[];
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
extern u16 D_800D2E1C[];
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
extern u16 g_BattleStateFlags;
extern u8 g_BattleLastMenuEvent;
extern u8 g_BattleSoundEnabled;
extern u8 g_BattleDisplayMode;
extern s32 *g_BattleSlotParamTable;
extern s32 *g_BattleStateWork;
extern u8 D_80059180;
extern u8 g_DebugPageIndex;
extern u8 D_800C3D44;
extern u8 D_800C3D5C;
extern s32 D_800C3DEC;
extern u8 g_BattleMenuEventHistory;
extern u8 D_800CCC58;
extern s32 D_800D2D3C;
extern u8 D_800D2D50;
extern s32 D_800D2F60;
extern u8 D_800D2FC4;
extern u8 D_8006F9DC[];
extern u8 D_8006F9DD;
extern u8 D_800658DC[];

// What the battle reset copies a character's two limit words into: one 4-byte
// pair per party slot.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
} BattleSlotLimits; // Size: 0x4

// Unsigned view of one UnkGearState record (0x20 bytes, the per-character table
// at g_GameState+0x16BC): the reset copies the unk4/unk6 word pair out with lhu,
// where the header types them s16 for the command-mask tests.
typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 unk6;
    /* 0x08 */ u8 unk8[0x18];
} BattleCharLimitRecord; // Size: 0x20

extern BattleSlotLimits D_800C3E0C[3];

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
extern void func_8007819C(void);
extern void SoundStopElementsForSoundFile(s32 *);
extern void SoundRemoveSedsEntry(s32 *);
extern void BattleResetSlotConditionCounters(void);
extern void BattleFreeWdsEntry(void);
extern void BattleUnloadActorArchive(void);
extern void func_800B853C(s32);
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80070F40);
/* 96.03% under 2.6.0 (1952 bytes against retail's 1944). Everything up to and
   including the mode selection is instruction-identical, including the frame
   (s0-s3), the s3/s2 assignment, the if/else-if chain that leaves mode unset on
   the overlay-enabled/D_800C3D5C==0 path (retail really does that; adding a
   `mode = 2` default moves the li and breaks the chain) and the switch tree with
   its unmerged case bodies. Three remaining shapes:
   1. the per-slot limit copy hoists `&g_GameState+0x16C0` into a register
      (`lui a1; addiu a1,5824` + `addiu a3,a1,2` before the loop, then
      `addu v0,v0,a1; lhu v0,4(v0)`) where retail re-forms
      `lui $at,%hi(g_GameState); addu at,at,v0; lhu v0,5824(at)` per access.
      gcc's cse shares the invariant address between the two loads; retail's two
      loads carry different %lo addends (0x16C0/0x16C2) so they never CSE.
      Tried: the direct s16 member access (gives lh, retail has lhu), a view
      struct, `*(u16 *)&...unk4`, the char-pointer arithmetic form, an explicit
      u16* record local, and an unbounded vs [3] declaration of
      g_BattleActorCharIds (the [3] one costs the `addiu a2,v1,3` loop bound, so
      the empty [] stays).
   2. the 0x80 arm's `*pFlags = 1` re-forms `lui at,%hi(D_800C48EA)` where retail
      reuses the a0 that holds &D_800C48EA (pFlags is already the one local used
      for the chain reads and for the case-body read/clear).
   3. the 0x3B4 loop folds 0x3B4 into the prim-pool biv (`addiu a0,v0,948`, then
      `sh v0,0(a0)`) where retail keeps `sh v0,948(a1)` on a biv that only
      advances by 2. The two slot-table reads do come out right (symbol base,
      `lui $at; addu at,at,offset`, and the byte offset advancing by 0x170)
      because the offset is a running variable and not `i * 0x170`.
   The two `mode` assignments and `g_GameDiscChangeRequest = mode` (not `= 3`)
   are load-bearing.
// Battle teardown / scene-exit driver: releases the battle work blocks and the
// sound banks, then switches on the request flags in D_800C48EA to pick which
// scene the game returns to (g_GameDiscChangeRequest / g_BattleDisplayMode).
void func_80070F40(void) {
    s32 block1;
    s32 block2;
    s32 nextMode;
    s32 *pAudioManager;
    s32 display;
    u16 animFlags;
    u16 value;
    u8 *pFlags;
    u8 *pSlotFlags;
    u8 mode;
    u8 savedFlags;
    s32 offset;
    s32 i;
    u8 blank;

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
        if (D_80059180 != 0) {
            D_80059180 = 0;
            SoundSetAudioManagerVolumeWithFade(pAudioManager, 0x7F, 0x3C);
        }
        if (g_GameSelectedSkinLoadPending == 0) {
            g_GameResumeScene = 0;
        }
    }
    if (g_GameSelectedSkinLoadPending != 0) {
        ArchiveSetIndex(0x10, 2);
        block1 = BattleAllocHeapBlock(4, 1);
        block2 = BattleAllocHeapBlock(block1 + 0x7FE20000, 1);
        ArchiveReadFileToBuffer(1, (void *)0x801E0000, 0, 0x80);
        ArchiveCdDataSync(0);
        HeapFree((void *)block1);
        HeapFree((void *)block2);
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
    memmove(D_8006F9DC, D_800658DC + g_GameSelectedSkinIndex * 0x20, 0x20);
    BattleLoadStage(g_SoundEffectBankIndex);
    func_8007252C();
    display = D_800C3DEC;
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
    D_800D3298 = 1;
    BattleBeginAnimTask(animFlags);
    BattleUpdateFrame();
    func_8007819C();
    while (D_800CCC58 == 0) {
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
            // unk4/unk6 are read unsigned here (retail's lhu); the header types
            // the pair s16 for the command-mask tests that read them as signed.
            D_800C3E0C[i].unk0 = ((BattleCharLimitRecord *)g_GameState.conditionRows)[g_BattleActorCharIds[i]].unk4;
            D_800C3E0C[i].unk2 = ((BattleCharLimitRecord *)g_GameState.conditionRows)[g_BattleActorCharIds[i]].unk6;
        }
    }

    while (D_800C48EA == 0 && D_800D2FC4 == 0) {
        if (D_800CCC58 != 0) {
            func_800723E0();
        }
        BattleUpdateFrame();
    }

    pFlags = &D_800C48EA;
    if ((*pFlags & 0xC0) == 0) {
        mode = 0;
    } else if ((*pFlags & 0x40) != 0) {
        mode = 1;
    } else if (g_BattleOverlayEnabled == 0) {
        mode = 2;
    } else if (D_800C3D5C != 0) {
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
            *(u16 *)((u8 *)g_BattlePrimPool + 0x394) = 0xFF;
            if (D_800C3D44 != 0 || D_800D2FC4 == 0) {
                *(u8 *)((u8 *)g_BattlePrimPool + 0x800) = 0;
                pSlotFlags = g_BattlePrimPool;
                savedFlags = *pFlags;
                offset = 0;
                *pFlags = 0;
                for (i = 0; i < 3; i++) {
                    value = *(u16 *)((u8 *)g_BattleSlotEntries + offset) & 0x8000;
                    *(u16 *)(pSlotFlags + 0x3B4) = value;
                    value |= *(u16 *)((u8 *)D_800CCE08 + offset) & 0x8000;
                    *(u16 *)(pSlotFlags + 0x3B4) = value;
                    pSlotFlags += 2;
                    offset += 0x170;
                }
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
    D_800D2D3C = block1;
    block2 = BattleAllocHeapBlock(block1 + 0x7FE22000, 1);
    D_800D2F60 = block2;
    ArchiveReadFileToBuffer(4, (void *)0x801DE000, 0, 0x80);
    func_800B853C(nextMode);
    while (D_800CCC58 != 0) {
        BattleUpdateFrame();
    }
    if (g_BattleOverlayEnabled == 0) {
        if ((D_800C48EA & 0x40) == 0 && (D_8006F9DD & 8) == 0) {
            BattleStartScreenFade(0x40, 2, 0x40, 0x40, 0x40);
        }
    }
    BattleFadeOutAudioIfOverlayIdle();
    func_801E252C();
    HeapFree((void *)D_800D2D3C);
    HeapFree((void *)D_800D2F60);
}
*/

s32 BattleUpdateFrame(void) {
    if (g_DebuggerState[0] != -1) {
        func_8028022C();
    }
    func_800BE790();
    return 0;
}

void func_8007171C(void) {
    u16 v;
    s32 dec;
    s32 i;

    if (D_800D3298 == 0) {
        return;
    }
    for (i = 0; i < 0xB; i++) {
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
            v = D_800D2E1C[i] ^ 1;
            D_800D2E1C[i] = v;
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071964);
/* 99.51% under 2.6.0 (also 99.51 -G0, 98.78 2.7.2, 94.59 cdk): 41 of 41
   insns and 164 bytes = retail's, and the ONLY residual is the entry flag
   load, retail 'lbu a1,D_800D2CAF' against our 'lbu v0'. Round 418 closed the
   last door on it with a repo-wide scan of the matched objects: of the 171
   functions whose first two insns are 'lui r,%hi(sym)' plus a load into the
   same r that feeds the entry branch, 155 take $v0 and EVERY non-$v0 case is
   either a parameter or a value read again in a later block (libarchive's
   ArchiveReleaseStreamBlock/StreamSection, the only two $a1). This value dies
   in the entry block, so local-alloc hands it $v0; moving it means putting the
   load and its test in different blocks, which needs a branch between them and
   costs an instruction. 24 spellings measured in rounds 405/418 (compound &&,
   plain, !(), nested ifs, two ifs, early return, goto-label, if(0), while(0),
   switch on the flag, u8/char/int/register/u8-with-init locals, pointer deref,
   volatile, D_800D2CAE[1], literal mask, hoisted u16 mask temp,
   -fschedule-insns/2) all emit lbu $2; 'flag == 1' reaches $3 but costs li+bne
   (42 insns). Three permuter runs (2081, 1343 and 1351 iterations) base 20
   best 20 - it scores maspsx expansion noise on this body. Verdict: allocator
   tie, leave it a stub.
*/
/*
void func_80071964(void) {
    u8 *pEntry;
    s32 i;

    if (D_800D2CAF != 0 && (g_BattleActionSlotMask & g_BattleSlotFlag8000Mask) == 0) {
        pEntry = GetStringEntry(D_800D39F0);
        D_800D39C6 = SystemRenderStringEntry(pEntry, D_800D39C0[0], 0x39, 1);
        LoadImage((RECT *)&D_800D39C0[-2], (u_long *)D_800D39C0[0]);
        for (i = 3; i != 0; i--) {
            BattleUpdateFrame();
        }
    }
}*/

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

    if (D_800D2CAF != 0 && (g_BattleActionSlotMask & g_BattleSlotFlag8000Mask) == 0) {
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
void func_80072270(void) {
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800723E0);
/* 77.55% under 2.6.0 (352 bytes vs retail's 332, 88 vs 83 insns). ROUND 411
   re-measured this exact body: 77.55%, 352 bytes, 88 insns - it does compile, so
   the numbers are current. Structure now
   matches retail's block graph: mask test as bnez-forward to a reset-call block
   laid out last, E1's func_80071B94(0) cross-jumped into the shared call block
   (j .L80072504), the 0x18 frame, and the *pCurrent = i + 1 tail with the
   0xB wrap. Two residuals:
   1. THE ROTATED ENTRY (the real blocker): retail evaluates the loop condition
      once in the preheader and `j`s straight to the single bottom test
      (j .L800724CC), keeping t0=1 in a register from the beqz delay slot and
      a3=0xB in the j's delay slot. gcc 2.6.0/2.7.2/2.7.2-cdk all emit an extra
      inline `beq state,1` at the entry for while/for/do-while/goto shapes
      (probed with cc1_probe.sh: while+return-exit, while+break, for, plain
      do-while, guarded do-while, goto-check - ALL keep the inline entry branch,
      none thread it into the bottom test). Cost: 4 instructions + t0/a3 hoisting.
      A compiler with real loop rotation (gcc 2.8+? not in tools/) is the likely
      answer; worth a try_flags screen if one is ever added.
   2. The found-handler reloads D_800D2DD8[i] (3 insns) where retail reuses the
      loop's slot register (v1); tied to the same register allocation as (1).
   Tried: while + circle-exit func_80071B94(0)+return (77.55), same with the
   final call shared after the inner if/else (75.11 - the shared call merged
   E1's call site; keep this), for-loop (68.84, old park), else-if nesting.
   Semantics note: the circle path (scan wrapped all 11 entries without a
   ready slot) must call func_80071B94(0) WITHOUT the actorIndex/cursor stores
   - retail's beq at the loop head jumps directly to the call.
void func_800723E0(void) {
    u8 *pCurrent;
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
            while (g_BattleActorCountdownState[D_800D2DD8[i]] != 1) {
                i++;
                if (i == 0xB) {
                    i = 0;
                }
                if (i == *pCurrent) {
                    func_80071B94(0);
                    return;
                }
            }
            g_BattleCommandQueue->actorIndex = D_800D2DD8[i] + 1;
            *pCurrent = i + 1;
            if (*pCurrent == 0xB) {
                *pCurrent = 0;
            }
        }
        func_80071B94(0);
    } else {
        BattleResetActorCommandSlots();
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007252C);
/* 93.02% under 2.6.0 (904 bytes against retail's 908, 226 vs 227 insns).
   Control flow, both loops and every load/store shape are retail's. Three
   residuals, in the order they cost:
   1. TAIL, 3 of the 4 missing bytes: retail RELOADS g_BattleStateFlags for the
      0x7F8 test, again for the 0x7 test, and once more for `mask`; our non-volatile
      global is cse'd into one load. Measured cure: `extern volatile u16
      g_BattleStateFlags;` (line ~164) makes both test loads appear (91.63 -> 93.02,
      892 -> 904 bytes) but ALSO turns the `mask = g_BattleStateFlags` load into
      `lhu v0 / andi s2,v0,0xffff` where retail has a bare `lui s2 / lhu s2`, so
      that one read has to stay non-volatile (a `*(u16 *)&` cast-away, or a second
      alias). The other user of the global in this unit (animFlags, line ~370) is a
      single load, so a volatile declaration does not change its code - it was not
      re-diffed, verify before committing it.
   2. REGISTER NUMBERING, the remaining byte: retail uses 7 callee-saved regs
      (s0 = the masked slot, s1 = i, s2..s5 = the four bivs, s6 = 0xFF; 0x30 frame
      saving ra+s0-s6) and we use 6, so every register from s0 up is one lower and
      the prologue/epilogue are 2 instructions shorter (the whole 4 byte gap is
      prologue/epilogue + the tail load). Retail materialises `(u8)i` into s0 in the
      `beqz` delay slots at +0x148/+0x1B0 and then passes it with `move a1,s0` /
      `move a0,s0`, i.e. the u8 conversion is one CSE'd pseudo with a callee-saved
      register; gcc 2.6.0 keeps our equivalent in a1 and re-does `andi` at each call.
      Tried: `u8 slot = i;` at the top of the loop body (slot lands in a1, 90.13%),
      `s32`/`u16` mask (the s32 form is what reproduces retail's bare `bnez s2`), a u8
      loop counter (gcc emits sltu plus an andi per iteration - wrong), three if/else
      arm orderings (retail's order is the best one and is what the body below uses).
   3. An addu-operand-only difference on the loop-2 enabled test: retail spells it
      D_800D2DCF[j] with j = i-3 in its own biv, we spell it g_BattleSlotEnabledFlags[i]
      with a 3(at) immediate. Same bytes after linking, folded by diff_func, still
      worth one objdiff point.
   A 4 minute permuter run (base 2434 -> best 1584) only found bodies that hoist
   `BattleSlotBitMask(i)` out of the branches - a call retail does not make on the
   continue paths, so wrong code; and one earlier run did the same.
*/
/*
// Rebuilds g_BattleStateFlags, the mask of the slots that have an active action
// pending, and then reconciles D_800C48EA (the battle menu event) with it.
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
        if (D_800C3EB8[i][0] == 0) {
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
}
*/

// Adds every second prim of a run of packages to the current ordering table: each
// package holds one POLY_FT4 per render context, and primStart selects which.
void BattleAddPrimListToOT(POLY_FT4 *pPrim, s32 count, s32 primStart) {
    s32 i;

    for (i = 0; i < count; i++) {
        AddPrim(g_BattleCurOT[0] + 4, &pPrim[i * 2 + primStart]);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072938);
/* 83.62% - body is otherwise exact: POLY_FT4 array subscripted by
   `g_BattlePrimStartIndex + i * 2` repeated per store (which is why the original reloads
   g_BattlePrimStartIndex four times per iteration), SetShadeTex first, then r0/g0, then b0.
   The `!= 1` spelling is what reproduces the original's branch layout (beq to the
   out-of-line r0/g0 = 0x80 block, fall through to r0 = 0x80 / g0 = 0).
   Left: 2.7.2 keeps the index `i` in a callee-saved reg and re-does `sll i,1` each
   iteration (6 saved regs, 0x30 frame) and re-masks `mode` in the loop; ours makes
   `i*2` itself the loop induction and hoists `andi mode,0xFF` into the preheader
   (5 saved regs, 0x28 frame), which also shifts s3-s5 down by one.
void func_80072938(POLY_FT4 *pPrim, s32 first, s32 last, s32 mode) {
    s32 i;

    for (i = first; i < last; i++) {
        SetShadeTex(&pPrim[g_BattlePrimStartIndex + i * 2], 0);
        if ((mode & 0xFF) != 1) {
            pPrim[g_BattlePrimStartIndex + i * 2].r0 = 0x80;
            pPrim[g_BattlePrimStartIndex + i * 2].g0 = 0;
        } else {
            pPrim[g_BattlePrimStartIndex + i * 2].r0 = 0x80;
            pPrim[g_BattlePrimStartIndex + i * 2].g0 = 0x80;
        }
        pPrim[g_BattlePrimStartIndex + i * 2].b0 = 0;
    }
}
*/
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073538);
/* 97.12% under 2.6.0, 1316 bytes against retail's 1312 (ONE instruction long).
   Every other instruction is present: the residuals are three orderings and the
   one extra `li`.
   1. THE SIZE GAP: the 9th argument of both func_80025FA8 calls (0xC00) is one
      callee-saved register in retail (`ori s0,zero,0xC00` once, then `sw s0,0x20(sp)`
      for both calls); this cc1 rematerialises `li t1,0xC00` per call. Measured dead:
      the literal in both calls, a block-scope `s16 angle = 0xC00;` used at both
      sites, and an angle local at function scope. A cc1_probe of the same two-call
      group under this unit's own cc1/flags DOES put it in a callee-saved register,
      with and without an enclosing loop, so the difference is the surrounding
      switch-in-loop context, not the spelling. THE CAUSE IS loop.c, read off cc1's
      own dumps (`build/src/battle/main/misc1.c.i.greg`): this compile HOISTS the
      constant to the function preheader (`Insn 296 (life 24) ... moved to 849`,
      i.e. insn 849 sets the 3072 pseudo next to the i=0/scale/0x2E08 ones), so its
      range covers the whole outer loop, global_alloc lists it conflicting with
      every other allocno (`;; 298 conflicts: ... 74 289 ... 16`) and reload
      rematerialises it (`Spilling reg 9` twice). Retail's `ori s0,zero,0xC00`
      sits INSIDE case 1 (0x3860, after the argument loads), so retail did not hoist
      it and its range covered only case 1, where s0 is free (case 0's counter is
      dead, case 1's `first` is not yet live). The reuse-a-local theory is not
      possible in C: the 2nd call's 9th argument is 0xC00 at 0x38C0 and
      `first = count*2` is at 0x38C4, so two different values are live either side
      of the first call. Measured this round, both at 1316 with the same double `li`:
      per-case counter locals (`scaledFirst`/`k`, no sharing with case 0) 92.24%, and
      retail's own loop shape `for (; first < count*2; first += 2)` (one variable,
      no `j = first` copy) 97.12% - so the allocnos' conflict set does not move.
      What is left is defeating that hoist, and none of the round-327 levers
      (lifetime/savings) is reachable here: retail's span between the two uses is
      LONGER than this compile's (20 vs 14 insns) yet was not hoisted.
   2. the prologue's save order: retail emits `sw s6 / move s6,zero` (actorOff, the
      i*0x1C IV) AFTER the s3/s4 IV inits; ours defines it earlier.
   3. case 1's 0x10/0x98 stack-argument immediates: retail emits them BEFORE the
      `lui a3,g_BattlePrimStartIndex` load, ours after.
   4. case 3's two stores are mirrored: retail puts `sb zero,0xCC(a1)`
      (panelPending) in the `j` delay slot and jumps to `sb zero,0x74(a1)`
      (panelPackageCounts), so its chain is `beq ... beq ... j`; ours inverts the
      polarity to `bne` and swaps the two stores.
   Load-bearing shapes found this round (each measured):
   - `first` must be `pState->...Count * 2`, ASSIGNED BEFORE the following package
     call (90.56 -> 95.93): written as `first = <count>` with `first * 2` in the
     loop init, gcc defers the `sll` into the loop preheader and fills retail's nop.
   - the loop-invariant bases must be SOURCE locals assigned before the loop, in
     this order: `i = 0;` as its own statement, `pOrigins = g_BattleSlotScreenOrigins;`,
     `scale = 0x1000;`, then the for-init clause (95.93 -> 97.12: it fixes the whole
     prologue save order and every case-0 register choice). With them spliced inline
     the invariants are loop.c-hoisted to a different point of the preheader.
   - case 1 needs `off`/`off2` locals holding `product + 0x3768` so the
     g_BattleGraphicsWork base is added last.
   - the flags need the walking pointer (`pSlotFlags += 8`), and g_BattleSlotActorIds
     needs its own for-clause IV (`actorOff += 0x1C`): a plain `i * 0x1C` makes gcc
     derive 28i as 4*(8i-i), drop s6 and shift the frame.
   - a `do { } while (0)` around case 1 is worse (93.95%, 1328 bytes).
// Draws the three slots' stat panels for one frame: which of the three package
// calls each slot gets is its panelMode, and the packages are stacked into the
// slot's prim area, x from the slot's screen origin.
//
// g_BattlePrimStartIndex is a byte where it counts packages and a word where it
// indexes the pool; retail reads it both ways here.
void func_80073538(void) {
    s32 actorOff;
    s32 first;
    s32 i;
    s32 j;
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
                for (j = first; j < g_BattleDisplayState->panelPackageCounts[i] * 2; j += 2) {
                    BattleSetPrimGrey40BlendSub((POLY_FT4 *)((s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) +
                                                             (*(s32 *)&g_BattlePrimStartIndex + j) * 40));
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
                for (j = first; j < g_BattleDisplayState->scaledPanelPackageCount * 2; j += 2) {
                    off2 = (j + *(s32 *)&g_BattlePrimStartIndex) * 40 + 0x3768;
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
                func_800765C4(i);
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
