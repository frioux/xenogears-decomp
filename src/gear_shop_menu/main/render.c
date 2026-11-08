#include "common.h"
#include "system/menu.h"
#include "system/controller.h"
#include "system/archive.h"
#include "system/graphics.h"
#include "system/debug.h"
#include "util/copyspec.h"
#include "gear_shop/gear_shop.h"
#include "libgear/main.h"

extern s32 D_80050100;
extern s32 D_8005A4AC[2];

s32 func_801C5260(u8);
void func_801CB690();
void GearShopMenuUpdateAndRender();

// TODO: remove once matched
s32 func_801D2B74(s32, u32, u8*);

u8 func_8001BD40(int, u8);

void GearShopMenuShoulderButtonUiInitialize(void) {
    int i;
    int polyIndex;

    for(i = 0; i < 2; i++) {
        func_8002675C(
            g_Menu->resources,
            MENU_TEX_L1_RED_ARROW + i,
            &g_Menu->pShoulderButtonUi->polys[i * 4],
            g_Menu->renderContext,
            D_801D6FD0[i],
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

void func_801CE32C(void) {
    int i;

    if (g_Menu->pManager->unk5A) {
        for(i = 0; i < 9; i++) {
            if (g_Menu->pShop->unk469C[i]) {
                AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->linesPortraitHighlight1[(i*2) + g_Menu->renderContext]);
                AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->linesPortraitHighlight2[(i*2) + g_Menu->renderContext]);
            }
        }

        GearShopMenuRenderString(g_Menu->pShop->explanationsLen, g_Menu->pShop->polysExplanations, g_Menu->pShop->explanationsRenderCtx);
        GearShopMenuRenderString(g_Menu->pShop->unk46A9, g_Menu->pShop->polys2D0, g_Menu->pShop->unk46A8);
        GearShopMenuRenderString(g_Menu->pShop->numPortraits, g_Menu->pShop->polysCharacterPortraits, g_Menu->pShop->portraitsRenderCtx);
        for(i = 0; i < 8; i++) {
            if (g_Menu->pShop->unk4684[i]) {
                GearShopMenuRenderPolygons(1, g_Menu->pShop->unk3C30[i].vertices, g_Menu->pShop->unk3C30[i].polys, g_Menu->pShop->unk3C30[i].renderContext);
                GearShopMenuRenderPolygons(1, g_Menu->pShop->unk4030[i].vertices, g_Menu->pShop->unk4030[i].polys, g_Menu->pShop->unk4030[i].renderContext);
            }
        }
        if (g_Menu->pShop->unk46B6[0]) {
            GearShopMenuRenderPolygons(1, g_Menu->pShop->str4530.vertices, g_Menu->pShop->str4530.polys, g_Menu->pShop->str4530.renderContext);
        }

        if (g_Menu->pShop->unk46A7) {
            GearShopMenuRenderPolygons(1, g_Menu->pShop->strItemDesc.vertices, g_Menu->pShop->strItemDesc.polys, g_Menu->pShop->strItemDesc.renderContext);
        }

        if (g_Menu->pShop->unk46B5) {
            GearShopMenuRenderPolygons(1, g_Menu->pShop->str44B0.vertices, g_Menu->pShop->str44B0.polys, g_Menu->pShop->str44B0.renderContext);
        }

        if (g_Menu->pShop->unk4785 != 0) {
            GearShopMenuRenderPolygons(1, g_Menu->pShop->str45B0.vertices, g_Menu->pShop->str45B0.polys, g_Menu->pShop->str45B0.renderContext);
        }

        if (g_Menu->pShop->unk46B2 != 0) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->lines3BF0[g_Menu->renderContext]);
            GearShopMenuRenderString(g_Menu->pShop->goldBeforeStrLen, g_Menu->pShop->polysGoldBefore, g_Menu->pShop->goldBeforeRenderCtx);
            GearShopMenuRenderString(g_Menu->pShop->totalPriceStrLen, g_Menu->pShop->polysTotalPrice, g_Menu->pShop->totalPriceRenderCtx);
            GearShopMenuRenderString(g_Menu->pShop->goldAfterStrLen, g_Menu->pShop->polysGoldAfter, g_Menu->pShop->goldAfterRenderCtx);
            if (g_Menu->pShop->unk46B6[3] != 0) {
                GearShopMenuRenderString(g_Menu->pShop->unk46B6[2], g_Menu->pShop->unk14F0, g_Menu->pShop->unk46B6[1]);
            }
        }

        for(i = 0; i < 8; i++) {
            GearShopMenuRenderString(g_Menu->pShop->unk468C[i], g_Menu->pShop->polysShopInventory[i*4], g_Menu->pShop->unk4694[i]);
        }

        for(i = 0; i < 9; i++) {
            GearShopMenuRenderString(g_Menu->pShop->unk46BC[i], g_Menu->pShop->polys27B0[i], g_Menu->pShop->unk46CE[i]);
            GearShopMenuRenderString(g_Menu->pShop->unk46C5[i], g_Menu->pShop->polys3020[i], g_Menu->pShop->unk46D7[i]);
        }
    }
    if (g_Menu->pManager->unk5B == 1) {
        GearShopMenuRenderString(g_Menu->pShop->finalPriceStrLen, g_Menu->pShop->polysFinalPrice, g_Menu->pShop->finalPriceRenderCtx);
    }
    if (g_Menu->pManager->unk5C[8] != 0) {
        GearShopMenuRenderString(g_Menu->pShop->unk46B6[5], g_Menu->pShop->unk17C0, g_Menu->pShop->unk46B6[4]);
    }
}


/*
 This is the function in gear_shop_menu that calls into LibGear
 */
void func_801CE7E0(void) {
    // most likely: if helper library is loaded into memory
    if (g_Menu->pManager->unk5C[7]) {
        func_801E7D14(&g_Menu->unk230, &g_Menu->unk298, g_Menu->pGfxEnv->unkB0, g_Menu->renderContext);
    }
}

void func_801CE82C(void) {
    s32 i;

    if (g_Menu->pManager->unk5C[9] != 0) {
        if (g_Menu->menuUnk8->unk1ED8 != 0) {
            GearShopMenuRenderString(g_Menu->menuUnk8->unk1ED0, g_Menu->menuUnk8->polysD0, g_Menu->menuUnk8->unk1ED4);
            GearShopMenuRenderString(g_Menu->menuUnk8->unk1ED0, &g_Menu->menuUnk8->polysD0[4], g_Menu->menuUnk8->unk1ED4);
            GearShopMenuRenderString(g_Menu->menuUnk8->unk1ED0, &g_Menu->menuUnk8->polysD0[8], g_Menu->menuUnk8->unk1ED4);
        }
        for(i = 0; i < 2; i++) {
            if (g_Menu->menuUnk8->unk1ED9[i] != 0) {
                GearShopMenuRenderString(g_Menu->menuUnk8->unk1ED1[i], &g_Menu->menuUnk8->polys2B0[(i*0x16)], g_Menu->menuUnk8->unk1ED5[i]);
            }
        }
        if (g_Menu->menuUnk8->unk1EDB != 0) {
            GearShopMenuRenderString(g_Menu->menuUnk8->unk1ED3, g_Menu->menuUnk8->polysD00, g_Menu->menuUnk8->unk1ED7);
        }
    }
    if (g_Menu->pManager->unk5C[0xA] != 0) {
        GearShopMenuRenderString(1U, g_Menu->menuUnk8->unk0.polys, g_Menu->renderContext);
        GearShopMenuRenderString(0xEU, g_Menu->menuUnk8->polys12A0, g_Menu->menuUnk8->unk1EED);
        GearShopMenuRenderString(g_Menu->menuUnk8->unk1EE8, g_Menu->menuUnk8->polys1700, g_Menu->menuUnk8->unk1EED);
        GearShopMenuRenderString(g_Menu->menuUnk8->unk1EE9, g_Menu->menuUnk8->polys1890, g_Menu->menuUnk8->unk1EED);
        GearShopMenuRenderString(g_Menu->menuUnk8->unk1EEA, g_Menu->menuUnk8->polys1A20, g_Menu->menuUnk8->unk1EED);
        GearShopMenuRenderString(g_Menu->menuUnk8->unk1EEB, g_Menu->menuUnk8->polys1BB0, g_Menu->menuUnk8->unk1EED);
        GearShopMenuRenderString(g_Menu->menuUnk8->unk1EEC, g_Menu->menuUnk8->polys1D40, g_Menu->menuUnk8->unk1EED);
    }
}

