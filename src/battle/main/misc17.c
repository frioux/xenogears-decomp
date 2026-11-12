#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern u8 D_800D2D24[];
extern u8 *D_800D2D28;
extern u8 D_800C3E50;
extern u8 *D_800C34B0;
extern u8 *D_800C3EA4;
extern s32 D_800CCB34;
extern u8 *D_800C3DFC;
extern u8 *D_800C3E34;
extern u8 D_800CCD3E[];
extern u8 D_800D2C54[];
extern u8 D_800D2C88[];
extern u16 D_800D2C9E;
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern u8 D_800CCD62[];
extern u8 *D_800C3E00;
extern u8 D_800CCE42[];
extern u8 D_800D32A1[];
extern u8 D_800C3CFA[];
extern u16 g_SystemPalette1;
extern u8 D_800D32A1[];
extern void *D_800D367C;

void func_8007FB70(void);
void func_8009AC48(u8, s32);
void func_800916D4(u8, u8, s32);
void func_8008AAA0(u32);
void func_80076C78(POLY_FT4 *, s16, s16, u8, u8, s16);
void func_80076CE8(POLY_FT4 *, s16, s16, u8, u8, s16, s16);
s32 func_8009A258(u8, s32);
void func_8009187C(u8, u8, u8);
void func_80091B38(u8, u8, u8);
void func_80093578(u8, u8);
void func_8009382C(u8, u8);
s32 func_80076A10(s32, u8 *, s32, s16);
s32 func_80089C6C(s32, u8);
extern void *func_8008AC00(s32);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern void func_800769E8(RECT *, u_long *);
extern u8 *D_800C3DE8;
extern u8 D_800CCD88[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091064);
/* 94.88%: this `+=` form reproduces the original's instruction ORDER exactly
   (accumulator load before `i++`, `addiu s0` in the bnez delay slot); only the
   three remaining addu's operand/dest regs differ. The `s32 size = f();
   acc = size + acc;` spelling fixes the accumulate (orig v0 = ret + acc,
   `+=` gives v1 = acc + ret) but hoists `i++` over the 0xF8 load and adds a
   nop in the lw-delay slot. Also tried: (s16)(arg0+offset) vs
   (s16)(offset+arg0) (gcc emits s2,s0 either way, original s0,s2), the
   arg1 pointer sum in both operand orders, and =f()+a. Operand-order tie.
   Re-screened for a compiler swap: 94.88 under 2.7.2 and 94.88 under
   2.7.2-cdk (a tie), 92.12 under gcc-2.6.0 - this preset is the right one. */
/*
void func_80091604(s16 arg0) {
    s32 i;
    s32 offset;

    *(s32 *)(D_800D2D28 + 0xF8) = 0;
    for (i = 0, offset = 0x38; i < 8; i++, offset += 8) {
        *(s32 *)(D_800D2D28 + 0xF8) += func_80076A10(0x66,
            D_800C3EA4 + 0xBA8 + *(s32 *)(D_800D2D28 + 0xF8) * 80, 0x20,
            (s16)(offset + arg0));
    }
    D_800D2D28[0xA5] = *(u8 *)&D_800CCB34;
    D_800D2D28[0x9C] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091604);
/* 86.04%: the s/2 and s-s/2*2 expansion, both UV args and the pointer sum are
   insn-identical; what is left is expansion order (gcc hoists `li a1,0x20` /
   `li a2,0xA4` to the top here, the original materialises them after the UV
   temps) and the arg1 narrowing landing in v0 instead of in place in a1.
   Tried: %2 vs the explicit s-s/2*2 form, u32 vs u8 temps for the quotient,
   pointer sum in both operand orders.
   Re-screened: best 91.42 under gcc-2.6.0 (76.76 under cdk). There the left-
   overs are different and partly source level - 2.6.0 narrows arg1 with
   `sll/sra` where the original has `andi a1,s1,0xffff` (a signed-type hint),
   and it keeps `li a1,0x20`/`li a2,0xA4` before the s/2*2 chain, not after. */
