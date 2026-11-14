#include "common.h"

#include "psyq/libapi.h"
#include "system/menu.h"
#include "psyq/libcd.h"

extern u8 D_80059460; // Index of the menu overlay the menu was entered with
extern u16 D_801EA53C[]; // String IDs of the menu strings
extern u16 D_801EA542[]; // String IDs of the menu strings
extern u16 D_801EA548[]; // String IDs of the menu strings
extern s32 D_801EA900;
extern s32 D_801EA904;
extern s32 D_801EA6FC;
extern s32 D_801EA700;
extern s32 D_801EA704;
extern s32 D_801EA708;
extern s32 D_801EA70C;
extern u8 D_801EA710;
extern u8 D_801EA714;
extern CdlCB D_801EA718;
extern CdlCB D_801EA71C;
extern CdlCB D_801EA720;
extern u8 D_801EA574[];
extern u8 D_801EA578[];
extern u8 D_801E9F88[];
extern s32 D_801E977C[]; // String IDs of the menu scroll bar textures

void MenuSetupPointerCursors(s32 arg0);
void MenuFreeScrollHandle(void);
void func_801D4EA0(s32 arg0);
void func_801C72BC(s32 arg0);
void func_801D7F50(s32 arg0, s32 arg1, u_char arg2);
void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
void func_801E8018(s32 count, void* pStrings, void* pStringIds, void* pFlags);
void func_801E8070(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u8 arg5, u8 arg6, u8 arg7);
void func_801E86C8(s32 arg0);
void func_801E8B4C(s32 arg0);
void func_801E8044(s32 arg0, void* pArg1);
s32 func_80038824(void);
void func_800386C4(s32 arg0);
void func_801E64E0(void);
void func_801E5B3C(void);
void func_801E649C(void);
void Vsync(s32 mode);
void MenuCloseCardEvents(void);
void MenuRenderFrame(void);
u_char func_801C93A8(void);
void func_801D2F4C(s32 arg0);
void MenuFreeWindow2Strings(void);
void func_801E8EAC(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void func_801E91C4(P_TAG* pTag);

s32 func_801D9C84(void) {
    MenuUnk2* pMenu;
    s32 result = 1;

    func_801D2F4C(0x20);
    g_Menu->pManager->_pad33 = 1;
    for (; g_Menu->transitionEffectState != 0; MenuRenderFrame()) {
    }
    g_Menu->menuUnk2->unk4FE7[0] = 1;
    MenuRenderFrame();
    func_801D9B08();
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    D_801EA718 = CdSyncCallback(0);
    D_801EA71C = CdReadyCallback(0);
    D_801EA720 = CdReadCallback(0);
    ExitCriticalSection();
    pMenu = g_Menu->menuUnk2;
    pMenu->unk4FE7[2] = 0xFF;
    pMenu->unk4FE7[1] = 0xFF;
    g_Menu->menuUnk2->unk4F84[4] = 0;
    g_Menu->menuUnk2->unk4F84[5] = 0;
    g_Menu->menuUnk2->unk4FE6 = 2;
    g_Menu->unk326 = 0x3C;
    MenuRenderFrame();
    MenuRenderFrame();
    if (func_801C93A8() == 0) {
        result = 0;
        if (g_Menu->pManager->_pad33 != 0) {
            MenuFreeWindow2Strings();
            g_Menu->pManager->_pad33 = 0;
        }
    }
    return result;
}
