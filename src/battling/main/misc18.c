#include "common.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4; // Size: 0x10

struct BattleActor;

// The flags word at +0x2C of a BattleWork. The state setup code sets one bit at
// a time, two call sites test several bits of the whole word, and +0x2E is
// stored as a byte, so the word is exposed three ways.
typedef union {
    /* 0x0 */ u32 flags;
    /* 0x0 */ struct {
        /* 0x0 */ u32 random : 8;
        /* 0x0 */ u32 unk2C_8 : 1;
        // The outcome of the rand() 0xFF roll func_8008F7B8 takes against each
        // of the work's action chances.
        /* 0x0 */ u32 actionChance0Roll : 1;
        /* 0x0 */ u32 actionChance1Roll : 1;
        /* 0x0 */ u32 actionChance2Roll : 1;
        /* 0x0 */ u32 unk2C_12 : 1;
        // BattleDispatchWorkState copies this bit into the actor's flags bit 1.
        /* 0x0 */ u32 setFlag2 : 1;
        /* 0x0 */ u32 pad2C : 18;
    } bits;
    /* 0x0 */ struct {
        /* 0x0 */ u16 pad2C;
        /* 0x2 */ u8 unk2E;
        /* 0x3 */ u8 pad2F;
    } bytes;
} BattleWorkFlags; // Size: 0x4

typedef struct {
    /* 0x00 */ struct BattleActor* pOwner;
    /* 0x04 */ s16 delay;
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ u8 state;
    /* 0x09 */ u8 ticks;
    /* 0x0A */ s16 targetAngle;
    /* 0x0C */ s16 maxDistance;
    /* 0x0E */ u8 setFlag8000;
    /* 0x0F */ u8 difficulty;
    /* 0x10 */ s32 actionChance0;
    /* 0x14 */ s32 actionChance1;
    /* 0x18 */ s32 actionChance2;
    /* 0x1C */ s32 actionChance3;
    // The sub-state of state 0: 0 waits out the delay and then may attack,
    // 1 waits until the actor drops below its break threshold (func_8008FCC8).
    /* 0x20 */ s32 phase;
    /* 0x24 */ s32 breakThreshold;
    // The 2D distance the state-2 handler waits for before it takes the action
    // (rand() % 1536 + 0x100 at state entry, compared with D_8009284C).
    /* 0x28 */ s32 actionDistance;
    /* 0x2C */ BattleWorkFlags flags;
    // The owner's moveCount as of the last func_8008F7B8, which the state-0
    // handler compares with the live one.
    /* 0x30 */ u16 savedMoveCount;
    /* 0x32 */ u16 pad32;
} BattleWork; // Size: 0x34

// The record the actor's four action probabilities are copied from.
typedef struct {
    /* 0x0 */ u8 unk0[0x6];
    /* 0x6 */ u8 actionChances[0x4];
    /* 0xA */ u8 actionEnabled[0xE];
} BattleActionData; // Size: 0x18

// The message ids an action queues, indexed by action; 0 means none.
typedef struct {
    /* 0x0 */ u8 messages[3];
} BattleActionMessages; // Size: 0x3

// One entry of an actor's per-action state; only the flag byte is read (clear
// means BattleQueueActionMessages skips the action).
typedef struct {
    /* 0x0 */ u8 unk0[0x3];
    /* 0x3 */ u8 flags;
} BattleActionSlot; // Size: 0x4

