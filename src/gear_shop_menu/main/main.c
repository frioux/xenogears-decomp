#include "common.h"
#include "system/menu.h"
#include "system/controller.h"
#include "system/archive.h"
#include "system/graphics.h"
#include "system/debug.h"
#include "system/sound.h"
#include "libgear/main.h"

#include "gear_shop/gear_shop.h"



/*
 * TODO: probably should be in a header for main program stuff since this is not coming from the overlay
 */

extern u8   D_80059171; // probably something like: DEBUGGER_ATTACHED, guards a breakpoint left in the code
extern s32* D_8005917C;
extern u32* D_8005945C; // file handle used to retrieve the overlay's resources
extern SoundFile* D_8006259C;

/*
 * this function doesn't seem to get called by anything, possibly dead code?
 */
u16 GearShopMenuGetRandomInRange(u16 arg0, u16 arg1) {
    s32 delta;
    s32 result;

    if(arg0 != 0xffff) {
        result = 0;
        if(arg1 != 0) {
            if(arg0 == arg1) {
                return arg0;
            }
            delta = arg1 - arg0;
            if (delta <= 0xFFFE) {
                return arg0 + ((u16)rand() % (delta+1));
            }
            result = (u16)rand();
            return result;
        }
    } else {
        result = 0xffff;
    }

    return result;
}

void GearShopMenuSetPolyFT4Quad(POLY_FT4* pPoly, short x, short y, u_char u, u_char v, short width, short height) {
    setXY4(
        pPoly,
        x,         y,
        x + width, y,
        x,         y + height,
        x + width, y + height
    );

    setUV4(
        pPoly,
        u,         v,
        u + width, v,
        u,         v + height,
        u + width, v + height
    );
}

u_short GearShopMenuIsCharacterFlagSet(u_short value, u_char maskIndex) {
    return D_801D6C68[maskIndex] & value;
}

u_short GearShopMenuGetCharacterBitMask(int index) {
    return D_801D6C68[index & 0xFF];
}

s32 GearShopMenuGetGearEquipFlags(u8 count) {
    return g_gearShopGearEquipFlags[count];
}

s32 GearShopMenuGearCanEquip(s32 equipFlags, u8 gearId) {
    return equipFlags & g_gearShopGearEquipFlags[gearId];
}

void GearShopMenuParseNumberToString(u_int amount) {
    int i;
    u_int place = 100000000;

    for(i = 0; i < MENU_MAX_DIGITS; i++) {
        g_Menu->digits[i] = amount / place;
        amount = amount % place;
        place /= 10;
    }

    for(i = 1; i < MENU_MAX_DIGITS; i++) {
        if(g_Menu->digits[i] == 0) {
            g_Menu->digits[i-1] = -1;
        } else {
            if(g_Menu->digits[i-1] == 0) {
                g_Menu->digits[i-1] = -1;
            }
            break;
        }
    }
}

/**
 * Manages the memory allocation for MenuUnk2 in the system menu (g_Menu).
 *
 * @param isInitialization determines action taken; valid values are either MENU_DATA_INITIALIZE or MENU_DATA_FREE
 */
void GearShopMenuMenuUnk2Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->menuUnk2 = HeapAlloc(sizeof(MenuUnk2), 0);
        bzero(g_Menu->menuUnk2, sizeof(MenuUnk2));
        return;
    }
    HeapFree(g_Menu->menuUnk2);
}

void GearShopMenuMenuManagerManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pManager = HeapAlloc(sizeof(MenuManager), 0);
        bzero(g_Menu->pManager, sizeof(MenuManager));
        return;
    }
    HeapFree(g_Menu->pManager);
}

void GearShopMenuSelectionMenuManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pSelectionMenu = HeapAlloc(sizeof(MenuSelectionMenu), 0);
        bzero(g_Menu->pSelectionMenu, sizeof(MenuSelectionMenu));
        return;
    }
    HeapFree(g_Menu->pSelectionMenu);
}

void GearShopMenuMenuUnk5Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk354 = HeapAlloc(sizeof(MenuUnk5), 0);
        bzero(g_Menu->unk354, sizeof(MenuUnk5));
        return;
    }
    HeapFree(g_Menu->unk354);
}

void GearShopMenuDressingRoomManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pDressingRoom = HeapAlloc(sizeof(MenuDressingRoom), 0);
        bzero(g_Menu->pDressingRoom, sizeof(MenuDressingRoom));
        return;
    }
    HeapFree(g_Menu->pDressingRoom);
}

void GearShopMenuMenuUnk1Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk348 = HeapAlloc(sizeof(MenuUnk1), 0);
        bzero(g_Menu->unk348, sizeof(MenuUnk1));
        return;
    }
    HeapFree(g_Menu->unk348);
}

void GearShopMenuMenuUnk7Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk1E20 = HeapAlloc(sizeof(MenuUnk7), 0);
        bzero(g_Menu->unk1E20, sizeof(MenuUnk7));
        return;
    }
    HeapFree(g_Menu->unk1E20);
}

void GearShopMenuMenuShopManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pShop = HeapAlloc(sizeof(MenuShop), 0);
        bzero(g_Menu->pShop, sizeof(MenuShop));
        return;
    }
    HeapFree(g_Menu->pShop);
}

void GearShopMenuMenuUnk8Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->menuUnk8 = HeapAlloc(sizeof(MenuUnk8), 0);
        bzero(g_Menu->menuUnk8, sizeof(MenuUnk8));
        return;
    }
    HeapFree(g_Menu->menuUnk8);
}

#define FIELD_TEX_COUNT(_i) ((_i) * 6) + 0
#define FIELD_TEX_PAGE(_i)  ((_i) * 6) + 1
#define FIELD_CLUT_X(_i)    ((_i) * 6) + 2
#define FIELD_CLUT_Y(_i)    ((_i) * 6) + 3
#define FIELD_TPAGE_X(_i)   ((_i) * 6) + 4
#define FIELD_TPAGE_Y(_i)   ((_i) * 6) + 5

void GearShopMenuLoadResources(void) {
    TIM_IMAGE tim;
    int metadata[18];
    SoundFile* pSoundFile;

    u32* pFileHandle;
    int i;
    u8 *pData;

    pFileHandle = D_8005945C;
    ResolveArchiveEntryPointers(pFileHandle);
    pData = LZSSHeapDecompress((void *)pFileHandle[1], 1);
    OpenTIM((void *)pData);
    ReadTIM(&g_Menu->menuUnk2->tim);
    memcpy(&g_Menu->menuUnk2->unk4FCE, "BISLPS-00800", 0xD);
    g_Menu->menuUnk2->unk4B94 = 0x53;
    g_Menu->menuUnk2->unk4B95 = 0x43;
    g_Menu->menuUnk2->unk4B96 = 0x11;
    g_Menu->menuUnk2->unk4B97 = 1;
    bzero(g_Menu->menuUnk2->unk4B98, 0x5C);

    memmove(g_Menu->menuUnk2->unk4BF4, (u8* ) g_Menu->menuUnk2->tim.caddr, 0x20);
    memmove(g_Menu->menuUnk2->unk4C14, (u8* ) g_Menu->menuUnk2->tim.paddr, 0x80);
    HeapFree(pData);
    pData = LZSSHeapDecompress((void *)pFileHandle[2], 1);
    func_8002DD20(pData);
    HeapFree(pData);
    g_Menu->resources = LZSSHeapDecompress((void *)pFileHandle[3], 0);
    g_Menu->unk2E0 = LZSSHeapDecompress((void *)pFileHandle[4], 0);

    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_PORTRAIT_PARTY_MEMBER_1,
                               &metadata[FIELD_TEX_COUNT(0)],
                               &metadata[FIELD_TEX_PAGE(0)],
                               &metadata[FIELD_CLUT_X(0)],
                               &metadata[FIELD_CLUT_Y(0)],
                               &metadata[FIELD_TPAGE_X(0)],
                               &metadata[FIELD_TPAGE_Y(0)]);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_PORTRAIT_PARTY_MEMBER_2,
                               &metadata[FIELD_TEX_COUNT(1)],
                               &metadata[FIELD_TEX_PAGE(1)],
                               &metadata[FIELD_CLUT_X(1)],
                               &metadata[FIELD_CLUT_Y(1)],
                               &metadata[FIELD_TPAGE_X(1)],
                               &metadata[FIELD_TPAGE_Y(1)]);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_PORTRAIT_PARTY_MEMBER_3,
                               &metadata[FIELD_TEX_COUNT(2)],
                               &metadata[FIELD_TEX_PAGE(2)],
                               &metadata[FIELD_CLUT_X(2)],
                               &metadata[FIELD_CLUT_Y(2)],
                               &metadata[FIELD_TPAGE_X(2)],
                               &metadata[FIELD_TPAGE_Y(2)]);

    metadata[FIELD_TPAGE_X(1)] += 0xC;
    pData = LZSSHeapDecompress((void *)pFileHandle[5], 1);

    for(i = 0; i < 3; i++) {
        if(g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
            OpenTIM((void *)&pData[(g_Menu->pManager->currentCharacterIDs[i] * 0xB20)]);
            ReadTIM(&tim);
            tim.crect->x = metadata[FIELD_CLUT_X(i)];
            tim.crect->y = metadata[FIELD_CLUT_Y(i)];
            tim.prect->x = metadata[FIELD_TPAGE_X(i)];
            tim.prect->y = metadata[FIELD_TPAGE_Y(i)];
            LoadImage(tim.crect, tim.caddr);
            LoadImage(tim.prect, tim.paddr);
        }
    }

    DrawSync(0);
    HeapFree(pData);
    if (g_MenuDebugEnabled) {
        ArchiveSetIndex(0x10, 2);
        pSoundFile = HeapAlloc(ArchiveDecodeAlignedSize(5U), 0U);
        D_8006259C = pSoundFile;
        ArchiveReadFileToBuffer(5, (s32) pSoundFile, 0U, 0x80U);
        ArchiveCdDataSync(0);
        ArchiveSetIndex(0x10, 0);
        SoundAddSedsEntry(D_8006259C);
    }
    g_Menu->unk2E4 = D_8006259C;
    g_Menu->pGearShopEntries = LZSSHeapDecompress((void *)pFileHandle[8], 1);
    HeapFree(pFileHandle);
}


void GearShopMenuFilterPartyMembers(void) {
    int i;
    int flags;

    g_Menu->mainMenuChoice = 4;
    g_Menu->mainMenuPrevChoice = 0xFF;
    g_Menu->unk326 = 0x3C;
    g_Menu->unk334 = 0;
    g_Menu->unk335 = 0;

    flags = g_GameState.unk1D30 & g_GameState.FrMask & 0x77F;

    // Filter availableCharacters by if gearId != 0xff
    for(i = 0; i < 16; i++) {
        if( GearShopMenuIsCharacterFlagSet(flags, i) > 0 && g_GameState.characters[i].gearId != 0xFF ) {
            g_Menu->availableCharacters[i] = TRUE;
        } else {
            g_Menu->availableCharacters[i] = FALSE;
        }
    }

    // Filter by current party
    for(i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if( g_GameState.partyMembers[i] != 0xFF && g_Menu->availableCharacters[g_GameState.partyMembers[i]]) {
            g_Menu->pManager->currentCharacterIDs[i] = g_GameState.partyMembers[i];
        } else {
            g_Menu->pManager->currentCharacterIDs[i] = 0xFF;
        }
    }

    GearShopMenuLoadResources();
}


void GearShopMenuResetRenderContext(void) {
    g_Menu->renderContext = 0;
}

void GearShopMenuInitializeMenuStringPolys(MenuString* pMenuString, s32 arg1, s32 arg2, u8 attributes) {
    s32 tpage_offset;
    int i;
    POLY_FT4* quad;

    for (i = 0; i < 2; i++) {
        quad = &pMenuString->polys[i];
        tpage_offset = 0;

        SetPolyFT4(quad);
        SetSemiTrans(quad, 0);
        SetShadeTex(quad, 0);
        setRGB0(quad, 0x80, 0x80, 0x80);

        if (!attributes) {
            pMenuString->unk7C = (arg1 & 1);
            setTPage(quad, 0, 0, 0x140, 0);

#define texCoordV  (((arg1 + arg2) / 4) * 13)
#define texCoordU  (((arg1 / 2) & 1) << 7)

            setUV4(
                quad,
                texCoordU,               texCoordV,
                texCoordU + pMenuString->width, texCoordV,
                texCoordU,               texCoordV + 13,
                texCoordU + pMenuString->width, texCoordV + 13
            );
#undef texCoordV
#undef texCoordU
        } else {
            if (!(attributes & 0x80)) {
                tpage_offset = 0x20;
                SetSemiTrans(quad, 1);
                setRGB0(quad, tpage_offset, tpage_offset, tpage_offset);
            }

            pMenuString->unk7C = (attributes & 0x7F) + 0xFF;
            setTPage(quad, 0, 0, 0x180, 0x80);
            quad->tpage = tpage_offset | quad->tpage;

#define texCoordU ((arg1 & 1) * 0x60)
#define texCoordV (((arg1 / 2) * 13) + arg2)

            setUV4(
                quad,
                texCoordU,               texCoordV,
                texCoordU + pMenuString->width, texCoordV,
                texCoordU,               texCoordV + 13,
                texCoordU + pMenuString->width, texCoordV + 13
            );
#undef texCoordV
#undef texCoordU
        }
        pMenuString->polys[i].clut = pMenuString->unk7C ? g_SystemPalette2 : g_SystemPalette1;
    }

    pMenuString->unk7F = 0;
}

