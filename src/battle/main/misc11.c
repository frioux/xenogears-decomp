#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern u8 *D_800C3EAC;

extern u8 *D_800C3DE8;
extern u8 *D_800D2D28;
extern u8 *D_800D2DB4;
extern void *D_800D367C;
extern u8 *D_800D3278;
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800D366C;
extern u8 D_800D32A1[];
extern u8 D_800D2DD7[];
extern u8 D_800D2DD8[];
extern u8 D_800C204C;
extern u8 D_800C3202[];
extern u8 *D_800C3EA4;
extern volatile s32 D_800CCB34;

extern s32 func_8008ABB8(s32, s32);
extern void func_8008AB94(void);
extern void BattleReleaseEffectSlotIfChar4(u8);
extern void func_8008AC50(void);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void HeapFree(void *pMem);
extern void bzero(void *p, s32 n);
extern u8 *D_800C3E24;
extern u8 D_800D2C34;
extern u8 func_80084854(u8 arg0, u8 arg1);
extern void func_80093B08(u8 arg0);
extern u16 func_80089C08(u8 arg0);
extern void func_800BC404(u16 arg0);
extern void func_800BCD98(u16 arg0);
extern void func_80088490(u8 arg0);
extern void func_8009AEFC(u8 arg0);
extern void func_800BAF48(u8 arg0);
extern void func_800883AC(u8 arg0);
extern u8 D_800CCE42[];
extern u8 D_800C3EB8[];
extern u8 D_800D2D24[];
extern u8 D_80059179;
extern u8 D_800C4022[];
extern u8 D_800C3FFE[];
extern void func_80077980(void);
extern void func_8009413C(u8 arg0, u8 arg1);
extern u8 D_800D2C0C[];
extern void func_8009A2D4(u8 arg0);
extern void func_80089AF8(u8 arg0);
extern void func_800898F0(u8 arg0);
extern void func_800716D8(void);

/* 82.95%: best of 6 shapes. The loop body matches instruction for
   instruction (four reloads of the volatile counter, the two separate
   `lbu 0(a1)` of pSrc[i], the incrementing pSrc pointer in the branch delay
   slot) and so do the prologue and the tail. What is left is loop.c's
   invariant hoisting: retail hoists `lw a3,D_800C3EA4` and `li t2,0xC` into
   the preheader and leaves `li v1,0x12` in the body, while our gcc hoists
   neither (it keeps the base load and both constants in the body, 1 insn
   shorter) and puts the `li 0xC` in the body. Writing pSrc as a pre-loop
   local (74.02) puts its init before the loop test, which retail has after;
   declaring it inside the loop body (this shape) fixes the order and the
   incrementing pointer. A `u8 *pBase = D_800C3EA4;` pre-loop local does hoist
   the load but reorders the preheader (71.44). -O3 / -G0 / cdk change nothing
   material. */
