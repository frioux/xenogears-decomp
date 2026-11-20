#include "common.h"

#include "system/menu.h"


// Quantity MenuFillInventoryTables gives every filled slot.
#define MENU_INVENTORY_FILL_QUANTITY 10

// Byte sizes of the two menu scratch buffers this unit allocates: one prim block
// per character cell (g_Menu->pCellPrimBlocks) and g_Menu->pPrimBuffer.
#define MENU_CELL_PRIM_BLOCK_SIZE 344
#define MENU_PRIM_BUFFER_SIZE 11712

// Pixel size of one character cell: the quad extent and the vertex offsets
// MenuInitializeCellLinePrims writes.
#define MENU_CELL_SIZE 16

// VRAM rect of the glyph texture MenuClearGlyphTexture clears: 64x32 VRAM words,
// the 0x1000-byte 4bpp bitmap the glyphs are rasterised into.
#define MENU_GLYPH_TEXTURE_X 0x140
#define MENU_GLYPH_TEXTURE_Y 0xE0
#define MENU_GLYPH_TEXTURE_WIDTH 64
#define MENU_GLYPH_TEXTURE_HEIGHT 32

// One glyph's bitmap, the shape MenuCompressGlyphRows walks: 16 rows of 16
// one-byte pixels, each row compressed in place into its first 12 bytes.
#define MENU_GLYPH_ROW_COUNT 16
#define MENU_GLYPH_ROW_BYTES 16

// Digits MenuDrawSlotResourceSprites draws of each of the two slot stats it
// formats: MenuParseNumberToDigits leaves the value in the tail of
// g_Menu->digits, any leading digit marked MENU_DIGIT_NONE.
#define MENU_SLOT_STAT_DIGIT_COUNT 3
// Where those digits start in g_Menu->digits.
#define MENU_SLOT_STAT_DIGIT_START (MENU_MAX_DIGITS - MENU_SLOT_STAT_DIGIT_COUNT)


// The save-slot record this unit writes: a packed copy of the parts of
// g_GameState that live in a save file.  Everything is word-aligned so the
// copies compile to lw/sw.
typedef struct {
    /* 0x00 */ u32 unk0[0x29];
} MenuSaveCharacter; // Size: 0xA4

typedef struct {
    /* 0x00 */ u32 unk0[0x8];
} MenuSaveGearHead20; // Size: 0x20

typedef struct {
    /* 0x00 */ u32 unk0[0x2];
} MenuSaveGearHead08; // Size: 0x08

typedef struct {
    /* 0x00 */ MenuSaveGearHead20 head;
    /* 0x20 */ MenuSaveGearHead08 headTail;
    // GameGear.unk5C: the four bytes of the equipped weapon's record head, see
    // MenuApplyGearWeapons. Nothing names those bytes, so neither end can be named.
    /* 0x28 */ u32 unk28;
    /* 0x2C */ u32 hp;
    // unk30, unk36 and unk3B are written by neither copy direction.
    /* 0x30 */ u32 unk30;
    /* 0x34 */ u16 fuel;
    /* 0x36 */ u16 unk36;
    // GameGear.unk99, which no matched code writes or reads.
    /* 0x38 */ u8 unk38;
    /* 0x39 */ u8 attackPower;
    /* 0x3A */ u8 baseWeightClass;
    /* 0x3B */ u8 unk3B;
} MenuSaveGear; // Size: 0x3C

typedef struct {
    /* 0x00 */ StringEntry entry[0xB];
} MenuSaveNamesA; // Size: 0xDC

typedef struct {
    /* 0x00 */ StringEntry entry[0x14];
} MenuSaveNamesB; // Size: 0x190

typedef struct {
    /* 0x00 */ u32 unk0[0x1E];
} MenuSaveBlob0x78; // Size: 0x78

typedef struct {
    /* 0x00 */ u32 unk0[0x58];
} MenuSaveBlob0x160; // Size: 0x160

typedef struct {
    /* 0x00 */ u32 unk0[0x40];
} MenuSaveBlob0x100; // Size: 0x100

typedef struct {
    /* 0x00 */ u8 unk0[0xA38];
} MenuSaveBlob0xA38; // Size: 0xA38

