#include "common.h"

extern u8 *D_800C3EAC;
extern u8 D_800C3CF4[];
extern u8 *D_800D2D28;
extern u8 D_800D366C;
extern int ArchiveDataSync(void);
extern void BattlePlaySoundWithBaseId(u8 arg0);
extern void BattleUpdateFrame(void);
extern void BattleFreePacketPrimitives(void);
extern void BattleClearFlagC6(void);
extern void func_8008FA60(s32 arg0);
extern u16 BattleSlotBitMask(u8 arg0);
extern void func_800BC404(u16 arg0);
extern void func_800BCD98(u16 arg0);
extern u16 BattleMaskHasSlotBit(u16 arg0, u8 arg1);
extern u16 BattleSlotBitMaskInverted(u8 arg0);
extern u8 D_800C400B[];
extern u16 D_800C4022[];
extern u8 D_800C402F[];
extern u8 D_800CCD1C[];
extern u8 D_800D2CE0[];
extern u8 D_800D2204[];
extern u8 D_800D3688[];
extern u8 D_800C3D70[];
extern u8 D_800C3FFE[];
extern u8 func_80085084(u16 arg0, u8 arg1, s32 arg2);
extern s32 BattleIsCharacterInParty(u8 arg0);
extern void func_8009A854(u8 arg0);
extern u16 D_800D2C94;
extern u8 D_800D2C98;
extern u8 D_800D32A1[];
extern u16 D_800C3D64;
extern u8 D_800C3E2C;
extern u8 D_800CDD40[];
extern u8 D_800CEF10[];
extern void func_80085388(void);
extern void func_80079840(u8 arg0, u8 arg1);
extern void BattleQueueCommandFE(u8 arg0);
extern void BattleApplySlotCurveAndStep(u8 arg0);
extern void BattleBeginActorAction(u8 arg0, u16 arg1, u16 arg2);
extern s32 BattleFindMinValueOtherSlot(u8 arg0);
extern void func_800B89FC(s32 arg0, u8 arg1, s32 arg2, s32 arg3);

void BattlePlaySoundIfEnabled(u8 arg0) {
    if (D_800D366C != 0) {
        BattlePlaySoundWithBaseId(arg0);
    }
}
void BattleParseNumberToDigits(u32 arg0) {
    s32 i;
    u32 divisor;

    divisor = 100000000;
    for (i = 0; i < 9; i++) {
        D_800C3CF4[i] = arg0 / divisor;
        arg0 %= divisor;
        divisor /= 10;
    }
    for (i = 1; i < 9; i++) {
        if (D_800C3CF4[i] == 0) {
            D_800C3CF4[i - 1] = 0xFF;
        } else {
            if (D_800C3CF4[i - 1] == 0) {
                D_800C3CF4[i - 1] = 0xFF;
            }
            break;
        }
    }
}
void BattleSetArchive20Entry0(void) {
    ArchiveSetIndex(0x20, 0);
}
void BattleSetArchive20Entry2(void) {
    ArchiveSetIndex(0x20, 2);
}
void BattleSetArchive20Entry3(void) {
    ArchiveSetIndex(0x20, 3);
}
s32 BattleAllocHeapBlock(s32 arg0, s32 arg1) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc(arg0, arg1);
}
s32 BattleAllocTextBuffer(s32 arg0) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc((arg0 + 3) * 26, 0);
}
void BattleWaitForArchiveSync(void) {
    while (ArchiveDataSync() != 0) {
        BattleUpdateFrame();
    }
}
void BattleQueueCommandF3WithSlotMask(u16 arg0, u8 arg1) {
    u16 mask;
    s32 i;
    s32 off;
    u8 idx;

    mask = arg0 & 0xFFF8;
    for (i = 0, off = 0x450; i < 8; i++) {
        idx = i + 3;
        if (BattleMaskHasSlotBit(mask, idx) != 0) {
            if ((*(u16 *)(D_800CCD1C + off) & 0x800) == 0) {
                mask &= BattleSlotBitMaskInverted(idx);
            }
        }
        off += 0x170;
    }
    if (mask == 0) {
        return;
    }
    D_800C400B[D_800C3EAC[0x2DA] * 72] = arg1;
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF3;
    D_800C4022[D_800C3EAC[0x2DA] * 36] = mask;
    D_800C3EAC[0x2DA] = D_800C3EAC[0x2DA] + 1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008ADD0);
