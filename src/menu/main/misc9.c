#include "common.h"

#include "main/game.h"
#include "psyq/libapi.h"
#include "system/memory.h"
#include "system/menu.h"

extern u8 D_801EA548[][8]; // 8 u8 string IDs per panel
extern u8 D_801EA558[][6]; // 6 u8 string IDs per panel
extern u16 g_MenuInventoryWindowHeight;
extern u8 g_DressingRoomItemIds[];
extern s32 g_MenuInventoryRowCount;
extern u16 g_MenuInventoryScrollStep;
extern u8 D_801E9EA0[];
extern u8 D_801E9785;
extern u16 D_801E9788[][2];
extern u16 D_801E9794[][2];
extern u16 D_801E97A0[][2];
extern u8 g_ShopIndex;

// 0x1094-byte item panel buffer built by MenuCreateUnk430Panel
typedef struct {
    /* 0x0000 */ u8 unk0[0x1084];
    /* 0x1084 */ u8 unk1084[0x10];
} MenuUnk430Panel; // Size: 0x1094

void func_801DC3D8(u_char arg0, u_char arg1);
void func_801DCE60(u_char arg0, u_char arg1, u_char arg2);
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
void func_801D3488(s32 arg0, s32 arg1);
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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DB920);
/* Best: 96.76, 1064 bytes (retail 1068). Structure matches retail everywhere
   except three points:
   1. Loop preheader order: retail emits the loop-entry `if (active)` test
      BEFORE the hoisted idx/pQuantity setup, i.e. its source computed
      `idx = arg0 * 2 + arg1` and `pQuantity = &g_GameState.itemQuantities[idx]`
      INSIDE the while body and loop.c hoisted them into the preheader. Moving
      both inside the loop closed 92.79 -> 96.76; the body below is that version.
   2. The itemIDs clear in the `quantity == 0` branch: retail has
      lui/addiu(g_GameState+0x1F90) + addiu 0x96 + addu fp + sb (5 insns); every
      spelling of `g_GameState.itemQuantities[idx + MAX_INVENTORY_ITEMS] = 0`
      now folds to the 3-insn %at macro `sb $0,%lo(g_GameState+0x2026)(fp)`,
      `(u8*)itemQuantities + MAX_INVENTORY_ITEMS` then `[idx]` compiles the same
      (measured 96.76/1064), and `pQuantity[MAX_INVENTORY_ITEMS]` gives
      `sb $0,0x96(s7)` (also short). Retail's fresh la of the base says its
      source took the array address into a variable; not reproduced.
   3. Callee-saved rotation: retail refresh=$s1, &itemIDs=$s2, result=$s3, ours
      $s3, $s1, $s2 (the other seven agree). Under gcc global-alloc priority
      (floor_log2(n_refs)*n_refs*10000/live_length) the order is &itemIDs
      (2 refs over ~12 insns) > result > refresh; no reachable length change
      reorders it to retail's, so the lever is elsewhere.
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

    slot = g_Menu->unk4DA[2];
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
    while (count < 0x96) {
        if (g_GameState.itemIDs[index] != 0) {
            last = index;
        }
        count++;
        index++;
        do {
        } while (0);
    }

    if (last < 0x10) {
        g_MenuInventoryWindowHeight = 0x74;
        g_MenuInventoryRowCount = 0;
        g_MenuInventoryScrollStep = 0;
    } else {
        s32 n = (last - 0x10) / 2 + 1;
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
   two `selectedSlot = 0xFF` store sites where gcc 2.6.0 cross-jumps them into one). */

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

    g_Menu->unk430 = HeapAlloc(0x1094, 0);
    bzero(g_Menu->unk430, 0x1094);
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
    MenuLoadStringsFromIds(8, g_Menu->unk10E0, D_801EA548[index], g_Menu->pManager->shouldRenderStrings10E0);
    func_801C72BC((u8)kind);
    MenuSetupPointerCursors(2);
    func_801D3488(2, arg0);
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
    func_801C72BC((kind | 0x10) & 0xFF);
    HeapFree(g_Menu->unk430);
    g_Menu->pManager->playTimeDigitsActive = 1;
    g_Menu->pManager->shouldRenderWindow[1] = 1;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DC3D8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DCE60);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DD5E8);
