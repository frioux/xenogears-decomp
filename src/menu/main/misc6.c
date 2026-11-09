#include "common.h"

#include "system/memory.h"
#include "system/menu.h"

void func_801C7BF4(void);
void func_801C8574(u_char soundEffectId);
void func_801D397C(u_char a0, u_char a1, u_char a2, u_char a3, u16 a4, u_char a5, u_char a6, u32 a7, u_char a8);
void func_801D4EA0(u_char a0);
void func_801D5BA4(u_char a0, u_char a1);
void func_801D5CF8(u_char a0, u_char a1);
void func_801D7C3C(u_char a0, u_char a1);
void func_801D7CFC(u_char a0, u_char a1, u_char a2);
void func_801D8DE4(u_char a0, s32 a1, s32 a2, u_char a3);
void func_801D8EA4(u_char a0, s32 a1, s32 a2, u_char a3);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D1CA0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D1D40);
void func_801D1E80(void) {
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION_START;
    func_801C8574(0x5B);
}
void func_801D1EB0(void) {
    g_Menu->transitionEffectState = MENU_CLOSE_ANIMATION_START;
    func_801C8574(0x5C);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D1EE0);
void func_801D22C4(void) {
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D22F4);
void func_801D2484(void) {
    g_Menu->pManager->shouldRenderCursors = 0;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D249C);
void func_801D25E4(void) {
    int i;

    for (i = 0; i < 6; i++) {
        g_Menu->pManager->unk14[i] = 0;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D261C);
void func_801D28A8(void) {
    func_801D397C(0, 0xD4, 0xB2, 0x60, 0x10, 0, 0, 4, 0);
    func_801D5BA4(0xD8, 0xB6);
}
void func_801D28FC(void) {
    func_801D397C(1, 0xCC, 0xC6, 0x50, 0x10, 0, 0, 4, 0);
    func_801D5CF8(0xD0, 0xCA);
    // volatile keeps reorg from sinking this store past the epilogue restores
    *(volatile u8*)&g_Menu->pManager->unk5[1] = 1;
}
void func_801D2968(void) {
    if (g_Menu->pManager->unk5[1] != 0) {
        func_801D5CF8(0xD0, 0xCA);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D29A8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D2D38);
void func_801D2EC0(u_char a0, u_char a1) {
    func_801D7C3C(a0, a1);
    func_801D7CFC(a0, a1, g_GameState.gearRide[a0]);
    func_801C7BF4();
    func_801D8DE4(a0, 0, 0, a1);
    func_801C7BF4();
    func_801D8EA4(a0, 0, 0, a1);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D2F4C);
void func_801D32B4(void) {
    s32 i;

    if (g_Menu->pManager->shouldRenderWindow[2] != 0) {
        func_801D4EA0(2);
        g_Menu->pManager->unk2B = 0;
        for (i = 0; i < 4; i++) {
            HeapFree(g_Menu->unk1DE0[i]);
        }
    }
    func_801C7BF4();
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D3344);
void func_801D3444(void) {
    g_Menu->pManager->scrollHandleActive = 0;
    HeapFree(g_Menu->pScrollHandle);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D3488);
void func_801D3674(void) {
    if (g_Menu->pManager->unk5C[0xB] != 0) {
        g_Menu->pManager->shoulderButtonUiActive = 0;
        g_Menu->pManager->unk5C[0xB] = 0;
        HeapFree(g_Menu->pShoulderButtonUi);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D36E0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D397C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D3B00);
