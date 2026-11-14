#include "common.h"
#include "system/memory.h"
#include "psyq/memory.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_80037878);
/* 99.94%: byte-identical size (316) and instruction stream; the ONLY difference
   is the inner loop's back-edge target: retail's .L800378EC label sits on the
   maspsx mult/mflo hazard nop and branches to +0x74, ours lands after that nop
   (+0x78). Same for a `while` inner loop and for the pRow-pointer spelling
   (97.15%). Everything else is exact, including the fresh `mult` for i*nSize
   (a pCur local or an nRowOffset temp instead makes gcc strength-reduce it into
   an accumulator, which is what the earlier 79.6% was).
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
/* 0%: retail is a bare `j FontPrintf` plus a nop delay slot and nothing else.
   Not a variadic problem: with a non-prototype `extern void FontPrintf();`
   (system/memory.h dropped so the variadic decl cannot conflict) gcc 2.7.2 still
   emits the 24-byte frame + jal, so this MIPS port does not emit sibling calls
   at all. No other function in the linked binary or any overlay starts with
   `j <name>` either, so retail's `j` is hand-written asm (or a compiler we do
   not have) and no C can reproduce it.
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
