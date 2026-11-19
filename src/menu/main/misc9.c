#include "common.h"

#include "main/game.h"
#include "psyq/libapi.h"
#include "system/memory.h"
#include "system/menu.h"

// One page of the inventory window: 8 row panels of MENU_INVENTORY_ROW_ITEM_COUNT items each.
#define MENU_INVENTORY_ROW_ITEM_COUNT 2
#define MENU_INVENTORY_PAGE_ITEM_COUNT 16
// String IDs per row panel of the item list and of the character list.
#define MENU_ITEM_ROW_STRING_COUNT 8
#define MENU_CHARACTER_ROW_STRING_COUNT 6
// An item name is three strings: GetStringEntry(pStrings, itemId * 3 + i).
#define MENU_ITEM_NAME_LINE_COUNT 3
// Weapon and accessory slots copied in and out of the equipment panel.
#define MENU_CHARACTER_WEAPON_SLOT_COUNT 5
#define MENU_GEAR_WEAPON_SLOT_COUNT 4
#define MENU_ACCESSORY_SLOT_COUNT 3
// Entries of MenuDressingRoom.statValues.
#define MENU_STAT_VALUE_COUNT 9
// Scratch buffer one rendered item name image is built in before LoadImage.
#define MENU_COLUMN_IMAGE_SIZE 0x618
// func_801C72BC's resource table: entry kind + 0x10 unloads what entry kind loaded.
#define MENU_PANEL_FREE_OFFSET 0x10
// Sprite records at the end of the 0x430 panel: 0x1C holds a copy of the selected
// column's string, 0x1D..0x1F the three strings func_801DCE60 renders.
#define MENU_PANEL_COLUMN_STRING 0x1C
#define MENU_PANEL_HEADING_STRING 0x1E

extern u8 g_MenuItemRowStringIds[][8]; // 8 u8 string IDs per panel
extern u8 g_MenuCharacterRowStringIds[][6]; // 6 u8 string IDs per panel
extern u16 g_MenuInventoryWindowHeight;
extern u8 g_DressingRoomItemIds[];
extern s32 g_MenuInventoryRowCount;
extern u16 g_MenuInventoryScrollStep;
extern u8 g_MenuPanelRowXOffsets[];
extern u8 g_MenuColumnRowStringIds[];
extern u8 D_801E9785;
extern u16 D_801E9788[][2];
extern u16 D_801E9794[][2];
extern u16 D_801E97A0[][2];
extern u8 g_ShopIndex;
extern u16 g_MenuEquipmentSlotRowY[][2];

// 0x1094-byte item panel buffer built by MenuCreateUnk430Panel: one MenuString
// sprite record per column, then the column heading string list and one flag per
// column (read as a u8 array by MenuAddUnk430Prims in misc5).
typedef struct {
    /* 0x0000 */ MenuString strings[0x21];
    /* 0x1080 */ void* pStringData; // String list the column headings are fetched from
    /* 0x1084 */ u8 columnFlags[0xE]; // 0x80 = the column holds an item, see func_801DDF24
    /* 0x1092 */ u8 shouldRenderColumnHeadings;
    /* 0x1093 */ u8 pad1093;
} MenuUnk430Panel; // Size: 0x1094

void func_801DC3D8(u_char arg0, u_char arg1);
void func_801DCE60(u_char arg0, s32 arg1, u_char arg2);
void func_801DD790(u_char arg0, s32 arg1, u_char arg2);
void MenuInitializeUnk42CPanel(void);
void MenuCreateScrollHandle(s32 a0, s32 a1, s32 a2);
void func_801DB0A8(s32 arg0, s32 arg1, u_char arg2, u_char arg3);
void MenuFreeArrowCursor(u_char index);
void MenuStartOpenAnimation(void);
void func_801D29A8(s32 arg0, s32 arg1);
void MenuSwapInventoryItems(s32 index0, s32 index1);
u_char func_801DB920(s32 arg0, s32 arg1);
void MenuRenderFrame(void);
void MenuHidePointerCursors(void);
void MenuFreeWindow(s32 arg0);
void func_801C72BC(s32 arg0);
void MenuFreeScrollHandle(void);
void MenuClearBytes(s32 arg0, void* pArg1);
void MenuLoadStringsFromIds(s32 arg0, void* pArg1, void* pArg2, void* pArg3);
void MenuSetupPointerCursors(s32 arg0);
void MenuInitializeArrowCursor(s32 arg0);
void MenuCreateShoulderButtonUi(s32 arg0, s32 arg1);
void func_801E3ECC(MenuDressingRoom* pDressingRoom, s32 gearId);
void MenuCreateWindow(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, u_char a6, s32 a7, u_char a8);
void MenuSetRowQuadVertices(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u8 arg5, u8 arg6, u8 arg7);
void MenuFillDressingRoomGearStats(MenuDressingRoom* pDressingRoom, s32 gearId);
void MenuSetPrimShadingMode(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
u32 MenuAndBitMaskLsbFirst(u32 mask, u8 index);
void MenuPlaySoundEffect(u_char soundEffectId);
s32 func_801D9704(s32 slot, u_char direction, u_char arg2);
void func_801DA5BC(s32 arg0);
void func_801DA9A8(s32 arg0, s32 arg1);
void func_801DB39C(s32 arg0);
void func_801DB5E4(s32 arg0);
s32 func_801E31C0(MenuDressingRoom* pDressingRoom, u8 characterId, u8 itemId);
void func_801E7C50(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);
extern void* GetStringEntry(void* pStringData, u32 index);
extern s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);
void func_801D8EA4(u8 arg0, u8 arg1, u8 arg2, u8 arg3);
int func_801DE5CC(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
u_char func_801DF0D4(u_char arg0, u_char arg1, u_char arg2, u_char arg3);
void MenuDrawStatTable(u8 arg0, u8 arg1, u8 arg2, u8 arg3);
void MenuApplyEquippedItemBonuses(MenuDressingRoom* pDressingRoom, u8 characterId);
void MenuComputeCharacterStats(MenuDressingRoom* pDressingRoom, u8 characterId);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DB920);
/* Best: 96.76, 1064 bytes (retail 1068). Structure matches retail everywhere
   except two points:
   1. Loop order: retail emits the loop-entry `if (active)` test BEFORE the
      hoisted idx/pQuantity setup, i.e. its source computed `idx = arg0 * 2 + arg1`
      and `pQuantity = &g_GameState.itemQuantities[idx]` INSIDE the while body and
      loop.c hoisted them into the preheader. Both are inside the loop below; that
      is worth 92.79 -> 96.76.
   2. The itemIDs clear in the `quantity == 0` branch, and with it the whole
      instruction deficit: retail has lui/addiu(g_GameState+0x1F90) + addiu 0x96 +
      addu fp + sb (5 insns) where ours has the assembler's 3-insn %at macro
      `sb $0,%lo(g_GameState+0x2026)(fp)`. Re-measured with cc1_probe.sh: SEVEN
      spellings of the clear (field index, itemQuantities + MAX + idx through a u8*
      or u32 local, a two-statement pointer walk, &arr[MAX] + idx) ALL compile to
      that same bare `sb $0,g_GameState+8230(reg)`, so cc1 always folds the 150 into
      the symbol addend. Retail's addiu 0x96 off a fresh `la` means its base was NOT
      known to be a constant, which no spelling of this address reproduces.
   3. Callee-saved rotation is a knock-on of 2: retail refresh=$s1, &itemIDs=$s2,
      result=$s3; ours $s3, $s1, $s2 (the other seven agree).
   permute.sh --minutes 3 on the earlier 92.79 body: PERMUTER_BETTER, cosmetic
   only. Not re-run from this body. */
