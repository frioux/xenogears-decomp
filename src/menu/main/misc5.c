#include "common.h"

#include "system/menu.h"

extern void MenuRenderTransformedQuads(s32 count, SVECTOR* pVertices, POLY_FT4* pPolys, u_char renderContext);
extern void MenuAddCharacterPrims(MenuCharacter* pCharacter, s32 flag);
extern void MenuRenderString(u_char length, void* pPolys, u_char renderContext);
extern void MenuRenderQuad(SVECTOR* pVertices, POLY_FT4* pPolys, s32 polyIndex, s32 otIndex);
extern void MenuRenderWindow(s32 menuIndex, u_char hasScrollBar);

extern void MenuAddUnk5StringPrims(void);
extern void MenuAddCellGridPrims(void);
extern void MenuAddCharacterSlotPrims(void);
extern void MenuRenderWindows(void);
extern void MenuAddUnk4E0Prims(void);
extern void MenuDelayLoop8(void);
extern void MenuAddSubMenuStringPrims(void);
extern void MenuDelayLoop6(void);
extern void MenuAddUnk10E0Prims(void);
extern void MenuAddUnk14E0Prims(void);
extern void MenuAddUnk17E0Prim(void);
extern void MenuAddUnk1DE0Prims(void);
extern void MenuAddUnk18E0Prims(void);
extern void MenuAddUnk1BE0Prims(void);
extern void MenuAddSpriteGroupPrims(void);
extern void MenuRenderSelectionCursor(void);
extern void MenuRenderPointerCursors(void);
extern void MenuAddSelectionMenuPrims(void);
extern void MenuUpdateWindowAnimations(void);
extern void MenuAddCharacterWindowPrims(void);
extern void MenuAddUnk358Prims(void);
extern void MenuAddUnk35CPrims(void);
extern void MenuAddUnk360Prims(void);
extern void MenuAddGoldAndPlayTimePrims(void);
extern void MenuAddUnk42CPrims(void);
extern void MenuAddUnk430Prims(void);
extern void MenuAddUnk434Prims(void);
extern void MenuAddUnk438Prims(void);

void MenuRenderWindow(s32 menuIndex, u_char hasScrollBar) {
    s32 primOffset;
    MenuWindow* pWindow;
    s32 i;
    long interpolated;
    long flag;

    pWindow = g_Menu->windows[menuIndex];
    for (i = 0; i < 4; i++) {
        MenuRenderQuad(&pWindow->vertsWindowBorderCorners[i * 4], &pWindow->polysWindowBorderCorners[i * 2],
                      pWindow->renderContext, pWindow->zIndex);
    }
    if (hasScrollBar & 0xFF) {
        for (i = 0; i < 2; i++) {
            MenuRenderQuad(&pWindow->vertsScrollBarEnds[i * 4], &pWindow->polysScrollBarEnds[i * 2],
                          pWindow->renderContext, pWindow->zIndex);
        }
        MenuRenderQuad(&pWindow->vertsScrollBarEmpty[0], &pWindow->polysScrollBarEmpty[0], pWindow->renderContext,
                      pWindow->zIndex);
    }
    MenuRenderQuad(&pWindow->vertsWindowBorderTop1[0], &pWindow->polysWindowBorderTop[0], pWindow->renderContext,
                  pWindow->zIndex);
    MenuRenderQuad(&pWindow->vertsWindowBorderTop2[0], &pWindow->polysWindowBorderTop[2], pWindow->renderContext,
                  pWindow->zIndex);
    MenuRenderQuad(&pWindow->vertsWindowBorderBottom1[0], &pWindow->polysWindowBorderBottom[0],
                  pWindow->renderContext, pWindow->zIndex);
    MenuRenderQuad(&pWindow->vertsWindowBorderBottom2[0], &pWindow->polysWindowBorderBottom[2],
                  pWindow->renderContext, pWindow->zIndex);
    MenuRenderQuad(&pWindow->vertsWindowBorderLeft1[0], &pWindow->polysWindowBorderLeft[0], pWindow->renderContext,
                  pWindow->zIndex);
    MenuRenderQuad(&pWindow->vertsWindowBorderLeft2[0], &pWindow->polysWindowBorderLeft[2], pWindow->renderContext,
                  pWindow->zIndex);
    MenuRenderQuad(&pWindow->vertsWindowBorderRight1[0], &pWindow->polysWindowBorderRight[0],
                  pWindow->renderContext, pWindow->zIndex);
    MenuRenderQuad(&pWindow->vertsWindowBorderRight2[0], &pWindow->polysWindowBorderRight[2],
                  pWindow->renderContext, pWindow->zIndex);
    RotTransPers4(&pWindow->vertsBackground[0], &pWindow->vertsBackground[1], &pWindow->vertsBackground[2],
                  &pWindow->vertsBackground[3], (long*)&pWindow->polysBackground[pWindow->renderContext].x0,
                  (long*)&pWindow->polysBackground[pWindow->renderContext].x1,
                  (long*)&pWindow->polysBackground[pWindow->renderContext].x2,
                  (long*)&pWindow->polysBackground[pWindow->renderContext].x3, &interpolated, &flag);
    primOffset = 0x70;
    AddPrim((void*)((u32)g_Menu->pGfxEnv + (primOffset + pWindow->zIndex * sizeof(u32))),
            &pWindow->polysBackground[pWindow->renderContext]);
    AddPrim((void*)((u32)g_Menu->pGfxEnv + (primOffset + pWindow->zIndex * sizeof(u32))),
            &pWindow->drawModes[pWindow->renderContext]);
}

