#include "common.h"
#include "psyq/libgte.h"
#include "system/animation.h"

extern void func_8001F6B0(void *pObj);
extern void func_80025180(void *pData);
extern void WorkListsRemoveEntriesForData(void *pTask);
extern void TimerWorkListRemoveTask(void *pTask);
// left unprototyped: the ramp passes a third argument the callee (misc31) does not read
extern void BattleSetSpriteColorFromAngle();
extern MATRIX g_BattleViewMatrix;
extern u8 g_BattleStateWork[];

/* Built with XenoBattleTailCdk0 (gcc-2.7.2-cdk, -G0). With -G0 the cdk cc1 splits
   every absolute symbol access itself (`lui $r,%hi(sym)` / `%lo(sym)($r)`, any free
   register), which is what retail's "atfree" stubs contain; at -G8 it leaves the
   bare macro to the assembler and emits `lui $at`. */

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCC60);
extern s16 g_BattleActorMask;

void BattleClearActorMask(void) {
    g_BattleActorMask = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCD98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCEAC);
// The sprite record a color-ramp task animates.
typedef struct {
    /* 0x00 */ u8 unk0[0x2];
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u8 unk4[0x2];
    /* 0x06 */ s16 unk6;
    /* 0x08 */ u8 unk8[0x2];
    /* 0x0A */ s16 unkA;
    /* 0x0C */ u8 unkC[0x1C];
    /* 0x28 */ u8 unk28[0x3]; // colors, written by BattleSetSpriteColorFromAngle
    /* 0x2B */ u8 unk2B;
} BattleEffectSprite; // Size: 0x2C

// The color-ramp task's own data block, reached through the task entry's +0x4.
typedef struct {
    /* 0x00 */ u8 unk0[0x38];
    /* 0x38 */ BattleEffectSprite *pSprite;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ u8 unk48[0x6];
    /* 0x4E */ u16 unk4E;
    /* 0x50 */ u8 unk50[0x4];
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58; // ramp angle, advanced once per step
    /* 0x5C */ void *unk5C;
} BattleEffectColorRampData; // Size: 0x60

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ BattleEffectColorRampData *pData;
} BattleEffectTask; // Size: 0x8

void BattleApplyEffectSpriteColorRamp(BattleEffectTask *pTask) {
    BattleEffectColorRampData *pData;
    BattleEffectSprite *pSprite;
    s32 timer;
    s16 unk6;
    s16 unkA;

    pData = pTask->pData;
    pSprite = pData->pSprite;
    timer = pData->unk58 + 1;
    pData->unk3C = pSprite->unk2;
    unk6 = pSprite->unk6;
    pData->unk40 = unk6; // retail stores unk6, then overwrites it below
    unkA = pSprite->unkA;
    pData->unk44 = unkA;
    pData->unk58 = timer;
    // kept after the timer store: retail loads 0x54 in that slot
    pData->unk40 = unk6 - pData->unk54 - 0x20;
    BattleSetSpriteColorFromAngle(pSprite, timer, unkA);
    pData->unk4E += 0x10;
}

void BattleFreeEffectSpriteTask(BattleEffectTask *pTask) {
    BattleEffectColorRampData *pData;
    BattleEffectSprite *pSprite;

    pData = pTask->pData;
    pSprite = pData->pSprite;
    pSprite->unk2B |= 1;
    func_8001F6B0(pSprite);
    func_80025180(pData->unk5C);
    WorkListsRemoveEntriesForData(pTask);
    TimerWorkListRemoveTask(pTask);
    HeapFree(pData);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD098);
extern s32 g_BattleCommandRowIndex;
extern void *D_800C3E1C;
extern s32 D_800C3D38;
extern s32 g_BattleColorMode;
extern void func_800BD3AC(void *pSprite, u16 arg1, u8 arg2);
extern void func_800BD1FC(s32 slot);

