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
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} Misc33LineEnds; // Size: 0x8

extern s32 func_80023124(Misc33LineEnds);
extern s32 func_800C0FAC(s32);
extern void func_800C1140(s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE11C);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEEB4);
/* best 97.32%: instruction multiset identical. Retail has lui $v0 for the
   D_800C3EB0 address (GNU as expand of la reuses the destination, $a3) and
   move $t1,$t0 for the two zeroed counters; both are allocator/assembler
   ties. The 0x8C8C offset must stay a separate add, not folded into %lo.
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
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEF24);
/* best 71.38%: the two 4-byte groups land at sp+0x10 and sp+0x18 in retail,
   so the original built two 4-byte temporaries, not one 8-byte struct.
s16 func_800BEF24(Misc33Point *p0, Misc33Point *p1) {
    Misc33LineEnds line;

    line.unk0 = p1->unk2;
    line.unk2 = p1->unkA;
    line.unk4 = p0->unk2;
    line.unk6 = p0->unkA;
    return func_80023124(line);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEF8C);
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
