#include "common.h"

#include "psyq/libgpu.h"
#include "system/controller.h"
#include "system/debug.h"
#include "system/math.h"
#include "system/menu.h"
#include "main/game.h"

// Confirmation window choices
#define MENU_CHOICE_NO 0
#define MENU_CHOICE_YES 1

// Shop mode choices
#define MENU_CHOICE_EXIT 0x0
#define MENU_CHOICE_SELL 0x1
#define MENU_CHOICE_BUY 0x2

// Sell mode menu choice
#define MENU_CHOCIE_EQUIPMENT 0x0
#define MENU_CHOICE_ACCESSORIES 0x1
#define MENU_CHOICE_WEAPONS 0x2
#define MENU_CHOICE_ITEMS 0x3

#define MENU_STAT_CHANGE_INCREASE 0x0
#define MENU_STAT_CHANGE_DECREASE 0x1

#define SHOP_DATA_INITIALIZE 0x0
#define SHOP_DATA_FREE 0x10

// Selection modes
#define MENU_AUTO_ADVANCE 0
#define MENU_MANUAL_CHOICE 0xFF

// ShopMenuSellEquipmentMenu exit codes
#define MENU_SELL_EQUIPMENT_CANCEL 0x0
#define MENU_SELL_EQUIPMENT_CONFIRM 0x1
#define MENU_SELL_EQUIPMENT_CONTINUE 0x2

// Debug-related?
extern s32* g_DebuggerState;

extern u32* g_MenuResourcesFile; // Pointer to the overlay's resources file
// .rodata: `const` matters - a non-const array lands in .data and leaves the
// switch jump tables as the only .rodata, which shifts the whole unit.
const u8 g_StrGameSerial[0x10] = {0x42, 0x49, 0x53, 0x4C, 0x50, 0x53, 0x2D, 0x30,
                             0x30, 0x38, 0x30, 0x30, 0x00, 0x00, 0x03, 0x00};
extern SoundFile* g_MenuSedsFile; // SEDS file pointer

extern int g_ShopItemCount; // Number of items
extern s32 g_ShopSubmenuStringIds[];
extern s32 g_ShopBuyMenuStringIds[];
extern s32 g_ShopSubmenuStringOffsets[];
extern u16 g_SelectedItemQuantityHeld;


extern s32 g_ShopModeSelectionTextureIds[];
// = {
//    MENU_TEX_BALL_CURSOR_1, MENU_TEX_STRING_BUY,
//    MENU_TEX_BALL_CURSOR_2, MENU_TEX_STRING_SELL,
//    MENU_TEX_BALL_CURSOR_3, MENU_TEX_STRING_EXIT
//    }

extern s32 g_ShopModeMenuStringIds[];
extern s32 g_ShopModeMenuStringOffsets[];

extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

extern u_char g_ShopMenuStringIds[]; // String IDs of the shop menu strings
extern int g_ShopSubmenuTextureIds[]; // Cursor textures, 4 per submenu
extern int g_ShopSubmenuTextTextureIds[]; // Text textures, 4 per submenu

extern u_short g_CharacterBitMasks[];

extern s32 g_ShopModeMenuStringIds[];

extern s32 g_GoldAmountTextX;
extern s32 g_GoldAmountTextY;
extern s32 g_TotalPriceTextX;
extern s32 g_TotalPriceTextY;
extern s32 g_NewGoldAmountTextX;
extern s32 g_NewGoldAmountTextY;

// Character portrait X positions
extern int g_CharacterPortraitX[];

// Cursor stuff
extern int g_PointerCursorBoxSlot[];
extern int g_PointerCursorBoxX[];
extern int g_PointerCursorBoxY[];

extern int g_PointerCursorInitX[];
extern int g_PointerCursorInitY[];

// Argument passed to the menu by the outside.
// For the shop menu, this is expected to be the index
// of the shop to load and run.
extern u8 g_ShopIndex;

extern s32 g_ShopItemCount;


extern int g_ShopPointerCursorX[];
extern int g_ShopPointerCursorY[];

// Explanations stuff
extern u_char g_BuyMenuExplanationTextureIds[]; // Buy menu explanation textures
// = { MENU_TEX_QUANTITY_EXPLANATION,
//     MENU_TEX_STRING_ATTACK,
//     MENU_TEX_STRING_DEFENSE,
//     MENU_TEX_MINUS
//    }

extern u_char g_SellMenuExplanationTextureIds[]; // Sell menu explanation textures
// = { MENU_TEX_QUANTITY_EXPLANATION, MENU_TEX_PLUS }

extern int g_BuyMenuExplanationX[]; // Buy menu explanation graphics X positions
extern int g_SellMenuExplanationX[]; // Sell menu explanation graphics X positions
extern int g_BuyMenuExplanationY[]; // Buy menu explanation graphics Y positions
extern int g_SellMenuExplanationY[]; // Sell menu explanation graphics Y positions


void ShopMenuSetPolyFT4Coords(POLY_FT4* pPoly, short x, short y, u_char u, u_char v, short width, short height) {
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

s32 ShopMenuIsCharacterFlagSet(u_short value, s32 maskIndex) {
    return g_CharacterBitMasks[maskIndex & 0xFF] & value;
}

u_short ShopMenuGetCharacterBitMask(int maskIndex) {
    return g_CharacterBitMasks[maskIndex & 0xFF];
}

void ShopMenuParseNumberToString(u_int number) {
    int i;
    unsigned int curValue;

    // 10 ** 8
    curValue = 100000000;

    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        g_Menu->digits[i] = number / curValue;
        number %= curValue;
        curValue /= 10;
    }

    for (i = 1; i < MENU_MAX_DIGITS; i++) {
        if (g_Menu->digits[i]) {
            if (g_Menu->digits[i - 1] == 0) {
                g_Menu->digits[i - 1] = 0xFF;
            }
            break;
        }
        g_Menu->digits[i - 1] = 0xFF;
    }
}

void ShopMenuUnk32CManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->menuUnk2 = HeapAlloc(sizeof(MenuUnk2), 0);
        bzero(g_Menu->menuUnk2, sizeof(MenuUnk2));
        return;
    }
    HeapFree(g_Menu->menuUnk2);
}

void ShopMenuSetManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pManager = HeapAlloc(sizeof(MenuManager), 0);
        bzero(g_Menu->pManager, sizeof(MenuManager));
        return;
    }
    HeapFree(g_Menu->pManager);
}

void ShopMenuSelectionMenuManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pSelectionMenu = HeapAlloc(sizeof(MenuSelectionMenu), 0);
        bzero(g_Menu->pSelectionMenu, sizeof(MenuSelectionMenu));
        return;
    }
    HeapFree(g_Menu->pSelectionMenu);
}

void ShopMenuUnk354Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pSubmenuPrims = HeapAlloc(sizeof(MenuUnk5), 0);
        bzero(g_Menu->pSubmenuPrims, sizeof(MenuUnk5));
        return;
    }
    HeapFree(g_Menu->pSubmenuPrims);
}

void ShopMenuDressingRoomManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pDressingRoom = HeapAlloc(sizeof(MenuDressingRoom), 0);
        bzero(g_Menu->pDressingRoom, sizeof(MenuDressingRoom));
        return;
    }
    HeapFree(g_Menu->pDressingRoom);
}

void ShopMenuUnk348Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pCursorPrims = HeapAlloc(sizeof(MenuUnk1), 0);
        bzero(g_Menu->pCursorPrims, sizeof(MenuUnk1));
        return;
    }
    HeapFree(g_Menu->pCursorPrims);
}

void ShopMenuUnk1E20Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->unk1E20 = HeapAlloc(sizeof(MenuUnk7), 0);
        bzero(g_Menu->unk1E20, sizeof(MenuUnk7));
        return;
    }
    HeapFree(g_Menu->unk1E20);
}

void ShopMenuShopManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pShop = HeapAlloc(sizeof(MenuShop), 0);
        bzero(g_Menu->pShop, sizeof(MenuShop));
        return;
    }
    HeapFree(g_Menu->pShop);
}

#define FIELD_TEX_COUNT(_i) ((_i) * 6) + 0
#define FIELD_TEX_PAGE(_i)  ((_i) * 6) + 1
#define FIELD_CLUT_X(_i)    ((_i) * 6) + 2
#define FIELD_CLUT_Y(_i)    ((_i) * 6) + 3
#define FIELD_TPAGE_X(_i)   ((_i) * 6) + 4
#define FIELD_TPAGE_Y(_i)   ((_i) * 6) + 5

// objdiff 100% but the link still differs: converting this function moved the
// unit's .rodata - the 16-byte "BISLPS-00800" blob it reads (g_StrGameSerial) used
// to come from this function's own asm, and cc1 emits the switch jump tables
// before a named C array, so every table lands 0x10 early and the overlay
// shifts. Needs the whole unit rodata moved to C in original order.
void ShopMenuLoadResources(void) {
    TIM_IMAGE tim;
    int metadata[18];
    SoundFile* pSoundFile;

    u32* pFileHandle;
    int i;
    u8 *pData;

    pFileHandle = g_MenuResourcesFile;
    ResolveArchiveEntryPointers(pFileHandle);
    pData = LZSSHeapDecompress((void *)pFileHandle[1], 1);
    OpenTIM((void *)pData);
    ReadTIM(&g_Menu->menuUnk2->tim);
    memcpy(&g_Menu->menuUnk2->gameSerialCode, g_StrGameSerial, 0xD);
    g_Menu->menuUnk2->saveFileMagic[0] = 0x53;
    g_Menu->menuUnk2->saveFileMagic[1] = 0x43;
    g_Menu->menuUnk2->saveIconDisplayFlag = 0x11;
    g_Menu->menuUnk2->saveBlockCount = 1;
    bzero(g_Menu->menuUnk2->saveTitle, 0x5C);

    memmove(g_Menu->menuUnk2->saveIconPalette, (u8* ) g_Menu->menuUnk2->tim.caddr, 0x20);
    memmove(g_Menu->menuUnk2->saveIconImage, (u8* ) g_Menu->menuUnk2->tim.paddr, 0x80);
    HeapFree(pData);
    pData = LZSSHeapDecompress((void *)pFileHandle[2], 1);
    ModelLoadTextureBlock(pData);
    HeapFree(pData);
    g_Menu->resources = LZSSHeapDecompress((void *)pFileHandle[3], 0);
    g_Menu->pStringBank = LZSSHeapDecompress((void *)pFileHandle[4], 0);

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
        g_MenuSedsFile = pSoundFile;
        ArchiveReadFileToBuffer(5, (s32) pSoundFile, 0U, 0x80U);
        ArchiveCdDataSync(0);
        ArchiveSetIndex(0x10, 0);
        SoundAddSedsEntry(g_MenuSedsFile);
    }
    g_Menu->pSoundFile = g_MenuSedsFile;
    g_Menu->pShopEntries = LZSSHeapDecompress((void *)pFileHandle[7], 0);
    HeapFree(pFileHandle);
}

void ShopMenuInitialize(void) {
    int flags;
    int characterID;
    int i;

    g_Menu->mainMenuChoice = 4;
    g_Menu->mainMenuPrevChoice = 0xFF;
    g_Menu->cardCheckTimer = 0x3C;
    g_Menu->unk334 = 0;
    g_Menu->unk335 = 0;

    flags = (g_GameState.characterFlags & g_GameState.FrMask) & 0x7FF;
    for (i = 0; i < 0x10; i++) {
        if ((u_short)ShopMenuIsCharacterFlagSet(flags, (u_char)i)) {
            g_Menu->availableCharacters[i] = TRUE;
        } else {
            g_Menu->availableCharacters[i] = FALSE;
        }
    }

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        characterID = g_GameState.partyMembers[i];
        if (characterID != 0xFF && g_Menu->availableCharacters[characterID]) {
            g_Menu->pManager->currentCharacterIDs[i] = characterID;
        } else {
            g_Menu->pManager->currentCharacterIDs[i] = 0xFF;
        }
    }

    ShopMenuLoadResources();
}

void ShopMenuResetRenderContext(void) {
    g_Menu->renderContext = 0;
}

static inline void MenuSetSystemPalette(void* pPrim, u8 flag) {
    POLY_FT4* pPoly = pPrim;
    pPoly->clut = flag ? g_SystemPalette2 : g_SystemPalette1;
}

void ShopMenuSetStringPrimitives(MenuString* pString, int index, s32 arg2, u32 attributes) {
    s32 var_s1;
    int i;
    POLY_FT4* pPoly;
    u32 attr = attributes;

    for (i = 0; i < 2; i++) {
        pPoly = &pString->polys[i];
        var_s1 = 0;

        SetPolyFT4(pPoly);
        SetSemiTrans(pPoly, 0);
        SetShadeTex(pPoly, 0);
        setRGB0(pPoly, 128, 128, 128);

        if (!(attr & 0xFF)) {
            pString->palette = (index & 1);
            setTPage(pPoly, 0, 0, 320, 0);

            #define texCoordV  (((index + arg2) / 4) * FONT_LETTER_HEIGHT)
            #define texCoordU  (((index / 2) & 1) << 7)
            setUV4(
                pPoly,
                texCoordU,                          texCoordV,
                texCoordU + pString->width,         texCoordV,
                texCoordU,                          texCoordV + FONT_LETTER_HEIGHT,
                texCoordU + pString->width,         texCoordV + FONT_LETTER_HEIGHT
            );
            #undef texCoordV
            #undef texCoordU
        } else {
            if (!(attr & 0x80)) {
                var_s1 = 0x20;
                SetSemiTrans(pPoly, 1);
                setRGB0(pPoly, var_s1, var_s1, var_s1);
            }

            pString->palette = (attr & 0x7F) + 0xFF;
            setTPage(pPoly, 0, 0, 0x180, 0x80);
            pPoly->tpage = var_s1 | pPoly->tpage;

            #define texCoordU ((index & 1) * 0x60)
            #define texCoordV (((index / 2) * FONT_LETTER_HEIGHT) + arg2)
            setUV4(
                pPoly,
                texCoordU,                          texCoordV,
                texCoordU + pString->width,         texCoordV,
                texCoordU,                          texCoordV + FONT_LETTER_HEIGHT,
                texCoordU + pString->width,         texCoordV + FONT_LETTER_HEIGHT
            );
            #undef texCoordV
            #undef texCoordU
        }

        MenuSetSystemPalette(pPoly, pString->palette);
    }

    pString->hasVertices = 0;
}

void ShopMenuRenderStringsToVram(MenuString* pStringList, u8* pStringIdList, s32 yOffset, s32 count) {
    s32 i;
    RECT *pRect;
    MenuString *pString;

    for (i = 0; i < count; i += 2) {
        pStringList[i].width = SystemRenderStringEntry(
            GetStringEntry(g_Menu->pStringBank, pStringIdList[i]),
            (u32*) g_Menu->titleStrings[0].pVramBuffer, 0x18, 0
        );
        pStringList[i + 1].width = SystemRenderStringEntry(
            GetStringEntry(g_Menu->pStringBank, pStringIdList[i + 1]),
            (u32*) g_Menu->titleStrings[0].pVramBuffer, 0x18, 1
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

        ShopMenuSetStringPrimitives(&pStringList[i], i, yOffset, 0);
        ShopMenuSetStringPrimitives(&pStringList[i + 1], i + 1, yOffset, 0);

        LoadImage(pRect, g_Menu->titleStrings[0].pVramBuffer);
        DrawSync(0);
    }
}

void ShopMenuLoadWhiteVramRow(void) {
    RECT rect;
    u_short* pVramData;
    u8 _unused[0x8];

    pVramData = HeapAlloc(0x20, 0);
    bzero(pVramData, 0x20);
    pVramData[1] = 0x7FFF;
    rect.y = 448;
    rect.w = 16;
    rect.x = 0;
    rect.h = 1;
    LoadImage(&rect, pVramData);
    DrawSync(0);
    HeapFree(pVramData);
}

void func_801C5EE8(void) {
    SystemTransferPaletteToVRAM(0, 0x1D1);
    g_Menu->titleStrings[0].pVramBuffer = HeapAlloc(0x38E, 0);
    ShopMenuRenderStringsToVram(g_Menu->titleStrings, g_ShopMenuStringIds, 0, 4);
    ShopMenuLoadWhiteVramRow();
}

void ShopMenuInitializeWindowBorders(void) {
    POLY_FT4 _unusued;

    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_TOP,
        &g_Menu->texCount0,
        &g_Menu->texPage0,
        &g_Menu->clutX0, &g_Menu->clutY0,
        &g_Menu->texPageX0, &g_Menu->texPageY0
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_BOTTOM,
        &g_Menu->texCount1,
        &g_Menu->texPage1,
        &g_Menu->clutX1, &g_Menu->clutY1,
        &g_Menu->texPageX1, &g_Menu->texPageY1
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_LEFT,
        &g_Menu->texCount2,
        &g_Menu->texPage2,
        &g_Menu->clutX2, &g_Menu->clutY2,
        &g_Menu->texPageX2, &g_Menu->texPageY2
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_RIGHT,
        &g_Menu->texCount3,
        &g_Menu->texPage3,
        &g_Menu->clutX3, &g_Menu->clutY3,
        &g_Menu->texPageX3, &g_Menu->texPageY3
    );
}

void ShopMenuMovePointerCursor(int index, u_char arg1) {
    func_8002675C(
        g_Menu->resources, MENU_TEX_POINTER_CURSOR,
        &g_Menu->pCursorPrims->polysPointerCursor, g_Menu->renderContext,
        g_ShopPointerCursorX[index], g_ShopPointerCursorY[index],
        0x1000
    );
    g_Menu->pCursorPrims->cursorRenderContext = g_Menu->renderContext;

    if (arg1) {
        // A yellow -> black shaded rectangle of shadeWidth width,
        // with green lines around it. Seems to be cursor-related,
        // as the pointer cursor is set to same position
        setXY4(
            &g_Menu->pCursorPrims->polyG4s[g_Menu->renderContext],
            g_ShopPointerCursorX[index] + 20, g_ShopPointerCursorY[index] - 36,
            g_ShopPointerCursorX[index] + (g_Menu->pCursorPrims->shadeWidth + 20), g_ShopPointerCursorY[index] - 36,
            g_ShopPointerCursorX[index] + 20, g_ShopPointerCursorY[index] - 20,
            g_ShopPointerCursorX[index] + (g_Menu->pCursorPrims->shadeWidth + 20), g_ShopPointerCursorY[index] - 20
        );

        setXY3(
            &g_Menu->pCursorPrims->lines1[g_Menu->renderContext],
            g_ShopPointerCursorX[index] + 20, g_ShopPointerCursorY[index] - 36,
            g_ShopPointerCursorX[index] + (g_Menu->pCursorPrims->shadeWidth + 20), g_ShopPointerCursorY[index] - 36,
            g_ShopPointerCursorX[index] + (g_Menu->pCursorPrims->shadeWidth + 20), g_ShopPointerCursorY[index] - 20
        );

        setXY3(
            &g_Menu->pCursorPrims->lines2[g_Menu->renderContext],
            g_ShopPointerCursorX[index] + 20, g_ShopPointerCursorY[index] - 36,
            g_ShopPointerCursorX[index] + 20, g_ShopPointerCursorY[index] - 20,
            g_ShopPointerCursorX[index] + (g_Menu->pCursorPrims->shadeWidth + 20), g_ShopPointerCursorY[index] - 20
        );

        g_Menu->pCursorPrims->drawModeRenderContext = g_Menu->renderContext;
        g_Menu->pManager->unk3 = 1;
    }
}

void ShopMenuHidePointerCursor(void) {
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
}

// ShopMenuSetPolyGradientColor
void ShopMenuSetPolyG4Gradient(POLY_G4* pPoly, u_char red, u_char green, u_char blue) {
    SetPolyG4(pPoly);
    setRGB0(pPoly, red, green, blue);
    setRGB1(pPoly, red, green, blue);
    setRGB2(pPoly, 0, 0, 0);
    setRGB3(pPoly, 0, 0, 0);
}

