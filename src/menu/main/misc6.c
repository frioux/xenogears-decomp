#include "common.h"

#include "system/memory.h"
#include "system/menu.h"

extern u8 g_MenuOverlayIndex;

void MenuRenderFrame(void);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void MenuAddDimEffectPrims(void);
void MenuAddScreen0Prims(void);
void MenuAddScreen2Prims(void);
void MenuAddScreen6Prims(void);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuPlaySoundEffect(u_char soundEffectId);
void MenuCreateWindow(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, u_char a6, s32 a7, u_char a8);
void MenuInitializeWindowGraphics(u_char a0);
void MenuFreeWindow(u_char a0);
void MenuSetGoldDigits(u_char a0, u_char a1);
void MenuSetPlayTimeDigitPrims(u_char a0, u_char a1);
void MenuSetGearWindowContents(u_char a0, u_char a1);
void MenuSetGearWindowQuads(u_char a0, u_char a1, u_char a2);
void MenuDrawStatTable(u_char a0, s32 a1, s32 a2, u_char a3);
void func_801D8EA4(u_char a0, s32 a1, s32 a2, u_char a3);

void MenuSetWindow(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, s32 a6, u_char a7);
void MenuTweenCharacterWindows(u_char isCreate, u_char isSubmenuOpening);
void MenuLoadCharacterNameTexture(u_char a0, u_char a1);
void MenuRevealSelectionMenu(s32 a0, u32* a1);
void* GetStringEntry(void* pStringData, u32 index);
s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 length, s32 mode);
void MenuSetStringPrimitives(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);

extern u32 g_MenuMainSelectionResourceIds[];
extern u32 g_MenuShoulderButtonArrowX[];
extern s16 D_801E9A1C[][2];
extern s16 D_801E9A48[][2];
extern s16 D_801E9E94[][2];
extern u32 g_MenuShoulderButtonArrowY[];
extern u32 D_801E9A58[];
extern u32 D_801E9A68[];
extern u32 g_MenuCursorXPositions[9]; // Cursor x positions
extern s32 g_MenuCursorYPositions[9]; // Cursor y positions

void MenuInitializePolyFt4(POLY_FT4* pPoly);
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);
extern s32 g_MenuNameTextureX[];
extern u8 g_MenuNameTextureY[];
extern u8 g_MenuGlyphSjisTable[];
extern s32 D_801EA17C[];
extern s32 D_801EA584[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

// Row origins and texture-space u/v of one drawn string; the u/v pair is the low
// half of a 4-byte entry, so both arrays are indexed with the entry, not the halfword.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
} MenuStringTextureOrigin; // Size: 0x4

extern MenuStringTextureOrigin D_801EA18C[];