void MenuRenderWindows(void) {
    s32 i;
    SVECTOR rot;
    VECTOR trans;
    MATRIX m;
    long unk48[2]; // unused: the original frame keeps 8 bytes after the matrix

    for (i = 0; i < 7; i++) {
        if (g_Menu->pManager->shouldRenderWindow[i]) {
            if (g_Menu->windows[i]->useDefaultMatrix == 0) {
                PushMatrix();
                rot.vx = rot.vy = rot.vz = 0;
                trans.vx = trans.vy = 0;
                trans.vz = 0x200;
                RotMatrix(&rot, &m);
                TransMatrix(&m, &trans);
                SetRotMatrix(&m);
                SetTransMatrix(&m);
                MenuRenderWindow(i, g_Menu->windows[i]->hasScrollBar);
                PopMatrix();
            } else {
                MenuRenderWindow(i, g_Menu->windows[i]->hasScrollBar);
            }
        }
    }
}

void MenuAddUnk4E0Prims(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->shouldRenderStrings4E0[i]) {
            offset = i * sizeof(MenuString) + 0x4E0;
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (void*)(offset + (u32)g_Menu + g_Menu->titleStrings[i].renderContext * 40));
        }
    }
}

void MenuDelayLoop8(void) {
    s32 i;

    for (i = 7; i >= 0; i--) {
    }
}

void MenuAddSubMenuStringPrims(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->shouldRenderSubMenuChoiceStrings[i] && g_Menu->subMenuChoiceStrings[i].hasVertices) {
            offset = i * sizeof(MenuString) + 0xAE0;
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset + (u32)g_Menu + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu), g_Menu->subMenuChoiceStrings[i].renderContext);
        }
    }
}

void MenuDelayLoop6(void) {
    s32 i;

    for (i = 5; i >= 0; i--) {
    }
}

void MenuAddUnk10E0Prims(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 8; i++) {
        if (g_Menu->pManager->shouldRenderStrings10E0[i]) {
            offset = i * sizeof(MenuString) + 0x10E0;
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset + (u32)g_Menu + 0x50), (POLY_FT4*)(offset + (u32)g_Menu),
                          g_Menu->itemRowStrings[i].renderContext);
        }
    }
}

void MenuAddUnk14E0Prims(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->shouldRenderStrings14E0[i]) {
            offset = i * sizeof(MenuString) + 0x14E0;
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset + (u32)g_Menu + 0x50), (POLY_FT4*)(offset + (u32)g_Menu),
                          g_Menu->characterRowStrings[i].renderContext);
        }
    }
}

void MenuAddUnk17E0Prim(void) {
    u32 offset;

    if (g_Menu->pManager->shouldRenderStrings17E0[0]) {
        offset = g_Menu->unk17E0[0].renderContext * 40 + 0x17E0;
        AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)g_Menu));
    }
}

void MenuAddUnk1DE0Prims(void) {
    s32 i;
    MenuString* pString;

    if (g_Menu->pManager->shouldRenderModalStrings == 0) {
        return;
    }
    for (i = 0; i < 3; i++) {
        pString = g_Menu->modalStrings[i];
        if (pString->hasVertices) {
            MenuRenderTransformedQuads(1, pString->vertices, pString->polys, pString->renderContext);
        } else {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &pString->polys[pString->renderContext]);
        }
    }
}

