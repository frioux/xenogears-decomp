#include "common.h"

#include "main/game.h"
#include "psyq/libapi.h"
#include "system/memory.h"
#include "system/menu.h"

// One page of the inventory window: 8 row panels of MENU_INVENTORY_ROW_ITEM_COUNT items each.
#define MENU_INVENTORY_ROW_ITEM_COUNT 2
#define MENU_INVENTORY_PAGE_ITEM_COUNT 16
// String IDs per row panel of the character list.
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
// Sprite records at the end of the 0x430 panel: 0x1C holds a copy of the selected
// column's string, 0x1D..0x1F the three strings func_801DCE60 renders.
#define MENU_PANEL_COLUMN_STRING 0x1C
#define MENU_PANEL_HEADING_STRING 0x1E
// Row sprite records at the start of the 0x430 panel; 0xE..0x1B is the shaded copy
// of the same rows that MenuSetUnk430PanelShading walks alongside them.
#define MENU_PANEL_ROW_STRING_COUNT 0xC
#define MENU_PANEL_SHADED_ROW_STRING 0xE
// The one character with two weapon slots: its equipment panel carries a second
// weapon slot group, which the left/right input and the weapon move below both
// switch between.
#define MENU_TWO_WEAPON_SLOT_CHARACTER_ID 4
// Slot groups of the equipment panel: one byte per weapon slot, then one per
// extra weapon slot, then one per accessory slot.
#define MENU_PANEL_WEAPON_ID 0x29C
#define MENU_PANEL_EXTRA_WEAPON_IDS 0x2A1
#define MENU_PANEL_ACCESSORY_IDS 0x2A6

extern u8 g_MenuItemRowStringIds[][8]; // 8 u8 string IDs per panel
extern u8 g_MenuCharacterRowStringIds[][6]; // 6 u8 string IDs per panel
extern u16 g_MenuInventoryWindowHeight;
extern u8 g_DressingRoomItemIds[];
extern u8 g_DressingRoomItemQuantities[]; // one quantity per g_DressingRoomItemIds entry
extern s32 g_MenuInventoryRowCount;
extern u16 g_MenuInventoryScrollStep;
extern u8 g_MenuPanelRowXOffsets[];
extern u8 g_MenuColumnRowStringIds[];
extern u8 g_MenuCharactersAllocated;
extern u8 g_MenuRowDigitTextureIds[]; // u8 [13][5] texture ids; the per-character column masks follow it
// One 16-bit column mask per character: column i of a character is drawn in the
// ink colour unless its bit is clear.
extern u16 g_MenuColumnShadeMasks[];
extern u16 D_801E9788[][2];
extern u16 D_801E9794[][2];
extern u16 D_801E97A0[][2];
extern u8 g_ShopIndex;
extern u16 g_MenuEquipmentSlotRowY[][2];
// The value rows of the two quad groups of the 0x430 panel, one 0x38-byte array per
// group (14 columns each), right after g_MenuEquipmentSlotRowY's own 8 rows.
extern u16 g_MenuPanelValueQuadX[][2];
extern u16 g_MenuPanelValueQuadY[][2];

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

// The same panel's sprite records from MENU_PANEL_SHADED_ROW_STRING on, seen as their
// own arrays: the shaded copy of the rows and the two heading strings, with the
// column string records between them.
typedef struct {
    /* 0x0000 */ u8 unk0[0x700];
    /* 0x0700 */ MenuString shadedRowStrings[MENU_PANEL_SHADED_ROW_STRING];
    /* 0x0E00 */ MenuString columnStrings[2];
    /* 0x0F00 */ MenuString headingStrings[2];
} MenuUnk430RowStrings; // Size: 0x1000

// One 0x28-byte equipment slot record of pDressingRoom->pCharacterSlots[] (and of
// pGearBlocks[], which uses the same layout), as far as this unit reaches it: the
// flag word a panel reads back and the mp/hp the equipped part costs.
typedef struct {
    /* 0x00 */ u16 flags;
    /* 0x02 */ u8 unk2[0x11];
    /* 0x13 */ u8 mpCost; // subtracted from the wearer's mp once the part is equipped
    /* 0x14 */ u8 unk14[0x10];
    /* 0x24 */ u16 fuelCost; // subtracted from the gear's fuel once the part is equipped
    /* 0x26 */ u8 unk26[0x2];
} MenuEquipmentSlotRecord; // Size: 0x28

void MenuBuildUnk430PanelColumns(u_char arg0, u_char arg1);
void func_801DCE60(u_char arg0, u_char arg1, u_char arg2);
void func_801DD790(u_char arg0, s32 arg1, u_char arg2);
void MenuInitializeUnk42CPanel(void);
void MenuCreateScrollHandle(s32 a0, s32 a1, s32 a2);
void MenuBuildArrowCursorPrim(s32 arg0, s32 arg1, u_char arg2, u_char arg3);
void MenuFreeArrowCursor(u_char index);
void MenuStartOpenAnimation(void);
void MenuTweenCharacterWindows(s32 arg0, s32 arg1);
void MenuSwapInventoryItems(s32 index0, s32 index1);
u_char func_801DB920(s32 arg0, s32 arg1);
void MenuRenderFrame(void);
void MenuHidePointerCursors(void);
void MenuFreeWindow(s32 arg0);
void MenuLoadResourceSet(s32 arg0);
void MenuFreeScrollHandle(void);
void MenuClearBytes(s32 arg0, void* pArg1);
void MenuLoadStringsFromIds(s32 arg0, void* pArg1, void* pArg2, void* pArg3);
void MenuSetupPointerCursors(s32 arg0);
void MenuInitializeArrowCursor(s32 arg0);
void MenuCreateShoulderButtonUi(s32 arg0, s32 arg1);
void MenuCopyGearEquipmentSlots(MenuDressingRoom* pDressingRoom, s32 gearId);
void MenuCreateWindow(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, u_char a6, s32 a7, u_char a8);
void MenuSetRowQuadVertices(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u8 arg5, u8 arg6, u8 arg7);
void MenuFillDressingRoomGearStats(MenuDressingRoom* pDressingRoom, s32 gearId);
void MenuSetPrimShadingMode(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
u16 MenuAndBitMaskLsbFirst(u32 mask, u8 index);
void MenuPlaySoundEffect(u_char soundEffectId);
s32 MenuStepPartySlot(s32 slot, u_char direction, u_char arg2);
void func_801DA5BC(s32 arg0);
void MenuBuildItemPrims(s32 arg0, s32 arg1);
void MenuSetAllPrimsShadingMode(s32 arg0);
void MenuBuildPartyCharacterPrims(s32 arg0);
s32 func_801E31C0(MenuDressingRoom* pDressingRoom, u8 characterId, u8 itemId);
void MenuSetStringPrimitives(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);
extern void* GetStringEntry(void* pStringData, u32 index);
extern s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
extern u8* GetStringEntryFromBank14(s32 index);
extern u8* GetStringEntryFromBank30(s32 index);
extern u8* GetStringEntryFromBank34(s32 index);
void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);
u16 MenuAndBitMaskMsbFirst(u32 mask, u8 index);
void MenuSetStringTexturePrims(MenuString* pString, u_char index, u_char isGlyph, u_char row);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);
void func_801D8EA4(u8 arg0, u8 arg1, u8 arg2, u8 arg3);
int func_801DE5CC(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
extern u8* GetWeaponName(s32 itemId);
extern u8* GetAccessoryName(s32 itemId);
extern u8* GetGearWeaponName(s32 itemId);
extern u8* GetGearAccessoryName(s32 itemId);
extern u32 MenuAndBitMaskLsbFirst32(u32 mask, u8 index);
u_char MenuEquipItemFromPanel(u_char arg0, u_char arg1, u_char arg2, u_char arg3);
void MenuDrawStatTable(u8 arg0, u8 arg1, u8 arg2, u8 arg3);
void MenuApplyEquippedItemBonuses(MenuDressingRoom* pDressingRoom, u8 characterId);
void MenuComputeCharacterStats(MenuDressingRoom* pDressingRoom, u8 characterId);

/* PARKED 97.10%, 1072 bytes (retail 1068), 268 vs 267 insns - 4 bytes OVER, so PARKED.
   Everything but one site is retail's: prologue, the hoisted idx/pQuantity preheader, both
   cursor-mask dispatches, all four switch arms, every call, store and delay slot, and the
   teardown. Register rotation is the only other delta (retail refresh=s1, result=s3;
   ours s3/s2) and it is a knock-on.
   THE RESIDUAL IS ONE ADDRESS: the itemIDs clear in the quantity==0 branch. Retail
   (5 insns) = la(g_GameState+0x1F90) [lui+addiu], addiu v0,t0,150, addu v0,fp,v0, sb -
   i.e. gcc kept the 150 OUT of the symbol addend and added it as an immediate to the base.
   Every honest spelling of the target byte (g_GameState.itemIDs[idx], itemQuantities
   [150+idx], &itemQuantities[150] then [idx], a u8/u32 local, a two-statement pointer
   walk, a cast through (u8 *)&g_GameState) folds 0x1F90+150 into ONE symbol addend, and
   that form is unrepresentable here: cc1 emits the bare 'sb $0,g_GameState+8230(reg)'
   macro (3 insns, 1064 bytes, 96.76). Probed again this round (cc1_probe.sh, 12 spellings
   with the live pQuantity pointer): gcc only keeps base+150 separate when a REGISTER
   already holds base, and then it also keeps it live - an extra callee-saved slot, 0xC0
   frame. The body below is the one spelling that recovers the 5th instruction: a short
   offset local the permuter found (base=874 -> 784 over 1255 iters in 4 min), but it lands
   the add on the INDEX side ('li t0,0x96; addu v0,t0,fp; la; addu'), so it is 6 insns, 4
   OVER retail, and the type is not shippable as written: the honest line is
   'g_GameState.itemIDs[idx] = 0;' (same byte, 96.76 / 1064 / 4 short). Port whichever side
   of that trade the next attempt wants; what is left is one instruction either way.
   JUMP TABLE: this stub owns NO table (no jtbl/.rodata reference in its .s). The unit's
   three tables belong to func_801DBE54 (+0x00), func_801DDF24 (+0x30) and func_801E05D0
   (+0x60), so the family cannot be converted whole (a C table is 0x2C + .align 3 = 0x30,
   three of them are 4 short of the 0x90 range). Do not convert this one expecting a table
   shift. func_801DB920 returns u_char (retail does andi v0,v0,0xFF after the call).
   Semantics: the item-use panel - arg0/arg1 select the inventory slot (idx = arg0*2+arg1),
   the pItem at itemIDs[idx] must have displayFlags 0x80 (and 0x20 unless g_ShopIndex is 0),
   flags bit 0 marks the paged multi-character item; the loop renders frames until BACK,
   confirming applies the item to every character in the cursor mask via func_801E31C0 and
   decrements the quantity, and both exits free the three currentCharacters.
   ROUND 429, 17 more spellings measured, the folding is total: cc1_probe in a body that keeps
   pQuantity live yields ONE bare 'sb $0,g_GameState+8230(idx)' for itemIDs[idx],
   itemQuantities[150+idx], itemQuantities[idx+150], the (u32) cast form, base += 150 then [idx],
   &itemQuantities[150] then [idx], and the (u8 *) pointer+const form; in context those are
   96.76 / 1064 (one instruction short). A u8 *p local assigned at the site is HOISTED into a
   callee-saved register and CSEd with pQuantity, giving 'sb $0,150(reg)' (1048, 90.32). Only a
   RUNTIME base keeps 150 as an immediate (probe: a u8 *parameter gives 'addiu $2,$17,$18' then
   'sb $0,150($2)'), and this base is a static symbol, so retail's la+addiu+addu+sb cannot be
   spelled. Presets do not help: 2.6.0 -O2 -G8 97.10 (unit), 2.7.2 93.64, 2.7.2-cdk 90.70,
   2.6.0 -G0 97.10, -O3 97.10.
   ROUND 442: a permuter run (1548 iters, base 784 best 550) found ONE type change that closes
   the SIZE: `s16 idOffset` -> `unsigned int idOffset` gives 1068 bytes / 267 insns, retail's
   exact size, 97.47. The residual is now only the ORDER of the two adds: retail forms
   la(g_GameState+0x1F90) / addiu v0,t0,150 / addu v0,fp,v0 (the constant on the BASE, then the
   index), this body forms addiu v0,fp,150 / la / addu v0,v0,t0 (the constant on the INDEX).
   Tried this round, no change: index-first `idx + (quantities + idOffset)` and a (s32) cast on
   the base - cc1 reassociates both back to index-side; an embedded-assignment pointer local
   breaks the pQuantity preheader (90.83 / 1056). The register rotation (retail
   refresh=s1/result=s3/pItemId=s2/active=s5, ours refresh=s5/result=s2/pItemId=s1/active=s3)
   rides along and is a knock-on of the same pseudo ordering. PARKED at 97.47 / 1068 / 267,
   i.e. retail's byte count: what is left is an operand-order question, not a size one.
*/

// BEST 98.45% / 1068 bytes / 267 insns (retail's exact size). Round 456 improves the old
//    97.47 park with two SOURCE changes, both verified by diff: (1) moving result = 0 INTO the
//    if (refresh) body - it is the first statement of that block, retail puts it in the beqz
//    delay slot (97.47 -> 97.70); (2) writing the case-4 mask loop as 'i = 0;' then
//    'pItemId = &g_GameState.itemIDs[idx];' then 'for (; i < 3; i++)', so the loop preheader
//    reads refresh=0, i=0, la, addu exactly as retail does instead of forming the address
//    first (97.70 -> 98.45, 36 -> 34 differing lines).
//    THREE residuals left, all 34 differing lines are these:
//    (a) retail refresh=$s1 / pItemId=$s2, this body refresh=$s2 / pItemId=$s1 - a pure
//        2-cycle swap in the three saving slots that follow result=$s3. Tried and worse:
//        init order result/active/refresh (97.44, 66 lines) and result/active/flags/refresh
//        (97.44, 66 lines); declaration order of pItemId/refresh/active has no effect at all
//        (allocnos are numbered by first use), so the order must come from ref counts this
//        body cannot change without changing the code.
//    (b) the itemIDs clear in the quantity==0 arm: retail la(g_GameState+0x1F90) then
//        addiu v0,t0,150 then addu v0,s8,v0 (the constant on the BASE); this body puts the
//        150 on the index side. Symbol+150 folds to 0x2026, so the split form needs the base
//        already in a register, which only a runtime base gives - rounds 429 and 446 measured
//        17 spellings and cc1_probe on this and closed it; re-tried this round: the itemIDs
//        address inlined into the loop (90.84 / 263, loses 4 insns), reusing pItemId for the
//        quantities base (96.43 / 268), a separate u8 *pQuantityBase local (96.45 / 268) and
//        the (u8*) cast form (no change, same 34 lines).
//    (c) this body's result=0 and refresh=0 stores inside the if land one delay slot later than
//        retail's (beqz slot vs first call slot) - reorg, rides on (a).
// ROUND 468: re-measured 98.45% / 1068 bytes / 267 insns (retail's exact size) from the body below; four scored runs this round, no new best.
// NEW MEASURED, and it explains residuals (a) and (c) as ONE decision: the two '= 1' inits are a near TIE in global alloc, and the tie breaks on their allocno numbers, i.e. on their statement order.
//   - 'result = 0; refresh = 1; active = 1;' (the body below): refresh's 'li' is hoisted into the prologue right after the s3 save and active's 'li' lands in the 'andi 0x80 / beqz' delay slot; registers active=$s5 and const-1=$s6 (both retail's) but refresh=$s2 and pItemId=$s1 (mirrored). 98.45 / 1068 / 267, 34 differing lines.
//   - 'result = 0; active = 1; refresh = 1;' (the flags load before or after refresh measures identically): retail's EXACT hoist/slot pattern - active's 'li' hoisted, refresh's in the delay slot - but then active=$s6 and const-1=$s5, and refresh is still $s2. 98.19 / 1068 / 267, 64 differing lines.
//   So the (active, const-1) pair IS a tie that a reorder flips, while the (refresh, pItemId) pair does not move in either order: retail has refresh=$s1 (6 QI refs live ~210 insns) ahead of pItemId=$s2 (2 SI refs live ~20 insns), a quotient gap the lever a-global-allocno-priority-is calls unreachable by spelling, and every spelling moves the two together. The one handle left is a variable split that changes the allocno SET, and its only candidate - hoisting pItemId's definition out of case 4 into the loop body to lengthen its live range - moves the 'addu $s2,$s8,$v0' that retail keeps in the arm's preheader (which is what round 456 bought). Do not re-park this on declaration order: six of them measure byte-identical.
// Residual (b), the itemIDs clear, is unchanged and stays closed: retail la(g_GameState+0x1F90) / addiu v0,t0,150 / addu v0,s8,v0 needs the 150 added to the BASE register, and every folding of it lands on the index side (see rounds 429/442/446/456 above).
// ROUND 481: un-parked and re-measured 98.45% / 1068 bytes / 267 insns (retail's exact size), so the note below is current.
// ONE new measurement this round: 'result = 0' moved OUT of the 'if (refresh)' to the top of the loop body (retail's zeroing does execute unconditionally, in the beqz delay slot) measures 98.22% / 1068 / 267 with 54 differing lines: the store is then hoisted into the prologue and the pair ROTATES to result=$s2, refresh=$s3, so that is not the shape retail had. Reverted.
// The rest of this round came from cc1's own dumps, which replace the note's estimate with numbers: -dg '20 regs to allocate: 76 74 123 179 87 82 81 77 83 73 84 131 80 75 ...' with -dl giving 87 (pItemId) 'used 5 times across 18 insns; pointer' = 2.22 and 81 (refresh) 'used 17 times across 144 insns; 1 bytes' = 0.47. The two are 4.7x apart and 87 is processed 5th against 81's 7th, so retail's (refresh=$s1, pItemId=$s2) hand-out needs 81 to clear 2.22 (refs >= 32, or a live range <= 30 insns on a QI pseudo that retail's own andi pins) or 87 to fall under 0.47 (a live range >= 85 insns for a 2-ref pointer that retail forms in the case-4 preheader). Neither is reachable by spelling: same verdict as lever a-two-init-near-tie-flips-one-pair-not-the-other, do not re-brief residual (a).
// Also confirmed current: the operand-order half of residual (b) (cc1 always puts the 150 on the index side; k-gcc-26-folds-symbol-plus), and the preset screen (2.7.2 93.64, cdk 90.70, -G0 97.10, -O3 97.10 against 98.45).
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
    u8 quantity;
    unsigned int idOffset;
    u8* pItemId;

    slot = g_Menu->characterSlot;
    g_MenuCharactersAllocated = 0;
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
        if (refresh) {
            result = 0;
            refresh = 0;
            func_801DA5BC(arg0);
            MenuBuildItemPrims(arg1, arg0);
            MenuSetAllPrimsShadingMode(1);
            MenuBuildPartyCharacterPrims(0);
        }
        pCursors = g_Menu->pCursors;
        pCursors->shouldRender[2] = 0;
        pCursors->shouldRender[1] = 0;
        pCursors->shouldRender[0] = 0;
        idOffset = MAX_INVENTORY_ITEMS;
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
            i = 0;
            pItemId = &g_GameState.itemIDs[idx];
            for (; i < 3; i++) {
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
                    *(g_GameState.itemQuantities + idOffset + idx) = 0;
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
            slot = MenuStepPartySlot(slot, 0, 0);
            break;
        case 3:
            slot = MenuStepPartySlot(slot, 1, 0);
            break;
        }
    }
    pCursors = g_Menu->pCursors;
    pCursors->shouldRender[2] = 0;
    pCursors->shouldRender[1] = 0;
    pCursors->shouldRender[0] = 0;
    MenuSetAllPrimsShadingMode(0);
    g_Menu->pManager->shouldRenderCharacters = 0;
    MenuRenderFrame();
    for (i = 0; i < 3; i++) {
        HeapFree(g_Menu->currentCharacters[i]);
    }
    g_MenuCharactersAllocated = 0;
    MenuFreeWindow(2);
    return result;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DB920);





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
   MenuBuildArrowCursorPrim(selectedSlot, row, 1, 1) call after the switch (89.26), moving
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

