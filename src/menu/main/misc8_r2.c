#include "common.h"

#include "psyq/libapi.h"
#include "system/menu.h"
#include "psyq/libcd.h"

extern u8 g_MenuOverlayIndex; // Index of the menu overlay the menu was entered with
extern u8 g_ShopIndex;
extern u16 D_801EA53C[]; // String IDs of the menu strings
extern u16 D_801EA542[]; // String IDs of the menu strings
extern u16 D_801EA548[]; // String IDs of the menu strings
extern s32 D_801EA900;
extern s32 D_801EA904;
extern s32 D_801EA6FC;
extern s32 D_801EA700;
extern s32 D_801EA704;
extern s32 D_801EA708;
extern s32 D_801EA70C;
extern u8 D_801EA710;
extern u8 D_801EA714;
extern CdlCB D_801EA718;
extern CdlCB D_801EA71C;
extern CdlCB D_801EA720;
extern u8 D_801EA574[];
extern u8 g_MenuNameTextureX[];
extern u8 D_801E9F88[];
extern s32 D_801E977C[]; // String IDs of the menu scroll bar textures

void MenuSetupPointerCursors(s32 arg0);
void MenuFreeScrollHandle(void);
void MenuFreeWindow(s32 arg0);
void func_801C72BC(s32 arg0);
void func_801D7F50(s32 arg0, s32 arg1, u_char arg2);
void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
void MenuLoadStringsFromIds(s32 count, void* pStrings, void* pStringIds, void* pFlags);
void MenuSetRowQuadVertices(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u8 arg5, u8 arg6, u8 arg7);
void func_801E86C8(s32 arg0);
void MenuSetSubMenuChoicePrims(s32 arg0);
void MenuClearBytes(s32 arg0, void* pArg1);
s32 SoundGetVolumeMode(void);
void func_800386C4(s32 arg0);
void MenuClearGlyphTexture(void);
void MenuFreeCellPrimBlocks(void);
void MenuFreePrimBuffer(void);
void Vsync(s32 mode);
void MenuCloseCardEvents(void);
void MenuRenderFrame(void);
u_char func_801C93A8(void);
void func_801D2F4C(s32 arg0);
void MenuFreeWindow2Strings(void);
void MenuSetPrimShadingMode(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void MenuSetPolySemiTransGrey(P_TAG* pTag);
void func_801E7C50(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);
void* GetItemName(int entryId);
s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);

void MenuShutdown(void) {
    s32 i;

    g_Menu->cardSlotState = 0;
    MenuClearGlyphTexture();
    MenuFreeCellPrimBlocks();
    MenuFreePrimBuffer();
    for (i = 0; i < 0x20; i++) {
        g_Menu->menuUnk2->records[i].unk58 = 0;
    }
    g_Menu->menuUnk2->unk4F8C = 0xFF;
    g_Menu->menuUnk2->unk4F8D = 0xFF;
    D_801EA904 = 0;
    D_801EA900 = 0;
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    CdSyncCallback(D_801EA718);
    CdReadyCallback(D_801EA71C);
    CdReadCallback(D_801EA720);
    ExitCriticalSection();
}

