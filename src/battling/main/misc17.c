#include "common.h"
#include "psyq/libgpu.h"

extern s32 D_80059488;
extern s32 D_8009284C;
extern u8 D_800928A0;
extern u8 D_80096DE0[];
extern u8 D_80096E00[];

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
extern s32 func_8008B650(s32, s16, s16);
extern void func_80039FF8(void);
extern u32 func_8003A5D0(u32);
extern void func_8003A20C(u8);
extern void func_8008FE80(u8*, u8*, s32);
extern void func_80090E10(u8*);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E120);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E2B8);
/* best: objdiff 89.48% - first half exact; retail expands the `flag ? 2 : 1` argument as an if/else that repeats
   `addu a0,zero,zero` in both arms and joins with a jump, and re-forms &D_80096E00 with a fresh lui/addiu for the
   second AddPrim instead of keeping it in a callee-saved register.
void func_8008E2B8(u8* p0, s32 color, s32 flag) {
    u8* pPkt;
    u_short tpage;

    pPkt = D_80096DE0 + D_800928A0 * 0x10;
    *(u32*)(pPkt + 0x4) = (color | (color << 8) | (color << 16)) | 0x60000000;
    *(u8*)(pPkt + 0x7) |= 2;
    *(u8*)(pPkt + 0x3) = 3;
    *(u32*)(pPkt + 0x8) = 0;
    *(u32*)(pPkt + 0xC) = 0xDA0140;
    AddPrim(p0, pPkt);
    tpage = GetTPage(0, flag ? 2 : 1, 0, 0);
    SetDrawMode((DR_MODE*)(D_80096E00 + D_800928A0 * 0xC), 0, 1, tpage, 0);
    AddPrim(p0, D_80096E00 + D_800928A0 * 0xC);
}
*/
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
/* best: objdiff 73.33% - identical instruction multiset; retail loads the arg-4 global after the hoisted lui 0x60000 and defers the addu/move a1 to after it, gcc 2.7.2 and 2.6.0 both emit lui, addu, move a1, then the arg-4 load. Also tried: locals for the constant, pre-loading D_80059488 into an s32, all give 73.33%.
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
void func_8008EE1C(u8* p, s16 arg1, s16 arg2) {
    s32 pad[4];
    s32 v98;
    s32 v9C;
    s32 flag;
    s32 i;

    v98 = *(s32*)(p + 0x98);
    v9C = *(s32*)(p + 0x9C);
    if (*(u32*)(p + 0xD0) & 0x100) {
        v9C = v9C * 2 / 3;
        flag = 0;
    } else {
        flag = 1;
    }
    for (i = 0; i < 2; i++) {
        if (arg2 != 0 && flag != 0) {
            *(s32*)(p + 0x48) += v98;
            if (*(s32*)(p + 0x48) > arg2) {
                *(s32*)(p + 0x48) = arg2;
            }
        } else {
            *(s32*)(p + 0x48) -= v9C;
            if (*(s32*)(p + 0x48) < 0) {
                *(s32*)(p + 0x48) = 0;
            }
        }
    }
    *(s32*)(p + 0x58) = func_8008B650(*(s32*)(p + 0x58), arg1, 0x40);
    *(u16*)(p + 0xCE) = 0;
}
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
void func_8008F094(u8* p0, u8* p1) {
    u16 timer;

    if (D_8009284C >= 0x801) {
        *(u16*)(p1 + 0x4) = 1;
        *(u16*)(p1 + 0xC) = 0;
        return;
    }
    *(u16*)(p1 + 0x4) -= 1;
    if ((s16)*(u16*)(p1 + 0x4) != -1) {
        return;
    }
    *(u16*)(p1 + 0xA) = rand() % 0x600 + 0x500;
    timer = rand() % 50 + 0xA;
    *(u16*)(p1 + 0xC) = 0xFF;
    *(u16*)(p1 + 0x4) = timer;
}
void func_8008F17C(u8* p0, u8* p1) {
    u16 timer;

    if (D_8009284C < 0x100) {
        *(u16*)(p1 + 0x4) = 1;
        *(u16*)(p1 + 0xC) = 0;
        return;
    }
    *(u16*)(p1 + 0x4) -= 1;
    if ((s16)*(u16*)(p1 + 0x4) != -1) {
        return;
    }
    *(u16*)(p1 + 0xA) = rand() % 0x600 - 0x300;
    timer = rand() % 120 + 0xA;
    *(u16*)(p1 + 0xC) = 0xFF;
    *(u16*)(p1 + 0x4) = timer;
}
void func_8008F260(u8* p0, u8* p1, u8 value) {
    p1[0xF] = value;
    func_80090E10(p0);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008F280);
s32 func_8008F4F4(u8* p, s32 arg1) {
    return (*(s16*)(p + 0xBC) * arg1) / 255 < *(s16*)(p + 0xB4);
}
