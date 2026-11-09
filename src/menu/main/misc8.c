#include "common.h"

#include "system/menu.h"

extern u8 D_80059460; // Index of the menu overlay the menu was entered with
extern u16 D_801EA53C[]; // String IDs of the menu strings
extern u16 D_801EA542[]; // String IDs of the menu strings
extern u16 D_801EA548[]; // String IDs of the menu strings

void func_801D22F4(s32 arg0);
void func_801D3444(void);
void func_801D4EA0(s32 arg0);
void func_801C72BC(s32 arg0);
void func_801D7F50(s32 arg0, s32 arg1, u_char arg2);
void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
void func_801E8018(s32 count, void* pStrings, void* pStringIds, void* pFlags);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D7CFC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D7F50);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D827C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D83AC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D84B4);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D85DC);

/* 87.69%. Instruction multiset and frame (0x18) match; the only difference is
   that gcc emits `i++` after the conditional `max = *p` load, while the
   original has it before it (it ends up in the beqz delay slot). The u16 max
   plus the volatile pointers are what reproduce the double load of `*p`.
s32 func_801D85DC(s32 unusedArg, volatile u16* p0, volatile u16* p1) {
    SVECTOR unusedSlot[3];
    u16 max = 0;
    s32 i;

    for (i = 0; i < 7; i++, p0++) {
        if (max < *p0) {
            max = *p0;
        }
    }
    for (i = 0; i < 7; i++, p1++) {
        if (max < *p1) {
            max = *p1;
        }
    }

    return max;
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D8644);
void func_801D8DE4(s32 arg0, u_char arg1, s32 arg2, s32 arg3) {
    s32 width = 0x98;
    s32 height = 0x26;
    u_char value;

    if (arg1 != 0) {
        width = 0x80;
        height = 0x90;
    }
    value = arg3;
    func_801D7F50(width, height, value);
    func_801D8644(arg0, width, height, arg2, value);
    g_Menu->pManager->unk8 = 1;
    g_Menu->unk35C->unk32F1 = *(u_char*)&g_Menu->renderContext;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D8EA4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9704);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9808);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9B08);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9C84);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9E3C);

void func_801D9F34(void) {
    if (D_80059460 != 2) {
        func_801E8018(6, g_Menu->unkDE0, D_801EA53C, g_Menu->pManager->unk1A);
    } else {
        func_801E8018(6, g_Menu->unkDE0, D_801EA542, g_Menu->pManager->unk1A);
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9F98);

void func_801DA4A8(void) {
    MenuUnk9* pMenu;

    func_801D22F4(2);
    func_801E8018(8, g_Menu->unk0x10E0, D_801EA548, g_Menu->pManager->unk38);
    pMenu = HeapAlloc(0x1198, 0);
    g_Menu->unk42C = pMenu;
    bzero(pMenu, 0x1198);
    func_801C72BC(0);
}

void func_801DA518(void) {
    func_801D3444();
    func_801D4EA0(3);
    func_801D4EA0(4);
    g_Menu->pManager->unk48 = 0;
    func_801C72BC(0x10);
    HeapFree(g_Menu->unk42C->pVramBuffer);
    HeapFree(g_Menu->unk42C);
    HeapFree(g_Menu->pDressingRoom->pItemsData);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DA5BC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DA9A8);

void func_801DB02C(u_char index) {
    g_Menu->arrowCursors[index] = HeapAlloc(0x78, 0);
    bzero(g_Menu->arrowCursors[index], 0x78);
    g_Menu->arrowCursors[index]->curAnimFrame = 4;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DB0A8);

void func_801DB340(u_char index) {
    HeapFree(g_Menu->arrowCursors[index]);
    g_Menu->pManager->shouldRenderArrowCursor[index] = 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DB39C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DB5E4);