void func_801C5EE8(MenuString* pStringList, u8* pStringIdList, s32 yOffset, s32 count) {
    s32 yPos;
    s32 i;
    u32 offset;
    RECT *pRect;
    MenuString *pString;

    for (i = 0; i < count; i += 2) {
        pStringList[i].width = SystemRenderStringEntry(
            GetStringEntry(g_Menu->unk2E0, pStringIdList[i]),
            (u32*) g_Menu->unk4E0[0].pVramBuffer, 0x18, 0
        );
        pStringList[i + 1].width = SystemRenderStringEntry(
            GetStringEntry(g_Menu->unk2E0, pStringIdList[i + 1]),
            (u32*) g_Menu->unk4E0[0].pVramBuffer, 0x18, 1
        );

        pString = &pStringList[i];
        pRect = &pString->vramDest;
        setRECT(pRect,
                ((u32)(i / 2) % 2 * 32) + 0x140,
                ((i+yOffset) / 4) * FONT_LETTER_HEIGHT,
                28,
                FONT_LETTER_HEIGHT
        );
        memcpy(&pString[1].vramDest, pRect, sizeof(RECT));

        GearShopMenuInitializeMenuStringPolys(&pStringList[i], i, yOffset, 0);
        GearShopMenuInitializeMenuStringPolys(&pStringList[i + 1], i + 1, yOffset, 0);

        LoadImage(pRect, g_Menu->unk4E0[0].pVramBuffer);
        DrawSync(0);
    }
}

void GearShopMenuFillVramRectWhite(void) {
    u32* pImg;
    RECT rect;
    u8 _unused[2];

    pImg = HeapAlloc(0x20, 0x0);
    bzero(pImg, 0x20);
    ((u16 *)pImg)[1] = 0x7fff;

    rect.x = 0;
    rect.y = 0x1C0;
    rect.w = 0x10;
    rect.h = 1;

    LoadImage(&rect, pImg);
    DrawSync(0);
    HeapFree(pImg);
}

/* initMenuGeometry() ? */
void GearShopMenuInitializeMenuStrings(void) {
    SystemTransferPaletteToVRAM(0, 0x1D1);
    g_Menu->unk4E0[0].pVramBuffer = HeapAlloc(0x38E, 0);
    func_801C5EE8(g_Menu->unk4E0, &D_801D6A80, 0, 4);
    GearShopMenuFillVramRectWhite();
}

void GearShopMenuInitializeWindowBorders(void) {
    POLY_FT4 _unused;

    ResourceHelperGetTexCoords(
        g_Menu->resources, // resource table
        MENU_TEX_WINDOW_BORDER_TOP, // resource index
        &g_Menu->texCount0, // out parameter: length of list @ table[index]
        &g_Menu->texPage0,
        &g_Menu->clutX0,
        &g_Menu->clutY0,
        &g_Menu->texPageX0,
        &g_Menu->texPageY0);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_WINDOW_BORDER_BOTTOM, &g_Menu->texCount1, &g_Menu->texPage1, &g_Menu->clutX1, &g_Menu->clutY1, &g_Menu->texPageX1, &g_Menu->texPageY1);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_WINDOW_BORDER_LEFT, &g_Menu->texCount2, &g_Menu->texPage2, &g_Menu->clutX2, &g_Menu->clutY2, &g_Menu->texPageX2, &g_Menu->texPageY2);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_WINDOW_BORDER_RIGHT, &g_Menu->texCount3, &g_Menu->texPage3, &g_Menu->clutX3, &g_Menu->clutY3, &g_Menu->texPageX3, &g_Menu->texPageY3);
}

void GearShopMenuSetSelectionCursor(s32 arg0, u8 doInit) {
    func_8002675C(g_Menu->resources, 0x108, g_Menu->unk348->polysPointerCursor, g_Menu->renderContext, D_801D6BFC[arg0], D_801D6C20[arg0], 0x1000U);
    g_Menu->unk348->cursorRenderContext = (u8) g_Menu->renderContext;

    if (doInit) {
        setXY4(&g_Menu->unk348->polyG4s[g_Menu->renderContext],
               D_801D6BFC[arg0] + 0x14, D_801D6C20[arg0] - 0x24,
               D_801D6BFC[arg0] + (g_Menu->unk348->unk15B + 0x14), D_801D6C20[arg0] - 0x24,
               D_801D6BFC[arg0] + 0x14, D_801D6C20[arg0] - 0x14,
               D_801D6BFC[arg0] + (g_Menu->unk348->unk15B + 0x14), D_801D6C20[arg0] - 0x14
        );

        setXY3(&g_Menu->unk348->lines1[g_Menu->renderContext],
               D_801D6BFC[arg0] + 0x14, D_801D6C20[arg0] - 0x24,
               D_801D6BFC[arg0] + (g_Menu->unk348->unk15B + 0x14), D_801D6C20[arg0] - 0x24,
               D_801D6BFC[arg0] + (g_Menu->unk348->unk15B + 0x14), D_801D6C20[arg0] - 0x14
        );

        setXY3(&g_Menu->unk348->lines2[g_Menu->renderContext],
               D_801D6BFC[arg0] + 0x14, D_801D6C20[arg0] - 0x24,
               D_801D6BFC[arg0] + 0x14, D_801D6C20[arg0] - 0x14,
               D_801D6BFC[arg0] + (g_Menu->unk348->unk15B + 0x14), D_801D6C20[arg0] - 0x14
        );

        g_Menu->unk348->unk159 = (u8) g_Menu->renderContext;
        g_Menu->pManager->unk3 = 1;
    }
}

void GearShopMenuResetSelectionCursor(void) {
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
}

void GearShopMenuSetPolyGradientColor(POLY_G4* pPoly, u_char red, u_char green, u_char blue) {
    SetPolyG4(pPoly);
    setRGB0(pPoly, red, green, blue);
    setRGB1(pPoly, red, green, blue);
    setRGB2(pPoly, 0, 0, 0);
    setRGB3(pPoly, 0, 0, 0);
}

void GearShopMenuInitializeBackgrounds(void) {
    int i;
    RECT rect;

    rect.y = 0;
    rect.x = 0;
    rect.h = 0x100;
    rect.w = 0x100;
    
    GearShopMenuResetSelectionCursor();

    for(i = 0; i < 2; i++) {
        GearShopMenuSetPolyGradientColor(&g_Menu->unk348->polyG4s[i], 0x80, 0x80, 0);
        SetSemiTrans(&g_Menu->unk348->polyG4s[i], 1);
        
        SetLineF3(&g_Menu->unk348->lines1[i]);
        setRGB0(&g_Menu->unk348->lines1[i], 0, 64, 0);

        SetLineF3(&g_Menu->unk348->lines2[i]);
        setRGB0(&g_Menu->unk348->lines2[i], 0, 64, 0);

        SetPolyF4(&g_Menu->unk348->polysDimEffect[i]);
        setXY4(&g_Menu->unk348->polysDimEffect[i],
            0,   0,
            320, 0,
            0,   224,
            320, 224
        );
        setRGB0(&g_Menu->unk348->polysDimEffect[i], 128, 128, 128);

        SetSemiTrans(&g_Menu->unk348->polysDimEffect[i], 1);
        
        SetDrawMode(&g_Menu->unk348->drModes1[i], 0, 0, GetTPage(0, 0, 0x140, 0x80), &rect);
        SetDrawMode(&g_Menu->unk348->drawModeDimEffect[i], 0, 0, GetTPage(0, 2, 0x180, 0), &rect);
    }
}


void GearShopMenuShopDataManager(u8 mode) {
    u_int* pArchive;

    if (mode < SHOP_DATA_FREE) {
        pArchive = HeapAlloc(ArchiveDecodeAlignedSize(2), 1);
        ArchiveReadFileToBuffer(2, pArchive, 0, 0x80);
        ArchiveCdDataSync(0);
        ResolveArchiveEntryPointers(pArchive);
    }

    switch (mode) {
        case SHOP_DATA_INITIALIZE:
            g_Menu->pDressingRoom->pGearFrames = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_FRAME_DATA], 0);
            g_Menu->pDressingRoom->pGearEngines = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_ENGINE_DATA], 0);
            g_Menu->pDressingRoom->pGearArmor = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_ARMOR_DATA], 0);
            g_Menu->pDressingRoom->pGearAccessories = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_ACCESORY_DATA], 0);
            g_Menu->pDressingRoom->pGearWeapons = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_WEAPON_DATA], 0);

            // NOTE: Name of struct field is misleading here!
            // In the non-gear shop menu, this points to weapon descriptions.
            // In the gear shop menu, this points to gear accesory descriptions.
            g_Menu->pShop->pWeaponDescriptions = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_ACCESSORY_DESC], 0);

            // ^ same situation as above
            g_Menu->pShop->pAccessoryDescriptions = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_WEAPON_DESC], 0);

            g_Menu->pShop->pGearFrameDescriptions = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_FRAME_DESC], 0);
            g_Menu->pShop->pGearEngineDescriptions = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_ENGINE_DESC], 0);
            g_Menu->pShop->pAmmoDescriptions = LZSSHeapDecompress(pArchive[SHOP_RES_AMMO_DESC], 0);
            g_Menu->pShop->pGearArmorNames = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_ARMOR_NAMES], 0);
            g_Menu->pShop->pGearFrameNames = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_FRAME_NAMES], 0);
            g_Menu->pShop->pGearEngineNames = LZSSHeapDecompress(pArchive[SHOP_RES_GEAR_ENGINE_NAMES], 0);
            break;
        case SHOP_DATA_FREE:
            HeapFree(g_Menu->pDressingRoom->pGearFrames);
            HeapFree(g_Menu->pDressingRoom->pGearEngines);
            HeapFree(g_Menu->pDressingRoom->pGearArmor);
            HeapFree(g_Menu->pDressingRoom->pGearAccessories);
            HeapFree(g_Menu->pDressingRoom->pGearWeapons);
            HeapFree(g_Menu->pShop->pWeaponDescriptions);
            HeapFree(g_Menu->pShop->pAccessoryDescriptions);
            HeapFree(g_Menu->pShop->pGearFrameDescriptions);
            HeapFree(g_Menu->pShop->pGearEngineDescriptions);
            HeapFree(g_Menu->pShop->pAmmoDescriptions);
            HeapFree(g_Menu->pShop->pGearArmorNames);
            HeapFree(g_Menu->pShop->pGearFrameNames);
            HeapFree(g_Menu->pShop->pGearEngineNames);
            break;   
    }
    
    if (mode < SHOP_DATA_FREE) {
        HeapFree(pArchive);
    }
}

void GearShopMenuLoadGearShopInventory(void) {
    int i, j;
    GearShopDefinition *pShopDef;

    pShopDef = &g_Menu->pGearShopEntries[D_80059171];

    g_GearShopInventoryCounts[0] = 0;
    for(i = 0; i < 20; i++) {
        g_Menu->pShop->gearShopItemTables[0].itemIds[i] = pShopDef->unk14[i];
        if(g_Menu->pShop->gearShopItemTables[0].itemIds[i] != 0) {
            g_GearShopInventoryCounts[0]++;
        }
    }
    i = 0;
    g_GearShopInventoryCounts[1] = 0;
    for(; i < 20; i++) {
        g_Menu->pShop->gearShopItemTables[1].itemIds[i] = pShopDef->unk0[i];
        if(g_Menu->pShop->gearShopItemTables[1].itemIds[i] != 0) {
            g_GearShopInventoryCounts[1]++;
        }
    }

    i = 0;
    g_GearShopInventoryCounts[2] = 0;
    for(; i < 20; i++) {
        g_Menu->pShop->gearShopItemTables[2].itemIds[i] = pShopDef->unk28[i];
        if(g_Menu->pShop->gearShopItemTables[2].itemIds[i] != 0) {
            g_GearShopInventoryCounts[2]++;
        }
    }

    i = 0;
    g_GearShopInventoryCounts[3] = 0;
    for(; i < 20; i++) {
        g_Menu->pShop->gearShopItemTables[3].itemIds[i] = pShopDef->unk50[i];
        if(g_Menu->pShop->gearShopItemTables[3].itemIds[i] != 0) {
            g_GearShopInventoryCounts[3]++;
        }
    }

    i = 0;
    g_GearShopInventoryCounts[4] = 0;
    for(; i < 20; i++) {
        g_Menu->pShop->gearShopItemTables[4].itemIds[i] = pShopDef->unk3C[i];
        if(g_Menu->pShop->gearShopItemTables[4].itemIds[i] != 0) {
            g_GearShopInventoryCounts[4]++;
        }
    }

    HeapFree(g_Menu->pGearShopEntries);
    GearShopMenuShopDataManager(0);

    for(j = 0; j < 9; j++) {
        for(i = 0; i < 2; i++) {
            SetLineF3(&g_Menu->pShop->linesPortraitHighlight1[(j*2)+i]);
            setRGB0(&g_Menu->pShop->linesPortraitHighlight1[(j*2)+i], 0xFF, 0, 0);
            SetLineF3(&g_Menu->pShop->linesPortraitHighlight2[(j*2)+i]);
            setRGB0(&g_Menu->pShop->linesPortraitHighlight2[(j*2)+i], 0xFF, 0, 0);
            setXY3(&g_Menu->pShop->linesPortraitHighlight1[(j*2)+i],
                   D_801D6C44[j], 0xA6,
                   D_801D6C44[j] + 0x18, 0xA6,
                   D_801D6C44[j] + 0x18, 0xBC
            );
            setXY3(&g_Menu->pShop->linesPortraitHighlight2[(j*2)+i],
                   D_801D6C44[j], 0xA6,
                   D_801D6C44[j], 0xBC,
                   D_801D6C44[j] + 0x18, 0xBC
            );
        }

        g_Menu->pShop->unk469C[j] = 0;
    }

    func_8002675C(g_Menu->resources, 0x166, (POLY_FT4* ) g_Menu->menuUnk8->unk80, g_Menu->renderContext, 0x108, 0x18, 0x1000U);

    for(j = 0; j < 2; j++) {
        SetPolyFT4(&g_Menu->menuUnk8->unk0.polys[j]);
        SetSemiTrans(&g_Menu->menuUnk8->unk0.polys[j], 0);
        SetShadeTex(&g_Menu->menuUnk8->unk0.polys[j], 0);

        setRGB0(&g_Menu->menuUnk8->unk0.polys[j], 0x80, 0x80, 0x80);

        g_Menu->menuUnk8->unk0.polys[j].tpage = GetTPage(0, 0, 0x180, 0);
        g_Menu->menuUnk8->unk0.polys[j].clut = g_SystemPalette1;

        setUV4(&g_Menu->menuUnk8->unk0.polys[j],
               0, 0x48,
               0x60, 0x48,
               0, 0x55,
               0x60, 0x55
        );

        setXY4(&g_Menu->menuUnk8->unk0.polys[j],
               0x10, 0x20,
               0x70, 0x20,
               0x10, 0x2D,
               0x70, 0x2D
        );
    }
}