// A battle participant as this unit drives it. Only the fields this unit
// touches are named; 0x1668 is the last one, so 0x166A is a lower bound on the
// real size.
typedef struct BattleActor {
    /* 0x000 */ Vec4 position;
    /* 0x010 */ u8 unk10[0x38];
    /* 0x048 */ u32 distance;
    /* 0x04C */ u8 unk4C[0xC];
    /* 0x058 */ s32 unk58;
    /* 0x05C */ u8 unk5C[0x24];
    /* 0x080 */ BattleActionSlot* pActionSlots;
    /* 0x084 */ u8 unk84[0x30];
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    /* 0x0B8 */ u8 unkB8[0x6];
    /* 0x0BE */ s16 breakThresholdOffset;
    /* 0x0C0 */ u8 unkC0[0x4];
    /* 0x0C4 */ u8 state;
    /* 0x0C5 */ u8 actionPhase;
    /* 0x0C6 */ u8 unkC6[0xA];
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 unkD4[0x4];
    /* 0x0D8 */ struct BattleActor* pTarget;
    /* 0x0DC */ u8 unkDC[0x818];
    /* 0x8F4 */ u32 unk8F4;
    /* 0x8F8 */ u8 unk8F8[0x19];
    /* 0x911 */ u8 actionCount;
    /* 0x912 */ u8 unk912[0xCEA];
    /* 0x15FC */ BattleWork* pWork;
    /* 0x1600 */ BattleActionData* pActionData;
    /* 0x1604 */ u8 unk1604[0x64];
    // Bumped by func_80075B50 each time it starts a move for the actor; the round
    // reset clears it.
    /* 0x1668 */ u16 moveCount;
} BattleActor; // Size: 0x166A

extern s32 func_80073DE4(BattleActor* p, s32 arg1);
extern void BattleTryQueueMessage5(BattleActor* p);
extern void BattleQueueMessage(BattleActor* p, s32 arg1);
extern u8 g_BattleOptionOn;
extern u32 BattleGetGridCellAtPosition(Vec4* p);
extern void func_8008F7B8(BattleWork* pWork);
extern s32 BattleRollActorFlag8000(BattleActor* pActor, s32 arg1);
extern s32 D_8009284C;
extern s32 func_8008FACC(BattleActor* pActor, BattleWork* pWork);
extern s32 BattleQueueActionMessages(BattleActor* pActor, BattleWork* pWork);
extern void BattleResetMessageQueue(BattleActor* p);
extern u8 g_BattleDifficultyLevel;
extern BattleWork g_BattleWorkSlots;
extern BattleWork D_80096F64;
extern BattleActionMessages g_BattleActionMessages[];
extern u8 g_BattleFacingUpdateEnabled;
extern void func_8008FCC8(BattleActor* pActor, BattleWork* pWork);
extern void BattleUpdateWorkState1(BattleActor* pActor, BattleWork* pWork);
extern void func_80090580(BattleActor* pActor, BattleWork* pWork);
extern void func_80090990(BattleActor* pActor, BattleWork* pWork);
extern void BattleUpdateActorApproach(BattleActor* p, s32 arg1, s32 arg2);
extern s32 BattleCheckValueAboveScaledMax(BattleActor* p, s32 arg1);


s32 BattleCheckActorBelowBreakThreshold(BattleActor* pActor, s32 arg1) {
    if (arg1 != 0) {
        return func_80073DE4(pActor, pActor->breakThresholdOffset);
    }
    return pActor->gaugePoints < 0x1000 - pActor->breakThresholdOffset;
}

s32 BattleGetActorBreakThreshold(BattleActor* pActor, BattleWork* pWork) {
    return 0x1000 - pActor->breakThresholdOffset;
}

s32 BattleGetActorBreakProximity(BattleActor* pActor) {
    s32 v1;
    s32 v0;

    v1 = pActor->gaugePoints;
    v0 = pActor->pWork->breakThreshold;
    return (v0 - 0x200 < v1) ? !(v0 < v1) : 2;
}

