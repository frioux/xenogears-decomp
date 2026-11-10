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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B3F04);
// 84.19% under the unit preset; the instruction set matches, the list
// scheduler places one insn differently (needs gcc 2.7.2-cdk, like the rest of
// the callback cluster here).
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
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B4EDC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B4F88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B50D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B51B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5588);
// This body is 100% under gcc 2.7.2-cdk (objdiff), 95.22% under the unit's
// current gcc 2.7.2: the two compilers steal a different insn into the delay
// slot. The whole unit wants the cdk compiler; see the note at the top of the
// func_800B5DC4 stub.

/*
void func_800B56E4(u8* pWork) {
    u8* pSprite;

    pSprite = *(u8**)(pWork + 0x4);
    func_8001E148(*(void**)(pSprite + 0x38));
    func_800C08CC(5, pSprite + 0x78, func_800B51B0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B56E4);
/*
void func_800B56E4(u8* pWork) {
    u8* pSprite;

    pSprite = *(u8**)(pWork + 0x4);
    func_8001E148(*(void**)(pSprite + 0x38));
    func_800C08CC(5, pSprite + 0x78, func_800B51B0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B572C);
long func_800B57E4(u8* pActor) {
    VECTOR v;
    VECTOR out;

    v.vx = *(s16*)(pActor + 0xA0) - *(s16*)(pActor + 0x2);
    v.vy = *(s16*)(pActor + 0xA2) - *(s16*)(pActor + 0x6);
    v.vz = *(s16*)(pActor + 0xA4) - *(s16*)(pActor + 0xA);
    Square0(&v, &out);
    return SquareRoot0(out.vx + out.vz + out.vy);
}
void func_800B5854(u8* pTask) {
    u8* pActor;
    s32 prev;
    s32 dist;
    s32 speed;
    u8 flag;

    flag = 0;
    pActor = *(u8**)(pTask + 0x1C);
    prev = *(s32*)(pTask + 0x28);
    dist = func_800B57E4(pActor);
    *(s32*)(pTask + 0x28) = dist;
    if (prev < dist || dist < *(s32*)(pTask + 0x2C)) {
        flag = 1;
        speed = *(s32*)(pTask + 0x30);
        *(u16*)(pActor + 0x9E) = 1;
        *(s32*)(pActor + 0x64) = speed;
    }
    if (*(s16*)(pActor + 0x9E) == 0) {
        flag = 1;
    }
    if (*(s8*)(pActor + 0xAF) != *(s32*)(pTask + 0x24)) {
        flag = 1;
    }
    if (flag) {
        ((void (*)(u8*))*(void**)(pTask + 0xC))(pTask);
    }
}
// 100% under gcc 2.7.2-cdk, 89.34% under the unit's current gcc 2.7.2 (which
// emits the callback's lui/addiu pair before move a0,s0 instead of after it).

/*
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
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5924);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B59BC);
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5B3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5C18);
// 98.15% under the unit preset; the instruction set matches, the list
// scheduler places one insn differently (needs gcc 2.7.2-cdk, like the rest of
// the callback cluster here).
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
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5CC0);
// PRESET: the whole unit compiles to the original under gcc 2.7.2-cdk, not the
// gcc 2.7.2 that XenoBattleTail selects. Verified with try_flags.sh: this stub,
// func_800B5FBC, func_800B61B0, func_800B56E4 and func_800B5924 are 100% under
// -cdk and 65-95% under -psx, while the already-matched func_800B57E4 and
// func_800B5AC4 stay 100% under both. The bodies are 100% under -cdk but do not
// match under the current preset, so they stay parked until gears.toml is
// changed.

/*
void func_800B5DC4(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    TimerWorkListSetTaskCallback(*(void**)(pWork + 0x6C), func_800B5CC0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5DC4);
/*
void func_800B5DC4(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    TimerWorkListSetTaskCallback(*(void**)(pWork + 0x6C), func_800B5CC0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5DF4);
// 100% under gcc 2.7.2-cdk, 71.11% under the unit's current gcc 2.7.2.

/*
void func_800B5FBC(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(pWork + 0x6C) + 0x1C, func_800B5DF4);
    pWork[0x2B] = 0x40;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5FBC);
/*
void func_800B5FBC(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(pWork + 0x6C) + 0x1C, func_800B5DF4);
    pWork[0x2B] = 0x40;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B6004);
// 100% under gcc 2.7.2-cdk, same shape as func_800B5FBC.

/*
void func_800B61B0(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(pWork + 0x6C) + 0x1C, func_800B6004);
    pWork[0x2B] = 0x40;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B61B0);
/*
void func_800B61B0(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(pWork + 0x6C) + 0x1C, func_800B6004);
    pWork[0x2B] = 0x40;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B61F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B626C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B62C8);
void func_800B639C(u8* arg0, u8* pScript) {
    BattleScriptOp* pOp;

    pOp = (BattleScriptOp*)(pScript + ((*(s8*)(pScript + 1) << 8) | pScript[0]));
    func_800B3CD4(pOp->unk5, pOp->unk3, pOp->unk4, (s8)pOp->unk0, pOp->unk1, pOp->unk2);
}
