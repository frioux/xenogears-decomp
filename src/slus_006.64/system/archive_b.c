#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/memory.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_80037878);
/* 99.94%, 316 bytes = retail's exact size and instruction stream. The ONE
   remaining difference is a real 4-BYTE BRANCH DISPLACEMENT, not a label name:
   the inner loop's back edge is `bnez v0,+0x74` in retail (targeting the
   mult/mflo hazard nop, which aspsx placed AFTER the loop label) and
   `bnez v0,+0x78` here (targeting the `mult s2,s3`, because maspsx placed its
   hazard nop BEFORE the label). The bytes therefore differ and check_bin can
   never pass, so this is not an objdiff artifact.
   WHY NO C SHAPE CAN MOVE IT - verified in the raw cc1 output
   (build/src/slus_006.64/system/archive_b.c.s): cc1 emits
     mult $20,$18 / mflo $8 / addu $17,$8,$22 / $L8: / mult $18,$19
   and maspsx's _handle_mflo_mfhi (tools/maspsx, the `next_next_instruction is
   mult` branch) emits, for the mflo LINE, [addu, nop] as a unit and only then
   replays the `$L8:` line it collected, giving addu / nop / $L8: / mult. The
   nop is appended in the same res.extend as the instruction following mflo, so
   the label can only ever land after it; putting the label before the addu (or
   removing the addu) moves the label to the addu, and the other maspsx branch
   (next IS a mult) emits two nops and 8 extra bytes. nop_mflo_mfhi off (aspsx <
   2.30) drops the nop and 4 bytes. Shapes already measured: for(;;)+break,
   for(j=i;j<nCount;j++), an empty do/while at the loop head (all 99.94% with
   this same single difference), a do/while around the three memcpys
   (95.29%/312), if/else+continue (81.78%), a pRow pointer (97.15%), and a
   permuter run (503 iterations, base=1 best=1). Leave as INCLUDE_ASM.
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
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379B4);
/* 96.00%: byte-identical size (20) and instruction stream, including the
   address-in-a-register form. The ONE remaining difference is that register:
   retail is `lui/addiu $t0` then `sw $a0,0($t0)`, ours is the same three
   instructions with `$v0`. Dropping `volatile` (or making the global a scalar
   instead of an array) puts the store back to the `sw $4,sym` macro form through
   $at, 77.80%; array decay plus `volatile` gets gcc to `la` but its allocator
   always hands a lone forced-address pseudo $v0 ($2 is the first entry of the
   register order, and nothing else in this 5-instruction body competes for it).
   Not a preset: 96.00 under gcc-2.6.0 and -O3, 77.80 under 2.7.2-cdk (with and
   without -G0, which emits `lui $2` + `sw $4,%lo` and no addiu), 77.80 under
   gcc-2.8.1 (same `lui $2` + `sw %lo($2)` form). decomp-permuter from this body:
   5643 iterations, base=20 best=20, nothing better. Everything else tried:
   pointer local, volatile-pointer local, `register volatile` local, a store
   through a `(volatile u32*)` cast, element [0] instead of `*ptr`, and an s32
   return type to move $v0 out of the way.
extern volatile u32 D_80050618[3];

void func_800379B4(u32 value) {
    *D_80050618 = value;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379C8);
/* 0%: retail is a bare `j FontPrintf` plus a nop delay slot and nothing else, so
   it is a sibling call of a one-line printf wrapper (heap_debug.c stores it in
   g_HeapDebugPrintfFn, archive.c and field/scripts/virtual_machine.c call it with
   varargs). No C reproduces it with the compilers this repo has: gcc 2.7.2 emits
   a 32-byte frame + jal for `void f(char*) { FontPrintf(pFormat); }` and a
   48-byte varargs version, gcc-2.6.0 is 0.0% too, and gcc-2.8.1 still emits the
   frame + jal (its mips.md has no sibcall patterns, which arrive in 3.x). A
   non-prototype `extern void FontPrintf();` does not help either. It is also the
   ONLY function in the whole linked binary and every overlay whose first
   instruction is `j` (grep over every glabel in asm/), so retail's bytes are a
   hand-written thunk or a compiler this project does not have. Leave as
   INCLUDE_ASM.
void func_800379C8(char* pFormat) {
    FontPrintf(pFormat);
}
*/