void GearShopMenuSetVertices(SVECTOR* pVertices, u_short x, u_short y, u_short width, u_short height) {
    pVertices[0].vx = x - 160;
    pVertices[0].vy = y - 112;
    pVertices[0].vz = 0;

    pVertices[1].vx = x + width - 160;
    pVertices[1].vy = y - 112;
    pVertices[1].vz = 0;

    pVertices[2].vx = x - 160;
    pVertices[2].vy = y + height - 112;
    pVertices[2].vz = 0;

    pVertices[3].vx = x + width - 160;
    pVertices[3].vy = y + height - 112;
    pVertices[3].vz = 0;
}

void GearShopMenuSetWindowBorderPrimitive(P_TAG* pTag) {
    SetSemiTrans(pTag, 1);
    SetShadeTex(pTag, 0);
    setRGB0(pTag, 128, 128, 128);
}

void GearShopMenuUpdateScrollBarHandle(int x, int y, int scrollHandleHeight, int numItems, int scrollOffset) {
    int yOffset = 0;

    if (!g_Menu->pManager->scrollHandleActive) {
        g_Menu->pScrollHandle = HeapAlloc(sizeof(MenuScrollBarHandle), 0x0);
        bzero((u8 *)g_Menu->pScrollHandle, sizeof(MenuScrollBarHandle));
    }

    // If the total number of items in the window fits within what we can view,
    // there's no need for scrolling so we set the scroll handle height to fill
    // the entire scroll bar.
    if (numItems <= SHOP_MAX_ITEMS_IN_VIEW) {
        scrollHandleHeight = 100;
    }
    // Compute the Y offset based on our current scroll offset
    else {
        yOffset = (scrollOffset * 100) / (numItems - SHOP_MAX_ITEMS_IN_VIEW);
        yOffset = (yOffset * 4000) / 10000;
    }

    func_8002675C(g_Menu->resources, MENU_TEX_SCROLL_BAR_HANDLE,
        g_Menu->pScrollHandle->polys, g_Menu->renderContext,
        x, y, 0x1000
    );

    GearShopMenuSetVertices(
        g_Menu->pScrollHandle->vertices,
        x, y + yOffset,
        8, scrollHandleHeight
    );

    g_Menu->pScrollHandle->renderContext = g_Menu->renderContext;
    g_Menu->pManager->scrollHandleActive = TRUE;
}

void GearShopMenuFreeScrollBarHandle(void) {
    g_Menu->pManager->scrollHandleActive = FALSE;
    HeapFree(g_Menu->pScrollHandle);
}

void GearShopMenuInitializeArrowCursor(u_char index) {
    MenuArrowCursor* pCursor;

    pCursor = HeapAlloc(sizeof(MenuArrowCursor), 0);
    g_Menu->arrowCursors[index] = pCursor;
    bzero((u8* ) pCursor, sizeof(MenuArrowCursor));
    g_Menu->arrowCursors[index]->curAnimFrame = 4;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

void GearShopMenuUpdateArrowCursor(s32 rowIndex, s32 _unused, u8 disableCursor, u_char cursorIndex) {
    POLY_FT4* pPoly;
    MenuArrowCursor* pCursor = g_Menu->arrowCursors[cursorIndex];
    int yPos;
    u8 shouldRenderCursor;

    if( ++pCursor->animFrameDuration >= 6) {
        if(--pCursor->curAnimFrame < 0) {
            pCursor->curAnimFrame = 4;
        }
        pCursor->animFrameDuration = 0;
    }

    if(disableCursor == FALSE) {
        yPos = (FONT_LETTER_HEIGHT * rowIndex) + 50;
        shouldRenderCursor = TRUE;
    }

    if(shouldRenderCursor) {
        func_8002675C(
            g_Menu->resources,
            pCursor->curAnimFrame + MENU_TEX_ARROW_CURSOR,
            &pCursor->polys[0],
            g_Menu->renderContext,
            0,
            0,
            0x1000
        );
        pPoly = &pCursor->polys[g_Menu->renderContext];
        GearShopMenuSetVertices(
            pCursor->vertices,
            pPoly->x0 + 0x1c,
            pPoly->y0 + yPos,
            pPoly->x1 - pPoly->x0,
            pPoly->y3 - pPoly->y0
        );
        pCursor->renderContext = g_Menu->renderContext;
        g_Menu->pManager->shouldRenderArrowCursor[cursorIndex] = TRUE;
        return;
    }
    g_Menu->pManager->shouldRenderArrowCursor[cursorIndex] = FALSE;
}

void GearShopMenuFreeArrowCursor(u_char cursorIndex) {
    HeapFree(g_Menu->arrowCursors[cursorIndex]);
    g_Menu->pManager->shouldRenderArrowCursor[cursorIndex] = FALSE;
}

void GearShopMenuInitializeWindowGraphics(u_char windowIndex) {
    RECT rect;
    MenuWindow* pWindow;
    u_char i;

    pWindow = g_Menu->windows[windowIndex];

    rect.y = 0;
    rect.x = 0;
    rect.h = 256;
    rect.w = 256;

    g_Menu->pManager->shouldRenderWindow[windowIndex] = FALSE;
    g_Menu->pManager->unk27[windowIndex] = FALSE;

    // Window background
    for (i = 0; i < 2; i++) {
        SetPolyG4(&pWindow->polysBackground[i]);
        setRGB0(&pWindow->polysBackground[i], 104, 104, 104);
        setRGB1(&pWindow->polysBackground[i], 104, 104, 104);
        setRGB2(&pWindow->polysBackground[i], 104, 104, 104);
        setRGB3(&pWindow->polysBackground[i], 104, 104, 104);
        SetSemiTrans(&pWindow->polysBackground[i], 1);
        SetDrawMode(
            &pWindow->drawModes[i],
            0, 0,
            GetTPage(0, 0, g_Menu->texPageX0, g_Menu->texPageY0),
            &rect
        );
    }

    // Window borders
    for (i = 0; i < 4; i++) {
        SetPolyFT4(&pWindow->polysWindowBorderTop[i]);
        SetShadeTex(&pWindow->polysWindowBorderTop[i], 1);
        setRGB0(&pWindow->polysWindowBorderTop[i], 0xFF, 0xFF, 0xFF);
        pWindow->polysWindowBorderTop[i].tpage = GetTPage(g_Menu->texPage0, 0, g_Menu->texPageX0, g_Menu->texPageY0);
        pWindow->polysWindowBorderTop[i].clut = GetClut(g_Menu->clutX0, g_Menu->clutY0);

        SetPolyFT4(&pWindow->polysWindowBorderBottom[i]);
        SetShadeTex(&pWindow->polysWindowBorderBottom[i], 1);
        setRGB0(&pWindow->polysWindowBorderBottom[i], 0xFF, 0xFF, 0xFF);
        pWindow->polysWindowBorderBottom[i].tpage = GetTPage(g_Menu->texPage1, 0, g_Menu->texPageX1, g_Menu->texPageY1);
        pWindow->polysWindowBorderBottom[i].clut = GetClut(g_Menu->clutX1, g_Menu->clutY1);

        SetPolyFT4(&pWindow->polysWindowBorderLeft[i]);
        SetShadeTex(&pWindow->polysWindowBorderLeft[i], 1);
        setRGB0(&pWindow->polysWindowBorderLeft[i], 0xFF, 0xFF, 0xFF);
        pWindow->polysWindowBorderLeft[i].tpage = GetTPage(g_Menu->texPage2, 0, g_Menu->texPageX2, g_Menu->texPageY2);
        pWindow->polysWindowBorderLeft[i].clut = GetClut(g_Menu->clutX2, g_Menu->clutY2);

        SetPolyFT4(&pWindow->polysWindowBorderRight[i]);
        SetShadeTex(&pWindow->polysWindowBorderRight[i], 1);
        setRGB0(&pWindow->polysWindowBorderRight[i], 0xFF, 0xFF, 0xFF);
        pWindow->polysWindowBorderRight[i].tpage = GetTPage(g_Menu->texPage3, 0, g_Menu->texPageX3, g_Menu->texPageY3);
        pWindow->polysWindowBorderRight[i].clut = GetClut(g_Menu->clutX3, g_Menu->clutY3);
    }
}

void GearShopMenuInitializeScrollBar(u_char windowIndex, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[windowIndex];

    // Top ornament
    func_8002675C(
        g_Menu->resources,
        MENU_TEX_SCROLL_BAR_ORNAMENT,
        pWindow->polysScrollBarEnds,
        g_Menu->renderContext,
        x, y, 0x1000
    );

    // Bottom ornament
    func_800263E4(
        g_Menu->resources,
        MENU_TEX_SCROLL_BAR_ORNAMENT,
        &pWindow->polysScrollBarEnds[2],
        g_Menu->renderContext,
        x,
        y + height - 8,
        0x1000, 0, 1
    );

    func_8002675C(
        g_Menu->resources,
        MENU_TEX_SCROLL_BAR_EMPTY,
        pWindow->polysScrollBarEmpty,
        g_Menu->renderContext,
        x,
        y + 8,
        0x1000
    );

    GearShopMenuSetVertices(&pWindow->vertsScrollBarEnds[0], x, y, 8, 8);
    GearShopMenuSetVertices(&pWindow->vertsScrollBarEnds[4], x, y + height, 8, -8);
    GearShopMenuSetVertices(&pWindow->vertsScrollBarEmpty[0], x, y + 8, 8, height - 8);
}

void GearShopMenuInitializeWindowBorderCorners(u_char windowIndex, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[windowIndex];
    int i;

    pWindow->unk710 = 0;
    pWindow->unk710 += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_TOP_LEFT,
        &pWindow->polysWindowBorderCorners[pWindow->unk710*2],
        g_Menu->renderContext, 0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_TOP_RIGHT,
        &pWindow->polysWindowBorderCorners[pWindow->unk710*2],
        g_Menu->renderContext, 0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_BOTTOM_LEFT,
        &pWindow->polysWindowBorderCorners[pWindow->unk710*2],
        g_Menu->renderContext, 0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_BOTTOM_RIGHT,
        &pWindow->polysWindowBorderCorners[pWindow->unk710*2],
        g_Menu->renderContext, 0, 0, 0x1000
    );

    GearShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[0], 
        x - 8, y +  8, 16, -16
    );
    GearShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[4], 
        x + width + 8, y + 8, -16, -16
    );
    GearShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[8], 
        x - 8, (y + height) - 8, 16, 16
    );
    GearShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[0xC], 
        x + width + 8, (y + height) - 8, -16, 16
    );

    for(i = 0; i < 4; i++) {
        GearShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderCorners[(i*2) + g_Menu->renderContext]);
    }
}

void GearShopMenuSetWindowBorderTop(u_char windowIndex, u_short x, u_short y, u_short width) {
    MenuWindow* pWindow = g_Menu->windows[windowIndex];
    int i;
    int innerWidth;
    u_short halfInnerWidth;

    setUV4(
        &pWindow->polysWindowBorderTop[g_Menu->renderContext],
        0, 132,
        7, 132,
        0, 148,
        7, 148
    );
    setUV4(
        &pWindow->polysWindowBorderTop[2 + g_Menu->renderContext],
        0, 132,
        7, 132,
        0, 148,
        7, 148
    );

    innerWidth = width - (MENU_WINDOW_BORDER_SIZE * 2);
    halfInnerWidth = innerWidth / 2;

    GearShopMenuSetVertices(
        pWindow->vertsWindowBorderTop1,
        x + MENU_WINDOW_BORDER_SIZE,
        y - MENU_WINDOW_BORDER_SIZE,
        halfInnerWidth,
        MENU_WINDOW_BORDER_SIZE * 2
    );
    GearShopMenuSetVertices(
        pWindow->vertsWindowBorderTop2,
        x + MENU_WINDOW_BORDER_SIZE + halfInnerWidth,
        y - MENU_WINDOW_BORDER_SIZE,
        halfInnerWidth,
        MENU_WINDOW_BORDER_SIZE * 2
    );

    for (i = 0; i < 2; i++) {
        GearShopMenuSetWindowBorderPrimitive(
            (P_TAG *)&pWindow->polysWindowBorderTop[i * 2 + g_Menu->renderContext]
        );
    }
}

void GearShopMenuSetWindowBorderBottom(u_char windowIndex, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerWidth;

    pWindow = g_Menu->windows[windowIndex];

    setUV4(
        &pWindow->polysWindowBorderBottom[g_Menu->renderContext],
        8, 132,
        15, 132,
        8, 148,
        15, 148
    );

    setUV4(
        &pWindow->polysWindowBorderBottom[2 + g_Menu->renderContext],
        8, 132,
        15, 132,
        8, 148,
        15, 148
    );

    innerWidth = width - 16;
    width = innerWidth / 2;
    height = y + height - 8;

    GearShopMenuSetVertices(
        pWindow->vertsWindowBorderBottom1,
        x + 8,
        height,
        width,
        16
    );
    GearShopMenuSetVertices(
        pWindow->vertsWindowBorderBottom2,
        x + 8 + width,
        height,
        width,
        16
    );

    for (i = 0; i < 2; i++) {
        GearShopMenuSetWindowBorderPrimitive((P_TAG *)&pWindow->polysWindowBorderBottom[i * 2 + g_Menu->renderContext]);
    }
}

void GearShopMenuSetWindowBorderLeft(u_char windowIndex, u_short x, u_short y, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerHeight;
    u_short halfInnerHeight;

    pWindow = g_Menu->windows[windowIndex];

    setUV4(
        &pWindow->polysWindowBorderLeft[g_Menu->renderContext],
        16, 132,
        32, 132,
        16, 139,
        32, 139
    );

    setUV4(
        &pWindow->polysWindowBorderLeft[2 + g_Menu->renderContext],
        16, 132,
        32, 132,
        16, 139,
        32, 139
    );

    innerHeight = height - 16;
    halfInnerHeight = innerHeight / 2;

    GearShopMenuSetVertices(
        pWindow->vertsWindowBorderLeft1,
        x - 8,
        y + 8,
        16,
        halfInnerHeight
    );
    GearShopMenuSetVertices(
        pWindow->vertsWindowBorderLeft2,
        x - 8,
        y + 8 + halfInnerHeight,
        16,
        halfInnerHeight
    );

    for (i = 0; i < 2; i++) {
        GearShopMenuSetWindowBorderPrimitive((P_TAG *)&pWindow->polysWindowBorderLeft[i * 2 + g_Menu->renderContext]);
    }
}

