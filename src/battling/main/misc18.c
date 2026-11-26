#include "common.h"

// battle actor.h is not includable in this overlay, so the constants these
// bodies share with the other battle units are spelled here again.
// flags bit 27, the second player's side (battle/misc3.c): the work block picks
// the alternate slot for such an actor.
#define BATTLE_ACTOR_FLAG_PLAYER2 0x8000000
// Half and three-quarter turn of the 12-bit angle space; BATTLE_ANGLE_HALF_TURN is
// spelled the same in battling/misc17.c, battle/misc3.c and battle/misc4.c.
#define BATTLE_ANGLE_HALF_TURN 0x800
#define BATTLE_ANGLE_THREE_QUARTER_TURN 0xC00
// The gauge's full value. An actor's break threshold is this minus its
// breakThresholdOffset, and the points within BATTLE_BREAK_PROXIMITY_RANGE above
// that threshold are the proximity BattleGetActorBreakProximity reports as 1.
#define BATTLE_GAUGE_FULL 0x1000
#define BATTLE_BREAK_PROXIMITY_RANGE 0x200
// The offset between the map's centre and the field coordinates. Both position
// tests shift the point by it before asking which side of the centre it is on
// (battling/misc9.c names the same value BATTLE_SPOKE_RADIUS, misc10 stores it as
// the scene origin).
#define BATTLE_SCENE_ORIGIN_OFFSET 0x3F80
// State 2 takes its action once the AI's target distance (BattleVectorDistance of the
// two positions, battling/misc2.c) reaches this.
#define BATTLE_AI_TARGET_DISTANCE_MIN 1201
// The action slots an actor carries: BattleQueueActionMessages walks one
// actionEnabled byte and one BattleActionSlot per slot.
#define BATTLE_ACTION_SLOT_COUNT 14

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
        // The rand() & 1 coin flip func_8008F7B8 takes: BattleUpdateWorkState0 sends the
        // actor into state 1 (approach and queue messages) when it reads set and
        // into state 2 when it reads clear.
        /* 0x0 */ u32 attackRoll : 1;
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
    // 1 waits until the actor drops below its break threshold (BattleUpdateWorkState0).
    /* 0x20 */ s32 phase;
    /* 0x24 */ s32 breakThreshold;
    // The 2D distance the state-2 handler waits for before it takes the action
    // (rand() % 1536 + 0x100 at state entry, compared with g_BattleActorDistance2D).
    /* 0x28 */ s32 actionDistance;
    /* 0x2C */ BattleWorkFlags flags;
    // The owner's moveCount as of the last func_8008F7B8, which the state-0
    // handler compares with the live one.
    /* 0x30 */ s16 savedMoveCount;
    /* 0x32 */ u16 pad32;
} BattleWork; // Size: 0x34

