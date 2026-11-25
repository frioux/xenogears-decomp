#include "common.h"
#include "system/menu.h"
#include "system/controller.h"
#include "system/archive.h"
#include "system/graphics.h"
#include "system/debug.h"
#include "util/copyspec.h"
#include "gear_shop/gear_shop.h"
#include "libgear/main.h"

// Menu resource ids: the first texture of a glyph sequence that is stepped one glyph per frame
#define MENU_TEX_SPARKLE_GLYPH_0 0x167
#define MENU_TEX_OPTION_HIGHLIGHT_GLYPH_0 0x172

// Menu resource id of the 'E' drawn over the portrait of a character that can equip the item
#define MENU_TEX_EQUIP_MARK 0xE

// Font glyph code of '0'; a digit n is glyph code MENU_GLYPH_DIGIT_0 + n
#define MENU_GLYPH_DIGIT_0 0x10

// Font glyph code drawn in a digit cell that holds no digit
#define MENU_GLYPH_BLANK 0xC3

// Stat label glyphs drawn across the HP, fuel and weight rows of the gear panel
#define MENU_GEAR_STAT_LABEL_GLYPH_COUNT 14

// Sparkle glyphs drawn in the row of sparkles behind the gear panel
#define MENU_SPARKLE_GLYPH_COUNT 3

// Weapon ids below this equip into weapon slot 0, the rest into an extra weapon slot
#define MENU_GEAR_EXTRA_WEAPON_ID_MIN 50

// Fuel is tuned in units of 100 fuel points, each costing 10 GP
#define MENU_FUEL_UNIT_AMOUNT 100
#define MENU_FUEL_UNIT_PRICE 10

// MenuUnk8.optionMarkerStates values
#define MENU_MARKER_STATE_IDLE 0
#define MENU_MARKER_STATE_HIGHLIGHTING 1
#define MENU_MARKER_STATE_LOOPING 2
#define MENU_MARKER_STATE_RETURNING 3

// MenuUnk8.optionHighlightState values
#define MENU_HIGHLIGHT_STATE_IDLE 0
#define MENU_HIGHLIGHT_STATE_HIGHLIGHTING 1
#define MENU_HIGHLIGHT_STATE_LOOPING 2
#define MENU_HIGHLIGHT_STATE_REVERSING 3
#define MENU_HIGHLIGHT_STATE_RETURNING 4

// MenuManager.finalPriceRenderState values
#define MENU_FINAL_PRICE_OFF 0
#define MENU_FINAL_PRICE_DRAWN 1
#define MENU_FINAL_PRICE_BUILT 2

extern s32 g_GfxOTDepthShift;
extern s32 g_MenuGfxEnvOtBuffer0[2];

s32 GearShopMenuGetGearEquipFlags(u8);
void GearShopMenuComputeTransitionSteps();
void GearShopMenuUpdateAndRender();

// TODO: remove once matched
s32 func_801D2B74(s32, s32, u8*);

u8 GameRandomByteRange(int, u8);

void GearShopMenuShoulderButtonUiInitialize(void) {
    int i;
    int polyIndex;

    for(i = 0; i < 2; i++) {
        func_8002675C(
            g_Menu->resources,
            MENU_TEX_L1_RED_ARROW + i,
            &g_Menu->pShoulderButtonUi->polys[i * 4],
            g_Menu->renderContext,
            g_GearShopShoulderButtonX[i],
            0x64,
            0x1000
        );
    }

    for(i = 0; i < 4; i++) {
        polyIndex = (i * 2 + g_Menu->renderContext);
        GearShopMenuSetVertices(
            &g_Menu->pShoulderButtonUi->vertices[i * 4],
            g_Menu->pShoulderButtonUi->polys[polyIndex].x0,
            g_Menu->pShoulderButtonUi->polys[polyIndex].y0,
            g_Menu->pShoulderButtonUi->polys[polyIndex].x1 - g_Menu->pShoulderButtonUi->polys[polyIndex].x0,
            g_Menu->pShoulderButtonUi->polys[polyIndex].y3 - g_Menu->pShoulderButtonUi->polys[polyIndex].y0
        );
    }

    g_Menu->pShoulderButtonUi->renderContext = g_Menu->renderContext;
}

void GearShopMenuShoulderButtonUiFree(void) {
    g_Menu->pManager->shoulderButtonUiActive = FALSE;
    GearShopMenuUpdateAndRender();
    HeapFree(g_Menu->pShoulderButtonUi);
}

void GearShopMenuRenderShopUi(void) {
    int i;

    if (g_Menu->pManager->shouldRenderShop) {
        for(i = 0; i < 9; i++) {
            if (g_Menu->pShop->shouldRenderPortraitHighlight[i]) {
                AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->linesPortraitHighlight1[(i*2) + g_Menu->renderContext]);
                AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->linesPortraitHighlight2[(i*2) + g_Menu->renderContext]);
            }
        }

        GearShopMenuRenderString(g_Menu->pShop->explanationsLen, g_Menu->pShop->polysExplanations, g_Menu->pShop->explanationsRenderCtx);
        GearShopMenuRenderString(g_Menu->pShop->equipMarkPrimCount, g_Menu->pShop->polys2D0, g_Menu->pShop->equipMarkRenderCtx);
        GearShopMenuRenderString(g_Menu->pShop->numPortraits, g_Menu->pShop->polysCharacterPortraits, g_Menu->pShop->portraitsRenderCtx);
        for(i = 0; i < SHOP_MAX_ITEMS_IN_VIEW; i++) {
            if (g_Menu->pShop->shouldRenderItemRows[i]) {
                GearShopMenuRenderPolygons(1, g_Menu->pShop->strItemNames[i].vertices, g_Menu->pShop->strItemNames[i].polys, g_Menu->pShop->strItemNames[i].renderContext);
                GearShopMenuRenderPolygons(1, g_Menu->pShop->strItemQuantities[i].vertices, g_Menu->pShop->strItemQuantities[i].polys, g_Menu->pShop->strItemQuantities[i].renderContext);
            }
        }
        if (g_Menu->pShop->shouldRenderStr4530) {
            GearShopMenuRenderPolygons(1, g_Menu->pShop->str4530.vertices, g_Menu->pShop->str4530.polys, g_Menu->pShop->str4530.renderContext);
        }

        if (g_Menu->pShop->shouldRenderItemDesc) {
            GearShopMenuRenderPolygons(1, g_Menu->pShop->strItemDesc.vertices, g_Menu->pShop->strItemDesc.polys, g_Menu->pShop->strItemDesc.renderContext);
        }

        if (g_Menu->pShop->shouldRenderStr44B0) {
            GearShopMenuRenderPolygons(1, g_Menu->pShop->str44B0.vertices, g_Menu->pShop->str44B0.polys, g_Menu->pShop->str44B0.renderContext);
        }

        if (g_Menu->pShop->shouldRenderStr45B0 != 0) {
            GearShopMenuRenderPolygons(1, g_Menu->pShop->str45B0.vertices, g_Menu->pShop->str45B0.polys, g_Menu->pShop->str45B0.renderContext);
        }

        if (g_Menu->pShop->shouldRenderPrices != 0) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->lines3BF0[g_Menu->renderContext]);
            GearShopMenuRenderString(g_Menu->pShop->goldBeforeStrLen, g_Menu->pShop->polysGoldBefore, g_Menu->pShop->goldBeforeRenderCtx);
            GearShopMenuRenderString(g_Menu->pShop->totalPriceStrLen, g_Menu->pShop->polysTotalPrice, g_Menu->pShop->totalPriceRenderCtx);
            GearShopMenuRenderString(g_Menu->pShop->goldAfterStrLen, g_Menu->pShop->polysGoldAfter, g_Menu->pShop->goldAfterRenderCtx);
            if (g_Menu->pShop->shouldRenderSalesTotal != 0) {
                GearShopMenuRenderString(g_Menu->pShop->salesTotalPrimCount, g_Menu->pShop->polysSalesTotal, g_Menu->pShop->salesTotalRenderCtx);
            }
        }

        for(i = 0; i < SHOP_MAX_ITEMS_IN_VIEW; i++) {
            GearShopMenuRenderString(g_Menu->pShop->inventoryPrimCounts[i], g_Menu->pShop->polysShopInventory[i*4], g_Menu->pShop->inventoryRenderCtxs[i]);
        }

        for(i = 0; i < 9; i++) {
            GearShopMenuRenderString(g_Menu->pShop->polys27B0PrimCounts[i], g_Menu->pShop->polys27B0[i], g_Menu->pShop->polys27B0RenderCtxs[i]);
            GearShopMenuRenderString(g_Menu->pShop->polys3020PrimCounts[i], g_Menu->pShop->polys3020[i], g_Menu->pShop->polys3020RenderCtxs[i]);
        }
    }
    if (g_Menu->pManager->finalPriceRenderState == MENU_FINAL_PRICE_DRAWN) {
        GearShopMenuRenderString(g_Menu->pShop->finalPriceStrLen, g_Menu->pShop->polysFinalPrice, g_Menu->pShop->finalPriceRenderCtx);
    }
    if (g_Menu->pManager->isPanelActive[1] != 0) {
        GearShopMenuRenderString(g_Menu->pShop->goldAmountPrimCount, g_Menu->pShop->polysGoldAmount, g_Menu->pShop->goldAmountRenderCtx);
    }
}


/*
 This is the function in gear_shop_menu that calls into LibGear
 */
void GearShopMenuRenderGearModel(void) {
    // most likely: if helper library is loaded into memory
    if (g_Menu->pManager->isPanelActive[0]) {
        LibGearRenderFrame(&g_Menu->modelMatrix, &g_Menu->viewMatrix, g_Menu->pGfxEnv->otBuffer, g_Menu->renderContext);
    }
}

void GearShopMenuRenderGearPanelStrings(void) {
    s32 i;

    if (g_Menu->pManager->isPanelActive[2] != 0) {
        if (g_Menu->menuUnk8->sparklesActive != 0) {
            GearShopMenuRenderString(g_Menu->menuUnk8->sparklePrimCount, g_Menu->menuUnk8->polysD0, g_Menu->menuUnk8->sparkleRenderContext);
            GearShopMenuRenderString(g_Menu->menuUnk8->sparklePrimCount, &g_Menu->menuUnk8->polysD0[4], g_Menu->menuUnk8->sparkleRenderContext);
            GearShopMenuRenderString(g_Menu->menuUnk8->sparklePrimCount, &g_Menu->menuUnk8->polysD0[8], g_Menu->menuUnk8->sparkleRenderContext);
        }
        for(i = 0; i < 2; i++) {
            if (g_Menu->menuUnk8->optionMarkerStates[i] != 0) {
                GearShopMenuRenderString(g_Menu->menuUnk8->optionMarkerPrimCounts[i], &g_Menu->menuUnk8->polys2B0[(i*0x16)], g_Menu->menuUnk8->optionMarkerRenderContexts[i]);
            }
        }
        if (g_Menu->menuUnk8->optionHighlightState != 0) {
            GearShopMenuRenderString(g_Menu->menuUnk8->optionHighlightPrimCount, g_Menu->menuUnk8->polysD00, g_Menu->menuUnk8->optionHighlightRenderContext);
        }
    }
    if (g_Menu->pManager->isPanelActive[3] != 0) {
        GearShopMenuRenderString(1U, g_Menu->menuUnk8->unk0.polys, g_Menu->renderContext);
        GearShopMenuRenderString(MENU_GEAR_STAT_LABEL_GLYPH_COUNT, g_Menu->menuUnk8->polys12A0, g_Menu->menuUnk8->gearStatStringsRenderContext);
        GearShopMenuRenderString(g_Menu->menuUnk8->gearHpStrLen, g_Menu->menuUnk8->polys1700, g_Menu->menuUnk8->gearStatStringsRenderContext);
        GearShopMenuRenderString(g_Menu->menuUnk8->gearMaxHpStrLen, g_Menu->menuUnk8->polys1890, g_Menu->menuUnk8->gearStatStringsRenderContext);
        GearShopMenuRenderString(g_Menu->menuUnk8->gearFuelStrLen, g_Menu->menuUnk8->polys1A20, g_Menu->menuUnk8->gearStatStringsRenderContext);
        GearShopMenuRenderString(g_Menu->menuUnk8->gearMaxFuelStrLen, g_Menu->menuUnk8->polys1BB0, g_Menu->menuUnk8->gearStatStringsRenderContext);
        GearShopMenuRenderString(g_Menu->menuUnk8->gearWeightStrLen, g_Menu->menuUnk8->polys1D40, g_Menu->menuUnk8->gearStatStringsRenderContext);
    }
}

void GearShopMenuAnimateOptionMarkers(void) {
    u16 coords[2][4];
    u8 needsUpdate;
    s32 i;
    s32 k;

    for(i = 0; i < 2; i++) {
        needsUpdate = FALSE;
        if (g_Menu->menuUnk8->optionMarkerStates[i] != 0) {
            switch(g_Menu->menuUnk8->optionMarkerStates[i]) {
            case MENU_MARKER_STATE_HIGHLIGHTING:
                coords[0][i] = g_GearShopOptionMarkerX[i];
                coords[1][i] = g_GearShopOptionMarkerY[(i * 6) + g_GearShopSubmenuRowIndex[(g_Menu->mainMenuChoice << 2) + g_Menu->subMenuChoice]];
                needsUpdate = TRUE;
                break;
            case MENU_MARKER_STATE_LOOPING:
                if (GameRandomByteRange(0, 0xFF) < 0x10) {
                    coords[0][i] = g_GearShopOptionMarkerX[i];
                    coords[1][i] = g_GearShopOptionMarkerY[(i * 6) + GameRandomByteRange(0, 5)];
                    needsUpdate = TRUE;
                }
                break;
            }

            if (needsUpdate) {
                g_Menu->menuUnk8->optionMarkerX[i] = coords[0][i];
                g_Menu->menuUnk8->optionMarkerY[i] = coords[1][i];
            }

            if (g_Menu->menuUnk8->optionMarkerTimers[i] != 0) {
                g_Menu->menuUnk8->optionMarkerTimers[i]--;
                continue;
            }

            g_Menu->menuUnk8->optionMarkerPrimCounts[i] = 0;
            for(k = 0; k < 4; k++) {
                if (g_GearShopOptionMarkerGlyphs[(i * 0x14) + ((g_Menu->menuUnk8->optionMarkerGlyphIndexes[i] << 2) + k)] != MENU_RESOURCE_NONE) {
                    g_Menu->menuUnk8->optionMarkerPrimCounts[i] += func_8002675C(
                        g_Menu->resources,
                        g_GearShopOptionMarkerGlyphs[(i * 0x14) + ((g_Menu->menuUnk8->optionMarkerGlyphIndexes[i] << 2) + k)],
                        &g_Menu->menuUnk8->polys2B0[(i * 0x16) + (g_Menu->menuUnk8->optionMarkerPrimCounts[i] * 2)],
                        g_Menu->renderContext,
                        g_Menu->menuUnk8->optionMarkerX[i],
                        g_Menu->menuUnk8->optionMarkerY[i],
                        0x1000
                    );
                }
            }

            g_Menu->menuUnk8->optionMarkerRenderContexts[i] = g_Menu->renderContext;

            switch(g_Menu->menuUnk8->optionMarkerStates[i]) {
            case MENU_MARKER_STATE_HIGHLIGHTING:
                if (++g_Menu->menuUnk8->optionMarkerGlyphIndexes[i] == 4) {
                    g_Menu->menuUnk8->optionMarkerStates[i] = MENU_MARKER_STATE_LOOPING;
                }
                g_Menu->menuUnk8->optionMarkerTimers[i] = 2;
                break;
            case MENU_MARKER_STATE_LOOPING:
                if (++g_Menu->menuUnk8->optionMarkerGlyphIndexes[i] >= 5) {
                    g_Menu->menuUnk8->optionMarkerGlyphIndexes[i] = 3;
                }
                g_Menu->menuUnk8->optionMarkerTimers[i] = 8;
                break;
            case MENU_MARKER_STATE_RETURNING:
                if (--g_Menu->menuUnk8->optionMarkerGlyphIndexes[i] < 0) {
                    g_Menu->menuUnk8->optionMarkerStates[i] = MENU_MARKER_STATE_IDLE;
                    g_Menu->menuUnk8->sparklesActive = 0;
                }
                g_Menu->menuUnk8->optionMarkerTimers[i] = 2;
                break;
            }
        }
    }
}