/*
void func_8007FBE0(u8 arg0, u8 arg1) {
    u8 count;
    s32 i;

    count = arg1;
    if (arg0 == arg1) {
        count = arg1 - 1;
    }
    for (i = 0; i < arg0 - 1; i++) {
        u8 *pSrc = D_800C3202 + arg0 * 6;

        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x910) = 0xC;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x912) = pSrc[i] + 0x5E;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x914) = 0x12;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x916) = pSrc[i] + 0x5E;
    }
    D_800D2D28[0x97] = count;
    D_800D2D28[0x98] = *(u8 *)&D_800CCB34;
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FBE0);
void func_8007FCE8(void) {
    if (D_800D2D28[0xAE] != 0) {
        HeapFree(D_800D367C);
        D_800D2D28[0xAE] = 0;
    }
}

void func_8007FD38(u8 arg0) {
    s32 type;

    if (D_800D2D28[0xAE] != 0) {
        return;
    }
    func_8008AC50();
    if (D_800D32A1[arg0 * 8] == 0) {
        type = 1;
    } else {
        type = 2;
    }
    func_8008AB94();
    D_800D367C = (void *)func_8008ABB8(ArchiveDecodeAlignedSize(type), 0);
    ArchiveReadFileToBuffer(type, D_800D367C, 0, 0x80);
    func_8008AC50();
    D_800D2D28[0xAE] = 1;
}

void func_8007FDEC(void) {
    if (D_800D2D28[0x96] != 0) {
        HeapFree(D_800C3DE8);
        D_800D2D28[0x96] = 0;
    }
}

void func_8007FE3C(void) {
    s32 pBuffer;

    if (D_800D2D28[0x96] == 0) {
        func_8008AC50();
        func_8008AB94();
        pBuffer = func_8008ABB8(ArchiveDecodeAlignedSize(3), 0);
        D_800C3DE8 = (u8 *)pBuffer;
        ArchiveReadFileToBuffer(3, (void *)pBuffer, 0, 0x80);
        func_8008AC50();
        D_800D2D28[0x96] = 1;
    }
}

void func_8007FEC4(void) {
    u8 *pBuffer;

    pBuffer = (u8 *)func_8008ABB8(0x5DA4, 0);
    D_800D2DB4 = pBuffer;
    bzero(pBuffer, 0x5DA4);
    *(volatile u16 *)(D_800D2DB4 + 0x5D9C) = 0xA0;
    *(volatile u16 *)(D_800D2DB4 + 0x5D9E) = 0x64;
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FF14);
/* 91.69%: shapes exhausted (12 tries). The instruction multiset is nearly
   identical; what is left is a rotation of the loop's register assignment -
   retail has v=$a1, digits=$a0, i*4=$a2, quotient=$a3, mfhi=$v0, while gcc
   gives v=$a2, digits=$a1, i*4=$a3, quotient=$a0, mfhi=$t3 (because $a3 is
   taken by the strength-reduced i*4 biv). Declaration order, a base local,
   u8 digits and s32 v all change nothing or make it worse (90.97/90.75/89.62).
   Under gcc-2.7.2-cdk it is 93.74 (still not 100), so not a preset question.
   Two instruction-order details remain: retail puts the division-by-10
   dividend's andi after the magic-constant lui/ori and hoists sll v0,v1,2
   into the bnez delay slot. */
/*
void func_8007FF14(u8 arg0) {
    s32 i;
    u8 v;
    s32 n;
    s32 digits;

    func_8009A2D4(arg0);
    for (i = 2; i < 5; i++) {
        v = D_800D2C0C[(i - 2) * 2];
        *(u8 *)(D_800C3EAC + (arg0 << 6) + i * 4 + 8) = 0xFF;
        *(u8 *)(D_800C3EAC + (arg0 << 6) + i * 4 + 9) = 0xFF;
        *(u8 *)(D_800C3EAC + (arg0 << 6) + i * 4 + 0xA) = 0xFF;
        n = v / 100;
        digits = 0;
        if (n != 0) {
            v -= n * 100;
            *(u8 *)(D_800C3EAC + (arg0 << 6) + i * 4 + 8) = n + 0x83;
            digits = 1;
        }
        n = v / 10;
        if (n != 0 || digits != 0) {
            v -= n * 10;
            *(u8 *)(D_800C3EAC + (arg0 << 6) + (i * 4 + digits) + 8) = n + 0x83;
            digits++;
        }
        *(u8 *)(D_800C3EAC + (arg0 << 6) + (i * 4 + digits) + 8) = v + 0x83;
    }
    func_8007FEC4();
    if (D_800D2D24[arg0] != 7) {
        func_80089AF8(arg0);
        func_800716D8();
        func_800898F0(arg0);
        func_800716D8();
        D_800D2D28[0xAD] = 1;
        D_800D2D28[0xC7] = 1;
    }
}
*/


void func_800800E8(u8 arg0) {
    s32 index;

    D_800D2D28[0xAD] = 0;
    D_800D2D28[0xC7] = 0;
    D_800D2D28[0xA8] = 0;
    index = arg0 * 8;
    if (D_800D32A1[index] == 2) {
        D_800D32A1[index] = 1;
    }
    HeapFree(D_800D2DB4);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080160);
s32 func_80080AE4(u8 arg0) {
    s32 best = 0xFF;
    s32 i = D_800D2DD7[0];
    s32 sel;
    s32 idx;
    s16 *pSlots = (s16 *)(D_800D2DD7 + 0x2F);

    do {
        idx = D_800D2DD8[i];
        i++;
        if (*(pSlots + idx) < best && idx != arg0) {
            sel = idx;
            best = *(u8 *)(pSlots + idx);
        }
        if (i == 0xB) {
            i = 0;
        }
    } while (i != D_800D2DD7[0]);
    return sel;
}

