#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/memory.h"

typedef s32 (*ArchiveCompareFn)(u8* pA, u8* pB);

/* PARKED 99.94 (round 542, un-parked and re-measured LIVE, whole first diff
   read): 316 bytes / 79 insns = retail's exact size and an identical
   instruction STREAM. The one differing line is a real 4-BYTE BRANCH
   DISPLACEMENT, not a label name: the inner loop's back edge is `bnez v0,+0x74`
   in retail (the mflo hazard nop at the loop head) and `+0x78` here (the
   `mult s2,s3`), so the bytes differ and check_bin can never pass - this is not
   an objdiff artifact.
   MECHANISM re-verified against the maspsx source: cc1 emits
     mult $20,$18 / mflo $8 / addu $17,$8,$22 / $L8: / mult $18,$19
   and in `_handle_mflo_mfhi`'s next_next-is-mult branch
   (tools/maspsx/maspsx/__init__.py) the follower is appended together with its
   nop (`res.extend([expand_move(inst), "nop  # DEBUG..."])`) while the
   collected `$L8:` line is appended only on the NEXT while-iteration, so the
   hazard nop always lands before the label. Retail's assembler used the
   opposite convention (maspsx itself emits label-then-nop in its other handler,
   _handle_nop_before_next_instruction), so only a maspsx change closes this,
   which is out of bounds for a unit. See lever c-a-loop-label-lands-after.
   The label has exactly three reachable positions: after the nop (this body,
   99.94), at the addu (wrong back edge, 4 bytes the other way), before the nops
   (the next-is-mult branch, which emits TWO nops and costs 8 bytes).
   SCREENED: Default (2.7.2 -O2 -G8) 99.94, -O3 99.94, -G0 99.94, 2.7.2-cdk
   95.59 (with and without -G0), 2.6.0 93.54 - the preset is right and no
   version reaches it.
   Loop shapes measured, each with this same single difference: for(;;)+break,
   for(j=i;j<nCount;j++), an empty do/while at the loop head. Worse: a do/while
   around the three memcpys (95.29%/312), if/else+continue (81.78%), a pRow
   pointer local (97.15%). Permuter 503 and 841 iterations, base=1 best=1 both -
   its score is dominated by branch-label renaming, so it cannot see this.
   The body IS a selection sort: pCompare picks the extreme element of
   [i,nCount) and three memcpys rotate it into slot i through a HeapAlloc'd
   temp.
   The typedef must stay LIVE C above the stub, not inside the park: while a park
   had swallowed it the body did not compile and every percentage measured for
   it came from a stale object. Do not re-hunt this one.
 */
/*
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

/* PARKED 96.00 (re-measured round 448, live body, not stale): 20 bytes = retail's
   exact size and instruction stream; the ONE residual is the register that holds
   the address - retail lui/addiu $t0 then sw $a0,0($t0), ours the same three with
   $v0. cc1 2.7.2 emits la $2,D_80050618 for every source shape, probed again this
   round with cc1_probe on six more spellings (plain *A, value copied through a
   local, a struct-typed alias read through a pointer local, a pointer local over
   the volatile array, a u32 element through a local, a mem_addr cast): f1/f2/f3/f5
   all la $2, f4/f6 fold to the bare symbol macro (the 77.80 / 16-byte form).
   Screened against lever a-call-free-single-block-temp-in-t0-is-a-compiler-trait:
   this stub is the ONLY lui $t0 in asm/slus_006.64/{non,matchings}/system/archive_b
   and it is a leaf one-block body with a single value, so the $t0 comes from the
   retail compiler's register-allocation order, not from a spelling. That the very
   next glabel in this unit (func_800379C8) is a hand-written 8-byte `j` thunk says
   the same thing: this tail of the module was not produced by our cc1.
   Already measured and dead: 2.6.0 96.00 (same la $2), cdk -G0 77.80, 2.8.1 77.80,
   -O3/-G0 on 2.7.2 no change, permuter 5643 iterations base=20 best=20.
   ROUND 461: un-parked and re-measured live, still 96.00 / 20 bytes / 5 insns,
   same single line (lui-addiu t0 vs v0). Six more spellings probed through
   tools/scripts/cc1_probe.sh: a cast-volatile store through a NON-volatile u32[3],
   a volatile s32[3] with an int parameter, "volatile u32* const p = A; *p = v",
   "volatile u32* p; p = A; *p = v" - all four emit la $2; a volatile SCALAR
   extern and any struct/offset form fold to the bare symbol macro ($at, 77.80,
   16 bytes). So the la-into-$2 shape is not a spelling choice at all. The target
   is a SINGLE word: g_FontClutData is 0x80 bytes (0x80050598..0x80050617), the
   store lands at 0x80050618 and the next symbols are g_BattleTransitionFlag
   0x8005061C and g_BattleRequestedMode 0x8005061D - so D_80050618 is a lone
   word next to the battle-transition flags, which is why Ghidra renders it as
   g_FontClutData+0x80. Both callers (MainSystemInit, FieldScriptSetTransitionParams)
   pass 0. Leave as INCLUDE_ASM: this tail (func_800379B4 .. func_800379C8) is not
   this cc1's output.
 */
