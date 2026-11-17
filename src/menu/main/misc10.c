#include "common.h"

#include "system/menu.h"
#include "main/game.h"

// Limits MenuComputeCharacterStats puts on the dressing room's stat values.
#define MENU_STAT_MAX 250
#define MENU_STAT_PERCENT_MAX 99 // hit and evade
#define MENU_STAT_AGILITY_MAX 20
#define MENU_STAT_AGILITY_CAPPED 16 // what agility above the maximum is set to

extern u8 D_801EA564[];
extern u8 D_801EA568[];
extern u8 D_801E9808[];
extern u8 D_801E9811;
extern u8 D_801E9EA0[];
extern u8 D_801EA56E[];
extern u32 D_801E9F48[];
extern u8 g_FieldPlayerWalkmeshMaterial;

void MenuLoadStringsFromIds(u8, u8*, u8*, void*);
void MenuApplyGearFrame(MenuDressingRoom*, u8);
void MenuApplyGearArmor(MenuDressingRoom*, u8);
void MenuApplyGearEngine(MenuDressingRoom*, u8);
void func_801C72BC(u8);
void MenuFreeShoulderButtonUi(void);
void MenuFreeWindow(u8);
void MenuRenderFrame(void);
void MenuClearBytes(u8, u8*);
void MenuLoadSubMenuChoiceStrings(u8);
void func_801D3488(u8, u8);
void func_801E05D0(u8, u8, u8);
void MenuShutdown(void);
void MenuFreeUnk430Panel(u8);
void MenuFreeUnk42CPanel(void);
void MenuFreeCharacterSpriteRecords(void);
void MenuFreeUnk35CPanel(void);
void MenuClearSubMenuChoiceFlags(void);
void MenuPlaySoundEffect(u8);
s32 func_801D9704(s32, u8, u8);
void MenuInitializeUnk438Panel(void);
void func_801E1AC8(u8);
void MenuSetRowQuadVertices(s32, void*, void*, void*, void*, u8, u8, u8);
void MenuCreateWindow(u_char, u_char, u_char, u_char, u16, u_char, u_char, u32, u_char);
void MenuApplyEquippedItemBonuses(MenuDressingRoom*, u8);
void MenuComputeCharacterStats(MenuDressingRoom*, u8);
void func_801D261C(void);
void func_801D29A8(s32, s32);
void func_801D8EA4(u8, u8, u8, u8);
void func_801DDF24(u8, u8, u8);
void func_801E86C8(u8);
void MenuStartOpenAnimation(void);
void MenuResetSelectionCursorFlags(void);
void MenuMovePointerCursor(s32, u8);
void MenuUpdateDressingRoomTotals(u_char);
void func_801D7CFC(s32, u8, u8);
void MenuDrawStatTable(u8, u8, u8, u8);
void MenuDrawGearScreen(u_char, u_char);
void MenuSetSubMenuChoicePrims(u8);

