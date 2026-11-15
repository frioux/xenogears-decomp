#include "common.h"

#include "main/game.h"
#include "psyq/libapi.h"
#include "system/memory.h"
#include "system/menu.h"

extern u8 D_801EA548[][8]; // 8 u8 string IDs per panel
extern u8 D_801EA558[][6]; // 6 u8 string IDs per panel
extern u16 D_801EA724;
extern u8 D_801EA730[];
extern s32 D_801EA728;
extern u16 D_801EA72C;
extern u8 D_801E9EA0[];

void func_801DDF24(u_char arg0, u_char arg1, s32 arg2);
void MenuRenderFrame(void);
void MenuHidePointerCursors(void);
void MenuFreeWindow(s32 arg0);
void func_801C72BC(s32 arg0);
void MenuFreeScrollHandle(void);
void MenuClearBytes(s32 arg0, void* pArg1);
void MenuLoadStringsFromIds(s32 arg0, void* pArg1, void* pArg2, void* pArg3);
void MenuSetupPointerCursors(s32 arg0);
void MenuInitializeArrowCursor(s32 arg0);
void func_801D3488(s32 arg0, s32 arg1);
void func_801E3ECC(MenuDressingRoom* pDressingRoom, s32 gearId);
void MenuCreateWindow(u_char a0, u_char a1, u_char a2, u_char a3, u16 a4, u_char a5, u_char a6, u32 a7, u_char a8);
void func_801E8070(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u8 arg5, u8 arg6, u8 arg7);
void MenuFillDressingRoomGearStats(MenuDressingRoom* pDressingRoom, s32 gearId);
void MenuSetPrimShadingMode(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DB920);

void MenuSwapInventoryItems(s32 index0, s32 index1) {
    u_char tmp;

    tmp = g_GameState.itemIDs[index0];
    g_GameState.itemIDs[index0] = g_GameState.itemIDs[index1];
    g_GameState.itemIDs[index1] = tmp;
    tmp = g_GameState.itemQuantities[index0];
    g_GameState.itemQuantities[index0] = g_GameState.itemQuantities[index1];
    g_GameState.itemQuantities[index1] = tmp;
}

void MenuFitInventoryWindowToItems(void) {
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
        do {
        } while (0);
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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DBE54);

void MenuCreateUnk430Panel(u_char arg0) {
    s32 kind;
    s32 index = 0;

    g_Menu->unk430 = HeapAlloc(0x1094, 0);
    bzero(g_Menu->unk430, 0x1094);
    switch (arg0) {
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
    MenuLoadStringsFromIds(8, g_Menu->unk10E0, D_801EA548[index], g_Menu->pManager->unk38);
    func_801C72BC((u8)kind);
    MenuSetupPointerCursors(2);
    func_801D3488(2, arg0);
}

void MenuFreeUnk430Panel(u_char arg0) {
    s32 kind;

    MenuFreeWindow(3);
    MenuFreeWindow(4);
    MenuFreeWindow(5);
    MenuFreeWindow(6);
    MenuHidePointerCursors();
    g_Menu->pManager->unk4A[0] = 0;
    MenuRenderFrame();
    switch (arg0 & 0xFF) {
    case 0:
        kind = 2;
        break;
    case 1:
        kind = 5;
        break;
    case 2:
        kind = 6;
        break;
    }
    func_801C72BC((kind | 0x10) & 0xFF);
    HeapFree(g_Menu->unk430);
    g_Menu->pManager->unk5[1] = 1;
    g_Menu->pManager->shouldRenderWindow[1] = 1;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DC3D8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DCE60);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD5E8);