/*
u8 func_801DB920(s32 arg0, s32 arg1) {
    MenuShopItem* pItem;
    MenuPointerCursors* pCursors;
    s32 idx;
    s32 i;
    u16 result;
    u32 paged;
    u16 mask;
    u8* pQuantity;
    u8 refresh;
    u8 flags;
    s32 slot;
    u8 active;
    u8* pItemId;
    u8 quantity;

    slot = g_Menu->characterSlot;
    D_801E9785 = 0;
    pItem = &g_Menu->pDressingRoom->pItemsData[g_GameState.itemIDs[arg0 * 2 + arg1]];
    result = 0;
    refresh = 1;
    active = 1;
    flags = pItem->displayFlags;
    if (flags & 0x80) {
        result = 1;
        if (flags & 0x20) {
            result = g_ShopIndex != 0;
        }
    }
    paged = pItem->flags & 1;
    if (result == 0) {
        return result;
    }
    MenuCreateWindow(2, 0x10, 0xE, 0x90, 0xB0, 0, 0, 4, 0);
    while (active) {
        idx = arg0 * 2 + arg1;
        pQuantity = &g_GameState.itemQuantities[idx];
        MenuRenderFrame();
        result = 0;
        if (refresh) {
            refresh = 0;
            func_801DA5BC(arg0);
            func_801DA9A8(arg1, arg0);
            func_801DB39C(1);
            func_801DB5E4(0);
        }
        pCursors = g_Menu->pCursors;
        pCursors->shouldRender[2] = 0;
        pCursors->shouldRender[1] = 0;
        pCursors->shouldRender[0] = 0;
        if (paged) {
            i = 0;
            while (1) {
                if (g_Menu->pManager->currentCharacterIDs[i] != 0xFF) {
                    result |= 1 << i;
                    g_Menu->pCursors->shouldRender[i] = 1;
                }
                i++;
                if (i >= 3) {
                    break;
                }
            }
        } else {
            result = 1 << slot;
            g_Menu->pCursors->shouldRender[slot] = 1;
        }
        g_Menu->pManager->shouldRenderCursors = 1;
        if (*pQuantity == 0) {
            active = 0;
        }
        if (active == 0) {
            break;
        }
        switch (g_Menu->input) {
        case 4:
            refresh = 0;
            pItemId = &g_GameState.itemIDs[idx];
            for (i = 0; i < 3; i++) {
                mask = MenuAndBitMaskLsbFirst(result, i);
                if (mask != 0) {
                    if (func_801E31C0(g_Menu->pDressingRoom, g_Menu->pManager->currentCharacterIDs[i],
                                      *pItemId) == 0) {
                        refresh |= 1;
                    }
                }
            }
            if (refresh) {
                MenuPlaySoundEffect(0x37);
                quantity = *pQuantity - 1;
                *pQuantity = quantity;
                if (quantity == 0) {
                    g_GameState.itemQuantities[idx + MAX_INVENTORY_ITEMS] = 0;
                }
            } else {
                MenuPlaySoundEffect(4);
            }
            refresh = 1;
            break;
        case 5:
            active = 0;
            result = 0;
            break;
        case 1:
            slot = func_801D9704(slot, 0, 0);
            break;
        case 3:
            slot = func_801D9704(slot, 1, 0);
            break;
        }
    }
    pCursors = g_Menu->pCursors;
    pCursors->shouldRender[2] = 0;
    pCursors->shouldRender[1] = 0;
    pCursors->shouldRender[0] = 0;
    func_801DB39C(0);
    g_Menu->pManager->shouldRenderCharacters = 0;
    MenuRenderFrame();
    for (i = 0; i < 3; i++) {
        HeapFree(g_Menu->currentCharacters[i]);
    }
    D_801E9785 = 0;
    MenuFreeWindow(2);
    return result;
}
*/

void MenuSwapInventoryItems(s32 index0, s32 index1) {
    u_char tmp;

    tmp = g_GameState.itemIDs[index0];
    g_GameState.itemIDs[index0] = g_GameState.itemIDs[index1];
    g_GameState.itemIDs[index1] = tmp;
    tmp = g_GameState.itemQuantities[index0];
    g_GameState.itemQuantities[index0] = g_GameState.itemQuantities[index1];
    g_GameState.itemQuantities[index1] = tmp;
}

void MenuFitInventoryWindowToItems(void) {
    s32 count;
    s32 index;
    s32 last;

    index = 0;
    count = 0;
    while (count < MAX_INVENTORY_ITEMS) {
        if (g_GameState.itemIDs[index] != 0) {
            last = index;
        }
        count++;
        index++;
        do {
        } while (0);
    }

    if (last < MENU_INVENTORY_PAGE_ITEM_COUNT) {
        g_MenuInventoryWindowHeight = 0x74;
        g_MenuInventoryRowCount = 0;
        g_MenuInventoryScrollStep = 0;
    } else {
        s32 n = (last - MENU_INVENTORY_PAGE_ITEM_COUNT) / MENU_INVENTORY_ROW_ITEM_COUNT + 1;
        s32 q = 0x1068 / n;

        g_MenuInventoryWindowHeight = 0x4A;
        g_MenuInventoryRowCount = n;
        g_MenuInventoryScrollStep = q;
    }
}


/* Best: 93.91%, 888 bytes (retail 896) - 222 vs 224 insns, 2 short. Everything else
   is retail's: the prologue save order, the 8 switch arms, the case listing order
   (4, 5, 1, 3, 0, 2, 9, 10 in the source, which is retail's body order - ascending
   gives 67%), every store and every delay slot except the two below. Decode: the
   inventory panel cursor, 16 columns x g_MenuInventoryRowCount rows; col/row are s32, prevCol,
   prevRow and selectedSlot start at 0xFF, needsOpen/active are u8.
   Delta 1 (dispatch, +1 insn in retail): retail's `beqz v0,X` duplicates the loop
   test's `andi v0,s6,0xFF` into its delay slot and targets the `bnez`; ours targets
   the andi block and fills the slot with `sll v0,v1,2`.
   Delta 2 (+1 insn in retail): retail's shared `selectedSlot = 0xFF` tail
   (jtbl case 4 success + case 5 else) sits AFTER case 5's body, reached by a `j`
   from the s2 store block plus case 5's `bne`; ours merges the same two stores into
   one block that sits right after case 4's else and falls through, putting
   `ori s3,0xFF` in a `j` delay slot. Same instructions, one fewer.
   Both are gcc finding the shorter encoding; not moveable by source order (tried:
   store order inside each arm, selectedSlot before/after the func_801DB920 call,
   case 5 if/else inverted, do-while vs while, `while (active)` vs `!= 0`).
   JUMP TABLE: all three misc9 tables are 11 words plus one pad word, and the range
   0xFC..0x18C is exactly 144 bytes, so a C switch's 44-byte table does NOT shift
   the overlay - the next stub's table starts with its own `.align 3`, which supplies
   the pad (misc10's two converted 10-case tables are 0x50 = 2 x 40, no pad, and
   `make check` passes). So JTBL_LAYOUT is probably not the blocker here, the 2
   insns are - but that is inferred from misc10, not measured for a 44-byte table:
   converting one of these and running check_bin.sh menu settles it in a minute.
   func_801DB920 returns u_char (retail does `andi v0,v0,0xFF` after the call).
   permuter --minutes 5 -j 2 from this body: PERMUTER_BETTER 1337 -> 1257, but the
   winner is an inline_fn helper (not portable); no lead on the two deltas.
   Round 176: re-measured 93.91%, 888 bytes against retail 896 (222 vs 224 insns),
   no source change closes either delta; both are gcc emitting the shorter encoding
   (the dispatch `beqz` duplicates the loop test's andi in retail, and retail keeps
   two `selectedSlot = 0xFF` store sites where gcc 2.6.0 cross-jumps them into one).
   Round 231: still 93.91% / 888. Tried, no change: `default: continue;`, `while
   (active != 0)`, chained `prevRow = prevCol = 0xFF;`, moving the
   func_801DB0A8(selectedSlot, row, 1, 1) call after the switch (89.26), moving
   `selectedSlot = 0xFF` INTO the func_801DB920 if-body (92.48), and writing that
   store both inside the body and after it (93.91 - gcc CSEs the pair). The delay
   slot andi is reorg duplicating the shared loop-test block, so no statement adds
   it, and the 1 extra instruction is not reachable from the source. Preset is
   right: 2.7.2 gives 93.66, cdk 84.82, -O3 65.55.
   FAMILY LAYOUT, measured this round: a C jump table is 0x2C bytes with `.align 3`,
   so converted tables land at range +0x00/+0x30/+0x60 and the LAST one ends at
   +0x8C - 4 bytes short of the 0x90 range, because the trailing pad word exists
   only in the asm stub (jtbl_801C515C carries a 12th zero word). So the family
   can NOT be converted whole: convert THIS function and func_801DDF24 (C tables
   at +0x00 and +0x30) and leave func_801E05D0 as INCLUDE_ASM at +0x60, which is
   exact. */

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DBE54);
/*
s32 func_801DBE54(void) {
    u8 active;
    u8 needsOpen;
    s32 row;
    s32 prevRow;
    s32 col;
    s32 prevCol;
    s32 selectedSlot;

    active = 1;
    needsOpen = 1;
    row = 0;
    prevRow = 0xFF;
    col = 0;
    prevCol = 0xFF;
    selectedSlot = 0xFF;
    MenuInitializeUnk42CPanel();
    MenuFitInventoryWindowToItems();
    MenuInitializeArrowCursor(0);
    MenuInitializeArrowCursor(1);
    do {
        MenuRenderFrame();
        if (row != prevRow) {
            func_801DA5BC(row);
            MenuCreateScrollHandle(0xC, g_MenuInventoryScrollStep * row / 100 + 0x12, g_MenuInventoryWindowHeight);
            prevRow = row;
        }
        func_801DB0A8(col, row, 0, 0);
        if (col != prevCol) {
            func_801DA9A8(col, row);
            prevCol = col;
        }
        if (needsOpen) {
            MenuCreateWindow(3, 0xC, 0xA, 0x124, 0x84, 0, 1, 4, 1);
            MenuCreateWindow(4, 8, 0x8F, 0x130, 0x22, 0, 1, 4, 0);
            MenuStartOpenAnimation();
            needsOpen = 0;
            func_801D29A8(0, 0);
        }
        func_801DB0A8(selectedSlot, row, 1, 1);
        switch (g_Menu->input) {
        case 4:
            if (selectedSlot == 0xFF) {
                selectedSlot = row * 2 + col;
            } else if (row * 2 + col == selectedSlot) {
                if (func_801DB920(row, col) != 0) {
                    prevRow = 0xFF;
                    prevCol = 0xFF;
                }
                selectedSlot = 0xFF;
            } else {
                MenuSwapInventoryItems(row * 2 + col, selectedSlot);
                prevRow = 0xFF;
                prevCol = 0xFF;
                selectedSlot = 0xFF;
            }
            break;
        case 5:
            if (selectedSlot != 0xFF) {
                selectedSlot = 0xFF;
            } else {
                active = 0;
            }
            break;
        case 1:
            if (col + 2 < 0x10) {
                col = col + 2;
            } else {
                row++;
                if (row > g_MenuInventoryRowCount) {
                    row--;
                }
            }
            prevCol = 0xFF;
            break;
        case 3:
            if (col - 2 >= 0) {
                col = col - 2;
            } else {
                row--;
                if (row < 0) {
                    row++;
                }
            }
            prevCol = 0xFF;
            break;
        case 0:
            if (col + 1 < 0x10) {
                col = col + 1;
            } else {
                row++;
                if (row > g_MenuInventoryRowCount) {
                    row--;
                } else {
                    col = 0xE;
                }
            }
            prevCol = 0xFF;
            break;
        case 2:
            if (col - 1 >= 0) {
                col = col - 1;
            } else {
                row--;
                if (row < 0) {
                    row++;
                } else {
                    col = 1;
                }
            }
            prevCol = 0xFF;
            break;
        case 9:
            row += 8;
            if (row > g_MenuInventoryRowCount) {
                row = g_MenuInventoryRowCount;
            }
            prevCol = 0xFF;
            break;
        case 10:
            row -= 8;
            if (row < 0) {
                row = 0;
            }
            prevCol = 0xFF;
            break;
        }
    } while (active);
    MenuHidePointerCursors();
    MenuFreeArrowCursor(0);
    MenuFreeArrowCursor(1);
    MenuClearBytes(8, g_Menu->pManager->shouldRenderStrings10E0);
    return 1;
}
*/