void func_801CEA68(void) {
    u16 coords[2][4];
    u8 needsUpdate;
    s32 i;
    s32 k;

    for(i = 0; i < 2; i++) {
        needsUpdate = FALSE;
        if (g_Menu->menuUnk8->unk1ED9[i] != 0) {
            switch(g_Menu->menuUnk8->unk1ED9[i]) {
            case 1:
                coords[0][i] = D_801D7040[i];
                coords[1][i] = D_801D7044[(i * 6) + D_801D7030[(g_Menu->mainMenuChoice << 2) + g_Menu->subMenuChoice]];
                needsUpdate = TRUE;
                break;
            case 2:
                if (func_8001BD40(0, 0xFF) < 0x10) {
                    coords[0][i] = D_801D7040[i];
                    coords[1][i] = D_801D7044[(i * 6) + func_8001BD40(0, 5)];
                    needsUpdate = TRUE;
                }
                break;
            }

            if (needsUpdate) {
                g_Menu->menuUnk8->unk1EF4[i] = coords[0][i];
                g_Menu->menuUnk8->unk1EF8[i] = coords[1][i];
            }

            if (g_Menu->menuUnk8->unk1EDD[i] != 0) {
                g_Menu->menuUnk8->unk1EDD[i]--;
                continue;
            }

            g_Menu->menuUnk8->unk1ED1[i] = 0;
            for(k = 0; k < 4; k++) {
                if (D_801D6FE0[(i * 0x14) + ((g_Menu->menuUnk8->unk1EE4[i] << 2) + k)] != 0xFFFF) {
                    g_Menu->menuUnk8->unk1ED1[i] += func_8002675C(
                        g_Menu->resources,
                        D_801D6FE0[(i * 0x14) + ((g_Menu->menuUnk8->unk1EE4[i] << 2) + k)],
                        &g_Menu->menuUnk8->polys2B0[(i * 0x16) + (g_Menu->menuUnk8->unk1ED1[i] * 2)],
                        g_Menu->renderContext,
                        g_Menu->menuUnk8->unk1EF4[i],
                        g_Menu->menuUnk8->unk1EF8[i],
                        0x1000
                    );
                }
            }

            g_Menu->menuUnk8->unk1ED5[i] = g_Menu->renderContext;

            switch(g_Menu->menuUnk8->unk1ED9[i]) {
            case 1:
                if (++g_Menu->menuUnk8->unk1EE4[i] == 4) {
                    g_Menu->menuUnk8->unk1ED9[i] = 2;
                }
                g_Menu->menuUnk8->unk1EDD[i] = 2;
                break;
            case 2:
                if (++g_Menu->menuUnk8->unk1EE4[i] >= 5) {
                    g_Menu->menuUnk8->unk1EE4[i] = 3;
                }
                g_Menu->menuUnk8->unk1EDD[i] = 8;
                break;
            case 3:
                if (--g_Menu->menuUnk8->unk1EE4[i] < 0) {
                    g_Menu->menuUnk8->unk1ED9[i] = 0;
                    g_Menu->menuUnk8->unk1ED8 = 0;
                }
                g_Menu->menuUnk8->unk1EDD[i] = 2;
                break;
            }
        }
    }
}

void func_801CEEA8(void) {
    u8 needsUpdate;
    u16 var_s1;
    u16 var_s2;

    if (g_Menu->menuUnk8->unk1EDB) {
        // update timer?
        if (g_Menu->menuUnk8->unk1EDF != 0) {
            (s8)g_Menu->menuUnk8->unk1EDF -= 1;
            return;
        }

        needsUpdate = FALSE;
        switch(g_Menu->menuUnk8->unk1EDB) {
            case 1:
                var_s2 = D_801D705C[D_801D7030[(g_Menu->mainMenuChoice * 4) + g_Menu->subMenuChoice]];
                var_s1 = D_801D7068[D_801D7030[(g_Menu->mainMenuChoice * 4) + g_Menu->subMenuChoice]];
                needsUpdate = TRUE;
                break;
            case 2:
                if(func_8001BD40(0, 0xFF) < 4) {
                    var_s2 = D_801D705C[func_8001BD40(0, 5)];
                    var_s1 = D_801D7068[func_8001BD40(0, 5)];
                    needsUpdate = TRUE;
                }
                break;
        }

        if(needsUpdate) {
            g_Menu->menuUnk8->unk1EFC = var_s2;
            g_Menu->menuUnk8->unk1EFE = var_s1;
        }

        g_Menu->menuUnk8->unk1ED3 = func_8002675C(g_Menu->resources,
                      g_Menu->menuUnk8->unk1EE2 + 0x172,
                      g_Menu->menuUnk8->polysD00,
                      g_Menu->renderContext,
                      g_Menu->menuUnk8->unk1EFC,
                      g_Menu->menuUnk8->unk1EFE,
                      0x1000);

        g_Menu->menuUnk8->unk1ED7 = (u8) g_Menu->renderContext;
        g_Menu->menuUnk8->unk1EDF = 1;

        switch (g_Menu->menuUnk8->unk1EDB) {
        case 1:
            if (++g_Menu->menuUnk8->unk1EE2 >= 3) {
                g_Menu->menuUnk8->unk1EDB = 2;
            }
            break;
        case 2:
            if (++g_Menu->menuUnk8->unk1EE2 >= 11) {
                g_Menu->menuUnk8->unk1EE2 = 3;
            }
            break;
        case 3:
            g_Menu->menuUnk8->unk1EE2 = 2;
            g_Menu->menuUnk8->unk1EDB = 4;
            break;
        case 4:
            if (--g_Menu->menuUnk8->unk1EE2 < 0) {
                g_Menu->menuUnk8->unk1EDB = 0;
            }
            break;
        }
    }
}

void func_801CF184(void) {
    MenuUnk8* pMenu;
    int i;
    short t1, t2;

    pMenu = g_Menu->menuUnk8;

    if (pMenu->unk1ED8) {
        if(pMenu->unk1EDC) {
            g_Menu->menuUnk8->unk1EDC--;
        } else {
            if(func_8001BD40(0, 0xFF) < 8) {
                t1 = D_801D7074[func_8001BD40(0, 5)];
                t2 = D_801D7080[func_8001BD40(0, 5)];
                g_Menu->menuUnk8->unk1EF0 = t1;
                g_Menu->menuUnk8->unk1EF2 = t2;
            }

            for(i = 0; i < 3; i++) {
                g_Menu->menuUnk8->unk1ED0 = func_8002675C(
                    g_Menu->resources,
                    g_Menu->menuUnk8->unk1EE0 + 0x167,
                    &g_Menu->menuUnk8->polysD0[i * 4],
                    g_Menu->renderContext,
                    g_Menu->menuUnk8->unk1EF0 + i * 0x8,
                    g_Menu->menuUnk8->unk1EF2 + i * 0xA,
                    0x1000
                );
            }

            g_Menu->menuUnk8->unk1EE0 ^= 1;
            g_Menu->menuUnk8->unk1ED4 = g_Menu->renderContext;
            g_Menu->menuUnk8->unk1EDC = 4;
        }
    }
}

void func_801CF33C(void) {
    if (g_Menu->pManager->unk5C[9]) {
        func_801CEA68();
        func_801CEEA8();
        func_801CF184();
    }
}

// Render gear name
void func_801CF38C(u_char stringIndex) {
    RECT rect;

    D_801D9088 = HeapAlloc(0x3F6, 0x0);
    bzero(D_801D9088, 0x3F6);
    SystemRenderStringEntry(&g_GameState.strings[stringIndex], D_801D9088, 0x24, 0);
    setRECT(&rect, 0x180, 0x48, 0x28, 0xD);
    LoadImage(&rect, (u32* ) D_801D9088);
    DrawSync(0);
    HeapFree(D_801D9088);
}

// Render gear stats (Fuel, HP, Weight)
// Resource IDs for text:
// 0x11, 0x19 = HP
// 0xF, 0x1E, 0xE, 0x15 = FUEL
// 0x20, 0xE, 0x12, 0x10, 0x11, 0x1D = WEIGHT
void func_801CF448(void) {
    int i, j;

    for (j = 0; j < 0xE; j++) {
        func_8002675C(
            g_Menu->resources,
            D_801D708C[j], // Resource ID
            &g_Menu->menuUnk8->polys12A0[j * 2],
            g_Menu->renderContext,
            D_801D70A8[j], D_801D70C4[j],
            0x1000
        );
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].hp);
    g_Menu->menuUnk8->unk1EE8 = 0;
    for (i = 0, j = 0; i < 5; i++) {

        if (g_Menu->digits[i + 4] != 0xFF) {
            g_Menu->menuUnk8->unk1EE8 += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 4],
                &g_Menu->menuUnk8->polys1700[g_Menu->menuUnk8->unk1EE8 * 2],
                g_Menu->renderContext,
                D_801D70E0 + (i * 8), D_801D70E2,
                0x1000
            );
        }
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].maxHp);
    g_Menu->menuUnk8->unk1EE9 = 0;
    for (i = 0, j = 0; i < 5; i++) {
        if (g_Menu->digits[i + 4] != 0xFF) {
            g_Menu->menuUnk8->unk1EE9 += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 4],
                &g_Menu->menuUnk8->polys1890[g_Menu->menuUnk8->unk1EE9 * 2],
                g_Menu->renderContext,
                D_801D70E4 + (j * 8), D_801D70E6,
                0x1000
            );
            j++;
        }
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].fuel);
    g_Menu->menuUnk8->unk1EEA = 0;
    for (i = 0, j = 0; i < 4; i++) {
        if (g_Menu->digits[i + 5] != 0xFF) {
            g_Menu->menuUnk8->unk1EEA += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 5],
                &g_Menu->menuUnk8->polys1A20[g_Menu->menuUnk8->unk1EEA * 2],
                g_Menu->renderContext,
                D_801D70E8 + (i * 8), D_801D70EA,
                0x1000
            );
        }
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].maxFuel);
    g_Menu->menuUnk8->unk1EEB = 0;
    for (i = 0, j = 0; i < 4; i++) {
        if (g_Menu->digits[i + 5] != 0xFF) {
            g_Menu->menuUnk8->unk1EEB += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 5],
                &g_Menu->menuUnk8->polys1BB0[g_Menu->menuUnk8->unk1EEB * 2],
                g_Menu->renderContext,
                D_801D70EC + (j * 8), D_801D70EE,
                0x1000
            );
            j++;
        }
    }

    GearShopMenuParseNumberToString(g_GameState.gears[g_gearShopCurrentGearId].weight);
    g_Menu->menuUnk8->unk1EEC = 0;
    for (i = 0; i < 5; i++) {
        if (g_Menu->digits[i + 4] != 0xFF) {
            g_Menu->menuUnk8->unk1EEC += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i + 4],
                &g_Menu->menuUnk8->polys1D40[g_Menu->menuUnk8->unk1EEC * 2],
                g_Menu->renderContext,
                D_801D70F0 + (i * 8), D_801D70F2,
                0x1000
            );
        }
    }

    // Render the gear name
    func_801CF38C((g_gearShopCurrentGearId + 0xB));

    g_Menu->menuUnk8->unk1EED = g_Menu->renderContext;
}