void GearShopMenuAnimateOptionHighlight(void) {
    u8 needsUpdate;
    u16 var_s1;
    u16 var_s2;

    if (g_Menu->menuUnk8->optionHighlightState) {
        // update timer?
        if (g_Menu->menuUnk8->optionHighlightTimer != 0) {
            (s8)g_Menu->menuUnk8->optionHighlightTimer -= 1;
            return;
        }

        needsUpdate = FALSE;
        switch(g_Menu->menuUnk8->optionHighlightState) {
            case MENU_HIGHLIGHT_STATE_HIGHLIGHTING:
                var_s2 = g_GearShopOptionHighlightX[g_GearShopSubmenuRowIndex[(g_Menu->mainMenuChoice * 4) + g_Menu->subMenuChoice]];
                var_s1 = g_GearShopOptionHighlightY[g_GearShopSubmenuRowIndex[(g_Menu->mainMenuChoice * 4) + g_Menu->subMenuChoice]];
                needsUpdate = TRUE;
                break;
            case MENU_HIGHLIGHT_STATE_LOOPING:
                if(GameRandomByteRange(0, 0xFF) < 4) {
                    var_s2 = g_GearShopOptionHighlightX[GameRandomByteRange(0, 5)];
                    var_s1 = g_GearShopOptionHighlightY[GameRandomByteRange(0, 5)];
                    needsUpdate = TRUE;
                }
                break;
        }

        if(needsUpdate) {
            g_Menu->menuUnk8->optionHighlightX = var_s2;
            g_Menu->menuUnk8->optionHighlightY = var_s1;
        }

        g_Menu->menuUnk8->optionHighlightPrimCount = func_8002675C(g_Menu->resources,
                      g_Menu->menuUnk8->optionHighlightGlyphIndex + MENU_TEX_OPTION_HIGHLIGHT_GLYPH_0,
                      g_Menu->menuUnk8->polysD00,
                      g_Menu->renderContext,
                      g_Menu->menuUnk8->optionHighlightX,
                      g_Menu->menuUnk8->optionHighlightY,
                      0x1000);

        g_Menu->menuUnk8->optionHighlightRenderContext = (u8) g_Menu->renderContext;
        g_Menu->menuUnk8->optionHighlightTimer = 1;

        switch (g_Menu->menuUnk8->optionHighlightState) {
        case MENU_HIGHLIGHT_STATE_HIGHLIGHTING:
            if (++g_Menu->menuUnk8->optionHighlightGlyphIndex >= 3) {
                g_Menu->menuUnk8->optionHighlightState = MENU_HIGHLIGHT_STATE_LOOPING;
            }
            break;
        case MENU_HIGHLIGHT_STATE_LOOPING:
            if (++g_Menu->menuUnk8->optionHighlightGlyphIndex >= 11) {
                g_Menu->menuUnk8->optionHighlightGlyphIndex = 3;
            }
            break;
        case MENU_HIGHLIGHT_STATE_REVERSING:
            g_Menu->menuUnk8->optionHighlightGlyphIndex = 2;
            g_Menu->menuUnk8->optionHighlightState = MENU_HIGHLIGHT_STATE_RETURNING;
            break;
        case MENU_HIGHLIGHT_STATE_RETURNING:
            if (--g_Menu->menuUnk8->optionHighlightGlyphIndex < 0) {
                g_Menu->menuUnk8->optionHighlightState = MENU_HIGHLIGHT_STATE_IDLE;
            }
            break;
        }
    }
}

void GearShopMenuAnimatePanelSparkles(void) {
    MenuUnk8* pMenu;
    int i;
    short t1, t2;

    pMenu = g_Menu->menuUnk8;

    if (pMenu->sparklesActive) {
        if(pMenu->sparkleTimer) {
            g_Menu->menuUnk8->sparkleTimer--;
        } else {
            if(GameRandomByteRange(0, 0xFF) < 8) {
                t1 = g_GearShopSparkleX[GameRandomByteRange(0, 5)];
                t2 = g_GearShopSparkleY[GameRandomByteRange(0, 5)];
                g_Menu->menuUnk8->sparkleX = t1;
                g_Menu->menuUnk8->sparkleY = t2;
            }

            for(i = 0; i < MENU_SPARKLE_GLYPH_COUNT; i++) {
                g_Menu->menuUnk8->sparklePrimCount = func_8002675C(
                    g_Menu->resources,
                    g_Menu->menuUnk8->sparkleGlyphIndex + MENU_TEX_SPARKLE_GLYPH_0,
                    &g_Menu->menuUnk8->polysD0[i * 4],
                    g_Menu->renderContext,
                    g_Menu->menuUnk8->sparkleX + i * 0x8,
                    g_Menu->menuUnk8->sparkleY + i * 0xA,
                    0x1000
                );
            }

            g_Menu->menuUnk8->sparkleGlyphIndex ^= 1;
            g_Menu->menuUnk8->sparkleRenderContext = g_Menu->renderContext;
            g_Menu->menuUnk8->sparkleTimer = 4;
        }
    }
}

void GearShopMenuUpdateGearPanelAnimations(void) {
    if (g_Menu->pManager->isPanelActive[2]) {
        GearShopMenuAnimateOptionMarkers();
        GearShopMenuAnimateOptionHighlight();
        GearShopMenuAnimatePanelSparkles();
    }
}

// Render gear name
void GearShopMenuLoadGearNameTexture(u_char stringIndex) {
    RECT rect;

    g_GearShopGearNameBuffer = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0x0);
    bzero(g_GearShopGearNameBuffer, MENU_STRING_IMAGE_BUFFER_SIZE);
    SystemRenderStringEntry(&g_GameState.strings[stringIndex], g_GearShopGearNameBuffer, 0x24, 0);
    setRECT(&rect, 0x180, 0x48, 0x28, FONT_LETTER_HEIGHT);
    LoadImage(&rect, (u32* ) g_GearShopGearNameBuffer);
    DrawSync(0);
    HeapFree(g_GearShopGearNameBuffer);
}

// Render gear stats (Fuel, HP, Weight)
// Resource IDs for text:
// 0x11, 0x19 = HP
// 0xF, 0x1E, 0xE, 0x15 = FUEL
// 0x20, 0xE, 0x12, 0x10, 0x11, 0x1D = WEIGHT
void GearShopMenuBuildGearStatsGraphics(void) {
    int i, j;

    for (j = 0; j < MENU_GEAR_STAT_LABEL_GLYPH_COUNT; j++) {
        func_8002675C(
            g_Menu->resources,
            g_GearShopGearStatLabelGlyphs[j], // Resource ID
            &g_Menu->menuUnk8->polys12A0[j * 2],
            g_Menu->renderContext,
            g_GearShopGearStatLabelX[j], g_GearShopGearStatLabelY[j],
            0x1000
        );
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].hp);
    g_Menu->menuUnk8->gearHpStrLen = 0;
    for (i = 0, j = 0; i < 5; i++) {

        if (g_Menu->digits[i + 4] != MENU_DIGIT_NONE) {
            g_Menu->menuUnk8->gearHpStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 4],
                &g_Menu->menuUnk8->polys1700[g_Menu->menuUnk8->gearHpStrLen * 2],
                g_Menu->renderContext,
                g_GearShopGearHpX + (i * 8), g_GearShopGearHpY,
                0x1000
            );
        }
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].maxHp);
    g_Menu->menuUnk8->gearMaxHpStrLen = 0;
    for (i = 0, j = 0; i < 5; i++) {
        if (g_Menu->digits[i + 4] != MENU_DIGIT_NONE) {
            g_Menu->menuUnk8->gearMaxHpStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 4],
                &g_Menu->menuUnk8->polys1890[g_Menu->menuUnk8->gearMaxHpStrLen * 2],
                g_Menu->renderContext,
                g_GearShopGearMaxHpX + (j * 8), g_GearShopGearMaxHpY,
                0x1000
            );
            j++;
        }
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].fuel);
    g_Menu->menuUnk8->gearFuelStrLen = 0;
    for (i = 0, j = 0; i < 4; i++) {
        if (g_Menu->digits[i + 5] != MENU_DIGIT_NONE) {
            g_Menu->menuUnk8->gearFuelStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 5],
                &g_Menu->menuUnk8->polys1A20[g_Menu->menuUnk8->gearFuelStrLen * 2],
                g_Menu->renderContext,
                g_GearShopGearFuelX + (i * 8), g_GearShopGearFuelY,
                0x1000
            );
        }
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].maxFuel);
    g_Menu->menuUnk8->gearMaxFuelStrLen = 0;
    for (i = 0, j = 0; i < 4; i++) {
        if (g_Menu->digits[i + 5] != MENU_DIGIT_NONE) {
            g_Menu->menuUnk8->gearMaxFuelStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 5],
                &g_Menu->menuUnk8->polys1BB0[g_Menu->menuUnk8->gearMaxFuelStrLen * 2],
                g_Menu->renderContext,
                g_GearShopGearMaxFuelX + (j * 8), g_GearShopGearMaxFuelY,
                0x1000
            );
            j++;
        }
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].weight);
    g_Menu->menuUnk8->gearWeightStrLen = 0;
    for (i = 0; i < 5; i++) {
        if (g_Menu->digits[i + 4] != MENU_DIGIT_NONE) {
            g_Menu->menuUnk8->gearWeightStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 4],
                &g_Menu->menuUnk8->polys1D40[g_Menu->menuUnk8->gearWeightStrLen * 2],
                g_Menu->renderContext,
                g_GearShopGearWeightX + (i * 8), g_GearShopGearWeightY,
                0x1000
            );
        }
    }

    // Render the gear name
    GearShopMenuLoadGearNameTexture((g_gearShopCurrentGearId + 0xB));

    g_Menu->menuUnk8->gearStatStringsRenderContext = g_Menu->renderContext;
}

void GearShopMenuLoadGearModelFiles(u_char gearId, u8 arg1) {
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);

    g_Menu->pGearModelArchiveData[arg1]->pModelSource = HeapAlloc(ArchiveDecodeAlignedSize(g_GearShopGearModelFileIds[gearId]), 0);
    ArchiveReadFileToBuffer(g_GearShopGearModelFileIds[gearId], g_Menu->pGearModelArchiveData[arg1]->pModelSource, 0, 0x80);
    ArchiveCdDataSync(0);

    g_Menu->pGearModelArchiveData[arg1]->pModelArchive = HeapAlloc(ArchiveDecodeAlignedSize(g_GearShopGearModelFileIds[gearId] + 1), 0);
    ArchiveReadFileToBuffer(g_GearShopGearModelFileIds[gearId] + 1, g_Menu->pGearModelArchiveData[arg1]->pModelArchive, 0, 0x80);
    ArchiveCdDataSync(0);

    ArchiveSetIndex(ARCHIVE_DIR_MENUS, 0);
}

void GearShopMenuCreateGearModel(u8 modelIndex, u8 gearId) {
    u8 characterId = 0;
    s32 s3 = modelIndex;
    s3 &= 0xFF;

    func_801E742C(
        modelIndex,
        0,
        g_Menu->pGearModelArchiveData[modelIndex]->pModelSource,
        g_Menu->pGearModelArchiveData[modelIndex]->pModelArchive,
        (modelIndex * 64) + 0x200,
        0,
        0,
        (s3 & 0xFF) + 0x1C0,
        &g_Menu->pGearModelArchiveData[modelIndex]->position
    );

    g_LibGearModels[modelIndex]->ground = g_GearShopGearModelGround[gearId];
    g_LibGearModels[modelIndex]->scale = g_GearShopGearModelScale[gearId];
    g_LibGearModels[1]->pSkeleton->rotation.vy -= 0x400;
    g_LibGearModels[1]->pSkeleton->rotation.vx -= 0x20;
    g_GfxOTDepthShift = 0;

    if(gearId != GEAR_ID_NONE) {
        characterId = g_GearShopGearAnimationIds[gearId];
    }
    LibGearSetModelAnimation((s3 & 0xFF), 0, characterId );
    HeapFree(g_Menu->pGearModelArchiveData[modelIndex]->pModelArchive);
    g_Menu->pGearModelArchiveData[modelIndex]->isModelCreated = 1;
    GearShopMenuBuildGearStatsGraphics();
    g_Menu->pManager->isPanelActive[3] = 1;
}

void GearShopMenuOpenGearPanel(void) {
    s32 temp;
    g_gearShopTransitionState.start.cameraX = g_gearShopTransitionState.target.cameraX;
    g_gearShopTransitionState.start.cameraY = g_gearShopTransitionState.target.cameraY;
    g_gearShopTransitionState.start.gearY   = g_gearShopTransitionState.target.gearY;
    g_gearShopTransitionState.target.cameraX = g_GearShopGearCameraX[g_gearShopCurrentGearId];
    temp = ((g_Menu->mainMenuChoice + (g_gearShopCurrentGearId * 3)) * 4) + g_Menu->subMenuChoice;
    g_gearShopTransitionState.target.cameraY = g_GearShopGearCameraY[temp];
    g_gearShopTransitionState.target.gearY = g_GearShopGearRotationY[(g_Menu->mainMenuChoice * 4) + g_Menu->subMenuChoice];
    GearShopMenuComputeTransitionSteps();
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION|MENU_CLOSE_ANIMATION|MENU_CLOSE_ANIMATION_START;
    g_Menu->menuUnk8->sparklesActive = 1;
    g_Menu->menuUnk8->optionMarkerStates[0] = MENU_MARKER_STATE_HIGHLIGHTING;
    g_Menu->menuUnk8->optionMarkerStates[1] = MENU_MARKER_STATE_HIGHLIGHTING;
    g_Menu->menuUnk8->optionHighlightState = MENU_HIGHLIGHT_STATE_HIGHLIGHTING;
    g_Menu->menuUnk8->sparkleTimer = 0;
    g_Menu->menuUnk8->optionMarkerTimers[0] = 0;
    g_Menu->menuUnk8->optionMarkerTimers[1] = 0;
    g_Menu->menuUnk8->optionHighlightTimer = 0;
    g_Menu->menuUnk8->sparkleGlyphIndex = 0;
    g_Menu->menuUnk8->optionMarkerGlyphIndexes[0] = 0;
    g_Menu->menuUnk8->optionMarkerGlyphIndexes[1] = 0;
    g_Menu->menuUnk8->optionHighlightGlyphIndex = 0;
    g_Menu->menuUnk8->sparkleX = 0x18;
    g_Menu->menuUnk8->sparkleY = 0x6E;
    g_Menu->pManager->isPanelActive[2] = 1;

    while (g_Menu->menuUnk8->optionMarkerStates[0] != MENU_MARKER_STATE_LOOPING) {
        GearShopMenuUpdateAndRender();
    }

    while (g_Menu->menuUnk8->optionHighlightState != MENU_HIGHLIGHT_STATE_LOOPING) {
        GearShopMenuUpdateAndRender();
    }
}

void GearShopMenuCloseGearPanel(void) {
    g_Menu->menuUnk8->optionMarkerStates[0] = MENU_MARKER_STATE_RETURNING;
    g_Menu->menuUnk8->optionMarkerStates[1] = MENU_MARKER_STATE_RETURNING;
    g_Menu->menuUnk8->optionHighlightState = MENU_HIGHLIGHT_STATE_REVERSING;
    g_Menu->menuUnk8->sparkleTimer = 0;
    g_Menu->menuUnk8->optionMarkerTimers[0] = 0;
    g_Menu->menuUnk8->optionMarkerTimers[1] = 0;
    g_Menu->menuUnk8->optionHighlightTimer = 0;
    g_Menu->pManager->isPanelActive[2] = 1;

    pushXYZ(
         0x400, 0, -0x400,
        &g_gearShopTransitionState.target.cameraX,
        &g_gearShopTransitionState.target.cameraY,
        &g_gearShopTransitionState.target.gearY,
        &g_gearShopTransitionState.start.cameraX,
        &g_gearShopTransitionState.start.cameraY,
        &g_gearShopTransitionState.start.gearY

    );

    GearShopMenuComputeTransitionSteps();
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION|MENU_CLOSE_ANIMATION|MENU_CLOSE_ANIMATION_START;
}

void GearShopMenuSetStatDeltaColor(int count, POLY_FT4 *pPolys, u_char mode) {
    int i;

    for(i = 0; i < count; i++) {
        SetShadeTex(&pPolys[(i*2)+g_Menu->renderContext], 0);
        switch(mode) {
            case 0:
                setRGB0(&pPolys[(i*2)+g_Menu->renderContext], 0x80, 0x40, 0x40);
                break;
            case 1:
                setRGB0(&pPolys[(i*2)+g_Menu->renderContext], 0x40, 0x40, 0x80);
                break;
        }
    }
}

void GearShopMenuUpdateCharacterPortraits(void) {
    int i;
    int characterIndex;
    int numCharacters;

    g_Menu->pManager->shouldRenderShop = 1;

    // This loop seems horribly inefficient, because for each character index, all
    // portraits are recomputed and the entire menu redrawn...
    for (i = 1; i < MAX_GAME_CHARACTERS + 1; i++) {
        g_Menu->pShop->numPortraits = 0;

        numCharacters = 0;
        for (characterIndex = 0; characterIndex < i; characterIndex++) {
            if (g_Menu->availableCharacters[characterIndex]) {
                g_Menu->pShop->numPortraits += func_8002675C(
                    g_Menu->resources, characterIndex + MENU_TEX_CHARACTER_PORTRAITS_SMALL,
                    &g_Menu->pShop->polysCharacterPortraits[numCharacters * 2],
                    g_Menu->renderContext,
                    g_GearShopCharacterPortraitX[numCharacters], 166,
                    0x1000
                );
                numCharacters++;
            }
        }

        g_Menu->pShop->portraitsRenderCtx = g_Menu->renderContext;
        GearShopMenuUpdateAndRender();
    }
}

void GearShopMenuSetAvailableCharacterCount(void) {
    int i;

    for(i = 0; i < MAX_GAME_CHARACTERS; i++) {
        if(g_Menu->availableCharacters[i]) {
            g_gearShopAvailableCharacterCount += 1;
        }
    }
}

void GearShopMenuChangeCurrentGear(u_char mode) {
    int characterIndex;
    int index;
    int tmp;

    if ((g_Menu->transitionEffectState == MENU_ANIMATION_DONE) && (g_gearShopAvailableCharacterCount >= 2)) {
        // Cycle to next or previous character
        index = g_gearShopCurCharacterIndex;
        if (mode == MENU_CYCLE_TO_NEXT) {
            index++;
            if (index >= g_gearShopAvailableCharacterCount) {
                index = 0;
            }
        } else {
            index--;
            if (index < 0) {
                index = g_gearShopAvailableCharacterCount - 1;
            }
        }
        g_gearShopCurCharacterIndex = index;

        index++;
        for (characterIndex = -1; index != 0; characterIndex++) {
            if (g_Menu->availableCharacters[++characterIndex]) {
                index--;
            }
            characterIndex--;
        }

        GearShopMenuUpdateAndRender();
        g_Menu->pGearModelArchiveData[1]->isModelCreated = 0;
        LibGearFreeModel(1);
        GearShopMenuUpdateAndRender();
        GearShopMenuLoadGearModelFiles(g_GameState.characters[characterIndex].gearId, 1);
        g_gearShopCurrentGearId = g_GameState.characters[characterIndex].gearId;
        GearShopMenuCreateGearModel(1, g_GameState.characters[characterIndex].gearId);
        GearShopMenuUpdateAndRender();
    }
}