void ShopMenuInitializeBackgrounds(void) {
    RECT rect;
    int i;

    rect.y = 0;
    rect.x = 0;
    rect.h = 256;
    rect.w = 256;

    ShopMenuHidePointerCursor();

    for (i = 0; i < 2; i++) {
        // Yellow -> Black gradient
        ShopMenuSetPolyG4Gradient(&g_Menu->pCursorPrims->polyG4s[i], 128, 128, 0);
        SetSemiTrans(&g_Menu->pCursorPrims->polyG4s[i], 1);

        // Green lines
        SetLineF3(&g_Menu->pCursorPrims->lines1[i]);
        setRGB0(&g_Menu->pCursorPrims->lines1[i], 0, 64, 0);

        SetLineF3(&g_Menu->pCursorPrims->lines2[i]);
        setRGB0(&g_Menu->pCursorPrims->lines2[i], 0, 64, 0);

        // Backgrounds dimming screen
        SetPolyF4(&g_Menu->pCursorPrims->polysDimEffect[i]);
        setXY4(&g_Menu->pCursorPrims->polysDimEffect[i],
            0,   0,
            320, 0,
            0,   224,
            320, 224
        );
        setRGB0(&g_Menu->pCursorPrims->polysDimEffect[i], 128, 128, 128);
        SetSemiTrans(&g_Menu->pCursorPrims->polysDimEffect[i], 1);

        SetDrawMode(&g_Menu->pCursorPrims->drModes1[i], 0, 0, GetTPage(0, 0, 0x140, 128), &rect);
        SetDrawMode(&g_Menu->pCursorPrims->drawModeDimEffect[i], 0, 0, GetTPage(0, 2, 0x180, 0), &rect);
    }
}

void ShopMenuLoadShopItemsData(u_char mode) {
    u_int* pArchive;

    if (mode < SHOP_DATA_FREE) {
        pArchive = HeapAlloc(ArchiveDecodeAlignedSize(2), 1);
        ArchiveReadFileToBuffer(2, pArchive, 0, 0x80);
        ArchiveCdDataSync(0);
        ResolveArchiveEntryPointers(pArchive);
    }

    switch (mode) {
        case SHOP_DATA_INITIALIZE:
            // Shop item definitions
            g_Menu->pDressingRoom->pWeaponsData = LZSSHeapDecompress(pArchive[SHOP_RES_WEAPON_DATA], 0);
            g_Menu->pDressingRoom->pAccessoriesData = LZSSHeapDecompress(pArchive[SHOP_RES_ACCESSORY_DATA], 0);
            g_Menu->pDressingRoom->pItemsData = LZSSHeapDecompress(pArchive[SHOP_RES_ITEM_DATA], 0);

            // Shop item descriptions
            // NOTE: There's something weird going on here, because if we extract and parse the shop resource archive,
            // entry 39 = weapon descriptions, entry 40 = accessory desc and entry 41 = item descriptions
            // Off by one? However, this alone would not explain the weird situation with the indices in the
            // gear shop menu...
            g_Menu->pShop->pWeaponDescriptions = LZSSHeapDecompress(pArchive[40], 0);
            g_Menu->pShop->pAccessoryDescriptions = LZSSHeapDecompress(pArchive[41], 0);
            g_Menu->pShop->pItemDescriptions = LZSSHeapDecompress(pArchive[42], 0);
            break;
        case SHOP_DATA_FREE:
            HeapFree(g_Menu->pDressingRoom->pWeaponsData);
            HeapFree(g_Menu->pDressingRoom->pAccessoriesData);
            HeapFree(g_Menu->pDressingRoom->pItemsData);

            HeapFree(g_Menu->pShop->pWeaponDescriptions);
            HeapFree(g_Menu->pShop->pAccessoryDescriptions);
            HeapFree(g_Menu->pShop->pItemDescriptions);
    }

    if (mode < SHOP_DATA_FREE) {
        HeapFree(pArchive);
    }
}

void ShopMenuInitializeShopData(void) {
    int i;
    int j;
    u_char* pShopItemData;

    j = 0;

    // Data for the items the shop we're loading is carrying
    pShopItemData = &g_Menu->pShopEntries[g_ShopIndex * 0x5C];

    for (i = 0; i < MAX_SHOP_ITEMS; i++) {
        g_Menu->shopItemIDs[i] = 0;
        g_Menu->shopItemTypes[i] = 0;
    }

    // 0x5A ??? Arrays being set are 0x30 sized
    // NOTE: Potentially danger if there's more than 0x30 valid items in the shop item data
    for (i = 0; i < 0x5A; i++) {
        if (pShopItemData[i]) {
            g_Menu->shopItemIDs[j] = pShopItemData[i];
            g_Menu->shopItemTypes[j] = i / 30;
            j++;
        }
    }
    g_ShopItemCount = j;

    ShopMenuLoadShopItemsData(SHOP_DATA_INITIALIZE);

    // Red highlight around character portraits
    for (i = 0; i < 9; i++) {
        for (j = 0; j < 2; j++) {
            SetLineF3(&g_Menu->pShop->linesPortraitHighlight1[i*2 + j]);
            setRGB0(&g_Menu->pShop->linesPortraitHighlight1[i*2 + j], 255, 0, 0);

            SetLineF3(&g_Menu->pShop->linesPortraitHighlight2[i*2 + j]);
            setRGB0(&g_Menu->pShop->linesPortraitHighlight2[i*2 + j], 255, 0, 0);

            setXY3(
                &g_Menu->pShop->linesPortraitHighlight1[i*2 + j],
                g_CharacterPortraitX[i], 0xA6,
                g_CharacterPortraitX[i] + 0x18, 0xA6,
                g_CharacterPortraitX[i] + 0x18, 0xBC
            );

            setXY3(
                &g_Menu->pShop->linesPortraitHighlight2[i*2 + j],
                g_CharacterPortraitX[i], 0xA6,
                g_CharacterPortraitX[i], 0xBC,
                g_CharacterPortraitX[i] + 0x18, 0xBC
            );
        }

        g_Menu->pShop->shouldRenderPortraitHighlight[i] = 0;
    }

    // White line between gold amounts?
    for (j = 0; j < 2; j++) {
        SetLineF2(&g_Menu->pShop->lines3BF0[j]);
        setRGB0(&g_Menu->pShop->lines3BF0[j], 255, 255, 255);
        setXY2(
            &g_Menu->pShop->lines3BF0[j],
            g_TotalPriceTextX - 8, g_TotalPriceTextY + 9,
            g_TotalPriceTextX + 0x4E, g_TotalPriceTextY + 9
        );
    }
}

// Set vertices relative to center of screen?
void ShopMenuSetVertices(SVECTOR* pVertices, u_short x, u_short y, u_short width, u_short height) {
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

void ShopMenuSetWindowBorderPrimitive(P_TAG* pPrim) {
    SetSemiTrans(pPrim, 1);
    SetShadeTex(pPrim, 0);
    setRGB0(pPrim, 128, 128, 128);
}

// scrollOffset refers to the index we're currently scrolled down at
void ShopMenuUpdateScrollBarHandle(int x, int y, int scrollHandleHeight, int numItems, int scrollOffset) {
    int yOffset = 0;

    if (!g_Menu->pManager->scrollHandleActive) {
        g_Menu->pScrollHandle = HeapAlloc(sizeof(MenuScrollBarHandle), 0x0);
        bzero(g_Menu->pScrollHandle, sizeof(MenuScrollBarHandle));
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

    ShopMenuSetVertices(
        g_Menu->pScrollHandle->vertices,
        x, y + yOffset,
        8, scrollHandleHeight
    );

    g_Menu->pScrollHandle->renderContext = g_Menu->renderContext;
    g_Menu->pManager->scrollHandleActive = TRUE;
}

void ShopMenuFreeScrollBarHandle(void) {
    g_Menu->pManager->scrollHandleActive = FALSE;
    HeapFree(g_Menu->pScrollHandle);
}

void ShopMenuInitializeArrowCursor(u_char index) {
    g_Menu->arrowCursors[index] = HeapAlloc(sizeof(MenuArrowCursor), 0x0);
    bzero(g_Menu->arrowCursors[index], sizeof(MenuArrowCursor));
    g_Menu->arrowCursors[index]->curAnimFrame = 4;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

void ShopMenuUpdateArrowCursor(int selectedIndex, int scrollOffset, u_char arg2, u_char cursorIndex) {
    int yOffset;
    MenuArrowCursor* pArrowCursor;
    POLY_FT4* pPoly;

    scrollOffset = 0;

    pArrowCursor = g_Menu->arrowCursors[cursorIndex];

    // Update animation
    if (++pArrowCursor->animFrameDuration >= 6) {
        pArrowCursor->curAnimFrame--;
        if (pArrowCursor->curAnimFrame < 0) {
            pArrowCursor->curAnimFrame = 4;
        }
        pArrowCursor->animFrameDuration = 0;
    }

    if (!arg2) {
        yOffset = (selectedIndex * FONT_LETTER_HEIGHT) + 50;
    }

    if (scrollOffset != 1) {
        func_8002675C(
            g_Menu->resources, pArrowCursor->curAnimFrame + MENU_TEX_ARROW_CURSOR,
            pArrowCursor->polys, g_Menu->renderContext,
            0, 0, 0x1000
        );

        pPoly = &pArrowCursor->polys[g_Menu->renderContext];

        ShopMenuSetVertices(
            pArrowCursor->vertices,
            pPoly->x0 + 28, pPoly->y0 + yOffset,
            pPoly->x1 - pPoly->x0, pPoly->y3 - pPoly->y0
        );
        pArrowCursor->renderContext = g_Menu->renderContext;
        g_Menu->pManager->shouldRenderArrowCursor[cursorIndex] = TRUE;
        return;
    }

    g_Menu->pManager->shouldRenderArrowCursor[cursorIndex] = FALSE;
}

void ShopMenuFreeArrowCursor(u_char index) {
    HeapFree(g_Menu->arrowCursors[index]);
    g_Menu->pManager->shouldRenderArrowCursor[index] = FALSE;
}

void ShopMenuInitializeWindowGraphics(u_char index) {
    RECT rect;
    MenuWindow* pWindow;
    u_char i;

    pWindow = g_Menu->windows[index];

    rect.y = 0;
    rect.x = 0;
    rect.h = 256;
    rect.w = 256;

    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    g_Menu->pManager->isWindowOpening[index] = FALSE;

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

void ShopMenuInitializeScrollBar(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[index];

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

    ShopMenuSetVertices(pWindow->vertsScrollBarEnds, x, y, 8, 8);
    ShopMenuSetVertices(&pWindow->vertsScrollBarEnds[4], x, y + height, 8, -8);
    ShopMenuSetVertices(pWindow->vertsScrollBarEmpty, x, y + 8, 8, height - 8);
}

void ShopMenuInitializeWindowBorderCorners(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    int i;

    pWindow = g_Menu->windows[index];

    pWindow->borderPrimCount = 0;
    pWindow->borderPrimCount += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_TOP_LEFT,
        &pWindow->polysWindowBorderCorners,
        g_Menu->renderContext,
        0, 0, 0x1000
    );
    pWindow->borderPrimCount += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_TOP_RIGHT,
        &pWindow->polysWindowBorderCorners[2 * pWindow->borderPrimCount],
        g_Menu->renderContext,
        0, 0, 0x1000
    );
    pWindow->borderPrimCount += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_BOTTOM_LEFT,
        &pWindow->polysWindowBorderCorners[2 * pWindow->borderPrimCount],
        g_Menu->renderContext,
        0, 0, 0x1000
    );
    pWindow->borderPrimCount += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_BOTTOM_RIGHT,
        &pWindow->polysWindowBorderCorners[2 * pWindow->borderPrimCount],
        g_Menu->renderContext,
        0, 0, 0x1000
    );

    ShopMenuSetVertices(
        pWindow->vertsWindowBorderCorners,
        x - 8,
        y + 8,
        16, -16
    );
    ShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[4],
        x + width + 8,
        y + 8,
        -16, -16
    );
    ShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[8],
        x - 8,
        y + height - 8,
        16, 16
    );
    ShopMenuSetVertices(
        &pWindow->vertsWindowBorderCorners[0xC],
        x + width + 8,
        y + height - 8,
        -16, 16
    );

    for (i = 0; i < 4; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderCorners[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindowBorderTop(u_char index, u_short x, u_short y, u_short width) {
    MenuWindow* pWindow;
    int i;
    int innerWidth;
    u_short halfInnerWidth;

    pWindow = g_Menu->windows[index];

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

    ShopMenuSetVertices(
        pWindow->vertsWindowBorderTop1,
        x + MENU_WINDOW_BORDER_SIZE,
        y - MENU_WINDOW_BORDER_SIZE,
        halfInnerWidth,
        MENU_WINDOW_BORDER_SIZE * 2
    );

    ShopMenuSetVertices(
        pWindow->vertsWindowBorderTop2,
        x + MENU_WINDOW_BORDER_SIZE + halfInnerWidth,
        y - MENU_WINDOW_BORDER_SIZE,
        halfInnerWidth,
        MENU_WINDOW_BORDER_SIZE * 2
    );

    for (i = 0; i < 2; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderTop[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindowBorderBottom(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerWidth;

    pWindow = g_Menu->windows[index];

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

    ShopMenuSetVertices(
        pWindow->vertsWindowBorderBottom1,
        x + 8,
        height,
        width, 16
    );
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderBottom2,
        x + 8 + width,
        height,
        width, 16
    );

    for (i = 0; i < 2; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderBottom[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindowBorderLeft(u_char index, u_short x, u_short y, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerHeight;
    u_short halfInnerHeight;

    pWindow = g_Menu->windows[index];

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

    ShopMenuSetVertices(
        pWindow->vertsWindowBorderLeft1,
        x - 8, y + 8,
        16, halfInnerHeight
    );
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderLeft2,
        x - 8, y + 8 + halfInnerHeight,
        16, halfInnerHeight
    );

    for (i = 0; i < 2; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderLeft[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindowBorderRight(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow;
    int i;
    int innerHeight;
    u_short halfInnerHeight;

    pWindow = g_Menu->windows[index];

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

    ShopMenuSetVertices(
        pWindow->vertsWindowBorderRight1,
        width, y + 8,
        16, halfInnerHeight
    );
    ShopMenuSetVertices(
        pWindow->vertsWindowBorderRight2,
        width, y + 8 + halfInnerHeight,
        16, halfInnerHeight
    );

    for (i = 0; i < 2; i++) {
        ShopMenuSetWindowBorderPrimitive(&pWindow->polysWindowBorderRight[i * 2 + g_Menu->renderContext]);
    }
}

void ShopMenuSetWindow(u_char index, u_short x, u_short y, u_short width, u_short height, u_char arg5, int zIndex, u_char hasScrollBar) {
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    ShopMenuSetVertices(pWindow->vertsBackground, x, y, width, height);
    ShopMenuInitializeWindowBorderCorners(index, x, y, width, height);
    ShopMenuSetWindowBorderTop(index, x, y, width);
    ShopMenuSetWindowBorderBottom(index, x, y, width, height);
    ShopMenuSetWindowBorderLeft(index, x, y, height);
    ShopMenuSetWindowBorderRight(index, x, y, width, height);
    if (hasScrollBar) {
        ShopMenuInitializeScrollBar(index, x, y, width, height);
    }
    pWindow->hasScrollBar = hasScrollBar;
    pWindow->useDefaultMatrix = arg5;
    pWindow->zIndex = zIndex;
    pWindow->renderContext = g_Menu->renderContext;
    g_Menu->pManager->shouldRenderWindow[index] = TRUE;
}

void ShopMenuFreeWindow(u_char index) {
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    g_Menu->pManager->isWindowOpening[index] = FALSE;
    HeapFree(g_Menu->windows[index]);
    HeapFree(g_Menu->windowParameters[index]);
}

void ShopMenuInitializeWindow(u_char windowIndex, u_short x, u_short y, u_short width, u_short height, u_char shouldInitializeHandle, u8 arg6, int zIndex, u_char hasScrollBar) {
    MenuWindowParameters* pWindowParams;

    if (windowIndex >= 2) {
        g_Menu->windows[windowIndex] = HeapAlloc(sizeof(MenuWindow), 0);
        bzero(g_Menu->windows[windowIndex], sizeof(MenuWindow));
        g_Menu->windowParameters[windowIndex] = HeapAlloc(sizeof(MenuWindowParameters), 0);
        bzero(g_Menu->windowParameters[windowIndex] , sizeof(MenuWindowParameters));
        ShopMenuInitializeWindowGraphics(windowIndex);
    }

    pWindowParams = g_Menu->windowParameters[windowIndex];
    if (shouldInitializeHandle) {
        pWindowParams->index = windowIndex;
        pWindowParams->isOpen = 0;
        pWindowParams->x = x;
        pWindowParams->y = y;
        pWindowParams->width = width;
        pWindowParams->height = height;
        pWindowParams->curWidth = 0;
        pWindowParams->curHeight = 0;
        g_Menu->pManager->isWindowOpening[windowIndex] = 1;
        pWindowParams->useDefaultMatrix = arg6;
        pWindowParams->zIndex = zIndex;
        return;
    }

    ShopMenuSetWindow(windowIndex, x, y, width, height, arg6, zIndex, hasScrollBar);
}

void ShopMenuUpdateWindows(void) {
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

            ShopMenuSetWindow(
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

// Project and draw polygons
void ShopMenuRenderPolygons(int numPolygons, SVECTOR* pVertices, POLY_FT4* pPolys, int renderContext) {
    long interpolated;
    long flag;
    int i;

    for (i = 0; i < numPolygons; i++) {
        RotTransPers4(
            &pVertices[i*4 + 0],
            &pVertices[i*4 + 1],
            &pVertices[i*4 + 2],
            &pVertices[i*4 + 3],
            &pPolys[i*2 + renderContext].x0,
            &pPolys[i*2 + renderContext].x1,
            &pPolys[i*2 + renderContext].x2,
            &pPolys[i*2 + renderContext].x3,
            &interpolated,
            &flag
        );

        AddPrim(&g_Menu->pGfxEnv->ot[4], &pPolys[i*2 + renderContext]);
    }
}

void ShopMenuRenderString(int stringLength, POLY_FT4* pPolys, int renderContext) {
    int i;

    for (i = 0; i < stringLength; i++) {
        AddPrim(
            &g_Menu->pGfxEnv->ot[4],
            &pPolys[i * 2 + renderContext]
        );
    }
}

void ShopMenuRenderScrollBarHandle(void) {
    if (g_Menu->pManager->scrollHandleActive) {
        ShopMenuRenderPolygons(1, g_Menu->pScrollHandle->vertices, g_Menu->pScrollHandle->polys, g_Menu->pScrollHandle->renderContext);
    }
}

void ShopMenuAddPointerCursorPrims(void) {
    AddPrim(
        &g_Menu->pGfxEnv->ot[4],
        &g_Menu->pCursorPrims->drModes1[g_Menu->pCursorPrims->drawModeRenderContext]
    );

    if (g_Menu->pManager->shouldRenderPointerCursor) {
        AddPrim(
            &g_Menu->pGfxEnv->ot[4],
            &g_Menu->pCursorPrims->polysPointerCursor[g_Menu->pCursorPrims->cursorRenderContext]
        );
    }
}

void ShopMenuRenderTopWindowBorder(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];

    RotTransPers4(
        &pWindow->vertsWindowBorderTop1[0],
        &pWindow->vertsWindowBorderTop1[1],
        &pWindow->vertsWindowBorderTop1[2],
        &pWindow->vertsWindowBorderTop1[3],
        (long*) &pWindow->polysWindowBorderTop[pWindow->renderContext].x0,
        (long*) &pWindow->polysWindowBorderTop[pWindow->renderContext].x1,
        (long*) &pWindow->polysWindowBorderTop[pWindow->renderContext].x2,
        (long*) &pWindow->polysWindowBorderTop[pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysWindowBorderTop[pWindow->renderContext]
    );

    RotTransPers4(
        &pWindow->vertsWindowBorderTop2[0],
        &pWindow->vertsWindowBorderTop2[1],
        &pWindow->vertsWindowBorderTop2[2],
        &pWindow->vertsWindowBorderTop2[3],
        (long*) &pWindow->polysWindowBorderTop[2 + pWindow->renderContext].x0,
        (long*) &pWindow->polysWindowBorderTop[2 + pWindow->renderContext].x1,
        (long*) &pWindow->polysWindowBorderTop[2 + pWindow->renderContext].x2,
        (long*) &pWindow->polysWindowBorderTop[2 + pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysWindowBorderTop[2 + pWindow->renderContext]
    );
}

void ShopMenuRenderBottomWindowBorder(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];

    RotTransPers4(
        &pWindow->vertsWindowBorderBottom1[0],
        &pWindow->vertsWindowBorderBottom1[1],
        &pWindow->vertsWindowBorderBottom1[2],
        &pWindow->vertsWindowBorderBottom1[3],
        (long*) &pWindow->polysWindowBorderBottom[pWindow->renderContext].x0,
        (long*) &pWindow->polysWindowBorderBottom[pWindow->renderContext].x1,
        (long*) &pWindow->polysWindowBorderBottom[pWindow->renderContext].x2,
        (long*) &pWindow->polysWindowBorderBottom[pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysWindowBorderBottom[pWindow->renderContext]
    );

    RotTransPers4(
        &pWindow->vertsWindowBorderBottom2[0],
        &pWindow->vertsWindowBorderBottom2[1],
        &pWindow->vertsWindowBorderBottom2[2],
        &pWindow->vertsWindowBorderBottom2[3],
        (long*) &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext].x0,
        (long*) &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext].x1,
        (long*) &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext].x2,
        (long*) &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysWindowBorderBottom[2 + pWindow->renderContext]
    );
}

void ShopMenuRenderLeftWindowBorder(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];

    RotTransPers4(
        &pWindow->vertsWindowBorderLeft1[0],
        &pWindow->vertsWindowBorderLeft1[1],
        &pWindow->vertsWindowBorderLeft1[2],
        &pWindow->vertsWindowBorderLeft1[3],
        (long*) &pWindow->polysWindowBorderLeft[pWindow->renderContext].x0,
        (long*) &pWindow->polysWindowBorderLeft[pWindow->renderContext].x1,
        (long*) &pWindow->polysWindowBorderLeft[pWindow->renderContext].x2,
        (long*) &pWindow->polysWindowBorderLeft[pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysWindowBorderLeft[pWindow->renderContext]
    );

    RotTransPers4(
        &pWindow->vertsWindowBorderLeft2[0],
        &pWindow->vertsWindowBorderLeft2[1],
        &pWindow->vertsWindowBorderLeft2[2],
        &pWindow->vertsWindowBorderLeft2[3],
        (long*) &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext].x0,
        (long*) &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext].x1,
        (long*) &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext].x2,
        (long*) &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysWindowBorderLeft[2 + pWindow->renderContext]
    );
}

void ShopMenuRenderRightWindowBorder(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];

    RotTransPers4(
        &pWindow->vertsWindowBorderRight1[0],
        &pWindow->vertsWindowBorderRight1[1],
        &pWindow->vertsWindowBorderRight1[2],
        &pWindow->vertsWindowBorderRight1[3],
        (long*) &pWindow->polysWindowBorderRight[pWindow->renderContext].x0,
        (long*) &pWindow->polysWindowBorderRight[pWindow->renderContext].x1,
        (long*) &pWindow->polysWindowBorderRight[pWindow->renderContext].x2,
        (long*) &pWindow->polysWindowBorderRight[pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysWindowBorderRight[pWindow->renderContext]
    );

    RotTransPers4(
        &pWindow->vertsWindowBorderRight2[0],
        &pWindow->vertsWindowBorderRight2[1],
        &pWindow->vertsWindowBorderRight2[2],
        &pWindow->vertsWindowBorderRight2[3],
        (long*) &pWindow->polysWindowBorderRight[2 + pWindow->renderContext].x0,
        (long*) &pWindow->polysWindowBorderRight[2 + pWindow->renderContext].x1,
        (long*) &pWindow->polysWindowBorderRight[2 + pWindow->renderContext].x2,
        (long*) &pWindow->polysWindowBorderRight[2 + pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysWindowBorderRight[2 + pWindow->renderContext]
    );
}

void ShopMenuRenderWindowBackground(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];

    RotTransPers4(
        &pWindow->vertsBackground[0],
        &pWindow->vertsBackground[1],
        &pWindow->vertsBackground[2],
        &pWindow->vertsBackground[3],
        (long*) &pWindow->polysBackground[pWindow->renderContext].x0,
        (long*) &pWindow->polysBackground[pWindow->renderContext].x1,
        (long*) &pWindow->polysBackground[pWindow->renderContext].x2,
        (long*) &pWindow->polysBackground[pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysBackground[pWindow->renderContext]
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->drawModes[pWindow->renderContext]
    );
}

void ShopMenuRenderWindowBorderCorners(int index) {
    long interpolated;
    long flag;
    int i;
    MenuWindow* pWindow;

    pWindow = g_Menu->windows[index];

    for (i = 0; i < 4; i++) {
        // Project vertices to x0/y0 coordinates of the polygon
        RotTransPers4(
            &pWindow->vertsWindowBorderCorners[i * 4],
            &pWindow->vertsWindowBorderCorners[i * 4 + 1],
            &pWindow->vertsWindowBorderCorners[i * 4 + 2],
            &pWindow->vertsWindowBorderCorners[i * 4 + 3],
            (long*) &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext].x0,
            (long*) &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext].x1,
            (long*) &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext].x2,
            (long*) &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext].x3,
            &interpolated,
            &flag
        );

        // Queue polygon for rendering
        AddPrim(
            &g_Menu->pGfxEnv->ot[pWindow->zIndex],
            &pWindow->polysWindowBorderCorners[i * 2 + pWindow->renderContext]
        );
    }
}

void ShopMenuRenderScrollBar(int index) {
    long interpolated;
    long flag;
    MenuWindow* pWindow;
    int i;

    pWindow = g_Menu->windows[index];

    for (i = 0; i < 2; i++) {
        RotTransPers4(
            &pWindow->vertsScrollBarEnds[i * 4],
            &pWindow->vertsScrollBarEnds[i * 4 + 1],
            &pWindow->vertsScrollBarEnds[i * 4 + 2],
            &pWindow->vertsScrollBarEnds[i * 4 + 3],
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext].x0,
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext].x1,
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext].x2,
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext].x3,
            &interpolated,
            &flag
        );
        AddPrim(
            &g_Menu->pGfxEnv->ot[pWindow->zIndex],
            &pWindow->polysScrollBarEnds[i * 2 + pWindow->renderContext]
        );
    }

    RotTransPers4(
        &pWindow->vertsScrollBarEmpty[0],
        &pWindow->vertsScrollBarEmpty[1],
        &pWindow->vertsScrollBarEmpty[2],
        &pWindow->vertsScrollBarEmpty[3],
        &pWindow->polysScrollBarEmpty[pWindow->renderContext].x0,
        &pWindow->polysScrollBarEmpty[pWindow->renderContext].x1,
        &pWindow->polysScrollBarEmpty[pWindow->renderContext].x2,
        &pWindow->polysScrollBarEmpty[pWindow->renderContext].x3,
        &interpolated,
        &flag
    );
    AddPrim(
        &g_Menu->pGfxEnv->ot[pWindow->zIndex],
        &pWindow->polysScrollBarEmpty[pWindow->renderContext]
    );
}

