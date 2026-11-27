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
extern u8 g_MenuSoundModeStringIds[];
extern u8 g_MenuGlyphSjisTable[]; // Glyph id of each character's name strip
extern u16 D_801E9D88[][2]; // Y offset of each equipment row of the equipment panel
extern s32 g_MenuNameTextureX[];
extern u8 g_MenuSubMenuRowXOffsets[];
extern s32 g_MenuShoulderButtonIconTexIds[]; // String IDs of the menu scroll bar textures
extern s32 g_MenuStatRowLabelOffsetX[]; // X offset of each stat row of the stat table
extern s32 g_MenuStatRowLabelOffsetY[]; // Y offset of each stat row of the stat table
extern s32 g_MenuStatRowLabelTexIds[]; // Texture id of each stat row's label, 7 rows per stat group
extern u8* GetWeaponName(s32 itemId);
extern u8* GetGearWeaponName(s32 itemId);
extern u8* GetAccessoryName(s32 itemId);
extern u8* GetGearAccessoryName(s32 itemId);
extern s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 length, s32 mode);

#define MENU_CHARACTER_SLOT_COUNT 3 // party slots the character cursor wraps over
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

// PARKED 88.01% under XenoMenu260 (gcc 2.6.0): 1964 bytes / 491 insns against retail 1952 / 488, frame 0xA0 against retail's 0xA8.
//  Named residual (lever j-slot-spacing-tells-decl-from-spill): retail's frame has NO
//  expand-time locals - its nine local slots sit 8 bytes apart (0x20 max, 0x28 arg3,
//  0x30 arg4, 0x38 pStat, 0x40 pSaved, 0x60 rowOfs, 0x68 deltaVertOfs, 0x70 colorOfs,
//  0x78 valueVertOfs; 12 slots x 8 = its 0x60 var area, with 0x48/0x50/0x58 allocated and
//  never touched), so every one of them is a reload SPILL and pStat/pSaved are plain
//  pointers the allocator ran out of registers for - not volatile decl locals. This body
//  puts pStat/pSaved/max in 4-byte-apart decl slots and only the four accumulators 8 apart,
//  so the frame is 8 short and every sp operand and slot number differs. Two smaller
//  residual pieces: arg1 lands in s4 here and s5 in retail (each later callee-saved
//  assignment rotates by one), and the two quad loops read their four u16 prim fields in a
//  different order (retail a1=0xEB8, a2=0xEBA, a3=0xEC0, v0=0xEB8, v1=0xEBA, v0=0xED2).
//  Third: retail reads D_801E9D78 and g_MenuStatBarValueWidth with lhu at the value quad
//  (0x3100) but with lw at the delta quad (0x31E0); this body gets lhu at both.
//  Next lever for the frame half: j-declaration-order-sets-the-reload-slot-order - sort the
//  declarations into retail's slot order (max, arg3, arg4, pStat, pSaved, rowOfs,
//  deltaVertOfs, colorOfs, valueVertOfs) once the pointers are spills, then re-measure; the
//  two attempts so far are the two ends (volatile = decl slots, plain = registers).
//  Tried this round: dropping volatile from pStat/pSaved reaches retail's EXACT 0xA8 frame
//  and 304 differing lines instead of 329, but gcc then keeps the two pointers in s0/s2
//  (retail spills them) and it measures 85.69% / 1968 bytes / 492 insns, so the slower
//  88.01 body is parked. Presets screened: 2.7.2 87.05, -O1 53.95, -O3 30.90, cdk and -G0
//  do not compile.
//  What is right: the whole shape - the arg3 if/else picking pEquipmentPanel+0x280 against
//  pDressingRoom->statValues, MenuMaxStatValue, the 7-arg4 row loop, one
//  MenuSetStatBarQuadColors + MenuSetQuadVertices + MenuParseNumberToDigits + 4-digit loop
//  per row, the digit-count quad loop reading the 0xEB8 prims into the 0x2A00 SVECTORs, the
//  every-other-row MenuSetStatBarTileColors, the arg3-gated delta block (0x2228 quad, 0x31E0
//  quad, 0x1770 prims, 3 delta digits, 0x2D80 SVECTORs) and the seven accumulators with
//  their 0x140/0x80/0x48/0x80/8 steps.
//  What moved it: 85.02 first body; 85.69 after spelling every per-row byte access as
//  ((u8*)g_Menu->unk35C + i)[0x32C0] - the (p + i)[K] form is the only one that emits
//  retail's base-first addu, probed with cc1_probe.sh, where p[K + i] emits index-first;
//  88.01 after declaring pStat/pSaved u16* volatile.
//  Also tried: an if around the loop instead of the early return (two blez, retail has
//  one), a do-while (83.76 - it moves the accumulator updates out of retail's order),
//  quadX split from digitX (kept, 85.09 -> 85.32), g_MenuStatRowLabelOffsetY[7..10] in
//  place of D_801E9D78..84 (reverted - it renames retail's R_MIPS_LO16 reloc).
// void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4) {
// 
// Round 489/current re-measured from a fresh unpark: 88.01% / 1964 bytes / 491 insns
// (frame 0xA0) reproduced exactly, and the next lever (j-declaration-order) is now
// measured DEAD on 2.6.0: sorting the declarations into retail's slot order
// (max, pStat, pSaved, rowOfs, deltaVertOfs, colorOfs, valueVertOfs, then the
// register-only ones) without volatile gives retail's EXACT frame (0xA8, vars=96 =
// 12 eight-byte slots) at 85.69% / 1968 / 492 and 304 differing lines instead of 329.
// Read the two ends properly now: retail has NO 4-byte decl slot at all (its var area
// starts at 0x20 and every slot is 8 apart), so it has no addressable local; its
// register set is only SEVEN long-lived values (arg1 s5, arg2 s6, yOfs s4, valuePrim
// fp, deltaPrim s7, i s1, j s0) with s2/s3 used as reload targets for rowOfs /
// valueVertOfs / deltaPrimOfs, while this body has NINE (it keeps pStat, pSaved and
// rowOfs) and spills deltaPrimOfs instead. So the residual is not a slot order or a
// frame size: it is that gcc 2.6.0 here finds redundant register candidates that
// retail's allocator spilled, i.e. retail's pStat/pSaved/rowOfs were not plain
// long-lived locals in the original source. Two ends measured, neither is the answer;
// both kept in this note.
/*
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
// PARKED best size: 2144 bytes / 536 insns = RETAIL'S EXACT SIZE, 226 differing lines, under
// XenoMenu260 (gcc 2.6.0) - the body below, which spells the row dispatch as a `switch (i)`.
// objdiff prints only 64.99% for it because the arms are laid out in a different order, so judge
// this body by the byte count and the differing-line count, not the percentage (levers.md:
// objdiff's fuzzy number is not monotone). The same body with the dispatch written as an
// if/else-if chain measures 91.59% at 2140 bytes / 535 insns and 232 differing lines - one
// instruction SHORT of retail, which is what makes the switch the better base.
// FIXED this round (worth 90.50 -> 91.59 on the if-chain body, and it is why the switch body now
// has retail's exact size): arg0 is a `u_char` PARAMETER, not `s32 arg0` masked with `& 0xFF` at
// every use. A QI parameter makes gcc emit retail's prologue exactly - `sw s1 / move s1,a0 /
// sw s4 / move s4,a1` at the very top, then `lui a1,%hi(g_Menu)` (a1 is free once arg1 has been
// copied out) - where the s32 spelling left the parameter copy sunk to its first use, put
// g_Menu in t0 and rotated every later callee-saved assignment by one (318 differing lines).
// REMAINING residuals, all measured:
// - the dispatch ORDER. Retail's loop head is `beqz s0 -> case 0` (out of line), then a SIGNED
//   range test on i (`bltz s0` / `slti v0,s0,5` / `beqz v0` -> the loadImage chain), then
//   `j <the i == 4 test>` with `li v0,4` in the delay slot. Our switch - case 0, case 4, default,
//   with the case-4 arm falling through into default - gives the right branch direction and the
//   right sizes (`beqz s0` / `li v0,4` / `beq s0,v0`) but no range test. A range test needs
//   case labels for 1..3 as well, and a 5-case dense switch makes 2.6.0 emit a JUMP TABLE, which
//   retail does not have (no `jr` anywhere in the stub), so the original is a switch this C
//   cannot express yet. An if/else-if chain (`if (i == 0) ... else if (i == 4 && arg3 == 0) skip
//   ...`) compiles to `bnez s0` (mirrored) and no range test.
// - the callee-saved rotation: retail keeps TextureX base=$s6, Glyph base=$s5, the pExtraId
//   cursor=$s4 (the register arg1's raw value vacates), arg1&0xff=$s7; ours gives Glyph=$s4,
//   TextureX=$s5, arg1&0xff=$s6, cursor=$s7, i.e. the four rotate by one. The cursor's load is
//   also hoisted into the loop-entry `beqz` delay slot where retail has a `nop` and loads it last
//   in the preheader (`lw s4,0x40(sp)` after `move s1,zero`). Not yet read with -dl/-dg.
// - the i == 4 arm: retail forms the poly address (`renderContext * 0x28 + panel + i * 0x80`)
//   ONCE and stores tpage/clut/u/v through it, and its `addu` order is `addu a0,s1,a0` then
//   `addu v0,v0,a0`; ours re-forms the address per store and swaps both operands. Same for the
//   four vertex reads (`lbu a0,0(s5)` off the Glyph walker in retail).
// - rowX: retail stores it with `sw` into 0x28(sp) (with 0x2C left empty, rowTableOfs at 0x30)
//   and reloads with `lhu a1,0x28(sp)`; a u16 local gives `sh` (2 lines), a u32 local gives
//   `lw`+`andi` and 2152 bytes / 538 insns. No spelling found for sw-store + lhu-load.
// - `sw s1,0x50(sp)` (the masked arg0 the i == 4 arm re-reads) sits before the two table bases
//   here and after them in retail.
// - retail names the texture-X table D_801EA584 where this file calls it g_MenuNameTextureX:
//   same address, so it costs 2 diff lines and no bytes.
// Measured dead this round: `u_char rowGroup = arg1;` used everywhere instead of arg1 (byte
// identical), `s32 arg0` (90.50), -O1 63.48, -O3 90.50, gcc 2.7.2 90.35, 2.7.2-cdk 80.65, and a
// 4-minute permuter run (base 5420 -> best 4965) whose only find was the u_char arg0 parameter.
// Ghidra/asm facts worth keeping: pEquipmentPanel is a 0x2AC buffer of 5 MenuString plus its own
// id copies; g_MenuNameTextureX is s32[] (lw, stride 4); g_MenuGlyphSjisTable is read as u8 with
// stride 4; D_801E9D88 is u16[][2] with stride 4; rect.x = 0x140 + ((i / 2) & 1) * 0x20 and
// rect.y = 0x27 + (i / 4) * FONT_LETTER_HEIGHT; the polys of one MenuString start at 0x200 and
// are 0x28 bytes each (u0 0xC, u1 0x14, u2 0x1C, u3 0x24, clut 0xE, tpage 0x16).
/*
void func_801D8EA4(u_char arg0, u_char arg1, u_char arg2, u_char arg3) {
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
    pWeaponIds = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].weaponId;
    pExtraIds = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].extraWeaponIds;
    pAccessoryIds = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].accessorySlots;
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
        pWeaponIds = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].weaponId;
        pExtraIds = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].extraWeaponIds;
        pAccessoryIds = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].accessorySlots;
    }
    if (arg2 != 0) {
        pWeaponIds = ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->weaponIds;
        pExtraIds = ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->extraWeaponIds;
        pAccessoryIds = ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->accessoryIds;
    }
    pHeap = HeapAlloc(0x3F6, 0);
    pExtraId = pExtraIds;
    for (i = 0; i < rowCount; i++) {
        switch (i) {
        case 0:
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
            break;
        case MENU_EQUIPMENT_ROW_COUNT - 1:
            // The character panel's fifth row is left empty.
            if (arg3 == 0) {
                break;
            }
            // falls through to the default arm
        default:
            if (arg1 < 2) {
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
            break;
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
            if ((g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId + 0xB) & 1) {
                palette = g_SystemPalette2;
            } else {
                palette = g_SystemPalette1;
            }
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].clut = palette;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].u0 = g_MenuNameTextureX[arg0] * 4;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].v0 = g_MenuGlyphSjisTable[arg0 * 4];
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].u1 = g_MenuNameTextureX[arg0] * 4 + 0x60;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].v1 = g_MenuGlyphSjisTable[arg0 * 4];
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].u2 = g_MenuNameTextureX[arg0] * 4;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].v2 = g_MenuGlyphSjisTable[arg0 * 4] + FONT_LETTER_HEIGHT;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].u3 = g_MenuNameTextureX[arg0] * 4 + 0x60;
            ((MenuEquipmentPanel*)g_Menu->pEquipmentPanel)->strings[i].polys[g_Menu->renderContext].v3 = g_MenuGlyphSjisTable[arg0 * 4] + FONT_LETTER_HEIGHT;
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




// Steps the character cursor one slot in `direction` (0 down, 1 up) and returns
// the first slot it lands on that `gearScreen` selects: an assigned character
// (currentCharacterIDs) or a live gear screen (isGearScreenActive).
s32 MenuStepPartySlot(s32 slot, u_char direction, u_char gearScreen) {
    switch (direction) {
    case 0:
        while (1) {
            if (++slot >= MENU_CHARACTER_SLOT_COUNT) {
                slot = 0;
            }
            if (gearScreen == 0) {
                if (g_Menu->pManager->currentCharacterIDs[slot] != 0xFF) {
                    break;
                }
            } else if (g_Menu->pManager->isGearScreenActive[slot] != 0) {
                break;
            }
        }
        break;
    case 1:
        while (1) {
            if (--slot < 0) {
                slot = MENU_CHARACTER_SLOT_COUNT - 1;
            }
            if (gearScreen == 0) {
                if (g_Menu->pManager->currentCharacterIDs[slot] != 0xFF) {
                    break;
                }
            } else if (g_Menu->pManager->isGearScreenActive[slot] != 0) {
                break;
            }
        }
        break;
    }
    return slot;
}



s32 MenuRunSoundModeMenu(void) {
    u_char quit = 1;
    u_char firstPass = 1;
    u_char accepted = 0;
    s32 choice;

    do {
        MenuRenderFrame();
        if (firstPass != 0) {
            MenuLoadStringsFromIds(4, g_Menu->unk1BE0, g_MenuSoundModeStringIds, g_Menu->pManager->shouldRenderStrings1BE0);
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
            MenuSetRowQuadVertices(6, g_Menu->unk1BE0, g_MenuNameTextureX, g_MenuSubMenuRowXOffsets, g_Menu->pManager->shouldRenderStrings1BE0,
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
            if (g_Menu->subMenuChoice != 0) {
                g_Menu->subMenuChoice--;
            } else {
                g_Menu->subMenuChoice = g_Menu->subMenuNumChoices - 1;
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