void GearShopMenuSetWindowBorderRight(u_char windowIndex, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerHeight;
    u_short halfInnerHeight;

    pWindow = g_Menu->windows[windowIndex];

    setUV4(
        &pWindow->polysWindowBorderRight[g_Menu->renderContext],
        16, 140,
        32, 140,
        16, 147,
        32, 147
    );

    setUV4(
        &pWindow->polysWindowBorderRight[2 + g_Menu->renderContext],
        16, 140,
        32, 140,
        16, 147,
        32, 147
    );

    width = x + width - 8;
    innerHeight = height - 16;
    halfInnerHeight = innerHeight / 2;

    GearShopMenuSetVertices(
        pWindow->vertsWindowBorderRight1,
        width,
        y + 8,
        16,
        halfInnerHeight
    );
    GearShopMenuSetVertices(
        pWindow->vertsWindowBorderRight2,
        width,
        y + 8 + halfInnerHeight,
        16,
        halfInnerHeight
    );

    for (i = 0; i < 2; i++) {
        GearShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderRight[i * 2 + g_Menu->renderContext]);
    }
}

void GearShopMenuSetWindow(u_char windowIndex, u_short x, u_short y, u_short width, u_short height, u8 arg5, int zIndex, u_char hasScrollbar) {
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[windowIndex];
    g_Menu->pManager->shouldRenderWindow[windowIndex] = FALSE;
    GearShopMenuSetVertices(pWindow->vertsBackground, x, y, width, height);
    GearShopMenuInitializeWindowBorderCorners(windowIndex, x, y, width, height);
    GearShopMenuSetWindowBorderTop(windowIndex, x, y, width);
    GearShopMenuSetWindowBorderBottom(windowIndex, x, y, width, height);
    GearShopMenuSetWindowBorderLeft(windowIndex, x, y, height);
    GearShopMenuSetWindowBorderRight(windowIndex, x, y, width, height);
    if (hasScrollbar) {
        GearShopMenuInitializeScrollBar(windowIndex, x, y, width, height);
    }
    pWindow->hasScrollBar = hasScrollbar;
    pWindow->unk714 = arg5;
    pWindow->zIndex = zIndex;
    pWindow->renderContext = g_Menu->renderContext;
    g_Menu->pManager->shouldRenderWindow[windowIndex] = TRUE;
}

void GearShopMenuFreeWindow(u_char index) {
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    g_Menu->pManager->unk27[index] = FALSE;
    HeapFree(g_Menu->windows[index]);
    HeapFree(g_Menu->windowParameters[index]);
}

void GearShopMenuInitializeWindow(u_char windowIndex, u_short x, u_short y, u_short width, u_short height, u_char shouldInitializeHandle, u8 arg6, int zIndex, u_char hasScrollbar) {
    MenuWindowParameters* pWindowParameters;

    if (windowIndex >= 2) {
        g_Menu->windows[windowIndex] = HeapAlloc(sizeof(MenuWindow), 0);
        bzero((u8* ) g_Menu->windows[windowIndex], sizeof(MenuWindow));
        g_Menu->windowParameters[windowIndex] = HeapAlloc(sizeof(MenuWindowParameters), 0);
        bzero((u8* ) g_Menu->windowParameters[windowIndex], sizeof(MenuWindowParameters));
        GearShopMenuInitializeWindowGraphics(windowIndex);
    }

    pWindowParameters = g_Menu->windowParameters[windowIndex];
    if (shouldInitializeHandle) {
        pWindowParameters->index = windowIndex;
        pWindowParameters->unk11 = 0;
        pWindowParameters->x = x;
        pWindowParameters->y = y;
        pWindowParameters->width = width;
        pWindowParameters->height = height;
        pWindowParameters->unk8 = 0;
        pWindowParameters->unkA = 0;
        g_Menu->pManager->unk27[windowIndex] = TRUE;
        pWindowParameters->unk12 = arg6;
        pWindowParameters->zIndex = zIndex;
        return;
    }

    GearShopMenuSetWindow(windowIndex, x, y, width, height, arg6, zIndex, hasScrollbar);
}

void GearShopMenuUpdateWindows(void) {
    MenuWindowParameters* windowParams;
    u8 clipCount;
    s32 i;
    u16 widthOffset;
    u16 heightOffset;
    u16 clippedWidth;
    u16 clippedHeight;

    for(i = 0; i < MENU_MAX_NUM_WINDOWS; i++) {
        windowParams = g_Menu->windowParameters[i];
        if ((g_Menu->pManager->unk27[i]) && (windowParams->unk11 == 0)) {
            clipCount = 0;
            if ((windowParams->unk8 + 0x20) >= (s32) windowParams->width) {
                clippedWidth = windowParams->width;
                clipCount = 1;
            } else {
                clippedWidth = windowParams->unk8 + 0x20;
            }
            windowParams->unk8 = clippedWidth;
            if ((windowParams->unkA + 0x20) >= (s32) windowParams->height) {
                clippedHeight = windowParams->height;
                clipCount += 1;
            } else {
                clippedHeight = windowParams->unkA + 0x20;
            }
            windowParams->unkA = clippedHeight;
            if (clipCount == 2) {
                windowParams->unk11 = TRUE;
            }
            heightOffset = windowParams->unkA;
            widthOffset = windowParams->unk8;
            GearShopMenuSetWindow(
                windowParams->index,
                ((windowParams->x + ((u16) windowParams->width >> 1)) - (widthOffset >> 1)),
                ((windowParams->y + ((u16) windowParams->height >> 1)) - (heightOffset >> 1)),
                widthOffset,
                (s32) heightOffset,
                (s32) windowParams->unk12,
                windowParams->zIndex,
                (s32) windowParams->hasScrollBar);
        }
    }
}

void GearShopMenuRenderPolygons(int count, SVECTOR* pVertices, POLY_FT4* pPolys, int renderContext) {
    long interpolated;
    long flag;
    int i;

    for (i = 0; i < count; i++) {
        RotTransPers4(
            &pVertices[i*4 + 0],
            &pVertices[i*4 + 1],
            &pVertices[i*4 + 2],
            &pVertices[i*4 + 3],
            (long *)&pPolys[i*2 + renderContext].x0,
            (long *)&pPolys[i*2 + renderContext].x1,
            (long *)&pPolys[i*2 + renderContext].x2,
            (long *)&pPolys[i*2 + renderContext].x3,
            &interpolated,
            &flag
        );

        AddPrim(&g_Menu->pGfxEnv->ot[4], &pPolys[i*2 + renderContext]);
    }
}

void GearShopMenuRenderString(int stringLength, POLY_FT4* pPolys, int renderContext) {
    int i;

    for(i = 0; i < stringLength; i++) {
        AddPrim(&g_Menu->pGfxEnv->ot[4], &pPolys[(2*i) + renderContext]);
    }
}

void GearShopMenuRenderScrollBarHandle(void) {
    if (g_Menu->pManager->scrollHandleActive) {
        GearShopMenuRenderPolygons(
            1,
            g_Menu->pScrollHandle->vertices,
            g_Menu->pScrollHandle,
            g_Menu->pScrollHandle->renderContext
        );
    }
}

void GearShopMenuRenderSelectionCursor(void) {
    AddPrim(
        &g_Menu->pGfxEnv->ot[4], 
        &g_Menu->unk348->drModes1[g_Menu->unk348->unk159]
    );
    if (g_Menu->pManager->unk4) {
        AddPrim(
            &g_Menu->pGfxEnv->ot[4], 
            &g_Menu->unk348->polysPointerCursor[g_Menu->unk348->cursorRenderContext]
        );
    }
}

void GearShopMenuRenderShoulderButtonUi(void) {
    if (g_Menu->pManager->shoulderButtonUiActive && (g_gearShopAvailableCharacterCount >= 2)) {
        GearShopMenuRenderPolygons(
            4, 
            g_Menu->pShoulderButtonUi->vertices, 
            g_Menu->pShoulderButtonUi, 
            g_Menu->pShoulderButtonUi->renderContext
        );
    }
}

