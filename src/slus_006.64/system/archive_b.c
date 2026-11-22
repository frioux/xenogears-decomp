#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/memory.h"

/* 99.94%, 316 bytes = retail's exact size, 79 instructions, and an identical
   instruction STREAM. The ONE remaining difference is a real 4-BYTE BRANCH
   DISPLACEMENT, not a label name: the inner loop's back edge is `bnez v0,+0x74`
   in retail (targeting the mult/mflo hazard nop at the loop head) and
   `bnez v0,+0x78` here (targeting the `mult s2,s3`, because maspsx placed its
   hazard nop BEFORE the label). The bytes therefore differ and check_bin can
   never pass, so this is not an objdiff artifact.
   RE-CONFIRMED this round: 99.94 / 316 / 79 insns, 2 differing lines, only that
   branch. WHY NO C SHAPE MOVES IT - re-verified in the raw cc1 output
   (build/src/slus_006.64/system/archive_b.c.s): cc1 emits
     mult $20,$18 / mflo $8 / addu $17,$8,$22 / $L8: / mult $18,$19
   and maspsx `_handle_mflo_mfhi` (tools/maspsx/maspsx/__init__.py, the
   next_next_instruction-is-mult branch) appends the follower and the nop as one
   unit (`res.extend([expand_move(inst), "nop # DEBUG..."])`) and only then
   replays the collected `$L8:` line, so the label can only ever land AFTER the
   nop. The three reachable label positions, all measured: after the nop (this
   body, 99.94), at the addu (label line before the follower, 4 bytes the other
   way), and before the nops (the next-is-mult branch, which emits TWO nops and
   costs 8 bytes). Retail's build put the label between the addu and the nop -
   its assembler's mflo handler emits label-before-nop, the opposite convention
   to maspsx's (maspsx does emit label-then-nop in its OTHER handler,
   _handle_nop_before_next_instruction), so closing this needs a maspsx change,
   which is out of bounds for a unit.
   PRESET SCREENED this round: Default (2.7.2 -O2 -G8) 99.94, -O3 99.94, -G0
   99.94, 2.7.2-cdk 95.59 (with and without -G0), 2.6.0 93.54 - the preset is
   right and no version reaches it.
   Shapes already measured: for(;;)+break, for(j=i;j<nCount;j++), an empty
   do/while at the loop head (all 99.94 with this same single difference), a
   do/while around the three memcpys (95.29%/312), if/else+continue (81.78%), a
   pRow pointer (97.15%), and a permuter run (503 iterations, base=1 best=1).
   The inner loop IS a selection sort: pCompare picks the extreme element in
   [i,nCount) and three memcpys rotate it into slot i through a HeapAlloc'd temp.
   Leave as INCLUDE_ASM.
 */
/*
typedef s32 (*ArchiveCompareFn)(u8* pA, u8* pB);

void func_80037878(u8* pData, s32 nCount, s32 nSize, ArchiveCompareFn pCompare) {
    u8* pTemp;
    s32 k;
    s32 i;
    s32 j;

    pTemp = HeapAlloc(nSize, 0);
    for (i = 0; i < nCount; i++) {
        k = i;
        j = i;
        while (j < nCount) {
            if (pCompare(pData + nSize * k, pData + j * nSize) > 0) {
                k = j;
            }
            j++;
        }
        memcpy(pTemp, pData + i * nSize, nSize);
        memcpy(pData + i * nSize, pData + nSize * k, nSize);
        memcpy(pData + nSize * k, pTemp, nSize);
    }
    HeapFree(pTemp);
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_80037878);

extern volatile u32 D_80050618[3];

/* RE-MEASURED 96.00%, 20 bytes = retail EXACT size and instruction stream; the
   one residual is the address register: retail lui/addiu $t0 then sw $a0,0($t0),
   ours the same three with $v0. The brief said 99.94 (stale). cc1 emits
   la $2,D_80050618 for EVERY spelling of a lone store - probed with cc1_probe
   this round: *A, A[i], struct field A.x, (u32*)u8-array cast, pointer local,
   *((u32*)A+0), &A[0]; all la $2 - so no source shape can reach $8 (seven live
   values would have to precede it and this body has one). Non-volatile drops to
   the bare sw-through-$at 2-instruction form (77.80, 16 bytes); 2.6.0 also 96.00;
   cdk -G0 77.80 (lui $2 + sw %lo); 2.8.1 77.80. Permuter 5643 iters base=20
   best=20. So $t0 came from something outside gcc 2.7.2 source shapes (a
   hand-written/hand-assembled store or a different libc-era compiler). Leave as
   INCLUDE_ASM.
 */
/*
void func_800379B4(u32 value) {
    *D_80050618 = value;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379B4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379C8);
/* PARKED 0%, RE-MEASURED this round: 8 bytes retail against 32 (honest signature)
   and 48 (variadic). Retail is `j FontPrintf` + `nop` - the disc word at 0x281C8
   is 0x0800DC03 (opcode 2, target 0x8003700C = FontPrintf, which splat prints
   byte-swapped as `03DC0008`) - with no frame and a0 untouched, i.e. a SIBLING
   CALL that forwards the caller's varargs unchanged. That is the only shape
   consistent with the callers (archive.c, field/scripts/virtual_machine.c and
   heap_debug.c all pass extra arguments; FontPrintf is variadic), and it is not
   expressible on this target:
   - gcc 2.7.2's mips.md has no sibcall patterns (they arrive in 3.x). Probed with
     cc1_probe.sh: `void f(char* p){ FontPrintf(p); }` gives subu sp / sw ra /
     jal / lw ra / addu sp / j ra, unchanged by -foptimize-sibling-calls and
     unchanged by an __attribute__((noreturn)) declaration of the callee.
   - the variadic signature `void f(char* p, ...){ FontPrintf(p); }` is worse: the
     frame plus the a1-a3 save area.
   - gcc-2.6.0 and 2.7.2-cdk 0%, 2.8.1 (an earlier round) frame + jal too.
   No other glabel in the linked binary or in any overlay starts with `j`, so
   retail's 8 bytes are a hand-written thunk or a compiler this project lacks.
   Leave as INCLUDE_ASM.
void func_800379C8(char* pFormat, ...) {
    FontPrintf(pFormat);
}
*/

