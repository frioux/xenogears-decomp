#include "common.h"
#include "system/memory.h"
#include "system/animation.h"

// TU is compiled on GCC 2.7.2 or 2.6.0

// .sdata
s32 g_WorkListNextTaskId; // Counter the entries' task IDs (unk10) are taken from
int g_NumTimerWorkListEntries;
int g_NumWorkListEntries;
SpriteData* g_SpriteAnimFrameListHead; // Head of the list of sprites with a changed animation frame

// .bss
WorkListEntry* g_WorkListActiveEntry; // Entry the update loops are currently processing
WorkListEntry* g_TimerWorkList;
WorkListEntry* g_WorkListUpdateCursor; // Entry the update loops will process next
WorkListEntry* g_WorkList;

// Declared in another TU, so these are not small data
extern u8 g_FlagTimerWorkListAdds; // Read by TimerWorkListAddTask
extern int g_WorkListCurTimer;
extern s32 g_NumFlaggedTimerWorkListEntries; // Number of entries with the unk14_3 flag set
extern short g_BattleWaitFrameCountdown; // Set to 0 when g_WorkListCurTimer above reaches 0

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
            g_BattleWaitFrameCountdown = 0;
        }

    // Timer has reached zero, run every timer entry
    } else {
        g_WorkListUpdateCursor = g_TimerWorkList;
        while (g_WorkListUpdateCursor != NULL) {
            pEntry = g_WorkListUpdateCursor;
            g_WorkListActiveEntry = pEntry;
            g_WorkListUpdateCursor = pEntry->pNext;
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

    g_WorkListUpdateCursor = g_WorkList;
    while (g_WorkListUpdateCursor != NULL) {
        pEntry = g_WorkListUpdateCursor;
        g_WorkListActiveEntry = pEntry;
        g_WorkListUpdateCursor = pEntry->pNext;
        pFnCallback = pEntry->onTriggerCallback;
        if (pFnCallback) {
            pFnCallback(pEntry);
        }
    }
}

/* Best: 89.34% (152 bytes, 38 insns, identical instruction multiset and frame).
   Only the order of the four hoisted loads differs - original: counter, g_WorkList,
   0x14(a1), 0x10(a1); this: 0x14(a1), 0x10(a1), g_WorkList, counter - and the register
   assignment follows it (original: unk10 word in v1, unk14 word in a2, counter in a3,
   old head in t1), so it is a sched2 ready-list order. 12 statement orderings measured
   (unk10 first 73.34, id local 86.03, count before unk10 79.76, flags first 38.74,
   callbacks early 73.39, head local 89.34, unk0 last 82.47, unk14 first 43.42, the
   original store order 69.37). Not the preset: 2.6.0 89.87, 2.7.2-cdk 84.03, cdk with
   --dont-expand-li 84.03. A 4-minute permuter run from this body found only 89.74 (it
   holds the NULL for the onTriggerCallback store in a local and stores it after the
   counter increment). The bitfield writes are right: unk10 gets the counter and keeps
   its top 3 bits, unk14 gets pData->unk10 and clears bits 29-31 one and at a time,
   exactly retail. */
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
    pEntry->unk10 = g_WorkListNextTaskId++;
    g_NumWorkListEntries++;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", WorkListAddTask);

WorkListEntry* WorkListAllocateTask(void* pData, int dataSize) {
    WorkListEntry* pEntry;

    pEntry = HeapAlloc(dataSize + sizeof(WorkListEntry), g_TaskHeapAllocFlags);
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

            if (g_WorkListUpdateCursor == pTargetEntry) {
                g_WorkListUpdateCursor = pTargetEntry->pNext;
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

// 58.60%: same shape as WorkListAddTask, plus the g_FlagTimerWorkListAdds arm that counts
// the entry in g_NumFlaggedTimerWorkListEntries and sets unk14_3.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", TimerWorkListAddTask);
/*
void TimerWorkListAddTask(WorkListEntry* pData, WorkListEntry* pEntry) {
    s32 id;

    id = g_WorkListNextTaskId++;
    pEntry->unk0 = pData;
    pEntry->pNext = g_TimerWorkList;
    g_TimerWorkList = pEntry;
    pEntry->onTriggerCallback = NULL;
    pEntry->onFreeCallback = TimerWorkListRemoveTask;
    pEntry->unk14 = pData->unk10;
    pEntry->unk14_1 = 0;
    pEntry->unk14_2 = 0;
    pEntry->unk10 = id;
    if (g_FlagTimerWorkListAdds != 0) {
        g_NumFlaggedTimerWorkListEntries++;
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
   word). Four orderings tried, 31.6-58.6%. The g_FlagTimerWorkListAdds arm itself does
   reproduce (lbu, lw plus at-store of g_NumFlaggedTimerWorkListEntries, or 0x80000000).
*/

WorkListEntry* TimerWorkListAllocateTask(void* pData, int dataSize) {
    WorkListEntry* pEntry;

    pEntry = HeapAlloc(dataSize + sizeof(WorkListEntry), g_TaskHeapAllocFlags);
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

            if (g_WorkListUpdateCursor == pTargetEntry) {
                g_WorkListUpdateCursor = pTargetEntry->pNext;
            }

            break;
        }

        pPrevEntry = pCurEntry;
    }

    if (pTargetEntry->unk14_3) {
        g_NumFlaggedTimerWorkListEntries--;
    }

    g_NumTimerWorkListEntries--;
}

void TimerWorkListDeleteTask(WorkListEntry* pTask) {
    TimerWorkListRemoveTask(pTask);
    HeapFree(pTask);
}

// Unlink target entry from lists if certain flags are met
void WorkListsRemoveEntriesForData(WorkListEntry* pTargetEntry) {
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
            if (g_WorkListUpdateCursor == pCurEntry) {
                g_WorkListUpdateCursor = pCurEntry->pNext;
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
            if (g_WorkListUpdateCursor == pCurEntry) {
                g_WorkListUpdateCursor = pCurEntry->pNext;
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

// Clear pCurEntry->unk4->field_0x70 of the target entry if its unk14_1 flag is set
void TimerWorkListClearDataField70(WorkListEntry* pTargetEntry) {
    WorkListEntry* pCurEntry;

    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry->unk0 == pTargetEntry) {
            if (pCurEntry->unk14 == pTargetEntry->unk10 && (pCurEntry->unk14_1 & 1)) {
                ((SpriteData*)pCurEntry->unk4)->field_0x70 = NULL;
            }
        }
    }
}

// Find a timer entry started for pTargetEntry whose callback matches
WorkListEntry* TimerWorkListFindEntryForDataAndCallback(WorkListEntry* pTargetEntry, WorkListCallback_t pCallback) {
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
WorkListEntry* TimerWorkListFindEntryForData(WorkListEntry* pTargetEntry) {
    WorkListEntry* pCurEntry;

    for (pCurEntry = g_TimerWorkList; pCurEntry != NULL; pCurEntry = pCurEntry->pNext) {
        if (pCurEntry->unk0 == pTargetEntry && pCurEntry->unk14 == pTargetEntry->unk10) {
            return pCurEntry;
        }
    }
    return NULL;
}

// Find the timer entry running pCallback
WorkListEntry* TimerWorkListFindEntryByCallback(WorkListCallback_t pCallback) {
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

    pEntry = HeapAlloc(allocSize, g_TaskHeapAllocFlags);
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
void SpriteAnimFrameListReset(void) {
    g_SpriteAnimFrameListHead = NULL;
}

void SpriteAnimFrameListClear(void) {
    g_SpriteAnimFrameListHead = NULL;
}

void SpriteClearDirTransforms(SpriteData* pSpriteData);
void SpriteApplyFrameDirTransforms(SpriteData* pSpriteData, s32 index, void* pVramData);
extern SpriteAnimPackage g_SpriteAnimPackageDirs6;
extern SpriteAnimPackage g_SpriteAnimPackageDirs5;

// Change Sprite Animation Frame maybe?
/* Best: 97.53% under the unit flags (328 bytes, retail 324). PRESET: this body is
   INSTRUCTION-IDENTICAL to retail under gcc-2.7.2-cdk-psx + maspsx --dont-expand-li
   (100.00, verified by objdump side-by-side). Retail has two things plain 2.7.2 cannot
   emit: (a) the address pair is lui $v0,%hi in the beqz delay slot then addiu a1,v0,%lo,
   i.e. the high and low halves are SEPARATE RTL insns with different registers, which
   only cdk produces (plain 2.7.2 ties %hi to its destination and leaves a nop, one
   instruction long); (b) retail assembles li 1 as addiu $v1,$zero,1 where maspsx
   expand_li gives ori. Under the unit flags the best measured residual is 97.53; the
   body itself is right. field_0x40 is tested with srl+andi, curSpriteFrame is read as
   u16. Tried and measured worse: plain while, guard+do/while, head read twice, 2.6.0
   (94.44), 2.7.2 + --dont-expand-li (98.27, one instruction short). The duplicated
   if (animFrame) advance is a permuter find that put the list cursor in v1; its arms
   are identical on purpose. */
/*
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
            SpriteClearDirTransforms(pSpriteData);
        }
    }
    if ((pSpriteData->field_0x40 >> 17) & 1) {
        pCurSpriteData = g_SpriteAnimFrameListHead;
        if (pCurSpriteData != NULL) {
          do {
            if (pCurSpriteData == pSpriteData) {
                if (
                    pSpriteData->pVramData != &g_SpriteAnimPackageDirs6 &&
                    pSpriteData->pVramData != &g_SpriteAnimPackageDirs5 &&
                    ((pSpriteData->field_0x40 >> 19) & 1) == 0
                ) {
                    SpriteApplyFrameDirTransforms(pSpriteData, *(u16*)&pSpriteData->curSpriteFrame, pSpriteData->pVramData);
                }
                pSpriteData->curSpriteFrame = animFrame;
                return;
            }
            if (animFrame) {
                pCurSpriteData = pCurSpriteData->pBase->pNextSpriteData;
            } else {
                pCurSpriteData = pCurSpriteData->pBase->pNextSpriteData;
            }
          } while (pCurSpriteData != NULL);
        }
    }
    pOldHead = g_SpriteAnimFrameListHead;
    pSpriteData->curSpriteFrame = animFrame;
    g_SpriteAnimFrameListHead = pSpriteData;
    pSpriteData->field_0x40 |= 0x20000;
    pSpriteData->pBase->pNextSpriteData = pOldHead;
} */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", func_8001D2B0);


// Unlink SpriteData entry from list
void SpriteAnimFrameListRemove(SpriteData* pSpriteData) {
    SpriteData* pCurSpriteData;
    SpriteData* pPrevSpriteData;

    pPrevSpriteData = NULL;
    for (pCurSpriteData = g_SpriteAnimFrameListHead; pCurSpriteData != NULL;
         pCurSpriteData = pCurSpriteData->pBase->pNextSpriteData) {
        if (pCurSpriteData == pSpriteData) {
            if (pPrevSpriteData != NULL) {
                pPrevSpriteData->pBase->pNextSpriteData = pCurSpriteData->pBase->pNextSpriteData;
            } else {
                g_SpriteAnimFrameListHead = pCurSpriteData->pBase->pNextSpriteData;
            }
        } else {
            pPrevSpriteData = pCurSpriteData;
        }
    }
}

void func_8001DAE8(SpriteData* pSpriteData, u16 animFrame, void* pVramData);

void SpriteAnimFrameListFlush(void) {
    SpriteData* pSpriteData;
    u16 animFrame;

    pSpriteData = g_SpriteAnimFrameListHead;
    while (pSpriteData != NULL) {
        // Read as unsigned: curSpriteFrame is s16 in the struct but -1 is a valid frame
        animFrame = (u16)pSpriteData->curSpriteFrame;
        if (animFrame == 0) {
            pSpriteData->field_0x40 &= ~0xFC;
        } else {
            func_8001DAE8(pSpriteData, animFrame, pSpriteData->pVramData);
        }

        pSpriteData = pSpriteData->pBase->pNextSpriteData;
    }

    g_SpriteAnimFrameListHead = NULL;
}