typedef struct {
    // Save header: the per-party-slot summary func_801CBA4C fills in (the record
    // src/menu/main/misc3.c declares as MenuSaveSlotHeader).
    /* 0x0000 */ u32 spuChannelMask; // g_SpuChannelMask, see MenuApplySaveSlot
    /* 0x0004 */ u16 hp[MAX_PARTY_MEMBERS];
    /* 0x000A */ u16 maxHp[MAX_PARTY_MEMBERS];
    /* 0x0010 */ u8 mp[MAX_PARTY_MEMBERS];
    /* 0x0013 */ u8 maxMp[MAX_PARTY_MEMBERS];
    /* 0x0016 */ u8 level[MAX_PARTY_MEMBERS];
    // GameCharacter.unk63 of the slot's character: a 3-digit number the menus draw
    // beside the level (MenuDrawSlotResourceSprites, MenuSetCharacterLevelDigits).
    /* 0x0019 */ u8 unk19[MAX_PARTY_MEMBERS];
    /* 0x001C */ u8 characterIds[MAX_PARTY_MEMBERS];
    // The slot builder clears unk1F; unk20 is read and written by nothing.
    /* 0x001F */ u8 unk1F;
    /* 0x0020 */ u8 unk20[3];
    /* 0x0023 */ u8 index; // card chunk index, as in MenuUnk2ImageChunk.index
    /* 0x0024 */ MenuSaveNamesA namesA;
    /* 0x0100 */ MenuSaveNamesB namesB;
    /* 0x0290 */ MenuSaveCharacter characters[MAX_GAME_CHARACTERS];
    /* 0x099C */ MenuSaveGear gears[MAX_GAME_GEARS];
    // The four blobs below are copied verbatim from g_GameState at the offset their
    // name gives, which is not their offset in this record.  0x1820 is the world map
    // state and 0x1920 the gold/party/inventory block (see include/main/game.h);
    // 0x1648 and 0x16C0 cut across GameState regions nothing here names yet.
    /* 0x0E4C */ MenuSaveBlob0x78 unk1648;
    /* 0x0EC4 */ MenuSaveBlob0x160 unk16C0;
    /* 0x1024 */ MenuSaveBlob0x100 unk1820;
    /* 0x1124 */ MenuSaveBlob0xA38 unk1920;
} MenuSaveSlot; // Size: 0x1B5C

void MenuSaveSlotWriteGameState(MenuSaveSlot* pSlot) {
    MenuSaveCharacter* pSaveCharacter;
    GameGear* pGear;
    MenuSaveGear* pSaveGear;
    u8 i;
    u32 size;

    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        pSaveCharacter = &pSlot->characters[i];
        *pSaveCharacter = *(MenuSaveCharacter*)&g_GameState.characters[i];
    }
    for (i = 0; i < MAX_GAME_GEARS; i++) {
        pSaveGear = &pSlot->gears[i];
        pGear = &g_GameState.gears[i];
        pSaveGear->head = *(MenuSaveGearHead20*)pGear;
        pSaveGear->headTail = *(MenuSaveGearHead08*)&pGear->weaponData[2];
        pSaveGear->unk28 = *(u32*)pGear->unk5C;
        pSaveGear->fuel = pGear->fuel;
        pSaveGear->hp = pGear->hp;
        pSaveGear->unk38 = pGear->unk99;
        pSaveGear->attackPower = pGear->attackPower;
        pSaveGear->baseWeightClass = pGear->baseWeightClass;
    }
    pSlot->namesA = *(MenuSaveNamesA*)g_GameState.strings;
    pSlot->namesB =
        *(MenuSaveNamesB*)&g_GameState.strings[sizeof(MenuSaveNamesA) / sizeof(StringEntry)];
    pSlot->unk1648 = *(MenuSaveBlob0x78*)((u8*)&g_GameState + 0x1648);
    pSlot->unk16C0 = *(MenuSaveBlob0x160*)((u8*)&g_GameState + 0x16C0);
    pSlot->unk1820 = *(MenuSaveBlob0x100*)((u8*)&g_GameState + 0x1820);
    size = sizeof(pSlot->unk1920);
    memcpy(&pSlot->unk1920, (u8*)&g_GameState + 0x1920, size);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E4D10);
