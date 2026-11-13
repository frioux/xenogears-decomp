#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"

extern void TimerWorkListSetTaskCallback();
extern void WorkListSetTaskCallback();
extern void func_8001E148();
extern void AnimScriptTick(void* pSprite);
extern s32 func_80022CAC(void* pSprite, s32 value);
extern void func_800C08CC();
extern void func_800B3CD4();
extern void func_800B51B0();
extern void func_800B50D4();
extern void func_800B5588();
extern u8 D_800C356C[];
extern u8* WorkListsAddTasks(s32 allocSize, void* pData, void* pTimerCallback, void* pWorkCallback,
                             void* pOnFreeCallback);
extern void func_800B5CC0();
extern void func_800B5DF4();
extern void func_800B6004();
extern void func_800B59BC();
extern void func_800B5854();
extern u8* TimerWorkListAllocateTask();

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ s8 unk1;
    /* 0x2 */ s8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
} BattleScriptOp;

// Split out of main/misc27 (see config/battle.yaml): compiled with gcc 2.7.2-cdk,
// which the callback cluster in this function range needs.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B3F04);
// 84.19% under both gcc 2.7.2 and 2.7.2-cdk: the instruction set matches and the
// list scheduler places one insn differently, so this is not a preset question.
/*
void func_800B4EDC(u8* pArg) {
    RECT rect;
    u8* p;
    u16 palette;
    u16 vram;

    p = *(u8**)(*(u8**)(pArg + 0x20) + 0x30);
    palette = *(u16*)(p + 0xA);
    rect.x = (*(u8*)(p + 0x4) >> 2) + ((palette & 0xF) << 6);
    rect.w = 8;
    rect.h = 8;
    rect.y = *(u8*)(p + 0x5) + ((palette << 4) & 0x100);
    MoveImage(&rect, 0x3F0, 0x1F0);
    vram = *(u16*)(p + 0xC);
    rect.w = 0x10;
    rect.h = 1;
    rect.x = vram & 0x3F;
    rect.y = (vram >> 6) & 0x1FF;
    MoveImage(&rect, 0x3F0, 0x1EE);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B4EDC);

void func_800B4F88(u8* pActor, s32 index, MATRIX* pMatrix) {
    SVECTOR rot;
    u8* pFrame;
    s32 x;
    s32 y;
    s16 scale;

    if (*(u8**)(pActor + 0x20) == NULL) {
        return;
    }
    pFrame = (u8*)((index << 3) + *(s32*)(*(u8**)(pActor + 0x20) + 0x34));
    y = *(s8*)(pFrame + 1);
    x = *(s8*)pFrame;
    if ((*(u32*)(pActor + 0xAC) >> 2) & 1) {
        x = -x;
    }
    scale = *(s16*)(pActor + 0x2C);
    y = (y * scale) / 0x1000;
    x = (x * scale) / 0x1000;
    rot.vx = *(u16*)(pFrame + 2);
    rot.vy = *(u16*)((index << 3) + *(s32*)(*(u8**)(pActor + 0x20) + 0x34) + 4);
    rot.vz = *(u16*)((index << 3) + *(s32*)(*(u8**)(pActor + 0x20) + 0x34) + 6);
    RotMatrix(&rot, pMatrix);
    pMatrix->t[0] = *(s32*)(*(u8**)(pActor + 0x20) + 0x20) + x;
    pMatrix->t[1] = *(s32*)(*(u8**)(pActor + 0x20) + 0x24) + y;
    pMatrix->t[2] = *(s32*)(*(u8**)(pActor + 0x20) + 0x28);
    SetMulMatrix(pMatrix, (MATRIX*)(*(u8**)(pActor + 0x20) + 0xC));
}
// 90.55% under gcc 2.7.2-cdk: the instruction stream and the two combined induction
// bases match; only the saved/temp register assignment differs (retail keeps pActor in
// $t1 because its local allocator had already given $a0 to the x temporary).
/*
void func_800B50D4(u8* pActor, u8* pArg) {
    s32 x;
    s32 y;
    s32 i;
    s16 scale;
    u8* pFrame;
    s16* pOut;

    if ((*(u32*)(pActor + 0x3C) & 3) != 1) {
        return;
    }
    if (*(u8**)(pActor + 0x20) == NULL) {
        return;
    }
    if (*(u8**)(*(u8**)(pActor + 0x20) + 0x34) == NULL) {
        return;
    }
    pOut = (s16*)pArg;
    for (i = 0; i != 5; i++) {
        pFrame = *(u8**)(*(u8**)(pActor + 0x20) + 0x34) + (D_800C356C[i] << 3);
        x = *(s8*)pFrame;
        y = *(s8*)(pFrame + 1);
        if ((*(u32*)(pActor + 0xAC) >> 2) & 1) {
            x = -x;
        }
        scale = *(s16*)(pActor + 0x2C);
        x = (x * scale) / 0x2000;
        y = (y * scale) / 0x2000;
        pOut[0] = x;
        pOut[1] = y;
        pOut[2] = 0;
        pOut += 4;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B50D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B51B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5588);
void func_800B56E4(u8* pWork) {
    u8* pSprite;

    pSprite = *(u8**)(pWork + 0x4);
    func_8001E148(*(void**)(pSprite + 0x38));
    func_800C08CC(5, pSprite + 0x78, func_800B51B0);
}

void func_800B572C(u8* pActor, u8* pArg) {
    u8* pTasks;

    pTasks = WorkListsAddTasks(0xB8, *(void**)(pActor + 0x6C), func_800B5588, func_800B56E4, 0);
    *(u8**)(pTasks + 0x38) = pActor;
    *(u32*)(pTasks + 0x3C) = *(u16*)(pActor + 0x34);
    *(s32*)(pTasks + 0x40) = *(s8*)(pActor + 0xAF);
    *(u8**)(pTasks + 0x4C) = pArg;
    *(u32*)(pTasks + 0x44) = *(u8*)(pActor + 0x3C) >> 5;
    if (*(u8*)pArg == 0) {
        *(s32*)(pTasks + 0x48) = 4;
    } else {
        *(s32*)(pTasks + 0x48) = *(u8*)pArg;
    }
    func_800B50D4(pActor, pTasks + 0x78);
    func_800B50D4(pActor, pTasks + 0x50);
}
long func_800B57E4(u8* pActor) {
    VECTOR v;
    VECTOR out;

    v.vx = *(s16*)(pActor + 0xA0) - *(s16*)(pActor + 0x2);
    v.vy = *(s16*)(pActor + 0xA2) - *(s16*)(pActor + 0x6);
    v.vz = *(s16*)(pActor + 0xA4) - *(s16*)(pActor + 0xA);
    Square0(&v, &out);
    return SquareRoot0(out.vx + out.vz + out.vy);
}