// best 91.07%: the instruction multiset and the frame are retail's; what is left
// is register choice and statement order. Retail loads the call's last argument
// (`lbu a2,0x150(a1)`) before the two global stores, keeps the sprite in $t0 (so
// $a0 can hold the 0x174 byte) and puts the row sum straight into $v0 in the tail.
// Tried: direct call arguments instead of the `kind` local, pByte/pHalf swapped in
// the declaration list, an `s32 offset` local, an `s32 row` local (that one
// restructures the head), and both pointer-cast spellings of the row base.
// Realistic form: the row is a 0x48-byte record of four 11-entry arrays (u16 at
// +0x138, u8 at +0x150, u16 at +0x15C, u8 at +0x174 - the block misc12 reaches as
// g_BattleCommandRows[]), indexed by g_BattleCommandRowIndex - 1; a slot's sprite is at +0x8C8C.
// 91.07%: COMPILES (externs added: D_800C3D38, g_BattleColorMode, D_800C3E1C,
// func_800BD3AC prototype). Register tie + $a1 vs $v0 in the tail's row sum.
/*
void func_800BD1FC(s32 slot) {
    u8 *pBase;
    u8 *pHalf;
    u8 *pByte;
    s32 offset;
    s32 kind;
    void **pEntry;

    pBase = g_BattleStateWork;
    pRow = pBase + (g_BattleCommandRowIndex - 1) * 0x48;
    offset = slot * 2;
    pByte = pRow + slot;
    if (pByte[0x150] == 0xFF) {
        return;
    }
    pHalf = (u8 *)(offset + pRow);
    if (*(u16 *)(pHalf + 0x138) == 0xFFFF) {
        return;
    }
    pSprite = *(void **)(pBase + slot * 4 + 0x8C8C);
    if (pSprite == NULL) {
        return;
    }
    kind = pByte[0x150];
    D_800C3D38 = *(u16 *)(pHalf + 0x15C);
    g_BattleColorMode = pByte[0x174];
    func_800BD3AC(pSprite, *(u16 *)(pHalf + 0x138), kind);
    pRow = pBase + (g_BattleCommandRowIndex - 1) * 0x48;
    *(u16 *)(offset + pRow + 0x138) = 0xFFFF;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD1FC);

void BattleUpdateCommandRows(void) {
    u8 *pSlot;
    u8 *pBase;
    void **pEntry;
    u8 flag;
    mem_addr rowAddr;
    s32 i;

    flag = 0;
    i = 0;
    pSlot = g_BattleStateWork;
    pBase = pSlot;
    while (i != 0xB) {
        rowAddr = (g_BattleCommandRowIndex - 1) * 0x48 + (mem_addr)pBase + i + 0x150;
        if (*(u8 *)rowAddr == 7) {
            flag = 1;
        }
        pEntry = (void **)(pSlot + 0x8C8C);
        if (*pEntry != NULL && (D_800C3E1C != *pEntry || !flag)) {
            // Two copies of the call: loop.c counts the extra branch before jump
            // opt merges them, which is what keeps the g_BattleCommandRowIndex address from
            // being hoisted out of the loop.
            if (flag) {
                func_800BD1FC(i);
            } else {
                func_800BD1FC(i);
            }
        }
        pSlot += 4;
        i++;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD3AC);
// An entry of the work list g_BattleTaskUnlinkList heads (the onFreeCallback list): only the
// next link is used by this unit.
typedef struct Misc32TaskNode {
    /* 0x00 */ u8 unk0[0x38];
    /* 0x38 */ struct Misc32TaskNode *pNext;
} Misc32TaskNode; // Size: 0x3C

extern Misc32TaskNode *g_BattleTaskUnlinkList;
extern void WorkListsDeleteTasks(WorkListEntry *pTasks);

// The g_BattleTaskUnlinkList list's on-free callback: unlink the entry, then free it.
void BattleUnlinkAndFreeTask(void *pEntry) {
    Misc32TaskNode *pCur;
    Misc32TaskNode *pPrev;
    Misc32TaskNode *pTarget;

    pCur = g_BattleTaskUnlinkList;
    pPrev = NULL;
    pTarget = pEntry;
    while (pCur != NULL) {
        if (pCur == pTarget) {
            if (pPrev != NULL) {
                pPrev->pNext = pCur->pNext;
            } else {
                g_BattleTaskUnlinkList = pCur->pNext;
            }
            break;
        }
        pPrev = pCur;
        pCur = pCur->pNext;
    }
    WorkListsDeleteTasks(pEntry);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD810);

void BattleProjectPointToScreen(SVECTOR *pVec, s32 *pOut) {
    SVECTOR vec; // unused: reproduces retail's 0x40 frame
    s16 sxy[4];
    s32 p;
    s32 flag;
    s32 ofx;
    s32 ofy;

    SetRotMatrix(&g_BattleViewMatrix);
    SetTransMatrix(&g_BattleViewMatrix);
    RotTransPers(pVec, (s32 *)sxy, &p, &flag);
    ReadGeomOffset(&ofx, &ofy);
    pOut[0] = (sxy[0] - ofx) * 2;
    pOut[1] = (sxy[1] - ofy) * 2;
    pOut[2] = ReadGeomScreen();
}
// The GTE geom-output block g_BattleGeomMatrix doubles as a matrix: its t[2] word is the
// geom screen offset (misc33 declares the same struct as BattleGeomMatrix).
typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ s32 geomScreen;
} BattleGeomMatrix; // Size: 0x20

