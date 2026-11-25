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

// Best: 84.03% under the unit's XenoWorkList preset (gcc-2.7.2-cdk + maspsx --dont-expand-li), 152 bytes / 38 insns = retail's exact size and instruction multiset. Retail's mask 0x1FFFFFFF takes t0, the counter a3, old head t1, the 0x14 word a2, the 0x10 word v1; this body puts mask in t1, counter in t0, head in a3, the two field words in v1/a2 and the count load in a0 (retail v1) - a whole-body caller-saved rotation with the LOAD ORDER already identical (counter, head, 0x14, 0x10), so it is an allocno hand-out, not scheduling and not statement order. Round 485 measured: this park body 84.03; the same body with an old-head local 84.03 (cse propagates it away); -O3 84.03. Earlier rounds, under the old Default-flags preset, measured 12 statement orderings (unk10 first 73.34, id local 86.03, head local 89.34, original store order 69.37, sibling TimerWorkListAddTask field order 49.71, two-statement counter 73.37) and 4 minutes of permuter (best 89.74); preset screen 2.6.0 89.87, 2.7.2 89.34. The bitfield writes are right: unk10 gets the counter and keeps its top 3 bits, unk14 gets pData->unk10 and clears bits 29-31 one at a time. Do not re-spell this body; it needs an allocator probe.
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

// Best: 89.55% under the unit's XenoWorkList preset (gcc-2.7.2-cdk + maspsx --dont-expand-li), 232 bytes / 58 insns against retail's 240 / 60. Structure is right: one merged unk14-word RMW (value from pData->unk10 plus three one-at-a-time clears of bits 29/30/31), unk10 word gets the counter and keeps its top bits, the four mask constants sit at the top in retail's registers, both flag arms are present, head insert and count increments are exact. Two residuals, both named walls in tools/rounds/levers.md under the lever a-bitfield-merge-needs-adjacency: (1) retail loads the counter early (lw a3,gp right after the masks) and its unk14/unk10 words land in v0/v1, while this body loads the counter late into v0 and the flag lbu is hoisted above the unk14 store; (2) retail's taken arm stores through t3 and its else arm through v1, a copy made in the beqz delay slot, so cross-jumping leaves retail's two arm-tail stores separate - gcc here merges them into one shared store after the j and fills the beqz slot with the pNext store, and that merge is exactly the 2 missing instructions. Round 485 measured: this park 89.55; the same body with if (g_FlagTimerWorkListAdds) and no != 0 89.55 (identical emission); -O3 89.55. Earlier rounds: old park shape 55.23, unk10=id between group and clears 58.70, group after unk0 with inline post-increment 84.20, id local first 80.62, group after callbacks 60.73, unk10 statement next to the if 84.20 (tie); presets 2.6.0 79.7 and 2.7.2-cdk 83.72 under the default maspsx flags; a 4-minute permuter run found one better body (956 -> 806) that wrote the set through an UNINITIALISED pointer and was rejected as wrong code. The two arm-tail registers are not reachable from an honest spelling - park, do not re-spell.
/*
void TimerWorkListAddTask(WorkListEntry* pData, WorkListEntry* pEntry) {
    pEntry->unk0 = pData;
    pEntry->unk14 = pData->unk10;
    pEntry->unk14_1 = 0;
    pEntry->unk14_2 = 0;
    pEntry->unk14_3 = 0;
    pEntry->unk10 = g_WorkListNextTaskId++;
    pEntry->onTriggerCallback = NULL;
    pEntry->onFreeCallback = TimerWorkListRemoveTask;
    pEntry->pNext = g_TimerWorkList;
    g_TimerWorkList = pEntry;
    if (g_FlagTimerWorkListAdds != 0) {
        g_NumFlaggedTimerWorkListEntries++;
        pEntry->unk14_3 = 1;
    } else {
        pEntry->unk14_3 = 0;
    }
    g_NumTimerWorkListEntries++;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/work_list", TimerWorkListAddTask);


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

void SpriteAnimFrameListSetFrame(SpriteData* pSpriteData, u16 animFrame) {
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
}



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