void GearShopMenuRenderTopWindowBorder(int windowId) {
    long interpolated;
    long flag;
    MenuWindow* window;

    window = g_Menu->windows[windowId];

    RotTransPers4(
        &window->vertsWindowBorderTop1[0],
        &window->vertsWindowBorderTop1[1],
        &window->vertsWindowBorderTop1[2],
        &window->vertsWindowBorderTop1[3],
        (long*) &window->polysWindowBorderTop[window->renderContext].x0,
        (long*) &window->polysWindowBorderTop[window->renderContext].x1,
        (long*) &window->polysWindowBorderTop[window->renderContext].x2,
        (long*) &window->polysWindowBorderTop[window->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysWindowBorderTop[window->renderContext]
    );

    RotTransPers4(
        &window->vertsWindowBorderTop2[0],
        &window->vertsWindowBorderTop2[1],
        &window->vertsWindowBorderTop2[2],
        &window->vertsWindowBorderTop2[3],
        (long*) &window->polysWindowBorderTop[2 + window->renderContext].x0,
        (long*) &window->polysWindowBorderTop[2 + window->renderContext].x1,
        (long*) &window->polysWindowBorderTop[2 + window->renderContext].x2,
        (long*) &window->polysWindowBorderTop[2 + window->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysWindowBorderTop[2 + window->renderContext]
    );
}

void GearShopMenuRenderBottomWindowBorder(int windowId) {
    long interpolated;
    long flag;
    MenuWindow* window;

    window = g_Menu->windows[windowId];

    RotTransPers4(
        &window->vertsWindowBorderBottom1[0],
        &window->vertsWindowBorderBottom1[1],
        &window->vertsWindowBorderBottom1[2],
        &window->vertsWindowBorderBottom1[3],
        (long*) &window->polysWindowBorderBottom[window->renderContext].x0,
        (long*) &window->polysWindowBorderBottom[window->renderContext].x1,
        (long*) &window->polysWindowBorderBottom[window->renderContext].x2,
        (long*) &window->polysWindowBorderBottom[window->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysWindowBorderBottom[window->renderContext]
    );

    RotTransPers4(
        &window->vertsWindowBorderBottom2[0],
        &window->vertsWindowBorderBottom2[1],
        &window->vertsWindowBorderBottom2[2],
        &window->vertsWindowBorderBottom2[3],
        (long*) &window->polysWindowBorderBottom[2 + window->renderContext].x0,
        (long*) &window->polysWindowBorderBottom[2 + window->renderContext].x1,
        (long*) &window->polysWindowBorderBottom[2 + window->renderContext].x2,
        (long*) &window->polysWindowBorderBottom[2 + window->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysWindowBorderBottom[2 + window->renderContext]
    );
}

void GearShopMenuRenderLeftWindowBorder(int windowId) {
    long interpolated;
    long flag;
    MenuWindow* window;

    window = g_Menu->windows[windowId];

    RotTransPers4(
        &window->vertsWindowBorderLeft1[0],
        &window->vertsWindowBorderLeft1[1],
        &window->vertsWindowBorderLeft1[2],
        &window->vertsWindowBorderLeft1[3],
        (long*) &window->polysWindowBorderLeft[window->renderContext].x0,
        (long*) &window->polysWindowBorderLeft[window->renderContext].x1,
        (long*) &window->polysWindowBorderLeft[window->renderContext].x2,
        (long*) &window->polysWindowBorderLeft[window->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysWindowBorderLeft[window->renderContext]
    );

    RotTransPers4(
        &window->vertsWindowBorderLeft2[0],
        &window->vertsWindowBorderLeft2[1],
        &window->vertsWindowBorderLeft2[2],
        &window->vertsWindowBorderLeft2[3],
        (long*) &window->polysWindowBorderLeft[2 + window->renderContext].x0,
        (long*) &window->polysWindowBorderLeft[2 + window->renderContext].x1,
        (long*) &window->polysWindowBorderLeft[2 + window->renderContext].x2,
        (long*) &window->polysWindowBorderLeft[2 + window->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysWindowBorderLeft[2 + window->renderContext]
    );
}

void GearShopMenuRenderRightWindowBorder(int windowId) {
    long interpolated;
    long flag;
    MenuWindow* window;

    window = g_Menu->windows[windowId];

    RotTransPers4(
        &window->vertsWindowBorderRight1[0],
        &window->vertsWindowBorderRight1[1],
        &window->vertsWindowBorderRight1[2],
        &window->vertsWindowBorderRight1[3],
        (long*) &window->polysWindowBorderRight[window->renderContext].x0,
        (long*) &window->polysWindowBorderRight[window->renderContext].x1,
        (long*) &window->polysWindowBorderRight[window->renderContext].x2,
        (long*) &window->polysWindowBorderRight[window->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysWindowBorderRight[window->renderContext]
    );

    RotTransPers4(
        &window->vertsWindowBorderRight2[0],
        &window->vertsWindowBorderRight2[1],
        &window->vertsWindowBorderRight2[2],
        &window->vertsWindowBorderRight2[3],
        (long*) &window->polysWindowBorderRight[2 + window->renderContext].x0,
        (long*) &window->polysWindowBorderRight[2 + window->renderContext].x1,
        (long*) &window->polysWindowBorderRight[2 + window->renderContext].x2,
        (long*) &window->polysWindowBorderRight[2 + window->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysWindowBorderRight[2 + window->renderContext]
    );
}

void GearShopMenuRenderWindowBackground(s32 windowId) {
    long interpolation;
    long flag;
    MenuWindow* window;

    window = g_Menu->windows[windowId];
    RotTransPers4(
        &window->vertsBackground[0],
        &window->vertsBackground[1],
        &window->vertsBackground[2],
        &window->vertsBackground[3],
        (long* ) &window->polysBackground[window->renderContext].x0,
        (long* ) &window->polysBackground[window->renderContext].x1,
        (long* ) &window->polysBackground[window->renderContext].x2,
        (long* ) &window->polysBackground[window->renderContext].x3,
        &interpolation,
        &flag);
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysBackground[window->renderContext]
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->drawModes[window->renderContext]
    );
}

void GearShopMenuRenderWindowBorderCorners(s32 arg0) {
    long interpolation;
    long code;
    MenuWindow *window;
    int i;

    window = g_Menu->windows[arg0];

    for(i = 0; i < 4; i++) {
        RotTransPers4(
            &window->vertsWindowBorderCorners[(i*4)],
            &window->vertsWindowBorderCorners[(i*4)+1],
            &window->vertsWindowBorderCorners[(i*4)+2],
            &window->vertsWindowBorderCorners[(i*4)+3],
            (long* ) &window->polysWindowBorderCorners[i*2 + window->renderContext].x0,
            (long* ) &window->polysWindowBorderCorners[i*2 + window->renderContext].x1,
            (long* ) &window->polysWindowBorderCorners[i*2 + window->renderContext].x2,
            (long* ) &window->polysWindowBorderCorners[i*2 + window->renderContext].x3,
            &interpolation,
            &code);
        AddPrim(&g_Menu->pGfxEnv->ot[window->zIndex], &window->polysWindowBorderCorners[i*2 + window->renderContext]);
    }
}

void GearShopMenuRenderScrollBar(s32 windowId) {
    int i;
    MenuWindow *window = g_Menu->windows[windowId];
    long p, flag;

    for(i = 0; i < 2; i++) {
        RotTransPers4(
            &window->vertsScrollBarEnds[(i*4)],
            &window->vertsScrollBarEnds[(i*4) + 1],
            &window->vertsScrollBarEnds[(i*4) + 2],
            &window->vertsScrollBarEnds[(i*4) + 3],
            (long *)&window->polysScrollBarEnds[(i*2) + window->renderContext].x0,
            (long *)&window->polysScrollBarEnds[(i*2) + window->renderContext].x1,
            (long *)&window->polysScrollBarEnds[(i*2) + window->renderContext].x2,
            (long *)&window->polysScrollBarEnds[(i*2) + window->renderContext].x3,
            &p,
            &flag
        );
        AddPrim(
            &g_Menu->pGfxEnv->ot[window->zIndex],
            &window->polysScrollBarEnds[(i*2) + window->renderContext]
        );
    }

    RotTransPers4(
        &window->vertsScrollBarEmpty[0],
        &window->vertsScrollBarEmpty[1],
        &window->vertsScrollBarEmpty[2],
        &window->vertsScrollBarEmpty[3],
        (long *)&window->polysScrollBarEmpty[window->renderContext].x0,
        (long *)&window->polysScrollBarEmpty[window->renderContext].x1,
        (long *)&window->polysScrollBarEmpty[window->renderContext].x2,
        (long *)&window->polysScrollBarEmpty[window->renderContext].x3,
        &p,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[window->zIndex],
        &window->polysScrollBarEmpty[window->renderContext]
    );
}

void GearShopMenuRenderWindows(void) {
    SVECTOR rotation;
    VECTOR translation;
    MATRIX matTransform;
    SVECTOR _unused;
    MenuWindow* pWindow;
    int i;

    for(i = 0; i < MENU_MAX_NUM_WINDOWS; i++) {
        if (g_Menu->pManager->shouldRenderWindow[i]) {
            pWindow = g_Menu->windows[i];

            if(pWindow->unk714 == 0) {
                PushMatrix();
                rotation.vz = 0;
                rotation.vy = 0;
                rotation.vx = 0;
                translation.vy = 0;
                translation.vx = 0;
                translation.vz = 512;
                RotMatrix(&rotation, &matTransform);
                TransMatrix(&matTransform, &translation);
                SetRotMatrix(&matTransform);
                SetTransMatrix(&matTransform);

                GearShopMenuRenderWindowBorderCorners(i);
                if (pWindow->hasScrollBar) {
                    GearShopMenuRenderScrollBar(i);
                }
                GearShopMenuRenderTopWindowBorder(i);
                GearShopMenuRenderBottomWindowBorder(i);
                GearShopMenuRenderLeftWindowBorder(i);
                GearShopMenuRenderRightWindowBorder(i);
                GearShopMenuRenderWindowBackground(i);

                PopMatrix();
            } else {
                GearShopMenuRenderWindowBorderCorners(i);
                if (pWindow->hasScrollBar) {
                    GearShopMenuRenderScrollBar(i);
                }
                GearShopMenuRenderTopWindowBorder(i);
                GearShopMenuRenderBottomWindowBorder(i);
                GearShopMenuRenderLeftWindowBorder(i);
                GearShopMenuRenderRightWindowBorder(i);
                GearShopMenuRenderWindowBackground(i);
            }
        }
    }
}

void GearShopMenuRenderPointerCursors(void) {
    int i;

    if (g_Menu->pManager->shouldRenderCursors) {
        for (i = 0; i < MENU_MAX_NUM_CURSORS; i++) {
            if (g_Menu->pCursors->shouldRender[i]) {
                if (g_Menu->pCursors->unk144[i]) {
                    setXY4(
                        &g_Menu->pCursors->polysCursor[i * 2 + g_Menu->pCursors->renderContexts[i]],
                        D_801D6AFC[D_801D6A84[g_Menu->menuUnk2->unk4F7C]] + 8,
                        D_801D6B7C[D_801D6A84[g_Menu->menuUnk2->unk4F7C]] - 6,
                        D_801D6AFC[D_801D6A84[g_Menu->menuUnk2->unk4F7C]] + 24,
                        D_801D6B7C[D_801D6A84[g_Menu->menuUnk2->unk4F7C]] - 6,
                        D_801D6AFC[D_801D6A84[g_Menu->menuUnk2->unk4F7C]] + 8,
                        D_801D6B7C[D_801D6A84[g_Menu->menuUnk2->unk4F7C]] + 10,
                        D_801D6AFC[D_801D6A84[g_Menu->menuUnk2->unk4F7C]] + 24,
                        D_801D6B7C[D_801D6A84[g_Menu->menuUnk2->unk4F7C]] + 10
                    );
                }

                AddPrim(
                    &g_Menu->pGfxEnv->ot[4],
                    &g_Menu->pCursors->polysCursor[i * 2 + g_Menu->pCursors->renderContexts[i]]
                );
            }
        }
    }
}

void GearShopMenuRenderStrings4E0(void) {
    int i;
    int renderContext;

    for(i = 0; i < 4; i++) {
        if(g_Menu->pManager->unk34[i]) {
            renderContext = g_Menu->unk4E0[i].renderContext;
            AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->unk4E0[i].polys[renderContext]);
        }
    }
}

void GearShopMenuRenderStrings6E0(void) {
    int i;

    for(i = 0; i < 8; i++) {
        if (g_Menu->pManager->unkC[i]) {
            AddPrim(
                &g_Menu->pGfxEnv->ot[4],
                &g_Menu->unk6E0[i].polys[g_Menu->unk6E0[i].renderContext]
            );
        }
    }
}
// identical to func_801CA09C() in ShopMenu
void GearShopMenuRenderStringsAE0(void) {
    long interpolated;
    long flag;
    int i;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->unk14[i]) {
            if (g_Menu->unkAE0[i].unk7F) {
                RotTransPers4(
                    &g_Menu->unkAE0[i].vertices[0],
                    &g_Menu->unkAE0[i].vertices[1],
                    &g_Menu->unkAE0[i].vertices[2],
                    &g_Menu->unkAE0[i].vertices[3],
                    (long*)&g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext].x0,
                    (long*)&g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext].x1,
                    (long*)&g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext].x2,
                    (long*)&g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext].x3,
                    &interpolated,
                    &flag
                );
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4],
                    &g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext]
                );
            } else {
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4],
                    &g_Menu->unkAE0[i].polys[g_Menu->unkAE0[i].renderContext]
                );
            }

        }
    }
}

void GearShopMenuRenderStringsDE0(void) {
    int i;
    int renderContext;

    for(i = 0; i < 6; i++) {
        if (g_Menu->pManager->unk1A[i]) {
            renderContext = g_Menu->unkDE0[i].renderContext;
            AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->unkDE0[i].polys[renderContext]);
        }
    }
}

// identical to ShopMenuAddUnk1DE0Prims in ShopMenu
void GearShopMenuRenderModalStrings(void) {
    long interpolation;
    long code;
    MenuString* pMenuString;
    int i;

    if(g_Menu->pManager->unk2B != 0) {
        for(i = 0; i < 3; i++) {
            pMenuString = g_Menu->unk1DE0[i];

            if(pMenuString->unk7F) {
                RotTransPers4(
                    &pMenuString->vertices[0],
                    &pMenuString->vertices[1],
                    &pMenuString->vertices[2],
                    &pMenuString->vertices[3],
                    (long *)&pMenuString->polys[pMenuString->renderContext].x0,
                    (long *)&pMenuString->polys[pMenuString->renderContext].x1,
                    (long *)&pMenuString->polys[pMenuString->renderContext].x2,
                    (long *)&pMenuString->polys[pMenuString->renderContext].x3,
                    &interpolation,
                    &code
                );
                AddPrim(&g_Menu->pGfxEnv->ot[4], &pMenuString->polys[pMenuString->renderContext]);
            } else {
                AddPrim(&g_Menu->pGfxEnv->ot[4], &pMenuString->polys[pMenuString->renderContext]);
            }
        }
    }
}

void GearShopMenuRenderBackgroundDim(void) {
}

void GearShopMenuRenderStringLayers(void) {
    GearShopMenuRenderStrings4E0();
    GearShopMenuRenderStrings6E0();
    GearShopMenuRenderStringsAE0();
    GearShopMenuRenderStringsDE0();
    GearShopMenuRenderModalStrings();
}