extern BattleGeomMatrix g_BattleGeomMatrix;
extern void func_800BD810(u8 *pEntry, u8 *pData);

// 96.95% under this unit's cdk preset (89.20 on 2.7.2). Left: (1) retail keeps the
// g_BattleGeomMatrix base in $s0 and stores `sw $v0,0x1C($s0)`, reusing $s0 for CompMatrix;
// gcc folds the store into one address or goes through $at. (2) retail loads the
// three SVECTOR halves with `lh`, gcc with `lhu`. (3) retail's `i = 0` lands after
// the ReadGeomScreen block, gcc hoists it into the delay slot. The declarations
// above must stay LIVE for this body to build (they used to sit inside the block).
/*
void func_800BDA1C(u8 *pTask) {
    u8 *pData;
    MATRIX mtx;
    SVECTOR vec;
    s32 sxy[3];
    s32 i;
    u8 *pEntry;

    pData = *(u8 **)(pTask + 0x4);
    g_BattleGeomMatrix.geomScreen = ReadGeomScreen();
    vec.vx = *(s16 *)(pData + 0x4A);
    vec.vy = *(s16 *)(pData + 0x4E);
    vec.vz = *(s16 *)(pData + 0x52);
    BattleProjectPointToScreen(&vec, sxy);
    RotMatrix((SVECTOR *)(pData + 0x40), &mtx);
    TransMatrix(&mtx, sxy);
    CompMatrix((MATRIX *)&g_BattleGeomMatrix, &mtx, &mtx);
    ScaleMatrix(&mtx, (VECTOR *)(pData + 0x58));
    SetRotMatrix(&mtx);
    SetTransMatrix(&mtx);
    i = 0;
    pEntry = pData + 0x90;
    while (i != *(s32 *)(pData + 0x8C)) {
        func_800BD810(pEntry, *(u8 **)(pData + 0x80));
        i++;
        pEntry += 0x18;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDA1C);
// A battle effect driven by the timer work list. Both work-list entries sit inside
// the task allocation: the timer entry at 0x0 (TimerWorkListSetTaskCallback and
// TimerWorkListRemoveTask take the task pointer itself) and the render entry at
// 0x1C, which BattleFreeTimerTask unlinks on its own.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry workEntry;
    /* 0x38 */ u8 unk38[0x8];
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 position;
    /* 0x4C */ u8 unk4C[0x14];
    /* 0x60 */ u8 unk60[0x3];
    /* 0x63 */ u8 unk63;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ u8 unk68[0x14];
    /* 0x7C */ u8 unk7C; // nonzero: position steps the other way
    /* 0x7D */ u8 unk7D[0x3];
    /* 0x80 */ u8 unk80[0x3]; // colors, faded by BattleStepEffectColorFade
    /* 0x83 */ u8 unk83;
    /* 0x84 */ u32 unk84;
    /* 0x88 */ s32 timer;
} Misc32Task; // Size: 0x8C

// Declared after the struct: the timer update takes the task. It reads none of
// the argument's fields, it works off g_BattleTimerTask.
extern void func_800BDF1C(Misc32Task *pTask);

void BattleStepEffectColorFade(Misc32Task *pTask) {
    WorkListCallback_t pCallback;

    func_800BDF1C(pTask);
    pTask->unk80[0] -= 2;
    pTask->unk80[1] -= 2;
    pTask->unk80[2] -= 2;
    if (--pTask->timer < 0) {
        pCallback = pTask->timerEntry.onFreeCallback;
        pCallback(pTask);
    }
}

extern u8 SpriteColorAddClamp(u8 arg0, s16 arg1);

// The effect task, reached through its own alias: retail keeps a second copy of the
// task pointer in $s1 (0x20 frame) and makes the callback through it, so the fade
// stores and the timer update must not share one variable. The two volatile
// re-reads are required too: without them cse forwards call 1's stored result into
// call 2's argument (86.00 instead of 100.00).
void BattleStepEffectColorFadeFromBase(Misc32Task *pTask) {
    Misc32Task *pEffect;
    WorkListCallback_t pCallback;

    pEffect = pTask;
    func_800BDF1C(pTask);
    // the volatile re-reads are required: without them cse forwards the previous
    // call's stored result into the next call's argument
    pEffect->unk80[0] = SpriteColorAddClamp(pEffect->unk80[0], -8);
    pEffect->unk80[1] = SpriteColorAddClamp(*(volatile u8 *)&pTask->unk80[0], -8);
    pEffect->unk80[2] = SpriteColorAddClamp(*(volatile u8 *)&pEffect->unk80[0], -8);
    if (--pTask->timer < 0 || (pTask->unk80[0] | pTask->unk80[1] | pEffect->unk80[2]) == 0) {
        pCallback = pTask->timerEntry.onFreeCallback;
        pCallback(pEffect);
    }
}