void ShopMenuRenderWindows(void) {
    SVECTOR rotation;
    VECTOR translation;
    MATRIX matTransform;
    SVECTOR _unused;
    MenuWindow* pWindow;
    int i;

    // Project and render all active windows
    for (i = 0; i < MENU_MAX_NUM_WINDOWS; i++) {
        if (g_Menu->pManager->shouldRenderWindow[i]) {
            pWindow = g_Menu->windows[i];
            if (pWindow->useDefaultMatrix == 0) {
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

                ShopMenuRenderWindowBorderCorners(i);
                if (pWindow->hasScrollBar) {
                    ShopMenuRenderScrollBar(i);
                }
                ShopMenuRenderTopWindowBorder(i);
                ShopMenuRenderBottomWindowBorder(i);
                ShopMenuRenderLeftWindowBorder(i);
                ShopMenuRenderRightWindowBorder(i);
                ShopMenuRenderWindowBackground(i);

                PopMatrix();
            } else {
                ShopMenuRenderWindowBorderCorners(i);
                if (pWindow->hasScrollBar) {
                    ShopMenuRenderScrollBar(i);
                }
                ShopMenuRenderTopWindowBorder(i);
                ShopMenuRenderBottomWindowBorder(i);
                ShopMenuRenderLeftWindowBorder(i);
                ShopMenuRenderRightWindowBorder(i);
                ShopMenuRenderWindowBackground(i);
            }
        }
    }
}

void ShopMenuRenderPointerCursors(void) {
    int i;

    if (g_Menu->pManager->shouldRenderCursors) {
        for (i = 0; i < MENU_MAX_NUM_CURSORS; i++) {
            if (g_Menu->pCursors->shouldRender[i]) {
                if (g_Menu->pCursors->shouldUpdatePosition[i]) {
                    setXY4(
                        &g_Menu->pCursors->polysCursor[i * 2 + g_Menu->pCursors->renderContexts[i]],
                        g_PointerCursorBoxX[g_PointerCursorBoxSlot[g_Menu->menuUnk2->selectedSlot]] + 8,
                        g_PointerCursorBoxY[g_PointerCursorBoxSlot[g_Menu->menuUnk2->selectedSlot]] - 6,
                        g_PointerCursorBoxX[g_PointerCursorBoxSlot[g_Menu->menuUnk2->selectedSlot]] + 24,
                        g_PointerCursorBoxY[g_PointerCursorBoxSlot[g_Menu->menuUnk2->selectedSlot]] - 6,
                        g_PointerCursorBoxX[g_PointerCursorBoxSlot[g_Menu->menuUnk2->selectedSlot]] + 8,
                        g_PointerCursorBoxY[g_PointerCursorBoxSlot[g_Menu->menuUnk2->selectedSlot]] + 10,
                        g_PointerCursorBoxX[g_PointerCursorBoxSlot[g_Menu->menuUnk2->selectedSlot]] + 24,
                        g_PointerCursorBoxY[g_PointerCursorBoxSlot[g_Menu->menuUnk2->selectedSlot]] + 10
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

void ShopMenuAddUnk4E0Prims(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->shouldRenderStrings4E0[i]) {
            AddPrim(
                &g_Menu->pGfxEnv->ot[4],
                &g_Menu->titleStrings[i].polys[g_Menu->titleStrings[i].renderContext]
            );
        }
    }
}

void ShopMenuAddUnk6E0Prims(void) {
    int i;

    for (i = 0; i < 8; i++) {
        if (g_Menu->pManager->shouldRenderStrings[i]) {
            AddPrim(
                &g_Menu->pGfxEnv->ot[4],
                &g_Menu->optionStrings[i].polys[g_Menu->optionStrings[i].renderContext]
            );
        }
    }
}

void ShopMenuAddUnkAE0Prims(void) {
    long interpolated;
    long flag;
    int i;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->shouldRenderSubMenuChoiceStrings[i]) {
            if (g_Menu->subMenuChoiceStrings[i].hasVertices) {
                RotTransPers4(
                    &g_Menu->subMenuChoiceStrings[i].vertices[0],
                    &g_Menu->subMenuChoiceStrings[i].vertices[1],
                    &g_Menu->subMenuChoiceStrings[i].vertices[2],
                    &g_Menu->subMenuChoiceStrings[i].vertices[3],
                    (long*)&g_Menu->subMenuChoiceStrings[i].polys[g_Menu->subMenuChoiceStrings[i].renderContext].x0,
                    (long*)&g_Menu->subMenuChoiceStrings[i].polys[g_Menu->subMenuChoiceStrings[i].renderContext].x1,
                    (long*)&g_Menu->subMenuChoiceStrings[i].polys[g_Menu->subMenuChoiceStrings[i].renderContext].x2,
                    (long*)&g_Menu->subMenuChoiceStrings[i].polys[g_Menu->subMenuChoiceStrings[i].renderContext].x3,
                    &interpolated,
                    &flag
                );
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4],
                    &g_Menu->subMenuChoiceStrings[i].polys[g_Menu->subMenuChoiceStrings[i].renderContext]
                );
            } else {
                AddPrim(
                    &g_Menu->pGfxEnv->ot[4],
                    &g_Menu->subMenuChoiceStrings[i].polys[g_Menu->subMenuChoiceStrings[i].renderContext]
                );
            }

        }
    }
}

int ShopMenuDelayLoop6(void) {
    int i;
    for (i = 5; i >= 0; i--) {}
    return i;
}

// Render confirmation window text?
void ShopMenuAddUnk1DE0Prims(void) {
    long interpolated;
    long flag;
    MenuString* pString;
    int i;

    if (g_Menu->pManager->shouldRenderModalStrings) {
        for (i = 0 ; i < 3; i++) {
            pString = g_Menu->modalStrings[i];
            if (pString->hasVertices) {
                RotTransPers4(
                    &pString->vertices[0],
                    &pString->vertices[1],
                    &pString->vertices[2],
                    &pString->vertices[3],
                    (long*) &pString->polys[pString->renderContext].x0,
                    (long*) &pString->polys[pString->renderContext].x1,
                    (long*) &pString->polys[pString->renderContext].x2,
                    (long*) &pString->polys[pString->renderContext].x3,
                    &interpolated,
                    &flag
                );
                AddPrim(&g_Menu->pGfxEnv->ot[4], &pString->polys[pString->renderContext]);
            } else {
                AddPrim(&g_Menu->pGfxEnv->ot[4], &pString->polys[pString->renderContext]);
            }

        }
    }
}

// Draw the black dimming effect
void ShopMenuRenderBackgroundDim(void) {
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->pCursorPrims->polysDimEffect[g_Menu->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->pCursorPrims->drawModeDimEffect[g_Menu->renderContext]);
}

void ShopMenuAddStringGroupPrims(void) {
    ShopMenuAddUnk4E0Prims();
    ShopMenuAddUnk6E0Prims();
    ShopMenuAddUnkAE0Prims();
    ShopMenuDelayLoop6();
    ShopMenuAddUnk1DE0Prims();
}