void ArchiveNoOp(void) {
}

extern StreamDataQueueEntry D_8005A1DC[];
extern void* D_8005A1E0;
extern u16 D_8005A1E4;
extern void* D_8005A1E8;
extern u16 D_8005A1EC;
extern void* D_8005A1F0;
extern void* g_BattleStageData;
/* archive.c declares two parameters; retail's callers pass a third (always 0). */
extern void ArchiveStartStreamQueue(StreamDataQueueEntry* pQueue, s32 channel, s32 arg2);

/* 99.68%, 432 bytes = retail's EXACT size and instruction count (108 insns). ONE
   residual: the second-size local and pSecond hold SWAPPED callee-saved registers.
   Retail: first size dies in the call in $a0, second size = $s2, pFirst = $s3,
   pSecond = $s1. Here: first size still $a0 and pFirst still $s3, but the second
   size takes $s1 and pSecond $s2 (7 instructions name the wrong register, positions
   all correct). That is the whole difference - no instruction is displaced.
   WHAT IS SETTLED: the first size must be its OWN local (sharing one nSize local
   makes gcc keep both values in $s2 and only scores 99.91 through an uninitialised
   read - see the warning below); the unsigned type matters (ArchiveDecodeAlignedSize
   takes an unsigned int); do-while(0) around the two alloc pairs is load-bearing.
   THE OLD 99.91 NOTE WAS UB: it wrapped the -1 arm in `if (pFirst)` with two
   identical arms, reading pFirst before it is assigned. The honest if (nIndex >=
   nCount / 2) form of that same shared-nSize body measures 99.63 (432 bytes), so
   99.91 was never reachable from shippable C. Do not port that spelling back.
   MEASURED, one line each: shared nSize local, honest arm = 99.63; separate locals
   with the second size assigned BEFORE the first HeapAlloc (this body) = 99.68, and
   the same with nSize/shared = 98.05 because gcc then emits that addiu at the top of
   the block; this body with the second size moved down to just before its own
   HeapAlloc = 99.68 as well but only with the duplicated UB arm, so it is not a
   comparable data point; first size inlined as
   ArchiveDecodeAlignedSize(nBaseIndex + 7 + nQueueIndex) = 93.88 and 420 bytes
   (the frame changes); nQueueIndex hoisted above the if = 95.22.
   Permuter: 9 min / 4934 iterations from the 98.05 body, RESULT PERMUTER_BETTER
   base=215 best=35, and the find was only the unsigned-copy spelling of the first
   size, i.e. this body. An older run (1000+1567 iterations) from the 99.91 and
   99.68 bodies returned NONE, so this is an allocator tie on priority between
   nSecondSize and pSecond, not a structural difference. Levers tried for it: the
   signedness-copy lever (int vs unsigned int on each size local - no move), and
   declaration order (no move). Next idea: make pSecond lose one reference or give
   nSecondSize one more honest use.
   g_BattleStageData and *pOut3 are assigned the SAME value (D_8005A1E8 + 4) and
   retail reloads D_8005A1E8 for the second one, so both stores stay written out. *//*
int func_800379D8(int nIndex, int nBaseIndex, int* pOut1, int* pOut2, int* pOut3) {
    StreamDataQueueEntry* pQueue;
    unsigned int nFirstSize;
    unsigned int nSecondSize;
    int nQueueIndex;
    void* pFirst;
    void* pSecond;
    int nSectionIndex;
    unsigned int nEntryIndex;
    int nResult;
    short nCount;

    nResult = 0;
    ArchiveGetArchiveOffsetIndices(&nSectionIndex, &nEntryIndex);
    ArchiveSetIndex(ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL, 3);
    HeapChangeCurrentUser(HEAP_USER_MASA, NULL);
    nCount = ArchiveDecodeSizeAbsolute(5);
    if (nIndex >= nCount / 2) {
        nResult = -1;
        *pOut1 = 0;
        *pOut2 = 0;
        *pOut3 = 0;
    } else {
        nQueueIndex = nIndex * 2;
        do {
            nFirstSize = nBaseIndex + 7;
            nSecondSize = nQueueIndex + 6;
            pFirst = HeapAlloc(ArchiveDecodeAlignedSize(nQueueIndex + nFirstSize), 1);
            HeapPinBlock(pFirst);
            pSecond = HeapAlloc(ArchiveDecodeAlignedSize(nSecondSize), 1);
        } while (0);
        HeapPinBlock(pSecond);
        pQueue = D_8005A1DC;
        pQueue->archiveIndex = nSecondSize;
        D_8005A1E0 = pSecond;
        D_8005A1E4 = nQueueIndex + 7 + nBaseIndex;
        D_8005A1E8 = pFirst;
        D_8005A1EC = 0;
        D_8005A1F0 = NULL;
        ArchiveStartStreamQueue(pQueue, 0, 0);
        *pOut1 = pSecond;
        *pOut2 = 0;
        *pOut3 = (int)D_8005A1E8 + 4;
        g_BattleStageData = (int)D_8005A1E8 + 4;
    }
    ArchiveSetIndex(nSectionIndex, nEntryIndex);
    return nResult;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379D8);