void MenuLoadUnkDE0Strings(void) {
    if (g_MenuOverlayIndex != 2) {
        MenuLoadStringsFromIds(6, g_Menu->unkDE0, D_801EA53C, g_Menu->pManager->shouldRenderStringsDE0);
    } else {
        MenuLoadStringsFromIds(6, g_Menu->unkDE0, D_801EA542, g_Menu->pManager->shouldRenderStringsDE0);
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801D9F98);

void MenuInitializeUnk42CPanel(void) {
    MenuUnk9* pMenu;

    MenuSetupPointerCursors(2);
    MenuLoadStringsFromIds(8, g_Menu->unk10E0, D_801EA548, g_Menu->pManager->shouldRenderStrings10E0);
    pMenu = HeapAlloc(0x1198, 0);
    g_Menu->pItemRows = pMenu;
    bzero(pMenu, 0x1198);
    func_801C72BC(0);
}

void MenuFreeUnk42CPanel(void) {
    MenuFreeScrollHandle();
    MenuFreeWindow(3);
    MenuFreeWindow(4);
    g_Menu->pManager->shouldRenderUnk42C = 0;
    func_801C72BC(0x10);
    HeapFree(g_Menu->pItemRows->pVramBuffer);
    HeapFree(g_Menu->pItemRows);
    HeapFree(g_Menu->pDressingRoom->pItemsData);
}

/* 91.33% (1028 vs 1012 bytes). Decode of the whole body is settled; every call,
   store and loop is in retail's order. Remaining deltas, all small:
   1) block order: retail lays the two failure paths out AFTER the main body and
      merges them at one `unk1184[i] = 0`; this C makes gcc emit the fail path
      inline with `bnez` and jump over it. The source shape is the early-continue
      form below, which is logically the same but gcc chose the other order.
   2) `(x + 0x28) & 0xFFF8` and `(x + 0x90) & 0xFFF8`: retail emits a single
      `andi a1,a1,0xfff8`; here gcc emits `li t0,-8 / and` plus `andi 0xfff8`
      (it simplifies the mask to -8 once it knows the value is 16-bit, then
      truncates again). Not fixed by spelling the mask on a u16 local or by
      dropping the mask in favour of the u16 parameter.
   3) four `addu` operand-order flips (retail base-first for the sb stores,
      offset-first for the func_801E7C50 pointers) - the embedded-assignment
      lever did not move them here; permuter job.
   4) two `sb $zero` to the HIGH bytes of the two u16 glyph codes (sp+0x19 and
      sp+0x1B) happen once before the loop; a zero initializer on the buffer
      makes gcc copy from .sdata, four explicit stores keep all four, so the
      attempt below writes the two bytes directly.
   Shape summary: for each of 16 item slots (2 columns x 8 rows, g_GameState
   inventory starting at arg0 * 2) either clear the dangling id/qty pair, or
   render the item name + 2-digit quantity into two 16-record MenuString arrays
   at unk42C + 0x00 and + 0x800, upload both with a LoadImage of one 0x28 x 0xD
   cell, then set up their prims with func_801E7C50 and MenuSetQuadVertices.
   The flag byte is pDressingRoom->pItemsData[id].displayFlags; bit 0x80 is the
   shaded variant and is dropped when bit 0x20 is set while g_ShopIndex is 0. */
/*
void func_801DA5BC(s32 arg0) {
    u8 digitCodes[4];
    u8 text[8];
    RECT rect;
    void* pWorkspace;
    u8 tens;
    s32 itemFlags;
    s32 x;
    s32 i;

    pWorkspace = HeapAlloc(0x3F6, 0);
    digitCodes[1] = 0;
    digitCodes[3] = 0;
    for (i = 0; i < 0x10; i++) {
        if (g_GameState.itemIDs[arg0 * 2 + i] == 0) {
            g_GameState.itemQuantities[arg0 * 2 + i] = 0;
        } else if (g_GameState.itemQuantities[arg0 * 2 + i] == 0) {
            g_GameState.itemIDs[arg0 * 2 + i] = 0;
        } else {
            if (g_GameState.itemQuantities[arg0 * 2 + i] >= 0x64) {
                g_GameState.itemQuantities[arg0 * 2 + i] = 0x63;
            }
            ((MenuString*)g_Menu->pItemRows)[i].width =
                SystemRenderStringEntry(GetItemName(g_GameState.itemIDs[arg0 * 2 + i]), pWorkspace, 0x24, 0);
            tens = g_GameState.itemQuantities[arg0 * 2 + i] / 10;
            digitCodes[0] = tens != 0 ? tens + 0x10 : 0xC3;
            digitCodes[2] = g_GameState.itemQuantities[arg0 * 2 + i] % 10 + 0x10;
            SystemDecodeGlyphCodesN((u16*)digitCodes, text, 2);
            ((MenuString*)g_Menu->pItemRows)[i + 0x10].width =
                SystemRenderStringEntry(text, pWorkspace, 0x24, 1);
            rect.x = 0x180 + (i & 1) * 0x18;
            rect.y = 0x80 + (i / 2) * FONT_LETTER_HEIGHT;
            rect.w = 0x28;
            rect.h = FONT_LETTER_HEIGHT;
            LoadImage(&rect, (u_long*)pWorkspace);
            DrawSync(0);
            itemFlags = g_Menu->pDressingRoom->pItemsData[g_GameState.itemIDs[arg0 * 2 + i]].displayFlags;
            if ((itemFlags & 0x20) && (g_ShopIndex == 0)) {
                itemFlags = 0;
            } else {
                itemFlags &= 0x80;
            }
            func_801E7C50(&((MenuString*)g_Menu->pItemRows)[i], i, 0x80, itemFlags | 1);
            func_801E7C50(&((MenuString*)g_Menu->pItemRows)[i + 0x10], i, 0x80, itemFlags | 2);
            x = (i % 2) * 0x88;
            MenuSetQuadVertices(((MenuString*)g_Menu->pItemRows)[i].vertices, (x + 0x28) & 0xFFF8,
                                (i / 2) * 0x10 + 0xE, ((MenuString*)g_Menu->pItemRows)[i].width,
                                FONT_LETTER_HEIGHT);
            MenuSetQuadVertices(((MenuString*)g_Menu->pItemRows)[i + 0x10].vertices, (x + 0x90) & 0xFFF8,
                                (i / 2) * 0x10 + 0xE, ((MenuString*)g_Menu->pItemRows)[i + 0x10].width,
                                FONT_LETTER_HEIGHT);
            ((MenuString*)g_Menu->pItemRows)[i].renderContext = *(u8*)&g_Menu->renderContext;
            ((MenuString*)g_Menu->pItemRows)[i + 0x10].renderContext = *(u8*)&g_Menu->renderContext;
            g_Menu->pItemRows->shouldRenderRows[i] = 1;
            continue;
        }
        g_Menu->pItemRows->shouldRenderRows[i] = 0;
    }
    HeapFree(pWorkspace);
    g_Menu->pManager->shouldRenderUnk42C = 1;
}

*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DA5BC);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DA9A8);

void MenuInitializeArrowCursor(u_char index) {
    g_Menu->arrowCursors[index] = HeapAlloc(0x78, 0);
    bzero(g_Menu->arrowCursors[index], 0x78);
    g_Menu->arrowCursors[index]->curAnimFrame = 4;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DB0A8);

/* 98.80%. Only difference: the MenuSetQuadVertices argument setup. Original emits
   `addu a1,v1,s2; addu a2,t0,s1; andi a1,0xffff; andi a2,0xffff` (both sums,
   then both truncations); gcc folds each sum into its argument position and
   emits `addu a1; andi a1; addu a2; andi a2`. Every other line is identical.
   Tried, all with the same interleaving: sums as s32 locals (also with the two
   statements in reverse order), inline expressions, u16 locals, the four poly
   loads hoisted into u16 locals first. cse replaces the use of a single-use sum
   pseudo with the recomputed add, so neither the statement order nor the
   innermost register survives. Also screened (all 98.80): gcc 2.7.2-cdk, -O3,
   -G0.
   Full decode: pCursor = g_Menu->arrowCursors[arg3] (MenuArrowCursor); the
   poly record is &pCursor->polys[g_Menu->renderContext], read as u16 at
   +8/+A/+10/+22 (POLY_FT4.x0/y0/x1/y3, hence lhu), ofsX in $s2, ofsY in $s1.
   void func_801DB0A8(s32 arg0, s32 arg1, u_char arg2, u_char arg3) {
       MenuArrowCursor* pCursor;
       u8* pPoly;
       s32 ofsX;
       s32 ofsY;
       s32 x;
       s32 y;
       u16 lx0;
       u16 ly0;
       u16 lx1;
       u16 ly1;
       s32 shouldDraw = 1;

       pCursor = g_Menu->arrowCursors[arg3];
       if (++pCursor->animFrameDuration >= 6) {
           pCursor->curAnimFrame--;
           if (pCursor->curAnimFrame < 0) {
               pCursor->curAnimFrame = 4;
           }
           pCursor->animFrameDuration = 0;
       }
       switch (arg2) {
       case 0:
           ofsX = (arg0 % 2) * 0x88 + 0x1C;
           ofsY = (arg0 / 2) * 0x10 + 0x11;
           break;
       case 1:
           if (arg0 >= arg1 * 2 && arg0 < arg1 * 2 + 0x10) {
               ofsX = (arg0 % 2) * 0x88 + 0x18;
               ofsY = ((arg0 - arg1 * 2) / 2) * 0x10 + 0x11;
           } else {
               shouldDraw = 0;
           }
           break;
       case 2:
           ofsX = (arg0 % 2) * 0x88 + 0x18;
           ofsY = (arg0 / 2) * 0x10 + 0x14;
           break;
       case 3:
           ofsX = 0xA0;
           ofsY = arg0 * 13 + 0x14;
           shouldDraw = 2;
           break;
       }
       if (shouldDraw != 0) {
           func_8002675C(g_Menu->resources, pCursor->curAnimFrame + 0x15B, (POLY_FT4*)pCursor,
                         g_Menu->renderContext, 0, 0, 0x1000);
           pPoly = (u8*)pCursor + g_Menu->renderContext * 0x28;
           lx0 = *(u16*)(pPoly + 8);
           ly0 = *(u16*)(pPoly + 0xA);
           lx1 = *(u16*)(pPoly + 0x10);
           ly1 = *(u16*)(pPoly + 0x22);
           y = ly0 + ofsY;
           x = lx0 + ofsX;
           MenuSetQuadVertices(pCursor->vertices, x, y, lx1 - lx0, ly1 - ly0);
           pCursor->renderContext = *(u8*)&g_Menu->renderContext;
           g_Menu->pManager->shouldRenderArrowCursor[arg3] = 1;
       } else {
           g_Menu->pManager->shouldRenderArrowCursor[arg3] = 0;
       }
   }
*/

void MenuFreeArrowCursor(u_char index) {
    HeapFree(g_Menu->arrowCursors[index]);
    g_Menu->pManager->shouldRenderArrowCursor[index] = 0;
}

/* 96.75%. Frame and instruction multiset identical except that the original
   keeps the record pointer in $a2 and re-forms the poly address a second time
   (`addu a0,a2,a1` after the `r0 != 0x20` check) where gcc CSEs it into one
   address, and the original carries the `i * 0x80` offset in an incrementing
   $s2 for the second array. Also `addu` operand order differs in three places
   (original puts the byte offset first, gcc the pointer): spelling the poly
   pointer as `renderContext * 0x28 + (u32)base` fixes the order but relocates
   the shift chain and dropped the function to 87.15%.
void func_801DB39C(u_char arg0) {
    POLY_FT4* pPoly;
    s32 i;

    func_801E8F60(3, arg0);
    func_801E8F60(4, arg0);
    pPoly = &g_Menu->pScrollHandle->polys[g_Menu->pScrollHandle->renderContext];
    MenuSetPrimShadingMode(pPoly, arg0);
    for (i = 0; i < 0x10; i++) {
        pPoly = &((MenuString*)g_Menu->pItemRows)[i].polys[((MenuString*)g_Menu->pItemRows)[i].renderContext];
        if (pPoly->r0 != 0x20) {
            MenuSetPrimShadingMode(pPoly, arg0);
            pPoly = &((MenuString*)g_Menu->pItemRows)[i + 0x10]
                         .polys[((MenuString*)g_Menu->pItemRows)[i + 0x10].renderContext];
            MenuSetPrimShadingMode(pPoly, arg0);
        }
    }
    for (i = 0; i < 2; i++) {
        pPoly = &g_Menu->arrowCursors[i]->polys[g_Menu->arrowCursors[i]->renderContext];
        MenuSetPrimShadingMode(pPoly, arg0);
    }
    pPoly = &((MenuString*)g_Menu->pItemRows)[0x20].polys[((MenuString*)g_Menu->pItemRows)[0x20].renderContext];
    MenuSetPrimShadingMode(pPoly, arg0);
    pPoly = &((MenuString*)g_Menu->pItemRows)[0x21].polys[((MenuString*)g_Menu->pItemRows)[0x21].renderContext];
    MenuSetPrimShadingMode(pPoly, arg0);
    pPoly = &((MenuString*)g_Menu->pItemRows)[0x22].polys[((MenuString*)g_Menu->pItemRows)[0x22].renderContext];
    MenuSetPrimShadingMode(pPoly, arg0);
    for (i = 0; i < 8; i++) {
        pPoly = &g_Menu->unk10E0[i].polys[g_Menu->unk10E0[i].renderContext];
        MenuSetPrimShadingMode(pPoly, arg0);
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DB39C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DB5E4);
