#include "common.h"

#include "system/menu.h"
#include "main/game.h"

// Limits MenuComputeCharacterStats puts on the dressing room's stat values.
#define MENU_STAT_MAX 250
#define MENU_STAT_PERCENT_MAX 99 // hit and evade
#define MENU_STAT_AGILITY_MAX 20
#define MENU_STAT_AGILITY_CAPPED 16 // what agility above the maximum is set to

extern u8 g_MenuUnk17E0StringIds[];
extern u8 g_MenuUnk18E0StringIdSets[];
extern u8 g_GearPilotIds[];
extern u8 D_801E9811;
extern u8 g_MenuPanelRowXOffsets[];
extern u8 D_801EA56E[];
extern u32 g_MenuGearStringIdTable[];
extern u8 g_FieldPlayerWalkmeshMaterial;
extern u8 g_MenuRowDigitTextureIds[];

void MenuLoadStringsFromIds(u8, void*, u8*, void*);
u32 MenuAndBitMaskMsbFirst(u32, u8);
u8* GetStringEntryFromBank(s32, u8);
void MenuSetStringTexturePrims(MenuString*, u8, s32, s32);
void func_801E1544(u8, u8, u8*, u32);
void MenuApplyGearFrame(MenuDressingRoom*, u8);
void MenuApplyGearArmor(MenuDressingRoom*, u8);
void MenuApplyGearEngine(MenuDressingRoom*, u8);
void MenuApplyGearAccessories(MenuDressingRoom*, u8);
void MenuApplyGearWeapons(MenuDressingRoom*, u8);
void MenuLoadResourceSet(u8);
void MenuFreeShoulderButtonUi(void);
void MenuFreeWindow(u8);
void MenuRenderFrame(void);
void MenuClearBytes(u8, u8*);
void MenuLoadSubMenuChoiceStrings(u8);
void MenuCreateShoulderButtonUi(u8, u8);
void func_801E05D0(u8, u8, u8);
void MenuShutdown(void);
void MenuFreeUnk430Panel(u8);
void MenuFreeUnk42CPanel(void);
void MenuFreeCharacterSpriteRecords(void);
void MenuFreeUnk35CPanel(void);
void MenuClearSubMenuChoiceFlags(void);
void MenuPlaySoundEffect(u8);
s32 func_8002675C(MenuResourceDirectory*, u32, POLY_FT4*, s32, s32, s32, s32);
void SystemDecodeGlyphCodesN(u16*, u8*, int);
s32 SystemRenderStringEntry(u8*, void*, s32, s32);
void MenuSetStringPrimitives(MenuString*, s32, s32, s32);
s32 func_801D9704(s32, u8, u8);
void MenuInitializeUnk438Panel(void);
void func_801E1AC8(u8);
void MenuSetRowQuadVertices(s32, void*, void*, void*, void*, u8, u8, u8);
void MenuCreateWindow(u_char, u_char, u_char, u_char, u16, u_char, u_char, u32, u_char);
void MenuApplyEquippedItemBonuses(MenuDressingRoom*, u8);
void MenuComputeCharacterStats(MenuDressingRoom*, u8);
void MenuSetSubMenuChoiceStringVertices(void);
void MenuTweenCharacterWindows(s32, s32);
void func_801D8EA4(u8, u8, u8, u8);
void func_801DDF24(u8, u8, u8);
void func_801E86C8(u8);
void MenuStartOpenAnimation(void);
void MenuResetSelectionCursorFlags(void);
void MenuMovePointerCursor(s32, u8);
void MenuUpdateDressingRoomTotals(u_char);
void MenuSetGearWindowQuads(s32, u8, u8);
void MenuDrawStatTable(u8, u8, u8, u8);
void MenuDrawGearScreen(u_char, u_char);
void MenuSetSubMenuChoicePrims(u8);

int MenuInitializeUnk35CPanel(u8 arg0, u8 arg1) {
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->pEquipmentPanel = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->pEquipmentPanel, 0x2AC);
    MenuLoadResourceSet(3);
    func_801E05D0(arg0, arg1, 0);
    MenuLoadResourceSet(0x13);
    return 1;
}
void MenuInitializeUnk438Panel(void) {
    MenuManager* pManager;
    int i;
    /* One variable for both the count and the inner POLY_G4 index below: it has
       to stay live across MenuCreateShoulderButtonUi so that it lands in a call-saved
       register, which is where retail keeps it. */
    int count;

    g_Menu->unk438 = HeapAlloc(0x25C0, 0);
    bzero(g_Menu->unk438, 0x25C0);
    MenuLoadResourceSet(4);
    MenuLoadStringsFromIds(2, g_Menu->unk17E0, g_MenuUnk17E0StringIds, g_Menu->pManager->shouldRenderStrings17E0);
    MenuSetRowQuadVertices(2, g_Menu->unk17E0, g_MenuUnk17E0StringIds, g_MenuPanelRowXOffsets, g_Menu->pManager->shouldRenderStrings17E0, 0, 0, 4);
    MenuCreateWindow(2, 0x44, 0xA, 0xE4, 0xC4, 0, 1, 4, 0);
    g_Menu->pManager->playTimeDigitsActive = 0;
    g_Menu->pManager->shouldRenderWindow[1] = 0;
    pManager = g_Menu->pManager;
    i = 0;
    count = 0;
    for (; i < 3; i++) {
        if (pManager->currentCharacterIDs[i] != 0xFF && (u8)(pManager->currentCharacterIDs[i] - 7) >= 2) {
            count++;
        }
    }
    if (count >= 2) {
        MenuCreateShoulderButtonUi(3, 0);
    } else {
        g_Menu->pManager->shoulderButtonUiActive = 0;
    }
    for (i = 0; i < 0xD; i++) {
        for (count = 0; count < 2; count++) {
            /* (i * 0x48 + 0x21D0) must stay grouped for retail's address build:
               retail increments the pre-scaled i and count indices and folds
               0x21D0 into the store displacement. Spelling this as
               ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[i][count] is
               91.63% (864 bytes), because gcc then forms (i * 2 + count) * 0x24. */
            SetPolyG4((POLY_G4*)((u8*)g_Menu->unk438 + (i * 0x48 + 0x21D0) + count * 0x24));
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->r0 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->g0 = 0xFF;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->b0 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->r1 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->g1 = 0xFF;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->b1 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->r2 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->g2 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->b2 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->r3 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->g3 = 0;
            ((POLY_G4*)((u8*)g_Menu->unk438 + i * 0x48 + count * 0x24 + 0x21D0))->b3 = 0;
        }
    }
}
void MenuFreeUnk438Panel(void) {
    MenuFreeShoulderButtonUi();
    g_Menu->pManager->shouldRenderPanel[3] = 0;
    MenuFreeWindow(2);
    MenuRenderFrame();
    MenuClearBytes(2, &g_Menu->pManager->shouldRenderPanel[4]);
    MenuLoadResourceSet(0x14);
    HeapFree(g_Menu->unk438);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E1418);
