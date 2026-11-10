#include "common.h"

extern long func_800B57E4(u8* pActor);

// Split out of main/misc27 (see config/battle.yaml): func_800B5854 is the one
// function in the unit that is 100% under gcc 2.7.2 and 93.71% under -cdk.
void func_800B5854(u8* pTask) {
    u8* pActor;
    s32 prev;
    s32 dist;
    s32 speed;
    u8 flag;

    flag = 0;
    pActor = *(u8**)(pTask + 0x1C);
    prev = *(s32*)(pTask + 0x28);
    dist = func_800B57E4(pActor);
    *(s32*)(pTask + 0x28) = dist;
    if (prev < dist || dist < *(s32*)(pTask + 0x2C)) {
        flag = 1;
        speed = *(s32*)(pTask + 0x30);
        *(u16*)(pActor + 0x9E) = 1;
        *(s32*)(pActor + 0x64) = speed;
    }
    if (*(s16*)(pActor + 0x9E) == 0) {
        flag = 1;
    }
    if (*(s8*)(pActor + 0xAF) != *(s32*)(pTask + 0x24)) {
        flag = 1;
    }
    if (flag) {
        ((void (*)(u8*))*(void**)(pTask + 0xC))(pTask);
    }
}