/*
void func_800916D4(u8 arg0, u8 arg1, s32 arg2) {
    u32 s;
    u8 c;
    s32 i;
    u16 k;

    s = arg1 * 2 + arg0;
    func_80076CE8(
        (POLY_FT4 *)(D_800CCB34 * 0x28 + 0x140 +
                     (s32)(*(u8 **)(D_800C3EA4 + 0xA230))),
        0x20, 0xA4, (s - s / 2 * 2) * 0x78 & 0xF8, s / 2 * 16, 0x60, 0x10);
    (*(u8 **)(D_800C3EA4 + 0xA230))[0x66E] = 0;
    if (D_800D2D24[(u8)arg2] == 1) {
        func_8008AAA0(func_8009A258((u8)arg2, s + 0x16));
        for (i = 0, k = 0x8C; i < 3; i++, k += 8) {
            c = D_800C3CFA[i];
            if (c == 0xFF) {
                continue;
            }
            func_80076C78(
                (POLY_FT4 *)((*(u8 **)(D_800C3EA4 + 0xA230)) +
                             (*(u8 *)(*(u8 **)(D_800C3EA4 + 0xA230) + 0x66E) *
                                  2 +
                              D_800CCB34) *
                                 0x28 +
                             0x190),
                k, 0xA6, (c * 8 + 0x78) & 0xF8, 0, 8);
            (*(u8 **)(D_800C3EA4 + 0xA230))[0x66E]++;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800916D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009187C);
/* 87.45%: the whole shape (index branch, RECT, two string draws, two
   func_80076C78 calls, HeapFree) reproduces; the leftovers are the
   &D_800CCB34 wall (retail holds `lui s3/addiu s3` once and does `lw 0(s3)`
   twice, gcc rematerialises `lui/lw` per call) and the `andi s0,s0,0xffff`
   retail emits at index's first use (gcc proves the u8-derived range here and
   drops it). Also tried: the (arg2*2+arg1)*2 sum in a local before the branch
   (74.68 - it hoists the whole vofs computation to the top of the frame). */
/*
void func_80091B38(u8 arg0, u8 arg1, u8 arg2) {
    RECT rect;
    u8 *pBuf;
    u16 index;
    s32 len1;
    s32 len2;

    if (D_800D32A1[arg0 * 8] == 0) {
        index = D_800D2D24[arg0] * 0x20 + (arg2 * 2 + arg1) * 2;
    } else {
        index = g_GameState.characters[D_800D2D24[arg0]].gearId * 0x20 + (arg2 * 2 + arg1) * 2;
    }
    pBuf = (u8 *)func_8008AC00(0x39);
    bzero(pBuf, 0x618);
    len1 = SystemRenderStringEntry(GetStringEntry(D_800D367C, index), pBuf, 0x39, 0);
    len2 = SystemRenderStringEntry(GetStringEntry(D_800D367C, index | 1), pBuf, 0x39, 1);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    func_800769E8(&rect, pBuf);
    func_80076C78((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x280), 0x30, 0xB6, 0, 0, len1 & 0xFF);
    func_80076C78((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x2D0), 0x30, 0xC6, 0, 0, len2 & 0xFF);
    HeapFree(pBuf);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091B38);
/* 98.40%: the body is byte-identical; only the entry copies differ - the
   original parks arg1 in a3 and copies a3 to s2, gcc here copies a1 straight
   to s2, so `addu a1,a3,a1` reads s2. Tried: arg1 as s32 (needed, the body
   masks it to u8) and the if/else-if shape that reproduces the call CFG.
   Re-screened: 98.40 under gcc-2.6.0 as well (73.63 under cdk), so the
   a3/s2 entry copy is not a compiler-version artifact. */
