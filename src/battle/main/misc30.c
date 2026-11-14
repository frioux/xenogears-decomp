#include "common.h"
#include "field/actor.h"
#include "psyq/libgte.h"

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
extern u8* D_800D3368[];
extern s32 D_80050100;
extern MATRIX D_800D30BC;
extern u_long* g_GfxCurOT;

extern void AnimScriptTick(SpriteData* pSprite);
extern void func_80022CDC(SpriteData* pSprite);
extern void func_800245D8(SpriteData* pSprite, s8 arg1);
extern void func_80021FE0(SpriteData* pSprite, s16 arg1);
extern void func_80022B2C(SpriteData* pSprite);
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
extern void func_8001E298(SpriteData* pSprite, u_long* pOT);
extern void* func_800BFC80(void* pArg0, s32 arg1, s32 arg2);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B905C);

void func_800B9258(void) {
    BattleState* pState = D_800C3610;

    if (pState != NULL) {
        ++pState->unk34;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9284);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9508);

void func_800B9B30(void) {
    D_800C3610->unk48 = 1;
    D_800C3610->unk49 = ((u8*)D_800C3610)[0x1C];
}
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
void func_800B9C00(SpriteData* pSprite, s32 arg1) {
    pSprite->position.x = pSprite->field_0xA0 << 16;
    pSprite->position.z = pSprite->field_0xA4 << 16;
    D_800C3610->unk48 = 1;
    func_800BF0B4(4);
    func_800245D8(pSprite, ((s8*)pSprite)[0xB0]);
    func_800B9B54(pSprite, arg1);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9C78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800B9F78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BA4E0);
void func_800BA59C(SpriteData* pSprite, s16 angle) {
    s32 scale = pSprite->moveSpeed >> 3;

    pSprite->direction = angle;
    pSprite->step.x = ((rsin(angle) >> 1) * scale) >> 8;
    pSprite->step.z = -(((rcos(pSprite->direction) >> 1) * scale)) >> 8;
}
void func_800BA614(SpriteData* pSprite) {
    VECTOR diff;
    SVECTOR vec;
    u8 out[0x10];
    s32 index;
    s32 len;
    s32 dy;
    s32 angle;

    func_80021B04(&vec, pSprite->field_0xA0, pSprite->field_0xA2, pSprite->field_0xA4);
    index = func_800A5914((s16*)&vec, pSprite->field_0x78, 4);
    if (index < 0) {
        index = func_800A579C((s16*)&vec);
    }
    func_800A5870((s16*)&vec, index, out);
    if (vec.vy > pSprite->field_0xA2) {
        *(u16*)&vec.vy = *(u16*)&pSprite->field_0xA2;
    }
    dy = ((vec.vy << 16) - *(s32*)((u8*)pSprite + 0x4)) >> 16;
    diff.vx = pSprite->field_0xA0 - *(s16*)((u8*)pSprite + 0x2);
    diff.vz = pSprite->field_0xA4 - *(s16*)((u8*)pSprite + 0xA);
    angle = -ratan2(diff.vz, diff.vx);
    Square0(&diff, &diff);
    len = SquareRoot0(diff.vx + diff.vz);
    pSprite->step.y = ((-(s32)pSprite->gravity * len) << 4) / (pSprite->moveSpeed >> 11) +
                      (pSprite->moveSpeed * dy) / len;
    func_800BA59C(pSprite, angle);
}
void func_800BA768(SpriteData* pSprite) {
    VECTOR diff;
    SVECTOR vec;
    u8 out[0x10];
    s32 index;
    s32 divisor;
    s32 angle;
    s32 len;
    s32 num;

    divisor = -((pSprite->step.y << 1) / (s32)pSprite->gravity);
    pSprite->position.x &= 0xFFFF0000;
    pSprite->position.y &= 0xFFFF0000;
    pSprite->position.z &= 0xFFFF0000;
    diff.vx = pSprite->field_0xA0 - *(s16*)((u8*)pSprite + 0x2);
    diff.vz = pSprite->field_0xA4 - *(s16*)((u8*)pSprite + 0xA);
    diff.vy = 0;
    angle = -ratan2(diff.vz, diff.vx);
    Square0(&diff, &diff);
    len = SquareRoot0(diff.vx + diff.vz);
    if (divisor != 0) {
        pSprite->moveSpeed = (len << 16) / divisor;
    } else {
        pSprite->moveSpeed = 0;
    }
    func_80021B04(&vec, pSprite->field_0xA0, pSprite->field_0xA2, pSprite->field_0xA4);
    index = func_800A5914((s16*)&vec, pSprite->field_0x78, 4);
    if (index < 0) {
        index = func_800A579C((s16*)&vec);
    }
    func_800A5870((s16*)&vec, index, out);
    if (vec.vy > pSprite->field_0xA2) {
        *(u16*)&vec.vy = *(u16*)&pSprite->field_0xA2;
    }
    num = (vec.vy << 16) - *(s32*)((u8*)pSprite + 0x4);
    if (divisor != 0) {
        pSprite->step.y += num / divisor;
    }
    func_800BA59C(pSprite, angle);
    func_80022B2C(pSprite);
}
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
// RotTransPers output block; the tail is unused but reproduces retail's 0x40 frame.
typedef struct {
    s32 sxy;
    s32 flag;
    s32 pad[4];
} RotTransPersOut;

