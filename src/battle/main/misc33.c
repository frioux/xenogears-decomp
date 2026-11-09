#include "common.h"
#include "system/memory.h"

typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ s32 unk1C;
} BattleMisc33State; // Size: 0x50

extern BattleMisc33State *D_800C3610;
extern s32 D_800C3618;
extern s32 D_800C3E20;
extern u16 D_800D2E54;
extern u8 D_800D3350;
extern u8 D_800D3351;
extern u8 D_800C3784[];
extern u8 D_800591AC;
extern s32 D_80059464;

extern s32 func_800C0FAC(s32);
extern void func_800C1140(s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE11C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE1C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE330);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE538);
void func_800BE6A0(s32 arg0, u8 *p, s32 n, s32 arg3) {
    s32 i;
    s32 last;
    u8 *pTable;
    u8 *pOut;
    s32 idx;

    last = n - 1;
    pTable = D_800C3784;
    for (i = 0; i != n; i++) {
        idx = (arg0 >> ((last - i) * 4)) & 0xF;
        pOut = i + p;
        pOut[1] = *(idx + pTable) + arg3;
    }
    p[0] = n;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE6E8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE790);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEB04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEBC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEC18);
void func_800BED30(void) {
    s32 *pInt;
    BattleMisc33State **ppState;
    u16 *pHalf;

    pInt = &D_800C3E20;
    ppState = &D_800C3610;
    pHalf = &D_800D2E54;
    *pInt = 0;
    *ppState = NULL;
    *pHalf = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BED4C);
void func_800BEDE8(void) {
    HeapFree(D_800C3610);
    D_80059464 = 0;
    D_800C3610 = NULL;
    D_800591AC = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEE2C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEEB4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEF24);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEF8C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEFF4);
void func_800BF0B4(s32 arg0) {
    D_800C3610->unk1C = arg0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF0C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF1EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF2B8);
s32 func_800BF354(void) {
    s32 ret;

    if (D_800D3350 == 0) {
        ret = func_800C0FAC(D_800C3618);
        D_800D3350 = 1;
    }
    return ret;
}
void func_800BF3A4(void) {
    u8 *pFlag;
    s32 *pState;

    pFlag = &D_800D3350;
    pState = &D_800C3618;
    if (*pFlag != 0) {
        func_800C1140(*pState);
        *pFlag = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF3E8);
