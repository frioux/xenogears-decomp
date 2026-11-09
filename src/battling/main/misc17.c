#include "common.h"

extern s32 D_80059488;

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 unk10[4];
    /* 0x20 */ u32* unk20;
} BattleSlot; // Size: 0x24

extern BattleSlot D_80096EA0[];

extern void func_8007639C(u8*, u8);
extern void func_800767C8(u8*, u8*);
extern void func_8008E78C(u8*, u32, u8*, u32);
extern void func_80039FF8(void);
extern void func_8008FE80(u8*, u8*, s32);
extern void func_80090E10(u8*);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E120);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E2B8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E3CC);
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
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E67C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E6F8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E78C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E8B0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008EADC);
void func_8008EB4C(u8* p) {
    if (p != NULL) {
        func_8008E78C(p + 0x60000, 0, NULL, D_80059488);
    }
}
void func_8008EB88(u8* p, s32 idx, u8* pData, s32 flags) {
    if (idx != 0) {
        func_8008E78C((u8*)(idx + 0x60000), flags, pData,
                      (idx & 0x7F) | ((*(u32*)(p + 0xD0) >> 20) & 0x80));
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008EBD0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008ECEC);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008ED6C);
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
    u32 timer;

    flags = *(u32*)(p1 + 0x2C);
    *(u32*)(p1 + 0x2C) = (flags & ~0x2000) | ((((flags >> 13) & 1) ^ 1) << 13);
    if (*(u32*)(p1 + 0x2C) & 0x2000) {
        *(u32*)(p0 + 0xD0) |= 2;
        timer = 0x3C;
    } else {
        *(u32*)(p0 + 0xD0) &= ~2;
        *(u32*)(p0 + 0xD0) &= ~0x38;
        timer = 0x1E;
    }
    *(u16*)(p1 + 4) = timer;
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
