#ifndef _XENO_MENU_H
#define _XENO_MENU_H

// A SystemMenu.digits[] entry holding no digit (nothing is drawn for it).
#define MENU_DIGIT_NONE 0xFF
// How many of g_Menu->digits[] each character window stat is formatted to; the
// polys<Stat> prim arrays in MenuCharacterWindow hold two prims per digit.
#define MENU_HP_DIGIT_COUNT 3
#define MENU_MP_DIGIT_COUNT 2
#define MENU_LEVEL_DIGIT_COUNT 3
#define MENU_WIDE_NUMBER_DIGIT_COUNT 7
// String IDs per row panel of the item list (g_Menu->itemRowStrings).
#define MENU_ITEM_ROW_STRING_COUNT 8
// MenuLoadResourceSet's resource table: entry kind + MENU_PANEL_FREE_OFFSET unloads what that kind loaded.
#define MENU_PANEL_FREE_OFFSET 0x10
// Arrow cursor animation: the frame index counts down from the last frame every
// MENU_ARROW_CURSOR_ANIM_FRAME_TICKS updates, the frame's texture being MENU_TEX_ARROW_CURSOR + index.
#define MENU_ARROW_CURSOR_LAST_ANIM_FRAME 4
#define MENU_ARROW_CURSOR_ANIM_FRAME_TICKS 6
// Entries in the character window icon tables; polysIcons holds two prims per icon.
#define MENU_CHARACTER_ICON_COUNT 20
// Border corner prims, two per corner (one per render context).
#define MENU_WINDOW_BORDER_CORNER_COUNT 4
#define MENU_SLOT_NONE 0xFF
// Card slot bookkeeping, see MenuUnk2: a memory card holds 15 slots, so the second card's
// slots start at index MENU_CARD_SLOT_COUNT, and a slot with no record reads MENU_SLOT_NONE.
#define MENU_CARD_SLOT_COUNT 15
#define MENU_CARD_COUNT 2 // memory card slots, the dimension of cardStatus[]/cardEnumerated[]
#define MENU_CARD_RECORD_COUNT 16 // save records per card; index / this is the card
#define MENU_WINDOW_TEXT_NONE 0xFF // MenuLoadWindow2Text's no-second-text argument
#define MENU_CARD_SLOT_STATE_READY 2 // g_Menu->cardSlotState once the slot list is built
// One card slot's save chunk, the size of MenuUnk2ImageChunk
#define MENU_IMAGE_CHUNK_SIZE 0x200
// Scratch buffer one rendered string image is built in before LoadImage
#define MENU_STRING_IMAGE_BUFFER_SIZE 0x3F6
// Bytes one item description (and a gear shop's sell price) is rendered into as an image
#define MENU_ITEM_DESC_IMAGE_BUFFER_SIZE 0x618
// Bytes one party slot's prim context occupies in g_Menu->pPrimBuffer
#define MENU_SLOT_PRIM_STRIDE 0x87C

#include "psyq/libgpu.h"
#include "system/menu_resources.h"
#include "system/sound.h"
#include "main/game.h"


// Shop resources archive indices
#define SHOP_RES_ITEM_DATA 1
#define SHOP_RES_WEAPON_DATA 2
#define SHOP_RES_ACCESSORY_DATA 3
#define SHOP_RES_GEAR_FRAME_DATA 17
#define SHOP_RES_GEAR_ARMOR_DATA 18
#define SHOP_RES_GEAR_ENGINE_DATA 19
#define SHOP_RES_GEAR_ACCESORY_DATA 20
#define SHOP_RES_WEAPON_DESC 39
#define SHOP_RES_ACCESORY_DESC 40
#define SHOP_RES_ITEM_DESC 41
#define SHOP_RES_GEAR_WEAPON_DATA 43
#define SHOP_RES_GEAR_ARMOR_NAMES 45
#define SHOP_RES_GEAR_FRAME_NAMES 46
#define SHOP_RES_GEAR_ENGINE_NAMES 47
#define SHOP_RES_GEAR_FRAME_DESC 48
#define SHOP_RES_GEAR_ENGINE_DESC 49
#define SHOP_RES_AMMO_DESC 50
#define SHOP_RES_GEAR_ACCESSORY_DESC 51
#define SHOP_RES_GEAR_WEAPON_DESC 52

// Mapped inputs
#define MENU_INPUT_RIGHT 0x0
#define MENU_INPUT_DOWN 0x1
#define MENU_INPUT_LEFT 0x2
#define MENU_INPUT_UP 0x3
#define MENU_INPUT_CONFIRM 0x4
#define MENU_INPUT_BACK 0x5
#define MENU_INPUT_IDLE 0x8
#define MENU_INPUT_NEXT 0x9
#define MENU_INPUT_PREV 0xA
#define MENU_INPUT_SQUARE 6 // a direction press stores the input id; the group above is the mapped menu input
#define MENU_INPUT_TRIANGLE 7
#define MENU_INPUT_SELECT 0xC

// Transition animation states
#define MENU_ANIMATION_DONE 0x0
#define MENU_OPEN_ANIMATION 0x1
#define MENU_CLOSE_ANIMATION 0x2
#define MENU_OPEN_ANIMATION_START 0x3
#define MENU_CLOSE_ANIMATION_START 0x4

#define MENU_MAX_NUM_WINDOWS 0x7
#define MENU_SCREEN_VRAM_X 0x2C0 // VRAM position of the selection menu framebuffer rect
#define MENU_SCREEN_VRAM_Y 0x100
#define MENU_CURSOR_SHADE_WIDTH 0x40 // shaded rectangle behind the cursor
#define MENU_CHARACTER_WINDOW_COUNT 3 // one window per party character
#define MENU_MAX_NUM_CURSORS 0x4
#define MENU_MAX_NUM_ARROW_CURSORS 0x2
#define MENU_WINDOW_BORDER_SIZE 0x8
#define MENU_MAX_DIGITS 0x9
// Pixels between one digit's quad and the next, in a menu digit row.
#define MENU_DIGIT_SPACING 8

// Max items to display in the shop at a time
#define SHOP_MAX_ITEMS_IN_VIEW 8

// Modes for memory management functions
#define MENU_DATA_INITIALIZE 0x1
#define MENU_DATA_FREE 0x0

#define SHOP_DATA_INITIALIZE 0x0
#define SHOP_DATA_FREE 0x10

// TODO: Move to more fitting file
#define FONT_LETTER_HEIGHT 0xD

#define MAX_SHOP_ITEMS 0x30
#define MAX_GEAR_SHOP_ITEMS 0x14

#define ITEM_TYPE_WEAPON 0x0
#define ITEM_TYPE_ACCESSORY 0x1
#define ITEM_TYPE_ITEM 0x2

#define ITEM_TYPE_GEAR_ARMOR     0x0
#define ITEM_TYPE_GEAR_FRAME     0x1
#define ITEM_TYPE_GEAR_ENGINE    0x2
#define ITEM_TYPE_GEAR_ACCESSORY 0x3
#define ITEM_TYPE_GEAR_WEAPON    0x4

// So far these _seem_ to be consistent across overlays?
#define MENU_SOUND_ERROR    4
#define MENU_SOUND_CONFIRM  2
#define MENU_SOUND_CURSOR   1
#define MENU_SOUND_CANCEL   3

