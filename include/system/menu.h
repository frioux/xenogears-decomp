#ifndef _XENO_MENU_H
#define _XENO_MENU_H

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

// Transition animation states
#define MENU_ANIMATION_DONE 0x0
#define MENU_OPEN_ANIMATION 0x1
#define MENU_CLOSE_ANIMATION 0x2
#define MENU_OPEN_ANIMATION_START 0x3
#define MENU_CLOSE_ANIMATION_START 0x4

#define MENU_MAX_NUM_WINDOWS 0x7
#define MENU_MAX_NUM_CURSORS 0x4
#define MENU_MAX_NUM_ARROW_CURSORS 0x2
#define MENU_WINDOW_BORDER_SIZE 0x8
#define MENU_MAX_DIGITS 0x9

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
    /* 0x4 */ u8 unk4[0xC];
} MenuShopItem; // Size: 0x10

typedef struct {
    /* 0x00 */ u32 equipFlags;
    /* 0x04 */ u16 price;
    /* 0x06 */ u16 unk6;
    /* 0x08 */ u16 unk8;
    /* 0x0A */ u16 unkA;
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
    /* 0x7C */ u8 unk7C; // Palette?
    /* 0x7D */ u8 renderContext;
    /* 0x7E */ u8 width;
    /* 0x7F */ u8 unk7F;
} MenuString; // Size: 0x80

typedef struct {
    /* 0x0 */ u_short equipFlags; // Which characters can use this item?
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u_short price;
    /* 0x6 */ u8 unk6; // group id
    /* 0x7 */ u8 unk7[5];
    /* 0xC */ u8 attack;
    /* 0xD */ u8 unkD[3];
} MenuShopWeapon; // Size: 0x10

typedef struct {
    /* 0x0 */ u_short equipFlags; // Which characters can use this item?
    /* 0x2 */ u_short price;
    /* 0x4 */ u8 unk4[4];
    /* 0x8 */ u8 defense;
    /* 0x9 */ u8 unk9[5];
    /* 0xE */ u16 unkE; // group id
} MenuShopAccessory; // Size: 0x10

typedef struct {
    /* 0x0  */ DRAWENV drawEnv;
    /* 0x5C */ DISPENV dispEnv;
    /* 0x70 */ unsigned long ot[16];  // 0x70 = ot[0], 0x74 = ot[1], 0x78 = ot[2], 0x7C = ot[3], 0x80 = ot[4]
    /* 0xB0 */ u32 unkB0;
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
    /* 0x159 */ u8 unk159;
    /* 0x15A */ u8 unk15A;
    /* 0x15B */ u8 unk15B;
} MenuUnk1; // Size: 0x15C

typedef struct {
    /* 0x00 */ u8 unk0[0x18];
    /* 0x18 */ u8 ids[0xC];
    /* 0x24 */ u8 unk24[0x38];
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
    /* 0x4B94 */ u8 unk4B94;
    /* 0x4B95 */ u8 unk4B95;
    /* 0x4B96 */ u8 unk4B96;
    /* 0x4B97 */ u8 unk4B97;
    /* 0x4B98 */ u8 unk4B98[0x5C];
    /* 0x4BF4 */ u8 unk4BF4[0x20];
    /* 0x4C14 */ u8 unk4C14[0x80];
    /* 0x4C94 */ u8 unk4C94[0x2E0];
    /* 0x4F74 */ s32 unk4F74[2];
    /* 0x4F7C */ s32 unk4F7C;
    /* 0x4F80 */ s32 unk4F80;
    /* 0x4F84 */ u8 unk4F84[0xA];
    /* 0x4F8E */ u8 unk4F8E[0x20];
    /* 0x4FAE */ u8 unk4FAE[0x20];
    /* 0x4FCE */ u8 unk4FCE[0xC];
    /* 0x4FDA */ u8 unk4FDA;
    /* 0x4FDB */ u8 unk4FDB;
    /* 0x4FDC */ u8 unk4FDC[0x8];
    /* 0x4FE4 */ u8 unk4FE4;
    /* 0x4FE5 */ u8 unk4FE5;
    /* 0x4FE6 */ u8 unk4FE6;
    /* 0x4FE7 */ u8 unk4FE7[0x4D];
} MenuUnk2; // Size: 0x5034

