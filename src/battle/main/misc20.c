#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

extern u8 *D_800C3D60;
extern BattleRow *g_BattleCurrentRow;
extern BattleTableEntry *g_BattleActingSlot;
extern BattleTableEntry *g_BattleTargetSlotEntry;
extern BattleWorkBlock *g_BattleCurWorkBlock;
extern BattleSlotState *g_BattleCurSlotState;
extern BattleSlotState *g_BattleTargetSlotState;
extern u8 g_BattleCurSlotIndex;
extern u8 g_BattleTargetSlotIndex;
extern u8 g_BattleAmountThirdFlag;
extern u8 *D_800C3D3C;
extern s32 D_800C34DC[];
extern BattleTableEntry g_BattleWorkBlock[];
extern u16 g_BattleActionSlotMask;
extern s8 func_8009DBFC();
extern s32 func_80097610();
extern void BattleSetSlotEntryFieldsTo2000(u8);
extern void BattleRollPercentAndSetState38(void);

void BattleSetActorMaskForSlotRecord3(void) {
    u8 i = 0;
    s32 target = 3; /* local, not a literal: the compare constant must live in a
                       register outside the loop */
    u16 *p = &g_BattleActionSlotMask;

    for (; i < 3; i++) {
        if (g_BattleWorkBlock[i].characterId == target) {
            *p = 1 << i;
        }
    }
}
void BattleExecuteActionRow(void) {
    BattleTableEntry *pSlot;
    u16 flags;
    u16 mask;
    u32 actor;
    u8 one;

    if (g_BattleTargetSlotIndex < 3) {
        g_BattleCurrentRow = (BattleRow *)((u8 *)g_BattleCurWorkBlock +
                                   (g_BattleCurWorkBlock->actorIndex * 0x690 + 0x2228) +
                                   g_BattleCurWorkBlock->rowIndex * 0x28);
    } else {
        g_BattleCurrentRow = (BattleRow *)((u8 *)g_BattleCurWorkBlock +
                                   (g_BattleCurWorkBlock->rowIndex * 0x28 + 0x35D8));
    }
    BattleSetSlotEntryFieldsTo2000(g_BattleTargetSlotIndex);
    actor = g_BattleCurWorkBlock->actorIndex;
    g_BattleAmountThirdFlag = 0;
    D_800C3D3C = &g_BattleCurWorkBlock->slots[actor].state.unkA4;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        if (g_BattleActingSlot->characterId == 1) {
            BattleRollPercentAndSetState38();
        }
    }
    if (g_BattleActingSlot->gearId == 5 || g_BattleActingSlot->gearId == 0xD) {
        if (g_BattleCurWorkBlock->rowIndex == 1) {
            g_BattleCurrentRow->element = g_BattleTargetSlotState->weaponData[1].attack;
        }
    }
    mask = 1;
    g_BattleCurSlotIndex = 0;
    do {
        if (g_BattleCurWorkBlock->activeSlotMask & mask) {
            pSlot = &g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex];
            g_BattleTargetSlotEntry = pSlot;
            g_BattleCurSlotState = &pSlot->state;
            D_800C3D60 = &pSlot->state.unkA4;
            BattleAdvanceActionRow();
            ((void (*)(void))D_800C34DC[g_BattleCurrentRow->opcode])();
            if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 0) {
                one = 1;
                flags = g_BattleCurrentRow->actionFlags;
                if (!(flags & 0x800)) {
                    one = 0;
                    if (!(flags & 0x4000)) {
                        goto skip;
                    }
                }
                func_8009DBFC(one);
            }
        skip:
            g_BattleCurWorkBlock->unk5FB0 = g_BattleCurrentRow->unk02;
            func_8009C4B4();
            BattleSetSlotBit20FromStatusBit200(g_BattleCurSlotIndex);
        }
        g_BattleCurSlotIndex++;
        mask <<= 1;
    } while (g_BattleCurSlotIndex < 0xB);
    if (g_BattleActingSlot->characterId == 4) {
        func_8009E788();
    }
    BattleSaveScriptCommandOperands();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009C4B4);