// The record the actor's four action probabilities are copied from.
typedef struct {
    /* 0x0 */ u8 unk0[0x6];
    /* 0x6 */ u8 actionChances[0x4];
    /* 0xA */ u8 actionEnabled[BATTLE_ACTION_SLOT_COUNT];
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

// The effect record pNearestEffect points at: one of the 0x44-byte slots the actor
// keeps at +0x64C, whose nearest active one func_80073644 records at +0x8F8. Only
// the mode byte is read here; misc2/misc3/misc10 declare the same slot.
typedef struct {
    /* 0x00 */ u8 unk0[0x33];
    /* 0x33 */ u8 mode;
} BattleEffect; // Size: 0x34

// A battle participant as this unit drives it: the fields this unit touches plus the
// offsets the sibling battle units name around them (misc2, misc3, misc4, misc17).
// 0x1668 is the last one, so 0x166A is a lower bound on the real size.
typedef struct BattleActor {
    /* 0x000 */ Vec4 position;
    /* 0x010 */ u8 unk10[0x38];
    /* 0x048 */ u32 distance;
    /* 0x04C */ u8 unk4C[0xC];
    /* 0x058 */ s32 angle;
    /* 0x05C */ u8 unk5C[0x24];
    /* 0x080 */ BattleActionSlot* pActionSlots;
    /* 0x084 */ u8 unk84[0x30];
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    // The gauge as it stood before this frame's update and the level its points
    // reached: func_80077038 rewrites all three around gaugePoints, and misc2's
    // BattleAccumulateGaugePoints raises gaugeLevel.
    /* 0x0B8 */ s16 prevGaugeValue;
    /* 0x0BA */ s16 gaugeLevel;
    // The gauge's full value: BattleCheckValueAboveScaledMax scales it (misc17) and
    // misc2's BattleSetResultCode compares gaugeLevelsMode1/2 with fractions of it.
    /* 0x0BC */ s16 gaugeMax;
    /* 0x0BE */ s16 breakThresholdOffset;
    // The gauge bar as it is drawn: its width, the width of the frame before, the
    // frames of hit flash left and the timer of the phase the bar animates (misc4's
    // round reset stores 0x7F and 0xF0 into the first and the third).
    /* 0x0C0 */ u8 gaugeWidth;
    /* 0x0C1 */ u8 prevGaugeWidth;
    /* 0x0C2 */ u8 flashTimer;
    /* 0x0C3 */ u8 actionPhaseTimer;
    /* 0x0C4 */ u8 state;
    /* 0x0C5 */ u8 actionPhase;
    // The state byte as the previous frame left it (misc4 copies it here), then the
    // frames left in the current step and in the move as a whole (misc4's reset
    // clears both).
    /* 0x0C6 */ u8 prevState;
    /* 0x0C7 */ u8 padC7;
    /* 0x0C8 */ s16 moveEndTimer;
    /* 0x0CA */ s16 moveTimer;
    // The 12-bit facing this actor's side starts from (the round reset stores 0x400
    // for one side and -0x400 for the other) and the offset every facing it computes
    // takes from it: func_80076884 reads one and writes the other.
    /* 0x0CC */ s16 sideBaseAngle;
    /* 0x0CE */ s16 facingOffset;
    /* 0x0D0 */ u32 flags;
    // The movement state in bits 0-1 and the state of the frame before in bits 2-3;
    // the movement code shifts one into the other, and the round reset clears bits 4
    // and 6 (misc4).
    /* 0x0D4 */ u32 stateFlags;
    /* 0x0D8 */ struct BattleActor* pTarget;
    /* 0x0DC */ u8 unkDC[0x818];
    // The distance from the actor to the nearest active effect slot and that slot,
    // recorded every frame by func_80073644 (0x10000 when none is active); the
    // state handlers here read the target's distance to tell whether it stands in
    // an effect.
    /* 0x8F4 */ u32 nearestEffectDistance;
    /* 0x8F8 */ BattleEffect* pNearestEffect;
    /* 0x8FC */ u8 unk8FC[0x15];
    /* 0x911 */ u8 actionCount;
    /* 0x912 */ u8 unk912[0xCEA];
    /* 0x15FC */ BattleWork* pWork;
    /* 0x1600 */ BattleActionData* pActionData;
    /* 0x1604 */ u8 unk1604[0x50];
    // The gauge levels this actor accumulated in each of the two categories; misc2's
    // BattleSetResultCode compares them with fractions of gaugeMax.
    /* 0x1654 */ s32 gaugeLevelsMode1;
    /* 0x1658 */ s32 gaugeLevelsMode2;
    // Counters the input handler bumps as it queues messages: message12Count for each
    // message 1 or 2, message3Count for each message 3 (misc3).
    /* 0x165C */ s32 message3Count;
    /* 0x1660 */ s32 message12Count;
    // The actor type's sound table, installed by BattleSetActorSoundTable and indexed
    // by sound id (misc17).
    /* 0x1664 */ u8* pSoundTable;
    // Bumped by BattleCommitActorMoveStep each time it starts a move for the actor; the round
    // reset clears it.
    /* 0x1668 */ s16 moveCount;
} BattleActor; // Size: 0x166A

extern s32 func_80073DE4(BattleActor* p, s32 arg1);
extern void BattleTryQueueMessage5(BattleActor* p);
extern void BattleQueueMessage(BattleActor* p, s32 arg1);
extern u8 g_BattleOptionOn;
extern u32 BattleGetGridCellAtPosition(Vec4* p);
extern void func_8008F7B8(BattleWork* pWork);
extern s32 BattleRollActorFlag8000(BattleActor* pActor, s32 arg1);
extern s32 g_BattleActorDistance2D;
extern s32 BattleTryUpdateFacingFromPosition(BattleActor* pActor, BattleWork* pWork);
extern s32 BattleQueueActionMessages(BattleActor* pActor, BattleWork* pWork);
extern void BattleResetMessageQueue(BattleActor* p);
extern u8 g_BattleDifficultyLevel;
extern BattleWork g_BattleWorkSlots;
extern BattleWork D_80096F64;
extern BattleActionMessages g_BattleActionMessages[];
extern u8 g_BattleFacingUpdateEnabled;
extern void BattleUpdateWorkState0(BattleActor* pActor, BattleWork* pWork);
extern void BattleUpdateWorkState1(BattleActor* pActor, BattleWork* pWork);
extern void BattleUpdateWorkState3(BattleActor* pActor, BattleWork* pWork);
extern void BattleUpdateWorkState2(BattleActor* pActor, BattleWork* pWork);
extern void BattleUpdateActorApproach(BattleActor* p, s32 arg1, s32 arg2);
extern s32 BattleCheckValueAboveScaledMax(BattleActor* p, s32 arg1);
extern s32 g_BattleAiTargetDistance;


s32 BattleCheckActorBelowBreakThreshold(BattleActor* pActor, s32 arg1) {
    if (arg1 != 0) {
        return func_80073DE4(pActor, pActor->breakThresholdOffset);
    }
    return pActor->gaugePoints < BATTLE_GAUGE_FULL - pActor->breakThresholdOffset;
}

s32 BattleGetActorBreakThreshold(BattleActor* pActor, BattleWork* pWork) {
    return BATTLE_GAUGE_FULL - pActor->breakThresholdOffset;
}

s32 BattleGetActorBreakProximity(BattleActor* pActor) {
    s32 v1;
    s32 v0;

    v1 = pActor->gaugePoints;
    v0 = pActor->pWork->breakThreshold;
    return (v0 - BATTLE_BREAK_PROXIMITY_RANGE < v1) ? !(v0 < v1) : 2;
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
            if (g_BattleActorDistance2D < 0x600) {
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
    pWork->flags.bits.attackRoll = rand() & 1;
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
    pos.x -= BATTLE_SCENE_ORIGIN_OFFSET;
    pos.z -= BATTLE_SCENE_ORIGIN_OFFSET;
    if (pos.x > 0 && pos.z > 0) {
        return 1;
    }
    return ((BattleGetGridCellAtPosition(pPos) & 0x3000000) ^ 0x1000000) < 1;
}extern long ratan2(long y, long x);
extern s32 g_BattleFacingAngle;

s32 BattleSetFacingFromPosition(Vec4* pPos, BattleWork* pWork) {
    Vec4 pos;
    s32 r;
    s16 d;
    u8 ticks;

    pos = *pPos;
    pos.x -= BATTLE_SCENE_ORIGIN_OFFSET;
    pos.z -= BATTLE_SCENE_ORIGIN_OFFSET;
    r = ratan2(pos.x, pos.z);
    pWork->targetAngle =
        (((r & 0xFFF) >= 0x201) ? BATTLE_ANGLE_HALF_TURN : BATTLE_ANGLE_THREE_QUARTER_TURN) - g_BattleFacingAngle;
    pWork->maxDistance = 0xFF;
    return 0;
}
// The middle distance arm returns no value: retail's taken edge hands back the
// comparison's own 1 from $v0, and a `return 1;` there costs an instruction retail
// does not have. The body runs to the end of the if, so its false path falls into
// the epilogue with $v0 still holding the callee's 0.
s32 BattleTryUpdateFacingFromPosition(BattleActor* pActor, BattleWork* pWork) {
    s32 v;

    if (g_BattleFacingUpdateEnabled == 0) {
        return 0;
    }
    if ((pActor->flags & BATTLE_ACTOR_FLAG_PLAYER2) == 0) {
        return 0;
    }
    if (BattleCheckPositionPastMapCenter(&pActor->position) != 0) {
        BattleSetFacingFromPosition(&pActor->position, pWork);
        return 1;
    }
    if (BattleCheckPositionPastMapCenter(&pActor->pTarget->position) != 0) {
        v = pActor->pTarget->nearestEffectDistance;
        if (v < 0x800) {
            pActor->flags |= 0x8000;
            pWork->setFlag8000 = 1;
            pWork->maxDistance = 0xFF;
            pWork->targetAngle = BATTLE_ANGLE_HALF_TURN;
            pActor->angle = BATTLE_ANGLE_HALF_TURN;
            return 1;
        }
        if (v < 0x1001) {
            return;
        }
        pActor->flags &= 0xFFFF7FFF;
        pWork->setFlag8000 = 0;
        pWork->maxDistance = 0;
        pActor->angle = BATTLE_ANGLE_HALF_TURN;
        return 1;
    }
}

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
}
void BattleUpdateWorkState0(BattleActor* pActor, BattleWork* pWork) {
    s32 distance;

    pWork->maxDistance = 0;
    pWork->setFlag8000 = 0;
    pWork->targetAngle = 0;
    if (BattleTryUpdateFacingFromPosition(pActor, pWork) != 0) {
        return;
    }
    if (g_BattleActorDistance2D < 0x200) {
        if (pWork->flags.bits.attackRoll != 0) {
            BattleEnterWorkState1(pActor);
        } else if (pWork->difficulty != 0) {
            BattleEnterWorkState2(pActor, 1);
        }
        if (pActor->pTarget->actionPhase == 2) {
            BattleSetActorFlag2AndClearQueue(pActor, pWork);
        }
        if (pActor->moveCount + 2 < pWork->savedMoveCount) {
            BattleEnterWorkState1(pActor);
        }
    }
    distance = pActor->pTarget->nearestEffectDistance;
    if (distance < 0x400) {
        BattleEnterWorkState2(pActor, 3);
        pWork->setFlag8000 = 1;
    }
    switch (pWork->phase) {
    case 0:
        if (--pWork->delay < 0) {
            pWork->phase++;
        }
        if (pActor->pTarget->state != 0) {
            break;
        }
        if (pWork->flags.bits.attackRoll != 0) {
            if (BattleRollActorActionChance(pActor, 0) != 0) {
                BattleQueueMessages4Then3(pActor);
            }
            BattleEnterWorkState3(pActor, 0);
        }
        break;
    case 1:
        if (BattleCheckActorBelowBreakThreshold(pActor, 0) != 0) {
            BattleEnterWorkState3(pActor, 0);
        }
        break;
    }
}
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
    for (i = 0; i < BATTLE_ACTION_SLOT_COUNT; i++) {
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
        if (g_BattleActorDistance2D < 0x1001) {
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

    if (BattleTryUpdateFacingFromPosition(pActor, pWork) != 0) {
        return;
    }
    if (pActor->actionPhase != 2 && pActor->pTarget->actionPhase == 2) {
        BattleSetActorFlag2AndClearQueue(pActor, pWork);
    }
    if (pActor->pTarget->nearestEffectDistance - 0x201 < 0x3FF && pWork->difficulty != 0) {
        BattleSetActorFlag2AndClearQueue(pActor, pWork);
    }
    if (--pWork->delay > 0) {
        return;
    }
    if (g_BattleActorDistance2D >= 0x301) {
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
    s16 d;
    u8 ticks;

    pWork = pActor->pWork;
    pWork->state = 3;
    r = rand();
    pWork->ticks = r % 4 + 1;
    pWork->delay = 0;
    func_8008F7B8(pWork);
    pWork->setFlag8000 = BattleRollActorFlag8000(pActor, 1);
    pWork->flags.bytes.unk2E = 0;
}
void BattleUpdateWorkState3(BattleActor* pActor, BattleWork* pWork) {
    s16 angle;
    s32 distance;
    s32 roll;
    s16 delay;
    u8 ticks;

    if (BattleCheckPositionPastMapCenter(&pActor->pTarget->position) != 0 && g_BattleFacingUpdateEnabled != 0 &&
        (pActor->flags & BATTLE_ACTOR_FLAG_PLAYER2) != 0) {
        BattleEnterWorkState0(pActor);
        return;
    }
    if (pActor->pTarget->pNearestEffect->mode == 1) {
        distance = pActor->pTarget->nearestEffectDistance;
        if (distance < 0x500 && pWork->difficulty != 0) {
            pWork->setFlag8000 = 1;
            angle = -0x400;
            if (pWork->flags.bits.attackRoll != 0) {
                angle = 0x400;
            }
            pWork->targetAngle = angle;
            if (pWork->difficulty >= 2 && (rand() & 0xFF) < pWork->actionChance1 &&
                BattleRollActorActionChance(pActor, 0) != 0 && pWork->flags.bits.attackRoll != 0) {
                if ((rand() & 3) == 0) {
                    BattleQueueMessage(pActor, 4);
                }
                BattleQueueMessages4Then3(pActor);
                pWork->flags.bytes.unk2E = 0;
            }
            pWork->flags.bytes.unk2E++;
        }
    }
    if (g_BattleActorDistance2D < 0x180) {
        if ((rand() & 3) == 0) {
            BattleQueueMessage(pActor, 4);
        }
        BattleEnterWorkState1(pActor);
    }
    if (BattleGetActorBreakProximity(pActor) == 0) {
        pWork->setFlag8000 = 0;
    }
    if (pWork->delay < 0) {
        pWork->targetAngle = rand() % 1536 - 0x300;
        roll = rand();
        if (pWork->flags.flags & 0x400) {
            delay = roll % 120 + 10;
        } else {
            delay = roll % 100 + 10;
        }
        ticks = pWork->ticks;
        pWork->delay = delay;
        pWork->maxDistance = 0xFF;
        if (ticks != 0) {
            pWork->ticks = ticks - 1;
        } else {
            if ((rand() & 3) == 0) {
                BattleQueueMessage(pActor, 4);
            }
            if (BattleDecideActorAction(pActor, pWork) != 0) {
                BattleEnterWorkState0(pActor);
            }
            pWork->ticks = rand() % 4 + 1;
        }
    }
    pWork->delay--;
}
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
void BattleUpdateWorkState2(BattleActor* pActor, BattleWork* pWork) {
    s16 angle;
    s32 distance;
    s32 roll;
    s16 delay;
    u8 ticks;

    if (BattleTryUpdateFacingFromPosition(pActor, pWork) != 0) {
        return;
    }
    if (pActor->pTarget->pNearestEffect->mode == 1) {
        distance = pActor->pTarget->nearestEffectDistance;
        if (distance < 0x500 && pWork->difficulty != 0) {
            pWork->setFlag8000 = 1;
            angle = -0x400;
            if (pWork->flags.bits.attackRoll != 0) {
                angle = 0x400;
            }
            pWork->targetAngle = angle;
            if (pWork->difficulty >= 2 && BattleRollActorActionChance(pActor, 0) != 0 &&
                pWork->flags.bits.attackRoll != 0) {
                if ((rand() & 3) == 0) {
                    BattleQueueMessage(pActor, 4);
                }
                BattleQueueMessages4Then3(pActor);
                pWork->flags.bytes.unk2E = 0;
            }
            pWork->flags.bytes.unk2E++;
        }
    }
    if (BattleGetActorBreakProximity(pActor) == 0) {
        pWork->setFlag8000 = 0;
    }
    if (g_BattleActorDistance2D > pWork->actionDistance ||
        (g_BattleOptionOn != 0 && g_BattleAiTargetDistance >= BATTLE_AI_TARGET_DISTANCE_MIN)) {
        if (g_BattleOptionOn != 0) {
            BattleQueueMessage(pActor, 4);
        }
        BattleQueueActionMessages(pActor, pWork);
        BattleEnterWorkState0(pActor);
    }
    if (pWork->delay < 0 || pWork->maxDistance == 0) {
        pWork->targetAngle = rand() % 1536 + 0x500;
        roll = rand();
        if (pWork->flags.flags & 0x400) {
            delay = roll % 40 + 10;
        } else {
            delay = roll % 60 + 10;
        }
        /* the tick count has to be read before the delay is stored: cc1 keeps
           that order, and the store lands in the delay compute otherwise */
        ticks = pWork->ticks;
        pWork->delay = delay;
        pWork->maxDistance = 0xFF;
        if (ticks != 0) {
            pWork->ticks = ticks - 1;
        } else {
            if (BattleRollActorActionChance(pActor, 0) != 0) {
                BattleQueueMessage(pActor, 4);
                BattleQueueMessages4Then3(pActor);
            }
            pWork->ticks = rand() % 5 + 3;
        }
    }
    pWork->delay--;
}
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
    if ((pActor->flags & BATTLE_ACTOR_FLAG_PLAYER2) != 0) {
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
            BattleUpdateWorkState0(pActor, pWork);
            break;
        case 1:
            BattleUpdateWorkState1(pActor, pWork);
            break;
        case 2:
            BattleUpdateWorkState2(pActor, pWork);
            break;
        case 3:
            BattleUpdateWorkState3(pActor, pWork);
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
