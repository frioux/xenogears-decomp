#include "common.h"
#include "main/game.h"
#include "battle/actor.h"

extern u16 D_800C3AA4[];
extern u8 D_800D2D10[];
extern u8 g_BattleActorCharIds[];
extern BattleWorkBlock *g_BattleCurWorkBlock;
extern BattleRow *g_BattleCurrentRow;
extern BattleTableEntry *g_BattleActingSlot;
extern u8 D_800C34AD;
extern BattleTableEntry *D_800C3E34;
extern u8 D_800C3E04;
extern u8 D_800C3E50;
extern u8 D_800D2DC4;
extern BattleSlotState *D_800D2D6C;
extern u8 *D_800D2DC8;

extern u16 g_BattleActionParam;
extern u16 D_800D2C9E;

// The 0x170-byte per-slot condition table. Its flag word sits at +0x8A
// (BattleTableEntry::unk8A) and its timer bytes at +0x15C, which the original
// reaches through a second base of its own.
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleSlotConditions[][0x170];
extern u8 D_800CCE44[][0x170];

// Per-slot entry of the table at 0x800D220E: one word per battle slot, 0x10
// bytes apart. Only the word at +0 is read here.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u8 unk2[0xE];
} BattleSlotWordEntry; // Size: 0x10

extern BattleSlotWordEntry D_800D220E[];

extern s32 rand(void);
extern s8 func_80097964(u8, u8, u16);
extern void BattleSetSlotConditionTimer(u8, u8, u16, s32);
extern void func_80098D2C(u8, u8);
extern void func_80094D24(void);
extern u8 BattleTestActorSlotFlagsC002ByMode(void);
extern void BattleSetCommandCodeForSlotKindFlag(u8, u16);