// retail spells each limit as (x * k) << 6 >> 4; folding it to x * (4 * k) is 2 insns shorter
s32 BattleRollActorActionChance(BattleActor* pActor, s32 arg1) {
    BattleWork* pWork;

    pWork = pActor->pWork;
    if ((rand() & 0xFF) < ((pWork->actionChance0 * 5) << 6) >> 4) {
        if (BattleCheckActorBelowBreakThreshold(pActor, 0) != 0) {
            goto tail;
        }
        if (BattleCheckValueAboveScaledMax(pActor, 0xC0) != 0) {
            goto tail;
        }
        if ((rand() & 0xFF) < ((pWork->actionChance3 * 3) << 6) >> 4) {
            goto tail;
        }
        if (BattleCheckValueAboveScaledMax(pActor, 0x80) == 0) {
            return 0;
        }
        if (pActor->pTarget->gaugeValue < pActor->gaugeValue) {
            goto tail;
        }
        return 0;
    }
    if ((rand() & 0xFF) >= ((pWork->actionChance3 * 5) << 6) >> 4) {
        return 0;
    }
tail:
    if ((rand() & 0xFF) < pWork->actionChance2) {
        if (pActor->pTarget->state == 4) {
            return 0;
        }
        if (BattleCheckValueAboveScaledMax(pActor->pTarget, 0x20) != 0) {
            return 1;
        }
        if (pActor->pTarget->gaugeValue < pActor->gaugeValue) {
            return 0;
        }
    }
    return 1;
}
s32 BattleRollActorFlag8000(BattleActor* pActor, s32 arg1) {
    BattleWork* pWork;

    pWork = pActor->pWork;
    if (arg1 != 0) {
        if ((rand() & 0xFF) < pWork->actionChance0) {
            if (D_8009284C < 0x600) {
                return 0;
            }
        }
    }
    if (BattleGetActorBreakProximity(pActor) != 0) {
        return 1;
    }
    return ((rand() & 0xFF) < pWork->actionChance3);
}void func_8008F7B8(BattleWork* pWork) {
    pWork->flags.bits.actionChance0Roll = (rand() & 0xFF) < pWork->actionChance0;
    pWork->flags.bits.actionChance1Roll = (rand() & 0xFF) < pWork->actionChance1;
    pWork->flags.bits.unk2C_12 = rand() & 1;
    pWork->flags.bits.actionChance2Roll = (rand() & 0xFF) < pWork->actionChance2;
    pWork->flags.bits.unk2C_8 = ((rand() & 0xFF) < pWork->actionChance0) && ((rand() % 10) < 3);
    pWork->flags.bits.random = rand();
    pWork->savedMoveCount = pWork->pOwner->moveCount;
}
void BattleQueueMessages4Then3(BattleActor* pActor) {
    if ((pActor->pTarget->flags & 0x60000000) == 0x20000000) {
        if ((rand() & 3) != 0) {
            return;
        }
    }
    if (g_BattleOptionOn != 0) {
        BattleTryQueueMessage5(pActor);
        BattleQueueMessage(pActor, 4);
        BattleQueueMessage(pActor, 3);
        return;
    }
    if ((pActor->flags & 0x60000000) != 0x20000000) {
        BattleQueueMessage(pActor, 3);
        return;
    }
    BattleQueueMessage(pActor, 4);
    BattleQueueMessage(pActor, 3);
}s32 BattleCheckPositionPastMapCenter(Vec4* pPos) {
    Vec4 pos;

    pos = *pPos;
    pos.x -= 0x3F80;
    pos.z -= 0x3F80;
    if (pos.x > 0 && pos.z > 0) {
        return 1;
    }
    return ((BattleGetGridCellAtPosition(pPos) & 0x3000000) ^ 0x1000000) < 1;
}extern long ratan2(long y, long x);
extern s32 g_BattleFacingAngle;

