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
// ROUND 459 REWRITE: best 89.50%, 404 bytes, 101 insns (retail 412 / 103). Body below is the runner-up B1 shape
//    (pRow advances 0x14, even read pRow[j], odd read ((u8*)&g_GameState)[rowOffset + j + 1] with rowOffset = i copied at the
//    top of the row body, row copy via outBuf[k++] into pRow) PLUS the row copy indexed by k, which frees outBuf's address and
//    removes the fifth save B1 paid (B1 alone: 84.02 / 416 / 104). Retail's row preheader is 8 insns, ours 7: retail keeps
//    $s0 = i, $s1 = pRow, $s2/$s3 = inBuf+1 / inBuf and rematerialises BOTH the odd base (lui/addiu %hi/%lo(g_GameState+1)
//    into caller-saved $t2) and 0xF (li $t1 per row); this body keeps i, pRow, 0xF ($s2) and s1+1 ($s3) instead and
//    rematerialises the inBuf pair (addiu a2,sp,16 / addiu a1,sp,17 per row where retail has move a2,s2 / move a1,s3).
//    Measured dead this round, each one edit from that body: odd index written with i instead of rowOffset (drops retail's
//    move t0,s0, 88.60 / 400 / 100); outer loop as do{}while(i < 0x26C) (byte-identical 89.50); the fully typed form
//    pSrc = (u8*)g_GameState.strings + pRow advancing, bounds as sizeof(g_GameState.strings) / sizeof(StringEntry) (the
//    size_t bound turns retail's slti into sltiu, cse folds pSrc onto pRow so the odd becomes 1(v1), buys $s4 = outBuf:
//    86.44 / 416 / 104). Consistent with the round-433/456 lever (g-which-four-invariants-are-hoisted): the fold of
//    symbol+1 onto the even read's LSR pointer is cse/combine's, and the arbitration between {0xF, odd base, outBuf} and
//    {inBuf, inBuf+1} for the four callee-saved slots has not been reachable from source in ~12 spellings over three
//    rounds. Residual counted: 1 insn in the row preheader (lui/addiu vs addiu s3,s1,1), the s3 save/restore pair (2),
//    retail's bne delay slot addiu a0,a0,2 where ours has nop, and sltu vs slt on the row-copy compare. Tail (frame,
//    call tail, volume loop, sb %gp_rel) is retail exact.
// ROUND 460: best 90.07%, 420 bytes, 105 insns (retail 412 / 103). Body below is the ROUND 459 shape with two edits:
//    (a) the inBuf pair written INDEXED (inBuf[j] / inBuf[j + 1] instead of pIn/pInOdd pointers) - this puts inBuf and
//    inBuf + 1 in the remaining callee-saved slots exactly as retail does (addiu s3,sp,16 / addiu s2,sp,17 in the outer
//    preheader, move a1,s3 / move a2,s2 per row) and that pressure pushes 0xF out into a per-row li t1 and outBuf's
//    address into the copy loop's own addiu, so inBuf, inBuf + 1, 0xF and outBuf now all sit where retail has them;
//    (b) the row-copy bound compared through (int) casts, which turns sltu into retail's slt (89.53 -> 90.07) - the slt
//    half of the round-418 lever g-two-source-inits, now measured on a walk with no index loop at all.
//    THE ONE NAMED RESIDUAL is the odd byte's base: gcc cse folds &g_GameState + 1 onto the register that still holds
//    the symbol at the outer preheader (addiu s2,s1,1, hoisted, executed once) where retail rematerialises
//    lui/addiu %hi/%lo(g_GameState + 1) into caller-saved t2 per ROW. That single hoist is the whole 2-insn / 8-byte
//    gap: it costs the fifth save pair (s4) and it replaces retail's 2-insn per-row preheader. So the target is a
//    spelling in which no register holds the base when the row preheader runs; retail's even read cannot supply it
//    (its odd address is (i + j) + a fresh %hi/%lo, not pRow + j), yet every body here has the symbol in s1 there.
//    Tried dead this round: odd index written ((u8*)&g_GameState + rowOffset + 1)[j] (byte-identical, gcc
//    canonicalises it back); (int) casts with the pair left as pIn/pInOdd pointers (89.53, unchanged). Tail (frame
//    0x58, call tail, volume loop, sb %gp_rel) is retail exact.
/*
void func_8001B970(void) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    u8* pRow;
    u8* pOut;
    u8* pDst;
    void* pBuffer;
    int i;
    int j;
    int k;
    int rowOffset;

    ArchiveSetIndex(ARCHIVE_DIR_MENUS, 0);
    HeapChangeCurrentUser(HEAP_USER_HIG, 0);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    ArchiveReadFileToBuffer(3, pBuffer, 0, 0x80);
    ArchiveCdDataSync(0);
    memmove(&g_GameState, pBuffer, 0x2358);
    HeapFree(pBuffer);

    pRow = (u8*)&g_GameState;
    for (i = 0; i < 0x26C; i += 0x14) {
        rowOffset = i;
        for (j = 0; j < 0x14; j += 2) {
            inBuf[j] = pRow[j];
            inBuf[j + 1] = ((u8*)&g_GameState + rowOffset + 1)[j];
            if (pRow[j] == 0xF && ((u8*)&g_GameState + rowOffset + 1)[j] == 0) {
                break;
            }
        }
        SystemDecodeGlyphCodesN((u16*)inBuf, outBuf, j / 2);
        k = 0;
        pDst = pRow;
        do {
            *pDst++ = outBuf[k++];
        } while ((int)pDst < (int)(pRow + 0x14));
        pRow += 0x14;
    }

    for (j = 19; j >= 0; j--) {
        ((s16*)&g_SoundVolumeController)[j - 16] = 0;
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
