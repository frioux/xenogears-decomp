#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"

// WorkListEntry of include/system/animation.h; an overlay unit cannot include
// that header (it holds tentative definitions of main-exe globals), so the
// layout is repeated here.
typedef struct WorkListEntry {
    /* 0x00 */ void *unk0;
    /* 0x04 */ void *unk4;
    /* 0x08 */ void (*onTriggerCallback)(void *pTask);
    /* 0x0C */ void (*onFreeCallback)(void *pTask);
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ struct WorkListEntry *pNext;
} WorkListEntry; // Size: 0x1C

// One frame of a battle model, as BattleGetFrame hands it out: a header of
// self-relative (offset, count) pairs into the same allocation, then the flags
// word that records whether the part scales are already applied.
typedef struct {
    /* 0x00 */ s32 partsOffset; // byte offset of the part array, from here
    /* 0x04 */ s32 partCount;
    /* 0x08 */ u8 unk8[0x8]; // a second (offset, count) pair this unit never reads
    /* 0x10 */ s32 primsOffset; // byte offset of the primitive packet array
    /* 0x14 */ s32 primCount;
    /* 0x18 */ u32 flags; // 0x8000: part scales already applied
} BattleModelFrame; // Size: 0x1C

// A three-component scale as the timed scale task keeps it; unsigned because the
// task only halves and subtracts, and the ease ends at zero.
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 pad6;
} BattleScaleVector; // Size: 0x8

// The scale-easing timer task: a timer work-list entry, then the scale it eases
// and the countdown driving the ease.
typedef struct {
    /* 0x00 */ WorkListEntry entry;
    /* 0x1C */ BattleScaleVector current; // scale the task is at right now
    /* 0x24 */ BattleScaleVector start; // scale the ease started from
    /* 0x2C */ BattleScaleVector target; // scale the ease ends at
    /* 0x34 */ s32 frame; // frames elapsed
    /* 0x38 */ s32 framesLeft; // frames left in the ease
    /* 0x3C */ s32 halfLength; // half the ease length, the divisor for each step
} BattleScaleTask; // Size: 0x40

// The screen fade drawn over the battle: a timer entry (0x00) that steps the
// colour and a draw entry (0x1C) that emits the quad, then the two ends of the
// fade and the colour it is at on this frame. D_800C3C00 and D_800C3C50 are the
// two static ones, which is where the size comes from.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry drawEntry;
    /* 0x38 */ s32 length; // twice the frame count the fade was given
    /* 0x3C */ s32 timer; // counts the length down
    /* 0x40 */ u8 unk40;
    /* 0x41 */ u8 abr; // semi-transparency mode handed to GetTPage
    /* 0x42 */ u8 targetR;
    /* 0x43 */ u8 targetG;
    /* 0x44 */ u8 targetB;
    /* 0x45 */ u8 startR;
    /* 0x46 */ u8 startG;
    /* 0x47 */ u8 startB;
    /* 0x48 */ u8 curR;
    /* 0x49 */ u8 curG;
    /* 0x4A */ u8 curB;
    /* 0x4B */ u8 pad4B[0x5];
} BattleScreenFadeTask; // Size: 0x50

// The battle image task: a timer entry (0x00) that eases a value and a draw
// entry (0x1C) that re-applies the resulting colour tint when it moves.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry drawEntry;
    /* 0x38 */ s32 length; // twice the frame count the ease was given
    /* 0x3C */ s32 timer; // counts the length down
    /* 0x40 */ s16 start; // value the ease started from
    /* 0x42 */ s16 end; // value the ease ends at
    /* 0x44 */ s16 applied; // value the last tint was applied for
    /* 0x46 */ s16 current; // value the ease is at on this frame
    /* 0x48 */ s16 blendMode;
    /* 0x4A */ s16 r;
    /* 0x4C */ s16 g;
    /* 0x4E */ s16 b;
} BattleImageTask; // Size: 0x50

extern BattleScreenFadeTask *g_BattleScreenFadeTask;
extern BattleScaleTask *g_BattleScaleTask;
extern u8 g_BattleScreenFadeOwnBuffer;
extern u32 g_BattleImageTask;

extern u8 *g_GfxCurWorkBuffer;
extern u8 *g_GfxCurWorkBufferEnd;
extern u_long *g_GfxCurOT;

extern BattleScaleTask *BattleGetScaleTask();
extern void func_800B3358(BattleScaleTask *pTask);
extern void func_800B39C0(s32, s32, u8, u8, u8);
extern void BattleSetActorRectParams(s32, s32, s32, s32, s32, s32);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void BattleUploadAndFreeImageResources(void);
extern WorkListEntry *TimerWorkListAllocateTask();
extern void TimerWorkListSetTaskCallback();
extern void WorkListTaskSetOnFreeCallback();

