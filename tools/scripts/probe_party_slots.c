/* Probe: which spelling of g_X[1] / g_X[0] with CONSTANT indices keeps retail's
 * bare symbol+const MEM (lui into the load's destination, lui $at for the
 * store) instead of cse-ing &g_X[1] into a register. */
#include "common.h"

typedef struct {
    s32 slot0;
    s32 slot1;
    s32 slot2;
} ProbeSlots;

extern s32 g_ProbeMembers[3];
extern s32 g_ProbeSkins[3];
extern ProbeSlots g_ProbeStMembers;
extern ProbeSlots g_ProbeStSkins;
extern s32 g_MemberSlot1 __asm__("g_ProbeMembers+4");
extern s32 g_SkinSlot1 __asm__("g_ProbeSkins+4");
extern s32 g_ProbeGameState;

void ProbeArray(void) {
    s32 m = g_ProbeMembers[1];
    s32 s = g_ProbeSkins[1];
    g_ProbeMembers[1] = 255;
    g_ProbeSkins[1] = 255;
    g_ProbeMembers[0] = m;
    g_ProbeSkins[0] = s;
}

void ProbeStruct(void) {
    s32 m = g_ProbeStMembers.slot1;
    s32 s = g_ProbeStSkins.slot1;
    g_ProbeStMembers.slot1 = 255;
    g_ProbeStSkins.slot1 = 255;
    g_ProbeStMembers.slot0 = m;
    g_ProbeStSkins.slot0 = s;
}

void ProbeAlias(void) {
    s32 m = g_MemberSlot1;
    s32 s = g_SkinSlot1;
    g_MemberSlot1 = 255;
    g_SkinSlot1 = 255;
    g_ProbeMembers[0] = m;
    g_ProbeSkins[0] = s;
}