// GearShopMenuUpdate ??? MenuExplanationGraphics
void GearShopMenuBuildPurchaseExplanations(u8 renderContext) {
    int i;

    g_Menu->pShop->explanationsLen = 0;
    for(i = 0; i < 4; i++) {
        g_Menu->pShop->explanationsLen += func_8002675C(
            g_Menu->resources,
            g_GearShopPurchaseExplanationGlyphs[(renderContext*4) + i],
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen*2],
            g_Menu->renderContext,
            g_GearShopPurchaseExplanationX[(renderContext*4) + i],
            g_GearShopPurchaseExplanationY[(renderContext*4) + i],
            0x1000
        );

    }

    g_Menu->pShop->explanationsRenderCtx = g_Menu->renderContext;
}

// GearShopMenuUpdate ??? MenuExplanationGraphics
void GearShopMenuBuildSellExplanations(void) {
    int i;

    g_Menu->pShop->explanationsLen = 0;

    for(i = 0; i < 2; i++) {
        g_Menu->pShop->explanationsLen += func_8002675C(
            g_Menu->resources,
            g_GearShopSellExplanationGlyphs[i],
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen*2],
            g_Menu->renderContext,
            g_GearShopSellExplanationX[i],
            g_GearShopSellExplanationY[i],
            0x1000
        );
    }

    g_Menu->pShop->explanationsRenderCtx = g_Menu->renderContext;
}

void GearShopMenuBuildTransactionSummary(u32 startingBalance, u32 totalPrice, u32 newBalance, u32 salesTotal, u8 renderSalesTotal) {
    int i;

    for(i = 0; i < 2; i++) {
        SetLineF2(&g_Menu->pShop->lines3BF0[i]);
        setRGB0(&g_Menu->pShop->lines3BF0[i], 0xFF, 0xFF, 0xFF);
        setXY2(&g_Menu->pShop->lines3BF0[i],
                g_GearShopTotalPriceX - 8 - renderSalesTotal * 16,
                g_GearShopTotalPriceY + 9 - renderSalesTotal * 8,
                g_GearShopTotalPriceX + 0x4e - renderSalesTotal * 16,
                g_GearShopTotalPriceY + 9 - renderSalesTotal * 8
               );
    }

    GearShopMenuParseNumberToString(startingBalance);

    g_Menu->pShop->goldBeforeStrLen = 0;
    for(i = 0; i < MENU_MAX_DIGITS; i++) {
        if(g_Menu->digits[i] != MENU_DIGIT_NONE) {
            g_Menu->pShop->goldBeforeStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->polysGoldBefore[g_Menu->pShop->goldBeforeStrLen*2],
                g_Menu->renderContext,
                ((i * 8) + g_GearShopGoldBeforeX) - (renderSalesTotal * 0x10),
                g_GearShopGoldBeforeY - (renderSalesTotal * 0x10),
                0x1000
            );
        }
    }
    g_Menu->pShop->goldBeforeRenderCtx = (u8) g_Menu->renderContext;

    GearShopMenuParseNumberToString(totalPrice);
    g_Menu->pShop->totalPriceStrLen = 0;
    for(i = 0; i < MENU_MAX_DIGITS; i++) {
        if(g_Menu->digits[i] != MENU_DIGIT_NONE) {
            g_Menu->pShop->totalPriceStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->polysTotalPrice[g_Menu->pShop->totalPriceStrLen*2],
                g_Menu->renderContext,
                ((i * 8) + g_GearShopTotalPriceX) - (renderSalesTotal * 0x10),
                g_GearShopTotalPriceY - (renderSalesTotal * 0x10),
                0x1000
            );
        }
    }
    g_Menu->pShop->totalPriceRenderCtx = (u8) g_Menu->renderContext;

    GearShopMenuParseNumberToString(newBalance);
    g_Menu->pShop->goldAfterStrLen = 0;
    for(i = 0; i < MENU_MAX_DIGITS; i++) {
        if(g_Menu->digits[i] != MENU_DIGIT_NONE) {
            g_Menu->pShop->goldAfterStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->polysGoldAfter[g_Menu->pShop->goldAfterStrLen*2],
                g_Menu->renderContext,
                ((i * 8) + g_GearShopGoldAfterX) - (renderSalesTotal * 0x10),
                g_GearShopGoldAfterY - (renderSalesTotal * 8), 0x1000U
            );
        }
    }
    g_Menu->pShop->goldAfterRenderCtx = (u8) g_Menu->renderContext;

    g_Menu->pShop->salesTotalPrimCount = 0;
    g_Menu->pShop->shouldRenderSalesTotal = 0;

    if (renderSalesTotal) {
        GearShopMenuParseNumberToString(salesTotal);
        for(i = 0; i < MENU_MAX_DIGITS; i++) {
            if(g_Menu->digits[i] != MENU_DIGIT_NONE) {
                g_Menu->pShop->salesTotalPrimCount += func_8002675C(
                    g_Menu->resources,
                    g_Menu->digits[i],
                    &g_Menu->pShop->polysSalesTotal[(g_Menu->pShop->salesTotalPrimCount)*2],
                    g_Menu->renderContext,
                    ((i * 8) + g_GearShopTotalPriceX) - (renderSalesTotal * 0x10),
                    g_GearShopTotalPriceY - (renderSalesTotal * 8),
                    0x1000
                );
            }
        }
        g_Menu->pShop->salesTotalRenderCtx = (u8) g_Menu->renderContext;
        g_Menu->pShop->shouldRenderSalesTotal = 1;
    }
    g_Menu->pShop->shouldRenderPrices = 1;
}

// GearShopMenuSetFinalPriceGraphics ?
void GearShopMenuBuildFinalPriceGraphics(int number, u8 arg1) {

    s32 i;
    s32 var_s1;
    s32 var_s2;

    var_s1 = 0x68;
    var_s2 = 0x54;
    if (arg1) {
        var_s1 = 0x50;
        var_s2 = 0x64;
    }
    i = 0;
    GearShopMenuParseNumberToString(number);
    g_Menu->pShop->finalPriceStrLen = 0;
    for(i = 0; i < MENU_MAX_DIGITS; i++) {
        if (g_Menu->digits[i] != MENU_DIGIT_NONE) {
            g_Menu->pShop->finalPriceStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->polysFinalPrice[g_Menu->pShop->finalPriceStrLen*2],
                g_Menu->renderContext,
                var_s1 + 3,
                var_s2,
                0x1000
            );
        }
        var_s1 += 8;
    }
    g_Menu->pShop->finalPriceRenderCtx = g_Menu->renderContext;
    g_Menu->pManager->finalPriceRenderState = MENU_FINAL_PRICE_BUILT;
}

void GearShopMenuBuildGoldGraphics(u32 value) {
    int i;

    GearShopMenuParseNumberToString(value);
    g_Menu->pShop->goldAmountPrimCount = 0;

    for(i = 0; i < MENU_MAX_DIGITS; i++) {
        if(g_Menu->digits[i] != MENU_DIGIT_NONE) {
            g_Menu->pShop->goldAmountPrimCount += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->polysGoldAmount[g_Menu->pShop->goldAmountPrimCount * 2],
                g_Menu->renderContext,
                0xAA + (i * 8),
                0xAE,
                0x1000
            );
        }
    }

    g_Menu->pShop->goldAmountPrimCount += func_8002675C(
        g_Menu->resources,
        0x10,
        &g_Menu->pShop->polysGoldAmount[g_Menu->pShop->goldAmountPrimCount * 2],
        g_Menu->renderContext,
        0xF2,
        0xAE,
        0x1000
    );
    g_Menu->pShop->goldAmountRenderCtx = g_Menu->renderContext;
    g_Menu->pManager->isPanelActive[1] = 1;
}

void GearShopMenuResetShopUi(u8 arg0) {
    int i;

    g_Menu->pManager->shouldRenderShop = 0;
    g_Menu->pShop->shouldRenderItemDesc = 0;
    g_Menu->pShop->shouldRenderStr44B0 = 0;
    g_Menu->pShop->shouldRenderPrices = 0;
    g_Menu->pShop->explanationsLen = 0;
    g_Menu->pShop->equipMarkPrimCount = 0;
    g_Menu->pShop->numPortraits = 0;
    for(i = 0; i < 9; i++) {
        g_Menu->pShop->shouldRenderPortraitHighlight[i] = 0;
        g_Menu->pShop->polys27B0PrimCounts[i] = 0;
        g_Menu->pShop->polys3020PrimCounts[i] = 0;
    }
    for(i = 0; i < SHOP_MAX_ITEMS_IN_VIEW; i++) {
        g_Menu->pShop->shouldRenderItemRows[i] = 0;
        g_Menu->pShop->inventoryPrimCounts[i] = 0;
    }

    g_Menu->pShop->shouldRenderStr4530 = 0;
    if (arg0) {
        GearShopMenuFreeWindow(2);
        GearShopMenuFreeWindow(3);
        GearShopMenuFreeScrollBarHandle();
        GearShopMenuFreeArrowCursor(0);
    }
}

s32 GearShopMenuBuildEquippedCharacterMask(u8 itemId, u8 itemType) {
    int i;
    int j;
    u8 found;
    u16 result = 0;

    if(itemId) {
        for(i = 0; i < 16; i++) {
            found = FALSE;
            if (g_Menu->availableCharacters[i]) {
                found = FALSE;

                switch(itemType) {
                    case ITEM_TYPE_GEAR_FRAME:
                        if(g_GameState.gears[g_GameState.characters[i].gearId].frameId == itemId) {
                            found = TRUE;
                        }
                        break;
                    case ITEM_TYPE_GEAR_ARMOR:
                        if(g_GameState.gears[g_GameState.characters[i].gearId].armorId == itemId) {
                            found = TRUE;
                        }
                        break;
                    case ITEM_TYPE_GEAR_ENGINE:
                        if(g_GameState.gears[g_GameState.characters[i].gearId].engineId == itemId) {
                            found = TRUE;
                            break;
                        }

                        break;
                    case ITEM_TYPE_GEAR_WEAPON:
                        for(j = 0; j < 4; j++) {
                            if (itemId < MENU_GEAR_EXTRA_WEAPON_ID_MIN) {
                                if(g_GameState.gears[g_GameState.characters[i].gearId].weaponId[j] == itemId) {
                                    found = TRUE;
                                    break;
                                }
                            } else {
                                if(g_GameState.gears[g_GameState.characters[i].gearId].extraWeaponIds[j] == itemId) {
                                    found = TRUE;
                                    break;
                                }
                            }
                        }
                        break;
                    case ITEM_TYPE_GEAR_ACCESSORY:
                        for(j = 0; j < 3; j++) {
                            if(g_GameState.gears[g_GameState.characters[i].gearId].accessorySlots[j] == itemId) {
                                found = TRUE;
                                break;
                            }
                        }
                        break;
                }
            }
            if(found) {
                result |= GearShopMenuGetGearEquipFlags(g_GameState.characters[i].gearId);
            }
        }
    }

    return result;
}

u32 GearShopMenuBuildSellItemDetails(u8 arg0, u8 itemType) {
    RECT sp20;
    u8 _pad[0x10];
    u8 sp28[0xE];
    u8 sp38[0xE];
    int places[5]; // sp48
    GearAccessory* temp_v0_3;
    GearWeapon* temp_v0_2;
    s16* var_a0_2;
    s32 quotient;
    u8 didRounding;
    s32 j; // s0
    s32 k; // s2
    s32 temp_s4;
    s32 price;
    s32 var_s5;
    s32 var_s6;
    u8 gearId;
    u8* temp_v0;


    var_s5 = 0;
    places[0] = 1;
    places[1] = 10;
    places[2] = 100;
    places[3] = 1000;
    places[4] = 10000;

    temp_v0 = HeapAlloc(MENU_ITEM_DESC_IMAGE_BUFFER_SIZE, 0U);
    bzero(temp_v0, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
    bzero(sp28, sizeof(sp28));
    if (itemType != ITEM_TYPE_GEAR_ACCESSORY) {
        if (itemType == ITEM_TYPE_GEAR_WEAPON) {
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetGearWeaponName(arg0), temp_v0, 0x39U, 0U);
            var_s5 = g_Menu->pDressingRoom->pGearWeapons[arg0].equipFlags;
            var_s6 = g_Menu->pDressingRoom->pGearWeapons[arg0].price >> 1;
            price = var_s6;
        }
        // @bug if itemType != 3 or 4 then var_s6 is never set but still returned
    } else {
        g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetGearAccessoryName(arg0), temp_v0, 0x39U, 0U);
        temp_v0_3 = &g_Menu->pDressingRoom->pGearAccessories[arg0];
        var_s5 = temp_v0_3->equipFlags;
        var_s6 = temp_v0_3->price >> 1;
        price = var_s6;
    }
    temp_s4 = GearShopMenuBuildEquippedCharacterMask(arg0, itemType);
    didRounding = FALSE;
    for(j = 4, k = 0; j > 0; k++, j--) {
        quotient = price / places[j];
        if(quotient || didRounding) {
            sp28[k * 2] = quotient + MENU_GLYPH_DIGIT_0;
            didRounding = TRUE;
            price -= (quotient * places[j]);
        } else {
            sp28[k * 2] = MENU_GLYPH_BLANK;
        }
    }
    sp28[8] = (price % 10) + MENU_GLYPH_DIGIT_0;
    SystemDecodeGlyphCodesN((u16* ) &sp28, sp38, 5);
    g_Menu->pShop->str44B0.width = SystemRenderStringEntry(&sp38, temp_v0, 0x39U, 1U);
    setRECT((RECT*)&sp20, 0x140, 0x4E, 0x3C, FONT_LETTER_HEIGHT);
    LoadImage((RECT*)&sp20, (u32* ) temp_v0);
    DrawSync(0);
    GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->strItemDesc, 0, 0, 0U);
    GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->str44B0, 0, 0, 0U);
    g_Menu->pShop->str44B0.polys[g_Menu->renderContext].clut = g_SystemPalette2;
    GearShopMenuSetPolyFT4Quad(&g_Menu->pShop->strItemDesc.polys[g_Menu->renderContext], 0x2C, 0x12, 0U, 0x4EU, g_Menu->pShop->strItemDesc.width, FONT_LETTER_HEIGHT);
    GearShopMenuSetPolyFT4Quad(&g_Menu->pShop->str44B0.polys[g_Menu->renderContext], 0x98, 0x12, 0U, 0x4EU, g_Menu->pShop->str44B0.width, FONT_LETTER_HEIGHT);
    GearShopMenuSetVertices(g_Menu->pShop->strItemDesc.vertices, 0x2C, 0x12, g_Menu->pShop->strItemDesc.width, FONT_LETTER_HEIGHT);
    GearShopMenuSetVertices(g_Menu->pShop->str44B0.vertices, 0x98, 0x12, g_Menu->pShop->str44B0.width, FONT_LETTER_HEIGHT);
    g_Menu->pShop->strItemDesc.renderContext = g_Menu->renderContext;
    g_Menu->pShop->str44B0.renderContext = g_Menu->renderContext;
    HeapFree(temp_v0);
    if (arg0 & 0xFF) {
        g_Menu->pShop->shouldRenderItemDesc = 1;
        g_Menu->pShop->shouldRenderStr44B0 = 1;
    } else {
        g_Menu->pShop->shouldRenderItemDesc = 0;
        g_Menu->pShop->shouldRenderStr44B0 = 0;
    }
    g_Menu->pShop->equipMarkPrimCount = 0;
    ;
    for (k = 0, j = 0; k < 0x10; k++) {
        if (g_Menu->availableCharacters[k] != 0) {
            gearId = g_GameState.characters[k].gearId;
            if (GearShopMenuGearCanEquip(var_s5, gearId) != 0) {
                g_Menu->pShop->shouldRenderPortraitHighlight[j] = 1;
            } else {
                g_Menu->pShop->shouldRenderPortraitHighlight[j] = 0;
            }
            if (GearShopMenuGearCanEquip(temp_s4, gearId) != 0) {
                g_Menu->pShop->equipMarkPrimCount += func_8002675C(g_Menu->resources, MENU_TEX_EQUIP_MARK, &g_Menu->pShop->polys2D0[g_Menu->pShop->equipMarkPrimCount * 2], g_Menu->renderContext, g_GearShopCharacterPortraitX[j] + 0xE, 0xB4, 0x1000U);
            }
            j += 1;
        }
    }
    g_Menu->pShop->equipMarkRenderCtx = (u8) g_Menu->renderContext;
    return var_s6;
}

