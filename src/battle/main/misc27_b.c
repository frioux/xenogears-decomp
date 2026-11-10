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

// Split out of main/misc27 (see config/battle.yaml): compiled with gcc 2.7.2-cdk.
u8* func_800B5924(u8* pActor, s32 arg1, s32 arg2) {
    u8* pTask;

    pTask = TimerWorkListAllocateTask(*(void**)(pActor + 0x6C), 0x18);
    TimerWorkListSetTaskCallback(pTask, func_800B5854);
    *(u8**)(pTask + 0x1C) = pActor;
    *(s32*)(pTask + 0x28) = func_800B57E4(pActor);
    *(s32*)(pTask + 0x2C) = arg1;
    *(s32*)(pTask + 0x30) = arg2;
    *(s32*)(pTask + 0x24) = *(s8*)(pActor + 0xAF);
    *(u32*)(pActor + 0xAC) |= 0x20;
    return pTask;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B59BC);
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
} BattlePoint;

BattlePoint* func_800B5AC4(BattlePoint* pOut, u8* pActor, s32 index) {
    BattlePoint point;
    int ofs;

    func_800B59BC(&point, pActor, index);
    ofs = *(s16*)(pActor + 0x2);
    point.x = point.x + ofs;
    ofs = *(s16*)(pActor + 0x6);
    point.y = point.y + ofs;
    *pOut = point;
    return pOut;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B5B3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B5C18);
/*
void func_800B5CC0(u8* pWork) {
    MATRIX m;
    SVECTOR rot;
    SVECTOR rot2;
    VECTOR v;
    u8* pSprite;

    pSprite = *(u8**)(pWork + 0x4);
    AnimScriptTick(pSprite);
    rot2.vy = *(s32*)(pSprite + 0x10) >> 13;
    rot2.vx = 0;
    rot2.vz = *(s32*)(pSprite + 0x14) >> 13;
    rot.vx = func_80022CAC(pSprite, *(s32*)(pSprite + 0xC) >> 13);
    rot.vy = 0;
    rot.vz = 0;
    RotMatrix(&rot2, &m);
    ApplyMatrix(&m, &rot, &v);
    v.vx += *(s16*)(pSprite + 0xA0);
    v.vy += *(s16*)(pSprite + 0xA2);
    v.vz += *(s16*)(pSprite + 0xA4);
    *(u32*)(pSprite + 0x0) = v.vx << 16;
    *(u32*)(pSprite + 0x4) = v.vy << 16;
    *(u32*)(pSprite + 0x8) = v.vz << 16;
    if (*(u32*)(pSprite + 0x64) == 0) {
        ((void (*)(u8*))*(void**)(pWork + 0xC))(pWork);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B5CC0);
void func_800B5DC4(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    TimerWorkListSetTaskCallback(*(void**)(pWork + 0x6C), func_800B5CC0);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B5DF4);
void func_800B5FBC(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(pWork + 0x6C) + 0x1C, func_800B5DF4);
    pWork[0x2B] = 0x40;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B6004);
void func_800B61B0(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(pWork + 0x6C) + 0x1C, func_800B6004);
    pWork[0x2B] = 0x40;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B61F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B626C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27_b", func_800B62C8);
void func_800B639C(u8* arg0, u8* pScript) {
    BattleScriptOp* pOp;

    pOp = (BattleScriptOp*)(pScript + ((*(s8*)(pScript + 1) << 8) | pScript[0]));
    func_800B3CD4(pOp->unk5, pOp->unk3, pOp->unk4, (s8)pOp->unk0, pOp->unk1, pOp->unk2);
}
