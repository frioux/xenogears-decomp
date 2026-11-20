#include "common.h"

#include "main/game.h"
#include "system/memory.h"
#include "system/menu.h"

// gear hp/maxHp are 32-bit, so their window numbers have five digits
#define MENU_GEAR_HP_DIGIT_COUNT 5

// the icon texture id tables hold one entry per slot of the character's icon strip
#define MENU_ICON_TEXTURE_COUNT 24
#define MENU_ICON_NONE 0xFFFF




void MenuParseNumberToDigits(u32 number);

// A window's tween state, one per window: &((MenuWindowTween*)g_Menu->windowTweens)[index].
typedef struct {
    /* 0x00 */ s32 startX; // the window's x at offsetX == 0
    /* 0x04 */ s32 endX; // where the tween stops
    /* 0x08 */ s32 startY;
    /* 0x0C */ s32 endY;
    /* 0x10 */ s32 stepX; // added to offsetX per step, in 8.8 fixed point
    /* 0x14 */ s32 stepY;
    /* 0x18 */ s32 offsetX; // x offset from startX, in 8.8 fixed point
    /* 0x1C */ s32 offsetY; // y offset from startY, in 8.8 fixed point
    /* 0x20 */ u8 isDecreasingX; // set when endX < startX
    /* 0x21 */ u8 isDecreasingY; // set when endY < startY
    /* 0x22 */ u8 stepCount; // steps taken per update
    /* 0x23 */ u8 isComplete; // set once the offset reaches its end
} MenuWindowTween; // Size: 0x24

// The gear window's prim buffer (g_Menu->pGearWindowPrims): this unit only needs the copy
// of the render context at its tail.
typedef struct {
    /* 0x0000 */ u8 unk0[0x2AE0];
    /* 0x2AE0 */ u_char renderContext;
    /* 0x2AE1 */ u8 unk2AE1;
    /* 0x2AE2 */ u8 unk2AE2;
    /* 0x2AE3 */ u8 firstWideNumberPrimCount; // prims the first wide number drew
    /* 0x2AE4 */ u8 secondWideNumberPrimCount; // prims the second wide number drew
    /* 0x2AE5 */ u8 unk2AE5; // prims the first weight-digit block drew
    /* 0x2AE6 */ u8 unk2AE6; // prims the second weight-digit block drew
    /* 0x2AE7 */ u8 hpPrimCount; // digit prims the hp number drew
    /* 0x2AE8 */ u8 maxHpPrimCount; // digit prims the maxHp number drew
    /* 0x2AE9 */ u8 unk2AE9[0x2];
    /* 0x2AEB */ u8 digitPrimCount; // digit prims the gear window's number draws appended
    /* 0x2AEC */ u8 iconPrimCount; // digit prims the character's icon ids drew
} MenuGearWindowPrims; // Size: 0x2AF0

// The gold window's prim buffer (g_Menu->pGoldWindow): the digit prims plus the two
// glyph prims at the tail.
typedef struct {
    /* 0x000 */ POLY_FT4 digitPrims[18]; // two per digit, one per render context
    /* 0x2D0 */ POLY_FT4 glyphPrims[2]; // one per render context, added by MenuAddGoldAndPlayTimePrims
    /* 0x320 */ s32 digitPrimCount;
    /* 0x324 */ u_char renderContext;
} MenuGoldWindow; // Size: 0x328