/*
 * Menu Resource type definitions
 */

/**
 * This maps to the header of the file. The actual length of offsets[] is contained in count.
 *
 * The 0-length array is a workaround to not having C99 features like Flexible Array Member (FAM) that allows
 * you to write 'u16 offsets[];'
 */
typedef struct {
    u32 count;
    u16 offsets[0];
} MenuResourceDirectory; /* size: (2 * count) + 4 */

/* This is the actual record structure */
typedef struct {
    /* 0x00 */ u16 u;
    /* 0x02 */ s16 v;
    /* 0x04 */ u16 width;
    /* 0x06 */ u16 height;
    /* 0x08 */ u16 x;
    /* 0x0A */ s16 y;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 texPageId;
    /* 0x12 */ s16 clutX;
    /* 0x14 */ s16 clutY;
    /* 0x16 */ u16 texPageX;
    /* 0x18 */ s16 texPageY;
    /* 0x1A */ u8 flipX;
    /* 0x1B */ u8 flipY;
} MenuResourceTextureMetadata;

/*
 * Menu Resource Directory Entries (dirent)
 */

typedef struct {
    s16 count;
    s16 pad;
    MenuResourceTextureMetadata data[0];
} MenuResourceDirentTextureMetadata; /* size: (count * sizeof(MenuResourceTextureMetadata)) + 4 */

/*
 * end menu resources
 */

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u_short price;
    /* 0x4 */ u16 flags; // Bit 0 = the item opens the character sub-list
    /* 0x6 */ u8 displayFlags; // 0x80 = shaded name prim, 0x20 = unshaded in the shop, 0x10 = hidden from the sell list
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u8 amount; // Stat effect magnitude, the fields below select what it is applied to
    /* 0x9 */ u8 unk9;
    /* 0xA */ s16 effectFlags;
    /* 0xC */ s16 statFlags;
    /* 0xE */ u8 unkE[0x2];
} MenuShopItem; // Size: 0x10

typedef struct {
    /* 0x00 */ u32 equipFlags;
    /* 0x04 */ u16 price;
    /* 0x06 */ u16 unk6;
    /* 0x08 */ u16 baseDefense;
    /* 0x0A */ u16 baseEtherDefense;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 unk12;
} GearArmor; /* size: 0x14 */

typedef struct {
    /* 0x0  */ POLY_FT4 polys[2];
    /* 0x50 */ SVECTOR vertices[4];
    /* 0x70 */ RECT vramDest;
    /* 0x78 */ void* pVramBuffer;
    /* 0x7C */ u8 palette; // System palette index, selects g_SystemPalette1 / g_SystemPalette2
    /* 0x7D */ u8 renderContext;
    /* 0x7E */ u8 width;
    /* 0x7F */ u8 hasVertices; // Set when vertices[] must go through RotTransPers4
} MenuString; // Size: 0x80

typedef struct {
    /* 0x0 */ u_short equipFlags; // Which characters can use this item?
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u_short price;
    /* 0x6 */ u8 equipGroup; // group id
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u16 slotFlags; // Copied into GameCharacterSlot.flags
    /* 0xA */ u8 unkA; // Copied into GameCharacterSlot.chance
    /* 0xB */ u8 unkB; // Copied into GameCharacterSlot.conditionKind
    /* 0xC */ u8 attack;
    /* 0xD */ u8 unkD[3];
} MenuShopWeapon; // Size: 0x10

typedef struct {
    /* 0x0 */ u_short equipFlags; // Which characters can use this item?
    /* 0x2 */ u_short price;
    /* 0x4 */ u8 unk4[4];
    /* 0x8 */ u8 defense;
    /* 0x9 */ u8 effectType; // switch: which effect the flags below drive
    /* 0xA */ u16 effectFlags;
    /* 0xC */ u8 effectAmount; // Added to every stat selected by statFlags, see MenuApplyEquippedItemBonuses
    /* 0xD */ u8 statFlags; // One bit per stat the effect raises; retail tests both bytes as the halfword at 0xC
    /* 0xE */ u16 equipGroup; // group id
} MenuShopAccessory; // Size: 0x10

typedef struct {
    /* 0x0  */ DRAWENV drawEnv;
    /* 0x5C */ DISPENV dispEnv;
    /* 0x70 */ unsigned long ot[16];  // 0x70 = ot[0], 0x74 = ot[1], 0x78 = ot[2], 0x7C = ot[3], 0x80 = ot[4]
    /* 0xB0 */ u32 otBuffer;
} GfxEnvironment; // Size: 0xB4

typedef struct {
    /* 0x0   */ POLY_FT4 polysPointerCursor[2];
    /* 0x50  */ POLY_G4 polyG4s[2];
    /* 0x98  */ POLY_F4 polysDimEffect[2];
    /* 0xC8  */ LINE_F3 lines1[2];
    /* 0xF8  */ LINE_F3 lines2[2];
    /* 0x128 */ DR_MODE drModes1[2];
    /* 0x140 */ DR_MODE drawModeDimEffect[2];
    /* 0x158 */ u_char cursorRenderContext;
    /* 0x159 */ u8 drawModeRenderContext;
    /* 0x15A */ u8 unk15A;
    /* 0x15B */ u8 shadeWidth; // Width of the shaded rectangle behind the cursor
} MenuUnk1; // Size: 0x15C

typedef struct {
    /* 0x00 */ u8 unk0[0x18];
    /* 0x18 */ u8 ids[0xC];
    /* 0x24 */ u8 unk24[0x34];
    /* 0x58 */ u8 unk58;
    /* 0x59 */ u8 unk59[0x3];
} MenuUnk2Record; // Size: 0x5C

typedef struct {
    /* 0x000 */ u8 pad0[0x100];
    /* 0x100 */ u8 data[0x23];
    /* 0x123 */ u8 index;
    /* 0x124 */ u8 pad124[0xDC];
} MenuUnk2ImageChunk; // Size: 0x200

