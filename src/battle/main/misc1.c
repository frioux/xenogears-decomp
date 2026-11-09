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
extern void func_80079E18(s32);
extern void func_80079E4C(s32);
extern s32 SystemRenderStringEntry(u8 *, s32, s32, s32);

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
/* 78.82% - original emits the return-value zeroing before the ra restore (nop in
   the last jal's delay slot); gcc steals lw ra into that slot from every shape
   tried: return 0, s32 ret = 0 at top / after the call, ret = 0; return ret.
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071964);
/* 98.78% - only the register for the leading lbu differs (original a1, gcc v0,
   forcing the two lhu temps to swap v0/v1). Tried: two early-return ifs, one
   compound && if, and nested ifs (49%).
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072270);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072324);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800723E0);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072A9C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072DA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072F38);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073380);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073538);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073A58);
