#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

extern BattleWorkBlock* g_BattleCurWorkBlock;
extern u8 g_BattleCurSlotIndex;
extern BattleRow* g_BattleCurrentRow;
extern u8* g_BattleCurSlotState;
extern u8 g_BattleSlotConditions[][0x170];
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleActorCharIds[];

extern int rand(void);

void BattleRollActorActionCondition(u8 arg0, u8* pInfo, s16* pStatus) {
    BattleTableEntry* p = &g_BattleWorkBlock[arg0];
    u16 mask;
    u8 count;
    u8 bit;

    if (g_BattleSlotConditions[arg0][0] & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        mask = 1;
    } else {
        if (rand() % BATTLE_PERCENT_ROLL_RANGE < 10 && !(p->conditionFlags & 0x100) && *pStatus == 0) {
            *pInfo = 4;
            return;
        }
        switch (p->characterId) {
        case 3:
            mask = 0xC3C0;
            count = 0xA;
            break;
        case 4:
            mask = 0xDF80;
            count = 10;
            break;
        case 5:
            mask = 0x1000;
            count = 4;
            break;
        case 7:
            mask = 0xE000;
            count = 3;
            break;
        case 0:
        case 8:
            mask = 0xC000;
            count = 2;
            break;
        case 2:
        case 9:
            mask = 0xBFE0;
            count = 11;
            break;
        case 1:
        case 6:
        case 10:
            mask = 0xF000;
            count = 4;
            break;
        }
        if (rand() % BATTLE_PERCENT_ROLL_RANGE < 25 && !(p->conditionFlags & 0x20)) {
            *pInfo = 2;
            bit = rand() % count;
            if (g_GameState.conditionRows[p->characterId].characterConditionMask & ((0x8000 >> bit) & mask)) {
                pInfo[1] = bit;
                return;
            }
        }
        if (p->characterId == 8) {
            *pInfo = 0;
            pInfo[1] = 0;
            return;
        }
        mask = 1;
    }
    *pInfo = mask;
    if (rand() % BATTLE_PERCENT_ROLL_RANGE < 80) {
        if (rand() % BATTLE_PERCENT_ROLL_RANGE >= 60) {
            pInfo[1] = mask;
        } else {
            pInfo[1] = 0;
        }
    } else {
        pInfo[1] = 2;
    }
}
void BattleSetSlotActionStateAndAmount(void) {
    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] =
        (u32)(g_BattleCurrentRow->power * ((BattleSlotState*)g_BattleCurSlotState)->maxHp) / 20;
}
void func_8009BE0C(void) {
    BattleTableEntry* p;
    BattleSlotState* pState;
    GameCharacter* pChar;
    GameGear* pGear;
    s32 gearId;
    u32 frame[2];
    u8 i;
    u8 j;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_BattleActorCharIds[i] == BATTLE_SLOT_CHARACTER_NONE) {
            continue;
        }
        p = &g_BattleWorkBlock[i];
        pState = (BattleSlotState*)((u8*)&g_BattleWorkBlock[i] + 0xA4);
        pChar = &g_GameState.characters[p->characterId];
        pGear = &g_GameState.gears[p->gearId];
        if (p->characterId == 7 && (g_BattleSlotConditions[i][0] & BATTLE_SLOT_GEAR_STATUS_FLAG)) {
            *(volatile u16*)&p->hp = (pState->hp + 1) / BATTLE_ACTOR_HP_SCALE;
            if (p->hp == 0) {
                p->hp = 1;
            }
        }
        pChar->hp = p->hp;
        pChar->mp = p->mp;
        if (pChar->hp > pChar->maxHp) {
            pChar->hp = pChar->maxHp;
        }
        if (pChar->mp > pChar->maxMp) {
            pChar->mp = pChar->maxMp;
        }
        for (j = 0; j < 7; j++) {
            pChar->statWeights[j] = p->statWeights[j];
        }
        pChar->actionCount = p->actionCount;
        if (p->activeEffectFlags1 & 0xC000) {
            pChar->hp = 1;
        }
        gearId = p->gearId;
        if (gearId < 0) {
            continue;
        }
        if (gearId >= 7) {
            if (gearId >= 0x11) {
                continue;
            }
            if (gearId <= 7) {
                continue;
            }
        }
        {
            *(volatile u32*)&pGear->hp = pState->hp;
            pGear->fuel = pState->fuel;
            if (pGear->hp > pGear->maxHp) {
                pGear->hp = pGear->maxHp;
            }
            if (pGear->fuel > pGear->maxFuel) {
                pGear->fuel = pGear->maxFuel;
            }
            if (pState->activeEffectFlags1 & 0x8000) {
                pGear->hp = pGear->maxHp / 10;
            }
        }
    }
}

// The state base (+0xA4) and the condition byte inside it (+0xA4 again) are formed
// from the slot table base before the slot stride is added, as retail does; the
// typed `&g_BattleWorkBlock[arg0].state` / `.state.unkA4` spellings derive the
// second one from the first and lose two registers (94.69%).
u8 BattleGetActorActionConditionMask(u8 arg0) {
    BattleTableEntry* pEntry = &g_BattleWorkBlock[arg0];
    BattleSlotState* pState = (BattleSlotState*)((u8*)&g_BattleWorkBlock[arg0] + 0xA4);
    u8* pCondition = (u8*)&g_BattleWorkBlock[arg0] + 0x148;
    s32 flag = pState->activeEffectFlags1 & 0x400;
    u8 ret = flag != 0;

    if (pState->hp < ((u32)pState->maxHp >> 3)) {
        if (!(pEntry->unk36 & 1)) {
            ret |= BATTLE_ACTION_CONDITION_LOW_HP;
        }
    }
    if (*pCondition == 4) {
        ret |= 4;
    }
    return ret;
}
void BattleSetSlotFields148And162(u8 arg0) {
    g_BattleWorkBlock[arg0].state.commandCounter = 4;
    g_BattleWorkBlock[arg0].state.conditionTimers[7] = 3;
    g_GameState.conditionRows[0].statusFlags |= 0x4000;
}
