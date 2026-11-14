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
extern void SoundSetAudioManagerVolumeWithFade(s32 *, s32, s32);
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
extern u8 D_800D2D88[];
extern u8 D_800D3280;
extern u16 D_800C3076[];
extern u16 D_800C3254[];
extern s32 func_80076A10(s32, s32, s16, s16);
extern void func_80072938(POLY_FT4 *, s32, s32, s32);
extern u8 D_800D3298;
extern u8 D_800D2DCC[];
extern u16 D_800CCD64[];
extern u16 D_800CCD68[];
extern u16 D_800CCD6C[];
extern u16 D_800CCD6E[];
extern u8 D_800CCE44[];
extern u16 D_800D2E1C[];
extern u8 D_800C3CF9[];
extern u8 D_800D32A0[];
extern u16 D_800CCDC4[];
extern void func_8008AAA0(s32);

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
        SoundSetAudioManagerVolumeWithFade(D_800C3E54, 0, 0xF0);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80070F40);

s32 func_800716D8(void) {
    if (D_8005917C[0] != -1) {
        func_8028022C();
    }
    func_800BE790();
    return 0;
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007171C);
/* 77.62% under 2.6.0. The whole control flow (the 1/2 decrement, the 0x1000
   bit toggle on D_800D2E1C, the 0x2000 timer, the 0x80/0x1000 guards and the
   0xB wrap) is right; the miss is that gcc strength-reduces the most-used
   `(u8 *)SYM + off` (D_800CCD64, four uses) into a pointer biv, so its base is
   hoisted once into a3 and the loads become `lhu 0(a3)`, where retail re-forms
   `lui $at,%hi(SYM); addu $at,$at,$a0` at every use and keeps `off` an integer
   biv. Tried: `off` as a running for-clause biv, `off = i * 0x170` inline (both
   77.62%), one-`if`-per-level nesting without continue (70.32%), splitting the
   two entry tests. The other four symbols already come out in the retail form.
void func_8007171C(void) {
    u16 v;
    u8 *pTimer;
    s32 dec;
    s32 i;
    s32 off;

    if (D_800D3298 == 0) {
        return;
    }

    for (i = 0; i < 0xB; i++) {
        off = i * 0x170;
        if (D_800D2DCC[i] == 0) {
            continue;
        }
        if (D_800D2DE4[i] != 0) {
            continue;
        }
        dec = 1;
        if ((*(u16 *)((u8 *)D_800CCD6C + off) | *(u16 *)((u8 *)D_800CCD6E + off)) & 0x8000) {
            dec = 2;
        }
        if (*(u16 *)((u8 *)D_800CCD64 + off) & 0x1000) {
            v = *(u16 *)((u8 *)D_800D2E1C + i * 2) ^ 1;
            *(u16 *)((u8 *)D_800D2E1C + i * 2) = v;
            if (v != 0) {
                continue;
            }
        }
        v = *(u16 *)((u8 *)D_800CCD64 + off);
        if (v & 0x2000) {
            pTimer = (u8 *)((u8 *)D_800CCE44 + off);
            *pTimer -= dec;
            if (*pTimer != 0) {
                continue;
            }
            v = *(u16 *)((u8 *)D_800CCD64 + off);
            *pTimer = 0;
            *(u16 *)((u8 *)D_800CCD64 + off) = v & 0xDFFF;
            continue;
        }
        if (v & 0x80) {
            continue;
        }
        if (*(u16 *)((u8 *)D_800CCD68 + off) & 0x1000) {
            continue;
        }
        pTimer = (u8 *)((u8 *)D_800D2E06 + i * 2);
        v = *(u16 *)pTimer - dec;
        *(u16 *)pTimer = v;
        if ((s16)v > 0) {
            continue;
        }
        D_800D2DE4[i] = 1;
        *(u16 *)pTimer = 0;
    }
}
*/
void func_800718BC(void) {
    u8 index;

    index = D_800C3EAC[0x2D3];
    if (D_800D2DE4[index] != 0xFF) {
        D_800D2DE4[index] = 0;
    }
    D_800D2E06[D_800C3EAC[0x2D3]] = func_80098AF8(D_800C3EAC[0x2D3], 0);
    D_800D2E06[D_800C3EAC[0x2D3] - 0xB] = D_800D2E06[D_800C3EAC[0x2D3]];
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071964);
/* 99.51% under 2.6.0 and 2.7.2: every instruction is identical except the
   leading flag load, which retail puts in $a1 (a dead argument register at
   entry) and gcc in $v0 (reused by the later lhu). Both pseudos have one use
   and no live range overlap, so this is an allocator tie; the and-operand
   register pair is identical. Tried: compound &&, !(), nested ifs, volatile on
   the flag, an int local for the flag.
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
        p = D_800C3EAC;
        *(p + i + 0x2EB) = 0;
    }
}

void func_80071A38(void) {
    s32 i;
    u8 *pDst;
    u8 *pSrc;

    // `*(p + i + k)` and not `p[i + k]`: only the former puts the base first in
    // the addu under both 2.6.0 and 2.7.2, which is the original's order.
    for (i = 0; i < 3; i++) {
        pDst = D_800D2D28;
        pSrc = D_800C3EAC;
        *(pDst + i + 0x7C) = *(pSrc + i + 0x2EB);
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
/* 72.36% under 2.6.0 and 2.7.2. The pointer pair (pSrc = pDst - 0xB gives the
   original addiu s2,s1,-22) and every operand are right; gcc's loop.c hoists
   `li s3,1` for the flag store into a callee-saved register, so the frame is
   0x28 instead of 0x20 and the D_800D2D28 load lands after the sh instead of
   before the lhu. Tried under 2.6.0: 0x7C store before/after *pDst = *pSrc.
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
/* 68.84% under 2.6.0 (69.63 under 2.7.2). Whole structure is right (0x18 frame,
   both early-exit paths, the 0xB wrap and the *pCurrent = i + 1 tail). Misses:
   gcc invents a 4th lbu of D_800D2DC0 in the middle block, reloads
   D_800D2DD8[i] after the loop instead of reusing the loop's slot register,
   materialises a second `li 0xb` for the tail compare where the original keeps
   a3, and hoists `move a0,zero` out of the final jal's delay slot.
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
/* 78.84% under both 2.6.0 and 2.7.2: the original recomputes index*40 inline
   each iteration (sll 2 / addu / sll 3, the sll 2 rotated into the branch delay
   slot) while gcc strength-reduces an int index into a pointer (+80/iteration)
   and hoists pPrim into it. An s16 index emits the original's inline multiply
   but adds a sll/sra pair per use. Tried under 2.6.0: index += 2 in the body,
   index += 2 in the for clause, u32/s16/s32 index.
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
void func_80072DA8(s32 arg0, u8 arg1) {
    s32 first;
    s32 i;

    first = *(s32 *)(D_800D2D28 + arg0 * 4 + 0xE0);
    for (i = 0; i < 5; i++) {
        if (D_800D2D88[i] != 0xFF) {
            *(s32 *)(D_800D2D28 + arg0 * 4 + 0xE0) += func_80076A10(
                D_800D2D88[i] + 0x67,
                (s32)(D_800C3EA4 + (arg0 * 3200 + 0x3A88)) + *(s32 *)(D_800D2D28 + arg0 * 4 + 0xE0) * 80,
                (s16)(D_800C3076[arg0 * 24 + i] + D_800C3254[D_800D3280 * 3 + arg0]),
                0x10);
        }
    }
    if (arg1 != 0) {
        func_80072938((POLY_FT4 *)(D_800C3EA4 + (arg0 * 3200 + 0x3A88)), first,
                      *(s32 *)(D_800D2D28 + arg0 * 4 + 0xE0), arg1);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072F38);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073380);
/* 99.82% under 2.6.0: only two `addu` operand orders differ in the first block.
   Retail does `addu v1,v1,v0` (the sum lands in the register that held
   D_800C3EA4, then `addu at,v1,at` for the 0x10000); gcc's commutative-operand
   precedence puts the `arg0 * 484` product first, so it emits `addu v0,v0,v1`
   and `addu at,v0,at`. Everything else - the 484-byte stride, the 0x853D
   displacement, the two amount sources, the func_80076A10 loop (80-stride,
   *i*6 accumulate, 0x25) and the final byte store - is exact. Tried: the
   expression inline as `D_800C3EA4 + (arg0*484 + 0x853D)`, a `u8 *pFlags` local,
   and a separate `off = arg0 * 484` temp; all give the same 99.82%.
void func_80073380(s32 arg0) {
    u8 *pFlags;
    s16 amount;
    s32 off;
    s32 i;

    off = arg0 * 484;
    pFlags = D_800C3EA4 + off;
    if (*(u8 *)(pFlags + 0x853D) == 1) {
        amount = *(u8 *)(D_800D32A0 + arg0 * 8);
    } else {
        amount = *(u16 *)((u8 *)D_800CCDC4 + arg0 * 0x170);
    }
    func_8008AAA0(amount);

    for (i = 0; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            *(s32 *)(D_800D2D28 + arg0 * 4 + 0xEC) += func_80076A10(
                D_800C3CF9[i] + 0x83,
                (s32)(D_800C3EA4 + (arg0 * 320 + 0x6008)) + *(s32 *)(D_800D2D28 + arg0 * 4 + 0xEC) * 80,
                (s16)(arg0 * 96 + (D_800C3254[D_800D3280 * 3 + arg0] + 0x4A) + i * 6),
                0x25);
        }
    }
    *(u8 *)(D_800D2D28 + arg0 + 0x99) = *(u8 *)&D_800CCB34;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073538);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073A58);
/* 91.30% - the three colour stores, the `lbu 0x6415` guard, the reload-after-store
   pattern (D_800C3EA4 is a u8 * so the sb forces a reload) and both `addu a1,a1,v1`
   (offset-first) are exact. The only remaining miss is the link address: retail
   keeps `&D_800CCB04` in s0 (one lui/addiu, then `lw a0,0(s0)` per call, 0x18 frame
   with s0 saved) while gcc re-forms lui/lw per use and needs no saved register
   (0x10 frame). Tried: `u8 **pOt = &D_800CCB04` then `*pOt` (constant-propagated
   away) and the bare `D_800CCB04 + 4` form (identical 91.30%). Same open problem as
   misc2's func_800743A4 and func_800744BC - no shape found in the repo makes gcc
   materialise the address of a global once across calls.
void func_80073A58(void) {
    if (((u8 *)D_800C3EA4)[0x6415] != 0) {
        ((u8 *)D_800C3EA4)[0x63CC + ((u8 *)D_800C3EA4)[0x6414] * 24] = ((u8 *)D_800C3EA4)[0x6410];
        ((u8 *)D_800C3EA4)[0x63CD + ((u8 *)D_800C3EA4)[0x6414] * 24] = ((u8 *)D_800C3EA4)[0x6410];
        ((u8 *)D_800C3EA4)[0x63CE + ((u8 *)D_800C3EA4)[0x6414] * 24] = ((u8 *)D_800C3EA4)[0x6410];
        AddPrim(D_800CCB04 + 4, (0x63C8 + ((u8 *)D_800C3EA4)[0x6414] * 24) + (u8 *)D_800C3EA4);
        AddPrim(D_800CCB04 + 4, (0x63F8 + ((u8 *)D_800C3EA4)[0x6414] * 12) + (u8 *)D_800C3EA4);
    }
}
*/
