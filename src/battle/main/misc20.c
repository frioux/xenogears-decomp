#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

extern u8 *D_800C3D60;
extern BattleRow *D_800C3DFC;
extern BattleTableEntry *D_800C3E00;
extern BattleTableEntry *D_800C3E34;
extern BattleWorkBlock *D_800C34B0;
extern BattleSlotState *D_800D2DC8;
extern BattleSlotState *D_800D2D6C;
extern u8 D_800C3E50;
extern u8 D_800C3E04;
extern u8 D_800D2DC4;
extern u8 *D_800C3D3C;
extern s32 D_800C34DC[];
extern BattleTableEntry D_800CCCE8[];
extern u16 D_800D2C94;
extern s8 func_8009DBFC();
extern s32 func_80097610();
extern void BattleSetSlotEntryFieldsTo2000(u8);
extern void BattleRollPercentAndSetState38(void);

void BattleSetActorMaskForSlotRecord3(void) {
    u8 i = 0;
    s32 target = 3; /* local, not a literal: the compare constant must live in a
                       register outside the loop */
    u16 *p = &D_800D2C94;

    for (; i < 3; i++) {
        if (D_800CCCE8[i].unk56 == target) {
            *p = 1 << i;
        }
    }
}
void func_8009C198(void) {
    BattleTableEntry *pSlot;
    u16 flags;
    u16 mask;
    u32 actor;
    u8 one;

    if (D_800C3E04 < 3) {
        D_800C3DFC = (BattleRow *)((u8 *)D_800C34B0 +
                                   (D_800C34B0->unk5FC1 * 0x690 + 0x2228) +
                                   D_800C34B0->unk5FC2 * 0x28);
    } else {
        D_800C3DFC = (BattleRow *)((u8 *)D_800C34B0 +
                                   (D_800C34B0->unk5FC2 * 0x28 + 0x35D8));
    }
    BattleSetSlotEntryFieldsTo2000(D_800C3E04);
    actor = D_800C34B0->unk5FC1;
    D_800D2DC4 = 0;
    D_800C3D3C = &D_800C34B0->slots[actor].state.unkA4;
    if (D_800C3DFC->unk0A & 0x100) {
        if (D_800C3E00->unk56 == 1) {
            BattleRollPercentAndSetState38();
        }
    }
    if (((u8 *)D_800C3E00)[0xA0] == 5 || ((u8 *)D_800C3E00)[0xA0] == 0xD) {
        if (D_800C34B0->unk5FC2 == 1) {
            D_800C3DFC->unk22 = ((u8 *)D_800D2D6C)[0x1A];
        }
    }
    mask = 1;
    D_800C3E50 = 0;
    do {
        if (D_800C34B0->unk5FAC & mask) {
            pSlot = &D_800C34B0->slots[D_800C3E50];
            D_800C3E34 = pSlot;
            D_800D2DC8 = &pSlot->state;
            D_800C3D60 = &pSlot->state.unkA4;
            func_8009CA90();
            ((void (*)(void))D_800C34DC[D_800C3DFC->unk16])();
            if (D_800C34B0->unk5FA0[D_800C3E50] == 0) {
                one = 1;
                flags = D_800C3DFC->unk0A;
                if (!(flags & 0x800)) {
                    one = 0;
                    if (!(flags & 0x4000)) {
                        goto skip;
                    }
                }
                func_8009DBFC(one);
            }
        skip:
            D_800C34B0->unk5FB0 = *(u16 *)((u8 *)D_800C3DFC + 2);
            func_8009C4B4();
            BattleSetSlotBit20FromStatusBit200(D_800C3E50);
        }
        D_800C3E50++;
        mask <<= 1;
    } while (D_800C3E50 < 0xB);
    if (D_800C3E00->unk56 == 4) {
        func_8009E788();
    }
    BattleSaveScriptCommandOperands();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009C4B4);
void BattleSaveScriptCommandOperands(void) {
    u16 a = D_800D2D6C->unk84;
    u16 b = D_800C3E00->unk8A;
    u16 c;

    D_800C34B0->unk5FBC = D_800C3DFC->unk20;
    D_800C34B0->unk5FBD = D_800C3DFC->unk21;
    D_800C34B0->unk5FBE = D_800C3DFC->unk22;
    D_800C34B0->unk5FBF = D_800C3DFC->unk23;
    D_800C34B0->unk5FC0 = D_800C34B0->unk5FC2;
    c = (a | b) >> 12;
    if ((D_800C34B0->unk5FBE & 0x3F) == 0) {
        D_800C34B0->unk5FBE |= c;
    }
}
void func_8009CA90(void) {
    if (D_800C34B0->unk5FC2 >= 0xC && D_800C34B0->unk5FC2 < 0xF &&
        !(D_800C34B0->slots[D_800C3E50].state.unkB6 & 0x80)) {
        D_800C3DFC += 3;
    } else if (D_800C34B0->unk5FC2 < 3 &&
               !(D_800C34B0->slots[D_800C3E50].state.unkB6 & 0x80)) {
        D_800C3DFC += 15;
    }
}
void BattleSetSlotBit20FromStatusBit200(u8 arg0) {
    BattleSlotState *pState = &D_800CCCE8[arg0].state;
    BattleTableEntry *pSlot = &D_800CCCE8[arg0];
    u16 v;

    if (pState->unk80 & 0x200) {
        v = pSlot->unk84 | 0x20;
    } else {
        v = pSlot->unk84 & 0xFFDF;
    }
    pSlot->unk84 = v;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009CBC4);