typedef struct {
    /* 0x0   */ MenuUnk2Record records[0x20];
    /* 0xB80 */ TIM_IMAGE tim;
    /* 0xB94 */ MenuUnk2ImageChunk chunks[0x20];
    /* 0x4B94 */ u8 saveFileMagic[0x2];
    /* 0x4B96 */ u8 saveIconDisplayFlag;
    /* 0x4B97 */ u8 saveBlockCount;
    /* 0x4B98 */ u8 saveTitle[0x5C]; // Shift-JIS title; the slot number is spliced in at 0x1A
    /* 0x4BF4 */ u8 saveIconPalette[0x20];
    /* 0x4C14 */ u8 saveIconImage[0x80];
    /* 0x4C94 */ u8 unk4C94[0x2E0];
    /* 0x4F74 */ s32 cardErrorCodes[2];
    /* 0x4F7C */ s32 selectedSlot;
    /* 0x4F80 */ s32 previousSelectedSlot; // Slot the character window was last rebuilt for, 0xFF = none
    /* 0x4F84 */ s32 slotWriteCursor; // Next free index into slotRecordIndexes
    /* 0x4F88 */ u8 cardEnumerated[MENU_CARD_COUNT]; // One flag per memory card, set once its directory was listed
    /* 0x4F8A */ u8 cardSlotCounts[2]; // Directory entries found, one count per card
    /* 0x4F8C */ u8 previousCardSlotCounts[2]; // cardSlotCounts[] as read last time, one per card
    /* 0x4F8E */ u8 slotHasGameSave[0x20]; // One flag per card slot, set when its record's game code matched
    /* 0x4FAE */ u8 slotRecordIndexes[0x20]; // Index into records[]/chunks[] per card slot, 0xFF = empty
    /* 0x4FCE */ u8 gameSerialCode[0xD]; // 12-character game code and its terminator, both copied from the record's ids[]
    /* 0x4FDB */ u8 unk4FDB;
    /* 0x4FDC */ u8 unk4FDC[0x8];
    /* 0x4FE4 */ u8 cardStatus[MENU_CARD_COUNT];
    /* 0x4FE6 */ u8 cardPollMode;
    /* 0x4FE7 */ u8 unk4FE7;
    /* 0x4FE8 */ u8 previousCardStatus[2];
    /* 0x4FEA */ u8 pad4FEA[0x2];
    /* 0x4FEC */ s32 cardEvents[4];
    /* 0x4FFC */ u8 saveTitleSuffix[0x20]; // Text strcat'd onto saveTitle by func_801CA8C0, filled from the archive by func_801C6400
    /* 0x501C */ u8 discTwoGameSerialCode[0xD]; // "BASLUS-01160"; MenuLoadResourceFiles fills it beside gameSerialCode's "BASLUS-00664"
    /* 0x5029 */ u8 pad5029[0xB];
} MenuUnk2; // Size: 0x5034

typedef struct {
    /* 0x0  */ u8 shouldRenderCharacterWindows[MENU_CHARACTER_WINDOW_COUNT]; // One flag per g_Menu->characterWindows entry
    /* 0x3  */ u8 unk3;
    /* 0x4  */ u8 shouldRenderPointerCursor; // On the ball selection windows?
    /* 0x5  */ u8 goldDigitsActive; // Set by MenuSetGoldDigits, gates the gold digits in MenuAddGoldAndPlayTimePrims
    /* 0x6  */ u8 playTimeDigitsActive; // Set by MenuCreatePlayTimeWindow, gates the play time digits in MenuAddGoldAndPlayTimePrims
    /* 0x7  */ u8 shouldRenderUnk358Prims; // Set by MenuSetGearWindowContents, gates the gear window prims in MenuAddUnk358Prims
    /* 0x8  */ u8 shouldRenderUnk35C;
    /* 0x9  */ u8 shouldRenderSelectionMenu;
    /* 0xA  */ u8 shouldRenderSubmenuStrings; // Gates MenuUnk5 (pSubmenuPrims) string prims
    /* 0xB  */ u8 characterWindowBuilt; // Set by MenuUpdateCharacterWindow once the window's strings are up; MenuAddCharacterSlotPrims's prims are added only while it is set, MenuClearGlyphTexture clears it
    /* 0xC  */ u8 shouldRenderStrings[0x8]; // One flag per g_Menu->optionStrings string
    /* 0x14 */ u8 shouldRenderSubMenuChoiceStrings[0x6]; // One flag per g_Menu->subMenuChoiceStrings
    /* 0x1A */ u8 shouldRenderStringsDE0[0x6];
    /* 0x20 */ u8 shouldRenderWindow[MENU_MAX_NUM_WINDOWS];
    /* 0x27 */ u8 isWindowOpening[MENU_MAX_NUM_WINDOWS]; // Open tween runs while isOpen == 0
    /* 0x2E */ u8 shouldRenderModalStrings; // after isWindowOpening's MENU_MAX_NUM_WINDOWS flags, not 0x2B
    /* 0x2F */ u_char shouldRenderCursors;
    /* 0x30 */ u8 currentCharacterIDs[0x3];
    /* 0x33 */ u8 window2StringsActive; // Set once MenuBeginCardOperation has window-2 text up, cleared after MenuFreeWindow2Strings
    /* 0x34 */ u8 shouldRenderStrings4E0[0x4];
    /* 0x38 */ u8 shouldRenderStrings10E0[0x8];
    /* 0x40 */ u8 shouldRenderStrings14E0[0x6];
    /* 0x46 */ u8 shouldRenderCharacters;
    /* 0x47 */ u8 unk47;
    /* 0x48 */ u8 shouldRenderUnk42C;
    /* 0x49 */ u_char scrollHandleActive;
    /* 0x4A */ u8 shouldRenderPanel[0x4]; // One flag per 0x360/0x430/0x434/0x438 panel
    /* 0x4E */ u8 shouldRenderStrings17E0[2];
    /* 0x50 */ u8 shouldRenderArrowCursor[MENU_MAX_NUM_ARROW_CURSORS]; // Should render arrow cursor (array, size 3?)
    /* 0x52 */ u8 cellGridScrollState; // 0 off, 1 scrolling the cell grid one step per frame, 2 drawn but not advanced
    /* 0x53 */ u_char shoulderButtonUiActive;
    /* 0x54 */ u8 shouldRenderStrings18E0[0x6];
    /* 0x5A */ u8 shouldRenderShop; // Gates the shop's primitive batch
    /* 0x5B */ u8 finalPriceRenderState; // 0 off, 1 drawn, 2 built but hidden under a modal
    /* 0x5C */ u8 shouldRenderStrings1BE0[0x4]; // One flag per g_Menu->unk1BE0 string
    /* 0x60 */ u8 isGearScreenActive[0x3]; // Gear screen buffer slots, MenuInitializeGearScreenBuffers returns the first set one
    /* 0x63 */ u8 isPanelActive[0x9]; // One flag per panel built on demand: gear model, shop prices, gear stats/strings, shoulder button UI, card slots
} MenuManager; // Size: 0x6C

// A menu with N options, each having a cursor and some text/content
typedef struct {
    /* 0x0    */ POLY_FT4 polysCursors[28 * 2]; // Cursor for each menu option
    /* 0x8C0  */ POLY_FT4 polysTexts[28 * 2]; // Texts / content for each menu option
    /* 0x1180 */ RECT vramRect;
    /* 0x1188 */ int numCursors;
    /* 0x118C */ int numTexts;
    /* 0x1190 */ u_char cursorsRenderCtx;
    /* 0x1191 */ u_char textsRenderCtx;
    /* 0x1192 */ u_char isDisabled; // When it differs from isDisabledRendered the prims are re-tinted
    /* 0x1193 */ u_char isDisabledRendered; // Tint state currently baked into the prims
} MenuSelectionMenu; // Size: 0x1194

typedef struct {
    /* 0x0    */ POLY_FT4 polys0[16 * 2]; // Submenu options ball cursor?
    /* 0x500  */ POLY_FT4 polys500[16 * 2]; // Submenu options text?
    /* 0xA00  */ u8 unkA00[0xA00];
    /* 0x1400 */ int polys0PrimCount;
    /* 0x1404 */ int polys500PrimCount;
    /* 0x1408 */ u_char polys0RenderCtx;
    /* 0x1409 */ u_char polys500RenderCtx;
    /* 0x140A */ u8 unk140A;
    /* 0x140B */ u8 unk140B;
} MenuUnk5; // Size: 0x140C

