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
extern void func_80021B04(SVECTOR *pVector, s16 x, s16 y, s16 z);
extern void func_801FC53C(void *pArg, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B63F0);
/* best 77.22%: retail passes the three zero args from one register
   (li a1,0; move a2,a1; move a3,a1) where gcc emits a separate li per arg.
void func_800B63F0(s32 arg0, u8 *p) {
    s32 pad[3];
    u8 *pData;

    pData = p + ((((s8 *)p)[1] << 8) | p[0]);
    pad[0] = 0;
    pad[1] = 0;
    pad[2] = 0;
    func_8002DDE4(pData, 0, 0, 0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6438);
/* best 63.64%: retail puts the `lui a1,%hi(D_80025A88)` of the callback address
   before the `sw ra` frame store; gcc emits it after the pointer load.
void func_800B6438(u8 *p) {
    WorkListSetTaskCallback((u8 *)*(u32 *)(p + 0x6C) + 0x1C, D_80025A88);
}
*/
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
/* best 77.59%: retail adds 0x8C8C as a separate li+addu on the runtime base
   (li v1,0x8C8C; addu v0,v0,v1; lw a0,0(v0)), gcc always folds it into the
   symbol address as lui at,0x1 / lw a0,-29556(at).  Tried: local base pointer,
   separate offset statements, u32 integer base, gcc-2.6.0 and 2.7.2-cdk.
void func_800B64D4(s32 arg0, u8 *p) {
    u32 nEntry = (u32)D_800C3EB0 + (p[1] << 2);

    func_800245D8(*(u32 *)(nEntry + 0x8C8C), p[0]);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6518);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B65B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6808);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6A7C);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6CEC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6DC0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6E84);
/* best 99.85%: only the second zero argument differs - retail copies a1 into
   a2 (one shared register for both zero args) where gcc rematerialises
   move a2,zero.  Tried: literal 0, s32/s16 zero locals, (s16) cast, shared
   variable for both args.  Size and every other instruction match.
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
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6F0C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B7134);
/* best 65.36%: retail stores D_800C3CB4 through a register base
   (lui v0,%hi; sw v1,%lo(v0), in the jal delay slot) where gcc+maspsx emit
   lui at,%hi / sw v0,0(at).  Same $at store as misc26's func_800B3588.
void func_800B7134(void) {
    D_800C3CB4 = g_GfxCurOT;
    func_800B7160();
}
*/
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B73A0);
/* best 78.95%: same instructions, only the position of `move a1,zero` and the
   `lui a3,%hi(func_800B7134)` differ.
void func_800B73A0(void) {
    func_800B7424(WorkListsAddTasks(0x10F7C, 0, func_800B6F0C, func_800B7134, func_800B7364));
}
*/
