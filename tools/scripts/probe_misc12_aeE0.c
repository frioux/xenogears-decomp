/* Probe snippet for misc12 func_8009AEE0 (not part of any build).
 *
 *   tools/scripts/cc1_probe.sh field/main/misc12 tools/scripts/probe_misc12_aeE0.c
 *
 * ProbeIdxAdd is the parked spelling (index written first, pointer arithmetic);
 * ProbeIdxSub is the plain subscript. Retail (2B468) is `lui/lw g_FieldActors`
 * then `addu v1,v0,v1` - index in the FIRST source, base destroyed in place.
 * ProbeTail* is retail's arrival tail (2B534): it wants the mask constants, then
 * `move v0,zero`, then the flags `lw`, with the position sll/sw after that.
 */
#include "common.h"
#include "field/actor.h"

extern s32 g_FieldPartyActorSlots[];

s32 ProbeIdxAdd(s32 slot) {
    int index = g_FieldPartyActorSlots[slot];
    FieldActor* pActor = index + g_FieldActors;
    return pActor->status & 0x20;
}

s32 ProbeIdxSub(s32 slot) {
    int index = g_FieldPartyActorSlots[slot];
    FieldActor* pActor = &g_FieldActors[index];
    return pActor->status & 0x20;
}

s32 ProbeTailA(ActorData* pActor, s32 x, s32 z) {
    pActor->position.vx = x << 16;
    pActor->position.vz = z << 16;
    pActor->stuckFrameCount = 0;
    pActor->scriptFlags.flags &= ~0x02200800;
    return 0;
}

s32 ProbeTailB(ActorData* pActor, s32 x, s32 z) {
    u_int flags = pActor->scriptFlags.flags;

    pActor->position.vx = x << 16;
    pActor->position.vz = z << 16;
    pActor->stuckFrameCount = 0;
    pActor->scriptFlags.flags = flags & ~0x02200800;
    return 0;
}