typedef struct {
    /* 0x00 */ int equipFlags;
    /* 0x04 */ int hp; // 0x9c4 or 2500
    /* 0x08 */ u16 weight; // 0xDAC or 3500
    /* 0x0A */ u16 price; // 0x320 or 800
    /* 0x0C */ int unkC; // 0
    /* 0x10 */ int unk10; // 0
    /* 0x14 */ u8 agility; // 10 -- agility?
    /* 0x15 */ u8 etherAmplification; // 10 -- eth amp?
    /* 0x16 */ u8 unk16; // 0
    /* 0x17 */ u8 responsiveness; // 0, added to the gear's responsiveness total
} GearFrame; /* size: 0x18 */

typedef struct {
    /* 0x00 */ int equipFlags;
    /* 0x04 */ u16 unused4; // matches maxFuel
    /* 0x06 */ u16 maxFuel;
    /* 0x08 */ u16 pad8; // always 00 00
    /* 0x0A */ u16 price;
    /* 0x0C */ u8 powerOutput;
    /* 0x0D */ u8 unkD; // always 0xA
    /* 0x0E */ u8 unkE; // always 0xA
    /* 0x0F */ u8 unkF; // always 0
} GearEngine; /* size: 0x10 */

typedef struct {
    /* 0x00 */ u32 equipFlags;
    /* 0x04 */ u16 price;
    /* 0x06 */ u16 weight;
    /* 0x08 */ u16 equipGroup;
    /* 0x0A */ u16 unkA;
    /* 0x0C */ u8 unkC;
    /* 0x0D */ u8 defense;
    /* 0x0E */ u8 etherDefense;
    /* 0x0F */ u8 unkF;
    /* 0x10 */ u8 unk10[4];
    /* 0x14 */ u8 responsiveness;
    /* 0x15 */ u8 effectType; // switch: which effect the flags/amount below drive
    /* 0x16 */ u16 effectFlags;
    /* 0x18 */ u8 unk18;
    /* 0x19 */ u8 unk19;
    /* 0x1A */ u8 effectAmount; // Accumulated per effectFlags bit, see GearShopMenuApplyGearAccessories
    /* 0x1B */ u8 etherAmplification; // Added to the gear's etherAmplificationFromEquipment, see MenuApplyGearAccessories
} GearAccessory; /* size: 0x1C */


// TODO: these are placeholders so the struct is the right size
typedef struct {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ u32 equipFlags;
    /* 0x08 */ u16 price;
    /* 0x0A */ u16 unkA;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u8 attack;
    /* 0x0F */ u8 equipGroup; // A new weapon replaces the equipped one of the same group, see GearShopMenuComputeEquipStatDelta
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x12 */ u16 flags; // Copied into GearWeaponData.flags
} GearWeapon; /* size: 0x14 */

typedef struct {
    /* 0x0  */ MenuShopWeapon* pWeaponsData;
    /* 0x4  */ MenuShopAccessory* pAccessoriesData;
    /* 0x8  */ GearFrame* pGearFrames;
    /* 0xC  */ GearEngine *pGearEngines;
    /* 0x10 */ GearArmor *pGearArmor;
    /* 0x14 */ GearAccessory *pGearAccessories;
    /* 0x18 */ GearWeapon *pGearWeapons;
    /* 0x1C */ MenuShopItem* pItemsData;
    /* 0x20 */ void* pCharacterSlots[MAX_GAME_CHARACTERS]; // Pointer per character id to its 0x28-stride equipment slot records
    /* 0x4C */ void* pGearBlocks[MAX_GAME_GEARS]; // Pointer per gear id, see MenuSetGearHpScaled
    /* 0x9C */ int hp; // hp
    /* 0xA0 */ int maxHp; // maxHp
    /* 0xA4 */ u16 totalDefense; // totalDefense
    /* 0xA6 */ u16 totalEtherDefense; // totalEtherDefense
    /* 0xA8 */ u16 totalWeight;
    /* 0xAA */ u16 unkAA;
    /* 0xAC */ u16 fuel;
    /* 0xAE */ u16 maxFuel;
    /* 0xB0 */ u16 totalAttack; // maybe ether??
    /* 0xB2 */ u8 totalResponsiveness; // responsiveness
    /* 0xB3 */ u8 totalAgility;
    /* 0xB4 */ u8 totalEtherAmplification;
    /* 0xB5 */ u8 unkB5;
    /* 0xB6 */ u8 unkB6;
    /* 0xB7 */ u8 unkB7;
    /* 0xB8 */ u16 statValues[9];
    /* 0xCA */ u16 unkCA;
} MenuDressingRoom; // Size: 0xCC

typedef struct {
    /* 0x0 */ u8 unk0[0xDEC];
} MenuUnk7; // Size: 0xDEC

typedef struct {
    /* 0x0000 */ MenuString unk0;
    /* 0x0080 */ POLY_FT4 unk80[2];
    /* 0x00D0 */ POLY_FT4 polysD0[0x6 * 2];
    /* 0x02B0 */ POLY_FT4 polys2B0[11 * 2 * 2];
    /* 0x0990 */ POLY_FT4 polys990[11 * 2];
    /* 0x00D0 */ POLY_FT4 polysD00[18 * 2];
    /* 0x12A0 */ POLY_FT4 polys12A0[14 * 2]; // Gear HP, FUEL, WEIGHT text
    /* 0x1700 */ POLY_FT4 polys1700[5 * 2]; // Gear HP digits
    /* 0x1890 */ POLY_FT4 polys1890[5 * 2]; // Gear Max HP digits
    /* 0x1A20 */ POLY_FT4 polys1A20[5 * 2]; // Gear fuel digits
    /* 0x1BB0 */ POLY_FT4 polys1BB0[5 * 2]; // Gear Max fuel digits
    /* 0x1D40 */ POLY_FT4 polys1D40[5 * 2]; // Gear weight digits
    /* 0x1ED0 */ u8 sparklePrimCount;
    /* 0x1ED1 */ u8 optionMarkerPrimCounts[2];
    /* 0x1ED3 */ u8 optionHighlightPrimCount;
    /* 0x1ED4 */ u8 sparkleRenderContext;
    /* 0x1ED5 */ u8 optionMarkerRenderContexts[2];
    /* 0x1ED7 */ u8 optionHighlightRenderContext;
    /* 0x1ED8 */ u8 sparklesActive;
    /* 0x1ED9 */ u8 optionMarkerStates[2]; // 0 idle, 1 highlighting, 2 looping, 3 returning
    /* 0x1EDB */ u8 optionHighlightState; // 0 idle, 1 highlighting, 2 looping, 3 reversing, 4 returning
    /* 0x1EDC */ u8 sparkleTimer;
    /* 0x1EDD */ u8 optionMarkerTimers[2];
    /* 0x1EDF */ u8 optionHighlightTimer;
    /* 0x1EE0 */ u8 sparkleGlyphIndex;
    /* 0x1EE1 */ u8 unk80RenderContext; // Render context of the unk80 prims
    /* 0x1EE2 */ s16 optionHighlightGlyphIndex;
    /* 0x1EE4 */ s16 optionMarkerGlyphIndexes[2];
    /* 0x1EE8 */ u8 gearHpStrLen;
    /* 0x1EE9 */ u8 gearMaxHpStrLen;
    /* 0x1EEA */ u8 gearFuelStrLen;
    /* 0x1EEB */ u8 gearMaxFuelStrLen;
    /* 0x1EEC */ u8 gearWeightStrLen;
    /* 0x1EED */ u8 gearStatStringsRenderContext;
    /* 0x1EEE */ u8 unk1EEE;
    /* 0x1EEF */ u8 unk1EEF;
    /* 0x1EF0 */ u16 sparkleX;
    /* 0x1EF2 */ u16 sparkleY;
    /* 0x1EF4 */ u16 optionMarkerX[2];
    /* 0x1EF8 */ u16 optionMarkerY[2];
    /* 0x1EFC */ u16 optionHighlightX;
    /* 0x1EFE */ u16 optionHighlightY;
} MenuUnk8; /* size: 1F00 */