typedef struct {
    /* 0x0  */ u8 unk0[0x3];
    /* 0x3  */ u8 unk3;
    /* 0x4  */ u8 unk4; // Should we draw the pointer cursor on ball selection windows?
    /* 0x5  */ u8 unk5[0x4];
    /* 0x9  */ u8 shouldRenderSelectionMenu;
    /* 0xA  */ u8 unkA; // Should we draw MenuUnk5 strings?
    /* 0xB  */ u8 unkB;
    /* 0xC  */ u8 unkC[0x8];
    /* 0x14 */ u8 unk14[0x6];
    /* 0x1A */ u8 unk1A[0x6];
    /* 0x20 */ u8 shouldRenderWindow[MENU_MAX_NUM_WINDOWS];
    /* 0x27 */ u8 unk27[MENU_MAX_NUM_WINDOWS];
    /* 0x2B */ u8 unk2B;
    /* 0x2C */ u_char shouldRenderCursors;
    /* 0x30 */ u8 currentCharacterIDs[0x3];
    /* 0x33 */ u8 _pad33;
    /* 0x34 */ u8 unk34[0x4];
    /* 0x38 */ u8 unk38[0xE];
    /* 0x46 */ u8 unk46; // Should render characters?
    /* 0x47 */ u8 unk47;
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u_char scrollHandleActive;
    /* 0x4A */ u8 unk4A[0x6];
    /* 0x50 */ u8 shouldRenderArrowCursor[MENU_MAX_NUM_ARROW_CURSORS]; // Should render arrow cursor (array, size 3?)
    /* 0x52 */ u8 unk52;
    /* 0x53 */ u_char shoulderButtonUiActive;
    /* 0x54 */ u8 unk54[0x6];
    /* 0x5A */ u8 unk5A;
    /* 0x5B */ u8 unk5B;
    /* 0x5C */ u8 unk5C[0x10];
} MenuManager; // Size: 0x6C

// A menu with N options, each having a cursor and some text/content
typedef struct {
    /* 0x0    */ POLY_FT4 polysCursors[28 * 2]; // Cursor for each menu option
    /* 0x8C0  */ POLY_FT4 polysTexts[28 * 2]; // Texts / content for each menu option
    /* 0x1180 */ RECT unk1180;
    /* 0x1188 */ int numCursors;
    /* 0x118C */ int numTexts;
    /* 0x1190 */ u_char cursorsRenderCtx;
    /* 0x1191 */ u_char textsRenderCtx;
    /* 0x1192 */ u_char unk1192; // If set to true, and the field below is not true, fade out the menu gfx
    /* 0x1193 */ u_char unk1193; // 
} MenuSelectionMenu; // Size: 0x1194

typedef struct {
    /* 0x0    */ POLY_FT4 polys0[16 * 2]; // Submenu options ball cursor?
    /* 0x500  */ POLY_FT4 polys500[16 * 2]; // Submenu options text?
    /* 0xA00  */ u8 unkA00[0xA00];
    /* 0x1400 */ int unk1400; // polys0 count
    /* 0x1404 */ int unk1404; // polys500 count
    /* 0x1408 */ u_char unk1408; // polys0 render context
    /* 0x1409 */ u_char unk1409; // polys500 render context
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
    /* 0x14 */ u8 unk14; // 10 -- agility?
    /* 0x15 */ u8 unk15; // 10 -- eth amp?
    /* 0x16 */ u8 unk16; // 0
    /* 0x17 */ u8 unk17; // 0
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
    /* 0x08 */ u16 unk8;
    /* 0x0A */ u16 unkA;
    /* 0x0C */ u8 unkC;
    /* 0x0D */ u8 defense;
    /* 0x0E */ u8 etherDefense;
    /* 0x0F */ u8 unkF;
    /* 0x10 */ u8 unk10[4];
    /* 0x14 */ u8 responsiveness;
    /* 0x15 */ u8 unk15;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u8 unk18;
    /* 0x19 */ u8 unk19;
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 unk1B;
} GearAccessory; /* size: 0x1C */


