#include "common.h"

#include "system/memory.h"
#include "system/menu.h"

void func_801D4F2C(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D50EC(u_char a0, s32 a1, s32 a2);
void func_801D51EC(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D53D0(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D55B4(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D5794(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D5ED4(u_char a0, u_char a1);
void func_801D6194(u_char a0);
void func_801D6338(u_char a0, u_char a1);
void func_801D680C(u_char a0, u_char a1);
void func_801D6CF4(u_char a0, u_char a1);
void func_801D7154(u_char a0, u_char a1);
void func_801D74EC(u_char a0, u_char a1);
void func_801D7884(u_char a0, u_char a1);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D3C4C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D3DB0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D3FF8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D433C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D4688);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D49D0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D4D1C);
void func_801D4EA0(u_char a0) {
    g_Menu->pManager->shouldRenderWindow[a0] = 0;
    g_Menu->pManager->unk27[a0] = 0;
    HeapFree(g_Menu->windows[a0]);
    HeapFree(g_Menu->windowParameters[a0]);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D4F2C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D50EC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D51EC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D53D0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D55B4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D5794);
void func_801D5A50(u_char a0, u_char a1) {
    s32* pData;
    u8* pWindow;
    s32 x;
    s32 y;

    if (a0 == 0xFF) {
        return;
    }
    pData = (s32*)((u8*)g_Menu + a0 * 0x24);
    pWindow = (u8*)g_Menu->unk39C[a0];
    x = pData[6] / 256 + pData[0];
    y = pData[7] / 256 + pData[2];
    func_801D4F2C(a0, a1, x, y);
    func_801D50EC(a0, x, y);
    func_801D51EC(a0, a1, x, y);
    func_801D53D0(a0, a1, x, y);
    func_801D55B4(a0, a1, x, y);
    func_801D5794(a0, a1, x, y);
    g_Menu->pManager->unk0[a0] = 1;
    *(u8*)(pWindow + 0x1270) = *(u8*)&g_Menu->renderContext;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D5BA4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D5CF8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D5ED4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D6194);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D6338);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D680C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D6CF4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D7154);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D74EC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D7884);
void func_801D7C3C(u_char a0, u_char a1) {
    func_801D5ED4(a0, a1);
    func_801D6194(a1);
    func_801D6338(a0, a1);
    func_801D680C(a0, a1);
    func_801D6CF4(a0, a1);
    func_801D74EC(a0, a1);
    func_801D7884(a0, a1);
    func_801D7154(a0, a1);
    g_Menu->pManager->unk5[2] = 1;
    *(u8*)((u8*)g_Menu->unk358 + 0x2AE0) = *(u8*)&g_Menu->renderContext;
}
