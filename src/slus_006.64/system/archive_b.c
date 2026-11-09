#include "common.h"
#include "system/memory.h"
#include "psyq/memory.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_80037878);
/* 79.6%: structure matches, but gcc 2.7.2 strength-reduces the row base
   i*nSize into an induction accumulator, which costs a register and the
   original's frame (0x40 vs 0x48). The original keeps a real mult.
typedef s32 (*ArchiveCompareFn)(u8* pA, u8* pB);

void func_80037878(u8* pData, s32 nCount, s32 nSize, ArchiveCompareFn pCompare) {
    u8* pTemp;
    u8* pRow;
    u8* pCur;
    s32 i;
    s32 j;
    s32 k;
    s32 nRowOffset;

    pTemp = HeapAlloc(nSize, 0);
    for (i = 0, k = 0, pCur = pData; i < nCount; i++, pCur += nSize) {
        nRowOffset = i * nSize;
        for (j = i, pRow = pData + nRowOffset; j < nCount; j++, pRow += nSize) {
            if (pCompare(pData + nSize * k, pRow) > 0) {
                k = j;
            }
        }
        memcpy(pTemp, pCur, nSize);
        memcpy(pCur, pData + k * nSize, nSize);
        memcpy(pData + k * nSize, pTemp, nSize);
        k = i;
    }
    HeapFree(pTemp);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379B4);
/* 77.8%: a store to the .sdata global D_80050618 (12 bytes there, per
   data/3F290.sdata.s). The original computes the address with an explicit
   lui/addiu pair in $t0, i.e. the address went into a register; every C
   spelling tried (plain extern of u32 / u32[3] / a pointer local / -G0 /
   gcc 2.6.0) emits the `sw $4,sym` macro form instead, which maspsx leaves
   for gas to expand into `lui $at` + `sw $4,0($at)`.
extern u32 D_80050618[3];

void func_800379B4(u32 value) {
    u32* pDst = D_80050618;
    *pDst = value;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379C8);
/* 0%: retail is a bare `j FontPrintf`. FontPrintf is variadic (system/memory.h),
   and gcc 2.7.2 refuses to tail-call a variadic callee, so every spelling that
   takes `...` or forwards a fixed list emits a `subu sp,24` frame plus jal. A
   non-variadic local prototype for FontPrintf is impossible without touching
   the header or casting the callee (which turns the call into la+jalr).
void func_800379C8(char* pFormat) {
    FontPrintf(pFormat);
}
*/

void func_800379D0(void) {
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_800379D8);
/* 97.82%: every instruction matches but one register permutation. The
   original puts the first parameter (the archive index) in $s0 and shifts
   pFirst/pSecond down one ($s3/$s2/$s1); gcc puts the first-allocated pointer
   in $s0 and the index in $s3. Ten declaration/expression orders give the same
   97.82% (pointer decls first, inline initializer, s32 instead of int, `nIndex
   + 6` recomputed instead of a local, locals scoped to the else block), so this
   is global.c's allocno-order tie, not the source. Second difference: the
   original stores to D_8005A1DC through the $a0 that also carries the queue
   address into func_80029AFC (one lui/addiu pair for both), gcc materialises
   the store separately with $at (2 extra insns, 436 vs 440 bytes).
   Needs include/system/archive.h for StreamDataQueueEntry and
   ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL.
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern int ArchiveDecodeSizeAbsolute(int entryIndex);
extern void func_80029AFC(StreamDataQueueEntry* pQueue, int arg1, int arg2);

extern u16 D_8005A1DC;
extern void* D_8005A1E0;
extern u16 D_8005A1E4;
extern void* D_8005A1E8;
extern u16 D_8005A1EC;
extern void* D_8005A1F0;
extern void* D_800658C8;

int func_800379D8(int nIndex, int nBaseIndex, int* pOut1, int* pOut2, int* pOut3) {
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
        pFirst = HeapAlloc(ArchiveDecodeAlignedSize(nIndex + 7 + nBaseIndex), 1);
        HeapPinBlock(pFirst);
        pSecond = HeapAlloc(ArchiveDecodeAlignedSize(nIndex + 6), 1);
        HeapPinBlock(pSecond);
        D_8005A1DC = nIndex + 6;
        D_8005A1E0 = pSecond;
        D_8005A1E4 = nIndex + 7 + nBaseIndex;
        D_8005A1E8 = pFirst;
        D_8005A1EC = 0;
        D_8005A1F0 = NULL;
        func_80029AFC((StreamDataQueueEntry*)&D_8005A1DC, 0, 0);
        *pOut1 = pSecond;
        *pOut2 = 0;
        *pOut3 = (int)D_8005A1E8 + 4;
        D_800658C8 = (int)D_8005A1E8 + 4;
    }
    ArchiveSetIndex(nSectionIndex, nEntryIndex);
    return nResult;
}
*/