/* PARKED 93.91%, 888 bytes / 222 insns against retail's 896 / 224 - re-measured this
   round from the body below. The two missing instructions are both reorg decisions,
   named in the note above: (1) retail duplicates the loop test's `andi v0,s6,0xFF`
   into the out-of-range dispatch's branch delay slot and targets the `bnez`, which
   leaves `sll v0,v1,2` as an ordinary instruction; ours puts the sll in the slot, so
   the switch region is one instruction short. (2) ours cross-jumps case 4's success
   store and case 5's `selectedSlot = 0xFF` into one site where retail keeps
   .L801DC054 and .L801DC070 as two jumps. Re-measured this round: spelling case 4's
   shared tail once - the `func_801DB920(row, col) == 0` guard breaking after
   `selectedSlot = 0xFF`, the swap in an else, then prevRow/prevCol/selectedSlot =
   0xFF once after the join - gives 93.37 at the same 888 bytes, worse; the
   three-statement duplicated form below stays.
   Preset ruled out (earlier round): 93.66 on plain 2.7.2, 84.82 on 2.7.2-cdk, 65.55
   at -O3, and 14 functions of this unit are 100.00 under gcc 2.6.0.
   ROUND 420, re-measured from the body below: 93.91% / 888 bytes, so the note
   above was still accurate. NEW AND IT CLOSES RESIDUAL (1): an empty
   `do { } while (0);` at the END of case 2's arm (after `prevCol = 0xFF;`, before
   `break;`) shifts reorg's block numbering so that reorg DUPLICATES the loop
   test's `andi v0,sX,0xFF` into the dispatch `beqz` delay slot exactly as retail
   prints it, with `sll v0,v1,2` as an ordinary instruction after the branch:
   93.91 -> 94.17%, 888 -> 892 bytes. It also swaps active/needsOpen between $s5
   and $s6 (retail: s5 = needsOpen, s6 = active), which is now one of the two
   residuals. The permuter (3 min, 1323 iterations) found the do-while
   (base 1337 best 1103); its other edit, a `volatile int new_var` passed as the
   a6 argument of the window-4 call, was NOT ported (debris, wrong code).
   Residual left, 892 bytes vs retail's 896: (a) the s5/s6 swap; (b) retail keeps
   an extra `j .L801DC070` - its case 4 tail stores prevCol=0xFF and JUMPS to the
   `selectedSlot = 0xFF; goto test` block shared with case 5's `!= 0xFF` arm,
   while ours merges prevCol's store into that block and falls through, which is
   the one instruction we are short. A second empty do-while between
   `prevCol = 0xFF` and `selectedSlot = 0xFF` in case 4 measures identical
   (94.17 / 892), so the block boundary is not reachable that way. */
// BEST 94.26% / 892 bytes / 223 insns against retail 896 / 224 - ONE instruction
// short, so parked (cannot link). Two residuals, and they trade against each other:
//  (a) the (active, needsOpen) callee-saved pair is MIRRORED: retail active=$s6 /
//      needsOpen=$s5, this body active=$s5 / needsOpen=$s6. The init statement order
//      moves only the prologue save order, never the pair: `active = 1; needsOpen = 1;`
//      = 94.17 / 892 with s5 saved first, `needsOpen = 1; active = 1;` = 94.26 / 892
//      with retail's s6-first order (the body below keeps the latter, the best number).
//      So this is the a-global-allocno-priority-is class: same refs, both QI and both
//      `= 1`, and the pair comes out the same way in both orders.
//  (b) retail keeps ONE extra `j`: its case-4 tail is block B {prevCol = 0xFF} -> j to
//      block C {selectedSlot = 0xFF -> break}, and C is also case 5's `!= 0xFF` arm;
//      this body merges B and C into a single block and threads case 5's bne onto that
//      block's `j`, which is the missing instruction. Retail's success arm then stores
//      selectedSlot twice (beqz delay slot, then C) - a cross-jump duplicate, not a
//      second source statement.
// ROUND 470 measured: the empty `do { } while (0)` at the end of case 2 (which restores
// retail's dispatch - beqz whose delay slot duplicates the loop test's `andi 0xFF` -
// 888 -> 892) also flips pair (a); the same do-while at the end of case 9 measures the
// same 892 / 94.17, and BOTH do-whiles together measure 892 / 94.17 again, so the two
// residuals are one reorg block-numbering parity and the net is one instruction. Also
// measured: without any do-while the body is 888 / 93.91 with retail's pair (a) but
// also missing the dispatch andi. Preset already ruled out in the notes above (2.7.2
// 93.66, cdk 84.82, -O3 65.55) and three permuter runs found no lead, so no fourth run
// below 99%.
// JUMP TABLE: this stub owns jtbl_801C50FC at +0x00 of the [0xFC, .rodata, main/misc9]
// range; a C switch emits its 0x2C table at the range start with `.align 3` padding it
// to 0x30, which is where func_801DDF24's table has to land, so converting this one is
// layout-safe on its own - the blocker is the instruction, not the table.
// Semantics (decoded, unchanged): the inventory page cursor - arg0/arg1 are unused,
// row/col index the 16-column page, input 4 selects / confirms / swaps, 5 backs out,
// 1/3/0/2 move by one column, 9/10 by eight rows, and both 0xFF sentinels force the
// row and item prims to be rebuilt.
// ROUND 483: un-parked and re-measured 94.26% / 892 bytes / 223 insns (retail 896 / 224) from
// the body below, so the note above is current. FIVE new spellings measured this round, all
// aimed at the one missing instruction, none moved it:
//  (1) case 5 with the test polarity swapped (if (selectedSlot == 0xFF) { active = 0; } else
//      { selectedSlot = 0xFF; }): byte-identical output, 94.26 / 892 / 223, 109 lines.
//  (2) case 5 with an explicit break in the then arm instead of an empty else: identical,
//      94.26 / 892 / 223.
//  (3) active = 1; needsOpen = 1; (the reversed init pair): 94.17 / 892, 113 lines - the
//      prologue then saves s5 first. The variable-to-register map does NOT move with it either
//      (active=$s5, needsOpen=$s6 in BOTH init orders), so retail active=$s6 is not an
//      init-order question: the two pseudos need a different allocno SET.
//  (4) the lever d-nested-else-keeps-a-case-copy spelling (case 4 outer test negated with the
//      nested test in its TRUE arm): 92.21 / 892, 111 lines, worse. That branch shape is
//      already retail's (bne s3,0xFF to the nested arm) in the current spelling too, so the
//      lever's premise does not apply to this residue.
// WHAT THE DUMPS SHOW, one decision rather than two: our case-4 success arm is
// instruction-identical to retail (beqz to the test with the selectedSlot store in its delay
// slot, then j to the prevCol block with the prevRow store in its delay slot). The whole
// residual is WHERE the once-only {selectedSlot = 0xFF; break} copy sits: retail keeps case
// 5 copy, which lands after case 5 body at 0x70 and is therefore separated from the prevCol
// block at 0x54 by case 5 text - that separation is what prints the extra j .L801DC070. This
// body keeps case 4 copy, which falls through from the prevCol block, so one j disappears.
// The prevCol block is the same story and AGREES with retail (case 10 copy survives, 0x158),
// so the cross-jump survivor is not simply "the later copy", and neither case-5 spelling
// flips it.
// ROUND 496: TWO source changes, both new, take this from the 94.26 / 892 park to 95.69% /
// 896 bytes / 224 insns = retail's EXACT size, so the body below is the best park:
//  (1) case 5's arm polarity swapped to `if (selectedSlot == 0xFF) { active = 0; } else
//      { selectedSlot = 0xFF; }` AND the empty `do { } while (0)` moved OUT of case 2 to the end
//      of case 5's arm (before its break). The PAIR is load-bearing, not either half: the swap
//      alone is 93.81 / 888 / 222, the case-5 do-while with case 5's ORIGINAL polarity is
//      94.89 / 896 / 107 lines (case 2's do-while present or not makes no difference there),
//      and both together are 95.69 / 896 / 107. With this body the case-4 tail
//      B {prevCol = 0xFF} is followed by case-5's text and the shared {selectedSlot = 0xFF}
//      block lands AFTER case 5 at 0x70 - retail's exact layout - so B needs its
//      `j .L801DC070` and the function reaches retail's 224 insns.
//  (2) Measured inert and left out: a do-while at the end of case 0's or case 3's arm changes
//      nothing at all (95.69 / 896 / 107 lines for all three positions).
// WHAT IS LEFT is 107 differing lines and no size: (a) the (active, needsOpen) pair is still
// mirrored (retail needsOpen=$s5 / active=$s6, this body the reverse) - rounds 468/481 closed
// that as an allocno-priority question; (b) the `col = v1` store that case 1 and case 0 share:
// retail keeps the LAST copy (at 0xF8, after case 0's body, case 1's `bnez` jumping forward to
// it) while this cc1 keeps the FIRST (inline after case 1, case 0 jumping back to it) - the
// same first-vs-last cross-jump choice as (1), and one block-numbering toggle cannot flip both
// (case 5's do-while flipped (1) and left (b) alone). Preset re-screened on THIS body, 2.6.0
// still the unit's: unit 95.69, gcc-2.7.2 95.45, gcc-2.7.2-cdk 92.59.
// ROUND 509 - BEST 99.84% / 896 bytes (= retail) / 224 insns (= retail), ONE residual.
// The buy: the four movement arms (cases 1, 0, 3, 2) must put the BIG body in the if's
// THEN arm and the single `col = col + n` in the ELSE arm (`if (col + 2 >= 0x10) { row++ ... } else col = col + 2;`),
// not the natural `if (col + 2 < 0x10) col += 2; else {...}`. With the big body first, both
// cross-jump-merged store blocks land where retail has them - the ++ family's at 0xF8 after
// case 0's body and the -- family's at 0x120 after case 2's body - and the differing lines fall
// 95.69 -> 97.77 (cases 1/0) -> 99.84 (cases 3/2) with the block order of the whole switch
// (4, 5, 1, 3, 0, 2, 9, 10) and the shared prevCol block after case 10 all retail's.
// RESIDUAL: the (active, needsOpen) pair is mirrored - retail active=$s6 / needsOpen=$s5, this
// body active=$s5 / needsOpen=$s6. Quotient-closed by the repo triage rule: -dl on this body
// (gcc-2.6.0, -O2 -G8 -da -dl -dg on build/src/menu/main/misc9.c.i) gives active = allocno 71
// "used 9 times across 176 insns; 1 bytes" = 3*9*10000/176 = 1534 and needsOpen = 72
// "used 5 times across 154 insns; 1 bytes" = 2*5*10000/154 = 649, a 2.36x gap, and processing
// order `100 111 116 107 112 120 73 117 75 76 77 74 71 72 89 90` hands s0..s6 in that
// descending order. Retail's map needs needsOpen at 8 refs (1558) or active at 5 refs (568);
// both andis pin the two as QI and the stream pins their live ranges, so no spelling reaches it.
// Measured dead this round: declaration order swap (byte-identical, map unchanged), init order
// swap (flips only the prologue's sw order, map unchanged, 99.75), an empty do/while at the end
// of case 2 (optimised away, unchanged), a 3-minute permuter run (base=25 best=25), and the
// preset on THIS body: 2.6.0 99.84, gcc-2.7.2 99.60, gcc-2.7.2-cdk does not compile.
// LAYOUT: this stub owns jtbl_801C50FC at +0x00 of [0xFC, .rodata, main/misc9] with .align 3
// padding to 0x30, which is where func_801DDF24's table must start, so converting this one is
// layout-safe on its own.
// ROUND 522: un-parked and re-measured 99.84% / 896 bytes (= retail) / 224 insns (= retail), 99 differing lines - all the mirrored (active, needsOpen) pair and its cascade. FOUR scored runs this round, no new best, plus the mandated permuter run:
// - lever merging-two-top-of-function-inits-into-one-chain (round 515, same gcc-2.6.0 preset) tried on the two '= 1' inits: 'needsOpen = active = 1;' 99.75 / 103 lines (only the prologue save order moves), 'active = needsOpen = 1;' 99.84 / 99 lines byte-identical to the two separate statements. Neither flips the map.
// - moving 'needsOpen = 1' down to just before the loop (to shorten its live range and raise its quotient) 97.91 / 107 lines: the pair does NOT flip, only the prologue's sw/li placement moves. Reverted.
// - permute.sh --minutes 3 -j 2: base=25 best=25, 1436 iterations, RESULT PERMUTER_NONE.
// - lever arithmetic re-read (a-global-allocno-priority-is): active QI 9 refs/176 insns = 1534, needsOpen QI 5/154 = 649, a 2.36x gap, and the lever's own triage says 2x or more = park; flipping needsOpen needs 8 refs and dropping active needs 4, both impossible without changing retail's instruction stream. CLOSED - do not re-brief this function.
/*
s32 func_801DBE54(void) {
    u8 active;
    u8 needsOpen;
    s32 row;
    s32 prevRow;
    s32 col;
    s32 prevCol;
    s32 selectedSlot;

    needsOpen = 1;
    active = 1;
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
        MenuBuildArrowCursorPrim(col, row, 0, 0);
        if (col != prevCol) {
            MenuBuildItemPrims(col, row);
            prevCol = col;
        }
        if (needsOpen) {
            MenuCreateWindow(3, 0xC, 0xA, 0x124, 0x84, 0, 1, 4, 1);
            MenuCreateWindow(4, 8, 0x8F, 0x130, 0x22, 0, 1, 4, 0);
            MenuStartOpenAnimation();
            needsOpen = 0;
            MenuTweenCharacterWindows(0, 0);
        }
        MenuBuildArrowCursorPrim(selectedSlot, row, 1, 1);
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
            if (selectedSlot == 0xFF) {
                active = 0;
            } else {
                selectedSlot = 0xFF;
            }
            do { } while (0);
            break;
        case 1:
            if (col + 2 >= 0x10) {
                row++;
                if (row > g_MenuInventoryRowCount) {
                    row--;
                }
            } else {
                col = col + 2;
            }
            prevCol = 0xFF;
            break;
        case 3:
            if (col - 2 < 0) {
                row--;
                if (row < 0) {
                    row++;
                }
            } else {
                col = col - 2;
            }
            prevCol = 0xFF;
            break;
        case 0:
            if (col + 1 >= 0x10) {
                row++;
                if (row > g_MenuInventoryRowCount) {
                    row--;
                } else {
                    col = 0xE;
                }
            } else {
                col = col + 1;
            }
            prevCol = 0xFF;
            break;
        case 2:
            if (col - 1 < 0) {
                row--;
                if (row < 0) {
                    row++;
                } else {
                    col = 1;
                }
            } else {
                col = col - 1;
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
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DBE54);






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
    MenuLoadResourceSet((u8)kind);
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
    MenuLoadResourceSet((kind | MENU_PANEL_FREE_OFFSET) & 0xFF);
    HeapFree(g_Menu->unk430);
    g_Menu->pManager->playTimeDigitsActive = 1;
    g_Menu->pManager->shouldRenderWindow[1] = 1;
}
// One column of the 0x430 panel is a heading string plus the value image the two
// decimal digits are rendered into; the last two columns carry the mp/fuel totals.
#define MENU_PANEL_COLUMN_COUNT 0xE
#define MENU_PANEL_MASKED_COLUMN_COUNT 0xC
// The second sprite-record set of MenuUnk430Panel holds one value image per column.
#define MENU_PANEL_VALUE_STRING 0xE
// g_MenuEquipmentSlotRowY rows the two value quad groups are drawn at.
#define MENU_PANEL_VALUE_ROW_Y 8
// Rows of MenuEquipmentSlotRecord a column's mp/fuel cost is read from.
#define MENU_PANEL_MP_DIGIT_COUNT 2
#define MENU_PANEL_FUEL_DIGIT_COUNT 5
// The panel's second sprite-record set: one value image per column, the tail of the
// same MenuString array as the column records.
#define MENU_PANEL_VALUE_STRING_COUNT 0x13
// One column's value image: a 0x28x0xD tile uploaded on a 0x18-pixel column pitch,
// one row of tiles per two columns.
#define MENU_VALUE_IMAGE_WIDTH 0x28
#define MENU_VALUE_IMAGE_HEIGHT 0xD
#define MENU_VALUE_IMAGE_COLUMN_PITCH 0x18
// Glyph codes SystemDecodeGlyphCodesN turns into texture tiles: '0'..'9' start at
// 0x10 and a suppressed (leading) digit is drawn as the blank glyph.
#define MENU_GLYPH_DIGIT_ZERO 0x10
#define MENU_GLYPH_BLANK 0xC3

typedef struct {
    /* 0x0000 */ MenuString columnStrings[MENU_PANEL_VALUE_STRING];
    /* 0x0700 */ MenuString valueStrings[MENU_PANEL_VALUE_STRING_COUNT];
} MenuUnk430ValueStrings; // Size: 0x1080