// TODO: these are placeholders so the struct is the right size
typedef struct {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ u32 equipFlags;
    /* 0x08 */ u16 price;
    /* 0x0A */ u16 unkA;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u8 unkE;
    /* 0x0F */ u8 unkF;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 unk11;
    /* 0x12 */ u16 unk12;
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
    /* 0x20 */ u8 unk20[0x7C];
    /* 0x9C */ int hp; // hp
    /* 0xA0 */ int maxHp; // maxHp
    /* 0xA4 */ u16 totalDefense; // totalDefense
    /* 0xA6 */ u16 totalEtherDefense; // totalEtherDefense
    /* 0xA8 */ u16 totalWeight;
    /* 0xAA */ u16 unkAA;
    /* 0xAC */ u16 fuel;
    /* 0xAE */ u16 maxFuel;
    /* 0xB0 */ u16 unkB0; // maybe ether??
    /* 0xB2 */ u8 totalResponsiveness; // responsiveness
    /* 0xB3 */ u8 unkB3;
    /* 0xB4 */ u8 unkB4;
    /* 0xB5 */ u8 unkB5;
    /* 0xB6 */ u8 unkB6;
    /* 0xB7 */ u8 unkB7;
    /* 0xB8 */ u16 unkB8[9];
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
    /* 0x1ED0 */ u8 unk1ED0;
    /* 0x1ED1 */ u8 unk1ED1[2];
    /* 0x1ED3 */ u8 unk1ED3;
    /* 0x1ED4 */ u8 unk1ED4;
    /* 0x1ED5 */ u8 unk1ED5[2];
    /* 0x1ED6 */ u8 unk1ED7;
    /* 0x1ED7 */ u8 unk1ED8;
    /* 0x1ED9 */ u8 unk1ED9[2];
    /* 0x1EDB */ u8 unk1EDB;
    /* 0x1EDC */ u8 unk1EDC;
    /* 0x1EDD */ u8 unk1EDD[2];
    /* 0x1EDF */ u8 unk1EDF;
    /* 0x1EE0 */ u8 unk1EE0;
    /* 0x1EE1 */ u8 unk1EE1;
    /* 0x1EE2 */ s16 unk1EE2;
    /* 0x1EE4 */ s16 unk1EE4[2];
    /* 0x1EE8 */ u8 unk1EE8; // Gear HP digits string length
    /* 0x1EE9 */ u8 unk1EE9; // Gear Max HP digits string length
    /* 0x1EEA */ u8 unk1EEA; // Gear Fuel digits string length
    /* 0x1EEB */ u8 unk1EEB; // Gear Max fuel digits string length
    /* 0x1EEC */ u8 unk1EEC; // Gear Weight digits string length
    /* 0x1EED */ u8 unk1EED; // Render context for the gear stat strings
    /* 0x1EEE */ u8 unk1EEE;
    /* 0x1EEF */ u8 unk1EEF;
    /* 0x1EF0 */ u16 unk1EF0;
    /* 0x1EF2 */ u16 unk1EF2;
    /* 0x1EF4 */ u16 unk1EF4[2];
    /* 0x1EF8 */ u16 unk1EF8[2];
    /* 0x1EFC */ u16 unk1EFC;
    /* 0x1EFE */ u16 unk1EFE;
} MenuUnk8; /* size: 1F00 */

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
    /* 0xBE1 */ u8 unkBE1; // Num polys5F0
    /* 0xBE2 */ u_char hpStringLength;
    /* 0xBE3 */ u_char maxHpStringLength;
    /* 0xBE4 */ u_char mpStringLength;
    /* 0xBE5 */ u_char maxMpStringLength;
    /* 0xBE6 */ u_char renderContext;
    /* 0xBE7 */ u8 unkBE7; // Is active / should render?
    /* 0xBE8 */ u_char descriptionStringsLength;
    /* 0xBE9 */ u8 unkBE9;
    /* 0xBEA */ u8 unkBEA[0x2];
} MenuCharacter; // Size: 0xBEC

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
    /* 0x710 */ s32 unk710;
    /* 0x714 */ s32 unk714; // If 0, use default transform matrix when rendering
    /* 0x718 */ int zIndex;
    /* 0x71C */ u_char renderContext;
    /* 0x71D */ u_char hasScrollBar;
    /* 0x71E */ u8 _pad71E[0x2];
} MenuWindow; // Size: 0x720