void MenuCreateUnk430Panel(u_char arg0) {
    s32 kind;
    s32 index = 0;

    g_Menu->unk430 = HeapAlloc(sizeof(MenuUnk430Panel), 0);
    bzero(g_Menu->unk430, sizeof(MenuUnk430Panel));
    switch (arg0) {
    case 0:
        kind = 2;
        break;
    case 1:
        kind = 5;
        break;
    case 2:
        kind = 6;
        index = 1;
        break;
    }
    MenuLoadStringsFromIds(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuItemRowStringIds[index], g_Menu->pManager->shouldRenderStrings10E0);
    func_801C72BC((u8)kind);
    MenuSetupPointerCursors(2);
    MenuCreateShoulderButtonUi(2, arg0);
}

void MenuFreeUnk430Panel(u_char arg0) {
    s32 kind;

    MenuFreeWindow(3);
    MenuFreeWindow(4);
    MenuFreeWindow(5);
    MenuFreeWindow(6);
    MenuHidePointerCursors();
    g_Menu->pManager->shouldRenderPanel[0] = 0;
    MenuRenderFrame();
    switch (arg0 & 0xFF) {
    case 0:
        kind = 2;
        break;
    case 1:
        kind = 5;
        break;
    case 2:
        kind = 6;
        break;
    }
    func_801C72BC((kind | MENU_PANEL_FREE_OFFSET) & 0xFF);
    HeapFree(g_Menu->unk430);
    g_Menu->pManager->playTimeDigitsActive = 1;
    g_Menu->pManager->shouldRenderWindow[1] = 1;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DC3D8);
/* Best: 97.64%, 1928 bytes (retail 1924 = 481 insns, ours 482). Fills one column of
   the 0x430 panel: renders the column's two heading strings into a scratch buffer,
   uploads them, points the four tail sprite records at them, copies the selected
   column's record into MENU_PANEL_COLUMN_STRING and sets the row flags the prim
   adder in misc5 renders. arg2 selects the panel kind (item / gear list / gear two
   columns) and with it the string id base and the dressing room prim block.
   Confirmed retail's: prologue and save set, frame 0x48, both switch trees, every
   call site and store, the shared exits, the 0x4E/0xD constants hoisted to $s0/$s1.
   Two edits got it from ~91% to here:
   - arg1 must be declared s32, not u_char: as u_char cc1 emits a `move a0,a1`
     parameter copy and masks that copy where retail masks $a1 in place, which also
     puts the arms' `(arg1 & 0xFF)` masks back (97.27 -> 97.64).
   - `(u32)(arg1 & 0xFF) >> 1` for the two-column prim offset: the signed shift
     gives `sra` where retail has `srl`.
   Three deltas left (permute.sh --minutes 4 -j 2: PERMUTER_BETTER 1751 -> 1210, the
   winner is an inline_fn helper, not portable):
   1. kind 2's shared `addu s0`: retail `addu s0,v0,v1` (the gearId << 3 side as the
      destination), ours `addu s0,v1,v0`. Kinds 0 and 1 match. Tried both operand
      orders, `gearId * 8` for `<< 3`, and an (arg1 & 0xFE)-first sum: cc1
      canonicalises to offset-first every time.
   2. the row flag: retail `andi 0x1000 / sltiu / move`, ours `srl 0xc / xori / andi`.
      Tried `== 0`, `!(...)`, `< 1`, a u_int flags local and a (u16) cast on the masked
      value: cc1 folds all of them to the bit-test form. Same instruction count.
   3. tail: g_MenuColumnRowStringIds, g_MenuPanelRowXOffsets and the literal 2 land in $s1/$s2/$s0
      where retail has $s0/$s1/$s2 (cyclic rotation), and the second
      MenuSetRowQuadVertices call's setup is scheduled around the first call's tail -
      that is the one extra instruction.
   Realism owed: `flags = *(u16*)pBase` and `(*pBase & 3)` are raw reads of a prim
   block. The panel itself is typed by MenuUnk430Panel above. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DCE60);
/*
void func_801DCE60(u_char arg0, s32 arg1, u_char arg2) {
    u8* pImage;
    u8* pBase;
    RECT rect;
    u_int flags;
    u16 stringId;
    s32 row;
    s32 i;

    switch (arg2) {
    case 0:
        stringId = (g_Menu->pManager->currentCharacterIDs[arg0] << 5) + (arg1 & 0xFF) * 2;
        break;
    case 1:
        stringId = (g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId << 5) + (arg1 & 0xFF) * 2;
        break;
    case 2:
        stringId = (arg1 & 0xFE) + g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId * 8;
        break;
    }
    if (((MenuUnk430Panel*)g_Menu->unk430)->columnFlags[arg1 & 0xFF] != 0) {
        pImage = HeapAlloc(MENU_COLUMN_IMAGE_SIZE, 0);
        bzero(pImage, MENU_COLUMN_IMAGE_SIZE);
        ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].width =
            SystemRenderStringEntry(GetStringEntry(((MenuUnk430Panel*)g_Menu->unk430)->pStringData, stringId), pImage, 0x39, 0);
        ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING + 1].width =
            SystemRenderStringEntry(GetStringEntry(((MenuUnk430Panel*)g_Menu->unk430)->pStringData, stringId + 1), pImage, 0x39, 1);
        rect.x = 0x140;
        rect.y = 0x4E;
        rect.w = 0x3C;
        rect.h = 0xD;
        LoadImage(&rect, pImage);
        DrawSync(0);
        func_801E7C50(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING], 0, 0, 0);
        MenuSetFt4QuadVertices(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].polys[g_Menu->renderContext],
                               0x1C, 0x9E, 0, 0x4E,
                               ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].width, 0xD);
        MenuSetQuadVertices(((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].vertices, 0x1C, 0x9E,
                            ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].width, 0xD);
        func_801E7C50(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING + 1], 1, 0, 0);
        MenuSetFt4QuadVertices(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING + 1].polys[g_Menu->renderContext],
                               0x1C, 0xAE, 0, 0x4E,
                               ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING + 1].width, 0xD);
        MenuSetQuadVertices(((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING + 1].vertices, 0x1C, 0xAE,
                            ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING + 1].width, 0xD);
        HeapFree(pImage);
        memmove(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING],
                &((MenuUnk430Panel*)g_Menu->unk430)->strings[arg1 & 0xFF], sizeof(MenuString));
        MenuSetQuadVertices(((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING].vertices, 0x12, 0x8E,
                            ((MenuUnk430Panel*)g_Menu->unk430)->strings[arg1 & 0xFF].width, 0xD);
        ((P_TAG*)&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING].polys[g_Menu->renderContext])->r0 = 0x80;
        ((P_TAG*)&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING].polys[g_Menu->renderContext])->g0 = 0x80;
        ((P_TAG*)&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING].polys[g_Menu->renderContext])->b0 = 0x80;
        SetSemiTrans(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING].polys[g_Menu->renderContext], 0);
        MenuClearBytes(MENU_ITEM_ROW_STRING_COUNT, g_Menu->pManager->shouldRenderStrings10E0);
        switch (arg2) {
        case 0:
            pBase = (u8*)g_Menu->pDressingRoom->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[arg0]] + (arg1 & 0xFF) * 40 + 0x370;
            break;
        case 1:
            pBase = (u8*)g_Menu->pDressingRoom->pGearBlocks[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId] + (arg1 & 0xFF) * 40 + 0x348;
            break;
        case 2:
            pBase = (u8*)g_Menu->pDressingRoom->pGearBlocks[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId] + ((u32)(arg1 & 0xFF) >> 1) * 40 + 0x5C8;
            break;
        }
        flags = *(u16*)pBase;
        if (flags & 0x4000) {
            row = 2;
        } else {
            row = (flags & 0x1000) == 0;
        }
        MenuSetRowQuadVertices(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuColumnRowStringIds, g_MenuPanelRowXOffsets,
                               g_Menu->pManager->shouldRenderStrings10E0, row, 0, 2);
        MenuSetRowQuadVertices(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuColumnRowStringIds, g_MenuPanelRowXOffsets,
                               g_Menu->pManager->shouldRenderStrings10E0, (*pBase & 3) + 3, 0, 2);
        ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING].renderContext = g_Menu->renderContext;
        ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING + 1].renderContext = g_Menu->renderContext;
        ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING + 2].renderContext = g_Menu->renderContext;
        ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_COLUMN_STRING + 3].renderContext = g_Menu->renderContext;
        ((MenuUnk430Panel*)g_Menu->unk430)->shouldRenderColumnHeadings = 1;
        g_Menu->pManager->shouldRenderStrings10E0[6] = 1;
        g_Menu->pManager->shouldRenderStrings10E0[7] = 1;
    } else {
        ((MenuUnk430Panel*)g_Menu->unk430)->shouldRenderColumnHeadings = 0;
        for (i = 0; i < 6; i++) {
            g_Menu->pManager->shouldRenderStrings10E0[i] = 0;
        }
    }
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD5E8);
/* Best: 99.58% at retail's exact size (424 bytes = 106 insns; the older 416-byte
   note here was wrong by two instructions). Six instructions differ, all
   register/operand ties:
   1. loop 1, second call site: retail's renderContext load address is
      `addu v0,v1,s0` (base first), ours `addu v0,s0,v1` (offset first).
   2. loop 2 preheader/body: retail puts g_Menu and the panel base in $v0 and the
      i*0x80 offset in $v1; ours has the pair mirrored. The instruction shapes and
      the order are identical, only those two scratch registers are swapped.
   What got it here from 94.01 (round 293's park):
   - retail has NO panel pointer local: every one of the six sites reads
     g_Menu->unk430 fresh (`lui/lw g_Menu` + `lw 0x430`), so a `MenuString*
     pStrings` local is wrong even though it looks tidier - inlining the global
     is what matched the register roles (94.01 -> 99.48).
   - the compare address is spelled with the two offsets summed first,
     `*(u8*)((u8*)g_Menu->unk430 + (i * 0x80 + rc * 0x28) + 4)`, which is the one
     spelling that reproduces retail's two-addu address (base untouched) while the
     call argument reuses `(base + i*0x80) + rc*0x28`; every typed spelling CSEs
     both uses into one address.
   - the renderContext load needs the panel base in a local for that site only
     (`pPanel = g_Menu->unk430;` then `&((MenuString*)pPanel)[i]`), which is what
     turns `addu a2,s0,a0` into retail's `addu a2,a0,s0` (99.48 -> 99.58).
   permute.sh --minutes 4 -j 2 (round 294, from this body) found exactly that last
   edit (base=55 -> best=45); the other two runs (rounds 63, 293) found only a
   non-portable winner that hoists g_Menu into a local and changes the size.
   NOT realistic C yet: item 1's spelling is a raw byte cast. */
