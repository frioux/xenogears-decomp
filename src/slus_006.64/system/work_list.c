#include "common.h"
#include "system/memory.h"
#include "system/animation.h"

// TU is compiled on GCC 2.7.2 or 2.6.0

// .sdata
s32 D_80059184; // Counter the entries' task IDs (unk10) are taken from
int g_NumTimerWorkListEntries;
int g_NumWorkListEntries;
SpriteData* D_80059190; // Head of the list of sprites with a changed animation frame

// .bss
WorkListEntry* D_800594C0; // Entry the update loops are currently processing
WorkListEntry* g_TimerWorkList;
WorkListEntry* D_80059590; // Entry the update loops will process next
WorkListEntry* g_WorkList;

// Declared in another TU, so these are not small data
extern u8 D_800591AC; // Read by TimerWorkListAddTask
extern int g_WorkListCurTimer;
extern s32 D_80059464; // Number of entries with the unk14_3 flag set
extern short D_80059494; // Set to 0 when g_WorkListCurTimer above reaches 0

void WorkListAddTask(WorkListEntry* pData, WorkListEntry* pEntry);
void WorkListRemoveTask(WorkListEntry* pTask);
void WorkListDeleteTask(WorkListEntry* pTask);
void TimerWorkListAddTask(WorkListEntry* pData, WorkListEntry* pEntry);
void TimerWorkListRemoveTask(WorkListEntry* pTask);
void TimerWorkListDeleteTask(WorkListEntry* pTask);

void WorkListsFreeAllEntries(void) {
    WorkListEntry* pEntry;

    pEntry = g_TimerWorkList;
    while (pEntry != NULL) {
        pEntry->onFreeCallback(pEntry);
        pEntry = g_TimerWorkList;
    }

    pEntry = g_WorkList;
    while (pEntry != NULL) {
        pEntry->onFreeCallback(pEntry);
        pEntry = g_WorkList;
    }
}

void WorkListsReset(void) {
    g_TimerWorkList = NULL;
    g_WorkList = NULL;
    g_NumTimerWorkListEntries = 0;
    g_NumWorkListEntries = 0;
    g_WorkListCurTimer = 0;
}

void TimerWorkListUpdate(void) {
    WorkListCallback_t pFnCallback;
    WorkListEntry* pEntry;

    // Count down timer
    if (g_WorkListCurTimer != 0) {
        g_WorkListCurTimer--;
        if (g_WorkListCurTimer == 0) {
            D_80059494 = 0;
        }

    // Timer has reached zero, run every timer entry
    } else {
        D_80059590 = g_TimerWorkList;
        while (D_80059590 != NULL) {
            pEntry = D_80059590;
            D_800594C0 = pEntry;
            D_80059590 = pEntry->pNext;
            pFnCallback = pEntry->onTriggerCallback;
            if (pFnCallback) {
                pFnCallback(pEntry);
            }
        }
    }
}

void WorkListUpdate(void) {
    WorkListCallback_t pFnCallback;
    WorkListEntry* pEntry;

    D_80059590 = g_WorkList;
    while (D_80059590 != NULL) {
        pEntry = D_80059590;
        D_800594C0 = pEntry;
        D_80059590 = pEntry->pNext;
        pFnCallback = pEntry->onTriggerCallback;
        if (pFnCallback) {
            pFnCallback(pEntry);
        }
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", WorkListAddTask);
/* Best: 89.34% (identical instruction multiset, 38 insns, same frame). Only the
   order of the four hoisted loads differs - original: counter, g_WorkList,
   0x14(a1), 0x10(a1); this: 0x14(a1), 0x10(a1), g_WorkList, counter - and the
   register assignment follows it (original has the unk10 word in v1, the unk14
   word in a2, the counter in a3, the old head in t1). Looks like a sched2
   ready-list order, not source order: 9 statement orderings tried (unk10 first
   73.34, id local 86.03, count before unk10 79.76, flags first 38.74,
   callbacks early 73.39, head local 89.34, unk0 last 82.47, unk14 first
   43.42). gcc 2.6.0 gives 89.87, 2.7.2-cdk 84.03, so it is not the preset.
void WorkListAddTask(WorkListEntry* pData, WorkListEntry* pEntry) {
    pEntry->unk0 = pData;
    pEntry->pNext = g_WorkList;
    g_WorkList = pEntry;
    pEntry->unk14 = pData->unk10;
    pEntry->unk14_1 = 0;
    pEntry->unk14_2 = 0;
    pEntry->unk14_3 = 0;
    pEntry->onTriggerCallback = NULL;
    pEntry->onFreeCallback = WorkListRemoveTask;
    pEntry->unk10 = D_80059184++;
    g_NumWorkListEntries++;
}
*/

WorkListEntry* WorkListAllocateTask(void* pData, int dataSize) {
    WorkListEntry* pEntry;

    pEntry = HeapAlloc(dataSize + sizeof(WorkListEntry), D_800591AF);
    WorkListAddTask(pData, pEntry);
    pEntry->onFreeCallback = WorkListDeleteTask;
    return pEntry;
}

void WorkListRemoveTask(WorkListEntry* pTargetEntry) {
    WorkListEntry* pPrevEntry;
    WorkListEntry* pCurEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_WorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry == pTargetEntry) {
            if (pPrevEntry != NULL) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_WorkList = pCurEntry->pNext;
            }

            if (D_80059590 == pTargetEntry) {
                D_80059590 = pTargetEntry->pNext;
            }

            break;
        }

        pPrevEntry = pCurEntry;
    }

    // Target entry was not found
    if (pCurEntry == NULL) {
        g_NumWorkListEntries++;
    }

    g_NumWorkListEntries--;
}

