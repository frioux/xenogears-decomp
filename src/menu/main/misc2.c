#include "common.h"

#include "psyq/libapi.h"
#include "system/menu.h"

extern u16 D_801E96A8[]; // Bit masks, LSB first
extern u16 D_801E96C8[]; // Bit masks, MSB first
extern u32 D_801E96E8[];
extern s32 D_801E9768[]; // Card result to error code

s32 func_801C881C(void);
extern void func_80039DB8(s32 arg0);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7B0C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7BF4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7D78);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C7F34);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C80B8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8164);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C81E0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8324);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C851C);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8960);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8A10);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8BEC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8CA4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8D1C);
