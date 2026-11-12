#include "common.h"
#include "psyq/libgpu.h"

extern void WorkListSetTaskCallback(void *pEntry, void *pCallback);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void HeapFree(void *pMem);
extern void func_80025180(void *pArg);
extern void func_800245D8(s32 arg0, s32 arg1);
extern void func_800B7160(void);
extern u_long *g_GfxCurOT;
extern u_long *D_800C3CB4;
extern u8 D_80025A88[];
extern u8 D_800C3EB0[];
extern long ratan2(long y, long x);
extern void func_8002DDE4(void *pImg, s32 arg1, s32 arg2, s32 arg3);
extern s32 WorkListsAddTasks(s32 arg0, s32 arg1, void *pFn1, void *pFn2, void *pFn3);
extern void func_800B7424(s32 arg0);
extern void func_800B73A0(void);
extern void func_800B6F0C(void);
extern void func_800B7134(void);
extern void func_800B7364(u8 *p);
extern s32 func_80023124(s32 arg0, s32 arg1);
extern s32 func_80022CAC(u8 *pActor, s32 arg1);
extern void func_801FC4C4(s32 arg0, s32 arg1, void *pArg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
extern void func_80021FE0(u8 *pActor, s16 arg1);
extern void func_800223B0(u8 *pActor, s16 arg1);
extern void func_80021B04(SVECTOR *pVector, s16 x, s16 y, s16 z);
extern void func_801FC53C(void *pArg, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);

void func_800B63F0(s32 arg0, u8 *p) {
    s32 pad[3];
    u8 *pData;

    pData = p + ((((s8 *)p)[1] << 8) | p[0]);
    pad[0] = 0;
    pad[1] = 0;
    pad[2] = 0;
    func_8002DDE4(pData, 0, 0, 0);
}
void func_800B6438(u8 *p) {
    WorkListSetTaskCallback((u8 *)*(u32 *)(p + 0x6C) + 0x1C, D_80025A88);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6464);
/* best 59.79%: retail loads both packet bytes before any arithmetic and
   copies the packet pointer to t1 in the first slot; gcc reuses a0 for the
   second lbu and sinks it below the srl.  Also tried: u16 v local, s32 byte
   locals, i less-than n (gives slt instead of the rotated bne), and both
   signed and unsigned packet pointer types.
void func_800B6464(u8 *pActor, u8 *p) {
    u8 *pEntry;
    s16 n;
    s16 i;

    n = *(u8 *)(pActor + 0x40) >> 2;
    pEntry = (u8 *)(*(u32 *)(*(u32 *)(pActor + 0x20) + 0x30)) + 0xC;
    for (i = 0; i != n; i++) {
        u16 v = *(u16 *)pEntry;
        s32 b0 = p[0];
        s32 b1 = p[1];

        *(u16 *)pEntry = ((v & 0x3F) + b0) | ((((v >> 6) & 0x1FF) + b1) << 6);
        pEntry += 0x18;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B64D4);
/* best 95.76% under gcc 2.7.2-cdk, 77.59% under the unit preset.
   retail adds 0x8C8C as a separate li+addu on the runtime base
   (li v1,0x8C8C; addu v0,v0,v1; lw a0,0(v0)), gcc always folds it into the
   symbol address as lui at,0x1 / lw a0,-29556(at).  Tried: local base pointer,
   separate offset statements, u32 integer base, gcc-2.6.0 and 2.7.2-cdk.
void func_800B64D4(s32 arg0, u8 *p) {
    u32 nEntry = (u32)D_800C3EB0 + (p[1] << 2);

    func_800245D8(*(u32 *)(nEntry + 0x8C8C), p[0]);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6518);
/* best 81.08%: retail materialises the two 8-byte locals in memory (sh) and
   rebuilds each word from two lhu, and its frame is 8 bytes larger (one
   unused slot); gcc reads each word back with a single lw.  Explicit
   (v.vy << 16) | v.vx recombination is worse (69.61).  The A pair loads lh
   (signed) and the B pair lhu, so the B fields are unsigned halfwords.
void func_800B6518(u8 *pActor) {
    SVECTOR a;
    SVECTOR b;
    s16 angle;

    a.vx = *(s16 *)(pActor + 0x2);
    a.vy = *(s16 *)(pActor + 0xA);
    b.vx = *(u16 *)(pActor + 0xA0);
    b.vy = *(u16 *)(pActor + 0xA4);
    angle = func_80023124(*(s32 *)&b, *(s32 *)&a);
    func_80021FE0(pActor, angle);
    func_800223B0(pActor, angle);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B65B0);
void func_800B6808(u8 *p) {
    u16 rot[3];
    VECTOR diff;
    VECTOR sq;
    SVECTOR vec;
    MATRIX mtx;
    VECTOR out;
    s32 len;
    s16 zero;

    zero = 0;
    diff.vx = *(s16 *)(p + 0xA0) - *(s16 *)(p + 0x2);
    diff.vy = *(s16 *)(p + 0xA2) - *(s16 *)(p + 0x6);
    diff.vz = *(s16 *)(p + 0xA4) - *(s16 *)(p + 0xA);
    Square0(&diff, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    rot[1] = -ratan2(diff.vz, diff.vx);
    rot[2] = ratan2(diff.vy, len);
    rot[0] = 0;
    *(u16 *)(p + 0x32) = rot[1];
    func_80021B04(&vec, (*(s32 *)(p + 0x18) << 9) >> 16, zero, zero);
    RotMatrix((SVECTOR *)rot, &mtx);
    ApplyMatrix(&mtx, &vec, &out);
    *(s32 *)(p + 0xC) = out.vx << 7;
    *(s32 *)(p + 0x10) = out.vy << 7;
    *(s32 *)(p + 0x14) = out.vz << 7;
}
void func_800B6930(u32 *pOut, s8 *p) {
    p += (p[1] << 8) | (u8)p[0];
    pOut[0] = ((p[1] << 8) | (u8)p[0]) << 16;
    pOut[1] = ((p[3] << 8) | (u8)p[2]) << 16;
    pOut[2] = ((p[5] << 8) | (u8)p[4]) << 16;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6990);
/* best 52.71%: the first read (p += ...) matches byte for byte with this shape;
   every pOut store instead gets lbu + sll 24/sra 16 because gcc truncates the
   signed high byte into a halfword store.  Zero-extending both bytes (u8*) or
   s8* with a (u8) cast on the low byte give the identical bad shape.
void func_800B6990(u16 *pOut, s8 *p) {
    p += (p[1] << 8) | (u8)p[0];
    pOut[0] = (p[1] << 8) | (u8)p[0];
    pOut[1] = (p[3] << 8) | (u8)p[2];
    pOut[2] = (p[5] << 8) | (u8)p[4];
}
*/
void func_800B69E4(u16 *pOut, s8 *p) {
    s32 v;

    p += (p[1] << 8) | (u8)p[0];
    v = (p[1] << 8) | (u8)p[0];
    pOut[0] += v;
    v = (p[3] << 8) | (u8)p[2];
    pOut[1] += v;
    v = (p[5] << 8) | (u8)p[4];
    pOut[2] += v;
}
void func_800B6A50(u8 *p) {
    u8 *pSrc = *(u8 **)(p + 0x70);

    *(u32 *)(p + 0xC) = *(u32 *)(pSrc + 0xC);
    *(u32 *)(p + 0x10) = *(u32 *)(pSrc + 0x10);
    *(u32 *)(p + 0x14) = *(u32 *)(pSrc + 0x14);
}
void func_800B6A7C(u8 *pActor, s8 *pArg) {
    s32 x2;
    s8 *p;
    s32 x1;
    s32 x3;

    p = pArg + ((pArg[1] << 8) | (u8)pArg[0]);
    if (*(u16 *)(pActor + 0x3A) != 0) {
        x1 = func_80022CAC(pActor, (p[1] << 3) | (u8)p[0]) << 8;
        x2 = func_80022CAC(pActor, (u8)p[2]) << 16;
        x3 = func_80022CAC(pActor, (u8)p[3]) << 16;
    } else {
        x1 = ((p[1] << 3) | (u8)p[0]) << 5;
        x2 = (u8)p[2] << 13;
        x3 = (u8)p[3] << 13;
    }
    func_801FC4C4(*(u32 *)(*(u32 *)(pActor + 0x20) + 0x34), *(u32 *)(*(u32 *)(pActor + 0x20) + 0x2C),
                  *(u32 *)(pActor + 0x20) + 0xC, x1, x2, x3, (u8)p[4] << 4, (u8)p[5] << 2);
    *(u32 *)(*(u32 *)(pActor + 0x20) + 0x34) = 0;
    *(u32 *)(*(u32 *)(pActor + 0x20) + 0x2C) = 0;
    *(u32 *)(*(u32 *)(pActor + 0x20) + 0x30) = 0;
}
void func_800B6B98(u8 *pArg, s8 *p) {
    p += (p[1] << 8) | (u8)p[0];
    func_801FC53C(pArg, (u8)p[0] << 4, (u8)p[1], p[2] << 3, (u8)p[3] << 3, p[4] << 3, (u8)p[5]);
}
void func_800B6BFC(void) {
    RECT rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xE0;
    MoveImage(&rect, 0x2C0, 0x100);
    func_800B73A0();
}
void func_800B6C44(u8 *p) {
    *(u16 *)(*(u32 *)(p + 0x20) + 4) = ratan2(*(s32 *)(p + 0x10) >> 8, *(s32 *)(p + 0xC) >> 8);
    *(u32 *)(p + 0x3C) |= 0x10000000;
}
void func_800B6C98(u8 *p) {
    *(u16 *)(*(u32 *)(p + 0x20) + 0) = ratan2(*(s32 *)(p + 0x14) >> 8, *(s32 *)(p + 0xC) >> 8);
    *(u32 *)(p + 0x3C) |= 0x10000000;
}
void func_800B6CEC(u8 *p) {
    VECTOR vec;
    VECTOR sq;
    s32 len;

    vec.vx = *(s32 *)(p + 0xC) >> 8;
    vec.vy = *(s32 *)(p + 0x10) >> 8;
    vec.vz = *(s32 *)(p + 0x14) >> 8;
    if (vec.vz == 0) {
        vec.vz = 4;
    }
    Square0(&vec, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    *(s16 *)(*(u32 *)(p + 0x20) + 0x2) = -ratan2(vec.vz, vec.vx);
    *(s16 *)(*(u32 *)(p + 0x20) + 0x4) = ratan2(vec.vy, len);
    *(s16 *)(*(u32 *)(p + 0x20) + 0x0) = 0;
    *(u32 *)(p + 0x3C) |= 0x10000000;
}
void func_800B6DC0(u8 *p) {
    s32 i;

    if (*(u32 *)(p + 0x20) == 0) {
        return;
    }
    if (*(u32 *)(*(u32 *)(p + 0x20) + 0x34) == 0) {
        return;
    }
    for (i = 0; i != 8; i++) {
        *(u8 *)(*(u32 *)(*(u32 *)(p + 0x20) + 0x34) + i * 8 + 0x0) = 0;
        *(u8 *)(*(u32 *)(*(u32 *)(p + 0x20) + 0x34) + i * 8 + 0x1) = 0;
        *(u16 *)(*(u32 *)(*(u32 *)(p + 0x20) + 0x34) + i * 8 + 0x2) = 0;
        *(u16 *)(*(u32 *)(*(u32 *)(p + 0x20) + 0x34) + i * 8 + 0x4) = 0;
        *(u16 *)(*(u32 *)(*(u32 *)(p + 0x20) + 0x34) + i * 8 + 0x6) = 0;
    }
    *(u8 *)(*(u32 *)(p + 0x20) + 0x3C) = 0;
    *(u8 *)(*(u32 *)(p + 0x20) + 0x3D) = 0;
}
void func_800B6E84(u8 *pActor, s8 *p) {
    SVECTOR rotation;
    MATRIX mtx;
    VECTOR vec;
    s16 zero;
    s32 z;

    zero = 0;
    z = p[0] << 4;
    func_80021B04(&rotation, zero, zero, z);
    RotMatrix(&rotation, &mtx);
    ApplyMatrixLV(&mtx, (VECTOR *)(pActor + 0xC), &vec);
    *(s32 *)(pActor + 0xC) = vec.vx;
    *(s32 *)(pActor + 0x10) = vec.vy;
    *(s32 *)(pActor + 0x14) = vec.vz;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6F0C);
void func_800B7134(void) {
    D_800C3CB4 = g_GfxCurOT;
    func_800B7160();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B7160);
void func_800B7330(void *p) {
    DrawSync(0);
    HeapFree(p);
}
void func_800B7364(u8 *p) {
    WorkListRemoveTask(p + 0x1C);
    TimerWorkListRemoveTask(p);
    func_80025180(p);
}
void func_800B73A0(void) {
    func_800B7424(WorkListsAddTasks(0x10F7C, 0, func_800B6F0C, func_800B7134, func_800B7364));
}
