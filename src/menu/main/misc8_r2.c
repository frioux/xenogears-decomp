#include "common.h"

#include "main/game.h"
#include "psyq/libapi.h"
#include "system/menu.h"
#include "psyq/libcd.h"

extern u8 g_MenuOverlayIndex; // Index of the menu overlay the menu was entered with
extern u8 g_ShopIndex;
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
extern u8 g_MenuCharactersAllocated; // Set once the three MenuCharacter records have been allocated
extern u8 g_MenuStatBarDeltaColorMode;
extern u8 g_MenuStatBarDeltaSignTexId;
extern CdlCB g_MenuSavedCdSyncCallback;
extern CdlCB g_MenuSavedCdReadyCallback;
extern CdlCB g_MenuSavedCdReadCallback;
extern u8 g_MenuSoundModeStringIds[];
extern u8 g_MenuNameTextureX[];
extern u8 g_MenuSubMenuRowXOffsets[];
extern s32 g_MenuShoulderButtonIconTexIds[]; // String IDs of the menu scroll bar textures
extern u8 g_MenuColumnRowStringIds[];
extern u8 g_MenuPanelRowXOffsets[];
extern s32 g_MenuCharacterLabelXPositions[]; // X positions of the character panel label glyphs, first row of the layout
extern s32 g_MenuGearCharacterLabelXPositions[]; // The same X positions, second row of the layout
extern s32 g_MenuCharacterLabelYPositions[]; // Y positions of the label glyphs of the first row
extern s32 g_MenuGearCharacterLabelYPositions[]; // Y positions of the label glyphs of the second row

void MenuBuildCharacterPrims(MenuCharacter* pCharacter, u_char characterId, u_char localIndex, s32 arg3, s32 arg4, u_char arg5);

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
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);
void MenuSetRowQuadVertices(s32 count, void* pStrings, void* pStringIds, void* pXOffsets, void* pShouldRender,
                            u8 arg5, u8 arg6, u8 arg7);
void MenuAllocatePrimBuffer(void);
void MenuCreateCellPrimBlocks(void);
void MenuStartOpenAnimation(void);
void MenuTweenCharacterWindows(u_char isCreate, u_char isSubmenuOpening);
s32 MenuBeginCardOperation(void);
void MenuResetSelectionCursorFlags(void);
s32 MenuUpdateSubMenuChoice(u_char arg0);
extern u8 g_MenuCardSaveListStale;
extern u8 g_MenuSoftResetEnabled;
void* GetStringEntry(void* pStringData, u32 index);
void MenuSetPolySemiTransGrey(P_TAG* pTag);
void MenuSetStringPrimitives(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);
void* GetItemName(int entryId);
s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);

void MenuShutdown(void) {
    s32 i;

    g_Menu->cardSlotState = 0;
    MenuClearGlyphTexture();
    MenuFreeCellPrimBlocks();
    MenuFreePrimBuffer();
    for (i = 0; i < MENU_CARD_COUNT * MENU_CARD_RECORD_COUNT; i++) {
        g_Menu->menuUnk2->records[i].unk58 = 0;
    }
    g_Menu->menuUnk2->previousCardSlotCounts[0] = MENU_SLOT_NONE;
    g_Menu->menuUnk2->previousCardSlotCounts[1] = MENU_SLOT_NONE;
    D_801EA904 = 0;
    g_MenuCardSlotTotals = 0;
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    CdSyncCallback(g_MenuSavedCdSyncCallback);
    CdReadyCallback(g_MenuSavedCdReadyCallback);
    CdReadCallback(g_MenuSavedCdReadCallback);
    ExitCriticalSection();
}

void MenuLoadUnkDE0Strings(void) {
    if (g_MenuOverlayIndex != 2) {
        MenuLoadStringsFromIds(sizeof(g_Menu->unkDE0) / sizeof(g_Menu->unkDE0[0]), g_Menu->unkDE0, D_801EA53C,
                               g_Menu->pManager->shouldRenderStringsDE0);
    } else {
        MenuLoadStringsFromIds(sizeof(g_Menu->unkDE0) / sizeof(g_Menu->unkDE0[0]), g_Menu->unkDE0, D_801EA542,
                               g_Menu->pManager->shouldRenderStringsDE0);
    }
}

