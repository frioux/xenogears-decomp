#include "common.h"
#include "field/actor.h"

extern s32 func_800B57E4(SpriteData* pSpriteData);

// WorkListEntry of include/system/animation.h; an overlay unit cannot include that
// header (it holds tentative definitions of main-exe globals), so the layout is
// repeated as in main/misc27_b.
typedef struct WorkListEntry {
    /* 0x00 */ void* unk0;
    /* 0x04 */ void* pSpriteData;
    /* 0x08 */ void* onTriggerCallback;
    /* 0x0C */ void (*onFreeCallback)(void*);
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ struct WorkListEntry* pNext;
} WorkListEntry; // Size: 0x1C

// The distance-wait task of main/misc27_b's BattleCreateActorDistanceWaitTask, which
// hangs func_800B5854 on its timer entry: the sprite to watch, the animation it plays
// while the task waits, the distance measured last frame, the bound that ends the wait
// and the sprite bytecode the task resumes from.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ SpriteData* pSpriteData;
    /* 0x20 */ u8 unk20[0x4];
    /* 0x24 */ s32 animId;
    /* 0x28 */ s32 distance;
    /* 0x2C */ s32 minDistance;
    /* 0x30 */ void* pSpriteBytecode;
} BattleDistanceWaitTask; // Size: 0x34

// Split out of main/misc27 (see config/battle.yaml): func_800B5854 is the one
// function in the unit that is 100% under gcc 2.7.2 and 93.71% under -cdk.
void func_800B5854(BattleDistanceWaitTask* pTask) {
    SpriteData* pSpriteData;
    s32 prevDistance;
    s32 distance;
    void* pSpriteBytecode;
    u8 flag;

    flag = 0;
    pSpriteData = pTask->pSpriteData;
    prevDistance = pTask->distance;
    distance = func_800B57E4(pSpriteData);
    pTask->distance = distance;
    if (prevDistance < distance || distance < pTask->minDistance) {
        flag = 1;
        pSpriteBytecode = pTask->pSpriteBytecode;
        pSpriteData->animScriptWaitTimer = 1;
        pSpriteData->pSpriteBytecode = pSpriteBytecode;
    }
    if (pSpriteData->animScriptWaitTimer == 0) {
        flag = 1;
    }
    if ((s8)pSpriteData->flags_0xAC.bytes[3] != pTask->animId) {
        flag = 1;
    }
    if (flag) {
        pTask->timerEntry.onFreeCallback(pTask);
    }
}
