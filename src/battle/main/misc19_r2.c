#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

extern BattleWorkBlock* g_BattleCurWorkBlock;

// 100.00% only under gcc-2.6.0 (88.68% on 2.7.2): 2.6.0 puts each `/20` mul by
// 0xCCCCCCCD first in the guarded block and mfhi in $v0, which is retail's order.
// 0x164 bytes against retail's 0x15c: the body is 8 bytes too long, so it cannot
// be converted even if it diffs clean (see the size check in AGENTS.md).
// PARKED (88.68%). Body COMPILES (park triage, round 111) and is 356 bytes
// against retail's 348: 8 bytes too long, so it cannot be converted even if it
// reached 100. See the note above for the /20 ordering difference.
// Round 397 re-measured live: 88.68% (356 bytes, 89 insns vs retail 87) under the unit's
// 2.7.2, and 100.0 under gcc-2.6.0-psx at -O2 -G8 (try_flags), confirming the note below.
// The whole 2.7.2 residual per '/20' block is the 2.6.0 ordering: retail fills the branch
// delay slot with the constant's `lui`, keeps the magic in v1 with the loaded value in v0,
// and does `mfhi v0 / srl v0,v0,0x4` in place, where 2.7.2 emits a nop delay slot, the lhu
// first, the constant in v0, and `mfhi a3 / srl v1,a3,0x4`. Splitting the load from the
// division (val = p[0x4E]; val = val / 20;) measured no change. The unit is mixed under
// 2.6.0 (round 393 screen: BattleClearSlotFlag15A1 95.71, BattleFillPartySlotWeaponRecord
// 96.15), so like func_8009A9D0 this is a one-function XenoLibrary carve-out candidate.
// Round 426 re-confirmed both numbers with try_flags, and screened the whole unit under
// gcc-2.6.0-psx (`--all`): 9 of the 13 already-matched functions drop below 100.0
// (BattleFillPartySlotWeaponRecord 96.15, BattleClearSlotFlag15A1 95.71,
// BattleInitItemInventoryAndConditionMasks 99.38, BattleSetSlotFlag15A1 99.57,
// BattleResetSlotEffectWords 99.64, BattleSetSlotEntryFieldsTo1 99.71,
// BattleSetSlotEntryFieldsTo2000 99.77, BattleDecrementGearPartUse 99.79,
// BattleSetCommandCodeForSlotKindFlag 99.96), so the unit cannot carry 2.6.0 and this
// needs a per-function split, not a source change. Park is final under 2.7.2.
// Round 439 re-measured live: 88.68% (356 bytes, 89 insns vs retail 87). Swept the whole
// preset matrix (try_flags --sweep: 2.7.2 and 2.7.2-cdk at -O1/-O2/-O3 x -G4/-G8 x
// -mcpu 3000/r8000, best 88.68) and 2.7.2 + maspsx --expand-div (88.68, no change: 2.7.2
// does the magic multiply itself, so expand-div never sees a div). gcc-2.6.0 is the only
// compiler at 100.0 (every -O/-G/-mcpu row). So this is a one-function 2.6.0 carve-out,
// i.e. a config/battle.yaml split with a new src/battle/main/misc19_r2.c - not a source
// change; the 2.7.2 body is 8 bytes too long and its div expansion is in gcc's own
// expand_divmod. Re-measured, park refreshed, no C change.
/*
s32 func_8009ADA0(u8 arg0, u32* arg1) {
    u8* p = arg0 * 0x170 + (u32)g_BattleCurWorkBlock;
    u16 f = *(u16*)(p + 0x7C);
    u16 val;
    s32 ret = 0;

    if (f & 0x8000) {
        return 0;
    }
    if (f & 0x800) {
        val = *(u16*)(p + 0x4E) / 20;
        *arg1 = val;
        ret = 1;
    }
    arg1++;
    if (*(u16*)(p + 0x80) & 0x200) {
        val = *(u16*)(p + 0x52) / 20;
        *arg1 = val;
        ret = 1;
    }
    if (*(u16*)(p + 0x120) & 0x200) {
        val = *(u16*)(p + 0x52) / 20;
        *arg1 = val;
        ret = 1;
    }
    arg1++;
    *arg1 = 0;
    if (*(u16*)(p + 0x120) & 0x80) {
        val = *(u16*)(p + 0xDE) / 50;
        *arg1 = val;
        ret = 1;
    }
    if (*(u16*)(p + 0x124) & 0x8000) {
        val = *(u16*)(p + 0xDE) / 50;
        *arg1 += val;
        ret = 1;
    }
    return ret;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19_r2", func_8009ADA0);