void ShopMenuRenderSelectionMenu(void) {
    int i;

    if (g_Menu->pManager->shouldRenderSelectionMenu) {
        if (g_Menu->pSelectionMenu->isDisabled != g_Menu->pSelectionMenu->isDisabledRendered) {

            // Render the selection menu as disabled
            if (g_Menu->pSelectionMenu->isDisabled) {
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

            g_Menu->pSelectionMenu->isDisabledRendered = g_Menu->pSelectionMenu->isDisabled;
        }

        ShopMenuRenderString(g_Menu->pSelectionMenu->numTexts, g_Menu->pSelectionMenu->polysTexts, g_Menu->pSelectionMenu->textsRenderCtx);
        ShopMenuRenderString(g_Menu->pSelectionMenu->numCursors, g_Menu->pSelectionMenu->polysCursors, g_Menu->pSelectionMenu->cursorsRenderCtx);
    }
}

void ShopMenuAddUnk354Prims(void) {
    if (g_Menu->pManager->shouldRenderSubmenuStrings) {
        ShopMenuRenderString(g_Menu->pSubmenuPrims->polys500PrimCount, g_Menu->pSubmenuPrims->polys500, g_Menu->pSubmenuPrims->polys500RenderCtx);
        ShopMenuRenderString(g_Menu->pSubmenuPrims->polys0PrimCount, g_Menu->pSubmenuPrims->polys0, g_Menu->pSubmenuPrims->polys0RenderCtx);
    }
}

void ShopMenuRenderArrowCursors(void) {
    int i;

    for (i = 0; i < MENU_MAX_NUM_ARROW_CURSORS; i++) {
        if (g_Menu->pManager->shouldRenderArrowCursor[i]) {
            ShopMenuRenderPolygons(1,
                g_Menu->arrowCursors[i]->vertices,
                g_Menu->arrowCursors[i]->polys,
                g_Menu->arrowCursors[i]->renderContext
            );
        }
    }
}

void ShopMenuRender(void) {
    if (g_Menu->shouldDrawMenu) {
        ShopMenuUpdateWindows();
        ShopMenuRenderPointerCursors();
        ShopMenuAddStringGroupPrims();
        ShopMenuAddPointerCursorPrims();
        ShopMenuRenderArrowCursors();
        ShopMenuAddShopPrims();
        ShopMenuRenderScrollBarHandle();
        ShopMenuRenderSelectionMenu();
        ShopMenuAddUnk354Prims();
        ShopMenuRenderWindows();
    }
    ShopMenuRenderBackgroundDim();
}

void ShopMenuPlaySoundEffect(u_char soundEffectId) {
    if (g_Menu->soundEffectsEnabled) {
        SoundPlaySoundEffectOnLastPair((g_Menu->pSoundFile->sedId << 16) | soundEffectId);
    }
}

void ShopMenuPollInput(void) {
    u_char wasControllerUnplugged;
    u_char isLooping;
    int savedValue;
    u_char input;

    input = MENU_INPUT_IDLE;
    isLooping = TRUE;
    wasControllerUnplugged = FALSE;
    while (isLooping) {
        // Is the main controller not plugged in?
        if (ControllerGetType(0) == CONTROLLER_TYPE_NONE) {
            if (wasControllerUnplugged == 0) {
                wasControllerUnplugged++;
                SoundMuteAllSpuChannels();
                savedValue = g_SpuChannelMask;
            }
            continue;
        }

        isLooping--;
        if (wasControllerUnplugged) {
            SoundEnableAllSpuChannels();
            g_SpuChannelMask = savedValue;
        }
    }

    if (ControllerIsStateStackFull()) {
        ControllerResetState();
    } else {
        while (ControllerPopState()) {
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_RIGHT) {
                input = MENU_INPUT_RIGHT;
                ShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
                input = MENU_INPUT_DOWN;
                ShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) {
                input = MENU_INPUT_LEFT;
                ShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
                input = MENU_INPUT_UP;
                ShopMenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
                input = MENU_INPUT_CONFIRM;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CROSS) {
                input = MENU_INPUT_BACK;
                ShopMenuPlaySoundEffect(3);
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
            if (g_C1ButtonStateReleased & CTRL_BTN_L1) {
                input = 10;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_R1) {
                input = 9;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_START) {
                input = 11;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_SELECT) {
                g_Menu->debuggerEnabled = g_Menu->debuggerEnabled == 0;
                input = 12;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_L2) {
                g_Menu->debuggerPage += 1;
                break;
            }
        }
    }
    g_Menu->input = input;
}

// Updates the effect where the menu is rotated and zoomed in/out
void ShopMenuUpdateTransitionEffect(void) {
    switch (g_Menu->transitionEffectState) {
        case MENU_CLOSE_ANIMATION_START:
            g_Menu->translation.vz = 512;
            g_Menu->rotation.vz = 0;
            g_Menu->rotation.vy = 0;
            g_Menu->rotation.vx = 0;
            g_Menu->translation.vy = 0;
            g_Menu->translation.vx = 0;
            g_Menu->transitionEffectState = MENU_CLOSE_ANIMATION;
            break;
        case MENU_OPEN_ANIMATION_START:
            // Set window to the far back
            g_Menu->translation.vz = 2048;
            g_Menu->rotation.vz = 0;
            g_Menu->rotation.vy = 0;
            g_Menu->rotation.vx = 0;
            g_Menu->translation.vy = 0;
            g_Menu->translation.vx = 0;
            g_Menu->transitionEffectState = MENU_OPEN_ANIMATION;
            break;
        case MENU_CLOSE_ANIMATION:
            g_Menu->rotation.vy -= 96;
            g_Menu->translation.vz += 64;
            if (g_Menu->translation.vz >= PSX_DEGREES(315)) {
                g_Menu->transitionEffectState = MENU_ANIMATION_DONE;
            }
            break;
        case MENU_OPEN_ANIMATION:
            // Flip the window around the X-axis while
            // bring it closer to the viewer, giving and
            // effect of zooming in.
            g_Menu->rotation.vx += 124;
            g_Menu->translation.vz -= 48;
            if (g_Menu->translation.vz < 512) {
                g_Menu->translation.vz = 512;
                g_Menu->rotation.vz = 0;
                g_Menu->rotation.vx = 0;
                g_Menu->rotation.vy = 0;
                g_Menu->transitionEffectState = MENU_ANIMATION_DONE;
            }
            break;
    }

    RotMatrix(&g_Menu->rotation, &g_Menu->matTransform);
    TransMatrix(&g_Menu->matTransform, &g_Menu->translation);
    SetRotMatrix(&g_Menu->matTransform);
    SetTransMatrix(&g_Menu->matTransform);
}

void ShopMenuUpdateAndRender(void) {
    GfxEnvironment* pNextGfxEnv;
    int renderContext;

    // Debugging
    if (*g_DebuggerState != -1) {
        asm("break 0x400");
    }

    ShopMenuPollInput();
    GameCheckAndHandleSoftReset();

    // Rotate graphics environment
    pNextGfxEnv = &g_Menu->gfxEnvs[0];
    if (g_Menu->pGfxEnv == &g_Menu->gfxEnvs[0]) {
        pNextGfxEnv = &g_Menu->gfxEnvs[1];
    }
    g_Menu->pGfxEnv = pNextGfxEnv;

    g_Menu->renderContext = g_Menu->renderContext == 0;
    ClearOTagR(g_Menu->pGfxEnv->ot, 0x10);
    ShopMenuUpdateTransitionEffect();
    ShopMenuRender();
    renderContext = g_Menu->renderContext == 0;
    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    MoveImage(&g_Menu->pSelectionMenu->vramRect, 0, renderContext * 0xE0);
    DrawOTag(&g_Menu->pGfxEnv->ot[0xF]);
}

void ShopMenuInitializePointerCursors(u_char mode) {
    int i;

    g_Menu->pCursors = HeapAlloc(sizeof(MenuPointerCursors), 0);
    bzero(g_Menu->pCursors, sizeof(MenuPointerCursors));

    switch (mode) {
        case 0:
            g_Menu->pManager->shouldRenderCursors = TRUE;
            g_Menu->pCursors->shouldUpdatePosition[0] = TRUE;
            g_Menu->pCursors->shouldUpdatePosition[1] = TRUE;
            /* fallthrough */
        case 2:
            for (i = 0; i < MENU_MAX_NUM_CURSORS; i++) {
                func_8002675C(
                    g_Menu->resources, MENU_TEX_POINTER_CURSOR,
                    &g_Menu->pCursors->polysCursor[i * 2], g_Menu->renderContext,
                    g_PointerCursorInitX[i], g_PointerCursorInitY[i],
                    0x800
                );
                g_Menu->pCursors->renderContexts[i] = g_Menu->renderContext;
            }
            return;
        case 3:
            func_8002675C(
                g_Menu->resources, MENU_TEX_POINTER_CURSOR,
                &g_Menu->pCursors->polysCursor[0], g_Menu->renderContext,
                0, 0,
                0x800
            );
            g_Menu->pCursors->renderContexts[0] = g_Menu->renderContext;
            g_Menu->pManager->shouldRenderCursors = TRUE;
            return;
        case 1:
            return;
    }
}

void ShopMenuFreePointerCursors(void) {
    g_Menu->pManager->shouldRenderCursors = FALSE;
    ShopMenuUpdateAndRender();
    HeapFree(g_Menu->pCursors);
}

void ShopMenuStartOpenMenuTransition(void) {
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION_START;
    ShopMenuPlaySoundEffect(0x5B);
}

void ShopMenuStartCloseMenuTransition(void) {
    g_Menu->transitionEffectState = MENU_CLOSE_ANIMATION_START;
}

void ShopMenuConfirmationWindowInitialize(u_char stringIndex) {
    MenuWindowParameters* pWindowParams;
    MenuString* pString;
    int i;
    int xPosition;

    xPosition = 0x50;

    ShopMenuInitializeWindow(4, 0x42, 0x46, 0xBC, 0x40, 1, 1, 4, 0);
    pWindowParams = g_Menu->windowParameters[4];
    while (!pWindowParams->isOpen) {
        ShopMenuUpdateAndRender();
    }

    for (i = 0; i < 4; i++) {
        g_Menu->modalStrings[i] = HeapAlloc(sizeof(MenuString), 0);
        bzero(g_Menu->modalStrings[i], sizeof(MenuString));

        // Even indices
        if (!(i & 1)) {
            // Buffer size = FONT_LETTER_HEIGHT * 57 * 2
            g_Menu->modalStrings[i]->pVramBuffer = HeapAlloc(0x5CA, 0);
            g_Menu->modalStrings[i]->vramDest.x = 320;
            g_Menu->modalStrings[i]->vramDest.y = ((i / 2) * FONT_LETTER_HEIGHT) + 78;
            g_Menu->modalStrings[i]->vramDest.w = 58;
            g_Menu->modalStrings[i]->vramDest.h = FONT_LETTER_HEIGHT;
            continue;
        }

        // Odd indices, share VRAM buffer w/ prev entry
        g_Menu->modalStrings[i]->pVramBuffer = g_Menu->modalStrings[i - 1]->pVramBuffer;
    }

    // Render strings to text areas
    for (i = 0; i < 3; i++) {
        pString = g_Menu->modalStrings[i];
        pString->width = SystemRenderStringEntry(
            GetStringEntry(g_Menu->pStringBank, stringIndex + i),
            pString->pVramBuffer, 0x36, i % 2
        );
        ShopMenuSetStringPrimitives(pString, i, 0, 0);
        ShopMenuSetVertices(pString->vertices, xPosition, (i * 16) + 0x50, pString->width, FONT_LETTER_HEIGHT);
        setUV4(
            &pString->polys[g_Menu->renderContext],
            0,                (i / 2) * FONT_LETTER_HEIGHT + 0x4E,
            pString->width,   (i / 2) * FONT_LETTER_HEIGHT + 0x4E,
            0,                (i / 2) * FONT_LETTER_HEIGHT + 0x5B,
            pString->width,   (i / 2) * FONT_LETTER_HEIGHT + 0x5B
        );
        pString->renderContext = g_Menu->renderContext;
        pString->hasVertices = 1;
    }

    // Transfer text areas to VRAM
    LoadImage(&g_Menu->modalStrings[0]->vramDest, g_Menu->modalStrings[0]->pVramBuffer);
    LoadImage(&g_Menu->modalStrings[2]->vramDest, g_Menu->modalStrings[2]->pVramBuffer);
    DrawSync(0);

    g_Menu->pManager->shouldRenderModalStrings = 1;
    HeapFree(g_Menu->modalStrings[0]->pVramBuffer);
    HeapFree(g_Menu->modalStrings[2]->pVramBuffer);
    if (g_Menu->pManager->finalPriceRenderState == 2) {
        g_Menu->pManager->finalPriceRenderState = 1;
    }
    ShopMenuUpdateAndRender();
    ShopMenuUpdateAndRender();
}

void ShopMenuConfirmationWindowFree(void) {
    int i;

    if (g_Menu->pManager->shouldRenderWindow[4]) {
        ShopMenuFreeWindow(4);
        g_Menu->pManager->shouldRenderModalStrings = 0;
        for (i = 0; i < 4; i++) {
            HeapFree(g_Menu->modalStrings[i]);
        }
    }
    g_Menu->pManager->finalPriceRenderState = 0;
    ShopMenuUpdateAndRender();
}

u_char ShopMenuConfirmationWindowGetChoice(u_char mode) {
    u_char choice;
    u_char isRunning;
    u_char curTimer;

    isRunning = TRUE;
    choice = MENU_CHOICE_NO;
    curTimer = 60;

    while (isRunning) {
        if (mode == MENU_AUTO_ADVANCE) {
            g_Menu->pCursors->shouldRender[2] = FALSE;
            g_Menu->pCursors->shouldRender[3] = FALSE;

            // Was any button pressed?
            if (g_Menu->input != MENU_INPUT_IDLE)
                break;

            curTimer--;
            if (curTimer == 0)
                break;
        }

        ShopMenuUpdateAndRender();
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);
                isRunning = FALSE;
                break;
            case MENU_INPUT_BACK:
                choice = MENU_CHOICE_NO;
                isRunning = FALSE;
                break;
            case MENU_INPUT_LEFT:
                g_Menu->pCursors->shouldRender[2] = TRUE;
                g_Menu->pCursors->shouldRender[3] = FALSE;
                choice = MENU_CHOICE_YES;
                break;
            case MENU_INPUT_RIGHT:
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

int ShopMenuConfirmationWindow(u_char stringIndex, u_char stringIndex2, u_char mode) {
    u_char choice;

    ShopMenuConfirmationWindowInitialize(stringIndex);
    g_Menu->pCursors->shouldRender[3] = TRUE;
    choice = ShopMenuConfirmationWindowGetChoice(mode);
    ShopMenuConfirmationWindowFree();

    // If we chose yes and have another window, run that confirmation window as well
    if (stringIndex2 != 0xFF) {
        if (choice != MENU_CHOICE_NO) {
            ShopMenuConfirmationWindowInitialize(stringIndex2);
            g_Menu->pCursors->shouldRender[3] = TRUE;
            choice = ShopMenuConfirmationWindowGetChoice(mode);
            ShopMenuConfirmationWindowFree();
        }
    }
    return choice;
}

void ShopMenuFree(void) {
    ShopMenuUpdateAndRender();
    ShopMenuUpdateAndRender();
    g_Menu->shouldDrawMenu = FALSE;
    ShopMenuUpdateAndRender();
    do {
        ShopMenuUpdateAndRender();
    } while (g_Menu->renderContext);
    ShopMenuUnk32CManager(MENU_DATA_FREE);
    ShopMenuSetManager(MENU_DATA_FREE);
    ShopMenuSelectionMenuManager(MENU_DATA_FREE);
    ShopMenuUnk354Manager(MENU_DATA_FREE);
    ShopMenuDressingRoomManager(MENU_DATA_FREE);
    ShopMenuUnk348Manager(MENU_DATA_FREE);
    ShopMenuLoadShopItemsData(0x10);
    ShopMenuShopManager(MENU_DATA_FREE);
    HeapFree(g_Menu->resources);
    HeapFree(g_Menu->pStringBank);
    HeapFree(g_Menu->titleStrings[0].pVramBuffer);
    if (g_MenuDebugEnabled) {
        SoundStopElementsForSoundFile(g_Menu->pSoundFile);
        ShopMenuUpdateAndRender();
        SoundRemoveSedsEntry(g_Menu->pSoundFile);
        ShopMenuUpdateAndRender();
        HeapFree(g_Menu->pSoundFile);
    }
    HeapFree(g_Menu->pShopEntries);
    ShopMenuUnk1E20Manager(MENU_DATA_FREE);
    HeapFree(g_Menu);
}

void ShopMenuSetStringListVisible(u_char mode, u_char numStrings, MenuString* pStrings, u_char* pStringIDs, u_char* pShouldRenderFlags) {
    int i;

    if (mode) {
        ShopMenuRenderStringsToVram(pStrings, pStringIDs, 2, numStrings);
        return;
    }

    for (i = 0; i < numStrings; i++) {
        pShouldRenderFlags[i] = 0;
    }
}

void ShopMenuPositionSelectedString(u_char numStrings, MenuString* pStrings, u_char* pStringIDs, int* pXOffsets, u_char* pShouldRenderFlags, u_char index, u_char cursorOffset, u_char mode) {
    switch (mode) {
        case 0:
            ShopMenuSetStringListVisible(0, numStrings, pStrings, pStringIDs, pShouldRenderFlags);
            setXYWH(
                &pStrings[index].polys[g_Menu->renderContext],
                g_ShopPointerCursorX[index + cursorOffset] + 0x16 + pXOffsets[index],
                g_ShopPointerCursorY[index + cursorOffset] - 0x22,
                pStrings[index].width,
                0xD
            );
            break;

        case 1:
            setXYWH(
                &pStrings[index].polys[g_Menu->renderContext],
                0xEC,
                0x7E,
                pStrings->width,
                0xD
            );
            break;
    }

    pStrings[index].renderContext = g_Menu->renderContext;
    pShouldRenderFlags[index] = TRUE;
}

void ShopMenuInitializeShopModeSelectionMenu(int numTextures, int* pTextureIDs) {
    int i;
    int j;

    // Render the selection menu as active
    g_Menu->pSelectionMenu->isDisabled = FALSE;
    g_Menu->pSelectionMenu->isDisabledRendered = FALSE;

    g_Menu->pManager->shouldRenderSelectionMenu = TRUE;

    // Another weird loop case similar to the one in ShopMenuUpdateCharacterPortraits.
    // We loop one and one cursor & text texture entry up to our current counter,
    // rebuild the cursor and text primitives and rerender the whole menu.
    for (i = 1; i <= numTextures; i++) {

        // Cursors for each menu option
        if (i != numTextures) {
            g_Menu->pSelectionMenu->numCursors = 0;
            for (j = 0; j < i; j++) {
                g_Menu->pSelectionMenu->numCursors += func_8002675C(
                    g_Menu->resources, pTextureIDs[j * 2],
                    &g_Menu->pSelectionMenu->polysCursors[g_Menu->pSelectionMenu->numCursors * 2],
                    g_Menu->renderContext,
                    160, 150,
                    0x1000
                );
            }
            g_Menu->pSelectionMenu->cursorsRenderCtx = g_Menu->renderContext;
        }

        // Text / Content for each menu option
        g_Menu->pSelectionMenu->numTexts = 0;
        if (i != 1) {
            for (j = 0; j < i - 1; j++) {
                g_Menu->pSelectionMenu->numTexts += func_8002675C(
                    g_Menu->resources, pTextureIDs[j * 2 + 1],
                    &g_Menu->pSelectionMenu->polysTexts[g_Menu->pSelectionMenu->numTexts * 2],
                    g_Menu->renderContext,
                    160, 150,
                    0x1000
                );
            }
            g_Menu->pSelectionMenu->textsRenderCtx = g_Menu->renderContext;
        }

        for (j = 0; j < 2; j++) {
            ShopMenuUpdateAndRender();
        }
    }
}

void ShopMenuInitializeSubmenu(u_char arg0) {
    int isValid;
    int i;
    int j;

    g_Menu->pSelectionMenu->isDisabled = 0;
    g_Menu->pSelectionMenu->isDisabledRendered = 0;
    g_Menu->pSubmenuPrims->polys0PrimCount = 0;
    g_Menu->pSubmenuPrims->polys500PrimCount = 0;
    g_Menu->pManager->shouldRenderSubmenuStrings = 1;

    isValid = TRUE;
    for (i = 1; i < 5; i++) {
        g_Menu->pSubmenuPrims->polys0PrimCount = 0;
        g_Menu->subMenuNumChoices = 0;

        for (j = 0; j < i; j++) {
            if (g_ShopSubmenuTextureIds[(arg0 + g_Menu->mainMenuChoice) * 8 + j * 2] != 0xFFFF) {
                g_Menu->pSubmenuPrims->polys0PrimCount += func_8002675C(
                    g_Menu->resources,
                    g_ShopSubmenuTextureIds[(arg0 + g_Menu->mainMenuChoice) * 8 + j * 2],
                    &g_Menu->pSubmenuPrims->polys0[g_Menu->pSubmenuPrims->polys0PrimCount * 2],
                    g_Menu->renderContext,
                    0xA0, 0x96,
                    0x1000
                );
                g_Menu->subMenuNumChoices++;
            } else {
                isValid = FALSE;
            }
        }

        g_Menu->pSubmenuPrims->polys0RenderCtx = g_Menu->renderContext;

        if (isValid) {
            for (j = 0; j < 2; j++) {
                ShopMenuUpdateAndRender();
            }
        }

        g_Menu->pSubmenuPrims->polys500PrimCount = 0;

        for (j = 0; j < i; j++) {
            if (g_ShopSubmenuTextureIds[(arg0 + g_Menu->mainMenuChoice) * 8 + j * 2] != 0xFFFF) {
                g_Menu->pSubmenuPrims->polys500PrimCount += func_8002675C(
                    g_Menu->resources,
                    g_ShopSubmenuTextureIds[(arg0 + g_Menu->mainMenuChoice) * 8 + j * 2 + 1],
                    &g_Menu->pSubmenuPrims->polys500[g_Menu->pSubmenuPrims->polys500PrimCount * 2],
                    g_Menu->renderContext,
                    0xA0, 0x96,
                    0x1000
                );
            }
        }

        g_Menu->pSubmenuPrims->polys500RenderCtx = g_Menu->renderContext;

        if (isValid) {
            for (j = 0; j < 2; j++) {
                ShopMenuUpdateAndRender();
            }
        }
    }
}

void ShopMenuUpdateShopModeSelectionMenu(u_char numOptions, u_char selectedIndex, int* pTextureIDs) {
    int cursorTextureID;
    int i;

    g_Menu->pSelectionMenu->numCursors = 0;
    g_Menu->pSelectionMenu->numTexts = 0;

    for (i = 0; i < numOptions; i++) {
        // If we're on the currently selected option, add 0xD to the cursor texture ID
        // which is expected to be the texture ID of the active cursor
        if (i == selectedIndex) {
            cursorTextureID = pTextureIDs[i * 2] + 0xD;
        } else {
            cursorTextureID = pTextureIDs[i * 2];
        }

        g_Menu->pSelectionMenu->numCursors += func_8002675C(
            g_Menu->resources, cursorTextureID,
            &g_Menu->pSelectionMenu->polysCursors[g_Menu->pSelectionMenu->numCursors * 2],
            g_Menu->renderContext,
            160, 150,
            0x1000
        );

        g_Menu->pSelectionMenu->numTexts += func_8002675C(
            g_Menu->resources, pTextureIDs[i * 2 + 1],
            &g_Menu->pSelectionMenu->polysTexts[g_Menu->pSelectionMenu->numTexts * 2],
            g_Menu->renderContext,
            160, 150,
            0x1000
        );
    }

    g_Menu->pSelectionMenu->cursorsRenderCtx = g_Menu->renderContext;
    g_Menu->pSelectionMenu->textsRenderCtx = g_Menu->renderContext;

    ShopMenuMovePointerCursor(selectedIndex, 1);

    g_Menu->pManager->shouldRenderPointerCursor = TRUE;
}

void ShopMenuUpdateSubmenuTextures(u_char menuOffset) {
    int i;
    int textureID;

    g_Menu->pSubmenuPrims->polys0PrimCount = 0;
    g_Menu->pSubmenuPrims->polys500PrimCount = 0;

    for (i = 0; i < g_Menu->subMenuNumChoices; i++) {
        // The active option uses a different cursor texture
        if (i == g_Menu->subMenuChoice) {
            textureID = g_ShopSubmenuTextureIds[(menuOffset + g_Menu->mainMenuChoice) * 8 + i * 2] + 0xD;
        } else {
            textureID = g_ShopSubmenuTextureIds[(menuOffset + g_Menu->mainMenuChoice) * 8 + i * 2];
        }

        g_Menu->pSubmenuPrims->polys0PrimCount += func_8002675C(
            g_Menu->resources, textureID,
            &g_Menu->pSubmenuPrims->polys0[g_Menu->pSubmenuPrims->polys0PrimCount * 2],
            g_Menu->renderContext,
            160, 150,
            0x1000
        );

        g_Menu->pSubmenuPrims->polys500PrimCount += func_8002675C(
            g_Menu->resources, g_ShopSubmenuTextTextureIds[(menuOffset + g_Menu->mainMenuChoice) * 8 + i * 2],
            &g_Menu->pSubmenuPrims->polys500[g_Menu->pSubmenuPrims->polys500PrimCount * 2],
            g_Menu->renderContext,
            160, 150,
            0x1000
        );
    }

    g_Menu->pSubmenuPrims->polys0RenderCtx = g_Menu->renderContext;
    g_Menu->pSubmenuPrims->polys500RenderCtx = g_Menu->renderContext;

    ShopMenuMovePointerCursor(g_Menu->subMenuChoice + 3, 1);

    g_Menu->pManager->shouldRenderPointerCursor = TRUE;
}

u_char ShopMenuShopModeMenuHandleSelectedOption() {
    u_char isRunning;
    u_char result;

    isRunning = TRUE;
    switch (g_Menu->mainMenuChoice) {
        case MENU_CHOICE_EXIT:
            isRunning = FALSE;
            break;

        case MENU_CHOICE_SELL:
            result = ShopMenuSellModeMenu();
            break;

        case MENU_CHOICE_BUY:
            result = ShopMenuBuyMenu();
            break;
    }

    if (result) {
        ShopMenuStartCloseMenuTransition();
        ShopMenuSetStringListVisible(1, 4, g_Menu->optionStrings, &g_ShopModeMenuStringIds, g_Menu->pManager->shouldRenderStrings);
    }

    ShopMenuClearBuyMenuGraphics();

    // Render the selection menu as active again
    g_Menu->pSelectionMenu->isDisabled = FALSE;
    g_Menu->pSelectionMenu->isDisabledRendered = TRUE;

    g_Menu->pManager->shouldRenderPointerCursor = TRUE;
    g_Menu->pManager->unk3 = TRUE;
    g_Menu->mainMenuPrevChoice = 0xFF;
    g_Menu->pManager->shouldRenderSubmenuStrings = FALSE;

    return isRunning;
}

// Buy / Sell / Exit menu
void ShopMenuShopModeMenuMain(void) {
    u_char isRunning;

    isRunning = TRUE;
    g_Menu->mainMenuChoice = 2;

    ShopMenuInitializeShopModeSelectionMenu(4, &g_ShopModeSelectionTextureIds);

    ShopMenuSetStringListVisible(1, 4, g_Menu->optionStrings, &g_ShopModeMenuStringIds, g_Menu->pManager->shouldRenderStrings);

    while (isRunning) {
        ShopMenuUpdateAndRender();

        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);

                // Render the selection menu as disabled
                g_Menu->pSelectionMenu->isDisabled = TRUE;

                ShopMenuHidePointerCursor();
                ShopMenuSetStringListVisible(0, 4, g_Menu->optionStrings, &g_ShopModeMenuStringIds, g_Menu->pManager->shouldRenderStrings);
                g_Menu->pCursorPrims->shadeWidth = 0x4C;
                isRunning = ShopMenuShopModeMenuHandleSelectedOption();
                g_Menu->pCursorPrims->shadeWidth = 0x40;
                break;

            case MENU_INPUT_BACK:
                isRunning = FALSE;
                break;

            case MENU_INPUT_DOWN:
                if (g_Menu->mainMenuChoice) {
                    g_Menu->mainMenuChoice--;
                } else {
                    g_Menu->mainMenuChoice = 2;
                }
                break;

            case MENU_INPUT_UP:
                if (++g_Menu->mainMenuChoice >= 3) {
                    g_Menu->mainMenuChoice = 0;
                }
                break;
        }

        // Update the selected option to the active one if we need to
        if (g_Menu->mainMenuChoice != g_Menu->mainMenuPrevChoice) {
            ShopMenuUpdateShopModeSelectionMenu(3, g_Menu->mainMenuChoice, &g_ShopModeSelectionTextureIds);
            ShopMenuPositionSelectedString(4, g_Menu->optionStrings, &g_ShopModeMenuStringIds, &g_ShopModeMenuStringOffsets,
                g_Menu->pManager->shouldRenderStrings, g_Menu->mainMenuChoice, 0, 0
            );
            g_Menu->mainMenuPrevChoice = g_Menu->mainMenuChoice;
        }
    }
}