void WorkListDeleteTask(WorkListEntry* pTask) {
    WorkListRemoveTask(pTask);
    HeapFree(pTask);
}

// 58.60%: same shape as WorkListAddTask, plus the D_800591AC arm that counts
// the entry in D_80059464 and sets unk14_3.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", TimerWorkListAddTask);
/*
void TimerWorkListAddTask(WorkListEntry* pData, WorkListEntry* pEntry) {
    s32 id;

    id = D_80059184++;
    pEntry->unk0 = pData;
    pEntry->pNext = g_TimerWorkList;
    g_TimerWorkList = pEntry;
    pEntry->onTriggerCallback = NULL;
    pEntry->onFreeCallback = TimerWorkListRemoveTask;
    pEntry->unk14 = pData->unk10;
    pEntry->unk14_1 = 0;
    pEntry->unk14_2 = 0;
    pEntry->unk10 = id;
    if (D_800591AC != 0) {
        D_80059464++;
        pEntry->unk14_3 = 1;
    } else {
        pEntry->unk14_3 = 0;
    }
    g_NumTimerWorkListEntries++;
}
   Does not match yet: the same load-order/register-allocation problem as
   WorkListAddTask above (original loads the counter into a3 first, keeps the
   unk10 word in v1 and the unk14 word in v0, and copies pEntry to t3 because a1
   becomes the 0xE0000000 mask; here pEntry lands in t4 and a1 takes the unk10
   word). Four orderings tried, 31.6-58.6%. The D_800591AC arm itself does
   reproduce (lbu, lw plus at-store of D_80059464, or 0x80000000).
*/

WorkListEntry* TimerWorkListAllocateTask(void* pData, int dataSize) {
    WorkListEntry* pEntry;

    pEntry = HeapAlloc(dataSize + sizeof(WorkListEntry), D_800591AF);
    TimerWorkListAddTask(pData, pEntry);
    pEntry->onFreeCallback = TimerWorkListDeleteTask;
    pEntry->unk4 = NULL;
    return pEntry;
}

void WorkListSetTaskCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onTriggerCallback = callback;
}

void TimerWorkListSetTaskCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onTriggerCallback = callback;
}

void WorkListTaskSetOnFreeCallback(WorkListEntry* pTask, WorkListCallback_t callback) {
    pTask->onFreeCallback = callback;
}

WorkListCallback_t WorkListTaskGetTaskCallback(WorkListEntry* pTask) {
    return pTask->onTriggerCallback;
}

WorkListCallback_t WorkListTaskGetOnFreeCallback(WorkListEntry* pTask) {
    return pTask->onFreeCallback;
}

void TimerWorkListRemoveTask(WorkListEntry* pTargetEntry) {
    WorkListEntry* pPrevEntry;
    WorkListEntry* pCurEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry == pTargetEntry) {
            if (pPrevEntry != NULL) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_TimerWorkList = pCurEntry->pNext;
            }

            if (D_80059590 == pTargetEntry) {
                D_80059590 = pTargetEntry->pNext;
            }

            break;
        }

        pPrevEntry = pCurEntry;
    }

    if (pTargetEntry->unk14_3) {
        D_80059464--;
    }

    g_NumTimerWorkListEntries--;
}

void TimerWorkListDeleteTask(WorkListEntry* pTask) {
    TimerWorkListRemoveTask(pTask);
    HeapFree(pTask);
}

