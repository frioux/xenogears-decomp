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
extern s32* D_8005917C;

extern u32* D_8005945C; // Pointer to the overlay's resources file
// .rodata: `const` matters - a non-const array lands in .data and leaves the
// switch jump tables as the only .rodata, which shifts the whole unit.
const u8 D_801C5000[0x10] = {0x42, 0x49, 0x53, 0x4C, 0x50, 0x53, 0x2D, 0x30,
                             0x30, 0x38, 0x30, 0x30, 0x00, 0x00, 0x03, 0x00};
extern SoundFile* D_8006259C; // SEDS file pointer

extern int D_801D1F50; // Number of items
extern s32 D_801D1FD0[];
extern s32 D_801D1FD4[];
extern s32 D_801D1FE8[];
extern u16 D_801D2260;


extern s32 D_801D1F54[];
// = {
//    MENU_TEX_BALL_CURSOR_1, MENU_TEX_STRING_BUY,
//    MENU_TEX_BALL_CURSOR_2, MENU_TEX_STRING_SELL,
//    MENU_TEX_BALL_CURSOR_3, MENU_TEX_STRING_EXIT
//    }

extern s32 D_801D1FCC[];
extern s32 D_801D1FD8[];

extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

extern u_char D_801D2018[]; // String IDs of the shop menu strings
extern int D_801D1F6C[]; // Cursor textures, 4 per submenu
extern int D_801D1F70[]; // Text textures, 4 per submenu

extern u_short D_801D21F0[];

extern s32 D_801D1FCC[];

extern s32 D_801D2248;
extern s32 D_801D224C;
extern s32 D_801D2250;
extern s32 D_801D2254;
extern s32 D_801D2258;
extern s32 D_801D225C;

// Character portrait X positions
extern int D_801D21CC[];

// Cursor stuff
extern int D_801D201C[];
extern int D_801D2094[];
extern int D_801D2114[];

extern int D_801D1FF8[];
extern int D_801D2008[];

// Argument passed to the menu by the outside.
// For the shop menu, this is expected to be the index
// of the shop to load and run.
extern u8 D_80059171;

extern s32 D_801D1F50;


extern int D_801D2194[];
extern int D_801D21B0[];

// Explanations stuff
extern u_char D_801D2210[]; // Buy menu explanation textures
// = { MENU_TEX_QUANTITY_EXPLANATION,
//     MENU_TEX_STRING_ATTACK,
//     MENU_TEX_STRING_DEFENSE,
//     MENU_TEX_MINUS
//    }

extern u_char D_801D2214[]; // Sell menu explanation textures
// = { MENU_TEX_QUANTITY_EXPLANATION, MENU_TEX_PLUS }

extern int D_801D2218[]; // Buy menu explanation graphics X positions
extern int D_801D2228[]; // Sell menu explanation graphics X positions
extern int D_801D2230[]; // Buy menu explanation graphics Y positions
extern int D_801D2240[]; // Sell menu explanation graphics Y positions


void func_801C5040(POLY_FT4* pPoly, short x, short y, u_char u, u_char v, short width, short height) {
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
    return D_801D21F0[maskIndex & 0xFF] & value;
}

u_short ShopMenuGetCharacterBitMask(int maskIndex) {
    return D_801D21F0[maskIndex & 0xFF];
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
        g_Menu->unk354 = HeapAlloc(sizeof(MenuUnk5), 0);
        bzero(g_Menu->unk354, sizeof(MenuUnk5));
        return;
    }
    HeapFree(g_Menu->unk354);
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
        g_Menu->unk348 = HeapAlloc(sizeof(MenuUnk1), 0);
        bzero(g_Menu->unk348, sizeof(MenuUnk1));
        return;
    }
    HeapFree(g_Menu->unk348);
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
// unit's .rodata - the 16-byte "BISLPS-00800" blob it reads (D_801C5000) used
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

    pFileHandle = D_8005945C;
    ResolveArchiveEntryPointers(pFileHandle);
    pData = LZSSHeapDecompress((void *)pFileHandle[1], 1);
    OpenTIM((void *)pData);
    ReadTIM(&g_Menu->menuUnk2->tim);
    memcpy(&g_Menu->menuUnk2->unk4FCE, D_801C5000, 0xD);
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
    g_Menu->pShopEntries = LZSSHeapDecompress((void *)pFileHandle[7], 0);
    HeapFree(pFileHandle);
}

void ShopMenuInitialize(void) {
    int flags;
    int characterID;
    int i;

    g_Menu->mainMenuChoice = 4;
    g_Menu->mainMenuPrevChoice = 0xFF;
    g_Menu->unk326 = 0x3C;
    g_Menu->unk334 = 0;
    g_Menu->unk335 = 0;

    flags = (g_GameState.unk1D30 & g_GameState.FrMask) & 0x7FF;
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

void func_801C5A7C(MenuString* pString, int index, s32 arg2, u32 attributes) {
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
            pString->unk7C = (index & 1);
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

            pString->unk7C = (attr & 0x7F) + 0xFF;
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

        MenuSetSystemPalette(pPoly, pString->unk7C);
    }

    pString->unk7F = 0;
}

void func_801C5CBC(MenuString* pStringList, u8* pStringIdList, s32 yOffset, s32 count) {
    s32 i;
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

        func_801C5A7C(&pStringList[i], i, yOffset, 0);
        func_801C5A7C(&pStringList[i + 1], i + 1, yOffset, 0);

        LoadImage(pRect, g_Menu->unk4E0[0].pVramBuffer);
        DrawSync(0);
    }
}

void func_801C5E6C(void) {
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
    g_Menu->unk4E0[0].pVramBuffer = HeapAlloc(0x38E, 0);
    func_801C5CBC(g_Menu->unk4E0, D_801D2018, 0, 4);
    func_801C5E6C();
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
        &g_Menu->unk348->polysPointerCursor, g_Menu->renderContext,
        D_801D2194[index], D_801D21B0[index],
        0x1000
    );
    g_Menu->unk348->cursorRenderContext = g_Menu->renderContext;

    if (arg1) {
        // A yellow -> black shaded rectangle of unk15B width,
        // with green lines around it. Seems to be cursor-related,
        // as the pointer cursor is set to same position
        setXY4(
            &g_Menu->unk348->polyG4s[g_Menu->renderContext],
            D_801D2194[index] + 20, D_801D21B0[index] - 36,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 36,
            D_801D2194[index] + 20, D_801D21B0[index] - 20,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 20
        );

        setXY3(
            &g_Menu->unk348->lines1[g_Menu->renderContext],
            D_801D2194[index] + 20, D_801D21B0[index] - 36,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 36,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 20
        );

        setXY3(
            &g_Menu->unk348->lines2[g_Menu->renderContext],
            D_801D2194[index] + 20, D_801D21B0[index] - 36,
            D_801D2194[index] + 20, D_801D21B0[index] - 20,
            D_801D2194[index] + (g_Menu->unk348->unk15B + 20), D_801D21B0[index] - 20
        );

        g_Menu->unk348->unk159 = g_Menu->renderContext;
        g_Menu->pManager->unk3 = 1;
    }
}

void func_801C6430(void) {
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
}