/* objdiff 81.47% (296 bytes = retail's size; the residual is one register rotation
   plus two scheduler/loop.c placements). Averages a character's 7 stat rows into a
   percentage: pct per row = weight * 100 / value, summed and divided by count.
   Settled: charId = g_Menu->pManager->currentCharacterIDs[slot]; weight = *pWeight++
   with pWeight = g_GameState.characters[charId].statWeights (0xA4 stride, the row
   table's stride is 0x110); value = pRow[i] with pRow = pTable + charId * 0x110 +
   index * 0xE and pTable = *(u16 **)((u8 *)g_Menu->unk438 + 0x2578); total/count
   are u32, the loop counter a signed int (retail ends the loop with slti); the
   weight != 0 test comes first in the source (retail branches over it into the
   else-if value != 0 block) and writing `if (weight == 0)` first costs 7 points.

   Round 378, each measured with diff_func from the body below:
   - ARM ORDER: `if (pct >= 100) total += 100; else total += pct;` (a permuter find)
     is 81.47 where `if (pct < 100) total += pct; ...` is 75.47 - retail branches
     INTO the += pct arm (`bnez v0,.L1508`).
   - `i = 0;` as its own statement before `count = 0;` with an empty for-init
     (`for (; i < 7; i++)`) reproduces retail's `move t1,zero` / `move a2,zero` order.
   - NOT the fix: splitting the row base into two statements (pRow = pTable +
     charId*0x110; pRow = pRow + index*0xE) measures 81.60 with MORE differing
     instructions, so objdiff's number is not monotone here; the flat sum, the
     ternary and an inline base are all 66-75; the hoisted pRow is the best shape.
   - Other shapes measured identical (99.6 of the instructions are in place, so
     every one of these re-rolls the register file): moving the count test, the
     do-while wrapper an earlier permuter found, reordering the declarations.
   Remaining three residuals: (1) gcc materialises g_Menu in $a1 and so copies the
   `index` param to $t2 at entry, while retail copies `slot` to $v1 (`move v1,a0`)
   because it loads g_Menu into $a0; the whole file then rotates (ours total $a3 /
   pWeight $t0 / weight $a1, retail total $t0 / pWeight $t2 / weight $v1);
   (2) ours hoists `li 0xffff` out of the loop into $t3, retail materialises it
   inside the loop before `beq a0,v0`; (3) ours bumps pWeight (`addiu t0,t0,2`)
   right after the weight load, retail in the loop tail's bnez delay slot.
   Permuter (3 runs from this body, base=1538 -> 1353) proposes reusing the dead
   `value` local for charId (`value = pManager->currentCharacterIDs[slot]; pWeight =
   g_GameState.characters[value].statWeights;`); not applied here because it reads as
   a different value - measure it first.
*/
/*
u32 func_801E1418(u8 slot, u8 index) {
    u8 charId;
    u16* pWeight;
    u16* pRow;
    u16* pTable;
    u32 weight;
    u32 value;
    u32 total;
    u32 count;
    int i;
    u32 pct;

    total = 0;
    if (index >= 7 && (g_GameState.unk22B6 & 0x4000) == 0) {
        return 0;
    }
    i = 0;
    count = 0;
    charId = g_Menu->pManager->currentCharacterIDs[slot];
    pWeight = g_GameState.characters[charId].statWeights;
    pTable = *(u16**)((u8*)g_Menu->unk438 + 0x2578);
    pRow = (u16*)((u8*)pTable + charId * 0x110 + index * 0xE);
    for (; i < 7; i++) {
        weight = *pWeight++;
        value = pRow[i];
        if (weight != 0) {
            if (value != 0) {
                if (value == 0xFFFF) {
                    count++;
                } else {
                    pct = weight * 100 / value;
                    if (pct >= 100) {
                        total += 100;
                    } else {
                        total += pct;
                    }
                    count++;
                }
            }
        } else if (value != 0) {
            count++;
        }
    }
    if (count == 0) {
        return 0;
    }
    return total / count;
}
*/

// The 0x25C0-byte block g_Menu->unk438 points at, as far as this unit reaches:
// one record per dressing-room row (13 of them), each with the MenuString for its
// number and the digit prims the number is built from, then the per-row state
// bytes and the stat-table pointer MenuComputeCharacterStats reads.
#define MENU_DRESSING_ROOM_ROW_COUNT 13

// One equipment slot record of pDressingRoom->pCharacterSlots[], as far as this
// unit reaches (misc9 walks the same records at a 0x28 stride).
typedef struct {
    /* 0x00 */ u8 unk0[0x11];
    /* 0x11 */ u8 unk11; // multiplies the wearer's ether into the slot's hp bonus
    /* 0x12 */ u8 unk12[0x16];
} MenuEquipmentSlot; // Size: 0x28

typedef struct {
    /* 0x0000 */ MenuString slotStrings[MENU_DRESSING_ROOM_ROW_COUNT]; // polys[]: the row's leading character, one per render context
    /* 0x0680 */ MenuString numberStrings[MENU_DRESSING_ROOM_ROW_COUNT]; // polys[]: the number's quad, one per render context
    /* 0x0D00 */ MenuString unk0D00;
    /* 0x0D80 */ POLY_FT4 digitPrims[MENU_DRESSING_ROOM_ROW_COUNT][5][2]; // per digit: one prim per render context
    /* 0x21D0 */ POLY_G4 bars[MENU_DRESSING_ROOM_ROW_COUNT][2];
    /* 0x2578 */ u16* pStatTable; // stat values per character and row, see func_801E1418
    /* 0x257C */ u8 digitPrimCount[MENU_DRESSING_ROOM_ROW_COUNT]; // digit prims used so far in digitPrims[]
    /* 0x2589 */ u8 rowRenderContext[MENU_DRESSING_ROOM_ROW_COUNT]; // render context the row's digit prims were built with
    /* 0x2596 */ u8 shouldRender[MENU_DRESSING_ROOM_ROW_COUNT];
    /* 0x25A3 */ u8 hasBar[MENU_DRESSING_ROOM_ROW_COUNT];
    /* 0x25B0 */ u8 barPrimIndex[MENU_DRESSING_ROOM_ROW_COUNT]; // which of the row's bars to add
    /* 0x25BD */ u8 unk25BD[3];
} MenuDressingRoomRowBlock; // Size: 0x25C0

INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E1544);
/* Best: 80.32% (1424 bytes; retail is 1412, so 3 instructions long). Structure is
   decoded and every call, both switch arms and all 16 prim stores are in retail's
   order. Sets up one dressing-room row: mode 1 appends the row's digit quads
   straight from the texture atlas (g_MenuRowDigitTextureIds is a u8 [13][5] of texture ids with
   0xFF = skip, one quad per digit at x = 0xD4 + i*0x10), mode 2 renders `value` as
   three glyph codes into the row's 0x28 x 0xD texture cell, uploads it, sets the
   number MenuString's quad and the value's POLY_G4 bar, then flags the row.
   Facts measured this round, all worth keeping:
   - the dispatch is a `switch (mode)`, NOT if/else-if: only the switch gives
     retail's `beq a1,1 / li v0,2 / beq a1,v0 / lui v0,magic / j epilogue / nop`
     (an if/else-if chain inlines the second test after the first arm).
   - the digit code buffer is `u8 digitCodes[8]` written at 0,2,4 with the high
     bytes zeroed FIRST and in the order 5,3,1 (gcc emits the three sb in reverse
     source order; the last one lands in the mode test's delay slot).
   - `if (tens != 0 || digitCodes[0] != 0)` is literally what retail tests (it
     reloads the hundreds byte and compares to zero), even though 0xC3 is also
     nonzero, so the all-blank arm is unreachable. Do not "fix" it.
   - the bar width is `0xD4 + (5400 * value) / 10000`; `value * 54 / 100` folds to
     a different multiply chain (measured with cc1_probe).
   - the block g_Menu->unk438 points at is 0x25C0 bytes: MenuString[13] at 0x680
     (one per row), one MenuString at 0xD00, POLY_FT4[13][5][2] at 0xD80 (two prims
     per digit, one per render context), POLY_G4[13][2] at 0x21D0, the stat table
     pointer at 0x2578, then five parallel u8[13] state arrays at 0x257C/0x2589/
     0x2596/0x25A3/0x25B0. MenuDressingRoomRowBlock below is that layout.
   - the row state bytes ARE a struct access: as `[slot]` on the struct the three
     byte stores come out right (base + slot, one addu, g_Menu reloaded per
     statement because the intervening stores may alias it). A pointer local for
     g_Menu->unk438 makes gcc hoist and every store shrinks.
   Residue (3 instructions and 16 store addresses): retail builds each prim-store
   address as ((base + slot*0x80) + renderContext*0x28) with the 0x680 folded into
   the store displacement, i.e. base-first; gcc builds (renderContext*0x28 +
   slot*0x80) + base. The two char* spellings that put base first -
   `(u8*)unk438 + slot*0x80 + 0x680` and the offsetof form - both make gcc
   materialise 1664 in a register instead of folding it (1424 -> 1448 bytes), and
   a `s32 rowOffset` local reproduces the add order in a cc1_probe snippet but not
   in the real body. Also left: `li s2,0xD4` (the loop's x initialiser) is emitted
   at the top of the mode-1 arm here but after the slot*13 chain in retail, and the
   `lui t1,(0xD1B71759>>16)` / `ori` pair is split around 17 instructions in retail
   (retail computes the bar width between the 6th and 7th FT4 stores) while this
   body emits the pair adjacent at the multiply. Tried and no better: if/else-if for
   the mode test (76.28%), the two char* store spellings (80.85%/1448 and
   71.72%/1428), and gcc-2.6.0 is the unit's preset already.
void func_801E1544(u8 slot, u8 mode, u8* pWorkspace, u32 value) {
    u8 digitCodes[8];
    u8 text[8];
    RECT rect;
    u8 texId;
    s32 i;
    u32 tens;
    s32 x;

    digitCodes[5] = 0;
    digitCodes[3] = 0;
    digitCodes[1] = 0;
    switch (mode) {
    case 1:
        for (i = 0, x = 0xD4; i < 5; ) {
            texId = g_MenuRowDigitTextureIds[slot * 5 + i++];
            if (texId != 0xFF) {
                ((MenuDressingRoomRowBlock*)g_Menu->unk438)->digitPrimCount[slot] +=
                    func_8002675C(g_Menu->resources, texId,
                        &((MenuDressingRoomRowBlock*)g_Menu->unk438)
                             ->digitPrims[slot][((MenuDressingRoomRowBlock*)g_Menu->unk438)->digitPrimCount[slot]][0],
                        g_Menu->renderContext, x, slot * FONT_LETTER_HEIGHT + 0x1F, 0x1000);
            }
            x += 0x10;
        }
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->rowRenderContext[slot] = *(u8*)&g_Menu->renderContext;
        break;
    case 2:
        if (value / 100 != 0) {
            digitCodes[0] = 0x11;
            value -= 100;
        } else {
            digitCodes[0] = 0xC3;
        }
        tens = value / 10;
        if (tens != 0 || digitCodes[0] != 0) {
            digitCodes[2] = tens + 0x10;
            digitCodes[4] = value % 10 + 0x10;
        } else {
            digitCodes[2] = 0xC3;
            digitCodes[4] = 0xC3;
        }
        SystemDecodeGlyphCodesN((u16*)digitCodes, text, 3);
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].width =
            SystemRenderStringEntry(text, pWorkspace, 0x24, 1);
        rect.x = 0x180 + (slot & 1) * 0x18;
        rect.y = 0x80 + (slot / 2) * FONT_LETTER_HEIGHT;
        rect.w = 0x28;
        rect.h = FONT_LETTER_HEIGHT;
        LoadImage(&rect, (u_long*)pWorkspace);
        DrawSync(0);
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].polys[g_Menu->renderContext].x0 = 0x10A;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].polys[g_Menu->renderContext].y0 = slot * FONT_LETTER_HEIGHT + 0x1F;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].polys[g_Menu->renderContext].x1 =
            0x10A + ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].width;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].polys[g_Menu->renderContext].y1 = slot * FONT_LETTER_HEIGHT + 0x1F;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].polys[g_Menu->renderContext].x2 = 0x10A;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].polys[g_Menu->renderContext].y2 = slot * FONT_LETTER_HEIGHT + 0x2C;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].polys[g_Menu->renderContext].x3 =
            0x10A + ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].width;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot].polys[g_Menu->renderContext].y3 = slot * FONT_LETTER_HEIGHT + 0x2C;
        MenuSetStringPrimitives(&((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[slot], slot, 0x80, 0x82);
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[slot][g_Menu->renderContext].x0 = 0xD4;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[slot][g_Menu->renderContext].y0 = slot * FONT_LETTER_HEIGHT + 0x23;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[slot][g_Menu->renderContext].x1 = 0xD4 + (5400 * value) / 10000;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[slot][g_Menu->renderContext].y1 = slot * FONT_LETTER_HEIGHT + 0x23;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[slot][g_Menu->renderContext].x2 = 0xD4;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[slot][g_Menu->renderContext].y2 = slot * FONT_LETTER_HEIGHT + 0x2B;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[slot][g_Menu->renderContext].x3 = 0xD4 + (5400 * value) / 10000;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->bars[slot][g_Menu->renderContext].y3 = slot * FONT_LETTER_HEIGHT + 0x2B;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->barPrimIndex[slot] = *(u8*)&g_Menu->renderContext;
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->hasBar[slot] = 1;
        break;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E1AC8);
/* Best: 87.09% at retail's EXACT size (1536 bytes). Structure is decoded and every
   call, both quads, the texture upload and the row-mode handoff are in retail's
   order; what is left is one register's worth of allocation.
   Facts worth keeping:
   - `g_Menu->pManager->currentCharacterIDs[slot]` is spelled INLINE at all three
     sites (the condition-mask test, GetStringEntryFromBank, and the 0x12F read):
     retail re-derives it each time, and a `charId` local costs 5 points
     (82.33 -> 87.09) and four bytes.
   - the row block's 0x680 area is MenuString[13] (slotStrings), not opaque bytes:
     width at +0x7E and renderContext at +0x7D are MenuString fields, and the two
     16-store quad groups are slotStrings[i].polys[g_Menu->renderContext] and
     numberStrings[i].polys[g_Menu->renderContext] (+0x680), each store re-spelling
     the whole access so gcc re-derives the address as retail does.
   - the frame is 16 bytes too big because `percent` spills to 0x30 where retail
     keeps it in $fp; retail's flag variable lives in $v0 (short-lived) where gcc
     here gives it a callee-saved register, and retail also carries a separate
     s1 = i*0x80 induction register. Tried with no change: the three assignments in
     the mask arm in a different order (87.72 at 1540 bytes), and `percent`'s
     declaration position.
*/
/*
// Draws the 13 dressing-room rows of one party slot: renders the row's leading
// character and its number into the block's texture cells, sets both quads, and
// hands the row's mode to func_801E1544.
void func_801E1AC8(u8 slot) {
    u8 code[2];
    u8 text[8];
    RECT rect;
    u8* pWorkspace;
    u8 found;
    u8 mode;
    u8 draw;
    u16 hit;
    s32 top;
    u32 percent;
    s32 bottom;
    s32 i;

    pWorkspace = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);
    code[1] = 0;
    i = 0;
    top = 0x1F;
    bottom = 0x2C;
    for (; i < MENU_DRESSING_ROOM_ROW_COUNT; i++) {
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->hasBar[i] = 0;
        hit = MenuAndBitMaskMsbFirst(
            g_GameState.conditionRows[g_Menu->pManager->currentCharacterIDs[slot]].commandMask, i);
        found = 0;
        draw = 0;
        if (hit != 0) {
            mode = 1;
            found = 1;
            draw = 1;
        } else {
            percent = func_801E1418(slot, i);
            if (percent < 50) {
                mode = 0;
            } else {
                mode = 2;
                draw = 1;
            }
        }
        if (draw != 0) {
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].width =
                SystemRenderStringEntry(GetStringEntryFromBank(g_Menu->pManager->currentCharacterIDs[slot], i),
                    pWorkspace, 0x24, 0);
            if (i < 7 && found != 0) {
                code[0] = ((u8*)g_Menu->pDressingRoom->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[slot]])
                              [i * sizeof(MenuEquipmentSlot) + 0x12F] +
                    0x10;
            } else {
                code[0] = 0xF;
            }
            SystemDecodeGlyphCodesN((u16*)code, text, 1);
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].width =
                SystemRenderStringEntry(text, pWorkspace, 0x24, 1);
            rect.x = 0x180 + (i & 1) * 0x18;
            rect.y = 0x80 + (i / 2) * FONT_LETTER_HEIGHT;
            rect.w = 0x28;
            rect.h = FONT_LETTER_HEIGHT;
            LoadImage(&rect, (u_long*)pWorkspace);
            DrawSync(0);
            MenuSetStringPrimitives(&((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i], i, 0x80, 0x81);
            MenuSetStringPrimitives(&((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i], i, 0x80, 0x82);
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].polys[g_Menu->renderContext].x0 = 0x5C;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].polys[g_Menu->renderContext].y0 = top;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].polys[g_Menu->renderContext].x1 =
                0x5C + ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].width;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].polys[g_Menu->renderContext].y1 = top;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].polys[g_Menu->renderContext].x2 = 0x5C;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].polys[g_Menu->renderContext].y2 = bottom;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].polys[g_Menu->renderContext].x3 =
                0x5C + ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].width;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].polys[g_Menu->renderContext].y3 = bottom;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].polys[g_Menu->renderContext].x0 = 0xC8;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].polys[g_Menu->renderContext].y0 = top;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].polys[g_Menu->renderContext].x1 =
                0xC8 + ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].width;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].polys[g_Menu->renderContext].y1 = top;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].polys[g_Menu->renderContext].x2 = 0xC8;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].polys[g_Menu->renderContext].y2 = bottom;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].polys[g_Menu->renderContext].x3 =
                0xC8 + ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].width;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->numberStrings[i].polys[g_Menu->renderContext].y3 = bottom;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->slotStrings[i].renderContext = *(u8*)&g_Menu->renderContext;
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->shouldRender[i] = 1;
        } else {
            ((MenuDressingRoomRowBlock*)g_Menu->unk438)->shouldRender[i] = 0;
        }
        ((MenuDressingRoomRowBlock*)g_Menu->unk438)->digitPrimCount[i] = 0;
        func_801E1544(i, mode, pWorkspace, percent);
        top += FONT_LETTER_HEIGHT;
        bottom += FONT_LETTER_HEIGHT;
    }
    HeapFree(pWorkspace);
    MenuSetStringTexturePrims(&((MenuDressingRoomRowBlock*)g_Menu->unk438)->unk0D00, slot, 0, 2);
    g_Menu->pManager->shouldRenderPanel[3] = 1;
}
*/

