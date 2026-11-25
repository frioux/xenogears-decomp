#include "common.h"

#include "psyq/libapi.h"
#include "system/menu.h"
#include "system/graphics.h"
#include "psyq/libcd.h"

#define MENU_STAT_BAR_WIDTH_MAX 6400 // width a stat value is scaled into, see the stat bar quads
#define MENU_STAT_LABEL_ROW_COUNT 7 // stat rows in one stat group, and the ids each group of labels owns
#define MENU_STAT_LABEL_SHADE 0x40 // grey the labels of every other stat row are shaded with
#define MENU_STAT_BAR_QUAD_HEIGHT 6 // height of one stat bar quad
// Digits MenuParseNumberToDigits leaves in the tail of g_Menu->digits that the
// stat bar value and the delta beside it are drawn from.
#define MENU_STAT_VALUE_DIGIT_COUNT 4
#define MENU_STAT_DELTA_DIGIT_COUNT 3
#define MENU_STAT_VALUE_DIGIT_START (MENU_MAX_DIGITS - MENU_STAT_VALUE_DIGIT_COUNT)
#define MENU_STAT_DELTA_DIGIT_START (MENU_MAX_DIGITS - MENU_STAT_DELTA_DIGIT_COUNT)

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
extern u16 D_801E9D78; // X offset of the stat bar value quads
extern u16 D_801E9D7C; // Y offset of the stat bar value quad rows
extern s32 D_801E9D80; // X offset of the stat bar value digits
extern s32 D_801E9D84; // Y offset of the stat bar value digits
extern CdlCB g_MenuSavedCdSyncCallback;
extern CdlCB g_MenuSavedCdReadyCallback;
extern CdlCB g_MenuSavedCdReadCallback;
extern u8 D_801EA574[];
extern u8 g_MenuGlyphSjisTable[]; // Glyph id of each character's name strip
extern u16 D_801E9D88[][2]; // Y offset of each equipment row of the equipment panel
extern s32 g_MenuNameTextureX[];
extern u8 D_801E9F88[];
extern s32 g_MenuShoulderButtonIconTexIds[]; // String IDs of the menu scroll bar textures
extern s32 g_MenuStatRowLabelOffsetX[]; // X offset of each stat row of the stat table
extern s32 g_MenuStatRowLabelOffsetY[]; // Y offset of each stat row of the stat table
extern s32 g_MenuStatRowLabelTexIds[]; // Texture id of each stat row's label, 7 rows per stat group
extern u8* GetWeaponName(s32 itemId);
extern u8* GetGearWeaponName(s32 itemId);
extern u8* GetAccessoryName(s32 itemId);
extern u8* GetGearAccessoryName(s32 itemId);
extern s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 length, s32 mode);

#define MENU_STAT_VALUE_COUNT 9 // stat values MenuCopyEquipmentToPanel copies out of the dressing room
#define MENU_EQUIPMENT_ROW_COUNT 5 // rows of the character panel; the two gear panels draw 4

// The buffer g_Menu->pEquipmentPanel points at (HeapAlloc(0x2AC) in MenuLoadEquipmentPanel):
// one MenuString per equipment row, then the ids those rows draw the name of.
typedef struct {
    /* 0x000 */ MenuString strings[MENU_EQUIPMENT_ROW_COUNT];
    /* 0x280 */ u16 statValues[MENU_STAT_VALUE_COUNT];
    /* 0x292 */ u8 unk292[0x2];
    /* 0x294 */ u8 builtRows[MENU_EQUIPMENT_ROW_COUNT]; // one flag per row MenuBuildEquipmentRows drew
    /* 0x299 */ u8 renderContext; // renderContext the string prims were built for
    /* 0x29A */ u8 unk29A[0x2];
    /* 0x29C */ u8 weaponIds[MENU_EQUIPMENT_ROW_COUNT];
    /* 0x2A1 */ u8 extraWeaponIds[MENU_EQUIPMENT_ROW_COUNT];
    /* 0x2A6 */ u8 accessoryIds[MENU_EQUIPMENT_ROW_COUNT];
    /* 0x2AB */ u8 unk2AB;
} MenuEquipmentPanel; // Size: 0x2AC

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
void ResourceHelperGetTexCoords(MenuResourceDirectory* pDir, u32 index, s32* pDirentCount, s32* pTexPageId, s32* pClutX, s32* pClutY, s32* pTexPageX, s32* pTexPageY);

/* The two address shapes the gear window block needs, both measured:
   the `tpage |= 0x20` read-modify-write only takes retail's base-first `addu`
   with the double-negated offset (lever c-a-double-negated-offset-flips), and the
   loop's four vertex reads only with a base pointer kept apart from the scaled
   offset (lever b-pointer-local-holding-base). Spelling the block as a typed
   POLY_FT4 struct measures 64.37% (592 bytes) - the pointer becomes a hoisted
   loop-invariant - so the byte offsets stay casts here. */