extern void TimerWorkListSetTaskCallback(Misc32Task *pTask, void (*pCallback)(Misc32Task *));
extern void BattleStepEffectColorFadeFromBase(Misc32Task *pTask);

// 91.60%: COMPILES (parked body below). `nextTimer` is worth +8.2 (it stops gcc
// folding 0x10 into the branch); the rest of the block is retail's, per the diff.
// CAUSE (round 115, from the cc1 -da dumps: misc32.flow vs misc32.lreg): the
// argument copy `(set (reg 4 a0) (reg 80))` is the parameter's LAST use, so
// local-alloc's combine_regs merges reg 80 into $a0, rewrites the two stores to
// `sw v1,136(a0)` / `sb v0,131(a0)` and reorders the block to put the copy first.
// Retail's own object proves combine_regs did not fire there: its `move $a0,$s0`
// sits at block position 3, before its `sw v1,136(s0)`, so $a0 is defined before
// those stores and they still use $s0. The lever is therefore a shape in which the
// parameter is still live when the copy is scanned, or one whose stores reach the
// base via a pseudo combine_regs cannot merge into $a0.
// Tried (all 91.60 or worse): a `u8 flags` temp, both store orders, the whole block
// in do{...}while(0) (55.56), the call alone in do{...}while(0) (67.40), a `pEffect`
// alias for the call argument (cse folds it), an alias for the stores, u8 pointer,
// offset casts, a callback local, indexed forms, and a 3-min permuter run.
// Round 116: moving the unk83 STORE after the call (compute the byte into a `u8
// flags` local before it, `pTask->unk83 = flags;` after it) DOES defeat
// combine_regs - the parameter then stays in a saved register with a real
// `move a0,sN` argument copy - but the flag byte then lives across the call and
// needs a SECOND saved register (0x20 frame, 108 bytes, 70.36%). Wrapping only the
// timer store in do{...}while(0), the lever that closed BattleStepEffectPosition
// next door, reproduces that function's shape instead (store straight after the
// bgez, so reorg steals the load-delay nop: 96 bytes, 68.96%). A 4-min permuter run
// from the 91.60 body: base=70 best=70, 1645 iterations, nothing. Retail's object
// has the same instruction multiset as the 91.60 body; the only differences are
// $a0 vs $s0 on the sw/sb and the position of the move, i.e. a local-alloc choice.
/*
void func_800BDC14(Misc32Task *pTask) {
    s32 nextTimer;

    func_800BDF1C(pTask);
    nextTimer = 0x10;
    if (--pTask->timer < 0) {
        pTask->timer = nextTimer;
        pTask->unk83 = (pTask->unk83 | 2) & 0xFE;
        TimerWorkListSetTaskCallback(pTask, BattleStepEffectColorFadeFromBase);
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDC14);

extern void func_800BDC14(Misc32Task *pTask);

void BattleStepEffectPosition(Misc32Task *pTask) {
    func_800BDF1C(pTask);
    if (pTask->unk7C != 0) {
        pTask->position += 0xC0000;
    } else {
        pTask->position -= 0xC0000;
    }
    if (--pTask->timer < 0) {
        do { /* the do-while stops reorg hoisting the call's move a0,s0 */
            pTask->timer = 0x10;
        } while (0);
        TimerWorkListSetTaskCallback(pTask, func_800BDC14);
    }
}
extern Misc32Task *g_BattleTimerTask;
extern void WorkListRemoveTask(WorkListEntry *pEntry);

void BattleFreeTimerTask(Misc32Task *pTask) {
    g_BattleTimerTask = NULL;
    WorkListRemoveTask(&pTask->workEntry);
    TimerWorkListRemoveTask(&pTask->timerEntry);
}
void BattleTimerTaskNoOp(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDD3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDE58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDF1C);
extern void BattleFreeTimerTask(Misc32Task *pTask);

void BattleFreeTimerTaskIfActive(void) {
    if (g_BattleTimerTask != NULL) {
        BattleFreeTimerTask(g_BattleTimerTask);
    }
}
extern s32 D_800C374C;

void BattleResetTimerTaskGlobals(void) {
    g_BattleTimerTask = NULL;
    D_800C374C = 0;
}