void BattleSaveScriptCommandOperands(void) {
    u16 a = g_BattleTargetSlotState->activeEffectFlags3;
    u16 b = g_BattleActingSlot->accessoryEffectFlags4;
    u16 c;

    g_BattleCurWorkBlock->unk5FBC = g_BattleCurrentRow->unk20;
    g_BattleCurWorkBlock->unk5FBD = g_BattleCurrentRow->unk21;
    g_BattleCurWorkBlock->element = g_BattleCurrentRow->element;
    g_BattleCurWorkBlock->unk5FBF = g_BattleCurrentRow->unk23;
    g_BattleCurWorkBlock->unk5FC0 = g_BattleCurWorkBlock->rowIndex;
    c = (a | b) >> 12;
    if ((g_BattleCurWorkBlock->element & BATTLE_ELEMENT_MASK) == 0) {
        g_BattleCurWorkBlock->element |= c;
    }
}
void BattleAdvanceActionRow(void) {
    if (g_BattleCurWorkBlock->rowIndex >= 0xC && g_BattleCurWorkBlock->rowIndex < 0xF &&
        !(g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 0x80)) {
        g_BattleCurrentRow += 3;
    } else if (g_BattleCurWorkBlock->rowIndex < 3 &&
               !(g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 0x80)) {
        g_BattleCurrentRow += 15;
    }
}
void BattleSetSlotBit20FromStatusBit200(u8 arg0) {
    BattleSlotState *pState = &g_BattleWorkBlock[arg0].state;
    BattleTableEntry *pSlot = &g_BattleWorkBlock[arg0];
    u16 v;

    if (pState->activeEffectFlags2 & 0x200) {
        v = pSlot->activeEffectFlags3 | 0x20;
    } else {
        v = pSlot->activeEffectFlags3 & 0xFFDF;
    }
    pSlot->activeEffectFlags3 = v;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009CBC4);