extern u8 *D_800C3BEC;
extern s32 D_800C3BF0;
extern DR_TPAGE D_800C3BF8;

u8 *BattleGetFrame(u8 *pBase, s32 n) {
    return &pBase[n * 0x1C + 0xC];
}

s32 BattleGetPrimitiveBufferSize(BattleModelFrame *pFrame) {
    s32 count;
    s32 total;
    u8 *pPrim;
    s32 n;

    count = 0;
    total = 0;
    pPrim = (u8 *)((u32)pFrame->primsOffset + (u32)pFrame);
    n = pFrame->primCount;
    for (; count != n; count++) {
        total += (pPrim[0] + 1) * 4;
        pPrim += (pPrim[1] + 1) * 4;
    }
    return total;
}

// Advances the primitive cursor by one packet and counts the packet emitted.
void func_800B16F0(void) {
    u8 *pCmd = D_800C3BEC;
    u8 size = pCmd[1];

    D_800C3BF0 += 1;
    D_800C3BEC = pCmd + (size + 1) * 4;
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1720);
void BattleModelScaleParts(BattleModelFrame *pFrame, s32 shift) {
    SVECTOR *pPart;
    s32 n;
    s32 i;

    if (pFrame->flags & 0x8000) {
        return;
    }
    pFrame->flags |= 0x8000;
    pPart = (SVECTOR *)((u32)pFrame->partsOffset + (u32)pFrame);
    n = pFrame->partCount;
    for (i = 0; i != n; i++) {
        pPart[i].vx <<= shift;
        pPart[i].vy <<= shift;
        pPart[i].vz <<= shift;
    }
}

// Emits the drawing-mode packet SetDrawTPage filled in at D_800C3BF8 as one more
// entry of the ordering table, and claims the bytes it takes from the packet
// buffer.
void func_800B1F0C(u8 *pOt) {
    u8 *pBuf = g_GfxCurWorkBuffer;

    if ((u8 *)(pBuf + sizeof(DR_TPAGE)) < g_GfxCurWorkBufferEnd) {
        g_GfxCurWorkBuffer = pBuf + sizeof(DR_TPAGE);
        *(DR_TPAGE *)pBuf = D_800C3BF8;
        AddPrim(pOt, pBuf);
    }
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1F6C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B2AEC);

void BattleNoOp2(void) {
}

void BattleNoOp3(void) {
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3358);

void BattleFreeScaleTask(void *pEntry) {
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
    g_BattleScaleTask = NULL;
}

// Returns the one scale task, allocating it on first use and restarting the
// ease from wherever the scale currently sits on every later call.
BattleScaleTask *BattleGetScaleTask(void) {
    BattleScaleTask *pTask;

    if (g_BattleScaleTask == NULL) {
        pTask = (BattleScaleTask *)TimerWorkListAllocateTask(NULL,
                    sizeof(BattleScaleTask) - sizeof(WorkListEntry));
        TimerWorkListSetTaskCallback(pTask, func_800B3358);
        WorkListTaskSetOnFreeCallback(pTask, BattleFreeScaleTask);
        pTask->start.vx = 0;
        pTask->start.vy = 0;
        pTask->start.vz = 0;
        g_BattleScaleTask = pTask;
    } else {
        pTask = g_BattleScaleTask;
        pTask->start.vx = pTask->current.vx;
        pTask->start.vy = pTask->current.vy;
        pTask->start.vz = pTask->current.vz;
    }
    return pTask;
}

void BattleCreateTimedScaleTask(BattleScaleVector *pTarget, s32 frames) {
    BattleScaleTask *pTask = BattleGetScaleTask();

    pTask->target.vx = pTarget->vx;
    pTask->target.vy = pTarget->vy;
    pTask->target.vz = pTarget->vz;
    pTask->framesLeft = frames * 2;
    pTask->halfLength = frames * 2;
    func_800B3358(pTask);
}

