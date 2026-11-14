#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"
#include "psyq/libcd.h"
#include "psyq/libgpu.h"

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x12];
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x28];
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ u8 unk50[0x30];
} Misc15Slot; // Size: 0x80

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ s32 unk8;
    /* 0xC */ u8 unkC[0x18];
} Misc15CdRequest; // Size: 0x24

typedef struct {
    /* 0x0 */ u32 unk0[4];
} Misc15Block16; // Size: 0x10

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ u16 unkA;
    /* 0xC */ s16 unkC;
    /* 0xE */ s16 unkE;
    /* 0x10 */ MATRIX unk10;
} Misc15Xform; // Size: 0x30


extern Misc15Slot *D_8009BE24;
extern void *D_8009C184[];
extern MATRIX D_8009A180;
extern MATRIX D_8009D534;
extern u32 D_8009C580[];
extern s32 D_8009C5BC;
extern s32 D_8009C618;
extern u8 *D_8009BC3C;
extern u8 *D_8009BCB4;
extern s32 D_8009BCB8;
extern s32 D_8009BD2C;
extern s32 D_8009CD44;
extern void *D_8009D788[16];
extern void *D_8009C624[16];
extern void func_800981C8(s32);
extern void func_80097DC0(void);

extern void func_800976C8(void);
extern CdlLOC D_8009CEBC;
extern u8 *D_8009D3BC;
extern s32 D_8009D7F4;
extern s32 D_8009D614;
extern s32 D_8009D56C;
extern s32 D_8009CEB8;
extern s32 D_8009C590;
extern s32 D_8009BE48;
extern s32 D_8009CCB0;
extern s32 D_8009CCA8;
extern s32 D_8009CCA0;
extern void func_80096A6C(void);
extern u32 ArchiveGetDebugTable(void);
extern s32 func_800968E0(void);
extern void WorldMapReadFileEntries(void *p);
extern s32 D_8009BBB4;
extern s32 D_8009BBB8;
extern s32 D_8009BBBC;
extern s16 D_8009C838;
extern s16 D_8009C83A;
extern s16 D_8009C83C;

s32 func_800967E4(void) {
    s32 result = 0;
    u32 first;
    u32 second;

    first = ArchiveGetDebugTable();
    second = ArchiveGetDebugTable();
    if ((first == 0) | (second == 0xFFFFFFFF)) {
        result = func_800968E0();
        if (result == 0) {
            if (D_8009D788[D_8009BCB8] != NULL) {
                func_8009699C((Misc15CdRequest *)D_8009D788[D_8009BCB8]);
            }
        }
    } else {
        if (D_8009C624[D_8009BCB8] != NULL) {
            WorldMapReadFileEntries(D_8009C624[D_8009BCB8]);
            D_8009C624[D_8009BCB8] = NULL;
            D_8009BCB8 = (D_8009BCB8 + 1) & 0xF;
        }
    }
    return result;
}
// Best: 99.79% -- blocked, not a match problem: this switch's table is
// jtbl_80070CA0 in asm/world_map/data/0.rodata.s, whose entries are
// .word .L80096910/.L80096918/.L80096950/.L80096958. Converting the function
// deletes those local labels from the asm and the overlay stops linking
// (undefined reference to .L80096910 at .rodata+0x11b0). The table would have
// to move to C with the function, and a C definition cannot name code labels.
/*
s32 func_800968E0(void) {
    switch (D_8009CD44) {
    case 0:
        return 0;
    case 1:
    case 2:
    case 3:
        return 1;
    case 4:
        if (--D_8009BD2C == 0) {
            D_8009CD44++;
        }
        return 1;
    case 5:
        D_8009CD44 = 0;
        D_8009D788[D_8009BCB8] = 0;
        D_8009BCB8 = (D_8009BCB8 + 1) & 0xF;
        return 2;
    }
    return 3;
}
 */
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800968E0);
void func_8009699C(Misc15CdRequest *p) {
    CdlLOC *pLoc;
    s32 lba;
    u32 size;

    lba = p->unk0;
    D_8009CD44 = 1;
    pLoc = &D_8009CEBC;
    /* retail keeps this dead store: the volatile store blocks gcc's elimination */
    *(u8 *volatile *)&D_8009D3BC = (u8 *)p;
    D_8009D3BC = (u8 *)p + 0xC;
    size = p->unk4;
    D_8009BE48 = 0;
    D_8009CCB0 = 0;
    D_8009CCA8 = 0;
    D_8009CCA0 = 0;
    D_8009D7F4 = lba;
    D_8009D614 = lba;
    D_8009D56C = (size + 0x7FF) >> 11;
    D_8009CEB8 = p->unk4;
    D_8009C590 = p->unk8;
    CdIntToPos(lba, pLoc);
    CdSyncCallback(func_80096A6C);
    CdControlF(CdlSetloc, (u8 *)pLoc);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096A6C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096C0C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096F18);