void BattleSetCurrentSlotState6(void) {
    BattleWorkBlock *p;
    u8 i;

    if (func_8009DBFC(0) == 0) {
        p = g_BattleCurWorkBlock;
        i = g_BattleCurSlotIndex;
        p->slotStates[i] = 6;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009D3A0);
// best 91.80%: the whole branch tree, the six rand()%100 blocks, the frame and the
// register assignment all line up; what is left is eight single instructions.
//   - evade = mod + evade and hit = mod/2 + hit: retail keeps the sum's
//     destination as the accumulator (addu a2,v0,a2), gcc 2.6 emits a2,a2,v0 in
//     both operand orders.
//   - the two `(s16)x / 2` sites: retail's last shift is srl, ours sra.
//   - (s8) of the u8 BattleRow.unk15: retail keeps lbu+sll/sra, gcc folds the
//     cast into an lb even through a u8 local + (s8) at the use.
//   - gcc cross-jumps the two `if (unk84|unk86 & 0x20)` / `& 0x40` blocks
//     (identical rand()%100-chance-%50 tails): ours threads the first block into
//     the second's body with a `j`, retail duplicates it.
//   - limit = limit + 0x55 - penalty is reassociated to limit - (penalty - 0x55).
//   - a 4-minute permuter run on this body (tools/scripts/permute.sh, -j 2) scored
//     base=4514 best=1813 and its only finding was wrapping the 0x40 block in
//     `do { ... } while (0);`. That does NOT break the cross-jump above (the
//     `j` into the other block's body survives) and costs objdiff 91.80 -> 91.67
//     because chance/limit swap s0/s1, so it was reverted. A longer run from this
//     body is the next step, not more hand shapes.
// Next step is a permuter run on this body, not more hand shapes.
/*
s32 func_8009D3A0(void) {
    s32 penalty = 0;
    s16 limit = 0;
    s32 hit;
    s32 evade;
    s32 chance;
    s16 roll;
    u8 rowMod;
    u8 isGear;

    if ((g_BattleCurrentRow->actionFlags & 0x200) && (*(u16 *)((u8 *)g_BattleTargetSlotEntry + 0x34) & 8)) {
        return 3;
    }
    if (g_BattleCurrentRow->actionFlags & 0x1000) {
        return 3;
    }
    if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x100) {
        return 3;
    }
    if (g_BattleTargetSlotEntry->activeEffectFlags1 & 0x2000) {
        return 1;
    }
    if (g_BattleTargetSlotEntry->activeEffectFlags2 & 0x1000) {
        return 1;
    }
    if (g_BattleCurrentRow->actionFlags & 0x8000) {
        return 1;
    }
    hit = g_BattleTargetSlotEntry->hit;
    isGear = ((u8 *)&g_GameState.unk22B6)[g_BattleTargetSlotState->unk04[0]];
    evade = g_BattleActingSlot->evade;
    if (!isGear) {
        evade = g_BattleTargetSlotState->responsiveness + evade;
    }
    if (g_BattleCurSlotState->responsiveness != 0) {
        hit = g_BattleCurSlotState->responsiveness / 2 + hit;
    }
    if (g_BattleActingSlot->characterId == 4) {
        if ((g_BattleCurrentRow->unk0C[4] & 0x80) && !isGear) {
            return 3;
        }
        if ((g_BattleCurrentRow->unk0C[4] & 0x20) &&
            ((u8 *)&g_GameState.unk22B6)[g_BattleTargetSlotState->unk04[3]] == 0) {
            return 3;
        }
    }
    if ((g_BattleTargetSlotState->unk80 | g_BattleTargetSlotState->unk82) & 0x800) {
        evade += (s16)evade / 2;
    }
    if (g_BattleTargetSlotState->unk7C & 0x10) {
        penalty = 0x3C;
    }
    if (g_BattleCurrentRow->actionFlags & 0x1000) {
        return 3;
    }
    if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 0x80) {
        if ((g_BattleCurSlotState->unk80 | g_BattleCurSlotState->unk82) & 0x400) {
            hit += (s16)hit / 2;
        }
        if (g_BattleCurSlotState->unk7C & 0x10) {
            limit = 0x1E;
        }
        if (g_BattleCurSlotState->unk7C & 0xC00) {
            return 1;
        }
    } else {
        evade = (s16)evade / 2;
        if (g_BattleTargetSlotEntry->activeEffectFlags1 & 0x2000) {
            return 1;
        }
        if (g_BattleTargetSlotEntry->activeEffectFlags2 & 0x1000) {
            return 1;
        }
        if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x800) {
            hit += (s16)hit / 2;
        }
    }
    if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 1) {
        if (rand() % 100 < 0x5F) {
            return 2;
        }
        return 1;
    }
    rowMod = g_BattleCurrentRow->evadeModifier;
    chance = evade + (s8)rowMod - hit;
    if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x20) {
        roll = rand() % 100 - chance;
        if (roll < 50) {
            return 1;
        }
        return 3;
    }
    if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x40) {
        roll = rand() % 100 - chance;
        if (roll < 50) {
            return 1;
        }
        return 2;
    }
    roll = rand() % 100 - chance;
    limit = limit + 0x55 - penalty;
    if (roll >= limit) {
        return 3;
    }
    roll = rand() % 100 - chance;
    if (roll < limit) {
        return 1;
    }
    return 2;
}
*/
u16 BattleScaleValueByMultiplierByte(void) {
    u16 ret = func_80096FBC();
    u16 f;
    u32 b;
    s32 n;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        f = g_BattleTargetSlotState->activeEffectFlags1;
        b = g_BattleTargetSlotState->unk9E;
        if ((f & 0x100) || b == 0) {
            n = 4;
        } else {
            n = b;
        }
        ret = n * ret / 4;
        if ((g_BattleTargetSlotState->activeEffectFlags2 | g_BattleTargetSlotState->accessoryEffectFlags2) & 0x40) {
            ret = ret + ((ret & 0xFFFF) >> 1);
        }
    }
    return ret;
}
u16 BattleApplySlotPercentToValue(void) {
    u16 ret = func_80097610();
    u16 v;
    u8 n;

    if (g_BattleCurrentRow->actionFlags & 0x100) {
        n = g_BattleCurSlotState->penaltyPercent;
        v = g_BattleCurSlotState->valueBonus;
        if (n != 0 && v != 0) {
            v = v * (100 - n) / 100;
        }
        if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 0x80) {
            ret = ret + v;
        }
        if ((g_BattleCurSlotState->activeEffectFlags2 | g_BattleCurSlotState->accessoryEffectFlags2) & 0x20) {
            ret = ret + (ret >> 1);
        }
    } else {
        n = g_BattleCurSlotState->penaltyPercent;
        if (n != 0) {
            ret = ret * (100 - n) / 100;
        }
    }
    return ret;
}
s32 BattleScaleValueByRowBitPercent(s32 arg0) {
    u16 mask = g_BattleCurrentRow->statusMask;
    u8 i;
    u8 idx; /* left unassigned when the scan finds no bit, as in retail */
    u8 v;

    for (i = 0; i < 0x10; i++) {
        if (mask & (0x8000 >> i)) {
            idx = i;
            break;
        }
    }
    v = g_BattleCurSlotState->statusPenaltyPercent[idx];
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
    g_BattleCurSlotState->penaltyPercent = 0;
}
void BattleSetSlotState2FromActorField4F(void) {
    BattleSlotState *p = g_BattleCurSlotState;
    u32 v = p->unk4F * p->maxHp;
    u32 q = v / 10;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = q;
}
void BattleSetSlotState2FromScriptField11Div20(void) {
    u32 v = g_BattleCurrentRow->power * g_BattleCurSlotState->maxHp;
    u32 q = v / 20;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = q;
}
void BattleSetSlotState2FromActorField5B(void) {
    s32 v = g_BattleActingSlot->ether * g_BattleCurrentRow->power;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = v;
}
void BattleSetSlotFields148And10C(void) {
    D_800C3D60[0] = 4;
    g_BattleWorkBlock[g_BattleCurSlotIndex].state.conditionTimers[7] = 3;
}
void BattleSetSlotState0AFromActorWord3ADiv20(void) {
    s32 v = g_BattleCurSlotState->unk3A * g_BattleCurrentRow->power;
    s32 q = v / 20;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0xA;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = q;
}
void BattleSetSlotState0BFromActorWord3ADiv20(void) {
    s32 v = g_BattleCurSlotState->unk3A * g_BattleCurrentRow->power;
    s32 q = v / 20;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0xB;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = q;
}
void BattleClearSlotConditionBits(void) {
    BattleSlotState *p = g_BattleCurSlotState;
    BattleTableEntry *q = g_BattleTargetSlotEntry;

    p->activeEffectFlags1 &= 0xF80B;
    q->conditionFlags &= 0xFFDF;
}
s32 BattleGearHasWeaponId(u8 arg0) {
    BattleGearWeaponRecord *pRec = &g_BattleCurWorkBlock->gearWeaponIds[arg0];
    u8 v = pRec->weaponId;
    u8 id = g_GameState.characters[4].gearId;

    if (g_GameState.gears[id].weaponData[0].weaponId == v ||
        g_GameState.gears[id].weaponData[1].weaponId == v) {
        return 1;
    }
    return g_GameState.gears[id].weaponData[2].weaponId == v;
}
void BattleApplyGearWeaponRecord(u8 arg0) {
    BattleGearWeaponRecord *p = &g_BattleCurWorkBlock->gearWeapons[arg0];
    u8 v = p->weaponId;
    u8 id = g_GameState.characters[4].gearId;
    u8 sel;
    u8 i;

    if (g_GameState.gears[id].weaponData[0].weaponId == v) {
        sel = 0;
    }
    if (g_GameState.gears[id].weaponData[1].weaponId == v) {
        sel = 1;
    }
    if (g_GameState.gears[id].weaponData[2].weaponId == v) {
        sel = 3;
    }
    g_GameState.gears[id].weaponData[sel].attack = p->attack;
    g_GameState.gears[id].weaponData[sel].unk4 = p->unk11;
    g_GameState.gears[id].weaponData[sel].unk3 = p->unk10;
    g_GameState.gears[id].weaponData[sel].unk4 = p->unk11;
    g_GameState.gears[id].extraWeaponIds[sel] = arg0;
    g_GameState.gearPartsIds[0x6C + arg0] = p->partId;
    for (i = 0; i < 3; i++) {
        if ((g_BattleCurWorkBlock->slots + i)->characterId == 4) {
            g_BattleCurWorkBlock->slots[i].state.weaponData[sel].attack = p->attack;
            g_BattleCurWorkBlock->slots[i].state.weaponData[sel].unk4 = p->unk11;
            g_BattleCurWorkBlock->slots[i].state.weaponData[sel].unk3 = p->unk10;
            g_BattleCurWorkBlock->slots[i].state.weaponData[sel].unk4 = p->unk11;
            g_BattleCurWorkBlock->slots[i].state.extraWeaponIds[sel] = arg0;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E788);
// 99.91% at retail's exact size (224 bytes): the dispatch, all four arm bodies and
// the shared decrement tail line up, and ONE instruction is left. Retail keeps two
// identical copies of the "idx = extraWeaponIds[0]" arm (cases 0 and 15, at
// .L8009E7C0 and .L8009E814); plain C merges them into one block and comes out 0x18
// bytes short (200 bytes, 89.11%). The volatile load in case 15 is the only lever
// found that keeps the two blocks rtx-unequal, and it costs an andi: gcc cannot fuse
// a volatile QI load with its zero-extension, so case 15 comes out
// "lbu v0,4(v0)" plus "andi v1,v0,0xff" in the jump delay slot where retail has the
// lbu straight into v1. Measured at 200 bytes (merged): a s32, u8, s16 and u32 index
// local, a copy through a second local (coalesced away), *arr against arr[0], a
// volatile global pointer, a volatile pointer to the array, arr[0] & 0xFF, braces
// and do-while(0) around the arm; and 208 bytes with a u8 index local. That the
// MERGE is what costs the 24 bytes is confirmed by a value difference
// (extraWeaponIds[1] in case 15), which gives 224 bytes at 99.80%.
// Re-measured with diff_func.sh on this body: 98.66% at 224 bytes, the two
// instructions being lbu v0,4 + andi v1,v0,0xff where retail has lbu v1,4 + nop.
// tools/scripts/permute.sh (base=205) reported a ZERO candidate that appends
// "idx = extraWeaponIds[3]" after the volatile load. Do NOT port it: that leaves
// the volatile value dead, so case 15 falls into the extraWeaponIds[3] block and
// reads 0x7 instead of 0x4 (wrong code), and objdiff reads only 99.73% - the case
// 15 block only looks aligned while one nop is still missing. Also re-tested here: the
// pointer-cast spelling *(u8 *)arr and a u8 shadow local are rtx-identical to
// case 0, so gcc 2.6.0 cross-jumps and the function is back to 200 bytes. Only a
// volatile load keeps the duplicate block, so this is a structural wall.
/*
void func_8009E788(void) {
    s32 idx;

    switch (g_BattleCurWorkBlock->rowIndex) {
    case 0:
        idx = g_BattleTargetSlotState->extraWeaponIds[0];
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        idx = g_BattleTargetSlotState->extraWeaponIds[0];
        if (((u8 *)&g_GameState.unk22B6)[idx] != 0) {
            ((u8 *)&g_GameState.unk22B6)[idx] = ((u8 *)&g_GameState.unk22B6)[idx] + 0xFF;
        }
        idx = g_BattleTargetSlotState->extraWeaponIds[3];
        break;
    case 15:
        idx = *(volatile u8 *)&g_BattleTargetSlotState->extraWeaponIds[0];
        break;
    case 2:
    case 17:
        idx = g_BattleTargetSlotState->extraWeaponIds[3];
        break;
    default:
        return;
    }
    if (((u8 *)&g_GameState.unk22B6)[idx] != 0) {
        ((u8 *)&g_GameState.unk22B6)[idx] = ((u8 *)&g_GameState.unk22B6)[idx] + 0xFF;
    }
}
*/
void BattleSetCommandCodeForFlagBit(u8 arg0, u16 arg1) {
    switch (arg0) {
    case 0:
        switch (arg1) {
        case 0x400:
            g_BattleCurWorkBlock->commandCode = 0x24;
            break;
        case 0x200:
            g_BattleCurWorkBlock->commandCode = 0x25;
            break;
        case 0x100:
            g_BattleCurWorkBlock->commandCode = 0x26;
            break;
        case 0x80:
            g_BattleCurWorkBlock->commandCode = 0x27;
            break;
        case 0x40:
            g_BattleCurWorkBlock->commandCode = 0x28;
            break;
        case 0x20:
            g_BattleCurWorkBlock->commandCode = 0x29;
            break;
        case 0x10:
            g_BattleCurWorkBlock->commandCode = 0x2A;
            break;
        case 4:
            g_BattleCurWorkBlock->commandCode = 0x2B;
            break;
        }
        break;
    case 1:
        switch (arg1) {
        case 0x1000:
            g_BattleCurWorkBlock->commandCode = 0x2D;
            break;
        case 0x800:
            g_BattleCurWorkBlock->commandCode = 0x2E;
            break;
        case 0x400:
            g_BattleCurWorkBlock->commandCode = 0x2F;
            break;
        case 0x40:
            g_BattleCurWorkBlock->commandCode = 0x15;
            break;
        case 0x20:
            g_BattleCurWorkBlock->commandCode = 0x16;
            break;
        case 8:
        case 2:
            g_BattleCurWorkBlock->commandCode = 0x19;
            break;
        case 4:
        case 1:
            g_BattleCurWorkBlock->commandCode = 0x1A;
            break;
        }
        break;
    case 3:
        switch (arg1) {
        case 0x8000:
            g_BattleCurWorkBlock->commandCode = 0x1B;
            break;
        case 0x4000:
            g_BattleCurWorkBlock->commandCode = 0x1C;
            break;
        case 0x2000:
            g_BattleCurWorkBlock->commandCode = 0x1D;
            break;
        case 0x1000:
            g_BattleCurWorkBlock->commandCode = 0x1E;
            break;
        case 0x400:
            g_BattleCurWorkBlock->commandCode = 0x1F;
            break;
        case 0x800:
            g_BattleCurWorkBlock->commandCode = 0x20;
            break;
        case 0x100:
            g_BattleCurWorkBlock->commandCode = 0x21;
            break;
        case 0x200:
            g_BattleCurWorkBlock->commandCode = 0x22;
            break;
        }
        break;
    }
}