void GearShopMenuRenderSellItemRows(s32 scrollOffset, u8* pItemIds, u8* pItemTypes, u8* pItemSellQtys, u8* pItemQtys) {
    RECT sp20;
    u8 digitGlyphs[0xE]; // sp28: the five glyph codes of the quantity
    u8 decodedGlyphs[0xE]; // sp38
    int places[5]; // sp48
    u32 polyColor;
    s32 quotient;
    u8 didRounding;
    RECT* pRect;
    s32 j;
    s32 k;
    u8 itemType;
    u8 tensDigit;
    u8 onesDigit;
    s32 qty;
    int i;
    void* pWorkspace;

    places[0] = 1;
    places[1] = 10;
    places[2] = 100;
    places[3] = 1000;
    places[4] = 10000;

    pWorkspace = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);

    for (i = 0; i < SHOP_MAX_ITEMS_IN_VIEW; i++) {
        pRect = &sp20;
        bzero(digitGlyphs, sizeof(digitGlyphs));
        g_Menu->pShop->inventoryPrimCounts[i] = 0;
        polyColor = 0x1000;
        if (pItemIds[scrollOffset + i]) {
            qty = pItemQtys[scrollOffset + i];
            itemType = pItemTypes[scrollOffset + i];
            if (itemType != ITEM_TYPE_GEAR_ACCESSORY) {
                if (itemType == ITEM_TYPE_GEAR_WEAPON) {
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(GetGearWeaponName(pItemIds[scrollOffset + i]), pWorkspace, 0x24U, 0U);
                }
            } else {
                g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(GetGearAccessoryName(pItemIds[scrollOffset + i]), pWorkspace, 0x24U, 0U);
            }
            didRounding = FALSE;
            for (j = 4, k = 0; j > 0; k++, j--) {
                quotient = qty / places[j];
                if (quotient || didRounding) {
                    ((u8*)pRect)[8 + k * 2] = MENU_GLYPH_DIGIT_0;
                    ((u8*)pRect)[8 + k * 2] = quotient + ((u8*)pRect)[8 + k * 2];
                    didRounding = TRUE;
                    qty -= (quotient * places[j]);
                } else {
                    ((u8*)pRect)[8 + k * 2] = MENU_GLYPH_BLANK;
                }
            }
            ((u8*)pRect)[16] = (qty % 10) + MENU_GLYPH_DIGIT_0;
            SystemDecodeGlyphCodesN((u16*)((u8*)&sp20 + 8), decodedGlyphs, 5);
            g_Menu->pShop->strItemQuantities[i].width = SystemRenderStringEntry(decodedGlyphs, pWorkspace, 0x24U, 1U);
            setRECT(&sp20, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(pRect, pWorkspace);
            GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->strItemNames[i], i, 0x80, 0x81);
            GearShopMenuSetVertices(g_Menu->pShop->strItemNames[i].vertices, 0x24, 0x32 + FONT_LETTER_HEIGHT * i, g_Menu->pShop->strItemNames[i].width, FONT_LETTER_HEIGHT);
            setRECT(&sp20, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(pRect, pWorkspace);
            DrawSync(0);
            GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->strItemQuantities[i], i, 0x80, 0x82);
            GearShopMenuSetVertices(g_Menu->pShop->strItemQuantities[i].vertices, 0x8C, 0x32 + FONT_LETTER_HEIGHT * i, g_Menu->pShop->strItemQuantities[i].width, FONT_LETTER_HEIGHT);
            g_Menu->pShop->strItemNames[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->strItemNames[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->shouldRenderItemRows[i] = 1;
            if (pItemSellQtys[scrollOffset + i]) {
                g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, 0xE5, g_Menu->pShop->polysShopInventory[i * 4], g_Menu->renderContext, 0xB4, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000U);
                tensDigit = pItemSellQtys[scrollOffset + i] / 10;
                if (tensDigit) {
                    g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, tensDigit, &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->inventoryPrimCounts[i] * 2], g_Menu->renderContext, 0xBC, 0x36 + FONT_LETTER_HEIGHT * i, polyColor);
                }
                onesDigit = pItemSellQtys[scrollOffset + i] % 10;
                g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, onesDigit, &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->inventoryPrimCounts[i] * 2], g_Menu->renderContext, 0xC4, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000U);
                g_Menu->pShop->inventoryRenderCtxs[i] = g_Menu->renderContext;
            }
        } else {
            g_Menu->pShop->shouldRenderItemRows[i] = 0;
        }
    }

    HeapFree(pWorkspace);
}


/*
 * Finalizes the sale.
 *
 * newBalance: the amount of GP the player has, will be clipped to MAX_GOLD_AMOUNT
 * pItemIds: a coalesced version of the player's inventory
 * pItemSellQtys: how many of each item in the pItemIds array were sold in the transaction
 * listLength: the size of the pItemIds / pItemSellQtys arrays
 * pInventoryItemIds: pointer to the actual game state containing un-coalesced inventory
 * pInventoryQtys: pointer to the actual game state contaning how many of each item the player owns
 * pItemTypes: unused
 * isBulkSale: set to TRUE if the sales transaction actually affects the player's inventory.
 * flag2: unused
 *
 */

void GearShopMenuFinalizeSalesTransaction(s32 newBalance, u8* pItemIds, u8* pItemSellQtys, s32 listLength, u8* pInventoryItemIds, u8* pInventoryQtys, u8* pUnused, u8 isBulkSale, u8 ignored) {
    s32 cartIndex, inventoryIndex;

    GearShopMenuPlaySoundEffect(0xD1);
    g_GameState.gold = (u32) newBalance;
    if ((u32) newBalance > MAX_GOLD_AMOUNT) {
        g_GameState.gold = MAX_GOLD_AMOUNT;
    }
    if (isBulkSale) {

        /*
         * indexes in pItemIds and pItemSellQtys are corrlated, and indexes in pInventoryItemIds and pInventoryQtys are correlated, but
         * the two lists do not correlate because the former has been defragmented. So, what we do here is iterate over the defragmented
         * list, and then search the fragmented list for the matching key (pItemId[i] == pInventoryItemIds[j]) and updating the quantities
         * accordingly.
         */

        for(cartIndex = 0; cartIndex < listLength; cartIndex++) {
            if(pItemIds[cartIndex]) {
                for(inventoryIndex = 0; inventoryIndex < listLength; inventoryIndex++) {
                    if(pItemIds[cartIndex] == pInventoryItemIds[inventoryIndex]) {
                        pInventoryQtys[inventoryIndex] -= pItemSellQtys[cartIndex];
                        if(pInventoryQtys[inventoryIndex] == 0) {
                            pInventoryItemIds[inventoryIndex] = 0;
                        }
                    }
                }
            }
       }
    }
}

void GearShopMenuSellMenu(s32 listLength, u8* pInventoryItemIds, u8* pInventoryQtys, int itemType, u8 flag1, u8 *unused, u8 flag2) {
    u8 continueProcessing = TRUE;
    u8 redrawWindows = TRUE;
    u8 redrawTransactionSummary = TRUE;
    int rowIndex = 0;
    int newRowIndex = 0xFF;
    int scrollOffset = 0;

    int newScrollOffset = 0xFF;

    u8 *pItemIds = alloca(listLength); // sp60
    u8 *pItemTypes = alloca(listLength); // sp68
    u8 *pItemSellQtys = alloca(listLength);    // s7
    u8 *pItemIsOwned = alloca(listLength);    // sp70

    u8 *pItemQtys; // sp78
    int currentGold;    // sp80
    int projectedBalance; // s4
    int itemPrice;
    int cartTotal = 0;
    int itemCount; // sp90
    int i;

    currentGold = g_GameState.gold;
    pItemQtys = alloca(listLength);
    projectedBalance = g_GameState.gold;

    // first, zero out the lists; we cheat here because we know the item type will be the same, so we just populate that list now.
    for(i = 0; i < listLength; i++) {
        pItemQtys[i] = 0;
        pItemIsOwned[i] = 0;
        pItemSellQtys[i] = 0;
        pItemIds[i] = 0;
        pItemTypes[i] = itemType;
    }

    // now coalesce the items & quantities, eliminating empty slots in the inventory list
    itemType = 0;
    for(i = 0; i < listLength; i++) {
        if(pInventoryItemIds[i] && pInventoryQtys[i]) {
            pItemIds[itemType] = pInventoryItemIds[i];
            pItemQtys[itemType] = pInventoryQtys[i];
            pItemIsOwned[itemType] = 1;
            itemType++;
        }
    }

    itemCount = itemType;
    GearShopMenuInitializeArrowCursor(0);

    while(continueProcessing) {
        GearShopMenuUpdateAndRender();

        if(scrollOffset != newScrollOffset || redrawTransactionSummary) {
            GearShopMenuRenderSellItemRows(scrollOffset, pItemIds, pItemTypes, pItemSellQtys, pItemQtys);
            GearShopMenuUpdateScrollBarHandle(0xC, 0x32, 0x3C, itemCount, scrollOffset);
        }

        if ((rowIndex != newRowIndex) || (scrollOffset != newScrollOffset)) {
            newRowIndex = rowIndex;
            newScrollOffset = scrollOffset;
            itemPrice = GearShopMenuBuildSellItemDetails(pItemIds[scrollOffset+rowIndex], pItemTypes[scrollOffset+rowIndex]);
            g_Menu->pManager->shouldRenderShop = 1;
        }

        GearShopMenuUpdateArrowCursor(rowIndex, scrollOffset, 0, 0);

        if (redrawWindows) {
            GearShopMenuInitializeWindow(2, 0xC, 0x2A, 0xC4, 0x74, 0, 1, 4, 1);
            GearShopMenuInitializeWindow(3, 0x20, 0xE, 0xFC, 0x14, 0, 1, 4, 0);
            GearShopMenuUpdateCharacterPortraits();
            GearShopMenuBuildSellExplanations();
            redrawWindows = FALSE;
        }
        if (redrawTransactionSummary) {
            GearShopMenuBuildTransactionSummary(currentGold, cartTotal, projectedBalance, 0, 0);
            redrawTransactionSummary = FALSE;
        }

        switch(g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                if (cartTotal) {
                    GearShopMenuPlaySoundEffect(MENU_SOUND_CONFIRM);
                    g_Menu->pManager->shouldRenderShop = 0;
                    g_Menu->pManager->shouldRenderWindow[2] = 0;
                    g_Menu->pManager->shouldRenderWindow[3] = 0;
                    g_Menu->pManager->scrollHandleActive = 0;
                    continueProcessing = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
                    GearShopMenuInitializePointerCursors(0);
                    GearShopMenuBuildFinalPriceGraphics(cartTotal, 0);
                    if(GearShopMenuShowModal(MENU_STRING_CONFIRM_SELL, MENU_STRING_NONE, TRUE)) {

                        GearShopMenuFinalizeSalesTransaction(projectedBalance, pItemIds, pItemSellQtys, listLength, pInventoryItemIds, pInventoryQtys, pItemTypes, flag1, flag2);
                    } else {
                        continueProcessing = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        newScrollOffset = 0xFF;
                        newRowIndex = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                    }
                    GearShopMenuFreePointerCursors();
                } else {
                    GearShopMenuPlaySoundEffect(MENU_SOUND_ERROR);
                }
                break;
            case MENU_INPUT_BACK:
                continueProcessing = FALSE;
                if (cartTotal != 0) {
                    g_Menu->pManager->shouldRenderShop = 0;
                    g_Menu->pManager->shouldRenderWindow[2] = 0;
                    g_Menu->pManager->shouldRenderWindow[3] = 0;
                    g_Menu->pManager->scrollHandleActive = 0;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
                    GearShopMenuInitializePointerCursors(0U);
                    if(!GearShopMenuShowModal(0x92, MENU_STRING_NONE, 1)) {
                        continueProcessing = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        newScrollOffset = 0xFF;

                        newRowIndex = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                    }
                    GearShopMenuFreePointerCursors();
                }
                break;
            case MENU_INPUT_DOWN:
                rowIndex += 1;
                if (rowIndex >= SHOP_MAX_ITEMS_IN_VIEW) {
                    rowIndex = SHOP_MAX_ITEMS_IN_VIEW - 1;
                    scrollOffset += 1;
                    if ((itemCount - SHOP_MAX_ITEMS_IN_VIEW) < scrollOffset) {
                        scrollOffset -= 1;
                    }
                }
                break;
            case MENU_INPUT_UP:
                rowIndex -= 1;
                if (rowIndex < 0) {
                    scrollOffset -= 1;
                    rowIndex = 0;
                    if (scrollOffset < 0) {
                        scrollOffset = 0;
                    }
                }
                break;
            case MENU_INPUT_RIGHT:
                if (pItemIsOwned[scrollOffset + rowIndex] && pItemQtys[scrollOffset + rowIndex] - 1 >= 0) {
                    redrawTransactionSummary = 1;
                    cartTotal += itemPrice;
                    projectedBalance += itemPrice;
                    pItemSellQtys[scrollOffset+rowIndex]++;
                    pItemQtys[scrollOffset + rowIndex]--;
                }
                break;
            case MENU_INPUT_LEFT:
                if (pItemIsOwned[scrollOffset + rowIndex] && pItemSellQtys[scrollOffset+rowIndex]-1 >= 0) {
                    cartTotal -= itemPrice;
                    redrawTransactionSummary = 1;
                    pItemQtys[scrollOffset + rowIndex]++;
                    pItemSellQtys[scrollOffset+rowIndex]--;
                    projectedBalance -= itemPrice;
                }
                break;
        }
    }
}

void GearShopMenuSellPartsMenu(void) {
    GearShopMenuSellMenu(
        MAX_INVENTORY_ITEMS,
        g_GameState.gearPartsIds,
        g_GameState.gearPartsQty,
        ITEM_TYPE_GEAR_ACCESSORY, 1,
        g_GameState.gearPartsQty,
        0
    );
}

void GearShopMenuSellWeaponsMenu(void) {
    GearShopMenuSellMenu(
        MAX_INVENTORY_WEAPONS,
        g_GameState.gearWeaponsIds,
        g_GameState.gearWeaponsQty,
        ITEM_TYPE_GEAR_WEAPON, 1,
        g_GameState.gearWeaponsQty,
        0
    );
}

void GearShopMenuSellModeMenuHandleSelectedOption(u_char offset) {
    int choice;
    u8 unkBool = 1;
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    g_Menu->pManager->shouldRenderSubmenuStrings = 0;
    GearShopMenuClearStringRenderFlags(4, g_Menu->pManager->shouldRenderStrings);

    // Here, subMenuChoice 0 and 1 correspond to Weapons and Parts
    choice = g_Menu->subMenuChoice + (offset * 3);

    // The functions here goes into and will run the actual sell menus
    switch (choice) {
        case MENU_CHOICE_GEAR_PARTS:
            GearShopMenuSellPartsMenu();
            break;

        case MENU_CHOICE_GEAR_WEAPONS:
            GearShopMenuSellWeaponsMenu();
            break;
    }

    GearShopMenuResetShopUi(unkBool);
    g_Menu->pManager->shouldRenderSubmenuStrings = 1;
    g_Menu->pManager->shouldRenderPointerCursor = 1;
    g_Menu->pManager->unk3 = 1;
    GearShopMenuRenderMenuStringsToVram(4, g_Menu->optionStrings, &g_GearShopSubmenuStringIds, g_Menu->pManager->shouldRenderStrings);
}

