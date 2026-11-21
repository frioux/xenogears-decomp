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
void func_800386C4(s32 arg0);
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
    g_Menu->menuUnk2->previousCardSlotCounts[0] = 0xFF;
    g_Menu->menuUnk2->previousCardSlotCounts[1] = 0xFF;
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
        MenuLoadStringsFromIds(6, g_Menu->unkDE0, D_801EA53C, g_Menu->pManager->shouldRenderStringsDE0);
    } else {
        MenuLoadStringsFromIds(6, g_Menu->unkDE0, D_801EA542, g_Menu->pManager->shouldRenderStringsDE0);
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801D9F98);

void MenuInitializeUnk42CPanel(void) {
    MenuUnk9* pMenu;

    MenuSetupPointerCursors(2);
    MenuLoadStringsFromIds(8, g_Menu->itemRowStrings, g_MenuItemRowStringIds, g_Menu->pManager->shouldRenderStrings10E0);
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
        pVertices = pCursor->vertices;
        pPoly = (u8*)pCursor + g_Menu->renderContext * 0x28;
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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DB39C);
/* PARK 98.77%, 588 bytes. Retail is 584 (146 insns: .s runs 0x1639C..0x165E4;
   an older note's "580" was wrong), so the body is ONE instruction over and
   cannot link until it goes. Frame, saved set ($s0-$s4 + $ra), both loop
   counters and every other instruction match.
   The one extra instruction: retail fills the delay slot of the item-name
   loop's `beq v1,v0,.L801DB490` (the r0==0x20 skip) with the LATCH increment
   `addiu s1,s1,1`, so the increment exists once; here the slot is a `nop` and
   the addiu stays at the latch. BLOCK_ROTATION class, and the fill is invariant
   in the source: for / while / do-while / guard+bottom-test / `goto nextRow` /
   `for (;;)` with an empty init / `++i` / `continue` / `!(cond)` / a
   `do {} while (0)` around the if body all measure 98.77% at 588 bytes with the
   identical nop-in-slot. Do not re-spend a lane on the loop shape.
   The five single-string sites need their poly reached OFFSET-FIRST
   (`rc * sizeof(POLY_FT4) + (u32)base`): the plain `&base->polys[rc]` emits
   `addu d,base,sum` where retail has `addu d,sum,base`.
   The item-name loop's TEST poly must be reached through the PANEL
   (`pItemRows->itemNameStrings[i].polys[pRow->renderContext]`) while the row
   pointer pRow supplies the renderContext and the CALL argument. That is what
   reproduces retail's two-addu test address (base + (rc*0x28 + i*0x80)) with the
   row pointer left in $a2 for the renderContext load, and with it the whole
   loop's register assignment (pRows $a0, row pointer $a2, rc $v1) comes out
   exact - 97.16 -> 98.77. Any single-expression spelling of the test makes gcc
   reuse the row pointer and need only one addu.
   Two addu operand orders left (no size effect):
   (a) quantity-string poly base: retail `addu v1,s2,v1` (giv + base), ours
       `addu v1,v1,s2`; (b) itemRowStrings: retail `addu a2,s2,v0`, ours
       `addu a2,v0,s2`. Both are the gcc giv+base order (the item-name loop's
       renderContext load, also base + i*0x80, comes out right, so it is not a
       blanket operand flip). Measured and rejected: the offset-first cast over
       the array element (`(0x800 + i*sizeof(MenuString) + (u32)pItemRows) +
       rc*0x28`, 94.33 / 580 bytes), `(u32)&arr[i] + rc * sizeof(POLY_FT4)`
       (97.88, 98.49), a pPoly local instead of the inline argument, inlining
       the rc load at both sites, a pQuantity pointer local, and a `pPanel`
       local for the panel (92.73 / 576 - it drops the reload retail has).
   permute.sh cannot parse this body (its parser rejects the (POLY_FT4*) casts:
   "conversion to non-scalar type requested").
   PRESET_260: under gcc 2.6.0 the earlier 97.16 body is 98.36 and all seven
   already-matched functions of this unit stay 100.00, so the unit is switchable
   if that ever helps.
*/
/*
void func_801DB39C(u_char arg0) {
    POLY_FT4* pPoly;
    MenuString* pRow;
    s32 rc;
    s32 i;

    func_801E8F60(3, arg0);
    func_801E8F60(4, arg0);
    pPoly = (POLY_FT4*)(g_Menu->pScrollHandle->renderContext * sizeof(POLY_FT4) + (u32)g_Menu->pScrollHandle);
    MenuSetPrimShadingMode(pPoly, arg0);
    for (i = 0; i < 0x10; i++) {
        pRow = &g_Menu->pItemRows->itemNameStrings[i];
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
*/

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