typedef struct {
    /* 0x0  */  POLY_FT4 polysCursor[8];
    /* 0x140 */ u8 shouldRender[0x4]; // Should render / isActive?
    /* 0x144 */ u8 unk144[0x4]; // Should update positions?
    /* 0x148 */ u8 renderContexts[0x4];
} MenuPointerCursors; // Size: 0x14C

typedef struct {
    /* 0x0  */ u_short x;
    /* 0x2  */ u_short y;
    /* 0x4  */ u_short width;
    /* 0x6  */ u_short height;
    /* 0x8  */ u_short unk8; // Width?
    /* 0xA  */ u_short unkA; // Height?
    /* 0xC  */ int zIndex;
    /* 0x10 */ u_char index;
    /* 0x11 */ u_char unk11;
    /* 0x12 */ u_char unk12;
    /* 0x13 */ u_char hasScrollBar;
    /* 0x13 */ u8 unk14[0x4];
} MenuWindowParameters; // Size: 0x18

typedef struct {
    u_char itemIds[MAX_GEAR_SHOP_ITEMS];
} GearShopItemTableEntry;

// Engines, frames, armors, weapons, parts.
// Not necessarily in that order.
typedef struct {
    /* 0x0  */ u_char unk0[MAX_GEAR_SHOP_ITEMS];
    /* 0x14 */ u_char unk14[MAX_GEAR_SHOP_ITEMS];
    /* 0x28 */ u_char unk28[MAX_GEAR_SHOP_ITEMS];
    /* 0x3C */ u_char unk3C[MAX_GEAR_SHOP_ITEMS];
    /* 0x50 */ u_char unk50[MAX_GEAR_SHOP_ITEMS];
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
    /* 0x14F0 */ POLY_FT4 unk14F0[9*2]; // I suspect these are the "sell" versions of the above
    /* 0x17C0 */ POLY_FT4 unk17C0[9*2];
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
    /* 0x3C30 */ MenuString unk3C30[8];
    /* 0x4030 */ MenuString unk4030[8];
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
    /* 0x4684 */ u8 unk4684[0x8];
    /* 0x468C */ u8 unk468C[0x8];
    /* 0x4694 */ u8 unk4694[0x8];
    /* 0x469C */ u8 unk469C[0x9];
    /* 0x46A5 */ u_char numPortraits;
    /* 0x46A6 */ u_char portraitsRenderCtx;
    /* 0x46A7 */ u8 unk46A7;
    /* 0x46A8 */ u8 unk46A8;
    /* 0x46A9 */ u8 unk46A9;
    /* 0x56AA */ u_char explanationsRenderCtx;
    /* 0x46AB */ u_char explanationsLen;
    /* 0x46AC */ u_char goldBeforeRenderCtx;
    /* 0x46AD */ u_char goldBeforeStrLen;
    /* 0x46AE */ u_char totalPriceRenderCtx;
    /* 0x46AF */ u_char totalPriceStrLen;
    /* 0x46B0 */ u_char goldAfterRenderCtx;
    /* 0x46B1 */ u_char goldAfterStrLen;
    /* 0x46B2 */ u8 unk46B2;
    /* 0x46B3 */ u_char finalPriceRenderCtx;
    /* 0x46B4 */ u_char finalPriceStrLen;
    /* 0x46B5 */ u8 unk46B5;
    /* 0x46B6 */ u8 unk46B6[0x6];
    /* 0x46BC */ u8 unk46BC[0x9]; // Number string Lengths
    /* 0x46C5 */ u8 unk46C5[0x9]; // Number string Lengths
    /* 0x46CE */ u8 unk46CE[0x9]; // Render contexts
    /* 0x46D7 */ u8 unk46D7[0x9]; // Render contexts
    /* 0x46E0 */ u16 unk46E0[0x10];
    /* 0x4700 */ u16 unk4700[0x10];
    // /* 0x4720 */ u_char gearShopItemTables[5 * 20]; // Frames, armor, engines, weapons, parts. Not necessarily in that order.
    /* 0x4720 */ GearShopItemTableEntry gearShopItemTables[5]; // Frames, armor, engines, weapons, parts. Not necessarily in that order.
    /* 0x4784 */ u8 unk4784;
    /* 0x4785 */ u8 unk4785;
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
    /* 0x00 */ u8 *unk0; // data read from CD
    /* 0x04 */ u8 *unk4;
    /* 0x08 */ int unk8;
    /* 0x0C */ int unkC;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ s8 unk12;
    /* 0x13 */ u8 unk13;
} SystemMenuArchiveData; /* size: 0x14 */