void MenuBuildUnk430PanelColumns(u_char arg0, u_char arg1) {
    u8 digitCodes[0x10];
    u8 stringBuffer[0x10];
    RECT rect;
    s32 placeValues[5];
    s32 available;
    u8* pImage;
    u8 visible;
    u8 shade;
    u8 didRound;
    s32 count;
    s32 i;
    s32 j;
    s32 k;
    s32 digit;
    s32 value;
    s32 number;
    u8 row;

    row = 0;
    available = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].mp;
    pImage = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);
    placeValues[0] = 1;
    placeValues[1] = 10;
    placeValues[2] = 100;
    placeValues[3] = 1000;
    placeValues[4] = 10000;
    for (i = 4; i >= 0; i--) {
        digitCodes[i * 2 + 1] = 0;
    }
    for (i = 0; i < MENU_PANEL_COLUMN_COUNT; i++) {
        bzero(digitCodes, 8);
        visible = 0;
        switch (arg1) {
        case 0:
            if (MenuAndBitMaskMsbFirst(
                    g_GameState.conditionRows[g_Menu->pManager->currentCharacterIDs[arg0]].characterConditionMask,
                    i) != 0 ||
                i >= MENU_PANEL_MASKED_COLUMN_COUNT) {
                visible = 1;
            }
            break;
        case 1:
            if (MenuAndBitMaskMsbFirst(
                    g_GameState.conditionRows[g_Menu->pManager->currentCharacterIDs[arg0]].gearConditionMask, i) != 0 ||
                i >= MENU_PANEL_MASKED_COLUMN_COUNT) {
                visible = 1;
            }
            break;
        case 2:
            if ((i & 1) == 0) {
                if (MenuAndBitMaskMsbFirst(
                        g_GameState.conditionRows[g_Menu->pManager->currentCharacterIDs[arg0]].statusFlags, i / 2) != 0 ||
                    i >= MENU_PANEL_MASKED_COLUMN_COUNT) {
                    visible = 1;
                }
            }
            break;
        }
        if (visible) {
            count = MENU_PANEL_MP_DIGIT_COUNT;
            if (i < MENU_PANEL_MASKED_COLUMN_COUNT) {
                switch (arg1) {
                case 0:
                    ((MenuUnk430Panel*)g_Menu->unk430)->strings[i].width = SystemRenderStringEntry(
                        GetStringEntryFromBank14(g_Menu->pManager->currentCharacterIDs[arg0] * 16 + i), pImage, 0x24, 0);
                    value = ((MenuEquipmentSlotRecord*)
                                 g_Menu->pDressingRoom->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[arg0]])[i + 22]
                                .mpCost;
                    break;
                case 1:
                    ((MenuUnk430Panel*)g_Menu->unk430)->strings[i].width = SystemRenderStringEntry(
                        GetStringEntryFromBank30(
                            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId * 16 + i),
                        pImage, 0x24, 0);
                    value = ((MenuEquipmentSlotRecord*)
                                 g_Menu->pDressingRoom->pGearBlocks
                                     [g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId])[i + 21]
                                .mpCost;
                    break;
                case 2:
                    ((MenuUnk430Panel*)g_Menu->unk430)->strings[i].width = SystemRenderStringEntry(
                        GetStringEntryFromBank34(
                            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId * 4 + i / 2),
                        pImage, 0x24, 0);
                    available = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].fuel;
                    value = ((MenuEquipmentSlotRecord*)
                                 g_Menu->pDressingRoom->pGearBlocks
                                     [g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId])[i / 2 + 37]
                                .fuelCost;
                    count = 4;
                    break;
                }
            } else if (i == MENU_PANEL_MASKED_COLUMN_COUNT) {
                if (arg1 != 2) {
                    value = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].mp;
                } else {
                    row = 3;
                    value = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].fuel;
                    count = MENU_PANEL_FUEL_DIGIT_COUNT;
                }
            } else {
                value = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].maxMp;
            }
            number = value;
            didRound = 0;
            for (j = count - 1, k = 0; j > 0; k++, j--) {
                digit = number / placeValues[j];
                if (digit != 0 || didRound) {
                    digitCodes[k * 2] = digit + MENU_GLYPH_DIGIT_ZERO;
                    didRound = 1;
                    number -= digit * placeValues[j];
                } else {
                    digitCodes[k * 2] = MENU_GLYPH_BLANK;
                }
            }
            digitCodes[k * 2] = (number % 10) + MENU_GLYPH_DIGIT_ZERO;
            SystemDecodeGlyphCodesN((u16*)digitCodes, stringBuffer, count);
            ((MenuUnk430ValueStrings*)g_Menu->unk430)->valueStrings[i].width =
                SystemRenderStringEntry(stringBuffer, pImage, 0x24, 1);
            rect.x = (i & 1) * MENU_VALUE_IMAGE_COLUMN_PITCH + 0x180;
            rect.y = (i / 2) * MENU_VALUE_IMAGE_HEIGHT + 0x80;
            rect.w = MENU_VALUE_IMAGE_WIDTH;
            rect.h = MENU_VALUE_IMAGE_HEIGHT;
            LoadImage(&rect, pImage);
            DrawSync(0);
            shade = 0x80;
            if (i < MENU_PANEL_MASKED_COLUMN_COUNT) {
                switch (arg1) {
                case 0:
                    if (MenuAndBitMaskLsbFirst(g_MenuColumnShadeMasks[g_Menu->pManager->currentCharacterIDs[arg0]], i) != 0) {
                        if (available < value) {
                            shade = 0;
                        }
                    } else {
                        shade = 0;
                    }
                    break;
                case 1:
                    shade = 0;
                    break;
                case 2:
                    if (i != 0 || available < value) {
                        shade = 0;
                    }
                    break;
                }
            }
            if (i < MENU_PANEL_MASKED_COLUMN_COUNT) {
                MenuSetStringPrimitives(&((MenuUnk430Panel*)g_Menu->unk430)->strings[i], i, 0x80, shade | 1);
                MenuSetQuadVertices(((MenuUnk430Panel*)g_Menu->unk430)->strings[i].vertices, (i % 2) * 0x88 + 0x24,
                                    (i / 2) * 0x10 + 0x12, ((MenuUnk430Panel*)g_Menu->unk430)->strings[i].width,
                                    MENU_VALUE_IMAGE_HEIGHT);
            }
            MenuSetStringPrimitives(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_VALUE_STRING + i], i, 0x80, shade | 2);
            MenuSetQuadVertices(
                ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_VALUE_STRING + i].vertices, g_MenuPanelValueQuadX[i][0],
                g_MenuPanelValueQuadY[i][0],
                ((MenuUnk430ValueStrings*)g_Menu->unk430)->valueStrings[i].width, MENU_VALUE_IMAGE_HEIGHT);
            ((MenuUnk430Panel*)g_Menu->unk430)->strings[i].renderContext = g_Menu->renderContext;
            ((MenuUnk430ValueStrings*)g_Menu->unk430)->valueStrings[i].renderContext = g_Menu->renderContext;
            ((MenuUnk430Panel*)g_Menu->unk430)->columnFlags[i] = shade | 1;
        } else {
            ((MenuUnk430Panel*)g_Menu->unk430)->columnFlags[i] = 0;
        }
    }
    HeapFree(pImage);
    MenuSetRowQuadVertices(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuColumnRowStringIds,
                           g_MenuPanelRowXOffsets, g_Menu->pManager->shouldRenderStrings10E0, 6, 1, row + 2);
    MenuSetRowQuadVertices(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuColumnRowStringIds,
                           g_MenuPanelRowXOffsets, g_Menu->pManager->shouldRenderStrings10E0, 7, 1, row + 2);
    MenuSetStringTexturePrims(&((MenuUnk430Panel*)g_Menu->unk430)->strings[0x20], arg0, arg1, 1);
    g_Menu->pManager->shouldRenderPanel[0] = 1;
}
// PARKED 97.81% / 1924 bytes / 481 insns against retail's 1928 / 482 - ONE instruction
// SHORT, so this body cannot link as it stands. Retail's whole instruction BLOCK SHAPE
// is reproduced now (andi 0x4000 / beqz / andi 0x1000 in the beqz delay slot / j /
// li v1,2 in the j delay slot / sltiu) thanks to the fold-blocker below, and the one
// missing instruction is retail's `move v1,v0` after the `sltiu v0,v0,1`: retail keeps
// the compare result in $v0 and copies it into row's $v1, ours coalesces the two.
// FOLD-BLOCKER (new this round, found by permuter.sh base 1605 -> 1112):
// gcc 2.6.0's combine folds `(flags & 0x1000) == 0` into srl/xori/andi whenever the
// andi and the compare share a basic block. `< 1` (97.66), a u_int/u_char boolean temp
// (97.81/97.52), `!(...)` and a `masked = flags & 0x1000;` statement before the if
// (97.30, cond/masked pair) all either still fold or re-assign the registers. The
// spelling that blocks the fold and keeps every register retail has is a compare
// against row itself:
//     row = 1;
//     row = (flags & 0x1000) < row;
// which is the same value: row = ((flags & 0x1000) == 0).
// Still owed after that: the two MenuSetRowQuadVertices call setups use
// $s1/$s2/$s0 where retail has $s0/$s1/$s2 (g_MenuColumnRowStringIds /
// g_MenuPanelRowXOffsets / the literal 2) - a downstream allocator rotation (the old
// note's "delta 3"), probably triggered by the missing move.
// The 97.66% spelling for the same block is `row = (flags & 0x1000) < 1;` with no
// `row = 1;` line: retail's EXACT 1928 bytes / 482 insns, but the else folds to
// srl v1,12 / xori / andi.
// PARAMETER TYPE: retail's body masks $a1 four times, and the definition must be
// `s32 arg1` - `u_char` gives 483 insns / 1932 bytes and a spurious `move a0,a1`.
// The file-scope declaration above was u_char, which made the definition a
// conflicting-type error (cc1 exit 33); it is s32 now for that reason. Re-check it if
// a caller's park (func_801DD790, 95.96%) starts measuring differently.
// Merged this round: case 2's stringId sum written id-side first
// (`...gearId * 8 + (arg1 & 0xFE)`), which restores retail's `addu s0,v0,v1`
// (97.64 -> 97.66). Also confirmed the second switch's three arms and the tail
// (4 sb, shouldRenderColumnHeadings, the two shouldRenderStrings stores, the else
// loop) are already retail's.
// try_flags: gcc-2.7.2 96.93, 2.7.2-cdk compile-fails, 2.6.0 with -G0 or -O3 no change.
// No dlabel data and no jump table in this stub's .s, so it converts the moment the
// body matches.
// --- carried over from the round-397 note ------------------------------
// Best: 97.64%, 1928 bytes - and retail's object is ALSO 1928 bytes / 482 insns
// (objdiff-cli: expected/asm/menu/main/misc9.s.o func_801DCE60 size 1928; the
// old note's "retail 1924 = 481" was STALE). Byte count is exact: only three
// equal-count spelling deltas remain. Fills one column of the 0x430 panel:
// renders the column's two heading strings into a scratch buffer, uploads them,
// points the four tail sprite records at them, copies the selected column's
// record into MENU_PANEL_COLUMN_STRING and sets the row flags the prim adder
// in misc5 renders. arg2 selects the panel kind (item / gear list / gear two
// columns) and with it the string id base and the dressing room prim block.
// Confirmed retail's: prologue and save set, frame 0x48, both switch trees,
// every call site and store, the shared exits, the 0x4E/0xD constants hoisted
// to $s0/$s1, and the FIRST switch's per-arm code (arm asm is byte-identical).
// Measured this round (round r397 worker lane):
// - try_flags: gcc-2.7.2-psx 96.93, gcc-2.7.2-cdk compile-fails, 2.6.0 with
// -G0 or -O3 no change (97.64). The preset is right; all three residuals are
// 2.6.0 source/allocator issues.
// - Delta 1 (kind 2's shared addu) is a CROSS-JUMP tie: the three arms' sum
// insns must be text-identical to share ONE addu. Ours all read addu s0,v1,v0
// (first RTL operand in v1: id for kinds 0/1, arg1 and 0xFE for kind 2),
// retail's all read addu s0,v0,v1. Flipping the C operand order in ALL THREE
// arms (arg1-side first in 0/1, gearId first in 2) restructures: gcc emits the
// id LOAD first either way, but the arg1 side then lands in $a0 and arms 0/1
// cross-jump a 3-insn tail (sll, sll, addu) that retail does not have:
// 96.09%, 480 insns.
// - Delta 2 (row flag) MECHANISM FOUND: gcc 2.6.0's combine folds
// (flags and 0x1000) == 0 into srl 12, xori 1, andi 1 whenever the andi and
// the seq are in the SAME basic block. Retail's andi v0,v1,0x1000 sits in
// the beqz DELAY SLOT and the compare stays sltiu+move. The spelling that
// defeats the fold AND keeps retail's delay-slot fill (measured 97.28%, the
// whole row-flag shape exact: andi 0x4000, beqz, andi 0x1000 delay, j,
// li v1,2 delay, sltiu):
// cond = flags & 0x4000;    (u_int)
// masked = flags & 0x1000;  (u_int)
// if (cond) row = 2; else row = masked == 0;
// Residual at 97.28: ours loads flags into v0 and cond into v1 (retail:
// flags in v1, cond+masked in v0) and lacks retail's move v1,v0 (a
// tmp = masked == 0; row = tmp; split is UNTRIED and may restore the move).
// u16 cond/masked instead of u_int reorders the andis and inverts the branch
// (96.69%, 480 insns); masked computed BEFORE the if also inverts the branch,
// because the else shrinks to one insn and reorg puts the sltiu itself in
// the beqz delay slot (96.69%, 480 insns).
// - Delta 3 (tail: g_MenuColumnRowStringIds / g_MenuPanelRowXOffsets / the
// literal 2 in $s1/$s2/$s0 vs retail's $s0/$s1/$s2, plus the second call's
// setup scheduled around the first call's tail) is probably DOWNSTREAM of
// delta 2: retail's move v1,v0 adds a pseudo that reshuffles the allocator.
// Close delta 2 first, then re-read this.
// Realism owed: flags = (u16) pBase read and (pBase[0] and 3) are raw reads of
// a prim block. The panel itself is typed by MenuUnk430Panel above.
// NOTE (superseded): the round-400 note said the second parameter is u_char. It is
// not: with `u_char arg1` the body compiles to 483 insns / 1932 bytes and gains a
// spurious `move a0,a1`, while `s32 arg1` reproduces retail's exact 482 / 1928 (the
// body masks $a1 four times itself, and a caller passing a u_char local still emits
// its own `andi` zero-extension at the call site). The file-scope declaration at the
// top of this file was changed from u_char to s32 for that reason - otherwise the
// definition is a conflicting type (cc1 exit 33) and nothing compiles.
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
        stringId = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId * 8 + (arg1 & 0xFE);
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
        MenuSetStringPrimitives(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING], 0, 0, 0);
        MenuSetFt4QuadVertices(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].polys[g_Menu->renderContext],
                               0x1C, 0x9E, 0, 0x4E,
                               ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].width, 0xD);
        MenuSetQuadVertices(((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].vertices, 0x1C, 0x9E,
                            ((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING].width, 0xD);
        MenuSetStringPrimitives(&((MenuUnk430Panel*)g_Menu->unk430)->strings[MENU_PANEL_HEADING_STRING + 1], 1, 0, 0);
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
            row = 1;
            row = (flags & 0x1000) < row;
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DCE60);

void MenuSetUnk430PanelShading(u_char arg0) {
    MenuString* pString;
    POLY_FT4* pPoly;
    void* pPanel;
    s32 i;
    s32 rc;

    func_801E8F60(5, arg0);
    func_801E8F60(6, arg0);
    for (i = 0; i < MENU_PANEL_ROW_STRING_COUNT; i++) {
        pPanel = g_Menu->unk430;
        pString = &((MenuString*)pPanel)[i];
        rc = pString->renderContext;
        if (*(u8*)((u8*)g_Menu->unk430 + (i * 0x80 + rc * 0x28) + 4) != 0x20) {
            MenuSetPrimShadingMode(&((MenuString*)g_Menu->unk430)[i].polys[rc], arg0);
            pPoly = &((MenuString*)g_Menu->unk430)[MENU_PANEL_SHADED_ROW_STRING + i].polys[((MenuUnk430RowStrings*)g_Menu->unk430)->shadedRowStrings[i].renderContext];
            MenuSetPrimShadingMode(pPoly, arg0);
        }
    }
    MenuSetPrimShadingMode(&g_Menu->arrowCursors[0]->polys[g_Menu->arrowCursors[0]->renderContext], arg0);
    MenuSetPrimShadingMode(&((MenuString*)g_Menu->unk430)[MENU_PANEL_COLUMN_STRING].polys[((MenuString*)g_Menu->unk430)[MENU_PANEL_COLUMN_STRING].renderContext], arg0);
    for (i = 0; i < 2; i++) {
        do {
            MenuSetPrimShadingMode(&((MenuString*)g_Menu->unk430)[MENU_PANEL_HEADING_STRING + i].polys[((MenuUnk430RowStrings*)g_Menu->unk430)->headingStrings[i].renderContext], arg0);
        } while (0);
    }
}

/* PARKED 95.96% (round 385 re-measure; the old 95.71 note is superseded). Retail
   EXACT 1940 bytes and a byte-for-byte identical instruction MULTISET; the only
   residual is which callee-saved register each long-lived value gets. Both compiles
   save the same set (ra + s0-s8), so this is NOT over-allocation and freeing a
   register cannot help:
     retail  s0=arg0/i  s1=refresh  s2=arg2(raw, then 'found')  s3=group
             s4=mask  s5=slot  s6=arg1  s7=charSlot  s8=pRecord
     ours    s0=arg0/i  s1=group(and raw arg2, same reg)  s2=mask
             s3=refresh  s4=slot  s5=found  s6=arg1  s7=charSlot  s8=pRecord
   i.e. retail masks arg2 into a FRESH register (s3) and later lets 'found' reuse
   the dead raw arg2 (s2); ours masks arg2 IN PLACE (s1) so group occupies arg2's
   register for the whole loop and 'found' has to take a new one, rotating
   refresh/mask/slot up by one. cc1 2.6.0 -dl/-dg (build/src/menu/main/misc9.c.i.lreg)
   shows the loop-live u8 quantities as 182/80/79/181/83 with the note's
   priority->_first_free_register map, so the ordering, not the shape, is the delta.
   Measured this round, all at 1940 bytes / retail's size: 'mask = 0;' moved from
   just after MenuRenderFrame to after the 'if (refresh)' block 95.71 -> 95.96
   (kept - it is also the more natural source), the hp-gate/arms comparing the
   local 'group' instead of 'arg2' (kept, retail compares s3), 'for (i = 0,
   refresh = 0; ...)' so the found/i/refresh init order is retail's (kept), a live
   copy of charSlot used by the else arm (no change, dropped). permute.sh
   --minutes 6 -j 2 (2082 iters) 2580 -> 2209 but only via 'volatile u32
   maxValue', which is 1952 bytes (12 over retail) and cannot link - same find as
   the previous round, not portable.
   Facts for the next agent: pManager (0x33C) keeps the party slots' ids at +0x30
   (3 bytes, 0xFF = empty); pDressingRoom (0x330) has pCharacterSlots[11] at +0x20 and
   pGearBlocks[20] at +0x4C. The three case arms build pRecord as
   '&pSlots[arg1] + 22/21/37' (0x370/0x348/0x5C8 bytes), i.e. 22/21/37 0x28-byte
   records precede the walked array. The hp/mp gate reads characters[id].mp
   (g_GameState+0x2BC, the record's 0x13 byte is an mp cost) for group != 2 and
   gears[gearId].fuel (+0x9B0, the record's 0x24 u16 a fuel cost) for group == 2;
   case 4 compares characters[].hp/.maxHp (+0x2B8/+0x2BA) or gears[].hp (+0x9D8)
   against gears[characters[currentCharacterIDs[arg1]].gearId].maxHp (+0x9DC) and the
   second index really is arg1 (the equipment column). The declaration
   'void func_801DCE60(u_char, u_char, u_char)' must stay: retail masks the second
   argument at this call site. Switch case order is 5, 4, 1, 3 (retail's body order).
*/
// ROUND 451: re-unparked and re-measured 95.96 / 1940 bytes / 485 insns vs retail 485 - the
// multiset equality in the note below still holds. Preset screen (try_flags --sweep) says the
// preset is right: 2.6.0 -O2/-O3 x -G4/-G8 x 3000/r8000 all 95.96, gcc-2.7.2 95.33, 2.7.2-cdk 84.55,
// -O1 71-81. So the residual is the allocator, not the compiler.
// WHY THE ROTATION IS NOT REACHABLE BY SPELLING (new evidence, -da/-dl/-dg on this body, cc1
// 2.6.0 -O2 -G8): the five loop-live quantities carry these priority terms
// (floor_log2(refs)*refs*10000/(live_insns*size)):
//   group   21 refs / 231 insns / 1 byte   = 3636
//   mask    23 refs / 296 insns / 1 byte   = 3108
//   refresh 18 refs / 250 insns / 1 byte   = 2880
//   slot    16 refs / 232 insns / 1 byte   = 2758
//   found    7 refs /  74 insns / 1 byte   = 1891
//   arg2     3 refs /  84 insns / 1 byte   =  357   (-dl prints the literal "1 bytes")
// which is exactly our hand-out (group s1, mask s2, refresh s3, slot s4, found s5). Retail
// hands out refresh s1, arg2 s2, group s3, mask s4, slot s5, i.e. it needs arg2 (357) to
// outrank refresh (2880) - an 8x gap on a QI parameter, the case lever
// a-qiparam-size-one-pins-the-saved-pair calls terminal - and it needs refresh to outrank
// both group and mask, which no body that emits these same 485 instructions can do. Retail
// also lets the dead arg2 register be reused by found; ours cannot, since group IS arg2s
// allocno (182 and 74 both land in s1 non-conflictingly), which is the whole cascade.
// MEASURED THIS ROUND, the only shape that moved anything: s8 group (signed) 94.41 / 1956
// bytes / 489 insns - the sign bit costs an andi at each u_char call site, so group is QI and
// must stay u8. No permuter run: the park lane reserves that for >=99%.
// ROUND 465: re-measured the body below at 95.96 / 1940 bytes / 485 insns against retail's
// 485 - the instruction multiset is still identical, only the callee-saved hand-out differs
// (retail refresh s1 / arg2+found s2 / group s3 / mask s4 / slot s5; ours group+arg2 s1 /
// mask s2 / refresh s3 / slot s4 / found s5). One new axis CLOSED by measurement: `s32
// refresh` (SI, 4x allocno size) gives 482 insns / 1928 bytes / 95.63, because retail's
// refresh truth test is `andi v0,s1,0xff` - refresh is QI-pinned, and so are mask (`andi
// v0,s4,0xff` before each of its tests) and group (its `== 2` / `== 0` tests reuse the
// preheader's `andi s3,s2,0xff`). allocno_size is therefore fixed at 1 for all five
// loop-live quantities and the only lever that could lift refresh's 2880 over group's 3636
// is gone: group needs <= 16 refs (it has 21, all of them pinned by retail's own call and
// test sites) or a 292-insn live range (it spans the loop), and refresh needs 24 refs on
// the same instructions. Same verdict as rounds 385/451 by arithmetic, not by taste.
//
// ROUND 478: re-measured the body below at 95.96% / 1940 bytes / 485 insns (retail's exact size) - UNCHANGED, and the instruction multiset is still identical, so every one of the 188 differing lines is a callee-saved register name. CLOSED THIS ROUND, with numbers: (1) -da/-dl/-dg dump on this body: 23 allocnos, processed 89 189 76 452 88 84 166 77 90 195 80 79 182 181 83 73 85 78 71 197 74 87 86; the five loop-live values are 80 'used 23 times across 298 insns; 1 bytes' -> $s1, 79 '18 / 251' -> $s2, 182 '16 / 233' -> $s3, 181 '16 / 234' -> $s4, 83 '7 / 75' -> $s5, i.e. 3087 / 2868 / 2746 / 2735 / 1866 by the priority quotient - exactly the hand-out, and refresh ($s1 under retail) is 4th. THE REAL SEAM IS NOT THE PRIORITY, IT IS THE TIE: pseudo 80 is arg2 AND group coalesced into one allocno (retail emits 'andi s3,s2,0xff', a real copy into a second register), so retail's own map needs a SIXTH cross-call value that our RTL does not have. (2) MEASURED the way to break that tie: '$s32 group' (SI vs QI, combine_regs refuses a cross-mode tie) does separate arg2 and group - but it costs ONE instruction (486 / 1944 bytes, 95.70%) because the SImode qty needs a truncating 'andi $16,$17,0xff' before the MenuBuildUnk430PanelColumns u_char argument, and gcc does not carry nonzero_bits for it across the block (same 486 with 'group = arg2 & 0xFF'), and the resulting map is a DIFFERENT permutation (arg0 lands in $s4, retail has it in $s0), 251 differing lines. (3) PRESET AXES CLOSED: -O3 95.96 / -G0 95.96 / gcc-2.7.2-cdk 84.55 (try_flags.sh), so the unit's 2.6.0 -O2 -G8 is right. (4) PROBE: spelling two of the three 'group != 2' tests as 'arg2 != 2' (same value, arg2 is u_char) makes arg2 loop-live and costs 4 instructions (489 / 1956 / 92.68%), which proves retail's three group tests really do read group. Next lane should attack the arg2/group coalescing itself (a source shape where the copy is not tied) rather than the ref/live arithmetic.
/*
void func_801DD790(u_char arg0, s32 arg1, u_char arg2) {
    MenuPointerCursors* pCursors;
    MenuEquipmentSlotRecord* pSlots;
    MenuEquipmentSlotRecord* pRecord;
    u8 refresh;
    u8 mask;
    u8 slot;
    u8 group;
    u8 found;
    s32 i;
    s32 charSlot;
    s32 flag;
    s32 heightAdd;
    s32 value;
    s32 maxValue;

    mask = 1;
    refresh = 1;
    heightAdd = 0;
    charSlot = g_Menu->characterSlot;
    switch (arg2) {
    case 0:
        pSlots = g_Menu->pDressingRoom->pCharacterSlots[g_Menu->pManager->currentCharacterIDs[arg0]];
        pRecord = &pSlots[arg1] + 22;
        break;
    case 1:
        pSlots = g_Menu->pDressingRoom->pGearBlocks[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId];
        pRecord = &pSlots[arg1] + 21;
        break;
    case 2:
        pSlots = g_Menu->pDressingRoom->pGearBlocks[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId];
        heightAdd = 0x18;
        charSlot = arg0;
        pRecord = &pSlots[arg1] + 37;
        break;
    }
    MenuCreateWindow(2, 0x10, 0xE, 0x90 + heightAdd, 0xB0, 0, 0, 4, 0);
    flag = pRecord->flags & 1;
    while (mask) {
        slot = arg0;
        group = arg2;
        MenuRenderFrame();
        if (refresh) {
            MenuBuildUnk430PanelColumns(slot, group);
            func_801DCE60(slot, arg1, group);
            MenuSetUnk430PanelShading(1);
            MenuBuildPartyCharacterPrims(group);
            refresh = 0;
        }
        mask = 0;
        pCursors = g_Menu->pCursors;
        pCursors->shouldRender[2] = 0;
        pCursors->shouldRender[1] = 0;
        pCursors->shouldRender[0] = 0;
        if (flag) {
            i = 0;
            while (1) {
                if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
                    mask |= 1 << i;
                    g_Menu->pCursors->shouldRender[i] = 1;
                }
                i++;
                if (i >= 3) {
                    break;
                }
            }
        } else {
            mask = 1 << charSlot;
            g_Menu->pCursors->shouldRender[charSlot] = 1;
        }
        g_Menu->pManager->shouldRenderCursors = 1;
        if (group != 2) {
            value = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].mp - pRecord->mpCost;
        } else {
            value = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].fuel - pRecord->fuelCost;
        }
        if (value < 0) {
            mask = 0;
        }
        if (mask == 0) {
            break;
        }
        switch (g_Menu->input) {
        case 5:
            mask = 0;
            break;
        case 4:
            found = 0;
            for (i = 0, refresh = 0; i < 3; i++) {
                if (MenuAndBitMaskLsbFirst(mask, i) != 0) {
                    if (group != 2) {
                        value = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].hp;
                        maxValue = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].maxHp;
                    } else {
                        value = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].gearId].hp;
                        maxValue = g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg1]].gearId].maxHp;
                    }
                    if (value != maxValue) {
                        found = 1;
                        refresh = 1;
                    }
                    if (refresh) {
                        MenuApplyDressingRoomSlotHpBonus(g_Menu->pDressingRoom, g_Menu->pManager->currentCharacterIDs[arg0], g_Menu->pManager->currentCharacterIDs[i], arg1, group);
                    }
                }
                refresh = 0;
            }
            if (found) {
                if (group != 2) {
                    g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].mp -= pRecord->mpCost;
                } else {
                    g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].fuel -= pRecord->fuelCost;
                }
                MenuPlaySoundEffect(0x37);
            } else {
                MenuPlaySoundEffect(4);
            }
            refresh = 1;
            break;
        case 1:
            if (group == 0) {
                charSlot = MenuStepPartySlot(charSlot, 0, 0);
            }
            break;
        case 3:
            if (group == 0) {
                charSlot = MenuStepPartySlot(charSlot, 1, 0);
            }
            break;
        }
    }
    pCursors = g_Menu->pCursors;
    pCursors->shouldRender[2] = 0;
    pCursors->shouldRender[1] = 0;
    pCursors->shouldRender[0] = 0;
    MenuSetUnk430PanelShading(0);
    g_Menu->pManager->shouldRenderCharacters = 0;
    MenuRenderFrame();
    if (g_MenuCharactersAllocated) {
        for (i = 0; i < 3; i++) {
            HeapFree(g_Menu->currentCharacters[i]);
        }
        g_MenuCharactersAllocated = 0;
    }
    MenuFreeWindow(2);
}*/
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
/* PARKED 94.46%, 884 bytes - retail's exact size, 221 insns each - re-measured this
   round from the body below. The only residual is which value gets the ninth
   callee-saved register: retail never allocates the a6=1 argument (it rematerialises
   `li t0,0x1` before each of the four MenuCreateWindow calls) and keeps needsOpen in
   $s5, which is why its `move s5,zero` lands in the `if (arg1)` branch delay slot;
   cc1 2.6.0 hoists that same constant into a register (`li s8,0x1` below), so
   needsOpen spills to 0x38($sp) and the frame is 0x70 against retail's 0x68. The
   prologue, both dispatch sites, all eight case bodies, every call and every other
   delay slot are retail's.
   GAINED THIS ROUND, and it is in the file for good: func_801DCE60's second
   parameter was declared s32 but is a u_char - retail masks it at this call site
   (`andi a1,s1,0xff`) and func_801DCE60's own body masks $a1 before `sll` - so the
   declaration at the top of this file is now (u_char, u_char, u_char) and the
   function went 94.14 -> 94.46.
   Tried in earlier rounds (notes above): needsOpen/active/slot/kind declaration
   order (94.45-94.46), a preheader `s32 mode = 4;` for the a7 argument (94.81 but
   888 bytes, 4 over retail, so it cannot link), `u_char border = 1;` for a6
   (identical), moving the needsOpen init after the prologue calls (93.31), and a
   permuter run whose winner assigned needsOpen before the window-4 call - wrong code,
   not ported.
   JUMP TABLE: this stub owns jtbl_801C512C at +0x30 of the unit's .rodata range
   (0xFC..0x18C = three 0x30-byte tables: func_801DBE54 at +0x00, this one at +0x30,
   func_801E05D0 at +0x60). C .rodata lands before asm .rodata, so convert
   func_801DBE54 and this one TOGETHER (C tables then land at +0x00/+0x30), leave
   func_801E05D0 as INCLUDE_ASM at +0x60, and judge with check_bin.sh menu.
   ROUND 420: un-parked and re-measured, 94.46% at 884 bytes = retail's exact
   size, so the note is current, and the whole diff is the nine callee-saved
   hand-out plus needsOpen's spill: retail keeps the `4` (fp) and rematerialises
   the a6 `1` four times, ours keeps BOTH (s7=4, s8=1) and spills needsOpen to
   0x38(sp), which is exactly the extra saved word that makes the frame 0x70
   against retail's 0x68. Parked as is.
   ROUND 433: still 94.46 percent / 884 bytes / 221 insns = retail's exact size,
   measured with the file-scope prototype of func_801DCE60 put back to
   (u_char, u_char, u_char): retail masks the second argument at this call site
   (andi a1,s1,0xff) and the s32 declaration that func_801DCE60's own park needs
   costs 4 bytes here (94.14). Register map read both ways this round: retail
   spills `kind` (0x38, reloaded with lw in cases 0 and 2) and rematerialises the
   a6 constant 1 at each of the four window calls, which is what leaves s5 for
   needsOpen; ours keeps the a6 1 in s8 and spills BOTH kind (0x40) and needsOpen
   (0x38) - that fourth local slot is the extra saved word that makes our frame
   0x70. Both bodies hold eleven live values in nine callee-saved registers and
   spill two, so the residual is a pure priority tie between needsOpen (3 refs) and
   the shared const-1 pseudo (4 refs): gcc 2.6.0 prefers the constant, retail's
   compiler preferred the variable. Tried this round: `s32 kind` with
   `kind = arg2 & 0xFF` (reproduces retail's lw compare but 90.28 / 892 bytes), a
   preheader `s32 mode = 4;` passed as a7 (93.80 / 888, 4 over), and permuter
   4 min from the 94.46 body: PERMUTER_BETTER 4368 -> 270, but the winner passes
   needsOpen where retail passes 0 (MenuStepPartySlot's direction and
   MenuFreeArrowCursor's index) - wrong code, rejected. Preset screen: gcc-2.7.2
   scores 94.81 here but the unit cannot switch (under it
   MenuFitInventoryWindowToItems 96.00, MenuBuildUnk430PanelColumns 98.87,
   MenuRunUnk430Panel 67.27, MenuEquipItemFromPanel 98.62,
   MenuRenderEquipmentSlotStrings 96.83, MenuReturnWeaponToInventory 98.11, all
   100.00 under 2.6.0). The u_char declaration is also what func_801DD790's park
   wants; func_801DCE60's own parked body measured 482 insns / 1928 bytes with
   s32 arg1, so reconcile the two when that one is un-parked.
   ROUND 446: un-parked and re-measured, still 94.46% / 884 bytes / 221 insns,
   retail's exact size, so the note stands and the tie is now PROVEN by the
   allocator dump rather than inferred. `cc1 -da -dg` on this body prints
   `;; 16 regs to allocate: 149 147 159 84 78 80 75 79 97 81 103 102 95 77 76 73`
   and dispositions `75 in 19 78 in 17 79 in 20 80 in 18 81 in 22 83 in 16
   84 in 16 85 in 2 94 in 2 97 in 21 102 in 30 103 in 23 ...`: allocno 103 is the
   a7 `4` and takes $s7 ($23), allocno 102 is the a6 `1` and takes $fp ($30) -
   its own RTL is `(set (reg:SI 30 $fp) (const_int 1))` with `REG_EQUIV
   (const_int 1)`, i.e. gcc knows the value is rematerialisable and still spends
   a callee-saved register on it because loop.c hoisted it out of the do-while -
   while the needsOpen allocno (u8, 3 refs) gets NO register at all. So this is
   levers.md `a-shared-const-argument-outranks-a-three-ref-byte-flag`: the flag
   would have to reach ~8 refs (instructions retail does not contain) to win the
   quotient, which makes it a park and not a spelling hunt.
   Preset re-screen, and it closes the last cheap axis: gcc-2.7.2's 94.81 is the
   SAME residual, not a different one - that compiler's object for this body has
   the identical `li s8,1` + `li s7,4` + flagged spill (`lbu v0,0x38(sp)`) and the
   same 0x70 frame - so there is no preset lead here (cdk 86.95, -O3 67.90). */

// ROUND 459: re-measured 94.46%, and the size claim in the round-433 note was WRONG - ours
//   is 884 bytes / 221 insns against retail's 888 / 222 (the symbol size in
//   expected/asm/menu/main/misc9.s.o), so this body is ONE INSTRUCTION SHORT, not at
//   retail's size. The accounting is exact: retail spends four `ori t0,0x1` inside the
//   `if (needsOpen)` block (one per MenuCreateWindow a6 argument) and keeps the flag in
//   $s5; this body hoists that constant into $s8 (+1 `li`), spills the flag (+3: `sb
//   t0,0x38(sp)` in the prologue, `lbu v0,0x38(sp)` in the loop and the nop that load
//   forces), and 3 - 4 lands it one instruction short. So flipping the tie to retail's
//   shape is worth exactly +4 bytes and 100.00%, and the residual is the recorded lever
//   a-shared-const-argument-outranks-a-three-ref-byte-flag (round 433 spelling sweep;
//   round 446 `-da -dg`: gcc hoists the REG_EQUIV (const_int 1) into $fp and gives the
//   3-ref u8 flag no disposition at all). Nothing new to port - park on the tie.
//   One more shape measured this round and NOT kept: `s32 needsOpen` with the loop test
//   written `if ((u8)needsOpen)` scores 94.59% at 888 bytes = retail's exact size, but it
//   gets there from the other side - the flag still spills, now as a WORD (`sw t0,0x38(sp)`
//   / `lw v0,0x38(sp)`), and it LOSES retail's `andi v0,s5,0xff` because a word test needs
//   no mask. Retail masks the flag at the test and clears it with `move s5,zero`, so the
//   original flag is a byte; the s32 body is a size coincidence, not the source shape.
//   JTBL CORRECTION: the round-293 line below is wrong, this stub DOES own
//   jtbl_801C512C (its .s has the dlabel at 0x801C512C - 11 cases plus a trailing 0
//   word - and the dispatch lui/addu/lw references it), and the unit owns its rodata
//   ([0xFC, .rodata, main/misc9]), so converting this switch moves its table to +0x00 of
//   the range where retail has it at +0x30. func_801DBE54 owns +0x00 (its switch body is
//   parked) and func_801E05D0 owns +0x60 (asm): a C table is 0x2C bytes and cc1 aligns
//   the next one to 0x30, so un-parking func_801DBE54 and this one TOGETHER lands them at
//   +0x00/+0x30 and the asm .align 3 puts func_801E05D0's table at +0x60 - do the pair
//   once the register tie is solved, never this one alone, and prove with check_bin.
// ROUND 473: BEST 94.81% / 888 bytes / 222 insns = retail's EXACT size and insn count, 172 differing
// lines (the previous park was 94.46 / 884 / 221 / 179). The gain is one register's worth of shape:
// 's32 mode = 4;' assigned next to 'kind = arg2;' and passed as the FOURTH MenuCreateWindow's a7
// argument. That gives our body retail's s8 = the 4 (retail: 'li s8,0x4' / 'sw s8,28(sp)' x4) instead
// of s8 = the const-1, which is where the 0.35 came from. The round-433 note dismissed this variant as
// '4 bytes over retail'; that was measured against a wrong retail size - retail IS 888/222 (round 459),
// so the variant is retail's exact size and is the body below.
// RESIDUAL (unchanged, one tie): retail keeps needsOpen in s5 and REMATERIALISES the a6 const 1
// ('li t0,1' before each of the four window calls, i.e. that global allocno lost and gcc used
// REG_EQUIV); ours hoists the 1 into s7 ('li s7,0x1' in the prologue), so needsOpen spills to 0x38 and
// the frame is 112 against retail's 104 - FOUR local slots against three. Both bodies hold 8 loop
// values in s1-s8 with s0 as the kind/slot-mask temp; the ONLY difference in the value set is the
// const-1, which is in ours and not in retail's.
// MEASURED DEAD THIS ROUND: a BLOCK-scope 'u_char border = 1;' as the first statement of the
// if (needsOpen) block, passed as a6 - BYTE-IDENTICAL to the literal (94.81 / 888 / 222, same 172
// lines), so gcc CSEs the block-scope initialiser with the four literals and loop.c hoists it anyway;
// this closes the last idea for keeping the const out of the register set from the variable side
// (the function-scope form was already known identical). See the earlier notes above for the
// allocator dump, the type and preset sweeps, and the two permuter runs (both winners wrong code).
// ROUND 486: un-parked and re-measured, still 94.81% / 888 bytes / 222 insns, and 'nm -S' on both
//   objects prints 0x378 for func_801DDF24, so this body IS retail's exact size (the round-459/473
//   accounting stands) and the 172 differing lines are the one-register rotation plus needsOpen's
//   spill. No new lever to try: lever.py returns a-shared-const-argument-outranks-a-three-ref-byte-flag,
//   whose -da -dg conclusion is that neither side can be spelled out of the tie (the a6 const 1 is
//   4 refs against the byte flag's 3, and its live range cannot be extended from source because the
//   only SI-const-1 uses in the body are the four a6 argument slots). Parked unchanged.
//   NEW THIS ROUND, and it decides what a 100.00 here is worth: this stub CANNOT be committed alone
//   even at 100%. Its jtbl_801C512C has to sit at +0x30 of the unit's own [0xFC, .rodata, main/misc9]
//   range and C .rodata lands before asm .rodata, so converting this stub by itself puts its table at
//   +0x00 and swaps it with func_801DBE54's jtbl_801C50FC (asm-owned at +0x00) - and the two tables
//   are NOT byte-identical (different .L targets), so check_bin would fail with every function at
//   100.00%. func_801DBE54 (parked at 93.91% / 888 bytes against retail's 896) and this one have to be
//   converted TOGETHER.
// ROUND 499: BEST 94.87% / 888 bytes / 222 insns = retail's exact size and insn count, up from
//   94.81 (170 differing lines against 172). The gain is one line's position: 'mode = 4;' is
//   assigned AFTER the two prologue calls instead of beside 'kind = arg2;', so our preheader now
//   reads retail's shape - kind spill, sll of kind*4, then the mode li - instead of retail's li
//   slot being taken by the CSE'd const 1. Measured worse on the same body: 'u_char mode' 94.46 /
//   884, -O1 77.75, gcc-2.7.2 'compile-failed' in try_flags on this body (earlier rounds scored
//   the near-identical 94.81 body 94.81 under it, same residual), and (earlier rounds) s32
//   kind 90.28, s32 needsOpen 94.59, literals for both constants 94.46, cdk 86.95, -O3 67.90.
//   THE RESIDUAL IS STILL THE ONE TIE (levers.md a-shared-const-argument-outranks-a-three-ref-
//   byte-flag) and this round closes its accounting: both bodies save the same ten words (s0-s8
//   + ra) and use the same eight dedicated registers; retail's NINTH is needsOpen, and its
//   scratch s0 survives the four window calls (retail holds the 0x18 in s0 across calls 3 and 4
//   - one 'li s0,0x18' then a bare 'sw s0,16(sp)' - and sinks 'move s5,zero' into the arg1-test
//   delay slot). Ours spends the ninth on the const 1: needsOpen spills to 0x38 (sb in the
//   save block, lbu+nop at the test, sb zero sunk to the top of the block), the 0x18 is
//   rematerialised twice, and the const is one 'li s7,0x1' in the preheader against retail's
//   four 'ori t0,1'. Net byte count identical, mix different, 4 local slots against 3 (frame
//   112 vs retail's 104). The const only loses its register in a compile whose cse does not
//   merge same-block constants; 2.6.0's does, and no source spelling reaches it (rounds 429,
//   433, 446, 473 measured 20+ forms, the -da -dg dump and two permuter runs).
//   CANNOT BE COMMITTED ALONE EVEN AT 100: jtbl_801C512C must land at +0x30 of the unit's
//   [0xFC, .rodata, main/misc9] range, so func_801DBE54 (owner of +0x00, parked at 93.91)
//   has to be converted in the same change and the pair proven with check_bin.sh menu.
// ROUND 525: un-parked and re-measured 94.87% / 888 bytes / 222 insns = retail's exact size (still the round-499 best, 170 differing lines), so that note stands and the residual is its one tie. ONE new measurement: 'register u8 needsOpen' compiles BYTE-IDENTICALLY (same prologue, same 0x38 spill, same 112 frame), so REG_USERVAR_P does not enter this compiler's allocno priority - the register keyword is not a handle on the tie. The tie's value set is now complete and has no tenth member to drop: retail's loop holds 8 live values (col, slot, arg2, prevCol, prevSlot, kind*4, mode=4, needsOpen) plus s0 as the scratch loop-test mask, and it keeps arg2 UNMASKED in s3 ('andi a2,s3,0xff' at each callee), so kind and arg2 are both live and none of the eight merges; ours holds the same eight plus the hoisted const-1, which is the ninth, and pays for it with a FOURTH local slot (kind spills to 0x40 against retail's 0x38, needsOpen takes 0x38) and the 112 frame. Confirmed again this round that this stub still cannot be committed even at 100%: all three of the unit's rodata tables are asm-owned (+0x00 func_801DBE54 jtbl_801C50FC, +0x30 jtbl_801C512C here, +0x60 func_801E05D0 jtbl_801C515C), and three C tables of 0x2C + .align 3 land at retail's +0x00/+0x30/+0x60 only if all three are converted in one change, proven with check_bin.sh menu.
/*
void func_801DDF24(u_char arg0, u_char arg1, s32 arg2) {
    u8 active;
    u8 needsOpen;
    s32 col;
    s32 prevCol;
    u8 slot;
    u8 prevSlot;
    u8 kind;
    s32 mode;

    active = 1;
    needsOpen = 1;
    col = 0;
    prevCol = 0xFF;
    slot = arg0;
    prevSlot = 0xFF;
    kind = arg2;
    MenuCreateUnk430Panel(kind);
    MenuInitializeArrowCursor(0);
    mode = 4;
    do {
        MenuRenderFrame();
        if (slot != prevSlot) {
            MenuBuildUnk430PanelColumns(slot, arg2);
            prevSlot = slot;
            prevCol = 0xFF;
        }
        MenuBuildArrowCursorPrim(col, 0, 2, 0);
        if (col != prevCol) {
            func_801DCE60(slot, col, arg2);
            prevCol = col;
        }
        if (needsOpen) {
            MenuCreateWindow(6, 0x10, 0xA, D_801E9788[kind][0], 0x70, 0, 1, mode, 0);
            MenuCreateWindow(5, 0xC, 0x86, 0xAC, 0x38, 0, 1, mode, 0);
            MenuCreateWindow(4, D_801E9794[kind][0], 0xA6, D_801E97A0[kind][0], 0x18, 0, 1, mode, 0);
            MenuCreateWindow(3, 0xC8, 0x86, 0x50, 0x18, 0, 1, mode, 0);
            needsOpen = 0;
            if (arg1) {
                MenuStartOpenAnimation();
                MenuTweenCharacterWindows(0, 0);
                MenuRenderFrame();
            }
            g_Menu->pManager->playTimeDigitsActive = 0;
            g_Menu->pManager->shouldRenderWindow[1] = 0;
        }
        switch (g_Menu->input) {
        case 4:
            if (((MenuUnk430Panel*)g_Menu->unk430)->columnFlags[col] & 0x80) {
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
            slot = MenuStepPartySlot(slot, 0, arg2);
            break;
        case 10:
            slot = MenuStepPartySlot(slot, 1, arg2);
            break;
        }
    } while (active);
    MenuClearBytes(8, g_Menu->pManager->shouldRenderStrings10E0);
    MenuFreeArrowCursor(0);
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DDF24);







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
    MenuLoadResourceSet(7);
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
    MenuLoadResourceSet(7 | MENU_PANEL_FREE_OFFSET);
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

// Best: 92.30%, 2768 bytes (retail 2824), 692 vs 706 insns. Written from scratch last round
// (79.90); this round four measured steps, each kept:
//   1. 79.90 -> 86.81: the eight-name render loop writes g_Menu->pCharacterSpriteRecords
//      inline at every use instead of into a local. Retail reloads g_Menu + 0x434 after
//      every call (lui/lw/nop/lw, 7 sites); a local made gcc keep the address in a
//      callee-saved reg and cost ~30 insns.
//   2. 86.81 -> 89.49: the page select writes ((u8*)g_Menu->pEquipmentPanel)[...] inline
//      instead of into pPanel: retail reloads g_Menu in each of the four arms.
//   3. 89.49 -> 91.64: page and limit are assigned inside EACH of the two arg3 != 0 arms
//      (retail: sb t0,0x50 / li 0x64 / sw t0,0x58 twice, and it tests arg4 first).
//   4. 91.64 -> 92.30: the page-5 equipGroup test is one OR chain
//      (eq == 0 || (slotGroup & eq) != 0 || (groupMask & eq) == 0) -> flag = 1;
//      the if/else-if form made gcc emit sltiu/move.
// REMAINING, 14 insns:
//   a. The frame is 0xC0 where retail is 0xD0 and retail saves fp. Retail spills
//      groupMask at 0x90, slotGroup 0x88, the five pointers 0x60-0x80, page 0x50,
//      limit 0x58, count 0x48, and a 32-BIT copy of arg4 at 0x98 (sw t0,0x98(sp) once,
//      lw/bnez per use) which is where the extra 16 bytes come from; ours tests the
//      u_char at 0x40 directly and spills page/limit/count at 0x70/0x90/0x88.
//      Declaration order was not solved.
//   b. The inventory scan loop keeps its entry test (slt/beqz, +3) where retail is a
//      do-while under the `if (limit != 0)` guard. Not found: what makes gcc invert
//      retail and not ours.
//   c. Cases 0 and 1..4 of BOTH page switches: retail duplicates the id check
//      (lbu / nop / sltiu / beqz / andi / j / li s0,1 = 7 insns per case, ~12 total);
//      ours cross-jumps the shared sltiu+beqz (-2 per case).
//   d. fp: retail keeps &pGearAccessories[gearPartsIds[i]] in fp across the loop body
//      (page-5 gear arm); ours allocates it to s8.
// Nothing was permuted: the residuals are structure (spill set, cross-jump), not
// registers. The unit keeps its tables/JTBL shape: this stub owns no jtbl.
// g_DressingRoomItemQuantities is the quantity array beside g_DressingRoomItemIds (one u8 per id,
// MAX_INVENTORY_ACCESSORIES of each, cleared together at the top); it is splat data
// (asm/menu/data/246A4.data.s), declared extern in the .c, never defined here.
// Best: 93.22%, 2736 bytes (retail 2824), 684 vs 706 insns. This round took the parked body 92.30 -> 93.22 in five measured steps, each kept (a step that measured worse was reverted):
//   1. 92.30 -> 92.39: declaration order rect, digitCodes, text, count, page, limit, then the five pointers, slotGroup, groupMask. Retail's slots are 0x18/0x20/0x28/0x48/0x50/0x58/0x60-0x80/0x88/0x90 and this reproduces all of them; every remaining sp offset differs only by the frame size below.
//   2. 92.39 -> 92.45: the four panel-id reads spelled ((u8*)g_Menu->pEquipmentPanel + arg2)[MENU_PANEL_WEAPON_ID], base first in the sum, which is the one spelling giving retail's 'addu v0,v0,a2' (lever c-pointer-from-memory-paren-sum-is-base-first; the same file's func_801DE7xx neighbour uses it).
//   3. 92.45 -> 92.77: the inventory scan written as i = 0; do { ... } while (i < limit); under the 'if (limit != 0)' guard, which is retail's bottom-tested loop and removes 4 insns.
//   4. 92.77 -> 92.78: pQuantities assigned before pIds; retail creates the quantity pointer first (s5) and the id pointer second (s4).
//   5. 92.78 -> 93.22: the render loop's second index made a biv, for (i = 0, j = arg1; i < 8; i++, j++), so arg1 is loaded once before the loop and the increment rides in the latch, exactly retail's 'lw s4,0x38(sp)' plus 'addiu s4,s4,1'.
// REMAINING, in order of size:
//   A. THE FRAME, and it is why 100% is not reachable from this body: ours is 0xC8 / vars 136 / args 24 where retail is 0xD0 / vars 144 / args 24. Retail's highest REFERENCED slot is 0x9C and its saves start at 0xA8, so retail's source declares one 8-byte object that no instruction touches (not rect, digitCodes or text: those are 0x18/0x20/0x28 on both sides). Until that local is found, all 22 save/lw sp offsets stay 8 low. Do not reorder declarations looking for it - the order above is already retail's.
//   B. 15 insns of cross-jumped id check: retail duplicates 'lbu / nop / sltiu 0x32 / beqz or bnez / andi / j / li s0,1' in each of the four page-switch arms and ours shares one copy (jump.c merge). Retail's case-0 copy branches 'beqz' to the fail label while its case-1..4 copy branches 'bnez' to the flag test, so the two blocks are not identical in retail; our compile makes them identical. No spelling tried: the arms' C is the same three-term AND chain in both.
//   C. page is hoisted into s7 for the render loop, where retail reloads it twice ('lbu v1,0x50(sp); nop'), and that one register rotates the whole loop's s4-s8 allocation by one: ours s8=0xD s6=0x12 s4=0x400 s3=0 s2=arg1, retail s7=0xD s6=0x12 s5=0x400 s2=0 s4=arg1. Retail keeps pGearAccessory in s8/fp across the loop; ours gives s8 to 0xD.
//   D. The two g_Menu reloads in the arg3==0 arms are emitted 2 insns later than retail's (the load is scheduled after the page/limit stores instead of before).
//   E. Insns are 22 short overall (684 vs 706): B is 15, C/D account for the rest.
// Not tried: the permuter - 93.22 is below the 99% bar and the residuals are structure, not register ties. Also not tried: a probe for the do-while in the OTHER loop shape (this one is closed), and gcc 2.7.2-cdk / 2.6.0-O3 on the whole unit (preset is XenoMenu260 as expected).
// Best: 95.01%, 2768 bytes (retail 2824), 692 vs 706 insns. This round 93.22 -> 95.01 in four
// measured steps, each kept (a step that measured worse was reverted):
//   1. 93.22 -> 94.02 (689 insns): the id test in the page 1-4 arms of BOTH switches is
//      `>= 0x32`, not `< 0x32`. Retail's chain there ends `sltiu v0,v0,0x32 / bnez v0,.L801DEC60`
//      (skip when id < 0x32) where page 0's chain ends `beqz`; with the term spelled `>= 0x32`
//      the two chains are no longer identical, jump.c stops cross-jumping case 0 into case 1-4,
//      and retail's duplicated id check (+5 insns) comes back.
//   2. 94.02 -> 94.18: `u8 tens` and not `u32`; retail has `srl v1,v0,0x3 / andi v0,v1,0xff /
//      bnez v0 / addiu v0,v1,0x10` - it tests the truncated byte but adds to the raw quotient.
//   3. 94.18 -> 94.64: the `% 10` term reads g_DressingRoomItemQuantities[j] a SECOND time. Retail reloads
//      `lbu a3,0(at)` after the digitCodes[0] store (the store blocks cse of the load); a
//      `quantity` local keeps the byte in a register and hides that reload entirely.
//   4. 94.64 -> 94.93 -> 95.01: `pPanelIds = (u8*)g_Menu->pEquipmentPanel;` cached once per
//      arm (a permuter find, checked against the asm first: retail keeps pEquipmentPanel in
//      a0 in the arg4==0 arm and in a2 in the arg4!=0 arm across the slot scan - `addu v0,a0,s1`
//      and `addu v0,a2,s1` - and reads g_Menu itself only once per arm).
// REMAINING, 14 insns plus the frame, all of them allocator or scheduler:
//   A. THE FRAME, still not reachable: ours is 0xC8 / vars 160 where retail is 0xD0 / vars 168.
//      Retail's spill slots run 0x30..0x98 (14) plus a 15th 8-byte slot at 0xA0 that NO
//      instruction touches, so retail has one more value that failed to be promoted, and all
//      22 save/lw offsets sit 8 low. A dead `u_long pad[2]` DOES take a slot and does raise the
//      frame to 0xD0 with every save at retail's offset, but gcc hands the fresh slot the lowest
//      free one (0x30) and pushes arg0..groupMask up by 8 instead - 93.17, reverted.
//   B. page: ours promotes the loop-invariant `lbu page,0x50(sp)` into a register before the
//      render loop (one insn retail does not have, `lbu s7,0x50(sp)`), retail reloads it twice
//      (`lbu v1,0x50(sp)` before each of its two equips). Not expressible from source.
//   C. register rotation from that: retail s7=FONT_LETTER_HEIGHT, s8/fp=pAccessory/gearAccessory,
//      s2=record offset, s4=j, s5=0x400 block; ours s8=FONT_LETTER_HEIGHT, s7=page, s3=record
//      offset. Every remaining diff line is one of these renames.
//   D. the two arg3==0 arms: retail loads g_Menu before the page/limit stores and uses v1 twice
//      (`lw v0,0x360(v1)` / `lw v1,0x330(v1)`); ours loads it after them. Scheduling.
// Permuter: two runs (4 and 3 min) on this body. The pPanelIds step above is its only find; its
// other candidate wrapped a case body in `do { } while (0)` and scored IDENTICAL, so it was
// rejected as unrealistic C. Preset is XenoMenu260 (gcc 2.6.0) and is right.
// REDO ROUND 513: the parked body re-measured unchanged at 95.01% / 2768 bytes / 692 vs 706 insns.
// Two new measurements, both closing a question this note left open:
//   P1. PRESET, settled for good: try_flags --sweep gives 2.6.0 -O2 and -O3 95.01 (both -mcpu),
//       2.7.2 -O2/-O3 90.50, 2.7.2-cdk -O2/-O3 84.57. XenoMenu260 is right; no preset lead here.
//   P2. THE FRAME IS NOT A DECLARATION. A dead `u_long pad[2]` as the LAST local does reach retail's
//       frame - `.frame $sp,208 # vars=144`, all 22 save/arg offsets at retail's, stack arg at
//       0xE0 - but gcc 2.6 puts a dead aggregate's slot at the first free offset at expand time, so
//       it takes 0x30 and pushes arg0..the arg4 copy up by 8: 450 differing lines, 94.95%, worse
//       than 95.01. That proves retail's unused 0xA0 slot is not a declared local either: an
//       aggregate decl gets a LOW slot and a dead scalar gets none, so an 8-byte object above the
//       0x98 arg4-copy spill can only be a late RTL temp (-fpcc-struct-return buffer for a
//       discarded call, or an inlined aggregate temp), and nothing in this body's call set returns
//       a struct. Do not spend another round on declaration order or on a pad local.
// Residual is the same as below: the frame (ours 0xC8/160, retail 0xD0/168, every save and the
// stack argument 8 low) plus the two id-check blocks retail keeps duplicated (14 of the 706 insns);
// everything else in the diff is register naming. The body must stay parked - it is 14 insns and
// 56 bytes SHORT of retail, so converting it would move the overlay.
/*
s32 func_801DE5CC(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4) {
    RECT rect;
    u8 digitCodes[8];
    u8 text[8];
    s32 count;
    u8 page;
    s32 limit;
    MenuShopWeapon* pWeapon;
    MenuShopAccessory* pAccessory;
    MenuShopWeapon* pSlotWeapon;
    GearWeapon* pGearWeapon;
    GearWeapon* pSlotGearWeapon;
    u16 slotGroup;
    u16 groupMask;
    MenuDressingRoom* pDressingRoom;
    GearAccessory* pGearAccessory;
    u8* pWorkspace;
    u8* pPanelIds;
    u8* pIds;
    u8* pQuantities;
    u8* pName;
    u8 flag;
    u8 itemId;
    u32 quantity;
    u8 tens;
    s32 i;
    s32 j;

    digitCodes[1] = 0;
    digitCodes[3] = 0;
    count = 0;
    if (arg3 != 0) {
        if (arg4 == 0) {
            page = arg2 + 1;
            limit = MAX_INVENTORY_WEAPONS;
            pSlotWeapon = &g_Menu->pDressingRoom->pWeaponsData[((u8*)g_Menu->pEquipmentPanel + arg2)[MENU_PANEL_WEAPON_ID]];
        } else {
            page = arg2 + 1;
            limit = MAX_INVENTORY_WEAPONS;
            pSlotGearWeapon = &g_Menu->pDressingRoom->pGearWeapons[((u8*)g_Menu->pEquipmentPanel + arg2)[MENU_PANEL_WEAPON_ID]];
        }
    } else if (arg4 == 0) {
        if (arg2 != 0) {
            page = 5;
            limit = MAX_INVENTORY_ACCESSORIES;
            pDressingRoom = g_Menu->pDressingRoom;
            pPanelIds = (u8*)g_Menu->pEquipmentPanel;
            pAccessory = &pDressingRoom->pAccessoriesData[(pPanelIds + arg2)[MENU_PANEL_ACCESSORY_IDS - 1]];
            slotGroup = pAccessory->equipGroup;
            groupMask = 0;
            for (j = 0; j < MENU_ACCESSORY_SLOT_COUNT; j++) {
                pAccessory = &pDressingRoom->pAccessoriesData[(pPanelIds + j)[MENU_PANEL_ACCESSORY_IDS]];
                groupMask |= pAccessory->equipGroup;
            }
        } else {
            page = 0;
            limit = MAX_INVENTORY_WEAPONS;
        }
    } else {
        if (arg2 != 0) {
            page = 5;
            limit = MAX_INVENTORY_ITEMS;
            pDressingRoom = g_Menu->pDressingRoom;
            pPanelIds = (u8*)g_Menu->pEquipmentPanel;
            pGearAccessory = &pDressingRoom->pGearAccessories[(pPanelIds + arg2)[MENU_PANEL_ACCESSORY_IDS - 1]];
            slotGroup = pGearAccessory->equipGroup;
            groupMask = 0;
            for (j = 0; j < MENU_ACCESSORY_SLOT_COUNT; j++) {
                pGearAccessory = &pDressingRoom->pGearAccessories[(pPanelIds + j)[MENU_PANEL_ACCESSORY_IDS]];
                groupMask |= pGearAccessory->equipGroup;
            }
        } else {
            page = 0;
            limit = MAX_INVENTORY_WEAPONS;
        }
    }

    for (i = 0; i < MAX_INVENTORY_ACCESSORIES; i++) {
        g_DressingRoomItemIds[i] = 0;
        g_DressingRoomItemQuantities[i] = 0;
    }

    i = 0;
    count = page == 5;
    if (limit != 0) {
        pQuantities = &g_DressingRoomItemQuantities[count];
        pIds = &g_DressingRoomItemIds[count];
        i = 0;
        do {
            if (arg4 == 0) {
                pAccessory = &g_Menu->pDressingRoom->pAccessoriesData[g_GameState.accessoryIDs[i]];
                pWeapon = &g_Menu->pDressingRoom->pWeaponsData[g_GameState.weaponIDs[i]];
            } else {
                pGearWeapon = &g_Menu->pDressingRoom->pGearWeapons[g_GameState.gearWeaponsIds[i]];
                pGearAccessory = &g_Menu->pDressingRoom->pGearAccessories[g_GameState.gearPartsIds[i]];
            }
            flag = 0;
            if (arg4 == 0) {
                switch (page) {
                case 0:
                    if (MenuAndBitMaskLsbFirst(pWeapon->equipFlags, g_Menu->pManager->currentCharacterIDs[arg0]) != 0
                        && pWeapon->equipGroup < 5 && g_GameState.weaponIDs[i] < 0x32) {
                        flag = 1;
                    }
                    break;
                case 1:
                case 2:
                case 3:
                case 4:
                    if (MenuAndBitMaskLsbFirst(pWeapon->equipFlags, g_Menu->pManager->currentCharacterIDs[arg0]) != 0
                        && pWeapon->equipGroup == pSlotWeapon->equipGroup && g_GameState.weaponIDs[i] >= 0x32) {
                        flag = 1;
                    }
                    break;
                case 5:
                    if (MenuAndBitMaskLsbFirst(pAccessory->equipFlags, g_Menu->pManager->currentCharacterIDs[arg0]) != 0) {
                        if (pAccessory->equipGroup == 0
                            || (slotGroup & pAccessory->equipGroup) != 0
                            || (groupMask & pAccessory->equipGroup) == 0) {
                            flag = 1;
                        }
                    }
                    break;
                }
            } else {
                switch (page) {
                case 0:
                    if (MenuAndBitMaskLsbFirst32(pGearWeapon->equipFlags, g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId) != 0
                        && pGearWeapon->equipGroup < 5 && g_GameState.gearWeaponsIds[i] < 0x32) {
                        flag = 1;
                    }
                    break;
                case 1:
                case 2:
                case 3:
                case 4:
                    if (MenuAndBitMaskLsbFirst32(pGearWeapon->equipFlags, g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId) != 0
                        && pGearWeapon->equipGroup == pSlotGearWeapon->equipGroup && g_GameState.gearWeaponsIds[i] >= 0x32) {
                        flag = 1;
                    }
                    break;
                case 5:
                    if (MenuAndBitMaskLsbFirst32(pGearAccessory->equipFlags, g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId) != 0) {
                        if (pGearAccessory->equipGroup == 0
                            || (slotGroup & pGearAccessory->equipGroup) != 0
                            || (groupMask & pGearAccessory->equipGroup) == 0) {
                            flag = 1;
                        }
                    }
                    break;
                }
            }

            if (flag != 0) {
                if (arg4 == 0) {
                    if (page != 5) {
                        *pIds = g_GameState.weaponIDs[i];
                        *pQuantities = g_GameState.weaponQuantities[i];
                    } else {
                        *pIds = g_GameState.accessoryIDs[i];
                        *pQuantities = g_GameState.accessoryQuantities[i];
                    }
                } else if (page != 5) {
                    *pIds = g_GameState.gearWeaponsIds[i];
                    *pQuantities = g_GameState.gearWeaponsQty[i];
                } else {
                    *pIds = g_GameState.gearPartsIds[i];
                    *pQuantities = g_GameState.gearPartsQty[i];
                }
                pQuantities++;
                pIds++;
                count++;
            }
            i++;
        } while (i < limit);
    }

    pWorkspace = HeapAlloc(0x3F6, 0);
    for (i = 0, j = arg1; i < 8; i++, j++) {
        itemId = g_DressingRoomItemIds[j];
        if (itemId != 0) {
            if (arg4 == 0) {
                if (page != 5) {
                    pName = GetWeaponName(itemId);
                } else {
                    pName = GetAccessoryName(itemId);
                }
            } else if (page != 5) {
                pName = GetGearWeaponName(itemId);
            } else {
                pName = GetGearAccessoryName(itemId);
            }
            g_Menu->pCharacterSpriteRecords->unk0[i].width = SystemRenderStringEntry(pName, pWorkspace, 0x24, 0);
            tens = g_DressingRoomItemQuantities[j] / 10;
            digitCodes[0] = (tens != 0) ? tens + 0x10 : 0xC3;
            digitCodes[2] = g_DressingRoomItemQuantities[j] % 10 + 0x10;
            SystemDecodeGlyphCodesN((u16*)digitCodes, text, 2);
            g_Menu->pCharacterSpriteRecords->unk400[i].width = SystemRenderStringEntry(text, pWorkspace, 0x24, 1);
            rect.x = 0x180 + (i & 1) * 0x18;
            rect.y = 0x80 + (i / 2) * FONT_LETTER_HEIGHT;
            rect.w = 0x28;
            rect.h = FONT_LETTER_HEIGHT;
            LoadImage(&rect, pWorkspace);
            DrawSync(0);
            MenuSetStringPrimitives(&g_Menu->pCharacterSpriteRecords->unk0[i], i, 0x80, 0x81);
            MenuSetStringPrimitives(&g_Menu->pCharacterSpriteRecords->unk400[i], i, 0x80, 0x82);
            MenuSetQuadVertices(g_Menu->pCharacterSpriteRecords->unk0[i].vertices, 0xA8, 0x12 + i * FONT_LETTER_HEIGHT, g_Menu->pCharacterSpriteRecords->unk0[i].width, FONT_LETTER_HEIGHT);
            MenuSetQuadVertices(g_Menu->pCharacterSpriteRecords->unk400[i].vertices, 0x10C, 0x12 + i * FONT_LETTER_HEIGHT, g_Menu->pCharacterSpriteRecords->unk400[i].width, FONT_LETTER_HEIGHT);
            g_Menu->pCharacterSpriteRecords->unk0[i].renderContext = g_Menu->renderContext;
            g_Menu->pCharacterSpriteRecords->unk400[i].renderContext = g_Menu->renderContext;
            g_Menu->pCharacterSpriteRecords->shouldRenderRows[i] = 1;
        } else {
            g_Menu->pCharacterSpriteRecords->shouldRenderRows[i] = 0;
        }
    }
    HeapFree(pWorkspace);
    MenuSetStringTexturePrims(&g_Menu->pCharacterSpriteRecords->unk800, arg0, arg4, 0);
    count -= 8;
    g_Menu->pManager->shouldRenderPanel[2] = 1;
    if (count < 0) {
        count = 0;
    }
    return count;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc9", func_801DE5CC);



// Moves the item the equipment panel has selected into (or out of) the
// inventory. arg2 selects a weapon slot group (the extra weapon slots) and arg1
// the slot; with arg2 == 0, arg1 == 0 is the primary weapon and arg1 != 0
// accessory slot arg1 - 1. arg3 picks the character or its gear, which also
// picks the inventory the item is taken from: weapons, accessories or gear
// parts. The result is 1 when the item cannot be moved - the character has two
// weapon slots (id MENU_TWO_WEAPON_SLOT_CHARACTER_ID) and the slot is taken.
//
// pQuantities is assigned in BOTH arms of the arg3 test although both values
// derive from pIds: retail emits two full `la` of g_GameState there, and cse
// folds the second into `addiu t2,t2,-900` when the arms are single statements,
// which leaves the body 4 bytes short. The duplicated addiu is then merged back
// into one after the join by cross-jumping.
u_char MenuEquipItemFromPanel(u_char arg0, u_char arg1, u_char arg2, u_char arg3) {
    u_char* pIds;
    u_char* pQuantities;
    u_char panelItem;
    u_char* pSlot;
    u_char* pPanelItem;
    u_char characterId;
    u_char gearId;
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
    if (arg3 == 0) {
        pIds = g_GameState.weaponIDs;
        pQuantities = g_GameState.weaponQuantities;
    } else {
        pIds = g_GameState.gearWeaponsIds;
        pQuantities = g_GameState.gearWeaponsQty;
    }
    count = MAX_INVENTORY_WEAPONS;

    if (arg2 != 0) {
        if (arg3 == 0) {
            characterId = g_Menu->pManager->currentCharacterIDs[arg0];
            pPanelItem = (u_char*)g_Menu->pEquipmentPanel + MENU_PANEL_EXTRA_WEAPON_IDS;
            panelItem = pPanelItem[arg1];
            pSlot = &g_GameState.characters[characterId].extraWeaponIds[arg1];
            slotItem = *pSlot;
            newItem = panelItem;
            if (slotItem != 0) {
                // No GameState field covers 0x2286/0x22B6: one byte per weapon id,
                // clamped to the inventory maximum.
                if (((u_char*)&g_GameState)[0x2286 + *pSlot] < MAX_INVENTORY_WEAPONS) {
                    newItem = 0;
                }
                ((u_char*)&g_GameState)[0x2286 + *pSlot] = MAX_INVENTORY_WEAPONS;
            } else {
                *pSlot = newItem;
                updated = 0;
            }
        } else {
            characterId = g_Menu->pManager->currentCharacterIDs[arg0];
            pPanelItem = (u_char*)g_Menu->pEquipmentPanel + MENU_PANEL_EXTRA_WEAPON_IDS;
            panelItem = pPanelItem[arg1];
            gearId = g_GameState.characters[characterId].gearId;
            pSlot = &g_GameState.gears[gearId].extraWeaponIds[arg1];
            slotItem = *pSlot;
            newItem = panelItem;
            // Opposite polarity to the arm above: retail falls through into the
            // shared `*pSlot = newItem` block from this arm, and jumps to it from
            // the character arm.
            if (slotItem == 0) {
                *pSlot = newItem;
                updated = 0;
            } else {
                if (((u_char*)&g_GameState)[0x22B6 + *pSlot] < MAX_INVENTORY_WEAPONS) {
                    newItem = 0;
                }
                ((u_char*)&g_GameState)[0x22B6 + *pSlot] = MAX_INVENTORY_WEAPONS;
            }
        }
    } else if (arg3 == 0) {
        if (arg1 == 0) {
            characterId = g_Menu->pManager->currentCharacterIDs[arg0];
            slotItem = g_GameState.characters[characterId].weaponId[0];
            newItem = ((u_char*)g_Menu->pEquipmentPanel)[MENU_PANEL_WEAPON_ID];
            if (slotItem == 0) {
                g_GameState.characters[characterId].weaponId[0] = newItem;
                updated = 0;
            } else if (characterId == MENU_TWO_WEAPON_SLOT_CHARACTER_ID) {
                result = 1;
            }
        } else {
            characterId = g_Menu->pManager->currentCharacterIDs[arg0];
            slotItem = g_GameState.characters[characterId].accessorySlots[arg1 - 1];
            newItem = ((u_char*)g_Menu->pEquipmentPanel + arg1)[MENU_PANEL_ACCESSORY_IDS - 1];
            pIds = g_GameState.accessoryIDs;
            pQuantities = g_GameState.accessoryQuantities;
            count = MAX_INVENTORY_ACCESSORIES;
        }
    } else {
        if (arg1 == 0) {
            characterId = g_Menu->pManager->currentCharacterIDs[arg0];
            gearId = g_GameState.characters[characterId].gearId;
            slotItem = g_GameState.gears[gearId].weaponId[0];
            newItem = ((u_char*)g_Menu->pEquipmentPanel)[MENU_PANEL_WEAPON_ID];
            if (slotItem == 0) {
                g_GameState.gears[gearId].weaponId[0] = newItem;
                updated = 0;
            } else if (characterId == MENU_TWO_WEAPON_SLOT_CHARACTER_ID) {
                result = 1;
            }
        } else {
            characterId = g_Menu->pManager->currentCharacterIDs[arg0];
            gearId = g_GameState.characters[characterId].gearId;
            slotItem = g_GameState.gears[gearId].accessorySlots[arg1 - 1];
            pPanelItem = (u_char*)g_Menu->pEquipmentPanel + (MENU_PANEL_ACCESSORY_IDS - 1);
            newItem = pPanelItem[arg1];
            pIds = g_GameState.gearPartsIds;
            pQuantities = g_GameState.gearPartsQty;
            count = MAX_INVENTORY_ITEMS;
        }
    }

    if (updated == 0) {
        goto done;
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
        if (added != 0) {
            for (i = 0; i < count; i++) {
                if (pIds[i] == 0) {
                    pIds[i] = newItem;
                    pQuantities[i] = 1;
                    break;
                }
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
done:
    return result;
}
void MenuCopyEquipmentToPanel(u_char arg0, u_char arg1) {
    s32 i;

    for (i = 0; i < MENU_STAT_VALUE_COUNT; i++) {
        ((u16*)((u8*)g_Menu->pEquipmentPanel + 0x280))[i] = g_Menu->pDressingRoom->statValues[i];
    }

    if (arg1 == 0) {
        for (i = 0; i < MENU_CHARACTER_WEAPON_SLOT_COUNT; i++) {
            ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_WEAPON_ID] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].weaponId[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_EXTRA_WEAPON_IDS] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].extraWeaponIds[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_ACCESSORY_IDS] =
                g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].accessorySlots[i];
        }
    } else {
        for (i = 0; i < MENU_GEAR_WEAPON_SLOT_COUNT; i++) {
            ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_WEAPON_ID] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].weaponId[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_EXTRA_WEAPON_IDS] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].extraWeaponIds[i];
            ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_ACCESSORY_IDS] =
                g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].accessorySlots[i];
        }
    }
}

void MenuCopyEquipmentFromPanel(u_char arg0, u_char arg1) {
    s32 i;

    if (arg1 == 0) {
        for (i = 0; i < MENU_CHARACTER_WEAPON_SLOT_COUNT; i++) {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].weaponId[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_WEAPON_ID];
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].extraWeaponIds[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_EXTRA_WEAPON_IDS];
        }
        for (i = 0; i < MENU_ACCESSORY_SLOT_COUNT; i++) {
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].accessorySlots[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_ACCESSORY_IDS];
        }
    } else {
        for (i = 0; i < MENU_GEAR_WEAPON_SLOT_COUNT; i++) {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].weaponId[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_WEAPON_ID];
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].extraWeaponIds[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_EXTRA_WEAPON_IDS];
        }
        for (i = 0; i < MENU_ACCESSORY_SLOT_COUNT; i++) {
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[arg0]].gearId].accessorySlots[i] =
                ((u8*)g_Menu->pEquipmentPanel + i)[MENU_PANEL_ACCESSORY_IDS];
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
    MenuCopyGearEquipmentSlots(g_Menu->pDressingRoom, g_GameState.characters[charId].gearId);
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
                MenuSetStringPrimitives(&g_Menu->pCharacterSpriteRecords->slotStrings[i], i + 8, 0x80, 0x81);
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
/* Best: 98.88%, 2468 bytes (retail 2472), 617 vs 618 insns - 4 bytes short, so
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
   body is 615 insns (2464 bytes, 98.39%); the order `shouldRender[0] = 1;` then
   `closeList = 1;` (what the body below uses) keeps retail's TWO `li 1` - `li
   t0` for the store, `li s0` for closeList - but still sinks the `li s0`
   (617/2468); the embedded form `closeList = g_Menu->pCursors->shouldRender[0]
   = 1;` gives the same shape as that, and `store = closeList = 1` or a
   `do { } while (0);` around the pair are worse (615/2464). Putting
   `closeList = 1;` BEFORE the `MenuCopyEquipmentToPanel(arg0, kind)` call in the
   same arm breaks the arm: 620 insns, 2480 bytes, 93.26%. An 8-minute permute.sh
   run on the 98.42% body (2205 iterations) found only the slotRow change below,
   so this tie is not reachable by random statement reordering. Round 419 measured
   two more orders, both 616 insns / 2464 bytes / 98.71: `closeList = 1;` between
   the MenuCopyEquipmentToPanel call and `cursorVisible = 0;`, and between
   `cursorVisible = 0;` and the shouldRender store - in both, cse merges the two QI
   1s (lever c-moved-constant-merges-then-sinks).
   ROUND 432: un-parked and re-measured, the note's number is current (98.88%,
   2468 bytes, 617 insns under gcc-2.6.0). One new spelling measured: closeList
   = 1 FIRST, then MenuBuildArrowCursorPrim, then the shouldRender store (i.e.
   the store moved below the call so cse cannot merge the two QI 1s) - 617
   insns / 2468 bytes / 98.00, so no merge and both `li 1`s survive, but cc1 then
   sinks the store below the call (`jal / lw g_Menu / #nop / lw 0x33C / li s0 /
   sb s0,0x140`) and the arm loses retail's shape without gaining the nop. Worth
   knowing for the next attempt: the BACK arm of this same function ALREADY has
   retail's shape - on the current body cc1 hoists its `li s0,1` above the
   cursor-prim argument setup AND keeps both `li 1`s (emission: `jal
   MenuCopyEquipmentFromPanel / li s0,1 / move a0,0 / move a1,row / lw g_Menu /
   li a2,3 / sb $0,col / lw pCursors / move a3,0 / li t0,1 / jal`) - so the
   hoist is reachable in this unit and the residual is only why cse merges the
   pair in the CONFIRM arm; every order that puts closeList first merges it.
   PROLOGUE: the init block must assign prevRow = 0xFF and row = 0 AFTER
   listOpen = 0, not before needsPanel - that is worth 98.56 to 98.88. The two
   prologue rows still differing are the `sb t0,0x38` (active) and `sb t0,0x40`
   (needsWindow) stores, emitted about two insns later than retail's among the
   callee-save spills; the statement order already equals retail's, so those are
   the same scheduler floating stores and not a source shape to hunt.
   PRESET RULED OUT (round 396, this body): 98.56 on the unit's gcc-2.6.0, 98.3
   on plain 2.7.2, 95.0 on 2.7.2-cdk, 71.7 on 2.6.0 -O3, and 2.6.0 -G0 does not
   compile the unit at all.
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
   JUMP TABLE (round 419, MEASURED - this replaces the old "C .rodata lands before
   asm .rodata" reading, which is the REPORT-mode artifact lever
   i-jtbl-offset-follows-file-position describes): this stub owns jtbl_801C515C at
   +0x60 of the unit's .rodata range (0xFC..0x18C, three 0x30-byte tables:
   func_801DBE54's at +0x00, func_801DDF24's at +0x30, this one last; 11 cases =
   44 bytes plus 4 of pad each). Unparking this body and building in MATCHING mode
   (check_bin.sh menu) gives an object .rodata of 0x8C with the two asm tables at
   +0x00 and +0x30 and this function's C table at +0x60 - retail's EXACT address -
   because cc1 emits function_rodata after its own function's body
   (build/src/menu/main/misc9.c.s ends with the .align 3 table, i.e. after the two
   earlier stubs' .include'd .rodata). The only thing missing is the trailing
   4-byte zero word, so the unit's .rodata is 4 short of its range and
   `culprit.sh menu` then lists every function of the overlay as shifted - exactly
   the misc10 case config/menu.yaml already fixes with a 4-byte carve
   (`[0x248, rodata]` there). So this function ships iff (a) the C reaches retail's
   2472 bytes and (b) `[0x188, rodata]` is added before main/misc10's range.
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
    needsPanel = 0;
    cursorVisible = 0;
    reopenList = 0;
    listOpen = 0;
    prevRow = 0xFF;
    row = 0;
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
            MenuBuildArrowCursorPrim(col, row, 3, 0);
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
                MenuTweenCharacterWindows(0, 0);
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
                if (g_Menu->pManager->currentCharacterIDs[arg0] == MENU_TWO_WEAPON_SLOT_CHARACTER_ID) {
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
                if (MenuStepPartySlot(arg0, 0, kind) != arg0) {
                    arg0 = MenuStepPartySlot(arg0, 0, kind);
                    group = 0;
                    needsPanel = 0;
                }
                break;
            case MENU_INPUT_PREV:
                if (MenuStepPartySlot(arg0, 1, kind) != arg0) {
                    arg0 = MenuStepPartySlot(arg0, 1, kind);
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
                MenuBuildArrowCursorPrim(0, row, 3, 0);
                col = 0;
                prevChar = 0xFF;
                g_Menu->pManager->shouldRenderArrowCursor[0] = 0;
                break;
            case MENU_INPUT_CONFIRM:
                equipResult = MenuEquipItemFromPanel(arg0, slot, group, kind);
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
                MenuBuildArrowCursorPrim(0, row, 3, 0);
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
