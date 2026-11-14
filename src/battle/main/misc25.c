#include "common.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"

extern u8 D_800C37C8;
extern u16 D_800C3E30;
extern u8 D_800C3D40;
extern u8 D_800C3D6C;
extern u8 *D_800C3BAC[];
extern s32 D_800C3BCC[];
extern s32 D_800C3BEC;
extern s32 D_800C3BF0;
extern s32 D_800C360C;
extern u8 D_800C4000[];
extern void *D_800658C8;

extern void HeapFree(void *pMem);
extern s32 func_800A2330(void);
extern void func_800A23E8(s32, u8 *);
extern s16 func_800A579C(s16 *);
extern void func_800A5870(s16 *, s16, void *);
extern s32 func_800A5914(s16 *, s16, s32);
extern void SoundRemoveSedsEntry(s32);
extern void func_800B00D0(void);
extern u8 D_800C3D0C[];
extern u8 *D_800D3368[];
extern void func_800A9FF0(s32);
extern void func_800AA934(void *, void *, void *, void *);
extern void func_800BE790(void);
extern void func_800B0D70(s16 *, u16 *);

typedef struct BattleModelNode BattleModelNode;

// Node of a 0x7C-stride hierarchy whose element 0 heads the list.
typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ BattleModelNode *pNodes;
} BattleModel;

struct BattleModelNode {
    /* 0x0 */ BattleModelNode *pParent;
    /* 0x4 */ u8 pad4[0x3];
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u8 pad8[0x2];
    /* 0xA */ u16 unkA;
    /* 0xC */ u8 padC[0x70];
}; // Size: 0x7C

// Current frame published to the rest of the battle code.
typedef struct {
    /* 0x0 */ mem_addr unk0;
    /* 0x4 */ mem_addr unk4;
    /* 0x8 */ mem_addr unk8;
    /* 0xC */ mem_addr unkC;
    /* 0x10 */ mem_addr unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} BattleFrame; // Size: 0x1C

// Streamed battle data; only the fields this unit reads are named.
typedef struct {
    /* 0x0 */ u8 pad0[0x34C];
    /* 0x34C */ s16 unk34C;
    /* 0x34E */ s16 unk34E;
    /* 0x350 */ s16 unk350;
    /* 0x352 */ s16 unk352;
} BattleStreamData;

extern BattleFrame D_800C3BD0;

s16 func_800AF2C4(s32 *pAxis, VECTOR *pV0, VECTOR *pV1, s32 divisor) {
    s32 vec[3];
    s32 dot;
    s32 len;

    OuterProduct12(pV0, pV1, (VECTOR *)vec);
    dot = vec[0] * pAxis[0] + vec[1] * pAxis[1] + vec[2] * pAxis[2];
    len = SquareRoot0(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]) + 1;
    dot = (dot << 4) / len;
    dot = (dot << 8) / divisor;
    return dot;
}

s32 func_800AF400(void) {
    s32 i;

    for (i = 0; i < 13; i++) {
        if ((D_800C3E30 >> i) & 1) {
            break;
        }
    }
    return i;
}
s32 func_800AF438(u8 *p, u32 id, u16 *pOut) {
    if ((id & 0xFF) == 0xFF) {
        id = func_800AF400();
    } else if ((id & 0xFF) == 0xFE) {
        id = D_800C3D40;
    } else if ((id & 0xFF) == 0xFD || (id & 0xFF) == 0xF9) {
        id = p[0x20];
    } else if ((id & 0xFF) == 0xFC) {
        id = p[0x21];
    } else if ((id & 0xFF) == 0xFA) {
        id = 0x1F;
    } else if ((id & 0xFF) == 0xF8) {
        id = p[0x20] * 2 + 0xD;
    } else if ((id & 0xFF) == 0xF7) {
        id = p[0x20] * 2 + 0xE;
    }
    *pOut = 1 << (id & 0xFF);
    return id & 0xFF;
}
extern u8 D_800CCD1E;
extern u8 func_8009C050(u8);

