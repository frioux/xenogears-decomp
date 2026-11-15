#include "common.h"
#include "main/game.h"
#include "battle/actor.h"

extern u8 D_800C3AA4[];
extern u8 D_800D2D10[];
extern u8 D_800D2D24[];
extern BattleWorkBlock *D_800C34B0;
extern BattleRow *D_800C3DFC;
extern BattleTableEntry *D_800C3E00;
extern u8 D_800C34AD;
extern BattleTableEntry *D_800C3E34;
extern u8 D_800C3E04;
extern u8 D_800C3E50;
extern u8 D_800D2DC4;
extern u8 *D_800D2D6C;
extern u8 *D_800D2DC8;

extern u8 D_800D220E[];
extern u16 D_800D2C98;
extern u16 D_800D2C9E;

// the 0x170-byte slot action table at 0x800CCCE8, and two of its columns:
// D_800CCD72 is the flag word at +0x8A, D_800CCE44 the timer bytes at +0x15C
// (spelling them as their own symbols is what keeps func_800995A0's
// address formation; one base + constant inside a sum measures 94.50)
extern u8 D_800CCCE8[];
extern u8 D_800CCD72[];
extern u8 D_800CCE44[];

extern s32 rand(void);
extern s8 func_80097964(u8, u8, u16);
extern void func_800995A0(u8, u8, u16, s32);
extern void func_80098D2C(u8, u8);
extern void func_80094D24(void);

