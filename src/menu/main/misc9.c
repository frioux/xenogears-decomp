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
void func_801E3ECC(MenuDressingRoom* pDressingRoom, s32 gearId);
void func_801E3C2C(MenuDressingRoom* pDressingRoom, s32 gearId);

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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DBDB4);
/* Best: 94.47% (gcc-2.6.0, the unit preset). Only difference: gcc turns this
   counted loop into a countdown (li a0,0x95 / addiu -1 / bgez) while retail
   keeps slti a0,0x96 / bnez. Swept gcc 2.6.0/2.7.2/2.7.2-cdk x -O1..-O3 x
   -G4/-G8/-G0 x 3000/r8000: every combination gives 94.47 or less, so no preset
   produces the slti shape. `while`/`do-while`/`for`, count++ before or after the
   if, count++ at the top of the body, `0x96 > count` all identical. `last` is
   genuinely uninitialised in retail (a1 is never set up front). Shape below
   reproduces retail's increment placement exactly (count++ in the beqz delay
   slot, index++ in the bnez delay slot).
void func_801DBDB4(void) {
    s32 count;
    s32 index;
    s32 last;

    index = 0;
    count = 0;
    while (count < 0x96) {
        if (g_GameState.itemIDs[index] != 0) {
            last = index;
        }
        count++;
        index++;
    }

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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DC1D4);
/* Best: 91.94%, same size (228 bytes). Everything matches except the case
   dispatch: retail built a binary-search tree (beq x,1 / slti x,2 / beqz x,0 /
   beq x,2) which gcc 2.7.2 and 2.6.0 (and -O3, and 2.7.2-cdk) only emit for a
   sparse switch; for these 3 dense cases both give a sequential compare chain
   (beq x,1 / beqz x,0 / beq x,2). A hand-written if/else chain in tree order
   inverts the branches instead. `kind` is genuinely uninitialised in retail
   (the default paths pass garbage to func_801C72BC).
void func_801DC1D4(u32 arg0) {
    s32 kind;
    s32 index = 0;

    g_Menu->unk430 = HeapAlloc(0x1094, 0);
    bzero(g_Menu->unk430, 0x1094);
    switch (arg0 & 0xFF) {
    case 0:
        kind = 2;
        break;
    case 1:
        kind = 5;
        break;
    case 2:
        kind = 6;
        index = 1;
        break;
    }
    func_801E8018(8, g_Menu->unk10E0, &D_801EA548[index * 8], g_Menu->pManager->unk38);
    func_801C72BC((u8)kind);
    func_801D22F4(2);
    func_801D3488(2, arg0 & 0xFF);
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DC2CC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DC3D8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DCE60);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD5E8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD790);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DDF24);

s32 func_801DE29C(u_char arg0, u_char arg1) {
    func_801DDF24(arg0, arg1, 0);
    return 1;
}

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
void func_801DFE2C(u_char arg0) {
    u8 charId;

    charId = g_Menu->pManager->currentCharacterIDs[arg0];
    func_801E3ECC(g_Menu->pDressingRoom, g_GameState.characters[charId].gearId);
    charId = g_Menu->pManager->currentCharacterIDs[arg0];
    func_801E3C2C(g_Menu->pDressingRoom, g_GameState.characters[charId].gearId);
    g_Menu->pDressingRoom->unkB8[0] = g_Menu->pDressingRoom->unkB0;
    g_Menu->pDressingRoom->unkB8[1] = g_Menu->pDressingRoom->totalDefense;
    g_Menu->pDressingRoom->unkB8[2] = g_Menu->pDressingRoom->totalEtherDefense;
    g_Menu->pDressingRoom->unkB8[3] = g_Menu->pDressingRoom->totalResponsiveness;
    g_Menu->pDressingRoom->unkB8[4] = g_Menu->pDressingRoom->unkB3;
    g_Menu->pDressingRoom->unkB8[5] = g_Menu->pDressingRoom->unkB4;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DFF5C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801E0434);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801E05D0);
