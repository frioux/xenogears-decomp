/* Example/proof snippet for tools/scripts/cc1_probe.sh; not part of any build.
 *
 *   tools/scripts/cc1_probe.sh battle/main/misc35 tools/scripts/probe_example.c
 *
 * ProbeArm is the shape retail has in BattleDrawActorLinePrim (battle/misc27_b):
 * the extern's address is taken at the use site inside one arm, so cc1 ties %hi
 * and %lo into the same register (`lui $4,%hi(g_Probe)` / `addiu $4,$4,%lo`).
 * ProbeStoreFirst stores through the extern in the same block, so the store's
 * register already holds the %hi and cc1 emits the low half into another one
 * (`lui $16,%hi` / `addiu $4,$16,%lo`) - the "lui v0 / addiu s0,v0" split. */
#include "common.h"

extern s32 g_Probe;
extern void ProbeTake(s32 *p);

s32 ProbeArm(s32 c) {
    if (c != 0) {
        ProbeTake(&g_Probe);
    }
    return g_Probe;
}

s32 ProbeStoreFirst(s32 c) {
    g_Probe = c;
    ProbeTake(&g_Probe);
    return g_Probe;
}
