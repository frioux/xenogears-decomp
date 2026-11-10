#include "common.h"

#include "psyq/libapi.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "system/menu.h"

extern u16 D_801E96A8[]; // Bit masks, LSB first
extern u16 D_801E96C8[]; // Bit masks, MSB first
extern u32 D_801E96E8[];
extern s32 D_801E9768[]; // Card result to error code
extern u8 D_801E9779;
extern u8 D_80059460; // Index of the menu overlay the menu was entered with

s32 func_801C881C(void);
extern void func_80039DB8(s32 arg0);
extern int Vsync(int mode);

void func_801C7BF4(void);
void func_801C8A10(s32 arg0);
extern s32 ArchiveGetDiscNumber(void);
void func_801D1E80(void);
void func_801D22F4(s32 arg0);
void func_801D2484(void);
s32 func_801D2F4C(u8 arg0);
s32 func_801D32B4(void);
void func_801E92CC(void);
s32 func_801E93A0(s32 arg0);
void func_801C6400(void);
void func_801C6AA0(void);
void func_801C6D4C(void);
void func_801C6D5C(void);
void func_801C6E0C(void);
void func_801C6E68(void);
void func_801C6F70(void);

void func_801C7B0C(void) {
    g_Menu->pSelectionMenu->unk1180.x = 0x2C0;
    g_Menu->pSelectionMenu->unk1180.y = 0x100;
    g_Menu->pSelectionMenu->unk1180.w = 0x140;
    g_Menu->pSelectionMenu->unk1180.h = 0xE0;
    g_Menu->unk348->unk15B = 0x40;
    func_801C6AA0();
    func_801C6D4C();
    func_801C6E0C();
    func_801C6F70();
    func_801C6E68();
    switch (D_80059460) {
    case 2:
        g_Menu->unk348->unk15B = 0x4C;
        /* fallthrough */
    case 0:
    case 6:
        func_801C6400();
        func_801C6D5C();
        break;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7BF4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7D78);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7F34);
/* Best: 83.02% (388 bytes, same size). The divisors below are decoded from the
   multiply-back shifts in the assembly and are certainly right (21600000,
   2160000, 216000, 36000, 3600, 600, 120 in a running-remainder chain, stored to
   g_Menu->unk2EC[0..6] as 32-bit words). What is left is allocation plus the
   scheduler: retail keeps all seven quotients in t0/a3/t2/t1/a2/a1/v0 and
   batches the six stores after the last `multu`, and each `mfhi` lands in the
   same register its `srl` reads, where gcc gives mfhi its own temp
   (`mfhi t3; srl t2,t3,0x16`) and stores as it goes. Tried: plain
   store-after-each-division form with a running `value %= k` (this one).
void func_801C7F34(u32 value) {
    ((u32*)g_Menu->unk2EC)[0] = value / 21600000;
    value %= 21600000;
    ((u32*)g_Menu->unk2EC)[1] = value / 2160000;
    value %= 2160000;
    ((u32*)g_Menu->unk2EC)[2] = value / 216000;
    value %= 216000;
    ((u32*)g_Menu->unk2EC)[3] = value / 36000;
    value %= 36000;
    ((u32*)g_Menu->unk2EC)[4] = value / 3600;
    value %= 3600;
    ((u32*)g_Menu->unk2EC)[5] = value / 600;
    value %= 600;
    ((u32*)g_Menu->unk2EC)[6] = value / 120;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C80B8);
/* Best: 82.91%. Instruction multiset and structure are right; only two things
   differ, and they are the same cause: the original has `srl` (the second half
   of `divisor /= 10`) before the `sb`, with the `sb` in the bnez delay slot,
   and allocates digit->a1, i->a2, divisor->a3, magic->t0 where gcc gives
   digit->v1, i->a1, divisor->a2, magic->a3. The original matches plain
   global_alloc priority order (digit 1 ref/2 insns, i and divisor 1, magic 0),
   so it looks like `digit` is a global allocno there and a local one here.
   Tried since: digit/u32 vs s32, u_char digit (80.35), `digit = 0` init (the
   dead init is removed, no change), declaration orders, `store first` and
   `store before divisor /= 10` (80.35), `number = number % divisor`, separate
   `value` copy of the parameter, u_char* pDigits (58.00), register on each
   local. None moves the allocation.
void func_801C80B8(u32 number) {
    s32 i;
    u32 digit;
    u32 divisor;

    divisor = 100000000;
    for (i = 0; i < 9; i++) {
        digit = number / divisor;
        number %= divisor;
        divisor /= 10;
        g_Menu->digits[i] = digit;
    }

    for (i = 1; i < 9; i++) {
        if (g_Menu->digits[i]) {
            if (g_Menu->digits[i - 1] == 0) {
                g_Menu->digits[i - 1] = 0xFF;
            }
            break;
        }
        g_Menu->digits[i - 1] = 0xFF;
    }
}
*/
void func_801C8164(POLY_G4* pPoly, u_char r, u_char g, u_char b) {
    SetPolyG4(pPoly);
    pPoly->r0 = r;
    pPoly->g0 = g;
    pPoly->b0 = b;
    pPoly->r1 = r;
    pPoly->g1 = g;
    pPoly->b1 = b;
    pPoly->r2 = 0;
    pPoly->g2 = 0;
    pPoly->b2 = 0;
    pPoly->r3 = 0;
    pPoly->g3 = 0;
    pPoly->b3 = 0;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C81E0);
