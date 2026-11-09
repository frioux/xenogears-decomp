#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"

extern s32 D_800D2D40;
extern s32 D_800D2D48;
extern u8 D_800C3D04[];
extern u8 D_800C3D0C[];
extern u16 D_800C3D40;
extern u16 D_800C3E30;
extern u8 D_800D3611;
extern u8 *D_800D3368[];

extern void func_800A22A8(u8 *);
extern void func_800A2D1C(u8 *);
extern void func_800A9FF0(s32);
extern void func_800AA934(void *, void *, void *, void *);
extern s32 func_800AF400(void);
extern void func_800A429C(void *);
extern void func_800A64E4(void);
extern void func_800BF85C(s32, s32);

extern u8 *D_800D33E4;
extern u16 D_800D361A;
extern u_long *D_800D3604;
extern RECT D_800D3628;
extern void *D_800C3AC4;
extern void *D_800C3AC8;
extern u8 D_800C3D68;
extern u8 D_800591AC;
extern s32 D_80059464;
extern s32 D_800C360C;
extern u8 D_800C4000[];

// Per-unit 6-byte state records, unit id 0..3; the original spells each byte
// as its own symbol.
extern u8 D_800C3AAC[];
extern u8 D_800C3AAD[];
extern u8 D_800C3AAE[];
extern u8 D_800C3AAF[];
extern u8 D_800C3AB0[];
extern u8 D_800C3AB1[];

// Allocation block owned by the structure passed to func_800A8A88.
typedef struct {
    /* 0x0  */ u8 unk0[0x14];
    /* 0x14 */ void *unk14;
    /* 0x18 */ void *unk18;
    /* 0x1C */ void *unk1C;
    /* 0x20 */ void *unk20;
} Misc23Block;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5914);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5A48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5BE8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5D54);

void func_800A5E9C(s32 arg0, s32 arg1) {
    D_800D2D40 = arg0;
    D_800D2D48 = arg1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5EB4);

void func_800A6444(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    if (arg0 < 4) {
        if (arg1 >= 0) {
            D_800D3611 = 1;
            D_800C3AAC[arg0 * 6] = 1;
            D_800C3AAD[arg0 * 6] = arg1;
            D_800C3AAE[arg0 * 6] = arg2;
            D_800C3AAF[arg0 * 6] = arg3;
            D_800C3AB0[arg0 * 6] = arg4;
            D_800C3AB1[arg0 * 6] = arg5;
        } else {
            D_800C3AAC[arg0 * 6] = 0;
        }
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A64E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6884);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6AE8);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6F98);
/* 84.20%: identical instruction multiset except that the original keeps
   &D_800D3628 in s0 across LoadImage/DrawSync and gcc 2.7.2 rematerialises the
   symbol address instead (also costs an s0 slot and shuffles the i++ into the
   delay slot). Tried: RECT *pRect, u8 *pBuf, reusing the loop pointer, all
   identical code.
void func_800A6F98(void) {
    u8 *p;
    s32 i;

    if (D_800D361A != 0) {
        p = *(u8 **)(D_800D33E4 + 0x118);
        for (i = 0; i < *(u8 *)(D_800D33E4 + 0x10E); i++) {
            if (*(u16 *)(p + 0x1A) != 0) {
                *(s32 *)p = 0;
            }
            p += 0x30;
        }
        p = (u8 *)&D_800D3628;
        LoadImage((RECT *)p, D_800D3604);
        DrawSync(0);
        func_800A429C(p - 0x28);
    }
    func_800A64E4();
    HeapFree(D_800C3AC4);
    HeapFree(D_800C3AC8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7064);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7948);

void func_800A8A88(Misc23Block *pBlock) {
    if (pBlock->unk14 != NULL) {
        HeapFree(pBlock->unk14);
        HeapFree(*(void **)pBlock->unk1C);
        HeapFree(pBlock->unk1C);
        HeapFree(pBlock->unk20);
        if (pBlock->unk18 != NULL) {
            HeapFree(pBlock->unk18);
        }
        pBlock->unk14 = NULL;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8B0C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8BF0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9540);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A96B4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A979C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9A50);

void func_800A9F94(void) {
    s32 i;

    for (i = 0; i < 0x1F; i++) {
        func_800A9FF0(i);
    }
    func_800A22A8(D_800C3D0C);
    func_800A2D1C(D_800C3D04);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9FF0);

void func_800AA320(u16 arg0, u16 arg1, void *arg2) {
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    ((u8 *)D_800D3368[arg0])[0x35] = 0;
    if (D_800D3368[arg0] != NULL) {
        func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
    }
}

void func_800AA384(u16 arg0, u16 arg1, void *arg2) {
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    D_800C3D68 = 1;
    ((u8 *)D_800D3368[arg0])[0x35] = 1;
    D_80059464 = 0;
    D_800591AC = 1;
    func_800BF85C(arg0, func_800AF400());
    D_800C360C = 1;
    D_800C4000[func_800AF400()] = 0;
    if (D_800D3368[arg0] != NULL) {
        func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
    }
}
void func_800AA454(u16 arg0, u16 arg1, void *arg2) {
    u16 savedIdx;
    u16 savedUnk;

    savedIdx = D_800C3D40;
    savedUnk = D_800C3E30;
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    ((u8 *)D_800D3368[arg0])[0x35] = 0;
    if (D_800D3368[arg0] != NULL) {
        if ((s32)arg0 != func_800AF400()) {
            func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
        }
    }
    D_800C3D40 = savedIdx;
    D_800C3E30 = savedUnk;
}
