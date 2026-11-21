#include "common.h"
#include "main/game.h"
#include "battle/actor.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

// The sound the battle menu plays when it refuses an input: the branch that found
// nothing to act on. battle/main/misc12 names the same id for the same call.
#define BATTLE_SOUND_MENU_DENIED 0x4F

// The sprite packages BattleDrawSpriteQuad appends to a slot's draw block, one per
// corner of the quad, each drawn with its own code.
#define BATTLE_SPRITE_PACKAGE_COUNT 4

// The tile prims one tile-row builder fills in a slot's draw block: the two quads at
// tilePrims[row][g_BattlePrimStartIndex] and [g_BattlePrimStartIndex + 2].
#define BATTLE_TILE_QUAD_COUNT 2

// Where a tile-row builder takes its texels from: the low byte of the record for the
// row's texture (g_BattleGraphicsWork).tiles[row + 1] (stride 0x18 from 0xA238,
// battle_work.h), texPageX for u and texPageY 4 bytes later for v. Read as bytes, as
// the original does, because the u coordinate is the page byte's low six bits.
#define BATTLE_TILE_SOURCE_BASE 0xA25C
#define BATTLE_TILE_SOURCE_STRIDE 0x18
#define BATTLE_TILE_SOURCE_V 4

// One actor's block of the per-actor action-row table g_BattleActionRows: 42 rows per
// actor (BattleRow, 0x28 bytes, actor.h), of which this unit reads flags (+0x0) and
// commandPointCost (+0x24) of the four rows at +0x25. The offset stays an integer:
// indexing a BattleRow array makes gcc form the actor term (arg0 * 0x690) before the
// row term and reach both fields off one base register (464 bytes against retail's
// 468, 87.56%), where retail forms the row term first and re-forms the symbol for
// each read.
#define BATTLE_ACTION_ROW_STRIDE 0x690
extern u8 g_BattleActionRows[];

// One sprite package of the per-slot draw block: the two POLY_FT4
// BattleDrawSpritePackage writes, i.e. the 0x50-byte stride the block's package
// counter advances by.
typedef struct {
    /* 0x00 */ POLY_FT4 prims[2];
} BattleSpritePackage; // Size: 0x50

// One slot's rect record (g_BattleSlotRects[slot]): the coordinates the draw block
// expands from and the size the expand animation grows to (battle/misc16_r2 reads
// the same record through the same array).
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 targetW;
    /* 0x6 */ u16 targetH;
    /* 0x8 */ u16 w;
    /* 0xA */ u16 h;
    /* 0xC */ u8 slot;
    /* 0xD */ u8 padD;
} BattleSlotRect; // Size: 0xE

extern u8 g_BattleCommandGridIds[];
extern u8 *D_800D329C;
extern u8 *g_BattleGraphicsWork;
extern u8 *g_BattleCommandQueue;
extern volatile s32 g_BattlePrimStartIndex;
extern BattleWork *g_BattleDisplayState;
extern BattleSlotGraphics *g_BattleSlotGraphics[];
extern BattleSlotRect *g_BattleSlotRects[];
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

