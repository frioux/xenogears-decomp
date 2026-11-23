#include "common.h"

#include "main/game.h"
#include "psyq/libapi.h"
#include "system/menu.h"
#include "psyq/libcd.h"

extern u8 g_MenuOverlayIndex; // Index of the menu overlay the menu was entered with
extern u8 g_ShopIndex;
extern u16 D_801EA53C[]; // String IDs of the menu strings
extern u16 D_801EA542[]; // String IDs of the menu strings
extern u16 g_MenuItemRowStringIds[]; // String IDs of the menu strings
extern s32 g_MenuCardSlotTotals;
extern s32 D_801EA904;
extern s32 D_801EA6FC;
extern s32 D_801EA700;
extern s32 D_801EA704;
extern s32 D_801EA708;
extern s32 D_801EA70C;
extern u8 D_801E9785; // Set once the three MenuCharacter records have been allocated
extern u8 D_801EA710;
extern u8 D_801EA714;
extern CdlCB g_MenuSavedCdSyncCallback;
extern CdlCB g_MenuSavedCdReadyCallback;
extern CdlCB g_MenuSavedCdReadCallback;
extern u8 D_801EA574[];
extern u8 g_MenuNameTextureX[];
extern u8 D_801E9F88[];
extern s32 D_801E977C[]; // String IDs of the menu scroll bar textures
extern u8 g_MenuColumnRowStringIds[];
extern u8 g_MenuPanelRowXOffsets[];
extern s32 D_801EA054[]; // X positions of the character panel label glyphs, first row of the layout
extern s32 D_801EA098[]; // The same X positions, second row of the layout
extern s32 D_801EA0DC[]; // Y positions of the label glyphs of the first row
extern s32 D_801EA120[]; // Y positions of the label glyphs of the second row

void MenuBuildCharacterPrims(MenuCharacter* pCharacter, u_char characterId, u_char localIndex, s32 arg3, s32 arg4, u_char arg5);

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
void SoundSetVolumeMode(s32 arg0);
void MenuClearGlyphTexture(void);
void MenuFreeCellPrimBlocks(void);
void MenuFreePrimBuffer(void);
void Vsync(s32 mode);
void MenuCloseCardEvents(void);
void MenuRenderFrame(void);
u_char func_801C93A8(void);
void MenuCreateWindow2Strings(s32 arg0);
void MenuFreeWindow2Strings(void);
void MenuSetPrimShadingMode(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);
void MenuSetRowQuadVertices(s32 count, void* pStrings, void* pStringIds, void* pXOffsets, void* pShouldRender,
                            u8 arg5, u8 arg6, u8 arg7);
void* GetStringEntry(void* pStringData, u32 index);
void MenuSetPolySemiTransGrey(P_TAG* pTag);
void MenuSetStringPrimitives(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);
void* GetItemName(int entryId);
s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);

void MenuShutdown(void) {
    s32 i;

    g_Menu->cardSlotState = 0;
    MenuClearGlyphTexture();
    MenuFreeCellPrimBlocks();
    MenuFreePrimBuffer();
    for (i = 0; i < MENU_CARD_COUNT * MENU_CARD_RECORD_COUNT; i++) {
        g_Menu->menuUnk2->records[i].unk58 = 0;
    }
    g_Menu->menuUnk2->previousCardSlotCounts[0] = MENU_SLOT_NONE;
    g_Menu->menuUnk2->previousCardSlotCounts[1] = MENU_SLOT_NONE;
    D_801EA904 = 0;
    g_MenuCardSlotTotals = 0;
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    CdSyncCallback(g_MenuSavedCdSyncCallback);
    CdReadyCallback(g_MenuSavedCdReadyCallback);
    CdReadCallback(g_MenuSavedCdReadCallback);
    ExitCriticalSection();
}