typedef struct {
    /* 0x0000 */ u8 unk0[0x32F1];
    /* 0x32F1 */ u8 renderContext; // Copy of SystemMenu::renderContext
} MenuUnk10; // Size: 0x32F2

// Second selection-menu-like object (0x1198 bytes, freed on menu exit)
typedef struct {
    /* 0x0000 */ MenuString itemNameStrings[0x10]; // Item name of each inventory row, left column
    /* 0x0800 */ MenuString itemQuantityStrings[0x10]; // Two-digit quantity of each inventory row, right column
    /* 0x1000 */ MenuString primGroupStrings[0x3]; // The three strings gated by shouldRenderPrimGroup
    /* 0x1180 */ void* pVramBuffer;
    /* 0x1184 */ u8 shouldRenderRows[0x10];
    /* 0x1194 */ u8 shouldRenderPrimGroup;
    /* 0x1195 */ u8 pad1195[0x3];
} MenuUnk9; // Size: 0x1198

// MenuString-shaped sprite records plus one visibility flag per record (0xA1C bytes, allocated in misc9)
typedef struct {
    /* 0x0000 */ MenuString unk0[8];
    /* 0x0400 */ MenuString unk400[8];
    /* 0x0800 */ MenuString unk800;
    /* 0x0880 */ MenuString slotStrings[3]; // The three strings of the equipment slot being shown
    /* 0x0A00 */ void* pStringLists[4];
    /* 0x0A10 */ u8 shouldRenderRows[8];
    /* 0x0A18 */ u8 shouldRenderStrings880[3];
    /* 0x0A1B */ u8 unkA1B;
} MenuCharacterSpriteRecords; // Size: 0xA1C

// Character Info
typedef struct {
    /* 0x0   */ POLY_FT4 polysDescriptionStrings[18];
    /* 0x2D0 */ POLY_FT4 polys2D0[2]; // Unknown if bigger or not
    /* 0x320 */ u8 unk320[0x140];
    /* 0x460 */ POLY_FT4 polysPortraitSmall[2];
    /* 0x4B0 */ POLY_FT4 polys4B0[2];
    /* 0x500 */ POLY_FT4 polysLevelString[6];
    /* 0x5F0 */ POLY_FT4 polys5F0[6];
    /* 0x6E0 */ POLY_FT4 polysHpString[6];
    /* 0x7D0 */ u8 unk7D0[0xA0];
    /* 0x870 */ POLY_FT4 polysMaxHpString[6];
    /* 0x960 */ u8 unk960[0xA0];
    /* 0xA00 */ POLY_FT4 polysMpString[4];
    /* 0xAA0 */ u8 unkAA0[0x50];
    /* 0xAF0 */ POLY_FT4 polysMaxMpString[4];
    /* 0xB90 */ u8 unkB90[0x50];
    /* 0xBE0 */ u_char levelStringLength;
    /* 0xBE1 */ u8 polys5F0PrimCount;
    /* 0xBE2 */ u_char hpStringLength;
    /* 0xBE3 */ u_char maxHpStringLength;
    /* 0xBE4 */ u_char mpStringLength;
    /* 0xBE5 */ u_char maxMpStringLength;
    /* 0xBE6 */ u_char renderContext;
    /* 0xBE7 */ u8 shouldRender;
    /* 0xBE8 */ u_char descriptionStringsLength;
    /* 0xBE9 */ u8 polys2D0PrimCount;
    /* 0xBEA */ u8 unkBEA[0x2];
} MenuCharacter; // Size: 0xBEC

// Character info window prims, one prim per digit and render context (allocated in misc1).
// Every prim array is indexed [digit * 2 + renderContext].
typedef struct {
    /* 0x0000 */ POLY_FT4 polysPortrait[2]; // Party member portrait, one prim per render context
    /* 0x0050 */ POLY_FT4 polysName[2]; // The portrait's character name strip, one prim per render context
    /* 0x00A0 */ POLY_FT4 polysIcons[0x28];
    /* 0x06E0 */ u8 unk6E0[0x230];
    /* 0x0910 */ POLY_FT4 polysLevel[6];
    /* 0x0A00 */ POLY_FT4 polysUnk63Digits[6]; // Digits of GameCharacter.unk63, see MenuSetCharacterLevelDigits
    /* 0x0AF0 */ POLY_FT4 polysHp[6];
    /* 0x0BE0 */ POLY_FT4 polysMaxHp[6];
    /* 0x0CD0 */ POLY_FT4 polysMp[4];
    /* 0x0D70 */ POLY_FT4 polysMaxMp[4];
    /* 0x0E10 */ POLY_FT4 polysWideNumber[14];
    /* 0x1040 */ POLY_FT4 polysWideNumber2[14];
    /* 0x1270 */ u_char renderContext;
    /* 0x1271 */ u_char levelPrimCount;
    /* 0x1272 */ u_char unk63DigitsPrimCount;
    /* 0x1273 */ u_char hpPrimCount;
    /* 0x1274 */ u_char maxHpPrimCount;
    /* 0x1275 */ u_char mpPrimCount;
    /* 0x1276 */ u_char maxMpPrimCount;
    /* 0x1277 */ u_char wideNumberPrimCount;
    /* 0x1278 */ u_char wideNumber2PrimCount;
    /* 0x1279 */ u_char iconsPrimCount;
    /* 0x127A */ u8 _pad127A[0x2];
} MenuCharacterWindow; // Size: 0x127C

