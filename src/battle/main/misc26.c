#include "common.h"

extern u8 *D_800C3558;
extern u32 D_800C3548;
extern u8 D_800C355C;
extern u32 D_800C3560;

extern u8 *func_800B35C0();
extern void func_800B3358(u8 *pArg0);
extern void func_800B39C0(s32, s32, u8, u8, u8);
extern void BattleSetActorRectParams(s32, s32, s32, s32, s32, s32);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void BattleUploadAndFreeImageResources(void);

u8 *func_800B168C(u8 *pBase, s32 n) {
    return &pBase[n * 0x1C + 0xC];
}

s32 func_800B16A4(u8 *pBase) {
    s32 count;
    s32 total;
    u8 *p;
    s32 n;

    count = 0;
    total = 0;
    p = (u8 *)((u32)*(s32 *)(pBase + 0x10) + (u32)pBase);
    n = *(s32 *)(pBase + 0x14);
    for (; count != n; count++) {
        total += (p[0] + 1) * 4;
        p += (p[1] + 1) * 4;
    }
    return total;
}

// best 62.50% under cdk -G0 (16.67% under 2.7.2 and 2.6.0): same instruction
// multiset, but gcc loads the counter global first where retail loads the
// pointer global first, into the register it reuses for the final store.
/*
void func_800B16F0(void) {
    u8 *pCmd = D_800C3BEC;

    D_800C3BF0 += 1;
    D_800C3BEC = pCmd + (pCmd[1] + 1) * 4;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B16F0);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1720);
void BattleModelScaleParts(u8 *pBase, s32 shift) {
    s16 *p;
    s32 n;
    s32 i;

    if (*(u32 *)(pBase + 0x18) & 0x8000) {
        return;
    }
    *(u32 *)(pBase + 0x18) |= 0x8000;
    p = (s16 *)((u32)*(s32 *)pBase + (u32)pBase);
    n = *(s32 *)(pBase + 4);
    for (i = 0; i != n; i++) {
        p[i * 4 + 0] <<= shift;
        p[i * 4 + 1] <<= shift;
        p[i * 4 + 2] <<= shift;
    }
}

// best 71.75% under cdk -G0 (59.46% under 2.7.2): -G0 gives retail's absolute
// store, but gcc re-forms &D_800C3BF8 for the second word where retail keeps one
// base register and reads it at +4.
/*
void func_800B1F0C(u8 *pOt) {
    u8 *pBuf = g_GfxCurWorkBuffer;

    if ((u8 *)(pBuf + 8) < g_GfxCurWorkBufferEnd) {
        g_GfxCurWorkBuffer = pBuf + 8;
        ((u32 *)pBuf)[0] = D_800C3BF8[0];
        ((u32 *)pBuf)[1] = D_800C3BF8[1];
        AddPrim(pOt, pBuf);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1F0C);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1F6C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B2AEC);

void func_800B3348(void) {
}

void func_800B3350(void) {
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3358);

void func_800B3588(void *pEntry) {
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
    D_800C3548 = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B35C0);
void BattleCreateTimedScaleTask(u8 *pSrc, s32 n) {
    u8 *p = func_800B35C0();

    *(u16 *)(p + 0x2C) = *(u16 *)(pSrc + 0);
    *(u16 *)(p + 0x2E) = *(u16 *)(pSrc + 2);
    *(u16 *)(p + 0x30) = *(u16 *)(pSrc + 4);
    *(s32 *)(p + 0x38) = n * 2;
    *(s32 *)(p + 0x3C) = n * 2;
    func_800B3358(p);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B36BC);

void func_800B383C(u8 *pEntry) {
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    D_800C3558 = NULL;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3878);

void func_800B397C(s32 a0, s32 a1, u8 a2, u8 a3, u8 a4) {
    D_800C355C = 1;
    func_800B39C0(a0, a1, a2, a3, a4);
    D_800C355C = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B39C0);

s32 BattleGetActorFlag41(void) {
    if (D_800C3558 != NULL) {
        return D_800C3558[0x41];
    }
    return 1;
}

// best 88.68% under cdk -G0, cdk -G8 and 2.6.0: register-only differences (which
// of the two $a0/$a1 copies feeds each load), same instruction multiset. Retail's
// else path reads the argument from $a0, leaving the branch delay slot empty
// where reorg fills ours.
/*
void func_800B3B94(u8 *pArg) {
    s32 n;
    s32 num;
    s32 base;
    s32 delta;

    if (*(s32 *)(pArg + 0x3C) != 0) {
        n = *(s32 *)(pArg + 0x3C) - 1;
        num = n * 0x20 / *(s32 *)(pArg + 0x38);
        base = *(s16 *)(pArg + 0x42);
        delta = (base - *(s16 *)(pArg + 0x40)) * num;
        *(s32 *)(pArg + 0x3C) = n;
        *(s16 *)(pArg + 0x46) = base - delta / 0x20;
    } else if (*(s16 *)(pArg + 0x46) == 0) {
        ((void (*)(u8 *))*(u32 *)(pArg + 0xC))(pArg);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3B94);

void func_800B3C2C(u8 *pEntry) {
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
    D_800C3560 = 0;
    BattleUploadAndFreeImageResources();
}

// best 60.58% under cdk -G0 (56.38% under 2.7.2): retail keeps 0x46 in $t0 and
// loads 0x48/0x4a/0x4c/0x4e after the branch; gcc hoists those loads above it
// and spills two of them.
/*
void func_800B3C74(u8 *pArg) {
    u8 *p = *(u8 **)(pArg + 4);
    s16 prev;
    s16 cur;

    prev = *(s16 *)(p + 0x44);
    cur = *(s16 *)(p + 0x46);
    if (prev != cur) {
        *(u16 *)(p + 0x44) = *(u16 *)(p + 0x46);
        BattleSetActorRectParams(0, *(s16 *)(p + 0x48), 0x20 - cur, *(s16 *)(p + 0x4A),
                      *(s16 *)(p + 0x4C), *(s16 *)(p + 0x4E));
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3C74);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3CD4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3E04);
