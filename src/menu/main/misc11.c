#include "common.h"

#include "system/menu.h"

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E433C);
void func_801E4754(MenuDressingRoom* pDressingRoom, u8 gearId) {
    GameGear* pGear = &g_GameState.gears[gearId];
    GearWeapon* pWeapon;

    pWeapon = pDressingRoom->pGearWeapons;
    pWeapon += pGear->weaponId[0];
    pGear->weaponData[0].unk2 = pWeapon->unkE;
    pGear->weaponData[0].unk0 = pWeapon->unk12;
    pGear->weaponData[0].unk3 = pWeapon->unk10;
    pGear->weaponData[0].unk4 = pWeapon->unk11;

    pGear->unk5C[0] = pWeapon->unk0[0];
    pGear->unk5C[1] = pWeapon->unk0[1];
    pGear->unk5C[2] = pWeapon->unk0[2];
    pGear->unk5C[3] = pWeapon->unk0[3];

    if (pGear->weaponData[0].unk4 == 100) {
        pGear->unk86 = pGear->unk86 & 0xFFF;
        pGear->unk86 |= pGear->weaponData[0].unk0;
    }
    if ((gearId == 5) || (gearId == 0xD)) {
        pWeapon = pDressingRoom->pGearWeapons;
        pWeapon += pGear->unk4[0];

        pGear->weaponData[0].unk2 = pWeapon->unkE;
        pGear->weaponData[0].unk0 = pWeapon->unk12;
        pGear->weaponData[0].unk3 = pWeapon->unk10;
        pGear->weaponData[0].unk4 = pWeapon->unk11;
        pGear->unk5C[1] = pWeapon->unk0[1];

        pWeapon = pDressingRoom->pGearWeapons;
        pWeapon += pGear->unk4[1];
        pGear->weaponData[1].unk2 = pWeapon->unkE;
        pGear->weaponData[1].unk0 = pWeapon->unk12;
        pGear->weaponData[1].unk3 = pWeapon->unk10;
        pGear->weaponData[1].unk4 = pWeapon->unk11;
        pGear->unk5C[2] = pWeapon->unk0[2];

        pWeapon = pDressingRoom->pGearWeapons;
        pWeapon += pGear->unk4[3];
        pGear->weaponData[2].unk2 = pWeapon->unkE;
        pGear->weaponData[2].unk0 = pWeapon->unk12;
        pGear->weaponData[2].unk3 = pWeapon->unk10;
        pGear->weaponData[2].unk4 = pWeapon->unk11;
        pGear->unk5C[3] = pWeapon->unk0[3];
    }
}