void MenuAddUnk18E0Prims(void) {
    s32 i;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->shouldRenderStrings18E0[i] && g_Menu->unk18E0[i].hasVertices) {
            MenuRenderTransformedQuads(1, g_Menu->unk18E0[i].vertices, g_Menu->unk18E0[i].polys,
                          g_Menu->unk18E0[i].renderContext);
        }
    }
}

void MenuAddUnk1BE0Prims(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->shouldRenderStrings1BE0[i]) {
            offset = i * sizeof(MenuString) + 0x1BE0;
            AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)g_Menu + g_Menu->unk1BE0[i].renderContext * 40));
        }
    }
}

void MenuAddSpriteGroupPrims(void) {
    MenuAddUnk4E0Prims();
    MenuDelayLoop8();
    MenuAddSubMenuStringPrims();
    MenuDelayLoop6();
    MenuAddUnk18E0Prims();
    MenuAddUnk1BE0Prims();
    MenuAddUnk10E0Prims();
    MenuAddUnk14E0Prims();
    MenuAddUnk17E0Prim();
    MenuAddUnk1DE0Prims();
}

void MenuAddDimEffectPrims(void) {
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->pCursorPrims->polysDimEffect[g_Menu->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->pCursorPrims->drawModeDimEffect[g_Menu->renderContext]);
}
void MenuAddCharacterPrims(MenuCharacter* pCharacter, s32 flag) {
    if (pCharacter->shouldRender == 0) {
        return;
    }
    AddPrim(&g_Menu->pGfxEnv->ot[4], &pCharacter->polysPortraitSmall[pCharacter->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[4], &pCharacter->polys4B0[pCharacter->renderContext]);
    MenuRenderString(pCharacter->descriptionStringsLength, pCharacter, pCharacter->renderContext);
    MenuRenderString(pCharacter->levelStringLength, pCharacter->polysLevelString, pCharacter->renderContext);
    MenuRenderString(pCharacter->polys5F0PrimCount, pCharacter->polys5F0, pCharacter->renderContext);
    MenuRenderString(pCharacter->hpStringLength, pCharacter->polysHpString, pCharacter->renderContext);
    MenuRenderString(pCharacter->maxHpStringLength, pCharacter->polysMaxHpString, pCharacter->renderContext);
    MenuRenderString(pCharacter->mpStringLength, pCharacter->polysMpString, pCharacter->renderContext);
    MenuRenderString(pCharacter->maxMpStringLength, pCharacter->polysMaxMpString, pCharacter->renderContext);
    if (flag & 0xFF) {
        MenuRenderString(5, pCharacter->polys2D0, pCharacter->renderContext);
    }
}
void MenuAddCurrentCharacterPrims(void) {
    s32 i;

    if (g_Menu->pManager->shouldRenderCharacters) {
        for (i = 0; i < 3; i++) {
            MenuAddCharacterPrims(g_Menu->currentCharacters[i], 1);
        }
    }
}

void MenuAddScrollHandlePrims(void) {
    if (g_Menu->pManager->scrollHandleActive) {
        MenuRenderTransformedQuads(1, g_Menu->pScrollHandle->vertices, g_Menu->pScrollHandle->polys,
                      g_Menu->pScrollHandle->renderContext);
    }
}

void MenuAddShoulderButtonPrims(void) {
    if (g_Menu->pManager->shoulderButtonUiActive) {
        MenuRenderTransformedQuads(4, g_Menu->pShoulderButtonUi->vertices, g_Menu->pShoulderButtonUi->polys,
                      g_Menu->pShoulderButtonUi->renderContext);
    }
}

void MenuAddUnk42CPrims(void) {
    s32 i;
    u32 offset;
    u32 offset2;

    if (g_Menu->pManager->shouldRenderUnk42C == 0) {
        return;
    }
    for (i = 0; i < 16; i++) {
        if (g_Menu->pItemRows->shouldRenderRows[i]) {
            offset = i * sizeof(MenuString);
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset + (u32)g_Menu->pItemRows + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu->pItemRows),
                          ((u8*)(offset + (u32)g_Menu->pItemRows))[0x7D]);
            offset2 = i * sizeof(MenuString) + 0x800;
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset2 + (u32)g_Menu->pItemRows + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->pItemRows),
                          ((u8*)((u32)g_Menu->pItemRows + offset))[0x87D]);
        }
    }
    if (g_Menu->pItemRows->shouldRenderPrimGroup) {
        MenuRenderTransformedQuads(1, (SVECTOR*)((u32)g_Menu->pItemRows + 0x1050), (POLY_FT4*)((u32)g_Menu->pItemRows + 0x1000),
                      ((u8*)g_Menu->pItemRows)[0x107D]);
        MenuRenderTransformedQuads(1, (SVECTOR*)((u32)g_Menu->pItemRows + 0x10D0), (POLY_FT4*)((u32)g_Menu->pItemRows + 0x1080),
                      ((u8*)g_Menu->pItemRows)[0x10FD]);
        MenuRenderTransformedQuads(1, (SVECTOR*)((u32)g_Menu->pItemRows + 0x1150), (POLY_FT4*)((u32)g_Menu->pItemRows + 0x1100),
                      ((u8*)g_Menu->pItemRows)[0x117D]);
    }
}
void MenuAddUnk430Prims(void) {
    u32 offset;
    s32 i;
    u32 offset2;
    u32 offset3;

    if (g_Menu->pManager->shouldRenderPanel[0] == 0) {
        return;
    }
    for (i = 0, offset2 = 0x700; i < 14; i++, offset2 += sizeof(MenuString)) {
        if (((u8*)((u32)g_Menu->unk430 + i))[0x1084]) {
            offset = i * sizeof(MenuString);
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset + (u32)g_Menu->unk430 + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu->unk430),
                          ((u8*)(offset + (u32)g_Menu->unk430))[0x7D]);
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset2 + (u32)g_Menu->unk430 + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->unk430),
                          ((u8*)((u32)g_Menu->unk430 + offset))[0x77D]);
        }
    }
    if (((u8*)g_Menu->unk430)[0x1092]) {
        MenuRenderTransformedQuads(1, (SVECTOR*)((u32)g_Menu->unk430 + 0xE50), (POLY_FT4*)((u32)g_Menu->unk430 + 0xE00),
                      ((u8*)g_Menu->unk430)[0xE7D]);
        MenuRenderTransformedQuads(1, (SVECTOR*)((u32)g_Menu->unk430 + 0xED0), (POLY_FT4*)((u32)g_Menu->unk430 + 0xE80),
                      ((u8*)g_Menu->unk430)[0xEFD]);
        for (i = 0, offset3 = 0xF00; i < 2; i++, offset3 += sizeof(MenuString)) {
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset3 + (u32)g_Menu->unk430 + 0x50),
                          (POLY_FT4*)(offset3 + (u32)g_Menu->unk430),
                          ((u8*)((u32)g_Menu->unk430 + (i << 7)))[0xF7D]);
        }
    }
    MenuRenderTransformedQuads(1, (SVECTOR*)((u32)g_Menu->unk430 + 0x1050), (POLY_FT4*)((u32)g_Menu->unk430 + 0x1000),
                  ((u8*)g_Menu->unk430)[0x107D]);
}
void MenuAddUnk434Prims(void) {
    s32 i;
    u32 offset2;
    u32 offset;

    if (g_Menu->pManager->shouldRenderPanel[2] == 0) {
        return;
    }
    for (i = 0, offset2 = 0x400; i < 8; i++, offset2 += sizeof(MenuString)) {
        if (g_Menu->pCharacterSpriteRecords->shouldRenderRows[i]) {
            offset = i * sizeof(MenuString);
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset + (u32)g_Menu->pCharacterSpriteRecords + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu->pCharacterSpriteRecords),
                          ((u8*)(offset + (u32)g_Menu->pCharacterSpriteRecords))[0x7D]);
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset2 + (u32)g_Menu->pCharacterSpriteRecords + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->pCharacterSpriteRecords),
                          ((u8*)((u32)g_Menu->pCharacterSpriteRecords + offset))[0x47D]);
        }
    }
    MenuRenderTransformedQuads(1, (SVECTOR*)((u32)g_Menu->pCharacterSpriteRecords + 0x850), (POLY_FT4*)((u32)g_Menu->pCharacterSpriteRecords + 0x800),
                  ((u8*)g_Menu->pCharacterSpriteRecords)[0x87D]);
    if (g_Menu->pCharacterSpriteRecords->shouldRenderStrings880[0]) {
        for (i = 0; i < 3; i++) {
            offset2 = i * sizeof(MenuString) + 0x880;
            MenuRenderTransformedQuads(1, (SVECTOR*)(offset2 + (u32)g_Menu->pCharacterSpriteRecords + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->pCharacterSpriteRecords), g_Menu->pCharacterSpriteRecords->slotStrings[i].renderContext);
        }
    }
}