u8 MenuRunUnk438PanelLoop(u8 slot) {
    u8 active;
    u8 lastSlot;

    active = 1;
    lastSlot = 0xFF;
    if ((u8)(g_Menu->pManager->currentCharacterIDs[slot] - 7) < 2) {
        MenuPlaySoundEffect(4);
        return 1;
    }
    MenuInitializeUnk438Panel();
    do {
        MenuRenderFrame();
        if (slot != lastSlot) {
            func_801E1AC8(slot);
            lastSlot = slot;
        }
        switch (g_Menu->input) {
        case 5:
            active = 0;
            break;
        case 9:
            do {
                slot = func_801D9704(slot, 0, 0);
            } while ((u8)(g_Menu->pManager->currentCharacterIDs[slot] - 7) < 2);
            break;
        case 10:
            do {
                slot = func_801D9704(slot, 1, 0);
            } while ((u8)(g_Menu->pManager->currentCharacterIDs[slot] - 7) < 2);
            break;
        }
    } while (active != 0);
    MenuFreeUnk438Panel();
    return 1;
}

u8 MenuInitializeGearScreenBuffers(void) {
    u32 i;

    g_Menu->pGearWindowPrims = HeapAlloc(0x2AF0, 0);
    bzero(g_Menu->pGearWindowPrims, 0x2AF0);
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->pEquipmentPanel = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->pEquipmentPanel, 0x2AC);
    MenuLoadResourceSet(3);
    i = 0;
    while (1) {
        if (g_Menu->pManager->isGearScreenActive[i] != 0) {
            break;
        }
        i++;
    }
    MenuCreateShoulderButtonUi(0, 1);
    return i;
}
void MenuLoadUnk18E0Strings(u8 arg0) {
    MenuLoadStringsFromIds(6, g_Menu->unk18E0, g_MenuUnk18E0StringIdSets + arg0, g_Menu->pManager->shouldRenderStrings18E0);
}
void MenuFreeGearScreenBuffers(void) {
    HeapFree(g_Menu->pGearWindowPrims);
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->pEquipmentPanel);
    MenuLoadResourceSet(0x13);
}
/* PARKED 99.66% - 1812 bytes, retail's exact size; 453 instructions, the same
   instruction multiset. Re-measured round 457: still 99.66% / 1812 bytes. The
   ONLY residual is the callee-saved mirror - retail keeps `s32 slot` in $s2 and
   `u8 lastSlot` in $s1 (andi $s2,$v0,0xFF after
   MenuInitializeGearScreenBuffers; ori $s1,$zero,0xF3 for lastSlot's init) and
   this body gets the exact mirror
   (slot $s1, lastSlot $s2). Every one of slot's and lastSlot's uses swaps; case
   order, the jump table, the delay slots, frame 0x48 and the whole save set
   agree. jtbl_801C518C vs .rodata is this unit's own rodata and is expected in
   diff_func.

   WHY NO SPELLING MOVES IT (see tools/rounds/levers.md,
   a-global-allocno-priority-is): gcc-2.6.0 global.c sorts allocnos by
   floor_log2(n_refs) * n_refs * 10000 * allocno_mode_size / live_length and
   hands the callee-saved registers out in ascending order along that list, so a
   mirrored pair is one whose two allocnos came out in the wrong order. Here the
   two are ~10x apart (slot SI, 22 refs -> 4*22; lastSlot QI, 11 refs -> 3*11,
   times size 4 vs 1, same live length), i.e. slot always wins. The ref count is
   fixed by the emitted code once cse has run, the mode size is fixed by retail's
   own andis, and the live length is the loop in both cases, so no C spelling of
   these 453 instructions can reverse the pair - and a spelling that changes the
   instructions cannot link.

   Round 457 read the quotients out of `-dl` instead of trusting the estimate
   above: slot is pseudo 75, "used 40 times across 290 insns" SI ->
   5*40*10000*4/290 = 2.76; lastSlot is pseudo 72, "used 15 times across 224
   insns; 1 bytes" -> 3*15*10000*1/224 = 0.20, a 13.7x gap. lastSlot would have
   to be live for at most 16 insns (it is live across the render call and the
   whole switch) and slot for 3982 (it is 290), so no spelling of these 453
   instructions reverses them - the same verdict as `a-global-allocno-priority-is`
   and `a-lreg-dump-gives-the`, now with the numbers in the note. That is why
   round 457 spent no fourth permuter run (three already returned base=145
   best=145 over 1304-2400 iterations each).

   The function takes NO parameters: its only caller (misc1 801C53D0) does jal
   with a nop delay slot and no $a0 setup, so slot is a LOCAL and retail reads it
   uninitialised at the entry call (addu $a0,$s2,$zero before anything writes
   $s2); as a parameter the body is 1808 bytes. Keep active = 1 before the entry
   call (retail's ori $s6,$zero,0x1 before the a0 copy) - that is what fixes the
   prologue save order (98.69 -> 99.66). MenuInitializeGearScreenBuffers returns
   u8 (retail's andi $s2,$v0,0xFF) and slot must stay s32 or cases 9/10 gain an
   andi. lastSlot must stay u8: retail narrows it twice (andi v0,s1,0xFF at the
   loop test, andi s0,s1,0xFF reused by MenuUpdateDressingRoomTotals and
   MenuDrawGearScreen); s32 lastSlot with explicit (u8) casts keeps those andis
   but measures 99.12% at 1804 bytes.

   Tried, one line each, no new best: every local declaration order (the tie is
   not sensitive to pseudo numbering); slot or lastSlot given the other width;
   s32 lastSlot with casts (99.12 / 1804); gcc-2.7.2-cdk (89.55), 2.7.2-psx
   (lower), gcc-2.6.0 -O3 (86.36), -G0 (does not compile); permuter 3 runs from
   the 98.69/98.48 bodies and this one (PERMUTER_NONE 543, 145 and, round 431,
   base 145 best 145 over 1304 iterations). Converting the switch owns
   jtbl_801C518C (the unit's first table), so this is not JTBL_LAYOUT.
*/
/*
u8 func_801E23CC(void) {
    u8 active;
    u8 lastSlot;
    u8 firstDraw;
    u8 stringSet;
    s32 slot;

    active = 1;
    slot = func_801D9704(slot, 0, 1);
    lastSlot = 0xF3;
    firstDraw = 1;
    g_Menu->subMenuChoice = 0;
    g_Menu->subMenuPrevChoice = 0xFF;
    slot = MenuInitializeGearScreenBuffers();
    while (active != 0) {
        MenuRenderFrame();
        if (slot != lastSlot) {
            lastSlot = slot;
            MenuUpdateDressingRoomTotals(lastSlot);
            MenuDrawGearScreen(lastSlot, 1);
            stringSet = (g_GameState.gearRide[slot] != 0) ? 6 : 0;
            MenuLoadUnk18E0Strings(stringSet);
            MenuSetRowQuadVertices(6, (u8*)g_Menu + 0x18E0, D_801EA56E, g_MenuGearStringIdTable, (u8*)g_Menu->pManager + 0x54, 4, 7, 6);
            MenuSetRowQuadVertices(6, (u8*)g_Menu + 0x18E0, D_801EA56E, g_MenuGearStringIdTable, (u8*)g_Menu->pManager + 0x54, 5, 7, 6);
            g_Menu->unk18E0[4].hasVertices = 1;
            g_Menu->unk18E0[5].hasVertices = 1;
            if (firstDraw != 0) {
                MenuStartOpenAnimation();
                firstDraw = 0;
                MenuTweenCharacterWindows(0, 0);
                func_801E86C8(0);
            }
        }
        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            MenuSetRowQuadVertices(4, (u8*)g_Menu + 0x18E0, D_801EA56E, g_MenuGearStringIdTable + stringSet, (u8*)g_Menu->pManager + 0x54, g_Menu->subMenuChoice, 7, 0);
            MenuSetSubMenuChoicePrims(0);
            g_Menu->subMenuPrevChoice = g_Menu->subMenuChoice;
        }
        switch (g_Menu->input) {
        case 4:
            MenuResetSelectionCursorFlags();
            MenuClearBytes(4, (u8*)g_Menu->pManager + 0x54);
            g_Menu->pManager->shouldRenderUnk358Prims = 0;
            g_Menu->pManager->shouldRenderUnk35C = 0;
            g_Menu->pManager->shouldRenderPanel[1] = 0;
            g_Menu->pManager->shouldRenderStrings18E0[4] = 0;
            g_Menu->pManager->shouldRenderStrings18E0[5] = 0;
            switch (g_Menu->subMenuChoice) {
            case 0:
                if (g_Menu->pManager->currentCharacterIDs[slot] != 7) {
                    g_Menu->pManager->shouldRenderSubmenuStrings = 0;
                    func_801E05D0(slot, 0, 1);
                    MenuFreeCharacterSpriteRecords();
                    MenuDrawStatTable(slot, 0, 0, 1);
                    func_801D8EA4(slot, 0, 0, 1);
                    g_Menu->pManager->shouldRenderSubmenuStrings = 1;
                } else {
                    MenuPlaySoundEffect(4);
                }
                lastSlot = 0xFF;
                break;
            case 1:
                if (g_Menu->pManager->currentCharacterIDs[slot] != 7) {
                    func_801DDF24(slot, 0, 1);
                    MenuFreeUnk430Panel(1);
                } else {
                    MenuPlaySoundEffect(4);
                }
                lastSlot = 0xFF;
                break;
            case 2:
                if (g_Menu->pManager->currentCharacterIDs[slot] != 7) {
                    func_801DDF24(slot, 0, 2);
                    MenuFreeUnk430Panel(2);
                } else {
                    MenuPlaySoundEffect(4);
                }
                lastSlot = 0xFF;
                break;
            case 3:
                if (g_FieldPlayerWalkmeshMaterial == 0) {
                    if (g_GameState.gearRide[slot] != 0) {
                        g_GameState.gearRide[slot] = 0;
                        stringSet = 0;
                    } else if (g_GameState.characters[g_Menu->pManager->currentCharacterIDs[slot]].gearId != 0xFF) {
                        stringSet = 6;
                        g_GameState.gearRide[slot] = 1;
                    }
                    MenuSetGearWindowQuads(slot, 1, g_GameState.gearRide[slot]);
                } else {
                    MenuPlaySoundEffect(4);
                }
                lastSlot = 0xFF;
                break;
            }
            g_Menu->pManager->shouldRenderStrings18E0[4] = 1;
            g_Menu->pManager->shouldRenderStrings18E0[5] = 1;
            MenuLoadStringsFromIds(6, (u8*)g_Menu + 0x18E0, g_MenuUnk18E0StringIdSets + stringSet, (u8*)g_Menu->pManager + 0x54);
            g_Menu->pManager->shouldRenderUnk358Prims = 1;
            g_Menu->pManager->shouldRenderUnk35C = 1;
            g_Menu->pManager->shouldRenderPanel[1] = 1;
            MenuMovePointerCursor(g_Menu->subMenuChoice + 7, 1);
            g_Menu->pManager->shouldRenderPointerCursor = 1;
            g_Menu->subMenuPrevChoice = 0xFF;
            MenuCreateShoulderButtonUi(0, 1);
            break;
        case 5:
            active = 0;
            break;
        case 1:
            if (g_Menu->subMenuChoice != 0) {
                g_Menu->subMenuChoice--;
            } else {
                g_Menu->subMenuChoice = g_Menu->subMenuNumChoices + 0xFF;
            }
            break;
        case 3:
            if (++g_Menu->subMenuChoice >= g_Menu->subMenuNumChoices) {
                g_Menu->subMenuChoice = 0;
            }
            break;
        case 9:
            slot = func_801D9704(slot, 0, 1);
            g_Menu->subMenuPrevChoice = 0xFF;
            break;
        case 10:
            slot = func_801D9704(slot, 1, 1);
            g_Menu->subMenuPrevChoice = 0xFF;
            break;
        }
    }
    MenuClearBytes(6, (u8*)g_Menu->pManager + 0x54);
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    return 1;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E23CC);

void MenuInitializeCharacterScreenBuffers(void) {
    MenuLoadSubMenuChoiceStrings(1);
    g_Menu->pGearWindowPrims = HeapAlloc(0x2AF0, 0);
    bzero(g_Menu->pGearWindowPrims, 0x2AF0);
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->pEquipmentPanel = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->pEquipmentPanel, 0x2AC);
    MenuLoadResourceSet(3);
    MenuCreateShoulderButtonUi(0, 0);
}
void MenuFreeCharacterScreenBuffers(void) {
    HeapFree(g_Menu->pGearWindowPrims);
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->pEquipmentPanel);
    MenuLoadResourceSet(0x13);
}
u8 MenuRunCharacterDressingRoomLoop(void) {
    u8 active;
    u8 lastSlot;
    u8 firstDraw;
    u8 drawSlot;
    s32 slot;
    s32 i;

    active = 1;
    lastSlot = 0xF3;
    firstDraw = 1;
    slot = g_Menu->characterSlot;
    g_Menu->subMenuChoice = 2;
    g_Menu->subMenuPrevChoice = 0xFF;
    MenuInitializeCharacterScreenBuffers();
    for (i = 0; i < 3; i++) {
        if (g_Menu->pManager->currentCharacterIDs[i] != 0xFF) {
            slot = i;
            break;
        }
    }
    while (active != 0) {
        MenuRenderFrame();
        if (slot != lastSlot) {
            MenuApplyEquippedItemBonuses(g_Menu->pDressingRoom, g_Menu->pManager->currentCharacterIDs[slot]);
            drawSlot = slot;
            MenuComputeCharacterStats(g_Menu->pDressingRoom, g_Menu->pManager->currentCharacterIDs[slot]);
            lastSlot = drawSlot;
            MenuDrawGearScreen(lastSlot, 0);
            if (firstDraw != 0) {
                MenuStartOpenAnimation();
                firstDraw = 0;
                MenuTweenCharacterWindows(0, 0);
                func_801E86C8(0);
            }
        }
        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            MenuSetSubMenuChoiceStringVertices();
            MenuSetSubMenuChoicePrims(0);
            g_Menu->subMenuPrevChoice = g_Menu->subMenuChoice;
        }
        switch (g_Menu->input) {
        case 4:
            MenuResetSelectionCursorFlags();
            MenuClearSubMenuChoiceFlags();
            g_Menu->pManager->shouldRenderUnk358Prims = 0;
            g_Menu->pManager->shouldRenderUnk35C = 0;
            g_Menu->pManager->shouldRenderPanel[1] = 0;
            switch (g_Menu->subMenuChoice) {
            case 0:
                lastSlot = slot;
                g_Menu->pManager->shouldRenderSubmenuStrings = 0;
                func_801E05D0(lastSlot, 0, 0);
                MenuFreeCharacterSpriteRecords();
                MenuDrawStatTable(lastSlot, 0, 0, 0);
                func_801D8EA4(lastSlot, 0, 0, 0);
                g_Menu->pManager->shouldRenderSubmenuStrings = 1;
                lastSlot = 0xFF;
                break;
            case 1:
                func_801DDF24(slot, 0, 0);
                MenuFreeUnk430Panel(0);
                lastSlot = 0xFF;
                break;
            case 2:
                MenuRunUnk438PanelLoop(slot);
                break;
            }
            MenuLoadSubMenuChoiceStrings(1);
            g_Menu->pManager->shouldRenderUnk358Prims = 1;
            g_Menu->pManager->shouldRenderUnk35C = 1;
            g_Menu->pManager->shouldRenderPanel[1] = 1;
            MenuMovePointerCursor(g_Menu->subMenuChoice + 7, 1);
            g_Menu->pManager->shouldRenderPointerCursor = 1;
            g_Menu->subMenuPrevChoice = 0xFF;
            MenuCreateShoulderButtonUi(0, 0);
            break;
        case 5:
            active = 0;
            break;
        case 1:
            if (g_Menu->subMenuChoice != 0) {
                g_Menu->subMenuChoice--;
            } else {
                g_Menu->subMenuChoice = g_Menu->subMenuNumChoices + 0xFF;
            }
            break;
        case 3:
            if (++g_Menu->subMenuChoice >= g_Menu->subMenuNumChoices) {
                g_Menu->subMenuChoice = 0;
            }
            break;
        case 9:
            slot = func_801D9704(slot, 0, 0);
            break;
        case 10:
            slot = func_801D9704(slot, 1, 0);
            break;
        }
    }
    MenuLoadSubMenuChoiceStrings(0);
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    return 1;
}

void MenuFreeSubMenuPanels(u8 arg0) {
    switch (g_Menu->mainMenuChoice + arg0) {
    case 0:
    case 7:
    case 9:
        break;
    case 1:
    case 8:
        MenuShutdown();
        break;
    case 2:
        g_Menu->pManager->shouldRenderUnk358Prims = 0;
        g_Menu->pManager->shouldRenderUnk35C = 0;
        g_Menu->pManager->shouldRenderPanel[1] = 0;
        MenuFreeGearScreenBuffers();
        break;
    case 3:
        MenuFreeUnk430Panel(0);
        break;
    case 4:
        MenuFreeUnk42CPanel();
        break;
    case 5:
        MenuFreeCharacterSpriteRecords();
        MenuFreeUnk35CPanel();
        break;
    case 6:
        g_Menu->pManager->shouldRenderUnk358Prims = 0;
        g_Menu->pManager->shouldRenderUnk35C = 0;
        g_Menu->pManager->shouldRenderPanel[1] = 0;
        MenuClearSubMenuChoiceFlags();
        MenuFreeCharacterScreenBuffers();
        break;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E31C0);
/* PARKED 99.75% (1020 bytes = retail's exact size; four differing lines, the
   residual is ONE instruction: retail's `andi v1,s1,0xff; beqz v1` where this
   body has `nop; beqz s1`). Applies one inventory item's stat effects to
   g_GameState.characters[charIndex]; returns nonzero when the item could not do
   anything (both bars already full), which is why its caller (misc9) consumes the
   item only on 0.

   Retail is 1020 bytes, not 1024: the next symbol (MenuApplyDressingRoomSlotHpBonus)
   is at 0x801E35BC.

   Settled, each measured (round 377 unless noted):
   - the item record is MenuShopItem (include/system/menu.h): amount at 0x8,
     effectFlags at 0xA, statFlags at 0xC.
   - `pItem = pDressingRoom->pItemsData; pItem = pItem + itemIndex;` as TWO
     statements: that is what puts `andi a2,a2,0xff; sll a2,a2,4` at the top of the
     function and gives retail's `lw s0,28(a0)` plus the base-first `addu s0,s0,a2`
     (the one-statement forms are 94-95%).
   - the 0xA&2 block reads statFlags through a function-top `s16 statFlags;` local
     that both the flag test and the compare use, while the add and subtract arms
     re-read pItem->statFlags (retail's move/andi pair in the beqz delay slot).
   - its compare is `pChar->statBonus < (statFlags & 0xFF)`: with `(u8)statFlags`
     gcc emits `sltu` where retail has `slt`.
   - the 0xA&1 block is a `switch (pItem->amount)` with case 1 / case 2.
   - the return type is `u16`. With s32 the tail is 1028 bytes (two extra moves).
   - mpGain = 10 before the 0x8000 if and mpScale = mpGain between the two ifs
     (permuter find, 94.18 -> 98.73): the `li a1,0xa` has to be emitted before the
     0x8000 test to fill its beqz delay slot.
   - the closing block returns per arm (`hpFull ? mpFull : 0`, `hpFull`,
     `(effectFlags & 0x4000) ? mpFull : 0`), not through one `result` variable
     (98.82-99.51 at the same size).

   THE TYPE OF hpFull DECIDES RETAIL'S TAIL LAYOUT. Retail's tail needs the
   deep arm (`0xA&0x8000 && 0xA&0x4000 && hpFull == 0`) to materialise its 0 in the
   beqz DELAY SLOT and mpFull in the j's. With `u8 hpFull` (a QI-mode compare)
   cc1 2.6.0 cross-jumps that 0 onto the last arm's fall-through 0 and inverts the
   branch instead (99.69, `beqz v0,<zero> / move v0,s2 / j / nop`). `unsigned int
   hpFull` - the permuter's find - stops the cross-jump and gives retail's exact
   tail (`nop` in the slot, `beqz s1,EXIT / move v0,zero / j EXIT / move v0,s2`),
   but loses retail's QI `andi v1,s1,0xff` because hpFull is no longer narrowed:
   99.75, 1020 bytes, 4 differing lines. The two levers are mutually exclusive in
   this compiler:
   - `(hpFull & 0xFF) == 0` gets the andi back and re-cross-jumps (99.69);
   - `(u8)hpFull` in the test, or a `u8 hpDone = hpFull;` copy, gives 98.67 /
     1012 (253 insns, the copy replaces the andi and the tail shrinks);
   - `(hpFull & 0xFF) == 0` plus a following `if (hpFull)` return: 99.16 / 1028;
   - last arm as `if (...) return mpFull; return 0;` or an `else if` chain:
     98.18 / 1004 (251 insns, the mpFull copies merge too).
   So whichever spelling narrows hpFull to QI gets the merge, and the merge is what
   moves the 0 out of the delay slot. Two 4-minute permuter runs from the u8 body
   had found only a do-while wrapper; a 3-minute run from the 99.69 body found this
   type change (base 216 -> 205) and a re-run from the u8 body nothing better.
   ROUND 405 (this lane), all measured at retail's 1020 bytes / 255 insns:
   - every spelling that puts retail's `andi` in that slot also loses the tail
     arrangement (99.69): `(hpFull & 0xFF) == 0`, `(hpFull & 0xFF) ? mpFull : 0`,
     `(u8)hpFull`, and a QI-to-SI copy (`u8 hpFull; unsigned int hpFullW;
     hpFullW = hpFull;`). The named cause is jump.c: the masked 0x4000 operand is a
     fresh temp, and the deep arm's `return 0` block is then redirected onto the
     last arm's trailing `move v0,zero` (the branch inverts, mpFull takes the delay
     slot). With an SI compare on hpFull itself jump.c leaves the block alone, so
     the tail is retail's and only the ANDI is missing. Retail's compile did neither
     inversion: its deep branch targets the epilogue with 0 in the delay slot (reorg
     threaded the block), and it kept the ANDI.
   - permuter 3 min from the 99.69 (masked) body: base 216 -> best 124, its candidate
     routes the deep arm through a local (`deepResult = ...; return deepResult;`)
     which measures 99.51 / 1024 bytes, one instruction MORE than retail - rejected,
     not a closer body.
   So the residual is a jump.c redirect decision, not a source shape: any spelling
   with the mask inverts, any spelling with retail's tail has no mask. Parked at
   99.75 (the SI body) because that is the highest measured and the simplest source.
   ROUND 418: screened as a compiler question and closed as far as a preset can reach.
   - try_flags one function: gcc-2.7.2 -O2 -G8 91.96, gcc-2.7.2-cdk -O2 -G8 91.3,
     gcc-2.6.0 -O3 -G8 99.75, gcc-2.6.0 -O2 -G0 99.75 - so 2.6.0 with this unit's
     flags is the compiler and no flag variant of it moves the number.
   - the block redirect is NOT flag_thread_jumps: cc1 2.6.0 -fno-thread-jumps
     produces a .s identical to the -O2 one (diff of the two pre-maspsx files shows
     only the option comment), so no preset can turn it off.
   - a third 3-minute permuter run from this SI body: RESULT PERMUTER_NONE,
     base=205 best=205, 797 iterations.
   Do not reopen without a different compiler version.
*/
/*
u16 func_801E31C0(MenuDressingRoom* pDressingRoom, u8 charIndex, u8 itemIndex) {
    MenuShopItem* pItem;
    s16 statFlags;
    GameCharacter* pChar;
    // Two locals for the same 10: the li has to be emitted before the first if to
    // fill retail's beqz delay slot, and the 0x4000 arm reads mpScale. One local
    // measures 94.18, this pair 98.73.
    s32 mpGain;
    unsigned int hpFull;
    u8 mpFull;
    s32 result;
    s32 hpScale;
    s32 mpScale;

    hpFull = 0;
    mpFull = 0;
    pChar = &g_GameState.characters[charIndex];
    pItem = pDressingRoom->pItemsData;
    pItem = pItem + itemIndex;
    mpGain = 10;
    if (pItem->effectFlags & 0x8000) {
        if (pChar->hp == pChar->maxHp) {
            hpFull = 1;
        } else {
            hpScale = 50;
            pChar->hp = pChar->hp + pItem->amount * hpScale;
        }
    }
    mpScale = mpGain;
    if (pItem->effectFlags & 0x4000) {
        if (pChar->mp == pChar->maxMp) {
            mpFull = 1;
        } else {
            pChar->mp = pChar->mp + mpScale * pItem->amount;
        }
    }
    if (pChar->hp > pChar->maxHp) {
        pChar->hp = pChar->maxHp;
    }
    if (pChar->mp > pChar->maxMp) {
        pChar->mp = pChar->maxMp;
    }
    if (pItem->effectFlags & 4) {
        if (pItem->statFlags & 0x8000) {
            pChar->attack = pChar->attack + pItem->amount;
        }
        if (pItem->statFlags & 0x4000) {
            pChar->defense = pChar->defense + pItem->amount;
        }
        if (pItem->statFlags & 0x2000) {
            pChar->ether = pChar->ether + pItem->amount;
        }
        if (pItem->statFlags & 0x1000) {
            pChar->etherDefense = pChar->etherDefense + pItem->amount;
        }
        if (pItem->statFlags & 0x800) {
            pChar->maxHp = pChar->maxHp + pItem->amount;
        }
        if (pItem->statFlags & 0x400) {
            pChar->maxMp = pChar->maxMp + pItem->amount;
        }
        if (pChar->attack > 200) {
            pChar->attack = 200;
        }
        if (pChar->defense > 200) {
            pChar->defense = 200;
        }
        if (pChar->ether > 200) {
            pChar->ether = 200;
        }
        if (pChar->etherDefense > 200) {
            pChar->etherDefense = 200;
        }
        if (pChar->maxHp > 999) {
            pChar->maxHp = 999;
        }
        if (pChar->maxMp > 99) {
            pChar->maxMp = 99;
        }
    }
    if (pItem->effectFlags & 2) {
        statFlags = pItem->statFlags;
        if (statFlags & 0x8000) {
            pChar->statBonus = pChar->statBonus + (u8)pItem->statFlags;
            if (pChar->statBonus > 200) {
                pChar->statBonus = 200;
            }
        } else if (pChar->statBonus < (statFlags & 0xFF)) {
            pChar->statBonus = 0;
        } else {
            pChar->statBonus = pChar->statBonus - (u8)pItem->statFlags;
        }
    }
    if (pItem->effectFlags & 1) {
        switch (pItem->amount) {
        case 1:
            MenuFillInventoryTables();
            break;
        case 2:
            MenuResetConditionMasks();
            break;
        }
    }
    if (pItem->effectFlags & 0x8000) {
        if (pItem->effectFlags & 0x4000) {
            if (hpFull) {
                return mpFull;
            } else {
                return 0;
            }
        }
        return hpFull;
    }
    return (pItem->effectFlags & 0x4000) ? mpFull : 0;
}
*/
void MenuApplyDressingRoomSlotHpBonus(MenuDressingRoom* pDressingRoom, u8 charIndex, u8 targetIndex, u8 slotIndex, u8 isGear) {
    GameCharacter* pChar;
    GameCharacter* pTarget;
    MenuEquipmentSlot* pSlots;
    GameGear* pGear;
    u32 value;

    pChar = &g_GameState.characters[charIndex];
    pTarget = &g_GameState.characters[targetIndex];
    pGear = &g_GameState.gears[g_GameState.characters[charIndex].gearId];
    if (isGear == 0) {
        pSlots = pDressingRoom->pCharacterSlots[charIndex];
        pSlots = &pSlots[slotIndex];
        value = pChar->ether * pSlots->unk11;
        pTarget->hp = pTarget->hp + value;
        if (pTarget->hp > pTarget->maxHp) {
            pTarget->hp = pTarget->maxHp;
        }
    } else {
        pGear->hp += pGear->maxHp / 10;
        if (pGear->maxHp < pGear->hp) {
            pGear->hp = pGear->maxHp;
        }
    }
}
void MenuApplyEquippedItemBonuses(MenuDressingRoom* pDressingRoom, u8 charIndex) {
    GameCharacter* pChar;
    MenuShopAccessory* pAccessory;
    MenuShopWeapon* pWeapon;
    u8 value;
    u8 i;

    pChar = &g_GameState.characters[charIndex];
    pChar->defenseFromAccessories = 0;
    pChar->accessoryEffectFlags5 = 0;
    pChar->attackFromEquipment = 0;
    pChar->defenseFromEquipment = 0;
    pChar->agilityFromEquipment = 0;
    pChar->etherFromEquipment = 0;
    pChar->etherDefenseFromEquipment = 0;
    pChar->hitPercentageFromEquipment = 0;
    pChar->evadePercentageFromEquipment = 0;
    pChar->accessoryEffectFlags8 = 0;
    pChar->mpPercentFromAccessories = 0;
    pChar->accessoryEffectFlags1 = 0;
    pChar->accessoryEffectFlags2 = 0;
    pChar->accessoryEffectFlags3 = 0;
    pChar->accessoryEffectFlags4 = 0;
    pChar->accessoryEffectFlags7 = 0;
    pChar->accessoryEffectFlags10 = 0;

    for (i = 0; i < 3; i++) {
        /* Retail adds the index into the base in place (addu a2,a2,v0), which
           only a separate copy of the base reproduces. */
        pAccessory = pDressingRoom->pAccessoriesData;
        pAccessory += pChar->accessorySlots[i];
        pChar->defenseFromAccessories += pAccessory->defense;
        switch (pAccessory->effectType) {
        case 1:
            pChar->accessoryEffectFlags1 |= pAccessory->effectFlags;
            break;
        case 2:
            pChar->accessoryEffectFlags2 |= pAccessory->effectFlags;
            break;
        case 3:
            pChar->accessoryEffectFlags3 |= pAccessory->effectFlags;
            break;
        case 4:
            pChar->accessoryEffectFlags4 |= pAccessory->effectFlags;
            break;
        case 7:
            pChar->accessoryEffectFlags7 |= pAccessory->effectFlags;
            break;
        case 5:
            pChar->accessoryEffectFlags5 |= pAccessory->effectFlags;
            break;
        case 8:
        case 9:
            pChar->accessoryEffectFlags8 += pAccessory->effectFlags;
            break;
        case 10:
            pChar->accessoryEffectFlags10 += pAccessory->effectFlags;
            break;
        }
        /* Retail keeps the byte at 0xC in one register across the eight bit
           tests, and adds it as the left operand, so spell it that way. The
           halfword view below is load-bearing: retail tests the flags through
           the lh/lhu at 0xC (the 0x8000 test even through an s16 load), and
           plain `pAccessory->statFlags & 0x80` bit tests are 97.09%. */
        value = pAccessory->effectAmount;
        if (*(s16*)&pAccessory->effectAmount & 0x8000) {
            pChar->attackFromEquipment = value + pChar->attackFromEquipment;
        }
        if (*(u16*)&pAccessory->effectAmount & 0x4000) {
            pChar->defenseFromEquipment = value + pChar->defenseFromEquipment;
        }
        if (*(u16*)&pAccessory->effectAmount & 0x2000) {
            pChar->agilityFromEquipment = value + pChar->agilityFromEquipment;
        }
        if (*(u16*)&pAccessory->effectAmount & 0x1000) {
            pChar->etherFromEquipment = value + pChar->etherFromEquipment;
        }
        if (*(u16*)&pAccessory->effectAmount & 0x800) {
            pChar->etherDefenseFromEquipment = value + pChar->etherDefenseFromEquipment;
        }
        if (*(u16*)&pAccessory->effectAmount & 0x400) {
            pChar->hitPercentageFromEquipment = value + pChar->hitPercentageFromEquipment;
        }
        if (*(u16*)&pAccessory->effectAmount & 0x200) {
            pChar->evadePercentageFromEquipment = value + pChar->evadePercentageFromEquipment;
        }
        if (*(u16*)&pAccessory->effectAmount & 0x100) {
            pChar->defenseFromAccessories = value + pChar->defenseFromAccessories;
        }
    }

    pWeapon = pDressingRoom->pWeaponsData;
    pWeapon += pChar->weaponId[0];
    pChar->slots[0].attack = pWeapon->attack;
    pChar->slots[0].flags = pWeapon->slotFlags;
    pChar->slots[0].chance = pWeapon->unkA;
    pChar->slots[0].conditionKind = pWeapon->unkB;
    if (pChar->characterId == 4) {
        pWeapon = pDressingRoom->pWeaponsData;
        pWeapon += pChar->extraWeaponIds[0];
        pChar->slots[0].attack = pWeapon->attack;
        pChar->slots[0].flags = pWeapon->slotFlags;
        pChar->slots[0].chance = pWeapon->unkA;
        pChar->slots[0].conditionKind = pWeapon->unkB;
        pWeapon = pDressingRoom->pWeaponsData;
        pWeapon += pChar->extraWeaponIds[3];
        pChar->slots[3].attack = pWeapon->attack;
        pChar->slots[3].flags = pWeapon->slotFlags;
        pChar->slots[3].chance = pWeapon->unkA;
        pChar->slots[3].conditionKind = pWeapon->unkB;
    }
    if (pChar->slots[0].conditionKind == 0x64) {
        pChar->accessoryEffectFlags7 |= pChar->slots[0].flags;
    }
}