void func_80080B64(u8 color) {
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFE;
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = color;
    D_800D366C = 0;
}

void func_80080BD0(void) {
    D_800C3EAC[0x2DB] = 1;
    if (D_800C3EAC[0x2D3] < 3 && D_800C204C == 0) {
        func_8007FCE8();
        func_8007FDEC();
        func_800800E8(D_800C3EAC[0x2D3]);
        BattleReleaseEffectSlotIfChar4(D_800C3EAC[0x2D3]);
    }
}

void func_80080C6C(u8 index) {
    u8 *pEntry = D_800D3278 + index * 0x38;

    pEntry[0x34] = 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080C94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8008115C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081318);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081504);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800816F8);
void func_8008189C(s32 arg0) {
    s32 i;

    if (D_800C3EAC[0x2E9] != 0) {
        return;
    }
    func_800BC404(func_80089C08(D_800C3EAC[0x2E8]));
    func_800BCD98(func_80089C08(D_800C3EAC[0x2E8]));
    i = 0;
    do {
        if (func_80084854(D_800C3EAC[0x2E8], i) != D_800C3EAC[0x2E8]) {
            (D_800C3E24 + i)[0xE6] = 1;
        } else {
            (D_800C3E24 + i)[0xE6] = 0;
        }
        i++;
    } while (i < 4);
    if (D_800D2C34 != 4) {
        func_80093B08(arg0);
    }
}
/* 92.34%: `func_80089C08` returns u16, so a separate `orBits = f(a); orBits |=
   f(b);` masks both results; writing the two calls as one `|` expression inside
   the func_800BC404 call lets gcc fold them into the single `or` + final `andi
   0xffff` the original has. What is left: the original loads D_800C3EAC once and
   shares the base between the `[0x2DA]` read and the `+arg0*0x40` pointer for
   the func_80089C08 argument (we reload it), and the two `addu base,base,idx`
   come out with the index register as destination instead of the base
   (allocator tie, both values dead after). */
/*
void func_800819A4(u8 arg0) {
    s32 orBits;

    if (D_800C3EAC[0x2E9] != 0) {
        return;
    }
    D_800C3EAC[0x2E9] = 1;
    func_800BCD98(0);
    *(u8 *)((u32)D_800C3EAC + arg0 * 0x40 + 0x3C) = D_800C3EAC[0x2E8];
    func_8007FCE8();
    func_8007FDEC();
    func_80077980();
    orBits = func_80089C08(arg0) | func_80089C08(D_800C3EAC[arg0 * 0x40 + 0x3C]);
    func_800BC404(orBits);
    if (D_800D2D24[arg0] != 4) {
        D_800C400B[D_800C3EAC[0x2DA] * 0x48] = arg0;
        D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFD;
        *(u16 *)(D_800C4022 + D_800C3EAC[0x2DA] * 0x48) = 0;
        *(u16 *)(D_800C3FFE + D_800C3EAC[0x2DA] * 0x48) =
            func_80089C08(D_800C3EAC[arg0 * 0x40 + 0x3C]);
        D_800C3EAC[0x2DA]++;
    }
    func_8009413C(arg0, 0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800819A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081B58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800820A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800822C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80082504);
void func_800826CC(u8 arg0) {
    s32 i;

    func_80088490(arg0);
    func_8009AEFC(arg0);
    func_800BAF48(arg0);
    D_800CCE42[arg0 * 0x170] |= 0x80;
    func_800883AC(arg0);
    D_800D32A1[arg0 * 8] = 2;
    if (D_800D2D24[arg0] != 7) {
        *(u8 *)(arg0 * 0x1E4 + (u32)D_800C3EA4 + 0x853D) = 2;
    }
    D_800C3EB8[arg0 * 0x1C] = 1;
    D_800C3EAC[arg0 + 0x2EB] = 1;
    for (i = 0; i < 3; i++) {
        if (g_GameState.partyMembers[i] == D_800D2D24[arg0] && D_80059179 == 0) {
            g_GameState.gearRide[i] = 1;
        }
    }
}