void GearShopMenuBuildGearPartNameGraphics(u8 itemType) {
    RECT rect;

    u32* pWorkspace;
    u8 itemId;
    void* pNames;

    pWorkspace = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);

    switch (itemType) {
        case ITEM_TYPE_GEAR_ARMOR:
            itemId = g_GameState.gears[g_gearShopCurrentGearId].armorId;
            pNames = g_Menu->pShop->pGearArmorNames;
            g_Menu->pShop->str4530.width = SystemRenderStringEntry(GetStringEntry(pNames, itemId), pWorkspace, 0x24, 0);
            break;
        case ITEM_TYPE_GEAR_FRAME:
            itemId = g_GameState.gears[g_gearShopCurrentGearId].frameId;
            pNames = g_Menu->pShop->pGearFrameNames;
            g_Menu->pShop->str4530.width = SystemRenderStringEntry(GetStringEntry(pNames, itemId), pWorkspace, 0x24, 0);
            break;
        case ITEM_TYPE_GEAR_ENGINE:
            itemId = g_GameState.gears[g_gearShopCurrentGearId].engineId;
            pNames = g_Menu->pShop->pGearEngineNames;
            g_Menu->pShop->str4530.width = SystemRenderStringEntry(GetStringEntry(pNames, itemId), pWorkspace, 0x24, 0);
            break;
    }

    setRECT(&rect, 408, 180, 40, FONT_LETTER_HEIGHT);
    LoadImage((RECT* ) &rect, pWorkspace);
    GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->str4530, 9, 128, 129);
    GearShopMenuSetVertices(g_Menu->pShop->str4530.vertices, 212, 142, g_Menu->pShop->str4530.width, FONT_LETTER_HEIGHT);
    DrawSync(0);
    g_Menu->pShop->str4530.renderContext = g_Menu->renderContext;
    g_Menu->pShop->shouldRenderStr4530 = 1;
    HeapFree(pWorkspace);
}
// https://decomp.me/scratch/5HauM
// PARKED: 96.75%, 2508 bytes (the ROUND 456 body at the end of this note is better:
// 97.64%, 2528 bytes - read that first). Retail is 2532 = 633 insns; 110 of 112 instruction
// segments are exact. The two that differ are ONE root cause: retail emits the per-case
// `equipFlags` load / `price = value` copy / spill suffix TWICE (once shared by the ARMOR
// and FRAME cases -- armor ends `j` into the frame block, delay slot `nop` -- and once
// inline for the ENGINE case) where gcc 2.6.0 merges all three into one copy (-6 insns).
// Retail's ENGINE copy exists only because its element address lands in $v1
// (`sll v0,v0,4; addu v1,v0,v1; lhu s3,10(v1); lw v1,0(v1)`) while ARMOR/FRAME use $a0
// (`addu a0,v0,v1`); we get $v0 at every site, so the three blocks compare equal after
// reload and gcc keeps one. The remaining 1-insn difference is the `places[]` base:
// retail `lw t1,120(sp); addiu a1,t1,16`, ours `addiu a1,sp,48`.
// Fixed here, do not undo: `equipFlags` MUST be declared before `sellPrice` (that is what
// puts equipFlags at 0x68 and sellPrice at 0x70); the case-block order, the jtbl
// (5 words + 4-byte pad at 0x801C5068), the prologue and every other reloc are exact.
// Proved dead ends (each re-diffed; all stay 2508 bytes with ONE merged copy): element
// pointer locals in all four indexed-vs-pointer combinations across the three cases
// (gcc folds them back to $v0); every statement order of `value`/`price`/`equipFlags`
// in the engine case, including equipFlags first and a `do { } while (0)` cluster; an
// extra s32/u32 temp for the price or for the flags; the pointer assignment moved above
// sellPrice; gcc 2.7.2-psx (96.4%) and gcc-2.7.2-cdk-psx (83.59%) - the unit's 2.6.0
// is right; three permuter runs (best candidates were variable-reuse games: pFrame holding
// &rect, equipFlags read through the u8 itemType; the third ran 1300 iterations and never
// beat the base score).
// Round 101 also re-proved, all still 2508 bytes: the embedded-assignment form
// (`value = (pEngine = &...)->price;`) in the engine case AND in the frame case; the engine
// case with no pointer local at all (plain double indexing); a do { } while (0) around just
// the engine case's value/price/equipFlags cluster. The unmerge needs the element address
// in a register other than $v0 at the ENGINE site, and no source shape moves it.
// The known-cheating `value = equipFlags; equipFlags = value;` in the engine case does
// unmerge 4 of those 6 insns (97.37%, 2524 bytes) but swaps what the digit loop renders
// for engines, so it is not shippable.
// Round 171 measured the mechanism, which is a register-allocation tie and nothing else.
// The `addu` that forms the element address (`addu dest, index, tableBase`) appears 18
// times in the original; retail's dest is $v0 at 16 of them, $a0 at the four ARMOR/FRAME
// ones (sellPrice AND tail, both) and $v1 at the two ENGINE ones. Rule in retail: a site
// whose address has one use (WEAPON/ACCESSORY tail, every gearId->gear index product) keeps
// $v0, a site whose address feeds both `lhu price` and `lw equipFlags` does not. Ours puts
// dest in $v0 at all 18, so the three two-use sites become RTL-identical after reload and
// cross_jumping keeps one copy of `lw/move/j/sw` plus the frame's address `addu` (-6 insns,
// 2508 vs 2532). Nothing else in the function differs: 110 of 112 segments are exact and
// the only other residual is the `places[]` biv, which retail re-bases onto the spilled
// pRect (`lw t1,0x78(sp); lw a0,0x78(sp); addiu a1,t1,16`) where ours uses `sp+48`.
// Pre-reload the three tails are the same RTL statement-by-statement, so the difference is
// purely which of the two dying operands the sum's destination coalesces with; that is
// global-alloc state in a 633-insn function, i.e. not addressable from this block's C. Do
// not re-derive: what would move it is any source change that shifts allocation for the
// whole function, and three permuter runs over this body found nothing.
// Round 443 re-measured (96.75%, 2508 bytes, retail 2532 / 633 insns) and closed the last
// plausible non-allocator story. The round-421/432 lever (a tail shared by two switch arms
// is SOURCE-level: write it once at a `goto` label between the arms) does NOT apply here,
// measured: with the armor/frame tail written once after the FRAME arm (`pFrame =
// (GearFrame*)pArmor; goto equipGearPart;` / `equipGearPart: equipFlags =
// pFrame->equipFlags; price = value;`) the body is still 2508 bytes, because post-reload
// the labeled block is register-identical to the ENGINE arm's copy (`lw $2,0($2); move
// $fp,$19; j; sw $2,0x68(sp)`) and cross_jump merges it into the last copy, as before.
// Also measured: our merge set is one block LARGER than retail's - the FRAME and ENGINE
// arms share their value load too (`addu $2,$2,$3; lhu $19,0xA($2)`; both prices sit at
// 0xA), so the 6 missing insns are 4 (one tail copy: lw/move/j/sw) + 2 (that shared value
// load). Retail keeps frame's `lhu` inline because its element address lands in $a0 there
// and in $v1 at the engine site, while ours lands in $v0 at both - the same tie.
// Spelling the engine arm through the frame view (`pFrame =
// (GearFrame*)&g_Menu->pDressingRoom->pGearEngines[id];` - legal, both structs have
// equipFlags at 0 and price at 0xA) is also 2508, and presets are unaffected: -O3 96.75,
// -G0 96.75 on top of the 2.7.2 (96.4) and cdk (83.59) screens above.
// ROUND 456: 97.64%, 2528 bytes / 632 insns against retail's 2532 / 633. The ENGINE
// arm CAN be unmerged from the armor/frame tail - one edit, and it is the double
// negation from the round-405/427 lever: `pEngine = (GearEngine*)((u8*)g_Menu->
// pDressingRoom->pGearEngines - -(g_Menu->shopItemIDs[scrollOffset + i] *
// sizeof(GearEngine)));` makes the element address BASE-FIRST with the base in the
// addu destination (`addu v1,v1,v0`), where the subscript and `pGearEngines + id`
// both coalesce into the index (`addu v0,v0,v1`) and cross_jump then keeps ONE copy
// of all three tails (that is the 2508 body below this one). Two residuals remain,
// both encoding-level: (a) retail's engine sites are INDEX-first with the base as
// destination (`addu v1,v0,v1`) and its armor/frame sites want a THIRD register
// (`addu a0,v0,v1`, at the sellPrice element and at the shared tail) - a destination
// that coalesces with neither dying operand, which is the round-171 allocator tie
// this note calls closed, now with the engine arm proved movable; (b) retail's
// `places[]` base is rebased onto the spilled pRect (`lw t1,0x78(sp); addiu a1,t1,
// 0x10`) where ours is `addiu a1,sp,48` - one instruction, and exactly the last 4
// bytes. Measured dead ends this round: pRect for setRECT/LoadImage/DecodeGlyph (the
// shape retail's `lw a0,0x78(sp)` shows) 88.86% / 2516 / 629, whole-function
// allocation churn; `goto equipGearPart` after the frame arm plus engine double
// subscripting 95.77% / 2496 / 624 (merges MORE, not fewer); one
// GearShopItemRowWork struct (rect + digits + decoded + places) 88.24% / 2524 / 631;
// the divisor written through pRect (`((s32*)((u8*)pRect + 0x28))[j]`) 96.22% /
// 2544 / 636, the +0x28 is materialised per use; `pGearEngines + id` 96.75% / 2508,
// identical to the subscript. Twin: ShopMenuRenderBuyItemRows
// (src/shop_menu/main/misc.c, loose 0.812) - same digit loop and rect-through-pointer
// idiom, but three item cases and no shared gear tail, so it does not carry the answer.
// The stub is convertible (`convertible.py` OK; the unit owns jtbl_801C5068), so a future
// fix has to be proved with check_bin.sh gear_shop_menu, not diff_func.sh alone.
// ROUND 469: 98.91%, 2528 bytes / 632 insns against retail's 2532 / 633. Improvement this
// round: LoadImage now takes the pRect pointer variable at BOTH call sites (retail reloads
// it: 'lw a0,0x78(sp)' before each jal; with '&rect' cc1 folds an 'addiu a0,sp,0x20'
// instead), 97.64 -> 98.91 and 109 -> 101 differing lines. NOT setRECT: retail writes the
// four RECT halves straight to the stack ('sh s1,0x20(sp)'), which is the '&rect' form, and
// the matched neighbour GearShopMenuRenderSellItemRows (this file, 100.00) is spelled exactly
// this way - setRECT(&sp20) + LoadImage(pRect). Also measured this round, all dead:
// pRect for every rect use at once (98.56% but the extra pseudo grows the frame to 0xE0 /
// 635 insns), one GearShopItemRowWork struct over rect+digitCodes+decodedCodes+places with
// the member form for setRECT/DecodeGlyph and the pointer form for the digit stores,
// LoadImage and places (97.50 / 2544 / 636), and two permuter runs (881 iters base 921 best
// 775 = the LoadImage edit above; 1199 iters base 401 best 357 = a no-op if/else around the
// 0x80 store, not shippable).
// THE THREE RESIDUALS LEFT, in order of how much they cost:
// (a) THE SIX SWITCH ARMS mirror one register. Retail puts the gear-table element address in
// a THIRD register that coalesces with neither dying operand ('addu a0,v0,v1' at the armor
// and frame sellPrice sites, 'addu v1,v0,v1' at the engine one - base in the destination,
// index first) and then keeps the price in v1 and pShop in v0; ours lands the element in the
// index register ('addu v0,v0,v1') and mirrors the pair ('lhu v0,4(v0)' / 'lw v1,0x450(a2)').
// Only a global-alloc outcome; the source shape (pointer local used for price then
// equipFlags) is confirmed by retail's one addu plus two loads at each of the three sites.
// (b) ONE INSTRUCTION, and it is the same rebasing the matched neighbour shows: retail's
// places base is 'lw t1,0x78(sp)' + 'addiu a1,t1,0x10', i.e. 'places[j]' reached off the
// spilled pRect with the 0x10 being the giv's 4*j at j=4 and 0x28 the displacement -
// GearShopMenuRenderSellItemRows has exactly that ('addiu a1,fp,0x10' / 'lw v0,0x28(a1)')
// with pRect held in a callee-saved register instead of spilled. Ours folds the base to 'addiu a1,sp,0x30' because
// pRect is not yet reloaded at that point, which is one instruction cheaper and unfixable
// from the source ('((s32*)((u8*)pRect + 0x28))[j]' materialises the 0x28 per use, 2544).
// (c) 'move s4,zero' sits one slot later than retail's inside the preheader init group
// (scheduler noise, same instruction count).
// ROUND 482: re-measured the body below at 98.91% / 2528 bytes / 632 insns against retail's
// 2532 / 633 - it still compiles and is still the best, and (b) is still the only size residual.
// The proof that (b) is reachable at all: writing BOTH digit-loop uses of the divisor through the
// pRect-derived cast - '((s32*)((u8*)pRect + 0x28))[j]' in the division AND in 'value -= quotient
// * ...' - reproduces retail's anchor instruction for instruction ('lw t1,0x78(sp)' / 'addiu
// a1,t1,0x10' / two 'lw v0,0x28(a1)', CSE'd to one anchor), i.e. the exact 2532 bytes, but it
// measures 97.50% / 2544 / 636: the one extra live reload takes a callee-saved register away, so
// the three 'func_8002675C(..., 0x1000)' calls in the inventory block emit 'li t1,0x1000' x3 where
// retail has ONE 'li s2,0x1000' and three 'sw s2,24(sp)'. A 'u32 polyColor' local holding 0x1000
// (assigned inside or outside the outer loop, used by one or by all three calls) does not change
// it - gcc rematerialises a known constant, which is exactly what the matched neighbour does
// ('ori t2,0x1000' x3) - so (b) and that hoisted constant are the same allocator budget: this body
// wins it, the pRect-anchored one loses it. Casting only the divisor, or a 's32* pPlaces =
// (s32*)((u8*)pRect + 0x28)' local, measures 2540 / 635 with TWO anchors ('addiu a2,sp,0x30' folded
// plus the pRect one) - the same trade, one instruction cheaper.
/*
s32 func_801D2B74(s32 scrollOffset, s32 funds, u8* pItemAffordable) {
    RECT rect;
    u8 sp28[0xE];
    u8 sp38[0xE];
    int places[5]; // sp48
    s32 quotient;
    u8 didRounding;
    u32 equipFlags;
    s32 sellPrice;
    s32 price;
    s32 value;
    u8 itemType;
    u8 tensDigit;
    u8 onesDigit;
    s32 j;
    s32 k;
    void* pWorkspace;
    s32 i;
    RECT* pRect;
    GearArmor* pArmor;
    GearFrame* pFrame;
    GearEngine* pEngine;

    places[0] = 1;
    places[1] = 10;
    places[2] = 100;
    places[3] = 1000;
    places[4] = 10000;

    pWorkspace = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);
    pRect = &rect;

    for (i = 0; i < 8; i++) {
        pItemAffordable[i] = 0;
        bzero(sp28, 0xE);
        g_Menu->pShop->inventoryPrimCounts[i] = 0;
        if (g_Menu->shopItemIDs[scrollOffset + i]) {
            itemType = g_Menu->shopItemTypes[scrollOffset + i];
            switch (itemType) {
                case ITEM_TYPE_GEAR_ARMOR:
                    sellPrice = g_Menu->pDressingRoom->pGearArmor[g_GameState.gears[g_gearShopCurrentGearId].armorId].price >> 1;
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(GetStringEntry(g_Menu->pShop->pGearArmorNames, g_Menu->shopItemIDs[scrollOffset + i]), pWorkspace, 0x24, 0);
                    pArmor = &g_Menu->pDressingRoom->pGearArmor[g_Menu->shopItemIDs[scrollOffset + i]];
                    value = pArmor->price;
                    price = value;
                    equipFlags = pArmor->equipFlags;
                    break;
                case ITEM_TYPE_GEAR_FRAME:
                    sellPrice = g_Menu->pDressingRoom->pGearFrames[g_GameState.gears[g_gearShopCurrentGearId].frameId].price >> 1;
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(GetStringEntry(g_Menu->pShop->pGearFrameNames, g_Menu->shopItemIDs[scrollOffset + i]), pWorkspace, 0x24, 0);
                    pFrame = &g_Menu->pDressingRoom->pGearFrames[g_Menu->shopItemIDs[scrollOffset + i]];
                    value = pFrame->price;
                    price = value;
                    equipFlags = pFrame->equipFlags;
                    break;
                case ITEM_TYPE_GEAR_ENGINE:
                    sellPrice = g_Menu->pDressingRoom->pGearEngines[g_GameState.gears[g_gearShopCurrentGearId].engineId].price >> 1;
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(GetStringEntry(g_Menu->pShop->pGearEngineNames, g_Menu->shopItemIDs[scrollOffset + i]), pWorkspace, 0x24, 0);
                    pEngine = (GearEngine*)((u8*)g_Menu->pDressingRoom->pGearEngines - -(g_Menu->shopItemIDs[scrollOffset + i] * sizeof(GearEngine)));
                    value = pEngine->price;
                    price = value;
                    equipFlags = pEngine->equipFlags;
                    break;
                case ITEM_TYPE_GEAR_WEAPON:
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(GetGearWeaponName(g_Menu->shopItemIDs[scrollOffset + i]), pWorkspace, 0x24, 0);
                    value = g_Menu->pDressingRoom->pGearWeapons[g_Menu->shopItemIDs[scrollOffset + i]].price;
                    price = value;
                    equipFlags = -1;
                    break;
                case ITEM_TYPE_GEAR_ACCESSORY:
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(GetGearAccessoryName(g_Menu->shopItemIDs[scrollOffset + i]), pWorkspace, 0x24, 0);
                    value = g_Menu->pDressingRoom->pGearAccessories[g_Menu->shopItemIDs[scrollOffset + i]].price;
                    price = value;
                    equipFlags = -1;
                    break;
            }
            if (funds >= price) {
                if (GearShopMenuGearCanEquip(equipFlags, g_gearShopCurrentGearId)) {
                    pItemAffordable[i] = 0x80;
                }
            }
            didRounding = FALSE;
            for (j = 4, k = 0; j > 0; k++, j--) {
                quotient = value / places[j];
                if (quotient || didRounding) {
                    ((u8*)pRect)[8 + k * 2] = quotient + 0x10;
                    didRounding = TRUE;
                    value -= (quotient * places[j]);
                } else {
                    ((u8*)pRect)[8 + k * 2] = 0xC3;
                }
            }
            ((u8*)pRect)[16] = (value % 10) + 0x10;
            SystemDecodeGlyphCodesN((u16*)((u8*)&rect + 8), sp38, 5);
            g_Menu->pShop->strItemQuantities[i].width = SystemRenderStringEntry(&sp38, pWorkspace, 0x24, 1);
            setRECT(&rect, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(pRect, pWorkspace);
            GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->strItemNames[i], i, 0x80, pItemAffordable[i] + 1);
            GearShopMenuSetVertices(g_Menu->pShop->strItemNames[i].vertices, 0x24, 0x32 + FONT_LETTER_HEIGHT * i, g_Menu->pShop->strItemNames[i].width, FONT_LETTER_HEIGHT);
            setRECT(&rect, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(pRect, pWorkspace);
            DrawSync(0);
            GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->strItemQuantities[i], i, 0x80, pItemAffordable[i] + 2);
            GearShopMenuSetVertices(g_Menu->pShop->strItemQuantities[i].vertices, 0x8C, 0x32 + FONT_LETTER_HEIGHT * i, g_Menu->pShop->strItemQuantities[i].width, FONT_LETTER_HEIGHT);
            g_Menu->pShop->strItemNames[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->strItemNames[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->shouldRenderItemRows[i] = 1;
            if (g_Menu->pShop->curItemQuantities[scrollOffset + i]) {
                g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, 0xF1, g_Menu->pShop->polysShopInventory[i * 4], g_Menu->renderContext, 0xB4, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000U);
                tensDigit = g_Menu->pShop->curItemQuantities[scrollOffset + i] / 10;
                if (tensDigit) {
                    g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, tensDigit, &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->inventoryPrimCounts[i] * 2], g_Menu->renderContext, 0xBC, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000U);
                }
                onesDigit = g_Menu->pShop->curItemQuantities[scrollOffset + i] % 10;
                g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, onesDigit, &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->inventoryPrimCounts[i] * 2], g_Menu->renderContext, 0xC4, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000U);
                g_Menu->pShop->inventoryRenderCtxs[i] = g_Menu->renderContext;
            }
        } else {
            g_Menu->pShop->shouldRenderItemRows[i] = 0;
        }
    }

    HeapFree(pWorkspace);
    return sellPrice;
}*/
INCLUDE_ASM("asm/gear_shop_menu/nonmatchings/main/render", func_801D2B74);