// Unlink target entry from lists if certain flags are met
void func_8001CE74(WorkListEntry* pTargetEntry) {
    WorkListCallback_t pFnOnDeleteCallback;
    WorkListEntry* pCurEntry;
    WorkListEntry* pPrevEntry;

    pPrevEntry = NULL;
    for (pCurEntry = g_WorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (
            pCurEntry->unk0 == pTargetEntry &&
            (pCurEntry->unk14_2 & 1) == 0 &&
            pCurEntry->unk14 == pTargetEntry->unk10
        ) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_WorkList = pCurEntry->pNext;
            }
            if (D_80059590 == pCurEntry) {
                D_80059590 = pCurEntry->pNext;
            }

            pFnOnDeleteCallback = pCurEntry->onFreeCallback;
            if (pFnOnDeleteCallback) {
                pFnOnDeleteCallback(pCurEntry);
            }
        } else {
            pPrevEntry = pCurEntry;
        }
    }

    pPrevEntry = NULL;
    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (
            pCurEntry->unk0 == pTargetEntry &&
            (pCurEntry->unk14_2 & 1) == 0 &&
            (pCurEntry->unk14 & 0x1FFFFFFF) == pTargetEntry->unk10
        ) {
            if (pPrevEntry) {
                pPrevEntry->pNext = pCurEntry->pNext;
            } else {
                g_TimerWorkList = pCurEntry->pNext;
            }
            if (D_80059590 == pCurEntry) {
                D_80059590 = pCurEntry->pNext;
            }

            pFnOnDeleteCallback = pCurEntry->onFreeCallback;
            if (pFnOnDeleteCallback) {
                pFnOnDeleteCallback(pCurEntry);
            }
        } else {
            pPrevEntry = pCurEntry;
        }
    }
}

// Set pCurEntry->unk4->unk70 of target entry if unk14_1 flag is set
// Set pCurEntry->unk4->unk70 of target entry if unk14_1 flag is set
void func_8001D034(WorkListEntry* pTargetEntry) {
    WorkListEntry* pCurEntry;

    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry->unk0 == pTargetEntry) {
            if (pCurEntry->unk14 == pTargetEntry->unk10 && (pCurEntry->unk14_1 & 1)) {
                *(u32*)(pCurEntry->unk4 + 0x70) = 0;
            }
        }
    }
}

// Find a timer entry started for pTargetEntry whose callback matches
WorkListEntry* func_8001D0A4(WorkListEntry* pTargetEntry, WorkListCallback_t pCallback) {
    WorkListEntry* pCurEntry;

    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (
            pCurEntry->unk0 == pTargetEntry &&
            pCurEntry->unk14 == pTargetEntry->unk10 &&
            pCurEntry->onTriggerCallback == pCallback
        ) {
            return pCurEntry;
        }
    }
    return NULL;
}

// Find the timer entry started for pTargetEntry
WorkListEntry* func_8001D10C(WorkListEntry* pTargetEntry) {
    WorkListEntry* pCurEntry;

    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry->unk0 == pTargetEntry && pCurEntry->unk14 == pTargetEntry->unk10) {
            return pCurEntry;
        }
    }
    return NULL;
}

// Find the timer entry running pCallback
WorkListEntry* func_8001D164(WorkListCallback_t pCallback) {
    WorkListEntry* pCurEntry;

    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry->onTriggerCallback == pCallback) {
            return pCurEntry;
        }
    }
    return NULL;
}

void WorkListsDeleteTasks(WorkListEntry* pTasks) {
    WorkListRemoveTask(pTasks + 1);
    TimerWorkListRemoveTask(pTasks);
    HeapFree(pTasks);
}

WorkListEntry* WorkListsAddTasks(u_int allocSize, void* pData, WorkListCallback_t pTimerCallback,
                                 WorkListCallback_t pWorkCallback, WorkListCallback_t pOnFreeCallback) {
    WorkListEntry* pEntry;
    WorkListEntry* pWorkEntry;

    pEntry = HeapAlloc(allocSize, D_800591AF);
    TimerWorkListAddTask(pData, pEntry);
    pWorkEntry = pEntry + 1;
    WorkListAddTask(pEntry, pWorkEntry);
    TimerWorkListSetTaskCallback(pEntry, pTimerCallback);
    WorkListSetTaskCallback(pWorkEntry, pWorkCallback);
    if (pOnFreeCallback != NULL) {
        WorkListTaskSetOnFreeCallback(pEntry, pOnFreeCallback);
    } else {
        WorkListTaskSetOnFreeCallback(pEntry, WorkListsDeleteTasks);
    }
    pEntry->unk4 = pEntry;
    pEntry[1].unk4 = pEntry; // pEntry[1] and not pWorkEntry, to keep pEntry in a register
    return pEntry;
}