void MenuComputeCharacterStats(MenuDressingRoom* pDressingRoom, u8 charIndex) {
    GameCharacter* pChar;

    pChar = &g_GameState.characters[charIndex];
    if (pChar->characterId == 4) {
        pDressingRoom->statValues[0] = (pChar->slots[0].attack + pChar->slots[3].attack) * 6 / 10;
    } else {
        pDressingRoom->statValues[0] = pChar->attack + pChar->attackFromEquipment + pChar->slots[0].attack;
    }
    pDressingRoom->statValues[1] = pChar->hitPercentage + pChar->hitPercentageFromEquipment;
    pDressingRoom->statValues[2] = pChar->defense + pChar->defenseFromEquipment + pChar->defenseFromAccessories;
    pDressingRoom->statValues[3] = pChar->evadePercentage + pChar->evadePercentageFromEquipment;
    pDressingRoom->statValues[4] = pChar->ether + pChar->etherFromEquipment;
    pDressingRoom->statValues[5] = pChar->etherDefense + pChar->etherDefenseFromEquipment;
    pDressingRoom->statValues[6] = pChar->agility + pChar->agilityFromEquipment;
    if (pDressingRoom->statValues[0] > MENU_STAT_MAX) {
        pDressingRoom->statValues[0] = MENU_STAT_MAX;
    }
    if (pDressingRoom->statValues[1] > MENU_STAT_PERCENT_MAX) {
        pDressingRoom->statValues[1] = MENU_STAT_PERCENT_MAX;
    }
    if (pDressingRoom->statValues[2] > MENU_STAT_MAX) {
        pDressingRoom->statValues[2] = MENU_STAT_MAX;
    }
    if (pDressingRoom->statValues[3] > MENU_STAT_PERCENT_MAX) {
        pDressingRoom->statValues[3] = MENU_STAT_PERCENT_MAX;
    }
    if (pDressingRoom->statValues[4] > MENU_STAT_MAX) {
        pDressingRoom->statValues[4] = MENU_STAT_MAX;
    }
    if (pDressingRoom->statValues[5] > MENU_STAT_MAX) {
        pDressingRoom->statValues[5] = MENU_STAT_MAX;
    }
    if (pDressingRoom->statValues[6] > MENU_STAT_AGILITY_MAX) {
        pDressingRoom->statValues[6] = MENU_STAT_AGILITY_CAPPED;
    }
}

