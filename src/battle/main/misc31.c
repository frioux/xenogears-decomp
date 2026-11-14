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

/* best 94.51%: everything matches except two things, and both come from the
   same place - retail splits D_800D30BC's address across five instructions
   (lui $s0 right after the func_800AA600 call, addiu $s0 five insns later);
   gcc 2.7.2 emits "la" as ONE macro that maspsx expands adjacently, so the
   pair can never be split (gcc-2.7.2-cdk, which does split it, scores 93.33
   overall here). Same for the register the three "lh" loads land in. The
   other gap: retail re-reads field 0x36 with lhu after storing it while gcc
   forwards the stored value as "andi v0,v0,0xffff". The mode expression must
   be its own local assigned before the call, or the two loads and the
   "or a0,a0,v0" operand order come out backwards. */
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
/* best 71.25%: emits retail's exact instruction sequence up to the increment,
   but the byte store `D_800C3CB8 = count` becomes gcc's macro store
   `sb $2,D_800C3CB8` (no base register) and the assembler expands it with
   `lui $at` instead of retail's reused `$v1` - the atfree wall. A u8 *pCount
   local for the increment folds back to the same RTL. */
/* void func_800BB690(u8 *pEntry) {
    func_800A9540(*(s32 *)(pEntry + 0x1C));
    D_800C3CB8++;
    TimerWorkListSetTaskCallback(pEntry, func_800BB620);
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB690);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB6E0);
/* best 44.87%: retail keeps &D_800591AC in $s1 and &D_800591AF in $s2
   across the calls and stores through them; gcc emits every store to these
   globals as a macro store with no base register, so the assembler picks
   $at (the atfree wall this whole unit shares) and the frame is 32 not 40.
   The two-use shape is right: retail loads D_800591AC into $s4, zeroes it,
   sets D_800591AF=1, allocates 4 bytes, sets the callback to func_800BB6E0,
   bumps D_800C35D8, stores pEntry at +0x1C, then restores both flags. */
/* void func_800BB760(u8 *pEntry) {
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
/* best 41.79%: every global here is touched exactly once, so retail's $v1/$v0
   bases are unreachable - gcc emits macro stores with no base register and the
   assembler materialises each address in $at (atfree). */
/* void func_800BB7F8(void) {
    D_800C3674 = 0x200;
    D_800C3678 = -1;
    D_800C3CC4 = 0;
    D_800C3CBC = 1;
    func_800BC2F0(0);
} */
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
