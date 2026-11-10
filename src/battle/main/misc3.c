#include "common.h"
#include "psyq/libgpu.h"

extern u8 *D_800C3EA4;
extern u8 *D_800C3EAC;
extern u8 *D_800D2D28;
extern u8 D_800C400B[];
extern u8 D_800C4022[];
extern u8 D_800C402F[];
extern u8 D_800D2E60[];
extern u8 D_800D2E61[];
extern s32 D_800595A0;

extern s32 func_8008ABB8(s32, s32);
extern void func_80077074(void);
extern void HeapFree(void *pMem);
extern void bzero(void *p, s32 n);

void func_80076AC8(u8 *p) {
    SetSemiTrans(p, 1);
    SetShadeTex(p, 0);
}
void func_80076B00(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    if (D_800595A0 != 0) {
        *(u16 *)(p + 0x16) |= 0x40;
    } else {
        *(u16 *)(p + 0x16) &= ~0x40;
    }
}
void func_80076B68(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    *(u16 *)(p + 0x16) |= 0x20;
}

void func_80076BAC(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x40;
    p[5] = 0x40;
    p[6] = 0x40;
    *(u16 *)(p + 0x16) |= 0x20;
}

void func_80076BF0(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    *(u16 *)(p + 0x16) |= 0x40;
}

void func_80076C34(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x40;
    p[5] = 0x40;
    p[6] = 0x40;
    *(u16 *)(p + 0x16) |= 0x40;
}

void func_80076C78(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, u8 dx) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->y2 = y + 13;
    p->y3 = y + 13;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + dx;
    p->x3 = x + dx;
    p->v0 = v;
    p->u1 = u + dx;
    p->v1 = v;
    p->v2 = v + 13;
    p->u3 = u + dx;
    p->v3 = v + 13;
}
void func_80076CE8(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx, s16 dy) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + dx;
    p->y2 = y + dy;
    p->x3 = x + dx;
    p->y3 = y + dy;
    p->v0 = v;
    p->u1 = u + dx;
    p->v1 = v;
    p->v2 = v + dy;
    p->u3 = u + dx;
    p->v3 = v + dy;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076D58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076EA4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077074);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077364);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077454);
void func_80077610(void) {
    u8 *p = (u8 *)func_8008ABB8(0x670, 0);

    *(u8 **)(D_800C3EA4 + 0xA230) = p;
    bzero(p, 0x670);
    func_80077074();
}

void func_8007765C(void) {
    func_800716D8();
    HeapFree(*(u8 **)(D_800C3EA4 + 0xA230));
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077698);
void func_80077980(void) {
    D_800D2D28[0xC6] = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077990);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800780A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_8007819C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078310);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078508);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800785D4);
/* 57.27%: the original keeps no pointer local for the first store (reloads
   D_800C3EAC, then uses one reload for the 0xC4022 store and the count
   increment), which the pEntry local reproduces, but it schedules the index
   mask/shift after the pointer load and this version puts them first. Tried:
   tile as a u16 local (57%), lo/hi u8 locals (43%), inline tile expression,
   post-increment inside the index, pEntry for all three stores.
void func_800785D4(u8 color, u8 index) {
    u16 tile;
    u8 *pEntry;

    tile = D_800D2E60[index * 8] | (D_800D2E61[index * 8] << 8);
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = color;
    pEntry = D_800C3EAC;
    *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = tile;
    pEntry[0x2DA]++;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078658);
void func_800787E0(u8 color, u8 flag) {
    u8 *pEntry;

    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = flag;
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF7;
    pEntry = D_800C3EAC;
    *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = color;
    pEntry[0x2DA]++;
}
