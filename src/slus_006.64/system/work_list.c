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

// 89.34%: every instruction is right, but gcc emits the unk14/unk10 read-modify-
// write loads before the D_80059184 and g_WorkList loads instead of after them.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", WorkListAddTask);
/*
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
    pEntry->unk0 = pData;
    pEntry->pNext = g_TimerWorkList;
    g_TimerWorkList = pEntry;
    pEntry->unk10 = D_80059184++;
    pEntry->onTriggerCallback = NULL;
    pEntry->onFreeCallback = TimerWorkListRemoveTask;
    pEntry->unk14 = pData->unk10;
    pEntry->unk14_1 = 0;
    pEntry->unk14_2 = 0;
    if (D_800591AC != 0) {
        D_80059464++;
        pEntry->unk14_3 = 1;
    } else {
        pEntry->unk14_3 = 0;
    }
    g_NumTimerWorkListEntries++;
}
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