void GearShopMenuComputeEquipStatDelta(s32* pDelta, s8* pIsImprovement, u8 itemId, u8 itemType, u8 characterId) {
    s16 oldValue[2];
    s16 newValue[2];
    u8 itemSlots[8];

    u8 gearId;
    u8 changeDetected;
    int i;
    u32 smallest;
    u8 leastDefense = 0xFF;
    GearAccessory* pAccessory;
    GearWeapon* pWeaponList;
    GearAccessory* pCurAccesory;


    gearId = g_GameState.characters[characterId].gearId;

    itemSlots[0] = g_GameState.gears[gearId].weaponId[0];

    itemSlots[1] = g_GameState.gears[gearId].extraWeaponIds[0];
    itemSlots[2] = g_GameState.gears[gearId].extraWeaponIds[1];
    itemSlots[3] = g_GameState.gears[gearId].extraWeaponIds[2];
    itemSlots[4] = g_GameState.gears[gearId].extraWeaponIds[3];

    itemSlots[5] = g_GameState.gears[gearId].accessorySlots[0];
    itemSlots[6] = g_GameState.gears[gearId].accessorySlots[1];
    itemSlots[7] = g_GameState.gears[gearId].accessorySlots[2];

    switch(itemType) {
        case ITEM_TYPE_GEAR_WEAPON:
            // This **may** be a check to see if the weapon belongs to Billy or not,
            // since Billy uses guns w/ ammo which needs special handling? This should be double checked though.
            if (itemId < MENU_GEAR_EXTRA_WEAPON_ID_MIN) {
                g_GameState.gears[gearId].weaponId[0] = itemId;
            } else {
                pWeaponList = g_Menu->pDressingRoom->pGearWeapons;

                for(i = 0; i < 4; i++) {
                    if(g_Menu->pDressingRoom->pGearWeapons[g_GameState.gears[gearId].extraWeaponIds[i]].equipGroup == pWeaponList[itemId].equipGroup) {
                        g_GameState.gears[gearId].extraWeaponIds[i] = itemId;
                    }
                }
            }
            break;
        case ITEM_TYPE_GEAR_ACCESSORY:
            changeDetected = TRUE;
            pAccessory = &g_Menu->pDressingRoom->pGearAccessories[itemId];

            for (i = 0; i < 3; i++) {
                int index = itemSlots[5+i];
                pCurAccesory = &g_Menu->pDressingRoom->pGearAccessories[index];

                if (pCurAccesory->equipGroup && pCurAccesory->equipGroup == pAccessory->equipGroup) {
                    changeDetected = FALSE;
                    g_GameState.gears[gearId].accessorySlots[i] = itemId;
                }
            }

            leastDefense = 0xFF;
            if (changeDetected) {
                for (i = 0; i < 3; i++) {
                    int index = itemSlots[5+i];
                    pCurAccesory = &g_Menu->pDressingRoom->pGearAccessories[index];
                    if (leastDefense >= pCurAccesory->defense) {
                        leastDefense = pCurAccesory->defense;
                        smallest = i;
                    }
                }
                g_GameState.gears[gearId].accessorySlots[smallest] = itemId;
            }
            break;
    }

    GearShopMenuRecalculateGearStats(g_Menu->pDressingRoom, g_GameState.characters[characterId].gearId);
    GearShopMenuFillDressingRoomStats(g_Menu->pDressingRoom, g_GameState.characters[characterId].gearId);

    oldValue[0] = g_Menu->pDressingRoom->totalAttack;
    oldValue[1] = g_Menu->pDressingRoom->totalDefense;
    newValue[0] = g_Menu->pShop->previousTotalAttacks[characterId];
    newValue[1] = g_Menu->pShop->previousTotalDefenses[characterId];

    for(i = 0; i < 2; i++) {
        if(oldValue[i] >= newValue[i]) {
            pDelta[i] = oldValue[i] - newValue[i];
            pIsImprovement[i] = FALSE;
        } else {
            pDelta[i] = newValue[i] - oldValue[i];
            pIsImprovement[i] = TRUE;
        }
    }

    g_GameState.gears[gearId].weaponId[0] = itemSlots[0];
    g_GameState.gears[gearId].extraWeaponIds[0] = itemSlots[1];
    g_GameState.gears[gearId].extraWeaponIds[1] = itemSlots[2];
    g_GameState.gears[gearId].extraWeaponIds[2] = itemSlots[3];
    g_GameState.gears[gearId].extraWeaponIds[3] = itemSlots[4];
    g_GameState.gears[gearId].accessorySlots[0] = itemSlots[5];
    g_GameState.gears[gearId].accessorySlots[1] = itemSlots[6];
    g_GameState.gears[gearId].accessorySlots[2] = itemSlots[7];

    GearShopMenuRecalculateGearStats(g_Menu->pDressingRoom, g_GameState.characters[characterId].gearId);
}

int GearShopMenuLookupItemQuantity(u8* index, u8* haystack, s32 size, u8 needle) {
    int i;
    u8 result = 0;

    for(i = 0; i < size; i++) {
        if(index[i] == needle) {
            result = haystack[i];
            break;
        }
    }

    return result;
}

void GearShopMenuBuildItemQuantityHeld(u8 itemType, u8 itemId) {
    RECT rect; // sp18
    u8 digitBuffer[4]; // sp20
    u8 stringBuffer[4]; // sp28
    u8 isValid = FALSE;

    u8 *pIdList;
    u8 *pQtyList;
    int listSize;
    u32 *pWorkspace;
    u16 qtyHeld;

    if(itemType != ITEM_TYPE_GEAR_ACCESSORY) {
        if(itemType == ITEM_TYPE_GEAR_WEAPON) {
            pIdList = g_GameState.gearWeaponsIds;
            pQtyList = g_GameState.gearWeaponsQty;
            listSize = MAX_INVENTORY_WEAPONS;
            isValid = TRUE;
        }
    } else {
        pIdList = g_GameState.gearPartsIds;
        pQtyList = g_GameState.gearPartsQty;
        listSize = MAX_INVENTORY_ITEMS;
        isValid = TRUE;
    }

    if (isValid) {
        g_gearShopCurrentItemQuantityHeld = qtyHeld = GearShopMenuLookupItemQuantity(pIdList, pQtyList, listSize, itemId);

        pWorkspace = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0U);
        digitBuffer[1] = 0;
        digitBuffer[3] = 0;
        digitBuffer[0] = ((qtyHeld / 10) == 0) ? MENU_GLYPH_BLANK : (qtyHeld / 10) + MENU_GLYPH_DIGIT_0;
        digitBuffer[2] = (qtyHeld % 10) + MENU_GLYPH_DIGIT_0;

        SystemDecodeGlyphCodesN((u16 *)digitBuffer, stringBuffer, 2);
        g_Menu->pShop->str45B0.width = SystemRenderStringEntry(stringBuffer, pWorkspace, 0x24, 1);
        setRECT(&rect, 0x198, 0xB4, 0x28, FONT_LETTER_HEIGHT);
        LoadImage(&rect, pWorkspace);
        DrawSync(0);

        GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->str45B0, 9, 0x80, 0x82);
        GearShopMenuSetVertices(g_Menu->pShop->str45B0.vertices, 0xF8, 0x8E, g_Menu->pShop->str45B0.width, FONT_LETTER_HEIGHT);
        g_Menu->pShop->str45B0.renderContext = g_Menu->renderContext;
        g_Menu->pShop->shouldRenderStr45B0 = 1;
        HeapFree(pWorkspace);
    }
}

s32 GearShopMenuBuildPurchaseItemDetails(s32 rowIndex, s32 scrollOffset, u8 *unused) {
    int currentItem = scrollOffset + rowIndex;

    u8 gearId;
    u8 *pStringBuffer;
    int price, totalPrice;
    u8 itemId = g_Menu->shopItemIDs[currentItem];
    u8 itemType = g_Menu->shopItemTypes[currentItem];
    int i, j, currentSlot;
    RECT rect;
    int sp28[2];
    u8 sp30[2];
    GearArmor *pArmor;
    GearFrame *pFrame;
    GearEngine *pEngine;
    GearWeapon *pWeapon;
    GearAccessory *pAccessory;
    int equipFlags;
    int xPos;

    pStringBuffer = HeapAlloc(MENU_ITEM_DESC_IMAGE_BUFFER_SIZE, 0);
    bzero(pStringBuffer, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);

    equipFlags = 0;
    switch(itemType) {
        case ITEM_TYPE_GEAR_ARMOR:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetStringEntry(g_Menu->pShop->pGearFrameDescriptions, itemId), (u32* ) pStringBuffer, 0x39U, 0U);
            pArmor = &g_Menu->pDressingRoom->pGearArmor[itemId];

            price = pArmor->price;
            equipFlags = pArmor->equipFlags;
            break;
        case ITEM_TYPE_GEAR_FRAME:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetStringEntry(g_Menu->pShop->pGearEngineDescriptions, itemId), (u32* ) pStringBuffer, 0x39U, 0U);
            pFrame = &g_Menu->pDressingRoom->pGearFrames[itemId];
            price = pFrame->price;
            equipFlags = pFrame->equipFlags;
            break;
        case ITEM_TYPE_GEAR_ENGINE:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetStringEntry(g_Menu->pShop->pAmmoDescriptions, itemId), (u32* ) pStringBuffer, 0x39U, 0U);
            pEngine = &g_Menu->pDressingRoom->pGearEngines[itemId];
            price = pEngine->price;
            equipFlags = pEngine->equipFlags;
            break;
        case ITEM_TYPE_GEAR_WEAPON:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetStringEntry(g_Menu->pShop->pWeaponDescriptions, itemId), (u32* ) pStringBuffer, 0x39U, 0U);
            pWeapon = &g_Menu->pDressingRoom->pGearWeapons[itemId];
            price = pWeapon->price;
            equipFlags = pWeapon->equipFlags;
            break;
        case ITEM_TYPE_GEAR_ACCESSORY:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetStringEntry(g_Menu->pShop->pAccessoryDescriptions, itemId), (u32* ) pStringBuffer, 0x39U, 0U);
            pAccessory = &g_Menu->pDressingRoom->pGearAccessories[itemId];
            price = pAccessory->price;
            equipFlags = pAccessory->equipFlags;
            break;
    }

    totalPrice = GearShopMenuBuildEquippedCharacterMask(itemId, itemType);
    setRECT(&rect, 0x140, 0x4E, 0x3C, FONT_LETTER_HEIGHT);
    LoadImage(&rect, (u32 *)pStringBuffer);
    DrawSync(0);
    GearShopMenuInitializeMenuStringPolys(&g_Menu->pShop->strItemDesc, 0, 0, 0U);
    GearShopMenuSetPolyFT4Quad(&g_Menu->pShop->strItemDesc.polys[g_Menu->renderContext], 0x2C, 0x12, 0, 0x4E, (s32) g_Menu->pShop->strItemDesc.width, FONT_LETTER_HEIGHT);
    GearShopMenuSetVertices(g_Menu->pShop->strItemDesc.vertices, 0x2C, 0x12, g_Menu->pShop->strItemDesc.width, FONT_LETTER_HEIGHT);

    g_Menu->pShop->strItemDesc.renderContext = g_Menu->renderContext;
    HeapFree(pStringBuffer);
    if(itemId) {
        g_Menu->pShop->shouldRenderItemDesc = TRUE;
    } else {
        g_Menu->pShop->shouldRenderItemDesc = FALSE                                ;
    }

    g_Menu->pShop->equipMarkPrimCount = 0;

    xPos = 0x49;
    for(i = 0, currentSlot = 0; i < 0x10; i++) {
        if(g_Menu->availableCharacters[i] != 0) {
            gearId = g_GameState.characters[i].gearId;
            if(GearShopMenuGearCanEquip(equipFlags, gearId)) {
                g_Menu->pShop->shouldRenderPortraitHighlight[currentSlot] = TRUE;
            } else {
                g_Menu->pShop->shouldRenderPortraitHighlight[currentSlot] = FALSE;
            }

            if(GearShopMenuGearCanEquip(totalPrice, gearId)) {
                g_Menu->pShop->equipMarkPrimCount += func_8002675C(
                    g_Menu->resources,
                    MENU_TEX_EQUIP_MARK,
                    &g_Menu->pShop->polys2D0[g_Menu->pShop->equipMarkPrimCount*2],
                    g_Menu->renderContext,
                    g_GearShopCharacterPortraitX[currentSlot] + 0xE,
                    0xB4,
                    0x1000
                );
            }
            g_Menu->pShop->polys27B0PrimCounts[currentSlot] = 0;
            g_Menu->pShop->polys3020PrimCounts[currentSlot] = 0;
            if( (u8)(itemType + 0xFD) < 2 && g_Menu->pShop->shouldRenderPortraitHighlight[currentSlot]) {
                sp28[1] = 0;
                sp28[0] = 0;
                GearShopMenuComputeEquipStatDelta(&sp28[0], &sp30[0], itemId, itemType, (u8)i);
                if(sp28[0]) {
                    GearShopMenuParseNumberToString(sp28[0]);
                    for(j = 0; j < 3; j++) {
                        if(g_Menu->digits[6+j] != MENU_DIGIT_NONE) {
                            g_Menu->pShop->polys27B0PrimCounts[currentSlot] += func_8002675C(
                                g_Menu->resources,
                                g_Menu->digits[6+j],
                                &g_Menu->pShop->polys27B0[currentSlot][g_Menu->pShop->polys27B0PrimCounts[currentSlot]*2],
                                g_Menu->renderContext,
                                (currentSlot * FONT_LETTER_HEIGHT * 2) + xPos + (8*j),
                                0xBE,
                                0x1000U);
                        }
                    }
                    GearShopMenuSetStatDeltaColor(g_Menu->pShop->polys27B0PrimCounts[currentSlot], &g_Menu->pShop->polys27B0[currentSlot], sp30[0]);
                    g_Menu->pShop->polys27B0RenderCtxs[currentSlot] = g_Menu->renderContext;
                }

                if(sp28[1]) {
                    GearShopMenuParseNumberToString(sp28[1]);
                    for(j = 0; j < 3; j++) {
                        if(g_Menu->digits[6+j] != MENU_DIGIT_NONE) {
                            g_Menu->pShop->polys3020PrimCounts[currentSlot] += func_8002675C(
                                g_Menu->resources,
                                g_Menu->digits[6+j],
                                &g_Menu->pShop->polys3020[currentSlot][g_Menu->pShop->polys3020PrimCounts[currentSlot]*2],
                                g_Menu->renderContext,
                                (currentSlot * FONT_LETTER_HEIGHT * 2) + xPos + (8*j),
                                0xC6,
                                0x1000U);
                        }
                    }
                    GearShopMenuSetStatDeltaColor(g_Menu->pShop->polys3020PrimCounts[currentSlot], &g_Menu->pShop->polys3020[currentSlot], sp30[1]);
                    g_Menu->pShop->polys3020RenderCtxs[currentSlot] = g_Menu->renderContext;
                }
            }
            currentSlot++;
        }

    }
    g_Menu->pShop->equipMarkRenderCtx = g_Menu->renderContext;
    GearShopMenuBuildItemQuantityHeld(itemType, itemId);
    return price;
}

