#include "common.h"
#include "system/memory.h"

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x12];
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u8 unk20[0x2C];
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ u8 unk50[0x30];
} Misc15Slot; // Size: 0x80

extern Misc15Slot *D_8009BE24;

extern void func_800976C8(void);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800967E4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800968E0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_8009699C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096A6C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096C0C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096F18);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097070);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097244);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097440);

void func_8009766C(void) {
    D_8009BE24 = HeapAlloc(0x2000, 0);
    func_800976C8();
}

void func_800976A0(void) {
    HeapFree(D_8009BE24);
}

void func_800976C8(void) {
    Misc15Slot *pSlot = D_8009BE24;
    s32 i;

    for (i = 0; i < 0x40; i++) {
        pSlot->unk4C = 0;
        pSlot->unk18 = 0;
        pSlot->unk1C = 0;
        pSlot++;
    }
}

void func_800976FC(s32 value, s32 index) {
    u8 *pSlot = (u8 *)D_8009BE24;

    pSlot += index << 7;
    *(u16 *)pSlot = 0;
    *(s32 *)(pSlot + 0x18) = value;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097718);

s32 func_80097770(s32 index, s16 value) {
    Misc15Slot *pSlot = &D_8009BE24[index];

    if (pSlot->unk4 != 0) {
        return 0;
    }
    pSlot->unk0 = 1;
    pSlot->unk4 = value;
    return 1;
}

void func_800977A8(s32 index) {
    *(u16 *)((u8 *)D_8009BE24 + (index << 7)) = 3;
}

void func_800977C4(s32 index) {
    *(u16 *)((u8 *)D_8009BE24 + (index << 7)) = 4;
}

void func_800977E0(s32 index, u16 value) {
    u8 *pSlot = (u8 *)D_8009BE24 + (index << 7);

    *(u16 *)pSlot = 2;
    *(u16 *)(pSlot + 2) = value;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097800);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800978FC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800979C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097BC0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097CB8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097D64);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097DC0);