void MenuAddUnk438Prims(void) {
    u32 offset;
    s32 i;
    u32 offset2;
    u32 offset3;
    u32 offset4;

    if (g_Menu->pManager->shouldRenderPanel[3] == 0) {
        return;
    }
    for (i = 0, offset2 = 0x680; i < 13; i++, offset2 += sizeof(MenuString)) {
        if (((u8*)((u32)g_Menu->unk438 + i))[0x2596]) {
            offset = i * sizeof(MenuString);
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (POLY_FT4*)(offset + (u32)g_Menu->unk438 +
                                ((u8*)(offset + (u32)g_Menu->unk438))[0x7D] * sizeof(POLY_FT4)));
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (POLY_FT4*)(offset2 + (u32)g_Menu->unk438 +
                                ((u8*)((u32)g_Menu->unk438 + offset))[0x7D] * sizeof(POLY_FT4)));
        }
    }
    MenuRenderTransformedQuads(1, (SVECTOR*)((u32)g_Menu->unk438 + 0xD50), (POLY_FT4*)((u32)g_Menu->unk438 + 0xD00),
                  ((u8*)g_Menu->unk438)[0xD7D]);
    for (i = 0, offset4 = 0x21D0, offset3 = 0xD80; i < 13; offset3 += 0x190, i++, offset4 += 0x48) {
        MenuRenderString(((u8*)((u32)g_Menu->unk438 + i))[0x257C], (void*)(offset3 + (u32)g_Menu->unk438),
                      ((u8*)((u32)g_Menu->unk438 + i))[0x2589]);
        if (((u8*)((u32)g_Menu->unk438 + i))[0x25A3]) {
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (POLY_G4*)(offset4 + (u32)g_Menu->unk438 +
                               ((u8*)((u32)g_Menu->unk438 + i))[0x25B0] * sizeof(POLY_G4)));
        }
    }
}
void MenuAddArrowCursorPrims(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (g_Menu->pManager->shouldRenderArrowCursor[i]) {
            MenuRenderTransformedQuads(1, g_Menu->arrowCursors[i]->vertices, g_Menu->arrowCursors[i]->polys,
                          g_Menu->arrowCursors[i]->renderContext);
        }
    }
}
void MenuAddScreen0Prims(void) {
    MenuUpdateWindowAnimations();
    MenuAddSpriteGroupPrims();
    MenuRenderPointerCursors();
    MenuRenderSelectionCursor();
    MenuAddCharacterSlotPrims();
    MenuAddCellGridPrims();
    MenuAddCharacterWindowPrims();
    MenuAddUnk358Prims();
    MenuAddUnk35CPrims();
    MenuAddUnk360Prims();
    MenuAddGoldAndPlayTimePrims();
    MenuAddCurrentCharacterPrims();
    MenuAddArrowCursorPrims();
    MenuAddUnk42CPrims();
    MenuAddUnk430Prims();
    MenuAddUnk434Prims();
    MenuAddUnk438Prims();
    MenuAddScrollHandlePrims();
    MenuAddShoulderButtonPrims();
    MenuRenderWindows();
    MenuAddSelectionMenuPrims();
    MenuAddUnk5StringPrims();
}

void MenuAddScreen2Prims(void) {
    MenuUpdateWindowAnimations();
    MenuAddSpriteGroupPrims();
    MenuRenderPointerCursors();
    MenuRenderSelectionCursor();
    MenuAddCharacterSlotPrims();
    MenuAddCellGridPrims();
    MenuAddSelectionMenuPrims();
    MenuAddUnk5StringPrims();
    MenuRenderWindows();
}

void MenuAddScreen6Prims(void) {
    MenuUpdateWindowAnimations();
    MenuRenderPointerCursors();
    MenuAddSpriteGroupPrims();
    MenuRenderSelectionCursor();
    MenuAddCharacterSlotPrims();
    MenuAddCellGridPrims();
    MenuRenderWindows();
    MenuAddUnk5StringPrims();
}
