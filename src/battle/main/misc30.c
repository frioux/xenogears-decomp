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
extern u8 D_800C3EB0[];

extern void AnimScriptTick(SpriteData* pSprite);
extern void func_80022CDC(SpriteData* pSprite);
extern void func_800245D8(SpriteData* pSprite, s8 arg1);
extern void func_80021FE0(SpriteData* pSprite, s16 arg1);
extern s32 func_800BEF24(SpriteData* p0, SpriteData* p1);
extern void func_800223B0(SpriteData* pSprite, s16 arg1);
extern void func_800BF0B4(s32 arg0);
extern void func_800B14CC(s32 arg0);
extern void func_800B9B54(SpriteData* p0, SpriteData* p1);
extern void func_8001CE74(void* pTask);
extern void func_8001D3F4(void* pPayload);
extern void WorkListRemoveTask(void* pTask);
extern void TimerWorkListRemoveTask(void* pTask);
extern s32 func_800A5914(s16* pVec, s32 arg1, s32 arg2);
extern s32 func_800A579C(s16* pVec);
extern void func_800A5870(s16* pVec, s32 arg1, u8* pOut);

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
void func_800B9B54(SpriteData* p0, SpriteData* p1) {
    if (p0 == p1) {
        return;
    }
    func_800223B0(p0, func_800BEF24(p0, p1));
    func_80021FE0(p0, func_800BEF24(p0, p1));
    if (((s8*)p1)[0xAF] != 0x15) {
        func_800223B0(p1, func_800BEF24(p1, p0));
        func_80021FE0(p1, func_800BEF24(p1, p0));
    }
}
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
void func_800BA59C(SpriteData* pSprite, s16 angle) {
    s32 scale = pSprite->moveSpeed >> 3;

    pSprite->direction = angle;
    pSprite->step.x = ((rsin(angle) >> 1) * scale) >> 8;
    pSprite->step.z = -(((rcos(pSprite->direction) >> 1) * scale)) >> 8;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA614);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA768);
void func_800BA8F4(u8* pSprite) {
    s16 vec[3];
    u8 out[0x10];
    s32 coord;
    s32 index;

    coord = *(s16*)(pSprite + 0x2);
    vec[0] = coord;
    coord = *(s16*)(pSprite + 0x6);
    vec[1] = coord;
    coord = *(s16*)(pSprite + 0xA);
    vec[2] = coord;
    index = func_800A5914(vec, *(s32*)(pSprite + 0x78), 4);
    if (index < 0) {
        index = func_800A579C(vec);
    }
    func_800A5870(vec, index, out);
    *(u16*)(pSprite + 0x84) = vec[1];
    *(s32*)(pSprite + 0x78) = index;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA984);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BAB0C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BABDC);
/* 92.48% (tried: pTask + 0x58 for the block read so pPayload is only used by
   the call; gcc still defers "addiu s1,s0,0x38" into the branch delay slot
   instead of the prologue, and both shapes score the same).
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
/* 75.68%: the known 0x8C8C wall (same as misc28's func_800B64D4).  Retail keeps
   &D_800C3EB0 in $a1 and adds 0x8C8C with a separate li+addu; gcc always folds
   the constant into the symbolic address as lui at,0x1 / lw -29556(at).
   Tried: raw (u8*)D_800C3EB0, base pointer local, base + separate offset local,
   gcc 2.6.0, 2.7.2-cdk (cc1 rejects), -O3.  The second half (index*0x1C + 0xA
   access, the lb 0xAF compare and both calls) matches exactly under the base
   pointer form, so only this one address is wrong.
void func_800BAEB8(s32 index) {
    SpriteData* pSprite;
    s32 sfx;

    pSprite = *(SpriteData**)((u8*)D_800C3EB0 + index * 4 + 0x8C8C);
    if (((s8*)pSprite)[0xAF] == 0x15) {
        return;
    }
    sfx = (D_800C3EB0[index * 0x1C + 0xA] != 0) << 11;
    func_800223B0(pSprite, sfx);
    func_80021FE0(pSprite, sfx);
}
*/
void func_800BAF40(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BAF48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BB080);
/* 76.91%: same 0x8C8C fold as func_800BAEB8.  Retail keeps the raw
   &D_800C3EB0 in $s0 and hoists 0x8C8C into $s4 (`li s4,0x8C8C; addu s2,s0,s4`),
   then adds 0x8CB8 with a second li+addu after the call; gcc folds 0x8CB8 into
   the symbol base (lui s0,0x1; addiu s0,-29512) and uses -0x2C for the other
   access, which also drags the loop-bound `3` into a register (li s3,3).
   The call through pSprite->field_0x6C+0xC, the three break tests and the
   func_800B14CC tail are otherwise identical.
void func_800BB080(s32 arg0) {
    u8* pEntry = D_800C3EB0;
    SpriteData* pSprite;
    void* pObj;
    s32 i;

    for (i = 0; i != 3; i++) {
        if (i == arg0) {
            break;
        }
        pSprite = *(SpriteData**)(pEntry + 0x8C8C);
        if (pSprite == NULL) {
            break;
        }
        if (pSprite->pDefaultAnimFile != NULL) {
            break;
        }
        pObj = *(void**)((u8*)pSprite + 0x6C);
        (*(void (**)(void*))((u8*)pObj + 0xC))(pObj);
        *(u32*)(pEntry + 0x8C8C) = 0;
        *(u32*)(pEntry + 0x8CB8) = 0;
        pEntry += 4;
    }
    func_800B14CC(arg0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BB13C);
