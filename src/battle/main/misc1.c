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
    /* 0x006 */ u8 unk06[0x82];
    /* 0x088 */ u32 effectFlags4Pair; // activeEffectFlags4 and accessoryEffectFlags4 as one word
    /* 0x08C */ u8 unk8C[0x18];
    /* 0x0A4 */ u16 stateFlags; // the first word of the row's BattleSlotState, 0xC000 as above
    /* 0x0A6 */ u8 unkA6[0x12];
    // The hp pair in 1/256ths that the display value ratios are taken of, as in
    // misc8.c's BattleSlotEntry.
    /* 0x0B8 */ s32 scaledHp;
    /* 0x0BC */ s32 scaledMaxHp;
    /* 0x0C0 */ u8 unkC0[0xB0];
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
extern u8 D_8005959C;
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
extern void func_800B8098(s32);
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
    D_8005959C = 0;
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
    func_800B8098(g_SoundEffectBankIndex);
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


INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007171C);
/* 77.62% under 2.6.0. The whole control flow (the 1/2 decrement, the 0x1000
   bit toggle on D_800D2E1C, the 0x2000 timer, the 0x80/0x1000 guards and the
   0xB wrap) is right; the miss is that gcc strength-reduces the most-used
   `(u8 *)SYM + off` (g_BattleSlotEntries, four uses) into a pointer biv, so its base is
   hoisted once into a3 and the loads become `lhu 0(a3)`, where retail re-forms
   `lui $at,%hi(SYM); addu $at,$at,$a0` at every use and keeps `off` an integer
   biv. Tried: `off` as a running for-clause biv, `off = i * 0x170` inline (both
   77.62%), one-`if`-per-level nesting without continue (70.32%), splitting the
   two entry tests. The other four symbols already come out in the retail form.
void func_8007171C(void) {
    u16 v;
    u8 *pTimer;
    s32 dec;
    s32 i;
    s32 off;

    if (D_800D3298 == 0) {
        return;
    }

    for (i = 0; i < 0xB; i++) {
        off = i * 0x170;
        if (g_BattleSlotEnabledFlags[i] == 0) {
            continue;
        }
        if (g_BattleActorCountdownState[i] != 0) {
            continue;
        }
        dec = 1;
        if ((*(u16 *)((u8 *)D_800CCD6C + off) | *(u16 *)((u8 *)D_800CCD6E + off)) & 0x8000) {
            dec = 2;
        }
        if (*(u16 *)((u8 *)g_BattleSlotEntries + off) & 0x1000) {
            v = *(u16 *)((u8 *)D_800D2E1C + i * 2) ^ 1;
            *(u16 *)((u8 *)D_800D2E1C + i * 2) = v;
            if (v != 0) {
                continue;
            }
        }
        v = *(u16 *)((u8 *)g_BattleSlotEntries + off);
        if (v & 0x2000) {
            pTimer = (u8 *)((u8 *)D_800CCE44 + off);
            *pTimer -= dec;
            if (*pTimer != 0) {
                continue;
            }
            v = *(u16 *)((u8 *)g_BattleSlotEntries + off);
            *pTimer = 0;
            *(u16 *)((u8 *)g_BattleSlotEntries + off) = v & 0xDFFF;
            continue;
        }
        if (v & 0x80) {
            continue;
        }
        if (*(u16 *)((u8 *)D_800CCD68 + off) & 0x1000) {
            continue;
        }
        pTimer = (u8 *)((u8 *)g_BattleSlotCountdownTimers + i * 2);
        v = *(u16 *)pTimer - dec;
        *(u16 *)pTimer = v;
        if ((s16)v > 0) {
            continue;
        }
        g_BattleActorCountdownState[i] = 1;
        *(u16 *)pTimer = 0;
    }
}
*/
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
/* 99.51% under 2.6.0 and 2.7.2: every instruction is identical except the
   leading flag load, which retail puts in $a1 (a dead argument register at
   entry) and gcc in $v0 (reused by the later lhu). Both pseudos have one use
   and no live range overlap, so this is an allocator tie; the and-operand
   register pair is identical. Tried: compound &&, !(), nested ifs, volatile on
   the flag, an int local for the flag, and the permuter (2081 iterations, base 20
   best 20) - it is an allocator tie, not a source shape.
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
}
*/
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072270);
/* 72.36% under 2.6.0 and 2.7.2. The pointer pair (pSrc = pDst - 0xB gives the
   original addiu s2,s1,-22) and every operand are right; gcc's loop.c hoists
   `li s3,1` for the flag store into a callee-saved register, so the frame is
   0x28 instead of 0x20 and the g_BattleDisplayState load lands after the sh instead of
   before the lhu. Tried under 2.6.0: 0x7C store before/after *pDst = *pSrc.
void func_80072270(void) {
    s32 i;
    u16 *pSrc;
    u16 *pDst;

    if ((g_BattleCommandCellFlags & 7) != 0) {
        pDst = g_BattleSlotCountdownTimers;
        pSrc = pDst - 0xB;
        for (i = 0; i < 3; i++) {
            if (BattleMaskHasSlotBit(g_BattleCommandCellFlags, i) != 0) {
                g_BattleActorCountdownState[i] = 0;
                *pDst = *pSrc;
                g_BattleDisplayState->slotMarks[i] = 1;
            }
            pDst++;
            pSrc++;
        }
    }
    g_BattleCommandCellFlags = 0;
}
*/

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
/* 68.84% under 2.6.0 (69.63 under 2.7.2). Whole structure is right (0x18 frame,
   both early-exit paths, the 0xB wrap and the *pCurrent = i + 1 tail). Misses:
   gcc invents a 4th lbu of g_BattlePendingCharIndex in the middle block, reloads
   D_800D2DD8[i] after the loop instead of reusing the loop's slot register,
   materialises a second `li 0xb` for the tail compare where the original keeps
   a3, and hoists `move a0,zero` out of the final jal's delay slot.
void func_800723E0(void) {
    u8 *pCurrent;
    s32 i;

    if (g_BattlePendingSlotMask != 0) {
        BattleResetActorCommandSlots();
        return;
    }

    if (g_BattlePendingCharIndex != 0) {
        g_BattleCommandQueue->actorIndex = g_BattlePendingCharIndex;
        D_800D2DE3[g_BattlePendingCharIndex] = 1;
        D_800D2E04[g_BattlePendingCharIndex] = 0;
        g_BattlePendingCharIndex = 0;
        func_80071B94(0);
        return;
    }

    g_BattleCommandQueue->actorIndex = 0;
    pCurrent = g_BattleTargetScan;
    for (i = *pCurrent; g_BattleActorCountdownState[D_800D2DD8[i]] != 1; i++) {
        if (i == 0xB) {
            i = 0;
        }
        if (i == *pCurrent) {
            break;
        }
    }
    g_BattleCommandQueue->actorIndex = D_800D2DD8[i] + 1;
    *pCurrent = i + 1;
    if (*pCurrent == 0xB) {
        *pCurrent = 0;
    }
    func_80071B94(0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007252C);
/* 91.63% under 2.6.0 (892 bytes against retail's 908). Both loops and the whole
   control flow are instruction-identical to retail modulo REGISTER NUMBERING: the
   only real miss is that retail keeps the loop counter in s1 and the 0x170 offset
   biv in s0, ours the other way round, which shifts every later register by one
   and reorders the prologue's save/increment pairs. The other gap is 3
   instructions in the tail: retail reloads g_BattleStateFlags for the second
   (0x7) test and again for the mask local (lui/lhu/nop each time) where cse
   follows the jump and reuses the first load.
   Tried: listing each if/else arm in retail's order (done - worth 11 points),
   s32 vs u16 mask (the s32 form reproduces retail's bare `bnez s2`), a u8 loop
   counter for the 3..0xA loop (gcc then emits sltu for the test and an andi per
   iteration - wrong), and a permuter run, which only found candidates that hoist
   BattleSlotBitMask(i) unconditionally - a call retail does not make on the
   continue paths, so wrong code. The 91.63% was measured with
   `extern s32 BattleMaskHasSlotBit(s32, u8)` and a `(u16)` cast on each result,
   which is the shape the mask loop wants; that prototype cannot live in this
   file, because BattleResetActorCommandSlots' call then loses its
   `andi v0,v0,0xFFFF` and that function links 4 bytes short.
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800728B8);
/* 78.84% under both 2.6.0 and 2.7.2: the original recomputes index*40 inline
   each iteration (sll 2 / addu / sll 3, the sll 2 rotated into the branch delay
   slot) while gcc strength-reduces an int index into a pointer (+80/iteration)
   and hoists pPrim into it. An s16 index emits the original's inline multiply
   but adds a sll/sra pair per use. Tried under 2.6.0: index += 2 in the body,
   index += 2 in the for clause, u32/s16/s32 index.
void func_800728B8(u8 *pPrim, s32 count, s16 index) {
    s32 i;

    for (i = 0; i < count; i++) {
        AddPrim(g_BattleCurOT[0] + 4, pPrim + index * 40);
        index += 2;
    }
}
*/

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
/* 81.16% under 2.6.0 (1088 bytes / 272 insns against retail's 1096 / 274). Every
   instruction matches except the frame: retail saves THREE callee-saved registers
   (s0 = 0x51EB851F, s1 = 0x66666667, s2 = 0xFF, each formed once and live to the
   end) and ours saves two, because gcc rematerialises both division magics in
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
   local for 0xFF, digits stored inline vs held to the end. A permuter run from
   this body is the next step, not more hand shapes.
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
/* 89.68% under 2.6.0, 1308 bytes against retail's 1312 (ONE instruction short).
   Every call, branch and store of the original is present, including all four
   loop IVs in retail's own registers (s3 = i*0x320 + 0x2E08 stepping 0x320,
   s4 = i*0x60, s5 = g_BattleSlotFlags + i*8, s6 = i*0x1C) and the tail's
   `s5 += 8 / s3 / s4 / i++ / slti / bnez / delay: s6 += 0x1C` order. What is
   left, in order of size:
   1. the 0xC00 stack argument of the two func_80025FA8 calls: retail CSEs it
      into a callee-saved register once (`ori s0,zero,0xC00`) and reuses it for
      both calls, ours re-materialises `li t1,0xC00` per call (the 0x10/0x98
      arguments are re-materialised in BOTH compiles, and 0x1000 is loop.c-
      hoisted into the prologue in both). That one instruction is the whole
      size gap; the frame and the save order follow from it.
   2. case 0's package count: retail loads it once into v0 and derives both
      `sll s0,v0,1` (the loop's 2*first) and the *80 argument from v0, before
      the third call; ours keeps the count in s0 and doubles it in place in the
      loop preheader, so the *80 chain comes first.
   3. case 1's second call: retail forms `idx*0x50 + 0x3768`, then loads
      g_BattleGraphicsWork into v0 and does `addu a2,a2,v0` in the jal delay
      slot; ours loads gfxWork into a2 before the stack stores and emits
      `addu a2,v0,a2`. Same for the following loop's `addiu a0,a0,0x3768`.
   4. case 3's two stores are mirrored: retail puts the panelPending store in the `j`
      delay slot and jumps to the panelPackageCounts store, ours the other way round.
   Load-bearing shapes found on the way (each worth 1-3 points):
   - g_BattleSlotActorIds needs its own for-clause offset local
     (`actorOff += 0x1C`); with a plain `i * 0x1C` gcc derives 28i as
     4*(8i - i) from another IV, drops s6 and shifts the whole frame.
   - the flags need an explicit walking pointer (`pSlotFlags += 8`); the
     `g_BattleSlotFlags[i * 8]` spelling gives an integer 8i IV plus a
     lui/addu/lbu per site instead of retail's `lbu 0(s5)`.
   - the 0x7F test must be an `if (...)` wrapping the switch, not a `continue`:
     only then is `pSlotFlags += 8` the last statement of the body, which is
     what puts its addiu at the arm join like retail.
   - a do/while(0) around case 1's body is 88.09% (it also moves the 28i IV
     into s7), so keep case 1 flat.
   A 3-minute permuter run (base 3005, best 2355) found only a candidate that
   reuses `first` to pass the 0x53 constant, i.e. wrong code, so nothing was
   ported.
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
       u8 *pSlotFlags;

       g_BattleDisplayState->scaledPanelPackageCount = 0;
       for (i = 0, actorOff = 0, pSlotFlags = g_BattleSlotFlags; i < 3; i++, actorOff += 0x1C) {
           if (g_BattleSlotActorIds[actorOff] != 0x7F) {
               switch (g_BattleDisplayState->panelMode[i]) {
           case 0:
               g_BattleDisplayState->panelPackageCounts[i] = 0;
               g_BattleDisplayState->panelPackageCounts[i] += BattleDrawSpritePackage(0x8F,
                   (s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) + g_BattleDisplayState->panelPackageCounts[i] * 80,
                   (s16)(i * 96 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + i] + 0x44)), 0x1F);
               g_BattleDisplayState->panelPackageCounts[i] += BattleDrawSpritePackageHalfScale(0x52,
                   (s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) + g_BattleDisplayState->panelPackageCounts[i] * 80,
                   (s16)(i * 96 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + i] + 0x48)), 0x1C);
               first = g_BattleDisplayState->panelPackageCounts[i];
               g_BattleDisplayState->panelPackageCounts[i] += BattleDrawSpritePackageHalfScale(0x53,
                   (s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) + g_BattleDisplayState->panelPackageCounts[i] * 80,
                   (s16)(i * 96 + (g_BattleSlotScreenOrigins[g_BattleSlotPageIndex * 3 + i] + 0x48)), 0x1C);
               // Every package of the panel that is not a digit is blended grey.
               for (j = first * 2; j < g_BattleDisplayState->panelPackageCounts[i] * 2; j += 2) {
                   BattleSetPrimGrey40BlendSub((POLY_FT4 *)((s32)(g_BattleGraphicsWork + (i * 0x320 + 0x2E08)) +
                                                           (*(s32 *)&g_BattlePrimStartIndex + j) * 40));
               }
               g_BattleDisplayState->panelPrimStart[i] = g_BattlePrimStartIndex;
               g_BattleDisplayState->panelPending[i] = 1;
               break;
           case 1:
               g_BattleDisplayState->scaledPanelPackageCount = func_80025FA8(g_BattleSpriteArchive, 0x52, g_BattleGraphicsWork + 0x3768,
                                                           *(s32 *)&g_BattlePrimStartIndex, 0x10, 0x98, 0x1000, 0x1000, 0xC00);
               first = g_BattleDisplayState->scaledPanelPackageCount;
               g_BattleDisplayState->scaledPanelPackageCount += func_80025FA8(g_BattleSpriteArchive, 0x53,
                                                            (u8 *)((first * 80 + 0x3768) + (s32)g_BattleGraphicsWork),
                                                            *(s32 *)&g_BattlePrimStartIndex, 0x10, 0x98, 0x1000, 0x1000, 0xC00);
               for (j = first * 2; j < g_BattleDisplayState->scaledPanelPackageCount * 2; j += 2) {
                   BattleSetPrimGrey40BlendSub((POLY_FT4 *)(((*(s32 *)&g_BattlePrimStartIndex + j) * 40 + 0x3768) +
                                                           (s32)g_BattleGraphicsWork));
               }
               g_BattleDisplayState->scaledPanelPrimStart = g_BattlePrimStartIndex;
               break;
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
