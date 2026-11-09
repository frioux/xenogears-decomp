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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E31C0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E35BC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E36D4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E3A80);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E3C2C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E3ECC);
void func_801E4170(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    func_801E41C0(pDressingRoom, gearIndex);
    func_801E42AC(pDressingRoom, gearIndex);
    func_801E4258(pDressingRoom, gearIndex);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E41C0);

/* 93.95% - instruction multiset identical up to the frameId load being scaled
   before pGearFrames is loaded; gcc picks a1 for pGear instead of a2.
void func_801E41C0(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    GameGear* pGear = &g_GameState.gears[gearIndex];
    GearFrame* pFrame = &pDressingRoom->pGearFrames[pGear->frameId];

    pGear->hp = pFrame->hp;
    pGear->maxHp = pFrame->hp;
    pGear->agility = pFrame->unk14;
    pGear->etherAmplification = pFrame->unk15;
    pGear->unk9D = pFrame->unk16;
    pGear->unk9F = pFrame->unk17;
    if (pGear->hp > pGear->maxHp) {
        pGear->hp = pGear->maxHp;
    }
}
*/

void func_801E4258(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    GameGear* pGear = &g_GameState.gears[gearIndex];
    GearArmor* pArmor = &pDressingRoom->pGearArmor[pGear->armorId];

    pGear->baseDefense = pArmor->unk8;
    pGear->baseEtherDefense = pArmor->unkA;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc10", func_801E42AC);

/* 89.56% - only the fuel clamp's allocator differs: retail keeps the old fuel
   in a0 from an early load and re-reads maxFuel for the store, gcc reuses the
   register it already has.  Everything else in the function matches.
void func_801E42AC(MenuDressingRoom* pDressingRoom, u8 gearIndex) {
    GameGear* pGear = &g_GameState.gears[gearIndex];
    GearEngine* pEngine = &pDressingRoom->pGearEngines[pGear->engineId];

    pGear->maxFuel = pEngine->maxFuel;
    pGear->engineOutput = pEngine->powerOutput;
    pGear->unk3D = pEngine->unkD;
    pGear->unk3E = pEngine->unkE;
    pGear->unk3F = pEngine->unkE;
    if (pGear->fuel > pGear->maxFuel) {
        pGear->fuel = pGear->maxFuel;
    }
}
*/
