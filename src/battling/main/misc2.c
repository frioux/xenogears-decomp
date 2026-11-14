#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80072D18);

/* The depth is consumed out of $a2 by gte_ldir123_dp_a2, so the third
   parameter must stay in place. */
void func_80073064(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf12_b();
    gte_stir123_lh(p1);
}

void func_800730AC(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf0_b();
    gte_stir123_lh(p1);
}

void func_800730F4(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh4(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf12_b();
    gte_stir123_lh(p1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_8007313C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800731F8);

/* 98.75%: only the load temp differs, the original uses $t0 where gcc picks
   $v0 (`lw t0,0(a1)` against `lw v0,0(a1)`). Tried byte and word pointer
   parameters, `p0 += 4` against `p0++`, a separate u32 temp, an `end` pointer
   local, and a while loop (that one also grows a pre-test, 71.25%). The
   do/while with `n += (s32)p1` is right; the register is an allocator pick -
   $2, $3 and $7 are all free here, so gcc's first fresh pseudo is $v0, and no
   source shape moved it to $t0. */
/* void func_800732AC(u32 *p0, u32 *p1, s32 n) {
    n += (s32)p1;
    do {
        *p0 = *p1;
        p1++;
        p0++;
    } while ((s32)p1 != n);
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800732AC);

extern u8 *func_8008D3F4(s32, s32);
extern void func_8008D5C0(u8 *, s32);
extern u8 *D_80092644;

void func_800732CC(void) {
    u8 *p = func_8008D3F4(3, 0);

    p[0x74] = 0x80;
    p[0x75] = 0x80;
    p[0x76] = 0xC0;
    func_8008D5C0(p, 0x60);
    *(s16 *)(p + 0x00) = 4;
    *(s16 *)(p + 0x44) = 0x300;
    *(s16 *)(p + 0x48) = 8;
    *(s16 *)(p + 0x4A) = 0x20;
    *(s16 *)(p + 0x68) = 0;
    *(s16 *)(p + 0x6A) = 0x20;
    D_80092644 = p;
}

extern s32 D_80092648;
extern s32 D_80092A24;
extern s32 D_80092A28;
extern s32 D_80092A2C;
extern u8 D_80091C0C[];
extern void func_8008D680(u8 *, u8 *, s32);
extern void BattleEffectUpdateEntries(u8 *, u8 *, u8 *);

void func_8007334C(u8 *p0, u8 *p1) {
    s32 n = D_80092648;
    u8 *p = D_80092644;

    *(s16 *)(p + 0x14) = D_80092A24;
    *(s16 *)(p + 0x16) = D_80092A28;
    *(s16 *)(p + 0x18) = D_80092A2C;
    *(s16 *)(p + 0x34) = 0;
    *(s16 *)(p + 0x36) = 0;
    *(s16 *)(p + 0x38) = 0;
    func_8008D680(p, D_80091C0C, n);
    gte_SetTransRotMatrix_gpr(p1);
    BattleEffectUpdateEntries(p, p0, p1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073424);
/* 98.09%: the loop's induction shape. The original keeps BOTH a walking
   pointer ($a2 = p2 + i*0x44, tested as `lbu v0,0x67C(a2)`) and the byte offset
   ($t0, used as `addiu v0,t0,0x64C; addu s0,s3,v0`); gcc folds the two into one
   register, and which one survives depends on the source shape: `p2 + 0x67C +
   i * 0x44` with `pSlot = p2 + 0x64C + i * 0x44` keeps the walking pointer for
   the test but turns pSlot into a sll/addu multiply chain (89.52%), and a single
   `off` variable feeding both gives the addiu back but recomputes the test
   address as `addu v0,off,p2; lbu v0,1660(v0)` (this 98.09%). Also tried a
   separate walking pFlags with off += 0x44 (gcc drops off entirely, 98.09 with
   the pointer in $a0). Everything else - the frame (the unused s32 pad[2] at
   sp+0x30), both VECTOR locals at sp+0x10/sp+0x20, the 0x10-byte copies and all
   four calls - matches instruction for instruction.
extern u8 D_800910F4[];
extern void func_8008859C(VECTOR *, SVECTOR *);
extern void func_8008EBD0(u8 *, s32, void *, s32);

void func_80073424(VECTOR *p0, VECTOR *p1, u8 *p2, s32 arg3, s32 arg4, s32 arg5) {
    VECTOR vDiff;
    VECTOR vNormal;
    s32 pad[2];
    u8 *pSrc;
    u8 *pSlot;
    u8 *pEntry;
    s32 i;
    s32 off;
    u8 *pFlags;

    off = 0;
    pFlags = p2;
    for (i = 0; i < 9; i++) {
        if (*(u8 *)(pFlags + 0x67C) == 0) {
            pSlot = p2 + 0x64C + off;
            break;
        }
        pFlags += 0x44;
        off += 0x44;
    }
    if (i == 8) {
        return;
    }
    pSrc = *(u8 **)(p2 + 0xD8);
    vNormal.vx = *(s32 *)(pSrc + 0x0) - p0->vx;
    vNormal.vy = *(s32 *)(pSrc + 0x4) - p0->vy - 0x90;
    vNormal.vz = *(s32 *)(pSrc + 0x8) - p0->vz;
    pEntry = D_800910F4 + arg3 * 8;
    if (p1 != NULL) {
        vDiff.vx = p0->vx - p1->vx;
        vDiff.vy = p0->vy - p1->vy;
        vDiff.vz = p0->vz - p1->vz;
    } else {
        vDiff = vNormal;
    }
    func_8008859C(&vDiff, (SVECTOR *)(pSlot + 0x28));
    *(s16 *)(pSlot + 0x3E) = *(u16 *)(pEntry + 0x0);
    *(u8 *)(pSlot + 0x31) = *(u8 *)(pEntry + 0x4);
    *(s16 *)(pSlot + 0x40) = *(u8 *)(pEntry + 0x3);
    *(u8 *)(pSlot + 0x32) = *(u8 *)(pEntry + 0x2);
    *(u8 *)(pSlot + 0x33) = *(u8 *)(pEntry + 0x5);
    *(s32 *)(pSlot + 0x38) = arg5;
    *(s16 *)(pSlot + 0x3C) = arg4;
    if (*(u8 *)(pEntry + 0x6) != 0) {
        func_8008EBD0(p2, *(u8 *)(pEntry + 0x6), pSlot, 2);
    }
    func_80073064((SVECTOR *)(pSlot + 0x28), (SVECTOR *)(pSlot + 0x20), *(u8 *)(pSlot + 0x31));
    *(VECTOR *)pSlot = *p0;
    *(VECTOR *)(pSlot + 0x10) = *(VECTOR *)pSlot;
    *(u8 *)(pSlot + 0x30) = 1;
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073644);

void func_80073B7C(u8 *p0, s32 arg1, s32 arg2, VECTOR *pOut) {
    u32 **pArray;
    u32 *pEntry;
    u8 *pSrc;
    u8 *pVerts;

    pArray = *(u32 ***)(*(u8 **)(*(u8 **)(p0 + 0x5C) + 0x4) + 0x4);
    pEntry = *(u32 **)(arg1 + pArray);
    if (arg2 != 0 && pEntry[0] == 1) {
        gte_SetRotMatrix_gpr((u8 *)pEntry + 0x4C);
        pSrc = *(u8 **)(*(u8 **)((u8 *)pEntry + 0x4) + 0x14);
        pVerts = *(u8 **)(pSrc + 0x8);
        gte_ldv0((SVECTOR *)(pVerts + (arg2 * 8 - 8)));
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(pOut);
        pOut->vx += *(s32 *)(p0 + 0x0);
        pOut->vy += *(s32 *)(p0 + 0x4);
        pOut->vz += *(s32 *)(p0 + 0x8);
    } else {
        pOut->vx = *(s32 *)((u8 *)pEntry + 0x60) + *(s32 *)(p0 + 0x0);
        pOut->vy = *(s32 *)((u8 *)pEntry + 0x64) + *(s32 *)(p0 + 0x4);
        pOut->vz = *(s32 *)((u8 *)pEntry + 0x68) + *(s32 *)(p0 + 0x8);
    }
}

void func_80073CA4(u8 *p0) {
    s32 i;

    for (i = 0; i < 16; i++) {
        u8 *p = p0 + 0x104 + i * 0x54;

        if (p[0x45] == 2) {
            p[0x45] = 1;
        } else {
            p[0x45] = 0;
        }
    }
}

extern s32 D_80092650;

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Quad; // Size: 0x10

void func_80073CEC(u8 *p0, u8 *p1, s32 arg2, u8 *p3, u8 *p4, s32 arg5, u8 *p6) {
    u32 flags;

    arg2 &= 1;
    *(Quad *)(p4 + 0x10) = *(Quad *)(p4 + 0x00);
    *(Quad *)(p4 + 0x30) = *(Quad *)(p4 + 0x20);
    *(Quad *)(p4 + 0x00) = *(Quad *)p0;
    *(Quad *)(p4 + 0x20) = *(Quad *)p1;
    flags = *(u32 *)(p4 + 0x44);
    flags &= ~2;
    flags |= arg2 << 1;
    *(u8 *)(p4 + 0x43) = arg5;
    *(u32 *)(p4 + 0x44) = flags;
    *(u8 *)(p4 + 0x45) = 2;
    *(u8 *)(p4 + 0x47) = *(u32 *)(p6 + 0x644);
    *(s32 *)(p4 + 0x50) = *(s32 *)(p6 + 0x84);
    *(u8 *)(p4 + 0x46) = *(u8 *)(p3 + 0x1);
    D_80092650++;
}

/* 98.06%: inverting the condition (`if (t >= 0x1001)`) is what makes reorg put
   `li v0,1` in the bnez delay slot instead of a separate block with a `j`, and
   `hw + arg1` fixes the addu operand order. Left over: the original keeps the
   sum in the parameter's $a1 and the hw temp in $v1 (`lh v1; addu a1,v1,a1;
   slti v1,a1`), ours assigns the sum to $v1 and the temp to $v0. Also tried
   `arg1 += hw` in place, a separate s32/s16 local, and `t` declared after the
   load - the sum pseudo never coalesces back into $a1. Also retried this
   session as `arg1 = *(s16 *)(p0 + 0xB6) + arg1` in place and with a `hw`
   local (both 97.78%, operands come out swapped as well); 2.6.0 gives 97.22
   and 2.7.2-cdk 97.78, so the preset is not the lever. */
/* s32 func_80073DE4(u8 *p0, s32 arg1) {
    s32 t = *(s16 *)(p0 + 0xB6) + arg1;

    if (t >= 0x1001) {
        return ((t - 0xFF1) / 20) < *(s16 *)(p0 + 0xB4);
    }
    return 1;
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073DE4);

extern s32 D_800928C8;
extern void func_8008EB4C(s32);

s32 func_80073E2C(u8 *p0, s32 arg1, s32 arg2) {
    s16 t;
    s32 q;

    if (D_800928C8 == 4) {
        return 1;
    }
    if (D_800928C8 == 6) {
        return 1;
    }
    t = *(u16 *)(p0 + 0xB6) + arg1;
    *(s16 *)(p0 + 0xB6) = t;
    if (t < 0x1001) {
        return 1;
    }
    q = (t - 0x1000) / 20;
    if (q >= *(s16 *)(p0 + 0xB4)) {
        *(s16 *)(p0 + 0xB6) = t - arg1;
        *(s16 *)(p0 + 0xBA) = 0;
        if (func_80083CD8() != 4) {
            func_8008EB4C(0x2E);
        }
        return 0;
    }
    *(s16 *)(p0 + 0xB6) = 0x1000;
    *(s16 *)(p0 + 0xBA) = q;
    switch (arg2) {
    case 1:
        *(s32 *)(p0 + 0x1654) += (s16)q;
        break;
    case 2:
        *(s32 *)(p0 + 0x1658) += (s16)q;
        break;
    }
    return 1;
}

/* 97.87% (428 bytes against the original's 432): the body is right but one
   instruction is scheduled differently. The original hoists `addiu a3,sp,0x10`
   (the vecA argument) to just after `move a0,s2`, which leaves
   `move s3,v0` free for the func_80073B7C delay slot; ours emits it as the last
   argument, it takes that delay slot, and `move s3,v0` then falls into the
   func_8007D25C delay slot, losing one `nop`. Also `lbu a2,1(s1)` in the
   original against `lbu v1,1(s1)` here. Tried a `s32 *pVecA` temp (coalesced
   away, no change) and both if/else orders (the inverted one is the 97.87; the
   direct one is 89.81). gcc 2.7.2-cdk rejects the unit. The frame layout
   (locals at sp+0x10 and sp+0x20, 0x10 unused below) matches with two s32[4]. */
/* extern s32 func_8007CD14(s32, u8, s16, s32);
extern s32 func_80073B7C(u8 *, u8, s16, void *);
extern s32 func_8007D25C(u8);
extern void func_8007CD44(s32, void *, void *, s32);
extern void func_8007D65C(void *, void *);
extern void func_8007C100(u8 *);
extern void func_8007C880(s32, void *, s32, s32);
extern void func_8007D190(void *, s32);

void func_80073F34(u8 *p0, u8 *p1) {
    s32 vecA[4];
    s32 vecB[4];
    s32 r;

    func_8007CD14((*(u32 *)(p0 + 0xD0) >> 27) & 1, *(u8 *)(p1 + 2), *(s16 *)(p1 + 4), 1);
    r = func_80073B7C(p0, *(u8 *)(p1 + 2), *(s16 *)(p1 + 4), vecA);
    if (func_8007D25C(*(u8 *)(p1 + 1)) != 0) {
        s32 *pVecB = vecB;

        func_80073B7C(p0, *(u8 *)(p1 + 3), *(s16 *)(p1 + 6), pVecB);
        if (*(u8 *)(p1 + 1) == 0x10) {
            func_8007CD44((*(u32 *)(p0 + 0xD0) >> 27) & 1, vecA, pVecB, r);
        } else {
            func_8007D65C(vecA, pVecB);
        }
        return;
    }
    if (*(u8 *)(p1 + 2) != *(u8 *)(p1 + 3) || *(s16 *)(p1 + 4) != *(s16 *)(p1 + 6)) {
        func_80073B7C(p0, *(u8 *)(p1 + 3), *(s16 *)(p1 + 6), vecB);
        vecA[0] = (vecA[0] + vecB[0]) / 2;
        vecA[1] = (vecA[1] + vecB[1]) / 2;
        vecA[2] = (vecA[2] + vecB[2]) / 2;
    }
    func_8007C100(p0 + 0x15D4);
    if (*(u8 *)(p1 + 1) >= 0x20) {
        func_8007C880((*(u32 *)(p0 + 0xD0) >> 27) & 1, vecA, r, *(u8 *)(p1 + 1) - 0x20);
    } else {
        func_8007D190(vecA, *(u8 *)(p1 + 1));
    }
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073F34);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800740E4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074678);

void func_80074998(u8 *p0) {
    u32 **pArray = *(u32 ***)(*(u8 **)(*(u8 **)(p0 + 0x5C) + 0x4) + 0x4);
    s32 i;

    for (i = 0; i < *(s16 *)(*(u8 **)(*(u8 **)(p0 + 0x5C) + 0x4)); i++) {
        u32 *pEntry = pArray[i];

        if (pEntry[0] == 1) {
            *(u32 *)*(u32 **)((u8 *)pEntry + 0x4) &= ~1;
        }
    }
    for (i = 0; i < *(u8 *)(p0 + 0x908); i++) {
        u32 *pEntry = pArray[*(u8 *)(*(u8 **)(p0 + 0x904) + i)];

        *(u32 *)*(u32 **)((u8 *)pEntry + 0x4) |= 1;
    }
    if (*(u8 *)(p0 + 0x909) == 0xD) {
        *(u32 *)*(u32 **)((u8 *)pArray[0xD] + 0x4) |= 1;
    }
}

extern void func_80074678(u8 *, s32, u8);
extern void func_8008B0D8(u8 *);
extern void func_8008B730(u8 *, s32, s32);

void func_80074AB4(u8 *p0) {
    u8 *pState = *(u8 **)(p0 + 0x15CC);
    u8 *pData = *(u8 **)(p0 + 0x15D0);
    u8 *pEntry;

    *(s32 *)(p0 + 0x00) = *(s16 *)(pState + 0x0);
    *(s32 *)(p0 + 0x04) = *(s16 *)(pState + 0x2);
    *(s32 *)(p0 + 0x08) = *(s16 *)(pState + 0x4);
    *(u8 *)(p0 + 0x4C) = *(u8 *)(pData + 0x8);
    *(s32 *)(p0 + 0x54) = (*(u16 *)(pState + 0x6) << 20) >> 20;
    pEntry = *(u8 **)(*(u8 **)(*(u8 **)(p0 + 0x5C) + 0x4) + 0x8) + *(u8 *)(pData + 0x8) * 0x14;
    if (*(u16 *)(pData + 0x6) & 0x1000) {
        func_80074998(p0);
        func_8008B0D8(pEntry);
    }
    func_80074678(p0, *(s16 *)(pEntry + 0x12), *(u8 *)(*(u8 **)(p0 + 0x15D0) + 0x9));
    if (*(u8 *)(pData + 0xA) != 0) {
        func_8008B730(pEntry, *(u8 *)(pData + 0x9), *(u8 *)(pData + 0xA));
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074BA4);

extern u8 D_8009872C[];
extern s32 D_80099D8C;
extern s32 D_80099D88;
extern u8 D_80050622;
extern s32 func_8008F4F4(u8 *, s32);

void func_80075060(s32 arg0) {
    u8 *p = D_8009872C;
    u8 *pDst;
    s16 v;
    s32 a;
    s32 b;

    if (arg0 != 0) {
        if (func_8008F4F4(p, 0xE0) != 0) {
            D_80050622 = 2;
            return;
        }
        if (func_8008F4F4(p, 0x10) != 0) {
            D_80050622 = 1;
            return;
        }
        D_80050622 = 3;
        return;
    }
    if (D_80099D8C == 0 && D_80099D88 == 0) {
        D_80050622 = 0x88;
        return;
    }
    if (D_80099D88 == 0) {
        D_80050622 = 0x82;
        return;
    }
    v = *(s16 *)(p + 0xBC);
    a = (v * 176) / 255;
    if (a < *(s32 *)(p + 0x1654)) {
        D_80050622 = 0x83;
        return;
    }
    b = (v * 160) / 255;
    if (b < *(s32 *)(p + 0x1658)) {
        D_80050622 = 0x84;
        return;
    }
    if (a < *(s32 *)(p + 0x1658) + *(s32 *)(p + 0x1654)) {
        D_80050622 = 0x85;
        return;
    }
    pDst = *(u8 **)(p + 0xD8);
    if (func_8008F4F4(pDst, 0xE0) != 0) {
        D_80050622 = 0x86;
        return;
    }
    if (func_8008F4F4(pDst, 0x10) != 0) {
        D_80050622 = 0x81;
        return;
    }
    D_80050622 = 0x87;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800751C8);

s32 func_8007570C(u8 *p0, u8 *p1, s32 scale) {
    return (*(s16 *)(p1 + 0x15EC) * scale) / 32 - *(s16 *)(p0 + 0x15EE);
}