typedef struct {
    /* 0x0   */ POLY_FT4 polysWindowBorderCorners[8];
    /* 0x140 */ POLY_FT4 polysWindowBorderTop[4]; // First 2 = 1st half, Second 2 = 2nd half
    /* 0x1E0 */ POLY_FT4 polysWindowBorderBottom[4]; // First 2 = 1st half, Second 2 = 2nd half
    /* 0x280 */ POLY_FT4 polysWindowBorderLeft[4]; // First 2 = 1st half, Second 2 = 2nd half
    /* 0x320 */ POLY_FT4 polysWindowBorderRight[4]; // First 2 = 1st half, Second 2 = 2nd half
    /* 0x3C0 */ POLY_FT4 polysScrollBarEmpty[2];
    /* 0x410 */ POLY_FT4 polysScrollBarEnds[4];
    /* 0x4B0 */ POLY_G4 polysBackground[2];
    /* 0x4F8 */ DR_MODE drawModes[2];
    /* 0x510 */ SVECTOR vertsWindowBorderCorners[16];
    /* 0x590 */ SVECTOR vertsWindowBorderTop1[4]; // First half of top border
    /* 0x5B0 */ SVECTOR vertsWindowBorderTop2[4]; // Second half of top border
    /* 0x5D0 */ SVECTOR vertsWindowBorderBottom1[4]; // First half of bottom border
    /* 0x5F0 */ SVECTOR vertsWindowBorderBottom2[4]; // Second half of bottom border
    /* 0x610 */ SVECTOR vertsWindowBorderLeft1[4]; // First half of left border
    /* 0x630 */ SVECTOR vertsWindowBorderLeft2[4]; // Second half of left border
    /* 0x650 */ SVECTOR vertsWindowBorderRight1[4]; // First half of right border
    /* 0x670 */ SVECTOR vertsWindowBorderRight2[4]; // Second half of right border
    /* 0x690 */ SVECTOR vertsBackground[4];
    /* 0x6B0 */ SVECTOR vertsScrollBarEmpty[4];
    /* 0x6D0 */ SVECTOR vertsScrollBarEnds[8];
    /* 0x710 */ s32 borderPrimCount;
    /* 0x714 */ s32 useDefaultMatrix; // If 0, use default transform matrix when rendering
    /* 0x718 */ int zIndex;
    /* 0x71C */ u_char renderContext;
    /* 0x71D */ u_char hasScrollBar;
    /* 0x71E */ u8 _pad71E[0x2];
} MenuWindow; // Size: 0x720

typedef struct {
    /* 0x0  */  POLY_FT4 polysCursor[8];
    /* 0x140 */ u8 shouldRender[0x4]; // Should render / isActive?
    /* 0x144 */ u8 shouldUpdatePosition[0x4];
    /* 0x148 */ u8 renderContexts[0x4];
} MenuPointerCursors; // Size: 0x14C

typedef struct {
    /* 0x0  */ u_short x;
    /* 0x2  */ u_short y;
    /* 0x4  */ u_short width;
    /* 0x6  */ u_short height;
    /* 0x8  */ u_short curWidth; // Grows to width while the open animation runs
    /* 0xA  */ u_short curHeight; // Grows to height while the open animation runs
    /* 0xC  */ int zIndex;
    /* 0x10 */ u_char index;
    /* 0x11 */ u_char isOpen;
    /* 0x12 */ u_char useDefaultMatrix; // Stored for the deferred MenuSetWindow call
    /* 0x13 */ u_char hasScrollBar;
    /* 0x14 */ u8 unk14[0x4];
} MenuWindowParameters; // Size: 0x18

typedef struct {
    u_char itemIds[MAX_GEAR_SHOP_ITEMS];
} GearShopItemTableEntry;

// The shop's stock, one id table per gear part type: GearShopMenuLoadGearShopInventory copies
// them into MenuShop.gearShopItemTables[0..4], and that index is the ITEM_TYPE_GEAR_* the
// purchase code switches on.
typedef struct {
    /* 0x0  */ u_char frameIds[MAX_GEAR_SHOP_ITEMS];
    /* 0x14 */ u_char armorIds[MAX_GEAR_SHOP_ITEMS];
    /* 0x28 */ u_char engineIds[MAX_GEAR_SHOP_ITEMS];
    /* 0x3C */ u_char weaponIds[MAX_GEAR_SHOP_ITEMS];
    /* 0x50 */ u_char accessoryIds[MAX_GEAR_SHOP_ITEMS];
} GearShopDefinition; // Size: 0x64

// Shop data
typedef struct {
    /* 0x0    */ POLY_FT4 polysCharacterPortraits[9*2];
    /* 0x2D0  */ POLY_FT4 polys2D0[9*2]; // Letter 'E' (for Equip) on character portraits if item is equipped
    /* 0x5A0  */ POLY_FT4 polysExplanations[4 * 2];
    /* 0x6E0  */ POLY_FT4 unk6E0[0x24];
    /* 0xC80  */ POLY_FT4 polysGoldBefore[9 * 2];
    /* 0xF50  */ POLY_FT4 polysTotalPrice[9 * 2];
    /* 0x1220 */ POLY_FT4 polysFinalPrice[9 * 2];
    /* 0x14F0 */ POLY_FT4 polysSalesTotal[9*2];
    /* 0x17C0 */ POLY_FT4 polysGoldAmount[9*2];
    /* 0x1A90 */ POLY_FT4 unk1A90[2];
    /* 0x1AE0 */ POLY_FT4 polysGoldAfter[9 * 2];
    /* 0x1DB0 */ POLY_FT4 polysShopInventory[SHOP_MAX_ITEMS_IN_VIEW][2];
    /* 0x2030 */ POLY_FT4 unk2030[0x30];
    /* 0x27B0 */ POLY_FT4 polys27B0[9][3*2];
    /* 0x3020 */ POLY_FT4 polys3020[9][3*2];
    /* 0x3890 */ LINE_F3 linesPortraitHighlight1[9 * 2]; // Red highlight line around portraits, 1st half
    /* 0x3A40 */ LINE_F3 linesPortraitHighlight2[9 * 2]; // Red highlight line around portraits, 2nd half
    /* 0x3BF0 */ LINE_F2 lines3BF0[2]; // White line between gold amounts?
    /* 0x3C10 */ u8 unk3C10[0x20];
    /* 0x3C30 */ MenuString strItemNames[8];
    /* 0x4030 */ MenuString strItemQuantities[8];
    /* 0x4430 */ MenuString strItemDesc;
    /* 0x44B0 */ MenuString str44B0;
    /* 0x4530 */ MenuString str4530;
    /* 0x45B0 */ MenuString str45B0;
    /* 0x4630 */ void* pItemDescriptions;
    /* 0x4634 */ void* pWeaponDescriptions;
    /* 0x4638 */ void* pAccessoryDescriptions;
    /* 0x463C */ void* pGearFrameDescriptions;
    /* 0x4640 */ void* pGearEngineDescriptions;
    /* 0x4644 */ void* pAmmoDescriptions;
    /* 0x4648 */ void* pGearArmorNames;
    /* 0x464C */ void* pGearFrameNames;
    /* 0x4650 */ void* pGearEngineNames;
    /* 0x4654 */ u_char curItemQuantities[MAX_SHOP_ITEMS];
    /* 0x4684 */ u8 shouldRenderItemRows[0x8];
    /* 0x468C */ u8 inventoryPrimCounts[0x8];
    /* 0x4694 */ u8 inventoryRenderCtxs[0x8];
    /* 0x469C */ u8 shouldRenderPortraitHighlight[0x9];
    /* 0x46A5 */ u_char numPortraits;
    /* 0x46A6 */ u_char portraitsRenderCtx;
    /* 0x46A7 */ u8 shouldRenderItemDesc;
    /* 0x46A8 */ u8 equipMarkRenderCtx;
    /* 0x46A9 */ u8 equipMarkPrimCount;
    /* 0x46AA */ u_char explanationsRenderCtx;
    /* 0x46AB */ u_char explanationsLen;
    /* 0x46AC */ u_char goldBeforeRenderCtx;
    /* 0x46AD */ u_char goldBeforeStrLen;
    /* 0x46AE */ u_char totalPriceRenderCtx;
    /* 0x46AF */ u_char totalPriceStrLen;
    /* 0x46B0 */ u_char goldAfterRenderCtx;
    /* 0x46B1 */ u_char goldAfterStrLen;
    /* 0x46B2 */ u8 shouldRenderPrices;
    /* 0x46B3 */ u_char finalPriceRenderCtx;
    /* 0x46B4 */ u_char finalPriceStrLen;
    /* 0x46B5 */ u8 shouldRenderStr44B0;
    /* 0x46B6 */ u8 shouldRenderStr4530;
    /* 0x46B7 */ u8 salesTotalRenderCtx;
    /* 0x46B8 */ u8 salesTotalPrimCount;
    /* 0x46B9 */ u8 shouldRenderSalesTotal;
    /* 0x46BA */ u8 goldAmountRenderCtx;
    /* 0x46BB */ u8 goldAmountPrimCount;
    /* 0x46BC */ u8 polys27B0PrimCounts[0x9];
    /* 0x46C5 */ u8 polys3020PrimCounts[0x9];
    /* 0x46CE */ u8 polys27B0RenderCtxs[0x9];
    /* 0x46D7 */ u8 polys3020RenderCtxs[0x9];
    /* 0x46E0 */ u16 previousTotalAttacks[0x10];
    /* 0x4700 */ u16 previousTotalDefenses[0x10];
    // /* 0x4720 */ u_char gearShopItemTables[5 * 20]; // Frames, armor, engines, weapons, parts. Not necessarily in that order.
    /* 0x4720 */ GearShopItemTableEntry gearShopItemTables[5]; // Frames, armor, engines, weapons, parts. Not necessarily in that order.
    /* 0x4784 */ u8 unk4784;
    /* 0x4785 */ u8 shouldRenderStr45B0;
    /* 0x4786 */ u8 unk4786[0x2];
} MenuShop; // Size: 0x4788

