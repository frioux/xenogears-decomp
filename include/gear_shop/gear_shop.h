#ifndef __GEAR_SHOP
#define __GEAR_SHOP

#define SHOP_DATA_INITIALIZE 0x0
#define SHOP_DATA_FREE 0x10

#define MAX_NUM_SUBMENU_OPTIONS 0x4

// Confirmation window choices
#define MENU_CHOICE_NO 0
#define MENU_CHOICE_YES 1

// Selection modes
#define MENU_AUTO_ADVANCE 0
#define MENU_MANUAL_CHOICE 0xFF

#define EFFECTSTATE_UPDATE_X 0x01
#define EFFECTSTATE_UPDATE_Y 0x02
#define EFFECTSTATE_UPDATE_Z 0x04

// Submenu choices for "Tune up"
#define MENU_CHOICE_ARMOR 0x0
#define MENU_CHOICE_FRAME 0x1
#define MENU_CHOICE_ENGINE 0x2
#define MENU_CHOICE_FUEL 0x3

// Submenu choices for "Sell"

#define MENU_CHOICE_GEAR_PARTS 0x3
#define MENU_CHOICE_GEAR_WEAPONS 0x4

#define MENU_CYCLE_TO_NEXT 0x0
#define MENU_CYCLE_TO_PREV 0x1

extern u8  g_GearShopDebugEnabled;

#define TEX_PAIR_CURSOR 0
#define TEX_PAIR_TEXT   1

extern int g_GearShopSubmenuTextureIds[]; // MENU_TEX_BALL_CURSOR_8
extern int D_801D69A4[];
// = {
//    Buy Submenu
//                             MENU_TEX_STRING_WEAPONS,
//    MENU_TEX_BALL_CURSOR_9,  MENU_TEX_STRING_PARTS,
//    MENU_TEX_BALL_CURSOR_10, MENU_TEX_STRING_FUEL,
//    MENU_TEX_BALL_CURSOR_11, MENU_TEX_STRING_ENGINE,
//    MENU_TEX_BALL_CURSOR_8,  MENU_TEX_STRING_PARTS,
//    MENU_TEX_BALL_CURSOR_9,  MENU_TEX_STRING_WEAPONS,
//    0xFFFF, 0xFFFF
//    0xFFFF, 0xFFFF,
//    ------------------------------------------------
//    Sell Submenu
//    MENU_TEX_BALL_CURSOR_8, MENU_TEX_STRING_PARTS,
//    MENU_TEX_BALL_CURSOR_9, MENU_TEX_STRING_WEAPONS,
//    0xFFFF, 0xFFFF,
//    0xFFFF, 0xFFFF,
//    ------------------------------------------------
//    Tune up Submenu
//    MENU_TEX_BALL_CURSOR_8,  MENU_TEX_STRING_ARMOR,
//    MENU_TEX_BALL_CURSOR_9,  MENU_TEX_STRING_FRAME,
//    MENU_TEX_BALL_CURSOR_10, MENU_TEX_STRING_ENGINE,
//    MENU_TEX_BALL_CURSOR_11, MENU_TEX_STRING_FUEL
//   }

extern u8  g_GearShopMainMenuStringIds[4];
extern u8  g_GearShopSubmenuStringIds[8];
extern int g_GearShopPointerCursorInitX[4];
extern int g_GearShopPointerCursorInitY[4];
extern u32 g_MenuStringIds;
extern int g_GearShopPointerCursorBoxSlot[];
extern int g_GearShopPointerCursorBoxX[];
extern int g_GearShopPointerCursorBoxY[];
extern u32 g_SelectionCursorX[9]; // = {0x49, 0x48, 0x43, 0x3C, 0x25, 0x24, 0x1F, 0x18, 0x10}
extern s32 g_SelectionCursorY[9]; // = {0xC9, 0xB5, 0xA2, 0x90, 0xC9, 0xB5, 0xA2, 0x90, 0x80}
extern int g_GearShopCharacterPortraitX[];
extern u16 g_GearShopCharacterBitMasks[];
extern int g_gearShopGearEquipFlags[];
extern u8  g_GearShopPurchaseExplanationGlyphs[8]; // indexed by (renderContext * 4)+i
extern u8  g_GearShopSellExplanationGlyphs[2]; // indexed by renderContext
extern int g_GearShopPurchaseExplanationX[8]; // indexed by (renderContext * 4)+i
extern s32 g_GearShopSellExplanationX[2]; // indexed by renderContext
extern int g_GearShopPurchaseExplanationY[8]; // indexed by (renderContext * 4)+i
extern s32 g_GearShopSellExplanationY[2]; // indexed by renderContext
extern s32 g_GearShopGoldBeforeX;
extern s32 g_GearShopGoldBeforeY;
extern int g_GearShopTotalPriceX;
extern int g_GearShopTotalPriceY;
extern s32 g_GearShopGoldAfterX;
extern s32 g_GearShopGoldAfterY;
extern u16 g_GearShopGearModelFileIds[2];
extern u8 g_GearShopGearAnimationIds[];
extern u16 g_GearShopGearModelGround[];
extern u16 g_GearShopGearModelScale[];