// Best: 59.80%. The structure is decoded and every call/argument is right (the
// scratchpad rot vector at 0x1F8000A0, the matrix at 0x1F8000F0, the vectors at
// 0x1F800000/0x1F800010, RotMatrixYXZ / ApplyMatrixLV / RotMatrixYXZ /
// ApplyMatrix, and the >>12 and ->>12 folds). What differs is the frame: gcc
// CSEs 0x1F8000A0/0xA2/0xA4 into the callee-saved s3/s4/s5, so the frame is
// 0x30 with six saved registers, while the original keeps only 0xA0 in s3 and
// reaches 0xA2/0xA4 with a fresh lui $at plus maspsx's low-half offset fold
// (sh $v0,0xA2($at)) -- a 4-byte size difference and a different allocation.
/*
void func_80096F18(Misc15Xform *pOut, u8 *pSrc, s32 height, u8 *pAngles) {
    pOut->unk8 = 0;
    pOut->unkA = *(s32 *)(pSrc + 4) >> 12;
    pOut->unkC = 0;
    *(u16 *)0x1F8000A0 = *(u16 *)(pAngles + 0);
    *(u16 *)0x1F8000A2 = *(u16 *)(pAngles + 2);
    *(u16 *)0x1F8000A4 = 0;
    RotMatrixYXZ((SVECTOR *)0x1F8000A0, (MATRIX *)0x1F8000F0);
    *(s32 *)0x1F800000 = 0;
    *(s32 *)0x1F800004 = 0;
    *(s32 *)0x1F800008 = -(height >> 12);
    ApplyMatrixLV((MATRIX *)0x1F8000F0, (VECTOR *)0x1F800000, (VECTOR *)0x1F800010);
    pOut->unk0 = *(s32 *)0x1F800010;
    pOut->unk2 = pOut->unkA + *(s32 *)0x1F800014;
    pOut->unk4 = *(s32 *)0x1F800018;
    *(u16 *)0x1F8000A0 = 0;
    *(u16 *)0x1F8000A2 = *(u16 *)(pAngles + 2);
    *(u16 *)0x1F8000A4 = *(u16 *)(pAngles + 4);
    RotMatrixYXZ((SVECTOR *)0x1F8000A0, (MATRIX *)0x1F8000F0);
    *(u16 *)0x1F8000A0 = 0;
    *(u16 *)0x1F8000A2 = -0x1000;
    *(u16 *)0x1F8000A4 = 0;
    ApplyMatrix((MATRIX *)0x1F8000F0, (SVECTOR *)0x1F8000A0, (VECTOR *)((u8 *)pOut + 0x10));
}
*/
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
} Misc15Rot; // Size: 0x6