void MenuSetGearWindowQuads(s32 arg0, u_char arg1, u_char arg2) {
    u_char selected = (arg2 == 0);
    u8* pEntry;
    u8* pBase;
    s32 off;
    s32 i;

    ((u8*)g_Menu->pGearWindowPrims)[0x2AED] = 0;
    if (arg1 != 0) {
        for (i = 0; i < 2; i++) {
            func_8002675C(g_Menu->resources, g_MenuShoulderButtonIconTexIds[i],
                          (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + 0x1E00 + i * 0x50),
                          g_Menu->renderContext, 0x78 + i * 0x20, 0x5A, 0x1000);
            off = (g_Menu->renderContext + i * 2) * 0x28;
            pBase = (u8*)g_Menu->pGearWindowPrims;
            pEntry = pBase + off;
            MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x2AA0 + i * 0x20),
                          *(u16*)(pEntry + 0x1E08), *(u16*)(pEntry + 0x1E0A),
                          *(u16*)(pEntry + 0x1E10) - *(u16*)(pEntry + 0x1E08),
                          *(u16*)(pEntry + 0x1E22) - *(u16*)(pEntry + 0x1E0A));
        }
        MenuSetPolySemiTransGrey((P_TAG*)((u8*)g_Menu->pGearWindowPrims + 0x1E00 + (g_Menu->renderContext + selected * 2) * 0x28));
        *(u16*)((u8*)g_Menu->pGearWindowPrims - -((g_Menu->renderContext + selected * 2) * sizeof(POLY_FT4)) + 0x1E16) |= 0x20;
        *(u8*)((u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E04) = 0x20;
        *(u8*)((u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E05) = 0x20;
        *(u8*)((u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E06) = 0x20;
        ((u8*)g_Menu->pGearWindowPrims)[0x2AEE] = *(u8*)&g_Menu->renderContext;
        ((u8*)g_Menu->pGearWindowPrims)[0x2AED] = 2;
    }
}

void MenuSetStatRowLabelQuads(s32 x, s32 y, u_char statGroup) {
    s32 texCoords[6]; // Filled and ignored
    s32 primOffset;
    s32 start;
    s32 i;
    s32 j;

    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_STRING_DEFENSE, &texCoords[0], &texCoords[1], &texCoords[2], &texCoords[3], &texCoords[4], &texCoords[5]);
    ((u8*)g_Menu->unk35C)[0x32F3] = 0;
    for (i = 0; i < MENU_STAT_LABEL_ROW_COUNT - statGroup; i++) {
        start = ((u8*)g_Menu->unk35C)[0x32F3];
        ((u8*)g_Menu->unk35C)[0x32F3] += func_8002675C(g_Menu->resources, g_MenuStatRowLabelTexIds[statGroup * MENU_STAT_LABEL_ROW_COUNT + i], (POLY_FT4*)((u8*)g_Menu->unk35C + start * 0x50), g_Menu->renderContext, x + g_MenuStatRowLabelOffsetX[i], y + g_MenuStatRowLabelOffsetY[i], 0x1000);
        if (i & 1) {
            for (j = start; j < ((u8*)g_Menu->unk35C)[0x32F3]; j++) {
                SetShadeTex((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + j * 2) * 0x28), 0);
                ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + j * 2) * 0x28))->r0 = MENU_STAT_LABEL_SHADE;
                ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + j * 2) * 0x28))->g0 = MENU_STAT_LABEL_SHADE;
                ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + j * 2) * 0x28))->b0 = MENU_STAT_LABEL_SHADE;
            }
        }
    }
    for (i = 0; i < ((u8*)g_Menu->unk35C)[0x32F3]; i++) {
        primOffset = (g_Menu->renderContext + i * 2) * 0x28;
        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->unk35C + 0x2420 + i * 0x20),
                      ((POLY_FT4*)((u8*)g_Menu->unk35C + primOffset))->x0,
                      ((POLY_FT4*)((u8*)g_Menu->unk35C + primOffset))->y0,
                      ((POLY_FT4*)((u8*)g_Menu->unk35C + primOffset))->x1 - ((volatile POLY_FT4*)(primOffset + (u32)g_Menu->unk35C))->x0,
                      ((POLY_FT4*)((u8*)g_Menu->unk35C + primOffset))->y3 - ((volatile POLY_FT4*)(primOffset + (u32)g_Menu->unk35C))->y0);
    }
}


void MenuSetStatBarQuadColors(POLY_G4* pPolys, u_char mode) {
    POLY_G4* pPoly;
    u8 rgb[3];
    s32 i;

    switch (mode) {
    case 0:
        rgb[0] = 0xFF;
        rgb[1] = 0x80;
        rgb[2] = 0x80;
        break;
    case 1:
        rgb[0] = 0x80;
        rgb[1] = 0xFF;
        rgb[2] = 0x80;
        break;
    case 2:
        rgb[0] = 0xFF;
        rgb[1] = 0;
        rgb[2] = 0;
        break;
    case 3:
        rgb[0] = 0;
        rgb[1] = 0;
        rgb[2] = 0xFF;
        break;
    }
    for (i = 0; i < 2; i++) {
        pPoly = &pPolys[i];
        SetPolyG4(pPoly);
        pPoly->r0 = rgb[0];
        pPoly->g0 = rgb[1];
        pPoly->b0 = rgb[2];
        pPoly->r1 = rgb[0];
        pPoly->g1 = rgb[1];
        pPoly->b1 = rgb[2];
        pPoly->r2 = 0;
        pPoly->g2 = 0;
        pPoly->b2 = 0;
        pPoly->r3 = 0;
        pPoly->g3 = 0;
        pPoly->b3 = 0;
    }
}
void MenuSetStatBarTileColors(POLY_FT4* pPolys, u_char mode, u_char count, u_char startIndex) {
    POLY_FT4* pPoly;
    u8 rgb[3];
    s32 i;

    rgb[1] = 0x40;
    switch (mode) {
    case 0:
        rgb[0] = 0x80;
        rgb[2] = 0x40;
        break;
    case 1:
        rgb[0] = 0x40;
        rgb[2] = 0x80;
        break;
    case 2:
        rgb[0] = 0x40;
        rgb[2] = 0x40;
        break;
    }
    for (i = 0; i < count; i++) {
        pPoly = &pPolys[startIndex + i * 2];
        SetShadeTex(pPoly, 0);
        pPoly->r0 = rgb[0];
        pPoly->g0 = rgb[1];
        pPoly->b0 = rgb[2];
    }
}
void MenuSetStatBarValues(u_short arg0, u_short arg1, s32 arg2) {
    s32 diff;

    g_MenuStatBarValue = arg0;
    g_MenuStatBarSavedValue = arg1;
    diff = arg1 - arg0;
    g_MenuStatBarDelta = diff;
    g_MenuStatBarValueWidth = MENU_STAT_BAR_WIDTH_MAX * (100 * arg0 / arg2) / 10000;
    if (diff >= 0) {
        g_MenuStatBarDeltaColorMode = 2;
        g_MenuStatBarDeltaSignTexId = MENU_TEX_PLUS;
    } else {
        g_MenuStatBarDeltaColorMode = 3;
        g_MenuStatBarDeltaSignTexId = MENU_TEX_MINUS;
        g_MenuStatBarDelta = arg0 - arg1;
    }
    g_MenuStatBarDeltaWidth = MENU_STAT_BAR_WIDTH_MAX * (100 * g_MenuStatBarDelta / arg2) / 10000;
}

