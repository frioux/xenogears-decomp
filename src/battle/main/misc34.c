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

// 55.00%: our assembler picks $at where retail picked a free register (atfree).
void BattleIncFrameCounter(void) {
    D_800C3CE8++;
}

extern void func_800B7C34(s32);
extern void func_80021BF8(u8 *pActor, void *pCallback);
extern void func_800245D8(u8 *pActor, s32 arg1);
extern void func_800BE790(void);

void BattleActorWaitFramesWithCallback(s32 arg0, u8 *pActor) {
    if (*(s32 *)(pActor + 0x48) == 0) {
        func_800B7C34(arg0);
        return;
    }
    D_800C3CE8 = 0;
    if (*(s8 *)(pActor + 0xAF) != 0) {
        func_80021BF8(pActor, BattleIncFrameCounter);
        func_800245D8(pActor, *(s8 *)(pActor + 0xAF));
    }
    func_800B7C34(arg0);
    if (*(s8 *)(pActor + 0xAF) != 0) {
        while (D_800C3CE8 == 0) {
            func_800BE790();
        }
        func_80021BF8(pActor, 0);
    }
}

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

// 45.00%: our assembler picks $at where retail picked a free register (atfree).
void func_800BF730(s32 arg0) {
    D_800C3628 = arg0;
}

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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF85C);
// 92.21% under gcc-2.7.2-cdk-psx (76.59 under 2.7.2): instruction multiset\n// matches, only the index/pointer registers are swapped. Not a preset question.
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BF8CC);
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
// 79.76%: frame differs (gcc reserves an extra slot); allocator tie.
void func_800BF998(void) {
    s16 first = D_800D2D4C + 1;
    s16 second = D_800D36BC + 1;

    D_800D2D4C = first;
    D_800D36BC = second;
    if (first == 2) {
        func_800A9FF0(0xB);
    }
}
extern u8 D_800C3621;
extern void func_800B8354(void);
extern int ArchiveDecodeAlignedSize(u32 entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void func_8002DDE4(void *p0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6);

void BattleLoadArchiveEntryFile(void) {
    void *pBuffer;

    if (D_800C3621 != 0) {
        func_800B8354();
        ArchiveSetIndex(0x2C, 0);
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(1), 0);
        ArchiveReadFileToBuffer(1, pBuffer, 0, 0x80);
        func_800B8354();
        func_8002DDE4(pBuffer, 0, 0, 0, 0, 0, 0);
        func_800BE790();
        HeapFree(pBuffer);
        D_800C3621 = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFA9C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFBA0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFC80);

void func_800BFD88(void *arg0, s32 arg1) {
    func_800BFC80(arg0, arg1, 1);
}

// 90.75%: everything matches except the D_8006BE10+0x10 load. Retail's ASPSX emits
// lui/addiu/%lo(sym) then lw 16; gcc folds the address into the load
// (lui %hi + lw 16(%hi_reg)), one instruction shorter - no C shape materialises it.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc34", func_800BFDA8);
/*
extern u8 D_800591AC;
extern u8 D_8006BE10[];
extern u8 *func_80023B84(u8 *pActor, u8 *pEntry);

void func_800BFDA8(u8 *pActor, s32 arg1) {
    u8 *pBase;
    u8 *pTable;
    u8 saved;
    u8 *pNew;
    s32 flags;

    if (*(s32 *)(pActor + 0x48) == 0) {
        return;
    }
    if (func_800BFC80(pActor, arg1, 0) != 0) {
        return;
    }
    pTable = &D_8006BE10[0];
    pBase = *(u8 **)(pTable + 0x10);
    saved = D_800591AC;
    D_800591AC = 0;
    pNew = func_80023B84(pActor, (u8 *)((s32)(arg1 + (u16 *)pBase)[1] + (s32)pBase));
    flags = *(s32 *)(pNew + 0xB0);
    *(s8 *)(pNew + 0xAF) = arg1;
    *(u8 **)(pNew + 0x74) = pActor;
    D_800591AC = saved;
    flags |= 0x100;
    *(s32 *)(pNew + 0xB0) = flags;
}
*/
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