void func_80097070(MATRIX *pMtx, Misc15Rot *pRot) {
    u16 *pAngles = (u16 *)pMtx;

    if ((pAngles[6] | pAngles[8]) == 0) {
        return;
    }
    pRot->unk2 = ratan2((s16)pAngles[6], (s16)pAngles[8]) & 0xFFF;
    *(MATRIX *)0x1F8000F0 = *pMtx;
    *(MATRIX *)0x1F800110 = D_8009A180;
    RotMatrixY(pRot->unk2, (MATRIX *)0x1F800110);
    MulMatrix0((MATRIX *)0x1F8000F0, (MATRIX *)0x1F800110, (MATRIX *)0x1F800130);
    pRot->unk0 = ratan2(*(s16 *)0x1F80013A, *(s16 *)0x1F800138);
    *(MATRIX *)0x1F800110 = D_8009A180;
    RotMatrixX(pRot->unk0, (MATRIX *)0x1F800110);
    MulMatrix0((MATRIX *)0x1F800130, (MATRIX *)0x1F800110, (MATRIX *)0x1F8000F0);
    pRot->unk4 = -ratan2(*(s16 *)0x1F8000F6, *(s16 *)0x1F8000F8);
}
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
    /* 0xC */ s16 unkC;
    /* 0xE */ s16 unkE;
    /* 0x10 */ VECTOR unk10;
} Misc15Frame; // Size: 0x1C

extern s16 D_8009BD38;
extern s16 D_8009BD3A;
extern s16 D_8009BD3C;
extern MATRIX D_8009C808;
extern s16 D_8009C80A;
extern s16 D_8009C80C;
extern s16 D_8009C80E;
extern s16 D_8009C810;
extern s16 D_8009C812;
extern s16 D_8009C814;
extern s16 D_8009C816;
extern s16 D_8009C818;