typedef struct {
    /* 0x0  */ POLY_FT4 polys[2];
    /* 0x50 */ SVECTOR vertices[4];
    /* 0x70 */ u_char renderContext;
    /* 0x71 */ u8 unk71[0x3];
} MenuScrollBarHandle; // Size: 0x74

typedef struct {
    /* 0x000 */ POLY_FT4 polys[8]; // L1 / R1 Red Arrow polys
    /* 0x140 */ SVECTOR vertices[16]; // Vertices for ^
    /* 0x1C0 */ u_char renderContext;
    /* 0x1C1 */ u8 unk1C1[3];
} MenuShoulderButtonUi; /* size: 0x1C4 */

typedef struct {
    /* 0x0  */ POLY_FT4 polys[2];
    /* 0x50 */ SVECTOR vertices[4];
    /* 0x70 */ int curAnimFrame;
    /* 0x74 */ u_char animFrameDuration;
    /* 0x75 */ u_char renderContext;
    /* 0x76 */ u8 unk76;
    /* 0x77 */ u8 unk77;
} MenuArrowCursor; // Size: 0x78

typedef struct {
    /* 0x00 */ u8 *pModelSource; // Archive entry loaded next to pModelArchive, handed to func_801E742C as its ModelSource
    /* 0x04 */ u8 *pModelArchive; // Archive entry loaded next to pModelSource, handed to func_801E742C as its ModelArchive
    /* 0x08 */ SVECTOR position; // Becomes the gear model skeleton's translation when the model is created
    /* 0x10 */ u16 unk10;
    /* 0x12 */ s8 isModelCreated; // Set by GearShopMenuCreateGearModel, cleared before LibGearFreeModel
    /* 0x13 */ u8 unk13;
} SystemMenuArchiveData; /* size: 0x14 */

// Play time display prims, allocated in misc1: 11 slots of one prim per render context.
typedef struct {
    /* 0x000 */ POLY_FT4 polys[11][2];
    /* 0x370 */ u_char renderContext;
    /* 0x371 */ u8 _pad371[0x3];
} MenuPlayTimeDigits; // Size: 0x374