/* 95.67% under this unit's XenoMenu260 (gcc 2.6.0): 1288 vs retail 1296 bytes, 322 vs
   324 insns. Written from the .s this round; there was no earlier park. gcc-2.7.2 scores
   94.37 on this body, so the unit preset is right (2.6.0 wins).
   Body shape is settled and every call, constant and store matches: the whole loop shell
   (MenuTweenCharacterWindows / func_801E86C8 / MenuBeginCardOperation / the subMenuChoice
   block / the four-case switch on g_Menu->input / the tail) is retail's, from the proven
   neighbour MenuRunCharacterDressingRoomLoop in misc10.c and the misc9/misc1 loops.
   GETTING HERE, each measured: `(MenuBeginCardOperation() & 0xFF) != 0` (the mask is what
   retail has; the bare call is one insn short), the `choice = 7; if (overlay != 2) choice
   = 0;` argument instead of a ternary (the ternary folds branchless into
   xori/sltiu/negu/andi, +4 insns), `switch` on g_MenuOverlayIndex rather than if/else
   (322 vs 319 insns; the if/else form tests 2 first like retail but drops retail's range
   test), and g_Menu->subMenuChoice/subMenuPrevChoice/input named from menu.h.
   RESIDUAL 1, the only instruction-count difference: retail holds the constant 2 in $s5
   (`ori s5,zero,0x2` in the loop preheader, used three times inside the loop: the
   g_MenuOverlayIndex switch test, the MenuSetSubMenuChoicePrims argument and the case-4
   test), and that is exactly its 8th callee-saved save/restore pair and its 0x48 frame.
   This body rematerialises `li v0,0x2` at each of the three sites and so has a 0x40 frame.
   RESIDUAL 2, the switch tree: retail's tree for case values {0,2} is
   `beq x,s5 -> case2; slti v0,x,3; beqz -> end; bnez x -> end; [case 0]` (the 2 is the
   ROOT, the 0 sits in a range-checked subtree). This body gets the mirror image with the
   0 as the root and no range test: `beqz x -> case0; li v0,2; beq x,v0 -> case2; j end`.
   Same instruction count (4 vs 4), different tree. Measured and NO help: source case
   order (0-first and 2-first both give the 0 root), an explicit `default: break;`, and an
   explicit empty `case 1: break;` (which makes gcc 328 insns at 95.02). gcc 2.6 sorts the
   case list, so the root is not source-order controlled; the neighbour switch on
   g_Menu->input (cases 1,3,4,5) DOES get retail's slti tree in this body, so the tree
   method is reachable, just not for two cases. If RESIDUAL 2 is solved, RESIDUAL 1
   (the $s5 constant) is likely to follow, since the constant is CSEd across the loop once
   the tree uses a register compare.
   RESIDUAL 3: ~140 register-name-only lines (the whole s0..s7 assignment shifts by one
   because of the missing $s5), including result in $s4 here vs $s1 in retail. Moving the
   `s32 result;` declaration to the top of the local list does not change it (95.67 either
   way), so it is the allocator's priority sort, not declaration order.
   MISC: the `lhu 0x4FE4` in the tail means retail reads menuUnk2->cardStatus[] as one
   halfword; this body's two separate u8 compares emit the same lhu, so the cardStatus
   test is fine as written.
   ROUND 429 RE-MEASURE (redo lane): unparked, recompiled and re-scored 95.67% / 322 vs
   324 insns, so this park is not stale and the body does compile.
   THE ROUND-406 HYPOTHESIS ABOVE IS BACKWARDS: the switch tree is not what unblocks the
   $s5 constant, the constant is what unblocks the tree, and the choice site blocks it.
   Retail's three in-loop uses of 2 are ALL register compares (switch root `beq v1,s5`,
   `beq v0,s5` at the choice site, `beq v0,s5` at the case-4 test); this body can only
   produce two, because the choice site folds to arithmetic. Measured this round, all
   four giving the SAME code and 95.67: the live `choice = 7; if (overlay != 2) choice =
   0;`, `if (overlay == 2) choice = 7; else choice = 0;`, the ternary passed straight to
   the call (`MenuSetSubMenuChoicePrims(overlay == 2 ? 7 : 0)`), and a `u_char choice`
   (that one also adds a surplus `andi a0,a0,0x7`). cc1 2.6.0 always emits
   `xori a0,a0,0x2; sltiu a0,a0,1; negu a0,a0; andi a0,a0,0x7` (= 7 & -(x == 2)), so
   the constant 2 survives into one insn and loop.c sees only two `li 2` in the loop;
   retail had three, which is exactly the break-even for hoisting a constant into a
   callee-saved register (preheader ori + save + restore). That missing hoist is the
   missing save/restore pair (0x48 vs 0x40 frame) and the ~140-line register shift.
   PRESET RE-SCREENED on this body: -O3 82.35, 2.7.2-cdk -G0 78.56, 2.7.2-cdk -G8
   compile-failed, so XenoMenu260 stays. Lever recorded in levers.md.
   CODE SHAPE / meaning, for the naming pass: this is the menu's sub-menu (item row)
   screen loop: it resets the soft-reset flag and the sub-menu choice on entry, builds the
   prim buffer and cell prim blocks once (firstRun), starts the card read with
   MenuBeginCardOperation, then per frame rebuilds the row quad vertices when the choice
   changed, dispatches g_Menu->input (1 up, 3 down, 4 confirm, 5 cancel) and tears the
   panel down on exit. Returns whether the screen completed normally.
*/
/*
s32 func_801D9F98(s32 arg0, s32 arg1) {
    s32 result;
    u8 active;
    u8 firstRun;
    s32 stringSet;
    s32 choice;

    active = 1;
    firstRun = 1;
    result = 1;
    stringSet = 0;
    g_MenuSoftResetEnabled = 0;
    g_Menu->subMenuChoice = 2;
    if (g_MenuOverlayIndex == 0 && g_ShopIndex == 0) {
        g_Menu->subMenuChoice = 1;
    }
    g_Menu->subMenuPrevChoice = 0xFF;
    MenuLoadUnkDE0Strings();
    MenuRenderFrame();
    while (active != 0) {
        g_Menu->pManager->characterWindowBuilt = 0;
        g_Menu->cardSlotState = 1;
        if (firstRun != 0) {
            MenuAllocatePrimBuffer();
            MenuCreateCellPrimBlocks();
            g_Menu->menuUnk2->cardStatus[0] = 1;
            g_Menu->menuUnk2->cardStatus[1] = 1;
            MenuStartOpenAnimation();
            switch (g_MenuOverlayIndex) {
            case 0:
                MenuTweenCharacterWindows(0, 0);
                if (g_ShopIndex == 0) {
                    stringSet = 9;
                }
                break;
            case 2:
                stringSet = 7;
                break;
            }
            func_801E86C8(stringSet);
            g_Menu->pManager->playTimeDigitsActive = 0;
            firstRun = 0;
            g_Menu->pManager->shouldRenderWindow[1] = 0;
            if ((MenuBeginCardOperation() & 0xFF) != 0) {
                if (g_MenuOverlayIndex != 0) {
                    result = 0;
                }
                break;
            }
            g_Menu->pManager->isPanelActive[5] = 1;
        }
        if (g_MenuCardSaveListStale != 0) {
            MenuCreateWindow2Strings(0x20);
        }
        if (func_801C93A8() != 0) {
            active = 0;
        }
        if (g_MenuCardSaveListStale != 0) {
            MenuFreeWindow2Strings();
            g_MenuCardSaveListStale = 0;
        }
        if (active == 0) {
            break;
        }
        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            MenuSetRowQuadVertices(6, g_Menu->unkDE0, D_801EA542, g_MenuPanelRowXOffsets,
                                   g_Menu->pManager->shouldRenderStringsDE0, g_Menu->subMenuChoice, 7, 0);
            choice = 7;
            if (g_MenuOverlayIndex != 2) {
                choice = 0;
            }
            MenuSetSubMenuChoicePrims(choice);
            g_Menu->subMenuPrevChoice = g_Menu->subMenuChoice;
        }
        switch (g_Menu->input) {
        case 4:
            MenuResetSelectionCursorFlags();
            MenuClearBytes(6, g_Menu->pManager->shouldRenderStringsDE0);
            active = MenuUpdateSubMenuChoice(arg1);
            g_Menu->subMenuPrevChoice = 0xFF;
            MenuLoadUnkDE0Strings();
            if (g_MenuOverlayIndex == 2) {
                break;
            }
            // fall-through
        case 5:
            active = 0;
            if ((arg0 & 0xFF) != 0) {
                result = 0;
            }
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
        }
    }
    if (g_Menu->menuUnk2->cardStatus[0] == 0 && g_Menu->menuUnk2->cardStatus[1] == 0 && g_MenuOverlayIndex != 0) {
        result = 0;
    }
    g_Menu->pManager->isPanelActive[5] = 0;
    g_Menu->pManager->characterWindowBuilt = 0;
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    MenuClearBytes(6, g_Menu->pManager->shouldRenderStringsDE0);
    MenuCloseCardEvents();
    return result;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801D9F98);




void MenuInitializeUnk42CPanel(void) {
    MenuUnk9* pMenu;

    MenuSetupPointerCursors(2);
    MenuLoadStringsFromIds(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuItemRowStringIds,
                           g_Menu->pManager->shouldRenderStrings10E0);
    pMenu = HeapAlloc(sizeof(MenuUnk9), 0);
    g_Menu->pItemRows = pMenu;
    bzero(pMenu, sizeof(MenuUnk9));
    MenuLoadResourceSet(0);
}

void MenuFreeUnk42CPanel(void) {
    MenuFreeScrollHandle();
    MenuFreeWindow(3);
    MenuFreeWindow(4);
    g_Menu->pManager->shouldRenderUnk42C = 0;
    MenuLoadResourceSet(MENU_PANEL_FREE_OFFSET);
    HeapFree(g_Menu->pItemRows->pVramBuffer);
    HeapFree(g_Menu->pItemRows);
    HeapFree(g_Menu->pDressingRoom->pItemsData);
}

/* 94.65% under this unit's XenoMenu260 (gcc 2.6.0): 996 vs retail 1004 bytes, 249 vs
   251 insns, round 428. THE HEADLINE USED TO READ 97.17 - that number is gcc 2.7.2
   (Default) and this unit was moved to 2.6.0 on purpose in round 421, trading this park
   for MenuSetAllPrimsShadingMode (100.00 only on 2.6.0). 94.65 is the number to work from here; see
   SPLIT below if the trade is ever revisited.
   Body shape is settled: nested if (outer test POSITIVE, inner test `!= 0`) so
   the two fail paths sit out of line where retail has them.
   SOLVED in round 406, each measured, and all still in the live body:
   - `(u16)(u16)(x + 0x28) & 0xFFF8` gives retail's SINGLE `andi a1,a1,0xfff8`. A bare
     `(u16)(x + 0x28) & 0xFFF8` inside a loop whose trip count bounds x lets cc1 prove
     the sum is 16-bit, so it emits `li t0,-8` + `and` AND a second `andi` for
     the u16 parameter truncation (4 surplus insns, 95.44). The cast on the SUM
     defeats the fold; a u16 local holding the sum works too, a u16 local
     holding x does not (the subreg is seen through).
   - `(i / 2) * 0x10 | 0xE` (not `+ 0xE`) gives retail's `ori s0,s0,0xe`, and
     the `andi s0,s0,0xffff` truncation stays 0xffff.
   - separate `displayFlags` and `itemShade` locals instead of mutating one flag
     variable: retail keeps the byte in $v1 and writes the masked value to $s0
     (`andi s0,v1,0x80`); one mutable variable gives `andi s0,s0,0x80`.
   RESIDUAL, all measured on this body:
   a) one surplus `nop`: after `lw v1,0x42C(v1)` retail fills the load-delay
      slot with the /10 magic `lui a0,0xcccc` and keeps it in $a0 with the
      dividend in $v0, coalescing `mfhi v0`; this body materialises the magic
      later in $v0 and lands the HI in $t0. permute.sh 1447 iterations,
      base=715 best=715, so not a register shuffle.
   b) eight `addu` operand flips, consistently MIRRORED: retail is base-first at
      the four `rows[k].field` store sites and index-first at the four
      `&rows[k]` argument sites; this body is the exact opposite in both groups,
      so no single spelling flips them together. A pointer local for
      g_Menu->pItemRows is the known trap (it hoists the pool load, lever
      b-inplace round 396); the double-negated offset moves only the store group.
   c) `andi a1,a1,0xfff8` comes one instruction early: retail emits it after the
      y `sll`/`ori` pair, here before.
   UNDER 2.6.0 THE RESIDUAL IS REGISTER ALLOCATION, NOT SPELLING. Retail keeps arg0 in
   $s6 and recomputes the row index twice per iteration (`sll v0,s6,1; addu v1,v0,s2` at
   0x15620, then `sll s1,s6,1; addu s1,s1,s2` at 0x156C8); 2.6.0 keeps arg0 in $s0 and
   strength-reduces the index into $s4 (`sll s4,s0,1` in the preheader, `addiu s4,s4,1`
   in the latch delay slot), which is exactly where the two missing instructions are.
   That one choice also reorders the prologue saves (retail: s6 first, then s8..s0
   descending; 2.6.0: s0 first, then s8..s2 with s1 last in the HeapAlloc delay slot)
   and cascades into every temp register, so it is not a spelling the source can reach.
   MEASURED THIS ROUND: gcc-2.7.2-cdk-psx 92.76 (worse), 2.6.0 -O1 59.62, -O3 94.65
   (identical to -O2, so no -O level helps), and `x = (i & 1) * 0x88` - copying the
   `andi v1,s2,0x1` retail uses for rect.x - is WORSE at 93.75% / 245 insns, because it
   derives x and y from one index and loses four instructions.
   RESIDUAL beyond the allocation:
   a) eight addu operand swaps, consistently MIRRORED: retail is base-first at the four
      `rows[k].field` store sites and index-first at the four `&rows[k]` argument sites;
      this body is the reverse in both groups. Lever a-empty-do-while-block-nudges
      records the same cc1 2.6.0 behaviour for `base + i * STRIDE` with a base loaded at
      the use site and lists six spellings that all stay offset-first, so this is cc1's
      address legitimisation, not the source. A pointer local for g_Menu->pItemRows is
      the known trap (it hoists the pool load). The one spelling NOT yet measured for the
      four store sites is lever c-pointer-from-memory-paren-sum-is-base-first, i.e.
      `((MenuString*)((u8*)g_Menu->pItemRows + i * sizeof(MenuString)))->width`; start
      there. It cannot make this function match on its own, see below.
   b) `andi a1,a1,0xfff8` comes one instruction early: retail emits it after the y
      `sll`/`ori` pair, here before.
   Under 2.7.2 (round 406) the same body was 97.17% with only (a) plus one surplus nop,
   and permute.sh ran 1447 iterations to base=715 best=715, so neither residual is a
   register shuffle.
   SPLIT: under Default (2.7.2) this body scores 97.17 and the register set matches
   retail's, i.e. this function really is 2.7.2 code sitting in a 2.6.0 unit. Splitting
   its range out of config/menu.yaml like menu/main/misc11_r2_260 would buy 2.5 points
   but not a match, so it is not worth a unit split on its own.
*/
// PARK 94.84% (996 vs retail 1004 bytes, 249 vs 251 insns), round 442, re-measured from the live body.
//    Retyped this round: the row accesses are now the harmless typed form
//    (g_Menu->pItemRows->itemNameStrings[i] / ->itemQuantityStrings[i] /
//    ->shouldRenderRows[i]) instead of the ((MenuString*)g_Menu->pItemRows)[i] casts the
//    earlier rounds carried - realistic C, and 94.65 -> 94.84 (116 -> 106 differing
//    lines). The same body under gcc 2.7.2 is 97.37 (the cast body was 97.17).
//    THE ONE RESIDUAL IS STRUCTURAL: gcc 2.6.0 strength-reduces the item index
//    'arg0 * 2 + i' into a reduced biv (sll s4,s0,0x1 in the preheader, addiu s4,s4,0x1
//    in the latch) and reuses that one register where retail RECOMPUTES the index twice
//    per iteration (sll v0,s6,1 / addu v1,v0,s2 at 0x15620, sll s1,s6,1 / addu s1,s1,s2
//    at 0x156C8). 2.7.2 does not reduce it, so this is a compiler difference and not a
//    spelling: the body is 2 insns / 8 bytes short and every other differing line is a
//    register name (or the prologue save order) downstream of it.
//    Measured this round: unit flags 94.84, gcc-2.7.2 97.37, gcc-2.7.2-cdk 92.96
//    (try_flags.sh); -G0 measured identical to unit flags earlier this round, -G4 does
//    not compile the unit.
//    NO SPLIT: 2.7.2 is still 97.37, and lever o-272-vs-260-operand-order shows 2.7.2
//    canonicalises a commutative PLUS by register NUMBER, so its residual (8 mirrored
//    addu operands) is allocation; a 1447-iteration permuter run on that body found
//    nothing (round 406).
// PARK 94.84% (996 vs retail 1004 bytes, 249 vs 251 insns), round 461, re-measured from this live body under XenoMenu260.
//    The body is unchanged from round 442 (typed row accesses). THIS round compared the compile
//    instruction by instruction against retail and there is NO spelling residual left: with the
//    current typed body everything except the index allocation already IS retail - the /10 magic
//    (lui a0,0xcccc in the lw delay slot, mfhi v0 / srl v1,v0), all four &rows[i] call-argument
//    addus (index-first, addu a0,s3,a0 / addu a0,s4,a0), the andi a1,0xfff8 position and the x/y
//    evaluation order all come out as retail has them.
//    THE WHOLE residual is lever o-260-reduces-a-param-based-index-biv: 2.6.0 strength-reduces
//    "arg0 * 2 + i" to a biv (sll s4,s0,0x1 in the preheader, addiu s4,s4,0x1 in the latch), which
//    costs the two per-use sll instructions retail spends at 0x15620 and 0x156C8 (the 2 insns /
//    8 bytes this body is short) and renames the whole saved set (arg0 s0 vs retail s6, pWorkspace
//    s6 vs s5, 0x800 s5 vs s4) and reorders the prologue saves.
//    The 2.7.2 path was re-measured this round by hand-compiling the SAME body with Default flags
//    (no split): 97.37%, 1008 vs 1004 bytes. Its structure is retail (two sll in the loop,
//    s3/s4 row-offset bivs, s6 = arg0), and the residual is exactly three things: a nop after
//    lw v1,1068(v1) where retail puts lui a0,0xcccc in that slot and coalesces the HI into v0
//    (this nop is also the 4 extra bytes), four mirrored addu a0,a0,s3 against retail addu a0,s3,a0,
//    and andi a1,a1,0xfff8 one instruction early. Round 406 permuted that 2.7.2 body (1447
//    iterations, base 715 unchanged), so the split path is not a register shuffle either and a
//    per-file preset split would buy 2.5 points and still not a match.
//    Presets measured dead earlier: 2.7.2-cdk 92.96, -O1 59.62, -O3 94.65, -G0 identical.
/*
void func_801DA5BC(s32 arg0) {
    u8 digitCodes[4];
    u8 text[8];
    RECT rect;
    void* pWorkspace;
    u8 tens;
    u8 itemShade;
    u8 displayFlags;
    s32 x;
    s32 i;

    pWorkspace = HeapAlloc(0x3F6, 0);
    digitCodes[1] = 0;
    digitCodes[3] = 0;
    for (i = 0; i < 0x10; i++) {
        if (g_GameState.itemIDs[arg0 * 2 + i] != 0) {
            if (g_GameState.itemQuantities[arg0 * 2 + i] != 0) {
                    if (g_GameState.itemQuantities[arg0 * 2 + i] >= 0x64) {
                        g_GameState.itemQuantities[arg0 * 2 + i] = 0x63;
                    }
                    g_Menu->pItemRows->itemNameStrings[i].width =
                        SystemRenderStringEntry(GetItemName(g_GameState.itemIDs[arg0 * 2 + i]), pWorkspace, 0x24, 0);
                    tens = g_GameState.itemQuantities[arg0 * 2 + i] / 10;
                    digitCodes[0] = tens != 0 ? tens + 0x10 : 0xC3;
                    digitCodes[2] = g_GameState.itemQuantities[arg0 * 2 + i] % 10 + 0x10;
                    SystemDecodeGlyphCodesN((u16*)digitCodes, text, 2);
                    g_Menu->pItemRows->itemQuantityStrings[i].width =
                        SystemRenderStringEntry(text, pWorkspace, 0x24, 1);
                    rect.x = 0x180 + (i & 1) * 0x18;
                    rect.y = 0x80 + (i / 2) * FONT_LETTER_HEIGHT;
                    rect.w = 0x28;
                    rect.h = FONT_LETTER_HEIGHT;
                    LoadImage(&rect, (u_long*)pWorkspace);
                    DrawSync(0);
                    displayFlags = g_Menu->pDressingRoom->pItemsData[g_GameState.itemIDs[arg0 * 2 + i]].displayFlags;
                    if ((displayFlags & 0x20) && (g_ShopIndex == 0)) {
                        itemShade = 0;
                    } else {
                        itemShade = displayFlags & 0x80;
                    }
                    MenuSetStringPrimitives(&g_Menu->pItemRows->itemNameStrings[i], i, 0x80, itemShade | 1);
                    MenuSetStringPrimitives(&g_Menu->pItemRows->itemQuantityStrings[i], i, 0x80, itemShade | 2);
                    x = (i % 2) * 0x88;
                    MenuSetQuadVertices(g_Menu->pItemRows->itemNameStrings[i].vertices, (u16)(x + 0x28) & 0xFFF8,
                                        (i / 2) * 0x10 | 0xE, g_Menu->pItemRows->itemNameStrings[i].width,
                                        FONT_LETTER_HEIGHT);
                    MenuSetQuadVertices(g_Menu->pItemRows->itemQuantityStrings[i].vertices, (u16)(x + 0x90) & 0xFFF8,
                                        (i / 2) * 0x10 | 0xE, g_Menu->pItemRows->itemQuantityStrings[i].width,
                                        FONT_LETTER_HEIGHT);
                    g_Menu->pItemRows->itemNameStrings[i].renderContext = *(u8*)&g_Menu->renderContext;
                    g_Menu->pItemRows->itemQuantityStrings[i].renderContext = *(u8*)&g_Menu->renderContext;
                g_Menu->pItemRows->shouldRenderRows[i] = 1;
            } else {
                g_GameState.itemIDs[arg0 * 2 + i] = 0;
                g_Menu->pItemRows->shouldRenderRows[i] = 0;
            }
        } else {
            g_GameState.itemQuantities[arg0 * 2 + i] = 0;
            g_Menu->pItemRows->shouldRenderRows[i] = 0;
        }
    }
    HeapFree(pWorkspace);
    g_Menu->pManager->shouldRenderUnk42C = 1;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DA5BC);




void MenuBuildItemPrims(s32 arg0, s32 arg1) {
    MenuShopItem* pItem;
    RECT rect;
    void* pImage;
    u8* pItemId;
    u8* pItemIds;
    u16 flags;
    u8 rowAlt;
    s32 quantityXOffset;
    s32 rowFlags;
    u8 row;

    pItemIds = g_GameState.itemIDs;
    pItemId = &pItemIds[arg1 * 2 + arg0];
    if (*pItemId != 0) {
        pImage = HeapAlloc(MENU_ITEM_DESC_IMAGE_BUFFER_SIZE, 0);
        bzero(pImage, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
        g_Menu->pItemRows->primGroupStrings[2].width =
            SystemRenderStringEntry(GetStringEntry(g_Menu->pItemRows->pVramBuffer, *pItemId), pImage, 0x39, 0);
        rect.x = 0x140;
        rect.y = 0x4E;
        rect.w = 0x3C;
        rect.h = FONT_LETTER_HEIGHT;
        LoadImage(&rect, pImage);
        DrawSync(0);
        MenuSetStringPrimitives(&g_Menu->pItemRows->primGroupStrings[2], 0, 0, 0);
        MenuSetFt4QuadVertices(&g_Menu->pItemRows->primGroupStrings[2].polys[g_Menu->renderContext], 0x1C, 0xA1, 0, 0x4E,
                               g_Menu->pItemRows->primGroupStrings[2].width, FONT_LETTER_HEIGHT);
        MenuSetQuadVertices(g_Menu->pItemRows->primGroupStrings[2].vertices, 0x1C, 0xA1,
                            g_Menu->pItemRows->primGroupStrings[2].width, FONT_LETTER_HEIGHT);
        HeapFree(pImage);
        quantityXOffset = (g_Menu->pItemRows->itemQuantityStrings[arg0].width == 0x10) * 4;
        memmove(&g_Menu->pItemRows->primGroupStrings[0], &g_Menu->pItemRows->itemNameStrings[arg0], sizeof(MenuString));
        memmove(&g_Menu->pItemRows->primGroupStrings[1], &g_Menu->pItemRows->itemQuantityStrings[arg0],
                sizeof(MenuString));
        MenuSetQuadVertices(g_Menu->pItemRows->primGroupStrings[0].vertices, 0x10, 0x93,
                            g_Menu->pItemRows->itemNameStrings[arg0].width, FONT_LETTER_HEIGHT);
        MenuSetQuadVertices(g_Menu->pItemRows->primGroupStrings[1].vertices, quantityXOffset | 0x78, 0x93,
                            g_Menu->pItemRows->itemQuantityStrings[arg0].width, FONT_LETTER_HEIGHT);
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[0].polys[g_Menu->renderContext])->r0 = 0x80;
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[0].polys[g_Menu->renderContext])->g0 = 0x80;
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[0].polys[g_Menu->renderContext])->b0 = 0x80;
        SetSemiTrans(&g_Menu->pItemRows->primGroupStrings[0].polys[g_Menu->renderContext], 0);
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[1].polys[g_Menu->renderContext])->r0 = 0x80;
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[1].polys[g_Menu->renderContext])->g0 = 0x80;
        ((P_TAG*)&g_Menu->pItemRows->primGroupStrings[1].polys[g_Menu->renderContext])->b0 = 0x80;
        SetSemiTrans(&g_Menu->pItemRows->primGroupStrings[1].polys[g_Menu->renderContext], 0);
        MenuClearBytes(MENU_ITEM_ROW_STRING_COUNT, g_Menu->pManager->shouldRenderStrings10E0);
        pItem = &g_Menu->pDressingRoom->pItemsData[*pItemId];
        if (pItem->displayFlags & 0xC0) {
            flags = pItem->flags;
            if (flags & 0x4000) {
                row = 2;
            } else {
                rowFlags = flags & 0x1000;
                row = rowFlags == 0;
            }
            MenuSetRowQuadVertices(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuColumnRowStringIds,
                                   g_MenuPanelRowXOffsets, g_Menu->pManager->shouldRenderStrings10E0, row, 0, 1);
            rowAlt = (pItem->flags & 3) + 3;
            MenuSetRowQuadVertices(MENU_ITEM_ROW_STRING_COUNT, g_Menu->itemRowStrings, g_MenuColumnRowStringIds,
                                   g_MenuPanelRowXOffsets, g_Menu->pManager->shouldRenderStrings10E0, rowAlt, 0, 1);
            ((P_TAG*)&g_Menu->itemRowStrings[row].polys[g_Menu->renderContext])->r0 = 0x80;
            ((P_TAG*)&g_Menu->itemRowStrings[row].polys[g_Menu->renderContext])->g0 = 0x80;
            ((P_TAG*)&g_Menu->itemRowStrings[row].polys[g_Menu->renderContext])->b0 = 0x80;
            SetSemiTrans(&g_Menu->itemRowStrings[row].polys[g_Menu->renderContext], 0);
            ((P_TAG*)&g_Menu->itemRowStrings[rowAlt].polys[g_Menu->renderContext])->r0 = 0x80;
            ((P_TAG*)&g_Menu->itemRowStrings[rowAlt].polys[g_Menu->renderContext])->g0 = 0x80;
            ((P_TAG*)&g_Menu->itemRowStrings[rowAlt].polys[g_Menu->renderContext])->b0 = 0x80;
            SetSemiTrans(&g_Menu->itemRowStrings[rowAlt].polys[g_Menu->renderContext], 0);
        }
        g_Menu->pItemRows->primGroupStrings[0].renderContext = g_Menu->renderContext;
        g_Menu->pItemRows->primGroupStrings[1].renderContext = g_Menu->renderContext;
        g_Menu->pItemRows->primGroupStrings[2].renderContext = g_Menu->renderContext;
        g_Menu->pItemRows->shouldRenderPrimGroup = 1;
    } else {
        MenuClearBytes(MENU_ITEM_ROW_STRING_COUNT, g_Menu->pManager->shouldRenderStrings10E0);
        g_Menu->pItemRows->shouldRenderPrimGroup = 0;
    }
}

void MenuInitializeArrowCursor(u_char index) {
    g_Menu->arrowCursors[index] = HeapAlloc(sizeof(MenuArrowCursor), 0);
    bzero(g_Menu->arrowCursors[index], sizeof(MenuArrowCursor));
    g_Menu->arrowCursors[index]->curAnimFrame = MENU_ARROW_CURSOR_LAST_ANIM_FRAME;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

void MenuBuildArrowCursorPrim(s32 arg0, s32 arg1, u_char arg2, u_char arg3) {
    MenuArrowCursor* pCursor;
    u8* pPoly;
    s32 ofsX;
    s32 ofsY;
    SVECTOR* pVertices;
    u16 x;
    u16 y;
    u16 lx0;
    u16 ly0;
    u16 lx1;
    u16 ly1;
    s32 shouldDraw = 1;

    pCursor = g_Menu->arrowCursors[arg3];
    if (++pCursor->animFrameDuration >= MENU_ARROW_CURSOR_ANIM_FRAME_TICKS) {
        pCursor->curAnimFrame--;
        if (pCursor->curAnimFrame < 0) {
            pCursor->curAnimFrame = MENU_ARROW_CURSOR_LAST_ANIM_FRAME;
        }
        pCursor->animFrameDuration = 0;
    }
    switch (arg2) {
    case 0:
        ofsX = (arg0 % 2) * 0x88 + 0x1C;
        ofsY = (arg0 / 2) * 0x10 + 0x11;
        break;
    case 1:
        if (arg0 >= arg1 * 2 && arg0 < arg1 * 2 + 0x10) {
            ofsX = (arg0 % 2) * 0x88 + 0x18;
            ofsY = ((arg0 - arg1 * 2) / 2) * 0x10 + 0x11;
        } else {
            shouldDraw = 0;
        }
        break;
    case 2:
        ofsX = (arg0 % 2) * 0x88 + 0x18;
        ofsY = (arg0 / 2) * 0x10 + 0x14;
        break;
    case 3:
        ofsX = 0xA0;
        ofsY = arg0 * 13 + 0x14;
        shouldDraw = 2;
        break;
    }
    if (shouldDraw != 0) {
        func_8002675C(g_Menu->resources, pCursor->curAnimFrame + MENU_TEX_ARROW_CURSOR, (POLY_FT4*)pCursor,
                      g_Menu->renderContext, 0, 0, 0x1000);
        pVertices = pCursor->vertices;
        pPoly = (u8*)pCursor + g_Menu->renderContext * sizeof(POLY_FT4);
        lx0 = *(u16*)(pPoly + 8);
        ly0 = *(u16*)(pPoly + 0xA);
        lx1 = *(u16*)(pPoly + 0x10);
        ly1 = *(u16*)(pPoly + 0x22);
        x = lx0 + ofsX;
        y = ly0 + ofsY;
        MenuSetQuadVertices(pVertices, x, y, lx1 - lx0, ly1 - ly0);
        pCursor->renderContext = *(u8*)&g_Menu->renderContext;
        g_Menu->pManager->shouldRenderArrowCursor[arg3] = 1;
    } else {
        g_Menu->pManager->shouldRenderArrowCursor[arg3] = 0;
    }
}
void MenuFreeArrowCursor(u_char index) {
    HeapFree(g_Menu->arrowCursors[index]);
    g_Menu->pManager->shouldRenderArrowCursor[index] = 0;
}

void MenuSetAllPrimsShadingMode(u_char arg0) {
    POLY_FT4* pPoly;
    MenuString* pRow;
    MenuString* pRows;
    s32 rc;
    s32 i;

    func_801E8F60(3, arg0);
    func_801E8F60(4, arg0);
    pPoly = (POLY_FT4*)(g_Menu->pScrollHandle->renderContext * sizeof(POLY_FT4) + (u32)g_Menu->pScrollHandle);
    MenuSetPrimShadingMode(pPoly, arg0);
    for (i = 0; i < 0x10; i++) {
        pRows = g_Menu->pItemRows->itemNameStrings;
        pRow = &pRows[i];
        if (g_Menu->pItemRows->itemNameStrings[i].polys[pRow->renderContext].r0 != 0x20) {
            MenuSetPrimShadingMode(&pRow->polys[pRow->renderContext], arg0);
            rc = g_Menu->pItemRows->itemQuantityStrings[i].renderContext;
            MenuSetPrimShadingMode(&g_Menu->pItemRows->itemQuantityStrings[i].polys[rc], arg0);
        }
    }
    for (i = 0; i < 2; i++) {
        pPoly = (POLY_FT4*)(g_Menu->arrowCursors[i]->renderContext * sizeof(POLY_FT4) + (u32)g_Menu->arrowCursors[i]);
        MenuSetPrimShadingMode(pPoly, arg0);
    }
    pPoly = (POLY_FT4*)(g_Menu->pItemRows->primGroupStrings[0].renderContext * sizeof(POLY_FT4) +
                        (u32)&g_Menu->pItemRows->primGroupStrings[0]);
    MenuSetPrimShadingMode(pPoly, arg0);
    pPoly = (POLY_FT4*)(g_Menu->pItemRows->primGroupStrings[1].renderContext * sizeof(POLY_FT4) +
                        (u32)&g_Menu->pItemRows->primGroupStrings[1]);
    MenuSetPrimShadingMode(pPoly, arg0);
    pPoly = (POLY_FT4*)(g_Menu->pItemRows->primGroupStrings[2].renderContext * sizeof(POLY_FT4) +
                        (u32)&g_Menu->pItemRows->primGroupStrings[2]);
    MenuSetPrimShadingMode(pPoly, arg0);
    for (i = 0; i < 8; i++) {
        pPoly = &g_Menu->itemRowStrings[i].polys[g_Menu->itemRowStrings[i].renderContext];
        MenuSetPrimShadingMode(pPoly, arg0);
    }
}

void MenuBuildPartyCharacterPrims(s32 arg0) {
    MenuCharacter* pCharacter;
    s32* pPositionsX;
    s32* pPositionsY;
    u8 characterId;
    u8 shouldBuild;
    s32 i;

    if (g_MenuCharactersAllocated == 0) {
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            pCharacter = (g_Menu->currentCharacters[i] = HeapAlloc(sizeof(MenuCharacter), 0));
            bzero(pCharacter, sizeof(MenuCharacter));
        }
        g_MenuCharactersAllocated = 1;
    }
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        bzero(g_Menu->currentCharacters[i], sizeof(MenuCharacter));
    }
    // Retail keeps the gear-position flag in the parameter's own variable: the
    // 0/1 store reuses its register, and a separate local lands in $v0.
    if ((u_char)arg0 != 2) {
        arg0 = 0;
        pPositionsX = g_MenuCharacterLabelXPositions;
        pPositionsY = g_MenuCharacterLabelYPositions;
    } else {
        pPositionsX = g_MenuGearCharacterLabelXPositions;
        pPositionsY = g_MenuGearCharacterLabelYPositions;
        arg0 = 1;
    }
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        pCharacter = g_Menu->currentCharacters[i];
        characterId = g_Menu->pManager->currentCharacterIDs[i];
        shouldBuild = 1;
        if (characterId != CHARACTER_ID_NONE) {
            if ((u_char)arg0) {
                shouldBuild = g_GameState.characters[characterId].gearId != GEAR_ID_NONE;
            }
            if (shouldBuild) {
                MenuBuildCharacterPrims(pCharacter, characterId, i, (s32)pPositionsX, (s32)pPositionsY,
                                        (u_char)arg0);
            }
        } else {
            pCharacter->shouldRender = 0;
        }
    }
    g_Menu->pManager->shouldRenderCharacters = 1;
    // The three prims are reached index-first: the `polysCursor[...]` subscript
    // re-bases the address sum onto the block pointer, which is the commuted addu
    // retail does not have.
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].x0 = 0x90;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].y0 = i * 0x38 + 0x30;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].x1 = 0xA0;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].y1 = i * 0x38 + 0x30;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].x2 = 0x90;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].y2 = i * 0x38 + 0x40;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].x3 = 0xA0;
        (i * 2 + g_Menu->pCursors->renderContexts[i])[g_Menu->pCursors->polysCursor].y3 = i * 0x38 + 0x40;
    }
}
