#include "common.h"
#include "field/actor.h"

// Battle state pointed to by D_800C3610. Only the fields this unit touches are recovered.
typedef struct {
    /* 0x00 */ u8 pad00[0x1C];
    /* 0x1C */ u32 unk1C;
    /* 0x20 */ u8 pad20[0x14];
    /* 0x34 */ u32 unk34;
    /* 0x38 */ u8 pad38[0x10];
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u8 unk49;
} BattleState;

extern BattleState* D_800C3610;
extern u8 D_800C3664;

extern void AnimScriptTick(SpriteData* pSprite);
extern void func_80022CDC(SpriteData* pSprite);
extern void func_800245D8(SpriteData* pSprite, s8 arg1);
extern void func_800BF0B4(s32 arg0);
extern void func_800B9B54(SpriteData* pSprite, s32 arg1);
extern void func_8001CE74(void* pTask);
extern void func_8001D3F4(void* pPayload);
extern void WorkListRemoveTask(void* pTask);
extern void TimerWorkListRemoveTask(void* pTask);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B905C);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9258);
/* 99.09%: address of D_800C3610 lands in $v1 here, $v0 in the original.
void func_800B9258(void) {
    BattleState* pState = D_800C3610;

    if (pState != NULL) {
        ++pState->unk34;
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9284);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9508);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9B30);
/* 80.56%: gcc re-forms the D_800C3610 address (lui/lw) for the second access
   instead of reusing the base register like the original.
void func_800B9B30(void) {
    D_800C3610->unk48 = 1;
    D_800C3610->unk49 = ((u8*)D_800C3610)[0x1C];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9B54);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9C00);
/* 81.73%: gcc hoists the D_800C3610 load above the position stores and folds
   the lui/lw base into one register; the original keeps them apart.
void func_800B9C00(SpriteData* pSprite, s32 arg1) {
    pSprite->position.x = pSprite->field_0xA0 << 16;
    pSprite->position.z = pSprite->field_0xA4 << 16;
    D_800C3610->unk48 = 1;
    func_800BF0B4(4);
    func_800245D8(pSprite, ((s8*)pSprite)[0xB0]);
    func_800B9B54(pSprite, arg1);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9C78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9F78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA4E0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA59C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA614);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA768);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA8F4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA984);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BAB0C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BABDC);
/* 92.48%: gcc rematerialises pTask + 0x38 as a 0x58 offset from pTask instead of
   holding it in $s1 like the original, which shifts the whole register shape.
void func_800BABDC(u8* pTask) {
    u8* pPayload = pTask + 0x38;
    void* pBlock;

    pBlock = *(void**)(pPayload + 0x20);
    pBlock = *(void**)((u8*)pBlock + 0x2C);
    if (pBlock != NULL) {
        HeapFree(pBlock);
    }
    func_8001CE74(pTask);
    func_8001D3F4(pPayload);
    WorkListRemoveTask(pTask + 0x1C);
    TimerWorkListRemoveTask(pTask);
    HeapFree(pTask);
}
*/

void func_800BAC50(u8* pTask) {
    SpriteData* pSprite = *(SpriteData**)(pTask + 0x4);

    if (D_800C3664 == 0) {
        AnimScriptTick(pSprite);
        func_80022CDC(pSprite);
        if (((u32)pSprite->flags_0xAC >> 6) & 1) {
            AnimScriptTick(pSprite);
            func_80022CDC(pSprite);
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BACBC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BADD4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BAEB8);
void func_800BAF40(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BAF48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BB080);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BB13C);