void ShopMenuMain(void) {
    ShopMenuUnk32CManager(MENU_DATA_INITIALIZE);
    ShopMenuSetManager(MENU_DATA_INITIALIZE);
    ShopMenuSelectionMenuManager(MENU_DATA_INITIALIZE);
    ShopMenuUnk354Manager(MENU_DATA_INITIALIZE);
    ShopMenuDressingRoomManager(MENU_DATA_INITIALIZE);
    ShopMenuUnk348Manager(MENU_DATA_INITIALIZE);
    ShopMenuUnk1E20Manager(MENU_DATA_INITIALIZE);
    ShopMenuShopManager(MENU_DATA_INITIALIZE);
    g_Menu->pSelectionMenu->vramRect.x = 0x2C0;
    g_Menu->pSelectionMenu->vramRect.y = 0x100;
    g_Menu->pSelectionMenu->vramRect.w = 0x140;
    g_Menu->pSelectionMenu->vramRect.h = 0xE0;
    g_Menu->pCursorPrims->shadeWidth = 0x40;
    ShopMenuInitialize();
    ShopMenuResetRenderContext();
    func_801C5EE8();
    ShopMenuInitializeBackgrounds();
    ShopMenuInitializeWindowBorders();
    ShopMenuInitializeShopData();
    g_Menu->shouldDrawMenu = TRUE;
    g_Menu->soundEffectsEnabled = TRUE;
    ShopMenuShopModeMenuMain();
    ShopMenuFree();
}

void ShopMenuComputeCharacterStats(MenuDressingRoom* pDressingRoom, u8 characterIndex) {
    GameCharacter* pCharacter;

    pCharacter = &g_GameState.characters[characterIndex];

    if (pCharacter->unk56 == 4) {
        pDressingRoom->statValues[0] = pCharacter->slots[0].attack + pCharacter->slots[3].attack;
    } else {
        pDressingRoom->statValues[0] = pCharacter->slots[0].attack + (pCharacter->attack + pCharacter->attackFromEquipment);
    }
    pDressingRoom->statValues[0] = pCharacter->slots[0].attack + (pCharacter->attack + pCharacter->attackFromEquipment);
    pDressingRoom->statValues[1] = pCharacter->hitPercentage + pCharacter->hitPercentageFromEquipment;
    pDressingRoom->statValues[2] = pCharacter->defenseFromAccessories + (pCharacter->defense + pCharacter->defenseFromEquipment);
    pDressingRoom->statValues[3] = pCharacter->evadePercentage + pCharacter->evadePercentageFromEquipment;
    pDressingRoom->statValues[4] = pCharacter->ether + pCharacter->etherFromEquipment;
    pDressingRoom->statValues[5] = 0x63;
    pDressingRoom->statValues[6] = pCharacter->etherDefense + pCharacter->etherDefenseFromEquipment;
    pDressingRoom->statValues[7] = 0xA;
    pDressingRoom->statValues[8] = pCharacter->agility + pCharacter->agilityFromEquipment;

    if (pDressingRoom->statValues[0] >= 1000) {
        pDressingRoom->statValues[0] = 999;
    }
    if (pDressingRoom->statValues[1] >= 100) {
        pDressingRoom->statValues[1] = 99;
    }
    if (pDressingRoom->statValues[2] >= 1000) {
        pDressingRoom->statValues[2] = 999;
    }
    if (pDressingRoom->statValues[3] >= 100) {
        pDressingRoom->statValues[3] = 99;
    }
    if (pDressingRoom->statValues[4] >= 1000) {
        pDressingRoom->statValues[4] = 999;
    }
    if (pDressingRoom->statValues[5] >= 100) {
        pDressingRoom->statValues[5] = 99;
    }
    if (pDressingRoom->statValues[6] >= 1000) {
        pDressingRoom->statValues[6] = 999;
    }
    if (pDressingRoom->statValues[7] >= 100) {
        pDressingRoom->statValues[7] = 99;
    }
    if (pDressingRoom->statValues[8] >= 100) {
        pDressingRoom->statValues[8] = 99;
    }
}

// Render Shop Stuff
void ShopMenuAddShopPrims(void) {
    int i;

    if (g_Menu->pManager->shouldRenderShop) {
        for (i = 0; i < 9; i++) {
            if (g_Menu->pShop->shouldRenderPortraitHighlight[i]) {
                AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->linesPortraitHighlight1[(i * 2) + g_Menu->renderContext]);
                AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->linesPortraitHighlight2[(i * 2) + g_Menu->renderContext]);
            }
        }

        ShopMenuRenderString(g_Menu->pShop->explanationsLen, g_Menu->pShop->polysExplanations, g_Menu->pShop->explanationsRenderCtx);
        ShopMenuRenderString(g_Menu->pShop->equipMarkPrimCount, g_Menu->pShop->polys2D0, g_Menu->pShop->equipMarkRenderCtx);
        ShopMenuRenderString(g_Menu->pShop->numPortraits, g_Menu->pShop->polysCharacterPortraits, g_Menu->pShop->portraitsRenderCtx);
        for (i = 0; i < 8; i++) {
            if (g_Menu->pShop->shouldRenderItemRows[i]) {
                ShopMenuRenderPolygons(1, g_Menu->pShop->strItemNames[i].vertices, g_Menu->pShop->strItemNames[i].polys, g_Menu->pShop->strItemNames[i].renderContext);
                ShopMenuRenderPolygons(1, g_Menu->pShop->strItemQuantities[i].vertices, g_Menu->pShop->strItemQuantities[i].polys, g_Menu->pShop->strItemQuantities[i].renderContext);
            }
        }

        if (g_Menu->pShop->shouldRenderItemDesc) {
            ShopMenuRenderPolygons(1, g_Menu->pShop->strItemDesc.vertices, g_Menu->pShop->strItemDesc.polys, g_Menu->pShop->strItemDesc.renderContext);
        }

        if (g_Menu->pShop->shouldRenderStr44B0) {
            ShopMenuRenderPolygons(1, g_Menu->pShop->str44B0.vertices, g_Menu->pShop->str44B0.polys, g_Menu->pShop->str44B0.renderContext);
        }

        if (g_Menu->pShop->shouldRenderStr45B0) {
            ShopMenuRenderPolygons(1, g_Menu->pShop->str45B0.vertices, g_Menu->pShop->str45B0.polys, g_Menu->pShop->str45B0.renderContext);
        }

        if (g_Menu->pShop->shouldRenderPrices) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->lines3BF0[g_Menu->renderContext]);
            ShopMenuRenderString(g_Menu->pShop->goldBeforeStrLen, g_Menu->pShop->polysGoldBefore, g_Menu->pShop->goldBeforeRenderCtx);
            ShopMenuRenderString(g_Menu->pShop->totalPriceStrLen, g_Menu->pShop->polysTotalPrice, g_Menu->pShop->totalPriceRenderCtx);
            ShopMenuRenderString(g_Menu->pShop->goldAfterStrLen, g_Menu->pShop->polysGoldAfter, g_Menu->pShop->goldAfterRenderCtx);
        }

        for (i = 0; i < 8; i++) {
            ShopMenuRenderString(g_Menu->pShop->inventoryPrimCounts[i], g_Menu->pShop->polysShopInventory[i * 4], g_Menu->pShop->inventoryRenderCtxs[i]);
        }

        for (i = 0; i < 9; i++) {
            ShopMenuRenderString(g_Menu->pShop->polys27B0PrimCounts[i], g_Menu->pShop->polys27B0[i], g_Menu->pShop->polys27B0RenderCtxs[i]);
            ShopMenuRenderString(g_Menu->pShop->polys3020PrimCounts[i], g_Menu->pShop->polys3020[i], g_Menu->pShop->polys3020RenderCtxs[i]);
        }
    }
    if (g_Menu->pManager->finalPriceRenderState == 1) {
        ShopMenuRenderString(g_Menu->pShop->finalPriceStrLen, g_Menu->pShop->polysFinalPrice, g_Menu->pShop->finalPriceRenderCtx);
    }
}

// Set the color for the stat change a piece of gear provides
void ShopMenuSetStatChangeColor(int numPolygons, POLY_FT4* pPolys, u_char mode) {
    int i;

    for (i = 0; i < numPolygons; i++) {
        SetShadeTex(&pPolys[i*2 + g_Menu->renderContext], 0);
        switch (mode) {
            case MENU_STAT_CHANGE_INCREASE:
                // Red
                setRGB0(&pPolys[i*2 + g_Menu->renderContext], 128, 64, 64);
                break;
            case MENU_STAT_CHANGE_DECREASE:
                // Blue
                setRGB0(&pPolys[i*2 + g_Menu->renderContext], 64, 64, 128);
                break;
        }
    }
}

void ShopMenuUpdateCharacterPortraits(void) {
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
                    g_CharacterPortraitX[numCharacters], 166,
                    0x1000
                );
                numCharacters++;
            }
        }

        g_Menu->pShop->portraitsRenderCtx = g_Menu->renderContext;
        ShopMenuUpdateAndRender();
    }
}

// Explanations are strings such as "Attack", "Defense", the "- <==> +" graphics.
void ShopMenuUpdateBuyMenuExplanationGraphics(void) {
    int i;

    g_Menu->pShop->explanationsLen = 0;
    for (i = 0; i < 4; i++) {
        g_Menu->pShop->explanationsLen += func_8002675C(
            g_Menu->resources, g_BuyMenuExplanationTextureIds[i],
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen * 2],
            g_Menu->renderContext,
            g_BuyMenuExplanationX[i], g_BuyMenuExplanationY[i],
            0x1000
        );
    }
    g_Menu->pShop->explanationsRenderCtx = g_Menu->renderContext;
}

void ShopMenuUpdateSellMenuExplanationGraphics(void) {
    int i;

    g_Menu->pShop->explanationsLen = 0;
    for (i = 0; i < 2; i++) {
        g_Menu->pShop->explanationsLen += func_8002675C(
            g_Menu->resources, g_SellMenuExplanationTextureIds[i],
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen * 2],
            g_Menu->renderContext,
            g_SellMenuExplanationX[i], g_SellMenuExplanationY[i],
            0x1000
        );
    }
    g_Menu->pShop->explanationsRenderCtx = g_Menu->renderContext;
}

void ShopMenuUpdateGoldGraphics(u_int goldBefore, u_int totalPrice, u_int goldAfter) {
    int i;
    u_char digit;

    ShopMenuParseNumberToString(goldBefore);
    g_Menu->pShop->goldBeforeStrLen = 0;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        digit = g_Menu->digits[i];
        if (digit != 0xFF) {
            g_Menu->pShop->goldBeforeStrLen += func_8002675C(
                g_Menu->resources, digit,
                &g_Menu->pShop->polysGoldBefore[g_Menu->pShop->goldBeforeStrLen * 2],
                g_Menu->renderContext,
                (i * 8) + g_GoldAmountTextX, g_GoldAmountTextY,
                0x1000
            );
        }
    }
    g_Menu->pShop->goldBeforeRenderCtx = g_Menu->renderContext;

    ShopMenuParseNumberToString(totalPrice);
    g_Menu->pShop->totalPriceStrLen = 0;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        digit = g_Menu->digits[i];
        if (digit != 0xFF) {
            g_Menu->pShop->totalPriceStrLen += func_8002675C(
                g_Menu->resources, digit,
                &g_Menu->pShop->polysTotalPrice[g_Menu->pShop->totalPriceStrLen * 2],
                g_Menu->renderContext,
                (i * 8) + g_TotalPriceTextX, g_TotalPriceTextY,
                0x1000
            );
        }
    }
    g_Menu->pShop->totalPriceRenderCtx = g_Menu->renderContext;

    ShopMenuParseNumberToString(goldAfter);
    g_Menu->pShop->goldAfterStrLen = 0;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        digit = g_Menu->digits[i];
        if (digit != 0xFF) {
            g_Menu->pShop->goldAfterStrLen += func_8002675C(
                g_Menu->resources, digit,
                &g_Menu->pShop->polysGoldAfter[g_Menu->pShop->goldAfterStrLen * 2],
                g_Menu->renderContext,
                (i * 8) + g_NewGoldAmountTextX, g_NewGoldAmountTextY,
                0x1000
            );
        }
    }
    g_Menu->pShop->goldAfterRenderCtx = g_Menu->renderContext;

    g_Menu->pShop->shouldRenderPrices = 1;
}

u_short ShopMenuGetCharacterEquippedItemFlags(u_char itemID, u_char itemType) {
    int i;
    int j;
    u_char isItemEquipped;
    u_short equippedFlags;

    equippedFlags = 0;

    if (itemID) {
        if (itemType != ITEM_TYPE_ITEM) {
            for (i = 0; i < 0x10; i++) {
                isItemEquipped = FALSE;

                if (g_Menu->availableCharacters[i]) {
                    switch (itemType) {
                        case ITEM_TYPE_WEAPON:
                            for (j = 0; j < 5; j++) {
                                if (itemID < 50) {
                                    if (g_GameState.characters[i].weaponId[j] == itemID) {
                                        isItemEquipped = TRUE;
                                        break;
                                    }
                                } else {
                                    if (g_GameState.characters[i].extraWeaponIds[j] == itemID) {
                                        isItemEquipped = TRUE;
                                        break;
                                    }
                                }
                            }
                            break;

                        case ITEM_TYPE_ACCESSORY:
                            for (j = 0; j < 3; j++) {
                                if (g_GameState.characters[i].accessorySlots[j] == itemID) {
                                    isItemEquipped = TRUE;
                                    break;
                                }
                            }
                            break;
                    }
                }

                if (isItemEquipped) {
                    equippedFlags |= ShopMenuGetCharacterBitMask(i);
                }
            }
        }
    }

    return equippedFlags;
}

INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc", func_801CDD14);
// Best attempt: 97.44%, 1920 bytes (retail 1900), and the 200-byte frame now matches.
// This is the shop twin of gear_shop_menu/main/render.c GearShopMenuRenderSellItemRows (that one
// is matched and is the reference for every shape in the body below); the residual is 5
// instructions plus ~30 stack-slot operands, all allocator.
// Levers already IN this body, keep them: `new_var = 0xD` immediately before the digitGlyphs[8]
// store with `0x32 + (new_var * i)` in the FIRST ShopMenuSetVertices y (that is what makes cc1
// CSE the 0x28 rect width into $s2 and grow the frame to retail's 200); `pItemAffordable[i] =
// FALSE` BEFORE the bzero (fixes the bzero delay-slot order); and the price needs TWO variables,
// `price` for the digit loop plus `itemPrice = price` right after the table load, which is what
// makes retail spill the price and reload it for the compare (`lw t2,0x68(sp); slt`). One
// `price` gives `slt v0,t1,s4` and no store, and scores 90.38%.
// Residual, all allocator: (1) retail's stack slots run a1, price, &rect, 0x1DB0, 0x4030,
// 0x32+13i, 13i, 0x3C30; this body gets a1, &rect, price, 0x1DB0, 13i, 0x4030, 0x32+13i,
// 0x3C30, so ~30 operands read 104 where retail reads 112 and back. Moving `pRect = &rect;`
// down next to the setRECT to flip the price/&rect order scored 91.46%.
// (2) Both ShopMenuSetVertices y arguments come out as `andi a2,<13i giv>,0xFFFF`; retail loads
// its 0x32-initialised accumulator and passes it with NO andi, so its giv holds the whole
// 0x32+13i sum. A `(u_short)` cast on the sum changed nothing, and an explicit loop-carried y in
// the for-init clause scored 89.97% at a 208 frame. func_801D05BC below has the same
// missing-andi residual: one known wall, not two.
// Two permuter runs from here (base 2132 -> 1108 -> 1048, ~2400 iterations) found only the two
// levers above; nothing else moved by hand.
/*
// Draws one row of the buy menu (item name, price digits and the quantity in stock) and marks
// each row in pItemAffordable with the 0x80 primitive attribute when the player can afford it.
void func_801CDD14(s32 scrollOffset, s32 newGoldAmount, u8* pItemAffordable) {
    RECT rect;
    u8 digitGlyphs[0xE];
    u8 decodedGlyphs[0xE];
    int new_var;
    int places[5];
    u32 polyColor;
    s32 quotient;
    u8 didRounding;
    RECT* pRect;
    s32 j;
    s32 k;
    u8 tensDigit;
    u8 onesDigit;
    s32 itemPrice;
    s32 price;
    int i;
    void* pWorkspace;

    places[0] = 1;
    places[1] = 10;
    places[2] = 100;
    places[3] = 1000;
    places[4] = 10000;

    pWorkspace = HeapAlloc(0x3F6, 0);

    for (i = 0; i < SHOP_MAX_ITEMS_IN_VIEW; i++) {
        pRect = &rect;
        pItemAffordable[i] = FALSE;
        bzero(digitGlyphs, 0xE);
        g_Menu->pShop->inventoryPrimCounts[i] = 0;
        polyColor = 0x1000;

        if (g_Menu->shopItemIDs[scrollOffset + i]) {
            switch (g_Menu->shopItemTypes[scrollOffset + i]) {
                case ITEM_TYPE_WEAPON:
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(
                        GetWeaponName(g_Menu->shopItemIDs[scrollOffset + i]), pWorkspace, 0x24, 0);
                    price = g_Menu->pDressingRoom->pWeaponsData[g_Menu->shopItemIDs[scrollOffset + i]].price;
                    itemPrice = price;
                    break;

                case ITEM_TYPE_ACCESSORY:
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(
                        GetAccessoryName(g_Menu->shopItemIDs[scrollOffset + i]), pWorkspace, 0x24, 0);
                    price = g_Menu->pDressingRoom->pAccessoriesData[g_Menu->shopItemIDs[scrollOffset + i]].price;
                    itemPrice = price;
                    break;

                case ITEM_TYPE_ITEM:
                    g_Menu->pShop->strItemNames[i].width = SystemRenderStringEntry(
                        GetItemName(g_Menu->shopItemIDs[scrollOffset + i]), pWorkspace, 0x24, 0);
                    price = g_Menu->pDressingRoom->pItemsData[g_Menu->shopItemIDs[scrollOffset + i]].price;
                    itemPrice = price;
                    break;
            }

            if (newGoldAmount >= itemPrice) {
                pItemAffordable[i] = 0x80;
            } else {
                pItemAffordable[i] = FALSE;
            }

            didRounding = FALSE;
            for (j = 4, k = 0; j > 0; k++, j--) {
                quotient = price / places[j];
                if (quotient || didRounding) {
                    ((u8*)pRect)[8 + k * 2] = quotient + 0x10;
                    didRounding = TRUE;
                    price -= (quotient * places[j]);
                } else {
                    ((u8*)pRect)[8 + k * 2] = 0xC3;
                }
            }
            new_var = 0xD;
            ((u8*)pRect)[16] = (price % 10) + 0x10;
            SystemDecodeGlyphCodesN((u16*)((u8*)&rect + 8), decodedGlyphs, 5);
            g_Menu->pShop->strItemQuantities[i].width = SystemRenderStringEntry(decodedGlyphs, pWorkspace, 0x24, 1);

            setRECT(&rect, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(pRect, pWorkspace);
            ShopMenuSetStringPrimitives(&g_Menu->pShop->strItemNames[i], i, 0x80, pItemAffordable[i] + 1);
            ShopMenuSetVertices(g_Menu->pShop->strItemNames[i].vertices, 0x24, 0x32 + (new_var * i),
                g_Menu->pShop->strItemNames[i].width, FONT_LETTER_HEIGHT);
            setRECT(&rect, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(pRect, pWorkspace);
            DrawSync(0);
            ShopMenuSetStringPrimitives(&g_Menu->pShop->strItemQuantities[i], i, 0x80, pItemAffordable[i] + 2);
            ShopMenuSetVertices(g_Menu->pShop->strItemQuantities[i].vertices, 0x8C, 0x32 + FONT_LETTER_HEIGHT * i,
                g_Menu->pShop->strItemQuantities[i].width, FONT_LETTER_HEIGHT);
            g_Menu->pShop->strItemNames[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->strItemNames[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->shouldRenderItemRows[i] = 1;

            if (g_Menu->pShop->curItemQuantities[scrollOffset + i]) {
                g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(
                    g_Menu->resources, 0xF1, g_Menu->pShop->polysShopInventory[i * 4],
                    g_Menu->renderContext, 0xB4, 0x36 + FONT_LETTER_HEIGHT * i, polyColor);

                tensDigit = g_Menu->pShop->curItemQuantities[scrollOffset + i] / 10;
                if (tensDigit) {
                    g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(
                        g_Menu->resources, tensDigit,
                        &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->inventoryPrimCounts[i] * 2],
                        g_Menu->renderContext, 0xBC, 0x36 + FONT_LETTER_HEIGHT * i, polyColor);
                }

                onesDigit = g_Menu->pShop->curItemQuantities[scrollOffset + i] % 10;
                g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(
                    g_Menu->resources, onesDigit,
                    &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->inventoryPrimCounts[i] * 2],
                    g_Menu->renderContext, 0xC4, 0x36 + FONT_LETTER_HEIGHT * i, polyColor);

                g_Menu->pShop->inventoryRenderCtxs[i] = g_Menu->renderContext;
            }
        } else {
            g_Menu->pShop->shouldRenderItemRows[i] = 0;
        }
    }

    HeapFree(pWorkspace);
}
*/

