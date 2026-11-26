#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern u8 g_BattleCommandGridIds[];
extern u8 *D_800D329C;
extern u8 *g_BattleGraphicsWork;
extern s32 g_BattlePrimStartIndex;
extern u8 *g_BattleDisplayState;
extern u8 *g_BattleSlotGraphics[];
extern u8 *g_BattleSlotRects[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
extern u8 D_800CCD38[];
extern u8 D_800CCD3A[];
extern u8 g_BattleCommandCellPanelSprites;
extern u8 D_800D2F7D;
extern u8 D_800D2F7E;
extern u8 D_800D2F7F;
extern u8 D_800D2F80;
extern u8 D_800D2F81;
extern u8 D_800D2F82;
extern u8 D_800D2F83;

extern void BattleUpdateFrame(void);
extern void HeapFree(void *pMem);
extern void func_8008FC1C(s32, s32, s32, s32, u8);
extern s32 BattleDrawSpritePackage(s32, s32, s16, s16);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx);
extern void BattleLoadImageAndSync(RECT *pRect, u_long *pData);
extern void *BattleAllocTextBuffer(s32);
extern void bzero(void *p, s32 n);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern u8 g_BattleCommandGridAltIds[];
extern void *BattleAllocHeapBlock(s32, s32);
extern void BattleInitSlotPrims(s32);
extern void func_8008F6E4(u8, s32, s32, s32, s32);
extern void BattleDrawSpriteQuad(u8, s32, s32, s32, u16);
extern void BattleBuildTileRow0Prims(u8, s32, s32, s32);
extern void BattleBuildTileRow1Prims(u8, s32, s32, s32, u16);
extern void BattleBuildTileRow2Prims(u8, s32, s32, s32);
extern void BattleBuildTileRow3Prims(u8, s32, s32, s32, u16);
extern void func_80090310(u8, u8);
extern void BattleDrawCommandCellIcons(s32, s32);
extern void BattleDrawMenuEntryString(u8, u8);
extern void *g_BattleSpriteArchive;
extern u8 g_BattleDisplayMode;
extern void BattleSetPrimGrey80BlendModeFromGlobal(void *);
extern s32 func_800263E4(void *, s32, void *, s32, s16, s16, s32, s32, s32);
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern u8 D_800CEF34[];
extern u8 g_BattleActionRows[];
extern u8 D_800CCDC4[];
extern u8 g_BattleSlotStatusRows[];
extern u8 g_BattleActorCharIds[];
extern s32 BattleMaskHasStatusBit(s32, u8);
extern u8 BattleRunTargetSlotSelection(u16, u8, s32);
extern void BattleClearPrimFlagsThreeSlots(u8);
extern void BattleSetPrimFlagsThreeSlots(u8);
extern void BattlePlaySoundIfEnabled(u8);
extern void BattleSetPolyFT4Rect(POLY_FT4 *, s16, s16, u8, u8, s16, s16);


void BattleFreeSlotBuffers(u8 arg0) {
    u8 *p;

    p = g_BattleDisplayState + arg0;
    p[0xB0] = 0;
    p = g_BattleDisplayState + arg0;
    p[0xB8] = 0;
    BattleUpdateFrame();
    HeapFree(g_BattleSlotGraphics[arg0]);
    HeapFree(g_BattleSlotRects[arg0]);
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
   line behind an extra `j` plus `move v0,a1` (88.4). void BattleExpandSlotRects(void) {
    s32 i;
    u8 *p;
    u8 *pFlags;
    u8 count;
    u16 x;
    u16 y;

    for (i = 0; i < 7; i++) {
        p = g_BattleSlotRects[i];
        pFlags = g_BattleDisplayState + i;
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
            g_BattleDisplayState[i + 0xBF] = 1;
        }
        x = ((u16 *)p)[0] + (((u16 *)p)[2] >> 1) - (((u16 *)p)[4] >> 1);
        y = ((u16 *)p)[1] + (((u16 *)p)[3] >> 1) - (((u16 *)p)[5] >> 1);
        func_8008F6E4(p[0xC], x, y, ((u16 *)p)[4], ((u16 *)p)[5]);
    }
}
*/