s32 func_800AF518(u8 *p, u32 id, s32 *pOut) {
    u32 sel = id;
    u8 kind;

    *pOut = 0;
    if ((sel & 0xFF) >= 0xFE) {
        if ((sel & 0xFF) == 0xFF) {
            kind = func_8009C050(p[0x20]);
            if ((kind & 4) && (*(u16 *)(p + 0x4A) & 0x100)) {
                p[0x2A] = 0x1B;
            } else if ((kind & 2) && (*(u16 *)(p + 0x4A) & 0x80)) {
                if (!(*(u16 *)((u8 *)&D_800CCD1E + p[0x20] * 0x170) & 1)) {
                    p[0x2A] = 6;
                }
            } else if (!(*(u16 *)(p + 0x4A) & 0x400)) {
                p[0x2A] = 1;
            }
            if (kind & 1) {
                p[0x2A] |= 0x80;
            }
        }
        sel = p[0x2A] & 0x7F;
        *pOut = p[0x2A] & 0x80;
    }
    if ((sel & 0xFF) < 0x40) {
        return ((s32 *)*(u32 *)(p + 0x14))[(sel & 0xFF) + 1];
    }
    return ((s32 *)*(u32 *)(p + 0x18))[(sel & 0xFF) - 0x3F];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF678);
void func_800AFA98(BattleModel *pModel, BattleModelNode *pNode, s32 flags) {
    BattleModelNode *pChild;
    s32 i;

    pNode->unk7 = flags & 1;
    if (flags & 0x80) {
        pChild = pModel->pNodes;
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            pChild++;
            if (pChild->pParent == pNode) {
                func_800AFA98(pModel, pChild, flags);
            }
        }
    }
}
extern u8 *func_80023FD8(s32, s32, s32, s32);
extern void SpriteSetDirection(void *, s16);
extern void func_800223B0(void *, s16);
extern void SpriteSetScale(void *, s16);
extern s32 WorkListTaskGetTaskCallback(void *);
extern void TimerWorkListSetTaskCallback(void *, void *);
extern void func_800AFC68();