void func_800BAB0C(u8* pTask) {
    SpriteData* pSprite;
    SVECTOR vec;
    s32 coord;
    s32 otz;
    RotTransPersOut out;

    if (D_800C3664 != 0) {
        return;
    }
    pSprite = *(SpriteData**)(pTask + 0x4);
    coord = *(s16*)((u8*)pSprite + 0x2);
    vec.vx = coord;
    coord = *(s16*)((u8*)pSprite + 0x6);
    vec.vy = coord;
    coord = *(s16*)((u8*)pSprite + 0xA);
    vec.vz = coord;
    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    otz = (RotTransPers(&vec, &out.sxy, (u_long*)&out.sxy, &out.flag) >> D_80050100) +
          *(s16*)((u8*)pSprite + 0x30);
    if (out.flag & 0x8000) {
        otz = 0;
    }
    *(s16*)((u8*)pSprite + 0x2E) = otz;
    if ((u32)(otz - 1) < 0xFFF) {
        func_8001E298(pSprite, g_GfxCurOT + otz);
    }
}

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
/* 99.29%: the only difference is that retail's sprite pointer takes $v1 and its
   SImode load temp $v0, gcc's the other way (identical size, 280 bytes, every
   other instruction exact, frame and 0x8C8C li+addu head included).  Writing
   `vec.vx = *(s16*)(...) + 0` flips the registers into retail's order but turns
   the loads into lhu; the `s32 coord` temp that gives lh also gives the swap.
   Tried: coord declared before/after pSprite, pBase via the global, s16[3] vec.
/*
void func_800BACBC(s32 index, s16* p0, s16* p1, s16* p2, s16* p3, s16* p4, s16* p5) {
    u8* pBase = D_800C3EB0;
    s32 coord;
    SpriteData* pSprite;
    SVECTOR vec;
    u16 sxy[2];
    s32 flag;

    pSprite = *(SpriteData**)(pBase + index * 4 + 0x8C8C);
    coord = *(s16*)((u8*)pSprite + 0x2);
    vec.vx = coord;
    coord = *(s16*)((u8*)pSprite + 0x6);
    vec.vy = coord;
    coord = *(s16*)((u8*)pSprite + 0xA);
    vec.vz = coord;
    PushMatrix();
    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    *p2 = RotTransPers(&vec, (long*)sxy, &flag, &flag) >> 4;
    *p0 = sxy[0];
    *p1 = sxy[1];
    *p3 = sxy[0] - 0x30;
    *p4 = 0x30;
    *p5 = sxy[0] - 0x18;
    PopMatrix();
}
*/
void func_800BADD4(s32 index) {
    u8* pBase = D_800C3EB0;
    u8* pBlock;
    u8* pTail;
    SpriteData* pSprite;
    void* pPayload;
    u32* pSlot;
    u32* pSlot2;

    pSlot = (u32*)(pBase + index * 4 + 0x8CB8);
    pSprite = *(SpriteData**)pSlot;
    if (pSprite == NULL) {
        return;
    }
    func_800BFC80(pSprite, 0, 2);
    if (index < 3) {
        pBlock = pBase + index * 0xC + 0x8000;
        pPayload = *(void**)(pBlock + 0xD24);
        if (pPayload != NULL) {
            HeapFree(pPayload);
        }
        *(u32*)(pBlock + 0xD24) = 0;
    }
    (*(void (**)(SpriteData*))((u8*)pSprite + 0xC))(pSprite);
    func_8001CE74(pSprite);
    pTail = D_800C3EB0;
    pSlot = (u32*)(pTail + index * 4 + 0x8C8C);
    *pSlot = 0;
    pSlot2 = (u32*)(pTail + index * 4 + 0x8CB8);
    *pSlot2 = 0;
}
void func_800BAEB8(s32 index) {
    u8* pBase = D_800C3EB0;
    u8* pEntry = pBase + index * 4;
    u8* pCounter;
    SpriteData** ppSlot;
    SpriteData* pSprite;
    s32 sfx;

    ppSlot = (SpriteData**)(pEntry + 0x8C8C);
    pSprite = *ppSlot;
    if (((s8*)pSprite)[0xAF] == 0x15) {
        return;
    }
    pCounter = pBase + index * 0x1C;
    sfx = (pCounter[0xA] != 0) << 11;
    func_800223B0(pSprite, sfx);
    func_80021FE0(pSprite, sfx);
}
void func_800BAF40(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BAF48);
void func_800BB080(s32 arg0) {
    u8* pEntry = D_800C3EB0;
    u8* pSlot;
    SpriteData* pSprite;
    void* pObj;
    s32 i;

    for (i = 0; i != 3; i++) {
        if (i == arg0) {
            break;
        }
        pSlot = pEntry + 0x8C8C;
        pSprite = *(SpriteData**)pSlot;
        if (pSprite == NULL) {
            break;
        }
        if (pSprite->pDefaultAnimFile != NULL) {
            break;
        }
        pObj = pSprite->field_0x6C;
        (*(void (**)(void*))((u8*)pObj + 0xC))(pObj);
        *(u32*)pSlot = 0;
        pSlot = pEntry + 0x8CB8;
        *(u32*)pSlot = 0;
        pEntry += 4;
    }
    func_800B14CC(arg0);
}
void func_800BB13C(u8* pTask) {
    SpriteData* pSprite;
    u8* pScript;
    s32 mode;
    s32 index;
    u32 unused[2]; // unused: reproduces retail's 0x28 frame

    pSprite = *(SpriteData**)(pTask + 0x4);
    mode = *(u32*)((u8*)pSprite + 0xA8) >> 30;
    index = ((*(u32*)((u8*)pSprite + 0xAC) & 3) << 2) | mode;
    pScript = D_800D3368[index];
    if (pScript == NULL) {
        return;
    }
    if (pSprite->animScriptWaitTimer == 0) {
        pSprite->pSpriteBytecode = NULL;
    }
    if (pSprite->pSpriteBytecode != NULL) {
        AnimScriptTick(pSprite);
        func_80022CDC(pSprite);
        if (((u32)pSprite->flags_0xAC >> 6) & 1) {
            AnimScriptTick(pSprite);
            func_80022CDC(pSprite);
        }
    }
    pSprite->position.x = *(u32*)(*(u32*)(pScript + 0x4) + 0x5C) << 16;
    pSprite->position.y = *(u32*)(*(u32*)(pScript + 0x4) + 0x60) << 16;
    pSprite->position.z = *(u32*)(*(u32*)(pScript + 0x4) + 0x64) << 16;
}