extern void BattleUpdateFrame(void);
extern void HeapFree(void *pMem);
extern void func_8008FC1C(s32, s32, s32, s32, u8);
extern s32 BattleDrawSpritePackage(s32, u8 *, s16, s16);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx);
extern void BattleLoadImageAndSync(RECT *pRect, u_long *pData);
extern void *BattleAllocTextBuffer(s32);
extern void bzero(void *p, s32 n);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern u8 g_BattleCommandGridAltIds[];
extern void *BattleAllocHeapBlock(s32, s32);
extern void BattleInitSlotPrims(s32);
extern void func_8008F6E4(s32, u16, u16, u16, u16);
extern void BattleDrawSpriteQuad(u8, s32, s32, s32, u16);
extern void func_8008DE04(u8, s32, s32, u16);
extern void func_8008E430(u8, s32, s32, u16, s32);
extern void func_8008EA70(u8, s32, s32, u16);
extern void func_8008F0A8(u8, s32, s32, s32, u16);
extern void func_80090310(u8, u8);
extern void func_800904A0(s32, s32);
extern void BattleDrawMenuEntryString(u8, u8);
extern void *g_BattleSpriteArchive;
extern u8 g_BattleDisplayMode;
extern void BattleSetPrimGrey80BlendModeFromGlobal(POLY_FT4 *);
extern s32 func_800263E4(void *, s32, void *, s32, s16, s16, s32, s32, s32);
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
// g_BattleSlotStatusRows and D_800CCDC4 are two bases of the same per-slot table (stride
// 0x170, actor.h's BattleTableEntry): the status mask sits at +0 of the g_BattleSlotStatusRows
// base and the slot's counter at +0 of the D_800CCDC4 base, 0x62 bytes later.
extern BattleTableEntry g_BattleSlotStatusRows[];
extern BattleTableEntry D_800CCDC4[];
extern u8 g_BattleActorCharIds[];
extern s32 BattleMaskHasStatusBit(s32, u8);
extern u8 BattleRunTargetSlotSelection(u16, u8, s32);
extern void BattleClearPrimFlagsThreeSlots(u8);
extern void BattleSetPrimFlagsThreeSlots(u8);
extern void BattlePlaySoundIfEnabled(u8);
extern void BattleSetPolyFT4Rect(POLY_FT4 *, s16, s16, u8, u8, s16, s16);
extern u8 g_BattleSlotTextIndex[];
extern u8 g_BattleMenuEvent;
extern void BattleFreePacketPrimitives(void);
extern void BattleClearFlagC6(void);
extern void BattleFreeSlotBuffers(u8);
extern void BattleDrawCharStatusIfMaskUnset(u8, u8);
extern void BattleStepAnimatedCursorPrim(s32, s32, s32 *, u8 *);
extern void func_800930AC(u8, u8 *, u16 *);
extern void BattleInitPulseTrianglePrims(void);