void MenuAddScreenPrims(void) {
    if (g_Menu->shouldDrawMenu) {
        switch (g_MenuOverlayIndex) {
            case 0:
                MenuAddScreen0Prims();
                break;
            case 2:
                MenuAddScreen2Prims();
                break;
            case 6:
                MenuAddScreen6Prims();
                break;
        }
    }
    MenuAddDimEffectPrims();
}
void MenuUpdateTransitionEffect(void) {
    SystemMenu *pMenu = g_Menu;

    switch (pMenu->transitionEffectState) {
        case 4:
            pMenu->translation.vz = 0x200;
            pMenu->rotation.vx = pMenu->rotation.vy = pMenu->rotation.vz = 0;
            pMenu->translation.vx = pMenu->translation.vy = 0;
            pMenu->transitionEffectState = 2;
            break;
        case 3:
            pMenu->translation.vz = 0x800;
            pMenu->rotation.vx = pMenu->rotation.vy = pMenu->rotation.vz = 0;
            pMenu->translation.vx = pMenu->translation.vy = 0;
            pMenu->transitionEffectState = 1;
            break;
        case 2:
            pMenu->rotation.vy -= 0x60;
            pMenu->translation.vz += 0x40;
            if (pMenu->translation.vz >= 0xE00) {
                pMenu->transitionEffectState = 0;
            }
            break;
        case 1:
            pMenu->rotation.vx += 0x7C;
            pMenu->translation.vz -= 0x30;
            if (pMenu->translation.vz < 0x200) {
                pMenu->translation.vz = 0x200;
                pMenu->rotation.vz = 0;
                pMenu->rotation.vx = 0;
                pMenu->rotation.vy = 0;
                pMenu->transitionEffectState = 0;
            }
            break;
    }
    RotMatrix(&g_Menu->rotation, &g_Menu->matTransform);
    TransMatrix(&g_Menu->matTransform, &g_Menu->translation);
    SetRotMatrix(&g_Menu->matTransform);
    SetTransMatrix(&g_Menu->matTransform);
}
void MenuStartOpenAnimation(void) {
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION_START;
    MenuPlaySoundEffect(0x5B);
}
void MenuStartCloseAnimation(void) {
    g_Menu->transitionEffectState = MENU_CLOSE_ANIMATION_START;
    MenuPlaySoundEffect(0x5C);
}
void MenuMovePointerCursor(s32 slot, u8 doInit) {
    func_8002675C(g_Menu->resources, 0x108, g_Menu->pCursorPrims->polysPointerCursor, g_Menu->renderContext, g_MenuCursorXPositions[slot], g_MenuCursorYPositions[slot], 0x1000);
    g_Menu->pCursorPrims->cursorRenderContext = (u8)g_Menu->renderContext;

    if (doInit) {
        // Shaded rectangle of shadeWidth width with green lines around it
        setXY4(&g_Menu->pCursorPrims->polyG4s[g_Menu->renderContext],
               g_MenuCursorXPositions[slot] + 0x14, g_MenuCursorYPositions[slot] - 0x24,
               g_MenuCursorXPositions[slot] + (g_Menu->pCursorPrims->shadeWidth + 0x14), g_MenuCursorYPositions[slot] - 0x24,
               g_MenuCursorXPositions[slot] + 0x14, g_MenuCursorYPositions[slot] - 0x14,
               g_MenuCursorXPositions[slot] + (g_Menu->pCursorPrims->shadeWidth + 0x14), g_MenuCursorYPositions[slot] - 0x14);

        setXY3(&g_Menu->pCursorPrims->lines1[g_Menu->renderContext],
               g_MenuCursorXPositions[slot] + 0x14, g_MenuCursorYPositions[slot] - 0x24,
               g_MenuCursorXPositions[slot] + (g_Menu->pCursorPrims->shadeWidth + 0x14), g_MenuCursorYPositions[slot] - 0x24,
               g_MenuCursorXPositions[slot] + (g_Menu->pCursorPrims->shadeWidth + 0x14), g_MenuCursorYPositions[slot] - 0x14);

        setXY3(&g_Menu->pCursorPrims->lines2[g_Menu->renderContext],
               g_MenuCursorXPositions[slot] + 0x14, g_MenuCursorYPositions[slot] - 0x24,
               g_MenuCursorXPositions[slot] + 0x14, g_MenuCursorYPositions[slot] - 0x14,
               g_MenuCursorXPositions[slot] + (g_Menu->pCursorPrims->shadeWidth + 0x14), g_MenuCursorYPositions[slot] - 0x14);

        g_Menu->pCursorPrims->drawModeRenderContext = (u8)g_Menu->renderContext;
        g_Menu->pManager->unk3 = 1;
    }
}
void MenuResetSelectionCursorFlags(void) {
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
}
void MenuSetupPointerCursors(u_char a0) {
    s32 i;

    g_Menu->pManager->shouldRenderCursors = 0;
    switch (a0) {
        case 0:
            g_Menu->pManager->shouldRenderCursors = 1;
            g_Menu->pCursors->shouldUpdatePosition[0] = 1;
            g_Menu->pCursors->shouldUpdatePosition[1] = 1;
        case 2:
            for (i = 0; i < 4; i++) {
                func_8002675C(g_Menu->resources, 0x108, &g_Menu->pCursors->polysCursor[i * 2],
                              g_Menu->renderContext, D_801E9A58[i], D_801E9A68[i], 0x800);
                g_Menu->pCursors->renderContexts[i] = (u_char)g_Menu->renderContext;
            }
            break;
        case 1:
            break;
        case 3:
            func_8002675C(g_Menu->resources, 0x108, g_Menu->pCursors->polysCursor,
                          g_Menu->renderContext, 0, 0, 0x800);
            g_Menu->pCursors->renderContexts[0] = (u_char)g_Menu->renderContext;
            break;
    }
}
void MenuHidePointerCursors(void) {
    g_Menu->pManager->shouldRenderCursors = 0;
}
extern u_char D_801EA534[]; // String IDs of the menu strings
extern u16 D_801E9E4C[];
extern u16 D_801E9E58[];