void func_801CF9BC(u_char gearId, u8 arg1) {
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);

    g_Menu->unk458[arg1]->unk0 = HeapAlloc(ArchiveDecodeAlignedSize(D_801D6D7C[gearId]), 0);
    ArchiveReadFileToBuffer(D_801D6D7C[gearId], g_Menu->unk458[arg1]->unk0, 0, 0x80);
    ArchiveCdDataSync(0);

    g_Menu->unk458[arg1]->unk4 = HeapAlloc(ArchiveDecodeAlignedSize(D_801D6D7C[gearId] + 1), 0);
    ArchiveReadFileToBuffer(D_801D6D7C[gearId] + 1, g_Menu->unk458[arg1]->unk4, 0, 0x80);
    ArchiveCdDataSync(0);

    ArchiveSetIndex(ARCHIVE_DIR_MENUS, 0);
}

void func_801CFAB8(u8 modelIndex, u8 gearId) {
    u8 characterId = 0;
    s32 s3 = modelIndex;
    s3 &= 0xFF;

    func_801E742C(
        modelIndex,
        0,
        g_Menu->unk458[modelIndex]->unk0,
        g_Menu->unk458[modelIndex]->unk4,
        (modelIndex * 64) + 0x200,
        0,
        0,
        (s3 & 0xFF) + 0x1C0,
        &g_Menu->unk458[modelIndex]->unk8
    );

    g_LibGearModels[modelIndex]->ground = D_801D6DB4[gearId];
    g_LibGearModels[modelIndex]->scale = D_801D6DD8[gearId];
    g_LibGearModels[1]->pSkeleton->rotation.vy -= 0x400;
    g_LibGearModels[1]->pSkeleton->rotation.vx -= 0x20;
    D_80050100 = 0;

    if(gearId != 0xFF) {
        characterId = D_801D6DA0[gearId];
    }
    func_801E8330((s3 & 0xFF), 0, characterId );
    HeapFree(g_Menu->unk458[modelIndex]->unk4);
    g_Menu->unk458[modelIndex]->unk12 = 1;
    func_801CF448();
    g_Menu->pManager->unk5C[0xA] = 1;
}

void func_801CFC60(void) {
    s32 temp;
    g_gearShopTransitionState.start.cameraX = g_gearShopTransitionState.target.cameraX;
    g_gearShopTransitionState.start.cameraY = g_gearShopTransitionState.target.cameraY;
    g_gearShopTransitionState.start.gearY   = g_gearShopTransitionState.target.gearY;
    g_gearShopTransitionState.target.cameraX = D_801D6DFC[g_gearShopCurrentGearId];
    temp = ((g_Menu->mainMenuChoice + (g_gearShopCurrentGearId * 3)) * 4) + g_Menu->subMenuChoice;
    g_gearShopTransitionState.target.cameraY = D_801D6E18[temp];
    g_gearShopTransitionState.target.gearY = D_801D6FB0[(g_Menu->mainMenuChoice * 4) + g_Menu->subMenuChoice];
    func_801CB690();
    g_Menu->transitionEffectState = 7;
    g_Menu->menuUnk8->unk1ED8 = 1;
    g_Menu->menuUnk8->unk1ED9[0] = 1;
    g_Menu->menuUnk8->unk1ED9[1] = 1;
    g_Menu->menuUnk8->unk1EDB = 1;
    g_Menu->menuUnk8->unk1EDC = 0;
    g_Menu->menuUnk8->unk1EDD[0] = 0;
    g_Menu->menuUnk8->unk1EDD[1] = 0;
    g_Menu->menuUnk8->unk1EDF = 0;
    g_Menu->menuUnk8->unk1EE0 = 0;
    g_Menu->menuUnk8->unk1EE4[0] = 0;
    g_Menu->menuUnk8->unk1EE4[1] = 0;
    g_Menu->menuUnk8->unk1EE2 = 0;
    g_Menu->menuUnk8->unk1EF0 = 0x18;
    g_Menu->menuUnk8->unk1EF2 = 0x6E;
    g_Menu->pManager->unk5C[9] = 1;

    while (g_Menu->menuUnk8->unk1ED9[0] != 2) {
        GearShopMenuUpdateAndRender();
    }

    while (g_Menu->menuUnk8->unk1EDB != 2) {
        GearShopMenuUpdateAndRender();
    }
}

void func_801CFF18(void) {
    g_Menu->menuUnk8->unk1ED9[0] = 3;
    g_Menu->menuUnk8->unk1ED9[1] = 3;
    g_Menu->menuUnk8->unk1EDB = 3;
    g_Menu->menuUnk8->unk1EDC = 0;
    g_Menu->menuUnk8->unk1EDD[0] = 0;
    g_Menu->menuUnk8->unk1EDD[1] = 0;
    g_Menu->menuUnk8->unk1EDF = 0;
    g_Menu->pManager->unk5C[9] = 1;

    pushXYZ(
         0x400, 0, -0x400,
        &g_gearShopTransitionState.target.cameraX,
        &g_gearShopTransitionState.target.cameraY,
        &g_gearShopTransitionState.target.gearY,
        &g_gearShopTransitionState.start.cameraX,
        &g_gearShopTransitionState.start.cameraY,
        &g_gearShopTransitionState.start.gearY

    );

    func_801CB690();
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION|MENU_CLOSE_ANIMATION|MENU_CLOSE_ANIMATION_START;
}

void func_801D0054(int count, POLY_FT4 *pPolys, u_char mode) {
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

    g_Menu->pManager->unk5A = 1;

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
                    D_801D6C44[numCharacters], 166,
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

    if ((g_Menu->transitionEffectState == 0) && (g_gearShopAvailableCharacterCount >= 2)) {
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
        g_Menu->unk458[1]->unk12 = 0;
        func_801E8030(1);
        GearShopMenuUpdateAndRender();
        func_801CF9BC(g_GameState.characters[characterIndex].gearId, 1);
        g_gearShopCurrentGearId = g_GameState.characters[characterIndex].gearId;
        func_801CFAB8(1, g_GameState.characters[characterIndex].gearId);
        GearShopMenuUpdateAndRender();
    }
}

// GearShopMenuUpdate ??? MenuExplanationGraphics
void func_801D04E8(u8 renderContext) {
    int i;

    g_Menu->pShop->explanationsLen = 0;
    for(i = 0; i < 4; i++) {
        g_Menu->pShop->explanationsLen += func_8002675C(
            g_Menu->resources,
            D_801D6D08[(renderContext*4) + i],
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen*2],
            g_Menu->renderContext,
            D_801D6D14[(renderContext*4) + i],
            D_801D6D3C[(renderContext*4) + i],
            0x1000
        );

    }

    g_Menu->pShop->explanationsRenderCtx = g_Menu->renderContext;
}

// GearShopMenuUpdate ??? MenuExplanationGraphics
void func_801D05EC(void) {
    int i;

    g_Menu->pShop->explanationsLen = 0;

    for(i = 0; i < 2; i++) {
        g_Menu->pShop->explanationsLen += func_8002675C(
            g_Menu->resources,
            D_801D6D10[i],
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen*2],
            g_Menu->renderContext,
            D_801D6D34[i],
            D_801D6D5C[i],
            0x1000
        );
    }

    g_Menu->pShop->explanationsRenderCtx = g_Menu->renderContext;
}

void func_801D06D8(u32 startingBalance, u32 totalPrice, u32 newBalance, u32 salesTotal, u8 renderSalesTotal) {
    int i;

    for(i = 0; i < 2; i++) {
        SetLineF2(&g_Menu->pShop->lines3BF0[i]);
        setRGB0(&g_Menu->pShop->lines3BF0[i], 0xFF, 0xFF, 0xFF);
        setXY2(&g_Menu->pShop->lines3BF0[i],
                D_801D6D6C - 8 - renderSalesTotal * 16,
                D_801D6D70 + 9 - renderSalesTotal * 8,
                D_801D6D6C + 0x4e - renderSalesTotal * 16,
                D_801D6D70 + 9 - renderSalesTotal * 8
               );
    }

    GearShopMenuParseNumberToString(startingBalance);

    g_Menu->pShop->goldBeforeStrLen = 0;
    for(i = 0; i < 9; i++) {
        if(g_Menu->digits[i] != 0xFF) {
            g_Menu->pShop->goldBeforeStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->polysGoldBefore[g_Menu->pShop->goldBeforeStrLen*2],
                g_Menu->renderContext,
                ((i * 8) + D_801D6D64) - (renderSalesTotal * 0x10),
                D_801D6D68 - (renderSalesTotal * 0x10),
                0x1000
            );
        }
    }
    g_Menu->pShop->goldBeforeRenderCtx = (u8) g_Menu->renderContext;

    GearShopMenuParseNumberToString(totalPrice);
    g_Menu->pShop->totalPriceStrLen = 0;
    for(i = 0; i < 9; i++) {
        if(g_Menu->digits[i] != 0xFF) {
            g_Menu->pShop->totalPriceStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->polysTotalPrice[g_Menu->pShop->totalPriceStrLen*2],
                g_Menu->renderContext,
                ((i * 8) + D_801D6D6C) - (renderSalesTotal * 0x10),
                D_801D6D70 - (renderSalesTotal * 0x10),
                0x1000
            );
        }
    }
    g_Menu->pShop->totalPriceRenderCtx = (u8) g_Menu->renderContext;

    GearShopMenuParseNumberToString(newBalance);
    g_Menu->pShop->goldAfterStrLen = 0;
    for(i = 0; i < 9; i++) {
        if(g_Menu->digits[i] != 0xFF) {
            g_Menu->pShop->goldAfterStrLen += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->polysGoldAfter[g_Menu->pShop->goldAfterStrLen*2],
                g_Menu->renderContext,
                ((i * 8) + D_801D6D74) - (renderSalesTotal * 0x10),
                D_801D6D78 - (renderSalesTotal * 8), 0x1000U
            );
        }
    }
    g_Menu->pShop->goldAfterRenderCtx = (u8) g_Menu->renderContext;

    g_Menu->pShop->unk46B6[2] = 0;
    g_Menu->pShop->unk46B6[3] = 0;

    if (renderSalesTotal) {
        GearShopMenuParseNumberToString(salesTotal);
        for(i = 0; i < 9; i++) {
            if(g_Menu->digits[i] != 0xFF) {
                g_Menu->pShop->unk46B6[2] += func_8002675C(
                    g_Menu->resources,
                    g_Menu->digits[i],
                    &g_Menu->pShop->unk14F0[(g_Menu->pShop->unk46B6[2])*2],
                    g_Menu->renderContext,
                    ((i * 8) + D_801D6D6C) - (renderSalesTotal * 0x10),
                    D_801D6D70 - (renderSalesTotal * 8),
                    0x1000
                );
            }
        }
        g_Menu->pShop->unk46B6[1] = (u8) g_Menu->renderContext;
        g_Menu->pShop->unk46B6[3] = 1;
    }
    g_Menu->pShop->unk46B2 = 1;
}