u8 BattleTryUseSlotCommand(u8 arg0, u8 arg1) {
    u16 statusMasks[4];
    u8 flag;
    u8 ret;
    s32 idx;
    u16 trigger;
    u16 cur;
    u16 dmg;

    statusMasks[0] = D_800C324E;
    statusMasks[1] = D_800C3250;
    statusMasks[2] = D_800C3252;
    statusMasks[3] = D_800C323A;
    ret = 0;
    flag = 1;
    idx = (arg1 + 0x25) * sizeof(BattleRow) + arg0 * BATTLE_ACTION_ROW_STRIDE;
    trigger = *(u16 *)(g_BattleActionRows + idx + 0x24); // row's commandPointCost
    cur = D_800CCDC4[arg0].flags;
    dmg = *(u16 *)(g_BattleActionRows + idx); // row's flags
    if (cur >= trigger &&
        (u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].statusFlags, arg1) != 0 &&
        (g_BattleSlotStatusRows[arg0].flags & statusMasks[arg1]) == 0) {
        BattleClearPrimFlagsThreeSlots(1);
        g_BattleDisplayState->pulseTriangleActive = 1;
        if (BattleRunTargetSlotSelection(dmg, arg0, 0) != 0) {
            D_800CCDC4[arg0].flags -= trigger;
            ret = 1;
        } else {
            g_BattleDisplayState->pulseTriangleActive = 0;
            BattleSetPrimFlagsThreeSlots(arg0);
        }
        flag = 0;
    }
    if (flag != 0) {
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
    }
    return ret;
}
/* 99.13% at retail's 880 bytes, up from 98.49% in round 167. Two
   differences left, both register and constant ties:
   1. s2 and s3 are swapped between `cursor` and the codes pointer: retail has
      cursor = s3, codes = s2. Everything else matches (s4 masked arg0, s5 row
      offset, s6 arg0 * 0x170, s1 damage pointer, s7 the constant 2).
   2. Retail runs its row offset from 0x5C8 with the base materialised as
      D_800CEF34; we strength-reduce i * 0x28 from 0 and fold 0x5C8 + 0x24 into
      the base (D_800CF4FC), so the preheader has `move s5,zero` where retail
      has `li s5,0x5c8`, and the addu operands come out swapped.
   The damage read must stay `(0x25 + i) * 0x28` inline in the address. An
   explicit rowOffset local (for-init or statement before the loop, with or
   without a rowBase) puts `li s5,0x5c8` at the top of the preheader instead,
   before the arg0 * 0x170 chain, for 98.49-98.54%; a statement form that also
   changes the base fold gets 97.13% / 876 bytes. `limit` must stay: writing 4
   in both places drops to 98.38%.
   Permuter: base 180 to 170 from a do-while(0) around the
   BattleStepAnimatedCursorPrim call. That wrapper alone is worth the +0.05% (a
   plain block is not, and the empty `if (1)` block it also found does nothing);
   a second 1262-iteration run found no further improvement. The 99.13% body was
   found by hand afterwards, not by the permuter.
   Tried earlier (round 167): u8/s16/s32 limit, u8/s32 state, s32 cursorTimer,
   6 declaration orders, for-init comma assignment, rowBase local, while-form
   second loop, and `s32 slotOffset = arg0 * 0x170` (fixes the chain position
   but costs a stack slot, 892 bytes).
*/
/*
u8 func_8008CFB8(u8 arg0) {
    u8 codes[4];
    u16 damage[4];
    u16 statusMasks[4];
    s32 cursorFrame;
    u8 cursorTimer;
    s32 lastCursor;
    u8 limit;
    u8 state;
    s32 cursor;
    s32 i;

    cursor = 0;
    lastCursor = 0xFF;
    state = 2;
    statusMasks[0] = D_800C324E;
    statusMasks[1] = D_800C3250;
    statusMasks[2] = D_800C3252;
    statusMasks[3] = D_800C323A;
    cursorFrame = 4;
    cursorTimer = 0;
    for (i = 0; i < 4; i++) {
        codes[i] = 0xFF;
        damage[i] = 0;
    }
    for (i = 0; i < (limit = 4); i++) {
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].statusFlags, i) != 0 &&
            (g_BattleSlotStatusRows[arg0].flags & statusMasks[i]) == 0) {
            codes[i] = g_BattleSlotTextIndex[arg0 * 0x170] * 4 + i;
            damage[i] = *(u16 *)(g_BattleActionRows + arg0 * 0x690 + (0x25 + i) * 0x28 + 0x24);
        }
    }
    g_BattleDisplayState->primListsPending = 0;
    BattleInitSlotGraphics(0, 0x74, 0xA0, 0xAC, 0x38, 0, 1);
    BattleInitSlotGraphics(1, 0x84, 0x4C, 0x9C, 0x50, 0, 1);
    BattleInitSlotGraphics(2, 0x1C, 0xA0, 0x50, 0x18, 0, 1);
    func_800930AC(arg0, codes, damage);
    BattleInitPulseTrianglePrims();
    while (state == 2) {
        if (cursor != lastCursor) {
            BattleDrawCharStatusIfMaskUnset(arg0, cursor);
            lastCursor = cursor;
        }
        do { BattleStepAnimatedCursorPrim(0x8C, cursor * 0x10 + 0x58, &cursorFrame, &cursorTimer); } while (0);
        BattleUpdateFrame();
        switch (g_BattleMenuEvent) {
        case 5:
            state = 0;
            break;
        case 4:
            if (BattleTryUseSlotCommand(arg0, cursor) != 0) {
                state = 1;
                g_BattleCommandQueue[0x2E6] = cursor + 0x10;
            }
            break;
        case 1:
            cursor++;
            if (cursor >= limit) {
                cursor = 3;
            }
            break;
        case 3:
            cursor--;
            if (cursor < 0) {
                cursor = 0;
            }
            break;
        }
    }
    BattleClearPrimFlagsThreeSlots(state);
    BattleFreePacketPrimitives();
    BattleClearFlagC6();
    BattleFreeSlotBuffers(0);
    BattleFreeSlotBuffers(1);
    BattleFreeSlotBuffers(2);
    return state;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008CFB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D328);
/* 96.12% (620 bytes against retail's 624). The address ASSOCIATION of the
   u16 |= store is the lever: written offset-chain first and gfx base last,
   `*((i + primStart) * 0x28 + j * 0xFA0 + gfx + 0x6432)`, gcc emits retail's
   two-step `addu v0,v0,s1 / addu v0,a1,v0`; the base-first spelling of the old
   park let cse hoist `gfx + j*0xFA0` into one register and cost 2 instructions
   (93.85 with that body). Everything else is matched: both guards, the two SetX
   calls, the three byte stores and the loop tails.
   Left, both allocator/cse ties: retail re-forms `s1 + base` in the 0x6420
   load's beqz delay slot and reuses the chain (a0) for the following store, and
   the inner back-branch delay slot holds a nop where we steal `li a1,1`.
   The old body's byte-offset spellings are struct arithmetic against today's
   BattleWork header and do not compile; packageCounts[j], primStart and
   primListsPending are its fields. The count must stay a live re-read of the
   global in the for-condition - that is what makes the original reload it at
   the loop tail.
void func_8008D328(void) {
    u8 found;
    s32 j;
    s32 i;

    found = 0;
    for (j = 0; j < 2; j++) {
        if (g_BattleDisplayState->packageCounts[j] != 0) {
            for (i = 0; i < g_BattleDisplayState->packageCounts[j] * 2; i += 2) {
                u8 c;

                SetSemiTrans(g_BattleGraphicsWork + 0x641C + j * 0xFA0 + (i + g_BattleDisplayState->primStart) * 0x28, 1);
                SetShadeTex(g_BattleGraphicsWork + 0x641C + j * 0xFA0 + (i + g_BattleDisplayState->primStart) * 0x28, 0);
                *(u16 *)((i + g_BattleDisplayState->primStart) * 0x28 + j * 0xFA0 + g_BattleGraphicsWork + 0x6432) |= 0x20;
                c = *(u8 *)(g_BattleGraphicsWork + j * 0xFA0 + 0x6420 + (i + g_BattleDisplayState->primStart) * 0x28);
                if (c != 0) {
                    found = 1;
                    *(u8 *)(g_BattleGraphicsWork + j * 0xFA0 + 0x6420 + (i + g_BattleDisplayState->primStart) * 0x28) = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                    *(u8 *)(g_BattleGraphicsWork + j * 0xFA0 + 0x6421 + (i + g_BattleDisplayState->primStart) * 0x28) = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                    *(u8 *)(g_BattleGraphicsWork + j * 0xFA0 + 0x6422 + (i + g_BattleDisplayState->primStart) * 0x28) = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                }
            }
        }
    }
    if (found == 0) {
        g_BattleDisplayState->primListsPending = 0;
    }
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D598);
// The per-slot draw block is a raw POLY_FT4 pool (0x28 stride) that
// BattleDrawSpriteQuad appends 0x50-byte sprite packages to; BattleSlotGraphics
// names the prims inside it, so the two views of the pool base are cast from it.
// The prim view is formed inside the loop on purpose: hoisting it above the four
// package calls gives it a second register to keep alive across them.
void BattleDrawSpriteQuad(u8 arg0, s32 arg1, s32 arg2, s32 arg3, u16 arg4) {
    u8 codes[BATTLE_SPRITE_PACKAGE_COUNT];
    BattleSlotGraphics *p;
    BattleSpritePackage *pPackage;
    POLY_FT4 *pPrims;
    s16 dx;
    s16 dy;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    if (g_BattleDisplayMode != 0) {
        codes[0] = 0x4A;
        codes[1] = 0x4C;
        codes[2] = 0x4F;
        codes[3] = 0x51;
    } else {
        codes[0] = 0xF0;
        codes[1] = 0xF2;
        codes[2] = 0xF5;
        codes[3] = 0xF7;
    }
    p->packageCount = 0;
    pPackage = (BattleSpritePackage *)p->primBuffer;
    p->packageCount = BattleDrawSpritePackage(codes[0], (u8 *)&pPackage[p->packageCount], (s16)arg1, (s16)arg2) + p->packageCount;
    dy = arg2; // the sum must go through dy: dy + arg4 keeps retail's addu operand order
    dx = arg1 + arg3 - 8;
    p->packageCount = BattleDrawSpritePackage(codes[1], (u8 *)&pPackage[p->packageCount], dx, (s16)dy) + p->packageCount;
    dy = (s16)(dy + arg4 - 8);
    p->packageCount = BattleDrawSpritePackage(codes[2], (u8 *)&pPackage[p->packageCount], (s16)arg1, dy) + p->packageCount;
    p->packageCount = BattleDrawSpritePackage(codes[3], (u8 *)&pPackage[p->packageCount], dx, dy) + p->packageCount;
    for (i = 0; i < BATTLE_SPRITE_PACKAGE_COUNT; i++) {
        pPrims = (POLY_FT4 *)p->primBuffer;
        BattleSetPrimGrey80BlendModeFromGlobal(&pPrims[i * 2 + g_BattlePrimStartIndex]);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008DE04);
/* 80.93% at retail's exact 1580 bytes (395 instructions; the 27 load-delay nops
   match). The tile-row builders (this and func_8008E430/EA70/F0A8) each write one
   row of BattleSlotGraphics' tilePrims[4][4]: quad 0 at [row][g_BattlePrimStartIndex]
   and quad 1 two prims later, i.e. a constant +0x50 displacement. That is why quad 1
   is spelled `(p->tilePrims[0] + 2)`: `p->tilePrims[0][idx + 2]` emits an addiu +2
   and loses the displacement. The index-first sum
   `(g_BattlePrimStartIndex + p->tilePrims[...])->field` is what makes gcc emit
   retail's `addu v0,v0,s1`; battle/misc3's BattleInitSlotPrims comments the same
   idiom. Every store being a volatile lvalue is the lever that took this body from
   65.30 to 80.93: with plain stores cc1 2.6.0 moves the next statement's volatile
   g_BattlePrimStartIndex load above the current store, which costs the whole
   interleaving and 14 load-delay nops (1528 against retail's 1580 bytes).
   Left: a pure three-register rotation - retail keeps left in a1, top in a0, mid in
   t0 and we get left a0, top t0, mid a1 - and retail's x1/x3 sum is
   (arg1 + 8) + half in one addu where ours reassociates to (half + 8) + arg1.
   Measured worse: an s16 `left` local (80.24), s16 locals for all five values
   (70.25 pre-volatile), the x1 store listed first (72.56), and the permuter from
   both bases (base 8640, nothing better in 4 minutes).
*/
/*
void func_8008DE04(u8 arg0, s32 arg1, s32 arg2, u16 arg3) {
    BattleSlotGraphics *p;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x0 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y0 = arg2 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x1 = (arg1 + 8) + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y1 = arg2 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x2 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y2 = arg2 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x3 = (arg1 + 8) + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y3 = arg2 + 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u0 = (g_BattleGraphicsWork[0xA25C] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v0 = g_BattleGraphicsWork[0xA260];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u1 = ((g_BattleGraphicsWork[0xA25C] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v1 = g_BattleGraphicsWork[0xA260];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u2 = (g_BattleGraphicsWork[0xA25C] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v2 = g_BattleGraphicsWork[0xA260] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u3 = ((g_BattleGraphicsWork[0xA25C] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v3 = g_BattleGraphicsWork[0xA260] + 16;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x0 = (arg1 + 8) + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y0 = arg2 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x1 = (arg1 + 8) + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y1 = arg2 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x2 = (arg1 + 8) + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y2 = arg2 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x3 = (arg1 + 8) + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y3 = arg2 + 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u0 = (g_BattleGraphicsWork[0xA25C] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v0 = g_BattleGraphicsWork[0xA260];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u1 = ((g_BattleGraphicsWork[0xA25C] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v1 = g_BattleGraphicsWork[0xA260];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u2 = (g_BattleGraphicsWork[0xA25C] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v2 = g_BattleGraphicsWork[0xA260] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u3 = ((g_BattleGraphicsWork[0xA25C] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v3 = g_BattleGraphicsWork[0xA260] + 16;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[0][g_BattlePrimStartIndex + i * 2]);
    }
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008E430);
/* 98.39% at retail's exact 1600 bytes (98.20 with retail's own store order, x0 then
   y0). The only difference is where gcc puts each store's VALUE: retail computes it
   inside that store's block (addiu a1,a1,8 lands after the first index load, the arg4
   load and its sum at the y0 store), while ours hoists the value computations of later
   volatile stores above the first store. Listing y0 first moves `left` to retail's
   exact slot and gives the 98.39% body below, at the price of emitting the two stores
   in swapped order. Permuter: 1791 iterations from the 98.20 body found only that swap
   (base 580 -> 200); a 2129-iteration run from the 98.39 body found nothing.
   Load-bearing spellings: every store must be a volatile lvalue (otherwise cc1 lifts
   the next store's g_BattlePrimStartIndex load above this one and the 27 load-delay
   nops disappear); quad 1 must stay `(p->tilePrims[1] + 2)` so +0x50 is a
   displacement; the index-first sum `(g_BattlePrimStartIndex + p->tilePrims[1])` gives
   retail's addu v0,v0,s1; and `(left = arg1 + 8)` is an embedded assignment on purpose
   - it keeps left in $a1 as retail has it and stops gcc reassociating
   (arg1 + 8) + half into arg1 + (half + 8).
   Tried and worse: an s32 `left` statement before the stores (left, the arg3 mask and
   the arg4 load all hoist; 98.20 at 1604 bytes), a `top` statement local, a
   `*(volatile s32 *)&arg4` read (1612 bytes), gcc-2.7.2 (98.14).
   ROUND 300: the value hoist is cse2's invariant-expression hoist to the top of the
   basic block, and it is NOT source-spellable away: probed with tools/scripts/
   cc1_probe.sh on a 2-store replica (a `quad[1]` stand-in for tilePrims[1]) and the
   arg4 load + `addu` still hoist above the first store with the values inline, in an
   embedded assignment, in brace blocks, in `do { } while (0)` blocks (one per store
   and one for all), with a per-store scoped temporary, with an s16-dest cast, with
   array-index vs pointer-sum addressing, and under gcc-2.6.0, 2.7.2 and 2.7.2-cdk.
   The one lever that DOES keep the load in its own block is taking the argument's
   address (`s32 *pArg4; pArg4 = &arg4;` then `arg2 + *pArg4 - 8`): the escaped
   address stops the hoist - but gcc then narrows the read to `lhu` where retail has
   `lw` (same size, same place, still not 100%). A second datapoint: five locals
   (left/bottom/top/right/farRight, each assigned at its first store) give retail's
   EXACT register assignment - a1=left, t0=bottom, a0=right, a3=half, a2=top - for
   94.88% at 1600 bytes, so the values-as-locals structure is right and only their
   placement is wrong. `-O1` removes the hoist but rewrites everything else (77.44).
*/
/*
void func_8008E430(u8 arg0, s32 arg1, s32 arg2, u16 arg3, s32 arg4) {
    BattleSlotGraphics *p;
    s32 left;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y0 = arg2 + arg4 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x0 = (left = arg1 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x1 = left + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y1 = arg2 + arg4 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y2 = arg2 + arg4 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x3 = left + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y3 = arg2 + arg4 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x0 = left + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y0 = arg2 + arg4 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x1 = left + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y1 = arg2 + arg4 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x2 = left + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y2 = arg2 + arg4 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x3 = left + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y3 = arg2 + arg4 + 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 16;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[1][g_BattlePrimStartIndex + i * 2]);
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008EA70);
/* 98.59% (1588 bytes against retail's 1592). Two differences, both in the note above:
   retail computes each store's value inside that store's block where we hoist it, and
   retail materialises the left edge as `ori $a0,0xFFF8` + `addu` (a 32-bit add) where
   every spelling of `arg1 - 8` / `arg1 + 0xFFF8` folds to `addiu -8` - that fold is
   why this body is one instruction short of retail's 1592. With `arg2 + 8` written
   inline in every y store instead of through `top` the body is retail's exact 1592
   bytes at 97.89%: the `top` local is what puts the y values in $a2 in place (retail's
   `addu a2,a2,a3` twice) and it is worth +0.7%.
   Row 2 is the vertical twin of row 1: x spans arg1 - 8..arg1 + 8 and arg3 is the
   HEIGHT, split down the two stacked quads.
*/
/*
void func_8008EA70(u8 arg0, s32 arg1, s32 arg2, u16 arg3) {
    BattleSlotGraphics *p;
    s32 top;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->x0 = arg1 + 0xFFF8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->y0 = (top = arg2 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->x1 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->y1 = top;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->x2 = arg1 + 0xFFF8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->y2 = top + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->x3 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->y3 = top + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->x0 = arg1 + 0xFFF8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->y0 = top + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->x1 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->y1 = top + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->x2 = arg1 + 0xFFF8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->y2 = top + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->x3 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->y3 = top + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[2][g_BattlePrimStartIndex + i * 2]);
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F0A8);
/* 80.42% (1596 bytes against retail's 1600) with today's headers. Row 3 is the
   vertical twin of row 2 shifted to the right end of arg1..arg1 + arg3: retail hoists
   `addu a1,a1,a3` (the centre) to the very top, computes `addiu a3,a1,-8` after the
   first index load and keeps left in $a3, and holds the y values in $a2 in place
   (`addiu a2,a2,8` for the top, `addu a2,a2,a0` twice for the halves). We keep left in
   $t0 and hoist the vertical values - the same lever func_8008E430's note describes,
   which has to be solved there first. arg4 is the HEIGHT (lhu from the stack, so u16);
   arg3 is added to arg1 raw, so it is s32.
*/
/*
void func_8008F0A8(u8 arg0, s32 arg1, s32 arg2, s32 arg3, u16 arg4) {
    BattleSlotGraphics *p;
    s32 left;
    s32 top;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->x0 = (left = arg1 + arg3 - 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->y0 = (top = arg2 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->x1 = arg1 + arg3 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->y1 = top;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->y2 = top + (arg4 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->x3 = arg1 + arg3 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->y3 = top + (arg4 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->x0 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->y0 = top + (arg4 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->x1 = arg1 + arg3 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->y1 = top + (arg4 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->y2 = top + (arg4 - 16) / 2 + (arg4 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->x3 = arg1 + arg3 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->y3 = top + (arg4 - 16) / 2 + (arg4 - 16) / 2;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[3][g_BattlePrimStartIndex + i * 2]);
    }
}
*/

