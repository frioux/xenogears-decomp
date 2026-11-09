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