/*
void func_801DD5E8(u_char arg0) {
    MenuString* pString;
    POLY_FT4* pPoly;
    void* pPanel;
    s32 i;
    s32 rc;

    func_801E8F60(5, arg0);
    func_801E8F60(6, arg0);
    for (i = 0; i < 0xC; i++) {
        pPanel = g_Menu->unk430;
        pString = &((MenuString*)pPanel)[i];
        rc = pString->renderContext;
        if (*(u8*)((u8*)g_Menu->unk430 + (i * 0x80 + rc * 0x28) + 4) != 0x20) {
            MenuSetPrimShadingMode(&((MenuString*)g_Menu->unk430)[i].polys[rc], arg0);
            pPoly = &((MenuString*)g_Menu->unk430)[i + 0xE].polys[((MenuString*)g_Menu->unk430)[i + 0xE].renderContext];
            MenuSetPrimShadingMode(pPoly, arg0);
        }
    }
    MenuSetPrimShadingMode(&g_Menu->arrowCursors[0]->polys[g_Menu->arrowCursors[0]->renderContext], arg0);
    MenuSetPrimShadingMode(&((MenuString*)g_Menu->unk430)[0x1C].polys[((MenuString*)g_Menu->unk430)[0x1C].renderContext], arg0);
    for (i = 0; i < 2; i++) {
        MenuSetPrimShadingMode(&((MenuString*)g_Menu->unk430)[0x1E + i].polys[((MenuString*)g_Menu->unk430)[0x1E + i].renderContext], arg0);
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD790);

/* Best: 94.46%, 884 bytes - retail's exact size, 221 insns each. The whole body is
   retail's: prologue, loop, all 8 switch arms in retail's body order (4, 5, 0, 2, 1,
   3, 9, 10 - ascending gives ~67%), every call, store and delay slot, and the loop
   test at the bottom. One root cause is left, and everything else in the diff is its
   cascade: `needsOpen` (u8, retail `$s5`, ours spilled to 0x38($sp), which is why the
   frame is 0x70 here against retail's 0x68). gcc 2.6.0 hoists the literal `1` (the
   7th MenuCreateWindow argument, 4 uses inside the loop) into a saved register and
   rematerialises the `4` (8th argument) in another - retail does the reverse: `4` in
   `$s8`/fp, `1` re-loaded into `$t0` before each of the 4 calls, and `$s5` free for
   needsOpen. Same phenomenon in func_801DBE54's matching prologue, where BOTH
   constants do get hoisted (retail `$s7`=1, `$s8`=4): that unit has one live value
   fewer, so the tie goes the other way. Tried: typed MenuUnk430Panel view for the
   case-4 flag byte (fixed that addu's operand order, 94.42 -> 94.46), and moving the
   `needsOpen = 1` init after the two prologue calls (93.31, worse).
   Round 176: permute.sh --minutes 4 -j 2 from this body: PERMUTER_BETTER 4368 ->
   375, but the winner assigns `needsOpen = D_801E97A0[kind][0]` before the window-4
   call - WRONG CODE (it changes the argument AND kills needsOpen's live range), so
   not portable. Also measured: moving `active = 1` after `kind = arg2` (retail's
   prologue order) 93.81; `needsOpen = 1` first 94.46 (identical body); a preheader
   `s32 mode = 4;` passed as the a7 argument 94.81 but 888 bytes, 4 over retail; a
   `u_char border = 1;` passed as a6 94.46, identical. The root cause is unchanged
   and is the allocator's, not the source's: ours hoists BOTH the a6=1 and the a7=4
   into $s8/$s7, retail rematerialises the 1 per call (`li $t0,1`) and keeps 4 in
   $fp, which is what leaves $s5 free for needsOpen. Every spelling of the a6
   argument folds to one (const_int 1) pseudo with four refs, so no source shape
   changes which constant wins the register.
   Round 231, register map read off the diff (retail frame 0x68, ours 0x70): retail's
   nine callee-saved values are s0=kind, s1=col, s2=slot, s3=arg2, s4=prevCol,
   s5=needsOpen, s6=prevSlot, s7=kind*4, fp=4, and it spills `kind` to 0x38($sp)
   while rematerialising the a6=1 into $t0 before each of the four calls. Ours hoists
   that 1 into $s8 (the extra save, hence 0x70) and spills needsOpen to the same
   0x38($sp) slot. So the tie is exactly `const 1` vs `needsOpen` for the ninth
   register, and declaration order does not move it: needsOpen first, active last,
   slot first and kind first all measure 94.45-94.46.
   JUMP TABLE: CORRECTION (round 293) - this stub owns NO table (its .s has no
   `jtbl`/`.rodata` reference), so it is a convertible stub; the note below had
   misattributed jtbl_801C512C to it. The unit's three tables belong to
   func_801DBE54 (+0x00 of the [0xFC, .rodata, main/misc9] range), func_801DDF24
   (+0x30) and func_801E05D0 (+0x60); func_801DBE54's switch is already C and the
   third must STAY asm (all three as C is 4 bytes short of the 0x90 range), so
   func_801DDF24 + func_801E05D0 cannot be converted in this unit. Also: this
   stub's parked body is no longer in the file, so it has to be rewritten from the
   assembly - the round-231 note below still describes its residue. `case 4`'s flag byte is g_Menu->unk430 + 0x1084 + col
   (the buffer is the 0x1094 bytes MenuCreateUnk430Panel allocates); the three window
   dimensions are D_801E9788/94/A0[kind][0], u16 tables 0xC bytes apart, stride 4
   (282/282/162, 218/218/200, 82/82/100 - one row per character slot). `kind` is the
   s32 arg2 masked to a byte; cases 0 and 2 are disabled when it is 2. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DDF24);
/*
void func_801DDF24(u_char arg0, u_char arg1, s32 arg2) {
    u8 active;
    u8 needsOpen;
    s32 col;
    s32 prevCol;
    u8 slot;
    u8 prevSlot;
    u8 kind;

    active = 1;
    needsOpen = 1;
    col = 0;
    prevCol = 0xFF;
    slot = arg0;
    prevSlot = 0xFF;
    kind = arg2;
    MenuCreateUnk430Panel(kind);
    MenuInitializeArrowCursor(0);
    do {
        MenuRenderFrame();
        if (slot != prevSlot) {
            func_801DC3D8(slot, arg2);
            prevSlot = slot;
            prevCol = 0xFF;
        }
        func_801DB0A8(col, 0, 2, 0);
        if (col != prevCol) {
            func_801DCE60(slot, col, arg2);
            prevCol = col;
        }
        if (needsOpen) {
            MenuCreateWindow(6, 0x10, 0xA, D_801E9788[kind][0], 0x70, 0, 1, 4, 0);
            MenuCreateWindow(5, 0xC, 0x86, 0xAC, 0x38, 0, 1, 4, 0);
            MenuCreateWindow(4, D_801E9794[kind][0], 0xA6, D_801E97A0[kind][0], 0x18, 0, 1, 4, 0);
            MenuCreateWindow(3, 0xC8, 0x86, 0x50, 0x18, 0, 1, 4, 0);
            needsOpen = 0;
            if (arg1) {
                MenuStartOpenAnimation();
                func_801D29A8(0, 0);
                MenuRenderFrame();
            }
            g_Menu->pManager->playTimeDigitsActive = 0;
            g_Menu->pManager->shouldRenderWindow[1] = 0;
        }
        switch (g_Menu->input) {
        case 4:
            if (((MenuUnk430Panel*)g_Menu->unk430)->unk1084[col] & 0x80) {
                func_801DD790(slot, col, arg2);
                prevSlot = 0xFF;
                prevCol = 0xFF;
            }
            break;
        case 5:
            active = 0;
            break;
        case 0:
            if (kind != 2) {
                col++;
                if (col >= 0xC) {
                    col = 0xB;
                }
            }
            break;
        case 2:
            if (kind != 2) {
                col--;
                if (col < 0) {
                    col = 0;
                }
            }
            break;
        case 1:
            if (col + 2 < 0xC) {
                col += 2;
            }
            break;
        case 3:
            if (col - 2 >= 0) {
                col -= 2;
            }
            break;
        case 9:
            slot = func_801D9704(slot, 0, arg2);
            break;
        case 10:
            slot = func_801D9704(slot, 1, arg2);
            break;
        }
    } while (active);
    MenuClearBytes(8, g_Menu->pManager->shouldRenderStrings10E0);
    MenuFreeArrowCursor(0);
}
*/