// Best: 83.39%. Every call and its arguments match (three VectorNormal /
// OuterProduct12 pairs on the scratchpad frames, the nine halfword stores into
// D_8009C808..D_8009C818, the negated SVECTOR, the matrix copy and
// ApplyMatrix/TransMatrix). What differs is cc1's scheduling of the nine 32-bit
// loads against the nine halfword stores: retail batches all nine lw before the
// first sh and keeps the three lhu/negu/sh of the SVECTOR after them, while this
// compile needs two more temporaries (t4/t5), interleaves the negates, and reads
// each subtraction pair in the opposite order. Not a type or frame problem.
/*
void func_80097244(Misc15Frame *p) {
    MATRIX *pMtx;

    ((VECTOR *)0x1F800000)->vx = p->unk8 - p->unk0;
    ((VECTOR *)0x1F800000)->vy = p->unkA - p->unk2;
    ((VECTOR *)0x1F800000)->vz = p->unkC - p->unk4;
    VectorNormal((VECTOR *)0x1F800000, (VECTOR *)0x1F800030);
    OuterProduct12((VECTOR *)0x1F800030, &p->unk10, (VECTOR *)0x1F800000);
    VectorNormal((VECTOR *)0x1F800000, (VECTOR *)0x1F800010);
    OuterProduct12((VECTOR *)0x1F800030, (VECTOR *)0x1F800010, (VECTOR *)0x1F800000);
    VectorNormal((VECTOR *)0x1F800000, (VECTOR *)0x1F800020);
    pMtx = &D_8009C808;
    pMtx->m[0][0] = ((VECTOR *)0x1F800010)->vx;
    D_8009C80A = ((VECTOR *)0x1F800010)->vy;
    D_8009C80C = ((VECTOR *)0x1F800010)->vz;
    D_8009C80E = ((VECTOR *)0x1F800020)->vx;
    D_8009C810 = ((VECTOR *)0x1F800020)->vy;
    D_8009C812 = ((VECTOR *)0x1F800020)->vz;
    D_8009C814 = ((VECTOR *)0x1F800030)->vx;
    D_8009C816 = ((VECTOR *)0x1F800030)->vy;
    D_8009C818 = ((VECTOR *)0x1F800030)->vz;
    ((SVECTOR *)0x1F800040)->vx = -(u16)p->unk0;
    ((SVECTOR *)0x1F800040)->vy = -(u16)p->unk2;
    ((SVECTOR *)0x1F800040)->vz = -(u16)p->unk4;
    *(MATRIX *)0x1F800048 = *pMtx;
    ApplyMatrix((MATRIX *)0x1F800048, (SVECTOR *)0x1F800040, (VECTOR *)0x1F800000);
    TransMatrix(pMtx, (VECTOR *)0x1F800000);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097244);

// Best: 95.06%. Instruction-identical and the same 0x20 frame; the only
// difference is the callee-saved numbering: retail has &D_8009C808 in s0, the
// 0x1F8000F0 matrix in s1 and the parameter in s2, this compile has p in s0,
// &D_8009C808 in s1 and 0x1F8000F0 in s2 (so objdiff reports 95.06 and the bytes
// differ). Five shapes tried: no pointer local, a local pointer, three separate
// pointers for 0x1F8000F0/110/130, a u16* view of the parameter, and an extra
// dummy local to shift the declaration order - all land on the same allocation.
// gcc-2.7.2-cdk gives 81.27, so the preset is right.
/*
void func_80097440(Misc15Frame *p) {
    MATRIX *pMtx = (MATRIX *)0x1F8000F0;

    *pMtx = D_8009A180;
    *(MATRIX *)0x1F800110 = *pMtx;
    *(MATRIX *)0x1F800130 = *pMtx;
    RotMatrixX(-(u16)p->unk0, pMtx);
    RotMatrixY(-(u16)p->unk2, (MATRIX *)0x1F800110);
    RotMatrixZ(-(u16)p->unk4, (MATRIX *)0x1F800130);
    MulMatrix0(pMtx, (MATRIX *)0x1F800110, (MATRIX *)0x1F800150);
    MulMatrix0((MATRIX *)0x1F800130, (MATRIX *)0x1F800150, &D_8009C808);
    ((SVECTOR *)0x1F8000A0)->vx = -(u16)p->unk0;
    ((SVECTOR *)0x1F8000A0)->vy = -(u16)p->unk2;
    ((SVECTOR *)0x1F8000A0)->vz = -(u16)p->unk4;
    *pMtx = D_8009C808;
    ApplyMatrix(pMtx, (SVECTOR *)0x1F8000A0, (VECTOR *)0x1F800000);
    TransMatrix(&D_8009C808, (VECTOR *)0x1F800000);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097440);


void func_8009766C(void) {
    D_8009BE24 = HeapAlloc(0x2000, 0);
    func_800976C8();
}

void func_800976A0(void) {
    HeapFree(D_8009BE24);
}

void func_800976C8(void) {
    s32 i;

    for (i = 0; i < 0x40; i++) {
        D_8009BE24[i].unk4C = 0;
        D_8009BE24[i].unk18 = 0;
        D_8009BE24[i].unk1C = 0;
    }
}

void func_800976FC(s32 value, s32 index) {
    D_8009BE24[index].unk0 = 0;
    D_8009BE24[index].unk18 = value;
}

void func_80097718(s32 value18, s32 value1C) {
    s32 i;

    for (i = 0; i < 0x40; i++) {
        if (D_8009BE24[i].unk1C == 0) {
            D_8009BE24[i].unk0 = 0;
            D_8009BE24[i].unk2 = 0;
            D_8009BE24[i].unk4 = 0;
            D_8009BE24[i].unk18 = value18;
            D_8009BE24[i].unk1C = value1C;
            D_8009BE24[i].unk20 = 0;
            D_8009BE24[i].unk22 = 0;
            break;
        }
    }
}

s32 func_80097770(s32 index, s16 value) {
    Misc15Slot *pSlot = &D_8009BE24[index];

    if (pSlot->unk4 != 0) {
        return 0;
    }
    pSlot->unk0 = 1;
    pSlot->unk4 = value;
    return 1;
}

void func_800977A8(s32 index) {
    *(u16 *)((u8 *)D_8009BE24 + (index << 7)) = 3;
}

void func_800977C4(s32 index) {
    *(u16 *)((u8 *)D_8009BE24 + (index << 7)) = 4;
}

void func_800977E0(s32 index, u16 value) {
    u8 *pSlot = (u8 *)D_8009BE24 + (index << 7);

    *(u16 *)pSlot = 2;
    *(u16 *)(pSlot + 2) = value;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097800);
// try_flags: --gp -G0 gives 88.53 but drops the matched func_80097CB8 to
// 97.21 and func_8009699C to 91.63, so the unit preset is right; gcc 2.6.0
// gives 79.25 with the parked shape.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800978FC);
// Best: 82.47%. Everything matches except that the original holds s0 =
// &D_8009BC3C across the second HeapAlloc and reads the buffer back with
// `lw v1,0(s0)`; every shape tried (global stores, u8 **ppBuffer, a
// u8 *pBuffer local) gets lui/at for the store plus a fresh lui for the read,
// so the frame has no s0 slot and the copy loop's 0x10000 constant lands in
// v0 instead of t0. The two do-while loops and the +6 slot-filler bias are
// required for the rest to match. gcc-2.7.2-cdk gives 84.65%, so the preset is
// not the answer either.
// Round 19 re-check: the parked shape is the best one (76.88% from a direct
// `*ppBuffer = ...` form; adding the pSlot[-3..1] form loses the s0 formation
// entirely -- cc1 rematerialises &D_8009BC3C as `lui $at` for the store and a
// fresh `la` for the reload, because copy-prop turns ppBuffer back into the
// constant and cse then expands each use separately. Also try_flags.sh
// compile-fails on every row in this tree (even the default one), so flag
// experiments here have to be done by hand.
/*
void func_800978FC(void) {
    u8 **ppBuffer;
    u8 *pBuffer;
    u8 *pSlot;
    Misc15Block16 *pSrc;
    Misc15Block16 *pDst;
    Misc15Block16 *pEnd;
    s32 i;

    ppBuffer = &D_8009BC3C;
    *ppBuffer = HeapAlloc(0x10000, 1);
    D_8009BCB4 = HeapAlloc(0x10000, 1);
    pBuffer = *ppBuffer;
    pSlot = pBuffer;
    i = 0;
    do {
        i++;
        pSlot[3] = 7;
        pSlot[7] = 0x24;
        pSlot[4] = 0x80;
        pSlot[5] = 0x80;
        pSlot[6] = 0x80;
        pSlot += 0x20;
    } while (i < 0x800);
    pSrc = (Misc15Block16 *)D_8009BC3C;
    pDst = (Misc15Block16 *)D_8009BCB4;
    pEnd = pSrc + 0x1000;
    do {
        *pDst++ = *pSrc++;
    } while (pSrc != pEnd);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800979C8);
// Best: 96.83%. Instruction-identical except the prologue: retail emits the
// `sw s2,0x20(sp)` + `move s2,zero` (i = 0 from the declaration) AFTER the
// LZSSHeapDecompress argument setup (lui/lw a0, li a1) and before `sw ra`; we
// emit that pair first, at the very top of the entry block. Same 2-insn class
// as the parked func_8008440C in world_map/main/misc8 (98.71%), not a register
// numbering problem. What DID close this from 92.13%: three separate pointer
// locals (pClut for loop 1, pClut2 for loop 2, pClut3 for loop 3 - one shared
// pClut put i in s3 and the pointer in s2, the reverse of retail), assigning
// `i = 0`/`i = 4` before the `pClut2/pClut3 = ...` address formation, and
// `for (; i < N; x += 0x80, i++, pClut++)` so x += 0x80 precedes i++.
extern void *LZSSHeapDecompress(void *pCompressedData, int flags);
extern void func_8002DD20(void *pBlock);
extern void func_800931D8(u8 *pSrc, u8 *pDst, s32 count, u8 *pFactors);

extern u8 *D_8009C59C;
extern u8 D_8009BB48[];
extern u16 D_8009CCB4[];
extern u16 D_8009CD54[];
extern u16 D_8009CD5C[];

/*
void func_800979C8(void) {
    RECT rect;
    u8 *pData;
    u8 *pDst;
    u8 *pFactors;
    s32 i = 0;
    s32 x;
    s32 y;
    u16 *pClut;
    u16 *pClut2;
    u16 *pClut3;

    pData = LZSSHeapDecompress(D_8009C59C, 1);
    func_8002DD20(pData);
    DrawSync(0);
    HeapFree(pData);
    HeapFree(D_8009C59C);
    pClut = D_8009CCB4;
    pData = HeapAlloc(0x400, 1);
    pDst = HeapAlloc(0x8000, 1);
    rect.x = 0;
    rect.y = 0x1E0;
    rect.w = 0x100;
    rect.h = 2;
    StoreImage(&rect, (u_long *)pData);
    DrawSync(0);
    pFactors = D_8009BB48;
    func_800931D8(pData, pDst, 0x20, pFactors);
    func_800931D8(pData + 0x200, pDst + 0x4000, 0x20, pFactors);
    rect.x = 0;
    rect.y = 0x1B0;
    rect.w = 0x100;
    rect.h = 0x40;
    LoadImage(&rect, (u_long *)pDst);
    DrawSync(0);
    for (; i < 0x40; i++, pClut++) {
        *pClut = GetClut(rect.x, rect.y);
        rect.y++;
    }
    x = 0x200;
    y = 0;
    i = 0;
    pClut2 = D_8009CD54;
    for (; i < 4; x += 0x80, i++, pClut2++) {
        *pClut2 = GetTPage(1, 0, x, y);
    }
    x = 0x180;
    y = 0x100;
    i = 4;
    pClut3 = D_8009CD5C;
    for (; i < 7; x += 0x80, i++, pClut3++) {
        *pClut3 = GetTPage(1, 0, x, y);
    }
    HeapFree(pDst);
    HeapFree(pData);
}

*/
// Best: 63.31%. Same shape as the parked 94.28% note class: cc1 2.7.2 sinks
// both masked `and`+store pairs to the end of the block and hoists the mask's
// (high) half alone, so the original's interleaved order (lui+ori together, then
// plain stores, then and+store, then second load) is unreachable from any
// source order tried; 2.6.0 gives 72% for this function but drops the already
// matched func_80097CB8 to 97.21%, so the preset is right.
// Round 19: confirmed at the RTL level (build/src/world_map/main/misc15.c.s).
// cc1 2.7.2 emits
// `li $4,0x7f0000` right after the clear loop and the `ori $4,$4,0xffff` ten
// insns later, and hoists the two `li $2,2` / C838,C83C stores above the BBBx
// stores; retail has one `li $a0,0x7FFFFF` and keeps source order, so the
// instruction multiset is identical and only the order differs (52.66% for the
// direct form, 63.31% for this parked x/z form). A `u32 mask = 0x7FFFFF;`
// local changes nothing; the x/z loads-as-statements form makes the hoisting
// worse. This is the documented allocator/scheduler-tie class. -O1/-O3/cdk
// could not be scored: try_flags.sh compile-fails on every row in this tree.
/*
void func_80097BC0(s32 *p) {
    u32 *pEnd;
    s32 i;
    s32 x;
    s32 z;

    D_8009D534 = D_8009A180;
    i = 0xFF;
    pEnd = D_8009C580;
    for (; i >= 0; i--) {
        *pEnd-- = 0;
    }
    x = p[0];
    D_8009BBB8 = 0;
    D_8009C5BC = 0;
    D_8009C618 = 0x400;
    D_8009BBB4 = x & 0x7FFFFF;
    z = p[2];
    D_8009C838 = 2;
    D_8009C83A = 0;
    D_8009C83C = 2;
    D_8009BBBC = z & 0x7FFFFF;
    func_800981C8((s32)p);
    func_80097DC0();
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097BC0);

void func_80097CB8(s32 arg0) {
    u32 *pEnd;
    s32 i;

    D_8009D534 = D_8009A180;
    i = 0xFF;
    pEnd = D_8009C580;
    for (; i >= 0; i--) {
        *pEnd-- = 0;
    }
    D_8009C5BC = 0;
    D_8009C618 = 0x400;
    func_800981C8(arg0);
    func_80097DC0();
}

void func_80097D64(void) {
    s32 i;

    for (i = 0; i < 0x100; i++) {
        if (D_8009C184[i] != NULL) {
            HeapFree(D_8009C184[i]);
        }
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097DC0);