void BattleSetCurrentSlotState6(void) {
    BattleWorkBlock *p;
    u8 i;

    if (func_8009DBFC(0) == 0) {
        p = D_800C34B0;
        i = D_800C3E50;
        p->unk5FA0[i] = 6;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009D3A0);
u16 BattleScaleValueByMultiplierByte(void) {
    u16 ret = func_80096FBC();
    u16 f;
    u32 b;
    s32 n;
    if (D_800C3DFC->unk0A & 0x100) {
        f = D_800D2D6C->unk7C;
        b = D_800D2D6C->unk9E;
        if ((f & 0x100) || b == 0) {
            n = 4;
        } else {
            n = b;
        }
        ret = n * ret / 4;
        if ((D_800D2D6C->unk80 | D_800D2D6C->unk82) & 0x40) {
            ret = ret + ((ret & 0xFFFF) >> 1);
        }
    }
    return ret;
}
u16 func_8009DA04(void) {
    u16 ret = func_80097610();
    u16 v;
    u8 n;

    if (D_800C3DFC->unk0A & 0x100) {
        n = D_800D2DC8->unk99;
        v = *(u16 *)&D_800D2DC8->unk68[0xA];
        if (n != 0 && v != 0) {
            v = v * (100 - n) / 100;
        }
        if (D_800C34B0->slots[D_800C3E50].state.unkB6 & 0x80) {
            ret = ret + v;
        }
        if ((D_800D2DC8->unk80 | D_800D2DC8->unk82) & 0x20) {
            ret = ret + (ret >> 1);
        }
    } else {
        n = D_800D2DC8->unk99;
        if (n != 0) {
            ret = ret * (100 - n) / 100;
        }
    }
    return ret;
}
s32 func_8009DB54(s32 arg0) {
    u16 mask = *(u16 *)((u8 *)D_800C3DFC + 8);
    u8 i;
    u8 idx; /* left unassigned when the scan finds no bit, as in retail */
    u8 v;

    for (i = 0; i < 0x10; i++) {
        if (mask & (0x8000 >> i)) {
            idx = i;
            break;
        }
    }
    v = D_800D2DC8->unk86[idx + 2];
    if (v != 0) {
        if (v < 0x14) {
            arg0 = arg0 * (0x14 - v) / 20;
        } else {
            arg0 = 0;
        }
    }
    return arg0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009DBFC);
void BattleClearActorPercent99(void) {
    D_800D2DC8->unk99 = 0;
}
void BattleSetSlotState2FromActorField4F(void) {
    BattleSlotState *p = D_800D2DC8;
    u32 v = p->unk4F * p->unk64;
    u32 q = v / 10;

    D_800C34B0->unk5FA0[D_800C3E50] = 2;
    D_800C34B0->unk5F6C[D_800C3E50] = q;
}
void BattleSetSlotState2FromScriptField11Div20(void) {
    u32 v = D_800C3DFC->unk11 * D_800D2DC8->unk64;
    u32 q = v / 20;

    D_800C34B0->unk5FA0[D_800C3E50] = 2;
    D_800C34B0->unk5F6C[D_800C3E50] = q;
}
void BattleSetSlotState2FromActorField5B(void) {
    s32 v = D_800C3E00->unk5B * D_800C3DFC->unk11;

    D_800C34B0->unk5FA0[D_800C3E50] = 2;
    D_800C34B0->unk5F6C[D_800C3E50] = v;
}
void BattleSetSlotFields148And10C(void) {
    D_800C3D60[0] = 4;
    D_800CCCE8[D_800C3E50].state.unkB7[7] = 3;
}
void BattleSetSlotState0AFromActorWord3ADiv20(void) {
    s32 v = D_800D2DC8->unk3A * D_800C3DFC->unk11;
    s32 q = v / 20;

    D_800C34B0->unk5FA0[D_800C3E50] = 0xA;
    D_800C34B0->unk5F6C[D_800C3E50] = q;
}
void BattleSetSlotState0BFromActorWord3ADiv20(void) {
    s32 v = D_800D2DC8->unk3A * D_800C3DFC->unk11;
    s32 q = v / 20;

    D_800C34B0->unk5FA0[D_800C3E50] = 0xB;
    D_800C34B0->unk5F6C[D_800C3E50] = q;
}
void BattleClearSlotConditionBits(void) {
    BattleSlotState *p = D_800D2DC8;
    BattleTableEntry *q = D_800C3E34;

    p->unk7C &= 0xF80B;
    q->unk7A &= 0xFFDF;
}
s32 BattleGearHasWeaponId(u8 arg0) {
    BattleGearWeaponRecord *pRec = &D_800C34B0->gearWeaponIds[arg0];
    u8 v = pRec->unkF;
    u8 id = g_GameState.characters[4].gearId;

    if (g_GameState.gears[id].weaponData[0].unk5 == v ||
        g_GameState.gears[id].weaponData[1].unk5 == v) {
        return 1;
    }
    return g_GameState.gears[id].weaponData[2].unk5 == v;
}
void BattleApplyGearWeaponRecord(u8 arg0) {
    BattleGearWeaponRecord *p = &D_800C34B0->gearWeapons[arg0];
    u8 v = p->unkF;
    s32 rowOffset;
    u8 id = g_GameState.characters[4].gearId;
    u8 sel;
    u8 i;

    if (g_GameState.gears[id].weaponData[0].unk5 == v) {
        sel = 0;
    }
    if (g_GameState.gears[id].weaponData[1].unk5 == v) {
        sel = 1;
    }
    if (g_GameState.gears[id].weaponData[2].unk5 == v) {
        sel = 3;
    }
    g_GameState.gears[id].weaponData[sel].unk2 = p->unkE;
    g_GameState.gears[id].weaponData[sel].unk4 = p->unk11;
    g_GameState.gears[id].weaponData[sel].unk3 = p->unk10;
    g_GameState.gears[id].weaponData[sel].unk4 = p->unk11;
    g_GameState.gears[id].unk4[sel] = arg0;
    g_GameState.gearPartsIds[0x6C + arg0] = p->unkC;
    for (i = 0; i < 3; i++) {
        rowOffset = i * 0x170;
        if (((u8 *)D_800C34B0)[rowOffset + 0x56] == 4) {
            *(u8 *)((u8 *)D_800C34B0 + (rowOffset + sel * 8) + 0xB6) = p->unkE;
            *(u8 *)((u8 *)D_800C34B0 + (rowOffset + sel * 8) + 0xB8) = p->unk11;
            *(u8 *)((u8 *)D_800C34B0 + (rowOffset + sel * 8) + 0xB7) = p->unk10;
            *(u8 *)((u8 *)D_800C34B0 + (rowOffset + sel * 8) + 0xB8) = p->unk11;
            *(u8 *)((u8 *)D_800C34B0 + rowOffset + sel + 0xA8) = arg0;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E788);
void func_8009E868(u8 arg0, u16 arg1) {
    switch (arg0) {
    case 0:
        switch (arg1) {
        case 0x400:
            D_800C34B0->unk5FC7 = 0x24;
            break;
        case 0x200:
            D_800C34B0->unk5FC7 = 0x25;
            break;
        case 0x100:
            D_800C34B0->unk5FC7 = 0x26;
            break;
        case 0x80:
            D_800C34B0->unk5FC7 = 0x27;
            break;
        case 0x40:
            D_800C34B0->unk5FC7 = 0x28;
            break;
        case 0x20:
            D_800C34B0->unk5FC7 = 0x29;
            break;
        case 0x10:
            D_800C34B0->unk5FC7 = 0x2A;
            break;
        case 4:
            D_800C34B0->unk5FC7 = 0x2B;
            break;
        }
        break;
    case 1:
        switch (arg1) {
        case 0x1000:
            D_800C34B0->unk5FC7 = 0x2D;
            break;
        case 0x800:
            D_800C34B0->unk5FC7 = 0x2E;
            break;
        case 0x400:
            D_800C34B0->unk5FC7 = 0x2F;
            break;
        case 0x40:
            D_800C34B0->unk5FC7 = 0x15;
            break;
        case 0x20:
            D_800C34B0->unk5FC7 = 0x16;
            break;
        case 8:
        case 2:
            D_800C34B0->unk5FC7 = 0x19;
            break;
        case 4:
        case 1:
            D_800C34B0->unk5FC7 = 0x1A;
            break;
        }
        break;
    case 3:
        switch (arg1) {
        case 0x8000:
            D_800C34B0->unk5FC7 = 0x1B;
            break;
        case 0x4000:
            D_800C34B0->unk5FC7 = 0x1C;
            break;
        case 0x2000:
            D_800C34B0->unk5FC7 = 0x1D;
            break;
        case 0x1000:
            D_800C34B0->unk5FC7 = 0x1E;
            break;
        case 0x400:
            D_800C34B0->unk5FC7 = 0x1F;
            break;
        case 0x800:
            D_800C34B0->unk5FC7 = 0x20;
            break;
        case 0x100:
            D_800C34B0->unk5FC7 = 0x21;
            break;
        case 0x200:
            D_800C34B0->unk5FC7 = 0x22;
            break;
        }
        break;
    }
}
