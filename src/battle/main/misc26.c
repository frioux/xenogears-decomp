#include "common.h"

extern u8 *D_800C3558;

extern u8 *func_800B35C0();
extern void func_800B3358(u8 *pArg0);
extern void func_800A6444(s32, s32, s32, s32, s16, s16);

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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B16F0);
/* best 16.67%: gcc re-forms each global's address instead of keeping the
   `lui a1,%hi` retail shares between the load and the store.
extern s32 D_800C3BF0;
extern u8 *D_800C3BEC;

void func_800B16F0(void) {
    u8 *pCmd = D_800C3BEC;

    D_800C3BF0 += 1;
    D_800C3BEC = pCmd + (pCmd[1] + 1) * 4;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1720);
void func_800B1EA0(u8 *pBase, s32 shift) {
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1F0C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1F6C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B2AEC);

void func_800B3348(void) {
}

void func_800B3350(void) {
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3358);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3588);
/* best 99.29%: retail stores through `lui v0,%hi`, gcc+maspsx through `$at`.
extern u32 D_800C3548;
extern void TimerWorkListRemoveTask(void *pEntry);
extern void HeapFree(void *pMem);

void func_800B3588(void *pEntry) {
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
    D_800C3548 = 0;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B35C0);
void func_800B3658(u8 *pSrc, s32 n) {
    u8 *p = func_800B35C0();

    *(u16 *)(p + 0x2C) = *(u16 *)(pSrc + 0);
    *(u16 *)(p + 0x2E) = *(u16 *)(pSrc + 2);
    *(u16 *)(p + 0x30) = *(u16 *)(pSrc + 4);
    *(s32 *)(p + 0x38) = n * 2;
    *(s32 *)(p + 0x3C) = n * 2;
    func_800B3358(p);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B36BC);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B383C);
/* best 99.33%: same $at store as func_800B3588.
extern u8 *D_800C3558;
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);

void func_800B383C(u8 *pEntry) {
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    D_800C3558 = NULL;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3878);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B397C);
/* best 66.12%: retail keeps the flag address in a saved register across the
   call where gcc re-forms it into $at twice, and its `li v0,1` is an addiu
   where maspsx expands the li to ori.
extern u8 D_800C355C;
extern void func_800B39C0(s32, s32, u8, u8, u8);

void func_800B397C(s32 a0, s32 a1, u8 a2, u8 a3, u8 a4) {
    D_800C355C = 1;
    func_800B39C0(a0, a1, a2, a3, a4);
    D_800C355C = 0;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B39C0);

s32 func_800B3B6C(void) {
    if (D_800C3558 != NULL) {
        return D_800C3558[0x41];
    }
    return 1;
}

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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3C2C);
/* best 90.83%: same $at store as func_800B3588.
extern u32 D_800C3560;
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void HeapFree(void *pMem);
extern void func_800A6F98(void);

void func_800B3C2C(u8 *pEntry) {
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
    D_800C3560 = 0;
    func_800A6F98();
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3C74);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3CD4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3E04);