// GearShopMenuSetFinalPriceGraphics ?
void func_801D0C20(int number, u8 arg1) {

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
        if (g_Menu->digits[i] != 0xFF) {
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
    g_Menu->pManager->unk5B = 2;
}

void func_801D0D4C(u32 value) {
    int i;

    GearShopMenuParseNumberToString(value);
    g_Menu->pShop->unk46B6[5] = 0;

    for(i = 0; i < 9; i++) {
        if(g_Menu->digits[i] != 0xFF) {
            g_Menu->pShop->unk46B6[5] += func_8002675C(
                g_Menu->resources,
                g_Menu->digits[i],
                &g_Menu->pShop->unk17C0[g_Menu->pShop->unk46B6[5] * 2],
                g_Menu->renderContext,
                0xAA + (i * 8),
                0xAE,
                0x1000
            );
        }
    }

    g_Menu->pShop->unk46B6[5] += func_8002675C(
        g_Menu->resources,
        0x10,
        &g_Menu->pShop->unk17C0[g_Menu->pShop->unk46B6[5] * 2],
        g_Menu->renderContext,
        0xF2,
        0xAE,
        0x1000
    );
    g_Menu->pShop->unk46B6[4] = g_Menu->renderContext;
    g_Menu->pManager->unk5C[8] = 1;
}

void func_801D0EC8(u8 arg0) {
    int i;

    g_Menu->pManager->unk5A = 0;
    g_Menu->pShop->unk46A7 = 0;
    g_Menu->pShop->unk46B5 = 0;
    g_Menu->pShop->unk46B2 = 0;
    g_Menu->pShop->explanationsLen = 0;
    g_Menu->pShop->unk46A9 = 0;
    g_Menu->pShop->numPortraits = 0;
    for(i = 0; i < 9; i++) {
        g_Menu->pShop->unk469C[i] = 0;
        g_Menu->pShop->unk46BC[i] = 0;
        g_Menu->pShop->unk46C5[i] = 0;
    }
    for(i = 0; i < 8; i++) {
        g_Menu->pShop->unk4684[i] = 0;
        g_Menu->pShop->unk468C[i] = 0;
    }

    g_Menu->pShop->unk46B6[0] = 0;
    if (arg0) {
        GearShopMenuFreeWindow(2);
        GearShopMenuFreeWindow(3);
        GearShopMenuFreeScrollBarHandle();
        GearShopMenuFreeArrowCursor(0);
    }
}

s32 func_801D1078(u8 itemId, u8 itemType) {
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
                            if (itemId < 0x32) {
                                if(g_GameState.gears[g_GameState.characters[i].gearId].weaponId[j] == itemId) {
                                    found = TRUE;
                                    break;
                                }
                            } else {
                                if(g_GameState.gears[g_GameState.characters[i].gearId].unk4[j] == itemId) {
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
                result |= func_801C5260(g_GameState.characters[i].gearId);
            }
        }
    }

    return result;
}

u32 func_801D1304(u8 arg0, u8 itemType) {
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

    temp_v0 = HeapAlloc(0x618U, 0U);
    bzero(temp_v0, 0x618);
    bzero(sp28, 0xE);
    if (itemType != ITEM_TYPE_GEAR_ACCESSORY) {
        if (itemType == ITEM_TYPE_GEAR_WEAPON) {
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(func_80033A5C(arg0), temp_v0, 0x39U, 0U);
            var_s5 = g_Menu->pDressingRoom->pGearWeapons[arg0].equipFlags;
            var_s6 = g_Menu->pDressingRoom->pGearWeapons[arg0].price >> 1;
            price = var_s6;
        }
        // @bug if itemType != 3 or 4 then var_s6 is never set but still returned
    } else {
        g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(func_80033A2C(arg0), temp_v0, 0x39U, 0U);
        temp_v0_3 = &g_Menu->pDressingRoom->pGearAccessories[arg0];
        var_s5 = temp_v0_3->equipFlags;
        var_s6 = temp_v0_3->price >> 1;
        price = var_s6;
    }
    temp_s4 = func_801D1078(arg0, itemType);
    didRounding = FALSE;
    for(j = 4, k = 0; j > 0; k++, j--) {
        quotient = price / places[j];
        if(quotient || didRounding) {
            sp28[k * 2] = quotient + 0x10;
            didRounding = TRUE;
            price -= (quotient * places[j]);
        } else {
            sp28[k * 2] = 0xC3;
        }
    }
    sp28[8] = (price % 10) + 0x10;
    func_80033B34((u16* ) &sp28, sp38, 5);
    g_Menu->pShop->str44B0.width = SystemRenderStringEntry(&sp38, temp_v0, 0x39U, 1U);
    setRECT((RECT*)&sp20, 0x140, 0x4E, 0x3C, 0xD);
    LoadImage((RECT*)&sp20, (u32* ) temp_v0);
    DrawSync(0);
    func_801C5CA8(&g_Menu->pShop->strItemDesc, 0, 0, 0U);
    func_801C5CA8(&g_Menu->pShop->str44B0, 0, 0, 0U);
    g_Menu->pShop->str44B0.polys[g_Menu->renderContext].clut = g_SystemPalette2;
    func_801C51B8(&g_Menu->pShop->strItemDesc.polys[g_Menu->renderContext], 0x2C, 0x12, 0U, 0x4EU, g_Menu->pShop->strItemDesc.width, 0xD);
    func_801C51B8(&g_Menu->pShop->str44B0.polys[g_Menu->renderContext], 0x98, 0x12, 0U, 0x4EU, g_Menu->pShop->str44B0.width, 0xD);
    GearShopMenuSetVertices(g_Menu->pShop->strItemDesc.vertices, 0x2C, 0x12, g_Menu->pShop->strItemDesc.width, 0xD);
    GearShopMenuSetVertices(g_Menu->pShop->str44B0.vertices, 0x98, 0x12, g_Menu->pShop->str44B0.width, 0xD);
    g_Menu->pShop->strItemDesc.renderContext = g_Menu->renderContext;
    g_Menu->pShop->str44B0.renderContext = g_Menu->renderContext;
    HeapFree(temp_v0);
    if (arg0 & 0xFF) {
        g_Menu->pShop->unk46A7 = 1;
        g_Menu->pShop->unk46B5 = 1;
    } else {
        g_Menu->pShop->unk46A7 = 0;
        g_Menu->pShop->unk46B5 = 0;
    }
    g_Menu->pShop->unk46A9 = 0;
    ;
    for (k = 0, j = 0; k < 0x10; k++) {
        if (g_Menu->availableCharacters[k] != 0) {
            gearId = g_GameState.characters[k].gearId;
            if (GearShopMenuGearCanEquip(var_s5, gearId) != 0) {
                g_Menu->pShop->unk469C[j] = 1;
            } else {
                g_Menu->pShop->unk469C[j] = 0;
            }
            if (GearShopMenuGearCanEquip(temp_s4, gearId) != 0) {
                g_Menu->pShop->unk46A9 += func_8002675C(g_Menu->resources, 0xE, &g_Menu->pShop->polys2D0[g_Menu->pShop->unk46A9 * 2], g_Menu->renderContext, D_801D6C44[j] + 0xE, 0xB4, 0x1000U);
            }
            j += 1;
        }
    }
    g_Menu->pShop->unk46A8 = (u8) g_Menu->renderContext;
    return var_s6;
}

