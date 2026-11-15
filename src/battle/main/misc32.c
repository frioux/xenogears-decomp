#include "common.h"
#include "psyq/libgte.h"

extern void func_800BDF1C(u8 *pEntry);
extern void func_8001F6B0(u8 *pObj);
extern void func_80025180(u8 *pData);
extern void WorkListsRemoveEntriesForData(u8 *pTask);
extern void TimerWorkListRemoveTask(u8 *pTask);
extern MATRIX D_800D30BC;

/* Built with XenoBattleTailCdk0 (gcc-2.7.2-cdk, -G0). With -G0 the cdk cc1 splits
   every absolute symbol access itself (`lui $r,%hi(sym)` / `%lo(sym)($r)`, any free
   register), which is what retail's "atfree" stubs contain; at -G8 it leaves the
   bare macro to the assembler and emits `lui $at`. */

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCC60);
extern s16 D_800C3D14;

void BattleClearActorMask(void) {
    D_800C3D14 = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCD98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCEAC);
void func_800BCFAC(u8 *pEntry) {
    u8 *pData;
    u8 *pSrc;
    s32 timer;
    s16 unk6;
    s16 unkA;

    pData = *(u8 **)(pEntry + 0x4);
    pSrc = *(u8 **)(pData + 0x38);
    timer = *(s32 *)(pData + 0x58) + 1;
    *(s32 *)(pData + 0x3C) = *(s16 *)(pSrc + 0x2);
    unk6 = *(s16 *)(pSrc + 0x6);
    *(s32 *)(pData + 0x40) = unk6; // retail stores unk6, then overwrites it below
    unkA = *(s16 *)(pSrc + 0xA);
    *(s32 *)(pData + 0x44) = unkA;
    *(s32 *)(pData + 0x58) = timer;
    // kept after the timer store: retail loads 0x54 in that slot
    *(s32 *)(pData + 0x40) = unk6 - *(s32 *)(pData + 0x54) - 0x20;
    func_800BCAFC(pSrc, timer, unkA);
    *(u16 *)(pData + 0x4E) += 0x10;
}

void BattleFreeEffectSpriteTask(u8 *pTask) {
    u8 *pData;
    u8 *pObj;

    pData = *(u8 **)(pTask + 0x4);
    pObj = *(u8 **)(pData + 0x38);
    pObj[0x2B] |= 1;
    func_8001F6B0(pObj);
    func_80025180(*(u8 **)(pData + 0x5C));
    WorkListsRemoveEntriesForData(pTask);
    TimerWorkListRemoveTask(pTask);
    HeapFree(pData);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD098);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD1FC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD2E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD3AC);
extern u8 *D_800C3750;
extern void WorkListsDeleteTasks(void);