// The zero-store order, the chained defense assignment, the u16 characterIndex (Billy
// is character 4) and the slotDefense copy are load-bearing: other spellings change the
// instruction order. The three sum loops need the element address in a pointer local -
// indexing the tables inline makes cc1 coalesce the address into the index register.
void ShopMenuComputeStatChange(s32* pDelta, s8* pIsImprovement, u8 itemId, u8 itemType, u8 characterId) {
    s16 oldValue[2];
    s16 newValue[2];
    u8 changeDetected;
    u8 slotDefense;
    u16 characterIndex;
    u8* pSlots;
    u8 leastDefense;
    int smallest;
    MenuShopWeapon* pItemWeapon;
    MenuShopWeapon* pCurWeapon;
    MenuShopAccessory* pItemAccessory;
    MenuShopAccessory* pCurAccessory;
    int i;

    newValue[1] = 0;
    newValue[0] = 0;
    oldValue[1] = 0;
    oldValue[0] = 0;

    switch (itemType) {
        case ITEM_TYPE_WEAPON:
            // Billy carries four guns, so both his current and his preview value are sums
            characterIndex = 4;
            if (characterId != 4) {
                pCurWeapon = &g_Menu->pDressingRoom->pWeaponsData[g_GameState.characters[characterId].weaponId[0]];
                newValue[0] = pCurWeapon->attack + g_GameState.characters[characterId].attack;
                pCurWeapon = &g_Menu->pDressingRoom->pWeaponsData[itemId];
                oldValue[0] = pCurWeapon->attack + g_GameState.characters[characterId].attack;
            } else {
                pSlots = &g_GameState.characters[characterIndex].extraWeaponIds[0];
                for (i = 0; i < 4; i++) {
                    pCurWeapon = &g_Menu->pDressingRoom->pWeaponsData[pSlots[i]];
                    newValue[0] += pCurWeapon->attack;
                }

                pItemWeapon = &g_Menu->pDressingRoom->pWeaponsData[itemId];
                for (i = 0; i < 4; i++) {
                    pCurWeapon = &g_Menu->pDressingRoom->pWeaponsData[g_GameState.characters[characterId].extraWeaponIds[i]];
                    if (pCurWeapon->equipGroup == pItemWeapon->equipGroup && pCurWeapon->equipGroup != 5) {
                        oldValue[0] += pItemWeapon->attack;
                    } else {
                        oldValue[0] += pCurWeapon->attack;
                    }
                }
            }
            break;

        case ITEM_TYPE_ACCESSORY:
            changeDetected = TRUE;
            newValue[1] = oldValue[1] = g_GameState.characters[characterId].defense;

            for (i = 0; i < 3; i++) {
                pCurAccessory = &g_Menu->pDressingRoom->pAccessoriesData[g_GameState.characters[characterId].accessorySlots[i]];
                newValue[1] += pCurAccessory->defense;
            }

            // The candidate only replaces an accessory of the same group
            pItemAccessory = &g_Menu->pDressingRoom->pAccessoriesData[itemId];
            for (i = 0; i < 3; i++) {
                pCurAccessory = &g_Menu->pDressingRoom->pAccessoriesData[g_GameState.characters[characterId].accessorySlots[i]];
                if (pCurAccessory->equipGroup != 0 && pCurAccessory->equipGroup == pItemAccessory->equipGroup) {
                    changeDetected = FALSE;
                    oldValue[1] += pItemAccessory->defense;
                } else {
                    oldValue[1] += pCurAccessory->defense;
                }
            }

            leastDefense = 0xFF;
            if (changeDetected) {
                // No group match: the candidate replaces the accessory with the lowest defense
                for (i = 0; i < 3; i++) {
                    pCurAccessory = &g_Menu->pDressingRoom->pAccessoriesData[g_GameState.characters[characterId].accessorySlots[i]];
                    if (leastDefense >= pCurAccessory->defense) {
                        slotDefense = pCurAccessory->defense;
                        leastDefense = slotDefense;
                        smallest = i;
                    }
                }

                oldValue[1] = g_GameState.characters[characterId].defense + pItemAccessory->defense;
                for (i = 0; i < 3; i++) {
                    if (i != smallest) {
                        pCurAccessory = &g_Menu->pDressingRoom->pAccessoriesData[g_GameState.characters[characterId].accessorySlots[i]];
                        oldValue[1] += pCurAccessory->defense;
                    }
                }
            }
            break;
    }

    for (i = 0; i < 2; i++) {
        if (oldValue[i] >= newValue[i]) {
            pDelta[i] = oldValue[i] - newValue[i];
            pIsImprovement[i] = FALSE;
        } else {
            pDelta[i] = newValue[i] - oldValue[i];
            pIsImprovement[i] = TRUE;
        }
    }
}
u_short ShopMenuFindInventoryQuantity(u_char* pIDs, u_char* pValues, int numIDs, u_char id) {
    int i;
    u_char value;

    value = 0;
    for (i = 0; i < numIDs; i++) {
        if (pIDs[i] == id) {
            value = pValues[i];
            break;
        }
    }
    return value;
}

void ShopMenuSetQuantityHeldGraphics(u_char itemType, u_char itemID) {
    RECT rect;
    u_char digitBuffer[4];
    u_char stringBuffer[4];
    u_char* pIDList;
    u_char* pQuantityList;
    int listSize;
    u_int* pWorkspace;
    u_short quantityHeld;

    switch (itemType) {
        case ITEM_TYPE_WEAPON:
            pIDList = g_GameState.weaponIDs;
            pQuantityList = g_GameState.weaponQuantities;
            listSize = MAX_INVENTORY_WEAPONS;
            break;

        case ITEM_TYPE_ACCESSORY:
            pIDList = g_GameState.accessoryIDs;
            pQuantityList = g_GameState.accessoryQuantities;
            listSize = MAX_INVENTORY_ACCESSORIES;
            break;

        case ITEM_TYPE_ITEM:
            pIDList = g_GameState.itemIDs;
            pQuantityList = g_GameState.itemQuantities;
            listSize = MAX_INVENTORY_ITEMS;
            break;
    }

    quantityHeld = ShopMenuFindInventoryQuantity(pIDList, pQuantityList, listSize, itemID);
    g_SelectedItemQuantityHeld = quantityHeld;

    pWorkspace = HeapAlloc(0x3F6, 0);
    digitBuffer[1] = 0;
    digitBuffer[3] = 0;
    digitBuffer[0] = ((quantityHeld / 10) == 0) ? 0xC3 : (quantityHeld / 10) + 0x10;
    digitBuffer[2] = (quantityHeld % 10) + 0x10;

    SystemDecodeGlyphCodesN((u16*)digitBuffer, stringBuffer, 2);
    g_Menu->pShop->str45B0.width = SystemRenderStringEntry(stringBuffer, pWorkspace, 0x24, 1);
    setRECT(&rect, 0x198, 0xB4, 0x28, FONT_LETTER_HEIGHT);
    LoadImage(&rect, pWorkspace);
    DrawSync(0);

    ShopMenuSetStringPrimitives(&g_Menu->pShop->str45B0, 9, 0x80, 0x82);
    ShopMenuSetVertices(
        g_Menu->pShop->str45B0.vertices,
        0xF8, 0x8E,
        g_Menu->pShop->str45B0.width,
        FONT_LETTER_HEIGHT
    );
    g_Menu->pShop->str45B0.renderContext = g_Menu->renderContext;
    g_Menu->pShop->shouldRenderStr45B0 = 1;
    HeapFree(pWorkspace);
}

INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc", func_801CEB3C);
// 99.89%, same byte count as retail (1892) and the same frame: buy-menu item description
// update -- returns the item's price and draws the description string into strItemDesc, the
// 'E' marker on the portraits that already have it, and each portrait's two stat-change
// digits. The only residual is a GIV_TIE: retail puts the digit-column accumulator in $s6
// and the inducted polys3020[] base in $s7, this body has the two the other way round -- the
// same instruction multiset and word count, only those two register names and the preheader
// `li` order differ. Two permuter runs from these bodies found nothing (the 99.47% body:
// base 260 best 260 over 3031 iterations; this one: base 70 best 70 over 2306), and hand
// tries of the declaration order, the init order and the col-copy placement did not move it,
// so it is an allocator tie, not a source shape.
// The lever that got 99.47 -> 99.89: the column base must be initialised in the OUTER loop's
// init clause and be LAST there (`for (i = 0, j = 0, x = 0x49; i < 0x10; i++)`); as a
// statement before the loop, or listed first in the clause, the preheader `li` lands in the
// wrong slot.
// Load-bearing details found on the way: the two 3-digit loops are
// `for (k = 0, col = x; k < 3; )` with the read and bump in one expression
// (`digit = g_Menu->digits[6 + k++]`) and `col += 8` at the end of the body; the digit column
// must be an ACCUMULATOR (`x += 0x1A` before `j++`) or gcc emits an extra copy of the base and
// drops the 0x1000 constant out of $s8 (1904 bytes); delta[] must be zeroed per character
// before ShopMenuComputeStatChange; and the scalars' DECLARATION ORDER fixes the stack frame
// (price 0x38, equippedFlags 0x40, equipFlags 0x48, itemId 0x50, itemType 0x58).

/*
// Draws the buy menu's description of the item under the cursor (description text, the 'E'
// marker on the portraits that have it equipped, and each portrait's stat change digits)
// and returns the item's price. The third argument is passed by the caller but unused.
s32 func_801CEB3C(s32 itemIndex, s32 scrollOffset, u8* pUnused) {
    RECT rect;
    s32 delta[2];
    s8 isImprovement[2];
    u_int* pWorkspace;
    int price;
    u_short equippedFlags;
    u_short equipFlags;
    u_char itemId;
    u_char itemType;
    u_char digit;
    int i;
    int j;
    int k;
    int col;
    int x;

    equipFlags = 0;
    itemId = g_Menu->shopItemIDs[scrollOffset + itemIndex];
    itemType = g_Menu->shopItemTypes[scrollOffset + itemIndex];

    pWorkspace = HeapAlloc(0x618, 0);
    bzero(pWorkspace, 0x618);

    switch (itemType) {
        case ITEM_TYPE_WEAPON:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(
                GetStringEntry(g_Menu->pShop->pWeaponDescriptions, itemId), pWorkspace, 0x39, 0);
            price = g_Menu->pDressingRoom->pWeaponsData[itemId].price;
            equipFlags = g_Menu->pDressingRoom->pWeaponsData[itemId].equipFlags;
            break;

        case ITEM_TYPE_ACCESSORY:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(
                GetStringEntry(g_Menu->pShop->pAccessoryDescriptions, itemId), pWorkspace, 0x39, 0);
            price = g_Menu->pDressingRoom->pAccessoriesData[itemId].price;
            equipFlags = g_Menu->pDressingRoom->pAccessoriesData[itemId].equipFlags;
            break;

        case ITEM_TYPE_ITEM:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(
                GetStringEntry(g_Menu->pShop->pItemDescriptions, itemId), pWorkspace, 0x39, 0);
            price = g_Menu->pDressingRoom->pItemsData[itemId].price;
            break;
    }

    equippedFlags = ShopMenuGetCharacterEquippedItemFlags(itemId, itemType);
    setRECT(&rect, 0x140, 0x4E, 0x3C, 0xD);
    LoadImage(&rect, pWorkspace);
    DrawSync(0);

    ShopMenuSetStringPrimitives(&g_Menu->pShop->strItemDesc, 0, 0, 0);
    ShopMenuSetPolyFT4Coords(
        &g_Menu->pShop->strItemDesc.polys[g_Menu->renderContext],
        0x2C, 0x12, 0, 0x4E,
        g_Menu->pShop->strItemDesc.width,
        0xD
    );
    ShopMenuSetVertices(
        g_Menu->pShop->strItemDesc.vertices,
        0x2C, 0x12,
        g_Menu->pShop->strItemDesc.width,
        0xD
    );
    g_Menu->pShop->strItemDesc.renderContext = g_Menu->renderContext;
    HeapFree(pWorkspace);

    if (itemId) {
        g_Menu->pShop->shouldRenderItemDesc = 1;
    } else {
        g_Menu->pShop->shouldRenderItemDesc = 0;
    }

    g_Menu->pShop->equipMarkPrimCount = 0;

    for (i = 0, j = 0, x = 0x49; i < 0x10; i++) {
        if (!g_Menu->availableCharacters[i]) {
            continue;
        }

        if (ShopMenuIsCharacterFlagSet(equipFlags, i)) {
            g_Menu->pShop->shouldRenderPortraitHighlight[j] = 1;
        } else {
            g_Menu->pShop->shouldRenderPortraitHighlight[j] = 0;
        }

        // Portrait marker for the characters that already have the item equipped
        if (ShopMenuIsCharacterFlagSet(equippedFlags, i)) {
            g_Menu->pShop->equipMarkPrimCount += func_8002675C(
                g_Menu->resources, 0xE,
                &g_Menu->pShop->polys2D0[g_Menu->pShop->equipMarkPrimCount * 2],
                g_Menu->renderContext,
                g_CharacterPortraitX[j] + 0xE, 0xB4,
                0x1000
            );
        }

        g_Menu->pShop->polys27B0PrimCounts[j] = 0;
        g_Menu->pShop->polys3020PrimCounts[j] = 0;

        if (g_Menu->pShop->shouldRenderPortraitHighlight[j]) {
            delta[1] = 0;
            delta[0] = 0;
            ShopMenuComputeStatChange(delta, isImprovement, itemId, itemType, i);

            if (delta[0]) {
                ShopMenuParseNumberToString(delta[0]);
                for (k = 0, col = x; k < 3; ) {
                    digit = g_Menu->digits[6 + k++];
                    if (digit != 0xFF) {
                        g_Menu->pShop->polys27B0PrimCounts[j] += func_8002675C(
                            g_Menu->resources, digit,
                            &g_Menu->pShop->polys27B0[j][g_Menu->pShop->polys27B0PrimCounts[j] * 2],
                            g_Menu->renderContext,
                            col, 0xBE,
                            0x1000
                        );
                    }
                    col += 8;
                }
                ShopMenuSetStatChangeColor(
                    g_Menu->pShop->polys27B0PrimCounts[j], g_Menu->pShop->polys27B0[j], isImprovement[0]);
                g_Menu->pShop->polys27B0RenderCtxs[j] = g_Menu->renderContext;
            }

            if (delta[1]) {
                ShopMenuParseNumberToString(delta[1]);
                for (k = 0, col = x; k < 3; ) {
                    digit = g_Menu->digits[6 + k++];
                    if (digit != 0xFF) {
                        g_Menu->pShop->polys3020PrimCounts[j] += func_8002675C(
                            g_Menu->resources, digit,
                            &g_Menu->pShop->polys3020[j][g_Menu->pShop->polys3020PrimCounts[j] * 2],
                            g_Menu->renderContext,
                            col, 0xC6,
                            0x1000
                        );
                    }
                    col += 8;
                }
                ShopMenuSetStatChangeColor(
                    g_Menu->pShop->polys3020PrimCounts[j], g_Menu->pShop->polys3020[j], isImprovement[1]);
                g_Menu->pShop->polys3020RenderCtxs[j] = g_Menu->renderContext;
            }
        }

        x += 0x1A;
        j++;
    }

    g_Menu->pShop->equipMarkRenderCtx = g_Menu->renderContext;
    ShopMenuSetQuantityHeldGraphics(itemType, itemId);

    return price;
}
*/

