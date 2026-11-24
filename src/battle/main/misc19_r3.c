#include "common.h"
#include "battle/actor.h"

extern BattleWorkBlock* g_BattleCurWorkBlock;

extern int rand(void);
extern void func_8009BE0C(void);

// This one function is its own TU because it is byte-identical only under gcc 2.6.0
// (the XenoLibrary preset in gears.toml): under 2.7.2 the signed `rand() % 100` comes
// out `mfhi a1 / sra a0,a1,5` where retail has `mfhi a0 / sra a0,a0,5`, the only
// residual at 99.66% (116 bytes, 29/29 insns). The rest of misc19 is Default, so it
// cannot carry the older compiler.
s32 BattleResetRowIndexAndMaybeRefreshStats(void) {
    g_BattleCurWorkBlock->rowIndex = 0;
    if (rand() % 100 < 50) {
        func_8009BE0C();
        return 1;
    }
    return 0;
}

