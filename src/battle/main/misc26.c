#include "common.h"

u8 *func_800B168C(u8 *pBase, s32 n) {
    return &pBase[n * 0x1C + 0xC];
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B16A4);

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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1EA0);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3658);
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3B6C);
/* best 94.00%: byte-identical except `li v0,1`, which maspsx expands to `ori`
   where retail has `addiu` (100% under --aspsx-version=2.67).
extern u8 *D_800C3558;

s32 func_800B3B6C(void) {
    if (D_800C3558 != NULL) {
        return D_800C3558[0x41];
    }
    return 1;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3B94);

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