void MenuSetWindowScrollBar(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuInitializeWindowBorderCorners(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuSetWindowBorderTop(u_char a0, u_short a1, u_short a2, u_short a3);
void MenuSetWindowBorderBottom(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuSetWindowBorderLeft(u_char a0, u_short a1, u_short a2, u_short a3);
void MenuSetWindowBorderRight(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuSetCharacterPortraitPrims(u_char a0, u_char a1, s32 a2, s32 a3);
void MenuSetCharacterWindowIcons(u_char a0, s32 a1, s32 a2);
void MenuSetCharacterHpDigits(u_char a0, u_char a1, s32 a2, s32 a3);
void MenuSetCharacterMpDigits(u_char a0, u_char a1, s32 a2, s32 a3);
void MenuSetCharacterWideNumberDigits(u_char a0, u_char a1, s32 a2, s32 a3);
void MenuSetCharacterLevelDigits(u_char a0, u_char a1, s32 a2, s32 a3);
extern s32 g_MenuCharacterIconX[];
extern s32 g_MenuCharacterIconY[];
extern s32 D_801E9B60[];
extern s32 D_801E9C20[];
extern s32 g_MenuCharacterIconTexIds[];
extern s32 D_801EA39C[];
extern s32 g_MenuCharacterHpDigitsX;
extern s32 g_MenuCharacterMpDigitsX;
extern s32 g_MenuCharacterMpDigitsY;
extern s32 g_MenuCharacterMaxMpDigitsX;
extern s32 g_MenuCharacterMaxMpDigitsY;
extern s32 g_MenuCharacterWideNumberDigitsX;
extern s32 g_MenuCharacterWideNumberDigitsY;
extern s32 g_MenuCharacterWideNumber2DigitsX;
extern s32 g_MenuCharacterWideNumber2DigitsY;
extern s32 g_MenuCharacterHpDigitsY;
extern s32 g_MenuCharacterMaxHpDigitsX;
extern s32 g_MenuCharacterMaxHpDigitsY;
extern s32 g_MenuCharacterNumberOrigins;
extern s32 g_MenuCharacterLevelDigitsY;
extern s32 g_MenuCharacterUnk63DigitsX;
extern s32 g_MenuCharacterUnk63DigitsY;
extern u16 D_801E9B58;
extern u16 D_801E9D38;
extern u16 D_801E9D3C;
extern u16 D_801E9B5C;
extern s32 g_MenuNameTextureX[];
extern u8 g_MenuNameTextureY[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);
void MenuInitializePolyFt4(POLY_FT4* pPoly);

void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
s32 func_800263E4(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale, s32 mirrorX, s32 mirrorY);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void func_801D5ED4(u_char index, u_char characterId);
void MenuSetCharacterIconPrims(u_char characterId);
void func_801D6338(u_char index, u_char characterId);
void func_801D680C(u_char index, u_char characterId);
void func_801D6CF4(u_char index, u_char characterId);
void MenuSetGearWindowWideNumberPrims(u_char index, u_char characterId);
void func_801D74EC(u_char index, u_char characterId);
void func_801D7884(u_char index, u_char characterId);

void MenuSetGoldDigits(s32 x, s32 y) {
    s32 i;
    s32 digitX;

    MenuParseNumberToDigits(g_GameState.gold);
    ((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount = 0;
    for (i = 0, digitX = x; i < MENU_MAX_DIGITS; ) {
        u8 digit = g_Menu->digits[i++];

        if (digit != MENU_DIGIT_NONE) {
            // The prim address is summed in integer space: a pointer sum here
            // gives gcc the commuted `addu a2,t0,a2` retail does not have.
            ((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount =
                func_8002675C(g_Menu->resources, digit,
                              (POLY_FT4*)(((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount *
                                              (2 * sizeof(POLY_FT4)) +
                                          (u32)((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrims),
                              g_Menu->renderContext, digitX, y, 0x1000) +
                ((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount;
        }
        digitX += MENU_DIGIT_SPACING;
    }
    func_8002675C(g_Menu->resources, 0x10,
                  &((MenuGoldWindow*)g_Menu->pGoldWindow)->glyphPrims[0],
                  g_Menu->renderContext, x + 0x50, y, 0x1000);
    g_Menu->pManager->goldDigitsActive = 1;
    ((MenuGoldWindow*)g_Menu->pGoldWindow)->renderContext = *(u8*)&g_Menu->renderContext;
}
void MenuSetPlayTimeDigitPrims(s32 x, s32 y) {
    s32 i;

    // Hours, minutes and seconds, each with its own x spacing. The slot address
    // is summed in integer space: `polys[i]` gives the commuted addu retail does
    // not have (MenuSetGoldDigits above has the same constraint).
    for (i = 0; i < 3; i++) {
        func_8002675C(g_Menu->resources, g_Menu->playTimeDigitIds[i],
                      (POLY_FT4*)(i * sizeof(g_Menu->pPlayTimeDigits->polys[0]) + (s32)g_Menu->pPlayTimeDigits->polys),
                      g_Menu->renderContext, x + i * MENU_DIGIT_SPACING, y, 0x1000);
    }
    for (i = 3; i < 5; i++) {
        func_8002675C(g_Menu->resources, g_Menu->playTimeDigitIds[i],
                      (POLY_FT4*)(i * sizeof(g_Menu->pPlayTimeDigits->polys[0]) + (s32)g_Menu->pPlayTimeDigits->polys),
                      g_Menu->renderContext, x + i * MENU_DIGIT_SPACING + MENU_DIGIT_SPACING, y, 0x1000);
    }
    for (i = 5; i < 7; i++) {
        func_8002675C(g_Menu->resources, g_Menu->playTimeDigitIds[i],
                      (POLY_FT4*)(i * sizeof(g_Menu->pPlayTimeDigits->polys[0]) + (s32)g_Menu->pPlayTimeDigits->polys),
                      g_Menu->renderContext, x + i * MENU_DIGIT_SPACING + MENU_DIGIT_SPACING * 2, y, 0x1000);
    }
    func_8002675C(g_Menu->resources, MENU_TEX_PLAY_TIME_COLON, &g_Menu->pPlayTimeDigits->polys[7],
                  g_Menu->renderContext, x + MENU_DIGIT_SPACING * 3, y, 0x1000);
    func_8002675C(g_Menu->resources, MENU_TEX_PLAY_TIME_COLON, &g_Menu->pPlayTimeDigits->polys[9],
                  g_Menu->renderContext, x + MENU_DIGIT_SPACING * 6, y, 0x1000);
    g_Menu->pPlayTimeDigits->renderContext = *(u8*)&g_Menu->renderContext;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7_r2", func_801D5ED4);
/* Best: 99.32% at retail's exact 704 bytes (176 insns, so it WOULD link).
   Every instruction is present and the statement shapes below are right; the
   only residue is which of $v1/$a0 carries the pBase/pRow address in three
   blocks (retail: then-arm store address in $a0, else-arm in $v1; this build
   picks the other way round), plus the second MenuSetQuadVertices block, where
   retail keeps the base in $v1 and the ctx in $a1 and this build swaps them.
   HOW IT GOT HERE (all re-measured under gcc 2.6.0):
   - the four 0x5E palette stores must have their OWN address variable per arm
     (pBase in the then arm, pRow - a copy - in the else arm). A single shared
     pBase over both arms lets the allocator CSE the two SystemPalette1 stores
     into one block, which costs 4 instructions and every branch offset
     (87.10% at 688 bytes). Fully inline in both arms is 85.84%.
   - the 0x66 GetTPage store must stay INLINE: assigning pBase before it makes
     cc1 hoist the address above the GetTPage call (97.13%).
   - `((texX << 1) << 1) & 0xFC` (two shifts, not one) is worth 97.13 -> 99.32
     and retail's exact size; a single `x << 2` makes gcc narrow the
     g_MenuNameTextureX load to `lbu` (retail has `lw`), which is 4
     instructions in the wrong place. texX is an s32 local for the same reason.
   Not tried: the permuter found a candidate with a junk `if (g_Menu)` arm in
   the else branch that measured better; port only if the register tie can be
   closed.

void func_801D5ED4(u_char index, u_char row) {
    s32 rowOffset = row ? 0x18 : 0;
    u8* pRow;
    u8* pBase;
    s32 texX;
    u16 width;

    func_8002675C(g_Menu->resources, index + 0x14B, (POLY_FT4*)g_Menu->pGearWindowPrims,
                  g_Menu->renderContext, 0x18 - rowOffset, 0xE, 0x1000);
    MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EA0),
                        *(u16*)((g_Menu->renderContext * 0x28 + (u8*)g_Menu->pGearWindowPrims) + 0x8),
                        *(u16*)((g_Menu->renderContext * 0x28 + (u8*)g_Menu->pGearWindowPrims) + 0xA),
                        0x30, 0x30);
    MenuInitializePolyFt4((POLY_FT4*)((g_Menu->renderContext * 0x28 + 0x50) +
                                      (u8*)g_Menu->pGearWindowPrims));
    *(u16*)((u8*)g_Menu->pGearWindowPrims + g_Menu->renderContext * 0x28 + 0x66) =
        GetTPage(0, 0, 0x180, 0);
    if (row == 0) {
        pBase = (u8*)g_Menu->pGearWindowPrims + g_Menu->renderContext * 0x28;
        if (g_Menu->pManager->currentCharacterIDs[index] & 1) {
            *(u16*)(pBase + 0x5E) = g_SystemPalette2;
        } else {
            *(u16*)(pBase + 0x5E) = g_SystemPalette1;
        }
    } else {
        pBase = (u8*)g_Menu->pGearWindowPrims + g_Menu->renderContext * 0x28;
        pRow = pBase;
        if ((g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId + 0xB) & 1) {
            *(u16*)(pRow + 0x5E) = g_SystemPalette2;
        } else {
            *(u16*)(pRow + 0x5E) = g_SystemPalette1;
        }
    }
    width = 0x18 * row + 0x48;
    texX = g_MenuNameTextureX[3 * row + index];
    MenuSetFt4QuadVertices((POLY_FT4*)((g_Menu->renderContext * 0x28 + 0x50) +
                                       (u8*)g_Menu->pGearWindowPrims),
                           0, 0, ((texX << 1) << 1) & 0xFC,
                           g_MenuNameTextureY[(3 * row + index) * 4], width, 0xD);
    MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EC0),
                        D_801E9D38 - rowOffset, D_801E9D3C, width, 0xD);
}
*/
void MenuSetCharacterIconPrims(u_char characterId) {
    s32 i;
    u8* pBase;
    u8* pEntry;

    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->iconPrimCount = 0;
    for (i = 0; i < MENU_ICON_TEXTURE_COUNT; i++) {
        if (D_801EA39C[characterId * MENU_ICON_TEXTURE_COUNT + i] == MENU_ICON_NONE) continue;
        ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->iconPrimCount += func_8002675C(
            g_Menu->resources,
            D_801EA39C[characterId * MENU_ICON_TEXTURE_COUNT + i],
            (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                        (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->iconPrimCount *
                             (2 * sizeof(POLY_FT4)) +
                         0xA0)),
            g_Menu->renderContext,
            D_801E9B60[characterId * MENU_ICON_TEXTURE_COUNT + i],
            D_801E9C20[characterId * MENU_ICON_TEXTURE_COUNT + i],
            0x1000);
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->iconPrimCount; i++) {
        POLY_FT4* pPrim;
        s32 offset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        pBase = (u8*)g_Menu->pGearWindowPrims;
        pEntry = pBase + offset;
        pPrim = (POLY_FT4*)(pEntry + 0xA0);
        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EE0 + i * 0x20),
                            pPrim->x0, pPrim->y0,
                            pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
extern s32 D_801E9CF0;
extern s32 D_801E9CF4;
extern s32 D_801E9CF8;
extern s32 D_801E9CFC;

INCLUDE_ASM("asm/menu/nonmatchings/main/misc7_r2", func_801D6338);
/* Best: 95.53% at retail's EXACT 1236 bytes (all 309 instructions present). Two
   diffs left, both in the two identical blocks and both only a PLACEMENT:
   (1) retail fills the g_Menu load-delay slot with `addu v0,s5,s0` (the
   `digitOffset + i` sum) where gcc emits its `nop` and puts the sum one slot
   earlier; (2) retail defers the prim address's base add to the jal delay slot
   (`sll/addu/sll a2` then `addiu a2,a2,0x1630` -- wait, it keeps 0x1630 in the
   sll chain and adds the g_Menu->pGearWindowPrims base in the slot) while gcc
   computes `addiu t0,t0,0x1630` on the base FIRST and then `addu a2,a2,t0`, and
   swaps which call-clobbered temp holds the base (t1 retail, t0 here) and the
   count (t0/t1). Same instruction multiset, same registers otherwise
   (s0=i, s1=x/vertOffset, s2=count, s3=24*characterId, s4=index,
   s5=digitOffset, s6=characterId all agree with retail).
   FACTS LEARNED (worth keeping, they got it from 85.01% to here):
   - the two 24/8 offsets must be INLINE (`x - characterId * 24`,
     `characterId * 8 + D_801E9CFC`), not locals: with locals the body is 1200
     bytes (9 instructions short) and 85.01%; inlining gives retail's 1236.
   - the first block's MenuParseNumberToDigits call is SHARED: write
     `x = ...; MenuParseNumberToDigits(...)` in each arm (x BEFORE the call) so
     cross-jumping merges the identical call tails into retail's single jal. The
     second block's arms each keep their OWN jal (there x is assigned AFTER the
     call, so the tails differ) - block B goes 89.62% if you hoist it out.
   - names: block A draws hp into g_Menu->pGearWindowPrims 0x2AE7 (now
     hpPrimCount) at prim base +0x1630, verts +0x2780; block B draws maxHp into
     0x2AE8 (maxHpPrimCount) at +0x17C0, verts +0x2820. x starts at
     D_801E9CF0 / D_801E9CF0 + 8 and D_801E9CF8 / D_801E9CF8 - 0x20, y is
     D_801E9CF4 and D_801E9CFC + 8*characterId, digits are g_Menu->digits[
     digitOffset + i] with digitOffset 6 (3 digits, hp) or 4 (5 digits, gear hp).
   - the quad loops need the twin-lhu volatile minuend
     (`pPrim->x1 - *(volatile u16*)&pPrim->x0`) or gcc CSEs the two loads.
   NOT DONE: the permuter rejects this body ("conversion to non-scalar type
   requested", same message as func_801D7884's park) - almost certainly the
   `(SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims)` casts; spelling them
   `(SVECTOR*)((u8*)g_Menu->pGearWindowPrims + vertOffset)` may let it parse.
// The character (3 digits of hp/maxHp) or gear (5 digits) window numbers: two
// digit-prim passes and the quad pass that grows each prim to its glyph box.
void func_801D6338(u_char index, u_char characterId) {
    s32 i;
    s32 x;
    s32 count;
    s32 digitOffset;

    if (characterId == 0) {
        count = MENU_HP_DIGIT_COUNT;
        digitOffset = 6;
        x = D_801E9CF0;
        MenuParseNumberToDigits(g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].hp);
    } else {
        count = MENU_GEAR_HP_DIGIT_COUNT;
        digitOffset = 4;
        x = D_801E9CF0 + 8;
        MenuParseNumberToDigits(g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].hp);
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount = 0;
    for (i = 0; i < count; ) {
        u8 digit = g_Menu->digits[digitOffset + i++];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount *
                                (2 * sizeof(POLY_FT4)) +
                            0x1630 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, x - characterId * 24, D_801E9CF4, 0x1000);
        }
        x += 8;
    }
    vertOffset = 0x2780;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0x1630);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }

    if (characterId == 0) {
        MenuParseNumberToDigits(g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].maxHp);
        x = D_801E9CF8;
    } else {
        MenuParseNumberToDigits(g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].maxHp);
        x = D_801E9CF8 - 0x20;
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount = 0;
    for (i = 0; i < count; ) {
        u8 digit = g_Menu->digits[digitOffset + i++];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount *
                                (2 * sizeof(POLY_FT4)) +
                            0x17C0 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, x - characterId * 24, characterId * 8 + D_801E9CFC, 0x1000);
        }
        x += 8;
    }
    vertOffset = 0x2820;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0x17C0);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
*/
extern s32 D_801E9D00;
extern s32 D_801E9D04;
extern s32 D_801E9D08;
extern s32 D_801E9D0C;

// The gear window's mp (character: two digits) or fuel (gear: four digits) numbers:
// the same two digit-prim passes and quad pass as the stat numbers above.
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7_r2", func_801D680C);
/* Best: 99.62% at retail's EXACT 1256 bytes (314 instructions, frame 0x68). The
   structure, all seven calls, both digit loops and both quad loops are exact; the
   ONLY residue is the fill of one load-delay slot in the SECOND digit loop: retail
   puts `addiu s1,s1,8` (the x bump) in the `lbu` count load's delay slot and a
   `nop` in the loop-back branch's delay slot; this build puts the nop in the lbu
   slot and the addiu in the branch slot. Same instruction multiset, so no spelling
   moves it - this is the delay-slot-fill class AGENTS.md records as having no
   source lever, and a 4-minute permuter run (1636 iterations, base=380 best=380)
   from the 97.60% body found nothing. The FIRST digit loop is identical in both
   compiles WITH the addiu in the branch delay slot, so retail's two halves really
   do differ only in this placement.
   FACTS LEARNED (97.57 -> 99.62, all re-measured under gcc 2.6.0):
   - this is the mp (character) / fuel (gear) window: characters[id].mp/maxMp
     (0x50/0x52) with 2 digits from g_Menu->digits[7 + i], and
     gears[characters[id].gearId].fuel/maxFuel (0x38/0x3A) with 4 digits from
     digits[5 + i]; count and digitOffset are set once at the top and reused by
     both halves (s3/s6).
   - x/y are per-arm locals: D_801E9D00 / D_801E9D04 (half 1), D_801E9D08 (0x20
     less for a gear) / D_801E9D0C (+0x10 for a gear) (half 2). Half 2's arms must
     call MenuParseNumberToDigits FIRST and assign x/y AFTER it.
   - the FIRST quad loop's verts sum must inline the global:
     `(SVECTOR*)(vertOffset + (u32)(u8*)g_Menu->pGearWindowPrims)`. With a pPrims
     local there, g_Menu's loop-carried reload lands in $v1 where retail has $a0,
     and both quad loops then emit their two loads in the wrong order with the
     wrong register set (97.60% at the same size).
   - the first quad loop's prim address keeps a LOCAL base
     (`pBase = (u8*)g_Menu->pGearWindowPrims; pEntry = pBase + primOffset;`): with
     the global inline in that sum, the address comes out `addu v1,v1,a0` where
     retail has base-first `addu v1,a0,v1` (99.59 vs 99.62).
   - the two verts offsets need SEPARATE locals (`vertOffset = 0x28C0 + i * 0x20`
     in loop 1, `vertsOffset = 0x2960 + i * 0x20` in loop 2): one variable shared
     by both loops is not strength-reduced in loop 1 (gcc recomputes
     `sll i,5 / addiu` there) and the body drops to 95.08% at 1260 bytes.
   - an explicit `vertOffset = 0x28C0;` before the loop plus a `+= 0x20` latch is
     97.09% at 1252 bytes: the `li` is hoisted ABOVE the loop guard, retail has it
     after.
   - the x bump stays the last statement of each digit loop body; the
     for-increment comma form is 98.90% and moving it inside the `if` is 99.28%.
   void func_801D680C(u_char index, u_char characterId) {
       u8* pPrims;
       u8* pBase;
       u8* pEntry;
       s32 vertOffset;
       s32 vertsOffset;
       s32 count;
       s32 digitOffset;
       s32 x;
       s32 y;
       s32 i;

       if (characterId == 0) {
           count = 2;
           digitOffset = 7;
           x = D_801E9D00;
           y = D_801E9D04;
           MenuParseNumberToDigits(
               g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].mp);
       } else {
           count = 4;
           digitOffset = 5;
           x = D_801E9D00;
           y = D_801E9D04 + 8;
           MenuParseNumberToDigits(
               g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].fuel);
       }
       ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE9[0] = 0;
       for (i = 0; i < count; i++) {
           if (g_Menu->digits[digitOffset + i] != MENU_DIGIT_NONE) {
               ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE9[0] += func_8002675C(
                   g_Menu->resources, g_Menu->digits[digitOffset + i],
                   (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                               (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE9[0] *
                                    (2 * sizeof(POLY_FT4)) +
                                0x1950)),
                   g_Menu->renderContext, x + i * MENU_DIGIT_SPACING - characterId * 24, y, 0x1000);
           }
       }
       for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE9[0]; i++) {
           POLY_FT4* pPrim;
           s32 primOffset;

           primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);
           pBase = (u8*)g_Menu->pGearWindowPrims;
           pEntry = pBase + primOffset;
           pPrim = (POLY_FT4*)(pEntry + 0x1950);
           vertOffset = 0x28C0 + i * 0x20;
           MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)(u8*)g_Menu->pGearWindowPrims), pPrim->x0,
                               pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                               pPrim->y3 - *(volatile u16*)&pPrim->y0);
       }

       if (characterId == 0) {
           MenuParseNumberToDigits(
               g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].maxMp);
           x = D_801E9D08;
           y = D_801E9D0C;
       } else {
           MenuParseNumberToDigits(
               g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].maxFuel);
           x = D_801E9D08 - 0x20;
           y = D_801E9D0C + 0x10;
       }
       ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE9[1] = 0;
       for (i = 0; i < count; i++) {
           if (g_Menu->digits[digitOffset + i] != MENU_DIGIT_NONE) {
               ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE9[1] += func_8002675C(
                   g_Menu->resources, g_Menu->digits[digitOffset + i],
                   (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                               (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE9[1] *
                                    (2 * sizeof(POLY_FT4)) +
                                0x1AE0)),
                   g_Menu->renderContext, x + i * MENU_DIGIT_SPACING - characterId * 24, y, 0x1000);
           }
       }
       for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE9[1]; i++) {
           POLY_FT4* pPrim;
           s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

           vertsOffset = 0x2960 + i * 0x20;
           pPrims = (u8*)g_Menu->pGearWindowPrims;
           pBase = pPrims;
           pEntry = pBase + primOffset;
           pPrim = (POLY_FT4*)(pEntry + 0x1AE0);
           MenuSetQuadVertices((SVECTOR*)(vertsOffset + (u32)pPrims), pPrim->x0, pPrim->y0,
                               pPrim->x1 - *(volatile u16*)&pPrim->x0,
                               pPrim->y3 - *(volatile u16*)&pPrim->y0);
       }
   }
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7_r2", func_801D6CF4);
/* Best: 63.92% (892 bytes; retail is 1120 = 0x460). Decode, all seven calls and both
   blocks are laid out below; the 228-byte shortfall is a source shape, not a missing
   statement - start from the first difference, which is that retail computes rowOffset
   (s0 = row * 24) AFTER the first MenuParseNumberToDigits (mine hoists it into the
   prologue) and keeps the row parameter in s3 masked at each use.
   Decode: two identical gear-stat blocks, the shape of the parked MenuSetGearWindowWideNumberPrims.
   Block A: MenuParseNumberToDigits(characters[id].level, g_GameState + 0x2CE), count
   byte 0x2AE1, a 3-digit loop over g_Menu->digits[6 + i] (!= 0xFF) calling
   func_8002675C with 0x50-byte prims at +0xAF0, x = i * 8 + D_801E9CE0 - row * 24,
   y = D_801E9CE4, then a count-time MenuSetQuadVertices loop over SVECTORs at
   +0x2300 + i * 0x20 reading the +0xAF0 prim at context (renderContext + 2 * i):
   x0 +0xAF8, y0 +0xAFA, x1 +0xB00, y3 +0xB12.
   Block B is the same with characters[id].unk63 (0x2CF), count 0x2AE2, prims +0xBE0,
   D_801E9CE8 / D_801E9CEC, verts +0x2360 - plus, per prim, SetShadeTex(pPrim, 0) and
   the three shade stores r0 = 0, g0 = 0x80, b0 = 0 (the same shape as the matched
   MenuSetCharacterUnk63Digits above).
   Both quad loops read x0 and y0 twice each (0xAF8 / 0xAFA and 0xBE8 / 0xBEA): the
   volatile-minuend trick measured in the func_801D7884 park above is used here.
/*
extern s32 D_801E9CE0;
extern s32 D_801E9CE4;
extern s32 D_801E9CE8;
extern s32 D_801E9CEC;

void func_801D6CF4(u_char index, u_char row) {
    s32 characterOffset;
    s32 rowOffset;
    s32 i;

    characterOffset = g_Menu->pManager->currentCharacterIDs[index] * sizeof(GameCharacter);
    rowOffset = row * 24;
    MenuParseNumberToDigits(*(u8*)((u8*)&g_GameState + 0x2CE + characterOffset));
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE1 = 0;
    for (i = 0; i < 3; i++) {
        u8 digit = g_Menu->digits[6 + i];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE1 += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE1 *
                                (2 * sizeof(POLY_FT4)) +
                            0xAF0 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, i * MENU_DIGIT_SPACING + D_801E9CE0 - rowOffset, D_801E9CE4,
                0x1000);
        }
    }
    vertOffset = 0x2300;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE1; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (g_Menu->renderContext + i * 2) * sizeof(POLY_FT4) + 0xAF0);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }

    MenuParseNumberToDigits(*(u8*)((u8*)&g_GameState + 0x2CF + characterOffset));
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE2 = 0;
    for (i = 0; i < 3; i++) {
        u8 digit = g_Menu->digits[6 + i];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE2 += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE2 *
                                (2 * sizeof(POLY_FT4)) +
                            0xBE0 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, i * MENU_DIGIT_SPACING + D_801E9CE8 - rowOffset, D_801E9CEC,
                0x1000);
        }
    }
    vertOffset = 0x2360;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE2; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (g_Menu->renderContext + i * 2) * sizeof(POLY_FT4) + 0xBE0);

        SetShadeTex(pPrim, 0);
        pPrim->r0 = 0;
        pPrim->g0 = 0x80;
        pPrim->b0 = 0;
        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
*/
extern s32 D_801E9D10;
extern s32 D_801E9D14;
extern s32 D_801E9D18;
extern s32 D_801E9D1C;
extern s32 D_801E9D20;
extern s32 D_801E9D24;
extern s32 D_801E9D28;
extern s32 D_801E9D2C;
extern s32 D_801E9D30;
extern s32 D_801E9D34;
// The gear window's two stat numbers (unk44, unk48 of the character): each is
// drawn as up to eight digits, then every prim it produced is grown to its glyph box.
void MenuSetGearWindowWideNumberPrims(u_char index, u_char characterId) {
    u8* pPrims;
    u8* pBase;
    u8* pEntry;
    s32 offset;
    s32 vertOffset;
    s32 i;

    MenuParseNumberToDigits(
        ((s32*)g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk34)[2]);
    offset = characterId * 24;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumberPrimCount = 0;
    for (i = 0; i < MENU_MAX_DIGITS - 1; i++) {
        if (g_Menu->digits[1 + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumberPrimCount +=
                func_8002675C(
                    g_Menu->resources, g_Menu->digits[1 + i],
                    (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)
                                         ->firstWideNumberPrimCount *
                                     (2 * sizeof(POLY_FT4)) +
                                 0xCD0)),
                    g_Menu->renderContext, D_801E9D10 + i * MENU_DIGIT_SPACING - offset,
                    D_801E9D14, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumberPrimCount;
         i++) {
        POLY_FT4* pPrim;
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        pBase = (u8*)g_Menu->pGearWindowPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0xCD0);
        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x23C0 + i * 0x20),
                            pPrim->x0, pPrim->y0,
                            pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }

    MenuParseNumberToDigits(
        ((s32*)g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk34)[3]);
    offset = characterId * 24;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumberPrimCount = 0;
    for (i = 0; i < MENU_MAX_DIGITS - 1; i++) {
        if (g_Menu->digits[1 + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumberPrimCount +=
                func_8002675C(
                    g_Menu->resources, g_Menu->digits[1 + i],
                    (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)
                                         ->secondWideNumberPrimCount *
                                     (2 * sizeof(POLY_FT4)) +
                                 0xF50)),
                    g_Menu->renderContext, D_801E9D18 + i * MENU_DIGIT_SPACING - offset,
                    D_801E9D1C, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumberPrimCount;
         i++) {
        POLY_FT4* pPrim;
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        pPrims = (u8*)g_Menu->pGearWindowPrims;
        pBase = pPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0xF50);
        vertOffset = 0x24C0 + i * 0x20;
        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)pPrims), pPrim->x0, pPrim->y0,
                            pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7_r2", func_801D74EC);
/* Best: 99.83% (920 bytes, retail's exact size). Both halves decode completely and
   the ONLY remaining difference is two `addu` operand orders, one per half:
   retail `addu a2,a2,t1` (prim address) where gcc gives `addu a2,t1,a2`, and
   retail `addu a0,s1,a0` (verts pointer, offset biv + base) where gcc gives
   `addu a0,a0,s1`. Measured: spelling either sum offset-first
   (`(off) + (u32)base`, or the flat `field * 0x50 + K + (u32)base`) gives
   retail's operand order but rotates t0/t1 and drops to 94.43%; an explicit
   `vertOffset` variable (the shape that matches in func_801D7884) is 98.00 and
   `(u8*)base + i * 0x20 + K` flat is 93.17 (928 bytes). A 2-minute permuter run
   (885 iters, base=40) found nothing. Everything else - the twin lhu (via the
   volatile minuend), `(i * 2 + renderContext) * 0x28` (fixes the prim base
   register) and `i * 8 + D_x - offset` - is exact.
void func_801D74EC(u_char index, u_char characterId) {
    s32 offset;
    s32 i;

    MenuParseNumberToDigits(
        ((s32*)g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk34)[2]);
    offset = characterId * 0x18;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE5 = 0;
    for (i = 0; i < 7; i++) {
        if (g_Menu->digits[2 + i] != 0xFF) {
            s32 primCount = ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE5;

            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE5 += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + (primCount * 0x50 + 0x11D0)),
                g_Menu->renderContext, i * 8 + D_801E9D20 - offset, D_801E9D24, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE5; i++) {
        u8* pEntry = (u8*)g_Menu->pGearWindowPrims + (i * 2 + g_Menu->renderContext) * 0x28;

        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + (i * 0x20 + 0x25C0)),
                            *(u16*)(pEntry + 0x11D8), *(u16*)(pEntry + 0x11DA),
                            *(u16*)(pEntry + 0x11E0) - *(volatile u16*)(pEntry + 0x11D8),
                            *(u16*)(pEntry + 0x11F2) - *(volatile u16*)(pEntry + 0x11DA));
    }

    MenuParseNumberToDigits(
        g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk48);
    offset = characterId * 0x18;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE6 = 0;
    for (i = 0; i < 7; i++) {
        if (g_Menu->digits[2 + i] != 0xFF) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE6 += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE6 * 0x50 + 0x1400)),
                g_Menu->renderContext, i * 8 + D_801E9D28 - offset, D_801E9D2C, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE6; i++) {
        u8* pEntry = (u8*)g_Menu->pGearWindowPrims + (i * 2 + g_Menu->renderContext) * 0x28;

        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + (i * 0x20 + 0x26A0)),
                            *(u16*)(pEntry + 0x1408), *(u16*)(pEntry + 0x140A),
                            *(u16*)(pEntry + 0x1410) - *(volatile u16*)(pEntry + 0x1408),
                            *(u16*)(pEntry + 0x1422) - *(volatile u16*)(pEntry + 0x140A));
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7_r2", func_801D7884);
/* Best: 92.68% at retail's exact 952 bytes. Structure, every call, every branch,
   the frame (0x58, s0-s7) and the quad loop are right; what is left is one
   allocator tie in the head: retail ranks `tensDigit` above `row` (s4 = tens,
   s7 = row) and gcc ranks it below (s7 = tens, s6 = row), which rotates the
   whole s4-s7 chain (row*8, row*24) and puts the /100 magic constant in a0
   instead of the free a1, so it cannot hoist to the block start as retail does.
   From global.c's allocno_compare: tens = 100*2refs/90live = 2.2 and
   row = 100*4refs/151live = 2.6, a floor_log2 tie broken by pseudo order.
   Tried: declaring tensDigit first (no change at all), inlining `remainder`
   (90.87%, 944 bytes - the local is required), `(i * 2 + ctx)` operand order
   (92.09 -> 92.68, keep it). The permuter rejects this base
   ("conversion to non-scalar type requested").
void func_801D7884(u_char index, u_char row) {
    u16 tensDigit;
    s32 characterOffset;
    u32 scaled;
    u16 hundreds;
    s32 remainder;
    s32 count;
    s32 digitOffset;
    s32 x;
    s32 i;

    // The character's stat row: characters[] is 0xA4 bytes and statBase/statBonus/unk79 sit 0x2E3/0x2E4/0x2E5 into g_GameState.
    // Retail folds that 0x2E4 into each load's %lo half, so the row is reached through
    // g_GameState (a pointer local would burn a register on the base).
    if (row == 0) {
        characterOffset = g_Menu->pManager->currentCharacterIDs[index] * sizeof(GameCharacter);
        scaled = (u16)(((*(u8*)((u8*)&g_GameState + 0x2E4 + characterOffset) +
                         *(u8*)((u8*)&g_GameState + 0x2E3 + characterOffset)) *
                            10 +
                        *(u8*)((u8*)&g_GameState + 0x2E5 + characterOffset)) *
                       22);
        hundreds = scaled / 100;
        remainder = scaled - hundreds * 100;
        count = 3;
        digitOffset = 6;
        tensDigit = remainder / 10;
        MenuParseNumberToDigits(hundreds);
        x = D_801E9D30;
    } else {
        characterOffset = g_Menu->pManager->currentCharacterIDs[index] * sizeof(GameCharacter);
        MenuParseNumberToDigits(g_GameState.gears[*(u8*)((u8*)&g_GameState + 0x30C + characterOffset)].weight);
        count = 5;
        digitOffset = 4;
        x = D_801E9D30 + 8;
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount = 0;
    for (i = 0; i < count; ) {
        u8 digit = g_Menu->digits[digitOffset + i++];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount *
                                (2 * sizeof(POLY_FT4)) +
                            0x1C70 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, x - row * 24, row * 8 + D_801E9D34, 0x1000);
        }
        x += MENU_DIGIT_SPACING;
    }
    if (row == 0) {
        ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount += func_8002675C(
            g_Menu->resources, tensDigit,
            (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount *
                            (2 * sizeof(POLY_FT4)) +
                        0x1C70 + (u32)g_Menu->pGearWindowPrims),
            g_Menu->renderContext, D_801E9D30 + 0x20, D_801E9D34, 0x1000);
    }
    vertOffset = 0x2A00;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0x1C70);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
*/
void MenuSetGearWindowContents(u_char index, u_char characterId) {
    func_801D5ED4(index, characterId);
    MenuSetCharacterIconPrims(characterId);
    func_801D6338(index, characterId);
    func_801D680C(index, characterId);
    func_801D6CF4(index, characterId);
    func_801D74EC(index, characterId);
    func_801D7884(index, characterId);
    MenuSetGearWindowWideNumberPrims(index, characterId);
    g_Menu->pManager->shouldRenderUnk358Prims = 1;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->renderContext = *(u8*)&g_Menu->renderContext;
}
