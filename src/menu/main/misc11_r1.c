#include "common.h"

#include "system/menu.h"


u8 func_801E4928(u8 gearId) {
    GameGear* pGear;
    u_short weightClass;
    s16 value;

    pGear = &g_GameState.gears[gearId];
    weightClass = pGear->weightFromEquipment / 120;
    value = (weightClass - pGear->unk75) / 2;
    if (value < 0) {
        value = 0;
    }
    return value;
}
void func_801E4998(MenuDressingRoom* pDressingRoom, u8 gearId) {
    u32 p;
    u32 hp;
    u16 value;

    hp = g_GameState.gears[gearId].maxHp / 10 * 2 / 9;
    p = *(u32*)((u8*)pDressingRoom + 0x4C + gearId * 4);
    p = p + 0x5C8;
    *(volatile u16*)(p + 0x24) = hp;
    value = *(volatile u16*)(p + 0x24) / 10;
    *(u16*)(p + 0x24) = value * 10;
}
