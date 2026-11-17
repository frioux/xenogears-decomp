#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/memory.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive_b", func_80037878);
/* 99.94%: byte-identical size (316) and instruction stream; the ONLY difference
   is the inner loop's back-edge target: retail's .L800378EC label sits on the
   maspsx mult/mflo hazard nop and branches to +0x74, ours lands after that nop
   (+0x78). Same for a `while` inner loop and for the pRow-pointer spelling
   (97.15%). Everything else is exact, including the fresh `mult` for i*nSize
   (a pCur local or an nRowOffset temp instead makes gcc strength-reduce it into
   an accumulator, which is what the earlier 79.6% was).
   WALL, and the assembler is the reason: maspsx's _handle_mflo_mfhi
   (tools/maspsx/maspsx/__init__.py, the `next_next_instruction is mult` branch)
   copies the instructions after mflo, appends the hazard `nop`, and only THEN
   prints the following `$L` label, so `--print-output` shows
   "addu / nop / $L8: / mult"; retail's aspsx printed the label first ("$L8: / nop
   / mult", label at 0x800378EC). No C shape can move that label: for(;;)+break,
   for(j=i;j<nCount;j++), an empty do/while at the loop head and a do/while around
   the three memcpys were all tried (the first three 99.94% with this same single
   difference; the memcpy do/while breaks the outer-loop rotation, 95.29%/312
   bytes; an if/else+continue form is 81.78%). permuter from this body: 503
   iterations, base=1 best=1. gcc itself emits the label on the `mult`, and
   maspsx inserts its nop before any label that follows a mflo window, so the only
   asmasp-version knob is nop_mflo_mfhi (off below aspsx 2.30), which would drop
   the nop and the 4 bytes with it. Leave as INCLUDE_ASM.
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
/* 97.82%, 436 bytes (exactly retail's size). Two things are left, and neither is
   a missing statement:
   1. A four-way callee-saved register rotation: retail allocates the archive
      index to $s0 and the three block locals to $s3/$s2/$s1, ours allocates the
      index to $s3 and the locals to $s2/$s1/$s0 - i.e. exactly retail's
      assignment shifted by one, with the index taking the top instead of the
      bottom register. gcc 2.7.2's global.c priority (log2(n_refs)*n_refs /
      live_length) ranks the long-lived index below the short-lived pointers; ten
      declaration/expression orders give the same 97.82% (pointer decls first,
      inline initializer, s32 instead of int, `nIndex + 6` recomputed instead of
      a local, locals scoped to the else block), as do gcc-2.6.0 and -O3.
   2. The queue struct's first field: retail stores D_8005A1DC through the $a0
      that also carries &D_8005A1DC into func_80029AFC (one lui/addiu pair for
      both). Taking the address into a pointer local and storing
      pQueue->archiveIndex through it does make gcc share the lui/addiu (that is
      what got the body from 440 to retail's 436 bytes), but gcse/cprop then
      folds the register back to the SYMBOL for the store, so the bytes come out
      as `sh $s1,0($at)` instead of retail's `sh $s1,0($a0)` - a 2.7.2/2.6.0
      codegen difference, not a source one. cdk -G0 makes it worse overall
      (84.95%).
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern int ArchiveDecodeSizeAbsolute(int entryIndex);
extern void func_80029AFC(StreamDataQueueEntry* pQueue, int arg1, int arg2);

extern u16 D_8005A1DC;
extern void* D_8005A1E0;
extern u16 D_8005A1E4;
extern void* D_8005A1E8;
extern u16 D_8005A1EC;
extern void* D_8005A1F0;
extern void* g_BattleStageData;

int func_800379D8(int nIndex, int nBaseIndex, int* pOut1, int* pOut2, int* pOut3) {
    StreamDataQueueEntry* pQueue;
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
        pQueue = (StreamDataQueueEntry*)&D_8005A1DC;
        pQueue->archiveIndex = nIndex + 6;
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
