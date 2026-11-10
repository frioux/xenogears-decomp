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

s32 func_801C881C(void);
extern void func_80039DB8(s32 arg0);
extern int Vsync(int mode);

void func_801C7BF4(void);
void func_801C8A10(s32 arg0);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7B0C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7BF4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7D78);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7F34);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C80B8);
/* Best: 82.91%. Instruction multiset and structure are right; only two things
   differ, and they are the same cause: the original has `srl` (the second half
   of `divisor /= 10`) before the `sb`, with the `sb` in the bnez delay slot,
   and allocates digit->a1, i->a2, divisor->a3 where gcc gives digit->v1,
   i->a1, divisor->a2. Already tried: no digit local (80.35%), digit as a
   separate u32 local, declaration orders (i/digit/divisor, 4 orderings),
   `divisor /= 10` in the for-increment clause, `value = number` copy.
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8694);
void func_801C87C4(void) {
    UnDeliverEvent(0xF4000001, 0x4);
    UnDeliverEvent(0xF4000001, 0x8000);
    UnDeliverEvent(0xF4000001, 0x100);
    UnDeliverEvent(0xF4000001, 0x2000);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C881C);
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