void GearShopMenuRenderSelectionMenu(void) {
    int i;

    if (g_Menu->pManager->shouldRenderSelectionMenu) {
        if (g_Menu->pSelectionMenu->unk1192 != g_Menu->pSelectionMenu->unk1193) {

            // Render the selection menu as disabled
            if (g_Menu->pSelectionMenu->unk1192) {
                for (i = 0; i < g_Menu->pSelectionMenu->numTexts; i++) {
                    SetSemiTrans(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 1);
                    SetShadeTex(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                    g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx].tpage |= 0x20;
                    setRGB0(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 32, 32, 32);
                }

                for (i = 0; i < g_Menu->pSelectionMenu->numCursors; i++) {
                    SetSemiTrans(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 1);
                    SetShadeTex(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                    g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx].tpage |= 0x20;
                    setRGB0(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 32, 32, 32);
                }

            // Render the seleciton menu as active
            } else {
                for (i = 0; i < g_Menu->pSelectionMenu->numTexts; i++) {
                    SetSemiTrans(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                    SetShadeTex(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                    g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx].tpage |= 0x20;
                    setRGB0(&g_Menu->pSelectionMenu->polysTexts[i*2 + g_Menu->pSelectionMenu->textsRenderCtx], 128, 128, 128);
                }

                for (i = 0; i < g_Menu->pSelectionMenu->numCursors; i++) {
                    SetSemiTrans(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                    SetShadeTex(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                    g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx].tpage |= 0x20;
                    setRGB0(&g_Menu->pSelectionMenu->polysCursors[i*2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 128, 128, 128);
                }
            }

            g_Menu->pSelectionMenu->unk1193 = g_Menu->pSelectionMenu->unk1192;
        }

        GearShopMenuRenderString(g_Menu->pSelectionMenu->numTexts, g_Menu->pSelectionMenu->polysTexts, g_Menu->pSelectionMenu->textsRenderCtx);
        GearShopMenuRenderString(g_Menu->pSelectionMenu->numCursors, g_Menu->pSelectionMenu->polysCursors, g_Menu->pSelectionMenu->cursorsRenderCtx);
    }
}

void GearShopMenuRenderSubmenuStrings(void) {
    if (g_Menu->pManager->unkA) {
        GearShopMenuRenderString(g_Menu->unk354->unk1404, g_Menu->unk354->polys500, g_Menu->unk354->unk1409);
        GearShopMenuRenderString(g_Menu->unk354->unk1400, g_Menu->unk354->polys0, g_Menu->unk354->unk1408);
    }
}

void GearShopMenuRenderArrowCursors(void) {
    MenuArrowCursor* pCursor;
    int i;

    for(i = 0; i < MENU_MAX_NUM_ARROW_CURSORS; i++) {
        if (g_Menu->pManager->shouldRenderArrowCursor[i]) {
            pCursor = g_Menu->arrowCursors[i];
            GearShopMenuRenderPolygons(1, pCursor->vertices, pCursor, pCursor->renderContext);
        }
    }
}

void GearShopMenuRender(void) {
    if (g_Menu->shouldDrawMenu) {
        GearShopMenuUpdateWindows();
        GearShopMenuUpdateGearPanelAnimations();
        GearShopMenuRenderPointerCursors();
        GearShopMenuRenderStringLayers();
        GearShopMenuRenderSelectionCursor();
        GearShopMenuRenderArrowCursors();
        GearShopMenuRenderShopUi();
        GearShopMenuRenderShoulderButtonUi();
        GearShopMenuRenderScrollBarHandle();
        GearShopMenuRenderSelectionMenu();
        GearShopMenuRenderSubmenuStrings();
        GearShopMenuRenderWindows();
        GearShopMenuRenderGearPanelStrings();
        GearShopMenuRenderString(1, &g_Menu->menuUnk8->unk80, g_Menu->menuUnk8->unk1EE1);
        GearShopMenuRenderGearModel();
    }
    GearShopMenuRenderBackgroundDim();
}

void GearShopMenuPlaySoundEffect(s32 arg0) {
    if (g_Menu->unk32A) {
        func_80039DB8((g_Menu->unk2E4->sedId << 0x10) | (arg0 & 0xFF), arg0);
    }
}

void GearShopMenuPollInput(void) {
    u8 wasControllerUnplugged;
    u8 isLooping;
    int savedValue;
    u8 input;

    input = MENU_INPUT_IDLE;
    isLooping = TRUE;
    wasControllerUnplugged = FALSE;
    while (isLooping) {
        // Is the main controller not plugged in?
        if (ControllerGetType(0) == CONTROLLER_TYPE_NONE) {
            if (wasControllerUnplugged == 0) {
                wasControllerUnplugged++;
                SoundMuteAllSpuChannels();
                savedValue = D_80059488;
            }
            continue;
        }

        isLooping--;
        if (wasControllerUnplugged) {
            SoundEnableAllSpuChannels();
            D_80059488 = savedValue;
        }
    }

    if (ControllerIsStateStackFull()) {
        ControllerResetState();
    } else {
        while (ControllerPopState()) {
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_RIGHT) {
                input = MENU_INPUT_RIGHT;
                GearShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
                input = MENU_INPUT_DOWN;
                GearShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) {
                input = MENU_INPUT_LEFT;
                GearShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
                input = MENU_INPUT_UP;
                GearShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
                input = MENU_INPUT_CONFIRM;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CROSS) {
                input = MENU_INPUT_BACK;
                GearShopMenuPlaySoundEffect(3);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_SQUARE) {
                input = 6;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_TRIANGLE) {
                input = 7;
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_L1) {
                input = MENU_INPUT_PREV;
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_R1) {
                input = MENU_INPUT_NEXT;
                break;
            }
        }
    }
    g_Menu->input = input;
}

void GearShopMenuComputeTransitionSteps(void) {
    u8 xTotal;
    u8 yTotal;
    int i;

    u8 notDone;

    g_gearShopTransitionState.delta.cameraX = g_gearShopTransitionState.delta.cameraY = g_gearShopTransitionState.delta.gearY = xTotal = yTotal = 0;

    if(g_gearShopTransitionState.target.cameraX >= g_gearShopTransitionState.start.cameraX) {
        g_gearShopTransitionState.step.cameraX = g_gearShopTransitionState.target.cameraX - g_gearShopTransitionState.start.cameraX;
        g_gearShopTransitionState.isCameraPanningLeft = FALSE;
    } else {
        g_gearShopTransitionState.step.cameraX = g_gearShopTransitionState.target.cameraX - g_gearShopTransitionState.start.cameraX;
        g_gearShopTransitionState.isCameraPanningLeft = TRUE;
    }

    if(g_gearShopTransitionState.target.cameraY >= g_gearShopTransitionState.start.cameraY) {
        g_gearShopTransitionState.step.cameraY = g_gearShopTransitionState.target.cameraY - g_gearShopTransitionState.start.cameraY;
        g_gearShopTransitionState.isCameraPanningDown = FALSE;
    } else {
        g_gearShopTransitionState.step.cameraY = g_gearShopTransitionState.target.cameraY - g_gearShopTransitionState.start.cameraY;
        g_gearShopTransitionState.isCameraPanningDown = TRUE;
    }

    if(g_gearShopTransitionState.step.cameraX < 0) {
        g_gearShopTransitionState.step.cameraX = (~g_gearShopTransitionState.step.cameraX)+1;
    }

    if (g_gearShopTransitionState.step.cameraY < 0) {
        g_gearShopTransitionState.step.cameraY = ~g_gearShopTransitionState.step.cameraY + 1;
    }

    if(g_gearShopTransitionState.step.cameraX >= g_gearShopTransitionState.step.cameraY) {

        g_gearShopTransitionState.step.cameraY = (g_gearShopTransitionState.step.cameraY << 16) / g_gearShopTransitionState.step.cameraX;
        g_gearShopTransitionState.step.cameraX = 0x10000;
    } else {

        g_gearShopTransitionState.step.cameraX = (g_gearShopTransitionState.step.cameraX << 16) / g_gearShopTransitionState.step.cameraY;
        g_gearShopTransitionState.step.cameraY = 0x10000;
    }

    notDone = TRUE;
    while(notDone) {
        for(i = 0; i < g_gearShopTransitionState.stepFactor; i++) {
            g_gearShopTransitionState.delta.cameraX += g_gearShopTransitionState.step.cameraX;
        }

        if(g_gearShopTransitionState.isCameraPanningLeft == FALSE) {
            if( (g_gearShopTransitionState.delta.cameraX / 0x10000) + g_gearShopTransitionState.start.cameraX >= g_gearShopTransitionState.target.cameraX ) {
                notDone = FALSE;
            } else {
                xTotal++;
            }
        } else {
            if(g_gearShopTransitionState.target.cameraX >= (g_gearShopTransitionState.start.cameraX - (g_gearShopTransitionState.delta.cameraX / 0x10000))) {
                notDone = FALSE;
            } else {
                xTotal += 1;
            }
        }
    }

    notDone = TRUE;
    while(notDone) {
        for(i = 0; i < g_gearShopTransitionState.stepFactor; i++) {
            g_gearShopTransitionState.delta.cameraY += g_gearShopTransitionState.step.cameraY;
        }

        if(g_gearShopTransitionState.isCameraPanningDown == FALSE) {
            if (((g_gearShopTransitionState.delta.cameraY / 0x10000) + g_gearShopTransitionState.start.cameraY) >= g_gearShopTransitionState.target.cameraY) {
                notDone = FALSE;
            } else {
                yTotal++;
            }
        } else {
            if( g_gearShopTransitionState.target.cameraY >= (g_gearShopTransitionState.start.cameraY - (g_gearShopTransitionState.delta.cameraY / 0x10000) )) {
                notDone = FALSE;
            } else {
                yTotal++;
            }
        }
    }

    if(xTotal < yTotal)
        xTotal = yTotal;

    if (g_gearShopTransitionState.target.gearY >= g_gearShopTransitionState.start.gearY) {
        g_gearShopTransitionState.step.gearY = g_gearShopTransitionState.target.gearY - g_gearShopTransitionState.start.gearY;
        g_gearShopTransitionState.isGearMovingDown = FALSE;
    } else {
        g_gearShopTransitionState.step.gearY = g_gearShopTransitionState.target.gearY - g_gearShopTransitionState.start.gearY;
        g_gearShopTransitionState.isGearMovingDown = TRUE;
    }

    if (g_gearShopTransitionState.step.gearY < 0) {
        g_gearShopTransitionState.step.gearY = ~g_gearShopTransitionState.step.gearY + 1;
    }

    g_gearShopTransitionState.delta.gearY = 0;
    g_gearShopTransitionState.delta.cameraY = 0;
    g_gearShopTransitionState.delta.cameraX = 0;

    g_gearShopTransitionState.step.gearY = (g_gearShopTransitionState.step.gearY << 0x10) / xTotal;
}


/*
 * In GearShopMenu, g_Menu->transitionEffectState is a bitmask to track which directional vectors need to be handled.
 * This function checks for each of these flags, clearing them when they are done processing.
 */

void GearShopMenuUpdateCameraTransition(void) {
    int i;

    if (g_Menu->transitionEffectState & EFFECTSTATE_UPDATE_X) {
        for(i = 0; i < g_gearShopTransitionState.stepFactor; i++) {
            g_gearShopTransitionState.delta.cameraX += g_gearShopTransitionState.step.cameraX;
        }

        if (g_gearShopTransitionState.isCameraPanningLeft == 0) {
            if (g_gearShopTransitionState.delta.cameraX / 0x10000 + g_gearShopTransitionState.start.cameraX < g_gearShopTransitionState.target.cameraX) {
            g_Menu->cameraPosition.vz = g_gearShopTransitionState.delta.cameraX / 0x10000 + g_gearShopTransitionState.start.cameraX;
            } else {
                g_Menu->cameraPosition.vz = g_gearShopTransitionState.target.cameraX;
                g_Menu->transitionEffectState &= EFFECTSTATE_UPDATE_Y|EFFECTSTATE_UPDATE_Z;
            }
        } else {

            if (g_gearShopTransitionState.target.cameraX >= g_gearShopTransitionState.start.cameraX - g_gearShopTransitionState.delta.cameraX / 0x10000) {
                g_Menu->cameraPosition.vz = g_gearShopTransitionState.target.cameraX;
                g_Menu->transitionEffectState &= EFFECTSTATE_UPDATE_Y|EFFECTSTATE_UPDATE_Z;
            } else {
                g_Menu->cameraPosition.vz = g_gearShopTransitionState.start.cameraX - g_gearShopTransitionState.delta.cameraX / 0x10000;
            }
        }
    }
    if (g_Menu->transitionEffectState & EFFECTSTATE_UPDATE_Y) {
        for(i = 0; i < g_gearShopTransitionState.stepFactor; i++) {
            g_gearShopTransitionState.delta.cameraY += g_gearShopTransitionState.step.cameraY;
        }

        if (g_gearShopTransitionState.isCameraPanningDown == 0) {

            if (g_gearShopTransitionState.delta.cameraY / 0x10000 + g_gearShopTransitionState.start.cameraY < g_gearShopTransitionState.target.cameraY) {
                g_Menu->cameraPosition.vy = g_gearShopTransitionState.delta.cameraY / 0x10000 + g_gearShopTransitionState.start.cameraY;
            } else {
                g_Menu->cameraPosition.vy = g_gearShopTransitionState.target.cameraY;
                g_Menu->transitionEffectState &= EFFECTSTATE_UPDATE_X|EFFECTSTATE_UPDATE_Z;
            }
        } else {

            if (g_gearShopTransitionState.target.cameraY >= g_gearShopTransitionState.start.cameraY - g_gearShopTransitionState.delta.cameraY / 0x10000) {
                g_Menu->cameraPosition.vy = g_gearShopTransitionState.target.cameraY;
                g_Menu->transitionEffectState &= EFFECTSTATE_UPDATE_X|EFFECTSTATE_UPDATE_Z;
            } else {
                g_Menu->cameraPosition.vy = g_gearShopTransitionState.start.cameraY - g_gearShopTransitionState.delta.cameraY / 0x10000;
            }
        }
    }
    if (g_Menu->transitionEffectState & EFFECTSTATE_UPDATE_Z) {
        g_gearShopTransitionState.delta.gearY += g_gearShopTransitionState.step.gearY;

        if (g_gearShopTransitionState.isGearMovingDown == 0) {
            if (g_gearShopTransitionState.start.gearY + g_gearShopTransitionState.delta.gearY / 0x10000 < g_gearShopTransitionState.target.gearY) {
                g_LibGearModels[1]->pSkeleton->rotation.vy = g_gearShopTransitionState.delta.gearY / 0x10000 + g_gearShopTransitionState.start.gearY;
            } else {
                g_LibGearModels[1]->pSkeleton->rotation.vy = g_gearShopTransitionState.target.gearY;
                g_Menu->transitionEffectState &= EFFECTSTATE_UPDATE_X|EFFECTSTATE_UPDATE_Y;
            }
        } else {
            if (g_gearShopTransitionState.target.gearY >= g_gearShopTransitionState.start.gearY - g_gearShopTransitionState.delta.gearY / 0x10000) {
                g_LibGearModels[1]->pSkeleton->rotation.vy = g_gearShopTransitionState.target.gearY;
                g_Menu->transitionEffectState &= EFFECTSTATE_UPDATE_X|EFFECTSTATE_UPDATE_Y;
            } else {
                g_LibGearModels[1]->pSkeleton->rotation.vy = g_gearShopTransitionState.start.gearY - g_gearShopTransitionState.delta.gearY / 0x10000;
            }
        }
    }
}


void GearShopMenuUpdateCameraMatrices(void) {
    GearShopMenuUpdateCameraTransition();
    RotMatrix(&g_Menu->unk218, &g_Menu->unk230);
    TransMatrix(&g_Menu->unk230, &g_Menu->cameraPosition);
    SetRotMatrix(& g_Menu->unk230);
    SetTransMatrix(&g_Menu->unk230);
    RotMatrix(&g_Menu->rotation, &g_Menu->matTransform);
    TransMatrix(&g_Menu->matTransform, &g_Menu->translation);
    SetRotMatrix(&g_Menu->matTransform);
    SetTransMatrix(&g_Menu->matTransform);
}
#define LETTER_WIDTH 0x8
#define LETTER_HEIGHT 0x10


// Debug function drawing X, Y, Z, A, G, S values
void GearShopMenuRenderDebugValues(void) {
    int values[6];
    int i;
    int j;
    int digitsOffset;
    int curValue;

    if ((D_801D697C != 0) && ((u8) g_Menu->unk458[1]->unk12 != 0)) {
        // X, Y, Z letters
        func_8002675C(g_Menu->resources, 0x21, &D_801D7108[0], g_Menu->renderContext, 0x10, 0x10, 0x1000U);
        func_8002675C(g_Menu->resources, 0x22, &D_801D7108[2], g_Menu->renderContext, 0x10, 0x20, 0x1000U);
        func_8002675C(g_Menu->resources, 0x23, &D_801D7108[4], g_Menu->renderContext, 0x10, 0x30, 0x1000U);
        // A, G, S letters
        func_8002675C(g_Menu->resources, 0xA, &D_801D7108[6], g_Menu->renderContext, 0x10, 0x40, 0x1000U);
        func_8002675C(g_Menu->resources, 0x10, &D_801D7108[8], g_Menu->renderContext, 0x10, 0x50, 0x1000U);
        func_8002675C(g_Menu->resources, 0x1C, &D_801D7108[10], g_Menu->renderContext, 0x10, 0x60, 0x1000U);

        func_8002675C(g_Menu->resources, MENU_TEX_PLUS, &D_801D7108[12], g_Menu->renderContext, 0xA0, 0x64, 0x1000U);
        values[0] = g_Menu->cameraPosition.vx;
        values[1] = g_Menu->cameraPosition.vy;
        values[2] = g_Menu->cameraPosition.vz;
        values[3] = g_LibGearModels[1]->pSkeleton->rotation.vy;
        values[4] = g_LibGearModels[1]->ground;
        values[5] = g_LibGearModels[1]->scale;

        D_801D9048 = 7; // poly count

        // There's 6 values to process in total (X, Y, Z, A, G, S)
        for(i = 0; i < 6; i++) {
            curValue = values[i];
            digitsOffset = 0;

            // Negative value, add a minus symbol to it first
            if (curValue < 0) {
                digitsOffset = 1;
                D_801D9048 += func_8002675C(
                    g_Menu->resources,
                    MENU_TEX_MINUS,
                    &D_801D7108[D_801D9048 * 2],
                    g_Menu->renderContext,
                    0x30, LETTER_HEIGHT * (i + 1),
                    0x1000
                );
                curValue = ~values[i] + 1;
            }
            // Parse value to string and create primitives for it
            GearShopMenuParseNumberToString(curValue);
           //;
            for (j = 0; j < 9; j++) {
                if (g_Menu->digits[j] != 0xFF) {
                    D_801D9048 += func_8002675C(
                        g_Menu->resources,
                        g_Menu->digits[j],
                        &D_801D7108[D_801D9048 * 2],
                        g_Menu->renderContext,
                        (LETTER_WIDTH * digitsOffset++) + 0x30,
                        LETTER_HEIGHT * (i + 1),
                        0x1000
                    );
                }
            }
        }
        // Render the values
        GearShopMenuRenderString(D_801D9048, &D_801D7108[0], g_Menu->renderContext);
    }
}

void GearShopMenuUpdateAndRender(void) {
    if (*D_8005917C != -1) {
        asm("break 0x400");
    }

    GearShopMenuPollInput();
    GameCheckAndHandleSoftReset();

    g_Menu->pGfxEnv = (g_Menu->pGfxEnv == g_Menu->gfxEnvs) ? &g_Menu->gfxEnvs[1] : g_Menu->gfxEnvs;
    g_Menu->renderContext = g_Menu->renderContext == 0;
    ClearOTagR((u32 *)g_Menu->pGfxEnv->ot, 0x10);
    ClearOTagR((u32* )g_Menu->pGfxEnv->unkB0, 0x400);
    GearShopMenuUpdateCameraMatrices();
    GearShopMenuRenderDebugValues();
    GearShopMenuRender();
    DrawSync(0);
    Vsync(0);

    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    ClearImage(&g_Menu->pGfxEnv->drawEnv.clip, 0U, 0U, 0U);
    AddPrims((void* ) g_Menu->pGfxEnv->unkB0, &g_Menu->pGfxEnv->ot[0xF], g_Menu->pGfxEnv->ot);
    DrawOTag((u32 *)(g_Menu->pGfxEnv->unkB0 + 0xFFC));
}

void GearShopMenuInitializePointerCursors(u8 mode) {
    int i;

    g_Menu->pCursors = HeapAlloc(sizeof(MenuPointerCursors), 0);
    bzero((u8 *)g_Menu->pCursors, sizeof(MenuPointerCursors));

    switch(mode) {
        case 0:
            g_Menu->pManager->shouldRenderCursors = TRUE;
            g_Menu->pCursors->unk144[0] = TRUE;
            g_Menu->pCursors->unk144[1] = TRUE;
        case 2:
            for(i = 0; i < 4; i++) {
                func_8002675C(
                    g_Menu->resources,
                    0x108,
                    &g_Menu->pCursors->polysCursor[i*2],
                    g_Menu->renderContext,
                    D_801D6A60[i],
                    D_801D6A70[i],
                    0x800
                );
                g_Menu->pCursors->renderContexts[i] = g_Menu->renderContext;
            }
            break;
        case 3:
            func_8002675C(
                g_Menu->resources,
                0x108,
                &g_Menu->pCursors->polysCursor[0],
                g_Menu->renderContext, 0, 0, 0x800
            );
            g_Menu->pCursors->renderContexts[0] = g_Menu->renderContext;
            g_Menu->pManager->shouldRenderCursors = TRUE;
        case 1:
            break;
    }
}

void GearShopMenuFreePointerCursors(void) {
    g_Menu->pManager->shouldRenderCursors = 0;
    GearShopMenuUpdateAndRender();
    HeapFree(g_Menu->pCursors);
}

void func_801CC520(void) {
}

void func_801CC528(void) {
}

void GearShopMenuConfirmationWindowInitialize(u8 stringIndex) {
    MenuWindowParameters* pWindowParams;
    MenuString* pString;
    int i;
    int xPosition;

    xPosition = 0x50;

    GearShopMenuInitializeWindow(4, 0x42, 0x46, 0xC0, 0x40, 1, 1, 4, 0);
    pWindowParams = g_Menu->windowParameters[4];
    while (!pWindowParams->unk11) {
        GearShopMenuUpdateAndRender();
    }

    for (i = 0; i < 4; i++) {
        g_Menu->unk1DE0[i] = HeapAlloc(sizeof(MenuString), 0);
        bzero((u8 *)g_Menu->unk1DE0[i], sizeof(MenuString));

        // Even indices
        if (!(i & 1)) {
            // Buffer size = FONT_LETTER_HEIGHT * 57 * 2
            g_Menu->unk1DE0[i]->pVramBuffer = HeapAlloc(0x5CA, 0);
            g_Menu->unk1DE0[i]->vramDest.x = 320;
            g_Menu->unk1DE0[i]->vramDest.y = ((i / 2) * FONT_LETTER_HEIGHT) + 78;
            g_Menu->unk1DE0[i]->vramDest.w = 58;
            g_Menu->unk1DE0[i]->vramDest.h = FONT_LETTER_HEIGHT;
            continue;
        }

        // Odd indices, share VRAM buffer w/ prev entry
        g_Menu->unk1DE0[i]->pVramBuffer = g_Menu->unk1DE0[i - 1]->pVramBuffer;
    }

    // Render strings to text areas
    for (i = 0; i < 3; i++) {
        pString = g_Menu->unk1DE0[i];
        pString->width = SystemRenderStringEntry(
            GetStringEntry(g_Menu->unk2E0, stringIndex + i),
            pString->pVramBuffer, 0x36, i % 2
        );
        GearShopMenuInitializeMenuStringPolys(pString, i, 0, 0);
        GearShopMenuSetVertices(
            pString->vertices,
            xPosition,
            (i * 16) + 0x50,
            pString->width,
            FONT_LETTER_HEIGHT
        );
        setUV4(
            &pString->polys[g_Menu->renderContext],
            0,                (i / 2) * FONT_LETTER_HEIGHT + 0x4E,
            pString->width,   (i / 2) * FONT_LETTER_HEIGHT + 0x4E,
            0,                (i / 2) * FONT_LETTER_HEIGHT + 0x5B,
            pString->width,   (i / 2) * FONT_LETTER_HEIGHT + 0x5B
        );
        pString->renderContext = g_Menu->renderContext;
        pString->unk7F = 1;
    }

    // Transfer text areas to VRAM
    LoadImage(&g_Menu->unk1DE0[0]->vramDest, g_Menu->unk1DE0[0]->pVramBuffer);
    LoadImage(&g_Menu->unk1DE0[2]->vramDest, g_Menu->unk1DE0[2]->pVramBuffer);
    DrawSync(0);

    g_Menu->pManager->unk2B = 1;
    HeapFree(g_Menu->unk1DE0[0]->pVramBuffer);
    HeapFree(g_Menu->unk1DE0[2]->pVramBuffer);
    if (g_Menu->pManager->unk5B == 2) {
        g_Menu->pManager->unk5B = 1;
    }
    GearShopMenuUpdateAndRender();
    GearShopMenuUpdateAndRender();
}

void GearShopMenuConfirmationWindowFree(void) {
    int i;

    if (g_Menu->pManager->shouldRenderWindow[4]) {
        GearShopMenuFreeWindow(4);

        g_Menu->pManager->unk2B = 0;

        for(i = 0; i < 4; i++) {
            HeapFree(g_Menu->unk1DE0[i]);
        }
    }
    g_Menu->pManager->unk5B = 0;
    GearShopMenuUpdateAndRender();
}

u_char GearShopMenuConfirmationWindowGetChoice(u_char isConfirmationPrompt, u8 isYesNo) {
    u_char choice;
    u_char isRunning;
    u_char curTimer;

    isRunning = TRUE;
    choice = MENU_CHOICE_NO;
    curTimer = 60;

    while (isRunning) {
        if (!isConfirmationPrompt) {
            g_Menu->pCursors->shouldRender[2] = FALSE;
            g_Menu->pCursors->shouldRender[3] = FALSE;

            // Was any button pressed?
            if (g_Menu->input != MENU_INPUT_IDLE)
                break;

            curTimer--;
            if (curTimer == 0)
                break;
        }
        
        GearShopMenuUpdateAndRender();
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                GearShopMenuPlaySoundEffect(2);
                isRunning = FALSE;
                break;
            case MENU_INPUT_BACK:
                choice = MENU_CHOICE_NO;
                isRunning = FALSE;
                break;
            case MENU_INPUT_LEFT:
                if (isYesNo == 0)
                    break;
                g_Menu->pCursors->shouldRender[2] = TRUE;
                g_Menu->pCursors->shouldRender[3] = FALSE;
                choice = MENU_CHOICE_YES;
                break;
            case MENU_INPUT_RIGHT:
                if (isYesNo == 0)
                    break;
                g_Menu->pCursors->shouldRender[2] = FALSE;
                g_Menu->pCursors->shouldRender[3] = TRUE;
                choice = MENU_CHOICE_NO;
                break;
        }
    }
    g_Menu->pCursors->shouldRender[2] = FALSE;
    g_Menu->pCursors->shouldRender[3] = FALSE;

    POSSIBLE_DEBUG_CODE;
    
    return choice;
}

#define STRING_ID_NONE 0xFF

/*
 * Display a modal yes/no dialog
 *
 * firstLine: string ID of the contents of the modal (required)
 * secondLine: an optional second line of text, set to STRING_ID_NONE to disable
 * isConfirmationPrompt: if true, presents and handles yes/no prompts. if false, the modal is presented
 * for up to 1 second and dismisses on any keypress.
 *
 * If secondLine is not STRING_ID_NONE, the first line of text will be displayed as a simple "OK" modal, after
 * which the yes/no prompt will be presented with the contents of `secondLine`.
 * returns 1 for yes, 0 for no.
 */

s32 GearShopMenuShowModal(u8 firstLine, s32 secondLine, s32 isConfirmationPrompt) {
    u8 isYesNo = TRUE;
    u8 result;

    GearShopMenuConfirmationWindowInitialize(firstLine);
    if ((u8)secondLine == STRING_ID_NONE) {
        g_Menu->pCursors->shouldRender[3] = TRUE;
    } else {
        g_Menu->pCursors->shouldRender[2] = FALSE;
        isYesNo = FALSE;
        g_Menu->pCursors->shouldRender[3] = FALSE;
    }

    result = GearShopMenuConfirmationWindowGetChoice(isConfirmationPrompt, isYesNo);
    GearShopMenuConfirmationWindowFree();

    if ((u8)secondLine != STRING_ID_NONE) {
        g_Menu->pCursors->shouldRender[3] = TRUE;
        GearShopMenuConfirmationWindowInitialize(secondLine);
        g_Menu->pCursors->shouldRender[3] = TRUE;
        result = GearShopMenuConfirmationWindowGetChoice(isConfirmationPrompt, TRUE);
        GearShopMenuConfirmationWindowFree();
    }
    return result;
}

void GearShopMenuFree(void) {
    GearShopMenuUpdateAndRender();
    GearShopMenuUpdateAndRender();
    g_Menu->shouldDrawMenu = FALSE;
    GearShopMenuUpdateAndRender();
    do {
        GearShopMenuUpdateAndRender();
    } while (g_Menu->renderContext);
    GearShopMenuMenuUnk2Manager(MENU_DATA_FREE);
    GearShopMenuMenuManagerManager(MENU_DATA_FREE);
    GearShopMenuSelectionMenuManager(MENU_DATA_FREE);
    GearShopMenuMenuUnk5Manager(MENU_DATA_FREE);
    GearShopMenuDressingRoomManager(MENU_DATA_FREE);
    GearShopMenuMenuUnk1Manager(MENU_DATA_FREE);
    GearShopMenuShopDataManager(SHOP_DATA_FREE);
    GearShopMenuMenuShopManager(MENU_DATA_FREE);
    GearShopMenuMenuUnk8Manager(MENU_DATA_FREE);
    HeapFree(g_Menu->resources);
    HeapFree(g_Menu->unk2E0);
    HeapFree(g_Menu->unk4E0[0].pVramBuffer);
    if (g_MenuDebugEnabled) {
        func_8003A094(g_Menu->unk2E4);
        GearShopMenuUpdateAndRender();
        func_8003852C(g_Menu->unk2E4);
        GearShopMenuUpdateAndRender();
        HeapFree(g_Menu->unk2E4);
    }
    GearShopMenuMenuUnk7Manager(MENU_DATA_FREE);
    HeapFree(g_Menu);
}

void GearShopMenuRenderMenuStringsToVram(s32 count, MenuString *menuString, s8 *arg2, s8 *unused) {
    func_801C5EE8(menuString, arg2, 2, (u8)count);
}

void GearShopMenuClearStringRenderFlags(u8 count, s8* buffer) {
    int i;

    for(i = 0; i < count; i++) {
        buffer[i] = 0;
    }
}

void GearShopMenuSetMenuStringPosition(s32 arg0, MenuString *pStringList, s32 __unused, int* pPaddingOffsets, u8 *pUnkByteArray, u8 choice, u8 scrollOffset, u8 unkBoolean) {
    switch(unkBoolean) {
        case FALSE:
            GearShopMenuClearStringRenderFlags(arg0, pUnkByteArray);

            setXY4(&pStringList[choice].polys[g_Menu->renderContext],
                (D_801D6BFC[scrollOffset + choice]) + (22 + pPaddingOffsets[choice]), // x0
                D_801D6C20[scrollOffset + choice] - 34, // y0

                pStringList[choice].width + ((D_801D6BFC[scrollOffset + choice]) + (22 + pPaddingOffsets[choice])), // x1
                D_801D6C20[scrollOffset + choice] - 34, // y1

                (D_801D6BFC[scrollOffset + choice]) + (22 + pPaddingOffsets[choice]), // x2
                D_801D6C20[scrollOffset + choice] - 21, // y2

                pStringList[choice].width + ((D_801D6BFC[scrollOffset + choice]) + (22 + pPaddingOffsets[choice])), // x3
                D_801D6C20[scrollOffset + choice] - 21 // y3
            );
            break;
        case TRUE:

            if(choice) {
                setXY4(&pStringList[choice].polys[g_Menu->renderContext],
                       206, 126,
                       pStringList[0].width + 206, 126,
                       206, 139,
                       pStringList[0].width + 206, 139
                );
            } else {
                setXY4(&pStringList[choice].polys[g_Menu->renderContext],
                       236, 126,
                       pStringList[choice].width + 236, 126,
                       236, 139,
                       pStringList[choice].width + 236, 139
                );
            }
            break;

    }
    pStringList[choice].renderContext = g_Menu->renderContext;
    pUnkByteArray[choice] = 1;
}

void GearShopMenuInitializeShopModeSelectionMenu(int count, int* pResourceIDs) {
    int i, j;

    g_Menu->pSelectionMenu->unk1192 = FALSE;
    g_Menu->pSelectionMenu->unk1193 = FALSE;

    g_Menu->pManager->shouldRenderSelectionMenu = TRUE;

    for (i = 1; i <= count; i++) {
        if (i != count) {
            g_Menu->pSelectionMenu->numCursors = 0;
            for(j = 0; j < i; j++) {
                g_Menu->pSelectionMenu->numCursors += func_8002675C(
                    g_Menu->resources,
                    pResourceIDs[j * 2],
                    &g_Menu->pSelectionMenu->polysCursors[g_Menu->pSelectionMenu->numCursors*2],
                    g_Menu->renderContext,
                    0xA0,
                    0x96,
                    0x1000
                );
            }
            g_Menu->pSelectionMenu->cursorsRenderCtx = g_Menu->renderContext;
        }

        g_Menu->pSelectionMenu->numTexts = 0;

        if (i != 1) {
            for (j = 0; j < i - 1; j++) {
                g_Menu->pSelectionMenu->numTexts += func_8002675C(
                    g_Menu->resources,
                    pResourceIDs[j * 2 + 1],
                    &g_Menu->pSelectionMenu->polysTexts[g_Menu->pSelectionMenu->numTexts*2],
                    g_Menu->renderContext,
                    0xA0,
                    0x96,
                    0x1000
                );
            }
            g_Menu->pSelectionMenu->textsRenderCtx = g_Menu->renderContext;
        }

        for(j = 0; j < 2; j++) {
            GearShopMenuUpdateAndRender();
        }
    }
}

void GearShopMenuInitializeSubmenu(u_char offset) {
    int i;
    int j;
    int resourceID; 
    int shouldUpdateAndRender;
    
    shouldUpdateAndRender = TRUE;
    
    g_Menu->pSelectionMenu->unk1192 = FALSE;
    g_Menu->pSelectionMenu->unk1193 = FALSE;
    
    g_Menu->unk354->unk1400 = 0;
    g_Menu->unk354->unk1404 = 0;
    
    g_Menu->pManager->unkA = TRUE;
    
    for (i = 1; i < MAX_NUM_SUBMENU_OPTIONS + 1; i++) {
        g_Menu->unk354->unk1400 = 0;
        g_Menu->subMenuNumChoices = 0;
        
        for (j = 0; j < i; j++) {
            if (D_801D69A0[((offset + g_Menu->mainMenuChoice) * 8) + (j * 2)] != MENU_RESOURCE_NONE) {
                g_Menu->unk354->unk1400 += func_8002675C(
                    g_Menu->resources, 
                    D_801D69A0[((offset + g_Menu->mainMenuChoice) * 8) + (j * 2)],
                    &g_Menu->unk354->polys0[g_Menu->unk354->unk1400 * 2], 
                    g_Menu->renderContext,
                    0xA0, 0x96,
                    0x1000
                );
                g_Menu->subMenuNumChoices++;
            } else {
                shouldUpdateAndRender = FALSE;
            }
        }
        
        g_Menu->unk354->unk1408 = g_Menu->renderContext;
        if (shouldUpdateAndRender) {
            for (j = 0; j < 2; j++) {
                GearShopMenuUpdateAndRender();
            }
        }
        
        g_Menu->unk354->unk1404 = 0;
        for (j = 0; j < i; j++) {
            if (D_801D69A0[((offset + g_Menu->mainMenuChoice) * 8) + j * 2] != MENU_RESOURCE_NONE) {
                g_Menu->unk354->unk1404 += func_8002675C(
                    g_Menu->resources, 
                    D_801D69A0[((offset + g_Menu->mainMenuChoice) * 8) + j * 2 + 1],
                    &g_Menu->unk354->polys500[g_Menu->unk354->unk1404 * 2],
                    g_Menu->renderContext,
                    0xA0, 0x96, 
                    0x1000
                );
            }
        }
        
        g_Menu->unk354->unk1409 = g_Menu->renderContext;
        if (shouldUpdateAndRender) {
            for (j = 0; j < 2; j++) {
                GearShopMenuUpdateAndRender();
            }
        }
    }
}

void GearShopMenuUpdateSelectionMenuTextures(u8 count, u8 currentChoice, s32* pTextureIds) {
    s32 i;
    s32* pTexturePair;
    int cursorTextureId;

    g_Menu->pSelectionMenu->numCursors = 0;
    g_Menu->pSelectionMenu->numTexts = 0;

    for(i = 0; i < count; i++) {
        pTexturePair = &pTextureIds[i*2];

        if(i == currentChoice) {
            cursorTextureId = FONT_LETTER_HEIGHT + pTexturePair[TEX_PAIR_CURSOR];
        } else {
            cursorTextureId = pTexturePair[TEX_PAIR_CURSOR];
        }

        g_Menu->pSelectionMenu->numCursors += func_8002675C(g_Menu->resources,
                                                            cursorTextureId,
                                                            &g_Menu->pSelectionMenu->polysCursors[g_Menu->pSelectionMenu->numCursors * 2],
                                                            g_Menu->renderContext,
                                                            0xA0,
                                                            0x96,
                                                            0x1000
                                                           );
        g_Menu->pSelectionMenu->numTexts += func_8002675C(g_Menu->resources,
                                                          pTexturePair[TEX_PAIR_TEXT],
                                                          &g_Menu->pSelectionMenu->polysTexts[g_Menu->pSelectionMenu->numTexts * 2],
                                                          g_Menu->renderContext,
                                                          0xA0,
                                                          0x96,
                                                          0x1000
                                                         );
    }
    g_Menu->pSelectionMenu->cursorsRenderCtx = g_Menu->renderContext;
    g_Menu->pSelectionMenu->textsRenderCtx = g_Menu->renderContext;
    GearShopMenuSetSelectionCursor(currentChoice, 1);
    g_Menu->pManager->unk4 = 1;
}

void GearShopMenuUpdateSubmenuTextures(u_char offset) {
    int i;
    int index;

    g_Menu->unk354->unk1400 = 0;
    g_Menu->unk354->unk1404 = 0;

    for(i = 0; i < g_Menu->subMenuNumChoices; i++) {
        // The ball cursor for the menu option
        if(i == g_Menu->subMenuChoice) {
            // Active / selected variant
            index = D_801D69A0[((offset + g_Menu->mainMenuChoice) * 8) + (i * 2)] + 0xD;
        } else {
            // Inactive / non-selected variant
            index = D_801D69A0[((offset + g_Menu->mainMenuChoice) * 8) + (i * 2)];
        }

        g_Menu->unk354->unk1400 += func_8002675C(
            g_Menu->resources,
            index,
            &g_Menu->unk354->polys0[g_Menu->unk354->unk1400 * 2],
            g_Menu->renderContext,
            0xA0,
            0x96,
            0x1000
        );

        // The string texture for the menu option
        g_Menu->unk354->unk1404 += func_8002675C(
            g_Menu->resources,
            D_801D69A4[((offset + g_Menu->mainMenuChoice) * 8) + (i * 2)],
            &g_Menu->unk354->polys500[g_Menu->unk354->unk1404*2],
            g_Menu->renderContext,
            0xA0,
            0x96,
            0x1000
        );
    }

    g_Menu->unk354->unk1408 = g_Menu->renderContext;
    g_Menu->unk354->unk1409 = g_Menu->renderContext;
    GearShopMenuSetSelectionCursor(g_Menu->subMenuChoice + 4, 1);
    g_Menu->pManager->unk4 = TRUE;
}

u_char GearShopMenuShopModeMenuHandleSelectedOption(void) {
    u_char isRunning;
    u_char selection;

    isRunning = TRUE;
    if (g_Menu->mainMenuChoice) {
        // Always returns true?
        selection = GearShopMenuShopModeSubmenu();
    } else {
        // "Exit" option was selected
        isRunning = FALSE;
    }

    // intentionally using selection w/o intialization
    if (selection) {
        func_801CC528();
        GearShopMenuRenderMenuStringsToVram(4, g_Menu->unk6E0, &D_801D6A20[0], g_Menu->pManager->unkC);
    }

    // Render selection menu as active agian
    g_Menu->pSelectionMenu->unk1192 = FALSE;
    g_Menu->pSelectionMenu->unk1193 = TRUE;

    g_Menu->pManager->unk4 = TRUE;
    g_Menu->pManager->unk3 = TRUE;
    g_Menu->mainMenuPrevChoice = 0xFF;
    g_Menu->pManager->unkA = FALSE;
    
    return isRunning;
}

// Tune up / Buy / Sell / Exit menu
void GearShopMenuShopModeMain(void) {
    u_char isRunning;

    isRunning = TRUE;
    GearShopMenuCreateGearModel(1, g_gearShopCurrentGearId);
    g_Menu->pManager->unk5C[7] = 1;

    // Options here are "Tune up", "Buy", "Sell", "Exit"
    g_Menu->mainMenuChoice = 2; // Start cursor at "Buy" option
    GearShopMenuInitializeShopModeSelectionMenu(5, &D_801D6980);

    GearShopMenuRenderMenuStringsToVram(4, g_Menu->unk6E0, &D_801D6A20[0], g_Menu->pManager->unkC);

    if (g_gearShopAvailableCharacterCount >= 2) {
        g_Menu->pManager->shoulderButtonUiActive = TRUE;
    }
    
    while (isRunning) {
        GearShopMenuUpdateAndRender();
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                GearShopMenuPlaySoundEffect(2);
                
                g_Menu->pSelectionMenu->unk1192 = TRUE;
                
                GearShopMenuResetSelectionCursor();
                GearShopMenuClearStringRenderFlags(4, g_Menu->pManager->unkC);
                g_Menu->unk348->unk15B = 0x4C;
                isRunning = GearShopMenuShopModeMenuHandleSelectedOption();
                g_Menu->unk348->unk15B = 0x40;
                break;
                
            case MENU_INPUT_BACK:
                isRunning = FALSE;
                break;
                
            case MENU_INPUT_DOWN:
                if (g_Menu->mainMenuChoice) {
                    g_Menu->mainMenuChoice--;
                } else {
                    g_Menu->mainMenuChoice = 3;
                }
                break;
                
            case MENU_INPUT_UP:
                if (++g_Menu->mainMenuChoice >= 4) {
                    g_Menu->mainMenuChoice = 0;
                }
                break;

            case MENU_INPUT_NEXT:
                GearShopMenuChangeCurrentGear(MENU_CYCLE_TO_NEXT);
                break;
                
            case MENU_INPUT_PREV:
                GearShopMenuChangeCurrentGear(MENU_CYCLE_TO_PREV);
                break;
        }
        
        if (g_Menu->mainMenuChoice != g_Menu->mainMenuPrevChoice) {
            GearShopMenuUpdateSelectionMenuTextures(4, g_Menu->mainMenuChoice, &D_801D6980);
            GearShopMenuSetMenuStringPosition(4, g_Menu->unk6E0, &D_801D6A20[0], &D_801D6A30, g_Menu->pManager->unkC, g_Menu->mainMenuChoice, 0, 0);
            g_Menu->mainMenuPrevChoice = g_Menu->mainMenuChoice;
        }
    }
    
    D_801D697C = 0;
    GearShopMenuFreeGearView();
    GearShopMenuShoulderButtonUiFree();
}

