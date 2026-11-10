#include "common.h"
#include "psyq/libgpu.h"

extern u8 D_800C3D48;
extern u8 D_800C48EA;
extern s32 *D_800C3E54;
extern u8 *D_800C3EAC;
extern u8 *D_800D2D28;
extern u8 D_800D3725[];
extern s32 D_800D3284;
extern s32 D_800D328C;
extern s32 *D_8005917C;
extern u8 *D_800CCB04;
extern u8 *D_800C3EA4;

extern void func_8008AB4C(void);
extern s32 func_8008ABB8(s32, s32);
extern void func_8008AC50(void);
extern void func_801E5160(void);
extern s32 func_801E563C(void);
extern void func_801E879C(s32);
extern void func_8028022C(void);
extern void func_800BE790(void);
extern void func_8003A89C(s32 *, s32, s32);
extern void ArchiveReadFileToBuffer(s32, void *, u32, u32);
extern u8 D_800D2CAF;
extern u16 D_800D2C94;
extern u16 D_800C48E8;
extern u8 *D_800D39F0;
extern s32 D_800D39C0[];
extern u8 D_800D39C6;
extern u8 *GetStringEntry(u8 *);
extern u8 D_800D3014;
extern u8 D_800D2DE4[];
extern u8 D_800D2DC0;
extern u8 D_800D2DD7[];
extern u8 D_800D2DD8[];
extern u8 D_800D2DE3[];
extern u16 D_800D2E04[];
extern u16 D_800D2DF0[];
extern u16 D_800D2E06[];
extern u16 D_800D2C9E;
extern u16 func_80089C9C(u16, u8);
extern u16 D_800D39E0;
extern u8 D_800C402F[];
extern u8 D_800D2E5C;
extern u8 D_800D2E60;
extern void func_80085AC4(u8);
extern void func_80071B94(s32);
extern u16 func_80098AF8(s32, s32);
extern void func_80079E18(s32);
extern void func_80079E4C(s32);
extern s32 SystemRenderStringEntry(u8 *, s32, s32, s32);
extern s32 D_800CCB34;

void func_80070E2C(void) {
    s32 first;
    s32 second;

    if (D_800C3D48 != 0) {
        func_8008AB4C();
        first = func_8008ABB8(4, 1);
        D_800D3284 = first;
        second = func_8008ABB8(first + 0x7FE1B000, 1);
        D_800D328C = second;
        ArchiveReadFileToBuffer(1, (void *)0x801E5000, 0, 0x80);
        func_8008AC50();
        func_801E5160();
    }
}

void func_80070EB0(u8 arg0) {
    if (D_800C3D48 != 0) {
        func_801E879C(arg0);
    }
}

void func_80070EDC(void) {
    u8 result;

    result = 0;
    if (D_800C3D48 != 0) {
        result = func_801E563C();
    }
    if (result == 0 && D_800C48EA == 0x81) {
        func_8003A89C(D_800C3E54, 0, 0xF0);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80070F40);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800716D8);
/* 78.82% - the C below is correct: it scores 100.00% under gcc-2.6.0. This unit
   is Default (2.6.0 breaks func_80071A38 to 99.05), and 2.7.2 orders the tail
   differently: ours is `lw ra; move v0,zero; jr ra; addiu sp` where the original
   is `addu v0,zero,zero; lw ra; addiu sp; jr ra; nop` (2.7.2 fills the jr's delay
   slot with the sp adjust and schedules lw ra before the return-value move).
   Tried: return 0, s32 ret = 0 at top / after the call, ret = 0; return ret.
s32 func_800716D8(void) {
    if (D_8005917C[0] != -1) {
        func_8028022C();
    }
    func_800BE790();
    return 0;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007171C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800718BC);
/* 94.76% - the body below is EXACTLY right: it scores 100.00% under gcc-2.6.0
   (so this unit is probably a 2.6.0 unit; see the note on func_800716D8). Under
   the unit's 2.7.2 the only difference is the tail: 2.7.2 schedules `lw ra`
   before the final `sh` and puts `addiu sp` in the `jr`'s delay slot.
   The old 87.57% snag is solved: the second table must be spelled relative to the
   first one, `D_800D2E06[i - 0xB]`, which is what makes gcc emit the original's
   `addiu a0,v0,-0x16` instead of re-forming D_800D2DF0's lui/addiu.
void func_800718BC(void) {
    u8 index;

    index = D_800C3EAC[0x2D3];
    if (D_800D2DE4[index] != 0xFF) {
        D_800D2DE4[index] = 0;
    }
    D_800D2E06[D_800C3EAC[0x2D3]] = func_80098AF8(D_800C3EAC[0x2D3], 0);
    D_800D2E06[D_800C3EAC[0x2D3] - 0xB] = D_800D2E06[D_800C3EAC[0x2D3]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071964);
/* 98.78% - only the register for the leading lbu differs (original a1, gcc v0,
   forcing the two lhu temps to swap v0/v1). Tried: two early-return ifs, one
   compound && if, and nested ifs (49%), u8 mode local + !(), u16 mask local
   with nested ifs (no change: the miss is an allocator tie-break on the lbu).
void func_80071964(void) {
    u8 *pEntry;
    s32 i;

    if (D_800D2CAF != 0 && (D_800D2C94 & D_800C48E8) == 0) {
        pEntry = GetStringEntry(D_800D39F0);
        D_800D39C6 = SystemRenderStringEntry(pEntry, D_800D39C0[0], 0x39, 1);
        LoadImage((RECT *)&D_800D39C0[-2], (u_long *)D_800D39C0[0]);
        for (i = 3; i != 0; i--) {
            func_800716D8();
        }
    }
}
*/