// best 91.96% (permuter base 86.25; two rounds of edits, both worth having:
// the head symbol goes into a local `pHead` that pCur copies, and the pPrev test
// goes through a short `hasPrev`, plus the do-while that re-assigns pCur before the
// if-branch load). Instruction multiset is identical; what is left is register and
// slot assignment: retail zeroes $v1 (pPrev) before it loads the head into $v0 and
// copies the pEntry parameter to $a1 (`move $a1,$a0`) so the compare is on the copy.
// gcc loads the head first, keeps pCur in $v0 and compares against $a0. Ordering
// pPrev=NULL ahead of the head load drops it back to 89.82.
/*
void func_800BD7A0(u8 *pEntry) {
    u8 *pCur;
    u8 *pPrev;
    u8 *pHead;
    short hasPrev;

    pHead = D_800C3750;
    pPrev = NULL;
    pCur = pHead;
    while (pCur != NULL) {
        if (pHead == pEntry) {
            hasPrev = pPrev != NULL;
            if (hasPrev) {
                do { *(u8 **)(pPrev + 0x38) = *(u8 **)((pCur = pHead) + 0x38); } while (0);
            } else {
                D_800C3750 = *(u8 **)(pCur + 0x38);
            }
            break;
        }
        pPrev = pCur;
        pCur = *(u8 **)(pCur + 0x38);
    }
    WorkListsDeleteTasks();
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD7A0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD810);

void func_800BD974(SVECTOR *pVec, s32 *pOut) {
    SVECTOR vec; // unused: reproduces retail's 0x40 frame
    s16 sxy[4];
    s32 p;
    s32 flag;
    s32 ofx;
    s32 ofy;

    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    RotTransPers(pVec, (s32 *)sxy, &p, &flag);
    ReadGeomOffset(&ofx, &ofy);
    pOut[0] = (sxy[0] - ofx) * 2;
    pOut[1] = (sxy[1] - ofy) * 2;
    pOut[2] = ReadGeomScreen();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDA1C);
/* best 89.20%: three families of difference left.
   1. gcc 2.7.2 folds the geom-state store into one address (`lui; addiu %lo+0x1C`)
      or, through a pointer local, into `$at`; retail keeps the plain base symbol
      in $s0 and stores with `sw $v0,0x1C($s0)`, reusing $s0 for CompMatrix.
      Tried: a struct type on the extern, `u8 D_800C3760[]` + offset, a
      BattleGeomState* and a MATRIX* local (`&pGeom->t[2]`): 77.95 at worst.
   2. retail loads the three SVECTOR halves with `lh`, gcc with `lhu`.
   3. retail's `i = 0` lands after the ShadeTriangle-free prologue block, gcc
      hoists it to the ReadGeomScreen delay slot.
   Everything else (matrix chain, the 0x18-stride loop, frame) is exact.
   Under gcc-2.7.2-cdk this body is 96.95 (against 89.20 on the unit
   preset), and -G0 changes nothing: the rest is the three items above.
typedef struct {
    // 0x00
    u8 unk0[0x1C];
    // 0x1C
    s32 geomScreen;
} BattleGeomState;

extern BattleGeomState D_800C3760;
extern void func_800BD810(u8 *pEntry, u8 *pData);

void func_800BDA1C(u8 *pTask) {
    u8 *pData;
    MATRIX mtx;
    SVECTOR vec;
    s32 sxy[3];
    s32 i;
    u8 *pEntry;

    pData = *(u8 **)(pTask + 0x4);
    D_800C3760.geomScreen = ReadGeomScreen();
    vec.vx = *(s16 *)(pData + 0x4A);
    vec.vy = *(s16 *)(pData + 0x4E);
    vec.vz = *(s16 *)(pData + 0x52);
    func_800BD974(&vec, sxy);
    RotMatrix((SVECTOR *)(pData + 0x40), &mtx);
    TransMatrix(&mtx, sxy);
    CompMatrix((MATRIX *)&D_800C3760, &mtx, &mtx);
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

void BattleStepEffectColorFade(u8 *pTask) {
    void (*pCallback)(u8 *);

    func_800BDF1C(pTask);
    pTask[0x80] -= 2;
    pTask[0x81] -= 2;
    pTask[0x82] -= 2;
    if (--*(s32 *)(pTask + 0x88) < 0) {
        pCallback = *(void (**)(u8 *))(pTask + 0xC);
        pCallback(pTask);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDB74);
/* best 90.50%: only the callback argument's register is left. Retail copies the
   task into $s1 (`move $s1,$s0`) and reaches the callback through it - `lw $v0,
   0xC($s1)`, `move $a0,$s1` - while gcc uses $s0 for both. gcc 2.7.2 refuses to
   keep a second copy: an `u8 *pEntry = pTask` local used only in the `if` gets
   coalesced away. gcc-2.7.2-cdk scores the same 90.50, so this one is not a
   preset problem either.
   The two `volatile` reads ARE needed: without them cse forwards call 1's stored
   result into call 2's argument (`andi $a0,$v0,0xff`, with the store deferred
   into the next call's delay slot) and the score drops to 86.00; retail reloads
   the byte. With them the instruction multiset is identical.
extern u8 SpriteColorAddClamp(u8 arg0, s16 arg1);

void func_800BDB74(u8 *pTask) {
    void (*pCallback)(u8 *);

    func_800BDF1C(pTask);
    pTask[0x80] = SpriteColorAddClamp(pTask[0x80], -8);
    pTask[0x81] = SpriteColorAddClamp(*(volatile u8 *)(pTask + 0x80), -8);
    pTask[0x82] = SpriteColorAddClamp(*(volatile u8 *)(pTask + 0x80), -8);
    if (--*(s32 *)(pTask + 0x88) < 0 || (pTask[0x80] | pTask[0x81] | pTask[0x82]) == 0) {
        pCallback = *(void (**)(u8 *))(pTask + 0xC);
        pCallback(pTask);
    }
}
*/
extern void TimerWorkListSetTaskCallback(u8 *pTask, void (*pCallback)(u8 *));
extern void func_800BDB74(u8 *pTask);

typedef struct {
    /* 0x00 */ u8 unk0[0x83];
    /* 0x83 */ u8 unk83;
    /* 0x84 */ u32 unk84;
    /* 0x88 */ s32 unk88;
} Misc32Task; // Size: 0x8C

// best 91.60% (permuter base 83.40; the one edit worth +8.2 was assigning 0x10
// to a local before the `if`, so gcc stops folding it into the branch).
// Left: the callback block. Retail does `move $4,$16` at the jal and keeps the two
// stores on $s0; gcc hoists the copy into the bgez delay slot and stores through $4
// (`sw v1,136(a0)`). Tried: u8 pointer, struct task pointer, offset casts, a task
// copy, a callback local, indexed and offset forms - all give $4. A 3-min permuter
// run from this body made no progress (427 iterations, 70 -> 70).
// The Misc32Task typedef above is unused unless this is unparked.
/*
void func_800BDC14(Misc32Task *pTask) {
    s32 nextUnk88;

    func_800BDF1C((u8 *)pTask);
    nextUnk88 = 0x10;
    if (--pTask->unk88 < 0) {
        pTask->unk88 = nextUnk88;
        pTask->unk83 = (pTask->unk83 | 2) & 0xFE;
        TimerWorkListSetTaskCallback(pTask, func_800BDB74);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDC14);
extern void func_800BDC14(u8 *pTask);

void BattleStepEffectPosition(u8 *pTask) {
    func_800BDF1C(pTask);
    if (pTask[0x7C] != 0) {
        *(s32 *)(pTask + 0x48) += 0xC0000;
    } else {
        *(s32 *)(pTask + 0x48) -= 0xC0000;
    }
    if (--*(s32 *)(pTask + 0x88) < 0) {
        do { /* the do-while stops reorg hoisting the call's move a0,s0 */
            *(s32 *)(pTask + 0x88) = 0x10;
        } while (0);
        TimerWorkListSetTaskCallback(pTask, func_800BDC14);
    }
}
extern u8 *D_800D2D68;
extern void WorkListRemoveTask(void *pEntry);

void BattleFreeTimerTask(u8 *pEntry) {
    D_800D2D68 = 0;
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
}
void BattleTimerTaskNoOp(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDD3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDE58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDF1C);
extern void BattleFreeTimerTask(u8 *pEntry);

void BattleFreeTimerTaskIfActive(void) {
    if (D_800D2D68 != NULL) {
        BattleFreeTimerTask(D_800D2D68);
    }
}
extern s32 D_800C374C;

void BattleResetTimerTaskGlobals(void) {
    D_800D2D68 = NULL;
    D_800C374C = 0;
}