void BattleApplyScriptConditionTimer(void) {
    s8 ret = func_80097964(D_800C3DFC->unk1C, D_800C3DFC->unk1D,
                           D_800C3DFC->unk1E);

    if (D_800C3DFC->unk0A & 0x4000) {
        func_800995A0(D_800C3E50, D_800C3DFC->unk1D, D_800C3DFC->unk1E, 5);
    } else {
        func_800995A0(D_800C3E50, D_800C3DFC->unk1D, D_800C3DFC->unk1E,
                      D_800C3DFC->unk11);
        if (ret != 1) {
            BattleWorkBlock *p = D_800C34B0;

            p->unk5FA0[D_800C3E50] = 6;
        }
    }
}
void BattleApplyActorConditionTimer(void) {
    BattleTableEntry *p = D_800C3E00;

    if (func_80097964(p->unk02, p->unk03, p->flags) == 1) {
        BattleRow *q = D_800C3DFC;

        func_800995A0(D_800C3E50, q->unk1D, q->unk1E, 5);
    }
}
void BattleRollAndClearTargetConditions(void) {
    u8 t = D_800C3DFC->unk1C;

    if (t < rand() % 100) {
        D_800C34B0->unk5FA0[D_800C3E50] = 6;
    } else {
        if (D_800C3DFC->unk1D & 0x80) {
            D_800C3E34->unk7C &= 0xC000;
        }
        if (D_800C3DFC->unk1D & 0x40) {
            D_800C3E34->unk80 = 0;
            D_800C3E34->unk7A &= 0xFFDF;
        }
        if (D_800C3DFC->unk1D & 0x20) {
            D_800C3E34->unk84 = 0;
        }
        if (D_800C3DFC->unk1D & 0x10) {
            D_800C3E34->unk88 = 0;
        }
        if (D_800C3DFC->unk1D & 0x8) {
            D_800C3E34->unk8C = 0;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80095D4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096018);
/* COMPILES as written (round 111 re-check): 93.04%, 908 bytes. The parked body
   was written when D_800C3DFC / D_800C3E00 / D_800C3E34 were u8 arrays; with
   today's typed externs only the two field reads changed
   (D_800C3DFC->unk22, a byte read of D_800C3E34->unk38 because retail uses lbu
   there but lhu 0x40 on the same address) plus the head/tail word reads.
   Residual: register allocation (retail weak=t6, bitAtk=t5, elemDef=t2) and the
   base-first addu on the actor[sel].unk15A stride. See the note below the old
   block.
/*
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

    pBase = D_800C34B0;
    bitAtk = *(u8 *)(pBase + D_800C3E50 * 0x170 + 0x15A) >> 7;
    elemAtk = D_800C3DFC->unk22 & 0x3F;
    elemDef = *(u8 *)&D_800C3E34->unk38 & 0x3F;
    bitDef = *(u8 *)(pBase + D_800C3E04 * 0x170 + 0x15A) >> 7;
    if (bitDef == 0) {
        pair = D_800C3E00->unk8C | D_800C3E00->unk8E;
    } else {
        pair = *(u16 *)(D_800D2D6C + 0x84) | *(u16 *)(D_800D2D6C + 0x86);
    }
    pair >>= 12;
    if (D_800C3DFC->unk0A & 0x100) {
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
    if (((u16 *)&D_800C3E00->unk04[0x2E])[0] & 0x10) {
        *pA += *pA / 5;
    }
    if (((u16 *)&D_800C3E34->unk04[0x2E])[0] & 0x10) {
        *pB += *pB / 5;
    }
}void BattleRollPercentAndSetState38(void) {
    s32 t = D_800C3E00->unk5B + D_800C3DFC->unk14;

    if (rand() % 100 >= t) {
        D_800C34B0->unk5FC7 = 0x38;
        D_800D2DC4 = 1;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096494);

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
    t = D_800C3DFC->unk0A;
    if (t & 0x2000) {
        return;
    }
    if (t & 0x100) {
        return;
    }
    pBase = (u8 *)D_800C34B0;
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
    if (D_800C3DFC->unk16 == 2) {
        return;
    }
    tmp = D_800C3E50;
    s = D_800C3E04;
    pOld = D_800C3E00;
    D_800C3E04 = tmp;
    D_800C3E00 = D_800C3E34;
    D_800C3E34 = pOld;
    D_800C3E50 = s;
    // row table at +0x1378, 0x5F0 bytes per slot (inside BattleWorkBlock's
    // unk450, so it is reached by offset)
    D_800C3DFC = (BattleRow *)((u8 *)D_800C34B0 + D_800C3E04 * 0x5F0 + 0x1378);
    D_800C34B0->unk5FA0[D_800C3E04] = 7;
    D_800C34B0->unk5F6C[D_800C3E04] = 0;
    D_800C34B0->unk5FC7 = 0x34;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096AB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096FBC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097610);
void BattleNop(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097964);
void BattleClearAllSlotStates(void) {
    s16 i;

    for (i = 0xB; i != -1; i--) {
        D_800C34B0->unk5FA0[i] = 0xFF;
        D_800C34B0->unk5F6C[i] = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097D5C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_8009892C);
/* COMPILES as written (round 111 re-check against today's externs): 96.96%,
   same 460 bytes and the same instruction multiset. Twelve source
   shapes tried (order of the D_800C3E00 / D_800D2D6C / D_800C3AA4 stores, hp
   temp vs. direct expression, u16[] vs. byte-cast array, decl order). The only
   residual difference is placement inside the block: the original leaves the
   last `sll v0,v0,0x4` of i*0x170 until after the D_800C34B0 load and computes
   the D_800C3AA4[i] address (`addu a0,a0,t0`) after the lhu, if the addresses
   are at all source-controlled; the 95-99% allocator/placement class. Presets
   screened: 2.6.0 (this, 96.96), 2.7.2 (93.91), 2.7.2-cdk (72.30). The tail
   from `flag = ...` to the end of the function is byte-identical.

void func_8009892C(void) {
    u8 i;
    u8 j;
    u8 v;
    u8 *pActor;
    u8 *pRow;
    u16 hp;
    u16 flag;

    for (i = 0; i < 3; i++) {
        if (D_800D2D24[i] != 0x7F) {
            pActor = (u8 *)(i * 0x170 + (s32)D_800C34B0);
            pRow = pActor + 0xA4;
            D_800C3E00 = pActor;
            hp = *(u16 *)(pActor + 0x7A);
            D_800D2D6C = pRow;
            *(u16 *)&D_800C3AA4[i * 2] = hp;
            for (j = 0; j < 4; j++) {
                v = D_800D2D10[j];
                if (v != 0) {
                    D_800D2D6C[0x98] -= v;
                    D_800D2D6C[0x98] += *(u8 *)((s32)D_800D2D6C + j + 0x50);
                }
            }
            if (D_800D2D6C[0x98] >= 0x11) {
                D_800D2D6C[0x98] = 0x10;
            }
        }
    }

    flag = *(u16 *)((u8 *)&g_GameState + 0x17DA);
    *(u8 *)((u8 *)&g_GameState + 0x7E6) = 7;
    if (flag & 0x2000) {
        *(u16 *)((u8 *)&g_GameState + 0x17C2) |= 0x800;
    }
    if (*(u16 *)((u8 *)&g_GameState + 0x1930) < 0xBB) {
        *(u8 *)((u8 *)&g_GameState + 0x9EC) = 10;
        *(u8 *)((u8 *)&g_GameState + 0xA90) = 10;
        *(u8 *)((u8 *)&g_GameState + 0x10F8) = 9;
        *(u8 *)((u8 *)&g_GameState + 0x119C) = 9;
        *(u8 *)((u8 *)&g_GameState + 0x1240) = 8;
        *(u8 *)((u8 *)&g_GameState + 0x12E4) = 12;
        *(u8 *)((u8 *)&g_GameState + 0x1388) = 12;
        *(u8 *)((u8 *)&g_GameState + 0x11CE) = 0x58;
        *(u8 *)((u8 *)&g_GameState + 0xDF7) = 0;
        *(u8 *)((u8 *)&g_GameState + 0x131C) = 0x28;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098AF8);
/* best: 95.16%, 94 instructions (same count as the original), in every preset
   screened (2.7.2, 2.6.0, 2.7.2-cdk), so the addiu fold below is not a preset
   question. Body, both
   branches, the outer/inner branch layout, the 9*x tails, the 0xA5/0xA0 clamp,
   the rand()%8-4 delta and the call are byte-identical. Left: gcc folds the
   row stride's +0x1058 / +0x2228 into the base add (addiu v0,a1,K then
   addu v1,v1,v0) where the original keeps it on the index (addiu v1,v1,K then
   addu v1,v1,a1), which then names the row pointer v0 instead of v1. Tried:
   the whole sum as ints with (s32)pBase, &pBase[idx + K] subscript form,
   (u8 *)(idx + K) + (s32)pBase (this one, 95.16), and a separate `s32 off`
   local for idx + K which DOES fix the addiu but flips t/other between a0 and
   v1 and drops to 90.97. Row strides: 0x5F0 (+0x1058) when bit 0x80 of
   actor[0x15A] is clear, 0x690 (+0x2228) when set, plus
   D_800C34B0[0x5FC2]*0x28; t = (s8)row[0x27]; v = t < other ? 9*(other-t) : 9
   with other = actor[0x5A] / actor[0x13C] respectively, 9*actor[0x5A] when
   sel >= 3; then v = 0xA5 - v; d = rand()%8 - 4; v -= d; func_80094D24();
   return v. The `d` local is load-bearing: `v -= rand() % 8 - 4` makes gcc
   fold the 4 into v before the call. func_80094D24 takes no arguments.

u8 func_80098AF8(u8 sel) {
    u8 *pBase = D_800C34B0;
    u8 *pActor = (u8 *)(sel * 0x170 + (s32)pBase);
    u8 *pRow;
    u8 other;
    s32 t;
    s32 v;
    s32 d;

    D_800C3E00 = pActor;
    D_800D2D6C = pActor + 0xA4;

    if (sel < 3) {
        if ((pActor[0x15A] & 0x80) == 0) {
            pRow = (u8 *)(sel * 0x5F0 + 0x1058) + (s32)pBase + pBase[0x5FC2] * 0x28;
            D_800C3DFC = pRow;
            t = (s8)pRow[0x27];
            other = pActor[0x5A];
            if (t < other) {
                v = (other - t) * 9;
            } else {
                v = 9;
            }
        } else {
            pRow = (u8 *)(sel * 0x690 + 0x2228) + (s32)pBase + pBase[0x5FC2] * 0x28;
            D_800C3DFC = pRow;
            t = (s8)pRow[0x27];
            other = pActor[0x13C];
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
}
*/
void BattleInitActiveSlotActions(u8 sel) {
    u8 i;
    u16 mask;

    BattleClearAllSlotStates();

    mask = 1;
    for (i = 0; i < 11; i++) {
        if (D_800C34B0->unk5FAC & mask) {
            func_80098D2C(i, sel);
        }
        mask <<= 1;
    }

    *(u16 *)((u8 *)D_800C34B0 + 0x5FB0) = *(u16 *)(D_800D220E + sel * 0x10);

    if (D_800D2C9E & 0x8000) {
        D_800D2C98 = 0xC2;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098D2C);
s32 BattleTestActorSlotFlagsC002ByMode(void) {
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
            if (D_800C34B0->slots[i].unk7C & 0xC002) {
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
            if (D_800C34B0->slots[i].unk7C & 0xC002) {
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
/* COMPILES as written (round 111 re-check): 97.11%, 768 bytes (retail 752),
   4 instructions too long; measured again just now. The three externs
   D_800CCCE8 / D_800CCD72 / D_800CCE44 at the top of the file are what it was
   missing. Tried and rejected this round: typed BattleTableEntry access
   (D_800CCCE8[i].unk8A, &D_800CCCE8[slot]) 94.89%, and one base symbol with
   +0x8A / +0x15C folded into the sum 94.50% - both lose retail address
   formation, so keep the three symbol spellings.
/*
void func_800995A0(u8 slot, u8 kind, u16 flag, s32 count) {
    u8 *pEntry;
    u8 *pTimer;
    u8 idx = 0xF;
    u8 val = count;
    u16 bits;

    if (*(u16 *)(D_800CCD72 + D_800C3E04 * 0x170) & 0x2000) {
        val *= 2;
    }
    pEntry = (u8 *)D_800CCCE8 + slot * 0x170;
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
        case 0x4000:
            idx = 0xA;
            break;
        case 0x8000:
            idx = 9;
            break;
        case 0x1000:
            idx = 0xB;
            break;
        }
    }
    if (kind == 9) {
        switch (flag) {
        case 0x100:
        case 0x200:
        case 0x400:
        case 0x800:
            idx = 0xD;
            break;
        case 0x1000:
        case 0x2000:
        case 0x4000:
        case 0x8000:
            idx = 0xC;
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
        bits = *(u16 *)&D_800C3E34->unk88 | *(u16 *)&D_800C3E34->unk8A;
        if (bits & 5) {
            val *= 2;
        }
        if (bits & 0xA) {
            val >>= 1;
        }
        break;
    }
    if (*(u16 *)(pEntry + 0x32) & 0x40) {
        switch (kind) {
        case 5:
        case 7:
        case 9:
            val *= 2;
            break;
        }
    }
    pTimer = (u8 *)D_800CCE44 + slot * 0x170;
    pTimer[idx] = val;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_800995A0);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80099890);
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
    u16 ab = D_800C3E00->unk8C | D_800C3E00->unk8E;

    D_800C34B0->unk5FBC = D_800C3DFC->unk20;
    D_800C34B0->unk5FBD = D_800C3DFC->unk21;
    D_800C34B0->unk5FBE = D_800C3DFC->unk22;
    D_800C34B0->unk5FBF = D_800C3DFC->unk23;
    D_800C34B0->unk5FC0 = D_800C34B0->unk5FC2;

    ab >>= 12;
    if ((D_800C34B0->unk5FBE & 0x3F) == 0) {
        D_800C34B0->unk5FBE |= ab;
    }
}
void BattleFilterSlotMaskByGroup(void) {
    if (D_800C34B0->unk5FAC & 7) {
        D_800C34B0->unk5FAC = D_800C34B0->unk5FAE & 7;
    }
    if (D_800C34B0->unk5FAC & 0x7F8) {
        D_800C34B0->unk5FAC = D_800C34B0->unk5FAE & 0x7F8;
    }
}