void func_80071A08(void) {
    s32 i;
    u8 *p;

    for (i = 0; i < 3; i++) {
        p = D_800C3EAC + i;
        p[0x2EB] = 0;
    }
}

void func_80071A38(void) {
    s32 i;
    u8 *pDst;
    u8 *pSrc;

    for (i = 0; i < 3; i++) {
        pDst = D_800D2D28 + i;
        pSrc = D_800C3EAC + i;
        pDst[0x7C] = pSrc[0x2EB];
    }
    func_800716D8();
}

void func_80071A8C(void) {
    s32 i;

    for (i = 0x2A0; i >= 0; i -= 0x60) {
        D_800D3725[i] = 0;
    }
    D_800D2D28[0xB5] = 0;
    D_800D2D28[0xB4] = 0;
    func_800716D8();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071AE0);
/* 99.33% - only the (D_800D2C94 & D_800C48E8) test: gcc loads D_800D2C94 into
   v1 first, the original loads it into v0 (the and's result reg). Same miss as
   func_80071964. Tried: operand swap, a u16 local for either side, volatile.
void func_80071AE0(void) {
    s32 i;

    if (D_800D2CAF != 0 && (D_800D2C94 & D_800C48E8) == 0) {
        func_80079E18(7);
        D_800C3EAC[0x2DB] = 0;
        for (i = 0x3B; i != 0; i--) {
            func_800716D8();
            if (D_800D3014 != 8) {
                break;
            }
        }
        D_800C3EAC[0x2DB] = 1;
        func_80079E4C(7);
        func_800716D8();
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071B94);
/* 72.24% - body is exact except the flag store: gcc hoists `li s3,1` out of the
   loop (loop.c invariant) and then schedules the D_800D2D28 store before the sh,
   costing a callee-saved reg and a 40-byte frame vs the original 32; the original
   re-materialises `li v0,1` inside the block. Tried: plain D_800D2D28[i + 0x7C]
   (index-first addu), (D_800D2D28 + i)[0x7C] (base-first addu, now identical),
   store order before/after *pDst = *pSrc (no effect), pFlag pointer local.
   The pointer pair pDst = D_800D2E06, pSrc = pDst - 0xB reproduces the original
   addiu s2,s1,-0x16 exactly. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072270);
/* 72.36% - body and pointer pair (pSrc = pDst - 0xB gives the original's
   addiu s2,s1,-0x16) are right; the miss is the flag store: gcc's loop.c hoists
   `li v0,1` into s3 (extra callee-saved reg, 40-byte frame vs 32) where the
   original re-materialises `li v0,1` inside the block, and it loads D_800D2D28
   after the sh instead of before the lhu. Same blocker as func_80071B94.
void func_80072270(void) {
    s32 i;
    u16 *pSrc;
    u16 *pDst;

    if ((D_800D2C9E & 7) != 0) {
        pDst = D_800D2E06;
        pSrc = pDst - 0xB;
        for (i = 0; i < 3; i++) {
            if (func_80089C9C(D_800D2C9E, i) != 0) {
                D_800D2DE4[i] = 0;
                *pDst = *pSrc;
                (D_800D2D28 + i)[0x7C] = 1;
            }
            pDst++;
            pSrc++;
        }
    }
    D_800D2C9E = 0;
}
*/
void func_80072324(void) {
    u8 *pBuf;
    u8 *p;
    u8 *pEnd;
    s32 i;
    s32 j;

    for (i = 3; i < 0xB; i++) {
        if (func_80089C9C(D_800D39E0, i) != 0) {
            p = pBuf;
            pEnd = p + 0x100;
            do {
                *p++ = 0;
            } while (p < pEnd);
            for (j = 0x8B8; j >= 0; j -= 0x48) {
                D_800C402F[j] = 0xFF;
            }
            D_800D2E5C = 4;
            D_800D2E60 = 0x17;
            func_80085AC4(i);
            func_80071B94(1);
        }
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800723E0);
/* 69.63% - whole structure is right (16-byte frame, both early-exit paths, the
   wrap at 0xB and the *pCurrent = i + 1 tail). Misses: gcc reloads
   D_800D2DD8[i] after the loop instead of reusing the loop's slot register
   (an `idx` local makes it worse, 68.58, and adds an andi), it materialises a
   second `li 0xb` for the tail compare where the original keeps a3, and it
   hoists `move a0,zero` out of the final jal's delay slot. Tried: for/while with
   the increment in the clause vs in the body, idx local, s32 vs u8 locals.
void func_800723E0(void) {
    u8 *pCurrent;
    s32 i;

    if (D_800D39E0 != 0) {
        func_80072324();
        return;
    }

    if (D_800D2DC0 != 0) {
        D_800C3EAC[0x2D3] = D_800D2DC0;
        D_800D2DE3[D_800D2DC0] = 1;
        D_800D2E04[D_800D2DC0] = 0;
        D_800D2DC0 = 0;
        func_80071B94(0);
        return;
    }

    D_800C3EAC[0x2D3] = 0;
    pCurrent = D_800D2DD7;
    for (i = *pCurrent; D_800D2DE4[D_800D2DD8[i]] != 1; i++) {
        if (i == 0xB) {
            i = 0;
        }
        if (i == *pCurrent) {
            break;
        }
    }
    D_800C3EAC[0x2D3] = D_800D2DD8[i] + 1;
    *pCurrent = i + 1;
    if (*pCurrent == 0xB) {
        *pCurrent = 0;
    }
    func_80071B94(0);
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007252C);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800728B8);
/* 78.84% - with an s32/u32 index gcc strength-reduces index*40 into a hoisted
   pointer (+80 per iteration); the original recomputes sll/addu/sll inside the
   loop. An s16 index blocks the reduction but adds sll/sra at each use.
void func_800728B8(u8 *pPrim, s32 count, s16 index) {
    s32 i;

    for (i = 0; i < count; i++) {
        AddPrim(D_800CCB04 + 4, pPrim + index * 40);
        index += 2;
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072938);
/* 83.62% - body is otherwise exact: POLY_FT4 array subscripted by
   `D_800CCB34 + i * 2` repeated per store (which is why the original reloads
   D_800CCB34 four times per iteration), SetShadeTex first, then r0/g0, then b0.
   The `!= 1` spelling is what reproduces the original's branch layout (beq to the
   out-of-line r0/g0 = 0x80 block, fall through to r0 = 0x80 / g0 = 0).
   Left: 2.7.2 keeps the index `i` in a callee-saved reg and re-does `sll i,1` each
   iteration (6 saved regs, 0x30 frame) and re-masks `mode` in the loop; ours makes
   `i*2` itself the loop induction and hoists `andi mode,0xFF` into the preheader
   (5 saved regs, 0x28 frame), which also shifts s3-s5 down by one.
void func_80072938(POLY_FT4 *pPrim, s32 first, s32 last, s32 mode) {
    s32 i;

    for (i = first; i < last; i++) {
        SetShadeTex(&pPrim[D_800CCB34 + i * 2], 0);
        if ((mode & 0xFF) != 1) {
            pPrim[D_800CCB34 + i * 2].r0 = 0x80;
            pPrim[D_800CCB34 + i * 2].g0 = 0;
        } else {
            pPrim[D_800CCB34 + i * 2].r0 = 0x80;
            pPrim[D_800CCB34 + i * 2].g0 = 0x80;
        }
        pPrim[D_800CCB34 + i * 2].b0 = 0;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072A9C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072DA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072F38);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073380);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073538);
/* 91.00% - every instruction but two addu operand orders and one CSE is right.
   Misses: (1) the original keeps `&D_800CCB04` in s0 across both AddPrim calls
   (lui/addiu once, then two `lw a0,0(s0)`), gcc re-forms lui/lw for each use even
   with a `u8 **pOt = &D_800CCB04;` local (constant-propagated away); (2) the delay
   slot add is `addu a1,a1,v1` (offset + base) in the original, gcc canonicalises an
   offset-first `(0x63C8 + idx*24) + p` back to `addu a1,v1,a1`.
   The three colour stores, the `lbu 0x6415` guard and the reload-after-store
   pattern (D_800C3EA4 is a u8 * so the sb forces a reload) all match exactly. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073A58);
/* void func_80073A58(void) {
    u8 **pOt;

    if (((u8 *)D_800C3EA4)[0x6415] != 0) {
        ((u8 *)D_800C3EA4)[0x63CC + ((u8 *)D_800C3EA4)[0x6414] * 24] = ((u8 *)D_800C3EA4)[0x6410];
        ((u8 *)D_800C3EA4)[0x63CD + ((u8 *)D_800C3EA4)[0x6414] * 24] = ((u8 *)D_800C3EA4)[0x6410];
        ((u8 *)D_800C3EA4)[0x63CE + ((u8 *)D_800C3EA4)[0x6414] * 24] = ((u8 *)D_800C3EA4)[0x6410];
        pOt = &D_800CCB04;
        AddPrim(*pOt + 4, (0x63C8 + ((u8 *)D_800C3EA4)[0x6414] * 24) + (u8 *)D_800C3EA4);
        AddPrim(*pOt + 4, (0x63F8 + ((u8 *)D_800C3EA4)[0x6414] * 12) + (u8 *)D_800C3EA4);
    }
}
*/
