#include "common.h"

#include "psyq/libapi.h"
#include "system/menu.h"
#include "psyq/libcd.h"

extern u8 g_MenuOverlayIndex; // Index of the menu overlay the menu was entered with
extern u16 D_801EA53C[]; // String IDs of the menu strings
extern u16 D_801EA542[]; // String IDs of the menu strings
extern u16 g_MenuItemRowStringIds[]; // String IDs of the menu strings
extern s32 g_MenuCardSlotTotals;
extern s32 D_801EA904;
extern s32 g_MenuStatBarValue;
extern s32 g_MenuStatBarSavedValue;
extern s32 g_MenuStatBarDelta;
extern s32 g_MenuStatBarValueWidth;
extern s32 g_MenuStatBarDeltaWidth;
extern u8 g_MenuStatBarDeltaColorMode;
extern u8 g_MenuStatBarDeltaSignTexId;
extern CdlCB g_MenuSavedCdSyncCallback;
extern CdlCB g_MenuSavedCdReadyCallback;
extern CdlCB g_MenuSavedCdReadCallback;
extern u8 g_MenuSoundModeStringIds[];
extern u8 g_MenuNameTextureX[];
extern u8 g_MenuSubMenuRowXOffsets[];
extern s32 g_MenuShoulderButtonIconTexIds[]; // String IDs of the menu scroll bar textures

void MenuSetupPointerCursors(s32 arg0);
void MenuFreeScrollHandle(void);
void MenuFreeWindow(s32 arg0);
void MenuLoadResourceSet(s32 arg0);
void MenuSetStatRowLabelQuads(s32 arg0, s32 arg1, u_char arg2);
void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
void MenuLoadStringsFromIds(s32 count, void* pStrings, void* pStringIds, void* pFlags);
void MenuSetRowQuadVertices(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u8 arg5, u8 arg6, u8 arg7);
void func_801E86C8(s32 arg0);
void MenuSetSubMenuChoicePrims(s32 arg0);
void MenuClearBytes(s32 arg0, void* pArg1);
s32 SoundGetVolumeMode(void);
void SoundSetVolumeMode(s32 arg0);
void MenuClearGlyphTexture(void);
void MenuFreeCellPrimBlocks(void);
void MenuFreePrimBuffer(void);
void Vsync(s32 mode);
void MenuCloseCardEvents(void);
void MenuRenderFrame(void);
u_char func_801C93A8(void);
void MenuCreateWindow2Strings(s32 arg0);
void MenuFreeWindow2Strings(void);
void MenuSetPrimShadingMode(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void MenuSetPolySemiTransGrey(P_TAG* pTag);

s32 MenuBeginCardOperation(void) {
    MenuUnk2* pMenu;
    s32 result = 1;

    MenuCreateWindow2Strings(0x20);
    g_Menu->pManager->window2StringsActive = 1;
    for (; g_Menu->transitionEffectState != 0; MenuRenderFrame()) {
    }
    g_Menu->menuUnk2->unk4FE7 = 1;
    MenuRenderFrame();
    MenuOpenCardEvents();
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    g_MenuSavedCdSyncCallback = CdSyncCallback(0);
    g_MenuSavedCdReadyCallback = CdReadyCallback(0);
    g_MenuSavedCdReadCallback = CdReadCallback(0);
    ExitCriticalSection();
    pMenu = g_Menu->menuUnk2;
    pMenu->previousCardStatus[1] = 0xFF;
    pMenu->previousCardStatus[0] = 0xFF;
    g_Menu->menuUnk2->cardEnumerated[0] = 0;
    g_Menu->menuUnk2->cardEnumerated[1] = 0;
    g_Menu->menuUnk2->cardPollMode = 2;
    g_Menu->cardCheckTimer = 0x3C;
    MenuRenderFrame();
    MenuRenderFrame();
    if (func_801C93A8() == 0) {
        result = 0;
        if (g_Menu->pManager->window2StringsActive != 0) {
            MenuFreeWindow2Strings();
            g_Menu->pManager->window2StringsActive = 0;
        }
    }
    return result;
}