s32 BattleSetFacingFromPosition(Vec4* pPos, BattleWork* pWork) {
    Vec4 pos;
    s32 r;

    pos = *pPos;
    pos.x -= 0x3F80;
    pos.z -= 0x3F80;
    r = ratan2(pos.x, pos.z);
    pWork->targetAngle = (((r & 0xFFF) >= 0x201) ? 0x800 : 0xC00) - g_BattleFacingAngle;
    pWork->maxDistance = 0xFF;
    return 0;
}
/* 89.78%, re-measured 166: the body BUILDS (the permuter sweep's ERROR row was spurious) and the old number is real. Two differences: retail keeps one 'j epilogue; v0=0' trampoline right after the flags test, with the flags test a POSITIVE bnez into the body and the trampoline as its fall-through; ours is 4 bytes longer (272 vs 268) because the 'v < 0x1001' branch delay slot gets a li v0,1 where retail reuses the slti result and fills the slot with the lui 0xFFFF7FFF. Tried: flat four-return form, nested positive form (same 89.78), combined or-condition, shared-exit goto (84.93), the call result returned directly, gcc-2.7.2-cdk (88.88), and a 3-minute permuter run (PERMUTER_BETTER 823->643 was only a split 0x800 temp, which measured 88.28 by hand). gcc 2.7.2 puts the shared return-0 block at the function end in every spelling; block placement here is not source-controllable.
s32 func_8008FACC(BattleActor* pActor, BattleWork* pWork) {
    s32 v;

    if (g_BattleFacingUpdateEnabled == 0) {
        return 0;
    }
    if ((pActor->flags & 0x8000000) == 0) {
        return 0;
    }
    if (BattleCheckPositionPastMapCenter(&pActor->position) != 0) {
        BattleSetFacingFromPosition(&pActor->position, pWork);
        return 1;
    }
    if (BattleCheckPositionPastMapCenter(&pActor->pTarget->position) == 0) {
        return 0;
    }
    v = pActor->pTarget->unk8F4;
    if (v < 0x800) {
        pActor->flags |= 0x8000;
        pWork->setFlag8000 = 1;
        pWork->maxDistance = 0xFF;
        pWork->targetAngle = 0x800;
        pActor->unk58 = 0x800;
        return 1;
    }
    if (v < 0x1001) {
        return 1;
    }
    pActor->flags &= 0xFFFF7FFF;
    pWork->setFlag8000 = 0;
    pWork->maxDistance = 0;
    pActor->unk58 = 0x800;
    return 1;
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FACC);
void BattleSetActorFlag2AndClearQueue(BattleActor* pActor, BattleWork* pWork) {
    u8 v;

    v = pWork->difficulty;
    if (v >= 2) {
        pActor->flags |= 2;
        BattleResetMessageQueue(pActor);
        return;
    }
    if (v != 0) {
        if (!(pWork->flags.bits.random & 1)) {
            return;
        }
    } else if (pWork->flags.bits.random % 6 != 0) {
        return;
    }
    pActor->flags |= 2;
    BattleResetMessageQueue(pActor);
}

void BattleEnterWorkState0(BattleActor* pActor) {
    BattleWork* pWork;

    pWork = pActor->pWork;
    pWork->phase = 0;
    pWork->state = 0;
    pWork->delay = (2 - pWork->difficulty) * 30 + 0x5A;
    func_8008F7B8(pWork);
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FCC8);
void BattleQueueRandomMessages(BattleActor* pActor) {
    s32 n;
    s32 v;

    v = rand() % 10;
    n = 1;
    if (v >= 2) {
        n = 2;
    }
    if (v >= 5) {
        n++;
    }
    for (; n != 0; n--) {
        BattleQueueMessage(pActor, (rand() & 1) + 1);
    }
}

void BattleQueueMessagesAndSetScaledDelay(BattleActor* pActor, BattleWork* pWork) {
    int one;

    if (pActor->pTarget->state == 4) {
        if ((pWork->flags.flags & 0xA00) == 0x200) {
            BattleQueueMessages4Then3(pActor);
            pWork->delay = 3;
        }
    } else {
        one = 1;
        BattleQueueRandomMessages(pActor);
        pWork->delay = (2 - pWork->difficulty) * 20 + (rand() % 20 + one);
    }
    func_8008F7B8(pWork);
}
s32 BattleQueueActionMessages(BattleActor* pActor, BattleWork* pWork) {
    s32 n;
    s32 i;
    s32 one;

    n = pActor->actionCount;
    if (n == 0) {
        return 1;
    }
    n = rand() % n;
    for (i = 0; i < 0xE; i++) {
        if (pActor->pActionData->actionEnabled[i] == 0) {
            continue;
        }
        if (pActor->pActionSlots[i].flags == 0) {
            continue;
        }
        if (n == 0) {
            if (g_BattleActionMessages[i].messages[0] != 0) {
                BattleQueueMessage(pActor, g_BattleActionMessages[i].messages[0]);
            }
            if (g_BattleActionMessages[i].messages[1] != 0) {
                BattleQueueMessage(pActor, g_BattleActionMessages[i].messages[1]);
            }
            if (g_BattleActionMessages[i].messages[2] != 0) {
                BattleQueueMessage(pActor, g_BattleActionMessages[i].messages[2]);
            }
            break;
        }
        n--;
    }
    one = 1;
    pWork->delay = (2 - pWork->difficulty) * 20 + (rand() % 20 + one);
    func_8008F7B8(pWork);
    return 0;
}

void BattleEnterWorkState1(BattleActor* pActor) {
    BattleWork* pWork;
    u8 v;

    pWork = pActor->pWork;
    pWork->state = 1;
    BattleQueueMessagesAndSetScaledDelay(pActor, pWork);
    v = pWork->difficulty;
    if (v >= 2) {
        pWork->ticks = rand() % 8 + 1;
    } else if (v != 0) {
        pWork->ticks = rand() % 6 + 1;
    } else {
        pWork->ticks = rand() % 4 + 1;
    }
    pWork->maxDistance = 0;
    pWork->setFlag8000 = 0;
    func_8008F7B8(pWork);
}

s32 BattleDecideActorAction(BattleActor* pActor, BattleWork* pWork) {
    if (pActor->actionCount != 0) {
        if ((rand() & 1) == 0) {
            return 1;
        }
        if ((rand() & 1) != 0 && BattleRollActorActionChance(pActor, 0) != 0) {
            BattleQueueMessages4Then3(pActor);
            return 0;
        }
        if (D_8009284C < 0x1001) {
            BattleQueueActionMessages(pActor, pWork);
            return 0;
        }
        return 1;
    }
    if (BattleRollActorActionChance(pActor, 0) != 0) {
        BattleQueueMessages4Then3(pActor);
        return 0;
    }
    return 1;
}
void BattleUpdateWorkState1(BattleActor* pActor, BattleWork* pWork) {
    u8 v;

    if (func_8008FACC(pActor, pWork) != 0) {
        return;
    }
    if (pActor->actionPhase != 2 && pActor->pTarget->actionPhase == 2) {
        BattleSetActorFlag2AndClearQueue(pActor, pWork);
    }
    if (pActor->pTarget->unk8F4 - 0x201 < 0x3FF && pWork->difficulty != 0) {
        BattleSetActorFlag2AndClearQueue(pActor, pWork);
    }
    if (--pWork->delay > 0) {
        return;
    }
    if (D_8009284C >= 0x301) {
        switch (rand() % 10) {
        case 0:
            if ((rand() & 0xFF) < pWork->actionChance1) {
                break;
            }
            BattleEnterWorkState2(pActor, 0);
            break;
        case 2:
            if (pActor->actionCount != 0) {
                BattleQueueActionMessages(pActor, pWork);
                break;
            }
            /* fall through */
        case 1:
            BattleEnterWorkState0(pActor);
            break;
        case 3:
        case 4:
        case 5:
            if (BattleDecideActorAction(pActor, pWork) != 0) {
                BattleEnterWorkState0(pActor);
            }
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            BattleEnterWorkState3(pActor, 0);
            break;
        }
    }
    v = pWork->ticks;
    pWork->ticks = v + 0xFF;
    if (v != 0) {
        BattleQueueMessagesAndSetScaledDelay(pActor, pWork);
    } else {
        BattleEnterWorkState2(pActor, rand() & 1);
    }
}
void BattleEnterWorkState3(BattleActor* pActor, s32 arg1) {
    BattleWork* pWork;
    s32 r;

    pWork = pActor->pWork;
    pWork->state = 3;
    r = rand();
    pWork->ticks = r % 4 + 1;
    pWork->delay = 0;
    func_8008F7B8(pWork);
    pWork->setFlag8000 = BattleRollActorFlag8000(pActor, 1);
    pWork->flags.bytes.unk2E = 0;
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090580);
void BattleEnterWorkState2(BattleActor* pActor, s32 arg1) {
    BattleWork* pWork;

    pWork = pActor->pWork;
    pWork->state = 2;
    if (rand() % 3 == 0) {
        BattleTryQueueMessage5(pActor);
    }
    pWork->actionDistance = rand() % 1536 + 0x100;
    pWork->delay = 0;
    pWork->ticks = rand() % 5 + 3;
    func_8008F7B8(pWork);
    pWork->setFlag8000 = BattleRollActorFlag8000(pActor, 0);
    pWork->flags.bytes.unk2E = 0;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090990);
