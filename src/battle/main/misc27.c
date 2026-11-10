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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B4F88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B50D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B51B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5588);
void func_800B56E4(u8* pWork) {
    u8* pSprite;

    pSprite = *(u8**)(pWork + 0x4);
    func_8001E148(*(void**)(pSprite + 0x38));
    func_800C08CC(5, pSprite + 0x78, func_800B51B0);
}

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