s32 MenuRunUnk430Panel(u_char arg0, u_char arg1) {
    func_801DDF24(arg0, arg1, 0);
    return 1;
}

void MenuInitializeCharacterSpriteRecords(u32 arg0) {
    MenuManager* pManager;

    g_Menu->pCharacterSpriteRecords = HeapAlloc(sizeof(MenuCharacterSpriteRecords), 0);
    bzero(g_Menu->pCharacterSpriteRecords, sizeof(MenuCharacterSpriteRecords));
    pManager = g_Menu->pManager;
    MenuLoadStringsFromIds(MENU_CHARACTER_ROW_STRING_COUNT, g_Menu->characterRowStrings, g_MenuCharacterRowStringIds[arg0 & 0xFF], pManager->shouldRenderStrings14E0);
    func_801C72BC(7);
    MenuSetupPointerCursors(3);
    MenuInitializeArrowCursor(0);
    MenuCreateShoulderButtonUi(1, arg0 & 0xFF);
}

void MenuFreeCharacterSpriteRecords(void) {
    g_Menu->pManager->shouldRenderPanel[2] = 0;
    MenuFreeWindow(2);
    MenuFreeWindow(3);
    MenuFreeWindow(4);
    MenuFreeWindow(5);
    MenuRenderFrame();
    MenuClearBytes(MENU_CHARACTER_ROW_STRING_COUNT, &g_Menu->pManager->shouldRenderStrings10E0[8]);
    func_801C72BC(7 | MENU_PANEL_FREE_OFFSET);
    HeapFree(g_Menu->pCharacterSpriteRecords);
    MenuFreeScrollHandle();
}

void MenuFreeUnk35CPanel(void) {
    g_Menu->pManager->shouldRenderUnk35C = 0;
    g_Menu->pManager->shouldRenderPanel[1] = 0;
    MenuRenderFrame();
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->pEquipmentPanel);
}

void MenuCreateListWindow(u_char arg0, u_char arg1) {
    s32 i;
    s32 start;
    s32 count;
    s32 label;

    for (i = 0; i < MENU_CHARACTER_ROW_STRING_COUNT; i++) {
        g_Menu->pManager->shouldRenderStrings14E0[i] = 0;
    }

    if (arg0 != 0) {
        start = 2;
        count = MENU_CHARACTER_ROW_STRING_COUNT;
        label = 0x72;
    } else {
        start = 0;
        count = 2;
        label = 0x5A;
    }

    for (i = start; i < count; i++) {
        MenuSetRowQuadVertices(MENU_CHARACTER_ROW_STRING_COUNT, g_Menu->characterRowStrings, g_MenuCharacterRowStringIds[arg1 & 0xFF], g_MenuPanelRowXOffsets,
                      g_Menu->pManager->shouldRenderStrings14E0, (u8)i, (u8)i, 3);
    }

    if (g_Menu->pManager->shouldRenderWindow[4] != 0) {
        MenuFreeWindow(4);
    }

    MenuCreateWindow(4, 0x10, 0xC, 0x80, label, 0, 1, 4, 0);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DE5CC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DF0D4);
