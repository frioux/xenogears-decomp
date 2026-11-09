#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x12];
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x28];
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ u8 unk50[0x30];
} Misc15Slot; // Size: 0x80

extern Misc15Slot *D_8009BE24;
extern void *D_8009C184[];
extern MATRIX D_8009A180;
extern MATRIX D_8009D534;
extern u32 D_8009C580[];
extern s32 D_8009C5BC;
extern s32 D_8009C618;
extern u8 *D_8009BC3C;
extern u8 *D_8009BCB4;
extern s32 D_8009BCB8;
extern s32 D_8009BD2C;
extern s32 D_8009CD44;
extern s32 D_8009D788[];
extern void func_800981C8(s32);
extern void func_80097DC0(void);

extern void func_800976C8(void);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800967E4);
s32 func_800968E0(void) {
    switch (D_8009CD44) {
    case 0:
        return 0;
    case 1:
    case 2:
    case 3:
        return 1;
    case 4:
        if (--D_8009BD2C == 0) {
            D_8009CD44++;
        }
        return 1;
    case 5:
        D_8009CD44 = 0;
        D_8009D788[D_8009BCB8] = 0;
        D_8009BCB8 = (D_8009BCB8 + 1) & 0xF;
        return 2;
    }
    return 3;
}
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
    s32 i;

    for (i = 0; i < 0x40; i++) {
        D_8009BE24[i].unk4C = 0;
        D_8009BE24[i].unk18 = 0;
        D_8009BE24[i].unk1C = 0;
    }
}

void func_800976FC(s32 value, s32 index) {
    D_8009BE24[index].unk0 = 0;
    D_8009BE24[index].unk18 = value;
}

void func_80097718(s32 value18, s32 value1C) {
    s32 i;

    for (i = 0; i < 0x40; i++) {
        if (D_8009BE24[i].unk1C == 0) {
            D_8009BE24[i].unk0 = 0;
            D_8009BE24[i].unk2 = 0;
            D_8009BE24[i].unk4 = 0;
            D_8009BE24[i].unk18 = value18;
            D_8009BE24[i].unk1C = value1C;
            D_8009BE24[i].unk20 = 0;
            D_8009BE24[i].unk22 = 0;
            break;
        }
    }
}

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
// Best: 74.73%. The 0x20-byte slot filler loop and the 0x10000-byte block copy
// match exactly (addiu v1,v1,6 bias and the bne-vs-pEnd loop included); what is
// missing is the original's s0 = &D_8009BC3C held across the 2nd HeapAlloc call
// (this shape gets lui at + a fresh lui for the later read, so the frame has no
// s0 slot).
/*
typedef struct {
    u32 unk0[4];
} Misc15Block16; // Size: 0x10

void func_800978FC(void) {
    u8 **ppBuffer;
    Misc15Block16 *pSrc;
    Misc15Block16 *pDst;
    Misc15Block16 *pEnd;
    u8 *pSlot;
    s32 i;

    ppBuffer = &D_8009BC3C;
    *ppBuffer = HeapAlloc(0x10000, 1);
    D_8009BCB4 = HeapAlloc(0x10000, 1);
    pSlot = *ppBuffer;
    for (i = 0; i < 0x800; i++) {
        pSlot[3] = 7;
        pSlot[7] = 0x24;
        pSlot[4] = 0x80;
        pSlot[5] = 0x80;
        pSlot[6] = 0x80;
        pSlot += 0x20;
    }
    pSrc = (Misc15Block16 *)D_8009BC3C;
    pDst = (Misc15Block16 *)D_8009BCB4;
    pEnd = pSrc + 0x1000;
    while (pSrc != pEnd) {
        *pDst++ = *pSrc++;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800978FC);INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800979C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097BC0);
// Best: 94.28%. Instruction multiset is identical; only the clear loop's
// allocator tie-break differs (original keeps pEnd in v0 and i in v1, this
// shape gets the two swapped).
/*
void func_80097CB8(s32 arg0) {
    s32 i;
    u32 *pEnd;

    D_8009D534 = D_8009A180;
    pEnd = D_8009C580;
    for (i = 0xFF; i >= 0; i--) {
        *pEnd-- = 0;
    }
    D_8009C5BC = 0;
    D_8009C618 = 0x400;
    func_800981C8(arg0);
    func_80097DC0();
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097CB8);
void func_80097D64(void) {
    s32 i;

    for (i = 0; i < 0x100; i++) {
        if (D_8009C184[i] != NULL) {
            HeapFree(D_8009C184[i]);
        }
    }
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097DC0);
