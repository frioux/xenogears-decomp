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

// One actor's block of the per-actor action-row table g_BattleActionRows: 42 rows per
// actor (BattleRow, 0x28 bytes, actor.h), of which this unit reads +0x0 and +0x24 of
// the four rows at +0x25. The offset stays an integer: indexing a BattleRow array
// makes gcc form the actor term (arg0 * 0x690) before the row term and reach both
// fields off one base register (464 bytes, 86.94%), where retail forms the row term
// first and re-forms the symbol for each read.
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
extern void func_8008F6E4(s32, u16, u16, u16, u16);
extern void BattleDrawSpriteQuad(u8, s32, s32, s32, u16);
extern void func_8008DE04(u8, s32, s32, s32);
extern void func_8008E430(u8, s32, s32, s32, u16);
extern void func_8008EA70(u8, s32, s32, s32);
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
extern u8 D_800CCD88[];
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
    trigger = *(u16 *)(g_BattleActionRows + idx + 0x24); // the action row's +0x24 word
    cur = D_800CCDC4[arg0].flags;
    dmg = *(u16 *)(g_BattleActionRows + idx); // the action row's +0x0 word
    if (cur >= trigger &&
        (u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].statusFlags, arg1) != 0 &&
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
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].statusFlags, i) != 0 &&
            (g_BattleSlotStatusRows[arg0].flags & statusMasks[i]) == 0) {
            codes[i] = D_800CCD88[arg0 * 0x170] * 4 + i;
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
    pPackage = (BattleSpritePackage *)p;
    p->packageCount = BattleDrawSpritePackage(codes[0], (s32)&pPackage[p->packageCount], (s16)arg1, (s16)arg2) + p->packageCount;
    dy = arg2; // the sum must go through dy: dy + arg4 keeps retail's addu operand order
    dx = arg1 + arg3 - 8;
    p->packageCount = BattleDrawSpritePackage(codes[1], (s32)&pPackage[p->packageCount], dx, (s16)dy) + p->packageCount;
    dy = (s16)(dy + arg4 - 8);
    p->packageCount = BattleDrawSpritePackage(codes[2], (s32)&pPackage[p->packageCount], (s16)arg1, dy) + p->packageCount;
    p->packageCount = BattleDrawSpritePackage(codes[3], (s32)&pPackage[p->packageCount], dx, dy) + p->packageCount;
    for (i = 0; i < BATTLE_SPRITE_PACKAGE_COUNT; i++) {
        pPrims = (POLY_FT4 *)p;
        BattleSetPrimGrey80BlendModeFromGlobal(&pPrims[i * 2 + g_BattlePrimStartIndex]);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008DE04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008E430);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008EA70);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F0A8);
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
    p->unk5A4[0] = *(u8 *)&g_BattlePrimStartIndex;
    ((u8 *)g_BattleDisplayState)[index + 0xB0] = 1;
}
*/
// Allocates a slot's draw block and rect list on first use, then either fills the
// rect list from the four coordinates and marks the slot dirty (arg5) or hands
// them to func_8008F6E4 and advances the frame (arg6).
void BattleInitSlotGraphics(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u16 arg4, u8 arg5, u8 arg6) {
    u8 *pFlags;
    BattleWork *pState;
    u16 slot;
    u8 index;
    BattleSlotRect *pRect;
    void *q;

    slot = arg0;
    index = slot; // two steps: the u16 copy is what puts arg0 in s2, the u8 index in s1
    if (*((u8 *)g_BattleDisplayState + index + 0xB0) == 0) {
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
        pFlags = (u8 *)g_BattleDisplayState;
        pRect->x = arg1;
        pRect->y = arg2;
        pRect->targetW = arg3;
        pRect->targetH = arg4;
        pRect->w = 0;
        pRect->h = 0;
        *(pFlags + index + 0xBF) = 0;
        pState = g_BattleDisplayState; // the BattleWork temp only decides v0/v1 at the store
        pFlags = (u8 *)pState;
        *(pFlags + index + 0xB8) = 1;
    } else {
        func_8008F6E4(index, arg1, arg2, arg3, arg4);
        if (arg6 != 0) {
            BattleUpdateFrame();
        }
    }
}