/* Best: 77.65%, 1264 bytes (retail 1236). Every block below is
   instruction-identical to retail except for register allocation, and these
   are the remaining deltas, all measured this round:
   1. FRAME: retail `.frame $sp,40` (vars=40, no stack traffic at all), this
      body gives vars=24. Four more locals that never get a register are
      needed. Related: retail opens with `addu a0,a1,zero` (arg1 moved into
      a0 and masked at each use) and keeps arg0 in t0; here arg1 stays in a1.
      Both are the same global-allocation decision. Try adding the extra
      candidates (a per-branch pCharacter/pGear pointer, a pPanel pointer, a
      separate u_char for the clamped value) until vars hits 40.
   2. ARG3 SELECT LAYOUT: retail emits `t2 = gearWeaponsIds` inline, then
      `bnez arg3, join`, then `t2 = weaponIDs` in the branch (default +
      override). Writing the pIds/pQuantities pair as if/else gets the
      computation right (single `addiu t3,t2,-100` after the join, which is
      correct because both inventory pairs are 0x64 apart) but gcc inlines
      the weaponIDs arm and branches with beqz instead. Default+override for
      BOTH pointers makes gcc pre-compute `gearWeaponsIds-0x900` (weaponIDs)
      out of line instead, which is worse.
   3. P4/P6 ANCHOR: retail anchors the accessory blocks on %hi(g_GameState
      + 0x2DF) (t2 = v0+0x1BE9, t3 = v0+0x1B21, and %lo(g_GameState+0x2DF)
      ($at) for the slot load); this body anchors on accessoryIDs and reaches
      the slot with `lbu -5448(reg)`. Retail materialises the slot address
      first, so try putting the slotItem statement before the
      pIds/pQuantities/count statements in those two blocks.
   4. `if (updated == 0) return result;` costs one extra `j`: retail emits
      `beqz v0,<return>` (return block out of line), here gcc inverts to
      `bnez v0,<scan>` + `j <return>`.
   5. Panel address: retail has base-first `addu v0,v0,a0`; every spelling
      tried here gives index-first `addu a0,a0,v0`. The embedded-assignment
      lever (a pointer local assigned `(u8*)g_Menu->pEquipmentPanel + arg1`, used once
      with [0x2A1]) was tried this round and dropped the score to 73.17%,
      so it is the wrong lever here.
   The `lbu` reload of the slot item in the clamp (retail re-reads it, so the
   clamp must index T through a pointer deref, not a local) already matches.
   gcc 2.6.0 emits switch-free straight-line code throughout; there is no
   jump table in this stub.
   Semantics recovered: arg2 != 0 selects the 0x2A1 slot group (characters
   .extraWeaponIds when arg3 == 0, gears .extraWeaponIds otherwise, both
   weapons inventory); arg2 == 0 selects the 0x29C/0x2A6 groups (arg1 == 0 is
   the primary weapon slot, characters.weaponId / gears.weaponId, weapons
   inventory; arg1 != 0 is accessory slot arg1 - 1, characters.accessorySlots /
   gears.accessorySlots,
   accessories inventory). Only the arg2 != 0 blocks clamp a per-item-id byte
   at g_GameState+0x2286 (characters) / +0x22B6 (gears) to 100 and zero the
   incoming item when the old value was below 100. The tail then walks the
   inventory: decrement the removed id, increment the arriving id (or take
   the first free slot with quantity 1), then clamp every quantity (0 clears
   the id, >= 100 becomes 99).
   u_char func_801DF0D4(u_char arg0, u_char arg1, u_char arg2, u_char arg3) {
       u8* pIds;
       u8* pQuantities;
       u8* pSlot;
       s32 characterId;
       s32 gearId;
       u_char slotItem;
       u_char newItem;
       u_char added;
       u_char updated;
       u_char result;
       s32 count;
       s32 i;

       added = 1;
       updated = 1;
       result = 0;
       if (arg3 != 0) {
           pIds = g_GameState.gearWeaponsIds;
           pQuantities = g_GameState.gearWeaponsQty;
       } else {
           pIds = g_GameState.weaponIDs;
           pQuantities = g_GameState.weaponQuantities;
       }
       count = MAX_INVENTORY_WEAPONS;

       if (arg2 != 0) {
           if (arg3 == 0) {
               characterId = g_Menu->pManager->currentCharacterIDs[arg0];
               pSlot = &g_GameState.characters[characterId].extraWeaponIds[arg1];
               slotItem = *pSlot;
               newItem = ((u8*)g_Menu->pEquipmentPanel)[0x2A1 + arg1];
               if (slotItem != 0) {
                   if (((u8*)&g_GameState)[0x2286 + *pSlot] < MAX_INVENTORY_WEAPONS) {
                       newItem = 0;
                   }
                   ((u8*)&g_GameState)[0x2286 + *pSlot] = MAX_INVENTORY_WEAPONS;
               } else {
                   *pSlot = newItem;
                   updated = 0;
               }
           } else {
               characterId = g_Menu->pManager->currentCharacterIDs[arg0];
               gearId = g_GameState.characters[characterId].gearId;
               pSlot = &g_GameState.gears[gearId].extraWeaponIds[arg1];
               slotItem = *pSlot;
               newItem = ((u8*)g_Menu->pEquipmentPanel)[0x2A1 + arg1];
               if (slotItem != 0) {
                   if (((u8*)&g_GameState)[0x22B6 + *pSlot] < MAX_INVENTORY_WEAPONS) {
                       newItem = 0;
                   }
                   ((u8*)&g_GameState)[0x22B6 + *pSlot] = MAX_INVENTORY_WEAPONS;
               } else {
                   *pSlot = newItem;
                   updated = 0;
               }
           }
       } else if (arg3 == 0) {
           if (arg1 == 0) {
               characterId = g_Menu->pManager->currentCharacterIDs[arg0];
               slotItem = g_GameState.characters[characterId].weaponId[0];
               newItem = ((u8*)g_Menu->pEquipmentPanel)[0x29C];
               if (slotItem == 0) {
                   g_GameState.characters[characterId].weaponId[0] = newItem;
                   updated = 0;
               } else if (characterId == 4) {
                   result = 1;
               }
           } else {
               characterId = g_Menu->pManager->currentCharacterIDs[arg0];
               pIds = g_GameState.accessoryIDs;
               pQuantities = g_GameState.accessoryQuantities;
               count = MAX_INVENTORY_ACCESSORIES;
               slotItem = g_GameState.characters[characterId].accessorySlots[arg1 - 1];
               newItem = ((u8*)g_Menu->pEquipmentPanel + (arg1 - 1))[0x2A6];
           }
       } else {
           if (arg1 == 0) {
               characterId = g_Menu->pManager->currentCharacterIDs[arg0];
               gearId = g_GameState.characters[characterId].gearId;
               slotItem = g_GameState.gears[gearId].weaponId[0];
               newItem = ((u8*)g_Menu->pEquipmentPanel)[0x29C];
               if (slotItem == 0) {
                   g_GameState.gears[gearId].weaponId[0] = newItem;
                   updated = 0;
               } else if (characterId == 4) {
                   result = 1;
               }
           } else {
               characterId = g_Menu->pManager->currentCharacterIDs[arg0];
               pIds = g_GameState.accessoryIDs;
               pQuantities = g_GameState.accessoryQuantities;
               count = MAX_INVENTORY_ACCESSORIES;
               gearId = g_GameState.characters[characterId].gearId;
               slotItem = g_GameState.gears[gearId].accessorySlots[arg1 - 1];
               newItem = ((u8*)g_Menu->pEquipmentPanel + (arg1 - 1))[0x2A6];
           }
       }

       if (updated == 0) {
           return result;
       }
       if (slotItem != 0) {
           for (i = 0; i < count; i++) {
               if (pIds[i] == slotItem) {
                   pQuantities[i]--;
                   break;
               }
           }
       }
       if (newItem != 0) {
           for (i = 0; i < count; i++) {
               if (pIds[i] == newItem) {
                   pQuantities[i]++;
                   added = 0;
                   break;
               }
           }
       }
       if (added != 0) {
           for (i = 0; i < count; i++) {
               if (pIds[i] == 0) {
                   pIds[i] = newItem;
                   pQuantities[i] = 1;
                   break;
               }
           }
       }
       for (i = 0; i < count; i++) {
           if (pQuantities[i] == 0) {
               pIds[i] = 0;
           } else if (pQuantities[i] >= MAX_INVENTORY_WEAPONS) {
               pQuantities[i] = MAX_ITEM_QUANTITY;
           }
       }
       return result;
   }
*/
void MenuCopyEquipmentToPanel(u_char arg0, u_char arg1) {
    s32 i;

    for (i = 0; i < MENU_STAT_VALUE_COUNT; i++) {
        ((u16*)((u8*)g_Menu->pEquipmentPanel + 0x280))[i] = g_Menu->pDressingRoom->statValues[i];
    }

    if (arg1 == 0) {
        for (i = 0; i < MENU_CHARACTER_WEAPON_SLOT_COUNT; i++) {
            ((u8*)g_Menu->pEquipmentPanel + i)[0x29C] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].weaponId[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[0x2A1] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].extraWeaponIds[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[0x2A6] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].accessorySlots[i];
        }
    } else {
        for (i = 0; i < MENU_GEAR_WEAPON_SLOT_COUNT; i++) {
            ((u8*)g_Menu->pEquipmentPanel + i)[0x29C] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].weaponId[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[0x2A1] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].extraWeaponIds[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[0x2A6] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].accessorySlots[i];
        }
    }
}

