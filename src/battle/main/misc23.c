#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"

extern s32 D_800D2D40;
extern s32 D_800D2D48;
extern u8 D_800C3D04[];
extern u8 D_800C3D0C[];
extern u16 D_800C3D40;
extern u16 D_800C3E30;
extern u8 D_800D3611;
extern u8 *D_800D3368[];

extern void func_800A22A8(u8 *);
extern void func_800A2D1C(u8 *);
extern void func_800A9FF0(s32);
extern void func_800AA934(void *, void *, void *, void *);
extern s32 func_800AF400(void);
extern void func_800A429C(void *);
extern void func_800A64E4(void);
extern void func_800BF85C(s32, s32);

extern u8 *D_800D33E4;
extern u16 D_800D361A;
extern u_long *D_800D3604;
extern RECT D_800D3628;
extern void *D_800C3AC4;
extern void *D_800C3AC8;
extern u8 D_800C3D68;
extern u8 D_800591AC;
extern s32 D_80059464;
extern s32 D_800C360C;
extern u8 D_800C4000[];

// Per-unit 6-byte state records, unit id 0..3; the original spells each byte
// as its own symbol.
extern u8 D_800C3AAC[];
extern u8 D_800C3AAD[];
extern u8 D_800C3AAE[];
extern u8 D_800C3AAF[];
extern u8 D_800C3AB0[];
extern u8 D_800C3AB1[];

// Allocation block owned by the structure passed to func_800A8A88.
typedef struct {
    /* 0x0  */ u8 unk0[0x14];
    /* 0x14 */ void *unk14;
    /* 0x18 */ void *unk18;
    /* 0x1C */ void *unk1C;
    /* 0x20 */ void *unk20;
} Misc23Block;

// 0xE-byte node of the tree rooted at D_800D39CC, indexed by D_800D3348.
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 unkD;
} Misc23TreeEntry; // Size: 0xE

extern s32 D_800D3344;
extern s32 D_800D3348;
extern s32 D_800D39CC;
extern u8 D_800D2F64;
extern s32 func_800A5D54(s32, s32, s32);

s32 func_800A5914(s32 arg0, s32 arg1, s32 arg2) {
    s32 result;
    s32 i;

    if (D_800D3344 == 0) {
        return -1;
    }
    if (D_800D39CC == 0) {
        return -1;
    }
    if (arg1 >= D_800D3348) {
        return -1;
    }
    for (i = 0; i < arg2; i++) {
        result = func_800A5D54(arg0, arg1, arg2);
        if (result >= 0) {
            break;
        }
    }
    D_800D2F64++;
    if (D_800D2F64 != 0) {
        return result;
    }
    D_800D2F64 = 1;
    for (i = 0; i < D_800D3348; i++) {
        ((Misc23TreeEntry *)(i * 14 + D_800D39CC))->unkD = 0;
    }
    return result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5A48);
void func_800A5BE8(SVECTOR *pBase, SVECTOR *pA, SVECTOR *pB, SVECTOR *pOut, VECTOR *pNormal) {
    VECTOR nA;
    VECTOR nB;
    long d[3];

    d[0] = pA->vx - pBase->vx;
    d[1] = pA->vy - pBase->vy;
    d[2] = pA->vz - pBase->vz;
    VectorNormal((VECTOR *)d, &nA);
    d[0] = pB->vx - pBase->vx;
    d[1] = pB->vy - pBase->vy;
    d[2] = pB->vz - pBase->vz;
    VectorNormal((VECTOR *)d, &nB);
    OuterProduct12(&nA, &nB, pNormal);
    if (pNormal->vy == 0) {
        pOut->vy = 0;
    } else {
        pOut->vy = pBase->vy + (-((pOut->vx - pBase->vx) * pNormal->vx) - (pOut->vz - pBase->vz) * pNormal->vz) / pNormal->vy;
    }
}
s32 func_800A5D54(s32 arg0, s32 index, s32 depth) {
    s32 result;
    s32 res2;
    s32 res3;

    if (index < 0) {
        return -1;
    }
    if (((Misc23TreeEntry *)(index * 14 + D_800D39CC))->unkD != D_800D2F64) {
        ((Misc23TreeEntry *)(index * 14 + D_800D39CC))->unkD = D_800D2F64;
        result = func_800A5A48((void *)(((Misc23TreeEntry *)(index * 14 + D_800D39CC))->unk0 * 8 + D_800D3344),
                               (void *)(((Misc23TreeEntry *)(index * 14 + D_800D39CC))->unk2 * 8 + D_800D3344),
                               (void *)(((Misc23TreeEntry *)(index * 14 + D_800D39CC))->unk4 * 8 + D_800D3344), arg0);
        if (result == -1) {
            return index;
        }
    }
    if (depth <= 0) {
        return -1;
    }
    result = func_800A5D54(arg0, ((Misc23TreeEntry *)(index * 14 + D_800D39CC))->unk6, depth - 1);
    if (result >= 0) {
        return result;
    }
    res2 = func_800A5D54(arg0, ((Misc23TreeEntry *)(index * 14 + D_800D39CC))->unk8, depth - 1);
    if (res2 >= 0) {
        return res2;
    }
    res3 = func_800A5D54(arg0, ((Misc23TreeEntry *)(index * 14 + D_800D39CC))->unkA, depth - 1);
    if (res3 < 0) {
        res3 = -1;
    }
    return res3;
}

