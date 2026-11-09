#include "common.h"

#include "system/menu.h"

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E433C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4754);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4928);
/* Best: 91.61%
u8 func_801E4928(u8 gearId) {
    GameGear* pGear;
    u_short weightClass;
    s16 value;

    pGear = &g_GameState.gears[gearId];
    weightClass = pGear->weightFromEquipment / 120;
    value = (weightClass - pGear->unk75) / 2;
    if (value < 0) {
        value = 0;
    }
    return value;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4998);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4A28);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4D10);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5058);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5178);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E53CC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E56E8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5924);
void func_801E5ACC(void) {
    void* pMem;
    int i;

    for (i = 0; i < 32; i++) {
        pMem = HeapAlloc(0x158, 0);
        g_Menu->unk3A8[i] = pMem;
        bzero(pMem, 0x158);
        func_801E56E8(i);
        func_801E5924(i);
    }
}
extern u16 D_801EA5D0[];
extern u32 D_801EA004[];
extern u32 D_801EA010[];
extern u8 D_801EA8C0;

void func_801E5B3C(void) {
    int i;

    for (i = 0; i < 32; i++) {
        HeapFree(g_Menu->unk3A8[i]);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5B88);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5E4C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E61B0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E6450);
/* Best: 99.79%
void func_801E6450(void) {
    void* pMem;

    pMem = HeapAlloc(0x2DC0, 0);
    g_Menu->unk34C = pMem;
    bzero(pMem, 0x2DC0);
    func_801E5B88();
    func_801E5E4C();
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E649C);
/* Best: 81.18%
void func_801E649C(void) {
    HeapFree(g_Menu->unk34C);
    g_Menu->pManager->unkB = 0;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E64E0);
/* Best: 87.20%
void func_801E64E0(void) {
    RECT rect;

    rect.x = 0x140;
    rect.y = 0xE0;
    rect.w = 0x40;
    rect.h = 0x20;
    ClearImage(&rect, 0, 0, 0);
    g_Menu->pManager->unkB = 0;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E6544);
s32 func_801E65E4(u8* p) {
    u8 hi;
    u8 lo;

    hi = p[0];
    lo = p[1];
    D_801EA8C0 = 1;
    if (hi < 0x80) {
        if (hi >= 0x20) {
            lo = *(u8*)&D_801EA5D0[hi];
            hi = D_801EA5D0[hi] >> 8;
            D_801EA8C0 = 0;
        } else {
            hi = 0x81;
            lo = 0x40;
            D_801EA8C0 = 0;
        }
    }
    return Krom2RawAdd((hi << 8) | lo);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E6668);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E68AC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E6AE8);
/* Best: 99.71%
void func_801E6AE8(u8 arg0, u8* pArg1) {
    u8* pPrims;
    u32 offset;

    offset = 0xA98 + arg0 * 0x87C;
    pPrims = (u8*)(offset + (u32)g_Menu->unk34C);
    func_8002675C(g_Menu->resources, pArg1[arg0 + 0x1C] + 0x14E, pPrims,
                  g_Menu->renderContext, D_801EA004[arg0], D_801EA010[arg0], 0x1000);
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E6B70);
