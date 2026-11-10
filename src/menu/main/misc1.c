#include "common.h"

#include "system/menu.h"

extern u8 D_80059460; // Index of the menu overlay the menu was entered with
extern u_char D_801EA524[]; // String IDs of the menu strings

void func_801E7E68(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);

void func_801C531C(u_char arg0);
void func_801C7BF4(void);
void func_801C8694(u_char arg0);
s32 func_801CACF8(u_char a0, u_char a1, u_char a2);
void func_801D1E80(void);
void func_801D22F4(s32 arg0);
void func_801D2484(void);
void func_801D2D38(void);
void func_801C7B0C(void);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);

extern u8 D_801E977A;
extern u8 D_801EA8FC;
extern u8 D_801E96A4;
extern u8 D_801E96A5;

extern u8 D_80059171;
extern u8 D_800594D0;

INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C531C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C55A0);

void func_801C57A4(void) {
    u_char shouldReset;

    D_80059171 = 1;
    D_801E977A = 0;
    func_801D1E80();
    shouldReset = 1;
    while (g_Menu->transitionEffectState != 0) {
        func_801C7BF4();
    }
    func_801D22F4(0);
    if ((u_char)func_801CACF8(0x7D, 0xFF, 1) != 0 || D_801EA8FC != 0) {
        if (func_801CACF8(0x80, 0xFF, 1) != 0) {
            shouldReset = 0;
        }
    }
    func_801D2484();
    if (shouldReset) {
        D_801E96A5 = 1;
        D_801E96A4 = 1;
        func_801C531C(0);
        D_801E96A4 = 0;
        D_801E96A5 = 0;
    }
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    func_801C8694(D_80059171);
}
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

/* Best: 92.33% - only the placement of the two func_801C8694 call blocks differs: the
   original lays the outer test's then-block out of line (beqz $v0 -> call block) and
   falls through to the second test, then to a "j end".  Tried: else-if, `!=`/else
   (87.67%), 2-way switch with default (92.33%), early break + separate if (92.33%),
   nested if (92.33%), 2-way switch (92.33%).  Instruction multiset is identical; this is
   a bb-reorder tie-break.
void func_801C62A8(void) {
    u8 menuIndex;

    func_801C5F10();
    func_801C7B0C();
    g_Menu->shouldDrawMenu = 1;
    g_Menu->unk32A = 1;
    menuIndex = D_80059460;
    switch (menuIndex) {
        case 0:
            func_801D2D38();
            func_801C55A0();
            break;
        case 2:
            D_800594D0 = 0;
            func_801C58EC();
            g_Menu->pManager->shouldRenderSelectionMenu = 0;
            g_Menu->pManager->unk4 = 0;
            g_Menu->pManager->unk3 = 0;
            if (D_800594D0 == 0) {
                func_801C8694(0);
            } else if (D_800594D0 == menuIndex) {
                func_801C8694(((u8*)g_GameState.unk1930)[0xA4]);
            }
            break;
        case 6:
            func_801C57A4();
            break;
    }
    func_801C5FE4();
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6400);

/* Best: 90.45% (480 bytes, same size).  Remaining: gcc rotates the saved registers
   (pBuffer/i/lineCount are s2/s0/s1 here vs s0/s1/s2 in the original) and puts the
   newline block in-line where the original keeps it after a `j` back to the loop top.
   Tried: per-statement re-deref of g_Menu->menuUnk2 (needed, the original reloads),
   `while` vs `if (n) for(;;)` vs do-while loop shapes, explicit pointer for the
   0x4FFC copy loop (needed, the original strength-reduces), declaration permutations.
void func_801C6400(void) {
   u8* pBuffer;
   s32 i;
   s32 j;
   u16 lineCount;
   u8* p;
   MenuUnk2* pInfo;

   for (i = 0; i < 2; i++) {
       g_Menu->menuUnk2->unk4F84[4 + i] = 0;
       g_Menu->menuUnk2->unk4F84[6 + i] = 0;
       ((u8*)&g_Menu->menuUnk2->unk4F8C)[i] = 0xFF;
   }
   for (i = 0; i < 0x20; i++) {
       g_Menu->menuUnk2->records[i].unk58 = 0;
       g_Menu->menuUnk2->unk4FAE[i] = 0xFF;
   }
   lineCount = g_GameState.unk1930[0];
   i = 0;
   ArchiveSetIndex(0x10, 1);
   pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(1), 1);
   ArchiveReadFileToBuffer(1, pBuffer, 0, 0x80);
   ArchiveCdDataSync(0);
   if (lineCount != 0) {
       for (;;) {
           if (pBuffer[i] >= 0x80) {
               i += 2;
               continue;
           }
           if (pBuffer[i] == '\n') {
               lineCount += 0xFFFF;
               if (lineCount != 0) {
                   i++;
                   continue;
               }
               break;
           }
           i++;
       }
   }
   p = &pBuffer[i];
   for (j = 0; j < 0x1E; j++) {
       g_Menu->menuUnk2->unk4FE7[0x15 + j] = p[j];
   }
   pInfo = g_Menu->menuUnk2;
   pInfo->unk4FE7[0x34] = 0;
   pInfo->unk4FE7[0x33] = 0;
   ArchiveSetIndex(0x10, 0);
   HeapFree(pBuffer);
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C65F4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6AA0);

void func_801C6D4C(void) {
    g_Menu->renderContext = 0;
}

void func_801C6D5C(void) {
    g_Menu->unk4CC = 0;
    g_Menu->unk4D0 = 0;
    g_Menu->unk4D8 = 0;
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
void func_801C6E68(void) {
    POLY_FT4 _unusued;

    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_TOP,
        &g_Menu->texCount0,
        &g_Menu->texPage0,
        &g_Menu->clutX0, &g_Menu->clutY0,
        &g_Menu->texPageX0, &g_Menu->texPageY0
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_BOTTOM,
        &g_Menu->texCount1,
        &g_Menu->texPage1,
        &g_Menu->clutX1, &g_Menu->clutY1,
        &g_Menu->texPageX1, &g_Menu->texPageY1
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_LEFT,
        &g_Menu->texCount2,
        &g_Menu->texPage2,
        &g_Menu->clutX2, &g_Menu->clutY2,
        &g_Menu->texPageX2, &g_Menu->texPageY2
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_RIGHT,
        &g_Menu->texCount3,
        &g_Menu->texPage3,
        &g_Menu->clutX3, &g_Menu->clutY3,
        &g_Menu->texPageX3, &g_Menu->texPageY3
    );
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6F70);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C72BC);