/* Best: 86.86% (324 bytes, same size). Two instructions sit one slot apart:
   retail loads the 5th parameter (`lbu t1,0x10(sp)`) after the four word
   stores and fills the `beqz` delay slot with `li v0,1`, gcc hoists the lbu to
   the top of the block and puts `sw a3,0xC(v1)` in the delay slot instead.
   The base pointer has to be a local for the first five stores and a fresh
   `g_Menu + index*0x24` re-deref for everything after it (retail recomputes the
   base once per basic block, five times in all). Tried: re-deref everywhere
   (80.41), pointer local everywhere (83.26), natural field order 0/4/8/C
   (86.72), s32* vs u8* base (same), pointer local through the second if
   (83.26). Struct: g_Menu + index*0x24, size 0x24, first member of SystemMenu
   (indexed through the opaque unk0[108], 3 entries).
void func_801C81E0(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u8 arg4, u8 index) {
    s32* pEntry;

    pEntry = (s32*)((u8*)g_Menu + index * 0x24);
    pEntry[0] = arg0;
    pEntry[2] = arg1;
    pEntry[1] = arg2;
    pEntry[3] = arg3;
    if (arg2 < arg0) {
        arg2 = arg0 - arg2;
        *(u8*)((u8*)pEntry + 0x20) = 1;
    } else {
        arg2 = arg2 - arg0;
        *(u8*)((u8*)pEntry + 0x20) = 0;
    }
    if (arg3 < arg1) {
        arg1 = arg1 - arg3;
        *(u8*)((u8*)pEntry + 0x21) = 1;
    } else {
        arg1 = arg3 - arg1;
        *(u8*)((u8*)pEntry + 0x21) = 0;
    }
    if (arg3 < arg1) {
        arg1 = arg1 - arg3;
        ((u8*)g_Menu + index * 0x24)[0x21] = 1;
    } else {
        arg1 = arg3 - arg1;
        ((u8*)g_Menu + index * 0x24)[0x21] = 0;
    }
    if (arg2 < arg1) {
        ((s32*)((u8*)g_Menu + index * 0x24))[5] = 0x100;
        ((s32*)((u8*)g_Menu + index * 0x24))[4] = (arg2 << 8) / arg1;
    } else {
        ((s32*)((u8*)g_Menu + index * 0x24))[4] = 0x100;
        ((s32*)((u8*)g_Menu + index * 0x24))[5] = (arg1 << 8) / arg2;
    }
    ((u8*)g_Menu + index * 0x24)[0x22] = arg4;
    ((s32*)((u8*)g_Menu + index * 0x24))[6] = 0;
    ((s32*)((u8*)g_Menu + index * 0x24))[7] = 0;
    ((u8*)g_Menu + index * 0x24)[0x23] = 0;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8324);
void func_801C851C(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height) {
    pVertices[0].vx = x + 0xFF60;
    pVertices[0].vy = y + 0xFF90;
    pVertices[0].vz = 0;
    pVertices[1].vx = x + width - 0xA0;
    pVertices[1].vy = y + 0xFF90;
    pVertices[1].vz = 0;
    pVertices[2].vx = x + 0xFF60;
    pVertices[2].vy = y + height - 0x70;
    pVertices[2].vz = 0;
    pVertices[3].vx = x + width - 0xA0;
    pVertices[3].vy = y + height - 0x70;
    pVertices[3].vz = 0;
}
void func_801C8574(u_char soundEffectId) {
    if (g_Menu->unk32A) {
        func_80039DB8((g_Menu->unk2E4->sedId << 16) | soundEffectId);
    }
}
u16 func_801C85C0(u8 index) {
    return D_801E96C8[index];
}
u16 func_801C85DC(u8 index) {
    return D_801E96A8[index];
}
u16 func_801C85F8(u8 index) {
    return ~D_801E96C8[index];
}
u16 func_801C861C(u8 index) {
    return ~D_801E96A8[index];
}
u32 func_801C8640(u32 mask, u8 index) {
    return D_801E96C8[index] & mask;
}
u32 func_801C865C(u32 mask, u8 index) {
    return D_801E96A8[index] & mask;
}
u32 func_801C8678(u32 mask, u8 index) {
    return mask & D_801E96E8[index];
}
void func_801C8694(u8 index) {
    u_char flag;
    u_char counter;

    func_801D1E80();
    flag = 1;
    while (g_Menu->transitionEffectState) {
        func_801C7BF4();
    }
    func_801D22F4(0);
    while (flag) {
        if (ArchiveGetDiscNumber() == index + 1) {
            flag = 0;
        } else {
            func_801E92CC();
            func_801D2F4C(index * 3 - 0x7D);
            if (func_801E93A0(index + 1)) {
                func_801D32B4();
                func_801D2F4C(0x89);
                for (counter = 0x1D; counter != 0; counter--) {
                    func_801C7BF4();
                }
                func_801D32B4();
                func_801C7BF4();
            } else {
                func_801D32B4();
                flag = 0;
            }
        }
    }
    func_801D2484();
}
void func_801C87C4(void) {
    UnDeliverEvent(0xF4000001, 0x4);
    UnDeliverEvent(0xF4000001, 0x8000);
    UnDeliverEvent(0xF4000001, 0x100);
    UnDeliverEvent(0xF4000001, 0x2000);
}
s32 func_801C881C(void) {
    while (1) {
        if (TestEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[5]) == 1) {
            func_801C87C4();
            return 3;
        }
        if (TestEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[3]) == 1) {
            func_801C87C4();
            return 1;
        }
        if (TestEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[2]) == 1) {
            func_801C87C4();
            return 0;
        }
        if (TestEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[4]) == 1) {
            func_801C87C4();
            return 2;
        }
    }
}
s32 func_801C891C(long chan) {
    if (_card_info(chan)) {
        return D_801E9768[func_801C881C() & 0xFF];
    }
    return -1;
}
void func_801C8960(void) {
    func_801C7BF4();
    EnterCriticalSection();
    CloseEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[2]);
    CloseEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[3]);
    CloseEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[4]);
    CloseEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[5]);
    ExitCriticalSection();
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8A10);
void func_801C8BEC(void) {
    u_char count;

    if (g_Menu->menuUnk2->unk4FE6) {
        count = ++g_Menu->unk326;
        if (D_801E9779 < count) {
            func_801C8A10(0);
            func_801C8A10(1);
            if (g_Menu->menuUnk2->unk4F74[0] == -1 && g_Menu->menuUnk2->unk4F74[1] == -1) {
                g_Menu->unk334 = 0;
            }
            g_Menu->unk326 = 0;
        }
    }
}
void func_801C8CA4(u8 index) {
    s32 i;

    if (g_Menu->menuUnk2->unk4F74[index] != -2) {
        g_Menu->menuUnk2->unk4FE6 = 2;
        for (i = 0x3B; i != 0; i--) {
            func_801C7BF4();
        }
        g_Menu->menuUnk2->unk4FE6 = 0;
    }
}
void func_801C8D1C(u8 index) {
    s32 i;

    if (g_Menu->menuUnk2->unk4F74[index] != -2) {
        for (i = 0x3B; i != 0; i--) {
            Vsync(0);
        }
    }
}
