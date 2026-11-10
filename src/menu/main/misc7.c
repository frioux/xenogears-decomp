#include "common.h"

#include "main/game.h"
#include "system/memory.h"
#include "system/menu.h"

void func_801C80B8(u32 number);
void func_801D4F2C(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D50EC(u_char a0, s32 a1, s32 a2);
void func_801D51EC(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D53D0(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D55B4(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D5794(u_char a0, u_char a1, s32 a2, s32 a3);
extern s32 D_801E9A78[];
extern s32 D_801E9AC8[];
extern s32 D_801EA34C[];

void func_801C851C(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
s32 func_800263E4(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale, s32 mirrorX, s32 mirrorY);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void func_801D5ED4(u_char a0, u_char a1);
void func_801D6194(u_char a0);
void func_801D6338(u_char a0, u_char a1);
void func_801D680C(u_char a0, u_char a1);
void func_801D6CF4(u_char a0, u_char a1);
void func_801D7154(u_char a0, u_char a1);
void func_801D74EC(u_char a0, u_char a1);
void func_801D7884(u_char a0, u_char a1);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D3C4C);
/* Best: 94.00%. The sibling ShopMenuInitializeScrollBar
   (src/shop_menu/main/misc.c) is this exact function and matches, so the C is
   right; the whole difference is s0<->s1: retail puts the 5th param `height`
   in s1 and the 0x1000 scale in s0, gcc does the reverse. Note func_800263E4
   must be declared with wide (s32) x/y/scale - a u16 there adds an andi 0xFFFF
   after the -8 and drops to 87%. Tried: s32 scale local, u16 x/y/scale for
   func_800263E4 (87.70%).
   void func_801D3C4C(u_char index, u_short x, u_short y, u_short width, u_short height) {
       MenuWindow* pWindow = g_Menu->windows[index];

       func_8002675C(g_Menu->resources, MENU_TEX_SCROLL_BAR_ORNAMENT,
                     pWindow->polysScrollBarEnds, g_Menu->renderContext, x, y, 0x1000);
       func_800263E4(g_Menu->resources, MENU_TEX_SCROLL_BAR_ORNAMENT,
                     &pWindow->polysScrollBarEnds[2], g_Menu->renderContext,
                     x, y + height - 8, 0x1000, 0, 1);
       func_8002675C(g_Menu->resources, MENU_TEX_SCROLL_BAR_EMPTY,
                     pWindow->polysScrollBarEmpty, g_Menu->renderContext, x, y + 8, 0x1000);
       func_801C851C(pWindow->vertsScrollBarEnds, x, y, 8, 8);
       func_801C851C(&pWindow->vertsScrollBarEnds[4], x, y + height, 8, -8);
       func_801C851C(pWindow->vertsScrollBarEmpty, x, y + 8, 8, height - 8);
   }
*/
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
void func_801D50EC(u_char a0, s32 a1, s32 a2) {
    u8* pWindow;
    s32 i;

    pWindow = (u8*)g_Menu->unk39C[a0];
    pWindow[0x1279] = 0;
    for (i = 0; i < 0x14; i++) {
        if (D_801EA34C[i] != 0xFFFF) {
            pWindow[0x1279] += func_8002675C(
                g_Menu->resources,
                D_801EA34C[i],
                (POLY_FT4*)(pWindow + (pWindow[0x1279] * 0x50 + 0xA0)),
                g_Menu->renderContext,
                a1 + D_801E9A78[i],
                a2 + D_801E9AC8[i],
                0x1000);
        }
    }
}
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
/* Best: 96.88%. Only the delay-slot fill differs: retail puts the loop
   increment `addiu s0,s0,1` in the `digits[i] != 0xFF` branch delay slot
   and keeps `li v0,0x1000` before the arg stores; gcc picks the other
   order. Tried: += then func()+count (addu order), `continue` form,
   digits[i++] with (i-1)*8, running x local with x += 8 (96.65%).
   void func_801D5BA4(s32 a0, s32 a1) {
       s32 i;

       func_801C80B8(g_GameState.gold);
       *(s32*)((u8*)g_Menu->unk340 + 0x320) = 0;
       for (i = 0; i < 9; i++) {
           if (g_Menu->digits[i] != 0xFF) {
               *(s32*)((u8*)g_Menu->unk340 + 0x320) =
                   func_8002675C(
                       g_Menu->resources,
                       g_Menu->digits[i],
                       (POLY_FT4*)(*(s32*)((u8*)g_Menu->unk340 + 0x320) * 0x50 +
                                   (u32)g_Menu->unk340),
                       g_Menu->renderContext,
                       a0 + i * 8,
                       a1,
                       0x1000) +
                   *(s32*)((u8*)g_Menu->unk340 + 0x320);
           }
       }
       func_8002675C(g_Menu->resources, 0x10,
                     (POLY_FT4*)((u8*)g_Menu->unk340 + 0x2D0),
                     g_Menu->renderContext, a0 + 0x50, a1, 0x1000);
       g_Menu->pManager->unk5[0] = 1;
       *(u8*)((u8*)g_Menu->unk340 + 0x324) = *(u8*)&g_Menu->renderContext;
   }
*/
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
