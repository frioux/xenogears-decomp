#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern u8 D_800D2CE0[];
extern u8 *D_800D329C;
extern u8 *D_800C3EA4;
extern s32 D_800CCB34;
extern u8 *D_800D2D28;
extern u8 *D_800D2E38[];
extern u8 *D_800D2D90[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
extern u8 D_800CCD38[];
extern u8 D_800CCD3A[];
extern u8 D_800D2F7C;
extern u8 D_800D2F7D;
extern u8 D_800D2F7E;
extern u8 D_800D2F7F;
extern u8 D_800D2F80;
extern u8 D_800D2F81;
extern u8 D_800D2F82;
extern u8 D_800D2F83;

extern void func_800716D8(void);
extern void HeapFree(void *pMem);
extern void func_8008FC1C(s32, s32, s32, s32, u8);
extern s32 func_80076A10(s32, s32, s16, s16);
extern void func_80076C78(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx);
extern void func_800769E8(RECT *pRect, u_long *pData);
extern void *func_8008AC00(s32);
extern void bzero(void *p, s32 n);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern u8 D_800C3D70[];
extern void *func_8008ABB8(s32, s32);
extern void func_80077454(s32);
extern void func_8008F6E4(u8, s32, s32, s32, s32);
extern void func_8008DC34(u8, s32, s32, s32, u16);
extern void func_8008DE04(u8, s32, s32, s32);
extern void func_8008E430(u8, s32, s32, s32, u16);
extern void func_8008EA70(u8, s32, s32, s32);
extern void func_8008F0A8(u8, s32, s32, s32, u16);
extern void func_80090310(u8, u8);
extern void func_800904A0(s32, s32);
extern void func_8009070C(u8, u8);
extern void *D_800D2F5C;
extern u8 D_800C3E4C;
extern void func_80076B00(void *);
extern s32 func_800263E4(void *, s32, void *, s32, s16, s16, s32, s32, s32);
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern u8 D_800CEF34[];
extern u8 D_800CEF10[];
extern u8 D_800CCDC4[];
extern u8 D_800CCD62[];
extern u8 D_800D2D24[];
extern s32 func_80089C6C(s32, u8);
extern u8 func_80085084(u16, u8, s32);
extern void func_8008CCCC(u8);
extern void func_8008CD28(u8);
extern void func_8008AA74(u8);
extern void func_80076CE8(POLY_FT4 *, s16, s16, u8, u8, s16, s16);


void func_8008FA60(u8 arg0) {
    u8 *p;

    p = D_800D2D28 + arg0;
    p[0xB0] = 0;
    p = D_800D2D28 + arg0;
    p[0xB8] = 0;
    func_800716D8();
    HeapFree(D_800D2E38[arg0]);
    HeapFree(D_800D2D90[arg0]);
}
/* 90.41% (was 87.51): the `>=` polarity below is what reproduces the original's
   branch direction (bnez to the `+= 0x20` arm, `cur = w` in the fall-through)
   with count=0 placed in the bnez delay slot. Remaining gap: the original's two
   arms share ONE `sh v0,8(a0)` at the join because its else arm reloads w into
   v0 (`lhu v0,4(a0); j; ori a1,zero,1`) and its taken arm reloads cur
   (`lhu v0,8(a0); nop; addiu v0,v0,0x20`), so jump.c can cross-jump the
   identical stores; here gcc reuses the compare's registers (a1=w, a2=cur) and
   the two stores stay split. count also lands in v1 instead of a1.
   Tried: original `if (<)` polarity with per-arm `+=`/`=` stores (87.51, wrong
   branch direction) and a u16 `size` temp in both arms with one store after the
   if -- that gives the shared join store but gcc then puts the then arm out of
   line behind an extra `j` plus `move v0,a1` (88.4). void func_8008FAD8(void) {
    s32 i;
    u8 *p;
    u8 *pFlags;
    u8 count;
    u16 x;
    u16 y;

    for (i = 0; i < 7; i++) {
        p = D_800D2D90[i];
        pFlags = D_800D2D28 + i;
        if (pFlags[0xB8] == 0) {
            continue;
        }
        if (pFlags[0xBF] != 0) {
            continue;
        }
        count = 0;
        if (((u16 *)p)[4] + 0x20 >= ((u16 *)p)[2]) {
            ((u16 *)p)[4] = ((u16 *)p)[2];
            count++;
        } else {
            ((u16 *)p)[4] += 0x20;
        }
        if (((u16 *)p)[5] + 0x20 >= ((u16 *)p)[3]) {
            ((u16 *)p)[5] = ((u16 *)p)[3];
            count++;
        } else {
            ((u16 *)p)[5] += 0x20;
        }
        if (count == 2) {
            D_800D2D28[i + 0xBF] = 1;
        }
        x = ((u16 *)p)[0] + (((u16 *)p)[2] >> 1) - (((u16 *)p)[4] >> 1);
        y = ((u16 *)p)[1] + (((u16 *)p)[3] >> 1) - (((u16 *)p)[5] >> 1);
        func_8008F6E4(p[0xC], x, y, ((u16 *)p)[4], ((u16 *)p)[5]);
    }
}
*/
