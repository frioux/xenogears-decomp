#include "common.h"

#include "system/menu.h"


// Digits MenuDrawSlotResourceSprites draws of each of the two slot stats it
// formats: MenuParseNumberToDigits leaves the value in the tail of
// g_Menu->digits, any leading digit marked MENU_DIGIT_NONE.
#define MENU_SLOT_STAT_DIGIT_COUNT 3
// Where those digits start in g_Menu->digits.
#define MENU_SLOT_STAT_DIGIT_START (MENU_MAX_DIGITS - MENU_SLOT_STAT_DIGIT_COUNT)

s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuParseNumberToDigits(u32 number);

extern u32 g_MenuSlotDigitX;
extern u32 g_MenuSlotDigitY;

// pSlot and pCount re-derive pCounter's address: gcc emits retail's base-first
// "addu d,base,offset" only when that sum is an assignment's own result.
void MenuDrawSlotResourceSprites(u8 arg0, u8* pArg1) {
    u8* pPrims;
    u8* pCounter;
    u8* pSlot;
    u8* pCount;
    s32 primOffset;
    int i;

    MenuParseNumberToDigits((pArg1 + arg0)[0x16]);
    pCounter = (u8*)g_Menu->pPrimBuffer + arg0 * MENU_SLOT_PRIM_STRIDE;
    pCounter[0x1308] = 0;
    for (i = 0; i < MENU_SLOT_STAT_DIGIT_COUNT; i++) {
        if (g_Menu->digits[MENU_SLOT_STAT_DIGIT_START + i] == MENU_DIGIT_NONE) {
            continue;
        }
        primOffset = arg0 * MENU_SLOT_PRIM_STRIDE + 0xA98;
        pPrims = (u8*)(primOffset + (u32)g_Menu->pPrimBuffer);
        *((pSlot = (u8*)g_Menu->pPrimBuffer + arg0 * 0x87C) + 0x1308) += func_8002675C(
            g_Menu->resources, g_Menu->digits[MENU_SLOT_STAT_DIGIT_START + i],
            (POLY_FT4*)(pPrims + (*((pCount = (u8*)g_Menu->pPrimBuffer + arg0 * MENU_SLOT_PRIM_STRIDE) + 0x1308) * 0x50 + 0x320)),
            g_Menu->renderContext, arg0 * 0x50 + g_MenuSlotDigitX + i * MENU_DIGIT_SPACING,
            g_MenuSlotDigitY, 0x1000);
    }
    MenuParseNumberToDigits((pArg1 + arg0)[0x19]);
    pCounter = (u8*)g_Menu->pPrimBuffer + arg0 * MENU_SLOT_PRIM_STRIDE;
    pCounter[0x1309] = 0;
}
