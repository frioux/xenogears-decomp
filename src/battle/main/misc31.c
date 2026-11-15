#include "common.h"
#include "field/actor.h"
#include "psyq/libgpu.h"

extern s32 D_800C367C;
extern s16 D_800C3740;
extern u8 D_800C37C8;

extern void func_800BC2F0(s32);
extern void HeapFree(void *pMem);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void func_8001F6B0(u8 *pEntry);
extern s32 rcos(s32 a);
extern s32 rsin(s32 a);
extern void SpriteSetIVec3(IVEC3 *pVector, s32 x, s32 y, s32 z);
extern s32 BattleGetActorHeight(s32 arg0);
extern s32 D_80050100;
extern MATRIX D_800D30BC;
extern void TimerWorkListSetTaskCallback(void *pEntry, void *pCallback);
extern void func_800A9540(s32 arg0);
extern void func_800BB620(u8 *pEntry);
extern void func_800BB6E0(u8 *pEntry);
extern u8 *TimerWorkListAllocateTask(void *pData, s32 dataSize);
extern u8 D_800591AC;
extern u8 D_800591AF;
extern s32 D_800C35D8;
extern u8 D_800C3CB8;
extern u32 D_800C3674;
extern s32 D_800C3678;
extern u8 D_800C3CC4;
extern s32 D_800C3CBC;
extern u8 *D_800C3748;
extern u8 *D_800C3680;
extern u8 *D_800C3684;
extern u16 D_800C3CCC[];
extern u16 D_800C3CD4[];
extern u16 D_800D30A0[];
extern u16 D_800D30A8[];
extern void WorkListsRemoveEntriesForData(u8 *pTask);
extern void AnimScriptTick(SpriteData *pSprite);
extern void SpriteApplyMovementStep(SpriteData *pSprite);
extern IVEC3 D_8006F99C;
extern IVEC3 D_8006F9AC;

