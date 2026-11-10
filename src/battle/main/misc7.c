#include "common.h"

// Operator argument tables: D_800D3420 is the 16-bit field table, D_800D3430
// the 8-bit one, D_800D3410 the 32-bit one; all are indexed by slot * 0x40.
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D342E[];
extern u8 D_800D3430[];
extern u8 D_800D343F[];
extern u16 D_8005A3A0[];
extern u8 D_800D2E5C;
extern u8 D_800CCE34[];
extern u8 D_800CCE38[];
extern u8 D_800CCE39[];
extern u8 D_800CCE3A[];
extern u8 D_800CCE3B[];
extern u8 D_800CCE3C[];
extern u8 D_800CCE3D[];
extern u8 D_800CCE3E[];

u8 func_8001BD40(int, u8);
u16 func_80089B50(int, u16);
s32 func_80079E7C(u16);
s32 func_80079ED8(u8, u8, u8, s32);
s32 func_8007A280(u8, u8, s32, s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B424);
/* best: 90.81%. The pTable = (u8 *)pField + 0x10 form is what puts the
   addiu 0x10 in the jal delay slot like the original. Remaining gap is the
   allocator: the original keeps slot in s0 and base+0x10 in s1, gcc folds
   slot+base into s0 before the call and needs one saved register fewer.

void func_8007B424(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pTable = (u8 *)pField + 0x10;
    u8 value = func_80079E7C(pField[(*p)[3]]);

    pTable[(*p)[1]] = func_80079ED8(value, (*p)[2], 0, 1);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B4B8);
void func_8007B578(u8 **p, u8 index) {
    u16 *pTable = (u16 *)(D_800D3420 + index * 0x40);
    u8 value = func_80079E7C(pTable[(*p)[3]]);

    pTable[(*p)[1]] = func_8007A280(value, (*p)[2], 0, 1);
}
u8 func_8007B608(u8 **p, u8 index, u8 arg2) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 ret = arg2;
    u8 value = func_80079E7C(pField[(*p)[3]]);

    if (value < 3) {
        D_800D2E5C = 0x20;
        ret = arg2 + 1;
    } else {
        func_8007A280(value, (*p)[2], pField[(*p)[1]], 0);
    }
    return ret;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B6C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B7B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B8D4);
/* best: 73.75%. Instruction multiset identical; the original loads *p into v0
   and puts the D_800D3420 base in a0, gcc emits the base first instead. Same
   allocator tie as the rest of the D_800D34x0 operators in misc6.

void func_8007B8D4(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] = D_8005A3A0[pArgs[2]];
}
*/
void func_8007B914(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    D_8005A3A0[pArgs[2]] = pField[pArgs[1]];
}
void func_8007B958(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;

    pTable[pArgs[2]] = pTable[pArgs[1]];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B98C);
/* best: 84.67%. Instruction multiset identical; the original loads *p into v0
   before materialising the D_800D3420 base in a0, gcc always emits the base
   first and reuses the dead a0 for the load. Declaration order, an explicit
   offset local and a volatile load all leave it unchanged.

void func_8007B98C(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[2]] = pField[pArgs[1]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B9C8);
/* best: 84.67%. Instruction multiset identical; the original loads *p into v0
   and puts the D_800D3410 base in a0, gcc emits the base first instead. Same
   allocator tie as func_8007B98C.

void func_8007B9C8(u8 **p, u8 index) {
    u32 *pField = (u32 *)(D_800D3410 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[2]] = pField[pArgs[1]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BA04);
/* best: 95.94%. Only the register pair chosen for the source byte and the
   destination halfword differs (v1/a0 vs v0/v1); the D_800D3430 base is already
   derived as D_800D3420+0x10 like the original.

void func_8007BA04(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pTable = D_800D3420 + 0x10 + index * 0x40;
    u8 *pArgs = *p;

    pField[pArgs[2]] = pTable[pArgs[1]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BA44);
/* best: 95.29%. Only the register holding the stored halfword differs
   (original a0, gcc v1); everything else, including the D_800D3420 base
   derived as D_800D3410+0x10, is identical.

void func_8007BA44(u8 **p, u8 index) {
    u32 *pField32 = (u32 *)(D_800D3410 + index * 0x40);
    u16 *pField = (u16 *)(D_800D3410 + 0x10 + index * 0x40);
    u8 *pArgs = *p;

    pField32[pArgs[2]] = pField[pArgs[1]];
}
*/
void func_8007BA88(u8 index) {
    s32 i = 0xF;
    u8 *p = D_800D343F + index * 0x40;

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}
void func_8007BAB8(u8 index) {
    s32 i = 0x7;
    u16 *p = (u16 *)(D_800D342E + index * 0x40);

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BAE8);
/* best: 96.76%. Only the register holding *p differs (original v1, gcc
   reuses a0). The two-statement slot local is needed: written as
   (index + 3) * 0x170 inside the store, gcc folds 3*0x170 into the
   displacement instead of the addiu the original has. Hoisting the value
   into a u32 local (which works for func_8007BB2C) drops it to 41.18%.

void func_8007BAE8(u8 **p, u8 index) {
    s32 slot = index + 3;

    *(u32 *)(D_800CCE34 + slot * 0x170) = (*p)[1] | ((*p)[2] << 8);
}
*/
void func_8007BB2C(u8 **p, u8 index) {
    u32 value = (*p)[1] | ((*p)[2] << 8);
    s32 slot = index + 3;

    *(u16 *)(D_800CCE3E + slot * 0x170) = value;
}
void func_8007BB70(u8 **p, u8 index) {
    s32 slot = index + 3;

    *(u8 *)(D_800CCE3D + slot * 0x170) = (*p)[1];
    *(u8 *)(D_800CCE3B + slot * 0x170) = (*p)[2];
    *(u8 *)(D_800CCE39 + slot * 0x170) = (*p)[3];
}
void func_8007BBD8(u8 **p, u8 index) {
    s32 slot = index + 3;

    *(u8 *)(D_800CCE3C + slot * 0x170) = (*p)[1];
    *(u8 *)(D_800CCE3A + slot * 0x170) = (*p)[2];
    *(u8 *)(D_800CCE38 + slot * 0x170) = (*p)[3];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BC40);
/* best: 98.82%. Only the last addu is commuted (original base first into a1,
   gcc reuses the dead a2): identical multiset. Writing *p inline twice is
   required, the second load is CSE'd if it goes through a local.

void func_8007BC40(u8 **p, u8 *pDest, u8 count) {
    pDest[count * 8 + (*p)[1]] = (*p)[2];
    pDest[count * 8 + ((*p)[1] + 1)] = (*p)[3];
}
*/
void func_8007BC84(u8 **p, u8 index) {
    u8 value = func_8001BD40(0, (*p)[2]);
    u8 *pTable = D_800D3430 + index * 0x40;

    pTable[(*p)[1]] = value;
}
void func_8007BCE8(u8 **p, u8 index) {
    u16 value = func_80089B50(0, (*p)[2] | ((*p)[3] << 8));
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);

    pField[(*p)[1]] = value;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BD5C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BEA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007C040);