void GearShopMenuMain(void) {
    GearShopMenuMenuUnk2Manager(MENU_DATA_INITIALIZE);
    GearShopMenuMenuManagerManager(MENU_DATA_INITIALIZE);
    GearShopMenuSelectionMenuManager(MENU_DATA_INITIALIZE);
    GearShopMenuMenuUnk5Manager(MENU_DATA_INITIALIZE);
    GearShopMenuDressingRoomManager(MENU_DATA_INITIALIZE);
    GearShopMenuMenuUnk1Manager(MENU_DATA_INITIALIZE);
    GearShopMenuMenuUnk7Manager(MENU_DATA_INITIALIZE);
    GearShopMenuMenuShopManager(MENU_DATA_INITIALIZE);
    GearShopMenuMenuUnk8Manager(MENU_DATA_INITIALIZE);
    g_Menu->pSelectionMenu->unk1180.x = 0x2C0;
    g_Menu->pSelectionMenu->unk1180.y = 0x100;
    g_Menu->pSelectionMenu->unk1180.w = 0x140;
    g_Menu->pSelectionMenu->unk1180.h = 0xE0;
    g_Menu->unk348->unk15B = 0x40;
    g_Menu->translation.vz = 0x200;
    g_Menu->rotation.vz = 0;
    g_Menu->rotation.vx = 0;
    g_Menu->rotation.vy = 0;
    g_Menu->cameraPosition.vz = 0x400;
    g_Menu->unk218.vz = 0;
    g_Menu->unk218.vx = 0;
    g_Menu->unk218.vy = 0x400;
    g_Menu->transitionEffectState = MENU_ANIMATION_DONE;
    g_gearShopTransitionState.start.gearY = -0x400;
    g_gearShopTransitionState.target.gearY = -0x400;
    g_gearShopTransitionState.start.cameraX = 0x400;
    g_gearShopTransitionState.start.cameraY = 0;
    g_gearShopTransitionState.target.cameraX = 0x400;
    g_gearShopTransitionState.target.cameraY = 0;
    g_gearShopTransitionState.stepFactor = 0x10;
    GearShopMenuFilterPartyMembers();
    GearShopMenuResetRenderContext();
    GearShopMenuInitializeMenuStrings(); // initMenuGeometry()?
    GearShopMenuInitializeBackgrounds();
    GearShopMenuInitializeWindowBorders();
    GearShopMenuLoadGearShopInventory();
    GearShopMenuInitializeGearView();
    g_Menu->pShoulderButtonUi = HeapAlloc(sizeof(MenuShoulderButtonUi), 0);
    bzero(g_Menu->pShoulderButtonUi, sizeof(MenuShoulderButtonUi));
    GearShopMenuShoulderButtonUiInitialize();
    g_Menu->shouldDrawMenu = TRUE;
    g_Menu->unk32A = TRUE;
    GearShopMenuShopModeMain();
    GearShopMenuFree();
}