void BattleLoadWorkActionChances(BattleActor* pActor) {
    BattleActionData* pActionData;
    BattleWork* pWork;

    pActionData = pActor->pActionData;
    pWork = pActor->pWork;
    pWork->actionChance0 = pActionData->actionChances[0];
    pWork->actionChance1 = pActionData->actionChances[1];
    pWork->actionChance2 = pActionData->actionChances[2];
    pWork->actionChance3 = pActionData->actionChances[3];
}

void BattleInitActorWork(BattleActor* pActor) {
    BattleWork* pWork;
    s32 thr;

    pWork = &g_BattleWorkSlots;
    if ((pActor->flags & 0x8000000) != 0) {
        pWork = &D_80096F64;
    }
    pActor->pWork = pWork;
    pWork->unk6 = 0x10;
    pWork->pOwner = pActor;
    pWork->delay = 0;
    pWork->unk7 = 0xA;
    pWork->targetAngle = 0;
    pWork->maxDistance = 0;
    pWork->state = 0;
    pWork->ticks = 0;
    pActor->distance = 0;
    thr = BattleGetActorBreakThreshold(pActor, pWork);
    /* without the block cc1 sinks this store below the difficulty load */
    do { pWork->breakThreshold = thr; } while (0);
    pWork->difficulty = g_BattleDifficultyLevel;
    BattleLoadWorkActionChances(pActor);
    switch (rand() % 3) {
    case 0:
        BattleEnterWorkState0(pActor);
        break;
    case 1:
        BattleEnterWorkState1(pActor);
        break;
    case 2:
        BattleEnterWorkState2(pActor, 0);
        break;
    case 3:
        BattleEnterWorkState3(pActor, 0);
        break;
    }
    BattleResetMessageQueue(pActor);
}

void BattleDispatchWorkState(BattleActor* pActor) {
    BattleWork* pWork;

    if ((pActor->flags & 0x40) != 0) {
        pWork = pActor->pWork;
        pWork->difficulty = g_BattleDifficultyLevel;
        switch (pWork->state) {
        case 0:
            func_8008FCC8(pActor, pWork);
            break;
        case 1:
            BattleUpdateWorkState1(pActor, pWork);
            break;
        case 2:
            func_80090990(pActor, pWork);
            break;
        case 3:
            func_80090580(pActor, pWork);
            break;
        }
        if (pWork->setFlag8000 != 0) {
            pActor->flags |= 0x8000;
        }
        if ((pWork->flags.flags & 0x2000) != 0) {
            pActor->flags |= 2;
        }
        BattleUpdateActorApproach(pActor, pWork->targetAngle, pWork->maxDistance);
    }
}