void func_800A5E9C(s32 arg0, s32 arg1) {
    D_800D2D40 = arg0;
    D_800D2D48 = arg1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5EB4);

void func_800A6444(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    if (arg0 < 4) {
        if (arg1 >= 0) {
            D_800D3611 = 1;
            D_800C3AAC[arg0 * 6] = 1;
            D_800C3AAD[arg0 * 6] = arg1;
            D_800C3AAE[arg0 * 6] = arg2;
            D_800C3AAF[arg0 * 6] = arg3;
            D_800C3AB0[arg0 * 6] = arg4;
            D_800C3AB1[arg0 * 6] = arg5;
        } else {
            D_800C3AAC[arg0 * 6] = 0;
        }
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A64E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6884);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6AE8);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6F98);
/* 84.20%: identical instruction multiset except that the original keeps
   &D_800D3628 in s0 across LoadImage/DrawSync and gcc 2.7.2 rematerialises the
   symbol address instead (also costs an s0 slot and shuffles the i++ into the
   delay slot). Tried: RECT *pRect, u8 *pBuf, reusing the loop pointer, all
   identical code.
void func_800A6F98(void) {
    u8 *p;
    s32 i;

    if (D_800D361A != 0) {
        p = *(u8 **)(D_800D33E4 + 0x118);
        for (i = 0; i < *(u8 *)(D_800D33E4 + 0x10E); i++) {
            if (*(u16 *)(p + 0x1A) != 0) {
                *(s32 *)p = 0;
            }
            p += 0x30;
        }
        p = (u8 *)&D_800D3628;
        LoadImage((RECT *)p, D_800D3604);
        DrawSync(0);
        func_800A429C(p - 0x28);
    }
    func_800A64E4();
    HeapFree(D_800C3AC4);
    HeapFree(D_800C3AC8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7064);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7948);

void func_800A8A88(Misc23Block *pBlock) {
    if (pBlock->unk14 != NULL) {
        HeapFree(pBlock->unk14);
        HeapFree(*(void **)pBlock->unk1C);
        HeapFree(pBlock->unk1C);
        HeapFree(pBlock->unk20);
        if (pBlock->unk18 != NULL) {
            HeapFree(pBlock->unk18);
        }
        pBlock->unk14 = NULL;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8B0C);
/* 86.40%: statement order and instruction multiset match, but gcc puts the
   pointer induction variable of the descending clear loop in $v1 and the counter
   in $v0 (the original has the pointer in $v0 and the counter in $v1), and the
   "1" stored to D_800C3B74 lands in $v1 instead of $v0. Tried: swapping the
   pWord/i declaration order, hoisting D_800C3D0C into a pBuf local (that fixed
   the other two differences: the early lui/addiu of the argument and the u16
   store to D_800C3B7C).
    u8 *pBuf;
    u32 *pWord;
    s32 i;
    pBuf = D_800C3D0C;
    D_800C3E88 = 0;
    D_800C3CF0 = 0;
    D_800C3D6C = 0;
    D_800C3D68 = 0;
    D_800C3B7C = 0;
    D_800C3B74 = 1;
    func_800A2234(pBuf, *(s16 *)(D_800658C8 + 0x348));
    func_800A2CA4(D_800C3D04, *(s16 *)(D_800658C8 + 0x34A));
    func_800B00D0();
    pWord = (u32 *)&D_800D33E4;
    for (i = 0x1F; i >= 0; i--) {
        *pWord-- = 0;
    }
    for (i = 0x98; i >= 0; i -= 8) {
        *(u32 *)(D_800C3ACC + i) = 0;
    }
    for (i = 0x14; i >= 0; i -= 0x14) {
        *(u16 *)(D_800D330A + i) = 0;
    }
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8BF0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9540);
// 0x18-byte archive entry allocated by func_800A96B4, stored in D_800C3B78.
typedef struct {
    /* 0x0  */ u16 unk0;
    /* 0x2  */ u16 pad2;
    /* 0x4  */ void *unk4;
    /* 0x8  */ u16 unk8;
    /* 0xA  */ u16 padA;
    /* 0xC  */ void *unkC;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 pad12;
    /* 0x14 */ s32 unk14;
} Misc23ArchiveEntry; // Size: 0x18