/*
void func_80091D38(u8 arg0, s32 arg1, u8 arg2) {
    u8 ret = 0;

    if (D_800D32A1[arg0 * 8] == 0) {
        ret = (u16)func_80089C6C(
                  (u16)g_GameState.unk16BC[D_800D2D24[arg0]].unk6,
                  arg1 + arg2 * 2) != 0;
    } else if ((u16)func_80089C6C(
                   (u16)g_GameState.unk16BC[D_800D2D24[arg0]].unkA,
                   arg1 + arg2 * 2) != 0) {
        ret = 1;
    }
    if (ret != 0) {
        func_800916D4(arg1, arg2, arg0);
        func_8009187C(arg0, arg1, arg2);
        func_80091B38(arg0, arg1, arg2);
        (*(u8 **)(D_800C3EA4 + 0xA230))[0x66A] = *(u8 *)&D_800CCB34;
        (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 1;
    } else {
        (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 0;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091D38);
/* 64.13%: shape is right (two palette stores, four func_80076CE8 calls with
   the arg0/0x60/0x10 stack args, trailing 0x668 store) but retail keeps
   `&D_800CCB34` in s0 (`lui/addiu` once) where gcc rematerialises
   `lui/lw D_800CCB34` at each use and a `lui/lbu` at the end; that also costs
   the 4th saved register (frame 56 vs 48) and swaps the s1/s2 roles of
   arg0/0x60. Tried: plain D_800CCB34 (64.13), `s32 *pIndex = &D_800CCB34`,
   a `u8 *pPal` plus an s32 cache of it (59.64, and the body grows). This is
   misc16's documented &D_800CCB34 wall. */
/*
void func_80091EC4(u8 arg0) {
    *(u16 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0xE) = g_SystemPalette1;
    *(u16 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x5E) = g_SystemPalette1;
    func_80076CE8((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28), 0x34, 0x34, 0, arg0, 0x60, 0x60);
    func_80076CE8((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x50), 0xB8, 0x34, 0x78, arg0, 0x60, 0x60);
    func_80076CE8((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0xA0), 0x9C, 0x34, 0, arg0, 0x10, 0x60);
    func_80076CE8((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0xF0), 0x120, 0x34, 0x40, arg0, 0x10, 0x60);
    *(u8 *)(*(u8 **)(D_800C3EA4 + 0xA230) + 0x668) = *(u8 *)&D_800CCB34;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091EC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009209C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092298);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092784);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092B74);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800930AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093578);
/* 72.03%: RECT/bzero/GetStringEntry/SystemRenderStringEntry order, both stack
   args and the frame all match. Same blocker as misc16's func_8009070C
   (78.72%): the original loads *(D_800C3EA4+0xA230) FIRST, keeps &D_800CCB34
   in a saved reg (lw 0(s3)) and does `addiu a0,a0,0x280` before the base addu;
   gcc loads the D_800CCB34 index first, re-forms its address per call and sinks
   the addiu into the jal delay slot. Improves to 74% with the index in a local.
   Re-screened: best 84.56 under gcc-2.6.0 (71.40 under cdk). There the frame
   is 56 vs 2.6.0's 48 - the original spills the &D_800CCB34 index into s3,
   2.6.0 does not re-form it - and the addiu/addu order of the +0x280/+0x2D0
   adds is still swapped. */