typedef struct {
    /* 0x0    */ u8 unk0[108];
    /* 0x6C   */ GfxEnvironment gfxEnvs[2];
    /* 0x1D4  */ GfxEnvironment* pGfxEnv;
    /* 0x1D8  */ SVECTOR rotation;
    /* 0x1E0  */ VECTOR translation;
    /* 0x1F0  */ MATRIX matTransform;
    /* 0x210  */ u8 unk1D8[0x8];
    /* 0x218  */ SVECTOR unk218;
    /* 0x220  */ VECTOR cameraPosition;
    /* 0x230  */ MATRIX unk230;
    /* 0x250  */ u8 unk250[0x48];
    /* 0x298  */ s16 unk298;
    /* 0x29A  */ s16 unk29A;
    /* 0x29C  */ s16 unk29C;
    /* 0x29E  */ s16 unk29E;
    /* 0x2A0  */ s16 unk2A0;
    /* 0x2A2  */ s16 unk2A2;
    /* 0x2A4  */ s16 unk2A4;
    /* 0x2A6  */ s16 unk2A6;
    /* 0x2A8  */ s16 unk2A8;
    /* 0x2AA  */ s16 unk2AA;
    /* 0x2AC  */ s16 unk2AC;
    /* 0x2AE  */ s16 unk2AE;
    /* 0x2B0  */ s16 unk2B0;
    /* 0x2B2  */ s16 unk2B2;
    /* 0x2B4  */ s16 unk2B4;
    /* 0x2B6  */ s16 unk2B6;
    /* 0x2B8  */ s16 unk2B8;
    /* 0x2BA  */ s16 unk2BA;
    /* 0x2BC  */ s16 unk2BC;
    /* 0x2BE  */ s16 unk2BE;
    /* 0x2C0  */ s16 unk2C0;
    /* 0x2C2  */ s16 unk2C2;
    /* 0x2C4  */ s16 unk2C4;
    /* 0x2C6  */ s16 unk2C6;
    /* 0x2C8  */ s16 unk2C8;
    /* 0x2CA  */ s16 unk2CA[6];
    /* 0x2D8  */ u32 unk2D8;
    /* 0x2DC  */ MenuResourceDirectory *resources;
    /* 0x2E0  */ void* unk2E0; // Pointer to resources (bin 3)
    /* 0x2E4  */ SoundFile* unk2E4; // Pointer to SEDS file
    /* 0x2E8  */ undefined32 unk2E8;
    /* 0x2EC  */ u8 unk2EC[0x1C];
    /* 0x308  */ int renderContext;
    /* 0x30C  */ u_char availableCharacters[0x10];
    /* 0x31C  */ u_char digits[0x9]; // Buffer for numbers parsed into a string
    /* 0x325  */ u_char input;
    /* 0x326  */ u8 unk326;
    /* 0x327  */ u_char shouldDrawMenu;
    /* 0x328  */ u8 unk328;
    /* 0x329  */ u_char transitionEffectState;
    /* 0x32A  */ u8 unk32A; // Sound effects enabled?
    /* 0x32B  */ u8 unk32B;
    /* 0x32C  */ MenuUnk2* menuUnk2;
    /* 0x330  */ MenuDressingRoom* pDressingRoom;
    /* 0x334  */ u8 unk334;
    /* 0x335  */ u8 unk335;
    /* 0x336  */ u_char mainMenuChoice;
    /* 0x337  */ u_char mainMenuPrevChoice;
    /* 0x338  */ u_char subMenuChoice;
    /* 0x339  */ u_char subMenuPrevChoice;
    /* 0x33A  */ u_char subMenuNumChoices;
    /* 0x33B  */ u8 unk33B;
    /* 0x33C */ MenuManager* pManager;
    /* 0x340  */ void* unk340;
    /* 0x344  */ void* unk344;
    /* 0x348  */ MenuUnk1* unk348;
    /* 0x34C  */ void* unk34C;
    /* 0x350  */ MenuSelectionMenu* pSelectionMenu;
    /* 0x354  */ MenuUnk5* unk354;
    /* 0x358  */ u8 unk358[0xC];
    /* 0x364  */ MenuWindow* windows[MENU_MAX_NUM_WINDOWS];
    /* 0x380  */ MenuWindowParameters* windowParameters[MENU_MAX_NUM_WINDOWS];
    /* 0x39C  */ void* unk39C[3];
    /* 0x3A8  */ void* unk3A8[32];
    /* 0x428  */ MenuPointerCursors* pCursors;
    /* 0x42C  */ u8 unk42C[0x10];
    /* 0x43C  */ MenuScrollBarHandle* pScrollHandle;
    /* 0x440  */ MenuShoulderButtonUi* pShoulderButtonUi;
    /* 0x444  */ MenuArrowCursor* arrowCursors[MENU_MAX_NUM_ARROW_CURSORS];
    /* 0x44C  */ u8 unk44C[0x4];
    /* 0x450  */ MenuShop* pShop;
    /* 0x454  */ MenuUnk8* menuUnk8;
    /* 0x458  */ SystemMenuArchiveData *unk458[2];
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

    /* 0x4CC  */ s32 unk4CC;
    /* 0x4D0  */ s32 unk4D0;
    /* 0x4D4  */ s32 unk4D4;
    /* 0x4D8  */ u8 unk4D8;
    /* 0x4D9  */ u8 unk4D9;
    /* 0x4DA  */ u8 unk4DA[0x6];
    /* 0x4E0  */ MenuString unk4E0[4];
    /* 0x6E0  */ MenuString unk6E0[8];
    /* 0xAE0  */ MenuString unkAE0[6];
    /* 0xDE0  */ MenuString unkDE0[6];
    /* 0x10E0 */ u8 unk0x10E0[0xD00];
    /* 0x1DE0 */ MenuString* unk1DE0[4];
    /* 0x1DF0 */ MenuCharacter* benchedCharacters[MAX_BENCHED_PARTY_MEMBERS];
    /* 0x1E08 */ MenuCharacter* currentCharacters[MAX_PARTY_MEMBERS];
    /* 0x1E14 */ u8 unk1E14[MAX_GAME_CHARACTERS];
    /* 0x1E1F */ u8 unk1E1F;
    /* 0x1E20 */ MenuUnk7* unk1E20;
    /* 0x1E24 */ u8 unk1E24[0x8];
    /* 0x1E2C */ void* pShopEntries;
    /* 0x1E30 */ u_char shopItemIDs[MAX_SHOP_ITEMS];
    /* 0x1E60 */ u_char shopItemTypes[MAX_SHOP_ITEMS];
    /* 0x1E90 */ GearShopDefinition* pGearShopEntries; // Array of GearShopDefinition
    /* 0x1E94 */ u8 unk1E94;
    // Decompiling `MenuProcessControllerInput` worked better if this field is
    // volatile.  If you need to remove `volatile`, you can insert a temporary
    // variable instead as done here: https://decomp.me/scratch/xsigV
    /* 0x1E95 */ volatile u8 unk1E95;
    /* 0x1E96 */ u8 unk1E96[2];
} SystemMenu; // Size: 0x1E98

extern SystemMenu* g_Menu;
extern u8 g_MenuDebugEnabled;

#endif