void MenuFillDressingRoomGearStats(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    GameCharacter* pChar;
    GameGear* pGear;
    s32 value;
    u32 pilotId;

    if (g_GameState.unk22B6 & 0x1000) {
        D_801E9811 = 10;
    }
    if (gearIndex == 7) {
        g_GameState.gears[7].hp = g_GameState.characters[7].hp * 50;
        g_GameState.gears[7].maxHp = g_GameState.characters[7].maxHp * 50;
        g_GameState.gears[7].engineOutput = g_GameState.characters[7].attack + g_GameState.characters[7].attackFromEquipment;
        g_GameState.gears[7].baseDefense = (g_GameState.characters[7].defense + g_GameState.characters[7].defenseFromEquipment) * 12;
        g_GameState.gears[7].baseEtherDefense = (g_GameState.characters[7].etherDefense + g_GameState.characters[7].etherDefenseFromEquipment) * 6;
        g_GameState.gears[7].agility = g_GameState.characters[7].agility + g_GameState.characters[7].agilityFromEquipment;
    }
    pGear = &g_GameState.gears[gearIndex];
    pilotId = g_GearPilotIds[gearIndex];
    pDressingRoom->hp = pGear->hp;
    pDressingRoom->maxHp = pGear->maxHp;
    pDressingRoom->totalDefense = pGear->baseDefense + pGear->defenseFromEquipment;
    pChar = &g_GameState.characters[pilotId];
    pDressingRoom->totalEtherDefense = pChar->etherDefense + pChar->etherDefenseFromEquipment + pGear->etherDefenseFromEquipment + pGear->baseEtherDefense;
    pDressingRoom->totalWeight = pGear->weight + pGear->weightFromEquipment;
    pDressingRoom->unkAA = pGear->unk6A;
    pDressingRoom->fuel = pGear->fuel;
    pDressingRoom->maxFuel = pGear->maxFuel;
    value = pGear->engineOutput * (pGear->attackPower + pGear->unk55[1]);
    if (gearIndex == 5 || gearIndex == 13) {
        pDressingRoom->totalAttack = (pGear->weaponData[0].attack + pGear->weaponData[2].attack) * 6 / 10 + value;
    } else {
        pDressingRoom->totalAttack = pGear->weaponData[0].attack + value;
    }
    pDressingRoom->totalResponsiveness = pGear->responsivenessFromFrame + pGear->responsiveness;
    pDressingRoom->totalAgility = pGear->agility - pGear->weightPenalty;
    pDressingRoom->totalEtherAmplification = pGear->etherAmplification + pGear->etherAmplificationFromEquipment;
    pDressingRoom->unkB5 = pGear->unk9D;
    pDressingRoom->unkB6 = pGear->unk9C;
}
void MenuCopyGearEquipmentSlots(MenuDressingRoom* pDressingRoom, u8 gearId) {
    u8 i;

    MenuApplyGearAccessories(pDressingRoom, gearId);
    MenuApplyGearWeapons(pDressingRoom, gearId);
    switch (gearId) {
    case 0:
        for (i = 0; i < 3; i++) {
            g_GameState.gears[1].accessorySlots[i] = g_GameState.gears[0].accessorySlots[i];
        }
        MenuApplyGearAccessories(pDressingRoom, 1);
        break;
    case 1:
        for (i = 0; i < 3; i++) {
            g_GameState.gears[15].accessorySlots[i] = g_GameState.gears[1].accessorySlots[i];
        }
        MenuApplyGearAccessories(pDressingRoom, 15);
        break;
    case 2:
        for (i = 0; i < 3; i++) {
            g_GameState.gears[10].accessorySlots[i] = g_GameState.gears[2].accessorySlots[i];
        }
        MenuApplyGearAccessories(pDressingRoom, 10);
        break;
    case 3:
        for (i = 0; i < 3; i++) {
            g_GameState.gears[11].accessorySlots[i] = g_GameState.gears[3].accessorySlots[i];
        }
        MenuApplyGearAccessories(pDressingRoom, 11);
        break;
    case 4:
        for (i = 0; i < 3; i++) {
            g_GameState.gears[12].accessorySlots[i] = g_GameState.gears[4].accessorySlots[i];
        }
        MenuApplyGearAccessories(pDressingRoom, 12);
        g_GameState.gears[12].weaponId[0] = g_GameState.gears[4].weaponId[0];
        MenuApplyGearWeapons(pDressingRoom, 12);
        break;
    case 5:
        for (i = 0; i < 3; i++) {
            g_GameState.gears[13].accessorySlots[i] = g_GameState.gears[5].accessorySlots[i];
        }
        MenuApplyGearAccessories(pDressingRoom, 13);
        g_GameState.gears[13].weaponId[0] = g_GameState.gears[5].weaponId[0];
        g_GameState.gears[13].weaponId[3] = g_GameState.gears[5].weaponId[3];
        g_GameState.gears[13].extraWeaponIds[0] = g_GameState.gears[5].extraWeaponIds[0];
        g_GameState.gears[13].extraWeaponIds[3] = g_GameState.gears[5].extraWeaponIds[3];
        MenuApplyGearWeapons(pDressingRoom, 13);
        break;
    case 6:
        for (i = 0; i < 3; i++) {
            g_GameState.gears[14].accessorySlots[i] = g_GameState.gears[6].accessorySlots[i];
        }
        MenuApplyGearAccessories(pDressingRoom, 14);
        break;
    }
}
void MenuRecalculateGearStats(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    MenuApplyGearFrame(pDressingRoom, gearIndex);
    MenuApplyGearEngine(pDressingRoom, gearIndex);
    MenuApplyGearArmor(pDressingRoom, gearIndex);
}

