#include "common.h"

#include "system/menu.h"

extern u8 D_80059460; // Index of the menu overlay the menu was entered with
extern u_char D_801EA524[]; // String IDs of the menu strings

void func_801E7E68(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C531C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C55A0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C57A4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C58EC);

void func_801C5B54(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->menuUnk2 = HeapAlloc(sizeof(MenuUnk2), 0);
        bzero(g_Menu->menuUnk2, sizeof(MenuUnk2));
        return;
    }
    HeapFree(g_Menu->menuUnk2);
}

void func_801C5BB8(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pManager = HeapAlloc(sizeof(MenuManager), 0);
        bzero(g_Menu->pManager, sizeof(MenuManager));
        return;
    }
    HeapFree(g_Menu->pManager);
}

void func_801C5C1C(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pSelectionMenu = HeapAlloc(sizeof(MenuSelectionMenu), 0);
        bzero(g_Menu->pSelectionMenu, sizeof(MenuSelectionMenu));
        return;
    }
    HeapFree(g_Menu->pSelectionMenu);
}

void func_801C5C80(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk354 = HeapAlloc(sizeof(MenuUnk5), 0);
        bzero(g_Menu->unk354, sizeof(MenuUnk5));
        return;
    }
    HeapFree(g_Menu->unk354);
}

void func_801C5CE4(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pDressingRoom = HeapAlloc(sizeof(MenuDressingRoom), 0);
        bzero(g_Menu->pDressingRoom, sizeof(MenuDressingRoom));
        return;
    }
    HeapFree(g_Menu->pDressingRoom);
}

void func_801C5D48(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk340 = HeapAlloc(0x328, 0);
        bzero(g_Menu->unk340, 0x328);
        return;
    }
    HeapFree(g_Menu->unk340);
}

void func_801C5DAC(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk344 = HeapAlloc(0x374, 0);
        bzero(g_Menu->unk344, 0x374);
        return;
    }
    HeapFree(g_Menu->unk344);
}

void func_801C5E10(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk348 = HeapAlloc(sizeof(MenuUnk1), 0);
        bzero(g_Menu->unk348, sizeof(MenuUnk1));
        return;
    }
    HeapFree(g_Menu->unk348);
}

void func_801C5E74(u_char isInitialization) {
    s32 i;

    if (isInitialization) {
        for (i = 0; i < 3; i++) {
            g_Menu->unk39C[i] = HeapAlloc(0x127C, 0);
            bzero(g_Menu->unk39C[i], 0x127C);
        }
        return;
    }
    for (i = 0; i < 3; i++) {
        HeapFree(g_Menu->unk39C[i]);
    }
}

void func_801C5F10(void) {
    func_801C5BB8(1);
    func_801C5C1C(1);
    func_801C5C80(1);
    func_801C5CE4(1);
    func_801C5E10(1);
    g_Menu->pCursors = HeapAlloc(sizeof(MenuPointerCursors), 0);
    bzero(g_Menu->pCursors, sizeof(MenuPointerCursors));
    switch (D_80059460) {
        case 0:
            func_801C5B54(1);
            func_801C5D48(1);
            func_801C5DAC(1);
            func_801C5E74(1);
            break;
        case 2:
        case 6:
            func_801C5B54(1);
            break;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C5FE4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C62A8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6400);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C65F4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6AA0);

void func_801C6D4C(void) {
    g_Menu->renderContext = 0;
}

void func_801C6D5C(void) {
    g_Menu->unk4D8 = 0;
    g_Menu->unk4CC = 0;
    g_Menu->unk4D0 = 0;
    g_Menu->unk4D9 = 0;
    g_Menu->unk4D4 = 0;
}

void func_801C6D90(void) {
    RECT rect;
    u_short* pVramData;
    u8 _unused[0x8];

    pVramData = HeapAlloc(0x20, 0);
    bzero(pVramData, 0x20);
    pVramData[1] = 0x7FFF;
    rect.y = 448;
    rect.w = 16;
    rect.x = 0;
    rect.h = 1;
    LoadImage(&rect, pVramData);
    DrawSync(0);
    HeapFree(pVramData);
}

void func_801C6E0C(void) {
    SystemTransferPaletteToVRAM(0, 0x1D1);
    g_Menu->unk4E0[0].pVramBuffer = HeapAlloc(0x38E, 0);
    func_801E7E68(g_Menu->unk4E0, D_801EA524, 0, 4);
    func_801C6D90();
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6E68);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6F70);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C72BC);
