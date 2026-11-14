#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u8 unk20[0x24];
    /* 0x44 */ s32 unk44;
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

extern s16 func_80023124(DVECTOR, DVECTOR);
extern s32 func_800C0FAC(s32);
extern u8 SpriteColorAddClamp(u8, s16);
extern s32 func_800C07CC(DVECTOR vec0, DVECTOR vec1);
extern void SpriteSetDirection(Misc33Point *p, s16 arg1);
extern void func_800223B0(Misc33Point *p, s16 arg1);
extern void func_800245D8(Misc33Point *p, s32 arg1);

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
    p->unk68 = SpriteColorAddClamp(p->unk68, -4);
    p->unk69 = SpriteColorAddClamp(p->unk69, -4);
    p->unk6A = SpriteColorAddClamp(p->unk6A, -4);
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
typedef struct {
    /* 0x00 */ u16 cur;
    /* 0x02 */ u16 pressed;
    /* 0x04 */ u16 released;
    /* 0x06 */ u16 unk6;
} BattlePadFrame; // Size: 0x8

typedef struct {
    /* 0x0000 */ u8 unk0[0xC58];
    /* 0xC58 */ u16 cur[2];
    /* 0xC5C */ u16 pressed[2];
    /* 0xC60 */ u16 released[2];
    /* 0xC64 */ BattlePadFrame history[4];
} BattlePadState; // Size: 0xC84

extern int ControllerGetButtonState(int controllerIndex);
extern u16 D_800D30E4;

void func_800BEC18(void) {
    u8 *pStateBase;
    BattlePadState *pState;
    s32 cur;
    u16 prev;
    u16 prev2;

    /* the mask is what makes gcc narrow the call result */
    cur = ControllerGetButtonState(0) & 0xFFFF;
    /* a symbol plus literal folds into one lui/addiu; retail adds 0x8000 in a register */
    pStateBase = D_800C3EB0;
    pState = (BattlePadState *)(pStateBase + 0x8000);
    prev = pState->cur[0];
    pState->cur[0] = cur;
    pState->pressed[0] = ~prev & cur;
    pState->released[0] = prev & ~cur;

    cur = ControllerGetButtonState(1);
    prev2 = pState->cur[1];
    pState->pressed[1] = ~prev2 & cur;
    pState->cur[1] = cur;
    pState->released[1] = pState->cur[0] & ~cur;

    if (pState->cur[0] == pState->history[0].cur) {
        return;
    }
    pState->history[3] = pState->history[2];
    pState->history[2] = pState->history[1];
    pState->history[1] = pState->history[0];
    pState->history[0].cur = pState->cur[0];
    pState->history[0].pressed = pState->pressed[0];
    pState->history[0].released = pState->released[0];
    pState->history[0].unk6 = D_800D30E4;
}

void func_800BED30(void) {
    D_800C3E20 = 0;
    D_800C3610 = NULL;
    D_800D2E54 = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BED4C);
void func_800BEDE8(void) {
    HeapFree(D_800C3610);
    D_80059464 = 0;
    D_800C3610 = NULL;
    D_800591AC = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEE2C);
/* Not convertible: retail switches the stack pointer into the 0x1000-byte
   HeapAlloc block for the func_800AA320 call (sw $sp,0($t0); addiu $t0,-4;
   addu $sp,$t0, then addiu $sp,4; lw $sp,0($sp)), which the original source
   wrote as inline asm -- the same shape as GfxLoadImageAccelerated in
   system/graphics.  No C produces it. */
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
s16 func_800BEF24(Misc33Point *p0, Misc33Point *p1) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p0->unk2;
    vec0.vx = c0;
    c1 = p0->unkA;
    vec0.vy = c1;
    c2 = p1->unk2;
    vec1.vx = c2;
    c3 = p1->unkA;
    vec1.vy = c3;
    return func_80023124(vec1, vec0);
}
s16 func_800BEF8C(Misc33Point *p) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p->unk2;
    vec0.vx = c0;
    c1 = p->unkA;
    vec0.vy = c1;
    c2 = p->unkA0;
    vec1.vx = c2;
    c3 = p->unkA4;
    vec1.vy = c3;
    return func_80023124(vec1, vec0);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEFF4);

void func_800BF0B4(s32 arg0) {
    D_800C3610->unk1C = arg0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF0C4);
void func_800BF1EC(Misc33Point *p, s32 arg1) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p->unk2;
    vec0.vx = c0;
    c1 = p->unkA;
    vec0.vy = c1;
    c2 = p->unkA0;
    vec1.vx = c2;
    c3 = p->unkA4;
    vec1.vy = c3;
    D_800C3610->unk44 = func_800C07CC(vec0, vec1);
    SpriteSetDirection(p, func_800BEF8C(p));
    func_800223B0(p, func_800BEF8C(p));
    func_800245D8(p, arg1);
    func_800BF0B4(6);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF2B8);
s32 func_800BF354(void) {
    s32 ret;

    if (D_800D3350 == 0) {
        ret = func_800C0FAC(D_800C3618);
        D_800D3350 = 1;
    }
    return ret;
}
void func_800BF3A4(void) {
    if (D_800D3350 != 0) {
        func_800C1140(D_800C3618);
        D_800D3350 = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF3E8);