void MenuUploadStringTextures(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);

void MenuLoadSubMenuChoiceStrings(u_char a0) {
    s32 i;

    if (a0 != 0) {
        MenuUploadStringTextures(g_Menu->subMenuChoiceStrings, D_801EA534, 4, 6);
        for (i = 0; i < 3; i++) {
            MenuSetQuadVertices(g_Menu->subMenuChoiceStrings[i + 3].vertices, D_801E9E4C[i * 2], D_801E9E58[i * 2], g_Menu->subMenuChoiceStrings[i + 3].width, 0xD);
            g_Menu->subMenuChoiceStrings[i + 3].renderContext = (u_char)g_Menu->renderContext;
            g_Menu->subMenuChoiceStrings[i + 3].hasVertices = 1;
            g_Menu->pManager->shouldRenderSubMenuChoiceStrings[i + 3] = 1;
        }
    } else {
        for (i = 0; i < 3; i++) {
            g_Menu->pManager->shouldRenderSubMenuChoiceStrings[i] = 0;
        }
    }
}

void MenuClearSubMenuChoiceFlags(void) {
    int i;

    for (i = 0; i < 6; i++) {
        g_Menu->pManager->shouldRenderSubMenuChoiceStrings[i] = 0;
    }
}
void MenuSetSubMenuChoiceStringVertices(void) {
    // Unused stack that rounds the frame up to retail's 0x60 (0x50 and 99.98 without it)
    u8 pad[0x48];

    MenuLoadSubMenuChoiceStrings(0);
    // Each store re-forms the prim address from g_Menu, so the sum stays inline: a
    // POLY_FT4* local is 496 bytes/65.41 and polys[renderContext] 660 bytes/93.62.
    (g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].polys + g_Menu->renderContext)->x0 = D_801E9A1C[g_Menu->subMenuChoice][0] + 0x16 + D_801E9E94[g_Menu->subMenuChoice][0];
    (g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].polys + g_Menu->renderContext)->y0 = D_801E9A48[g_Menu->subMenuChoice][0] - 0x22;
    (g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].polys + g_Menu->renderContext)->x1 = D_801E9A1C[g_Menu->subMenuChoice][0] + 0x16 + D_801E9E94[g_Menu->subMenuChoice][0] + g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].width;
    (g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].polys + g_Menu->renderContext)->y1 = D_801E9A48[g_Menu->subMenuChoice][0] - 0x22;
    (g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].polys + g_Menu->renderContext)->x2 = D_801E9A1C[g_Menu->subMenuChoice][0] + 0x16 + D_801E9E94[g_Menu->subMenuChoice][0];
    (g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].polys + g_Menu->renderContext)->y2 = D_801E9A48[g_Menu->subMenuChoice][0] - 0x15;
    (g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].polys + g_Menu->renderContext)->x3 = D_801E9A1C[g_Menu->subMenuChoice][0] + 0x16 + D_801E9E94[g_Menu->subMenuChoice][0] + g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].width;
    (g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].polys + g_Menu->renderContext)->y3 = D_801E9A48[g_Menu->subMenuChoice][0] - 0x15;
    g_Menu->subMenuChoiceStrings[g_Menu->subMenuChoice].renderContext = (u_char)g_Menu->renderContext;
    g_Menu->pManager->shouldRenderSubMenuChoiceStrings[g_Menu->subMenuChoice] = 1;
}
void MenuCreateGoldWindow(void) {
    MenuCreateWindow(0, 0xD4, 0xB2, 0x60, 0x10, 0, 0, 4, 0);
    MenuSetGoldDigits(0xD8, 0xB6);
}
void MenuCreatePlayTimeWindow(void) {
    MenuCreateWindow(1, 0xCC, 0xC6, 0x50, 0x10, 0, 0, 4, 0);
    MenuSetPlayTimeDigitPrims(0xD0, 0xCA);
    // volatile keeps reorg from sinking this store past the epilogue restores
    *(volatile u8*)&g_Menu->pManager->playTimeDigitsActive = 1;
}
void MenuUpdatePlayTimeDigits(void) {
    if (g_Menu->pManager->playTimeDigitsActive != 0) {
        MenuSetPlayTimeDigitPrims(0xD0, 0xCA);
    }
}
// The three 0x24-byte tween records in g_Menu->windowTweens, the same view misc7.c
// declares for MenuSetCharacterWindowContents; hoist to menu.h when a header round runs.
typedef struct {
    /* 0x00 */ s32 startX;
    /* 0x04 */ s32 endX;
    /* 0x08 */ s32 startY;
    /* 0x0C */ s32 endY;
    /* 0x10 */ s32 stepX;
    /* 0x14 */ s32 stepY;
    /* 0x18 */ s32 offsetX;
    /* 0x1C */ s32 offsetY;
    /* 0x20 */ u8 isDecreasingX;
    /* 0x21 */ u8 isDecreasingY;
    /* 0x22 */ u8 stepCount;
    /* 0x23 */ u8 isComplete;
} MenuWindowTween; // Size: 0x24