// Fills both greyscale quads' corners at the slot's current prim index, draws the
// slot's four sprite packages and records the prim index they were built at. The
// callees take u16 coordinates, so each argument is truncated once here.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F6E4);
/* 69.38% (532 bytes against retail's 528) with today's headers. Everything but
   one register tie is in the body below; what retail's .s shows:
   - the coordinates are u16: gcc masks each of x/y/w/h once with 0xFFFF (they go
     to the callees' s32 parameters) and reuses the masked registers for all five
     calls. Declaring them s32 loses all four masks (508 bytes, 70.05%).
   - g_BattlePrimStartIndex must be volatile: retail re-reads it with a fresh
     lui/lw for EVERY one of the eight sh stores (see the declaration above).
   - the slot pointer g_BattleSlotGraphics[index] is loaded once into a saved
     register (s2) and all eight prim stores go through it; the eight stores are
     p->prims[index].x0/y0/x1/y1/x2/y2/x3/y3 with the sums (x+w, y+h) left inline
     so gcc CSEs each into one register.
   - the trailing byte store reads the index back with lbu, hence the u8 cast.
   Left: retail keeps index in s3, x s1, y s0, w s5, h s4, p s2; we get index s1,
   p s0, x s3, y s4, w s5, h s6 and copy a3 to a temp for w where retail masks w
   in place (one extra move). Hand shapes tried: sums as right/bottom locals
   (516 bytes), p loaded after the 0xB0 store (61.88%), u32/u8 index, s32 params
   with u16 callee coordinates (520); a 4-minute permuter run from this body
   found only a worse-sized variant (540 bytes, 73.98%). Next lever: the u16
   narrowing that closed BattleInitSlotGraphics below (a u16 copy of arg0 feeding a u8
   index) - not yet tried here.
void func_8008F6E4(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4) {
    BattleSlotGraphics *p;
    u8 index;

    index = arg0;
    p = g_BattleSlotGraphics[index];
    ((u8 *)g_BattleDisplayState)[index + 0xB0] = 0;
    p->prims[g_BattlePrimStartIndex].x0 = arg1;
    p->prims[g_BattlePrimStartIndex].y0 = arg2;
    p->prims[g_BattlePrimStartIndex].x1 = arg1 + arg3;
    p->prims[g_BattlePrimStartIndex].y1 = arg2;
    p->prims[g_BattlePrimStartIndex].x2 = arg1;
    p->prims[g_BattlePrimStartIndex].y2 = arg2 + arg4;
    p->prims[g_BattlePrimStartIndex].x3 = arg1 + arg3;
    p->prims[g_BattlePrimStartIndex].y3 = arg2 + arg4;
    BattleDrawSpriteQuad(index, arg1, arg2, arg3, arg4);
    func_8008DE04(index, arg1, arg2, arg3);
    func_8008E430(index, arg1, arg2, arg3, arg4);
    func_8008EA70(index, arg1, arg2, arg4);
    func_8008F0A8(index, arg1, arg2, arg3, arg4);
    p->packagePrimStart = *(u8 *)&g_BattlePrimStartIndex;
    ((u8 *)g_BattleDisplayState)[index + 0xB0] = 1;
}
*/
// Allocates a slot's draw block and rect list on first use, then either fills the
// rect list from the four coordinates and marks the slot dirty (arg5) or hands
// them to func_8008F6E4 and advances the frame (arg6).
void BattleInitSlotGraphics(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u16 arg4, u8 arg5, u8 arg6) {
    u16 slot;
    u8 index;
    BattleSlotRect *pRect;
    void *q;

    slot = arg0;
    index = slot; // two steps: the u16 copy is what puts arg0 in s2, the u8 index in s1
    if (g_BattleDisplayState->primFlags[index] == 0) {
        q = BattleAllocHeapBlock(sizeof(BattleSlotGraphics), 0);
        g_BattleSlotGraphics[index] = q;
        bzero(q, sizeof(BattleSlotGraphics));
        q = BattleAllocHeapBlock(sizeof(BattleSlotRect), 0);
        g_BattleSlotRects[index] = q;
        bzero(q, sizeof(BattleSlotRect));
        BattleInitSlotPrims(index);
    }
    if (arg5 != 0) {
        pRect = g_BattleSlotRects[index];
        pRect->slot = arg0;
        pRect->x = arg1;
        pRect->y = arg2;
        pRect->targetW = arg3;
        pRect->targetH = arg4;
        pRect->w = 0;
        pRect->h = 0;
        g_BattleDisplayState->rectExpandDone[index] = 0;
        g_BattleDisplayState->rectExpandActive[index] = 1;
    } else {
        func_8008F6E4(index, arg1, arg2, arg3, arg4);
        if (arg6 != 0) {
            BattleUpdateFrame();
        }
    }
}
