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
extern void func_80021B14(IVEC3 *pVector, s32 x, s32 y, s32 z);
extern s32 func_800AA600(s32 arg0);
extern s32 D_80050100;
extern MATRIX D_800D30BC;
extern void TimerWorkListSetTaskCallback(void *pEntry, void *pCallback);

/* best 94.51%: everything but three details. (a) retail re-reads field 0x36
   from memory (lhu) before shifting it into 0x38 while gcc folds the just-stored
   value (`andi v0,v0,0xffff`), even when the store goes through a u16* local and
   the read through another one. (b) retail loads the three SVECTOR halves with
   `lh` (SImode temp) into one register; ours needs a per-field s32 temp to avoid
   a narrowing HImode load and lands them in $v1. (c) the %hi/%lo of D_800D30BC
   lands two instructions later than retail's, which is reorg filling the
   SetRotMatrix jal slot. The mode expression needs its high bits pulled into a
   local first (`mode = ... >> 30`) to get retail's `or a0,a0,v0` operand order. */
/* void func_800BB248(u8 *pTask) {
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
    *pField = func_800AA600(((*(u32 *)(pSpriteData + 0xAC) & 3) << 2) | mode);
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
} */
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB690);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB6E0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB760);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB7F8);
/* best 94.60%: the layout, the four VECTOR slots (0x10/0x20/0x30/0x40) and
   every call are exact; the only difference is the matrix copy at the end.
   Retail reads the three rows as halfwords at stride 4 (lhu 48/52/56(sp)) with
   the assembler's load-delay nop after each, while gcc 2.7.2 merges each pair of
   loads across the row boundary and drops the nops, so our object is 16 bytes
   short. Reading through a u16 pointer or a u16* local changes nothing;
   `(u16)` casts on the field form give lw + sh instead of lhu. */
/* void func_800BB844(MATRIX *pMatrix, SVECTOR *pA, SVECTOR *pB, SVECTOR *pC) {
    VECTOR vTmp;
    VECTOR vB;
    VECTOR vC;
    VECTOR vD;

    func_80021B14((IVEC3 *)&vTmp, pB->vx - pA->vx, pB->vy - pA->vy, pB->vz - pA->vz);
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
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB844);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB9D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BBAB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BBEE0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC018);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BCB54);
/* best 86.08%: retail keeps the address of D_800C3748 in $s1 across the calls
   (lui s1,%hi then two %lo(...)($s1) uses); gcc rematerialises it per use
   because the load and the store sit in different basic blocks, so we get
   lui s0 / lw s0 plus a lui $at store, and the frame is 24 instead of 32.
   Plain `pEntry = D_800C3748`, a u8 **pSlot = &D_800C3748 local, and moving the
   store out of the if all fold back to the same RTL. */
/* void func_800BCB54(void) {
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
} */
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
