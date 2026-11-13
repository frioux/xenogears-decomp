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
extern s32 D_80050100;
extern MATRIX D_800D30BC;
extern u_long* g_GfxCurOT;

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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc30", func_800BADD4);
/* 92.04%: the 0x8C8C constant-fold wall, second shape.  With pBase a local the
   tail's instruction stream is retail's exactly (`li v0,0x8C8C; addu; sw 0` and
   the same for 0x8CB8), but gcc then keeps pBase live in $s3 across the calls
   and saves it, where retail re-forms lui/addiu for D_800C3EB0 and reuses $s1
   for the if-block pointer.  Spelling the tail address with the global
   (`D_800C3EB0 + index*4 + 0x8C8C`) instead makes gcc fold the constant into
   the symbol (lui 0x1; addiu -29556; sw 0x2C) and reach 94.02%, and routing it
   through a cast `(mem_addr)` pointer gets the frame right but splits the add
   into li 0x8000 + displacement (92.75%).  Everything else, frame and the
   0x8000/D24 if-block included, is exact.
/*
void func_800BADD4(s32 index) {
    u8* pBase = D_800C3EB0;
    SpriteData* pSprite;
    u8* pSlot;
    void* pPayload;

    pSprite = *(SpriteData**)(pBase + index * 4 + 0x8CB8);
    if (pSprite == NULL) {
        return;
    }
    func_800BFC80(pSprite, 0, 2);
    if (index < 3) {
        pSlot = pBase + index * 0xC + 0x8000;
        pPayload = *(void**)(pSlot + 0xD24);
        if (pPayload != NULL) {
            HeapFree(pPayload);
        }
        *(u32*)(pSlot + 0xD24) = 0;
    }
    (*(void (**)(SpriteData*))((u8*)pSprite + 0xC))(pSprite);
    func_8001CE74(pSprite);
    *(u32*)(pBase + index * 4 + 0x8C8C) = 0;
    *(u32*)(pBase + index * 4 + 0x8CB8) = 0;
}
*/
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
