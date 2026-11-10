#include "common.h"
#include "psyq/libgte.h"

extern s32 D_80059464;
extern s16 D_800D3678;
extern s32 D_800D363C[];
extern s32 D_800D3640[];
extern u16 D_800D2D4C;
extern u16 D_800D36BC;
extern s32 D_800C3610;
extern s32 D_800C3628;
extern s32 D_800C3CE8;
extern s32 D_800D2D68;

extern void func_800A9FF0(s32);
s32 func_800BF720(void);
extern void func_800BD2E4(void);
extern void *func_800BFC80(void *, s32, s32);
extern long func_800B57E4(u8 *pActor);
extern u8 *TimerWorkListAllocateTask(void *pList, s32 size);
extern void TimerWorkListSetTaskCallback(void *pTask, void *pCallback);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF4F0);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF5E8);
// 55.00%: retail keeps the address in one register and reuses it for the load
// and the store (lui $v1; lw $v0,%lo($v1); sw $v0,%lo($v1)). gcc emits the bare
// `sw $2,D_800C3CE8` and maspsx expands it with $at, so the store cannot reuse
// the address register. Not affected by --dont-expand-li (XenoBattleTail).
/*
void func_800BF5E8(void) {
    D_800C3CE8++;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF600);

void func_800BF6CC(void) {
    if (D_800C3610 != 0) {
        func_800BD2E4();
    }
}

s32 func_800BF6F8(void) {
    return D_80059464 - func_800BF720();
}

s32 func_800BF720(void) {
    return D_800D2D68 != 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF730);
// 45.00%: same maspsx store-expansion wall as func_800BF5E8 - retail emits
// `lui $v0` itself and puts the `sw` in the jr delay slot.
/*
void func_800BF730(s32 arg0) {
    D_800C3628 = arg0;
}
*/

void func_800BF73C(u8 *pArg) {
    u8 *pTask;
    s32 prev;
    s32 dist;

    pTask = pArg;
    prev = *(s32 *)(pArg + 0x24);
    dist = func_800B57E4(*(u8 **)(pArg + 0x1C));
    *(s32 *)(pArg + 0x24) = dist;
    if (prev < dist || dist < *(s32 *)(pArg + 0x28)) {
        ((void (*)(u8 *)) * (void **)(pTask + 0x2C))(*(u8 **)(pTask + 0x1C));
        ((void (*)(u8 *)) * (void **)(pTask + 0xC))(pTask);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF7C8);
// 100.00% under gcc-2.7.2-cdk-psx, 92.97% under the unit's gcc-2.7.2: the only
// difference is the callback argument order, i.e. reorg stealing the
// `addiu a1,%lo(func_800BF73C)` into the jal delay slot, which 2.7.2 cannot do
// because it emits the address as a single `la`. Same symptom as misc27's
// func_800B5924; every function matched in this unit stays 100.00% under cdk.
/*
void func_800BF7C8(u8 *pActor, s32 arg1, s32 arg2) {
    u8 *pTask;

    pTask = TimerWorkListAllocateTask(*(void**)(pActor + 0x6C), 0x14);
    TimerWorkListSetTaskCallback(pTask, func_800BF73C);
    *(s32*)(pTask + 0x2C) = arg2;
    *(u8**)(pTask + 0x1C) = pActor;
    *(s32*)(pTask + 0x20) = *(s8*)(pActor + 0xAF);
    *(s32*)(pTask + 0x24) = func_800B57E4(pActor);
    *(s32*)(pTask + 0x28) = arg1;
    *(u32*)(pActor + 0xAC) |= 0x20;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF85C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF8CC);
// 92.21% under gcc-2.7.2-cdk-psx, 76.59% under the unit's 2.7.2, and the shape
// below under both: the instruction multiset matches and only the index/pointer
// registers are swapped (retail v1 = index, a1 = pointer; gcc a1/v1 the other
// way round) - the same allocator tie as func_800BF954. A `for (i = 0; i <
// D_800D3678; i++)` with the global read in the condition does NOT invert to
// retail's `beqz` guard + `bne` back edge, so the count has to be a local.
/*
void func_800BF8CC(u8 *pActor) {
    s32 count;
    s32 i = 0;
    s32 *pEntry;

    count = D_800D3678;
    if (count != 0) {
        pEntry = D_800D363C;
        for (i = 0; i != count; i++) {
            if (pEntry[i] == *(s32*)(pActor + 0x74)) {
                break;
            }
        }
    }
    if (i >= D_800D3678) {
        *(s32*)(pActor + 0x74) = D_800D363C[0];
    } else {
        pEntry = D_800D3640;
        *(s32*)(pActor + 0x74) = pEntry[i];
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF954);
// 96.76%: identical apart from an allocator tie - retail keeps the index in
// $a1 and the advancing pointer in $v1, gcc 2.7.2 swaps them (four source
// shapes tried: while/for, indexed/deref, decl order, split increments).
/*
s32 func_800BF954(s32 arg0) {
    s16 count = D_800D3678;
    s32 i = 0;

    if (count != 0) {
        s32 *pEntry = D_800D363C;

        for (i = 0; i != count; i++) {
            if (pEntry[i] == arg0) {
                break;
            }
        }
    }
    return i;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF998);
// Untested (the round ended before the preset fix landed).
/*
void func_800BF998(void) {
    s16 first = D_800D2D4C + 1;
    s16 second = D_800D36BC + 1;

    D_800D2D4C = first;
    D_800D36BC = second;
    if (first == 2) {
        func_800A9FF0(0xB);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF9EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFA9C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFBA0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFC80);

void func_800BFD88(void *arg0, s32 arg1) {
    func_800BFC80(arg0, arg1, 1);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFDA8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFE48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0314);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800C0564);
s32 func_800C06E4(VECTOR *pVec0, VECTOR *pVec1) {
    VECTOR v;

    v.vx = pVec0->vx - pVec1->vx;
    v.vy = pVec0->vy - pVec1->vy;
    v.vz = pVec0->vz - pVec1->vz;
    Square0(&v, &v);
    return SquareRoot0(v.vy + v.vz + v.vx);
}

s32 func_800C0758(SVECTOR *pVec0, SVECTOR *pVec1) {
    VECTOR v;

    v.vx = pVec0->vx - pVec1->vx;
    v.vy = pVec0->vy - pVec1->vy;
    v.vz = pVec0->vz - pVec1->vz;
    Square0(&v, &v);
    return SquareRoot0(v.vy + v.vz + v.vx);
}

s32 func_800C07CC(DVECTOR vec0, DVECTOR vec1) {
    VECTOR v;

    v.vx = vec0.vx - vec1.vx;
    v.vz = vec0.vy - vec1.vy;
    Square0(&v, &v);
    return SquareRoot0(v.vx + v.vz);
}
