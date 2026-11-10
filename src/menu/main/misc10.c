#include "common.h"

#include "system/menu.h"
#include "main/game.h"

extern u8 D_801EA568[];

void func_801E8018(u8, u8*, u8*, void*);
void func_801E41C0(MenuDressingRoom*, u8);
void func_801E4258(MenuDressingRoom*, u8);
void func_801E42AC(MenuDressingRoom*, u8);
void func_801C72BC(u8);
void func_801D3674(void);
void func_801D4EA0(u8);
void func_801C7BF4(void);
void func_801E8044(u8, u8*);
void func_801D249C(u8);
void func_801D3488(u8, u8);
void func_801E05D0(u8, u8, u8);
void func_801D9E3C(void);
void func_801DC2CC(u8);
void func_801DA518(void);
void func_801DE36C(void);
void func_801DE400(void);
void func_801D25E4(void);

int func_801E0F78(u8 arg0, u8 arg1) {
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->unk360 = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->unk360, 0x2AC);
    func_801C72BC(3);
    func_801E05D0(arg0, arg1, 0);
    func_801C72BC(0x13);
    return 1;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E1014);
void func_801E1398(void) {
    func_801D3674();
    g_Menu->pManager->unk4A[3] = 0;
    func_801D4EA0(2);
    func_801C7BF4();
    func_801E8044(2, &g_Menu->pManager->unk4A[4]);
    func_801C72BC(0x14);
    HeapFree(g_Menu->unk438);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E1418);

/* objdiff 75.33% (296 bytes, same size; was 68.19% with the old if/else order).
   Recovered shape and the details that are now settled: charId =
   g_Menu->pManager->currentCharacterIDs[slot]; weight = *pWeight++ where pWeight
   = (u16*)((u8*)&g_GameState.characters[charId] + 0x90); value = pRow[i] where
   pRow = (u8*)pTable + charId * 0x110 + index * 0xE and
   pTable = *(u32*)((u8*)g_Menu->unk438 + 0x2578); total/count are u32, the loop
   counter is a signed int (retail ends the loop with slti, not sltiu). The
   weight != 0 test comes FIRST in the source (retail branches over it into the
   `else if (value != 0) count++;` block) -- writing `if (weight == 0)` first
   costs 7 points and 4 bytes.
   Residue: retail recomputes charId * 0x110 INSIDE the loop and keeps
   `li v0,0xffff` in the loop, i.e. loop.c did not hoist either; writing the row
   address as one flat expression inside the loop body does reproduce that
   (no hoisting, li stays) but gcc canonicalises the operand order (it emits the
   index * 0xE chain before the charId chain, retail the reverse) and puts charId
   in a1 instead of a3, which scores 66.44%. Any hoisted row pointer scores
   75.33% because loop.c lifts it into the pre-header. Both 2.6.0 and 2.7.2 emit
   byte-identical code for this function (75.33 either way), so this is not a
   preset question.
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E20C8);
u8 func_801E2250(void) {
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
void func_801E2324(u8 arg0) {
    func_801E8018(6, (u8*)g_Menu + 0x18E0, D_801EA568 + arg0, (u8*)g_Menu->pManager + 0x54);
}
void func_801E2368(void) {
    HeapFree(g_Menu->unk358);
    HeapFree(g_Menu->unk35C);
    HeapFree(g_Menu->unk360);
    func_801C72BC(0x13);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E23CC);
void func_801E2AE0(void) {
    func_801D249C(1);
    g_Menu->unk358 = HeapAlloc(0x2AF0, 0);
    bzero(g_Menu->unk358, 0x2AF0);
    g_Menu->unk35C = HeapAlloc(0x32F4, 0);
    bzero(g_Menu->unk35C, 0x32F4);
    g_Menu->unk360 = HeapAlloc(0x2AC, 0);
    bzero(g_Menu->unk360, 0x2AC);
    func_801C72BC(3);
    func_801D3488(0, 0);
}
void func_801E2B80(void) {
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
        func_801E2368();
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
        func_801D25E4();
        func_801E2B80();
        break;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E31C0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E35BC);

/* objdiff 96.71% (280 bytes, same size; 80.36% was the previous 2.7.2 attempt).
   Structure is complete: pTarget->hp += value and both clamps (which reload the
   stored halfword exactly as retail does) already match byte for byte, as does
   the isGear else-branch. The only residue is register allocation plus load
   scheduling in the first nine instructions: retail loads the slot pointer into
   a0 straight after forming pDressingRoom + charIndex*4 and puts ether in v1
   (mult v1,v0), this compile keeps the pointer in v1, ether in a0. Note for
   whoever retries: the pSlots table is pDressingRoom->unk20[charIndex] and each
   slot record is 0x28 bytes with a u8 at 0x11 -- NOT sizeof(GameCharacter)
   (0xA4); using GameCharacter* gave a 288-byte object and 80.36%. Retried:
   single-statement indexing (93.86%), &pSlots[slotIndex*0x28] vs byte offset
   (96.71 both), swapped multiply operands (93.33%), gcc-2.7.2-cdk (84.13%).
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E3A80);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E3C2C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E3ECC);
void func_801E4170(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    func_801E41C0(pDressingRoom, gearIndex);
    func_801E42AC(pDressingRoom, gearIndex);
    func_801E4258(pDressingRoom, gearIndex);
}

void func_801E41C0(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
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


void func_801E4258(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    GameGear* pGear;
    GearArmor* pArmorList;
    GearArmor* pArmor;

    pGear = &g_GameState.gears[gearIndex];
    pArmorList = pDressingRoom->pGearArmor;
    pArmor = &pArmorList[pGear->armorId];

    pGear->baseDefense = pArmor->unk8;
    pGear->baseEtherDefense = pArmor->unkA;
}

void func_801E42AC(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
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
