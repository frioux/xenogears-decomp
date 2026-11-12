#include "common.h"

#include "main/game.h"
#include "system/memory.h"
#include "system/menu.h"

void func_801C80B8(u32 number);
void func_801D3C4C(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void func_801D3DB0(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void func_801D3FF8(u_char a0, u_short a1, u_short a2, u_short a3);
void func_801D433C(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void func_801D4688(u_char a0, u_short a1, u_short a2, u_short a3);
void func_801D49D0(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void func_801D4F2C(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D50EC(u_char a0, s32 a1, s32 a2);
void func_801D51EC(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D53D0(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D55B4(u_char a0, u_char a1, s32 a2, s32 a3);
void func_801D5794(u_char a0, u_char a1, s32 a2, s32 a3);
extern s32 D_801E9A78[];
extern s32 D_801E9AC8[];
extern s32 D_801E9B60[];
extern s32 D_801E9C20[];
extern s32 D_801EA34C[];
extern s32 D_801EA39C[];
extern s32 D_801E9B28;
extern s32 D_801E9B38;
extern s32 D_801E9B3C;
extern s32 D_801E9B40;
extern s32 D_801E9B44;
extern s32 D_801E9B48;
extern s32 D_801E9B4C;
extern s32 D_801E9B50;
extern s32 D_801E9B54;
extern s32 D_801E9B2C;
extern s32 D_801E9B30;
extern s32 D_801E9B34;
extern u16 D_801E9B58;
extern u16 D_801E9B5C;
extern s32 D_801EA578[];
extern u8 D_801EA5C4[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
void func_801E920C(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);
void func_801E927C(POLY_FT4* pPoly);

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
   func_800263E4 (87.70%). Also gcc-2.7.2-cdk: 93.33 (and the matched
   func_801D55B4 drops to 82.14 under cdk, so this is not a cdk unit). The tie
   is priority(0x1000) > priority(height) in retail, the reverse in 2.7.2; both
   are live across the same two calls with 3 uses each.
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
void func_801E91C4(P_TAG* pTag);
void func_801D3DB0(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[index];
    u16 left;
    u16 top;
    u16 right;
    u16 bottom;
    s32 i;

    pWindow->unk710 = 0;
    pWindow->unk710 = func_8002675C(g_Menu->resources, 0xFD,
                                    pWindow->polysWindowBorderCorners,
                                    g_Menu->renderContext, 0, 0, 0x1000)
                    + pWindow->unk710;
    pWindow->unk710 = func_8002675C(g_Menu->resources, 0xFF,
                                    (POLY_FT4*)((u8*)pWindow + pWindow->unk710 * 0x50),
                                    g_Menu->renderContext, 0, 0, 0x1000)
                    + pWindow->unk710;
    pWindow->unk710 = func_8002675C(g_Menu->resources, 0x102,
                                    (POLY_FT4*)((u8*)pWindow + pWindow->unk710 * 0x50),
                                    g_Menu->renderContext, 0, 0, 0x1000)
                    + pWindow->unk710;
    pWindow->unk710 = func_8002675C(g_Menu->resources, 0x104,
                                    (POLY_FT4*)((u8*)pWindow + pWindow->unk710 * 0x50),
                                    g_Menu->renderContext, 0, 0, 0x1000)
                    + pWindow->unk710;
    left = x - 8;
    top = y + 8;
    func_801C851C(pWindow->vertsWindowBorderCorners, left, top, 0x10, 0xFFF0);
    right = x + width + 8;
    func_801C851C(&pWindow->vertsWindowBorderCorners[4], right, top, 0xFFF0, 0xFFF0);
    bottom = y + height - 8;
    func_801C851C(&pWindow->vertsWindowBorderCorners[8], left, bottom, 0x10, 0x10);
    func_801C851C(&pWindow->vertsWindowBorderCorners[12], right, bottom, 0xFFF0, 0x10);
    for (i = 0; i < 4; i++) {
        func_801E91C4((P_TAG*)&pWindow->polysWindowBorderCorners[i * 2 + g_Menu->renderContext]);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D3FF8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D433C);
/* Best: 99.00%. The sibling of func_801D3DB0 (bottom border instead of the
   corners) and the decode is complete: 16 UV stores to polysWindowBorderBottom
   [ctx] and [ctx+2], two func_801C851C quads, then a 2-iteration
   func_801E91C4 loop. Three codegen facts already worked out here: the UV
   stores must be byte offsets off a raw pointer with the context product FIRST
   (`g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x1EC`); a struct access
   (`pWindow->polysWindowBorderBottom[ctx].u0`) gives the reversed addu and an
   `addiu v1,v1,2` for the [ctx+2] group instead of retail's +0x50
   displacement; and the second quad's x argument must be written
   `halfWidth + 8 + x`, the only order that gives retail's `addiu s0,s0,8` /
   `addu s4,s4,s0` and the s4=x / s3=(u16)halfWidth assignment.
   One instruction left: retail computes the second call's address
   (`addiu a0,s6,0x5F0`) AFTER the x sum, gcc hoists it before it.
   void func_801D433C(u_char index, u_short x, u_short y, u_short width, u_short height) {
       MenuWindow* pWindow = g_Menu->windows[index];
       u16 bottom;
       s32 halfWidth;
       s32 i;

       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x1EC) = 8;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x1ED) = 0x84;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x1F4) = 0xF;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x1F5) = 0x84;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x1FC) = 8;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x1FD) = 0x94;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x204) = 0xF;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x205) = 0x94;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x23C) = 8;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x23D) = 0x84;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x244) = 0xF;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x245) = 0x84;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x24C) = 8;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x24D) = 0x94;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x254) = 0xF;
       *(u8*)(g_Menu->renderContext * 0x28 + (u8*)pWindow + 0x255) = 0x94;
       bottom = y + height - 8;
       halfWidth = (width - 0x10) / 2;
       func_801C851C(pWindow->vertsWindowBorderBottom1, x + 8, bottom, halfWidth, 0x10);
       func_801C851C(pWindow->vertsWindowBorderBottom2, halfWidth + 8 + x, bottom, halfWidth, 0x10);
       for (i = 0; i < 2; i++) {
           func_801E91C4((P_TAG*)&pWindow->polysWindowBorderBottom[i * 2 + g_Menu->renderContext]);
       }
   }
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D4688);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D49D0);
void func_801D4D1C(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, s32 a6, u_char a7) {
    MenuWindow* pWindow = g_Menu->windows[a0];

    g_Menu->pManager->shouldRenderWindow[a0] = 0;
    func_801C851C(pWindow->vertsBackground, a1, a2, a3, a4);
    func_801D3DB0(a0, a1, a2, a3, a4);
    func_801D3FF8(a0, a1, a2, a3);
    func_801D433C(a0, a1, a2, a3, a4);
    func_801D4688(a0, a1, a2, a4);
    func_801D49D0(a0, a1, a2, a3, a4);
    if (a7 != 0) {
        func_801D3C4C(a0, a1, a2, a3, a4);
    }
    pWindow->hasScrollBar = a7;
    pWindow->unk714 = a5;
    pWindow->zIndex = a6;
    pWindow->renderContext = *(u8*)&g_Menu->renderContext;
    g_Menu->pManager->shouldRenderWindow[a0] = 1;
}
void func_801D4EA0(u_char a0) {
    g_Menu->pManager->shouldRenderWindow[a0] = 0;
    g_Menu->pManager->unk27[a0] = 0;
    HeapFree(g_Menu->windows[a0]);
    HeapFree(g_Menu->windowParameters[a0]);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D4F2C);
/* Best: 87.79%. Structure and every called function/prototype is right; what is
   left is allocation and instruction order: (1) retail emits `addu v1,s1,v1`
   for the tpage-store address where gcc commutes it to `addu v1,v1,s1`, (2)
   retail keeps a0 in s3 / a3 in s4, gcc the reverse, (3) in the func_801E920C
   arg setup retail does `andi a3,a3,0xFC` after the a1/a2 masks and keeps the
   7th stack store (`sw v0,0x18(sp)`) before the jal, gcc puts it in the delay
   slot. func_801E920C must be declared `u_short x, u_short y` HERE (misc12.c
   defines it as `short`): with `short` the caller gets sll/sra 0x10 instead of
   retail's `andi 0xFFFF`.
   void func_801D4F2C(u_char a0, u_char a1, s32 a2, s32 a3) {
       u8* pWindow = (u8*)g_Menu->unk39C[a0];
       u8* pBase;
   
       func_8002675C(g_Menu->resources, a0 + 0x14B, (POLY_FT4*)pWindow,
                     g_Menu->renderContext, a2, a3, 0x1000);
       func_801E927C((POLY_FT4*)(pWindow + (g_Menu->renderContext * 0x28 + 0x50)));
       *(u16*)(pWindow + g_Menu->renderContext * 0x28 + 0x66) = GetTPage(0, 0, 0x180, 0);
       pBase = pWindow + g_Menu->renderContext * 0x28;
       if (g_Menu->pManager->currentCharacterIDs[a0] & 1) {
           *(u16*)(pBase + 0x5E) = g_SystemPalette2;
       } else {
           *(u16*)(pBase + 0x5E) = g_SystemPalette1;
       }
       func_801E920C((POLY_FT4*)(pWindow + (g_Menu->renderContext * 0x28 + 0x50)),
                     D_801E9B58 + a2, D_801E9B5C + a3,
                     (D_801EA578[a0] << 2) & 0xFC, D_801EA5C4[a0], 0x48, 0xD);
   }
*/
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
/* Best: 98.31%, frame 0x38, a1 and n share s2 as in retail. The one remaining
   diff is loop 2's `i++`: retail has `addiu s1,s1,1` in the `beq
   digits[6+i]==0xFF` delay slot with `ori v0,0x1000` first in the body, gcc
   puts the `li 0x1000` in the slot and `addiu s1,s1,1` in the latch. That is
   reorg's delay-slot choice, not allocation (both sides are 122 insns).
   The key discovery is the statement order after loop 1: gcc's loop-2 preheader
   hoists the `n = 0` biv init to the TOP of the fall-through block, so reorg
   steals it into loop 1's back-branch slot and a1 has to keep a register of its
   own (frame 0x40). Writing `i = 0;` as a statement BEFORE the second
   func_801C80B8 call (and `n = 0` after the counter store) makes `i = 0` the
   block's first insn - reorg rejects it because s1 is live at loop 1's head -
   so reorg takes the `andi v1,s2,0xFF` of the second characters[] chain instead,
   exactly like retail, and a1 then dies before n is born. Re-read since: retail
   keeps `addiu s1,s1,1` in the latch for loop 1 but has it in the `beq` delay
   slot with no latch copy for loop 2 - i.e. in retail's RTL the loop-2
   increment sits at the top of the body block. No for/while/continue/
   digits[i++] shape gives that (a `while (i < 3)` puts the test at the head,
   where retail's head starts at the `g_Menu` load), and cdk does not change it,
   so this is an RTL rotation, not a delay-slot choice. Tried: `continue`
   form, n=0 before the call / after the sb, i=0 only in the for-init, `i < 3`
   with the increment in the latch: all 94.45-98.31%.
   void func_801D51EC(u_char a0, u_char a1, s32 a2, s32 a3) {
       u8* pWindow = (u8*)g_Menu->unk39C[a0];
       s32 i;
       s32 n;

       func_801C80B8(g_GameState.characters[a1].hp);
       pWindow[0x1273] = 0;
       for (i = 0; i < 3; i++) {
           if (g_Menu->digits[6 + i] != 0xFF) {
               pWindow[0x1273] += func_8002675C(
                   g_Menu->resources, g_Menu->digits[6 + i],
                   (POLY_FT4*)((u8*)pWindow + (pWindow[0x1273] * 0x50 + 0xAF0)),
                   g_Menu->renderContext, a2 + D_801E9B28 + i * 8,
                   a3 + D_801E9B2C, 0x1000);
           }
       }
       i = 0;
       func_801C80B8(g_GameState.characters[a1].maxHp);
       pWindow[0x1274] = 0;
       n = 0;
       for (; i < 3; i++) {
           if (g_Menu->digits[6 + i] != 0xFF) {
               pWindow[0x1274] += func_8002675C(
                   g_Menu->resources, g_Menu->digits[6 + i],
                   (POLY_FT4*)((u8*)pWindow + (pWindow[0x1274] * 0x50 + 0xBE0)),
                   g_Menu->renderContext, a2 + D_801E9B30 + n * 8,
                   a3 + D_801E9B34, 0x1000);
               n++;
           }
       }
   }
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D53D0);
/* Best: 98.31%. Same shape as func_801D51EC, which see: `i = 0;` written
   before the second func_801C80B8 call and `n = 0;` after the counter store
   reproduces retail's loop-1 exit block (`andi v1,s2,0xFF` in the back-branch
   delay slot, `addu s2,zero,zero` in the jal delay slot) and drops the extra
   callee-saved register, frame 0x38. The one remaining diff is loop 2's
   `i++`: retail keeps `addiu s1,s1,1` in the `beq digits[7+i]==0xFF` delay
   slot, gcc puts it in the latch (both sides 122 insns, so reorg's slot
   choice).
   void func_801D53D0(u_char a0, u_char a1, s32 a2, s32 a3) {
       u8* pWindow = (u8*)g_Menu->unk39C[a0];
       s32 i;
       s32 n;

       func_801C80B8(g_GameState.characters[a1].mp);
       pWindow[0x1275] = 0;
       for (i = 0; i < 2; i++) {
           if (g_Menu->digits[7 + i] != 0xFF) {
               pWindow[0x1275] += func_8002675C(
                   g_Menu->resources, g_Menu->digits[7 + i],
                   (POLY_FT4*)((u8*)pWindow + (pWindow[0x1275] * 0x50 + 0xCD0)),
                   g_Menu->renderContext, a2 + D_801E9B38 + i * 8,
                   a3 + D_801E9B3C, 0x1000);
           }
       }
       i = 0;
       func_801C80B8(g_GameState.characters[a1].maxMp);
       pWindow[0x1276] = 0;
       n = 0;
       for (; i < 2; i++) {
           if (g_Menu->digits[7 + i] != 0xFF) {
               pWindow[0x1276] += func_8002675C(
                   g_Menu->resources, g_Menu->digits[7 + i],
                   (POLY_FT4*)((u8*)pWindow + (pWindow[0x1276] * 0x50 + 0xD70)),
                   g_Menu->renderContext, a2 + D_801E9B40 + n * 8,
                   a3 + D_801E9B44, 0x1000);
               n++;
           }
       }
   }
*/
void func_801D55B4(u_char a0, u_char a1, s32 a2, s32 a3) {
    u8* pWindow = (u8*)g_Menu->unk39C[a0];
    s32 i;

    func_801C80B8(*(s32*)((u8*)&g_GameState.characters[a1] + 0x44));
    pWindow[0x1277] = 0;
    for (i = 0; i < 7; i++) {
        if (g_Menu->digits[2 + i] != 0xFF) {
            pWindow[0x1277] += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                (POLY_FT4*)((u8*)pWindow + (pWindow[0x1277] * 0x50 + 0xE10)),
                g_Menu->renderContext, a2 + D_801E9B48 + i * 8,
                a3 + D_801E9B4C, 0x1000);
        }
    }
    func_801C80B8(*(s32*)((u8*)&g_GameState.characters[a1] + 0x48));
    pWindow[0x1278] = 0;
    for (i = 0; i < 7; i++) {
        if (g_Menu->digits[2 + i] != 0xFF) {
            pWindow[0x1278] += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                (POLY_FT4*)((u8*)pWindow + (pWindow[0x1278] * 0x50 + 0x1040)),
                g_Menu->renderContext, a2 + D_801E9B50 + i * 8,
                a3 + D_801E9B54, 0x1000);
        }
    }
}
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
void func_801D5CF8(s32 a0, s32 a1) {
    s32 i;

    for (i = 0; i < 3; i++) {
        func_8002675C(g_Menu->resources, ((s32*)g_Menu->unk2EC)[i],
                      (POLY_FT4*)(i * 0x50 + (s32)g_Menu->unk344),
                      g_Menu->renderContext, a0 + i * 8, a1, 0x1000);
    }
    for (i = 3; i < 5; i++) {
        func_8002675C(g_Menu->resources, ((s32*)g_Menu->unk2EC)[i],
                      (POLY_FT4*)(i * 0x50 + (s32)g_Menu->unk344),
                      g_Menu->renderContext, a0 + i * 8 + 8, a1, 0x1000);
    }
    for (i = 5; i < 7; i++) {
        func_8002675C(g_Menu->resources, ((s32*)g_Menu->unk2EC)[i],
                      (POLY_FT4*)(i * 0x50 + (s32)g_Menu->unk344),
                      g_Menu->renderContext, a0 + i * 8 + 0x10, a1, 0x1000);
    }
    func_8002675C(g_Menu->resources, 0xEE,
                  (POLY_FT4*)((u8*)g_Menu->unk344 + 0x230),
                  g_Menu->renderContext, a0 + 0x18, a1, 0x1000);
    func_8002675C(g_Menu->resources, 0xEE,
                  (POLY_FT4*)((u8*)g_Menu->unk344 + 0x2D0),
                  g_Menu->renderContext, a0 + 0x30, a1, 0x1000);
    *(u8*)((u8*)g_Menu->unk344 + 0x370) = *(u8*)&g_Menu->renderContext;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D5ED4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D6194);
/* Best: 93.40%. Two diffs left. (1) retail has `addiu s0,s0,1` in the delay
   slot of the `digit == 0xFFFF` beq and then does the render; gcc puts the
   increment at the end of the body - the same delay-slot-fill puzzle as the
   func_801D5BA4 note. (2) second loop: retail loads lhu 0xA8 twice (a1 for the
   arg, v0 for the width subtraction) and loads x/y straight into a1/a2; gcc
   CSEs the twin lhu and then emits `move a1,v1`/`move a2,t0`.
   Tried: `if (== 0xFFFF) continue;` form (same), outer `if (count > 0)` guard
   around loop 2 (extra beqz+nop - a bare `for` gives retail's `blez`).
   Symbols added: D_801E9B60, D_801E9C20, D_801EA39C (all live in
   asm/menu/data/246A4.data.s).
   void func_801D6194(u_char a0) {
       s32 i;
       u8* pEntry;
   
       ((u8*)g_Menu->unk358)[0x2AEC] = 0;
       for (i = 0; i < 0x18; i++) {
           if (D_801EA39C[a0 * 0x18 + i] == 0xFFFF) continue;
           {
               ((u8*)g_Menu->unk358)[0x2AEC] += func_8002675C(
                   g_Menu->resources,
                   D_801EA39C[a0 * 0x18 + i],
                   (POLY_FT4*)(((u8*)g_Menu->unk358) +
                               (((u8*)g_Menu->unk358)[0x2AEC] * 0x50 + 0xA0)),
                   g_Menu->renderContext,
                   D_801E9B60[a0 * 0x18 + i],
                   D_801E9C20[a0 * 0x18 + i],
                   0x1000);
           }
       }
       {
           for (i = 0; i < ((u8*)g_Menu->unk358)[0x2AEC]; i++) {
               pEntry = (u8*)g_Menu->unk358 + (g_Menu->renderContext + i * 2) * 0x28;
               func_801C851C((SVECTOR*)((u8*)g_Menu->unk358 + 0x1EE0 + i * 0x20),
                             *(u16*)(pEntry + 0xA8),
                             *(u16*)(pEntry + 0xAA),
                             *(u16*)(pEntry + 0xB0) - *(u16*)(pEntry + 0xA8),
                             *(u16*)(pEntry + 0xC2) - *(u16*)(pEntry + 0xAA));
           }
       }
   }
*/
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