void ShopMenuHandleBoughtItems(u_int newGoldnewWeaponAmount) {
    int i;
    int j;
    u_char itemNotInInventory;
    u_char newWeaponAmount;
    u_char newAccessorynewWeaponAmount;
    u_char newItemnewWeaponAmount;

    ShopMenuPlaySoundEffect(0xD1);

    g_GameState.gold = newGoldnewWeaponAmount;
    if (newGoldnewWeaponAmount > MAX_GOLD_AMOUNT) {
        g_GameState.gold = MAX_GOLD_AMOUNT;
    }

    for (i = 0; i < MAX_SHOP_ITEMS; i++) {
        // Did we buy any of this item?
        if (g_Menu->shopItemIDs[i] && g_Menu->pShop->curItemQuantities[i]) {
            switch (g_Menu->shopItemTypes[i]) {
                case ITEM_TYPE_WEAPON:
                    itemNotInInventory = TRUE;
                    for (j = 0; j < MAX_INVENTORY_WEAPONS; j++) {
                        if (g_GameState.weaponIDs[j] == g_Menu->shopItemIDs[i]) {
                            itemNotInInventory = FALSE;
                            newWeaponAmount = g_GameState.weaponQuantities[j] + g_Menu->pShop->curItemQuantities[i];
                            g_GameState.weaponQuantities[j] = newWeaponAmount;
                            if (newWeaponAmount > MAX_ITEM_QUANTITY) {
                                g_GameState.weaponQuantities[j] = MAX_ITEM_QUANTITY;
                            }
                        }
                    }

                    // Find an empty inventory slot and set it to the bought item
                    if (itemNotInInventory) {
                        for (j = 0; j < MAX_INVENTORY_WEAPONS; j++) {
                            if (g_GameState.weaponIDs[j] == 0) {
                                g_GameState.weaponIDs[j] = g_Menu->shopItemIDs[i];
                                g_GameState.weaponQuantities[j] = g_Menu->pShop->curItemQuantities[i];
                                break;
                            }
                        }
                    }
                    break;

                case ITEM_TYPE_ACCESSORY:
                    itemNotInInventory = TRUE;
                    for (j = 0; j < MAX_INVENTORY_ACCESSORIES; j++) {
                        if (g_GameState.accessoryIDs[j] == g_Menu->shopItemIDs[i]) {
                            itemNotInInventory = FALSE;
                            newAccessorynewWeaponAmount = g_GameState.accessoryQuantities[j] + g_Menu->pShop->curItemQuantities[i];
                            g_GameState.accessoryQuantities[j] = newAccessorynewWeaponAmount;
                            if (newAccessorynewWeaponAmount > MAX_ITEM_QUANTITY) {
                                g_GameState.accessoryQuantities[j] = MAX_ITEM_QUANTITY;
                            }
                        }
                    }

                    // Find an empty inventory slot and set it to the bought item
                    if (itemNotInInventory) {
                        for (j = 0; j < MAX_INVENTORY_ACCESSORIES; j++) {
                            if (g_GameState.accessoryIDs[j] == 0) {
                                g_GameState.accessoryIDs[j] = g_Menu->shopItemIDs[i];
                                g_GameState.accessoryQuantities[j] = g_Menu->pShop->curItemQuantities[i];
                                break;
                            }
                        }
                    }
                    break;

                case ITEM_TYPE_ITEM:
                    itemNotInInventory = TRUE;
                    for (j = 0; j < MAX_INVENTORY_ITEMS; j++) {
                        if (g_GameState.itemIDs[j] == g_Menu->shopItemIDs[i]) {
                            itemNotInInventory = FALSE;
                            newItemnewWeaponAmount = g_GameState.itemQuantities[j] + g_Menu->pShop->curItemQuantities[i];
                            g_GameState.itemQuantities[j] = newItemnewWeaponAmount;
                            if (newItemnewWeaponAmount > MAX_ITEM_QUANTITY) {
                                g_GameState.itemQuantities[j] = MAX_ITEM_QUANTITY;
                            }
                        }
                    }

                    // Find an empty inventory slot and set it to the bought item
                    if (itemNotInInventory) {
                        for (j = 0; j < MAX_INVENTORY_ITEMS; j++) {
                            if (g_GameState.itemIDs[j] == 0) {
                                g_GameState.itemIDs[j] = g_Menu->shopItemIDs[i];
                                g_GameState.itemQuantities[j] = g_Menu->pShop->curItemQuantities[i];
                                break;
                            }
                        }
                    }
                    break;
            }
        }
    }
}

// Parse and set graphics for final price for confirmation window
void ShopMenuSetFinalPriceGraphics(unsigned int number) {
    int i;
    u_char digit;

    ShopMenuParseNumberToString(number);
    g_Menu->pShop->finalPriceStrLen = 0;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        digit = g_Menu->digits[i];
        if (digit != 0xFF) {
            g_Menu->pShop->finalPriceStrLen += func_8002675C(
                g_Menu->resources, digit,
                &g_Menu->pShop->polysFinalPrice[g_Menu->pShop->finalPriceStrLen * 2],
                g_Menu->renderContext,
                107 + i * 8, 84,
                0x1000
            );
        }
    }
    g_Menu->pShop->finalPriceRenderCtx = g_Menu->renderContext;
    g_Menu->pManager->finalPriceRenderState = 2;
}

u_char ShopMenuBuyMenu(void) {
    u8 sp28[0x4]; // ???
    u_char isRunning;
    u_char shouldInitialize;
    u_char refreshGoldGraphics;
    int initialGold;
    int newGoldAmount;
    int totalPrice;
    int itemPrice;
    int curChoice;
    int prevChoice;
    int scrollOffset;
    int prevScrollOffset;
    int i;

    shouldInitialize = TRUE;
    isRunning = TRUE;
    refreshGoldGraphics = TRUE;

    curChoice = 0;
    prevChoice = 0xFF;
    scrollOffset = 0;
    prevScrollOffset = 0xFF;

    initialGold = g_GameState.gold;
    totalPrice = 0;
    newGoldAmount = g_GameState.gold;

    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        ShopMenuComputeCharacterStats(g_Menu->pDressingRoom, i);
        g_Menu->pShop->previousTotalAttacks[i] = g_Menu->pDressingRoom->statValues[0];
        g_Menu->pShop->previousTotalDefenses[i] = g_Menu->pDressingRoom->statValues[2];
    }

    bzero(g_Menu->pShop->curItemQuantities, MAX_SHOP_ITEMS);
    g_Menu->pSelectionMenu->isDisabled = TRUE;

    ShopMenuInitializeArrowCursor(0);

    while (isRunning) {
        ShopMenuUpdateAndRender();

        if ((scrollOffset != prevScrollOffset) || refreshGoldGraphics) {
            func_801CDD14(scrollOffset, newGoldAmount, &sp28);
            ShopMenuUpdateScrollBarHandle(12, 50, 60, g_ShopItemCount, scrollOffset);
        }

        // Did we change our currently selected item in any way?
        if ((curChoice != prevChoice) || (scrollOffset != prevScrollOffset)) {
            itemPrice = func_801CEB3C(curChoice, scrollOffset, &sp28);
            prevChoice = curChoice;
            prevScrollOffset = scrollOffset;
            g_Menu->pManager->shouldRenderShop = TRUE;
        }

        ShopMenuUpdateArrowCursor(curChoice, scrollOffset, 0, 0);

        if (shouldInitialize) {
            ShopMenuSetStringListVisible(1, 2, g_Menu->optionStrings, &g_ShopBuyMenuStringIds, g_Menu->pManager->shouldRenderStrings);
            ShopMenuPositionSelectedString(2, g_Menu->optionStrings, &g_ShopBuyMenuStringIds, &g_ShopSubmenuStringOffsets, g_Menu->pManager->shouldRenderStrings, 0, 0, 1);
            g_Menu->pManager->shouldRenderStrings[0] = FALSE;
            ShopMenuInitializeWindow(2, 0xC, 0x2A, 0xC4, 0x74, 0, 1, 4, 1);
            ShopMenuInitializeWindow(3, 0x20, 0xE, 0xFC, 0x14, 0, 1, 4, 0);
            ShopMenuInitializeWindow(5, 0xE0, 0x7A, 0x40, 0x24, 0, 1, 4, 0);
            ShopMenuStartOpenMenuTransition();
            ShopMenuUpdateCharacterPortraits();
            ShopMenuUpdateBuyMenuExplanationGraphics();
            shouldInitialize = FALSE;
            while (g_Menu->transitionEffectState != MENU_ANIMATION_DONE) {
                ShopMenuUpdateAndRender();
            }
            g_Menu->pManager->shouldRenderStrings[0] = TRUE;
        }

        if (refreshGoldGraphics) {
            ShopMenuUpdateGoldGraphics(initialGold, totalPrice, newGoldAmount);
            refreshGoldGraphics = FALSE;
        }

        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                if (totalPrice != 0) {
                    ShopMenuPlaySoundEffect(2);
                    g_Menu->pManager->shouldRenderShop = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    isRunning = FALSE;
                    g_Menu->pManager->shouldRenderStrings[0] = FALSE;
                    ShopMenuInitializePointerCursors(0);
                    ShopMenuSetFinalPriceGraphics(totalPrice);
                    if (ShopMenuConfirmationWindow(0x8F, 0xFF, 1) != MENU_CHOICE_NO) {
                        ShopMenuHandleBoughtItems(newGoldAmount);
                    } else {
                        isRunning = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[5] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                        prevScrollOffset = 0xFF;
                        prevChoice = 0xFF;
                        g_Menu->pManager->shouldRenderStrings[0] = TRUE;
                    }
                    ShopMenuFreePointerCursors();
                } else {
                    ShopMenuPlaySoundEffect(4);
                }
                break;

            case MENU_INPUT_BACK:
                isRunning = FALSE;
                g_Menu->pManager->shouldRenderStrings[0] = 0;
                if (totalPrice != 0) {
                    g_Menu->pManager->shouldRenderShop = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    ShopMenuInitializePointerCursors(0);
                    if (ShopMenuConfirmationWindow(0x8C, 0xFF, 1) == MENU_CHOICE_NO) {
                        isRunning = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[5] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                        prevScrollOffset = 0xFF;
                        prevChoice = 0xFF;
                        g_Menu->pManager->shouldRenderStrings[0] = TRUE;
                    }
                    ShopMenuFreePointerCursors();
                }
                ShopMenuUpdateAndRender();
                break;

            case MENU_INPUT_DOWN:
                curChoice++;

                // If we move beyond the items visible at a time, we need to scroll down
                if (curChoice >= SHOP_MAX_ITEMS_IN_VIEW) {
                    curChoice = SHOP_MAX_ITEMS_IN_VIEW - 1;
                    scrollOffset++;

                    // If we're at the bottom of the window, we undo our scrolling,
                    // since there's nothing mroe to scroll to
                    if ((g_ShopItemCount - SHOP_MAX_ITEMS_IN_VIEW) < scrollOffset) {
                        scrollOffset--;
                    }
                }
                break;

            case MENU_INPUT_UP:
                curChoice--;
                if (curChoice < 0) {
                    scrollOffset--;
                    curChoice = 0;
                    if (scrollOffset < 0) {
                        scrollOffset = 0;
                    }
                }
                break;

            // Increase item quantity
            case MENU_INPUT_RIGHT:
                if (sp28[curChoice] != 0) {
                    if (g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] + 1 + g_SelectedItemQuantityHeld < 100) {
                        g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] += 1;
                        totalPrice += itemPrice;
                        newGoldAmount -= itemPrice;
                        refreshGoldGraphics = TRUE;
                    }
                }
                break;

            // Decrease item quantity
            case MENU_INPUT_LEFT:
                if (g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] != 0) {
                    g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] -= 1;
                    totalPrice -= itemPrice;
                    newGoldAmount += itemPrice;
                    refreshGoldGraphics = TRUE;
                }
                break;
        }
    }

    g_Menu->pShop->shouldRenderStr45B0 = 0;
    return TRUE;
}

// Shop twin of gear_shop_menu/main/render.c GearShopMenuBuildSellItemDetails: draws the item description
// panel (name, half price and one portrait marker per character that can equip it) and
// returns the item's sell price. unk28 is the 0x10-byte hole the original frame has
// between the RECT and the digit buffer.
s32 ShopMenuUpdateItemDescription(u_char itemId, u_char itemType) {
    RECT rect;
    u8 unk28[0x10];
    u_char digitBuffer[0xE];
    u_char stringBuffer[0xE];
    int places[5];
    MenuShopWeapon* pWeapon;
    MenuShopAccessory* pAccessory;
    MenuShopItem* pItem;
    s32 equipFlags;
    u_short equippedFlags;
    s32 quotient;
    u8 didRounding;
    s32 j;
    s32 k;
    s32 price;
    s32 sellPrice;
    void* pWorkspace;

    equipFlags = 0;
    places[0] = 1;
    places[1] = 10;
    places[2] = 100;
    places[3] = 1000;
    places[4] = 10000;

    pWorkspace = HeapAlloc(0x618, 0);
    bzero(pWorkspace, 0x618);
    bzero(digitBuffer, 0xE);

    switch (itemType) {
        case ITEM_TYPE_WEAPON:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetWeaponName(itemId), pWorkspace, 0x39, 0);
            pWeapon = &g_Menu->pDressingRoom->pWeaponsData[itemId];
            sellPrice = pWeapon->price >> 1;
            equipFlags = pWeapon->equipFlags;
            price = sellPrice;
            break;

        case ITEM_TYPE_ACCESSORY:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetAccessoryName(itemId), pWorkspace, 0x39, 0);
            pAccessory = &g_Menu->pDressingRoom->pAccessoriesData[itemId];
            sellPrice = pAccessory->price >> 1;
            equipFlags = pAccessory->equipFlags;
            price = sellPrice;
            break;

        case ITEM_TYPE_ITEM:
            g_Menu->pShop->strItemDesc.width = SystemRenderStringEntry(GetItemName(itemId), pWorkspace, 0x39, 0);
            pItem = &g_Menu->pDressingRoom->pItemsData[itemId];
            sellPrice = pItem->price >> 1;
            price = sellPrice;
            break;
    }

    equippedFlags = ShopMenuGetCharacterEquippedItemFlags(itemId, itemType);

    didRounding = FALSE;
    for (j = 4, k = 0; j > 0; k++, j--) {
        quotient = price / places[j];
        if (quotient || didRounding) {
            digitBuffer[k * 2] = quotient + 0x10;
            didRounding = TRUE;
            price -= (quotient * places[j]);
        } else {
            digitBuffer[k * 2] = 0xC3;
        }
    }
    digitBuffer[8] = (price % 10) + 0x10;
    SystemDecodeGlyphCodesN((u16*)digitBuffer, stringBuffer, 5);

    g_Menu->pShop->str44B0.width = SystemRenderStringEntry(stringBuffer, pWorkspace, 0x39, 1);
    setRECT(&rect, 0x140, 0x4E, 0x3C, 0xD);
    LoadImage(&rect, pWorkspace);
    DrawSync(0);

    ShopMenuSetStringPrimitives(&g_Menu->pShop->strItemDesc, 0, 0, 0);
    ShopMenuSetStringPrimitives(&g_Menu->pShop->str44B0, 0, 0, 0);

    g_Menu->pShop->str44B0.polys[g_Menu->renderContext].clut = g_SystemPalette2;
    ShopMenuSetPolyFT4Coords(&g_Menu->pShop->strItemDesc.polys[g_Menu->renderContext], 0x2C, 0x12, 0, 0x4E,
        g_Menu->pShop->strItemDesc.width, FONT_LETTER_HEIGHT);
    ShopMenuSetPolyFT4Coords(&g_Menu->pShop->str44B0.polys[g_Menu->renderContext], 0x98, 0x12, 0, 0x4E,
        g_Menu->pShop->str44B0.width, FONT_LETTER_HEIGHT);
    ShopMenuSetVertices(g_Menu->pShop->strItemDesc.vertices, 0x2C, 0x12, g_Menu->pShop->strItemDesc.width,
        FONT_LETTER_HEIGHT);
    ShopMenuSetVertices(g_Menu->pShop->str44B0.vertices, 0x98, 0x12, g_Menu->pShop->str44B0.width,
        FONT_LETTER_HEIGHT);

    g_Menu->pShop->strItemDesc.renderContext = g_Menu->renderContext;
    g_Menu->pShop->str44B0.renderContext = g_Menu->renderContext;
    HeapFree(pWorkspace);

    if (itemId) {
        g_Menu->pShop->shouldRenderItemDesc = 1;
        g_Menu->pShop->shouldRenderStr44B0 = 1;
    } else {
        g_Menu->pShop->shouldRenderItemDesc = 0;
        g_Menu->pShop->shouldRenderStr44B0 = 0;
    }

    g_Menu->pShop->equipMarkPrimCount = 0;
    for (k = 0, j = 0; k < 0x10; k++) {
        if (g_Menu->availableCharacters[k] != 0) {
            if (ShopMenuIsCharacterFlagSet(equipFlags, k)) {
                g_Menu->pShop->shouldRenderPortraitHighlight[j] = 1;
            } else {
                g_Menu->pShop->shouldRenderPortraitHighlight[j] = 0;
            }

            if (ShopMenuIsCharacterFlagSet(equippedFlags, k)) {
                g_Menu->pShop->equipMarkPrimCount += func_8002675C(g_Menu->resources, 0xE,
                    &g_Menu->pShop->polys2D0[g_Menu->pShop->equipMarkPrimCount * 2], g_Menu->renderContext,
                    g_CharacterPortraitX[j] + 0xE, 0xB4, 0x1000);
            }

            j += 1;
        }
    }

    g_Menu->pShop->equipMarkRenderCtx = g_Menu->renderContext;

    return sellPrice;
}

INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc", func_801D05BC);
// Best attempt: 94.20%, 1652 bytes (retail 1628). Structure is retail's: the three-way compare
// tree, the shared switch tail, the digit loop, the two setRECT/LoadImage pairs and the two
// renderContext stores to the same field are all reproduced. The whole delta is ONE conversion:
// retail passes the y argument 0x32 + FONT_LETTER_HEIGHT * i with NO andi 0xFFFF although
// ShopMenuSetVertices takes u_short (proven by ShopMenuUpdateScrollBarHandle, whose matched asm
// andi's x/y/height at 0x2034-0x204C), and keeps the y as a word accumulator at 0x80($sp)
// (lw/addiu/sw at the loop tail). That missing andi needs one extra live register across both
// ShopMenuSetVertices calls, which costs the extra spill, the +8 frame (200 vs 192) and every
// register shift (ours s4 = &rect, retail s8/$fp = &rect).
// PROBED, do not redo: temporarily giving ShopMenuSetVertices `int` params (which only this unit
// would then disagree with) removes the andi and scores 98.35%, 1636 bytes - so the andi is the
// whole first delta, and a K&R / no-prototype declaration in the original translation unit (where
// this function probably lived alone) is the only mechanism that yields retail's untruncated
// argument. Not expressible once the definition is in the same TU as the call, so the u_short
// form is the ceiling here.
// The residual 8 bytes at 98.35%: cc1 makes a SECOND pseudo of &rect for the digit loop's giv base
// (&rect + 0x10, read as 0x28(a1) off it) and spills it to 120($sp), where retail keeps &rect in
// $fp and emits addiu a1, $fp, 0x10. Same allocno tie as gear_shop_menu/main/render.c
// GearShopMenuRenderSellItemRows (parked 98.73% there), whose note lists the pointer-local shapes already tried.
// Preset screened: 2.7.2-cdk 88.58 (the unit's 2.6.0 is best).
/*
void func_801D05BC(s32 scrollOffset, u8* pItemIds, u8* pItemTypes, u8* pItemSellQtys, u8* pItemQtys) {
    RECT rect;
    u8 digitBuffer[0xE];
    u8 stringBuffer[0xE];
    int places[5];
    s32 quotient;
    u8 didRounding;
    RECT* pRect;
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
    pRect = &rect;
    places[2] = 100;
    places[3] = 1000;
    places[4] = 10000;

    pWorkspace = HeapAlloc(0x3F6, 0);

    for (i = 0; i < SHOP_MAX_ITEMS_IN_VIEW; i++) {
        bzero(digitBuffer, 0xE);
        g_Menu->pShop->inventoryPrimCounts[i] = 0;
        if (pItemIds[scrollOffset + i]) {
            qty = pItemQtys[scrollOffset + i];
            itemType = pItemTypes[scrollOffset + i];
            switch (itemType) {
                case ITEM_TYPE_WEAPON:
                    g_Menu->pShop->strItemNames[i].width =
                        SystemRenderStringEntry(GetWeaponName(pItemIds[scrollOffset + i]), pWorkspace, 0x24, 0);
                    break;

                case ITEM_TYPE_ACCESSORY:
                    g_Menu->pShop->strItemNames[i].width =
                        SystemRenderStringEntry(GetAccessoryName(pItemIds[scrollOffset + i]), pWorkspace, 0x24, 0);
                    break;

                case ITEM_TYPE_ITEM:
                    g_Menu->pShop->strItemNames[i].width =
                        SystemRenderStringEntry(GetItemName(pItemIds[scrollOffset + i]), pWorkspace, 0x24, 0);
                    break;
            }

            didRounding = FALSE;
            for (j = 4, k = 0; j > 0; k++, j--) {
                quotient = qty / places[j];
                if (quotient || didRounding) {
                    ((u8*)pRect)[8 + k * 2] = quotient + 0x10;
                    didRounding = TRUE;
                    qty -= (quotient * places[j]);
                } else {
                    ((u8*)pRect)[8 + k * 2] = 0xC3;
                }
            }
            ((u8*)pRect)[16] = (qty % 10) + 0x10;
            SystemDecodeGlyphCodesN((u16*)((u8*)pRect + 8), stringBuffer, 5);
            g_Menu->pShop->strItemQuantities[i].width = SystemRenderStringEntry(stringBuffer, pWorkspace, 0x24, 1);
            setRECT(pRect, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(pRect, pWorkspace);
            ShopMenuSetStringPrimitives(&g_Menu->pShop->strItemNames[i], i, 0x80, 0x81);
            ShopMenuSetVertices(g_Menu->pShop->strItemNames[i].vertices, 0x24, 0x32 + FONT_LETTER_HEIGHT * i,
                g_Menu->pShop->strItemNames[i].width, FONT_LETTER_HEIGHT);
            setRECT(pRect, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(pRect, pWorkspace);
            DrawSync(0);
            ShopMenuSetStringPrimitives(&g_Menu->pShop->strItemQuantities[i], i, 0x80, 0x82);
            ShopMenuSetVertices(g_Menu->pShop->strItemQuantities[i].vertices, 0x8C, 0x32 + FONT_LETTER_HEIGHT * i,
                g_Menu->pShop->strItemQuantities[i].width, FONT_LETTER_HEIGHT);
            g_Menu->pShop->strItemNames[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->strItemNames[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->shouldRenderItemRows[i] = 1;

            if (pItemSellQtys[scrollOffset + i]) {
                g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, 0xE5,
                    g_Menu->pShop->polysShopInventory[i * 4], g_Menu->renderContext, 0xB4, 0x36 + FONT_LETTER_HEIGHT * i,
                    0x1000);

                tensDigit = pItemSellQtys[scrollOffset + i] / 10;
                if (tensDigit) {
                    g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, tensDigit,
                        &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->inventoryPrimCounts[i] * 2],
                        g_Menu->renderContext, 0xBC, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000);
                }

                onesDigit = pItemSellQtys[scrollOffset + i] % 10;
                g_Menu->pShop->inventoryPrimCounts[i] += func_8002675C(g_Menu->resources, onesDigit,
                    &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->inventoryPrimCounts[i] * 2], g_Menu->renderContext,
                    0xC4, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000);
                g_Menu->pShop->inventoryRenderCtxs[i] = g_Menu->renderContext;
            }
        } else {
            g_Menu->pShop->shouldRenderItemRows[i] = 0;
        }
    }

    HeapFree(pWorkspace);
}
*/


