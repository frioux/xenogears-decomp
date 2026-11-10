#include "common.h"

extern s32 D_80059488;

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 unk10[4];
    /* 0x20 */ u32* unk20;
} BattleSlot; // Size: 0x24

extern BattleSlot D_80096EA0[];

extern u16 D_80096EA4;
extern u16 D_80096EC8;
extern u16 D_80096EEC;
extern u16 D_80096F10;

extern u8 D_80091EE0[];
extern u8 D_80091F60[];
extern u8 D_80091F70[];
extern u8 D_80091F80[];
extern u8 D_80091F90[];
extern u8 D_80091FA0[];

extern void func_8007639C(u8*, u8);
extern void func_800767C8(u8*, u8*);
extern void func_8008E78C(u8*, u32, u8*, u32);
extern void func_80039FF8(void);
extern u32 func_8003A5D0(u32);
extern void func_8003A20C(u8);
extern void func_8008FE80(u8*, u8*, s32);
extern void func_80090E10(u8*);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E120);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E2B8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E3CC);
/* best: objdiff 99.39% - only difference: gcc folds the address of the loop body's last store into the induction pointer (base = &unk2, offsets 10/-1/2/0) instead of using &unk0 with 12/1/4/2. Statement order does not change the choice; the base always follows the last store. An indexed form (D_80096EA0[i].unkC) loses the strength reduction entirely (45%).
void func_8008E620(void) {
    BattleSlot* pSlot;
    s32 timer;
    u32 i;

    timer = 0x300;
    func_80039FF8();
    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        pSlot->unkC = timer;
        timer <<= 2;
        pSlot->unk1 = 0;
        pSlot->unk4 = 0;
        pSlot->unk2 = i * 2;
        pSlot++;
    }
}*/
void func_8008E620(void) {
    BattleSlot* pSlot;
    s32 timer;
    u32 i;

    timer = 0x300;
    func_80039FF8();
    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        pSlot[i].unkC = timer;
        timer <<= 2;
        pSlot[i].unk1 = 0;
        pSlot[i].unk4 = 0;
        pSlot[i].unk2 = i * 2;
    }
}
void func_8008E67C(void) {
    if (D_80096EA0[0].unk4 != 0xFFFF) {
        D_80096EA0[0].unk4++;
    }
    if (D_80096EC8 != 0xFFFF) {
        D_80096EC8++;
    }
    if (D_80096EEC != 0xFFFF) {
        D_80096EEC++;
    }
    if (D_80096F10 != 0xFFFF) {
        D_80096F10++;
    }
}
u8* func_8008E6F8(u8* p) {
    u8* pTable;

    switch (*(u8*)(p + 0x909)) {
    case 9:
        pTable = D_80091FA0;
        break;
    case 0x1D:
        pTable = D_80091F60;
        break;
    case 0x1B:
        pTable = D_80091F80;
        break;
    case 0x24:
        pTable = D_80091F70;
        break;
    default:
        pTable = D_80091F90;
        break;
    }
    *(u8**)(p + 0x1664) = pTable;
    return pTable;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E78C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E8B0);
void func_8008EADC(void) {
    BattleSlot* pSlot;
    s32 i;

    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        if ((func_8003A5D0(pSlot[i].unk8) & pSlot[i].unkC) == 0) {
            pSlot[i].unk1 = 0;
        }
    }
    func_8008E67C();
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008EB4C);
/* best: objdiff 73.33% - identical instruction multiset; gcc emits lui 0x60000, then addu, then the arg-4 load, retail emits the arg-4 load between the lui and the addu (and keep a1's zero before the jal, not in its delay slot).
void func_8008EB4C(u8* p) {
    if (p != NULL) {
        func_8008E78C(p + 0x60000, 0, NULL, D_80059488);
    }
}*/
void func_8008EB88(u8* p, s32 idx, u8* pData, s32 flags) {
    if (idx != 0) {
        func_8008E78C((u8*)(idx + 0x60000), flags, pData,
                      (idx & 0x7F) | ((*(u32*)(p + 0xD0) >> 20) & 0x80));
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008EBD0);
void func_8008ECEC(u8 kind) {
    BattleSlot* pSlot;
    s32 i;

    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        if (pSlot[i].unk1 != 0 && pSlot[i].unk3 == kind) {
            func_8003A20C(pSlot[i].unk2);
            pSlot[i].unk1 = 0;
        }
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008ED6C);
/* best: objdiff 90.11% - body, operand order and frame all match except register numbering (original keeps &D_80091EE0 in s1 and the byte index in s2; gcc gives the index s1) and an 8-byte larger frame in the original. %gp/%hi spelling of D_80091EE0 is already right (first read folds hi+index through at, second goes through a base pointer).
void func_8008ED6C(u8* p, s32 idx) {
    u8* pTable;
    s32 i;
    u8 v;

    v = *(u8*)(*(u32*)(p + 0x1664) + idx);
    if (v == 0xFF) {
        return;
    }
    i = v << 1;
    pTable = D_80091EE0;
    if (D_80091EE0[i] != 0) {
        func_8008ECEC(((*(u32*)(p + 0xD0) >> 20) & 0x80) | (D_80091EE0[i] & 0x7F));
    }
    if (pTable[i + 1] != 0) {
        func_8008ECEC(((*(u32*)(p + 0xD0) >> 20) & 0x80) | (pTable[i + 1] & 0x7F));
    }
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008EE1C);
void func_8008EF00(u8* p0, u8* p1) {
    func_800767C8(p0, p1);
    *(u16*)(p1 + 4) = 0x3C;
}
void func_8008EF30(u8* p0, u8* p1, s32 flag) {
    func_8008FE80(p0, p1, flag);
    *(u16*)(p1 + 4) = flag ? 0x1E : 0xA;
}
void func_8008EF74(u8* p0, u8* p1) {
    func_8007639C(p0, 3);
    *(u16*)(p1 + 4) = 0x3C;
}
void func_8008EFA8(u8* p0, u8* p1) {
    u32 flags;

    flags = *(u32*)(p1 + 0x2C);
    *(u32*)(p1 + 0x2C) = (flags & ~0x2000) | ((((flags >> 13) & 1) ^ 1) << 13);
    if (*(u32*)(p1 + 0x2C) & 0x2000) {
        *(u32*)(p0 + 0xD0) |= 2;
        *(u16*)(p1 + 4) = 0x3C;
    } else {
        *(u32*)(p0 + 0xD0) &= ~2;
        *(u32*)(p0 + 0xD0) &= ~0x38;
        *(u16*)(p1 + 4) = 0x1E;
    }
}
void func_8008F014(u8* p0, u8* p1) {
    func_8007639C(p0, 4);
    func_8007639C(p0, 3);
    *(u16*)(p1 + 4) = 0x3C;
}
void func_8008F060(u8* p0, u8* p1) {
    func_8007639C(p0, 4);
    *(u16*)(p1 + 4) = 0x3C;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008F094);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008F17C);
void func_8008F260(u8* p0, u8* p1, u8 value) {
    p1[0xF] = value;
    func_80090E10(p0);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008F280);
s32 func_8008F4F4(u8* p, s32 arg1) {
    return (*(s16*)(p + 0xBC) * arg1) / 255 < *(s16*)(p + 0xB4);
}