/*
void func_8009382C(u8 arg0, u8 arg1) {
    RECT rect;
    u8 *pBuf;
    u16 index;
    s32 len1;
    s32 len2;

    index = (D_800CCD88[arg0 * 0x170] * 4 + arg1) * 2;
    pBuf = (u8 *)func_8008AC00(0x39);
    bzero(pBuf, 0x618);
    len1 = SystemRenderStringEntry(GetStringEntry(D_800C3DE8, index), pBuf, 0x39, 0);
    len2 = SystemRenderStringEntry(GetStringEntry(D_800C3DE8, index | 1), pBuf, 0x39, 1);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    func_800769E8(&rect, pBuf);
    func_80076C78((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x280),
                  0x88, 0xB6, 0, 0, len1 & 0xFF);
    func_80076C78((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x2D0),
                  0x88, 0xC6, 0, 0, len2 & 0xFF);
    HeapFree(pBuf);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009382C);
// 100.0% under gcc 2.6.0-psx (99.81/99.76/99.90 under this unit's 2.7.2): this
// function wants a 2.6.0 TU of its own; the unit as a whole is a 2.7.2 unit.
void func_800939CC(u8 arg0, u8 arg1) {
    u16 tbl[4];
    s32 o;

    tbl[0] = D_800C324E;
    tbl[1] = D_800C3250;
    tbl[2] = D_800C3252;
    tbl[3] = D_800C323A;
    if ((u16)func_80089C6C((u16)g_GameState.unk16BC[D_800D2D24[arg0]].unk1E,
                           arg1)) {
        o = arg0 * 0x170;
        if ((*(u16 *)(D_800CCD62 + o) & tbl[arg1]) == 0) {
            func_80093578(arg0, arg1);
            func_8009382C(arg0, arg1);
            (*(u8 **)(D_800C3EA4 + 0xA230))[0x66A] = *(u8 *)&D_800CCB34;
            (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 1;
        } else {
            (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 0;
        }
    } else {
        (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093B08);
void func_8009413C(u8 arg0, u8 arg1) {
    if (D_800D2D24[arg0] == 4) {
        D_800D2D28[0xB7] = 0;
        D_800D2D28[0xB0] = 0;
        if (arg1 != 0) {
            func_8007FB70();
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800941A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800946F4);
/* 99.77%: instruction multiset identical; the only difference is the operand
   order of one addu for i*0x170 + pBase (original base-first, gcc index-first).
   Tried: array-ref, pointer-plus (this version), integer base and a pointer
   local for that sum; also 2.7.2-cdk -> 99.77. */
void func_80094C78(void) {
    u8 *pBase = D_800C34B0;
    u8 *pOther = (u8 *)(D_800C3E50 * 4 + (u32)pBase);
    u8 *pRow;
    u8 i;

    for (i = 0; i < 3; i++) {
        if (((u8 *)(pBase + i))[0x5FA0] != 0) {
            continue;
        }
        pRow = pBase + i * 0x170;
        if (pRow[0x15A] & 0x80) {
            *(u32 *)(i * 4 + (s32)pBase + 0x5F60) += *(u32 *)(pOther + 0x5F6C);
        } else {
            *(u32 *)(i * 4 + (s32)pBase + 0x5F54) += *(u32 *)(pOther + 0x5F6C);
        }
    }
}
/* 91.61% first attempt: the whole shape is right (two count loops, the
   if/else-if pair, the two duplicated fix loops, u8 counters masked with
   0xFF). Left to close: the beqz delay slot (the original puts `i++` there,
   gcc leaves a nop and emits it after the counter increment) and the
   base/index order of the row-pointer addu in the two fix loops
   (original `addu a0,t1,v1` base-first, gcc `addu v1,a3,v1`). Making the
   count loops use a row pointer fixed their addu order (91.47 -> 91.61).
   Tried: p[0x3E] vs the full expression, one u16 *p reused vs re-declared
   per loop. Un-parking needs `extern u16 D_800C3AA4[];`.
   Re-screened: best 96.96 under gcc-2.6.0 (90.04 under cdk). There only the
   two count loops' registers differ (n1 in a2 vs t0, base in a0 vs a3), both
   row addu's are operand-swapped, and one `j`/`nop` pair is left over. */