extern int g_GearShopInventoryCounts[5];
extern s16 g_GearShopGearCameraX[14];
extern s16 g_GearShopGearCameraY[204];
extern s16 g_GearShopGearRotationY[0x10];
extern s32 g_GearShopShoulderButtonX[2];

extern u16 g_GearShopOptionMarkerGlyphs[40];
extern u8 g_GearShopSubmenuRowIndex[16];
extern u16 g_GearShopOptionMarkerX[2];
extern u16 g_GearShopOptionMarkerY[12];
extern u16 g_GearShopOptionHighlightX[6];
extern u16 g_GearShopOptionHighlightY[6];
extern u16 g_GearShopSparkleX[6];
extern u16 g_GearShopSparkleY[6];
extern u16 g_GearShopGearStatLabelGlyphs[];
extern u16 g_GearShopGearStatLabelX[];
extern u16 g_GearShopGearStatLabelY[];
extern u16 g_GearShopGearHpX;
extern u16 g_GearShopGearHpY;
extern u16 g_GearShopGearMaxHpX;
extern u16 g_GearShopGearMaxHpY;
extern u16 g_GearShopGearFuelX;
extern u16 g_GearShopGearFuelY;
extern u16 g_GearShopGearMaxFuelX;
extern u16 g_GearShopGearMaxFuelY;
extern u16 g_GearShopGearWeightX;
extern u16 g_GearShopGearWeightY;
extern u8  g_GearShopGearPilotIds[9];

extern POLY_FT4 g_GearShopDebugDigitPrims[];

extern s8  D_801D70FD;
extern s32 g_GearShopDebugPrimCount;
extern s32 D_801D9050;
extern s32 D_801D9054;
extern s32 D_801D9058;
extern s32 D_801D905C;
extern s32 D_801D9060;
extern s32 D_801D9064;
extern s8  D_801D9083;
extern int g_gearShopAvailableCharacterCount;
extern int g_gearShopCurCharacterIndex;

extern u_char  g_gearShopCurrentGearId;
extern u8* g_GearShopGearNameBuffer;

extern s32 g_GearShopMainMenuTextureIds[];
// = {
//    MENU_TEX_BALL_CURSOR_1, MENU_TEX_STRING_BUY,
//    MENU_TEX_BALL_CURSOR_2, MENU_TEX_STRING_SELL,
//    MENU_TEX_BALL_CURSOR_3, MENU_TEX_STRING_EXIT,
//    MENU_TEX_BALL_CURSOR_4, MENU_TEX_STRING_TUNE_UP,
//    }
extern s8 g_GearShopPurchaseTotalStringIds[4];
extern s32 g_GearShopMainMenuStringOffsets[];
extern int g_GearShopMenuStringPadding[8]; // = { 0, 0x18, 0, 0, 0x18, 0xC, 0xC, 0x6 }

extern u16 g_gearShopCurrentItemQuantityHeld;

typedef struct {
    /* 0x00 */ int cameraX;
    /* 0x04 */ int cameraY;
    /* 0x08 */ int gearY;
} TransitionAnimationState; /* size: 0xC */

typedef struct {
    /* 0x00 */ TransitionAnimationState start;
    /* 0x0C */ TransitionAnimationState target;
    /* 0x18 */ TransitionAnimationState step;
    /* 0x24 */ TransitionAnimationState delta;
    /* 0x30 */ u8 isCameraPanningLeft;
    /* 0x31 */ u8 isCameraPanningDown;
    /* 0x32 */ u8 isGearMovingDown;
    /* 0x33 */ u8 stepFactor;
} MenuTransitionEffectState; /* size: 0x34 */

extern MenuTransitionEffectState g_gearShopTransitionState;

#endif