/* Best: 91.40% (840 vs 744 bytes), the inverse of MenuSaveSlotWriteGameState (which
   matches): same record layout, same copy shapes, and the four
   MenuApplyGear* calls per gear land in the right order with s2 = i & 0xFF
   shared across them.  What is left is one allocator tie that swaps the two
   loop bases: retail keeps the g_GameState gear pointer in $s1 and the slot
   pointer in $s0, gcc gives $s1 to the slot pointer (the whole 0x28 head
   copy and the five field stores are mirror-imaged, which is why the byte
   count grows).  Tried: pGear/pSaveGear assignment in both orders, and the
   loop body with the head copies first (no change).  The 0x160 copy before
   the loop, the 0xDC/0x190/0x78/0x100 copies after it and the trailing
   memcpy of 0xA38 all match retail.
/*
void MenuApplyGearFrame(MenuDressingRoom*, u8);
void MenuApplyGearArmor(MenuDressingRoom*, u8);
void MenuApplyGearEngine(MenuDressingRoom*, u8);
void MenuApplyGearAccessories(MenuDressingRoom*, u8);

void func_801E4D10(MenuSaveSlot* pSlot, MenuDressingRoom* pDressingRoom) {
    GameGear* pGear;
    MenuSaveGear* pSaveGear;
    u8 i;
    u32 size;

    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        s32 charOffset = i * 0xA4 + 0x290;
        *(MenuSaveCharacter*)&g_GameState.characters[i] =
            *(MenuSaveCharacter*)((mem_addr)pSlot + charOffset);
    }
    *(MenuSaveBlob0x160*)((u32*)&g_GameState + 0x16C0 / 4) = *(MenuSaveBlob0x160*)((u32*)pSlot + 0xEC4 / 4);
    for (i = 0; i < MAX_GAME_GEARS; i++) {
        s32 gearOffset = i * 0x3C + 0x99C;
        pGear = &g_GameState.gears[i];
        pSaveGear = (MenuSaveGear*)((mem_addr)pSlot + gearOffset);
        *(MenuSaveGearHead20*)pGear = pSaveGear->head;
        *(MenuSaveGearHead08*)((u32*)pGear + 8) = pSaveGear->headTail;
        *(u32*)pGear->unk5C = pSaveGear->unk28;
        MenuApplyGearFrame(pDressingRoom, i);
        MenuApplyGearArmor(pDressingRoom, i);
        MenuApplyGearEngine(pDressingRoom, i);
        MenuApplyGearAccessories(pDressingRoom, i);
        pGear->fuel = pSaveGear->fuel;
        pGear->hp = pSaveGear->hp;
        pGear->unk99 = pSaveGear->unk38;
        pGear->attackPower = pSaveGear->attackPower;
        pGear->baseWeightClass = pSaveGear->baseWeightClass;
    }
    *(MenuSaveNamesA*)((u32*)&g_GameState) = pSlot->namesA;
    *(MenuSaveNamesB*)((u32*)&g_GameState + 0xDC / 4) = pSlot->namesB;
    *(MenuSaveBlob0x78*)((u32*)&g_GameState + 0x1648 / 4) = pSlot->unk1648;
    *(MenuSaveBlob0x100*)((u32*)&g_GameState + 0x1820 / 4) = pSlot->unk1820;
    size = 0xA38;
    memcpy((u8*)&g_GameState + 0x1920, (u8*)pSlot + 0x1124, size);
}
*/
void MenuFillInventoryTables(void) {
    u8 i;

    for (i = 1; i < MENU_WEAPON_ID_COUNT; i++) {
        g_GameState.weaponIDs[i] = i;
        g_GameState.weaponQuantities[i] = MENU_INVENTORY_FILL_QUANTITY;
    }
    for (i = 1; i < MENU_ACCESSORY_ID_COUNT; i++) {
        g_GameState.accessoryIDs[i] = i;
        g_GameState.accessoryQuantities[i] = MENU_INVENTORY_FILL_QUANTITY;
    }
    for (i = 1; i < MENU_ITEM_ID_COUNT; i++) {
        g_GameState.itemIDs[i + 2] = i;
        g_GameState.itemQuantities[i + 2] = MENU_INVENTORY_FILL_QUANTITY;
    }
    for (i = 1; i < MENU_GEAR_WEAPON_ID_COUNT; i++) {
        g_GameState.gearWeaponsIds[i] = i;
        g_GameState.gearWeaponsQty[i] = MENU_INVENTORY_FILL_QUANTITY;
    }
    for (i = 1; i < MENU_GEAR_PART_ID_COUNT; i++) {
        g_GameState.gearPartsIds[i] = i;
        g_GameState.gearPartsQty[i] = MENU_INVENTORY_FILL_QUANTITY;
    }
}

