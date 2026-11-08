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

extern u8  D_801D697C;

#define TEX_PAIR_CURSOR 0
#define TEX_PAIR_TEXT   1

extern int D_801D69A0[]; // MENU_TEX_BALL_CURSOR_8
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

extern u8  D_801D6A20[4];
extern u8  D_801D6A24[8];
extern int D_801D6A60[4];
extern int D_801D6A70[4];
extern u32 D_801D6A80;
extern int D_801D6A84[];
extern int D_801D6AFC[];
extern int D_801D6B7C[];
extern u32 D_801D6BFC[9]; // = {0x49, 0x48, 0x43, 0x3C, 0x25, 0x24, 0x1F, 0x18, 0x10}
extern s32 D_801D6C20[9]; // = {0xC9, 0xB5, 0xA2, 0x90, 0xC9, 0xB5, 0xA2, 0x90, 0x80}
extern int D_801D6C44[];
extern u16 D_801D6C68[];
extern int g_gearShopGearEquipFlags[];
extern u8  D_801D6D08[8]; // indexed by (renderContext * 4)+i
extern u8  D_801D6D10[2]; // indexed by renderContext
extern int D_801D6D14[8]; // indexed by (renderContext * 4)+i
extern s32 D_801D6D34[2]; // indexed by renderContext
extern int D_801D6D3C[8]; // indexed by (renderContext * 4)+i
extern s32 D_801D6D5C[2]; // indexed by renderContext
extern s32 D_801D6D64;
extern s32 D_801D6D68;
extern int D_801D6D6C;
extern int D_801D6D70;
extern s32 D_801D6D74;
extern s32 D_801D6D78;
extern u16 D_801D6D7C[2];
extern u8 D_801D6DA0[];
extern u16 D_801D6DB4[];
extern u16 D_801D6DD8[];

extern int g_GearShopInventoryCounts[5];
extern s16 D_801D6DFC[14];
extern s16 D_801D6E18[204];
extern s16 D_801D6FB0[0x10];
extern s32 D_801D6FD0[2];

extern u16 D_801D6FE0[40];
extern u8 D_801D7030[16];
extern u16 D_801D7040[2];
extern u16 D_801D7044[12];
extern u16 D_801D705C[6];
extern u16 D_801D7068[6];
extern u16 D_801D7074[6];
extern u16 D_801D7080[6];
extern u16 D_801D708C[];
extern u16 D_801D70A8[];
extern u16 D_801D70C4[];
extern u16 D_801D70E0;
extern u16 D_801D70E2;
extern u16 D_801D70E4;
extern u16 D_801D70E6;
extern u16 D_801D70E8;
extern u16 D_801D70EA;
extern u16 D_801D70EC;
extern u16 D_801D70EE;
extern u16 D_801D70F0;
extern u16 D_801D70F2;
extern u8  D_801D70F4[9];

extern POLY_FT4 D_801D7108[];

extern s8  D_801D70FD;
extern s32 D_801D9048;
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
extern u8* D_801D9088;

extern s32 D_801D6980[];
// = {
//    MENU_TEX_BALL_CURSOR_1, MENU_TEX_STRING_BUY,
//    MENU_TEX_BALL_CURSOR_2, MENU_TEX_STRING_SELL,
//    MENU_TEX_BALL_CURSOR_3, MENU_TEX_STRING_EXIT,
//    MENU_TEX_BALL_CURSOR_4, MENU_TEX_STRING_TUNE_UP,
//    }
extern s8 D_801D6A2C[4];
extern s32 D_801D6A30[];
extern int D_801D6A40[8]; // = { 0, 0x18, 0, 0, 0x18, 0xC, 0xC, 0x6 }

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