void GearShopMenuFinalizePurchase(u32 newBalance) {
    s32 inventoryIndex;
    s32 itemIndex;
    u8 notFound;

    GearShopMenuPlaySoundEffect(0xD1);
    g_GameState.gold = newBalance;

    if (newBalance > MAX_GOLD_AMOUNT) {
        g_GameState.gold = MAX_GOLD_AMOUNT;
    }
    for(itemIndex = 0; itemIndex < MAX_SHOP_ITEMS; itemIndex++) {
        if ((g_Menu->shopItemIDs[itemIndex] != 0) && (g_Menu->pShop->curItemQuantities[itemIndex] != 0)) {
            switch (g_Menu->shopItemTypes[itemIndex]) {
            case ITEM_TYPE_GEAR_ARMOR:
                g_GameState.gears[g_gearShopCurrentGearId].armorId = g_Menu->shopItemIDs[itemIndex];
                break;
            case ITEM_TYPE_GEAR_FRAME:
                g_GameState.gears[g_gearShopCurrentGearId].frameId = g_Menu->shopItemIDs[itemIndex];
                break;
            case ITEM_TYPE_GEAR_ENGINE:
                g_GameState.gears[g_gearShopCurrentGearId].engineId = g_Menu->shopItemIDs[itemIndex];
                break;
            case ITEM_TYPE_GEAR_WEAPON:
                notFound = 1;

                for(inventoryIndex = 0; inventoryIndex < MAX_INVENTORY_WEAPONS; inventoryIndex++) {
                    if (g_GameState.gearWeaponsIds[inventoryIndex] == g_Menu->shopItemIDs[itemIndex]) {
                        notFound = 0;

                        if( (g_GameState.gearWeaponsQty[inventoryIndex] = g_GameState.gearWeaponsQty[inventoryIndex] + g_Menu->pShop->curItemQuantities[itemIndex]) > MAX_ITEM_QUANTITY) {
                            g_GameState.gearWeaponsQty[inventoryIndex] = MAX_ITEM_QUANTITY;
                        }
                    }
                }

                if(notFound) {
                    for(inventoryIndex = 0; inventoryIndex < MAX_INVENTORY_WEAPONS; inventoryIndex++) {
                        if (g_GameState.gearWeaponsIds[inventoryIndex] == 0) {
                            g_GameState.gearWeaponsIds[inventoryIndex] = g_Menu->shopItemIDs[itemIndex];
                            g_GameState.gearWeaponsQty[inventoryIndex] = g_Menu->pShop->curItemQuantities[itemIndex];
                            break;
                        }
                    }
                }
                break;
            case ITEM_TYPE_GEAR_ACCESSORY:
                notFound = 1;
                for(inventoryIndex = 0; inventoryIndex < MAX_INVENTORY_ITEMS; inventoryIndex++) {
                    if (g_GameState.gearPartsIds[inventoryIndex] == g_Menu->shopItemIDs[itemIndex]) {
                        notFound = 0;
                        if( (g_GameState.gearPartsQty[inventoryIndex] = g_GameState.gearPartsQty[inventoryIndex] + g_Menu->pShop->curItemQuantities[itemIndex]) > MAX_ITEM_QUANTITY) {
                            g_GameState.gearPartsQty[inventoryIndex] = MAX_ITEM_QUANTITY;
                        }
                    }
                }

                if(notFound) {
                    for(inventoryIndex = 0; inventoryIndex < MAX_INVENTORY_ITEMS; inventoryIndex++) {
                        if (g_GameState.gearPartsIds[inventoryIndex] == 0) {
                            g_GameState.gearPartsIds[inventoryIndex] = g_Menu->shopItemIDs[itemIndex];
                            g_GameState.gearPartsQty[inventoryIndex] = g_Menu->pShop->curItemQuantities[itemIndex];
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
}


u8 GearShopMenuIsGearItemUpgrade(s32 itemId) {
    u8 equipped;
    u8 itemType = g_Menu->shopItemTypes[itemId];
    u8 result = TRUE;

    switch(itemType) {
        case ITEM_TYPE_GEAR_ARMOR:
            equipped = g_GameState.gears[g_gearShopCurrentGearId].armorId;
            if(equipped >= g_Menu->shopItemIDs[itemId]) {
                result = FALSE;
            }
            break;
        case ITEM_TYPE_GEAR_FRAME:
            equipped = g_GameState.gears[g_gearShopCurrentGearId].frameId;
            if(equipped >= g_Menu->shopItemIDs[itemId]) {
                result = FALSE;
            }
            break;
        case ITEM_TYPE_GEAR_ENGINE:
            equipped = g_GameState.gears[g_gearShopCurrentGearId].engineId;
            if(equipped >= g_Menu->shopItemIDs[itemId]) {
                result = FALSE;
            }
            break;
    }

    return result;
}


s32 GearShopMenuPurchaseDialog(u8 arg0, u8 disableQuantitySelection) {
    u8 values[8]; // sp28
    int i; // s0
    u8 *temp; // s0
    u8 continueProcessing = TRUE; // s3
    u8 redrawWindows = TRUE; // sp40
    u8 redrawTransactionSummary = TRUE; // fp
    int rowIndex = 0; // s4
    int newRowIndex = 0xFF; // sp48
    int scrollOffset = 0; // s2
    int newScrollOffset = 0xFF; // sp50
    int cartTotal = 0; // s6
    int current_gold; // sp58
    int newProjectedRemainingFunds; // sp60
    int sellPrice; // sp68
    int salesTotal; // sp70
    u8 purchaseConfirmStringId1; // sp78
    u8 purchaseConfirmStringId2; // sp80
    int unkWindowX; // sp88
    int unkWindowWidth; // sp90
    int numItems; // sp98
    int projectedRemainingFunds; // s5
    int itemPrice; // s7

    current_gold = g_GameState.gold;
    purchaseConfirmStringId1 = 0x8F;
    purchaseConfirmStringId2 = MENU_STRING_NONE;
    unkWindowX = 0xE0;
    projectedRemainingFunds = current_gold;
    newProjectedRemainingFunds = projectedRemainingFunds;
    salesTotal = 0;
    unkWindowWidth = 0x40;
    for(i = 0; i < MAX_GAME_CHARACTERS; i++) {
        if(g_GameState.characters[i].gearId != GEAR_ID_NONE) {
            GearShopMenuRecalculateGearStats(g_Menu->pDressingRoom, g_GameState.characters[i].gearId);
            GearShopMenuFillDressingRoomStats(g_Menu->pDressingRoom, g_GameState.characters[i].gearId);
            g_Menu->pShop->previousTotalAttacks[i] = g_Menu->pDressingRoom->totalAttack;
            g_Menu->pShop->previousTotalDefenses[i] = g_Menu->pDressingRoom->totalDefense;
        }
    }

    bzero(g_Menu->shopItemIDs, MAX_SHOP_ITEMS);
    bzero(g_Menu->shopItemTypes, MAX_SHOP_ITEMS);
    bzero(g_Menu->pShop->curItemQuantities, MAX_SHOP_ITEMS);

    for(i = 0; i < MAX_GEAR_SHOP_ITEMS; i++) {
        u_char *table = (u_char *)g_Menu->pShop->gearShopItemTables;
        g_Menu->shopItemIDs[i] = table[((arg0 * 3) + g_Menu->subMenuChoice) * 20 + i];
        g_Menu->shopItemTypes[i] = (arg0 * 3) + g_Menu->subMenuChoice;
    }

    numItems = g_GearShopInventoryCounts[(arg0 * 3) + g_Menu->subMenuChoice];
    g_Menu->pSelectionMenu->isDisabled = 1;
    GearShopMenuInitializeArrowCursor(0);

     while(continueProcessing) {
        GearShopMenuUpdateAndRender();

        if ((scrollOffset != newScrollOffset) || (redrawTransactionSummary)) {
            if (disableQuantitySelection) {
                sellPrice = func_801D2B74(scrollOffset, newProjectedRemainingFunds, values);
            } else {
                sellPrice = func_801D2B74(scrollOffset, projectedRemainingFunds, values);
            }
            GearShopMenuUpdateScrollBarHandle(12, 50, 60, numItems, scrollOffset);
        }

        if ((rowIndex != newRowIndex) || (scrollOffset != newScrollOffset)) {
            newRowIndex = rowIndex;
            itemPrice = GearShopMenuBuildPurchaseItemDetails(rowIndex, scrollOffset, values);
            newScrollOffset = scrollOffset;
            g_Menu->pManager->shouldRenderShop = 1;
            if (disableQuantitySelection) {
                unkWindowX = 0xC8;
                projectedRemainingFunds = newProjectedRemainingFunds;
                unkWindowWidth = 0x70; // 112
                redrawTransactionSummary = TRUE;
                cartTotal = 0;
                salesTotal = 0;
                if (values[rowIndex]) {
                    cartTotal = itemPrice;
                    projectedRemainingFunds -= itemPrice;
                    salesTotal = sellPrice;
                    projectedRemainingFunds += sellPrice;
                }
            }
        }

        GearShopMenuUpdateArrowCursor(rowIndex, scrollOffset, 0, 0);

            if (redrawWindows) {
                // void GearShopMenuInitializeWindow(u_char windowIndex, u_short x, u_short y, u_short width, u_short height, u_char shouldInitializeHandle, u8 arg6, int zIndex, u_char hasScrollbar)
                GearShopMenuInitializeWindow(2, 0xC, 0x2A, 0xC4 - (disableQuantitySelection * 0x14), 0x74, 0, 1, 4, 1);
                GearShopMenuInitializeWindow(3, 0x20, 0xE, 0xFCU, 0x14U, 0U, 1, 4, 0);
                GearShopMenuInitializeWindow(5, unkWindowX, 0x7A, unkWindowWidth, 0x24, FALSE, 1, 4, FALSE);
                temp = g_GearShopPurchaseTotalStringIds;
                GearShopMenuRenderMenuStringsToVram(2, g_Menu->optionStrings, (s8* ) temp, (s8* ) g_Menu->pManager->shouldRenderStrings);
                GearShopMenuSetMenuStringPosition(2, g_Menu->optionStrings, (s32) temp, g_GearShopMenuStringPadding, g_Menu->pManager->shouldRenderStrings, disableQuantitySelection, 0, 1);
                if (disableQuantitySelection) {
                    GearShopMenuBuildGearPartNameGraphics(g_Menu->subMenuChoice + (arg0 * 3));
                }
                GearShopMenuUpdateCharacterPortraits();
                GearShopMenuBuildPurchaseExplanations(disableQuantitySelection);
                redrawWindows = FALSE;
            }

        if (redrawTransactionSummary) {
            GearShopMenuBuildTransactionSummary(current_gold, cartTotal, projectedRemainingFunds, salesTotal, disableQuantitySelection);
            redrawTransactionSummary = FALSE;
        }
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:

                if (cartTotal != 0) {
                    GearShopMenuPlaySoundEffect(MENU_SOUND_CONFIRM);
                    g_Menu->pManager->shouldRenderShop = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                    continueProcessing = FALSE;
                    g_Menu->pManager->shouldRenderStrings[disableQuantitySelection] = FALSE;
                    if (disableQuantitySelection) {
                        if (GearShopMenuIsGearItemUpgrade(scrollOffset + rowIndex) & 0xFF) {
                            purchaseConfirmStringId1 = MENU_STRING_CONFIRM_PURCHASE;
                        } else {
                            purchaseConfirmStringId1 = MENU_STRING_NOT_AN_UPGRADE;
                            purchaseConfirmStringId2 = MENU_STRING_BUY_ANYWAY;
                        }
                    } else {
                        GearShopMenuBuildFinalPriceGraphics(cartTotal, 0);
                    }
                    GearShopMenuInitializePointerCursors(0);
                    if (GearShopMenuShowModal(purchaseConfirmStringId1, purchaseConfirmStringId2, TRUE) != MENU_CHOICE_NO) {
                        if (disableQuantitySelection) {
                            g_Menu->pShop->curItemQuantities[scrollOffset + rowIndex] = 1;
                        }
                        GearShopMenuFinalizePurchase(projectedRemainingFunds);
                    } else {
                        continueProcessing = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[5] = TRUE;
                        g_Menu->pManager->shouldRenderStrings[disableQuantitySelection] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        newScrollOffset = 0xFF;
                        newRowIndex = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                    }
                    GearShopMenuFreePointerCursors();
                    purchaseConfirmStringId2 = MENU_STRING_NONE;
                } else {
                    GearShopMenuPlaySoundEffect(MENU_SOUND_ERROR);
                }
                break;
            case MENU_INPUT_BACK:
                g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                continueProcessing = FALSE;
                g_Menu->pManager->shouldRenderStrings[disableQuantitySelection] = 0;
                if(cartTotal && !disableQuantitySelection) {
                    g_Menu->pManager->shouldRenderShop = 0;
                    g_Menu->pManager->shouldRenderWindow[2] = 0;
                    g_Menu->pManager->shouldRenderWindow[3] = 0;
                    g_Menu->pManager->scrollHandleActive = 0;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
                    GearShopMenuInitializePointerCursors(0U);
                    // display the "Cancel all purchases?" modal; returns 1 for "yes" and 0 for "no"
                    if (GearShopMenuShowModal(MENU_STRING_CANCEL_ALL_PURCHASES, MENU_STRING_NONE, TRUE) == MENU_CHOICE_NO) { // user chose "no" so reactivate the windows and keep on
                        continueProcessing = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = 1;
                        g_Menu->pManager->shouldRenderWindow[3] = 1;
                        g_Menu->pManager->shouldRenderWindow[5] = 1;
                        g_Menu->pManager->shouldRenderStrings[0] = 1;
                        g_Menu->pManager->scrollHandleActive = 1;
                        newScrollOffset = 0xFF;
                        newRowIndex = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = 1;

                    }
                    GearShopMenuFreePointerCursors();
                }
                break;
            case MENU_INPUT_DOWN:
                rowIndex += 1;

                if (rowIndex >= SHOP_MAX_ITEMS_IN_VIEW) {
                    rowIndex = SHOP_MAX_ITEMS_IN_VIEW - 1;
                    scrollOffset += 1;
                    if ((numItems - SHOP_MAX_ITEMS_IN_VIEW) < scrollOffset) {
                        scrollOffset -= 1;
                    }
                }
                break;
            case MENU_INPUT_UP:
                rowIndex -= 1;

                if (rowIndex < 0) {
                    scrollOffset -= 1;
                    rowIndex = 0;
                    if (scrollOffset < 0) {
                        scrollOffset = 0;
                    }
                }
                break;
            case MENU_INPUT_RIGHT:
                if(values[rowIndex]) {
                    if(!disableQuantitySelection) {
                        if ((g_Menu->pShop->curItemQuantities[scrollOffset + rowIndex] + 1 + (g_gearShopCurrentItemQuantityHeld)) <= MAX_ITEM_QUANTITY) {
                            cartTotal += itemPrice;
                            projectedRemainingFunds -= itemPrice;
                            redrawTransactionSummary = TRUE;
                            g_Menu->pShop->curItemQuantities[scrollOffset + rowIndex]++;
                        }
                    }
                }

                break;
            case MENU_INPUT_LEFT:
                if(!disableQuantitySelection) {
                    if(g_Menu->pShop->curItemQuantities[scrollOffset+rowIndex]) {
                        cartTotal -= itemPrice;
                        projectedRemainingFunds += itemPrice;
                        redrawTransactionSummary = TRUE;
                        g_Menu->pShop->curItemQuantities[scrollOffset+rowIndex] += 0xFF;
                    }
                }
                break;
        }
    }
    g_Menu->pShop->shouldRenderStr45B0 = 0;
    GearShopMenuFreeWindow(5);
    GearShopMenuClearStringRenderFlags(2, (s8* ) g_Menu->pManager->shouldRenderStrings);
    GearShopMenuResetShopUi(1);
    return 1;
}

// Handles the logic for the "Fuel" option under "Tune up"
void GearShopMenuTuneUpFuel(void) {
    u8 var_s1;
    u8 var_s4;
    s32 var_s6;
    u16 fuelUnits;
    s32 var_s0;
    u16 temp_v1;
    u8 var_s2;

    var_s4 = FALSE;
    var_s2 = 0xA6;
    var_s1 = 1;
    var_s6 = TRUE;
    if(g_GameState.gears[g_gearShopCurrentGearId].maxFuel == g_GameState.gears[g_gearShopCurrentGearId].fuel) {
        var_s2 = 0xB5;
        if(g_GameState.gears[g_gearShopCurrentGearId].hp == g_GameState.gears[g_gearShopCurrentGearId].maxHp) {
            var_s2 = 0xB8;
            var_s1 = 0;
            var_s6 = FALSE;
        } else {
            var_s1 = 2;
        }
    }

    fuelUnits = (g_GameState.gears[g_gearShopCurrentGearId].maxFuel - g_GameState.gears[g_gearShopCurrentGearId].fuel) / MENU_FUEL_UNIT_AMOUNT;
    if(fuelUnits == 0) {
        fuelUnits = 1;
    }

    var_s0 = fuelUnits * MENU_FUEL_UNIT_PRICE;

    GearShopMenuInitializeWindow(5, 0xA2, 0xA6, 0x60, 0x14, 0, 1, 4, 0);
    GearShopMenuBuildGoldGraphics(g_GameState.gold);
    if (var_s1 != 0) {
        GearShopMenuBuildFinalPriceGraphics(var_s0, 1);
    }
    GearShopMenuUpdateAndRender();
    GearShopMenuInitializePointerCursors(0);
    if (GearShopMenuShowModal(var_s2, MENU_STRING_NONE, var_s6) != 0) {
        g_Menu->pManager->finalPriceRenderState = MENU_FINAL_PRICE_OFF;
        switch (var_s1) {
            case 1:
                if (g_GameState.gold < var_s0) {
                    if (GearShopMenuShowModal(0xA9U, MENU_STRING_NONE, 1) != 0) {
                        if (g_GameState.gold != 0) {
                            var_s4 = TRUE;
                        }

                        var_s0 = g_GameState.gold / MENU_FUEL_UNIT_PRICE;
                        g_GameState.gears[g_gearShopCurrentGearId].fuel += ((var_s0) * MENU_FUEL_UNIT_AMOUNT);
                        g_GameState.gold %= MENU_FUEL_UNIT_PRICE;
                        temp_v1 = g_GameState.gears[g_gearShopCurrentGearId].maxFuel;
                        if (temp_v1 < g_GameState.gears[g_gearShopCurrentGearId].fuel) {
                            g_GameState.gears[g_gearShopCurrentGearId].fuel = temp_v1;
                        }
                    }
                } else {
                    g_GameState.gold -= var_s0;
                    g_GameState.gears[g_gearShopCurrentGearId].fuel = g_GameState.gears[g_gearShopCurrentGearId].maxFuel;
                    var_s4 = TRUE;
                }
            break;
            case 2:
                var_s4 = TRUE;
                break;
        }
    }
    if (var_s4 != FALSE) {
        g_GameState.gears[g_gearShopCurrentGearId].hp = g_GameState.gears[g_gearShopCurrentGearId].maxHp;
        GearShopMenuPlaySoundEffect(0xD1);
    }
    GearShopMenuFreePointerCursors(0);
    g_Menu->pManager->isPanelActive[1] = 0;
    GearShopMenuFreeWindow(5);
}


u_char GearShopMenuResetTuneUpSubmenu(void) {
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    GearShopMenuClearStringRenderFlags(4, g_Menu->pManager->shouldRenderStrings);
    GearShopMenuResetShopUi(0);
    return 2;
}

// Called if we chose "Tune up", dispatches into the correct menu based on
// the submenu choice
void GearShopMenuTuneUpMenuHandleSelectedOption(void) {
    switch(g_Menu->subMenuChoice) {
        case MENU_CHOICE_ARMOR:
        case MENU_CHOICE_FRAME:
        case MENU_CHOICE_ENGINE:
            GearShopMenuPurchaseDialog(0, 1);
            GearShopMenuRecalculateGearStats(g_Menu->pDressingRoom, g_gearShopCurrentGearId);
            break;
        case MENU_CHOICE_FUEL:
            GearShopMenuTuneUpFuel();
    }
}

s32 GearShopMenuShopModeSubmenu(void) {
    u8 continueProcessing = TRUE;
    u8 redrawWindows = TRUE;
    u8 var_s0 = 0;

    if ((g_Menu->mainMenuChoice == 3) && !GearShopMenuResetTuneUpSubmenu()) return 1;

    g_Menu->subMenuChoice = 0;
    g_Menu->subMenuPrevChoice = 0xFF;
    if (g_Menu->mainMenuChoice == 3) {
        g_Menu->subMenuChoice = MENU_CHOICE_FUEL;
        var_s0 = 4;
    }

    while(continueProcessing) {
            GearShopMenuUpdateAndRender();
            if (g_Menu->transitionEffectState == MENU_ANIMATION_DONE) {
                if (g_gearShopAvailableCharacterCount >= 2) {
                    g_Menu->pManager->shoulderButtonUiActive = 1;
                }
            }
            if (redrawWindows) {
                GearShopMenuRenderMenuStringsToVram(4, g_Menu->optionStrings, &g_GearShopSubmenuStringIds[var_s0], (s8* ) g_Menu->pManager->shouldRenderStrings);
                GearShopMenuInitializeSubmenu(0);
                redrawWindows = FALSE;
            }
            if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
                GearShopMenuSetMenuStringPosition(4, g_Menu->optionStrings, (s32) &g_GearShopSubmenuStringIds[var_s0], &g_GearShopMenuStringPadding[var_s0], g_Menu->pManager->shouldRenderStrings, g_Menu->subMenuChoice, 4, 0);
                GearShopMenuUpdateSubmenuTextures(0);
                g_Menu->subMenuPrevChoice = g_Menu->subMenuChoice;
            }
            if (g_Menu->menuUnk8->optionMarkerStates[0] == MENU_MARKER_STATE_IDLE) {
                g_Menu->pManager->isPanelActive[3] = 1;
                g_Menu->pManager->isPanelActive[2] = 0;
            }

            switch (g_Menu->input) {
                case MENU_INPUT_CONFIRM:
                    g_Menu->pManager->shouldRenderPointerCursor = 0;
                    g_Menu->pManager->unk3 = 0;
                    g_Menu->pManager->shouldRenderSubmenuStrings = 0;
                    g_Menu->pManager->isPanelActive[3] = 0;
                    GearShopMenuClearStringRenderFlags(4, (s8* ) g_Menu->pManager->shouldRenderStrings);
                    GearShopMenuPlaySoundEffect(MENU_SOUND_CONFIRM);
                    GearShopMenuOpenGearPanel();
                    g_Menu->pManager->shoulderButtonUiActive = 0;
                    switch (g_Menu->mainMenuChoice) {              /* switch 2; irregular */
                        case 1:
                            GearShopMenuSellModeMenuHandleSelectedOption(1);
                            break;
                        case 2:
                            GearShopMenuPurchaseDialog(1, 0);
                            break;
                        case 3:
                            GearShopMenuTuneUpMenuHandleSelectedOption();
                            break;
                    }
                    GearShopMenuCloseGearPanel();
                    GearShopMenuUpdateAndRender();
                    GearShopMenuBuildGearStatsGraphics();
                    GearShopMenuUpdateAndRender();
                    g_Menu->pManager->shouldRenderPointerCursor = 1;
                    g_Menu->pManager->unk3 = 1;
                    g_Menu->pManager->shouldRenderSubmenuStrings = 1;
                    GearShopMenuRenderMenuStringsToVram(4, g_Menu->optionStrings, &g_GearShopSubmenuStringIds[var_s0], (s8* ) g_Menu->pManager->shouldRenderStrings);
                    g_Menu->subMenuPrevChoice = 0xFF;
                case MENU_INPUT_LEFT:                             /* switch 1 */
                case MENU_INPUT_SQUARE:             /* switch 1 */
                case MENU_INPUT_TRIANGLE:           /* switch 1 */
                case MENU_INPUT_IDLE:                             /* switch 1 */
                    break;
                case MENU_INPUT_BACK:                             /* switch 1 */
                    continueProcessing = 0;
                    break;
                case MENU_INPUT_DOWN:                             /* switch 1 */
                    if (g_Menu->subMenuChoice != 0) {
                        (s8)g_Menu->subMenuChoice -= 1;
                    } else {
                        g_Menu->subMenuChoice = g_Menu->subMenuNumChoices + 0xFF;
                    }
                    break;
                case MENU_INPUT_UP:                             /* switch 1 */
                    if ( ++g_Menu->subMenuChoice >= (u8) g_Menu->subMenuNumChoices) {
                        g_Menu->subMenuChoice = 0;
                    }
                    break;
                case MENU_INPUT_NEXT:                             /* switch 1 */
                    GearShopMenuChangeCurrentGear(0);
                    break;
                case MENU_INPUT_PREV:                            /* switch 1 */
                    GearShopMenuChangeCurrentGear(1);
                    break;
            }
    }
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    GearShopMenuClearStringRenderFlags(4, (s8* ) g_Menu->pManager->shouldRenderStrings);

    return 1;
}

void GearShopMenuInitializeGearView(void) {
    s32 firstAvailable = 0;

    LibGearInitialize(0x40);

    g_Menu->viewMatrix.m[0][0] = 0x546;
    g_Menu->viewMatrix.m[0][1] = -0xE39;
    g_Menu->viewMatrix.m[0][2] = 0x546;
    g_Menu->viewMatrix.m[1][0] = 0;
    g_Menu->viewMatrix.m[1][1] = 0;
    g_Menu->viewMatrix.m[1][2] = 0;
    g_Menu->viewMatrix.m[2][0] = 0;
    g_Menu->viewMatrix.m[2][1] = 0;
    g_Menu->viewMatrix.m[2][2] = 0;

    g_Menu->colorMatrix.m[0][0] = 0x600;
    g_Menu->colorMatrix.m[0][1] = 0;
    g_Menu->colorMatrix.m[0][2] = 0;
    g_Menu->colorMatrix.m[1][0] = 0x600;
    g_Menu->colorMatrix.m[1][1] = 0;
    g_Menu->colorMatrix.m[1][2] = 0;
    g_Menu->colorMatrix.m[2][0] = 0x600;
    g_Menu->colorMatrix.m[2][1] = 0;
    g_Menu->colorMatrix.m[2][2] = 0;

    D_801E8644 = (s16*)&g_Menu->colorMatrix;
    SetBackColor(0x3C, 0x3C, 0x3C);

    g_Menu->gfxEnvs[0].otBuffer = g_MenuGfxEnvOtBuffer0[0];
    g_Menu->gfxEnvs[1].otBuffer = g_MenuGfxEnvOtBuffer0[1];

    g_Menu->pGearModelArchiveData[0] = HeapAlloc(sizeof(SystemMenuArchiveData), 0);
    bzero(g_Menu->pGearModelArchiveData[0], sizeof(SystemMenuArchiveData));

    g_Menu->pGearModelArchiveData[1] = HeapAlloc(sizeof(SystemMenuArchiveData), 0);
    bzero(g_Menu->pGearModelArchiveData[1], sizeof(SystemMenuArchiveData));

    while(g_Menu->availableCharacters[firstAvailable] == 0) {
        firstAvailable++;
    }

    g_gearShopCurrentGearId = g_GameState.characters[firstAvailable].gearId;

    GearShopMenuLoadGearModelFiles(g_gearShopCurrentGearId, 1);
    GearShopMenuSetAvailableCharacterCount();
}

void GearShopMenuFreeGearView(void) {
    while (g_Menu->transitionEffectState != MENU_ANIMATION_DONE) {
        GearShopMenuUpdateAndRender();
    }

    g_GearShopDebugEnabled = 0;
    g_Menu->pManager->isPanelActive[0] = 0;
    GearShopMenuUpdateAndRender();
    LibGearShutdown();
    g_Menu->pGearModelArchiveData[0]->isModelCreated = 0;
    g_Menu->pGearModelArchiveData[1]->isModelCreated = 0;
    HeapFree(g_Menu->pGearModelArchiveData[0]);
    HeapFree(g_Menu->pGearModelArchiveData[1]);
}

void GearShopMenuFillDressingRoomStats(MenuDressingRoom* pView, u_char gearId) {
    GameGear* pGear;
    GameCharacter* pPilot;
    u_short ether;
    int etherModifier;

    if(g_GameState.unk22B6 & 0x1000) {
        D_801D70FD = 0xA;
    }

    pGear = &g_GameState.gears[gearId];
    pPilot = &g_GameState.characters[g_GearShopGearPilotIds[gearId]];

    pView->hp = pGear->hp;
    pView->maxHp = pGear->maxHp;
    pView->totalDefense = pGear->baseDefense + pGear->defenseFromEquipment;

    pView->totalEtherDefense = pPilot->etherDefense + pPilot->etherDefenseFromEquipment + pGear->etherDefenseFromEquipment + pGear->baseEtherDefense;
    pView->totalWeight = pGear->weight + pGear->weightFromEquipment;
    pView->unkAA = pGear->unk6A;
    pView->fuel = pGear->fuel;
    pView->maxFuel = pGear->maxFuel;

    ether = pGear->engineOutput * (pGear->attackPower + pGear->unk55[1]);

    if(gearId == 0x5 || gearId == 0xD) {
        etherModifier = ((pGear->weaponData[0].attack + pGear->weaponData[2].attack) * 6) / 10;
        pView->totalAttack = etherModifier + ether;
    } else {
        pView->totalAttack = pGear->weaponData[0].attack + ether;
    }

    pView->totalResponsiveness = pGear->responsivenessFromFrame + pGear->responsiveness;
    pView->totalAgility = pGear->agility - pGear->weightPenalty;
    pView->totalEtherAmplification = pGear->etherAmplification;
    pView->unkB5 = pGear->unk9D;
    pView->unkB6 = pGear->unk9C;
}

void GearShopMenuRecalculateGearStats(MenuDressingRoom* arg0, u8 arg1) {
    GearShopMenuApplyGearFrame(arg0, arg1);
    GearShopMenuApplyGearEngine(arg0, arg1);
    GearShopMenuApplyGearArmor(arg0, arg1);
    GearShopMenuApplyGearAccessories(arg0, arg1);
    GearShopMenuApplyGearWeapons(arg0, arg1);
}

void GearShopMenuApplyGearFrame(MenuDressingRoom* arg0, u_char gearIndex) {
    GameGear* pGear;
    GearFrame* pPreview;
    u_char previewId;

    pGear = &g_GameState.gears[gearIndex];
    pPreview = arg0->pGearFrames;
    previewId = pGear->frameId;
    pPreview = &pPreview[previewId];

    pGear->maxHp = pPreview->hp;

    pGear->weight = pPreview->weight;
    pGear->agility = pPreview->agility;
    pGear->etherAmplification = pPreview->etherAmplification;
    pGear->unk9D = pPreview->unk16;
    pGear->responsivenessFromFrame = pPreview->responsiveness;
    if (pGear->maxHp < pGear->hp) {
        pGear->hp = pGear->maxHp;
    }
}

void GearShopMenuApplyGearArmor(MenuDressingRoom* menu, u_char gearIndex) {
    GameGear* pGear;
    GearArmor* itemList;
    GearArmor* pItem;

    pGear = &g_GameState.gears[gearIndex];
    itemList = menu->pGearArmor
;
    pItem = &itemList[pGear->armorId];

    pGear->baseDefense = pItem->baseDefense;
    pGear->baseEtherDefense = pItem->baseEtherDefense;
}

void GearShopMenuApplyGearEngine(MenuDressingRoom* shopInfo, u8 gearId) {
    GameGear* pGear;
    GearEngine *item;
    u16 fuel;
    u8 itemId;

    pGear = &g_GameState.gears[gearId];
    itemId = pGear->engineId;
    item = shopInfo->pGearEngines;
    fuel = pGear->fuel;
    item = &item[itemId];

    pGear->maxFuel = item->maxFuel;
    pGear->engineOutput = item->powerOutput;
    pGear->unk3D = item->unkD;
    pGear->unk3E = item->unkE;
    pGear->unk3F = item->unkE;

    if (pGear->maxFuel < fuel) {
        pGear->fuel = pGear->maxFuel;
    }
}


void GearShopMenuApplyGearAccessories(MenuDressingRoom* dressingRoom, u8 gearId) {
    GameGear* pGear;
    GearAccessory* pAccessory;
    u8 accessoryInd;
    u8 i;
    u8 characterId;
    u16 *pFlags1, *pFlags2;

    pGear = &g_GameState.gears[gearId];
    characterId = g_GearShopGearPilotIds[gearId];

    pFlags1 = &g_GameState.conditionRows[characterId].gearCommandMask;
    pFlags2 = &g_GameState.conditionRows[characterId].statusFlags;

    pGear->defenseFromEquipment = 0;
    pGear->etherDefenseFromEquipment = 0;
    pGear->weightFromEquipment = 0;
    pGear->accessoryEffectFlags9 = 0;
    pGear->unk4C = 0;
    pGear->responsiveness = 0;
    pGear->unk4E = 0;
    pGear->accessoryEffectFlags5 = 0;
    pGear->accessoryEffectFlags4 = 0;


    for(i = 0; i < 16; i++) {
        pGear->accessoryEffectAmounts[i] = 0;
    }

    for(i = 0; i < 4; i++) {
        pGear->unk50[i] = 0;
    }

    for(i = 0; i < 3; i++) {
        pGear->unk55[i] = 0;
    }

    pGear->etherAmplificationFromEquipment = 0;
    pGear->accessoryEffectFlags1 = 0;
    pGear->accessoryEffectFlags2 = 0;
    pGear->accessoryEffectFlags3 &= 0xF000;
    *pFlags1 &= 0xDB7F;

    for(accessoryInd = 0; accessoryInd < 3; accessoryInd++) {
        pAccessory = dressingRoom->pGearAccessories;
        pAccessory += pGear->accessorySlots[accessoryInd];

        pGear->defenseFromEquipment += pAccessory->defense;
        pGear->etherDefenseFromEquipment += pAccessory->etherDefense;
        pGear->weightFromEquipment += pAccessory->weight;
        pGear->unk4C += pAccessory->unk18;
        pGear->responsiveness += pAccessory->responsiveness;

        for(i = 0; i < 4; i++) {
            pGear->unk50[i] += pAccessory->unk10[i];
        }

        switch(pAccessory->effectType) {
            case 1:
                pGear->accessoryEffectFlags1 |= pAccessory->effectFlags;
                break;
            case 2:
                pGear->accessoryEffectFlags2 |= pAccessory->effectFlags;
                break;
            case 3:
                pGear->accessoryEffectFlags3 |= pAccessory->effectFlags;
                break;
            case 4:
                for(i = 0; i < 16; i++) {
                    pGear->accessoryEffectFlags4 |= pAccessory->effectFlags;
                    if(pAccessory->effectFlags & (0x8000 >> i)) {
                        pGear->accessoryEffectAmounts[i] += pAccessory->effectAmount;
                    }
                }
                break;
            case 5:
                pGear->accessoryEffectFlags5 += pAccessory->effectFlags;
                break;
            case 6:
                if( (*pFlags1 & 0x1000) && (*pFlags1 & 0x800) )
                    *pFlags1 |= 0x400;
                break;
            case 7:
                if( (*pFlags1 & 0x200) && (*pFlags1 & 0x100) )
                    *pFlags1 |= 0x80;
                break;
            case 8:
                if( (*pFlags1 & 0x40) && (*pFlags1 & 0x20) ) {
                    *pFlags1 |= 0x10;
                }
                break;
            case 9:
                pGear->accessoryEffectFlags9 |= pAccessory->effectFlags;
            case 10:
                pGear->unk55[1] += (u8) pAccessory->effectFlags;
                break;
            case 11:
                pGear->unk55[2] += (u8) pAccessory->effectFlags;
                break;
        }
    }

    pGear->weightPenalty = GearShopMenuComputeEquipWeightPenalty(gearId);
    if (pGear->accessoryEffectFlags5) {
        *pFlags2 |= 0x8000;
    }
    else if (gearId == g_GameState.characters[g_GearShopGearPilotIds[gearId]].gearId) {
        *pFlags2 &= 0x7FFF;
    }
}

void GearShopMenuApplyGearWeapons(MenuDressingRoom* dressingRoom, u8 gearId) {
    GameGear* pGear = &g_GameState.gears[gearId];
    GearWeapon *pList = dressingRoom->pGearWeapons;
    GearWeapon *pWeapon;

    pWeapon = dressingRoom->pGearWeapons;
    pWeapon += pGear->weaponId[0];
    pGear->weaponData[0].attack = pWeapon->attack;
    pGear->weaponData[0].flags = pWeapon->flags;
    pGear->weaponData[0].chance = pWeapon->unk10;
    pGear->weaponData[0].conditionKind = pWeapon->unk11;

    pGear->unk5C[0] = pWeapon->unk0[0];
    pGear->unk5C[1] = pWeapon->unk0[1];
    pGear->unk5C[2] = pWeapon->unk0[2];
    pGear->unk5C[3] = pWeapon->unk0[3];

    if (pGear->weaponData[0].conditionKind == 100) {
        pGear->accessoryEffectFlags3 = pGear->accessoryEffectFlags3 & 0xFFF;
        pGear->accessoryEffectFlags3 |= pGear->weaponData[0].flags;
    }
    if ((gearId == 5) || (gearId == 0xD)) {
        pWeapon = dressingRoom->pGearWeapons;
        pWeapon += pGear->extraWeaponIds[0];

        pGear->weaponData[0].attack = pWeapon->attack;
        pGear->weaponData[0].flags = pWeapon->flags;
        pGear->weaponData[0].chance = pWeapon->unk10;
        pGear->weaponData[0].conditionKind = pWeapon->unk11;
        pGear->unk5C[1] = pWeapon->unk0[1];

        pWeapon = dressingRoom->pGearWeapons;
        pWeapon += pGear->extraWeaponIds[1];
        pGear->weaponData[1].attack = pWeapon->attack;
        pGear->weaponData[1].flags = pWeapon->flags;
        pGear->weaponData[1].chance = pWeapon->unk10;
        pGear->weaponData[1].conditionKind = pWeapon->unk11;
        pGear->unk5C[2] = pWeapon->unk0[2];

        pWeapon = dressingRoom->pGearWeapons;
        pWeapon += pGear->extraWeaponIds[3];
        pGear->weaponData[2].attack = pWeapon->attack;
        pGear->weaponData[2].flags = pWeapon->flags;
        pGear->weaponData[2].chance = pWeapon->unk10;
        pGear->weaponData[2].conditionKind = pWeapon->unk11;
        pGear->unk5C[3] = pWeapon->unk0[3];
    }
}

s32 GearShopMenuComputeEquipWeightPenalty(u8 index) {
    GameGear* pGear;
    s32 temp_v0;
    u8 result;

    pGear = &g_GameState.gears[index];
    temp_v0 = (s32) ((pGear->weightFromEquipment / MENU_WEIGHT_POINTS_PER_CLASS) - pGear->baseWeightClass) / 2;
    result = temp_v0;
    if (temp_v0 << 16 < 0) {
        result = 0;
    }
    return result;
}