extern Misc23ArchiveEntry *D_800C3B78;
extern int ArchiveDecodeAlignedSize(unsigned int);
extern void func_80029AFC(Misc23ArchiveEntry *, s32, s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A96B4);
/* 99.10%: identical instruction multiset; the only difference is which of
   $s1/$s2 holds the (arg0 * 2) index and which holds index + 1 - the original
   puts the index in $s1 (reusing the saved parameter register: "sll s1,s1,1")
   and gcc 2.7.2 puts it in $s2, so both the prologue saves and the three
   "move a0,sN" swaps follow. Tried: computing index before the HeapAlloc
   calls (45.83%), after them (99.10%), naming index + 1 as its own local,
   reordering all five declarations, rewriting it as "arg0 *= 2" so the scale
   happens in place (99.10%), and gcc 2.6.0 (99.1, so not a preset problem).
void func_800A96B4(s32 arg0) {
    Misc23ArchiveEntry *pEntry;
    s32 alignedIndex;
    s32 remainder;
    s32 next;

    ArchiveGetArchiveOffsetIndices(&alignedIndex, &remainder);
    ArchiveSetIndex(0x28, 0);
    HeapChangeCurrentUser(4, NULL);
    pEntry = HeapAlloc(0x18, 1);
    D_800C3B78 = pEntry;
    arg0 *= 2;
    next = arg0 + 1;
    ArchiveGetFilePath(next);
    pEntry->unk0 = next;
    pEntry->unk4 = HeapAlloc(ArchiveDecodeAlignedSize(next), 1);
    pEntry->unk8 = arg0 + 2;
    pEntry->unkC = HeapAlloc(ArchiveDecodeAlignedSize(arg0 + 2), 1);
    pEntry->unk10 = 0;
    pEntry->unk14 = 0;
    func_80029AFC(D_800C3B78, 0, 0);
    ArchiveSetIndex(alignedIndex, remainder);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A979C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9A50);

void func_800A9F94(void) {
    s32 i;

    for (i = 0; i < 0x1F; i++) {
        func_800A9FF0(i);
    }
    func_800A22A8(D_800C3D0C);
    func_800A2D1C(D_800C3D04);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9FF0);

void func_800AA320(u16 arg0, u16 arg1, void *arg2) {
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    ((u8 *)D_800D3368[arg0])[0x35] = 0;
    if (D_800D3368[arg0] != NULL) {
        func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
    }
}

void func_800AA384(u16 arg0, u16 arg1, void *arg2) {
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    D_800C3D68 = 1;
    ((u8 *)D_800D3368[arg0])[0x35] = 1;
    D_80059464 = 0;
    D_800591AC = 1;
    func_800BF85C(arg0, func_800AF400());
    D_800C360C = 1;
    D_800C4000[func_800AF400()] = 0;
    if (D_800D3368[arg0] != NULL) {
        func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
    }
}
void func_800AA454(u16 arg0, u16 arg1, void *arg2) {
    u16 savedIdx;
    u16 savedUnk;

    savedIdx = D_800C3D40;
    savedUnk = D_800C3E30;
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    ((u8 *)D_800D3368[arg0])[0x35] = 0;
    if (D_800D3368[arg0] != NULL) {
        if ((s32)arg0 != func_800AF400()) {
            func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
        }
    }
    D_800C3D40 = savedIdx;
    D_800C3E30 = savedUnk;
}