int MenuInitializeUnk35CPanel(u8 arg0, u8 arg1) {
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->pEquipmentPanel = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->pEquipmentPanel, 0x2AC);
    func_801C72BC(3);
    func_801E05D0(arg0, arg1, 0);
    func_801C72BC(0x13);
    return 1;
}
void MenuInitializeUnk438Panel(void) {
    MenuManager* pManager;
    int i;
    /* One variable for both the count and the inner POLY_G4 index below: it has
       to stay live across func_801D3488 so that it lands in a call-saved
       register, which is where retail keeps it. */
    int count;

    g_Menu->unk438 = HeapAlloc(0x25C0, 0);
    bzero(g_Menu->unk438, 0x25C0);
    func_801C72BC(4);
    MenuLoadStringsFromIds(2, (u8*)g_Menu + 0x17E0, D_801EA564, (u8*)g_Menu->pManager + 0x4E);
    MenuSetRowQuadVertices(2, (u8*)g_Menu + 0x17E0, D_801EA564, D_801E9EA0, (u8*)g_Menu->pManager + 0x4E, 0, 0, 4);
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
        func_801D3488(3, 0);
    } else {
        g_Menu->pManager->shoulderButtonUiActive = 0;
    }
    for (i = 0; i < 0xD; i++) {
        for (count = 0; count < 2; count++) {
            /* (i * 0x48 + 0x21D0) must stay grouped for retail's address build. */
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
    func_801C72BC(0x14);
    HeapFree(g_Menu->unk438);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E1418);

/* objdiff 75.33% (296 bytes, same size; was 68.19% with the old if/else order).
   Settled: charId = g_Menu->pManager->currentCharacterIDs[slot]; weight =
   *pWeight++ where pWeight = (u16*)((u8*)&g_GameState.characters[charId] + 0x90);
   value = pRow[i] where pRow = (u8*)pTable + charId * 0x110 + index * 0xE and
   pTable = *(u32*)((u8*)g_Menu->unk438 + 0x2578); total/count are u32, the loop
   counter is a signed int (retail ends the loop with slti, not sltiu). The
   weight != 0 test comes FIRST in the source (retail branches over it into the
   `else if (value != 0) count++;` block) -- writing `if (weight == 0)` first
   costs 7 points and 4 bytes.
   Residue: retail recomputes charId * 0x110 AND index * 0xE inside the loop and
   keeps `li v0,0xffff` in the loop, i.e. loop.c hoisted neither -- yet a hoisted
   pRow local scores 75.33% because it lands in the pre-header, and the flat
   `*(u16*)((u8*)pTable + charId*0x110 + index*0xE + i*2)` does reproduce the
   non-hoisting but scores 66.44%: gcc then emits the index * 0xE chain before the
   charId chain (retail the reverse: charId chain, addu t3, index chain) and
   rotates the whole register assignment (retail t0=total, t1=i, a2=count,
   t2=pWeight, a3=charId, t3=pTable, a1=index; this compile gives total a3,
   index a masked t2 copy, charId a1, pWeight t0). Tried this round and no
   better: flat expression with the (u32)index cast dropped (66.44, cdk too),
   explicit ((...) + (index*0xE)) + i*2 parenthesisation (66.44), the indexed
   pRow[i] form (75.33 after hoisting), gcc-2.7.2-cdk on the flat form (66.44).
   Both 2.6.0 and 2.7.2 emit byte-identical code for the hoisted version, so this
   is not a preset question.
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
    if ((u32)index >= 7 && (g_GameState.unk22B6 & 0x4000) == 0) {
        return 0;
    }
    count = 0;
    charId = g_Menu->pManager->currentCharacterIDs[slot];
    pWeight = (u16*)((u8*)&g_GameState.characters[charId] + 0x90);
    pTable = *(u16**)((u8*)g_Menu->unk438 + 0x2578);
    pRow = (u16*)((u8*)pTable + charId * 0x110 + index * 0xE);
    for (i = 0; i < 7; i++) {
        weight = *pWeight++;
        value = pRow[i];
        if (weight != 0) {
            if (value != 0) {
                if (value == 0xFFFF) {
                    count++;
                } else {
                    pct = weight * 100 / value;
                    if (pct < 100) {
                        total += pct;
                    } else {
                        total += 100;
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E1544);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E1AC8);
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

    g_Menu->unk358 = HeapAlloc(0x2AF0, 0);
    bzero(g_Menu->unk358, 0x2AF0);
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->pEquipmentPanel = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->pEquipmentPanel, 0x2AC);
    func_801C72BC(3);
    i = 0;
    while (1) {
        if (g_Menu->pManager->isGearScreenActive[i] != 0) {
            break;
        }
        i++;
    }
    func_801D3488(0, 1);
    return i;
}
void MenuLoadUnk18E0Strings(u8 arg0) {
    MenuLoadStringsFromIds(6, (u8*)g_Menu + 0x18E0, D_801EA568 + arg0, (u8*)g_Menu->pManager + 0x54);
}
void MenuFreeGearScreenBuffers(void) {
    HeapFree(g_Menu->unk358);
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->pEquipmentPanel);
    func_801C72BC(0x13);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E23CC);
/* Best: 98.48% (1808 bytes, retail 1812), from a 94.95 first draft.
   Structure is complete and the case bodies are in retail order (4, 5, 1, 3, 9, 10
   outer, and 0, 1, 2, 3 inner). What closed most of the gap: the four
   MenuPlaySoundEffect(4) arms are the ELSE of the "not selectable" test
   (writing them first costs 1.5 points each — retail branches INTO the shared
   sound block from every case, i.e. the body is the fall-through arm), and
   shouldRenderSubmenuStrings = 0 must be written BEFORE func_801E05D0 because
   retail has it in that call s delay slot (it then reuses the pre-call pManager
   in $v1 instead of reloading g_Menu).
   Left: retail keeps slot in $s2 and lastSlot in $s1, this body gets the
   opposite pair (slot $s1), so ~40 instructions differ by one register, and the
   case-3 sound arm is laid out inline (beqz over it) where retail has the shared
   block, which is also where the missing 4 bytes are. Tried: the 4-minute
   permuter (PERMUTER_BETTER 1552 -> 1353, its only finding was the case-1 arm
   inversion, which is in this body), arm inversion on all four cases. Next: a
   permuter run from THIS body chasing the s1/s2 tie.
// Gear dressing room loop: same structure as MenuRunCharacterDressingRoomLoop above.
u8 func_801E23CC(s32 slot) {
    u8 active;
    u8 lastSlot;
    u8 firstDraw;
    u8 stringSet;

    slot = func_801D9704(slot, 0, 1);
    active = 1;
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
            MenuSetRowQuadVertices(6, (u8*)g_Menu + 0x18E0, D_801EA56E, D_801E9F48, (u8*)g_Menu->pManager + 0x54, 4, 7, 6);
            MenuSetRowQuadVertices(6, (u8*)g_Menu + 0x18E0, D_801EA56E, D_801E9F48, (u8*)g_Menu->pManager + 0x54, 5, 7, 6);
            g_Menu->unk18E0[4].hasVertices = 1;
            g_Menu->unk18E0[5].hasVertices = 1;
            if (firstDraw != 0) {
                MenuStartOpenAnimation();
                firstDraw = 0;
                func_801D29A8(0, 0);
                func_801E86C8(0);
            }
        }
        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            MenuSetRowQuadVertices(4, (u8*)g_Menu + 0x18E0, D_801EA56E, D_801E9F48 + stringSet, (u8*)g_Menu->pManager + 0x54, g_Menu->subMenuChoice, 7, 0);
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
                    func_801D7CFC(slot, 1, g_GameState.gearRide[slot]);
                } else {
                    MenuPlaySoundEffect(4);
                }
                lastSlot = 0xFF;
                break;
            }
            g_Menu->pManager->shouldRenderStrings18E0[4] = 1;
            g_Menu->pManager->shouldRenderStrings18E0[5] = 1;
            MenuLoadStringsFromIds(6, (u8*)g_Menu + 0x18E0, D_801EA568 + stringSet, (u8*)g_Menu->pManager + 0x54);
            g_Menu->pManager->shouldRenderUnk358Prims = 1;
            g_Menu->pManager->shouldRenderUnk35C = 1;
            g_Menu->pManager->shouldRenderPanel[1] = 1;
            MenuMovePointerCursor(g_Menu->subMenuChoice + 7, 1);
            g_Menu->pManager->shouldRenderPointerCursor = 1;
            g_Menu->subMenuPrevChoice = 0xFF;
            func_801D3488(0, 1);
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
} */
void MenuInitializeCharacterScreenBuffers(void) {
    MenuLoadSubMenuChoiceStrings(1);
    g_Menu->unk358 = HeapAlloc(0x2AF0, 0);
    bzero(g_Menu->unk358, 0x2AF0);
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->pEquipmentPanel = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->pEquipmentPanel, 0x2AC);
    func_801C72BC(3);
    func_801D3488(0, 0);
}
void MenuFreeCharacterScreenBuffers(void) {
    HeapFree(g_Menu->unk358);
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->pEquipmentPanel);
    func_801C72BC(0x13);
}
/* objdiff 96.76% (1188 bytes, exactly retail's size). Structure is complete: every call, both switches and all case bodies are in retail's body order (4, 5, 1, 3, 9, 10 and the nested 0, 1, 2). The one residue is the `lastSlot = slot` copy: retail emits a two-instruction u8 copy (`move v0,s1` before the MenuComputeCharacterStats jal, `move s0,v0` in its delay slot) AFTER that call, while gcc 2.6.0 here always folds it to a single `move s0,s1` and then steals it into the PREVIOUS call's delay slot, so every branch past it is 4 bytes early and one extra instruction turns up later. Tried: plain assignment, embedded assignment in the MenuDrawGearScreen argument, an extra u8 intermediate local, and a 3 min permuter run (best was a bogus case-3 mutation at 96.04). Note the MenuMovePointerCursor prototype must be (s32, u8) or every call site gains a useless andi.
*/
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
                func_801D29A8(0, 0);
                func_801E86C8(0);
            }
        }
        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            func_801D261C();
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
            func_801D3488(0, 0);
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
/* Best: 89.92% (1008 bytes, retail 1024). Applies one inventory item's stat
   effects to g_GameState.characters[charIndex]; returns nonzero when the item
   could not do anything (both bars already full), which is why its caller
   (misc9) consumes the item only on 0.

   Settled: pItem = pDressingRoom->pItemsData + itemIndex (MenuShopItem is 0x10
   bytes; the fields this unit reads are 0x8 = amount (u8), 0xA = effect flags,
   0xC = stat/status flags). 0xA: 0x8000 hp, 0x4000 mp, 4 stat bonuses, 2 the
   0x78 field, 1 the inventory rebuild call. 0xC: 0x8000 attack, 0x4000 defense,
   0x2000 ether, 0x1000 etherDefense, 0x800 maxHp, 0x400 maxMp; the 0x78 block
   runs when 0xA has bit 2, and builds the result as
   hpFull && mpFull / hpFull / mpFull / 0 for the four 0xA flag combinations.
   Every branch is written as `if (hp == maxHp) hpFull = 1; else ...` (the
   equality in the `if` gives retail's bne-to-body plus its one-instruction j);
   the whole 0xA&4 block and all six clamps are instruction-identical.

   The two multiplications are the reason this is parked. A literal
   (`hp + amount * 50`) makes gcc 2.6.0 synthesise a shift chain
   (sll,addu,sll,addu,sll) - retail has `mult v1,v0` with the 50 materialised by
   `ori v0,zero,0x32` in the preceding bne's delay slot. It only emits `mult`
   when the multiplier is a *variable*, so the source multiplied by a local
   (`hpScale = 50`) - but then the li's position and register differ from
   retail: retail's 50 sits in block 1's bne delay slot (matched) and its 10 is
   hoisted from the head of the merge block into the FIRST beqz's delay slot,
   while this body puts the 10 in block 2's beqz slot and retail has `nop`
   there.

   Also left: (a) frame 40 vs retail's 48 - declared-but-unused s32 locals do
   not grow the frame on gcc 2.6.0, so whatever occupies retail's extra 8 bytes
   is live; (b) the 0xA&2 block - retail keeps the lh 0xC in a register, copies
   it (`move v1,v0`) and compares `slt` against the `andi 0xff` copy, then
   reloads `lbu 0xC(s0)` for the subtraction, where this body's `(u8)` cast
   folds into a single lbu and gives `sltu`; (c) the final block - this body
   builds the result in a0 and copies, retail builds it in v0.
   Tried: both scales as literals/u16/s32 locals at the top, both inside their
   else blocks, 2.7.2 and 2.7.2-cdk (82.27 / 77.23), -O1/-O3 (79.53 / 86.60).
   Next: a permuter run from this body, then (b)+(c) by hand.

   Item record as this unit reads it; the stat-effect fields are the same ones
   MenuShopItem now declares at 0x8/0xA/0xC.
typedef struct {
    u16 unk0;                  // 0x00
    u16 price;                 // 0x02
    u8 unk4[4];                // 0x04
    u8 amount;                 // 0x08
    u8 unk9;                   // 0x09
    s16 unkA;                  // 0x0A
    s16 unkC;                  // 0x0C
    u8 unkE[2];                // 0x0E
} MenuItemStatEffect; // Size: 0x10

s32 func_801E31C0(MenuDressingRoom* pDressingRoom, u8 charIndex, u8 itemIndex) {
    MenuItemStatEffect* pItem;
    GameCharacter* pChar;
    u8 hpFull;
    u8 mpFull;
    s32 result;
    s32 hpScale;
    s32 mpScale;

    hpFull = 0;
    mpFull = 0;
    pItem = (MenuItemStatEffect*)(pDressingRoom->pItemsData + itemIndex);
    pChar = &g_GameState.characters[charIndex];
    if (pItem->unkA & 0x8000) {
        if (pChar->hp == pChar->maxHp) {
            hpFull = 1;
        } else {
            hpScale = 50;
            pChar->hp = pChar->hp + pItem->amount * hpScale;
        }
    }
    mpScale = 10;
    if (pItem->unkA & 0x4000) {
        if (pChar->mp == pChar->maxMp) {
            mpFull = 1;
        } else {
            pChar->mp = pChar->mp + pItem->amount * mpScale;
        }
    }
    if (pChar->hp > pChar->maxHp) {
        pChar->hp = pChar->maxHp;
    }
    if (pChar->mp > pChar->maxMp) {
        pChar->mp = pChar->maxMp;
    }
    if (pItem->unkA & 4) {
        if (pItem->unkC & 0x8000) {
            pChar->attack = pChar->attack + pItem->amount;
        }
        if (pItem->unkC & 0x4000) {
            pChar->defense = pChar->defense + pItem->amount;
        }
        if (pItem->unkC & 0x2000) {
            pChar->ether = pChar->ether + pItem->amount;
        }
        if (pItem->unkC & 0x1000) {
            pChar->etherDefense = pChar->etherDefense + pItem->amount;
        }
        if (pItem->unkC & 0x800) {
            pChar->maxHp = pChar->maxHp + pItem->amount;
        }
        if (pItem->unkC & 0x400) {
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
    if (pItem->unkA & 2) {
        if (pItem->unkC & 0x8000) {
            pChar->unk78 = pChar->unk78 + pItem->amount;
            if (pChar->unk78 > 200) {
                pChar->unk78 = 200;
            }
        } else if (pChar->unk78 < (u8)pItem->unkC) {
            pChar->unk78 = 0;
        } else {
            pChar->unk78 = pChar->unk78 - pItem->unkC;
        }
    }
    if (pItem->unkA & 1) {
        if (pItem->amount == 1) {
            MenuFillInventoryTables();
        } else if (pItem->amount == 2) {
            func_801E5178();
        }
    }
    if (pItem->unkA & 0x8000) {
        if (pItem->unkA & 0x4000) {
            result = hpFull ? mpFull : 0;
        } else {
            result = hpFull;
        }
    } else if (pItem->unkA & 0x4000) {
        result = mpFull;
    } else {
        result = 0;
    }
    return result;
}
*/
void MenuApplyDressingRoomSlotHpBonus(MenuDressingRoom* pDressingRoom, u8 charIndex, u8 targetIndex, u8 slotIndex, u8 isGear) {
    GameCharacter* pChar;
    GameCharacter* pTarget;
    u8* pSlots;
    GameGear* pGear;
    u32 value;

    pChar = &g_GameState.characters[charIndex];
    pTarget = &g_GameState.characters[targetIndex];
    pGear = &g_GameState.gears[g_GameState.characters[charIndex].gearId];
    if (isGear == 0) {
        pSlots = pDressingRoom->pCharacterSlots[charIndex];
        pSlots = &pSlots[slotIndex * 0x28];
        value = pChar->ether * pSlots[0x11];
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
    pChar->unk31 = 0;
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
           tests, and adds it as the left operand, so spell it that way. */
        value = pAccessory->unkC[0];
        if (*(s16*)&pAccessory->unkC[0] & 0x8000) {
            pChar->attackFromEquipment = value + pChar->attackFromEquipment;
        }
        if (*(u16*)&pAccessory->unkC[0] & 0x4000) {
            pChar->defenseFromEquipment = value + pChar->defenseFromEquipment;
        }
        if (*(u16*)&pAccessory->unkC[0] & 0x2000) {
            pChar->agilityFromEquipment = value + pChar->agilityFromEquipment;
        }
        if (*(u16*)&pAccessory->unkC[0] & 0x1000) {
            pChar->etherFromEquipment = value + pChar->etherFromEquipment;
        }
        if (*(u16*)&pAccessory->unkC[0] & 0x800) {
            pChar->etherDefenseFromEquipment = value + pChar->etherDefenseFromEquipment;
        }
        if (*(u16*)&pAccessory->unkC[0] & 0x400) {
            pChar->hitPercentageFromEquipment = value + pChar->hitPercentageFromEquipment;
        }
        if (*(u16*)&pAccessory->unkC[0] & 0x200) {
            pChar->evadePercentageFromEquipment = value + pChar->evadePercentageFromEquipment;
        }
        if (*(u16*)&pAccessory->unkC[0] & 0x100) {
            pChar->defenseFromAccessories = value + pChar->defenseFromAccessories;
        }
    }

    pWeapon = pDressingRoom->pWeaponsData;
    pWeapon += pChar->weaponId[0];
    pChar->slots[0].attack = pWeapon->attack;
    pChar->slots[0].flags = pWeapon->slotFlags;
    pChar->slots[0].unk2 = pWeapon->unkA;
    pChar->slots[0].unk3 = pWeapon->unkB;
    if (pChar->unk56 == 4) {
        pWeapon = pDressingRoom->pWeaponsData;
        pWeapon += pChar->extraWeaponIds[0];
        pChar->slots[0].attack = pWeapon->attack;
        pChar->slots[0].flags = pWeapon->slotFlags;
        pChar->slots[0].unk2 = pWeapon->unkA;
        pChar->slots[0].unk3 = pWeapon->unkB;
        pWeapon = pDressingRoom->pWeaponsData;
        pWeapon += pChar->extraWeaponIds[3];
        pChar->slots[3].attack = pWeapon->attack;
        pChar->slots[3].flags = pWeapon->slotFlags;
        pChar->slots[3].unk2 = pWeapon->unkA;
        pChar->slots[3].unk3 = pWeapon->unkB;
    }
    if (pChar->slots[0].unk3 == 0x64) {
        pChar->accessoryEffectFlags7 |= pChar->slots[0].flags;
    }
}

void MenuComputeCharacterStats(MenuDressingRoom* pDressingRoom, u8 charIndex) {
    GameCharacter* pChar;

    pChar = &g_GameState.characters[charIndex];
    if (pChar->unk56 == 4) {
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
    pilotId = D_801E9808[gearIndex];
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E3ECC);
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
