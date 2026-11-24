#include "common.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/sound.h"

// .bss
u8 D_8005946C;
u32 g_SoundStreamReserveStart;
u32 g_SoundStreamReserveBlock;
u8 g_SystemBackdropColor[3];
u8 g_GameSelectedSkinLoadPending;
u8 g_SoundEffectBankIndex;
s32 g_SystemSemiTransMode;
void* g_SoundStreamBuffer;
SoundFile* g_SoundSedsBank;

extern u8 g_GameResumeScene;

extern u8 g_MenuSavedMainMenuChoice;

/* Best: 86.53%, 404 bytes, 101 insns (retail 424 bytes / 103 insns). Measured four times this round (baseline
   re-measured twice, plus variants (d) below); up from the
   old 85.15. Round 445 re-measured the baseline at 86.53 and tried variants (e)/(f); the TAIL is now SETTLED and
   it is the blocker (see TAIL below). THE FRAME, THE SAVE SET AND THE ROW-COPY LOOP ARE NOW RETAIL EXACT: 0x58 with ra,s3,s2,s1,s0,
   the whole prologue/call tail, and the copy loop as a do-while (`do *pDst++ = *pOut++; while (pDst < pSrc
   + i + 0x14)`) which reproduces retail's `move v1,s1 / addiu a0,sp,40 / addiu a1,s1,20 / lbu / sb /
   addiu v1,v1,1 / slt / bnez / addiu a0,a0,1`.
   RESIDUAL (the whole 2-insn gap): the inner scan loop's EVEN byte access. Retail strength-reduces
   pSrc[i + j] to an induction pointer (a0 = &g_GameState + i, +2 per step) whose base s1 = &g_GameState + i
   is reused by the row copy, and then recomputes the ODD byte as (i + j) + %lo(g_GameState + 1), with that
   address REMATERIALISED per row into $t2 and 0xF into $t1. Our body CSEs the shared base (v1 = pSrc + (i +
   j), odd read as 1(v1)), so gcc emits two addu per iteration, no induction and no s1 (it parks outBuf in
   s1 instead).
   MEASURED ALTERNATIVES, all worse: (a) a user-level pRow induction (`for (i = 0, pRow = (u8*)&g_GameState;
   ...; pRow += 0x14)`, even = pRow[j], row copy = pRow) — this DOES produce retail's a0 induction, retail s
   s1 and retail's copy loop exactly, but gcc then folds the odd's &g_GameState + 1 + i into pRow + 1, which
   it keeps in callee-saved $s4 (per row), and hoists 0xF out of the whole nest into $s3: frame 0x60, 420
   bytes, 80.80%. (b) both accesses spelled against the raw symbol — gets retail's %lo(g_GameState) and
   %lo(g_GameState+1) relocs and 104 insns, but $s4 again, frame 0x60: 78.08%. (c) inner loop as a
   source-level do-while (j = 0; do { ... } while (j < 0x14)) — gcc peels the first iteration: 121 insns, 484
   bytes, 63.40%. (d) round 433: (a)'s pRow induction for the even read and the row copy, but the odd read
   through its OWN pointer local `pOdd = (u8*)&g_GameState + 1` indexed `pOdd[i + j]` (so the +1 is not a
   literal in the inner loop) — 84.11%, 420 bytes, 105 insns, frame 88 with SIX saves: it reproduces the
   whole inner loop shape (a0 induction from pRow, `addu v0,i+j` / `addu v1,s4,v0`, `slt` on the row-copy
   compare, retail's prologue) but gcc still hoists pOdd's VALUE into $s4 and 0xF into $s3, which is the
   extra save. Hoisting pIn/pInOdd before the outer loop instead of per row changed nothing (84.11,
   identical code - the stores are dead). So the residual is not the ADDRESS FORM at all: it is WHICH four
   loop invariants gcc keeps. Retail keeps i/pRow/inBuf/inBuf+1 in $s0-$s3 and rematerialises the odd base
   (lui/addiu into $t2) and 0xF (li $t1) per row; every body here keeps i/pSrc|pRow/outBuf/0xF instead, so
   inBuf's address is recomputed per row into $a1/$a2. Getting the flip needs outBuf's address and the 0xF
   to stop looking worth a register, which no spelling tried does.
   (e) round 445: variant (d)'s pRow induction for the even read AND the row copy, but the odd base assigned
   PER ROW inside the outer loop (`pOdd = (u8*)&g_GameState + i + 1;` then `pOdd[j]`) instead of before it —
   85.53%, 408 bytes, 102 insns. This one gets retail's OUTER loop and row copy exactly (`slti $v0,$s0,620 /
   bnez` with `addiu $s1,$s1,0x14` in the delay slot, `move $v1,$s1 / addiu $a1,$s1,0x14`), but gcc folds i
   into the odd base per row (`lui $v0,%hi(g_GameState+1)` then `addu $a0,$s0,$v0`), so the odd read becomes
   `lbu 0($a0)` off its own induction and the inner loop loses retail's second addu.  (f) the same with the
   odd written inline as `((u8*)&g_GameState + 1)[i + j]` to stop that per-row fold — 82.93%, 412 bytes, 103
   insns: gcc re-folds it to `pRow + 1` in $s4 (frame stays 0x58 but `sw $ra,0x54` / `sw $s4,0x50` appear),
   exactly (a)'s fold. So the per-row-temp shape and the inline-constant shape bracket retail's and neither
   yields retail's `lui/addiu $t2,%hi/%lo(g_GameState+1)` per row plus `addu $v0,$t0,$a3 / addu $v1,$v0,$t2`
   per iteration. Also differing in every variant: retail's row-copy compare is `slt` (signed) where gcc emits
   `sltu` for a pointer compare, and the s16 volume-zeroing loop counter lands in $s0 not retail's $a3.
   TAIL (settled, round 445 — THIS is the real blocker, not the loop): the two retail stores are
   `sb %gp_rel(g_MenuSavedMainMenuChoice)($gp)` and `sb %gp_rel(g_GameResumeScene)($gp)`, i.e. the object that
   owns this function DEFINES both as small data, so with plain externs here gcc gives `lui at,0x0` + `sb
   0(at)` (2 insns each) and the function can never reach 103 instructions or 424 bytes. Tentative definitions
   work: `u8 g_GameResumeScene; u8 g_MenuSavedMainMenuChoice;` in this file (the unit already does exactly that
   for g_DebugPageIndex at the end, which links and stores gp_rel) gives the retail-exact gp_rel tail and
   takes THIS body to 88.67%, 396 bytes, 99 insns. But GameSetUpDisplayAndChangeState, in the same file,
   drops 100.00% -> 97.71% (376 bytes) because its two reads of g_GameResumeScene become `lbu 0($gp)` while
   retail's are `lui %hi/lbu`. One TU cannot have both, so the original temp3 object boundary is this
   function's start (file 0xC170, not the 0xC350 the yaml uses) and the two globals are commons in the object
   that owns func_8001B970 (0xC170..0xC350). Matching it needs a UNIT SPLIT (config/slus_006.64.yaml range +
   new TU + gears.toml preset entry) with the two globals defined there; until then the tail is a hard -2
   instructions and this stub is unreachable in system/temp3. Handling that split is what to do next — the
   loop residual above is worth ~4 instructions on top of it.
*/
/*
void func_8001B970(void) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    u8* pSrc;
    u8* pIn;
    u8* pInOdd;
    u8* pOut;
    u8* pDst;
    void* pBuffer;
    int i;
    int j;

    ArchiveSetIndex(ARCHIVE_DIR_MENUS, 0);
    HeapChangeCurrentUser(HEAP_USER_HIG, 0);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    ArchiveReadFileToBuffer(3, pBuffer, 0, 0x80);
    ArchiveCdDataSync(0);
    memmove(&g_GameState, pBuffer, 0x2358);
    HeapFree(pBuffer);

    pSrc = (u8*)&g_GameState;
    for (i = 0; i < 0x26C; i += 0x14) {
        pIn = inBuf;
        pInOdd = inBuf + 1;
        for (j = 0; j < 0x14; j += 2) {
            *pIn = pSrc[i + j];
            *pInOdd = pSrc[i + j + 1];
            if (pSrc[i + j] == 0xF && pSrc[i + j + 1] == 0) {
                break;
            }
            pIn += 2;
            pInOdd += 2;
        }
        SystemDecodeGlyphCodesN((u16*)inBuf, outBuf, j / 2);
        pOut = outBuf;
        pDst = pSrc + i;
        do {
            *pDst++ = *pOut++;
        } while (pDst < pSrc + i + 0x14);
    }

    for (i = 19; i >= 0; i--) {
        ((s16*)&g_SoundVolumeController)[i - 16] = 0;
    }
    g_MenuSavedMainMenuChoice = 6;
    g_GameResumeScene = 0;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3_b", func_8001B970);




extern u8 g_SystemStreamedFileIndex;
extern s32 g_SystemStreamedFileBuffer;
extern s32 D_80059520;
extern s32 g_SystemStreamedFileData;

extern s32 ArchiveStartStreamedFilePair(s32, s32, s32*, s32*, s32*);

void ArchiveLoadStreamedFilePair(void) {
    ArchiveStartStreamedFilePair(g_SystemStreamedFileIndex, 0, &g_SystemStreamedFileBuffer, &D_80059520, &g_SystemStreamedFileData);
}

void GameStartSelectedSkinLoad(void) {
    g_GameSelectedSkinLoadPending = 1;
    D_8005946C = 0;
    func_8001B970();
    ArchiveCdDataSync(0);
    g_SystemBackdropColor[0] = 0x88;
    g_SystemBackdropColor[1] = 0x76;
    g_SystemBackdropColor[2] = 0x54;
    g_SystemSemiTransMode = 2;
}

extern StreamDataQueueEntry g_SoundSedsStreamQueue[];
extern u8 g_SoundEffectIds[];
extern void SoundAddSedsEntry(SoundFile* pSoundFile);
extern void SoundPlaySoundEffectOnLastPair(s32 arg0);

void SoundLoadAndPlaySedsBank(void) {
    int i;
    u8 soundEffectId;
    void* pEndAddress;

    HeapChangeCurrentUser(HEAP_USER_HIG, 0);
    ArchiveSetIndex(0xC, 0);

    g_SoundStreamReserveStart = HeapAlloc(4, 1);
    g_SoundStreamReserveBlock = HeapAlloc(g_SoundStreamReserveStart + 0x7FE1C000, 1);
    pEndAddress = (void*)0x801E4000;
    g_SoundSedsBank = HeapAlloc(ArchiveDecodeAlignedSize(2), 1);
    g_SoundStreamBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);

    g_SoundSedsStreamQueue[0].archiveIndex = 2;
    g_SoundSedsStreamQueue[1].archiveIndex = 3;
    g_SoundSedsStreamQueue[1].pData = g_SoundStreamBuffer;
    g_SoundSedsStreamQueue[2].archiveIndex = 4;
    g_SoundSedsStreamQueue[2].pData = pEndAddress;
    g_SoundSedsStreamQueue[3].archiveIndex = 0;
    g_SoundSedsStreamQueue[3].pData = NULL;
    g_SoundSedsStreamQueue[0].pData = g_SoundSedsBank;
    ArchiveStartStreamQueue(g_SoundSedsStreamQueue, 0, 0x80);

    while (ArchiveDataSync() == 3) {}

    SoundAddSedsEntry(g_SoundSedsBank);

    if (g_SoundEffectBankIndex != 4) {
        for (i = 0; i < 3;) {
            soundEffectId = g_SoundEffectIds[g_SoundEffectBankIndex * 3 + i];
            i++;
            if (soundEffectId != 0xFF) {
                SoundPlaySoundEffectOnLastPair((g_SoundSedsBank->sedId << 16) | soundEffectId);
            }
        }
    }
}

extern int rand(void);

s32 GameRandomByteRange(u32 arg0, u32 arg1) {
    u32 arg0Byte;
    u32 arg1Byte;
    s32 range;
    s32 result;

    arg0Byte = arg0 & 0xFF;
    if (arg0Byte == 0xFF) {
        return 0xFF;
    }
    arg1Byte = arg1 & 0xFF;
    if (arg1Byte == 0) {
        return 0;
    }
    if (arg0Byte == arg1Byte) {
        return arg0Byte;
    }
    range = arg1Byte - arg0Byte;
    if (range < 0xFF) {
        result = (arg0 + (rand() & 0xFF) % (range + 1)) & 0xFF;
    } else {
        result = rand() & 0xFF;
    }
    return result;
}