void ShopMenuHandleSoldItems(u_int newGoldAmount, u_char* pItemIds, u_char* pItemSellQtys, int listLength,
    u_char* pInventoryItemIds, u_char* pInventoryQtys, u_char* pItemTypes, u_char isBulkSale, u_char characterIndex) {
    int i;
    int j;
    u_char itemID;

    ShopMenuPlaySoundEffect(0xD1);

    g_GameState.gold = newGoldAmount;
    if (newGoldAmount > MAX_GOLD_AMOUNT) {
        g_GameState.gold = MAX_GOLD_AMOUNT;
    }

    if (isBulkSale) {
        for (i = 0; i < listLength; i++) {
            if (pItemIds[i]) {
                for (j = 0; j < listLength; j++) {
                    if (pItemIds[i] == pInventoryItemIds[j]) {
                        pInventoryQtys[j] -= pItemSellQtys[i];
                        if (pInventoryQtys[j] == 0) {
                            pInventoryItemIds[j] = 0;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < listLength; i++) {
            itemID = pItemIds[i];
            if (itemID && pItemSellQtys[i]) {
                switch (pItemTypes[i]) {
                    case ITEM_TYPE_WEAPON:
                        if (itemID < 50) {
                            g_GameState.characters[characterIndex].weaponId[0] = 0;
                        } else {
                            for (j = 0; j < 4; j++) {
                                if (itemID == g_GameState.characters[characterIndex].extraWeaponIds[j]) {
                                    g_GameState.characters[characterIndex].extraWeaponIds[j] = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ITEM_TYPE_ACCESSORY:
                        for (j = 0; j < 3; j++) {
                            if (pItemIds[i] == g_GameState.characters[characterIndex].accessorySlots[j]) {
                                g_GameState.characters[characterIndex].accessorySlots[j] = 0;
                                break;
                            }
                        }
                        break;
                }
            }
        }
    }
}

INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc", ShopMenuSellMenu);
// 98.61%, 2024 bytes (retail 2032), frame and every stack slot identical to retail. Ported from
// the 100% twin gear_shop_menu/main/render.c GearShopMenuSellMenu (same gcc 2.6.0 preset).
// Two findings that got it from 95.12 to 98.61:
// 1. The key-item test is NOT an inline 'a || b' expression: retail emits
//    andi/sltiu/addu v1,v0/andi 0xff, which only comes from a u_char local assigned TRUE, then
//    overwritten in an if/else. The '||' spelling compiles to srl/xori/andi (97.79%).
// 2. The coalesce loop counter must be a separate counter copied into itemCount after the loop
//    (the twin's itemType shape). Making itemCount itself the counter - the shape that matches
//    retail's per-iteration store - makes gcc give itemCount $s6 and spill pItemSellQtys to
//    0x70, shifting every later slot by 8 (95.95%).
// The whole remaining delta is ONE allocation: retail's counter is memory-resident and synced
// every iteration ("addiu t4,t4,1 / sw t4,0x90($fp) / lbu v0,0(a2) / lw t4,0x90($fp)", retail
// 0xC05C); ours keeps it in $a3 and stores it once in the jal delay slot, which is what the twin
// does and is 2 instructions short. Tried without effect: itemCount as the counter (95.95),
// itemCount declared first / last / with an initializer, 'itemCount = itemCount + 1', volatile
// itemCount (90.49), a separate int j counter, itemType as the only counter with no itemCount
// local, pItemSellQtys declared before the allocas, the '&&' guard form, moving
// 'itemCount = itemType' into the loop (98.65, register scramble). Two permuter runs (4 and 8
// minutes from this body, base 701 -> 666) found only a declaration-order no-op and a
// semantics-changing write to continueProcessing, so this needs a real lever, not more shapes.
/*
void ShopMenuSellMenu(s32 listLength, u8* pInventoryItemIds, u8* pInventoryQtys, int itemType, u8 flag1, u8* pGivenItemTypes, u8 flag2) {
    u8 continueProcessing = TRUE;
    u8 redrawWindows = TRUE;
    u8 redrawTransactionSummary = TRUE;
    u_char isSellable;
    int rowIndex = 0;
    int newRowIndex = 0xFF;
    int scrollOffset = 0;
    int newScrollOffset = 0xFF;

    u8 *pItemIds = alloca(listLength); // sp60
    u8 *pItemTypes = alloca(listLength); // sp68
    u8 *pItemSellQtys = alloca(listLength); // s7
    u8 *pItemIsOwned = alloca(listLength); // sp70

    u8 *pItemQtys; // sp78
    int currentGold; // sp80
    int projectedBalance; // s4
    int itemPrice; // sp88
    int cartTotal = 0; // s3
    int itemCount; // sp90
    int i;

    currentGold = g_GameState.gold;
    pItemQtys = alloca(listLength);
    projectedBalance = g_GameState.gold;

    for (i = 0; i < listLength; i++) {
        pItemQtys[i] = 0;
        pItemIsOwned[i] = 0;
        pItemSellQtys[i] = 0;
        pItemIds[i] = 0;
        if (flag1) {
            pItemTypes[i] = itemType;
        } else {
            pItemTypes[i] = pGivenItemTypes[i];
        }
    }

    // coalesce the item quantities, dropping the empty slots
    itemType = 0;
    for (i = 0; i < listLength; i++) {
        if (pInventoryItemIds[i]) {
            if (pInventoryQtys[i]) {
                isSellable = TRUE;
                if (pItemTypes[i] == ITEM_TYPE_ITEM) {
                    if (g_Menu->pDressingRoom->pItemsData[pInventoryItemIds[i]].displayFlags & 0x10) {
                        isSellable = FALSE;
                    } else {
                        isSellable = TRUE;
                    }
                }
                if (isSellable) {
                    pItemIds[itemType] = pInventoryItemIds[i];
                    pItemQtys[itemType] = pInventoryQtys[i];
                    pItemIsOwned[itemType] = 1;
                    itemType++;
                }
            }
        }
    }

    itemCount = itemType;
    ShopMenuInitializeArrowCursor(0);

    while (continueProcessing) {
        ShopMenuUpdateAndRender();

        if ((scrollOffset != newScrollOffset) || redrawTransactionSummary) {
            func_801D05BC(scrollOffset, pItemIds, pItemTypes, pItemSellQtys, pItemQtys);
            ShopMenuUpdateScrollBarHandle(0xC, 0x32, 0x3C, itemCount, scrollOffset);
        }

        if ((rowIndex != newRowIndex) || (scrollOffset != newScrollOffset)) {
            itemPrice = ShopMenuUpdateItemDescription(pItemIds[scrollOffset + rowIndex], pItemTypes[scrollOffset + rowIndex]);
            newRowIndex = rowIndex;
            newScrollOffset = scrollOffset;
            g_Menu->pManager->shouldRenderShop = 1;
        }

        ShopMenuUpdateArrowCursor(rowIndex, scrollOffset, 0, 0);

        if (redrawWindows) {
            ShopMenuInitializeWindow(2, 0xC, 0x2A, 0xC4, 0x74, 0, 1, 4, 1);
            ShopMenuInitializeWindow(3, 0x20, 0xE, 0xFC, 0x14, 0, 1, 4, 0);
            ShopMenuStartOpenMenuTransition();
            if (flag1) {
                ShopMenuUpdateCharacterPortraits();
            }
            while (g_Menu->transitionEffectState != MENU_ANIMATION_DONE) {
                ShopMenuUpdateAndRender();
            }
            ShopMenuUpdateSellMenuExplanationGraphics();
            redrawWindows = FALSE;
        }

        if (redrawTransactionSummary) {
            ShopMenuUpdateGoldGraphics(currentGold, cartTotal, projectedBalance);
            redrawTransactionSummary = FALSE;
        }

        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                if (cartTotal != 0) {
                    ShopMenuPlaySoundEffect(MENU_SOUND_CONFIRM);
                    g_Menu->pManager->shouldRenderShop = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    continueProcessing = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    ShopMenuInitializePointerCursors(0);
                    ShopMenuSetFinalPriceGraphics(cartTotal);
                    if (ShopMenuConfirmationWindow(MENU_STRING_CONFIRM_SELL, 0xFF, 1) != MENU_CHOICE_NO) {
                        ShopMenuHandleSoldItems(projectedBalance, pItemIds, pItemSellQtys, listLength, pInventoryItemIds, pInventoryQtys, pItemTypes, flag1, flag2);
                    } else {
                        continueProcessing = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        newScrollOffset = 0xFF;
                        newRowIndex = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                    }
                    ShopMenuFreePointerCursors();
                } else {
                    ShopMenuPlaySoundEffect(MENU_SOUND_ERROR);
                }
                break;

            case MENU_INPUT_BACK:
                continueProcessing = FALSE;
                if (cartTotal != 0) {
                    g_Menu->pManager->shouldRenderShop = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    ShopMenuInitializePointerCursors(0);
                    if (ShopMenuConfirmationWindow(0x92, 0xFF, 1) == MENU_CHOICE_NO) {
                        continueProcessing = TRUE;
                        g_Menu->pManager->shouldRenderWindow[2] = TRUE;
                        g_Menu->pManager->shouldRenderWindow[3] = TRUE;
                        g_Menu->pManager->scrollHandleActive = TRUE;
                        newScrollOffset = 0xFF;
                        newRowIndex = 0xFF;
                        g_Menu->pManager->shouldRenderArrowCursor[0] = TRUE;
                    }
                    ShopMenuFreePointerCursors();
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
                    pItemSellQtys[scrollOffset + rowIndex]++;
                    pItemQtys[scrollOffset + rowIndex]--;
                }
                break;

            case MENU_INPUT_LEFT:
                if (pItemIsOwned[scrollOffset + rowIndex] && pItemSellQtys[scrollOffset + rowIndex] - 1 >= 0) {
                    cartTotal -= itemPrice;
                    redrawTransactionSummary = 1;
                    pItemQtys[scrollOffset + rowIndex]++;
                    pItemSellQtys[scrollOffset + rowIndex]--;
                    projectedBalance -= itemPrice;
                }
                break;
        }
    }
}
*/


int ShopMenuSellEquipmentMenu(void) {
    u_char itemIDs[8];
    u_char itemQuantities[8];
    u_char itemFlags[8];
    u_char shouldExit;
    int curChoice;
    int prevChoice;
    int i;
    int j;
    int k;

    curChoice = 0;
    prevChoice = 1;
    shouldExit = MENU_SELL_EQUIPMENT_CONTINUE;

    ShopMenuUpdateCharacterPortraits();
    ShopMenuConfirmationWindowInitialize(0x98);

    while (shouldExit == MENU_SELL_EQUIPMENT_CONTINUE) {
        ShopMenuUpdateAndRender();

        switch (g_Menu->input) {
            case MENU_INPUT_BACK:
                shouldExit = MENU_SELL_EQUIPMENT_CANCEL;
                break;

            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);
                shouldExit = MENU_SELL_EQUIPMENT_CONFIRM;
                break;

            case MENU_INPUT_LEFT:
                curChoice--;
                if (curChoice < 0) {
                    curChoice = g_Menu->pShop->numPortraits - 1;
                }
                break;

            case MENU_INPUT_RIGHT:
                curChoice++;
                if (curChoice >= g_Menu->pShop->numPortraits) {
                    curChoice = 0;
                }
                break;
        }

        if (curChoice != prevChoice) {
            g_Menu->pShop->shouldRenderPortraitHighlight[curChoice] = 1;
            g_Menu->pShop->shouldRenderPortraitHighlight[prevChoice] = 0;
            prevChoice = curChoice;
        }
    }

    ShopMenuConfirmationWindowFree();

    if (shouldExit) {
        // Find the character index of the selected portrait
        i = 0;
        if (curChoice) {
            do {
                i++;
                if (g_Menu->availableCharacters[i]) {
                    curChoice--;
                }
            } while (curChoice != 0);
        }

        for (j = 0; j < 8; j++) {
            itemIDs[j] = 0;
            itemQuantities[j] = 1;
        }

        // Only the accessories the character has equipped can be sold here
        j = 0;
        for (k = 0; k < 3; k++) {
            if (g_GameState.characters[i].accessorySlots[k]) {
                itemIDs[j] = g_GameState.characters[i].accessorySlots[k];
                itemFlags[j] = 1;
                j++;
            }
        }

        ShopMenuSellMenu(8, itemIDs, itemQuantities, ITEM_TYPE_ACCESSORY, 0, itemFlags, (u_char) i);
    }

    return shouldExit;
}

void ShopMenuSellAccessoriesMenu(void) {
    ShopMenuSellMenu(
        MAX_INVENTORY_ACCESSORIES,
        g_GameState.accessoryIDs,
        g_GameState.accessoryQuantities,
        ITEM_TYPE_ACCESSORY,
        1,
        g_GameState.accessoryQuantities,
        0
    );
}

void ShopMenuSellWeaponsMenu(void) {
    ShopMenuSellMenu(
        MAX_INVENTORY_WEAPONS,
        g_GameState.weaponIDs,
        g_GameState.weaponQuantities,
        ITEM_TYPE_WEAPON,
        1,
        g_GameState.weaponQuantities,
        0
    );
}

void ShopMenuSellItemsMenu(void) {
    ShopMenuSellMenu(
        MAX_INVENTORY_ITEMS,
        g_GameState.itemIDs,
        g_GameState.itemQuantities,
        ITEM_TYPE_ITEM,
        1,
        g_GameState.itemQuantities,
        0
    );
}

void ShopMenuClearShopGraphics(u_char arg0, u_char arg1) {
    int i;

    g_Menu->pManager->shouldRenderShop = 0;
    g_Menu->pShop->shouldRenderItemDesc = 0;
    g_Menu->pShop->shouldRenderStr44B0 = 0;
    g_Menu->pShop->shouldRenderPrices = 0;
    g_Menu->pShop->explanationsLen = 0;
    g_Menu->pShop->equipMarkPrimCount = 0;
    g_Menu->pShop->numPortraits = 0;

    for (i = 0; i < 9; i++) {
        g_Menu->pShop->shouldRenderPortraitHighlight[i] = 0;
        g_Menu->pShop->polys27B0PrimCounts[i] = 0;
        g_Menu->pShop->polys3020PrimCounts[i] = 0;
    }

    for (i = 0; i < 8; i++) {
        g_Menu->pShop->shouldRenderItemRows[i] = 0;
        g_Menu->pShop->inventoryPrimCounts[i] = 0;
    }

    if (arg0) {
        ShopMenuFreeWindow(2);
        ShopMenuFreeWindow(3);
        if (arg1) {
            ShopMenuFreeWindow(5);
        }
        ShopMenuFreeScrollBarHandle();
        ShopMenuFreeArrowCursor(0);
    }
}

void ShopMenuSellModeMenuHandleSelectedOption(void) {
    u_char var_a0;

    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    g_Menu->pManager->shouldRenderSubmenuStrings = 0;
    ShopMenuSetStringListVisible(0, 4, g_Menu->optionStrings, &g_ShopSubmenuStringIds, g_Menu->pManager->shouldRenderStrings);

    var_a0 = 1;
    switch (g_Menu->subMenuChoice) {
        case MENU_CHOCIE_EQUIPMENT:
            var_a0 = ShopMenuSellEquipmentMenu();
            break;

        case MENU_CHOICE_ACCESSORIES:
            ShopMenuSellAccessoriesMenu();
            break;

        case MENU_CHOICE_WEAPONS:
            ShopMenuSellWeaponsMenu();
            break;

        case MENU_CHOICE_ITEMS:
            ShopMenuSellItemsMenu();
            break;
    }

    ShopMenuClearShopGraphics(var_a0, 0);
    g_Menu->pManager->shouldRenderSubmenuStrings = 1;
    g_Menu->pManager->shouldRenderPointerCursor = 1;
    g_Menu->pManager->unk3 = 1;
    ShopMenuSetStringListVisible(1, 4, g_Menu->optionStrings, &g_ShopSubmenuStringIds, g_Menu->pManager->shouldRenderStrings);
}

// Items / Weapons / Accessories / Equipment
int ShopMenuSellModeMenu() {
    u_char shouldInitialize;
    u_char isRunning;

    isRunning = TRUE;
    shouldInitialize = TRUE;
    g_Menu->subMenuChoice = MENU_CHOICE_ITEMS;
    g_Menu->subMenuPrevChoice = 0xFF;

    while (isRunning) {
        ShopMenuUpdateAndRender();

        if (shouldInitialize) {
            ShopMenuSetStringListVisible(1, 4, g_Menu->optionStrings, &g_ShopSubmenuStringIds, g_Menu->pManager->shouldRenderStrings);
            ShopMenuStartOpenMenuTransition();
            ShopMenuInitializeSubmenu(0);
            shouldInitialize = FALSE;
        }

        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            ShopMenuPositionSelectedString(4, g_Menu->optionStrings, &g_ShopSubmenuStringIds, &g_ShopSubmenuStringOffsets, g_Menu->pManager->shouldRenderStrings, g_Menu->subMenuChoice, 3, 0);
            ShopMenuUpdateSubmenuTextures(0);
            g_Menu->subMenuPrevChoice = g_Menu->subMenuChoice;
        }

        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);
                ShopMenuSellModeMenuHandleSelectedOption();
                g_Menu->subMenuPrevChoice = 0xFF;
                break;

            case MENU_INPUT_BACK:
                isRunning = FALSE;
                break;

            case MENU_INPUT_DOWN:
                if (g_Menu->subMenuChoice != 0) {
                    g_Menu->subMenuChoice--;
                } else {
                    g_Menu->subMenuChoice = g_Menu->subMenuNumChoices - 1;
                }
                break;

            case MENU_INPUT_UP:
                if (++g_Menu->subMenuChoice >= g_Menu->subMenuNumChoices) {
                    g_Menu->subMenuChoice = 0;
                }
                break;
        }
    }

    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    ShopMenuSetStringListVisible(0, 4, g_Menu->optionStrings, &g_ShopSubmenuStringIds, g_Menu->pManager->shouldRenderStrings);
    return TRUE;
}

void ShopMenuClearBuyMenuGraphics(void) {
    switch (g_Menu->mainMenuChoice) {
        case MENU_CHOICE_SELL:
            break;

        case MENU_CHOICE_BUY:
            ShopMenuClearShopGraphics(1, 1);
            break;
    }
}