// Resets the eleven 0x20-byte per-gear stat limit rows the gear screens clamp
// against, plus the party flag word.  BattleInitItemInventoryAndConditionMasks
// (src/battle/main/misc19.c) writes the same rows, with battle's values.
void func_801E5178(void) {
    g_GameState.characterFlags = 0x7FF;
    *(u16 *)&g_GameState.unk16BC[0x00].commandMask = 0xFFF8;
    *(u16 *)&g_GameState.unk16BC[0x00].characterConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x00].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x00].gearConditionMask = 0xFE00;
    *(u16 *)&g_GameState.unk16BC[0x00].statusFlags = 0xE000;
    ((u_char *)&g_GameState.unk16BC[0x00])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x01].commandMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[0x01].characterConditionMask = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x01].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x01].gearConditionMask = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x01].statusFlags = 0xC000;
    ((u_char *)&g_GameState.unk16BC[0x01])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x02].commandMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[0x02].characterConditionMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[0x02].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x02].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x02].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[0x02])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x03].commandMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[0x03].characterConditionMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[0x03].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x03].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x03].statusFlags = 0xF000;
    ((u_char *)&g_GameState.unk16BC[0x03])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x04].commandMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[0x04].characterConditionMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[0x04].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x04].gearConditionMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[0x04].statusFlags = 0xE000;
    ((u_char *)&g_GameState.unk16BC[0x04])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x05].commandMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[0x05].characterConditionMask = 0xF000;
    *(u16 *)&g_GameState.unk16BC[0x05].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x05].gearConditionMask = 0xF000;
    *(u16 *)&g_GameState.unk16BC[0x05].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[0x05])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x06].commandMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[0x06].characterConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x06].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x06].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x06].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[0x06])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x07].commandMask = 0x0000;
    *(u16 *)&g_GameState.unk16BC[0x07].characterConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x07].unk8 = 0x0000;
    *(u16 *)&g_GameState.unk16BC[0x07].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x07].statusFlags = 0x0000;
    ((u_char *)&g_GameState.unk16BC[0x07])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x08].commandMask = 0x0000;
    *(u16 *)&g_GameState.unk16BC[0x08].characterConditionMask = 0xF800;
    *(u16 *)&g_GameState.unk16BC[0x08].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x08].gearConditionMask = 0x0000;
    *(u16 *)&g_GameState.unk16BC[0x08].statusFlags = 0xE000;
    ((u_char *)&g_GameState.unk16BC[0x08])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x09].commandMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[0x09].characterConditionMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[0x09].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x09].gearConditionMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[0x09].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[0x09])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[0x0A].commandMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[0x0A].characterConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x0A].unk8 = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[0x0A].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0x0A].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[0x0A])[0x1B] = 7;
}
// Builds window `windowIndex`'s background quad and its four border strips.  The
// same body ships as MemberChangeMenuInitializeWindowGraphics.
void func_801E53CC(u_char windowIndex) {
    RECT texWindow;
    MenuWindow* pWindow;
    u_char i;

    pWindow = g_Menu->windows[windowIndex];

    texWindow.y = 0;
    texWindow.x = 0;
    texWindow.h = 0x100;
    texWindow.w = 0x100;

    g_Menu->pManager->shouldRenderWindow[windowIndex] = FALSE;
    g_Menu->pManager->isWindowOpening[windowIndex] = 0;

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
            &texWindow
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