// This may be the start of a new TU
void func_8001D298(void) {
    D_80059190 = 0;
}

void func_8001D2A4(void) {
    D_80059190 = 0;
}

// Change Sprite Animation Frame maybe?
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", func_8001D2B0);
/* Best: 97.28% (same insns, same frame, 328 vs 320 bytes). Every branch, delay
   slot and store order matches; the only difference is that the list cursor in
   the search loop lands in v0 here and v1 in the original (which rotates the two
   lui/addiu address constants into v1/a0 instead of v0/a0), plus one extra word.
   field_0x40 is tested with shift-and-mask (srl/andi) in the original, so write
   (x >> 20) & 1 rather than x & 0x100000; curSpriteFrame is read as u16 (lhu).
   Reading the old head into a local pOldHead and storing it last reproduces the
   original's tail store order (0x34, head, 0x40, 0x38).
void func_8001F8E8(SpriteData* pSpriteData, s32 index, void* pAnimPackage);
void func_800234AC(SpriteData* pSpriteData);
extern u8 D_8005A474[];
extern u8 D_8006BE10[];

void func_8001D2B0(SpriteData* pSpriteData, u16 animFrame) {
    SpriteData* pOldHead;
    SpriteData* pCurSpriteData;

    if ((pSpriteData->field_0x3C_0 & 3) != 1) {
        pSpriteData->curSpriteFrame = 0;
        return;
    }
    if ((pSpriteData->field_0x40 >> 20) & 1) {
        pSpriteData->field_0x40 &= ~0x100000;
        if (pSpriteData->pBase->pDirTransforms != NULL) {
            func_800234AC(pSpriteData);
        }
    }
    if ((pSpriteData->field_0x40 >> 17) & 1) {
        pCurSpriteData = D_80059190;
        while (pCurSpriteData != NULL) {
            if (pCurSpriteData == pSpriteData) {
                if (
                    pSpriteData->pVramData != &D_8005A474[0] &&
                    pSpriteData->pVramData != &D_8006BE10[0] &&
                    ((pSpriteData->field_0x40 >> 19) & 1) == 0
                ) {
                    func_8001F8E8(pSpriteData, *(u16*)&pSpriteData->curSpriteFrame, pSpriteData->pVramData);
                }
                pSpriteData->curSpriteFrame = animFrame;
                return;
            }
            pCurSpriteData = pCurSpriteData->pBase->pNextSpriteData;
        }
    }
    pOldHead = D_80059190;
    pSpriteData->curSpriteFrame = animFrame;
    D_80059190 = pSpriteData;
    pSpriteData->field_0x40 |= 0x20000;
    pSpriteData->pBase->pNextSpriteData = pOldHead;
} */


// Unlink SpriteData entry from list
void func_8001D3F4(SpriteData* pSpriteData) {
    SpriteData* pCurSpriteData;
    SpriteData* pPrevSpriteData;

    pPrevSpriteData = NULL;
    for (pCurSpriteData = D_80059190; pCurSpriteData != NULL;
         pCurSpriteData = pCurSpriteData->pBase->pNextSpriteData) {
        if (pCurSpriteData == pSpriteData) {
            if (pPrevSpriteData != NULL) {
                pPrevSpriteData->pBase->pNextSpriteData = pCurSpriteData->pBase->pNextSpriteData;
            } else {
                D_80059190 = pCurSpriteData->pBase->pNextSpriteData;
            }
        } else {
            pPrevSpriteData = pCurSpriteData;
        }
    }
}

void func_8001DAE8(SpriteData* pSpriteData, u16 animFrame, void* pVramData);

void func_8001D468(void) {
    SpriteData* pSpriteData;

    pSpriteData = D_80059190;
    while (pSpriteData != NULL) {
        // Read as unsigned: curSpriteFrame is s16 in the struct but -1 is a valid frame
        if (*(u16*)&pSpriteData->curSpriteFrame == 0) {
            pSpriteData->field_0x40 &= ~0xFC;
        } else {
            func_8001DAE8(pSpriteData, *(u16*)&pSpriteData->curSpriteFrame, pSpriteData->pVramData);
        }

        pSpriteData = pSpriteData->pBase->pNextSpriteData;
    }

    D_80059190 = NULL;
}