/*
void func_80094D24(void) {
    u8 i;
    u8 n1;
    u8 n2;
    u16 *p;

    for (i = 0, n1 = 0; i < 3; i++) {
        p = (u16 *)(D_800C34B0 + i * 0x170);
        if (p[0x3E] & 0xC000) {
            n1++;
        }
    }
    for (i = 0, n2 = 0; i < 3; i++) {
        p = (u16 *)(D_800C34B0 + i * 0x170);
        if (p[0x3E] & 2) {
            n2++;
        }
    }
    if (n1 == 1 && n2 == 2) {
        for (i = 0; i < 3; i++) {
            p = (u16 *)(D_800C34B0 + i * 0x170);
            if (p[0x3E] & 2) {
                p[0x3E] &= ~2;
                p[0x3D] = D_800C3AA4[i];
            }
        }
    } else if (n1 == 2 && n2 == 1) {
        for (i = 0; i < 3; i++) {
            p = (u16 *)(D_800C34B0 + i * 0x170);
            if (p[0x3E] & 2) {
                p[0x3E] &= ~2;
                p[0x3D] = D_800C3AA4[i];
            }
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094D24);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094EE4);
void func_80095690(void) {
    u8 *p = D_800C3E00;
    u8 *q = D_800C3DFC;
    u8 *r;
    s16 v;
    u16 f;

    v = p[0x5B] * q[0x11];
    if (*(u16 *)(p + 0x8A) & 0x2000) {
        v *= 2;
    }
    r = D_800C3E34;
    f = *(u16 *)(r + 0x8C) | *(u16 *)(r + 0x8E);
    if (f & 0x100) {
        v = 7 * v / 10;
    }
    if (f & 0x200) {
        v = 13 * v / 10;
    }
    if (*(u8 *)(D_800CCE42 + D_800C3E50 * 0x170) & 0x80) {
        v = 0;
    }
    *(u8 *)(D_800C34B0 + D_800C3E50 + 0x5FA0) = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v;
}
/* 95.00% (this shape): instruction multiset identical; gcc emits the sb's
   `li 2` before the five `sh zero` stores (the original keeps it after), and
   picks v1/a1 for the p/index pair where the original has v0/v1, which also
   drags the /10 `mfhi` to a2 instead of v0. Tried: index/p as locals
   (this, 95.00), both globals inline (89.30 - nop after the p load, index load
   sunk past the stores), both declaration orders, `p` reassigned for the
   0x4E read. Operand/allocator tie.
   Re-screened: best 95.16 under gcc-2.6.0 (69.17 under cdk). There the p/index
   pair is v1/a1 against the original's v0/v1, and the /10 `mfhi` follows it. */
/*
void func_800957D8(void) {
    s32 t;
    u8 *p;
    s32 v;

    if (D_800CCD3E[D_800C3E50 * 0x170] == 2) {
        func_8009AC48(D_800C3E50, 1);
    }
    t = D_800C3E50;
    p = D_800C3E34;
    *(u16 *)(p + 0x7C) = 0;
    *(u16 *)(p + 0x80) = 0;
    *(u16 *)(p + 0x84) = 0;
    *(u16 *)(p + 0x88) = 0;
    *(u16 *)(p + 0x8C) = 0;
    D_800D2C88[t] = 2;
    p = D_800C3E34;
    v = *(u16 *)(p + 0x4E) * D_800C3DFC[0x11];
    t = D_800C3E50;
    *(s32 *)(t * 4 + D_800D2C54) = v / 10;
    D_800D2C9E |= 1 << t;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800957D8);
// 100.0% under gcc 2.6.0-psx (99.81/99.76/99.90 under this unit's 2.7.2): this
// function wants a 2.6.0 TU of its own; the unit as a whole is a 2.7.2 unit.
void func_800958D8(void) {
    if (rand() % 100 > D_800C3DFC[0x1C]) {
        return;
    }
    if (D_800C3DFC[0x1D] != 0x6E) {
        return;
    }
    if (*(u16 *)(D_800C3DFC + 0x1E) & 0x8000) {
        *(u16 *)(D_800C3E34 + 0x84) = 0;
    }
    if (*(u16 *)(D_800C3DFC + 0x1E) & 0x4000) {
        *(u16 *)(D_800C3E34 + 0x86) = 0;
    }
    if (*(u16 *)(D_800C3DFC + 0x1E) & 0x2000) {
        *(u16 *)(D_800C3E34 + 0x88) = 0;
    }
    if (*(u16 *)(D_800C3DFC + 0x1E) & 0x1000) {
        *(u16 *)(D_800C3E34 + 0x8A) = 0;
    }
    if (*(u16 *)(D_800C3DFC + 0x1E) & 0x800) {
        *(u16 *)(D_800C3E34 + 0x8C) = 0;
    }
    if (*(u16 *)(D_800C3DFC + 0x1E) & 0x400) {
        *(u16 *)(D_800C3E34 + 0x8E) = 0;
    }
    D_800C34B0[0x5FC7] = 0x3A;
}
