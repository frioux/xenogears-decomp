#include "common.h"
#include "psyq/libgte.h"

extern void func_800BDF1C(u8 *pEntry);
extern void func_8001F6B0(u8 *pObj);
extern void func_80025180(u8 *pData);
extern void func_8001CE74(u8 *pTask);
extern void TimerWorkListRemoveTask(u8 *pTask);
extern MATRIX D_800D30BC;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCC60);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCD8C);
/* best 45.00%: retail's aspsx expands the bare `sh $zero,SYM` macro into
   `lui v0,%hi; sh $zero,%lo(v0)` and fills the jr delay slot with the store.
   gcc leaves the macro to GNU as, which emits `lui $at` + `sh 0($at)`: two
   insns, so the delay slot stays empty and the register differs.
extern s16 D_800C3D14;

void func_800BCD8C(void) {
    D_800C3D14 = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCD98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCEAC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCFAC);
/* best 87.20%: instruction multiset identical, ordering only. Retail schedules
   the 0x58 load (and the `addiu a1,a1,1` on it) before the 0x54 load and the
   0xA load; gcc hoists the 0x54 load into the slot after the 0x38 load and
   leaves the 0x58 group late. Tried: ++field inline in the call, a separate
   ++ statement plus a re-read, a timer local read before the stores, unk6/unkA
   locals (needed - without them gcc reloads both and it drops to 70%).
extern void func_800BCAFC(u8 *pSrc, s32 arg1, s32 arg2);

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
    *(s32 *)(pData + 0x40) = unk6;
    *(s32 *)(pData + 0x40) = unk6 - *(s32 *)(pData + 0x54) - 0x20;
    unkA = *(s16 *)(pSrc + 0xA);
    *(s32 *)(pData + 0x44) = unkA;
    *(s32 *)(pData + 0x58) = timer;
    func_800BCAFC(pSrc, timer, unkA);
    *(u16 *)(pData + 0x4E) += 0x10;
}
*/

void func_800BD024(u8 *pTask) {
    u8 *pData;
    u8 *pObj;

    pData = *(u8 **)(pTask + 0x4);
    pObj = *(u8 **)(pData + 0x38);
    pObj[0x2B] |= 1;
    func_8001F6B0(pObj);
    func_80025180(*(u8 **)(pData + 0x5C));
    func_8001CE74(pTask);
    TimerWorkListRemoveTask(pTask);
    HeapFree(pData);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD098);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD1FC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD2E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD3AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD7A0);
