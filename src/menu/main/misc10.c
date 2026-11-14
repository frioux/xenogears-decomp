#include "common.h"

#include "system/menu.h"
#include "main/game.h"

extern u8 D_801EA564[];
extern u8 D_801EA568[];
extern u8 D_801E9808[];
extern u8 D_801E9811;
extern u8 D_801E9EA0[];

void func_801E8018(u8, u8*, u8*, void*);
void MenuApplyGearFrame(MenuDressingRoom*, u8);
void MenuApplyGearArmor(MenuDressingRoom*, u8);
void MenuApplyGearEngine(MenuDressingRoom*, u8);
void func_801C72BC(u8);
void MenuFreeShoulderButtonUi(void);
void func_801D4EA0(u8);
void MenuRenderFrame(void);
void func_801E8044(u8, u8*);
void MenuLoadSubMenuChoiceStrings(u8);
void func_801D3488(u8, u8);
void func_801E05D0(u8, u8, u8);
void func_801D9E3C(void);
void func_801DC2CC(u8);
void func_801DA518(void);
void func_801DE36C(void);
void func_801DE400(void);
void MenuClearSubMenuChoiceFlags(void);
void MenuPlaySoundEffect(u8);
u8 func_801D9704(u8, u8, u8);
void MenuInitializeUnk438Panel(void);
void func_801E1AC8(u8);
void func_801E8070(s32, void*, void*, void*, void*, u8, u8, u8);
void MenuCreateWindow(u_char, u_char, u_char, u_char, u16, u_char, u_char, u32, u_char);

int MenuInitializeUnk35CPanel(u8 arg0, u8 arg1) {
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->unk360 = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->unk360, 0x2AC);
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
    func_801E8018(2, (u8*)g_Menu + 0x17E0, D_801EA564, (u8*)g_Menu->pManager + 0x4E);
    func_801E8070(2, (u8*)g_Menu + 0x17E0, D_801EA564, D_801E9EA0, (u8*)g_Menu->pManager + 0x4E, 0, 0, 4);
    MenuCreateWindow(2, 0x44, 0xA, 0xE4, 0xC4, 0, 1, 4, 0);
    g_Menu->pManager->unk5[1] = 0;
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
    g_Menu->pManager->unk4A[3] = 0;
    func_801D4EA0(2);
    MenuRenderFrame();
    func_801E8044(2, &g_Menu->pManager->unk4A[4]);
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
    g_Menu->unk360 = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->unk360, 0x2AC);
    func_801C72BC(3);
    i = 0;
    while (1) {
        if (g_Menu->pManager->unk60[i] != 0) {
            break;
        }
        i++;
    }
    func_801D3488(0, 1);
    return i;
}
void MenuLoadUnk18E0Strings(u8 arg0) {
    func_801E8018(6, (u8*)g_Menu + 0x18E0, D_801EA568 + arg0, (u8*)g_Menu->pManager + 0x54);
}
void MenuFreeGearScreenBuffers(void) {
    HeapFree(g_Menu->unk358);
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->unk360);
    func_801C72BC(0x13);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E23CC);
void MenuInitializeCharacterScreenBuffers(void) {
    MenuLoadSubMenuChoiceStrings(1);
    g_Menu->unk358 = HeapAlloc(0x2AF0, 0);
    bzero(g_Menu->unk358, 0x2AF0);
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->unk360 = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->unk360, 0x2AC);
    func_801C72BC(3);
    func_801D3488(0, 0);
}
void MenuFreeCharacterScreenBuffers(void) {
    HeapFree(g_Menu->unk358);
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->unk360);
    func_801C72BC(0x13);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E2BE4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E3088);