void ArchiveNoOp(void) {
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379D8);

/* 98.99%, 432 bytes = retail's exact instruction count (108 insns). The queue
   store is SOLVED: with D_8005A1DC declared as a StreamDataQueueEntry[] and the
   first field stored through the pointer taken for the func_80029AFC call, gcc
   shares the address and emits retail's `sh $s2,0($a0)` (a local
   `StreamDataQueueEntry*` over three separate 8-byte symbols does not: cse folds
   the store back to the symbol and the bytes come out `sh $sN,0($at)`; that was
   round 194's 98.01%). Entries 1 and 2 must still be written as the absolute
   D_8005A1E4/D_8005A1EC and the pData fields as D_8005A1E0/E8/F0, because a
   `pQueue[1]` access renders the relocation as D_8005A1DC+0x8, which objdiff
   never scores as retail's D_8005A1E4.
   The do-while(0) around the two HeapAlloc/HeapPinBlock pairs is load-bearing
   (found by the permuter in round 194): it is what puts the size temp in $s0 and
   pFirst/pSecond in retail's registers.
   REMAINING WALL - one allocator tie, everything else follows from it: retail
   keeps the doubled index in $s0 (so the size temp is $s2, pFirst $s3, and the
   second size sum is computed IN PLACE on the index: `addiu s0,s0,7; addu
   s0,s0,s4`). We hold the index in $s3, so the temp lands in $s0, pFirst in $s2,
   and that sum goes to $v0. Two permuter runs from this body (1547 and 1780
   iterations) could not move it; they contributed only the `nFirstSize =
   nBaseIndex + 7;` local (which keeps the +7 off the index and is why this body
   is 98.99% and not 98.71%) and the `(pQueue + 0)` spelling.

/*
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern int ArchiveDecodeSizeAbsolute(int entryIndex);
extern void func_80029AFC(StreamDataQueueEntry* pQueue, int arg1, int arg2);

extern StreamDataQueueEntry D_8005A1DC[];
extern void* D_8005A1E0;
extern u16 D_8005A1E4;
extern u16 D_8005A1EC;
extern void* D_8005A1E8;
extern void* D_8005A1F0;
extern void* g_BattleStageData;

int func_800379D8(int nIndex, int nBaseIndex, int* pOut1, int* pOut2, int* pOut3) {
    StreamDataQueueEntry* pQueue;
    int nFirstSize;
    void* pFirst;
    void* pSecond;
    int nSectionIndex;
    int nEntryIndex;
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
        nIndex *= 2;
        do {
            nFirstSize = nBaseIndex + 7;
            pFirst = HeapAlloc(ArchiveDecodeAlignedSize(nIndex + nFirstSize), 1);
            HeapPinBlock(pFirst);
            pSecond = HeapAlloc(ArchiveDecodeAlignedSize(nIndex + 6), 1);
        } while (0);
        HeapPinBlock(pSecond);
        pQueue = D_8005A1DC;
        (pQueue + 0)->archiveIndex = nIndex + 6;
        D_8005A1E0 = pSecond;
        D_8005A1E4 = nIndex + 7 + nBaseIndex;
        D_8005A1E8 = pFirst;
        D_8005A1EC = 0;
        D_8005A1F0 = NULL;
        func_80029AFC(pQueue, 0, 0);
        *pOut1 = pSecond;
        *pOut2 = 0;
        *pOut3 = (int)D_8005A1E8 + 4;
        g_BattleStageData = (int)D_8005A1E8 + 4;
    }
    ArchiveSetIndex(nSectionIndex, nEntryIndex);
    return nResult;
}
*/