/* best 86.25%: retail materialises the head symbol once, `lui a2,%hi` at the
   top, and folds `%lo(D_800C3750)` into both the load and the store through
   $a2. gcc emits the bare asm macro for the load (gas expands it with $v0/$at)
   and re-forms the address with `lui $at` for the store. Going through a local
   `u8 **ppHead = &D_800C3750` gives lui+addiu in $a1 instead. Retail also
   copies the parameter to $a1 at entry; gcc compares $a0 directly.
extern void WorkListsDeleteTasks(void);
extern u8 *D_800C3750;

void func_800BD7A0(u8 *pEntry) {
    u8 *pCur;
    u8 *pPrev;

    pPrev = NULL;
    pCur = D_800C3750;
    while (pCur != NULL) {
        if (pCur == pEntry) {
            if (pPrev != NULL) {
                *(u8 **)(pPrev + 0x38) = *(u8 **)(pCur + 0x38);
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

void func_800BDB08(u8 *pTask) {
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
   coalesced away, and cdk scores the same 90.5.
   The two `volatile` reads ARE needed: without them cse forwards call 1's stored
   result into call 2's argument (`andi $a0,$v0,0xff`, with the store deferred
   into the next call's delay slot) and the score drops to 86.00; retail reloads
   the byte. With them the instruction multiset is identical.
extern u8 func_80021AD8(u8 arg0, s16 arg1);

void func_800BDB74(u8 *pTask) {
    void (*pCallback)(u8 *);

    func_800BDF1C(pTask);
    pTask[0x80] = func_80021AD8(pTask[0x80], -8);
    pTask[0x81] = func_80021AD8(*(volatile u8 *)(pTask + 0x80), -8);
    pTask[0x82] = func_80021AD8(*(volatile u8 *)(pTask + 0x80), -8);
    if (--*(s32 *)(pTask + 0x88) < 0 || (pTask[0x80] | pTask[0x81] | pTask[0x82]) == 0) {
        pCallback = *(void (**)(u8 *))(pTask + 0xC);
        pCallback(pTask);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDC14);
/* best 83.40%: only the base register of the four accesses in the `if` body
   differs. gcc hoists the call's `move $4,$16` to the top of the block and
   addresses through $4; retail emits the copy at the call and stays on $16.
   Tried: u8* with pEntry+0x88 casts, a struct type, a pTask copy, a callback
   local, indexed and offset forms - all give $4.
extern void func_800BDF1C(u8 *pEntry);
extern void TimerWorkListSetTaskCallback(void *pEntry, void *pCallback);
extern void func_800BDB74(void);

typedef struct {
    u8 unk0[0x83];
    u8 unk83;
    u32 unk84;
    s32 unk88;
} Misc32Task;

void func_800BDC14(Misc32Task *pTask) {
    func_800BDF1C((u8 *)pTask);
    if (--pTask->unk88 < 0) {
        pTask->unk88 = 0x10;
        pTask->unk83 = (pTask->unk83 | 2) & 0xFE;
        TimerWorkListSetTaskCallback(pTask, func_800BDB74);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDC78);
/* best 87.50%: the whole body matches; only the callback address is ordered
   differently. Retail emits `lui a1,%hi; move a0,s0; jal; addiu a1,%lo`, i.e.
   the %hi/%lo pair is split around the move - the cdk two-insn `la`. gcc 2.7.2
   emits `la` as one macro that maspsx expands adjacently, so reorg steals the
   `move a0,s0` into the slot instead. gcc-2.7.2-cdk scores 93.75, still not
   100 (the addiu stays next to the lui there too). Tried: the address in a
   function-pointer local assigned last.
extern void TimerWorkListSetTaskCallback(u8 *pTask, void (*pCallback)(u8 *));
extern void func_800BDC14(u8 *pTask);

void func_800BDC78(u8 *pTask) {
    func_800BDF1C(pTask);
    if (pTask[0x7C] != 0) {
        *(s32 *)(pTask + 0x48) += 0xC0000;
    } else {
        *(s32 *)(pTask + 0x48) -= 0xC0000;
    }
    if (--*(s32 *)(pTask + 0x88) < 0) {
        *(s32 *)(pTask + 0x88) = 0x10;
        TimerWorkListSetTaskCallback(pTask, func_800BDC14);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDCF8);
/* best 81.60%: same $at macro-store problem as func_800BCD8C. Retail hoists
   `lui v0,%hi(D_800D2D68)`, expands the store with $v0 and reorg fills the
   first jal delay slot with it; GNU as' `lui $at` expansion cannot be stolen.
extern u8 *D_800D2D68;
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);

void func_800BDCF8(u8 *pEntry) {
    D_800D2D68 = 0;
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
}
*/
void func_800BDD34(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDD3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDE58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDF1C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BE0DC);
/* best 99.09%: the load's macro expansion only. Retail has
   `lui v0,%hi; lw a0,%lo(v0)`; GNU as puts the address in the destination
   register (`lui a0; lw a0,0(a0)`).
extern u8 *D_800D2D68;
extern void func_800BDCF8(u8 *pEntry);

void func_800BE0DC(void) {
    if (D_800D2D68 != NULL) {
        func_800BDCF8(D_800D2D68);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BE108);
/* best 65.00%: same $at macro-store problem, two stores; retail uses $v0 twice
   and fills the jr delay slot with the second.
extern u8 *D_800D2D68;
extern s32 D_800C374C;

void func_800BE108(void) {
    D_800D2D68 = NULL;
    D_800C374C = 0;
}
*/