// ROUND 474 re-measured (un-parked, live body, whole diff read): 96.00 / 20 bytes
// = retail's exact 5 insns, single residual lui-addiu $t0 (retail) vs $v0 (ours);
// lever a-call-free-single-block-temp-in-t0-is-a-compiler-trait names this closed
// (all three compilers in tools/ start the local-alloc scan at $2) and 2.8.1 was
// screened in an earlier round. Do not re-brief.
// ROUND 550 re-measured (un-parked, live, whole first diff read): 96.00 / 20
// bytes / 5 insns, unchanged. One new probe closes the REGISTER question for
// good: with $v0 forced busy (`u32 f(u32 v) { *A = v; return v; }`) the address
// pseudo moves to $v1, never to $t0 - `move $2,$4 / la $3,A / sw $2,0($3)` - so
// local-alloc walks {2,3,...} in order and no 5-instruction shape can reach $8.
// Retail's $t0 is its compiler's hand-out, not a spelling.
/*
void func_800379B4(u32 value) {
    *D_80050618 = value;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379B4);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379C8);
/* PARKED 0%. Retail is 8 bytes: `j FontPrintf` + `nop` - the disc word at 0x281C8
   is 0x0800DC03 (opcode 2, target 0x8003700C), no frame and a0 untouched, i.e. a
   SIBLING CALL that forwards the caller's arguments unchanged (callers in
   archive.c, field/scripts/virtual_machine.c and heap_debug.c pass up to five,
   and FontPrintf is variadic).
   RE-MEASURED (round 422, live, not stale): the variadic spelling
   `func_800379C8(char* pFormat, ...) { FontPrintf(pFormat); }` is 48 bytes /
   12 insns (the a1-a3 save area plus subu sp / sw ra / jal / lw ra / addu sp /
   j ra) and the one-argument spelling is 32 bytes / 8 insns. Both 0.00%.
   NOT EXPRESSIBLE ON THIS TOOLCHAIN: gcc 2.7.2 MIPS has no sibcall patterns -
   checked in the compiler source this round, /private/tmp/gcc-2.7.2.3/config/
   mips/mips.md has no sibcall/sib_call define_expand (neither does 2.8.1's), they
   arrive in gcc 3.x. Probed earlier with cc1_probe.sh under the unit's flags: the
   call still frames and `jal`s with -foptimize-sibling-calls and with a
   `__attribute__((noreturn))` callee declaration.
   - gcc-2.6.0 and 2.7.2-cdk 0%, gcc 2.8.1 frames + jal too (round 406).
   No other glabel in the linked binary or in any overlay starts with `j`, so
   retail's 8 bytes are a hand-written thunk or a compiler this project lacks.
   Levers: delay slot / c-a-bare-j-tail-call-thunk. Report SIBLING_CALL; a lone
   `j`-first glabel is not worth further diff runs. Leave as INCLUDE_ASM.
*/

void ArchiveNoOp(void) {
}

extern StreamDataQueueEntry D_8005A1DC[];
extern void* D_8005A1E0;
extern u16 D_8005A1E4;
extern u8* g_ArchiveStreamHeader;
extern u16 D_8005A1EC;
extern void* D_8005A1F0;
extern void* g_BattleStageData;
/* archive.c declares two parameters; retail's callers pass a third (always 0). */
extern void ArchiveStartStreamQueue(StreamDataQueueEntry* pQueue, s32 channel, s32 arg2);

/* The bare block below is load-bearing: written as `do { ... } while (0)` gcc
   swaps $s1/$s2 between nSecondSize and pSecond (99.68%, same instruction stream). */
int ArchiveStartStreamedFilePair(int nIndex, int nBaseIndex, void** pOut1, void** pOut2, void** pOut3) {
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
        *pOut1 = NULL;
        *pOut2 = NULL;
        *pOut3 = NULL;
    } else {
        nQueueIndex = nIndex * 2;
        {
            nFirstSize = nBaseIndex + 7;
            pFirst = HeapAlloc(ArchiveDecodeAlignedSize(nQueueIndex + nFirstSize), 1);
            HeapPinBlock(pFirst);
            nSecondSize = nQueueIndex + 6;
            pSecond = HeapAlloc(ArchiveDecodeAlignedSize(nSecondSize), 1);
        }
        HeapPinBlock(pSecond);
        pQueue = D_8005A1DC;
        pQueue->archiveIndex = nSecondSize;
        D_8005A1E0 = pSecond;
        D_8005A1E4 = nQueueIndex + 7 + nBaseIndex;
        g_ArchiveStreamHeader = pFirst;
        D_8005A1EC = 0;
        D_8005A1F0 = NULL;
        ArchiveStartStreamQueue(pQueue, 0, 0);
        *pOut1 = pSecond;
        *pOut2 = NULL;
        *pOut3 = g_ArchiveStreamHeader + 4;
        g_BattleStageData = g_ArchiveStreamHeader + 4;
    }
    ArchiveSetIndex(nSectionIndex, nEntryIndex);
    return nResult;
}
