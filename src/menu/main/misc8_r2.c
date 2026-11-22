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
      offset-first for the MenuSetStringPrimitives pointers) - the embedded-assignment
      lever did not move them here; permuter job.
   4) two `sb $zero` to the HIGH bytes of the two u16 glyph codes (sp+0x19 and
      sp+0x1B) happen once before the loop; a zero initializer on the buffer
      makes gcc copy from .sdata, four explicit stores keep all four, so the
      attempt below writes the two bytes directly.
   Shape summary: for each of 16 item slots (2 columns x 8 rows, g_GameState
   inventory starting at arg0 * 2) either clear the dangling id/qty pair, or
   render the item name + 2-digit quantity into two 16-record MenuString arrays
   at unk42C + 0x00 and + 0x800, upload both with a LoadImage of one 0x28 x 0xD
   cell, then set up their prims with MenuSetStringPrimitives and MenuSetQuadVertices.
   The flag byte is pDressingRoom->pItemsData[id].displayFlags; bit 0x80 is the
   shaded variant and is dropped when bit 0x20 is set while g_ShopIndex is 0.
   RE-MEASURED AND IMPROVED (round 394), best 95.44% (1024 vs 1012 bytes, 256 vs
   251 insns). Item 1 above is SOLVED: the two fail paths now sit out of line
   exactly where retail has them and share one shouldRenderRows[i] = 0. The
   source shape is the NESTED one, not the leading-continue one -- the outer test
   POSITIVE with the main body in the inner if and the inner test written
   `!= 0` so the main body is generated before its else (91.33 leading-continue,
   93.54 nested with the inner test `== 0`, 95.44 nested with the inner `!= 0`).
   Still open, all re-measured on this body:
   a) both (x + 0x28) & 0xFFF8 and (x + 0x90) & 0xFFF8 emit an extra
      `li t0,-8` + `and` in front of retail's single `andi a1,a1,0xfff8`; that is
      4 of the 5 surplus insns. Unchanged by a u16 x local (re-probed on this
      body: still 256 / 95.44), by the sizeof spelling and by dropping the mask.
   b) x and y swap registers (retail keeps x in $s1, y in $s0), the /10 magic
      stays in $a0 across an addu in retail but in $v0 here, and mfhi lands in
      $t0 here vs $v0 in retail. That is the round 391 preset signature, but the
      same body scores 90.48% / 1016 bytes under gcc 2.6.0, so this unit is MIXED:
      2.6.0 is better for func_801DB39C (100.00 vs 98.77, see its park) and worse here.
   c) five addu operand flips (retail base-first, ours index-first).
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DA5BC);
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
                    itemFlags = g_Menu->pDressingRoom->pItemsData[g_GameState.itemIDs[arg0 * 2 + i]].displayFlags;
                    if ((itemFlags & 0x20) && (g_ShopIndex == 0)) {
                        itemFlags = 0;
                    } else {
                        itemFlags &= 0x80;
                    }
                    MenuSetStringPrimitives(&((MenuString*)g_Menu->pItemRows)[i], i, 0x80, itemFlags | 1);
                    MenuSetStringPrimitives(&((MenuString*)g_Menu->pItemRows)[i + 0x10], i, 0x80, itemFlags | 2);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DA9A8);

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

/* PARK: best 100.00% under gcc 2.6.0, 584 bytes / 172 insns, instruction for
   instruction identical. Under this unit's Default preset (2.7.2, -O2 -G8) the
   same body is 98.77% at 588 bytes (retail 584): the item-name loop's counter
   `addiu s1,s1,1` sits in the LATCH here, and retail has it in the delay slot
   of that loop's `beq v1,v0` (the r0==0x20 skip), so the slot is a nop here and
   the body is one instruction over and cannot link.
   That placement is a compiler-version difference, not a source shape: 2.7.2
   emits a biv increment in the latch block, and its scheduler cannot move it
   across the loop's if (probed with cc1_probe.sh on a standalone loop of the
   same shape - the increment lands at the latch), so no for / while / do-while /
   goto / continue / ++i spelling reaches retail's slot. Every shape the earlier
   park tried measures 98.77 at 588 bytes with the identical nop.
   PRESET_260: with `try_flags.sh menu/main/misc8_r2 --all --gcc gcc-2.6.0-psx`
   all NINE C functions of this unit are 100.00 - MenuShutdown,
   MenuLoadUnkDE0Strings, MenuInitializeUnk42CPanel, MenuFreeUnk42CPanel,
   MenuInitializeArrowCursor, MenuBuildArrowCursorPrim, MenuFreeArrowCursor,
   this function and MenuBuildPartyCharacterPrims - so the unit is switchable to
   XenoMenu260, and this body is then a straight un-park: under 2.6.0 it is
   100.00 at retail's exact 584 bytes. UN-PARK IT, do not re-derive it.
   The `pRows` local is load bearing under 2.6.0: `pRows =
   g_Menu->pItemRows->itemNameStrings; pRow = &pRows[i];` is the spelling that
   emits retail's `addu a2,a0,s0` (panel base first), where the plain
   `&g_Menu->pItemRows->itemNameStrings[i]` gives the mirrored `addu a2,s0,a0`
   (99.93 at the same 584 bytes). Also measured for that addu and rejected:
   `itemNameStrings + i` (no change) and a double-negated offset cast, which
   promotes a register and grows the frame to 0x30 with 176 insns.
   Two facts from the earlier park, still true under 2.6.0: the item-name loop's
   TEST poly must be reached through the PANEL
   (pItemRows->itemNameStrings[i].polys[pRow->renderContext]) while pRow supplies
   the renderContext and the call argument - that produces retail's two-addu test
   address and the loop's register assignment (pItemRows in a0, row in a2, render
   context in v1); and the five single-string sites need their poly reached
   OFFSET FIRST (renderContext * sizeof(POLY_FT4) + (u32)base), because the plain
   &base->polys[rc] emits `addu d,base,sum` where retail has `addu d,sum,base`.
   What it does: applies one shading mode (arg0) to every prim of the item panel.
*/
/*
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
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DB39C);

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