typedef struct {
    /* 0x0    */ u8 windowTweens[108]; // 3 x 0x24 tween records, one per window, see MenuWindowTween in misc7
    /* 0x6C   */ GfxEnvironment gfxEnvs[2];
    /* 0x1D4  */ GfxEnvironment* pGfxEnv;
    /* 0x1D8  */ SVECTOR rotation;
    /* 0x1E0  */ VECTOR translation;
    /* 0x1F0  */ MATRIX matTransform;
    /* 0x210  */ u8 unk210[0x8];
    /* 0x218  */ SVECTOR modelRotation;
    /* 0x220  */ VECTOR cameraPosition;
    /* 0x230  */ MATRIX modelMatrix;
    /* 0x250  */ u8 unk250[0x48];
    /* 0x298  */ MATRIX viewMatrix; // Gear model view matrix, handed to LibGearRenderFrame beside modelMatrix and used as func_801DCEC8's pViewMatrix
    /* 0x2B8  */ MATRIX colorMatrix; // Gear model colour matrix, handed to SetColorMatrix through D_801E8644
    /* 0x2D8  */ u32 idleFrameCount;
    /* 0x2DC  */ MenuResourceDirectory *resources;
    /* 0x2E0  */ void* pStringBank; // Decompressed string bank, indexes through GetStringEntry
    /* 0x2E4  */ SoundFile* pSoundFile; // Pointer to SEDS file
    /* 0x2E8  */ undefined32 unk2E8;
    /* 0x2EC  */ s32 playTimeDigitIds[7]; // Texture ids of the play time digits
    /* 0x308  */ int renderContext;
    /* 0x30C  */ u_char availableCharacters[0x10];
    /* 0x31C  */ u_char digits[0x9]; // Buffer for numbers parsed into a string
    /* 0x325  */ u_char input;
    /* 0x326  */ u8 cardCheckTimer; // Frames until the card slots are polled again
    /* 0x327  */ u_char shouldDrawMenu;
    /* 0x328  */ u8 unk328;
    /* 0x329  */ u_char transitionEffectState;
    /* 0x32A  */ u8 soundEffectsEnabled; // Sound effects enabled?
    /* 0x32B  */ u8 availableCharacterCount; // Party slots holding an available character, built in MenuBuildAvailableCharacters
    /* 0x32C  */ MenuUnk2* menuUnk2;
    /* 0x330  */ MenuDressingRoom* pDressingRoom;
    /* 0x334  */ u8 unk334;
    /* 0x335  */ u8 unk335;
    /* 0x336  */ u_char mainMenuChoice;
    /* 0x337  */ u_char mainMenuPrevChoice;
    /* 0x338  */ u_char subMenuChoice;
    /* 0x339  */ u_char subMenuPrevChoice;
    /* 0x33A  */ u_char subMenuNumChoices;
    /* 0x33B  */ u8 gearCharacterCount; // Party slots whose character has a gear, built in MenuBuildAvailableCharacters
    /* 0x33C */ MenuManager* pManager;
    /* 0x340  */ void* pGoldWindow; // Gold-window prim block, see MenuGoldWindow in misc7
    /* 0x344  */ MenuPlayTimeDigits* pPlayTimeDigits;
    /* 0x348  */ MenuUnk1* pCursorPrims; // Selection cursor prims and the shading behind them
    /* 0x34C  */ void* pPrimBuffer; // Scratch prim buffer, see MenuAllocatePrimBuffer
    /* 0x350  */ MenuSelectionMenu* pSelectionMenu;
    /* 0x354  */ MenuUnk5* pSubmenuPrims; // Submenu option cursor and text prims
    /* 0x358  */ void* pGearWindowPrims; // Gear window prim block, see MenuGearWindowPrims in misc7
    /* 0x35C  */ MenuUnk10* unk35C;
    /* 0x360  */ void* pEquipmentPanel; // Stat values and the equipped weapon/accessory ids being edited
    /* 0x364  */ MenuWindow* windows[MENU_MAX_NUM_WINDOWS];
    /* 0x380  */ MenuWindowParameters* windowParameters[MENU_MAX_NUM_WINDOWS];
    /* 0x39C  */ MenuCharacterWindow* characterWindows[MENU_CHARACTER_WINDOW_COUNT];
    /* 0x3A8  */ void* pCellPrimBlocks[32]; // One prim block per character cell, see MenuCreateCellPrimBlocks
    /* 0x428  */ MenuPointerCursors* pCursors;
    /* 0x42C  */ MenuUnk9* pItemRows; // Item name and quantity rows
    /* 0x430  */ void* unk430;
    /* 0x434  */ MenuCharacterSpriteRecords* pCharacterSpriteRecords;
    /* 0x438  */ void* unk438;
    /* 0x43C  */ MenuScrollBarHandle* pScrollHandle;
    /* 0x440  */ MenuShoulderButtonUi* pShoulderButtonUi;
    /* 0x444  */ MenuArrowCursor* arrowCursors[MENU_MAX_NUM_ARROW_CURSORS];
    /* 0x44C  */ void* pCellGrid; // Cell grid panel block, allocated in MenuSetCellGridPanelState, drawn by MenuAddCellGridPrims
    /* 0x450  */ MenuShop* pShop;
    /* 0x454  */ MenuUnk8* menuUnk8;
    /* 0x458  */ SystemMenuArchiveData *pGearModelArchiveData[2]; // Per gear-model slot: its two archive files, load position and isModelCreated
    /* 0x460  */ u8 unk460[0x8];
    /* 0x468  */ u8 unk468;
    /* 0x469  */ u8 unk469;
    /* 0x46A  */ u8 unk46A;
    /* 0x46B  */ u8 unk46B;

    // Window borders
    /* 0x46C  */ u32 texCount0;
    /* 0x470  */ s32 texPage0;
    /* 0x474  */ s32 clutX0;
    /* 0x478  */ s32 clutY0;
    /* 0x47C  */ s32 texPageX0;
    /* 0x480  */ s32 texPageY0;
    /* 0x484  */ u32 texCount1;
    /* 0x488  */ s32 texPage1;
    /* 0x48C  */ s32 clutX1;
    /* 0x490  */ s32 clutY1;
    /* 0x494  */ s32 texPageX1;
    /* 0x498  */ s32 texPageY1;
    /* 0x49C  */ u32 texCount2;
    /* 0x4A0  */ s32 texPage2;
    /* 0x4A4  */ s32 clutX2;
    /* 0x4A8  */ s32 clutY2;
    /* 0x4AC  */ s32 texPageX2;
    /* 0x4B0  */ s32 texPageY2;
    /* 0x4B4  */ u32 texCount3;
    /* 0x4B8  */ s32 texPage3;
    /* 0x4BC  */ s32 clutX3;
    /* 0x4C0  */ s32 clutY3;
    /* 0x4C4  */ s32 texPageX3;
    /* 0x4C8  */ s32 texPageY3;

    /* 0x4CC  */ s32 characterCellFrame; // Indexes the 6-entry colour table stepped every 15 frames
    /* 0x4D0  */ s32 characterCellTimer; // Frames spent on the current characterCellFrame
    /* 0x4D4  */ s32 characterCellShade; // Written to every colour channel of the character cell prims
    /* 0x4D8  */ u8 cardSlotState; // 0 off, 1 set while the card slots are read, 2 slot list ready
    /* 0x4D9  */ u8 characterCellShadeReversing;
    /* 0x4DA  */ u8 unk4DA[0x2];
    /* 0x4DC  */ u_char characterSlot; // Party slot the dressing room / item cursor sits on
    /* 0x4DD  */ u8 unk4DD[0x3];
    /* 0x4E0  */ MenuString titleStrings[4]; // The current menu screen's titles, uploaded with its title string ids
    /* 0x6E0  */ MenuString optionStrings[8]; // Option strings of the current screen
    /* 0xAE0  */ MenuString subMenuChoiceStrings[6];
    /* 0xDE0  */ MenuString unkDE0[6];
    // Three arrays of MenuString-shaped sprite records (0x80 each)
    /* 0x10E0 */ MenuString itemRowStrings[8]; // One string per inventory/item-list row, see g_MenuItemRowStringIds
    /* 0x14E0 */ MenuString characterRowStrings[6]; // One string per character panel row, see g_MenuCharacterRowStringIds
    /* 0x17E0 */ MenuString unk17E0[2];
    /* 0x18E0 */ MenuString unk18E0[6];
    /* 0x1BE0 */ MenuString unk1BE0[4];
    /* 0x1DE0 */ MenuString* modalStrings[4];
    /* 0x1DF0 */ MenuCharacter* benchedCharacters[MAX_BENCHED_PARTY_MEMBERS];
    /* 0x1E08 */ MenuCharacter* currentCharacters[MAX_PARTY_MEMBERS];
    /* 0x1E14 */ u8 characterIds[MAX_GAME_CHARACTERS]; // Character id per party slot, 0xFF = empty
    /* 0x1E1F */ u8 unk1E1F;
    /* 0x1E20 */ MenuUnk7* unk1E20;
    /* 0x1E24 */ u8 unk1E24[0x8];
    /* 0x1E2C */ void* pShopEntries;
    /* 0x1E30 */ u_char shopItemIDs[MAX_SHOP_ITEMS];
    /* 0x1E60 */ u_char shopItemTypes[MAX_SHOP_ITEMS];
    /* 0x1E90 */ GearShopDefinition* pGearShopEntries; // Array of GearShopDefinition
    /* 0x1E94 */ u8 debuggerEnabled;
    // Decompiling `MenuProcessControllerInput` worked better if this field is
    // volatile.  If you need to remove `volatile`, you can insert a temporary
    // variable instead as done here: https://decomp.me/scratch/xsigV
    /* 0x1E95 */ volatile u8 debuggerPage;
    /* 0x1E96 */ u8 unk1E96[2];
} SystemMenu; // Size: 0x1E98

extern SystemMenu* g_Menu;
extern u8 g_MenuDebugEnabled;

#endif