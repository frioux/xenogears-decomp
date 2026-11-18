#include "common.h"

#include "main/game.h"
#include "system/menu.h"

extern u8 g_MenuOverlayIndex;
extern u_char func_801CB304(void);
extern u_char func_801CBD90(u_char arg0);
extern u_char func_801CC6D8(void);
extern u_char func_801CD2AC(void);
extern void MenuSetupPointerCursors(s32 arg0);
extern void MenuHidePointerCursors(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CD2AC);
s32 MenuUpdateSubMenuChoice(u_char arg0) {
    u_char result = 1;
    u8 choice;

    MenuSetupPointerCursors(0);
    g_Menu->pManager->shouldRenderCursors = 0;
    choice = g_Menu->subMenuChoice;
    switch (choice) {
        case 0:
            g_Menu->unk334 = 7;
            if (func_801CD2AC()) {
                result = 0;
            }
            break;
        case 1:
            g_Menu->unk334 = 6;
            if (func_801CC6D8()) {
                result = 0;
            }
            break;
        case 2:
            if (g_MenuOverlayIndex != g_Menu->subMenuChoice) {
                g_Menu->unk334 = 3;
                if (func_801CBD90(arg0)) {
                    result = 0;
                }
            } else {
                g_Menu->unk334 = 2;
                if (func_801CB304()) {
                    result = 0;
                }
            }
            break;
    }
    MenuHidePointerCursors();
    return result;
}