void MenuLoadUnkDE0Strings(void) {
    if (g_MenuOverlayIndex != 2) {
        MenuLoadStringsFromIds(sizeof(g_Menu->unkDE0) / sizeof(g_Menu->unkDE0[0]), g_Menu->unkDE0, D_801EA53C,
                               g_Menu->pManager->shouldRenderStringsDE0);
    } else {
        MenuLoadStringsFromIds(sizeof(g_Menu->unkDE0) / sizeof(g_Menu->unkDE0[0]), g_Menu->unkDE0, D_801EA542,
                               g_Menu->pManager->shouldRenderStringsDE0);
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801D9F98);

void MenuInitializeUnk42CPanel(void) {
    MenuUnk9* pMenu;

    MenuSetupPointerCursors(2);
    MenuLoadStringsFromIds(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuItemRowStringIds,
                           g_Menu->pManager->shouldRenderStrings10E0);
    pMenu = HeapAlloc(sizeof(MenuUnk9), 0);
    g_Menu->pItemRows = pMenu;
    bzero(pMenu, sizeof(MenuUnk9));
    func_801C72BC(0);
}

void MenuFreeUnk42CPanel(void) {
    MenuFreeScrollHandle();
    MenuFreeWindow(3);
    MenuFreeWindow(4);
    g_Menu->pManager->shouldRenderUnk42C = 0;
    func_801C72BC(MENU_PANEL_FREE_OFFSET);
    HeapFree(g_Menu->pItemRows->pVramBuffer);
    HeapFree(g_Menu->pItemRows);
    HeapFree(g_Menu->pDressingRoom->pItemsData);
}

/* 97.17% (1008 vs retail 1004 bytes, 252 vs 251 insns), round 406.
   Body shape is settled: nested if (outer test POSITIVE, inner test `!= 0`) so
   the two fail paths sit out of line where retail has them.
   SOLVED this round, each measured:
   - `(u16)(u16)(x + 0x28) & 0xFFF8` gives retail's SINGLE `andi a1,a1,0xfff8`. A bare
     `(u16)(x + 0x28) & 0xFFF8` inside a loop whose trip count bounds x lets cc1 prove
     the sum is 16-bit, so it emits `li t0,-8` + `and` AND a second `andi` for
     the u16 parameter truncation (4 surplus insns, 95.44). The cast on the SUM
     defeats the fold; a u16 local holding the sum works too, a u16 local
     holding x does not (the subreg is seen through).
   - `(i / 2) * 0x10 | 0xE` (not `+ 0xE`) gives retail's `ori s0,s0,0xe`, and
     the `andi s0,s0,0xffff` truncation stays 0xffff.
   - separate `displayFlags` and `itemShade` locals instead of mutating one flag
     variable: retail keeps the byte in $v1 and writes the masked value to $s0
     (`andi s0,v1,0x80`); one mutable variable gives `andi s0,s0,0x80`.
   RESIDUAL, all measured on this body:
   a) one surplus `nop`: after `lw v1,0x42C(v1)` retail fills the load-delay
      slot with the /10 magic `lui a0,0xcccc` and keeps it in $a0 with the
      dividend in $v0, coalescing `mfhi v0`; this body materialises the magic
      later in $v0 and lands the HI in $t0. permute.sh 1447 iterations,
      base=715 best=715, so not a register shuffle.
   b) eight `addu` operand flips, consistently MIRRORED: retail is base-first at
      the four `rows[k].field` store sites and index-first at the four
      `&rows[k]` argument sites; this body is the exact opposite in both groups,
      so no single spelling flips them together. A pointer local for
      g_Menu->pItemRows is the known trap (it hoists the pool load, lever
      b-inplace round 396); the double-negated offset moves only the store group.
   c) `andi a1,a1,0xfff8` comes one instruction early: retail emits it after the
      y `sll`/`ori` pair, here before.
   Presets: gcc-2.6.0 scores 94.65 on this body (2.7.2 97.17), so this function
   wants 2.7.2 even though every already-matched function in the unit is 100.00
   under 2.6.0.
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DA5BC);
/*
void func_801DA5BC(s32 arg0) {
    u8 digitCodes[4];
    u8 text[8];
    RECT rect;
    void* pWorkspace;
    u8 tens;
    u8 itemShade;
    u8 displayFlags;
    s32 x;
    s32 i;

    pWorkspace = HeapAlloc(0x3F6, 0);
    digitCodes[1] = 0;
    digitCodes[3] = 0;
    for (i = 0; i < 0x10; i++) {
        if (g_GameState.itemIDs[arg0 * 2 + i] != 0) {
            if (g_GameState.itemQuantities[arg0 * 2 + i] != 0) {
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
                    displayFlags = g_Menu->pDressingRoom->pItemsData[g_GameState.itemIDs[arg0 * 2 + i]].displayFlags;
                    if ((displayFlags & 0x20) && (g_ShopIndex == 0)) {
                        itemShade = 0;
                    } else {
                        itemShade = displayFlags & 0x80;
                    }
                    MenuSetStringPrimitives(&((MenuString*)g_Menu->pItemRows)[i], i, 0x80, itemShade | 1);
                    MenuSetStringPrimitives(&((MenuString*)g_Menu->pItemRows)[i + 0x10], i, 0x80, itemShade | 2);
                    x = (i % 2) * 0x88;
                    MenuSetQuadVertices(((MenuString*)g_Menu->pItemRows)[i].vertices, (u16)(x + 0x28) & 0xFFF8,
                                        (i / 2) * 0x10 | 0xE, ((MenuString*)g_Menu->pItemRows)[i].width,
                                        FONT_LETTER_HEIGHT);
                    MenuSetQuadVertices(((MenuString*)g_Menu->pItemRows)[i + 0x10].vertices, (u16)(x + 0x90) & 0xFFF8,
                                        (i / 2) * 0x10 | 0xE, ((MenuString*)g_Menu->pItemRows)[i + 0x10].width,
                                        FONT_LETTER_HEIGHT);
                    ((MenuString*)g_Menu->pItemRows)[i].renderContext = *(u8*)&g_Menu->renderContext;
                    ((MenuString*)g_Menu->pItemRows)[i + 0x10].renderContext = *(u8*)&g_Menu->renderContext;
                g_Menu->pItemRows->shouldRenderRows[i] = 1;
            } else {
                g_GameState.itemIDs[arg0 * 2 + i] = 0;
                g_Menu->pItemRows->shouldRenderRows[i] = 0;
            }
        } else {
            g_GameState.itemQuantities[arg0 * 2 + i] = 0;
            g_Menu->pItemRows->shouldRenderRows[i] = 0;
        }
    }
    HeapFree(pWorkspace);
    g_Menu->pManager->shouldRenderUnk42C = 1;
}
*/
/* Draws the item detail block: renders the item name of the row the cursor sits
   on into an image, uploads it, and points the three item-row sprite groups at
   the row's name and quantity prims. */
void func_801DA9A8(s32 arg0, s32 arg1) {
    MenuShopItem* pItem;
    RECT rect;
    void* pImage;
    u8* pItemId;
    s32 quantityXOffset;
    s32 row;
    s32 rowAlt;

    pItemId = &g_GameState.itemIDs[arg1 * 2 + arg0];
    if (*pItemId != 0) {
        pImage = HeapAlloc(MENU_ITEM_DESC_IMAGE_BUFFER_SIZE, 0);
        bzero(pImage, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
        g_Menu->pItemRows->primGroupStrings[2].width =
            SystemRenderStringEntry(GetStringEntry(g_Menu->pItemRows->pVramBuffer, *pItemId), pImage, 0x39, 0);
        rect.x = 0x140;
        rect.y = 0x4E;
        rect.w = 0x3C;
        rect.h = FONT_LETTER_HEIGHT;
        LoadImage(&rect, pImage);
        DrawSync(0);
        MenuSetStringPrimitives(&g_Menu->pItemRows->primGroupStrings[2], 0, 0, 0);
        MenuSetFt4QuadVertices(&g_Menu->pItemRows->primGroupStrings[2].polys[g_Menu->renderContext], 0x1C, 0xA1, 0, 0x4E,
                               g_Menu->pItemRows->primGroupStrings[2].width, FONT_LETTER_HEIGHT);
        MenuSetQuadVertices(g_Menu->pItemRows->primGroupStrings[2].vertices, 0x1C, 0xA1,
                            g_Menu->pItemRows->primGroupStrings[2].width, FONT_LETTER_HEIGHT);
        HeapFree(pImage);
        quantityXOffset = (g_Menu->pItemRows->itemQuantityStrings[arg0].width == 0x10) * 4;
        memmove(&g_Menu->pItemRows->primGroupStrings[0], &g_Menu->pItemRows->itemNameStrings[arg0], sizeof(MenuString));
        memmove(&g_Menu->pItemRows->primGroupStrings[1], &g_Menu->pItemRows->itemQuantityStrings[arg0],
                sizeof(MenuString));
        MenuSetQuadVertices(g_Menu->pItemRows->primGroupStrings[0].vertices, 0x10, 0x93,
                            g_Menu->pItemRows->itemNameStrings[arg0].width, FONT_LETTER_HEIGHT);
        MenuSetQuadVertices(g_Menu->pItemRows->primGroupStrings[1].vertices, quantityXOffset | 0x78, 0x93,
                            g_Menu->pItemRows->itemQuantityStrings[arg0].width, FONT_LETTER_HEIGHT);
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[0].polys[g_Menu->renderContext])->r0 = 0x80;
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[0].polys[g_Menu->renderContext])->g0 = 0x80;
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[0].polys[g_Menu->renderContext])->b0 = 0x80;
        SetSemiTrans(&g_Menu->pItemRows->primGroupStrings[0].polys[g_Menu->renderContext], 0);
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[1].polys[g_Menu->renderContext])->r0 = 0x80;
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[1].polys[g_Menu->renderContext])->g0 = 0x80;
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[1].polys[g_Menu->renderContext])->b0 = 0x80;
        SetSemiTrans(&g_Menu->pItemRows->primGroupStrings[1].polys[g_Menu->renderContext], 0);
        MenuClearBytes(MENU_ITEM_ROW_STRING_COUNT, g_Menu->pManager->shouldRenderStrings10E0);
        pItem = &g_Menu->pDressingRoom->pItemsData[*pItemId];
        if (pItem->displayFlags & 0xC0) {
            if (pItem->flags & 0x4000) {
                row = 2;
            } else {
                row = (pItem->flags & 0x1000) == 0;
            }
            MenuSetRowQuadVertices(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuColumnRowStringIds,
                                   g_MenuPanelRowXOffsets, g_Menu->pManager->shouldRenderStrings10E0, row, 0, 1);
            rowAlt = (pItem->flags & 3) + 3;
            MenuSetRowQuadVertices(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuColumnRowStringIds,
                                   g_MenuPanelRowXOffsets, g_Menu->pManager->shouldRenderStrings10E0, rowAlt, 0, 1);
            ((P_TAG*)&g_Menu->itemRowStrings[row].polys[g_Menu->renderContext])->r0 = 0x80;
            ((P_TAG*)&g_Menu->itemRowStrings[row].polys[g_Menu->renderContext])->g0 = 0x80;
            ((P_TAG*)&g_Menu->itemRowStrings[row].polys[g_Menu->renderContext])->b0 = 0x80;
            SetSemiTrans(&g_Menu->itemRowStrings[row].polys[g_Menu->renderContext], 0);
            ((P_TAG*)&g_Menu->itemRowStrings[rowAlt].polys[g_Menu->renderContext])->r0 = 0x80;
            ((P_TAG*)&g_Menu->itemRowStrings[rowAlt].polys[g_Menu->renderContext])->g0 = 0x80;
            ((P_TAG*)&g_Menu->itemRowStrings[rowAlt].polys[g_Menu->renderContext])->b0 = 0x80;
            SetSemiTrans(&g_Menu->itemRowStrings[rowAlt].polys[g_Menu->renderContext], 0);
        }
        g_Menu->pItemRows->primGroupStrings[0].renderContext = g_Menu->renderContext;
        g_Menu->pItemRows->primGroupStrings[1].renderContext = g_Menu->renderContext;
        g_Menu->pItemRows->primGroupStrings[2].renderContext = g_Menu->renderContext;
        g_Menu->pItemRows->shouldRenderPrimGroup = 1;
    } else {
        MenuClearBytes(MENU_ITEM_ROW_STRING_COUNT, g_Menu->pManager->shouldRenderStrings10E0);
        g_Menu->pItemRows->shouldRenderPrimGroup = 0;
    }
}