// best 95.69% under gcc-2.6.0 (94.51% under 2.7.2): only the lui %hi(D_800D30BC)
// lands late and turns the three "lh" loads into v1 instead of v0. 2.6.0 fixes the
// lhu re-read of field 0x36 that 2.7.2 forwards from the store.
/*
void func_800BB248(u8 *pTask) {
    u8 *pSpriteData;
    SVECTOR vec;
    long sxy;
    long flag;
    s32 nOTOffset;
    s32 mode;
    u16 *pField;
    u16 *pRead;
    s32 v;

    pSpriteData = *(u8 **)(pTask + 4);
    mode = *(u32 *)(pSpriteData + 0xA8) >> 30;
    pField = (u16 *)(pSpriteData + 0x36);
    *pField = BattleGetActorHeight(((*(u32 *)(pSpriteData + 0xAC) & 3) << 2) | mode);
    pRead = (u16 *)(pSpriteData + 0x36);
    *(u16 *)(pSpriteData + 0x38) = *pRead >> 1;
    v = *(s16 *)(pSpriteData + 0x2);
    vec.vx = v;
    v = *(s16 *)(pSpriteData + 0x6);
    vec.vy = v;
    v = *(s16 *)(pSpriteData + 0xA);
    vec.vz = v;
    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    nOTOffset = (RotTransPers(&vec, &sxy, (u_long *)&sxy, &flag) >> D_80050100) +
                *(s16 *)(pSpriteData + 0x30);
    if (flag & 0x8000) {
        nOTOffset = 0;
    }
    *(u16 *)(pSpriteData + 0x2E) = nOTOffset;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB248);

void func_800BB314(u8 *pEntry) {
    u32 unused[2];
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB350);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB540);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB620);
void func_800BB690(u8 *pEntry) {
    func_800A9540(*(s32 *)(pEntry + 0x1C));
    D_800C3CB8++;
    TimerWorkListSetTaskCallback(pEntry, func_800BB620);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB6E0);
// best 88.71% under cdk -G0 (44.87% under 2.7.2 -G8): the frame is 40 and every
// instruction is present, only the global-base registers differ (we keep
// &D_800591AC in s0 with the saved byte in s3, retail uses s1/s4), plus one
// load-delay nop after lw D_800C35D8 that retail fills with the pEntry store.
/*
void func_800BB760(u8 *pEntry) {
    u8 saved;
    u8 *pTask;

    saved = D_800591AC;
    D_800591AC = 0;
    D_800591AF = 1;
    pTask = TimerWorkListAllocateTask(NULL, 4);
    TimerWorkListSetTaskCallback(pTask, func_800BB6E0);
    D_800C35D8++;
    *(u8 **)(pTask + 0x1C) = pEntry;
    D_800591AF = 0;
    D_800591AC = saved;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB760);
void func_800BB7F8(void) {
    D_800C3674 = 0x200;
    D_800C3678 = -1;
    D_800C3CC4 = 0;
    D_800C3CBC = 1;
    func_800BC2F0(0);
}
void func_800BB844(MATRIX *pMatrix, SVECTOR *pA, SVECTOR *pB, SVECTOR *pC) {
    VECTOR vTmp;
    VECTOR vB;
    VECTOR vC;
    VECTOR vD;

    SpriteSetIVec3((IVEC3 *)&vTmp, pB->vx - pA->vx, pB->vy - pA->vy, pB->vz - pA->vz);
    vD.vx = pC->vx;
    vD.vy = pC->vy;
    vD.vz = pC->vz;
    VectorNormal(&vTmp, &vB);
    OuterProduct12(&vD, &vB, &vTmp);
    VectorNormal(&vTmp, &vC);
    OuterProduct12(&vB, &vC, &vTmp);
    VectorNormal(&vTmp, &vD);
    pMatrix->m[0][0] = vC.vx;
    pMatrix->m[0][1] = vC.vy;
    pMatrix->m[0][2] = vC.vz;
    pMatrix->m[1][0] = vD.vx;
    pMatrix->m[1][1] = vD.vy;
    pMatrix->m[1][2] = vD.vz;
    pMatrix->m[2][0] = vB.vx;
    pMatrix->m[2][1] = vB.vy;
    pMatrix->m[2][2] = vB.vz;
    PushMatrix();
    ApplyMatrix(pMatrix, pA, &vTmp);
    pMatrix->t[0] = -vTmp.vx;
    pMatrix->t[1] = -vTmp.vy;
    pMatrix->t[2] = -vTmp.vz;
    PopMatrix();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB9D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BBAB8);
void func_800BBEE0(u8 *pEntry) {
    SpriteData *pSprite;

    pSprite = *(SpriteData **)(pEntry + 4);
    if ((pSprite->field_0x40 >> 13 & 0xF) == 0xA) {
        if (D_800C3680 == pEntry) {
            D_800C3680 = NULL;
            if (D_800C37C8 == 0) {
                D_800D30A0[0] = D_800C3CCC[0];
                D_800D30A0[1] = D_800C3CCC[1];
                D_800D30A0[2] = D_800C3CCC[2];
            }
        }
    } else if (D_800C3684 == pEntry) {
        D_800C3684 = NULL;
        if (D_800C37C8 == 0) {
            D_800D30A8[0] = D_800C3CD4[0];
            D_800D30A8[1] = D_800C3CD4[1];
            D_800D30A8[2] = D_800C3CD4[2];
        }
    }
    if (((u32)pSprite->flags_0xAC >> 5) & 1) {
        WorkListsRemoveEntriesForData(pEntry);
    }
    TimerWorkListRemoveTask(pEntry);
    WorkListRemoveTask(pEntry + 0x1C);
    HeapFree(pEntry);
    D_800C3CC4--;
    if (D_800C3CC4 == 0) {
        func_800BC2F0(D_800C367C);
    }
}
void func_800BC018(u8 *pEntry) {
    SpriteData *pSprite;

    pSprite = *(SpriteData **)(pEntry + 4);
    AnimScriptTick(pSprite);
    SpriteApplyMovementStep(pSprite);
    if ((pSprite->field_0x40 >> 13 & 0xF) == 0xA) {
        D_8006F99C.x = pSprite->position.x;
        D_8006F99C.y = pSprite->position.y;
        D_8006F99C.z = pSprite->position.z;
    } else {
        D_8006F9AC.x = pSprite->position.x;
        D_8006F9AC.y = pSprite->position.y;
        D_8006F9AC.z = pSprite->position.z;
    }
    if (pSprite->pSpriteBytecode == NULL) {
        (*((void (**)(u8 *))(pEntry + 0xC)))(pEntry);
        return;
    }
    if ((((u32)pSprite->flags_0xAC >> 6) & 1) == 0) {
        return;
    }
    AnimScriptTick(pSprite);
    SpriteApplyMovementStep(pSprite);
    if ((pSprite->field_0x40 >> 13 & 0xF) == 0xA) {
        D_8006F99C.x = pSprite->position.x;
        D_8006F99C.y = pSprite->position.y;
        D_8006F99C.z = pSprite->position.z;
    } else {
        D_8006F9AC.x = pSprite->position.x;
        D_8006F9AC.y = pSprite->position.y;
        D_8006F9AC.z = pSprite->position.z;
    }
    if (pSprite->pSpriteBytecode == NULL) {
        (*((void (**)(u8 *))(pEntry + 0xC)))(pEntry);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC158);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC2F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC3F8);
/* best 45%: unmatchable macro-store, see func_800BC454 below. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC404);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC454);
/* best 45%, and 45.0 under every gcc/-O/-G in try_flags --sweep: gcc emits the
   one-insn macro store `sw $a0,D_800C3740` and maspsx expands it with $at
   (`lui at,%hi` + `sw ...,0(at)`), while retail ASPSX picks $v0 and reorg then
   fills the jr delay slot with the store. Same $at wall as misc32/misc28. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC460);

void func_800BCAA4(void) {
    if (D_800C37C8 == 0) {
        func_800BC2F0(4);
    }
}

void func_800BCAD0(void) {
    if (D_800C37C8 == 0) {
        func_800BC2F0(1);
    }
}

void func_800BCAFC(u8 *pEntry, s32 angle) {
    s32 color;

    color = rcos(angle << 6);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pEntry[0x28] = color;
    pEntry[0x29] = color;
    pEntry[0x2A] = color;
    func_8001F6B0(pEntry);
}
void func_800BCB54(void) {
    u8 *pEntry;

    pEntry = D_800C3748;
    if (pEntry != NULL) {
        u8 *pFlags;

        pFlags = *(u8 **)(pEntry + 0x1C);
        pFlags[0x2B] |= 1;
        TimerWorkListRemoveTask(pEntry);
        HeapFree(pEntry);
        D_800C3748 = NULL;
    }
}

void func_800BCBB4(u8 *pEntry) {
    s32 color;
    s32 angle;
    u8 *pData;
    /* the incoming $a0 only survives the angle load; the rest reads a saved copy */
    u8 *pSaved = pEntry;

    angle = *(s32 *)(pEntry + 0x20) << 6;
    pData = *(u8 **)(pSaved + 0x1C);
    color = rcos(angle);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pData[0x28] = color;
    color = rsin(angle);
    color += 0x1000;
    color >>= 6;
    color += 0x80;
    if (color >= 0x100) {
        color = 0xFF;
    }
    pData[0x29] = color;
    pData[0x2A] = color;
    func_8001F6B0(pData);
    *(s32 *)(pSaved + 0x20) += 1;
}