s32 MenuMaxStatValue(s32 unusedArg, volatile u16* p0, volatile u16* p1) {
    SVECTOR unusedFrame[3]; // original allocates a 0x18 frame it never writes
    u16 max = 0;
    s32 i;

    for (i = 0; i < MENU_STAT_LABEL_ROW_COUNT; p0++) {
        i++;
        if (max < *p0) {
            max = *p0;
        }
    }
    for (i = 0; i < MENU_STAT_LABEL_ROW_COUNT; p1++) {
        i++;
        if (max < *p1) {
            max = *p1;
        }
    }

    return max;
}

/* PARKED 88.01% under XenoMenu260 (gcc 2.6.0): 1964 bytes / 491 insns against retail 1952 / 488, frame 0xA0 against retail's 0xA8.
 Named residual (lever j-slot-spacing-tells-decl-from-spill): retail's frame has NO
 expand-time locals - its nine local slots sit 8 bytes apart (0x20 max, 0x28 arg3,
 0x30 arg4, 0x38 pStat, 0x40 pSaved, 0x60 rowOfs, 0x68 deltaVertOfs, 0x70 colorOfs,
 0x78 valueVertOfs; 12 slots x 8 = its 0x60 var area, with 0x48/0x50/0x58 allocated and
 never touched), so every one of them is a reload SPILL and pStat/pSaved are plain
 pointers the allocator ran out of registers for - not volatile decl locals. This body
 puts pStat/pSaved/max in 4-byte-apart decl slots and only the four accumulators 8 apart,
 so the frame is 8 short and every sp operand and slot number differs. Two smaller
 residual pieces: arg1 lands in s4 here and s5 in retail (each later callee-saved
 assignment rotates by one), and the two quad loops read their four u16 prim fields in a
 different order (retail a1=0xEB8, a2=0xEBA, a3=0xEC0, v0=0xEB8, v1=0xEBA, v0=0xED2).
 Third: retail reads D_801E9D78 and g_MenuStatBarValueWidth with lhu at the value quad
 (0x3100) but with lw at the delta quad (0x31E0); this body gets lhu at both.
 Next lever for the frame half: j-declaration-order-sets-the-reload-slot-order - sort the
 declarations into retail's slot order (max, arg3, arg4, pStat, pSaved, rowOfs,
 deltaVertOfs, colorOfs, valueVertOfs) once the pointers are spills, then re-measure; the
 two attempts so far are the two ends (volatile = decl slots, plain = registers).
 Tried this round: dropping volatile from pStat/pSaved reaches retail's EXACT 0xA8 frame
 and 304 differing lines instead of 329, but gcc then keeps the two pointers in s0/s2
 (retail spills them) and it measures 85.69% / 1968 bytes / 492 insns, so the slower
 88.01 body is parked. Presets screened: 2.7.2 87.05, -O1 53.95, -O3 30.90, cdk and -G0
 do not compile.
 What is right: the whole shape - the arg3 if/else picking pEquipmentPanel+0x280 against
 pDressingRoom->statValues, MenuMaxStatValue, the 7-arg4 row loop, one
 MenuSetStatBarQuadColors + MenuSetQuadVertices + MenuParseNumberToDigits + 4-digit loop
 per row, the digit-count quad loop reading the 0xEB8 prims into the 0x2A00 SVECTORs, the
 every-other-row MenuSetStatBarTileColors, the arg3-gated delta block (0x2228 quad, 0x31E0
 quad, 0x1770 prims, 3 delta digits, 0x2D80 SVECTORs) and the seven accumulators with
 their 0x140/0x80/0x48/0x80/8 steps.
 What moved it: 85.02 first body; 85.69 after spelling every per-row byte access as
 ((u8*)g_Menu->unk35C + i)[0x32C0] - the (p + i)[K] form is the only one that emits
 retail's base-first addu, probed with cc1_probe.sh, where p[K + i] emits index-first;
 88.01 after declaring pStat/pSaved u16* volatile.
 Also tried: an if around the loop instead of the early return (two blez, retail has
 one), a do-while (83.76 - it moves the accumulator updates out of retail's order),
 quadX split from digitX (kept, 85.09 -> 85.32), g_MenuStatRowLabelOffsetY[7..10] in
 place of D_801E9D78..84 (reverted - it renames retail's R_MIPS_LO16 reloc).
void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4) {
    u16* volatile pStat;
    u16* volatile pSaved;
    s32 max;
    s32 rowOfs;
    s32 valuePrimOfs;
    s32 deltaPrimOfs;
    s32 colorOfs;
    s32 valueVertOfs;
    s32 deltaVertOfs;
    s32 yOfs;
    s32 digitX;
    s32 quadX;
    s32 primOfs;
    s32 i;
    s32 j;

    if (arg3 == 0) {
        pSaved = g_Menu->pDressingRoom->statValues;
        pStat = pSaved;
    } else {
        pStat = (u16*)((u8*)g_Menu->pEquipmentPanel + 0x280);
        pSaved = g_Menu->pDressingRoom->statValues;
    }
    max = MenuMaxStatValue(0, pStat, pSaved);
    rowOfs = 0;
    valuePrimOfs = 0xEB0;
    deltaPrimOfs = 0x1770;
    colorOfs = 0;
    valueVertOfs = 0x2A00;
    deltaVertOfs = 0x2D80;
    if (MENU_STAT_LABEL_ROW_COUNT - arg4 <= 0) {
        return;
    }
    yOfs = 0;
    for (i = 0; i < MENU_STAT_LABEL_ROW_COUNT - arg4; i++) {
        ((u8*)g_Menu->unk35C + i)[0x32C0] = 0;
        ((u8*)g_Menu->unk35C + i)[0x32CE] = 0;
        MenuSetStatBarValues(pStat[i], pSaved[i], max);
        MenuSetStatBarQuadColors((POLY_G4*)(colorOfs + 0x2030 + (u32)g_Menu->unk35C), 0);
        MenuSetQuadVertices((SVECTOR*)(i * 0x20 + 0x3100 + (u32)g_Menu->unk35C),
                            D_801E9D78 + arg1,
                            arg2 + D_801E9D7C + yOfs,
                            g_MenuStatBarValueWidth, MENU_STAT_BAR_QUAD_HEIGHT);
        ((u8*)g_Menu->unk35C + i)[0x32DC] = g_Menu->renderContext;
        MenuParseNumberToDigits(g_MenuStatBarValue);
        for (j = 0; j < MENU_STAT_VALUE_DIGIT_COUNT; j++) {
            if (g_Menu->digits[MENU_STAT_VALUE_DIGIT_START + j] != MENU_DIGIT_NONE) {
                ((u8*)g_Menu->unk35C + i)[0x32C0] +=
                    func_8002675C(g_Menu->resources, g_Menu->digits[MENU_STAT_VALUE_DIGIT_START + j],
                                  (POLY_FT4*)(valuePrimOfs + (u32)g_Menu->unk35C +
                                              ((u8*)g_Menu->unk35C + i)[0x32C0] * 0x50),
                                  g_Menu->renderContext,
                                  arg1 + D_801E9D80 + j * MENU_DIGIT_SPACING,
                                  arg2 + D_801E9D84 + yOfs, 0x1000);
            }
        }
        for (j = 0; j < ((u8*)g_Menu->unk35C + i)[0x32C0]; j++) {
            primOfs = rowOfs + (g_Menu->renderContext + j * 2) * sizeof(POLY_FT4);
            MenuSetQuadVertices((SVECTOR*)(valueVertOfs + (u32)g_Menu->unk35C + j * 0x20),
                                *(u16*)((u8*)g_Menu->unk35C + primOfs + 0xEB8),
                                *(u16*)((u8*)g_Menu->unk35C + primOfs + 0xEBA),
                                *(u16*)((u8*)g_Menu->unk35C + primOfs + 0xEC0) -
                                    *(volatile u16*)((u8*)g_Menu->unk35C + primOfs + 0xEB8),
                                *(u16*)((u8*)g_Menu->unk35C + primOfs + 0xED2) -
                                    *(volatile u16*)((u8*)g_Menu->unk35C + primOfs + 0xEBA));
        }
        if (i & 1) {
            MenuSetStatBarTileColors((POLY_FT4*)(valuePrimOfs + (u32)g_Menu->unk35C), 2,
                                     ((u8*)g_Menu->unk35C + i)[0x32C0], g_Menu->renderContext);
        }
        ((u8*)g_Menu->unk35C + i)[0x32C7] = g_Menu->renderContext;
        if (arg3 != 0) {
            MenuSetStatBarQuadColors((POLY_G4*)(colorOfs + 0x2228 + (u32)g_Menu->unk35C), g_MenuStatBarDeltaColorMode);
            if (g_MenuStatBarDeltaColorMode == 2) {
                quadX = D_801E9D78 + arg1 + g_MenuStatBarValueWidth;
            } else {
                quadX = D_801E9D78 + arg1 + g_MenuStatBarValueWidth - g_MenuStatBarDeltaWidth;
            }
            MenuSetQuadVertices((SVECTOR*)(i * 0x20 + 0x31E0 + (u32)g_Menu->unk35C), quadX,
                                arg2 + D_801E9D7C + yOfs,
                                g_MenuStatBarDeltaWidth, MENU_STAT_BAR_QUAD_HEIGHT);
            ((u8*)g_Menu->unk35C + i)[0x32E3] = g_Menu->renderContext;
            if (arg3 != 0 && g_MenuStatBarDelta != 0) {
                ((u8*)g_Menu->unk35C + i)[0x32CE] +=
                    func_8002675C(g_Menu->resources, g_MenuStatBarDeltaSignTexId,
                                  (POLY_FT4*)(deltaPrimOfs + (u32)g_Menu->unk35C),
                                  g_Menu->renderContext, arg1 + D_801E9D80 + 0x20,
                                  arg2 + D_801E9D84 + yOfs, 0x1000);
                MenuParseNumberToDigits(g_MenuStatBarDelta);
                digitX = 0x28;
                for (j = 0; j < MENU_STAT_DELTA_DIGIT_COUNT; j++) {
                    if (g_Menu->digits[MENU_STAT_DELTA_DIGIT_START + j] == MENU_DIGIT_NONE) {
                        continue;
                    }
                    ((u8*)g_Menu->unk35C + i)[0x32CE] +=
                        func_8002675C(g_Menu->resources, g_Menu->digits[MENU_STAT_DELTA_DIGIT_START + j],
                                      (POLY_FT4*)(deltaPrimOfs + (u32)g_Menu->unk35C +
                                                  ((u8*)g_Menu->unk35C + i)[0x32CE] * 0x50),
                                      g_Menu->renderContext, arg1 + D_801E9D80 + digitX,
                                      arg2 + D_801E9D84 + yOfs, 0x1000);
                    digitX += MENU_DIGIT_SPACING;
                }
                for (j = 0; j < ((u8*)g_Menu->unk35C + i)[0x32CE]; j++) {
                    primOfs = rowOfs + (g_Menu->renderContext + j * 2) * sizeof(POLY_FT4);
                    MenuSetQuadVertices((SVECTOR*)(deltaVertOfs + (u32)g_Menu->unk35C + j * 0x20),
                                        *(u16*)((u8*)g_Menu->unk35C + primOfs + 0x1778),
                                        *(u16*)((u8*)g_Menu->unk35C + primOfs + 0x177A),
                                        *(u16*)((u8*)g_Menu->unk35C + primOfs + 0x1780) -
                                            *(volatile u16*)((u8*)g_Menu->unk35C + primOfs + 0x1778),
                                        *(u16*)((u8*)g_Menu->unk35C + primOfs + 0x1792) -
                                            *(volatile u16*)((u8*)g_Menu->unk35C + primOfs + 0x177A));
                }
                MenuSetStatBarTileColors((POLY_FT4*)(deltaPrimOfs + (u32)g_Menu->unk35C),
                                         g_MenuStatBarDeltaColorMode - 2, ((u8*)g_Menu->unk35C + i)[0x32CE],
                                         g_Menu->renderContext);
                ((u8*)g_Menu->unk35C + i)[0x32D5] = g_Menu->renderContext;
                ((u8*)g_Menu->unk35C)[0x32F2] = 1;
            }
        }
        ((u8*)g_Menu->unk35C + i)[0x32EA] = 1;
        rowOfs += 0x140;
        valuePrimOfs += 0x140;
        deltaPrimOfs += 0x140;
        colorOfs += 0x48;
        valueVertOfs += 0x80;
        deltaVertOfs += 0x80;
        yOfs += MENU_DIGIT_SPACING;
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D8644);

void MenuDrawStatTable(s32 arg0, u_char arg1, s32 arg2, s32 arg3) {
    s32 width = 0x98;
    s32 height = 0x26;
    u_char value;

    if (arg1 != 0) {
        width = 0x80;
        height = 0x90;
    }
    value = arg3;
    MenuSetStatRowLabelQuads(width, height, value);
    func_801D8644(arg0, width, height, arg2, value);
    g_Menu->pManager->shouldRenderUnk35C = 1;
    g_Menu->unk35C->renderContext = *(u_char*)&g_Menu->renderContext;
}

// Draws the name rows of the equipment panel: one row per equipped weapon or
// accessory, its name rendered into the row's MenuString and the row's quad set
// from that width. arg0 is the party slot whose ids are shown, arg1 picks the row
// group (0 the character's own equipment, 1 and 2 the two gear panels), arg2 reads
// the ids the panel itself holds instead of the live records, and arg3 takes the
// names from the character's gear rather than the character.
// PARKED 90.50% under XenoMenu260 (gcc 2.6.0): 2140 bytes / 535 insns against retail 2144 / 536.
// ONE cause for the whole 318-line residual: the parameter copy `move s4,a1` is not in the
// prologue. Retail moves arg1 to s4 as its second instruction, which frees a1, and retail then
// keeps g_Menu in a1 and the row constants in t0 (lui a1,%hi(g_Menu) ... li t0,5 / li t0,0xD0).
// Here gcc keeps arg1 in a1 until just before the loop preheader (the move lands after
// `lbu v1,0x30(v0)`), picks t0 for g_Menu and t1 for the constants, and every later callee-saved
// assignment rotates by one: retail s4 = the pExtraId cursor and s7 = arg1&0xff, here s7 = the
// cursor and s6 = arg1&0xff. Fix that one copy and the rest should collapse; nothing else in the
// body differs structurally.
// Moved up this round:
//  - rowX is u16, not s32 (measured 89.92 -> 90.50, 2152 -> 2140 bytes). Retail reloads it for
//    MenuSetQuadVertices with `lhu a1,0x28(sp)`; an s32 local gives `lw` + `andi 0xffff` (+1 insn).
//    Retail still stores it with `sw` where a u16 local stores with `sh`, so one instruction is
//    missing overall (535 vs 536) - the slot is 8 bytes wide (next local at 0x30, not 0x2c), which
//    no single u16 explains.
//  - the i == 0 name render is `if (arg1 != 2) {weapon ids} else {extra ids}` (89.81 -> 89.92).
//    Retail is `beq s7,2 -> out-of-line extra arm` with the weapon arm as the fall-through, which
//    is what a `!=` test gives; the old `if (arg1 == 2) ... else` put the extra arm first.
// Measured and rejected this round:
//  - the loop-body dispatch as a C switch. Retail is `beqz s0 -> i==0 arm` (out of line) then
//    `bltz s0` / `slti s0,5` / `beqz` (the 0..4 range test) then `j <i==4 test>; li v0,4`.
//    `switch (i) { case 0: ...; case 4: if (arg3==0) break; default: ... }` compiles to
//    `beq s0,0; beq s0,4; j default`, no range test, 63.41% - reverted. The range test says the
//    original really is a switch over 0..4 (gcc range_test for a signed index emits bltz + slti),
//    so the case labels are not {0,4}: cases 1..3 must be labelled too and share the i==4 test.
//  - u16 rowX still leaves the clut store recomputing the poly address (see below): retail reuses
//    the address it computed for tpage, putting `addu v1,v1,a0` in the beqz delay slot; this body
//    emits a second full rc*0x28 chain (+8 insns there, and it is why the body is 1 insn short
//    overall rather than over).
//  - the poly field stores still differ in addu operand order: retail `addu a0,s1,a0` (i*0x80 +
//    panel) then `addu v0,v0,a0` (rc*0x28 + that); here `addu v0,v0,s1` then `addu a0,a0,v0`.
// Kept from the earlier park: the local slot picture (0x20 rowCount, 0x28 rowX, 0x30
// rowTableOfs, 0x38/0x40 the id pointers, 0x48 loadImage, 0x50 the masked arg0), the 5-vs-4 row
// count, the 0xD0/0x28 row X and 9/5 D_801E9D88 offset picked by arg1, the four-way i == 0
// render, the i == 4 && arg3 == 0 skip, the arg1 < 2 accessory rows with the i == 1
// weaponIds[3] special case, the extraWeaponIds[i] rows for arg1 == 2, the i & 1 / arg3
// loadImage chain (retail really does read the slot uninitialised), the RECT + LoadImage +
// DrawSync block, MenuSetStringPrimitives per row, the i == 4 tpage/CLUT/uv block and the
// MenuSetQuadVertices row.
// Ghidra/asm facts worth keeping: pEquipmentPanel is a 0x2AC buffer of 5 MenuString plus its own
// id copies; g_MenuNameTextureX is s32[] (lw, stride 4); g_MenuGlyphSjisTable is read as u8 with
// stride 4; D_801E9D88 is u16[][2] with stride 4; rect.x = 0x140 + ((i / 2) & 1) * 0x20 and
// rect.y = 0x27 + (i / 4) * FONT_LETTER_HEIGHT; the polys of one MenuString start at 0x200 and
// are 0x28 bytes each (u0 0xC, u1 0x14, u2 0x1C, u3 0x24, clut 0xE, tpage 0x16).
/*
void func_801D8EA4(s32 arg0, u_char arg1, u_char arg2, u_char arg3) {
    s32 rowCount;
    u16 rowX;
    s32 rowTableOfs;
    RECT rect;
    u8* pWeaponIds;
    u8* pExtraIds;
    u8* pAccessoryIds;
    u8* pExtraId;
    u8* pHeap;
    u16 palette;
    u_char loadImage;
    s32 i;

    rowCount = MENU_EQUIPMENT_ROW_COUNT;
    rowX = 0xD0;
    rowTableOfs = 0;
    pWeaponIds = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0 & 0xFF]].weaponId;
    pExtraIds = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0 & 0xFF]].extraWeaponIds;
    pAccessoryIds = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0 & 0xFF]].accessorySlots;
    if (arg1 != 0) {
        rowCount = MENU_EQUIPMENT_ROW_COUNT - 1;
        rowX = 0x28;
        rowTableOfs = 9;
        if (arg1 == 1) {
            rowTableOfs = 5;
        }
        ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->builtRows[MENU_EQUIPMENT_ROW_COUNT - 1] = 0;
    }
    if (arg3 != 0) {
        pWeaponIds = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0 & 0xFF]].gearId].weaponId;
        pExtraIds = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0 & 0xFF]].gearId].extraWeaponIds;
        pAccessoryIds = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0 & 0xFF]].gearId].accessorySlots;
    }
    if (arg2 != 0) {
        pWeaponIds = ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->weaponIds;
        pExtraIds = ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->extraWeaponIds;
        pAccessoryIds = ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->accessoryIds;
    }
    pHeap = HeapAlloc(0x3F6, 0);
    pExtraId = pExtraIds;
    for (i = 0; i < rowCount; i++) {
        if (i == 0) {
            if (arg1 != 2) {
                if (arg3 == 0) {
                    ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[0].width = SystemRenderStringEntry(GetWeaponName(pWeaponIds[0]), pHeap, 0x24, 0);
                } else {
                    ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[0].width = SystemRenderStringEntry(GetGearWeaponName(pWeaponIds[0]), pHeap, 0x24, 0);
                }
            } else if (arg3 == 0) {
                ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[0].width = SystemRenderStringEntry(GetWeaponName(pExtraIds[0]), pHeap, 0x24, 0);
            } else {
                ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[0].width = SystemRenderStringEntry(GetGearWeaponName(pExtraIds[0]), pHeap, 0x24, 0);
            }
        } else if (i == MENU_EQUIPMENT_ROW_COUNT - 1 && arg3 == 0) {
            // The character panel's fifth row is left empty.
        } else if (arg1 < 2) {
            if (arg3 == 0) {
                ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].width = SystemRenderStringEntry(GetAccessoryName(pAccessoryIds[i - 1]), pHeap, 0x24, i % 2);
            } else if (arg1 != 0) {
                ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].width = SystemRenderStringEntry(GetGearAccessoryName(pAccessoryIds[i - 1]), pHeap, 0x24, i % 2);
            } else if (i == 1) {
                ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].width = SystemRenderStringEntry(GetGearWeaponName(pWeaponIds[3]), pHeap, 0x24, 1);
            } else {
                ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].width = SystemRenderStringEntry(GetGearAccessoryName(pAccessoryIds[i - 2]), pHeap, 0x24, i % 2);
            }
        } else if (arg3 == 0) {
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].width = SystemRenderStringEntry(GetWeaponName(*pExtraId), pHeap, 0x24, i % 2);
        } else {
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].width = SystemRenderStringEntry(GetGearWeaponName(*pExtraId), pHeap, 0x24, i % 2);
        }
        if (i & 1) {
            loadImage = 1;
        } else if (arg3 == 0) {
            loadImage = 0;
        } else if (arg1 == 0 && i == MENU_EQUIPMENT_ROW_COUNT - 1) {
            loadImage = 1;
        }
        if (loadImage != 0) {
            rect.x = 0x140 + ((i / 2) & 1) * 0x20;
            rect.y = 0x27 + (i / 4) * FONT_LETTER_HEIGHT;
            rect.w = 0x28;
            rect.h = FONT_LETTER_HEIGHT;
            LoadImage(&rect, pHeap);
            DrawSync(0);
        }
        MenuSetStringPrimitives(&((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i], i, 0xC, 0);
        if (i == MENU_EQUIPMENT_ROW_COUNT - 1 && arg3 == 0) {
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].tpage = GetTPage(0, 0, 0x180, 0);
            if ((g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0 & 0xFF]].gearId + 0xB) & 1) {
                palette = g_SystemPalette2;
            } else {
                palette = g_SystemPalette1;
            }
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].clut = palette;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].u0 = g_MenuNameTextureX[arg0 & 0xFF] * 4;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].v0 = g_MenuGlyphSjisTable[(arg0 & 0xFF) * 4];
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].u1 = g_MenuNameTextureX[arg0 & 0xFF] * 4 + 0x60;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].v1 = g_MenuGlyphSjisTable[(arg0 & 0xFF) * 4];
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].u2 = g_MenuNameTextureX[arg0 & 0xFF] * 4;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].v2 = g_MenuGlyphSjisTable[(arg0 & 0xFF) * 4] + FONT_LETTER_HEIGHT;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].u3 = g_MenuNameTextureX[arg0 & 0xFF] * 4 + 0x60;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].v3 = g_MenuGlyphSjisTable[(arg0 & 0xFF) * 4] + FONT_LETTER_HEIGHT;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].width = 0x60;
        }
        MenuSetQuadVertices(((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].vertices, rowX,
                            D_801E9D88[i + rowTableOfs][0],
                            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].width, FONT_LETTER_HEIGHT);
        pExtraId++;
        ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->builtRows[i] = 1;
    }
    ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->renderContext = g_Menu->renderContext;
    g_Menu->pManager->shouldRenderPanel[1] = 1;
    HeapFree(pHeap);
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D8EA4);


/* 84.26%. Same instruction multiset; gcc lays the `arg2` if/else out the
   other way in case 1 (it falls through to the 0x60 block and jumps to
   0x30) and therefore places the return block differently. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9704);

/* 84.26% re-measured under the unit's XenoMenu260 preset (260 bytes / 65
   insns, retail's exact size and count). Two residuals, both block order:
   (a) the `arg2` if/else: retail is `bnez a2 -> gear block` with the
   currentCharacterIDs arm as the fall-through, gcc always emits the gear arm as
   the fall-through and `beqz a2` to the characterIDs arm; (b) the case-0
   prologue order (`addiu a0,a0,1` before the `andi a2,a2,0xff`/`lui g_Menu`
   group where retail has it last).
   Round 430: an else-less then-arm ending in an explicit `continue` (which is
   the shape retail's duplicated `addiu a0,a0,1` in both back-edge delay slots
   suggests), the same with the arms swapped, and an `if (!arg2)` spelling all
   compile to output byte-identical to the if/else-if body below - gcc
   canonicalises the two arms, so arm order cannot reach (a).
   Tried: if/else-if in both
   orders, `for (arg0++; 1; arg0++)` vs `while (1)` + trailing arg0++, a
   pManager local (66.43%, makes gcc drop the second block's reload), goto
   labels (right order, but the loop stops being recognised so g_Menu and the
   0xFF constant are no longer hoisted, 62.38%). An explicit
   `arg0++; continue;` plus a second `arg0++` at the bottom gets the block
   order and the `bnez a2` right but cross-jumps the two increments into one
   shared block, costing 8 bytes (85.62%). The increment sitting in the branch
   delay slot of both paths is what the for-increment clause produces.
   Round 15: also tried the arms swapped (arg2 != 0 first) - identical
   84.26%; the block order follows whichever arm is written first and gcc
   then puts a beqz to the other one, so the original's `bnez a2` -> 0x60
   with the 0x30 block as fall-through is not reachable by arm order alone.
   With this variant the case-block prologue also puts `addiu a0,a0,1`
   before `andi a2,a2,0xff` whereas the original has it last.
s32 func_801D9704(s32 arg0, u_char arg1, u_char arg2) {
    switch (arg1) {
    case 0:
        for (arg0++; 1; arg0++) {
            if (arg0 >= 3) {
                arg0 = 0;
            }
            if (arg2 == 0) {
                if (g_Menu->pManager->currentCharacterIDs[arg0] != 0xFF) {
                    break;
                }
            } else if (g_Menu->pManager->isGearScreenActive[arg0] != 0) {
                arg0--;
                break;
            }
        }
        break;
    case 1:
        for (arg0--; 1; arg0--) {
            if (arg0 < 0) {
                arg0 = 2;
            }
            if (arg2 == 0) {
                if (g_Menu->pManager->currentCharacterIDs[arg0] != 0xFF) {
                    break;
                }
            } else if (g_Menu->pManager->isGearScreenActive[arg0] != 0) {
                arg0++;
                break;
            }
        }
        break;
    }
    return arg0;
}*/


INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9808);

/* 97.08%. Only `case 1` of the g_Menu->input switch differs: the original
   lays the then-arm (subMenuChoice--) out first and branches `beqz` forward to
   the subMenuNumChoices - 1 arm; gcc always emits the numChoices - 1 arm first
   and branches `bnez` forward to the decrement, and it reuses the loaded value
   in the branch delay slot instead of reloading it in the arm (the original
   reloads). Both if/else arm orders give byte-identical code. Everything else
   matches: frame 0x38, $s0 = choice, $s1 = firstPass, $s2 = quit (and the
   loop-test mask), $s3 = accepted, $s4 = the hoisted constant 1 of `case 1`,
   and the blocks come out in the order case 4+5 (fall-through), 1, 3, so the
   switch's source order is 4/5, 1, 3. The function returns 1 (the trailing
   `li v0,1` before jr ra). Full decode below.
s32 func_801D9808(void) {
    u_char quit = 1;
    u_char firstPass = 1;
    u_char accepted = 0;
    s32 choice;

    do {
        MenuRenderFrame();
        if (firstPass != 0) {
            MenuLoadStringsFromIds(4, g_Menu->unk1BE0, D_801EA574, g_Menu->pManager->shouldRenderStrings1BE0);
            func_801E86C8(0);
            g_Menu->subMenuPrevChoice = 0xFF;
            switch (SoundGetVolumeMode()) {
            case 0:
                choice = 0;
                break;
            case 1:
                choice = 2;
                break;
            case 2:
                choice = 1;
                break;
            }
            g_Menu->subMenuChoice = choice;
            firstPass = 0;
        }
        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            MenuSetRowQuadVertices(6, g_Menu->unk1BE0, g_MenuNameTextureX, D_801E9F88, g_Menu->pManager->shouldRenderStrings1BE0,
                          g_Menu->subMenuChoice, 7, 0);
            MenuSetSubMenuChoicePrims(0);
            g_Menu->subMenuPrevChoice = g_Menu->subMenuChoice;
        }
        switch (g_Menu->input) {
        case 4:
            accepted = 1;
        case 5:
            quit = 0;
            break;
        case 1:
            if (g_Menu->subMenuChoice == 0) {
                g_Menu->subMenuChoice = g_Menu->subMenuNumChoices - 1;
            } else {
                g_Menu->subMenuChoice--;
            }
            break;
        case 3:
            if (++g_Menu->subMenuChoice >= g_Menu->subMenuNumChoices) {
                g_Menu->subMenuChoice = 0;
            }
            break;
        }
    } while (quit != 0);
    if (accepted != 0) {
        switch (g_Menu->subMenuChoice) {
        case 0:
            choice = 0;
            break;
        case 1:
            choice = 2;
            break;
        case 2:
            choice = 1;
            break;
        }
        SoundSetVolumeMode(choice);
    }
    MenuClearBytes(4, g_Menu->pManager->shouldRenderStrings1BE0);
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    return 1;
}
*/
void MenuOpenCardEvents(void) {
    MenuCloseCardEvents();
    Vsync(0);
    InitCARD(1);
    StartCARD();
    _bu_init();
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    g_Menu->menuUnk2->cardEvents[0] = OpenEvent(SwCARD, EvSpIOE, EvMdNOINTR, NULL);
    g_Menu->menuUnk2->cardEvents[1] = OpenEvent(SwCARD, EvSpERROR, EvMdNOINTR, NULL);
    g_Menu->menuUnk2->cardEvents[2] = OpenEvent(SwCARD, EvSpTIMOUT, EvMdNOINTR, NULL);
    g_Menu->menuUnk2->cardEvents[3] = OpenEvent(SwCARD, EvSpNEW, EvMdNOINTR, NULL);
    EnableEvent(g_Menu->menuUnk2->cardEvents[0]);
    EnableEvent(g_Menu->menuUnk2->cardEvents[1]);
    EnableEvent(g_Menu->menuUnk2->cardEvents[2]);
    EnableEvent(g_Menu->menuUnk2->cardEvents[3]);
    ExitCriticalSection();
}