// ShopMenuSetPolyGradientColor
void func_801C6460(POLY_G4* pPoly, u_char red, u_char green, u_char blue) {
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

    func_801C6430();

    for (i = 0; i < 2; i++) {
        // Yellow -> Black gradient
        func_801C6460(&g_Menu->unk348->polyG4s[i], 128, 128, 0);
        SetSemiTrans(&g_Menu->unk348->polyG4s[i], 1);

        // Green lines
        SetLineF3(&g_Menu->unk348->lines1[i]);
        setRGB0(&g_Menu->unk348->lines1[i], 0, 64, 0);

        SetLineF3(&g_Menu->unk348->lines2[i]);
        setRGB0(&g_Menu->unk348->lines2[i], 0, 64, 0);

        // Backgrounds dimming screen
        SetPolyF4(&g_Menu->unk348->polysDimEffect[i]);
        setXY4(&g_Menu->unk348->polysDimEffect[i],
            0,   0,
            320, 0,
            0,   224,
            320, 224
        );
        setRGB0(&g_Menu->unk348->polysDimEffect[i], 128, 128, 128);
        SetSemiTrans(&g_Menu->unk348->polysDimEffect[i], 1);

        SetDrawMode(&g_Menu->unk348->drModes1[i], 0, 0, GetTPage(0, 0, 0x140, 128), &rect);
        SetDrawMode(&g_Menu->unk348->drawModeDimEffect[i], 0, 0, GetTPage(0, 2, 0x180, 0), &rect);
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
    pShopItemData = &g_Menu->pShopEntries[D_80059171 * 0x5C];

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
    D_801D1F50 = j;

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
                D_801D21CC[i], 0xA6,
                D_801D21CC[i] + 0x18, 0xA6,
                D_801D21CC[i] + 0x18, 0xBC
            );

            setXY3(
                &g_Menu->pShop->linesPortraitHighlight2[i*2 + j],
                D_801D21CC[i], 0xA6,
                D_801D21CC[i], 0xBC,
                D_801D21CC[i] + 0x18, 0xBC
            );
        }

        g_Menu->pShop->unk469C[i] = 0;
    }

    // White line between gold amounts?
    for (j = 0; j < 2; j++) {
        SetLineF2(&g_Menu->pShop->lines3BF0[j]);
        setRGB0(&g_Menu->pShop->lines3BF0[j], 255, 255, 255);
        setXY2(
            &g_Menu->pShop->lines3BF0[j],
            D_801D2250 - 8, D_801D2254 + 9,
            D_801D2250 + 0x4E, D_801D2254 + 9
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
    g_Menu->pManager->unk27[index] = FALSE;

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

    pWindow->unk710 = 0;
    pWindow->unk710 += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_TOP_LEFT,
        &pWindow->polysWindowBorderCorners,
        g_Menu->renderContext,
        0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_TOP_RIGHT,
        &pWindow->polysWindowBorderCorners[2 * pWindow->unk710],
        g_Menu->renderContext,
        0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_BOTTOM_LEFT,
        &pWindow->polysWindowBorderCorners[2 * pWindow->unk710],
        g_Menu->renderContext,
        0, 0, 0x1000
    );
    pWindow->unk710 += func_8002675C(
        g_Menu->resources,
        MENU_TEX_WINDOW_BORDER_BOTTOM_RIGHT,
        &pWindow->polysWindowBorderCorners[2 * pWindow->unk710],
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
    pWindow->unk714 = arg5;
    pWindow->zIndex = zIndex;
    pWindow->renderContext = g_Menu->renderContext;
    g_Menu->pManager->shouldRenderWindow[index] = TRUE;
}

void ShopMenuFreeWindow(u_char index) {
    g_Menu->pManager->shouldRenderWindow[index] = FALSE;
    g_Menu->pManager->unk27[index] = FALSE;
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
        pWindowParams->unk11 = 0;
        pWindowParams->x = x;
        pWindowParams->y = y;
        pWindowParams->width = width;
        pWindowParams->height = height;
        pWindowParams->unk8 = 0;
        pWindowParams->unkA = 0;
        g_Menu->pManager->unk27[windowIndex] = 1;
        pWindowParams->unk12 = arg6;
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
        if ((g_Menu->pManager->unk27[i]) && pWindowInfo->unk11 == 0) {

            flag = 0;

            if ((pWindowInfo->unk8 + 32) >= pWindowInfo->width) {
                pWindowInfo->unk8 = pWindowInfo->width;
                flag += 1;
            } else {
                pWindowInfo->unk8 += 32;
            }

            if ((pWindowInfo->unkA + 32) >= pWindowInfo->height) {
                pWindowInfo->unkA = pWindowInfo->height;
                flag += 1;
            } else {
                pWindowInfo->unkA += 32;
            }

            if (flag == 2) {
                pWindowInfo->unk11 = 1;
            }

            ShopMenuSetWindow(
                pWindowInfo->index,
                (pWindowInfo->x + (pWindowInfo->width / 2)) - (pWindowInfo->unk8 / 2),
                (pWindowInfo->y + (pWindowInfo->height / 2)) - (pWindowInfo->unkA / 2),
                pWindowInfo->unk8,
                pWindowInfo->unkA,
                pWindowInfo->unk12,
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

void func_801C8E28(void) {
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
            if (pWindow->unk714 == 0) {
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
                if (g_Menu->pCursors->unk144[i]) {
                    setXY4(
                        &g_Menu->pCursors->polysCursor[i * 2 + g_Menu->pCursors->renderContexts[i]],
                        D_801D2094[D_801D201C[g_Menu->menuUnk2->unk4F7C]] + 8,
                        D_801D2114[D_801D201C[g_Menu->menuUnk2->unk4F7C]] - 6,
                        D_801D2094[D_801D201C[g_Menu->menuUnk2->unk4F7C]] + 24,
                        D_801D2114[D_801D201C[g_Menu->menuUnk2->unk4F7C]] - 6,
                        D_801D2094[D_801D201C[g_Menu->menuUnk2->unk4F7C]] + 8,
                        D_801D2114[D_801D201C[g_Menu->menuUnk2->unk4F7C]] + 10,
                        D_801D2094[D_801D201C[g_Menu->menuUnk2->unk4F7C]] + 24,
                        D_801D2114[D_801D201C[g_Menu->menuUnk2->unk4F7C]] + 10
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

void func_801C9F7C(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->unk34[i]) {
            AddPrim(
                &g_Menu->pGfxEnv->ot[4],
                &g_Menu->unk4E0[i].polys[g_Menu->unk4E0[i].renderContext]
            );
        }
    }
}

void func_801CA00C(void) {
    int i;

    for (i = 0; i < 8; i++) {
        if (g_Menu->pManager->unkC[i]) {
            AddPrim(
                &g_Menu->pGfxEnv->ot[4],
                &g_Menu->unk6E0[i].polys[g_Menu->unk6E0[i].renderContext]
            );
        }
    }
}

void func_801CA09C(void) {
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

int func_801CA214(void) {
    int i;
    for (i = 5; i >= 0; i--) {}
    return i;
}

// Render confirmation window text?
void func_801CA22C(void) {
    long interpolated;
    long flag;
    MenuString* pString;
    int i;

    if (g_Menu->pManager->unk2B) {
        for (i = 0 ; i < 3; i++) {
            pString = g_Menu->unk1DE0[i];
            if (pString->unk7F) {
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
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->polysDimEffect[g_Menu->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->drawModeDimEffect[g_Menu->renderContext]);
}

void func_801CA404(void) {
    func_801C9F7C();
    func_801CA00C();
    func_801CA09C();
    func_801CA214();
    func_801CA22C();
}

void ShopMenuRenderSelectionMenu(void) {
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

        ShopMenuRenderString(g_Menu->pSelectionMenu->numTexts, g_Menu->pSelectionMenu->polysTexts, g_Menu->pSelectionMenu->textsRenderCtx);
        ShopMenuRenderString(g_Menu->pSelectionMenu->numCursors, g_Menu->pSelectionMenu->polysCursors, g_Menu->pSelectionMenu->cursorsRenderCtx);
    }
}

void func_801CAB0C(void) {
    if (g_Menu->pManager->unkA) {
        ShopMenuRenderString(g_Menu->unk354->unk1404, g_Menu->unk354->polys500, g_Menu->unk354->unk1409);
        ShopMenuRenderString(g_Menu->unk354->unk1400, g_Menu->unk354->polys0, g_Menu->unk354->unk1408);
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
        func_801CA404();
        func_801C8E28();
        ShopMenuRenderArrowCursors();
        func_801CCFF4();
        ShopMenuRenderScrollBarHandle();
        ShopMenuRenderSelectionMenu();
        func_801CAB0C();
        ShopMenuRenderWindows();
    }
    ShopMenuRenderBackgroundDim();
}

void ShopMenuPlaySoundEffect(u_char soundEffectId) {
    if (g_Menu->unk32A) {
        func_80039DB8((g_Menu->unk2E4->sedId << 16) | soundEffectId);
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

    if (func_80036410()) {
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
                g_Menu->unk1E94 = g_Menu->unk1E94 == 0;
                input = 12;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_L2) {
                g_Menu->unk1E95 += 1;
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
    if (*D_8005917C != -1) {
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
    MoveImage(&g_Menu->pSelectionMenu->unk1180, 0, renderContext * 0xE0);
    DrawOTag(&g_Menu->pGfxEnv->ot[0xF]);
}

void ShopMenuInitializePointerCursors(u_char mode) {
    int i;

    g_Menu->pCursors = HeapAlloc(sizeof(MenuPointerCursors), 0);
    bzero(g_Menu->pCursors, sizeof(MenuPointerCursors));

    switch (mode) {
        case 0:
            g_Menu->pManager->shouldRenderCursors = TRUE;
            g_Menu->pCursors->unk144[0] = TRUE;
            g_Menu->pCursors->unk144[1] = TRUE;
            /* fallthrough */
        case 2:
            for (i = 0; i < MENU_MAX_NUM_CURSORS; i++) {
                func_8002675C(
                    g_Menu->resources, MENU_TEX_POINTER_CURSOR,
                    &g_Menu->pCursors->polysCursor[i * 2], g_Menu->renderContext,
                    D_801D1FF8[i], D_801D2008[i],
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
    while (!pWindowParams->unk11) {
        ShopMenuUpdateAndRender();
    }

    for (i = 0; i < 4; i++) {
        g_Menu->unk1DE0[i] = HeapAlloc(sizeof(MenuString), 0);
        bzero(g_Menu->unk1DE0[i], sizeof(MenuString));

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
        func_801C5A7C(pString, i, 0, 0);
        ShopMenuSetVertices(pString->vertices, xPosition, (i * 16) + 0x50, pString->width, FONT_LETTER_HEIGHT);
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
    ShopMenuUpdateAndRender();
    ShopMenuUpdateAndRender();
}

void ShopMenuConfirmationWindowFree(void) {
    int i;

    if (g_Menu->pManager->shouldRenderWindow[4]) {
        ShopMenuFreeWindow(4);
        g_Menu->pManager->unk2B = 0;
        for (i = 0; i < 4; i++) {
            HeapFree(g_Menu->unk1DE0[i]);
        }
    }
    g_Menu->pManager->unk5B = 0;
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
    HeapFree(g_Menu->unk2E0);
    HeapFree(g_Menu->unk4E0[0].pVramBuffer);
    if (g_MenuDebugEnabled) {
        func_8003A094(g_Menu->unk2E4);
        ShopMenuUpdateAndRender();
        func_8003852C(g_Menu->unk2E4);
        ShopMenuUpdateAndRender();
        HeapFree(g_Menu->unk2E4);
    }
    HeapFree(g_Menu->pShopEntries);
    ShopMenuUnk1E20Manager(MENU_DATA_FREE);
    HeapFree(g_Menu);
}

void func_801CBC88(u_char mode, u_char numStrings, MenuString* pStrings, u_char* pStringIDs, u_char* pShouldRenderFlags) {
    int i;

    if (mode) {
        func_801C5CBC(pStrings, pStringIDs, 2, numStrings);
        return;
    }

    for (i = 0; i < numStrings; i++) {
        pShouldRenderFlags[i] = 0;
    }
}

void func_801CBCF0(u_char numStrings, MenuString* pStrings, u_char* pStringIDs, int* pXOffsets, u_char* pShouldRenderFlags, u_char index, u_char cursorOffset, u_char mode) {
    switch (mode) {
        case 0:
            func_801CBC88(0, numStrings, pStrings, pStringIDs, pShouldRenderFlags);
            setXYWH(
                &pStrings[index].polys[g_Menu->renderContext],
                D_801D2194[index + cursorOffset] + 0x16 + pXOffsets[index],
                D_801D21B0[index + cursorOffset] - 0x22,
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
    g_Menu->pSelectionMenu->unk1192 = FALSE;
    g_Menu->pSelectionMenu->unk1193 = FALSE;

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

void func_801CC278(u_char arg0) {
    int isValid;
    int i;
    int j;

    g_Menu->pSelectionMenu->unk1192 = 0;
    g_Menu->pSelectionMenu->unk1193 = 0;
    g_Menu->unk354->unk1400 = 0;
    g_Menu->unk354->unk1404 = 0;
    g_Menu->pManager->unkA = 1;

    isValid = TRUE;
    for (i = 1; i < 5; i++) {
        g_Menu->unk354->unk1400 = 0;
        g_Menu->subMenuNumChoices = 0;

        for (j = 0; j < i; j++) {
            if (D_801D1F6C[(arg0 + g_Menu->mainMenuChoice) * 8 + j * 2] != 0xFFFF) {
                g_Menu->unk354->unk1400 += func_8002675C(
                    g_Menu->resources,
                    D_801D1F6C[(arg0 + g_Menu->mainMenuChoice) * 8 + j * 2],
                    &g_Menu->unk354->polys0[g_Menu->unk354->unk1400 * 2],
                    g_Menu->renderContext,
                    0xA0, 0x96,
                    0x1000
                );
                g_Menu->subMenuNumChoices++;
            } else {
                isValid = FALSE;
            }
        }

        g_Menu->unk354->unk1408 = g_Menu->renderContext;

        if (isValid) {
            for (j = 0; j < 2; j++) {
                ShopMenuUpdateAndRender();
            }
        }

        g_Menu->unk354->unk1404 = 0;

        for (j = 0; j < i; j++) {
            if (D_801D1F6C[(arg0 + g_Menu->mainMenuChoice) * 8 + j * 2] != 0xFFFF) {
                g_Menu->unk354->unk1404 += func_8002675C(
                    g_Menu->resources,
                    D_801D1F6C[(arg0 + g_Menu->mainMenuChoice) * 8 + j * 2 + 1],
                    &g_Menu->unk354->polys500[g_Menu->unk354->unk1404 * 2],
                    g_Menu->renderContext,
                    0xA0, 0x96,
                    0x1000
                );
            }
        }

        g_Menu->unk354->unk1409 = g_Menu->renderContext;

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

    g_Menu->pManager->unk4 = TRUE;
}

void func_801CC720(u_char menuOffset) {
    int i;
    int textureID;

    g_Menu->unk354->unk1400 = 0;
    g_Menu->unk354->unk1404 = 0;

    for (i = 0; i < g_Menu->subMenuNumChoices; i++) {
        // The active option uses a different cursor texture
        if (i == g_Menu->subMenuChoice) {
            textureID = D_801D1F6C[(menuOffset + g_Menu->mainMenuChoice) * 8 + i * 2] + 0xD;
        } else {
            textureID = D_801D1F6C[(menuOffset + g_Menu->mainMenuChoice) * 8 + i * 2];
        }

        g_Menu->unk354->unk1400 += func_8002675C(
            g_Menu->resources, textureID,
            &g_Menu->unk354->polys0[g_Menu->unk354->unk1400 * 2],
            g_Menu->renderContext,
            160, 150,
            0x1000
        );

        g_Menu->unk354->unk1404 += func_8002675C(
            g_Menu->resources, D_801D1F70[(menuOffset + g_Menu->mainMenuChoice) * 8 + i * 2],
            &g_Menu->unk354->polys500[g_Menu->unk354->unk1404 * 2],
            g_Menu->renderContext,
            160, 150,
            0x1000
        );
    }

    g_Menu->unk354->unk1408 = g_Menu->renderContext;
    g_Menu->unk354->unk1409 = g_Menu->renderContext;

    ShopMenuMovePointerCursor(g_Menu->subMenuChoice + 3, 1);

    g_Menu->pManager->unk4 = TRUE;
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
        func_801CBC88(1, 4, g_Menu->unk6E0, &D_801D1FCC, g_Menu->pManager->unkC);
    }

    func_801D1F10();

    // Render the selection menu as active again
    g_Menu->pSelectionMenu->unk1192 = FALSE;
    g_Menu->pSelectionMenu->unk1193 = TRUE;

    g_Menu->pManager->unk4 = TRUE;
    g_Menu->pManager->unk3 = TRUE;
    g_Menu->mainMenuPrevChoice = 0xFF;
    g_Menu->pManager->unkA = FALSE;

    return isRunning;
}

// Buy / Sell / Exit menu
void ShopMenuShopModeMenuMain(void) {
    u_char isRunning;

    isRunning = TRUE;
    g_Menu->mainMenuChoice = 2;

    ShopMenuInitializeShopModeSelectionMenu(4, &D_801D1F54);

    func_801CBC88(1, 4, g_Menu->unk6E0, &D_801D1FCC, g_Menu->pManager->unkC);

    while (isRunning) {
        ShopMenuUpdateAndRender();

        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                ShopMenuPlaySoundEffect(2);

                // Render the selection menu as disabled
                g_Menu->pSelectionMenu->unk1192 = TRUE;

                func_801C6430();
                func_801CBC88(0, 4, g_Menu->unk6E0, &D_801D1FCC, g_Menu->pManager->unkC);
                g_Menu->unk348->unk15B = 0x4C;
                isRunning = ShopMenuShopModeMenuHandleSelectedOption();
                g_Menu->unk348->unk15B = 0x40;
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
            ShopMenuUpdateShopModeSelectionMenu(3, g_Menu->mainMenuChoice, &D_801D1F54);
            func_801CBCF0(4, g_Menu->unk6E0, &D_801D1FCC, &D_801D1FD8,
                g_Menu->pManager->unkC, g_Menu->mainMenuChoice, 0, 0
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
    g_Menu->pSelectionMenu->unk1180.x = 0x2C0;
    g_Menu->pSelectionMenu->unk1180.y = 0x100;
    g_Menu->pSelectionMenu->unk1180.w = 0x140;
    g_Menu->pSelectionMenu->unk1180.h = 0xE0;
    g_Menu->unk348->unk15B = 0x40;
    ShopMenuInitialize();
    ShopMenuResetRenderContext();
    func_801C5EE8();
    ShopMenuInitializeBackgrounds();
    ShopMenuInitializeWindowBorders();
    ShopMenuInitializeShopData();
    g_Menu->shouldDrawMenu = TRUE;
    g_Menu->unk32A = TRUE;
    ShopMenuShopModeMenuMain();
    ShopMenuFree();
}

void func_801CCE1C(MenuDressingRoom* pDressingRoom, u8 characterIndex) {
    GameCharacter* pCharacter;

    pCharacter = &g_GameState.characters[characterIndex];

    if (pCharacter->unk56 == 4) {
        pDressingRoom->unkB8[0] = pCharacter->unk0[4] + pCharacter->unk0[0x1C];
    } else {
        pDressingRoom->unkB8[0] = pCharacter->unk0[4] + (pCharacter->attack + pCharacter->unk0[0x28]);
    }
    pDressingRoom->unkB8[0] = pCharacter->unk0[4] + (pCharacter->attack + pCharacter->unk0[0x28]);
    pDressingRoom->unkB8[1] = pCharacter->hitPercentage + pCharacter->unk2D[1];
    pDressingRoom->unkB8[2] = pCharacter->unk2D[0] + (pCharacter->defense + pCharacter->unk0[0x29]);
    pDressingRoom->unkB8[3] = pCharacter->evadePercentage + pCharacter->unk2D[2];
    pDressingRoom->unkB8[4] = pCharacter->ether + pCharacter->unk0[0x2B];
    pDressingRoom->unkB8[5] = 0x63;
    pDressingRoom->unkB8[6] = pCharacter->etherDefense + pCharacter->etherDefenseFromEquipment;
    pDressingRoom->unkB8[7] = 0xA;
    pDressingRoom->unkB8[8] = pCharacter->agility + pCharacter->unk0[0x2A];

    if (pDressingRoom->unkB8[0] >= 1000) {
        pDressingRoom->unkB8[0] = 999;
    }
    if (pDressingRoom->unkB8[1] >= 100) {
        pDressingRoom->unkB8[1] = 99;
    }
    if (pDressingRoom->unkB8[2] >= 1000) {
        pDressingRoom->unkB8[2] = 999;
    }
    if (pDressingRoom->unkB8[3] >= 100) {
        pDressingRoom->unkB8[3] = 99;
    }
    if (pDressingRoom->unkB8[4] >= 1000) {
        pDressingRoom->unkB8[4] = 999;
    }
    if (pDressingRoom->unkB8[5] >= 100) {
        pDressingRoom->unkB8[5] = 99;
    }
    if (pDressingRoom->unkB8[6] >= 1000) {
        pDressingRoom->unkB8[6] = 999;
    }
    if (pDressingRoom->unkB8[7] >= 100) {
        pDressingRoom->unkB8[7] = 99;
    }
    if (pDressingRoom->unkB8[8] >= 100) {
        pDressingRoom->unkB8[8] = 99;
    }
}

// Render Shop Stuff
void func_801CCFF4(void) {
    int i;

    if (g_Menu->pManager->unk5A) {
        for (i = 0; i < 9; i++) {
            if (g_Menu->pShop->unk469C[i]) {
                AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->linesPortraitHighlight1[(i * 2) + g_Menu->renderContext]);
                AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->linesPortraitHighlight2[(i * 2) + g_Menu->renderContext]);
            }
        }

        ShopMenuRenderString(g_Menu->pShop->explanationsLen, g_Menu->pShop->polysExplanations, g_Menu->pShop->explanationsRenderCtx);
        ShopMenuRenderString(g_Menu->pShop->unk46A9, g_Menu->pShop->polys2D0, g_Menu->pShop->unk46A8);
        ShopMenuRenderString(g_Menu->pShop->numPortraits, g_Menu->pShop->polysCharacterPortraits, g_Menu->pShop->portraitsRenderCtx);
        for (i = 0; i < 8; i++) {
            if (g_Menu->pShop->unk4684[i]) {
                ShopMenuRenderPolygons(1, g_Menu->pShop->unk3C30[i].vertices, g_Menu->pShop->unk3C30[i].polys, g_Menu->pShop->unk3C30[i].renderContext);
                ShopMenuRenderPolygons(1, g_Menu->pShop->unk4030[i].vertices, g_Menu->pShop->unk4030[i].polys, g_Menu->pShop->unk4030[i].renderContext);
            }
        }

        if (g_Menu->pShop->unk46A7) {
            ShopMenuRenderPolygons(1, g_Menu->pShop->strItemDesc.vertices, g_Menu->pShop->strItemDesc.polys, g_Menu->pShop->strItemDesc.renderContext);
        }

        if (g_Menu->pShop->unk46B5) {
            ShopMenuRenderPolygons(1, g_Menu->pShop->str44B0.vertices, g_Menu->pShop->str44B0.polys, g_Menu->pShop->str44B0.renderContext);
        }

        if (g_Menu->pShop->unk4785) {
            ShopMenuRenderPolygons(1, g_Menu->pShop->str45B0.vertices, g_Menu->pShop->str45B0.polys, g_Menu->pShop->str45B0.renderContext);
        }

        if (g_Menu->pShop->unk46B2) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &g_Menu->pShop->lines3BF0[g_Menu->renderContext]);
            ShopMenuRenderString(g_Menu->pShop->goldBeforeStrLen, g_Menu->pShop->polysGoldBefore, g_Menu->pShop->goldBeforeRenderCtx);
            ShopMenuRenderString(g_Menu->pShop->totalPriceStrLen, g_Menu->pShop->polysTotalPrice, g_Menu->pShop->totalPriceRenderCtx);
            ShopMenuRenderString(g_Menu->pShop->goldAfterStrLen, g_Menu->pShop->polysGoldAfter, g_Menu->pShop->goldAfterRenderCtx);
        }

        for (i = 0; i < 8; i++) {
            ShopMenuRenderString(g_Menu->pShop->unk468C[i], g_Menu->pShop->polysShopInventory[i * 4], g_Menu->pShop->unk4694[i]);
        }

        for (i = 0; i < 9; i++) {
            ShopMenuRenderString(g_Menu->pShop->unk46BC[i], g_Menu->pShop->polys27B0[i], g_Menu->pShop->unk46CE[i]);
            ShopMenuRenderString(g_Menu->pShop->unk46C5[i], g_Menu->pShop->polys3020[i], g_Menu->pShop->unk46D7[i]);
        }
    }
    if (g_Menu->pManager->unk5B == 1) {
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
                    D_801D21CC[numCharacters], 166,
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
            g_Menu->resources, D_801D2210[i],
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen * 2],
            g_Menu->renderContext,
            D_801D2218[i], D_801D2230[i],
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
            g_Menu->resources, D_801D2214[i],
            &g_Menu->pShop->polysExplanations[g_Menu->pShop->explanationsLen * 2],
            g_Menu->renderContext,
            D_801D2228[i], D_801D2240[i],
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
                (i * 8) + D_801D2248, D_801D224C,
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
                (i * 8) + D_801D2250, D_801D2254,
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
                (i * 8) + D_801D2258, D_801D225C,
                0x1000
            );
        }
    }
    g_Menu->pShop->goldAfterRenderCtx = g_Menu->renderContext;

    g_Menu->pShop->unk46B2 = 1;
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
                                    if (g_GameState.characters[i].unk6A[j] == itemID) {
                                        isItemEquipped = TRUE;
                                        break;
                                    }
                                } else {
                                    if (g_GameState.characters[i].unk6F[j] == itemID) {
                                        isItemEquipped = TRUE;
                                        break;
                                    }
                                }
                            }
                            break;

                        case ITEM_TYPE_ACCESSORY:
                            for (j = 0; j < 3; j++) {
                                if (g_GameState.characters[i].unk74[j] == itemID) {
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

INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc", func_801CE480);
// 93.95%: shop twin of gear_shop_menu/main/render.c func_801D3558 (same signature and tail loop), but it
// computes the two preview stat pairs inline instead of calling the DressingRoom helpers.
// characterId is the 5th, stack argument; itemType 0 = weapon, 1 = accessory. The two s16[2] pairs are
// zeroed with four sh stores in the order 10(sp), 8(sp), 2(sp), 0(sp).
// The four loops must be written as plain `characters[].unk6F[i]` / `.unk74[i]` subscripts: gcc then
// strength-reduces them to the pointer walk (lbu/addiu/slt against base+N) the original has, and the
// preview values must go through MenuShopWeapon/MenuShopAccessory pointer locals so one pWeaponsData /
// pAccessoriesData entry feeds both .unk6/.unkE and .attack/.defense from one register.
// Remaining 66-instruction delta, all register allocation: gcc 2.7.2 here is one call-clobbered temp
// short of the original's set. It swaps pDelta/pIsImprovement out of $t5/$t6 into $t6/$t5, gives
// changeDetected $t1 instead of $t0 (so the tail loop's i lands in $a3 instead of $t0 and the
// accessory pointer in $t0 instead of $t1), and coalesces `addu $v0,$v0,$v1` / `lbu $v0,0xC($v0)`
// where the original coalesces the sum with the base (`addu $v1,$v0,$v1`). Everything else matches,
// including frame size, block order and the tail loop.
/*
void func_801CE480(s32* pDelta, s8* pIsImprovement, u8 itemId, u8 itemType, u8 characterId) {
    s16 oldValue[2];
    s16 newValue[2];
    u8 changeDetected;
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
            // Billy has four equipped guns, so both the current and the new value are sums
            if (characterId != 4) {
                pCurWeapon = &g_Menu->pDressingRoom->pWeaponsData[g_GameState.characters[characterId].unk6A[0]];
                newValue[0] = pCurWeapon->attack + g_GameState.characters[characterId].attack;
                pItemWeapon = &g_Menu->pDressingRoom->pWeaponsData[itemId];
                oldValue[0] = pItemWeapon->attack + g_GameState.characters[characterId].attack;
            } else {
                for (i = 0; i < 4; i++) {
                    newValue[0] += g_Menu->pDressingRoom->pWeaponsData[g_GameState.characters[4].unk6F[i]].attack;
                }

                pItemWeapon = &g_Menu->pDressingRoom->pWeaponsData[itemId];
                for (i = 0; i < 4; i++) {
                    pCurWeapon = &g_Menu->pDressingRoom->pWeaponsData[g_GameState.characters[characterId].unk6F[i]];
                    if (pCurWeapon->unk6 == pItemWeapon->unk6 && pCurWeapon->unk6 != 5) {
                        oldValue[0] += pItemWeapon->attack;
                    } else {
                        oldValue[0] += pCurWeapon->attack;
                    }
                }
            }
            break;

        case ITEM_TYPE_ACCESSORY:
            changeDetected = TRUE;
            oldValue[1] = g_GameState.characters[characterId].defense;
            newValue[1] = g_GameState.characters[characterId].defense;

            for (i = 0; i < 3; i++) {
                newValue[1] += g_Menu->pDressingRoom->pAccessoriesData[g_GameState.characters[characterId].unk74[i]].defense;
            }

            // The candidate only replaces an accessory of the same group
            pItemAccessory = &g_Menu->pDressingRoom->pAccessoriesData[itemId];
            for (i = 0; i < 3; i++) {
                pCurAccessory = &g_Menu->pDressingRoom->pAccessoriesData[g_GameState.characters[characterId].unk74[i]];
                if (pCurAccessory->unkE != 0 && pCurAccessory->unkE == pItemAccessory->unkE) {
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
                    pCurAccessory = &g_Menu->pDressingRoom->pAccessoriesData[g_GameState.characters[characterId].unk74[i]];
                    if (leastDefense >= pCurAccessory->defense) {
                        leastDefense = pCurAccessory->defense;
                        smallest = i;
                    }
                }

                oldValue[1] = g_GameState.characters[characterId].defense + pItemAccessory->defense;
                for (i = 0; i < 3; i++) {
                    if (i != smallest) {
                        oldValue[1] += g_Menu->pDressingRoom->pAccessoriesData[g_GameState.characters[characterId].unk74[i]].defense;
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
*/

u_short func_801CE8D8(u_char* pIDs, u_char* pValues, int numIDs, u_char id) {
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

void func_801CE91C(u_char itemType, u_char itemID) {
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

    quantityHeld = func_801CE8D8(pIDList, pQuantityList, listSize, itemID);
    D_801D2260 = quantityHeld;

    pWorkspace = HeapAlloc(0x3F6, 0);
    digitBuffer[1] = 0;
    digitBuffer[3] = 0;
    digitBuffer[0] = ((quantityHeld / 10) == 0) ? 0xC3 : (quantityHeld / 10) + 0x10;
    digitBuffer[2] = (quantityHeld % 10) + 0x10;

    func_80033B34((u16*)digitBuffer, stringBuffer, 2);
    g_Menu->pShop->str45B0.width = SystemRenderStringEntry(stringBuffer, pWorkspace, 0x24, 1);
    setRECT(&rect, 0x198, 0xB4, 0x28, FONT_LETTER_HEIGHT);
    LoadImage(&rect, pWorkspace);
    DrawSync(0);

    func_801C5A7C(&g_Menu->pShop->str45B0, 9, 0x80, 0x82);
    ShopMenuSetVertices(
        g_Menu->pShop->str45B0.vertices,
        0xF8, 0x8E,
        g_Menu->pShop->str45B0.width,
        FONT_LETTER_HEIGHT
    );
    g_Menu->pShop->str45B0.renderContext = g_Menu->renderContext;
    g_Menu->pShop->unk4785 = 1;
    HeapFree(pWorkspace);
}

INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc", func_801CEB3C);

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
    g_Menu->pManager->unk5B = 2;
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
        func_801CCE1C(g_Menu->pDressingRoom, i);
        g_Menu->pShop->unk46E0[i] = g_Menu->pDressingRoom->unkB8[0];
        g_Menu->pShop->unk4700[i] = g_Menu->pDressingRoom->unkB8[2];
    }

    bzero(g_Menu->pShop->curItemQuantities, MAX_SHOP_ITEMS);
    g_Menu->pSelectionMenu->unk1192 = TRUE;

    ShopMenuInitializeArrowCursor(0);

    while (isRunning) {
        ShopMenuUpdateAndRender();

        if ((scrollOffset != prevScrollOffset) || refreshGoldGraphics) {
            func_801CDD14(scrollOffset, newGoldAmount, &sp28);
            ShopMenuUpdateScrollBarHandle(12, 50, 60, D_801D1F50, scrollOffset);
        }

        // Did we change our currently selected item in any way?
        if ((curChoice != prevChoice) || (scrollOffset != prevScrollOffset)) {
            itemPrice = func_801CEB3C(curChoice, scrollOffset, &sp28);
            prevChoice = curChoice;
            prevScrollOffset = scrollOffset;
            g_Menu->pManager->unk5A = TRUE;
        }

        ShopMenuUpdateArrowCursor(curChoice, scrollOffset, 0, 0);

        if (shouldInitialize) {
            func_801CBC88(1, 2, g_Menu->unk6E0, &D_801D1FD4, g_Menu->pManager->unkC);
            func_801CBCF0(2, g_Menu->unk6E0, &D_801D1FD4, &D_801D1FE8, g_Menu->pManager->unkC, 0, 0, 1);
            g_Menu->pManager->unkC[0] = FALSE;
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
            g_Menu->pManager->unkC[0] = TRUE;
        }

        if (refreshGoldGraphics) {
            ShopMenuUpdateGoldGraphics(initialGold, totalPrice, newGoldAmount);
            refreshGoldGraphics = FALSE;
        }

        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                if (totalPrice != 0) {
                    ShopMenuPlaySoundEffect(2);
                    g_Menu->pManager->unk5A = FALSE;
                    g_Menu->pManager->shouldRenderWindow[2] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[3] = FALSE;
                    g_Menu->pManager->shouldRenderWindow[5] = FALSE;
                    g_Menu->pManager->scrollHandleActive = FALSE;
                    g_Menu->pManager->shouldRenderArrowCursor[0] = FALSE;
                    isRunning = FALSE;
                    g_Menu->pManager->unkC[0] = FALSE;
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
                        g_Menu->pManager->unkC[0] = TRUE;
                    }
                    ShopMenuFreePointerCursors();
                } else {
                    ShopMenuPlaySoundEffect(4);
                }
                break;

            case MENU_INPUT_BACK:
                isRunning = FALSE;
                g_Menu->pManager->unkC[0] = 0;
                if (totalPrice != 0) {
                    g_Menu->pManager->unk5A = FALSE;
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
                        g_Menu->pManager->unkC[0] = TRUE;
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
                    if ((D_801D1F50 - SHOP_MAX_ITEMS_IN_VIEW) < scrollOffset) {
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
                    if (g_Menu->pShop->curItemQuantities[scrollOffset + curChoice] + 1 + D_801D2260 < 100) {
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

    g_Menu->pShop->unk4785 = 0;
    return TRUE;
}

// Shop twin of gear_shop_menu/main/render.c func_801D1304: draws the item description
// panel (name, half price and one portrait marker per character that can equip it) and
// returns the item's sell price. unk28 is the 0x10-byte hole the original frame has
// between the RECT and the digit buffer.
s32 func_801CFF58(u_char itemId, u_char itemType) {
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
    func_80033B34((u16*)digitBuffer, stringBuffer, 5);

    g_Menu->pShop->str44B0.width = SystemRenderStringEntry(stringBuffer, pWorkspace, 0x39, 1);
    setRECT(&rect, 0x140, 0x4E, 0x3C, 0xD);
    LoadImage(&rect, pWorkspace);
    DrawSync(0);

    func_801C5A7C(&g_Menu->pShop->strItemDesc, 0, 0, 0);
    func_801C5A7C(&g_Menu->pShop->str44B0, 0, 0, 0);

    g_Menu->pShop->str44B0.polys[g_Menu->renderContext].clut = g_SystemPalette2;
    func_801C5040(&g_Menu->pShop->strItemDesc.polys[g_Menu->renderContext], 0x2C, 0x12, 0, 0x4E,
        g_Menu->pShop->strItemDesc.width, FONT_LETTER_HEIGHT);
    func_801C5040(&g_Menu->pShop->str44B0.polys[g_Menu->renderContext], 0x98, 0x12, 0, 0x4E,
        g_Menu->pShop->str44B0.width, FONT_LETTER_HEIGHT);
    ShopMenuSetVertices(g_Menu->pShop->strItemDesc.vertices, 0x2C, 0x12, g_Menu->pShop->strItemDesc.width,
        FONT_LETTER_HEIGHT);
    ShopMenuSetVertices(g_Menu->pShop->str44B0.vertices, 0x98, 0x12, g_Menu->pShop->str44B0.width,
        FONT_LETTER_HEIGHT);

    g_Menu->pShop->strItemDesc.renderContext = g_Menu->renderContext;
    g_Menu->pShop->str44B0.renderContext = g_Menu->renderContext;
    HeapFree(pWorkspace);

    if (itemId) {
        g_Menu->pShop->unk46A7 = 1;
        g_Menu->pShop->unk46B5 = 1;
    } else {
        g_Menu->pShop->unk46A7 = 0;
        g_Menu->pShop->unk46B5 = 0;
    }

    g_Menu->pShop->unk46A9 = 0;
    for (k = 0, j = 0; k < 0x10; k++) {
        if (g_Menu->availableCharacters[k] != 0) {
            if (ShopMenuIsCharacterFlagSet(equipFlags, k)) {
                g_Menu->pShop->unk469C[j] = 1;
            } else {
                g_Menu->pShop->unk469C[j] = 0;
            }

            if (ShopMenuIsCharacterFlagSet(equippedFlags, k)) {
                g_Menu->pShop->unk46A9 += func_8002675C(g_Menu->resources, 0xE,
                    &g_Menu->pShop->polys2D0[g_Menu->pShop->unk46A9 * 2], g_Menu->renderContext,
                    D_801D21CC[j] + 0xE, 0xB4, 0x1000);
            }

            j += 1;
        }
    }

    g_Menu->pShop->unk46A8 = g_Menu->renderContext;

    return sellPrice;
}

INCLUDE_ASM("asm/shop_menu/nonmatchings/main/misc", func_801D05BC);
// 89.44%: structurally right (block order, switch tail-merge, digit loop, two accumulators) but the
// frame is 8 bytes short: retail keeps &sp20 (the RECT) in $fp and spills the 0x3C30 + i*0x80
// accumulator to 0x90($sp), this version does the reverse and puts 0x3C30 in $s8, so i lands in $s2
// instead of $s3. Same tie-break as gear_shop_menu/main/render.c func_801D18F8 (see the note there).
// One mechanical delta worth attacking first: retail's y argument 0x32 + FONT_LETTER_HEIGHT * i is
// passed with NO andi 0xffff although ShopMenuSetVertices takes a u_short (ShopMenuInitializeScrollBar's
// andi on a2 proves the parameter type), and retail keeps a second i*0xD accumulator at 0x88($sp)
// for 0x36 + FONT_LETTER_HEIGHT * i. Here the u_short conversion of the y kills that reduction: gcc
// folds 0x32+0xD*i into 0xD*i plus an addiu and an andi, which is what generates one accumulator and
// the 8-byte-smaller frame. So retail's y expression did not need a truncation at all; find the source
// shape where gcc knows 0x32+0xD*i is in range (suspicion: the y comes from a local the original
// incremented itself, not from gcc's strength reduction) and the frame should become 0xC0.
// Ruled out since: a hoisted `int y = 0x32` incremented in the for-header and passed to both
// ShopMenuSetVertices calls is WORSE (88.23%): it does reproduce retail's 0x32-based accumulator
// but gcc still emits one andi 0xffff (CSE'd, hoisted above both calls), so the truncation is the
// blocker and not the shape of the accumulator. Landing it needs a y whose RTL is ALREADY a
// zero/sign extend (gcc only cancels a redundant `and` in that case), yet retail's slot is
// word-sized and incremented with lw/addiu/sw -- that combination is the open question here.
// The u_short parameter type itself is confirmed twice: andi 0xFFFF on the y argument of the
// matched ShopMenuUpdateScrollBarHandle (asm/shop_menu/matchings/main/misc/ShopMenuUpdateScrollBarHandle.s:2038)
// and func_8002675C's y is 0x36+0xD*i with a plain addiu, i.e. a wider parameter.
/*
void func_801D05BC(s32 scrollOffset, u8* pItemIds, u8* pItemTypes, u8* pItemSellQtys, u8* pItemQtys) {
    RECT rect;
    u8 digitBuffer[0xE];
    u8 stringBuffer[0xE];
    int places[5];
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

    for (i = 0; i < SHOP_MAX_ITEMS_IN_VIEW; i++) {
        bzero(digitBuffer, 0xE);
        g_Menu->pShop->unk468C[i] = 0;
        if (pItemIds[scrollOffset + i]) {
            qty = pItemQtys[scrollOffset + i];
            itemType = pItemTypes[scrollOffset + i];
            switch (itemType) {
                case ITEM_TYPE_WEAPON:
                    g_Menu->pShop->unk3C30[i].width = SystemRenderStringEntry(
                        GetWeaponName(pItemIds[scrollOffset + i]), pWorkspace, 0x24, 0);
                    break;

                case ITEM_TYPE_ACCESSORY:
                    g_Menu->pShop->unk3C30[i].width = SystemRenderStringEntry(
                        GetAccessoryName(pItemIds[scrollOffset + i]), pWorkspace, 0x24, 0);
                    break;

                case ITEM_TYPE_ITEM:
                    g_Menu->pShop->unk3C30[i].width = SystemRenderStringEntry(
                        GetItemName(pItemIds[scrollOffset + i]), pWorkspace, 0x24, 0);
                    break;
            }

            didRounding = FALSE;
            for (j = 4, k = 0; j > 0; k++, j--) {
                quotient = qty / places[j];
                if (quotient || didRounding) {
                    digitBuffer[k * 2] = quotient + 0x10;
                    didRounding = TRUE;
                    qty -= (quotient * places[j]);
                } else {
                    digitBuffer[k * 2] = 0xC3;
                }
            }
            digitBuffer[8] = (qty % 10) + 0x10;
            func_80033B34((u16*)digitBuffer, stringBuffer, 5);
            g_Menu->pShop->unk4030[i].width = SystemRenderStringEntry(stringBuffer, pWorkspace, 0x24, 1);
            setRECT(&rect, 0x180 + (i & 1) * 0x18, 0x80 + (i / 2) * FONT_LETTER_HEIGHT, 0x28, FONT_LETTER_HEIGHT);
            LoadImage(&rect, pWorkspace);
            func_801C5A7C(&g_Menu->pShop->unk3C30[i], i, 0x80, 0x81);
            ShopMenuSetVertices(g_Menu->pShop->unk3C30[i].vertices, 0x24, 0x32 + FONT_LETTER_HEIGHT * i,
                g_Menu->pShop->unk3C30[i].width, FONT_LETTER_HEIGHT);
            LoadImage(&rect, pWorkspace);
            DrawSync(0);
            func_801C5A7C(&g_Menu->pShop->unk4030[i], i, 0x80, 0x82);
            ShopMenuSetVertices(g_Menu->pShop->unk4030[i].vertices, 0x8C, 0x32 + FONT_LETTER_HEIGHT * i,
                g_Menu->pShop->unk4030[i].width, FONT_LETTER_HEIGHT);
            g_Menu->pShop->unk3C30[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->unk4030[i].renderContext = g_Menu->renderContext;
            g_Menu->pShop->unk4684[i] = 1;

            if (pItemSellQtys[scrollOffset + i]) {
                g_Menu->pShop->unk468C[i] += func_8002675C(g_Menu->resources, 0xE5,
                    g_Menu->pShop->polysShopInventory[i * 4], g_Menu->renderContext, 0xB4,
                    0x36 + FONT_LETTER_HEIGHT * i, 0x1000);

                tensDigit = pItemSellQtys[scrollOffset + i] / 10;
                if (tensDigit) {
                    g_Menu->pShop->unk468C[i] += func_8002675C(g_Menu->resources, tensDigit,
                        &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->unk468C[i] * 2],
                        g_Menu->renderContext, 0xBC, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000);
                }

                onesDigit = pItemSellQtys[scrollOffset + i] % 10;
                g_Menu->pShop->unk468C[i] += func_8002675C(g_Menu->resources, onesDigit,
                    &g_Menu->pShop->polysShopInventory[i * 4][g_Menu->pShop->unk468C[i] * 2],
                    g_Menu->renderContext, 0xC4, 0x36 + FONT_LETTER_HEIGHT * i, 0x1000);
                g_Menu->pShop->unk4694[i] = g_Menu->renderContext;
            }
        } else {
            g_Menu->pShop->unk4684[i] = 0;
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
                            g_GameState.characters[characterIndex].unk6A[0] = 0;
                        } else {
                            for (j = 0; j < 4; j++) {
                                if (itemID == g_GameState.characters[characterIndex].unk6F[j]) {
                                    g_GameState.characters[characterIndex].unk6F[j] = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ITEM_TYPE_ACCESSORY:
                        for (j = 0; j < 3; j++) {
                            if (pItemIds[i] == g_GameState.characters[characterIndex].unk74[j]) {
                                g_GameState.characters[characterIndex].unk74[j] = 0;
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
// 95.12%: ported from the 100% twin gear_shop_menu/main/render.c GearShopMenuSellMenu, so the
// block order, the five alloca'd arrays and the switch bodies are right. The remaining delta is
// one callee-saved register: retail keeps pItemSellQtys in $s7 and redrawTransactionSummary in
// $s6, this version spills pItemSellQtys to 0x70($fp) and gives $s7 to redrawTransactionSummary,
// which also pushes i/itemCount into callee-saved registers ($s6/$t3 vs retail's $a3/$t4) and
// moves the u32 locals up 8 bytes (currentGold at 0x88, not 0x80). Frame size (0xD0) is right.
/*
void ShopMenuSellMenu(s32 listLength, u8* pInventoryItemIds, u8* pInventoryQtys, int itemType, u8 flag1, u8* pGivenItemTypes, u8 flag2) {
    u8 continueProcessing = TRUE;
    u8 redrawWindows = TRUE;
    u8 redrawTransactionSummary = TRUE;
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
    itemCount = 0;
    for (i = 0; i < listLength; i++) {
        if (pInventoryItemIds[i]) {
            if (pInventoryQtys[i]) {
                if ((pItemTypes[i] != ITEM_TYPE_ITEM) || ((g_Menu->pDressingRoom->pItemsData[pInventoryItemIds[i]].unk4[2] & 0x10) == 0)) {
                    pItemIds[itemCount] = pInventoryItemIds[i];
                    pItemQtys[itemCount] = pInventoryQtys[i];
                    pItemIsOwned[itemCount] = 1;
                    itemCount++;
                }
            }
        }
    }

    ShopMenuInitializeArrowCursor(0);

    while (continueProcessing) {
        ShopMenuUpdateAndRender();

        if ((scrollOffset != newScrollOffset) || redrawTransactionSummary) {
            func_801D05BC(scrollOffset, pItemIds, pItemTypes, pItemSellQtys, pItemQtys);
            ShopMenuUpdateScrollBarHandle(0xC, 0x32, 0x3C, itemCount, scrollOffset);
        }

        if ((rowIndex != newRowIndex) || (scrollOffset != newScrollOffset)) {
            itemPrice = func_801CFF58(pItemIds[scrollOffset + rowIndex], pItemTypes[scrollOffset + rowIndex]);
            newRowIndex = rowIndex;
            newScrollOffset = scrollOffset;
            g_Menu->pManager->unk5A = 1;
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
                    g_Menu->pManager->unk5A = FALSE;
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
                    g_Menu->pManager->unk5A = FALSE;
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
            g_Menu->pShop->unk469C[curChoice] = 1;
            g_Menu->pShop->unk469C[prevChoice] = 0;
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
            if (g_GameState.characters[i].unk74[k]) {
                itemIDs[j] = g_GameState.characters[i].unk74[k];
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

void func_801D1968(u_char arg0, u_char arg1) {
    int i;

    g_Menu->pManager->unk5A = 0;
    g_Menu->pShop->unk46A7 = 0;
    g_Menu->pShop->unk46B5 = 0;
    g_Menu->pShop->unk46B2 = 0;
    g_Menu->pShop->explanationsLen = 0;
    g_Menu->pShop->unk46A9 = 0;
    g_Menu->pShop->numPortraits = 0;

    for (i = 0; i < 9; i++) {
        g_Menu->pShop->unk469C[i] = 0;
        g_Menu->pShop->unk46BC[i] = 0;
        g_Menu->pShop->unk46C5[i] = 0;
    }

    for (i = 0; i < 8; i++) {
        g_Menu->pShop->unk4684[i] = 0;
        g_Menu->pShop->unk468C[i] = 0;
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

    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    g_Menu->pManager->unkA = 0;
    func_801CBC88(0, 4, g_Menu->unk6E0, &D_801D1FD0, g_Menu->pManager->unkC);

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

    func_801D1968(var_a0, 0);
    g_Menu->pManager->unkA = 1;
    g_Menu->pManager->unk4 = 1;
    g_Menu->pManager->unk3 = 1;
    func_801CBC88(1, 4, g_Menu->unk6E0, &D_801D1FD0, g_Menu->pManager->unkC);
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
            func_801CBC88(1, 4, g_Menu->unk6E0, &D_801D1FD0, g_Menu->pManager->unkC);
            ShopMenuStartOpenMenuTransition();
            func_801CC278(0);
            shouldInitialize = FALSE;
        }

        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            func_801CBCF0(4, g_Menu->unk6E0, &D_801D1FD0, &D_801D1FE8, g_Menu->pManager->unkC, g_Menu->subMenuChoice, 3, 0);
            func_801CC720(0);
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

    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    func_801CBC88(0, 4, g_Menu->unk6E0, &D_801D1FD0, g_Menu->pManager->unkC);
    return TRUE;
}

void func_801D1F10(void) {
    switch (g_Menu->mainMenuChoice) {
        case MENU_CHOICE_SELL:
            break;

        case MENU_CHOICE_BUY:
            func_801D1968(1, 1);
            break;
    }
}