// One step of the fade: while the timer runs, move the colour from the start
// toward the target in equal slices, and once it is spent settle on the target
// and release the task if the fade has ended on black.
void BattleStepScreenFade(void *pArg) {
    BattleScreenFadeTask *pTask = pArg;
    s32 step[3];

    if (pTask->timer == 0) {
        pTask->curR = pTask->targetR;
        pTask->curG = pTask->targetG;
        pTask->curB = pTask->targetB;
        if ((pTask->curR | pTask->curG | pTask->curB) == 0) {
            g_BattleScreenFadeTask->timerEntry.onFreeCallback(g_BattleScreenFadeTask);
        }
        return;
    }
    pTask->timer--;
    step[0] = pTask->targetR - pTask->startR;
    step[1] = pTask->targetG - pTask->startG;
    step[2] = pTask->targetB - pTask->startB;
    gte_lddp(pTask->timer >> 1);
    gte_ldlvl(step);
    gte_gpf0();
    gte_stlvl(step);
    step[0] = step[0] / (pTask->length >> 1);
    step[1] = step[1] / (pTask->length >> 1);
    step[2] = step[2] / (pTask->length >> 1);
    pTask->curR = pTask->targetR - step[0];
    pTask->curG = pTask->targetG - step[1];
    pTask->curB = pTask->targetB - step[2];
}

void BattleFreeScreenFadeTask(u8 *pEntry) {
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    g_BattleScreenFadeTask = NULL;
}

// Draws the fade as one semi-transparent quad over the whole screen. The 0x50
// asked of the work buffer is more than the quad and its draw mode take.
void BattleDrawScreenFade(WorkListEntry *pEntry) {
    BattleScreenFadeTask *pTask = pEntry->unk4;
    u8 *pBuf = g_GfxCurWorkBuffer;
    POLY_F4 *pPoly;
    DR_MODE *pMode;

    if ((u8 *)(pBuf + 0x50) >= g_GfxCurWorkBufferEnd) {
        return;
    }
    pPoly = (POLY_F4 *)pBuf;
    pMode = (DR_MODE *)(pBuf + sizeof(POLY_F4));
    g_GfxCurWorkBuffer = pBuf + sizeof(POLY_F4) + sizeof(DR_MODE);
    SetPolyF4(pPoly);
    SetSemiTrans(pPoly, 1);
    pPoly->r0 = pTask->curR;
    pPoly->g0 = pTask->curG;
    pPoly->b0 = pTask->curB;
    pPoly->x0 = -0x20;
    pPoly->y0 = -0x20;
    pPoly->x1 = 0x140;
    pPoly->y1 = -0x20;
    pPoly->x2 = -0x20;
    pPoly->y2 = 0xF0;
    pPoly->x3 = 0x140;
    pPoly->y3 = 0xF0;
    SetDrawMode(pMode, 0, 0, GetTPage(0, pTask->abr, 0, 0), NULL);
    AddPrim(g_GfxCurOT + 2, pPoly);
    AddPrim(g_GfxCurOT + 2, pMode);
}

void BattleStartScreenFadeOwnBuffer(s32 a0, s32 a1, u8 a2, u8 a3, u8 a4) {
    g_BattleScreenFadeOwnBuffer = 1;
    func_800B39C0(a0, a1, a2, a3, a4);
    g_BattleScreenFadeOwnBuffer = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B39C0);

s32 BattleGetActorFlag41(void) {
    if (g_BattleScreenFadeTask != NULL) {
        return g_BattleScreenFadeTask->abr;
    }
    return 1;
}

// best 88.68% under cdk -G0, cdk -G8 and 2.6.0: register-only differences (which
// of the two $a0/$a1 copies feeds each load), same instruction multiset. Retail's
// else path reads the argument from $a0, leaving the branch delay slot empty
// where reorg fills ours.
/*
void func_800B3B94(u8 *pArg) {
    s32 n;
    s32 num;
    s32 base;
    s32 delta;

    if (*(s32 *)(pArg + 0x3C) != 0) {
        n = *(s32 *)(pArg + 0x3C) - 1;
        num = n * 0x20 / *(s32 *)(pArg + 0x38);
        base = *(s16 *)(pArg + 0x42);
        delta = (base - *(s16 *)(pArg + 0x40)) * num;
        *(s32 *)(pArg + 0x3C) = n;
        *(s16 *)(pArg + 0x46) = base - delta / 0x20;
    } else if (*(s16 *)(pArg + 0x46) == 0) {
        ((void (*)(u8 *))*(u32 *)(pArg + 0xC))(pArg);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3B94);

void BattleFreeImageTask(u8 *pEntry) {
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
    g_BattleImageTask = 0;
    BattleUploadAndFreeImageResources();
}

// Re-applies the colour tint the image task eases whenever its value has moved
// since the last application.
void func_800B3C74(WorkListEntry *pEntry) {
    BattleImageTask *pTask = pEntry->unk4;

    if (pTask->applied != pTask->current) {
        pTask->applied = *(u16 *)&pTask->current;
        BattleSetActorRectParams(0, pTask->blendMode, 0x20 - pTask->current,
                      pTask->r, pTask->g, pTask->b);
    }
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3CD4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3E04);