/* objdiff 99.87% (312 bytes, same size) but NOT linkable: the jump table
   jtbl_801C51DC lives in asm/menu/data/0.rodata.s and its entries are .word
   .L801E31B0/.L801E30C8/... labels defined inside this function's asm, so
   converting the function leaves those references undefined and the overlay
   stops linking. Checked with tools/scripts/check_bin.sh menu.
void func_801E3088(u8 arg0) {
    switch (g_Menu->mainMenuChoice + arg0) {
    case 0:
    case 7:
    case 9:
        break;
    case 1:
    case 8:
        func_801D9E3C();
        break;
    case 2:
        g_Menu->pManager->unk5[2] = 0;
        g_Menu->pManager->unk8 = 0;
        g_Menu->pManager->unk4A[1] = 0;
        MenuFreeGearScreenBuffers();
        break;
    case 3:
        func_801DC2CC(0);
        break;
    case 4:
        func_801DA518();
        break;
    case 5:
        func_801DE36C();
        func_801DE400();
        break;
    case 6:
        g_Menu->pManager->unk5[2] = 0;
        g_Menu->pManager->unk8 = 0;
        g_Menu->pManager->unk4A[1] = 0;
        MenuClearSubMenuChoiceFlags();
        MenuFreeCharacterScreenBuffers();
        break;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E31C0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E35BC);

/* objdiff 96.71% (280 bytes, same size; 80.36% was the previous 2.7.2 attempt).
   Structure is complete: pTarget->hp += value and both clamps (which reload the
   stored halfword exactly as retail does) already match byte for byte, as does
   the isGear else-branch. The only residue is the five-instruction register
   choice in the isGear == 0 head: retail builds the base temp in v0
   (addu v0,t0,t2), loads pSlots into a0 and the slot byte into v0 with ether in
   v1; this compile always builds the base in a0, the pointer in v1 and ether in
   a0. The instruction multiset is already identical, so this is the allocator
   tie-break class. Shapes tried this round (all 280 bytes):
   ether hoisted into its own local before pSlots (96.71), pSlot as one flat
   expression (93.86), raw *(u8**)(unk20 + charIndex*4) load (93.86). Earlier:
   single-statement indexing (93.86), &pSlots[slotIndex*0x28] vs byte offset
   (96.71 both), swapped multiply operands (93.33%), gcc-2.7.2-cdk (84.13%).
   The pSlots table is pDressingRoom->unk20[charIndex] and each slot record is
   0x28 bytes with a u8 at 0x11 -- NOT sizeof(GameCharacter) (0xA4); using
   GameCharacter* gave a 288-byte object and 80.36%.
void func_801E35BC(MenuDressingRoom* pDressingRoom, u8 charIndex, u8 targetIndex, u8 slotIndex, u8 isGear) {
    GameCharacter* pChar;
    GameCharacter* pTarget;
    u8* pSlots;
    u8* pSlot;
    GameGear* pGear;
    u32 value;

    pChar = &g_GameState.characters[charIndex];
    pTarget = &g_GameState.characters[targetIndex];
    pGear = &g_GameState.gears[g_GameState.characters[charIndex].gearId];
    if (isGear == 0) {
        pSlots = ((u8**)pDressingRoom->unk20)[charIndex];
        pSlot = &pSlots[slotIndex * 0x28];
        value = pChar->ether * pSlot[0x11];
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
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E36D4);
void MenuComputeCharacterStats(MenuDressingRoom* pDressingRoom, u8 charIndex) {
    GameCharacter* pChar;

    pChar = &g_GameState.characters[charIndex];
    if (pChar->unk56 == 4) {
        pDressingRoom->unkB8[0] = (pChar->unk0[4] + pChar->unk0[0x1C]) * 6 / 10;
    } else {
        pDressingRoom->unkB8[0] = pChar->attack + pChar->unk0[0x28] + pChar->unk0[4];
    }
    pDressingRoom->unkB8[1] = pChar->hitPercentage + pChar->unk2D[1];
    pDressingRoom->unkB8[2] = pChar->defense + pChar->unk0[0x29] + pChar->unk2D[0];
    pDressingRoom->unkB8[3] = pChar->evadePercentage + pChar->unk2D[2];
    pDressingRoom->unkB8[4] = pChar->ether + pChar->unk0[0x2B];
    pDressingRoom->unkB8[5] = pChar->etherDefense + pChar->etherDefenseFromEquipment;
    pDressingRoom->unkB8[6] = pChar->agility + pChar->unk0[0x2A];
    if (pDressingRoom->unkB8[0] > 0xFA) {
        pDressingRoom->unkB8[0] = 0xFA;
    }
    if (pDressingRoom->unkB8[1] > 0x63) {
        pDressingRoom->unkB8[1] = 0x63;
    }
    if (pDressingRoom->unkB8[2] > 0xFA) {
        pDressingRoom->unkB8[2] = 0xFA;
    }
    if (pDressingRoom->unkB8[3] > 0x63) {
        pDressingRoom->unkB8[3] = 0x63;
    }
    if (pDressingRoom->unkB8[4] > 0xFA) {
        pDressingRoom->unkB8[4] = 0xFA;
    }
    if (pDressingRoom->unkB8[5] > 0xFA) {
        pDressingRoom->unkB8[5] = 0xFA;
    }
    if (pDressingRoom->unkB8[6] > 0x14) {
        pDressingRoom->unkB8[6] = 0x10;
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
        g_GameState.gears[7].engineOutput = g_GameState.characters[7].attack + g_GameState.characters[7].unk0[0x28];
        g_GameState.gears[7].baseDefense = (g_GameState.characters[7].defense + g_GameState.characters[7].unk0[0x29]) * 12;
        g_GameState.gears[7].baseEtherDefense = (g_GameState.characters[7].etherDefense + g_GameState.characters[7].etherDefenseFromEquipment) * 6;
        g_GameState.gears[7].agility = g_GameState.characters[7].agility + g_GameState.characters[7].unk0[0x2A];
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
        pDressingRoom->unkB0 = (pGear->weaponData[0].unk2 + pGear->weaponData[2].unk2) * 6 / 10 + value;
    } else {
        pDressingRoom->unkB0 = pGear->weaponData[0].unk2 + value;
    }
    pDressingRoom->totalResponsiveness = pGear->unk9F + pGear->responsiveness;
    pDressingRoom->unkB3 = pGear->agility - pGear->unk4A;
    pDressingRoom->unkB4 = pGear->etherAmplification + pGear->unk54;
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
    pGear->agility = pFrame->unk14;
    pGear->etherAmplification = pFrame->unk15;
    pGear->unk9D = pFrame->unk16;
    pGear->unk9F = pFrame->unk17;
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

    pGear->baseDefense = pArmor->unk8;
    pGear->baseEtherDefense = pArmor->unkA;
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