void func_801C81E0(s32 startX, s32 startY, s32 endX, s32 endY, u_char stepCount, u_char index);
void func_801C8324(u_char index);
void MenuSetCharacterWindowContents(u_char index, u_char characterId);
void MenuLoadStringsFromIds(s32 count, void* pStrings, void* pStringIds, void* pRenderFlags);
void MenuClearBytes(s32 count, void* pBytes);

extern u8 g_MenuMainRowStringIds[];

void MenuTweenCharacterWindows(u_char isCreate, u_char isSubmenuOpening) {
    u_char characterId;
    s32 i;

    if (isCreate) {
        MenuLoadStringsFromIds(8, g_Menu->optionStrings, g_MenuMainRowStringIds, g_Menu->pManager->shouldRenderStrings);
        func_801C81E0(0x100, 0x86, 0x60, 6, 8, 0);
        func_801C81E0(0x108, 0x3E, 0x68, 0x3E, 8, 1);
        func_801C81E0(0x110, -0xA, 0x70, 0x76, 8, 2);
    } else {
        MenuClearBytes(8, g_Menu->pManager->shouldRenderStrings);
        func_801C81E0(0x60, 6, 0x100, 0x86, 8, 0);
        func_801C81E0(0x68, 0x3E, 0x108, 0x3E, 8, 1);
        func_801C81E0(0x70, 0x76, 0x110, -0xA, 8, 2);
    }

    // Wait for the three character panels to finish tweening in
    while (!((MenuWindowTween*)g_Menu->windowTweens)[0].isComplete
        && !((MenuWindowTween*)g_Menu->windowTweens)[1].isComplete
        && !((MenuWindowTween*)g_Menu->windowTweens)[2].isComplete) {
        for (i = 0; i < 3; i++) {
            characterId = g_Menu->pManager->currentCharacterIDs[i];
            if (characterId != CHARACTER_ID_NONE) {
                MenuSetCharacterWindowContents(i, characterId);
            }
        }
        MenuRenderFrame();
        for (i = 0; i < 3; i++) {
            if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
                func_801C8324(i);
            }
        }
    }

    if (isCreate) {
        ((MenuWindowTween*)g_Menu->windowTweens)[0].startX = 0x60;
        ((MenuWindowTween*)g_Menu->windowTweens)[0].startY = 6;
        ((MenuWindowTween*)g_Menu->windowTweens)[1].startX = 0x68;
        ((MenuWindowTween*)g_Menu->windowTweens)[1].startY = 0x3E;
        ((MenuWindowTween*)g_Menu->windowTweens)[2].startX = 0x70;
        ((MenuWindowTween*)g_Menu->windowTweens)[2].startY = 0x76;
        ((MenuWindowTween*)g_Menu->windowTweens)[0].offsetX =
            ((MenuWindowTween*)g_Menu->windowTweens)[0].offsetY = 0;
        ((MenuWindowTween*)g_Menu->windowTweens)[1].offsetX =
            ((MenuWindowTween*)g_Menu->windowTweens)[1].offsetY = 0;
        ((MenuWindowTween*)g_Menu->windowTweens)[2].offsetX =
            ((MenuWindowTween*)g_Menu->windowTweens)[2].offsetY = 0;
        for (i = 0; i < 3; i++) {
            characterId = g_Menu->pManager->currentCharacterIDs[i];
            if (characterId != CHARACTER_ID_NONE) {
                MenuSetCharacterWindowContents(i, characterId);
            }
        }
        MenuPlaySoundEffect(0x5D);
        if (!isSubmenuOpening) {
            MenuCreateGoldWindow();
        }
        g_Menu->pManager->shouldRenderWindow[1] = 1;
        g_Menu->pManager->playTimeDigitsActive = 1;
    } else {
        g_Menu->pManager->shouldRenderCharacterWindows[0] =
            g_Menu->pManager->shouldRenderCharacterWindows[1] =
            g_Menu->pManager->shouldRenderCharacterWindows[2] = 0;
        if (!isSubmenuOpening) {
            g_Menu->pManager->shouldRenderWindow[0] = 0;
            g_Menu->pManager->goldDigitsActive = 0;
        }
    }
    MenuRenderFrame();
}
/* PARKED 99.74% (392 bytes, retail's exact size, 98 insns, instruction multiset identical).
   The ONE difference is the register the loaded gearId takes up: retail loads it into
   a0 and keeps it there (lbu a0,780 / beq a0,s2 / addiu a0,a0,0xB in the branch delay
   slot / andi a0,a0,0xff in place), ours loads into v0 and folds the u_char truncation
   into the copy to the argument register (andi a0,v0,0xff). The frame is retail's
   exactly (0x20, ra/s2/s1/s0 only), which proves gearId is NOT live across the first
   MenuLoadCharacterNameTexture call: every one of the three uses is in place on the
   loaded value, i.e. retail has ONE quantity here where our spelling has a ternary
   result plus a separate narrowing, and local-alloc hands that block's quantities
   $5/$2/$3 and never $4, so a0 is not one of the registers it gives out here.
   Measured, all 99.74 or worse: the same ternary inlined as the call's first argument
   with the array expression written twice (99.74, byte-identical, gcc CSEs the load);
   ALL 120 declaration orders of the five locals; u_char/u32/s32 gearId; the ternary
   assigned back to the u_char local (97.65, the 0xFF arm's li is then dropped);
   an if/else writing the same local (97.76); an intermediate u_char textureId (97.65);
   an intermediate s32 + separate (u_char) cast at the call (96.73 / 388 / 97 insns,
   the ternary result still lands in v0); the cast on the arm vs on the whole ternary
   vs none; reusing the dead menuCharacterId local.
   Not a preset question: 99.74 under gcc-2.7.2 too, and the matched neighbours drop
   (MenuMovePointerCursor 94.70, MenuSetupPointerCursors 80.21), so 2.6.0 is right;
   -O1 and -O3 are far off. A cc1_probe replica of the same expression with simpler
   operands puts the value in a2, so the register is decided by the surrounding
   allocation; a -dl/-da dump of this body (round 421) puts block 6's eleven quantities
   in $2/$3/$5 only, with v0 free at the gearId load.
   Two counters are load-bearing: writing `j * 2` for the first call's second argument
   (the natural spelling) is 93.87 / 404 bytes and grows a third callee-saved counter,
   because characterIndex's u_char width is what yields sll a1,s0,1 / andi a1,a1,0xFE;
   and writing the second call's 6 + (characterIndex * 2) stops the strength
   reduction (94.28 / 400).
   A u_char intermediate with the arm cast explicit measures the same 97.65 / 388 /
   97 insns as the uncast one (the shape turns beq+j into bne and loses a word).
   Three permuter runs on this body found nothing (1650 iterations best==base, 991
   best==base, round 421 1217 iterations base 25 best 25).
   Round 439 re-measured (99.74 again) and closed the remaining spellings, all 99.74:
   the inverted ternary ((gearId != NONE) ? (u_char)(gearId + 0xB) : NONE), the mask
   form (cond ? NONE : gearId + 0xB) & 0xFF, both arms cast to u_char, and gearId
   declared inside the if block; a fourth permuter run (1360 iterations, 181 s) came
   back base=25 best=25. What those runs also show is why the register cannot be
   reached: the if/else store-back bodies (97.76 / 388 / 97 insns) DO put the load,
   the addiu and the andi all in a0 (lbu a0,780 / addiu a0,a0,0xB / andi a0,a0,0xff),
   but gcc sinks the single truncation past the branch there and the whole thing
   becomes a select (bne + li + one shared andi, no j), i.e. 97 insns - the a0 chain
   and retail's j-with-arm-truncation are mutually exclusive in this compiler, and
   every 98-insn spelling (the correct shape) allocates the ternary result to v0.
   Round 453 re-measured this body (99.74 again, 392 / 98 insns) with GEAR_ID_NONE in
   place of CHARACTER_ID_NONE and closed nothing new: the residual is still the single
   local-alloc hand-out of the ternary result ($2 where retail has $4).
   Parked body below:
*/
/*
void func_801D2D38(void) {
    s32 i;
    s32 j;
    u_char menuCharacterId;
    u_char characterIndex;
    u_char gearId;

    if (g_MenuOverlayIndex == 0) {
        for (i = 0; i < 2; i++) {
            g_Menu->windows[i] = HeapAlloc(sizeof(MenuWindow), 0);
            bzero(g_Menu->windows[i], sizeof(MenuWindow));
            g_Menu->windowParameters[i] = HeapAlloc(sizeof(MenuWindowParameters), 0);
            bzero(g_Menu->windowParameters[i], sizeof(MenuWindowParameters));
            MenuInitializeWindowGraphics(i);
        }
        MenuPlaySoundEffect(0x5E);
    }

    for (j = 0; j < 3; j++) {
        menuCharacterId = g_Menu->pManager->currentCharacterIDs[j];
        if (menuCharacterId != CHARACTER_ID_NONE) {
            characterIndex = j;
            MenuLoadCharacterNameTexture(menuCharacterId, characterIndex * 2);
            gearId = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[j]].gearId;
            MenuLoadCharacterNameTexture((u_char)((gearId == GEAR_ID_NONE) ? GEAR_ID_NONE : gearId + 0xB), 6 + (j << 1));
        }
    }
    MenuRevealSelectionMenu(8, g_MenuMainSelectionResourceIds);
    MenuTweenCharacterWindows(1, 0);
    MenuCreatePlayTimeWindow();
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D2D38);

void MenuDrawGearScreen(u_char a0, u_char a1) {
    MenuSetGearWindowContents(a0, a1);
    MenuSetGearWindowQuads(a0, a1, g_GameState.gearRide[a0]);
    MenuRenderFrame();
    MenuDrawStatTable(a0, 0, 0, a1);
    MenuRenderFrame();
    func_801D8EA4(a0, 0, 0, a1);
}
void MenuCreateWindow2Strings(u_char stringIndex) {
    MenuWindowParameters* pWindowParams;
    MenuString* pString;
    int i;
    int xPosition;

    MenuCreateWindow(2, 0x7A, 0x96, 0xBC, 0x40, 1, 1, 4, 0);
    pWindowParams = g_Menu->windowParameters[2];
    while (!pWindowParams->isOpen) {
        MenuRenderFrame();
    }
    xPosition = 0x84;

    for (i = 0; i < 4; i++) {
        g_Menu->modalStrings[i] = HeapAlloc(sizeof(MenuString), 0);
        bzero(g_Menu->modalStrings[i], sizeof(MenuString));

        // Even entries own a VRAM buffer; the odd one below them shares it
        if (!(i & 1)) {
            g_Menu->modalStrings[i]->pVramBuffer = HeapAlloc(0x5CA, 0);
            g_Menu->modalStrings[i]->vramDest.x = 320;
            g_Menu->modalStrings[i]->vramDest.y = ((i / 2) * FONT_LETTER_HEIGHT) + 78;
            g_Menu->modalStrings[i]->vramDest.w = 58;
            g_Menu->modalStrings[i]->vramDest.h = FONT_LETTER_HEIGHT;
            continue;
        }

        g_Menu->modalStrings[i]->pVramBuffer = g_Menu->modalStrings[i - 1]->pVramBuffer;
    }

    for (i = 0; i < 3; i++) {
        pString = g_Menu->modalStrings[i];
        pString->width = SystemRenderStringEntry(GetStringEntry(g_Menu->pStringBank, stringIndex + i),
            pString->pVramBuffer, 0x36, i % 2);
        MenuSetStringPrimitives(pString, i, 0, 0);
        MenuSetFt4QuadVertices(&pString->polys[g_Menu->renderContext], xPosition, (i * 16) + 0xA0, 0,
            (i / 2) * FONT_LETTER_HEIGHT + 0x4E, pString->width, FONT_LETTER_HEIGHT);
        MenuSetQuadVertices(pString->vertices, xPosition, (i * 16) + 0xA0, pString->width, FONT_LETTER_HEIGHT);
        pString->renderContext = (u_char)g_Menu->renderContext;
        pString->hasVertices = 1;
    }

    LoadImage(&g_Menu->modalStrings[0]->vramDest, g_Menu->modalStrings[0]->pVramBuffer);
    LoadImage(&g_Menu->modalStrings[2]->vramDest, g_Menu->modalStrings[2]->pVramBuffer);
    DrawSync(0);

    g_Menu->pManager->shouldRenderModalStrings = 1;
    HeapFree(g_Menu->modalStrings[0]->pVramBuffer);
    HeapFree(g_Menu->modalStrings[2]->pVramBuffer);
    MenuRenderFrame();
    MenuRenderFrame();
}
void MenuFreeWindow2Strings(void) {
    s32 i;

    if (g_Menu->pManager->shouldRenderWindow[2] != 0) {
        MenuFreeWindow(2);
        g_Menu->pManager->shouldRenderModalStrings = 0;
        for (i = 0; i < 4; i++) {
            HeapFree(g_Menu->modalStrings[i]);
        }
    }
    MenuRenderFrame();
}
void MenuCreateScrollHandle(s32 a0, s32 a1, s32 a2) {
    if (g_Menu->pManager->scrollHandleActive == 0) {
        g_Menu->pScrollHandle = HeapAlloc(0x74, 0);
        bzero(g_Menu->pScrollHandle, 0x74);
    }
    func_8002675C(g_Menu->resources, 0x107, (POLY_FT4*)g_Menu->pScrollHandle, g_Menu->renderContext, a0, a1, 0x1000);
    MenuSetQuadVertices(g_Menu->pScrollHandle->vertices, a0, a1, 8, a2);
    // the original reads renderContext as a byte here, but as a word for the arg above
    g_Menu->pScrollHandle->renderContext = (u_char)g_Menu->renderContext;
    g_Menu->pManager->scrollHandleActive = 1;
}
void MenuFreeScrollHandle(void) {
    g_Menu->pManager->scrollHandleActive = 0;
    HeapFree(g_Menu->pScrollHandle);
}
void MenuCreateShoulderButtonUi(u_char a0, u_char a1) {
    s32 i;

    if ((a1 == 0 ? g_Menu->availableCharacterCount : g_Menu->gearCharacterCount) == 1) {
        return;
    }

    if (g_Menu->pManager->isPanelActive[4] == 0) {
        g_Menu->pShoulderButtonUi = HeapAlloc(sizeof(MenuShoulderButtonUi), 0);
        bzero(g_Menu->pShoulderButtonUi, sizeof(MenuShoulderButtonUi));
        g_Menu->pManager->isPanelActive[4] = 1;
    }

    for (i = 0; i < 2; i++) {
        func_8002675C(g_Menu->resources, 0x164 + i, &g_Menu->pShoulderButtonUi->polys[i * 4], g_Menu->renderContext, g_MenuShoulderButtonArrowX[i], g_MenuShoulderButtonArrowY[a0], 0x1000);
    }

    for (i = 0; i < 4; i++) {
        MenuShoulderButtonUi* pUi = g_Menu->pShoulderButtonUi;
        POLY_FT4* pPoly = pUi->polys;
        s32 polyIndex = i * 2 + g_Menu->renderContext;
        SVECTOR* pVertices = g_Menu->pShoulderButtonUi->vertices + i * 4;

        pPoly += polyIndex;
        MenuSetQuadVertices(pVertices, pPoly->x0, pPoly->y0, pPoly->x1 - pPoly->x0, pPoly->y3 - pPoly->y0);
    }

    g_Menu->pShoulderButtonUi->renderContext = (u_char)g_Menu->renderContext;
    g_Menu->pManager->shoulderButtonUiActive = 1;
}
void MenuFreeShoulderButtonUi(void) {
    if (g_Menu->pManager->isPanelActive[4] != 0) {
        g_Menu->pManager->shoulderButtonUiActive = 0;
        g_Menu->pManager->isPanelActive[4] = 0;
        HeapFree(g_Menu->pShoulderButtonUi);
    }
}
void MenuSetStringTexturePrims(MenuString* pString, u_char index, u_char isGlyph, u_char row) {
    s32 originY;
    s32 u;
    s32 width;
    s32 v;

    MenuInitializePolyFt4(&pString->polys[g_Menu->renderContext]);
    pString->polys[g_Menu->renderContext].tpage = GetTPage(0, 0, 0x180, 0);

    // The FT4 call is duplicated per arm and only the vertices quad is shared: with
    // both shared, gcc keeps FONT_LETTER_HEIGHT in a callee-saved register across
    // the first call and the frame costs an instruction.
    if (isGlyph == 0) {
        width = 0x48;
        pString->polys[g_Menu->renderContext].clut =
            (g_Menu->pManager->currentCharacterIDs[index] & 1) ? g_SystemPalette2 : g_SystemPalette1;
        originY = D_801EA17C[row] - 0x24;
        u = g_MenuNameTextureX[index] << 2;
        v = g_MenuNameTextureY[index * 4];
        MenuSetFt4QuadVertices(&pString->polys[g_Menu->renderContext], originY, D_801EA18C[row].x, u & 0xFC, v, width, FONT_LETTER_HEIGHT);
    } else {
        width = 0x60;
        pString->polys[g_Menu->renderContext].clut =
            ((g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId + 0xB) & 1) ? g_SystemPalette2 : g_SystemPalette1;
        originY = D_801EA17C[row] - 0x30;
        u = D_801EA584[index] << 2;
        v = g_MenuGlyphSjisTable[index * 4];
        MenuSetFt4QuadVertices(&pString->polys[g_Menu->renderContext], originY, D_801EA18C[row].x, u & 0xFC, v, width, FONT_LETTER_HEIGHT);
    }
    MenuSetQuadVertices(pString->vertices, originY, D_801EA18C[row].x, width, FONT_LETTER_HEIGHT);
    pString->renderContext = (u_char)g_Menu->renderContext;
}

void MenuCreateWindow(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, u_char a6, s32 a7, u_char a8) {
    MenuWindowParameters* pParams;

    if (a0 >= 2) {
        g_Menu->windows[a0] = HeapAlloc(sizeof(MenuWindow), 0);
        bzero(g_Menu->windows[a0], sizeof(MenuWindow));
        g_Menu->windowParameters[a0] = HeapAlloc(sizeof(MenuWindowParameters), 0);
        bzero(g_Menu->windowParameters[a0], sizeof(MenuWindowParameters));
        MenuInitializeWindowGraphics(a0);
    }

    pParams = g_Menu->windowParameters[a0];
    if (a5 != 0) {
        pParams->index = a0;
        pParams->isOpen = 0;
        pParams->x = a1;
        pParams->y = a2;
        pParams->width = a3;
        pParams->height = a4;
        pParams->curWidth = 0;
        pParams->curHeight = 0;
        g_Menu->pManager->isWindowOpening[a0] = 1;
        pParams->useDefaultMatrix = a6;
        pParams->zIndex = a7;
        return;
    }

    MenuSetWindow(a0, a1, a2, a3, a4, a6, a7, a8);
}

void MenuUpdateWindowAnimations(void) {
    u_char flag;
    int i;
    MenuWindowParameters* pWindowInfo;

    for (i = 0; i < MENU_MAX_NUM_WINDOWS; i++) {
        pWindowInfo = g_Menu->windowParameters[i];
        if ((g_Menu->pManager->isWindowOpening[i]) && pWindowInfo->isOpen == 0) {
            flag = 0;

            if ((pWindowInfo->curWidth + 32) >= pWindowInfo->width) {
                pWindowInfo->curWidth = pWindowInfo->width;
                flag += 1;
            } else {
                pWindowInfo->curWidth += 32;
            }

            if ((pWindowInfo->curHeight + 32) >= pWindowInfo->height) {
                pWindowInfo->curHeight = pWindowInfo->height;
                flag += 1;
            } else {
                pWindowInfo->curHeight += 32;
            }

            if (flag == 2) {
                pWindowInfo->isOpen = 1;
            }

            MenuSetWindow(
                pWindowInfo->index,
                (pWindowInfo->x + (pWindowInfo->width / 2)) - (pWindowInfo->curWidth / 2),
                (pWindowInfo->y + (pWindowInfo->height / 2)) - (pWindowInfo->curHeight / 2),
                pWindowInfo->curWidth,
                pWindowInfo->curHeight,
                pWindowInfo->useDefaultMatrix,
                pWindowInfo->zIndex,
                pWindowInfo->hasScrollBar
            );
        }
    }
}
