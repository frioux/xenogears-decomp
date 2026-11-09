#include "common.h"

// Operator argument tables: D_800D3420 is the 16-bit field table, D_800D3430
// the 8-bit one, D_800D3410 the 32-bit one; all are indexed by slot * 0x40.
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D342E[];
extern u8 D_800D3430[];
extern u8 D_800D343F[];
extern u16 D_8005A3A0[];
extern u8 D_800CCE34[];
extern u8 D_800CCE3E[];

u8 func_8001BD40(int, u8);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B424);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B4B8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B578);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B608);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B6C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B7B0);
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
/* best: 84.67%. Instruction multiset identical; the original loads *p into v0
   and puts the D_800D3420 base in a0, gcc emits the base first instead. Same
   allocator tie as func_8007B8D4.

void func_8007B98C(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[2]] = pField[pArgs[1]];
}
*/
/* best: 84.67%. Instruction multiset identical; the original loads *p into v0
   and puts the D_800D3410 base in a0, gcc emits the base first instead. Same
   allocator tie as func_8007B98C.

void func_8007B9C8(u8 **p, u8 index) {
    u32 *pField = (u32 *)(D_800D3410 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[2]] = pField[pArgs[1]];
}
*/
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
void func_8007BA44(u8 **p, u8 index) {
    u32 *pField32 = (u32 *)(D_800D3410 + index * 0x40);
    u16 *pField = (u16 *)(D_800D3410 + 0x10 + index * 0x40);
    u8 *pArgs = *p;

    pField32[pArgs[2]] = pField[pArgs[1]];
}
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
void func_8007BAE8(u8 **p, u8 index) {
    u8 *pArgs = *p;

    *(u32 *)(D_800CCE34 + (index + 3) * 0x170) = pArgs[1] | (pArgs[2] << 8);
}
void func_8007BB2C(u8 **p, u8 index) {
    u8 *pArgs = *p;

    *(u16 *)(D_800CCE3E + (index + 3) * 0x170) = pArgs[1] | (pArgs[2] << 8);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BB70);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BBD8);
void func_8007BC40(u8 **p, u8 *pDest, u8 count) {
    pDest[count * 8 + (*p)[1]] = (*p)[2];
    pDest[count * 8 + (*p)[1] + 1] = (*p)[3];
}
void func_8007BC84(u8 **p, u8 index) {
    u8 value = func_8001BD40(0, (*p)[2]);
    u8 *pTable = D_800D3430 + index * 0x40;

    pTable[(*p)[1]] = value;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BCE8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BD5C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BEA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007C040);