/* Best: 94.01%, same size (416 bytes), from the round-63 attempt (u_char arg0).
   Remaining deltas, all tried by hand this round:
   1. loop 1: retail forms the compare address as base + (rc*0x28 + i*0x80)
      (two addu, keeping base unmodified) and only then reuses the saved
      base + i*0x80 and rc*0x28 for the call argument (addu a2,a1). Every C
      that names pStrings[i].polys[pStrings[i].renderContext] once, or via a
      separate ctx local, or via a pString pointer local, CSEs to
      (base + i*0x80) + rc*0x28 and loses one addu (93.96-94.01%).
   2. loop 2: retail repeats andi a1,arg0,0xff in the loop-back delay slot;
      gcc un-hoists it out of the loop and leaves a nop there. A `u_char mode
      = arg0;` local inside the body does not stop the hoist.
   3. u_int arg0 with (arg0 & 0xFF) at each call site (so the mask is a real
      per-site expression) gives 93.58%.
   permute.sh --minutes 4 -j 2: PERMUTER_BETTER 777 -> 547, but the winner
   caches g_Menu in a callee-saved local, which drops a lui/lw pair and
   compiles to 408 bytes - wrong size, so not portable.
   Round 106 re-check: the whole loop-1 delta is that one addu, nothing else.
   Dropping the pPoly local and writing the address twice, and the
   'pStrings[i].polys + ctx' spelling, both compile identically to each other
   and to the version below (416 bytes, if-body one insn short, 93.96%): gcc
   CSEs the poly address out of the compare and the argument regardless, and
   the load then uses the record-base register that retail keeps in a2.
   void func_801DD5E8(u_char arg0) {
       MenuString* pStrings;
       POLY_FT4* pPoly;
       s32 i;

       func_801E8F60(5, arg0);
       func_801E8F60(6, arg0);
       for (i = 0; i < 0xC; i++) {
           pStrings = g_Menu->unk430;
           pPoly = &pStrings[i].polys[pStrings[i].renderContext];
           if (pPoly->r0 != 0x20) {
               MenuSetPrimShadingMode(pPoly, arg0);
               pStrings = g_Menu->unk430;
               pPoly = &pStrings[i + 0xE].polys[pStrings[i + 0xE].renderContext];
               MenuSetPrimShadingMode(pPoly, arg0);
           }
       }
       MenuSetPrimShadingMode(&g_Menu->arrowCursors[0]->polys[g_Menu->arrowCursors[0]->renderContext], arg0);
       pStrings = g_Menu->unk430;
       MenuSetPrimShadingMode(&pStrings[0x1C].polys[pStrings[0x1C].renderContext], arg0);
       for (i = 0; i < 2; i++) {
           pStrings = g_Menu->unk430;
           pPoly = &pStrings[0x1E + i].polys[pStrings[0x1E + i].renderContext];
           MenuSetPrimShadingMode(pPoly, arg0);
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
   JUMP TABLE: this stub owns jtbl_801C512C, which is at +0x30 of the unit's .rodata
   range (config/menu.yaml [0xFC, .rodata, main/misc9]) - func_801DBE54 owns the table
   at +0x00 and func_801E05D0 the one at +0x60, so the three must be converted IN THAT
   ORDER or every later table shifts by 0x30. Un-park this one only after
   func_801DBE54's switch is C. `case 4`'s flag byte is g_Menu->unk430 + 0x1084 + col
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

    g_Menu->pCharacterSpriteRecords = HeapAlloc(0xA1C, 0);
    bzero(g_Menu->pCharacterSpriteRecords, 0xA1C);
    pManager = g_Menu->pManager;
    MenuLoadStringsFromIds(6, g_Menu->unk14E0, D_801EA558[arg0 & 0xFF], pManager->shouldRenderStrings14E0);
    func_801C72BC(7);
    MenuSetupPointerCursors(3);
    MenuInitializeArrowCursor(0);
    func_801D3488(1, arg0 & 0xFF);
}

void MenuFreeCharacterSpriteRecords(void) {
    g_Menu->pManager->shouldRenderPanel[2] = 0;
    MenuFreeWindow(2);
    MenuFreeWindow(3);
    MenuFreeWindow(4);
    MenuFreeWindow(5);
    MenuRenderFrame();
    MenuClearBytes(6, &g_Menu->pManager->shouldRenderStrings10E0[8]);
    func_801C72BC(0x17);
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

    for (i = 0; i < 6; i++) {
        g_Menu->pManager->shouldRenderStrings14E0[i] = 0;
    }

    if (arg0 != 0) {
        start = 2;
        count = 6;
        label = 0x72;
    } else {
        start = 0;
        count = 2;
        label = 0x5A;
    }

    for (i = start; i < count; i++) {
        MenuSetRowQuadVertices(6, g_Menu->unk14E0, D_801EA558[arg1 & 0xFF], D_801E9EA0,
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

    for (i = 0; i < 9; i++) {
        ((u16*)((u8*)g_Menu->pEquipmentPanel + 0x280))[i] = g_Menu->pDressingRoom->statValues[i];
    }

    if (arg1 == 0) {
        for (i = 0; i < 5; i++) {
            ((u8*)g_Menu->pEquipmentPanel + i)[0x29C] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].weaponId[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[0x2A1] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].extraWeaponIds[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[0x2A6] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].accessorySlots[i];
        }
    } else {
        for (i = 0; i < 4; i++) {
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
        for (i = 0; i < 5; i++) {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].weaponId[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x29C];
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].extraWeaponIds[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x2A1];
        }
        for (i = 0; i < 3; i++) {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].accessorySlots[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x2A6];
        }
    } else {
        for (i = 0; i < 4; i++) {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].weaponId[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x29C];
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].extraWeaponIds[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[0x2A1];
        }
        for (i = 0; i < 3; i++) {
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
                if (arg1 > 3) {
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
                if (arg1 > 3) {
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
            pBuf = HeapAlloc(0x3F6, 0);
            bzero(pBuf, 0x3F6);
            for (i = 0; i < 3; i++) {
                g_Menu->pCharacterSpriteRecords->slotStrings[i].width = SystemRenderStringEntry(GetStringEntry(pStrings, itemId * 3 + i),
                    pBuf, 0x24, 0);
                rect.x = 0x180 + ((i + 8) & 1) * 0x18;
                rect.y = 0x80 + ((i + 8) / 2) * 0xD;
                rect.w = 0x28;
                rect.h = 0xD;
                LoadImage(&rect, (u_long*)pBuf);
                DrawSync(0);
                func_801E7C50(&g_Menu->pCharacterSpriteRecords->slotStrings[i], i + 8, 0x80, 0x81);
                MenuSetQuadVertices(g_Menu->pCharacterSpriteRecords->slotStrings[i].vertices, 0x10, i * 0x10 + 0x96,
                    g_Menu->pCharacterSpriteRecords->slotStrings[i].width, 0xD);
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