void MenuCopyEquipmentFromPanel(u_char arg0, u_char arg1) {
    s32 i;

    if (arg1 == 0) {
        for (i = 0; i < MENU_CHARACTER_WEAPON_SLOT_COUNT; i++) {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].weaponId[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x29C];
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].extraWeaponIds[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x2A1];
        }
        for (i = 0; i < MENU_ACCESSORY_SLOT_COUNT; i++) {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].accessorySlots[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x2A6];
        }
    } else {
        for (i = 0; i < MENU_GEAR_WEAPON_SLOT_COUNT; i++) {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].weaponId[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x29C];
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].extraWeaponIds[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x2A1];
        }
        for (i = 0; i < MENU_ACCESSORY_SLOT_COUNT; i++) {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].accessorySlots[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x2A6];
        }
    }
}
void MenuSetEquipmentSlotValue(u_char arg0, s32 arg1, s32 arg2, s32 arg3, u_char arg4, u_char arg5) {
    if (arg5 == 0) {
        if (arg4 == 0) {
            if (arg1 == 0) {
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].weaponId[0] =
                    g_DressingRoomItemIds[arg3 + arg2];
            } else {
                if (arg1 < 0) {
                    return;
                }
                if (arg1 > MENU_ACCESSORY_SLOT_COUNT) {
                    return;
                }
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].accessorySlots[arg1 - 1] =
                    g_DressingRoomItemIds[arg3 + arg2];
            }
        } else {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].extraWeaponIds[arg1] =
                g_DressingRoomItemIds[arg3 + arg2];
        }
    } else {
        if (arg4 == 0) {
            if (arg1 == 0) {
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId]
                    .weaponId[0] = g_DressingRoomItemIds[arg3 + arg2];
            } else {
                if (arg1 < 0) {
                    return;
                }
                if (arg1 > MENU_ACCESSORY_SLOT_COUNT) {
                    return;
                }
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId]
                    .accessorySlots[arg1 - 1] = g_DressingRoomItemIds[arg3 + arg2];
            }
        } else {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId]
                .extraWeaponIds[arg1] = g_DressingRoomItemIds[arg3 + arg2];
        }
    }
}
void MenuUpdateDressingRoomTotals(u_char arg0) {
    u8 charId;

    charId = g_Menu->pManager->currentCharacterIDs[arg0];
    func_801E3ECC(g_Menu->pDressingRoom, g_GameState.characters[charId].gearId);
    charId = g_Menu->pManager->currentCharacterIDs[arg0];
    MenuFillDressingRoomGearStats(g_Menu->pDressingRoom, g_GameState.characters[charId].gearId);
    g_Menu->pDressingRoom->statValues[0] = g_Menu->pDressingRoom->totalAttack;
    g_Menu->pDressingRoom->statValues[1] = g_Menu->pDressingRoom->totalDefense;
    g_Menu->pDressingRoom->statValues[2] = g_Menu->pDressingRoom->totalEtherDefense;
    g_Menu->pDressingRoom->statValues[3] = g_Menu->pDressingRoom->totalResponsiveness;
    g_Menu->pDressingRoom->statValues[4] = g_Menu->pDressingRoom->totalAgility;
    g_Menu->pDressingRoom->statValues[5] = g_Menu->pDressingRoom->totalEtherAmplification;
}
// Draws the three item name strings of one dressing room slot.
void MenuRenderEquipmentSlotStrings(s32 arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4, u_char arg5, u_char arg6) {
    RECT rect;
    void* pStrings;
    u8* pBuf;
    u16 itemId;
    u_char slot;
    s32 i;

    itemId = g_DressingRoomItemIds[arg2 + arg1];
    slot = 0;
    if (arg5 != 0) {
        itemId = 0xFF;
    }
    if (itemId != 0) {
        if (arg3 == 0 && arg0 != 0) {
            slot = 1;
        }
        slot += arg4 * 2;
        switch (slot) {
        case 0:
            pStrings = g_Menu->pCharacterSpriteRecords->pStringLists[0];
            if (arg5 != 0) {
                if (arg3 == 0) {
                    itemId = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg6]].weaponId[0];
                } else {
                    itemId = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg6]].extraWeaponIds[arg0];
                }
            }
            break;
        case 1:
            pStrings = g_Menu->pCharacterSpriteRecords->pStringLists[1];
            if (arg5 != 0) {
                itemId = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg6]].accessorySlots[arg0 - 1];
            }
            break;
        case 2:
            pStrings = g_Menu->pCharacterSpriteRecords->pStringLists[2];
            if (arg5 != 0) {
                if (arg3 == 0) {
                    itemId = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg6]].gearId]
                        .weaponId[0];
                } else {
                    itemId = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg6]].gearId]
                        .extraWeaponIds[arg0];
                }
            }
            break;
        case 3:
            pStrings = g_Menu->pCharacterSpriteRecords->pStringLists[3];
            if (arg5 != 0) {
                itemId = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg6]].gearId]
                    .accessorySlots[arg0 - 1];
            }
            break;
        }
        if (itemId != 0) {
            pBuf = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);
            bzero(pBuf, MENU_STRING_IMAGE_BUFFER_SIZE);
            for (i = 0; i < MENU_ITEM_NAME_LINE_COUNT; i++) {
                g_Menu->pCharacterSpriteRecords->slotStrings[i].width = SystemRenderStringEntry(GetStringEntry(pStrings, itemId * MENU_ITEM_NAME_LINE_COUNT + i),
                    pBuf, 0x24, 0);
                rect.x = 0x180 + ((i + 8) & 1) * 0x18;
                rect.y = 0x80 + ((i + 8) / 2) * FONT_LETTER_HEIGHT;
                rect.w = 0x28;
                rect.h = FONT_LETTER_HEIGHT;
                LoadImage(&rect, (u_long*)pBuf);
                DrawSync(0);
                func_801E7C50(&g_Menu->pCharacterSpriteRecords->slotStrings[i], i + 8, 0x80, 0x81);
                MenuSetQuadVertices(g_Menu->pCharacterSpriteRecords->slotStrings[i].vertices, 0x10, i * 0x10 + 0x96,
                    g_Menu->pCharacterSpriteRecords->slotStrings[i].width, FONT_LETTER_HEIGHT);
                g_Menu->pCharacterSpriteRecords->slotStrings[i].renderContext = g_Menu->renderContext;
            }
            g_Menu->pCharacterSpriteRecords->shouldRenderStrings880[0] = 1;
            HeapFree(pBuf);
            return;
        }
    }
    g_Menu->pCharacterSpriteRecords->shouldRenderStrings880[0] = 0;
}
void MenuReturnWeaponToInventory(u_char arg0, u_char arg1) {
    u_char* pIds;
    u_char* pQuantities;
    u_char characterId;
    u_char gearId;
    u_char weaponId;
    u_char found = 1;
    u_char count;
    s32 i;

    if (arg1 == 0) {
        pIds = g_GameState.weaponIDs;
        characterId = g_Menu->pManager->currentCharacterIDs[arg0];
        weaponId = g_GameState.characters[characterId].extraWeaponIds[0];
        g_GameState.characters[characterId].extraWeaponIds[0] = 0;
        pQuantities = g_GameState.weaponQuantities;
        count = MAX_INVENTORY_WEAPONS;
    } else {
        pIds = g_GameState.gearWeaponsIds;
        characterId = g_Menu->pManager->currentCharacterIDs[arg0];
        gearId = g_GameState.characters[characterId].gearId;
        weaponId = g_GameState.gears[gearId].extraWeaponIds[0];
        g_GameState.gears[gearId].extraWeaponIds[0] = 0;
        pQuantities = g_GameState.gearWeaponsQty;
        count = MAX_INVENTORY_WEAPONS;
    }

    for (i = 0; i < count; i++) {
        if (pIds[i] == weaponId) {
            u_char quantity = pQuantities[i] + 1;

            pQuantities[i] = quantity;
            if (quantity >= MAX_INVENTORY_WEAPONS) {
                pQuantities[i] = MAX_ITEM_QUANTITY;
            }
            found = 0;
        }
    }

    if (found) {
        for (i = 0; i < count; i++) {
            if (pIds[i] == 0) {
                pIds[i] = weaponId;
                pQuantities[i] = 1;
                break;
            }
        }
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801E05D0);
/* Best: 98.56%, 2468 bytes (retail 2472), 617 vs 618 insns - 4 bytes short, so
   PARKED. Everything in the body below is retail's: the prologue save order, all
   three redraw guards, both switches with retail's case-body order (outer: 5, 4,
   0/2, 1, 3, 9, 10; inner: 5, 4, 1, 3), the eight setXYWH stores with their
   reloaded `g_Menu->pCursors`, and every call argument.
   THE ONE MISSING INSTRUCTION IS A GAS LOAD-DELAY `nop`, and it is a scheduler
   tie, not a structure error: in the inner switch's shared tail retail keeps
   `lui v0,g_Menu / lw v0,0(v0) / nop / lw v0,0x33C(v0)` (a load-use hole gas
   pads) and has its `li s0,0x1` for `closeList` ten insns EARLIER, right after
   the `MenuCopyEquipmentFrom/ToPanel` call; our compile sinks that `li` into the
   hole, so gas has nothing to pad and the function is one insn short.
   MEASURED SPELLINGS of the closeList/shouldRender pair: `closeList = 1;` then a
   separate `g_Menu->pCursors->shouldRender[0] = 1;` (either statement order)
   cse's the two constants into one, so the store becomes `sb s0,0x140` and the
   body is 615 insns (2460 bytes, 98.22%); the order `shouldRender[0] = 1;` then
   `closeList = 1;` (what the body below uses) keeps retail's TWO `li 1` - `li
   t0` for the store, `li s0` for closeList - but still sinks the `li s0`
   (617/2468); the embedded form `closeList = g_Menu->pCursors->shouldRender[0]
   = 1;` gives the same shape as that, and `store = closeList = 1` or a
   `do { } while (0);` around the pair are worse (615/2460). An 8-minute
   permute.sh run on the 98.42% body (2205 iterations) found only the slotRow
   change below, so this tie is not reachable by random statement reordering.
   Also needed and already in: prevChar is u_char and group is u_char (retail
   masks both sides of every `!= prevChar` and does `andi ...s6,0xff` at the
   calls); `scrollY = 100 * row / count; scrollY = scrollY / 2;` as two
   statements (one expression puts the div result in v0, retail uses a1); the
   y-row table read via `(pSlotY = &g_MenuEquipmentSlotRowY[...][0])[0]` inside the setXYWH
   argument (a separate statement hoists the index math into the two load-delay
   slots and costs two more nops; 4 lhu of it, so the address - not the value -
   is what retail CSEs into $s0), and that row index in its own `s32 slotRow`
   local at the 2nd and 3rd y store, which is what closed the old v0/v1-vs-v1/v0
   swap of the table and prim addresses (98.42 -> 98.56; found by permute.sh).
   JUMP TABLE, AND WHY THIS CANNOT SHIP ALONE EVEN AT 100%: this stub owns
   jtbl_801C515C at +0x60 of the unit's .rodata range (0xFC..0x18C, three
   0x30-byte tables: func_801DBE54's at +0x00, func_801DDF24's at +0x30, this one
   last; 11 cases = 44 bytes plus 4 of pad each). C .rodata lands before asm
   .rodata in the same object, so converting this function while the other two
   are still stubs moves its table to +0x00. Convert the three IN ADDRESS ORDER,
   first to last, and judge with `tools/scripts/check_bin.sh menu`.
   Semantics: the character/gear equipment panel - arg2 != 0 is the gear panel,
   arg1 asks for the open animation, and group (toggled by LEFT/RIGHT on
   character id 4 and by a successful equip) selects the weapon vs accessory page
   whose four row y's are g_MenuEquipmentSlotRowY[group][slot]. Slot 0 is the weapon, 1..3 the
   accessories; for gears (arg2 != 0) with group != 0 slots 1 and 2 are skipped. */
/*
void func_801E05D0(u_char arg0, u_char arg1, u_char arg2) {
    u8 active;
    u8 needsOpen;
    u8 needsWindow;
    u8 needsPanel;
    u8 cursorVisible;
    u8 reopenList;
    u8 listOpen;
    u8 closeList;
    u8 kind;
    u8 equipResult;
    s32 slot;
    s32 col;
    s32 row;
    u_char group;
    u_char prevChar;
    s32 prevSlot;
    s32 prevCol;
    s32 prevRow;
    s32 count;
    s32 scrollY;
    s32 scrollHeight;
    u16* pSlotY;
    s32 slotRow;

    active = 1;
    needsOpen = 1;
    group = 0;
    prevChar = 0xFF;
    slot = 0;
    needsWindow = 1;
    col = 0;
    prevSlot = 0xFF;
    prevCol = 0xFF;
    row = 0;
    prevRow = 0xFF;
    needsPanel = 0;
    cursorVisible = 0;
    reopenList = 0;
    listOpen = 0;
    kind = arg2;

    g_Menu->pManager->shouldRenderUnk358Prims = 0;
    MenuInitializeCharacterSpriteRecords(arg2);
    MenuCopyEquipmentToPanel(arg0, kind);

    while (active) {
        MenuRenderFrame();
        if (needsOpen || arg0 != prevChar) {
            func_801D8EA4(arg0, group + 1, listOpen, kind);
            MenuCreateListWindow(group, kind);
            needsOpen = 0;
        }
        if (row != prevRow || arg0 != prevChar) {
            count = func_801DE5CC(arg0, row, slot, group, kind);
            if (count != 0) {
                scrollY = 100 * row / count;
                scrollY = scrollY / 2;
                scrollHeight = 0x32;
            } else {
                scrollY = 0;
                scrollHeight = 0x64;
            }
            MenuCreateScrollHandle(0x94, scrollY + 0x12, scrollHeight);
        }
        if (cursorVisible) {
            func_801DB0A8(col, row, 3, 0);
        } else {
            g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
        }
        if (col != prevCol || arg0 != prevChar || row != prevRow) {
            if (needsPanel) {
                MenuSetEquipmentSlotValue(arg0, slot, col, row, group, kind);
            }
            needsPanel = 1;
            if (kind == 0) {
                MenuApplyEquippedItemBonuses(g_Menu->pDressingRoom, g_Menu->pManager->currentCharacterIDs[arg0]);
                MenuComputeCharacterStats(g_Menu->pDressingRoom, g_Menu->pManager->currentCharacterIDs[arg0]);
            } else {
                MenuUpdateDressingRoomTotals(arg0);
            }
            MenuRenderEquipmentSlotStrings(slot, col, row, group, kind, 0, arg0);
            MenuDrawStatTable(arg0, 1, listOpen, kind);
            prevCol = col;
            prevRow = row;
        }
        if (slot != prevSlot || arg0 != prevChar) {
            MenuRenderEquipmentSlotStrings(slot, col, row, group, kind, 1, arg0);
            slotRow = group * 4 + slot;
            (&g_Menu->pCursors->polysCursor[g_Menu->pCursors->renderContexts[0]])->x0 = 0x8C,
            (&g_Menu->pCursors->polysCursor[g_Menu->pCursors->renderContexts[0]])->y0 = (pSlotY = &g_MenuEquipmentSlotRowY[group * 4 + slot][0])[0],
            (&g_Menu->pCursors->polysCursor[g_Menu->pCursors->renderContexts[0]])->x1 = 0x8C + 0x10,
            (&g_Menu->pCursors->polysCursor[g_Menu->pCursors->renderContexts[0]])->y1 = (pSlotY = &g_MenuEquipmentSlotRowY[slotRow][0])[0],
            (&g_Menu->pCursors->polysCursor[g_Menu->pCursors->renderContexts[0]])->x2 = 0x8C,
            (&g_Menu->pCursors->polysCursor[g_Menu->pCursors->renderContexts[0]])->y2 = (pSlotY = &g_MenuEquipmentSlotRowY[slotRow][0])[0] + 0x10,
            (&g_Menu->pCursors->polysCursor[g_Menu->pCursors->renderContexts[0]])->x3 = 0x8C + 0x10,
            (&g_Menu->pCursors->polysCursor[g_Menu->pCursors->renderContexts[0]])->y3 = (pSlotY = &g_MenuEquipmentSlotRowY[group * 4 + slot][0])[0] + 0x10;
            prevSlot = slot;
            prevChar = arg0;
        }
        if (needsWindow) {
            MenuCreateWindow(2, 0x94, 0xA, 0x94, 0x74, 0, 1, 4, 1);
            MenuCreateWindow(3, 0x6C, 0x87, 0xC4, 0x48, 0, 1, 4, 0);
            MenuCreateWindow(5, 8, 0x8E, 0x60, 0x40, 0, 1, 4, 0);
            if (arg1) {
                MenuStartOpenAnimation();
                func_801D29A8(0, 0);
                while (g_Menu->transitionEffectState) {
                    MenuRenderFrame();
                }
            }
            do {
                g_Menu->pCursors->shouldRender[0] = 1;
                g_Menu->pManager->playTimeDigitsActive = 0;
            } while (0);
            needsWindow = 0;
            g_Menu->pManager->shouldRenderWindow[1] = 0;
        }
        g_Menu->pManager->shouldRenderCursors = 1;
        if (reopenList) {
            reopenList = 0;
            g_Menu->input = MENU_INPUT_CONFIRM;
        }
        if (listOpen == 0) {
            switch (g_Menu->input) {
            case MENU_INPUT_BACK:
                active = 0;
                break;
            case MENU_INPUT_CONFIRM:
                MenuCopyEquipmentToPanel(arg0, arg2);
                row = 0;
                listOpen = 1;
                prevRow = 0xFF;
                prevCol = 0xFF;
                g_Menu->pCursors->shouldRender[0] = 0;
                needsOpen = 1;
                cursorVisible = 1;
                col = 0;
                g_Menu->pManager->shouldRenderArrowCursor[0] = 1;
                break;
            case MENU_INPUT_RIGHT:
            case MENU_INPUT_LEFT:
                if (g_Menu->pManager->currentCharacterIDs[arg0] == 4) {
                    group ^= 1;
                    prevSlot = 0xFF;
                    needsOpen = 1;
                    slot = 0;
                }
                break;
            case MENU_INPUT_DOWN:
                if (++slot >= 4) {
                    slot = 0;
                }
                if (group != 0 && kind != 0 && (u32)(slot - 1) < 2) {
                    slot = 3;
                }
                break;
            case MENU_INPUT_UP:
                if (--slot < 0) {
                    slot = 3;
                }
                if (0 != group && kind != 0 && (u32)(slot - 1) < 2) {
                    slot = 0;
                }
                break;
            case MENU_INPUT_NEXT:
                if (func_801D9704(arg0, 0, kind) != arg0) {
                    arg0 = func_801D9704(arg0, 0, kind);
                    group = 0;
                    needsPanel = 0;
                }
                break;
            case MENU_INPUT_PREV:
                if (func_801D9704(arg0, 1, kind) != arg0) {
                    arg0 = func_801D9704(arg0, 1, kind);
                    group = 0;
                    needsPanel = 0;
                }
                break;
            }
        } else {
            closeList = 0;
            switch (g_Menu->input) {
            case MENU_INPUT_BACK:
                MenuCopyEquipmentFromPanel(arg0, arg2);
                cursorVisible = 0;
                g_Menu->pCursors->shouldRender[0] = 1;
                closeList = 1;
                func_801DB0A8(0, row, 3, 0);
                col = 0;
                prevChar = 0xFF;
                g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
                break;
            case MENU_INPUT_CONFIRM:
                equipResult = func_801DF0D4(arg0, slot, group, kind);
                prevChar = 0xFF;
                if (equipResult != 0) {
                    group ^= 1;
                    needsOpen = 1;
                    reopenList = 1;
                    MenuReturnWeaponToInventory(arg0, kind);
                }
                MenuCopyEquipmentToPanel(arg0, kind);
                cursorVisible = 0;
                g_Menu->pCursors->shouldRender[0] = 1;
                closeList = 1;
                func_801DB0A8(0, row, 3, 0);
                col = 0;
                g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
                break;
            case MENU_INPUT_DOWN:
                if (++col >= 8) {
                    col = 7;
                    row++;
                    if (row > count) {
                        row = count;
                    }
                }
                break;
            case MENU_INPUT_UP:
                if (--col < 0) {
                    col = 0;
                    if (--row < 0) {
                        row = 0;
                    }
                }
                break;
            }
            if (closeList) {
                col = 0;
                row = 0;
                listOpen = 0;
                prevCol = 0xFF;
                needsOpen = 1;
                needsPanel = 0;
                ((u8*)g_Menu->unk35C)[0x32F2] = 0;
            }
        }
    }
    MenuHidePointerCursors();
    MenuFreeArrowCursor(0);
}
*/