/* Best: 96.69% under the unit flags (816 bytes vs the original's 796), but
   99.73% under -G0 -- report PRESET_G0. Two matched neighbours
   (BattleParseNumberToDigits, BattleClearPrimFlagsTwoSlots) stay at 100.0 under
   -G0, so this unit probably wants XenoBattle260 with -G0.
   Under -G8 the whole diff traces to one instruction: retail materialises
   D_800D2C94 into a callee-saved register once (`lui s1,%hi; addiu s1,s1,%lo`)
   and reads it twice across three calls (`lhu a0,0(s1)`, later `lhu v1,0(s1)`),
   which also gives the loop counter `i` its own register (retail `li s2,3` sits
   in the delay slot right after `jal func_80085388`) and arg0 $s3. gcc 2.6 folds
   the constant address into each load (`lui a0,%hi; lhu a0,0(a0)`), which leaves
   3 saved registers instead of 4, so every later saved-register name shifts by
   one and the diff can never reach 100. Tried: u16 *pMask local assigned at the
   load site and at the top of the function, `register u16 *pMask`, volatile
   pointer-to-const, pMask[0], -O1 (83.43), -O3 (cc1 fails).
void func_8008ADD0(u8 arg0) {
    u16 *pMask;
    s32 off;
    s32 i;
    u16 mask;
    u16 sel;
    u8 value;

    D_800D366C = 0;
    func_800BCD98(0);
    value = 0xFF;
    for (off = 0x8B8; off >= 0; off -= 0x48) {
        D_800C402F[off] = value;
    }
    if (D_800D32A1[arg0 * 8] == 0) {
        D_800C3EAC[0x2DC] = D_800C3EAC[0x2E6] + 0x17;
        off = (D_800C3EAC[0x2E6] + 0x16) * 40 + arg0 * 1520;
        sel = *(u16 *)(D_800CDD40 + off) & 7;
    } else {
        D_800C3EAC[0x2DC] = D_800C3EAC[0x2E6] + 0x16;
        off = (D_800C3EAC[0x2E6] + 0x15) * 40 + arg0 * 1680;
        sel = *(u16 *)(D_800CEF10 + off) & 7;
    }
    if (sel != 0) {
        mask = D_800C3D64;
    } else {
        mask = BattleSlotBitMask(D_800C3E2C);
    }
    pMask = &D_800D2C94;
    BattleQueueCommandF3WithSlotMask(mask, arg0);
    func_80085388();
    BattleBeginActorAction(arg0, mask, D_800C3EAC[0x2DC] - 1);
    BattleApplySlotCurveAndStep(D_800C3EAC[0x2DA]);
    mask = BattleSlotBitMask(arg0);
    func_800BC404(*pMask | mask);
    func_800B89FC(1, arg0, D_800C3EAC[arg0 * 0x40 + 0x3C], BattleFindMinValueOtherSlot(arg0));
    D_800C402F[D_800C3EAC[0x2DA] * 72] = D_800D2C98;
    D_800C400B[D_800C3EAC[0x2DA] * 72] = arg0;
    *(u16 *)(D_800C3FFE + D_800C3EAC[0x2DA] * 72) = *pMask;
    for (i = 3; i < 11; i++) {
        if (BattleMaskHasSlotBit(D_800D2C94, i) != 0) {
            func_80079840(arg0, i);
        }
    }
    D_800C3EAC[0x2DA] = D_800C3EAC[0x2DA] + 1;
    BattleQueueCommandFE(arg0);
    while (D_800C3EAC[0x2DB] == 0) {
        BattleUpdateFrame();
    }
}
*/
void BattleClearPrimFlagsFourSlots(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = D_800D2D28[0xB3] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
void BattleSetPrimFlagsFourSlots(u8 arg0) {
    int slotStride = 0x40;
    u16 value;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = D_800D2D28[0xB3] = 1;
    D_800D2D28[0xB7] = 1;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask((D_800C3EAC + arg0 * slotStride)[0x3C]);
    func_800BC404(value);
    func_800BCD98(BattleSlotBitMask((D_800C3EAC + arg0 * slotStride)[0x3C]));
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B224);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B478);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B908);
void BattleClearPrimFlagsTwoSlots(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
void BattleSetPrimFlagsTwoSlots(u8 arg0) {
    u16 value;
    int slotStride = 0x40;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = 1;
    D_800D2D28[0xB7] = 2;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask((D_800C3EAC + arg0 * slotStride)[0x3C]);
    func_800BC404(value);
    func_800BCD98(BattleSlotBitMask((D_800C3EAC + arg0 * slotStride)[0x3C]));
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BD50);
/* Best: 79.27%, 94 insns / 376 bytes = the original's exact size. Verified
   against retail: every parameter is used after a call (s5/s4/s3 saves
   match byte-for-byte), and the index `arg2 * 2 + arg1` must be written
   inline at every use -- a saved `off` local moves it to s0 and drops the
   s4/s3 saves, because retail rematerialises the index after the calls.
   Remaining diff is the base address: retail materialises D_800D2CE0 into
   a0 (lui/addiu) and adds the index with a register addu, and re-loads
   *p in the arg3==0 branch; gcc folds the base into the load operand
   ($at form, 3 insns instead of 4). Tried: p local, pBase local, (u32)
   cast with and without parens (parens fold straight back to $at, no
   parens give the register form but reassociate base+arg2*2+arg1), off
   local. Also unmatched: the arg3 branch is emitted inverted (retail
   `bnez arg3` with the *p path falling through).
s32 func_8008BD50(u8 arg0, u8 arg1, u8 arg2, u8 arg3) {
    u8 value;
    u16 entry;
    u8 *p;
    u8 *pBase;
    s32 result;

    result = 0;
    pBase = D_800D2CE0;
    p = pBase + (arg2 * 2 + arg1);
    entry = *(u16 *)(D_800D2204 + *p * 0x10);
    if (arg3 == 0) {
        value = *p;
    } else {
        value = D_800C3D70[arg2 * 2 + arg1];
    }
    if (value == 0) {
        BattlePlaySoundIfEnabled(0x4F);
    } else if (arg3 == 0) {
        BattleClearPrimFlagsTwoSlots(1);
        D_800D2D28[0xC6] = 1;
        if (func_80085084(entry, arg0, 0) != 0) {
            result = 1;
        } else {
            D_800D2D28[0xC6] = 0;
            BattleSetPrimFlagsTwoSlots(arg0);
        }
    } else {
        if (BattleIsCharacterInParty(value - 0x32) != 0) {
            func_8009A854(value);
            value = D_800D3688[arg2 * 2 + arg1] - 1;
            D_800D3688[arg2 * 2 + arg1] = value;
            if (value == 0) {
                D_800C3D70[arg2 * 2 + arg1] = 0;
            }
            result = 2;
        }
    }
    return result;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BED8);
void BattleFreeSlotPrims(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 != 0) {
        D_800D2D28[0xC6] = 0;
        func_8008FA60(0);
        func_8008FA60(1);
        BattleUpdateFrame();
        BattleFreePacketPrimitives();
        BattleClearFlagC6();
    } else {
        D_800D2D28[0xB0] = D_800D2D28[0xB1] = 0;
    }
}
/* Best: 99.78%. One addu left: the FIRST call's address add comes out
   index-first (`addu v1,s1,v1`) where the original has base-first
   (`addu v1,v1,s1`); the index s1 = arg0*0x40 lives across both calls.
   Same tail as BattleSetPrimFlagsFourSlots / BattleSetPrimFlagsTwoSlots / BattleSetPrimFlagsThreeSlots, which the
   permuter did fix there with a stride local -- here it only fixed the
   second call: a `char maskOffset` assigned 0x3C inline in its subscript
   `D_800C3EAC[arg0 * 0x40 + (maskOffset = 0x3C)]` (worth 99.36 -> 99.78).
   Tried: two more permute.sh runs (450 and 1105 iterations, score stuck at
   10), `0x3C + arg0 * 0x40`, `0x40 * arg0 + 0x3C`, plus everything the
   previous note listed: subscript, opt to the add, explicit u8* pEntry/pBase
   locals (worse), u32 integer arithmetic, a typed 0x40 struct pointer, and
   gcc 2.7.2 / 2.7.2-cdk / -O1 / -O3.
*/
/*
void func_8008C3F0(u8 arg0) {
    char maskOffset;
    u16 value;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB7] = 3;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = 1;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(D_800C3EAC[arg0 * 0x40 + 0x3C]);
    func_800BC404(value);
    func_800BCD98(BattleSlotBitMask(D_800C3EAC[arg0 * 0x40 + (maskOffset = 0x3C)]));
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C3F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C4A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C81C);
void BattleClearPrimFlagsThreeSlots(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
void BattleSetPrimFlagsThreeSlots(u8 arg0) {
    u16 value;
    int slotStride = 0x40;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = 1;
    D_800D2D28[0xB7] = 4;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask((D_800C3EAC + arg0 * slotStride)[0x3C]);
    func_800BC404(value);
    func_800BCD98(BattleSlotBitMask((D_800C3EAC + arg0 * slotStride)[0x3C]));
}

