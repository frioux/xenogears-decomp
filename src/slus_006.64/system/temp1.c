#include "common.h"
#include "system/menu.h"
#include "system/animation.h"

// Sprite / Animation functions

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022B2C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022CAC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022CDC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022D44);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022DF4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022E8C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022EB8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022FC4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_8002303C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800230A8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023124);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023170);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800231E0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800231F8);

extern int D_80059198;
extern void func_800248D4(SpriteData* pSpriteData);

void AnimScriptTick(SpriteData* pSpriteData) {
    int i;

    for (i = 0; i != D_80059198 + 1; i++) {
        if (pSpriteData->animScriptWaitTimer) {
            pSpriteData->animScriptWaitTimer--;
            if (pSpriteData->animScriptWaitTimer == 0) {
                func_800248D4(pSpriteData);
            }
        }
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023290);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023340);

// Allocate struct stuff
AnimTask* func_800233A4(void* pData, int dataSize) {
    AnimTask* pEntry;
    WorkListEntry* pTask1;
    WorkListEntry* pTask2;

    pEntry = HeapAlloc(dataSize + 0xEC, D_800591AF);
    pTask1 = &pEntry->task1;
    pTask2 = &pEntry->task2;
    TimerWorkListAddTask(pData, pTask1);
    WorkListAddTask(pEntry, pTask2);
    func_80023804(&pEntry->spriteData);
    pTask1->unk4 = &pEntry->spriteData;
    pTask2->unk4 = &pEntry->spriteData;
    TimerWorkListSetTaskCallback(pEntry, &func_80022DF4);
    WorkListTaskSetOnFreeCallback(pEntry, &func_80022EB8);
    return pEntry;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023440);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023468);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800234AC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023538);

// SpriteData stuff
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023804);



void func_8002393C(SpriteDataI1* pBase) {
    pBase->rotation.x = 0;
    pBase->rotation.y = 0;
    pBase->rotation.z = 0;
    pBase->pFramesData = NULL;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023950);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023958);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800239A0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800239F4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023A48);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023B84);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023FD8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80024294);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800242F4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_8002435C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80024524);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_800245D8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80024730);
