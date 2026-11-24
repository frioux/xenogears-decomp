#include "common.h"
#include "main/game.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/sound.h"

extern void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);

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

u8 g_GameResumeScene;
u8 g_MenuSavedMainMenuChoice;

/* Best: 88.48%, 392 bytes, 98 insns (retail 412 bytes / 103 insns). Round 448 re-measured from the split TU.
   THE TAIL IS NOW MATCHED (this function is in its own object, system/temp3_b, so the two globals are tentative
   definitions here and the tail comes out retail exact: `sb %gp_rel(g_MenuSavedMainMenuChoice)($gp)` and
   `sb %gp_rel(g_GameResumeScene)($gp)`). The frame (0x58, ra+s3+s2+s1+s0), the whole call tail and the row-copy
   loop's shape are retail exact too. The volume-zeroing loop counts with `j`, not `i` (round 448): with `i` gcc
   reuses $s0 and the loop reads `li s0,0x13 / addiu s0,s0,-1`, retail has $a3; `j` is retired there and gives
   retail's $a3 - 88.67 -> 88.48 only because it removes one instruction from a body that is already 5 short.
   RESIDUAL (the whole 5-insn gap): retail's inner scan loop keeps the EVEN byte on a strength-reduced induction
   pointer (a0 = &g_GameState + i, +2 per step, reused by the row copy through s1) and computes the ODD byte as
   (i + j) + a freshly rematerialised %hi/%lo(g_GameState + 1) in $t2, with 0xF rematerialised into $t1 per row;
   every body here CSEs the two bases into one v1 = base + (i + j) and reads the odd as 1(v1), so the inner loop
   loses an addu and gcc spends the freed registers on the odd base / 0xF / outBuf instead of on inBuf and
   inBuf+1 (retail's s3/s2 are &inBuf and &inBuf+1, and outBuf is rematerialised as `addiu a0,sp,40`).
   RETRY FROM THIS BODY (variants tried before this round are summarised below it): the closest body is
   B1 = pSrc advanced 0x14 per row (`pSrc[j]` for the even read, `pSrc` for the row copy) + `int rowOffset;`
   with `rowOffset = i;` as the first statement of the outer body and the odd read spelled
   `((u8*)&g_GameState)[rowOffset + j + 1]`. B1 is 84.02%, 416 bytes / 104 insns: it is the ONLY body measured
   that reproduces retail's `move $t0,$s0` outer-preheader copy of i into a caller-saved register (retail's odd
   index uses $t0), and its outer loop and row copy are retail exact, so its whole residual is that gcc derives
   the odd base from the giv (`addiu $s4,$s1,1` in the preheader, `addu $v1,$v0,$s4`) where retail
   rematerialises %hi/%lo(g_GameState+1) into $t2 per row, plus the 0xF in $s3 and outBuf in $s2. Do not bother
   with extra buffer-base locals: `pBufEven = inBuf; pBufOdd = inBuf + 1;` hoisted above the outer loop and
   copied into pIn/pInOdd per row is 82.06%, 440 bytes / 110 insns - gcc gives them $s5/$s6 on top of everything
   instead of displacing outBuf/0xF. Also differing in every variant: retail's row-copy compare is `slt` where a
   pointer compare gives `sltu` (a signed compare needs the addresses held as integers, which no realistic
   spelling here produces), and retail puts the even-pointer `addiu $a0,$a0,2` in the `bne` delay slot where ours
   leaves a nop and increments in the loop-tail slot.
   EARLIER VARIANTS (all measured, all worse; kept so they are not re-tried): both accesses spelled against the
   raw symbol (retail's %lo(g_GameState) / %lo(g_GameState+1) relocs, 104 insns, but $s4 and a 0x60 frame,
   78.08); the inner loop as a source-level do-while (gcc peels the first iteration, 121 insns, 63.40); the odd
   read through its own pointer local `pOdd = (u8*)&g_GameState + 1` indexed `pOdd[i + j]` (84.11, 420 bytes,
   105 insns, $s4 + $s3 still the extra save); `pOdd` assigned per row inside the outer loop (85.53, 408 bytes,
   102 insns, but gcc folds i into the odd base per row so the odd gets its own induction and the second addu
   disappears); and the odd written inline as `((u8*)&g_GameState + 1)[i + j]` with pRow present (82.93,
   412 bytes, 103 insns, retail's size, gcc re-folds it to pRow + 1 in $s4).
   ROUND 449 (this round): the row copy as an index loop `for (k = 0; k < 0x14; k++) pRow[k] = outBuf[k];` with
   `pRow = pSrc + i;` as the first statement of the outer body gives retail's `slt` compare but recomputes
   `addu $s1,$s2,$s0` per row and buys a fifth save ($s4 = outBuf) -> 87.17%, 408 bytes / 102 insns. A 3-minute
   permuter run (base=2651) found only bodies that merge `i` and `j` into one int (best=1976) - an infinite outer
   loop, wrong code - so the useful signal is that ONE FEWER integer pseudo moves the hand-out towards retail's,
   and both retail variables are provably live (`j / 2`'s sign fixup in a2 and the `slti $s0,0x26C` counter).
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3_b", func_8001B970);
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

    for (j = 19; j >= 0; j--) {
        ((s16*)&g_SoundVolumeController)[j - 16] = 0;
    }
    g_MenuSavedMainMenuChoice = 6;
    g_GameResumeScene = 0;
}*/




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
