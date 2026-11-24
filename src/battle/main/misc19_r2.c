#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

extern BattleWorkBlock* g_BattleCurWorkBlock;

// This one function is its own TU because it is byte-identical only under gcc 2.6.0
// (the XenoLibrary preset in gears.toml): under 2.7.2 its own expand_divmod shapes all
// four of the `/20` and `/50` divisions differently and the body comes out 8 bytes too
// long. The rest of misc19 is Default, so it cannot carry the older compiler.
s32 BattleFillSlotEffectAmounts(u8 arg0, u32* arg1) {
    BattleTableEntry* p = &g_BattleCurWorkBlock->slots[arg0];
    u16 f = p->activeEffectFlags1;
    u16 val;
    s32 ret = 0;

    if (f & 0x8000) {
        return 0;
    }
    if (f & 0x800) {
        val = p->maxHp / 20;
        *arg1 = val;
        ret = 1;
    }
    arg1++;
    if (p->activeEffectFlags2 & 0x200) {
        val = p->maxMp / 20;
        *arg1 = val;
        ret = 1;
    }
    if (p->state.activeEffectFlags1 & 0x200) {
        val = p->maxMp / 20;
        *arg1 = val;
        ret = 1;
    }
    arg1++;
    *arg1 = 0;
    if (p->state.activeEffectFlags1 & 0x80) {
        val = p->state.maxFuel / 50;
        *arg1 = val;
        ret = 1;
    }
    if (p->state.activeEffectFlags2 & 0x8000) {
        val = p->state.maxFuel / 50;
        *arg1 += val;
        ret = 1;
    }
    return ret;
}
