#include "common.h"

#include "system/menu.h"

extern u8 D_801E9808[];
extern u8 MenuGetGearWeightClassExcess(u8 gearId);

void MenuApplyGearAccessories(MenuDressingRoom* pDressingRoom, u8 gearId) {
    GameGear* pGear;
    GearAccessory* pAccessory;
    u8 accessoryInd;
    u8 i;
    u8 characterId;
    u16* pFlags1;
    u16* pFlags2;

    pGear = &g_GameState.gears[gearId];
    characterId = D_801E9808[gearId];

    pFlags1 = &g_GameState.unk16BC[characterId].unk8;
    pFlags2 = &g_GameState.unk16BC[characterId].statusFlags;

    pGear->defenseFromEquipment = 0;
    pGear->etherDefenseFromEquipment = 0;
    pGear->weightFromEquipment = 0;
    pGear->accessoryEffectFlags9 = 0;
    pGear->unk4C = 0;
    pGear->responsiveness = 0;
    pGear->unk4E = 0;
    pGear->accessoryEffectFlags5 = 0;
    pGear->accessoryEffectFlags4 = 0;
    pGear->etherAmplificationFromEquipment = 0;

    for (i = 0; i < 16; i++) {
        pGear->accessoryEffectAmounts[i] = 0;
    }

    for (i = 0; i < 4; i++) {
        pGear->unk50[i] = 0;
    }

    for (i = 0; i < 3; i++) {
        pGear->unk55[i] = 0;
    }

    pGear->accessoryEffectFlags1 = 0;
    pGear->accessoryEffectFlags2 = 0;
    pGear->accessoryEffectFlags3 &= 0xF000;
    *pFlags1 &= 0xFB6F;

    for (accessoryInd = 0; accessoryInd < 3; accessoryInd++) {
        pAccessory = pDressingRoom->pGearAccessories;
        pAccessory += pGear->accessorySlots[accessoryInd];

        pGear->defenseFromEquipment += pAccessory->defense;
        pGear->etherDefenseFromEquipment += pAccessory->etherDefense;
        pGear->weightFromEquipment += pAccessory->weight;
        pGear->unk4C += pAccessory->unk18;
        pGear->responsiveness += pAccessory->responsiveness;
        pGear->etherAmplificationFromEquipment += pAccessory->unk1B;

        for (i = 0; i < 4; i++) {
            pGear->unk50[i] += pAccessory->unk10[i];
        }

        switch (pAccessory->effectType) {
            case 1:
                pGear->accessoryEffectFlags1 |= pAccessory->effectFlags;
                break;
            case 2:
                pGear->accessoryEffectFlags2 |= pAccessory->effectFlags;
                break;
            case 3:
                pGear->accessoryEffectFlags3 |= pAccessory->effectFlags;
                break;
            case 4:
                pGear->accessoryEffectFlags4 |= pAccessory->effectFlags;
                for (i = 0; i < 16; i++) {
                    if (pAccessory->effectFlags & (0x8000 >> i)) {
                        pGear->accessoryEffectAmounts[i] += pAccessory->effectAmount;
                    }
                }
                break;
            case 5:
                pGear->accessoryEffectFlags5 += pAccessory->effectFlags;
                break;
            case 6:
                if ((*pFlags1 & 0x1000) && (*pFlags1 & 0x800)) {
                    *pFlags1 |= 0x400;
                }
                break;
            case 7:
                if ((*pFlags1 & 0x200) && (*pFlags1 & 0x100)) {
                    *pFlags1 |= 0x80;
                }
                break;
            case 8:
                if ((*pFlags1 & 0x40) && (*pFlags1 & 0x20)) {
                    *pFlags1 |= 0x10;
                }
                break;
            case 9:
                pGear->accessoryEffectFlags9 |= pAccessory->effectFlags;
            case 10:
                pGear->unk55[1] += (u8)pAccessory->effectFlags;
                break;
            case 11:
                pGear->unk55[2] += (u8)pAccessory->effectFlags;
                break;
        }
    }

    pGear->weightPenalty = MenuGetGearWeightClassExcess(gearId);
    if (pGear->accessoryEffectFlags5) {
        *pFlags2 |= 0x8000;
    } else if (gearId == g_GameState.characters[D_801E9808[gearId]].gearId) {
        *pFlags2 &= 0x7FFF;
    }
}
void MenuApplyGearWeapons(MenuDressingRoom* pDressingRoom, u8 gearId) {
    GameGear* pGear = &g_GameState.gears[gearId];
    GearWeapon* pWeapon;

    pWeapon = pDressingRoom->pGearWeapons;
    pWeapon += pGear->weaponId[0];
    pGear->weaponData[0].attack = pWeapon->attack;
    pGear->weaponData[0].flags = pWeapon->flags;
    pGear->weaponData[0].unk3 = pWeapon->unk10;
    pGear->weaponData[0].unk4 = pWeapon->unk11;

    pGear->unk5C[0] = pWeapon->unk0[0];
    pGear->unk5C[1] = pWeapon->unk0[1];
    pGear->unk5C[2] = pWeapon->unk0[2];
    pGear->unk5C[3] = pWeapon->unk0[3];

    if (pGear->weaponData[0].unk4 == 100) {
        pGear->accessoryEffectFlags3 = pGear->accessoryEffectFlags3 & 0xFFF;
        pGear->accessoryEffectFlags3 |= pGear->weaponData[0].flags;
    }
    if ((gearId == 5) || (gearId == 0xD)) {
        pWeapon = pDressingRoom->pGearWeapons;
        pWeapon += pGear->extraWeaponIds[0];

        pGear->weaponData[0].attack = pWeapon->attack;
        pGear->weaponData[0].flags = pWeapon->flags;
        pGear->weaponData[0].unk3 = pWeapon->unk10;
        pGear->weaponData[0].unk4 = pWeapon->unk11;
        pGear->unk5C[1] = pWeapon->unk0[1];

        pWeapon = pDressingRoom->pGearWeapons;
        pWeapon += pGear->extraWeaponIds[1];
        pGear->weaponData[1].attack = pWeapon->attack;
        pGear->weaponData[1].flags = pWeapon->flags;
        pGear->weaponData[1].unk3 = pWeapon->unk10;
        pGear->weaponData[1].unk4 = pWeapon->unk11;
        pGear->unk5C[2] = pWeapon->unk0[2];

        pWeapon = pDressingRoom->pGearWeapons;
        pWeapon += pGear->extraWeaponIds[3];
        pGear->weaponData[2].attack = pWeapon->attack;
        pGear->weaponData[2].flags = pWeapon->flags;
        pGear->weaponData[2].unk3 = pWeapon->unk10;
        pGear->weaponData[2].unk4 = pWeapon->unk11;
        pGear->unk5C[3] = pWeapon->unk0[3];
    }
}
