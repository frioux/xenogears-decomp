#include "common.h"

extern u8 D_800C37C8;
extern u16 D_800C3E30;
extern u8 D_800C3D6C;
extern u8 *D_800C3BAC[];
extern s32 D_800C3BCC[];
extern void *D_800658C8;

extern void HeapFree(void *pMem);
extern s32 func_800A2330(void);
extern void func_800A23E8(s32, u8 *);
extern void func_8003852C(s32);
extern void func_800B00D0(void);

// Streamed battle data; only the fields this unit reads are named.
typedef struct {
    /* 0x0 */ u8 pad0[0x34C];
    /* 0x34C */ s16 unk34C;
    /* 0x34E */ s16 unk34E;
    /* 0x350 */ s16 unk350;
    /* 0x352 */ s16 unk352;
} BattleStreamData;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF2C4);

s32 func_800AF400(void) {
    s32 i;

    for (i = 0; i < 13; i++) {
        if ((D_800C3E30 >> i) & 1) {
            break;
        }
    }
    return i;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF438);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF518);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AF678);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFA98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFB4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFC68);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFD98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFF9C);
void func_800B0060(u8 *p) {
    if (*(s32 *)(p + 0xC) != 0) {
        if (*(u8 *)(p + 0x63) != 0) {
            func_8003852C(*(s32 *)(*(u8 **)(p + 0xB4) + 8));
            *(u8 *)(p + 0x63) = 0;
        }
        HeapFree(*(void **)(p + 0xC));
        *(s32 *)(p + 0xC) = 0;
        *(s32 *)(p + 0x18) = 0;
    }
}

void func_800B00D0(void) {
    s32 i = 8;
    s32 *p = D_800C3BCC;

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}

void func_800B00F4(s32 arg0) {
    s32 i = 0;
    u8 **p = D_800C3BAC;
    u8 *pEntry;

    for (; i < 9; i++) {
        pEntry = *p++;
        if (pEntry != NULL) {
            func_800A23E8(arg0, pEntry);
        }
    }
    func_800B00D0();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0164);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B026C);

s32 func_800B0AB4(s16 *pPos) {
    BattleStreamData *pData = D_800658C8;

    if (pPos[0] > pData->unk34E && pPos[0] < pData->unk34C && pPos[2] > pData->unk350 &&
        pPos[2] < pData->unk352) {
        return 1;
    }
    return 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0B14);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0D70);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0FF4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B10EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B12D0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B136C);
void func_800B14B8(void) {
    D_800C3D6C = 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B14CC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B15D8);