void BattleApplyScriptConditionTimer(void) {
    s8 ret = func_80097964(g_BattleCurrentRow->unk1C, g_BattleCurrentRow->unk1D,
                           g_BattleCurrentRow->unk1E);

    if (g_BattleCurrentRow->unk0A & 0x4000) {
        BattleSetSlotConditionTimer(D_800C3E50, g_BattleCurrentRow->unk1D, g_BattleCurrentRow->unk1E, 5);
    } else {
        BattleSetSlotConditionTimer(D_800C3E50, g_BattleCurrentRow->unk1D, g_BattleCurrentRow->unk1E,
                      g_BattleCurrentRow->unk11);
        if (ret != 1) {
            BattleWorkBlock *p = g_BattleCurWorkBlock;

            p->unk5FA0[D_800C3E50] = 6;
        }
    }
}
void BattleApplyActorConditionTimer(void) {
    BattleTableEntry *p = g_BattleActingSlot;

    if (func_80097964(p->unk02, p->unk03, p->flags) == 1) {
        BattleRow *q = g_BattleCurrentRow;

        BattleSetSlotConditionTimer(D_800C3E50, q->unk1D, q->unk1E, 5);
    }
}
void BattleRollAndClearTargetConditions(void) {
    u8 t = g_BattleCurrentRow->unk1C;

    if (t < rand() % 100) {
        g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 6;
    } else {
        if (g_BattleCurrentRow->unk1D & 0x80) {
            D_800C3E34->unk7C &= 0xC000;
        }
        if (g_BattleCurrentRow->unk1D & 0x40) {
            D_800C3E34->unk80 = 0;
            D_800C3E34->unk7A &= 0xFFDF;
        }
        if (g_BattleCurrentRow->unk1D & 0x20) {
            D_800C3E34->unk84 = 0;
        }
        if (g_BattleCurrentRow->unk1D & 0x10) {
            D_800C3E34->unk88 = 0;
        }
        if (g_BattleCurrentRow->unk1D & 0x8) {
            D_800C3E34->unk8C = 0;
        }
    }
}
void BattleRollSlotConditionAction(void) {
    u8 chance;
    u16 hp;
    u16 dmg;

    switch (g_BattleCurrentRow->unk17[1]) {
    case 0:
        chance = g_BattleActingSlot->unk60[0];
        break;
    case 1:
        chance = g_BattleCurrentRow->unk1C;
        break;
    }

    if (chance < rand() % 100) {
        g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 6;
        return;
    }

    switch (g_BattleCurrentRow->unk17[3]) {
    case 0:
        hp = g_BattleActingSlot->maxHp;
        break;
    case 1:
        hp = D_800C3E34->maxHp;
        break;
    case 2:
        hp = g_BattleActingSlot->unk52;
        break;
    case 3:
        hp = D_800C3E34->unk52;
        break;
    }

    dmg = hp * g_BattleCurrentRow->unk11 / 20;
    if (g_BattleCurrentRow->unk17[3] == 5) {
        dmg = D_800C3E34->hp - 1;
    }

    switch (g_BattleCurrentRow->unk17[3]) {
    case 0:
    case 1:
    case 5:
        g_BattleCurWorkBlock->unk5FA0[D_800C3E04] = 2;
        g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 0;
        g_BattleCurWorkBlock->unk5F6C[D_800C3E04] = dmg;
        g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = dmg;
        break;
    case 2:
    case 3:
        if ((D_800C3E34->unk88 | D_800C3E34->unk8A) & 0x200) {
            g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 6;
            return;
        }
        g_BattleCurWorkBlock->unk5FA0[D_800C3E04] = 3;
        g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 1;
        g_BattleCurWorkBlock->unk5F6C[D_800C3E04] = dmg;
        g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = dmg;
        break;
    default:
        return;
    }
}
void BattleSetSlotConditionActionValue(void) {
    u8 chance;

    switch (g_BattleCurrentRow->unk17[1]) {
    case 0:
        chance = g_BattleActingSlot->unk60[0];
        break;
    case 1:
        chance = g_BattleCurrentRow->unk1C;
        break;
    }

    if (chance < rand() % 100) {
        g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 6;
        return;
    }

    g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 0;

    switch (g_BattleCurrentRow->unk17[3]) {
    case 0:
        if (g_BattleCurWorkBlock->slots[D_800C3E50].state.unkB6 & 0x80) {
            g_BattleCurWorkBlock->unk5F6C[D_800C3E50] =
                ((BattleSlotState *)D_800D2DC8)->hp / g_BattleCurrentRow->unk11;
        } else {
            g_BattleCurWorkBlock->unk5F6C[D_800C3E50] =
                D_800C3E34->hp / g_BattleCurrentRow->unk11;
        }
        break;
    case 1:
        if (g_BattleCurWorkBlock->slots[D_800C3E50].state.unkB6 & 0x80) {
            g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = ((BattleSlotState *)D_800D2DC8)->hp - 1;
        } else {
            g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = D_800C3E34->hp - 1;
        }
        break;
    case 2:
        if (g_BattleCurWorkBlock->slots[D_800C3E04].state.unkB6 & 0x80) {
            g_BattleCurWorkBlock->unk5F6C[D_800C3E50] =
                D_800D2D6C->maxHp - D_800D2D6C->hp;
        } else {
            g_BattleCurWorkBlock->unk5F6C[D_800C3E50] =
                g_BattleActingSlot->maxHp - g_BattleActingSlot->hp;
        }
        break;
    case 3:
        g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = D_800C3E34->unk50 * 10;
        break;
    case 4:
        g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = 1;
        break;
    case 5:
        g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = D_800C3E34->hp;
        break;
    case 6:
        g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 0;
        if (g_BattleWorkBlock[D_800C3E50].state.unkB6 & 0x80) {
            g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = ((BattleSlotState *)D_800D2DC8)->maxHp;
            if (((BattleSlotState *)D_800D2DC8)->maxHp >= 0x2710) {
                ((BattleSlotState *)D_800D2DC8)->maxHp = 0x270F;
            }
        } else {
            g_BattleCurWorkBlock->unk5F6C[D_800C3E50] = D_800C3E34->maxHp;
            if (D_800C3E34->maxHp >= 0x2710) {
                D_800C3E34->maxHp = 0x270F;
            }
        }
        break;
    case 7:
        if (g_BattleCurWorkBlock->slots[D_800C3E50].state.unkB6 & 0x80) {
            g_BattleCurWorkBlock->unk5FA0[D_800C3E50] = 6;
        } else {
            D_800C3E34->unk7C |= 1;
            D_800C3E34->unk80 |= 1;
        }
        break;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096494);
/* COMPILES as written (round 111 re-check): 93.04%, 908 bytes. The parked body
   used raw u8-array spellings of g_BattleCurrentRow / g_BattleActingSlot / D_800C3E34; with
   today's typed externs only the field reads below changed (a byte read of
   D_800C3E34->unk38, because retail uses lbu there but lhu for the 0x40 test).
   Same block layout and instruction multiset (226 vs 227); every
   branch, the two duplicated (elemAtk & elem2) blocks, the element case tree,
   the /10 pair and the two /5 tails line up instruction for instruction. Left:
   register allocation (retail weak=t6, bitAtk=t5, elemDef=t2; ours weak=t5,
   bitAtk=t2, elemDef=a0), retail's base-first `addu v0,a0,v0` on the
   actor[sel].unk15A stride (both `pBase + sel * 0x170` and
   `sel * 0x170 + (s32)pBase` give index-first), and one extra
   `andi v0,t1,0xff` on the second `elemAtk & pair` (retail does the AND in
   QImode because its result only feeds a compare against 0). Notes for the next
   attempt: `weak`/`hitB` must be s32 (a u8 costs an andi at each test),
   `hitA` is u8 (its test takes andi 0xff), and the switch needs a hoisted
   `mask = elem2 << 8` local - that is what makes retail emit the single
   `sll v1,t2,0x8` before the case chain. Tried: u8 vs s32 weak/hitB, nested vs
   && for the elemAtk == 0 guard, declaration order, u16 vs u8 elemAtk.
void func_80096494(u16 *pA, u16 *pB, u8 *pFlag) {
    u8 *pBase;
    u8 bitAtk;
    u8 bitDef;
    u8 elemAtk;
    u8 elemDef;
    u8 elem2;
    s32 weak = 0;
    u8 hitA = 0;
    s32 hitB;
    u16 pair;
    u16 m;
    u16 mask;
    s8 mul;
    u8 mul2;

    pBase = g_BattleCurWorkBlock;
    bitAtk = *(u8 *)(pBase + D_800C3E50 * 0x170 + 0x15A) >> 7;
    elemAtk = g_BattleCurrentRow->unk22 & 0x3F;
    elemDef = *(u8 *)&D_800C3E34->unk38 & 0x3F;
    bitDef = *(u8 *)(pBase + D_800C3E04 * 0x170 + 0x15A) >> 7;
    if (bitDef == 0) {
        pair = g_BattleActingSlot->unk8C | g_BattleActingSlot->unk8E;
    } else {
        pair = *(u16 *)((u8 *)D_800D2D6C + 0x84) | *(u16 *)((u8 *)D_800D2D6C + 0x86);
    }
    pair >>= 12;
    if (g_BattleCurrentRow->unk0A & 0x100) {
        weak = 1;
    }
    if (elemAtk == 0) {
        if (pair != 0) {
            elemAtk = pair;
        }
    }
    if (elemAtk & pair) {
        hitA = 1;
    }
    mul = 10;
    mul2 = 10;
    if (elemAtk & elemDef) {
        if (D_800C3E34->unk38 & 0x40) {
            mul = 0x12;
        } else {
            mul = 0xF;
        }
        if (hitA) {
            mul += 2;
        }
    }
    if (bitAtk == 0) {
        m = D_800C3E34->unk8C | D_800C3E34->unk8E;
        elem2 = (m & 0xF00) >> 8;
        hitB = (m >> 1) & 1;
    } else {
        hitB = 0;
        m = *(u16 *)(D_800D2DC8 + 0x84) | *(u16 *)(D_800D2DC8 + 0x86);
        elem2 = (m & 0xF00) >> 8;
        if (m & 2) {
            hitB = 1;
        }
    }
    if (elemAtk & elem2) {
        if (weak) {
            mul -= 3;
            if (m & 4) {
                mul -= 3;
            }
            if (m & 8) {
                *pFlag = 4;
            }
        }
    }
    if (elemAtk & elem2) {
        if (hitB) {
            mul -= 3;
            if (m & 4) {
                mul -= 3;
            }
            if (m & 8) {
                *pFlag = 4;
            }
        }
    }
    mask = elem2 << 8;
    switch (elemAtk) {
    case 1:
        if (mask & 0x200) {
            mul += 3;
        }
        break;
    case 2:
        if (mask & 0x100) {
            mul += 3;
        }
        break;
    case 4:
        if (mask & 0x800) {
            mul += 3;
        }
        break;
    case 8:
        if (mask & 0x400) {
            mul += 3;
        }
        break;
    }
    if (mul < 1) {
        mul = 1;
    }
    if (mul2 == 0) {
        mul2 = 1;
    }
    *pA = *pA * mul / 10;
    *pB = mul2 * *pB / 10;
    if (((u16 *)&g_BattleActingSlot->unk04[0x2E])[0] & 0x10) {
        *pA += *pA / 5;
    }
    if (((u16 *)&D_800C3E34->unk04[0x2E])[0] & 0x10) {
        *pB += *pB / 5;
    }
}
*/
void BattleRollPercentAndSetState38(void) {
    s32 t = g_BattleActingSlot->unk5B + g_BattleCurrentRow->unk14;

    if (rand() % 100 >= t) {
        g_BattleCurWorkBlock->commandCode = 0x38;
        D_800D2DC4 = 1;
    }
}
void BattleRollPercentAndSwapActiveActor(void) {
    BattleTableEntry *pTarget;
    u8 *pBase;
    BattleTableEntry *pActor;
    BattleTableEntry *pOld;
    u16 t;
    u8 sel;
    u8 s;
    u8 tmp;
    s32 lim;

    pTarget = D_800C3E34;
    if (pTarget->unk7C & 0xA000) {
        return;
    }
    t = pTarget->unk80;
    if (t & 0x1000) {
        return;
    }
    sel = D_800C3E04;
    if (sel < 3 && !(t & 0x2000)) {
        return;
    }
    if (!(pTarget->unk88 & 0x1000)) {
        return;
    }
    t = g_BattleCurrentRow->unk0A;
    if (t & 0x2000) {
        return;
    }
    if (t & 0x100) {
        return;
    }
    pBase = (u8 *)g_BattleCurWorkBlock->slots;
    pActor = (BattleTableEntry *)(pBase + sel * sizeof(*pActor));
    if (pActor->state.unkB6 & 0x80) {
        return;
    }
    lim = 0x32;
    if (pTarget->unk56 == 0) {
        lim = 0x3C;
    }
    if (lim < rand() % 100) {
        return;
    }
    if (g_BattleCurrentRow->unk16 == 2) {
        return;
    }
    tmp = D_800C3E50;
    s = D_800C3E04;
    pOld = g_BattleActingSlot;
    D_800C3E04 = tmp;
    g_BattleActingSlot = D_800C3E34;
    D_800C3E34 = pOld;
    D_800C3E50 = s;
    // row table at +0x1378, 0x5F0 bytes per slot (inside BattleWorkBlock's
    // unk450, so it is reached by offset)
    g_BattleCurrentRow = (BattleRow *)((u8 *)g_BattleCurWorkBlock + D_800C3E04 * 0x5F0 + 0x1378);
    g_BattleCurWorkBlock->unk5FA0[D_800C3E04] = 7;
    g_BattleCurWorkBlock->unk5F6C[D_800C3E04] = 0;
    g_BattleCurWorkBlock->commandCode = 0x34;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096AB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096FBC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097610);
void BattleNop(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097964);
/* 95.67%, 944 bytes against retail 1016; instruction stream and both jump tables
   (jtbl_800703C0, jtbl_800703E8) line up block for block, so the shape is right.
   Left over: the case-0 guard masks the wrong operand (retail andi v1,s1,0xFFFD then
   and with the 0x7E load, gcc folds the mask onto the load), D_800C3E34 lands in v1
   where retail uses a0, and the two "sh v0,0x7A(v1)" tails of case 0 and case 5 are
   not cross-jumped because the case 5 pointer sums index-first (addu v0,v0,v1 where
   retail has addu v1,v1,v0). 4 minutes of permuter found nothing.
s8 func_80097964(u8 arg0, u8 arg1, u16 arg2) {
    if (g_BattleCurWorkBlock->slots[D_800C3E50].state.unkB6 & 0x80) {
        return 0;
    }

    if (arg0 < rand() % 100) {
        return 0;
    }

    switch (arg1) {
    case 0:
        if ((arg2 & 0xFFFD) & D_800C3E34->unk7E) {
            return 0;
        }
        if (arg2 & 0x1000) {
            D_800C3E34->unk84 &= 0x7FFF;
        }
        break;
    case 2:
        if (D_800C3E34->unk82 & arg2) {
            return 0;
        }
        break;
    case 5:
        if ((D_800C3E34->unk7C | D_800C3E34->unk7E) & 1) {
            return 0;
        }
        if (arg2 & 0x8000) {
            D_800C3E34->unk7C &= 0xEFFF;
        }
        break;
    case 7:
        if ((D_800C3E34->unk80 | D_800C3E34->unk82) & 1) {
            return 0;
        }
        if (arg2 & 0xA) {
            if (D_800C3E34->unk88 & 0x5) {
                D_800C3E34->unk88 &= 0xFFFA;
                return 1;
            }
            break;
        }
        if (arg2 & 0x5) {
            if (D_800C3E34->unk88 & 0xA) {
                D_800C3E34->unk88 &= 0xFFF5;
                return 1;
            }
            break;
        }
        // falls into case 9 when the flags request neither of the 0x8A pair
    case 9:
        if (arg2 & 0xF000) {
            if (D_800C3E34->unk8E & 0xF000) {
                g_BattleCurWorkBlock->commandCode = 0x39;
                return 0;
            }
            D_800C3E34->unk8C &= 0xFFF;
        }
        if (arg2 & 0xF00) {
            if (D_800C3E34->unk8E & 0xF00) {
                g_BattleCurWorkBlock->commandCode = 0x39;
                return 0;
            }
            D_800C3E34->unk8C &= 0xF0FF;
        }
        break;
    }

    switch (arg1) {
    case 0:
        D_800C3E34->unk7C = (arg2 | D_800C3E34->unk7C) & 0xFFFD;
        if (arg2 & 0x2) {
            if (BattleTestActorSlotFlagsC002ByMode()) {
                D_800C3E34->unk7C |= arg2;
                g_BattleCurWorkBlock->commandCode = 0x31;
                D_800C3E34->unk7A = 0xFFEF;
            } else {
                g_BattleCurWorkBlock->commandCode = 0x32;
            }
        }
        break;
    case 2:
        D_800C3E34->unk80 |= arg2;
        if (arg2 & 0x800) {
            D_800C3E34->unk7A |= 0x20;
        }
        break;
    case 5:
    case 7:
    case 9: {
        // the per-kind status words at +0x7A, indexed by the kind
        u16 *pStatus = (u16 *)D_800C3E34 + arg1;

        pStatus[0x3D] |= arg2;
        break;
    }
    }

    BattleSetCommandCodeForSlotKindFlag(arg1, arg2);
    return 1;
}*/
void BattleClearAllSlotStates(void) {
    s16 i;

    for (i = 0xB; i != -1; i--) {
        g_BattleCurWorkBlock->unk5FA0[i] = 0xFF;
        g_BattleCurWorkBlock->unk5F6C[i] = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097D5C);
void BattleResetSlotConditionCounters(void) {
    u8 i;
    u8 j;
    u8 v;
    BattleTableEntry *pSlot;
    u16 hp;
    u16 flag;

    for (i = 0; i < 3; i++) {
        if (g_BattleActorCharIds[i] != 0x7F) {
            pSlot = &g_BattleCurWorkBlock->slots[i];
            g_BattleActingSlot = pSlot;
            hp = pSlot->unk7A;
            D_800D2D6C = &pSlot->state;
            D_800C3AA4[i] = hp;
            for (j = 0; j < 4; j++) {
                v = D_800D2D10[j];
                if (v != 0) {
                    D_800D2D6C->unk86[0x12] -= v;
                    D_800D2D6C->unk86[0x12] += D_800D2D6C->unk50[j];
                }
            }
            if (D_800D2D6C->unk86[0x12] >= 0x11) {
                D_800D2D6C->unk86[0x12] = 0x10;
            }
        }
    }

    flag = (u16)g_GameState.unk16BC[8].unk1E;
    g_GameState.characters[8].agility = 7;
    if (flag & 0x2000) {
        g_GameState.unk16BC[8].unk6 |= 0x800;
    }
    if (g_GameState.fieldScriptMemory[0] < 0xBB) {
        g_GameState.gears[0].attackPower = 10;
        g_GameState.gears[1].attackPower = 10;
        g_GameState.gears[11].attackPower = 9;
        g_GameState.gears[12].attackPower = 9;
        g_GameState.gears[13].attackPower = 8;
        g_GameState.gears[14].attackPower = 12;
        g_GameState.gears[15].attackPower = 12;
        g_GameState.gears[13].frameId = 0x58;
        g_GameState.gears[7].engineId = 0;
        g_GameState.gears[15].armorId = 0x28;
    }
}
// 99.84% (round 148), 372 bytes = the original's. Both branches, the strides,
// the 9*x tails, the 0xA5/0xA0 cap, the rand()%8-4 delta and the call are
// instruction-identical. 'other' must be 'int' (a u8 costs an andi and keeps
// retail's lbu a0,90(a2) after the lb v1,39).
// LEFT: the row base add in the 0x2228 branch only. Retail keeps 0x2228 on the
// INDEX (addiu v1,v1,8744, then addu v1,v1,a1); gcc folds the constant into the
// base instead (addiu v0,a1,8744, then addu v1,v1,v0). The same fold in the
// 0x1058 branch IS defeated by hoisting the constant into an 'int rowOffset'
// variable assigned before the if - that is the permuter's 15-point candidate
// (99.35 -> 99.84). The 0x2228 branch cannot use it: assigning rowOffset inside
// the branch, a second offset variable, or an 's32 rowIndex = sel*0x690+0x2228'
// local each land at 99.68, because the row then ends in v0 and the sw/lb use
// v0 where retail has v1. The two-statement split kept in that branch scores the
// same 99.84 as the chained form (it is the permuter's first edit). A further
// 4-minute permuter run from 99.84 found nothing (base 15, best 15): structural
// wall, do not permute again.
// Row strides: 0x5F0 (+0x1058) when bit 0x80 of actor[0x15A] is clear, 0x690
// (+0x2228) when set, plus pBase[0x5FC2]*0x28. func_80094D24 takes no arguments;
// the 'd' local is load-bearing (inlining rand()%8-4 folds the 4).
/*
u8 func_80098AF8(u8 sel) {
    u8 *pBase = g_BattleCurWorkBlock;
    u8 *pActor = (u8 *)(sel * 0x170 + (s32)pBase);
    u8 *pRow;
    int other;
    int rowOffset;
    s32 t;
    s32 v;
    s32 d;

    g_BattleActingSlot = pActor;
    D_800D2D6C = (BattleSlotState *)(pActor + 0xA4);
    rowOffset = 0x1058;

    if (sel < 3) {
        if ((pActor[0x15A] & 0x80) == 0) {
            pRow = (u8 *)(sel * 0x5F0 + rowOffset) + (s32)pBase + pBase[0x5FC2] * 0x28;
            g_BattleCurrentRow = pRow;
            other = pActor[0x5A];
            t = (s8)pRow[0x27];
            if (t < other) {
                v = (other - t) * 9;
            } else {
                v = 9;
            }
        } else {
            pRow = (u8 *)(sel * 0x690 + 0x2228) + (s32)pBase;
            pRow = pRow + pBase[0x5FC2] * 0x28;
            g_BattleCurrentRow = pRow;
            other = pActor[0x13C];
            t = (s8)pRow[0x27];
            if (t < other) {
                v = (other - t) * 9;
            } else {
                v = 9;
            }
        }
    } else {
        v = pActor[0x5A] * 9;
    }

    if ((u16)v >= 0xA6) {
        v = 0xA0;
    }
    v = 0xA5 - v;
    d = rand() % 8 - 4;
    v -= d;
    func_80094D24();
    return v;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098AF8);


void BattleInitActiveSlotActions(u8 sel) {
    u8 i;
    u16 mask;

    BattleClearAllSlotStates();

    mask = 1;
    for (i = 0; i < 11; i++) {
        if (g_BattleCurWorkBlock->unk5FAC & mask) {
            func_80098D2C(i, sel);
        }
        mask <<= 1;
    }

    *(u16 *)((u8 *)g_BattleCurWorkBlock + 0x5FB0) = D_800D220E[sel].unk0;

    if (D_800D2C9E & 0x8000) {
        g_BattleActionParam = 0xC2;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098D2C);
u8 BattleTestActorSlotFlagsC002ByMode(void) {
    u8 i;
    u8 n;
    s32 ret = 0;
    s32 mode = D_800C34AD;

    switch (mode) {
    case 1:
        break;
    case 2:
        n = 0;
        for (i = 0; i < 3; i++) {
            if (g_BattleCurWorkBlock->slots[i].unk7C & 0xC002) {
                n++;
            }
        }
        if (n == 0) {
            ret = 1;
        }
        break;
    case 3:
        n = 0;
        for (i = 0; i < 3; i++) {
            if (g_BattleCurWorkBlock->slots[i].unk7C & 0xC002) {
                n++;
            }
        }
        if (n != 2) {
            ret = 1;
        }
        break;
    default:
        break;
    }
    return ret;
}
void BattleSetSlotConditionTimer(u8 slot, u8 kind, u16 flag, s32 count) {
    BattleTableEntry *pEntry;
    u8 *pTimer;
    u8 idx = 0xF;
    u8 val = count;
    u16 bits;

    if (g_BattleWorkBlock[D_800C3E04].unk8A & 0x2000) {
        val *= 2;
    }
    pEntry = &g_BattleWorkBlock[slot];
    if (kind == 0) {
        if (flag != 0x1000) {
            idx = (flag == 0x2000) ? 0 : 0xF;
        } else {
            idx = 1;
        }
    }
    if (kind == 2) {
        switch (flag) {
        case 0x1000:
            idx = 2;
            break;
        case 0x800:
            idx = 3;
            break;
        }
    }
    if (kind == 5) {
        switch (flag) {
        case 0x8000:
            idx = 4;
            break;
        case 0x4000:
            idx = 5;
            break;
        }
    }
    if (kind == 7) {
        switch (flag) {
        case 0x8000:
            idx = 9;
            break;
        case 0x4000:
            idx = 0xA;
            break;
        case 0x1000:
            idx = 0xB;
            break;
        }
    }
    if (kind == 9) {
        switch (flag) {
        case 0x1000:
        case 0x2000:
        case 0x4000:
        case 0x8000:
            idx = 0xC;
            break;
        case 0x100:
        case 0x200:
        case 0x400:
        case 0x800:
            idx = 0xD;
            break;
        }
    }
    if (idx == 0xF) {
        return;
    }
    switch (kind) {
    case 5:
    case 7:
    case 9:
        bits = D_800C3E34->unk88 | D_800C3E34->unk8A;
        if (bits & 5) {
            val *= 2;
        }
        if (bits & 0xA) {
            val >>= 1;
        }
        break;
    }
    if (((u16 *)&pEntry->unk04[0x2E])[0] & 0x40) {
        switch (kind) {
        case 5:
        case 7:
        case 9:
            val *= 2;
            break;
        }
    }
    pTimer = D_800CCE44[slot];
    pTimer[idx] = val;
}
// Rolls one slot's condition timers down and returns the mask of the conditions
// that expired this frame. Each armed flag word (0x7C..0x8E) owns one timer byte
// at entry+0x15C, i.e. state.unkB7[1], and the 0x84/0x88 tests are a pair: the
// low halfword's flag armed with the high halfword's copy of it clear. The timer
// and state pointers are formed off the symbol base, not off pEntry: the original
// keeps that second base in a register of its own.
u16 BattleDecrementSlotConditionTimers(u8 slot) {
    BattleTableEntry *pEntry = &g_BattleWorkBlock[slot];
    BattleSlotState *pState = (BattleSlotState *)(slot * 0x170 + 0xA4 + (u32)g_BattleWorkBlock);
    u8 *pCnt = (u8 *)(slot * 0x170 + 0x15C + (u32)g_BattleWorkBlock);
    u32 ret;

    if (g_BattleSlotConditions[slot][0] & 0x80) {
        BattleDecrementConditionTimers((BattleTableEntry *)pState, pEntry, pCnt);
        return 0;
    }
    ret = 0;
    if (pEntry->unk7C & 0x1000) {
        pCnt[1] = pCnt[1] + 0xFF;
        if (pCnt[1] == 0) {
            ret |= 0x4000;
            pEntry->unk7C &= 0xEFFF;
        }
    }
    if (pEntry->unk80 & 0x1000) {
        pCnt[2] = pCnt[2] + 0xFF;
        if (pCnt[2] == 0) {
            ret |= 0x2000;
            pEntry->unk80 &= 0xEFFF;
        }
    }
    if (pEntry->unk80 & 0x800) {
        pCnt[3] = pCnt[3] + 0xFF;
        if (pCnt[3] == 0) {
            ret |= 0x1000;
            pEntry->unk80 &= 0xF7FF;
            pEntry->unk7A &= 0xFFDF;
        }
    }
    if ((pEntry->unk84 & 0x8000) && !(pEntry->unk86 & 0x8000)) {
        pCnt[4] = pCnt[4] + 0xFF;
        if (pCnt[4] == 0) {
            ret |= 0x800;
            pEntry->unk84 &= 0x7FFF;
        }
    }
    if ((pEntry->unk84 & 0x4000) && !(pEntry->unk86 & 0x4000)) {
        pCnt[5] = pCnt[5] + 0xFF;
        if (pCnt[5] == 0) {
            ret |= 0x400;
            pEntry->unk84 &= 0xBFFF;
        }
    }
    if ((pEntry->unk84 & 0x2000) && !(pEntry->unk86 & 0x2000)) {
        pCnt[6] = pCnt[6] + 0xFF;
        if (pCnt[6] == 0) {
            ret |= 0x200;
            pEntry->unk84 &= 0xDFFF;
        }
    }
    if ((pEntry->unk84 & 0x1000) && !(pEntry->unk86 & 0x1000)) {
        pCnt[7] = pCnt[7] + 0xFF;
        if (pCnt[7] == 0) {
            ret |= 0x100;
            pEntry->unk84 &= 0xEFFF;
        }
    }
    if ((pEntry->unk84 & 0x800) && !(pEntry->unk86 & 0x800)) {
        pCnt[8] = pCnt[8] + 0xFF;
        if (pCnt[8] == 0) {
            ret |= 0x80;
            pEntry->unk84 &= 0xF7FF;
        }
    }
    if ((pEntry->unk88 & 0x8000) && !(pEntry->unk8A & 0x8000)) {
        pCnt[9] = pCnt[9] + 0xFF;
        if (pCnt[9] == 0) {
            ret |= 0x40;
            pEntry->unk88 &= 0x7FFF;
        }
    }
    if ((pEntry->unk88 & 0x4000) && !(pEntry->unk8A & 0x4000)) {
        pCnt[0xA] = pCnt[0xA] + 0xFF;
        if (pCnt[0xA] == 0) {
            ret |= 0x20;
            pEntry->unk88 &= 0xBFFF;
        }
    }
    if ((pEntry->unk88 & 0x1000) && !(pEntry->unk8A & 0x1000)) {
        pCnt[0xB] = pCnt[0xB] + 0xFF;
        if (pCnt[0xB] == 0) {
            ret |= 0x10;
            pEntry->unk88 &= 0xEFFF;
        }
    }
    if (pEntry->unk8C & 0xF000) {
        if (!(pEntry->unk8E & 0xF000)) {
            pCnt[0xC] = pCnt[0xC] + 0xFF;
            if (pCnt[0xC] == 0) {
                ret |= 8;
                pEntry->unk8C &= 0xFFF;
            }
        }
    }
    if (pEntry->unk8C & 0xF00) {
        if (!(pEntry->unk8E & 0xF00)) {
            pCnt[0xD] = pCnt[0xD] + 0xFF;
            if (pCnt[0xD] == 0) {
                ret |= 4;
                pEntry->unk8C &= 0xF0FF;
            }
        }
    }
    return ret;
}
void BattleDecrementConditionTimers(BattleTableEntry *p, BattleTableEntry *pOther, u8 *pCnt) {
    if (p->unk7C & 0x200) {
        pCnt[1] = pCnt[1] + 0xFF;
        if (pCnt[1] == 0) {
            p->unk7C &= 0xFDFF;
        }
    }
    if (p->unk7C & 0x100) {
        pCnt[2] = pCnt[2] + 0xFF;
        if (pCnt[2] == 0) {
            p->unk7C &= 0xFEFF;
        }
    }
    if (p->unk7C & 0x80) {
        pCnt[3] = pCnt[3] + 0xFF;
        if (pCnt[3] == 0) {
            p->unk7C &= 0xFF7F;
        }
    }
    if (p->unk7C & 0x20) {
        pCnt[4] = pCnt[4] + 0xFF;
        if (pCnt[4] == 0) {
            p->unk7C &= 0xFFDF;
            pOther->unk7C &= 0xEFFF;
        }
    }
    if (p->unk7C & 0x10) {
        pCnt[5] = pCnt[5] + 0xFF;
        if (pCnt[5] == 0) {
            p->unk7C &= 0xFFEF;
        }
    }
    if (p->unk7C & 0xF000) {
        pCnt[7] = pCnt[7] + 0xFF;
        if (pCnt[7] == 0) {
            p->unk7C &= 0x0FFF;
        }
    }
    if (p->unk7C & 0xF00) {
        pCnt[8] = pCnt[8] + 0xFF;
        if (pCnt[8] == 0) {
            p->unk7C &= 0xF0FF;
        }
    }
    if (p->unk80 & 0x1000) {
        pCnt[0xA] = pCnt[0xA] + 0xFF;
        if (pCnt[0xA] == 0) {
            p->unk80 &= 0xEFFF;
        }
    }
    if (p->unk80 & 0x40) {
        pCnt[0xB] = pCnt[0xB] + 0xFF;
        if (pCnt[0xB] == 0) {
            p->unk80 &= 0xFFBF;
        }
    }
    if (p->unk80 & 0x20) {
        pCnt[0xC] = pCnt[0xC] + 0xFF;
        if (pCnt[0xC] == 0) {
            p->unk80 &= 0xFFDF;
        }
    }
}
void BattleSaveScriptCommandOperandsFromActor(void) {
    u16 ab = g_BattleActingSlot->unk8C | g_BattleActingSlot->unk8E;

    g_BattleCurWorkBlock->unk5FBC = g_BattleCurrentRow->unk20;
    g_BattleCurWorkBlock->unk5FBD = g_BattleCurrentRow->unk21;
    g_BattleCurWorkBlock->unk5FBE = g_BattleCurrentRow->unk22;
    g_BattleCurWorkBlock->unk5FBF = g_BattleCurrentRow->unk23;
    g_BattleCurWorkBlock->unk5FC0 = g_BattleCurWorkBlock->unk5FC2;

    ab >>= 12;
    if ((g_BattleCurWorkBlock->unk5FBE & 0x3F) == 0) {
        g_BattleCurWorkBlock->unk5FBE |= ab;
    }
}
void BattleFilterSlotMaskByGroup(void) {
    if (g_BattleCurWorkBlock->unk5FAC & 7) {
        g_BattleCurWorkBlock->unk5FAC = g_BattleCurWorkBlock->unk5FAE & 7;
    }
    if (g_BattleCurWorkBlock->unk5FAC & 0x7F8) {
        g_BattleCurWorkBlock->unk5FAC = g_BattleCurWorkBlock->unk5FAE & 0x7F8;
    }
}