/* Best: 94.01%, same size (416 bytes), from the round-63 attempt (u_char arg0).
   Remaining deltas, all tried by hand this round:
   1. loop 1: retail forms the compare address as base + (rc*0x28 + i*0x80)
      (two addu, keeping base unmodified) and only then reuses the saved
      base + i*0x80 and rc*0x28 for the call argument (addu a2,a1). Every C
      that names pStrings[i].polys[pStrings[i].renderContext] once, or via a
      separate ctx local, or via a pString pointer local, CSEs to
      (base + i*0x80) + rc*0x28 and loses one addu (93.96-94.01%).
   2. loop 2: retail repeats andi a1,arg0,0xff in the loop-back delay slot;
      gcc un-hoists it out of the loop and leaves a nop there. A `u_char mode
      = arg0;` local inside the body does not stop the hoist.
   3. u_int arg0 with (arg0 & 0xFF) at each call site (so the mask is a real
      per-site expression) gives 93.58%.
   permute.sh --minutes 4 -j 2: PERMUTER_BETTER 777 -> 547, but the winner
   caches g_Menu in a callee-saved local, which drops a lui/lw pair and
   compiles to 408 bytes - wrong size, so not portable.
   Round 106 re-check: the whole loop-1 delta is that one addu, nothing else.
   Dropping the pPoly local and writing the address twice, and the
   'pStrings[i].polys + ctx' spelling, both compile identically to each other
   and to the version below (416 bytes, if-body one insn short, 93.96%): gcc
   CSEs the poly address out of the compare and the argument regardless, and
   the load then uses the record-base register that retail keeps in a2.
   void func_801DD5E8(u_char arg0) {
       MenuString* pStrings;
       POLY_FT4* pPoly;
       s32 i;

       func_801E8F60(5, arg0);
       func_801E8F60(6, arg0);
       for (i = 0; i < 0xC; i++) {
           pStrings = g_Menu->unk430;
           pPoly = &pStrings[i].polys[pStrings[i].renderContext];
           if (pPoly->r0 != 0x20) {
               MenuSetPrimShadingMode(pPoly, arg0);
               pStrings = g_Menu->unk430;
               pPoly = &pStrings[i + 0xE].polys[pStrings[i + 0xE].renderContext];
               MenuSetPrimShadingMode(pPoly, arg0);
           }
       }
       MenuSetPrimShadingMode(&g_Menu->arrowCursors[0]->polys[g_Menu->arrowCursors[0]->renderContext], arg0);
       pStrings = g_Menu->unk430;
       MenuSetPrimShadingMode(&pStrings[0x1C].polys[pStrings[0x1C].renderContext], arg0);
       for (i = 0; i < 2; i++) {
           pStrings = g_Menu->unk430;
           pPoly = &pStrings[0x1E + i].polys[pStrings[0x1E + i].renderContext];
           MenuSetPrimShadingMode(pPoly, arg0);
       }
   }
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD790);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DDF24);

s32 MenuRunUnk430Panel(u_char arg0, u_char arg1) {
    func_801DDF24(arg0, arg1, 0);
    return 1;
}

void MenuInitializeCharacterSpriteRecords(u32 arg0) {
    MenuManager* pManager;

    g_Menu->unk434 = HeapAlloc(0xA1C, 0);
    bzero(g_Menu->unk434, 0xA1C);
    pManager = g_Menu->pManager;
    MenuLoadStringsFromIds(6, g_Menu->unk14E0, D_801EA558[arg0 & 0xFF], pManager->unk40);
    func_801C72BC(7);
    MenuSetupPointerCursors(3);
    MenuInitializeArrowCursor(0);
    func_801D3488(1, arg0 & 0xFF);
}

void MenuFreeCharacterSpriteRecords(void) {
    g_Menu->pManager->unk4A[2] = 0;
    MenuFreeWindow(2);
    MenuFreeWindow(3);
    MenuFreeWindow(4);
    MenuFreeWindow(5);
    MenuRenderFrame();
    MenuClearBytes(6, &g_Menu->pManager->unk38[8]);
    func_801C72BC(0x17);
    HeapFree(g_Menu->unk434);
    MenuFreeScrollHandle();
}

void MenuFreeUnk35CPanel(void) {
    g_Menu->pManager->unk5[3] = 0;
    g_Menu->pManager->unk4A[1] = 0;
    MenuRenderFrame();
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->unk360);
}

void MenuCreateListWindow(u_char arg0, u_char arg1) {
    s32 i;
    s32 start;
    s32 count;
    s32 label;

    for (i = 0; i < 6; i++) {
        g_Menu->pManager->unk40[i] = 0;
    }

    if (arg0 != 0) {
        start = 2;
        count = 6;
        label = 0x72;
    } else {
        start = 0;
        count = 2;
        label = 0x5A;
    }

    for (i = start; i < count; i++) {
        func_801E8070(6, g_Menu->unk14E0, D_801EA558[arg1 & 0xFF], D_801E9EA0,
                      g_Menu->pManager->unk40, (u8)i, (u8)i, 3);
    }

    if (g_Menu->pManager->shouldRenderWindow[4] != 0) {
        MenuFreeWindow(4);
    }

    MenuCreateWindow(4, 0x10, 0xC, 0x80, label, 0, 1, 4, 0);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DE5CC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DF0D4);
void MenuCopyEquipmentToPanel(u_char arg0, u_char arg1) {
    s32 i;

    for (i = 0; i < 9; i++) {
        ((u16*)((u8*)g_Menu->unk360 + 0x280))[i] = g_Menu->pDressingRoom->unkB8[i];
    }

    if (arg1 == 0) {
        for (i = 0; i < 5; i++) {
            ((u8*)g_Menu->unk360 + i)[0x29C] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk6A[i];
            ((u8*)g_Menu->unk360 + i)[0x2A1] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk6F[i];
            ((u8*)g_Menu->unk360 + i)[0x2A6] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk74[i];
        }
    } else {
        for (i = 0; i < 4; i++) {
            ((u8*)g_Menu->unk360 + i)[0x29C] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].weaponId[i];
            ((u8*)g_Menu->unk360 + i)[0x2A1] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].unk4[i];
            ((u8*)g_Menu->unk360 + i)[0x2A6] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].accessorySlots[i];
        }
    }
}

void MenuCopyEquipmentFromPanel(u_char arg0, u_char arg1) {
    s32 i;

    if (arg1 == 0) {
        for (i = 0; i < 5; i++) {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk6A[i] =
                ((u8*)g_Menu->unk360 + i)[0x29C];
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk6F[i] =
                ((u8*)g_Menu->unk360 + i)[0x2A1];
        }
        for (i = 0; i < 3; i++) {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk74[i] =
                ((u8*)g_Menu->unk360 + i)[0x2A6];
        }
    } else {
        for (i = 0; i < 4; i++) {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].weaponId[i] =
                ((u8*)g_Menu->unk360 + i)[0x29C];
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].unk4[i] =
                ((u8*)g_Menu->unk360 + i)[0x2A1];
        }
        for (i = 0; i < 3; i++) {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].accessorySlots[i] =
                ((u8*)g_Menu->unk360 + i)[0x2A6];
        }
    }
}
void MenuSetEquipmentSlotValue(u_char arg0, s32 arg1, s32 arg2, s32 arg3, u_char arg4, u_char arg5) {
    if (arg5 == 0) {
        if (arg4 == 0) {
            if (arg1 == 0) {
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk6A[0] =
                    D_801EA730[arg3 + arg2];
            } else {
                if (arg1 < 0) {
                    return;
                }
                if (arg1 > 3) {
                    return;
                }
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk74[arg1 - 1] =
                    D_801EA730[arg3 + arg2];
            }
        } else {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].unk6F[arg1] =
                D_801EA730[arg3 + arg2];
        }
    } else {
        if (arg4 == 0) {
            if (arg1 == 0) {
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId]
                    .weaponId[0] = D_801EA730[arg3 + arg2];
            } else {
                if (arg1 < 0) {
                    return;
                }
                if (arg1 > 3) {
                    return;
                }
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId]
                    .accessorySlots[arg1 - 1] = D_801EA730[arg3 + arg2];
            }
        } else {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId]
                .unk4[arg1] = D_801EA730[arg3 + arg2];
        }
    }
}
void MenuUpdateDressingRoomTotals(u_char arg0) {
    u8 charId;

    charId = g_Menu->pManager->currentCharacterIDs[arg0];
    func_801E3ECC(g_Menu->pDressingRoom, g_GameState.characters[charId].gearId);
    charId = g_Menu->pManager->currentCharacterIDs[arg0];
    MenuFillDressingRoomGearStats(g_Menu->pDressingRoom, g_GameState.characters[charId].gearId);
    g_Menu->pDressingRoom->unkB8[0] = g_Menu->pDressingRoom->unkB0;
    g_Menu->pDressingRoom->unkB8[1] = g_Menu->pDressingRoom->totalDefense;
    g_Menu->pDressingRoom->unkB8[2] = g_Menu->pDressingRoom->totalEtherDefense;
    g_Menu->pDressingRoom->unkB8[3] = g_Menu->pDressingRoom->totalResponsiveness;
    g_Menu->pDressingRoom->unkB8[4] = g_Menu->pDressingRoom->unkB3;
    g_Menu->pDressingRoom->unkB8[5] = g_Menu->pDressingRoom->unkB4;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DFF5C);
void MenuReturnWeaponToInventory(u_char arg0, u_char arg1) {
    u_char* pIds;
    u_char* pQuantities;
    u_char characterId;
    u_char gearId;
    u_char weaponId;
    u_char found = 1;
    u_char count;
    s32 i;

    if (arg1 == 0) {
        pIds = g_GameState.weaponIDs;
        characterId = g_Menu->pManager->currentCharacterIDs[arg0];
        weaponId = g_GameState.characters[characterId].unk6F[0];
        g_GameState.characters[characterId].unk6F[0] = 0;
        pQuantities = g_GameState.weaponQuantities;
        count = MAX_INVENTORY_WEAPONS;
    } else {
        pIds = g_GameState.gearWeaponsIds;
        characterId = g_Menu->pManager->currentCharacterIDs[arg0];
        gearId = g_GameState.characters[characterId].gearId;
        weaponId = g_GameState.gears[gearId].unk4[0];
        g_GameState.gears[gearId].unk4[0] = 0;
        pQuantities = g_GameState.gearWeaponsQty;
        count = MAX_INVENTORY_WEAPONS;
    }

    for (i = 0; i < count; i++) {
        if (pIds[i] == weaponId) {
            u_char quantity = pQuantities[i] + 1;

            pQuantities[i] = quantity;
            if (quantity >= MAX_INVENTORY_WEAPONS) {
                pQuantities[i] = MAX_ITEM_QUANTITY;
            }
            found = 0;
        }
    }

    if (found) {
        for (i = 0; i < count; i++) {
            if (pIds[i] == 0) {
                pIds[i] = weaponId;
                pQuantities[i] = 1;
                break;
            }
        }
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801E05D0);
