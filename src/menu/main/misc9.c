#include "common.h"

#include "main/game.h"
#include "psyq/libapi.h"
#include "system/memory.h"
#include "system/menu.h"

extern u8 D_801EA548[];
extern u8 D_801EA558[];
extern u16 D_801EA724;
extern s32 D_801EA728;
extern u16 D_801EA72C;

void func_801DDF24(u_char arg0, u_char arg1, s32 arg2);
void func_801C7BF4(void);
void func_801D4EA0(s32 arg0);
void func_801C72BC(s32 arg0);
void func_801D3444(void);
void func_801E8044(s32 arg0, void* pArg1);
void func_801E8018(s32 arg0, void* pArg1, void* pArg2, void* pArg3);
void func_801D22F4(s32 arg0);
void func_801DB02C(s32 arg0);
void func_801D3488(s32 arg0, s32 arg1);
void func_801D2484(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DB920);

void func_801DBD4C(s32 index0, s32 index1) {
    u_char tmp;

    tmp = g_GameState.itemIDs[index0];
    g_GameState.itemIDs[index0] = g_GameState.itemIDs[index1];
    g_GameState.itemIDs[index1] = tmp;
    tmp = g_GameState.itemQuantities[index0];
    g_GameState.itemQuantities[index0] = g_GameState.itemQuantities[index1];
    g_GameState.itemQuantities[index1] = tmp;
}

s32 func_801DBDB4(void) {
    s32 count;
    s32 index;
    s32 last;

    count = 0;
    index = 0;
    do {
        if (g_GameState.itemIDs[index] != 0) {
            last = index;
        }
        count++;
        index++;
    } while (count < 0x96);

    if (last < 0x10) {
        D_801EA724 = 0x74;
        D_801EA728 = 0;
        D_801EA72C = 0;
    } else {
        s32 n = (last - 0x10) / 2 + 1;
        s32 q = 0x1068 / n;

        D_801EA724 = 0x4A;
        D_801EA728 = n;
        D_801EA72C = q;
    }
}
/*
Best: 86.97%. Everything but the loop matches; gcc-2.7.2 turns the upward
counter into a countdown (li a0,0x95 / addiu a0,-1 / bgez), retail keeps
slti a0,0x96 / addiu a0,1.
    s32 count = 0;
    s32 index = 0;
    s32 last;

    do {
        if (g_GameState.itemIDs[index] != 0) {
            last = index;
        }
        count++;
        index++;
    } while (count < 0x96);

    if (last < 0x10) {
        D_801EA724 = 0x74;
        D_801EA728 = 0;
        D_801EA72C = 0;
    } else {
        s32 n = (last - 0x10) / 2 + 1;
        s32 q = 0x1068 / n;

        D_801EA724 = 0x4A;
        D_801EA728 = n;
        D_801EA72C = q;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DBE54);
void func_801DC1D4(u32 arg0) {
    s32 mode;
    s32 index = 0;

    g_Menu->unk430 = HeapAlloc(0x1094, 0);
    bzero(g_Menu->unk430, 0x1094);
    switch (arg0 & 0xFF) {
    case 0:
        mode = 2;
        break;
    case 1:
        mode = 5;
        break;
    case 2:
        mode = 6;
        index = 1;
        break;
    }
    func_801E8018(8, g_Menu->unk10E0, &D_801EA548[index * 8], g_Menu->pManager->unk38);
    func_801C72BC(mode & 0xFF);
    func_801D22F4(2);
    func_801D3488(2, arg0 & 0xFF);
}

void func_801DC2CC(u32 arg0) {
    s32 mode;

    func_801D4EA0(3);
    func_801D4EA0(4);
    func_801D4EA0(5);
    func_801D4EA0(6);
    func_801D2484();
    g_Menu->pManager->unk4A[0] = 0;
    func_801C7BF4();
    switch (arg0 & 0xFF) {
    case 0:
        mode = 2;
        break;
    case 1:
        mode = 5;
        break;
    case 2:
        mode = 6;
        break;
    }
    func_801C72BC((mode | 0x10) & 0xFF);
    HeapFree(g_Menu->unk430);
    g_Menu->pManager->unk5[1] = 1;
    g_Menu->pManager->shouldRenderWindow[1] = 1;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DC3D8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DCE60);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD5E8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD790);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DDF24);

s32 func_801DE29C(u32 arg0, u32 arg1) {
    func_801DDF24(arg0, arg1, 0);
    return 1;
}
/* Best: 67.27%. The C above is byte-identical under gcc-2.6.0 (try_flags 100.0)
   but gcc-2.7.2 fills the jr delay slot with the sp restore and sinks the
   return value after the ra restore.
*/

void func_801DE2C8(u32 arg0) {
    MenuManager* pManager;

    g_Menu->unk434 = HeapAlloc(0xA1C, 0);
    bzero(g_Menu->unk434, 0xA1C);
    pManager = g_Menu->pManager;
    func_801E8018(6, (u8*)g_Menu + 0x14E0, &D_801EA558[(arg0 & 0xFF) * 6], &pManager->unk38[8]);
    func_801C72BC(7);
    func_801D22F4(3);
    func_801DB02C(0);
    func_801D3488(1, arg0 & 0xFF);
}

void func_801DE36C(void) {
    g_Menu->pManager->unk4A[2] = 0;
    func_801D4EA0(2);
    func_801D4EA0(3);
    func_801D4EA0(4);
    func_801D4EA0(5);
    func_801C7BF4();
    func_801E8044(6, &g_Menu->pManager->unk38[8]);
    func_801C72BC(0x17);
    HeapFree(g_Menu->unk434);
    func_801D3444();
}

void func_801DE400(void) {
    g_Menu->pManager->unk5[3] = 0;
    g_Menu->pManager->unk4A[1] = 0;
    func_801C7BF4();
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->unk360);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DE474);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DE5CC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DF0D4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DF5D0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DF890);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DFB68);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DFE2C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DFF5C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801E0434);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801E05D0);