void MenuInitializeArrowCursor(u_char index) {
    g_Menu->arrowCursors[index] = HeapAlloc(sizeof(MenuArrowCursor), 0);
    bzero(g_Menu->arrowCursors[index], sizeof(MenuArrowCursor));
    g_Menu->arrowCursors[index]->curAnimFrame = MENU_ARROW_CURSOR_LAST_ANIM_FRAME;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

void MenuBuildArrowCursorPrim(s32 arg0, s32 arg1, u_char arg2, u_char arg3) {
    MenuArrowCursor* pCursor;
    u8* pPoly;
    s32 ofsX;
    s32 ofsY;
    SVECTOR* pVertices;
    u16 x;
    u16 y;
    u16 lx0;
    u16 ly0;
    u16 lx1;
    u16 ly1;
    s32 shouldDraw = 1;

    pCursor = g_Menu->arrowCursors[arg3];
    if (++pCursor->animFrameDuration >= MENU_ARROW_CURSOR_ANIM_FRAME_TICKS) {
        pCursor->curAnimFrame--;
        if (pCursor->curAnimFrame < 0) {
            pCursor->curAnimFrame = MENU_ARROW_CURSOR_LAST_ANIM_FRAME;
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
        func_8002675C(g_Menu->resources, pCursor->curAnimFrame + MENU_TEX_ARROW_CURSOR, (POLY_FT4*)pCursor,
                      g_Menu->renderContext, 0, 0, 0x1000);
        pVertices = pCursor->vertices;
        pPoly = (u8*)pCursor + g_Menu->renderContext * sizeof(POLY_FT4);
        lx0 = *(u16*)(pPoly + 8);
        ly0 = *(u16*)(pPoly + 0xA);
        lx1 = *(u16*)(pPoly + 0x10);
        ly1 = *(u16*)(pPoly + 0x22);
        x = lx0 + ofsX;
        y = ly0 + ofsY;
        MenuSetQuadVertices(pVertices, x, y, lx1 - lx0, ly1 - ly0);
        pCursor->renderContext = *(u8*)&g_Menu->renderContext;
        g_Menu->pManager->shouldRenderArrowCursor[arg3] = 1;
    } else {
        g_Menu->pManager->shouldRenderArrowCursor[arg3] = 0;
    }
}
void MenuFreeArrowCursor(u_char index) {
    HeapFree(g_Menu->arrowCursors[index]);
    g_Menu->pManager->shouldRenderArrowCursor[index] = 0;
}

void func_801DB39C(u_char arg0) {
    POLY_FT4* pPoly;
    MenuString* pRow;
    MenuString* pRows;
    s32 rc;
    s32 i;

    func_801E8F60(3, arg0);
    func_801E8F60(4, arg0);
    pPoly = (POLY_FT4*)(g_Menu->pScrollHandle->renderContext * sizeof(POLY_FT4) + (u32)g_Menu->pScrollHandle);
    MenuSetPrimShadingMode(pPoly, arg0);
    for (i = 0; i < 0x10; i++) {
        pRows = g_Menu->pItemRows->itemNameStrings;
        pRow = &pRows[i];
        if (g_Menu->pItemRows->itemNameStrings[i].polys[pRow->renderContext].r0 != 0x20) {
            MenuSetPrimShadingMode(&pRow->polys[pRow->renderContext], arg0);
            rc = g_Menu->pItemRows->itemQuantityStrings[i].renderContext;
            MenuSetPrimShadingMode(&g_Menu->pItemRows->itemQuantityStrings[i].polys[rc], arg0);
        }
    }
    for (i = 0; i < 2; i++) {
        pPoly = (POLY_FT4*)(g_Menu->arrowCursors[i]->renderContext * sizeof(POLY_FT4) + (u32)g_Menu->arrowCursors[i]);
        MenuSetPrimShadingMode(pPoly, arg0);
    }
    pPoly = (POLY_FT4*)(g_Menu->pItemRows->primGroupStrings[0].renderContext * sizeof(POLY_FT4) +
                        (u32)&g_Menu->pItemRows->primGroupStrings[0]);
    MenuSetPrimShadingMode(pPoly, arg0);
    pPoly = (POLY_FT4*)(g_Menu->pItemRows->primGroupStrings[1].renderContext * sizeof(POLY_FT4) +
                        (u32)&g_Menu->pItemRows->primGroupStrings[1]);
    MenuSetPrimShadingMode(pPoly, arg0);
    pPoly = (POLY_FT4*)(g_Menu->pItemRows->primGroupStrings[2].renderContext * sizeof(POLY_FT4) +
                        (u32)&g_Menu->pItemRows->primGroupStrings[2]);
    MenuSetPrimShadingMode(pPoly, arg0);
    for (i = 0; i < 8; i++) {
        pPoly = &g_Menu->itemRowStrings[i].polys[g_Menu->itemRowStrings[i].renderContext];
        MenuSetPrimShadingMode(pPoly, arg0);
    }
}

void MenuBuildPartyCharacterPrims(s32 arg0) {
    MenuCharacter* pCharacter;
    s32* pPositionsX;
    s32* pPositionsY;
    u8 characterId;
    u8 shouldBuild;
    s32 i;

    if (D_801E9785 == 0) {
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            pCharacter = (g_Menu->currentCharacters[i] = HeapAlloc(sizeof(MenuCharacter), 0));
            bzero(pCharacter, sizeof(MenuCharacter));
        }
        D_801E9785 = 1;
    }
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        bzero(g_Menu->currentCharacters[i], sizeof(MenuCharacter));
    }
    // Retail keeps the gear-position flag in the parameter's own variable: the
    // 0/1 store reuses its register, and a separate local lands in $v0.
    if ((u_char)arg0 != 2) {
        arg0 = 0;
        pPositionsX = D_801EA054;
        pPositionsY = D_801EA0DC;
    } else {
        pPositionsX = D_801EA098;
        pPositionsY = D_801EA120;
        arg0 = 1;
    }
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        pCharacter = g_Menu->currentCharacters[i];
        characterId = g_Menu->pManager->currentCharacterIDs[i];
        shouldBuild = 1;
        if (characterId != CHARACTER_ID_NONE) {
            if ((u_char)arg0) {
                shouldBuild = g_GameState.characters[characterId].gearId != GEAR_ID_NONE;
            }
            if (shouldBuild) {
                MenuBuildCharacterPrims(pCharacter, characterId, i, (s32)pPositionsX, (s32)pPositionsY,
                                        (u_char)arg0);
            }
        } else {
            pCharacter->shouldRender = 0;
        }
    }
    g_Menu->pManager->shouldRenderCharacters = 1;
    // The three prims are reached index-first: the `polysCursor[...]` subscript
    // re-bases the address sum onto the block pointer, which is the commuted addu
    // retail does not have.
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].x0 = 0x90;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].y0 = i * 0x38 + 0x30;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].x1 = 0xA0;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].y1 = i * 0x38 + 0x30;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].x2 = 0x90;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].y2 = i * 0x38 + 0x40;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].x3 = 0xA0;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].y3 = i * 0x38 + 0x40;
    }
}