void func_800AFB4C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, u8 *arg5, s32 arg6) {
    u8 *p;
    u8 *pSub;
    u8 *pNode;

    p = func_80023FD8(arg1, arg0, arg2, 0x18);
    pSub = p + 0x38;
    SpriteSetDirection(pSub, arg3);
    func_800223B0(pSub, arg3);
    SpriteSetScale(pSub, arg4);
    pNode = p + *(s16 *)(p + 0xBE);
    *(s32 *)(pNode + 0x8) = arg6;
    *(u16 *)(pNode + 0xC) = arg5[5];
    if (arg5[0x13] != 0) {
        *(s32 *)(pNode + 0x4) = WorkListTaskGetTaskCallback(p);
        TimerWorkListSetTaskCallback(p, func_800AFC68);
        *(u16 *)(pNode + 0x10) = *(u16 *)(arg5 + 0x6);
        *(u16 *)(pNode + 0x12) = *(u16 *)(arg5 + 0x8);
        *(u16 *)(pNode + 0x14) = *(u16 *)(arg5 + 0xA);
        *(u16 *)(pNode + 0xE) = arg5[0xC];
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFC68);
/* 91.71%: structure, the GTE block (gte_ldv0/gte_mvmva/gte_stlvnl are an exact
   match) and the callback tail are right; the only difference is which temp
   register holds pMat: original `lw $v0,4($v0)` in $v0 + offset in $a1, ours
   the other way round. Tried: offset as a local before the call, pMat local.
extern void CompMatrix(void *, void *, void *);
extern void SetRotMatrix(void *);
extern void SetTransMatrix(void *);

void func_800AFC68(u8 *p) {
    u8 *pNode;
    u8 *pMatrix;
    s32 index;
    s32 vec[3];

    pNode = p + *(s16 *)(p + 0xBE);
    index = *(s16 *)(pNode + 0xC);
    pMatrix = PSX_SCRATCH;
    if (index != 0) {
        s32 offset = index * 0x7C;
        u8 *pMat = *(u8 **)(*(u8 **)(pNode + 0x8) + 4);
        CompMatrix(pMat + 0xC, pMat + offset + 0x2C, PSX_SCRATCH);
    } else {
        pMatrix = *(u8 **)(*(u8 **)(pNode + 0x8) + 4) + 0xC;
    }
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    gte_ldv0(pNode + 0x10);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(vec);
    if (*(s16 *)(pNode + 0xE) != 0) {
        vec[1] = *(s16 *)(*(u8 **)(pNode + 0x8) + 0x60);
    }
    *(s32 *)(p + 0x38) = vec[0] << 16;
    *(s32 *)(p + 0x3C) = vec[1] << 16;
    *(s32 *)(p + 0x40) = vec[2] << 16;
    ((void (*)(u8 *)) * (u32 *)(pNode + 0x4))(p);
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFD98);
/* 98.68% (516 bytes, identical instruction multiset): the whole body, the three
   mode blocks, the two sb writes and the child walk are exact; only the
   callee-saved assignment of the pModel and flags pseudos differs (original
   pModel=s6 / flags=s7, ours pModel=s7 / flags=s3; original keeps a second copy
   of flags in a2 for the three `flags & 0x20` tests, which needs the source to
   hold flags in two variables - not reproduced). Tried: s32 vs u8 vs unsigned
   flags, recursive call with and without `& 0xFF`, s16 vs s32 x/y/z with casts,
   mode/i/pChild declaration orders - all leave the ranking untouched.
void func_800AFD98(BattleModel *pModel, BattleModelNode *pNode, s32 flags, s16 x, s16 y, s16 z) {
    BattleModelNode *pChild;
    s32 mode;
    s32 i;

    mode = flags & 7;
    if (mode == 0) {
        if (flags & 0x20) {
            *(u16 *)((u8 *)pNode + 0x54) += x;
            *(u16 *)((u8 *)pNode + 0x56) += y;
            *(u16 *)((u8 *)pNode + 0x58) += z;
        } else {
            *(u16 *)((u8 *)pNode + 0x54) = x;
            *(u16 *)((u8 *)pNode + 0x56) = y;
            *(u16 *)((u8 *)pNode + 0x58) = z;
        }
    } else if (mode == 1) {
        if (flags & 0x20) {
            *(s32 *)((u8 *)pNode + 0x5C) += x;
            *(s32 *)((u8 *)pNode + 0x60) += y;
            *(s32 *)((u8 *)pNode + 0x64) += z;
        } else {
            *(s32 *)((u8 *)pNode + 0x5C) = x;
            *(s32 *)((u8 *)pNode + 0x60) = y;
            *(s32 *)((u8 *)pNode + 0x64) = z;
        }
    } else if (flags & 0x20) {
        *(u16 *)((u8 *)pNode + 0x4C) += x;
        *(u16 *)((u8 *)pNode + 0x4E) += y;
        *(u16 *)((u8 *)pNode + 0x50) += z;
    } else {
        *(u16 *)((u8 *)pNode + 0x4C) = x;
        *(u16 *)((u8 *)pNode + 0x4E) = y;
        *(u16 *)((u8 *)pNode + 0x50) = z;
    }
    *(u8 *)((u8 *)pNode + 4) = 1;
    *(u8 *)((u8 *)pNode + 5) = 1;
    if (flags & 0x80) {
        pChild = pModel->pNodes;
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            pChild++;
            if (pChild->pParent == pNode) {
                func_800AFD98(pModel, pChild, flags & 0xFF, x, y, z);
            }
        }
    }
}
*/

void func_800AFF9C(u8 *p) {
    u8 unk10[0x10];
    s16 vec[3];
    s16 index;

    vec[0] = *(u16 *)(*(u8 **)(p + 4) + 0x5C);
    vec[1] = *(u16 *)(*(u8 **)(p + 4) + 0x60);
    vec[2] = *(u16 *)(*(u8 **)(p + 4) + 0x64);
    index = func_800A5914(vec, *(s16 *)(p + 0x1E), 4);
    *(s16 *)(p + 0x1E) = index;
    if (index < 0) {
        index = func_800A579C(vec);
        *(s16 *)(p + 0x1E) = index;
    }
    func_800A5870(vec, *(s16 *)(p + 0x1E), unk10);
    *(u16 *)(p + 0x60) = vec[1];
    if (*(u8 *)(p + 0x36) == 0) {
        *(s32 *)(*(u8 **)(p + 4) + 0x60) = vec[1];
    }
}

void func_800B0060(u8 *p) {
    if (*(s32 *)(p + 0xC) != 0) {
        if (*(u8 *)(p + 0x63) != 0) {
            SoundRemoveSedsEntry(*(s32 *)(*(u8 **)(p + 0xB4) + 8));
            *(u8 *)(p + 0x63) = 0;
        }
        HeapFree(*(void **)(p + 0xC));
        *(s32 *)(p + 0xC) = 0;
        *(s32 *)(p + 0x18) = 0;
    }
}

void func_800B00D0(void) {
    s32 i = 8;
    s32 *p = D_800C3BCC;

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}

void func_800B00F4(s32 arg0) {
    s32 i = 0;
    u8 **p = D_800C3BAC;
    u8 *pEntry;

    for (; i < 9; i++) {
        pEntry = *p++;
        if (pEntry != NULL) {
            func_800A23E8(arg0, pEntry);
        }
    }
    func_800B00D0();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0164);
/* 99.70%: everything right except the scratch register gcc uses to spill the
   7th stack argument at entry (original `lhu $t0,104(sp)` / `sh $t0,16(sp)`,
   ours $a0); a0 is free here because arg0 is unused, so it is a global.c
   tie-break. Tried: s16 vs u16 params, &D_800C3BAC[index] vs index+base
   (needed a `pBase` local to get the original lui/addiu-then-sll order).
void func_800B0164(s32 arg0, s32 index, u8 arg2, u8 arg3, u16 arg4, u16 arg5, u16 arg6,
                   u16 arg7, u16 arg8, u16 arg9, u16 arg10) {
    u8 **pBase;
    u8 **pp;
    u8 *p;

    if (D_800C37C8 == 0) {
        pBase = D_800C3BAC;
        pp = &pBase[index];
        if (*pp == NULL) {
            *pp = (u8 *)func_800A2330();
        }
        p = *pp;
        if (p != NULL) {
            p[0] = 1;
            p[1] = 0;
            p[2] = arg2;
            p[3] = arg3;
            *(s16 *)(p + 0x4) = arg4;
            *(s16 *)(p + 0x6) = arg5;
            *(s16 *)(p + 0x8) = arg6;
            *(s16 *)(p + 0xA) = arg7;
            *(s16 *)(p + 0xC) = arg8;
            *(s16 *)(p + 0xE) = arg9;
            *(s16 *)(p + 0x10) = 0;
            *(s16 *)(p + 0x12) = arg10;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B026C);

s32 func_800B0AB4(s16 *pPos) {
    BattleStreamData *pData = D_800658C8;

    if (pPos[0] > pData->unk34E && pPos[0] < pData->unk34C && pPos[2] > pData->unk350 &&
        pPos[2] < pData->unk352) {
        return 1;
    }
    return 0;
}
extern u16 D_800D3354;
extern s16 D_800D3356;
extern u16 D_800D3358;
extern u16 D_800D335C;
extern s16 D_800D335E;
extern u16 D_800D3360;
extern s16 D_800C3542;
extern u16 D_800C3544;
extern s16 D_800C3546;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0B14);
/* 74.97% (588 bytes vs 584): the control flow - the two-condition test, the
   func_800A5914/func_800A579C retry, the D_800C3546/D_800C3542/D_800C3544
   bookkeeping - is in place; what is left is the head: retail loads
   D_800D335C and D_800D335E first, then reads D_800D3354/3356/3358 through one
   base pointer s5 (= &D_800D3354) both for the deltas and for the vec sums, and
   it ranks dx in s1 / dy in s2 where ours swaps them. Tried: dy-first vs dx-first,
   s16 vec[3] vs vec[4], direct globals vs one base pointer local.
s16 func_800B0B14(s32 index) {
    u8 work[0x10];
    s16 vec[4];
    s32 dx;
    s32 dy;
    s32 dz;
    s32 len;
    s32 result;

    dy = (D_800D335E - D_800D3356) / 4;
    dx = (s16)(D_800D335C - D_800D3354);
    dz = (s16)(D_800D3360 - D_800D3358);
    len = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
    vec[0] = D_800D3354 + (dx << 9) / len;
    vec[1] = D_800D3356 + (dy << 9) / len;
    vec[2] = D_800D3358 + (dz << 9) / len;
    result = -1;
    if (func_800B0AB4(vec) != 0 && index != D_800C3546) {
        D_800C3546 = index;
        result = func_800A5914(vec, D_800C3542, 5);
        if (result < 0) {
            result = func_800A579C(vec);
        }
        if (result >= 0) {
            D_800C3542 = result;
        }
    }
    if (result < 0) {
        vec[1] = D_800C3544;
    } else {
        func_800A5870(vec, D_800C3542, work);
        D_800C3544 = vec[1];
    }
    return vec[1];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0D70);
void func_800B0FF4(u16 *pA, u16 *pB) {
    s16 vec[3];
    s32 len;

    vec[0] = pA[0] - pB[0];
    vec[2] = pA[2] - pB[2];
    len = SquareRoot0(vec[0] * vec[0] + vec[2] * vec[2]) + 1;
    vec[0] = (vec[0] << 9) / len;
    vec[2] = (vec[2] << 9) / len;
    func_800B0D70(vec, pB);
}
void func_800B10EC(s32 index, s32 x, s32 z, s32 dist) {
    u8 **pBase = D_800D3368;
    u8 **pp = &pBase[index];
    u8 *pModel;
    s32 dx;
    s32 dz;
    s32 len;
    s32 rem;
    s32 angle;
    u32 diff;

    pModel = *(u8 **)(*pp + 4);
    dx = x - *(s32 *)(pModel + 0x5C);
    dz = z - *(s32 *)(pModel + 0x64);
    len = SquareRoot0(dx * dx + dz * dz) + 1;
    if (len >= dist) {
        return;
    }
    angle = ratan2(dz, -dx);
    diff = (u32)((*(s16 *)(*(u8 **)(*pp + 4) + 0x56) - angle) & 0xFFF) - 0x401;
    if (diff < 0x7FF) {
        angle += 0x800;
    }
    rem = dist - len;
    *(s32 *)(*(u8 **)(*pp + 4) + 0x5C) = x - dx * dist / len - rcos(angle) * rem / 4096;
    *(s32 *)(*(u8 **)(*pp + 4) + 0x64) = z - dz * dist / len - rsin(angle) * rem / 4096;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B12D0);
/* 99.74% (156 bytes, identical instruction stream): only the switch's jump
   table operand differs (the original names jtbl_800707C4). It cannot be
   converted as-is: that table lives in the splat-owned .rodata
   (asm/battle/data/0.rodata.s) and its entries point at this stub's local
   labels (.L800B132C/1334/133C/1344), so `make build` fails to link with
   "undefined reference to .L800B132C" the moment the stub becomes C.
   The mapping is kind -> flag mask:
   0,1 -> 1; 2,3 -> 8; 4 -> 4; 5 -> 2; else -> id & 0xFF.
s32 func_800B12D0(s32 arg0, s32 arg1) {
    s32 result = 0;
    s32 v;
    u8 *pRow = &D_800C4000[(D_800C360C - 1) * 0x48];
    u8 kind = pRow[arg0];

    if (kind < 6) {
        switch (kind) {
        case 0:
        case 1:
            v = arg1 & 1;
            break;
        case 5:
            v = arg1 & 2;
            break;
        case 4:
            v = arg1 & 4;
            break;
        case 2:
        case 3:
            v = arg1 & 8;
            break;
        }
        if (v == 0) {
            result = 1;
        }
    }
    if ((arg1 & 0xFF) == 0) {
        result = 1;
    }
    return result;
}
*/
extern s32 func_800BF6F8(void);
extern s32 ArchiveDataSync(void);
extern void func_8002A498(s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B136C);
/* 96.96% (332 bytes vs 328): the whole control flow, the two 11-entry polls and
   the ArchiveDataSync wait loop match; only two register-allocation ties remain.
   1. prologue save order: original `sw ra,32 / sw s1,20 / sw s0,16 / move s1,zero
      / move s0,zero`, ours emits `sw s1,20 / move s1,zero` before `sw ra,32`.
   2. first poll loop: original keeps the array walker in $a0 and the element in
      $v1, ours the other way round.
   Tried: D_800D3368[i] and *(pp++) walkers, one base pointer local (pBase) vs
   two (pBase + ppBase), comma for-init, flagA declared before/after the base.
   Shape notes for whoever retries: phase 1 is
     for (;;) { poll 11 entries; if (func_800BF6F8()) flagA = 1;
                if (flagA) { func_800BE790(); flagA = 0; continue; } break; }
   phase 2 needs BOTH a base local (loaded once into $s2 before the while) and a
   walker reset from it each pass, and a `if (flagA == 0) break;` after the
   sync test so gcc shares the loop test with the break.
void func_800B136C(void) {
    s32 flagB = 0;
    u8 **pBase = D_800D3368;
    s32 flagA = 0;
    u8 *pEntry;
    s32 i;
    u8 **pp;
    u8 **ppBase;

    for (;;) {
        for (i = 0; i < 11; i++) {
            pEntry = pBase[i];
            if (pEntry != NULL) {
                if (pEntry[0x38] != 0) {
                    flagA = 1;
                }
                if (*(s32 *)(pEntry + 0xC) != 0) {
                    flagB = 1;
                }
            }
        }
        if (func_800BF6F8() != 0) {
            flagA = 1;
        }
        if (flagA != 0) {
            func_800BE790();
            flagA = 0;
            continue;
        }
        break;
    }
    if (flagB == 0) {
        return;
    }
    func_8002A498(0);
    flagA = 1;
    ppBase = D_800D3368;
    while (flagA != 0) {
        if (ArchiveDataSync() == 0) {
            for (i = 0, pp = ppBase; i < 11; i++) {
                pEntry = *pp++;
                if (pEntry != NULL && *(s32 *)(pEntry + 0xC) != 0) {
                    func_800B0060(pEntry);
                }
            }
            flagA = 0;
        }
        if (flagA == 0) {
            break;
        }
        func_800BE790();
    }
}
*/
void func_800B14B8(void) {
    D_800C3D6C = 1;
}
void func_800B14CC(s32 arg0) {
    s32 i;
    s32 flag;
    u8 *pEntry;

    for (i = 0; i < 3; i++) {
        if (i != arg0) {
            func_800AA934(D_800D3368[i], D_800D3368[i], D_800C3D0C, 5);
        }
    }
    do {
        flag = 0;
        for (i = 0; i < 3; i++) {
            if (i != arg0) {
                pEntry = D_800D3368[i];
                if (pEntry != NULL && (pEntry[0x34] != 0 || pEntry[0x38] != 0)) {
                    flag = 1;
                }
            }
        }
        func_800BE790();
    } while (flag != 0);
    func_800BE790();
    for (i = 0; i < 3; i++) {
        if (i != arg0) {
            func_800A9FF0(i);
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B15D8);
/* 71.84% (192 bytes, same size as the original): the 7-word copy and the frame
   arithmetic are right. Remaining differences: the original keeps the copy
   destination in $a2 and the lui high half in $a3 (so the later global accesses
   are `lw v0,%lo(D_800C3BD0)($a3)`), and blocks the copy 3+3+1 instead of 4+3.
   Tried: pDst pointer local, direct global access on every reference,
   `s32 offset` local folding the +0xC into the register (that one matches the
   original's `addiu v0,v0,0xC` but the rest is unchanged).
s32 func_800B15D8(u8 *p, s32 index) {
    BattleFrame *pFrame = (BattleFrame *)(p + index * 0x1C + 0xC);
    BattleFrame *pDst = &D_800C3BD0;

    *pDst = *pFrame;
    if ((*(u32 *)(p + 4) & 1) == 0) {
        pDst->unk0 += (mem_addr)pFrame;
        pDst->unk8 += (mem_addr)pFrame;
        pDst->unk10 += (mem_addr)pFrame;
    }
    D_800C3BF0 = 0;
    D_800C3BEC = pDst->unk10;
    return pFrame->unk14;
}
*/