INCLUDE_ASM("asm/gear_shop_menu/nonmatchings/main/render", func_801D18F8);
// 92.03%: frame size, instruction count of every block and the spill set are right;
// the only remaining delta is which of two loop values gcc keeps in $fp: the original
// keeps &sp20 there and spills 0x3C30 + i*0x80 to 0x90($sp), this version does the
// reverse (and so puts i in $s2 instead of $s3). Everything else is identical.
/*
void func_801D18F8(s32 scrollOffset, u8* pItemIds, u8* pItemTypes, u8* pItemSellQtys, u8* pItemQtys) {
    RECT sp20;
    u8 sp28[0xE];
    u8 sp38[0xE];
    int places[5]; // sp48
    s32 quotient;
    u8 didRounding;
    s32 j;
    s32 k;
    u8 itemType;
    u8 tensDigit;
    u8 onesDigit;
    s32 qty;
    void* pWorkspace;
    int i;

    places[0] = 1;
    places[1] = 10;
    places[2] = 100;
    places[3] = 1000;
    places[4] = 10000;

    pWorkspace = HeapAlloc(0x3F6, 0);

    for (i = 0; i < 8; i++) {
        bzero(sp28, 0xE);
        g_Menu->pShop->unk468C[i] = 0;
        if (pItemIds[scrollOffset + i]) {
            qty = pItemQtys[scrollOffset + i];
            itemType = pItemTypes[scrollOffset + i];
            if (itemType != ITEM_TYPE_GEAR_ACCESSORY) {
                if (itemType == ITEM_TYPE_GEAR_WEAPON) {
                    g_Menu->pShop->unk3C30[i].width = SystemRenderStringEntry(func_80033A5C(pItemIds[scrollOffset + i]), pWorkspace, 0x24U, 0U);
                }
            } else {
                g_Menu->pShop->unk3C30[i].width = SystemRenderStringEntry(func_80033A2C(pItemIds[scrollOffset + i]), pWorkspace, 0x24U, 0U);
            }
            didRounding = FALSE;
            for (j = 4, k = 0; j > 0; k++, j--) {
                quotient = qty / places[j];
                if (quotient || didRounding) {
                    sp28[k * 2] = quotient + 0x10;
                    didRounding = TRUE;
                    qty -= (quotient * places[j]);
                } else {
                    sp28[k * 2] = 0xC3;
                }
            }
            sp28[8] = (qty % 10) + 0x10;
            func_80033B34((u16*)&sp28, sp38, 5);
            g_Menu->pShop->unk4030[i].width = SystemRenderStringEntry(&sp38, pWorkspace, 0x24U, 1U);
            setRECT(&sp20, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage((RECT*)&sp20, pWorkspace);
            func_801C5CA8(&g_Menu->pShop->unk3C30[i], i, 0x80, 0x81);
            GearShopMenuSetVertices(g_Menu->pShop->unk3C30[i].vertices, 0x24, 0x32 + FONT_LETTER_HEIGHT * i, g_Menu->pShop->unk3C30[i].width, FONT_LETTER_HEIGHT);
            LoadImage((RECT*)&sp20, pWorkspace);
            DrawSync(0);
            func_801C5CA8(&g_Menu->pShop->unk4030[i], i, 0x80, 0x82);
            GearShopMenuSetVertices(g_Menu->pShop->unk4030[i].vertices, 0x8C, 0x32 + FONT_LETTER_HEIGHT * i, g_Menu->pShop->unk4030[i].width, FONT_LETTER_HEIGHT);
            g_Menu->pShop->unk3C30[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->unk3C30[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->unk4684[i] = 1;
            if (pItemSellQtys[scrollOffset + i]) {
                g_Menu->pShop->unk468C[i] += func_8002675C(g_Menu->resources, 0xE5, g_Menu->pShop->polysShopInventory[i * 4], g_Menu->renderContext, 0xB4, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000U);
                tensDigit = pItemSellQtys[scrollOffset + i] / 10;
                if (tensDigit) {
                    g_Menu->pShop->unk468C[i] += func_8002675C(g_Menu->resources, tensDigit, &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->unk468C[i]], g_Menu->renderContext, 0xBC, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000U);
                }
                onesDigit = pItemSellQtys[scrollOffset + i] % 10;
                g_Menu->pShop->unk468C[i] += func_8002675C(g_Menu->resources, onesDigit, &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->unk468C[i]], g_Menu->renderContext, 0xC4, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000U);
                g_Menu->pShop->unk4694[i] = g_Menu->renderContext;
            }
        } else {
            g_Menu->pShop->unk4684[i] = 0;
        }
    }

    HeapFree(pWorkspace);
}
*/

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
            func_801D18F8(scrollOffset, pItemIds, pItemTypes, pItemSellQtys, pItemQtys);
            GearShopMenuUpdateScrollBarHandle(0xC, 0x32, 0x3C, itemCount, scrollOffset);
        }

        if ((rowIndex != newRowIndex) || (scrollOffset != newScrollOffset)) {
            newRowIndex = rowIndex;
            newScrollOffset = scrollOffset;
            itemPrice = func_801D1304(pItemIds[scrollOffset+rowIndex], pItemTypes[scrollOffset+rowIndex]);
            g_Menu->pManager->unk5A = 1;
        }

        GearShopMenuUpdateArrowCursor(rowIndex, scrollOffset, 0, 0);

        if (redrawWindows) {
            GearShopMenuInitializeWindow(2, 0xC, 0x2A, 0xC4, 0x74, 0, 1, 4, 1);
            GearShopMenuInitializeWindow(3, 0x20, 0xE, 0xFC, 0x14, 0, 1, 4, 0);
            GearShopMenuUpdateCharacterPortraits();
            func_801D05EC();
            redrawWindows = FALSE;
        }
        if (redrawTransactionSummary) {
            func_801D06D8(currentGold, cartTotal, projectedBalance, 0, 0);
            redrawTransactionSummary = FALSE;
        }

        switch(g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                if (cartTotal) {
                    GearShopMenuPlaySoundEffect(MENU_SOUND_CONFIRM);
                    g_Menu->pManager->unk5A = 0;
                    g_Menu->pManager->shouldRenderWindow[2] = 0;
                    g_Menu->pManager->shouldRenderWindow[3] = 0;
                    g_Menu->pManager->scrollHandleActive = 0;
                    continueProcessing = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
                    GearShopMenuInitializePointerCursors(0);
                    func_801D0C20(cartTotal, 0);
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
                    g_Menu->pManager->unk5A = 0;
                    g_Menu->pManager->shouldRenderWindow[2] = 0;
                    g_Menu->pManager->shouldRenderWindow[3] = 0;
                    g_Menu->pManager->scrollHandleActive = 0;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
                    GearShopMenuInitializePointerCursors(0U);
                    if(!GearShopMenuShowModal(0x92, 0xFF, 1)) {
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
                if (rowIndex >= 8) {
                    rowIndex = 7;
                    scrollOffset += 1;
                    if ((itemCount - 8) < scrollOffset) {
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
        0x96,
        g_GameState.gearPartsIds,
        g_GameState.gearPartsQty,
        ITEM_TYPE_GEAR_ACCESSORY, 1,
        g_GameState.gearPartsQty,
        0
    );
}

void GearShopMenuSellWeaponsMenu(void) {
    GearShopMenuSellMenu(
        0x64,
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
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    g_Menu->pManager->unkA = 0;
    func_801CCEBC(4, g_Menu->pManager->unkC);

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

    func_801D0EC8(unkBool);
    g_Menu->pManager->unkA = 1;
    g_Menu->pManager->unk4 = 1;
    g_Menu->pManager->unk3 = 1;
    func_801CCE90(4, g_Menu->unk6E0, &D_801D6A24, g_Menu->pManager->unkC);
}

void func_801D2950(u8 itemType) {
    RECT rect;

    u32* pWorkspace;
    u8 itemId;
    void* pNames;

    pWorkspace = HeapAlloc(0x3F6, 0);

    switch (itemType) {
        case 0:
            itemId = g_GameState.gears[g_gearShopCurrentGearId].armorId;
            pNames = g_Menu->pShop->pGearArmorNames;
            g_Menu->pShop->str4530.width = SystemRenderStringEntry(GetStringEntry(pNames, itemId), pWorkspace, 0x24, 0);
            break;
        case 1:
            itemId = g_GameState.gears[g_gearShopCurrentGearId].frameId;
            pNames = g_Menu->pShop->pGearFrameNames;
            g_Menu->pShop->str4530.width = SystemRenderStringEntry(GetStringEntry(pNames, itemId), pWorkspace, 0x24, 0);
            break;
        case 2:
            itemId = g_GameState.gears[g_gearShopCurrentGearId].engineId;
            pNames = g_Menu->pShop->pGearEngineNames;
            g_Menu->pShop->str4530.width = SystemRenderStringEntry(GetStringEntry(pNames, itemId), pWorkspace, 0x24, 0);
            break;
    }

    setRECT(&rect, 408, 180, 40, FONT_LETTER_HEIGHT);
    LoadImage((RECT* ) &rect, pWorkspace);
    func_801C5CA8(&g_Menu->pShop->str4530, 9, 128, 129);
    GearShopMenuSetVertices(g_Menu->pShop->str4530.vertices, 212, 142, g_Menu->pShop->str4530.width, FONT_LETTER_HEIGHT);
    DrawSync(0);
    g_Menu->pShop->str4530.renderContext = g_Menu->renderContext;
    g_Menu->pShop->unk46B6[0] = 1;
    HeapFree(pWorkspace);
}
// https://decomp.me/scratch/5HauM
INCLUDE_ASM("asm/gear_shop_menu/nonmatchings/main/render", func_801D2B74);

void func_801D3558(s32* pDelta, s8* pIsImprovement, u8 itemId, u8 itemType, u8 characterId) {
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

    itemSlots[1] = g_GameState.gears[gearId].unk4[0];
    itemSlots[2] = g_GameState.gears[gearId].unk4[1];
    itemSlots[3] = g_GameState.gears[gearId].unk4[2];
    itemSlots[4] = g_GameState.gears[gearId].unk4[3];

    itemSlots[5] = g_GameState.gears[gearId].accessorySlots[0];
    itemSlots[6] = g_GameState.gears[gearId].accessorySlots[1];
    itemSlots[7] = g_GameState.gears[gearId].accessorySlots[2];

    switch(itemType) {
        case ITEM_TYPE_GEAR_WEAPON:
            // This **may** be a check to see if the weapon belongs to Billy or not,
            // since Billy uses guns w/ ammo which needs special handling? This should be double checked though.
            if (itemId < 0x32) {
                g_GameState.gears[gearId].weaponId[0] = itemId;
            } else {
                pWeaponList = g_Menu->pDressingRoom->pGearWeapons;

                for(i = 0; i < 4; i++) {
                    if(g_Menu->pDressingRoom->pGearWeapons[g_GameState.gears[gearId].unk4[i]].unkF == pWeaponList[itemId].unkF) {
                        g_GameState.gears[gearId].unk4[i] = itemId;
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

                if (pCurAccesory->unk8 && pCurAccesory->unk8 == pAccessory->unk8) {
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

    func_801D6150(g_Menu->pDressingRoom, g_GameState.characters[characterId].gearId);
    func_801D5F94(g_Menu->pDressingRoom, g_GameState.characters[characterId].gearId);

    oldValue[0] = g_Menu->pDressingRoom->unkB0;
    oldValue[1] = g_Menu->pDressingRoom->totalDefense;
    newValue[0] = g_Menu->pShop->unk46E0[characterId];
    newValue[1] = g_Menu->pShop->unk4700[characterId];

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
    g_GameState.gears[gearId].unk4[0] = itemSlots[1];
    g_GameState.gears[gearId].unk4[1] = itemSlots[2];
    g_GameState.gears[gearId].unk4[2] = itemSlots[3];
    g_GameState.gears[gearId].unk4[3] = itemSlots[4];
    g_GameState.gears[gearId].accessorySlots[0] = itemSlots[5];
    g_GameState.gears[gearId].accessorySlots[1] = itemSlots[6];
    g_GameState.gears[gearId].accessorySlots[2] = itemSlots[7];

    func_801D6150(g_Menu->pDressingRoom, g_GameState.characters[characterId].gearId);
}

int func_801D3A3C(u8* index, u8* haystack, s32 size, u8 needle) {
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

void func_801D3A80(u8 itemType, u8 itemId) {
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
        g_gearShopCurrentItemQuantityHeld = qtyHeld = func_801D3A3C(pIdList, pQtyList, listSize, itemId);

        pWorkspace = HeapAlloc(0x3F6U, 0U);
        digitBuffer[1] = 0;
        digitBuffer[3] = 0;
        digitBuffer[0] = ((qtyHeld / 10) == 0) ? 0xC3 : (qtyHeld / 10) + 0x10;
        digitBuffer[2] = (qtyHeld % 10) + 0x10;

        func_80033B34((u16 *)digitBuffer, stringBuffer, 2);
        g_Menu->pShop->str45B0.width = SystemRenderStringEntry(stringBuffer, pWorkspace, 0x24, 1);
        setRECT(&rect, 0x198, 0xB4, 0x28, FONT_LETTER_HEIGHT);
        LoadImage(&rect, pWorkspace);
        DrawSync(0);

        func_801C5CA8(&g_Menu->pShop->str45B0, 9, 0x80, 0x82);
        GearShopMenuSetVertices(g_Menu->pShop->str45B0.vertices, 0xF8, 0x8E, g_Menu->pShop->str45B0.width, FONT_LETTER_HEIGHT);
        g_Menu->pShop->str45B0.renderContext = g_Menu->renderContext;
        g_Menu->pShop->unk4785 = 1;
        HeapFree(pWorkspace);
    }
}

s32 func_801D3C78(s32 rowIndex, s32 scrollOffset, u8 *unused) {
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

    pStringBuffer = HeapAlloc(0x618, 0);
    bzero(pStringBuffer, 0x618);

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

    totalPrice = func_801D1078(itemId, itemType);
    setRECT(&rect, 0x140, 0x4E, 0x3C, 0xD);
    LoadImage(&rect, (u32 *)pStringBuffer);
    DrawSync(0);
    func_801C5CA8(&g_Menu->pShop->strItemDesc, 0, 0, 0U);
    func_801C51B8(&g_Menu->pShop->strItemDesc.polys[g_Menu->renderContext], 0x2C, 0x12, 0, 0x4E, (s32) g_Menu->pShop->strItemDesc.width, 0xD);
    GearShopMenuSetVertices(g_Menu->pShop->strItemDesc.vertices, 0x2C, 0x12, g_Menu->pShop->strItemDesc.width, 0xD);

    g_Menu->pShop->strItemDesc.renderContext = g_Menu->renderContext;
    HeapFree(pStringBuffer);
    if(itemId) {
        g_Menu->pShop->unk46A7 = TRUE;
    } else {
        g_Menu->pShop->unk46A7 = FALSE                                ;
    }

    g_Menu->pShop->unk46A9 = 0;

    xPos = 0x49;
    for(i = 0, currentSlot = 0; i < 0x10; i++) {
        if(g_Menu->availableCharacters[i] != 0) {
            gearId = g_GameState.characters[i].gearId;
            if(GearShopMenuGearCanEquip(equipFlags, gearId)) {
                g_Menu->pShop->unk469C[currentSlot] = TRUE;
            } else {
                g_Menu->pShop->unk469C[currentSlot] = FALSE;
            }

            if(GearShopMenuGearCanEquip(totalPrice, gearId)) {
                g_Menu->pShop->unk46A9 += func_8002675C(
                    g_Menu->resources,
                    0xE,
                    &g_Menu->pShop->polys2D0[g_Menu->pShop->unk46A9*2],
                    g_Menu->renderContext,
                    D_801D6C44[currentSlot] + 0xE,
                    0xB4,
                    0x1000
                );
            }
            g_Menu->pShop->unk46BC[currentSlot] = 0;
            g_Menu->pShop->unk46C5[currentSlot] = 0;
            if( (u8)(itemType + 0xFD) < 2 && g_Menu->pShop->unk469C[currentSlot]) {
                sp28[1] = 0;
                sp28[0] = 0;
                func_801D3558(&sp28[0], &sp30[0], itemId, itemType, (u8)i);
                if(sp28[0]) {
                    GearShopMenuParseNumberToString(sp28[0]);
                    for(j = 0; j < 3; j++) {
                        if(g_Menu->digits[6+j] != 0xff) {
                            g_Menu->pShop->unk46BC[currentSlot] += func_8002675C(
                                g_Menu->resources,
                                g_Menu->digits[6+j],
                                &g_Menu->pShop->polys27B0[currentSlot][g_Menu->pShop->unk46BC[currentSlot]*2],
                                g_Menu->renderContext,
                                (currentSlot * FONT_LETTER_HEIGHT * 2) + xPos + (8*j),
                                0xBE,
                                0x1000U);
                        }
                    }
                    func_801D0054(g_Menu->pShop->unk46BC[currentSlot], &g_Menu->pShop->polys27B0[currentSlot], sp30[0]);
                    g_Menu->pShop->unk46CE[currentSlot] = g_Menu->renderContext;
                }

                if(sp28[1]) {
                    GearShopMenuParseNumberToString(sp28[1]);
                    for(j = 0; j < 3; j++) {
                        if(g_Menu->digits[6+j] != 0xff) {
                            g_Menu->pShop->unk46C5[currentSlot] += func_8002675C(
                                g_Menu->resources,
                                g_Menu->digits[6+j],
                                &g_Menu->pShop->polys3020[currentSlot][g_Menu->pShop->unk46C5[currentSlot]*2],
                                g_Menu->renderContext,
                                (currentSlot * FONT_LETTER_HEIGHT * 2) + xPos + (8*j),
                                0xC6,
                                0x1000U);
                        }
                    }
                    func_801D0054(g_Menu->pShop->unk46C5[currentSlot], &g_Menu->pShop->polys3020[currentSlot], sp30[1]);
                    g_Menu->pShop->unk46D7[currentSlot] = g_Menu->renderContext;
                }
            }
            currentSlot++;
        }

    }
    g_Menu->pShop->unk46A8 = g_Menu->renderContext;
    func_801D3A80(itemType, itemId);
    return price;
}

void func_801D44FC(u32 newBalance) {
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


u8 func_801D4888(s32 itemId) {
    u8 equipped;
    u8 itemType = g_Menu->shopItemTypes[itemId];
    u8 result = TRUE;

    switch(itemType) {
        case 0:
            equipped = g_GameState.gears[g_gearShopCurrentGearId].armorId;
            if(equipped >= g_Menu->shopItemIDs[itemId]) {
                result = FALSE;
            }
            break;
        case 1:
            equipped = g_GameState.gears[g_gearShopCurrentGearId].frameId;
            if(equipped >= g_Menu->shopItemIDs[itemId]) {
                result = FALSE;
            }
            break;
        case 2:
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
    purchaseConfirmStringId2 = 0xFF;
    unkWindowX = 0xE0;
    projectedRemainingFunds = current_gold;
    newProjectedRemainingFunds = projectedRemainingFunds;
    salesTotal = 0;
    unkWindowWidth = 0x40;
    for(i = 0; i < MAX_GAME_CHARACTERS; i++) {
        if(g_GameState.characters[i].gearId != GEAR_ID_NONE) {
            func_801D6150(g_Menu->pDressingRoom, g_GameState.characters[i].gearId);
            func_801D5F94(g_Menu->pDressingRoom, g_GameState.characters[i].gearId);
            g_Menu->pShop->unk46E0[i] = g_Menu->pDressingRoom->unkB0;
            g_Menu->pShop->unk4700[i] = g_Menu->pDressingRoom->totalDefense;
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
    g_Menu->pSelectionMenu->unk1192 = 1;
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
            itemPrice = func_801D3C78(rowIndex, scrollOffset, values);
            newScrollOffset = scrollOffset;
            g_Menu->pManager->unk5A = 1;
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
                temp = D_801D6A2C;
                func_801CCE90(2, g_Menu->unk6E0, (s8* ) temp, (s8* ) g_Menu->pManager->unkC);
                func_801CCEE8(2, g_Menu->unk6E0, (s32) temp, D_801D6A40, g_Menu->pManager->unkC, disableQuantitySelection, 0, 1);
                if (disableQuantitySelection) {
                    func_801D2950(g_Menu->subMenuChoice + (arg0 * 3));
                }
                GearShopMenuUpdateCharacterPortraits();
                func_801D04E8(disableQuantitySelection);
                redrawWindows = FALSE;
            }

        if (redrawTransactionSummary) {
            func_801D06D8(current_gold, cartTotal, projectedRemainingFunds, salesTotal, disableQuantitySelection);
            redrawTransactionSummary = FALSE;
        }
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:

                if (cartTotal != 0) {
                    GearShopMenuPlaySoundEffect(MENU_SOUND_CONFIRM);
                    g_Menu->pManager->unk5A = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                    continueProcessing = FALSE;
                    g_Menu->pManager->unkC[disableQuantitySelection] = FALSE;
                    if (disableQuantitySelection) {
                        if (func_801D4888(scrollOffset + rowIndex) & 0xFF) {
                            purchaseConfirmStringId1 = MENU_STRING_CONFIRM_PURCHASE;
                        } else {
                            purchaseConfirmStringId1 = MENU_STRING_NOT_AN_UPGRADE;
                            purchaseConfirmStringId2 = MENU_STRING_BUY_ANYWAY;
                        }
                    } else {
                        func_801D0C20(cartTotal, 0);
                    }
                    GearShopMenuInitializePointerCursors(0);
                    if (GearShopMenuShowModal(purchaseConfirmStringId1, purchaseConfirmStringId2, TRUE) != MENU_CHOICE_NO) {
                        if (disableQuantitySelection) {
                            g_Menu->pShop->curItemQuantities[scrollOffset + rowIndex] = 1;
                        }
                        func_801D44FC(projectedRemainingFunds);
                    } else {
                        continueProcessing = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[5] = TRUE;
                        g_Menu->pManager->unkC[disableQuantitySelection] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        newScrollOffset = 0xFF;
                        newRowIndex = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                    }
                    GearShopMenuFreePointerCursors();
                    purchaseConfirmStringId2 = 0xFF;
                } else {
                    GearShopMenuPlaySoundEffect(MENU_SOUND_ERROR);
                }
                break;
            case MENU_INPUT_BACK:
                g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                continueProcessing = FALSE;
                g_Menu->pManager->unkC[disableQuantitySelection] = 0;
                if(cartTotal && !disableQuantitySelection) {
                    g_Menu->pManager->unk5A = 0;
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
                        g_Menu->pManager->unkC[0] = 1;
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

                if (rowIndex >= 8) {
                    rowIndex = 7;
                    scrollOffset += 1;
                    if ((numItems - 8) < scrollOffset) {
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
    g_Menu->pShop->unk4785 = 0;
    GearShopMenuFreeWindow(5);
    func_801CCEBC(2, (s8* ) g_Menu->pManager->unkC);
    func_801D0EC8(1);
    return 1;
}

// Handles the logic for the "Fuel" option under "Tune up"
void func_801D5398(void) {
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

    fuelUnits = (g_GameState.gears[g_gearShopCurrentGearId].maxFuel - g_GameState.gears[g_gearShopCurrentGearId].fuel) / 100;
    if(fuelUnits == 0) {
        fuelUnits = 1;
    }

    var_s0 = fuelUnits * 10;

    GearShopMenuInitializeWindow(5, 0xA2, 0xA6, 0x60, 0x14, 0, 1, 4, 0);
    func_801D0D4C(g_GameState.gold);
    if (var_s1 != 0) {
        func_801D0C20(var_s0, 1);
    }
    GearShopMenuUpdateAndRender();
    GearShopMenuInitializePointerCursors(0);
    if (GearShopMenuShowModal(var_s2, 0xFF, var_s6) != 0) {
        g_Menu->pManager->unk5B = 0;
        switch (var_s1) {
            case 1:
                if (g_GameState.gold < var_s0) {
                    if (GearShopMenuShowModal(0xA9U, 0xFF, 1) != 0) {
                        if (g_GameState.gold != 0) {
                            var_s4 = TRUE;
                        }

                        var_s0 = g_GameState.gold / 10;
                        g_GameState.gears[g_gearShopCurrentGearId].fuel += ((var_s0) * 100);
                        g_GameState.gold %= 10;
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
    g_Menu->pManager->unk5C[8] = 0;
    GearShopMenuFreeWindow(5);
}


u_char func_801D573C(void) {
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    func_801CCEBC(4, g_Menu->pManager->unkC);
    func_801D0EC8(0);
    return 2;
}

// Called if we chose "Tune up", dispatches into the correct menu based on
// the submenu choice
void func_801D57A8(void) {
    switch(g_Menu->subMenuChoice) {
        case MENU_CHOICE_ARMOR:
        case MENU_CHOICE_FRAME:
        case MENU_CHOICE_ENGINE:
            GearShopMenuPurchaseDialog(0, 1);
            func_801D6150(g_Menu->pDressingRoom, g_gearShopCurrentGearId);
            break;
        case MENU_CHOICE_FUEL:
            func_801D5398();
    }
}

s32 func_801D5828(void) {
    u8 continueProcessing = TRUE;
    u8 redrawWindows = TRUE;
    u8 var_s0 = 0;

    if ((g_Menu->mainMenuChoice == 3) && !func_801D573C()) return 1;

    g_Menu->subMenuChoice = 0;
    g_Menu->subMenuPrevChoice = 0xFF;
    if (g_Menu->mainMenuChoice == 3) {
        g_Menu->subMenuChoice = 3;
        var_s0 = 4;
    }

    while(continueProcessing) {
            GearShopMenuUpdateAndRender();
            if (g_Menu->transitionEffectState == 0) {
                if (g_gearShopAvailableCharacterCount >= 2) {
                    g_Menu->pManager->shoulderButtonUiActive = 1;
                }
            }
            if (redrawWindows) {
                func_801CCE90(4, g_Menu->unk6E0, &D_801D6A24[var_s0], (s8* ) g_Menu->pManager->unkC);
                GearShopMenuInitializeSubmenu(0);
                redrawWindows = FALSE;
            }
            if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
                func_801CCEE8(4, g_Menu->unk6E0, (s32) &D_801D6A24[var_s0], &D_801D6A40[var_s0], g_Menu->pManager->unkC, g_Menu->subMenuChoice, 4, 0);
                GearShopMenuUpdateSubmenuTextures(0);
                g_Menu->subMenuPrevChoice = g_Menu->subMenuChoice;
            }
            if (g_Menu->menuUnk8->unk1ED9[0] == 0) {
                g_Menu->pManager->unk5C[0xA] = 1;
                g_Menu->pManager->unk5C[9] = 0;
            }

            switch (g_Menu->input) {
                case MENU_INPUT_CONFIRM:
                    g_Menu->pManager->unk4 = 0;
                    g_Menu->pManager->unk3 = 0;
                    g_Menu->pManager->unkA = 0;
                    g_Menu->pManager->unk5C[0xA] = 0;
                    func_801CCEBC(4, (s8* ) g_Menu->pManager->unkC);
                    GearShopMenuPlaySoundEffect(2);
                    func_801CFC60();
                    g_Menu->pManager->shoulderButtonUiActive = 0;
                    switch (g_Menu->mainMenuChoice) {              /* switch 2; irregular */
                        case 1:
                            GearShopMenuSellModeMenuHandleSelectedOption(1);
                            break;
                        case 2:
                            GearShopMenuPurchaseDialog(1, 0);
                            break;
                        case 3:
                            func_801D57A8();
                            break;
                    }
                    func_801CFF18();
                    GearShopMenuUpdateAndRender();
                    func_801CF448();
                    GearShopMenuUpdateAndRender();
                    g_Menu->pManager->unk4 = 1;
                    g_Menu->pManager->unk3 = 1;
                    g_Menu->pManager->unkA = 1;
                    func_801CCE90(4, g_Menu->unk6E0, &D_801D6A24[var_s0], (s8* ) g_Menu->pManager->unkC);
                    g_Menu->subMenuPrevChoice = 0xFF;
                case MENU_INPUT_LEFT:                             /* switch 1 */
                case 6:                             /* switch 1 */
                case 7:                             /* switch 1 */
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
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    func_801CCEBC(4, (s8* ) g_Menu->pManager->unkC);

    return 1;
}

void func_801D5D38(void) {
    s32 firstAvailable = 0;

    LibGearInitialize(0x40);

    g_Menu->unk298 = 0x546;
    g_Menu->unk29A = -0xE39;
    g_Menu->unk29C = 0x546;
    g_Menu->unk29E = 0;
    g_Menu->unk2A0 = 0;
    g_Menu->unk2A2 = 0;
    g_Menu->unk2A4 = 0;
    g_Menu->unk2A6 = 0;
    g_Menu->unk2A8 = 0;

    g_Menu->unk2B8 = 0x600;
    g_Menu->unk2BA = 0;
    g_Menu->unk2BC = 0;
    g_Menu->unk2BE = 0x600;
    g_Menu->unk2C0 = 0;
    g_Menu->unk2C2 = 0;
    g_Menu->unk2C4 = 0x600;
    g_Menu->unk2C6 = 0;
    g_Menu->unk2C8 = 0;

    D_801E8644 = &g_Menu->unk2B8;
    SetBackColor(0x3C, 0x3C, 0x3C);

    g_Menu->gfxEnvs[0].unkB0 = D_8005A4AC[0];
    g_Menu->gfxEnvs[1].unkB0 = D_8005A4AC[1];

    g_Menu->unk458[0] = HeapAlloc(sizeof(SystemMenuArchiveData), 0);
    bzero(g_Menu->unk458[0], sizeof(SystemMenuArchiveData));

    g_Menu->unk458[1] = HeapAlloc(sizeof(SystemMenuArchiveData), 0);
    bzero(g_Menu->unk458[1], sizeof(SystemMenuArchiveData));

    while(g_Menu->availableCharacters[firstAvailable] == 0) {
        firstAvailable++;
    }

    g_gearShopCurrentGearId = g_GameState.characters[firstAvailable].gearId;

    func_801CF9BC(g_gearShopCurrentGearId, 1);
    GearShopMenuSetAvailableCharacterCount();
}

void func_801D5EB8(void) {
    while (g_Menu->transitionEffectState != 0) {
        GearShopMenuUpdateAndRender();
    }

    D_801D697C = 0;
    g_Menu->pManager->unk5C[7] = 0;
    GearShopMenuUpdateAndRender();
    func_801E7FD4();
    g_Menu->unk458[0]->unk12 = 0;
    g_Menu->unk458[1]->unk12 = 0;
    HeapFree(g_Menu->unk458[0]);
    HeapFree(g_Menu->unk458[1]);
}

void func_801D5F94(MenuDressingRoom* pView, u_char gearId) {
    GameGear* pGear;
    GameCharacter* pPilot;
    u_short ether;
    int etherModifier;

    if(g_GameState.unk22B6 & 0x1000) {
        D_801D70FD = 0xA;
    }

    pGear = &g_GameState.gears[gearId];
    pPilot = &g_GameState.characters[D_801D70F4[gearId]];

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
        etherModifier = ((pGear->weaponData[0].unk2 + pGear->weaponData[2].unk2) * 6) / 10;
        pView->unkB0 = etherModifier + ether;
    } else {
        pView->unkB0 = pGear->weaponData[0].unk2 + ether;
    }

    pView->totalResponsiveness = pGear->unk9F + pGear->responsiveness;
    pView->unkB3 = pGear->agility - pGear->unk4A;
    pView->unkB4 = pGear->etherAmplification;
    pView->unkB5 = pGear->unk9D;
    pView->unkB6 = pGear->unk9C;
}

void func_801D6150(MenuDressingRoom* arg0, u8 arg1) {
    func_801D61B8(arg0, arg1);
    func_801D62A4(arg0, arg1);
    func_801D6250(arg0, arg1);
    func_801D6334(arg0, arg1);
    func_801D6738(arg0, arg1);
}

void func_801D61B8(MenuDressingRoom* arg0, u_char gearIndex) {
    GameGear* pGear;
    GearFrame* pPreview;
    u_char previewId;

    pGear = &g_GameState.gears[gearIndex];
    pPreview = arg0->pGearFrames;
    previewId = pGear->frameId;
    pPreview = &pPreview[previewId];

    pGear->maxHp = pPreview->hp;

    pGear->weight = pPreview->weight;
    pGear->agility = pPreview->unk14;
    pGear->etherAmplification = pPreview->unk15;
    pGear->unk9D = pPreview->unk16;
    pGear->unk9F = pPreview->unk17;
    if (pGear->maxHp < pGear->hp) {
        pGear->hp = pGear->maxHp;
    }
}

void func_801D6250(MenuDressingRoom* menu, u_char gearIndex) {
    GameGear* pGear;
    GearArmor* itemList;
    GearArmor* pItem;

    pGear = &g_GameState.gears[gearIndex];
    itemList = menu->pGearArmor
;
    pItem = &itemList[pGear->armorId];

    pGear->baseDefense = pItem->unk8;
    pGear->baseEtherDefense = pItem->unkA;
}

void func_801D62A4(MenuDressingRoom* shopInfo, u8 gearId) {
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


void func_801D6334(MenuDressingRoom* dressingRoom, u8 gearId) {
    GameGear* pGear;
    GearAccessory* pAccessory;
    u8 accessoryInd;
    u8 i;
    u8 characterId;
    u16 *pFlags1, *pFlags2;

    pGear = &g_GameState.gears[gearId];
    characterId = D_801D70F4[gearId];

    pFlags1 = &g_GameState.unk16BC[characterId].unk8;
    pFlags2 = &g_GameState.unk16BC[characterId].unk1E;

    pGear->defenseFromEquipment = 0;
    pGear->etherDefenseFromEquipment = 0;
    pGear->weightFromEquipment = 0;
    pGear->unk48 = 0;
    pGear->unk4C = 0;
    pGear->responsiveness = 0;
    pGear->unk4E = 0;
    pGear->unk4F = 0;
    pGear->unk6E = 0;


    for(i = 0; i < 16; i++) {
        pGear->unk88[i] = 0;
    }

    for(i = 0; i < 4; i++) {
        pGear->unk50[i] = 0;
    }

    for(i = 0; i < 3; i++) {
        pGear->unk55[i] = 0;
    }

    pGear->unk54 = 0;
    pGear->unk7E = 0;
    pGear->unk82 = 0;
    pGear->unk86 &= 0xF000;
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

        switch(pAccessory->unk15) {
            case 1:
                pGear->unk7E |= pAccessory->unk16;
                break;
            case 2:
                pGear->unk82 |= pAccessory->unk16;
                break;
            case 3:
                pGear->unk86 |= pAccessory->unk16;
                break;
            case 4:
                for(i = 0; i < 16; i++) {
                    pGear->unk6E |= pAccessory->unk16;
                    if(pAccessory->unk16 & (0x8000 >> i)) {
                        pGear->unk88[i] += pAccessory->unk1A;
                    }
                }
                break;
            case 5:
                pGear->unk4F += pAccessory->unk16;
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
                pGear->unk48 |= pAccessory->unk16;
            case 10:
                pGear->unk55[1] += (u8) pAccessory->unk16;
                break;
            case 11:
                pGear->unk55[2] += (u8) pAccessory->unk16;
                break;
        }
    }

    pGear->unk4A = func_801D690C(gearId);
    if (pGear->unk4F) {
        *pFlags2 |= 0x8000;
    }
    else if (gearId == g_GameState.characters[D_801D70F4[gearId]].gearId) {
        *pFlags2 &= 0x7FFF;
    }
}

void func_801D6738(MenuDressingRoom* dressingRoom, u8 gearId) {
    GameGear* pGear = &g_GameState.gears[gearId];
    GearWeapon *pList = dressingRoom->pGearWeapons;
    GearWeapon *pWeapon;

    pWeapon = dressingRoom->pGearWeapons;
    pWeapon += pGear->weaponId[0];
    pGear->weaponData[0].unk2 = pWeapon->unkE;
    pGear->weaponData[0].unk0 = pWeapon->unk12;
    pGear->weaponData[0].unk3 = pWeapon->unk10;
    pGear->weaponData [0].unk4 = pWeapon->unk11;

    pGear->unk5C[0] = pWeapon->unk0[0];
    pGear->unk5C[1] = pWeapon->unk0[1];
    pGear->unk5C[2] = pWeapon->unk0[2];
    pGear->unk5C[3] = pWeapon->unk0[3];

    if (pGear->weaponData[0].unk4 == 100) {
        pGear->unk86 = pGear->unk86 & 0xFFF;
        pGear->unk86 |= pGear->weaponData[0].unk0;
    }
    if ((gearId == 5) || (gearId == 0xD)) {
        pWeapon = dressingRoom->pGearWeapons;
        pWeapon += pGear->unk4[0];

        pGear->weaponData[0].unk2 = pWeapon->unkE;
        pGear->weaponData[0].unk0 = pWeapon->unk12;
        pGear->weaponData[0].unk3 = pWeapon->unk10;
        pGear->weaponData[0].unk4 = pWeapon->unk11;
        pGear->unk5C[1] = pWeapon->unk0[1];

        pWeapon = dressingRoom->pGearWeapons;
        pWeapon += pGear->unk4[1];
        pGear->weaponData[1].unk2 = pWeapon->unkE;
        pGear->weaponData[1].unk0 = pWeapon->unk12;
        pGear->weaponData[1].unk3 = pWeapon->unk10;
        pGear->weaponData[1].unk4 = pWeapon->unk11;
        pGear->unk5C[2] = pWeapon->unk0[2];

        pWeapon = dressingRoom->pGearWeapons;
        pWeapon += pGear->unk4[3];
        pGear->weaponData[2].unk2 = pWeapon->unkE;
        pGear->weaponData[2].unk0 = pWeapon->unk12;
        pGear->weaponData[2].unk3 = pWeapon->unk10;
        pGear->weaponData[2].unk4 = pWeapon->unk11;
        pGear->unk5C[3] = pWeapon->unk0[3];
    }
}

s32 func_801D690C(u8 index) {
    GameGear* pGear;
    s32 temp_v0;
    u8 result;

    pGear = &g_GameState.gears[index];
    temp_v0 = (s32) ((pGear->weightFromEquipment / 120) - pGear->unk75) / 2;
    result = temp_v0;
    if (temp_v0 << 16 < 0) {
        result = 0;
    }
    return result;
}
