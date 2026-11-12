#include "common.h"
#include "system/memory.h"

typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ s32 unk1C;
} BattleMisc33State; // Size: 0x50

extern BattleMisc33State *D_800C3610;
extern s32 D_800C3618;
extern s32 D_800C3E20;
extern u16 D_800D2E54;
extern u8 D_800D3350;
extern u8 D_800C3784[];
extern u8 D_800C3EB0[];
extern u32 D_800C37A4[];
extern u8 D_800591AC;
extern s32 D_80059464;

typedef struct {
    /* 0x00 */ u8 unk0[0x2];
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u8 unk4[0x6];
    /* 0x0A */ s16 unkA;
    /* 0x0C */ u8 unkC[0x94];
    /* 0xA0 */ u16 unkA0;
    /* 0xA2 */ u8 unkA2[0x2];
    /* 0xA4 */ u16 unkA4;
} Misc33Point;

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
} Misc33Pair; // Size: 0x4

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} Misc33LineEnds; // Size: 0x8

extern s16 func_80023124(Misc33Pair, Misc33Pair);
extern s32 func_800C0FAC(s32);
extern u8 func_80021AD8(u8, s16);

typedef struct {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ void (*unkC)(void *);
    /* 0x10 */ u8 unk10[0x2C];
    /* 0x3C */ u16 unk3C;
    /* 0x3E */ u8 unk3E[0x2];
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ u8 unk4C[0x14];
    /* 0x60 */ s32 unk60;
    /* 0x64 */ u16 unk64;
    /* 0x66 */ u8 unk66[0x2];
    /* 0x68 */ u8 unk68;
    /* 0x69 */ u8 unk69;
    /* 0x6A */ u8 unk6A;
} BattleMisc33Actor;
extern void func_800C1140(s32);

void func_800BE11C(BattleMisc33Actor *p) {
    p->unk3C = p->unk3C + p->unk64;
    p->unk40 += 0x330;
    p->unk44 += 0x330;
    p->unk48 += 0x330;
    p->unk68 = func_80021AD8(p->unk68, -4);
    p->unk69 = func_80021AD8(p->unk69, -4);
    p->unk6A = func_80021AD8(p->unk6A, -4);
    if (--p->unk60 == 0) {
        p->unkC(p);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE1C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE330);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE538);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE6A0);
/* best 69.67%: the table address must be materialised once into t2 before the
   loop, which only happens for D_800C3784 itself, not for a pointer local.
void func_800BE6A0(s32 arg0, u8 *p, s32 n, s32 arg3) {
    s32 i;
    s32 last;

    last = n - 1;
    for (i = 0; i != n; i++) {
        (i + p)[1] = D_800C3784[(arg0 >> ((last - i) * 4)) & 0xF] + arg3;
    }
    p[0] = n;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE6E8);
/* best 82.71%: retail walks ONE pointer that is decremented in the loop-test
   block's branch delay slot, so both `lw` reads are at offset 0(v1) and the
   pointer is restored with a dead `addiu v1,v1,4` after the loop; gcc keeps the
   source pointer as the base here and reads the quotient at -4(v1), and it also
   emits a `beqz` loop guard instead of retail's `j` into the test block.  The
   divisors really are tab[n-1] (quotient) and tab[n] (remainder) -- the two
   divu are NOT CSE'd, so a single `divisor` local (which merges them into one
   div with mflo+mfhi) is wrong.  `(u32)` on both divisions is required: retail
   uses divu, and the table D_800C37A4 holds u32.
s32 func_800BE6E8(s32 value, u8 *pBuf, s32 n, u8 started, s32 base) {
    u8 *pDst;
    u8 digit;
    s32 count;
    u32 *pTab;

    count = 0;
    pDst = pBuf + 1;
    if (value < 0) {
        count = 1;
        pBuf[1] = '-';
        pDst = pBuf + 2;
        value = -value;
        n--;
    }
    pTab = &D_800C37A4[n];
    while (n != 0) {
        digit = (u32)value / pTab[-1];
        if (digit != 0) {
            started = 1;
        }
        n--;
        if (started != 0) {
            *pDst = digit + base;
            pDst++;
            count++;
        }
        value = (u32)value % *pTab;
        pTab--;
    }
    *pDst = value + base;
    pBuf[0] = count + 1;
    return count + 1;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE790);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEB04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEBC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEC18);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BED30);
/* best 73.57%: retail expands the bare zero stores with lui $v0 as the scratch,
   maspsx leaves them to GNU as, which always uses $at. Unfixable in C.
void func_800BED30(void) {
    D_800C3E20 = 0;
    D_800C3610 = NULL;
    D_800D2E54 = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BED4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEDE8);
/* untested: same bare global stores as func_800BED30, so the $at expansion
   probably blocks it too.
void func_800BEDE8(void) {
    HeapFree(D_800C3610);
    D_80059464 = 0;
    D_800C3610 = NULL;
    D_800591AC = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEE2C);
s32 func_800BEEB4(u16 mask, void **pOut, s32 arg2) {
    u8 *pBase;
    u8 *pEntity;
    s32 i;
    s32 count;

    pBase = D_800C3EB0;
    i = 0;
    count = 0;
    do {
        if ((mask & 1) != 0) {
            pEntity = *(u32 **)(pBase + i * 4 + 0x8C8C);
            if (pEntity != NULL) {
                *(s32 *)(pEntity + 0x74) = arg2;
                pOut[count] = pEntity;
                count++;
            }
        }
        i++;
        mask >>= 1;
    } while (i != 0xB);
    pOut[count] = NULL;
    return count;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEF24);
/* best 72.54%: passing two 4-byte by-value structs reproduces retail's
   store-two-halves / reload-with-lhu-and-or argument shape and gives the right
   slots (sp+0x10, sp+0x18), but gcc over-allocates the frame (0x40 vs retail
   0x30, 16 dead bytes) and loads the 0x2/0xA fields with lhu where retail uses
   lh.  Declaring the locals as 4- and 8-byte structs changes nothing.
s16 func_800BEF24(Misc33Point *p0, Misc33Point *p1) {
    Misc33Pair e0;
    Misc33Pair e1;

    e0.unk0 = p0->unk2;
    e0.unk2 = p0->unkA;
    e1.unk0 = p1->unk2;
    e1.unk2 = p1->unkA;
    return func_80023124(e1, e0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEF8C);
/* best 68.54%: same shape as func_800BEF24 (two 4-byte structs by value, slots
   sp+0x10 / sp+0x18 correct); frame 0x40 vs retail 0x30.
s16 func_800BEF8C(Misc33Point *p) {
    Misc33LineEnds e0;
    Misc33LineEnds e1;

    e0.unk0 = p->unk2;
    e0.unk2 = p->unkA;
    e1.unk0 = p->unkA0;
    e1.unk2 = p->unkA4;
    return func_80023124(*(Misc33Pair *)&e1, *(Misc33Pair *)&e0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEFF4);

void func_800BF0B4(s32 arg0) {
    D_800C3610->unk1C = arg0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF0C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF1EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF2B8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF354);
/* untested: retail keeps the flag address in s0 across the call; gcc 2.7.2
   rematerialises it from %hi instead, which costs the saved register.
s32 func_800BF354(void) {
    s32 ret;

    if (D_800D3350 == 0) {
        ret = func_800C0FAC(D_800C3618);
        D_800D3350 = 1;
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF3A4);
/* best 71.35%: same rematerialisation as func_800BF354.
void func_800BF3A4(void) {
    if (D_800D3350 != 0) {
        func_800C1140(D_800C3618);
        D_800D3350 = 0;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF3E8);