void MenuApplyGearFrame(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    GameGear* pGear;
    GearFrame* pFrame;
    u8 frameId;

    pGear = &g_GameState.gears[gearIndex];
    pFrame = pDressingRoom->pGearFrames;
    frameId = pGear->frameId;
    pFrame = &pFrame[frameId];

    pGear->hp = pFrame->hp;
    pGear->maxHp = pFrame->hp;
    pGear->agility = pFrame->agility;
    pGear->etherAmplification = pFrame->etherAmplification;
    pGear->unk9D = pFrame->unk16;
    pGear->responsivenessFromFrame = pFrame->responsiveness;
    if (pGear->maxHp < pGear->hp) {
        pGear->hp = pGear->maxHp;
    }
}


void MenuApplyGearArmor(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    GameGear* pGear;
    GearArmor* pArmorList;
    GearArmor* pArmor;

    pGear = &g_GameState.gears[gearIndex];
    pArmorList = pDressingRoom->pGearArmor;
    pArmor = &pArmorList[pGear->armorId];

    pGear->baseDefense = pArmor->baseDefense;
    pGear->baseEtherDefense = pArmor->baseEtherDefense;
}

void MenuApplyGearEngine(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    GameGear* pGear;
    GearEngine* pEngine;
    u16 fuel;
    u8 engineId;

    pGear = &g_GameState.gears[gearIndex];
    engineId = pGear->engineId;
    pEngine = pDressingRoom->pGearEngines;
    fuel = pGear->fuel;
    pEngine = &pEngine[engineId];

    pGear->maxFuel = pEngine->maxFuel;
    pGear->engineOutput = pEngine->powerOutput;
    pGear->unk3D = pEngine->unkD;
    pGear->unk3E = pEngine->unkE;
    pGear->unk3F = pEngine->unkE;

    if (pGear->maxFuel < fuel) {
        pGear->fuel = pGear->maxFuel;
    }
}
