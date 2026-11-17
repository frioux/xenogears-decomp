#include "common.h"
#include "main/game.h"
#include "battle/actor.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

// One actor's block of the per-actor action-row table D_800CEF10: 42 BattleRow
// rows (0x28 each, actor.h), of which this unit reads +0x24 and +0 of the rows at
// +0x25. The offset stays an integer: a BattleRow index makes gcc form the actor
// term (arg0 * 0x690) before the row term, and retail forms the row term first.
extern u8 D_800CEF10[];

// One sprite package of the per-slot draw block: the two POLY_FT4
// BattleDrawSpritePackage writes, i.e. the 0x50-byte stride the block's package
// counter advances by.
typedef struct {
    /* 0x00 */ POLY_FT4 prims[2];
} BattleSpritePackage; // Size: 0x50

extern u8 g_BattleCommandGridIds[];
extern u8 *D_800D329C;
extern u8 *g_BattleGraphicsWork;
extern u8 *g_BattleCommandQueue;
extern s32 g_BattlePrimStartIndex;
extern BattleWork *g_BattleDisplayState;
extern BattleSlotGraphics *g_BattleSlotGraphics[];
extern u8 *g_BattleSlotRects[];
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
extern void func_8008F6E4(u8, s32, s32, s32, s32);
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
// D_800CCD62 and D_800CCDC4 are two bases of the same per-slot table (stride
// 0x170, actor.h's BattleTableEntry): the status mask sits at +0 of the D_800CCD62
// base and the slot's counter at +0 of the D_800CCDC4 base, 0x62 bytes later.
extern BattleTableEntry D_800CCD62[];
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
    idx = (arg1 + 0x25) * 0x28 + arg0 * 0x690;
    trigger = *(u16 *)(D_800CEF10 + idx + 0x24);
    cur = D_800CCDC4[arg0].flags;
    dmg = *(u16 *)(D_800CEF10 + idx);
    if (cur >= trigger &&
        (u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].unk1E, arg1) != 0 &&
        (D_800CCD62[arg0].flags & statusMasks[arg1]) == 0) {
        BattleClearPrimFlagsThreeSlots(1);
        g_BattleDisplayState->unkC6 = 1;
        if (BattleRunTargetSlotSelection(dmg, arg0, 0) != 0) {
            D_800CCDC4[arg0].flags -= trigger;
            ret = 1;
        } else {
            g_BattleDisplayState->unkC6 = 0;
            BattleSetPrimFlagsThreeSlots(arg0);
        }
        flag = 0;
    }
    if (flag != 0) {
        BattlePlaySoundIfEnabled(0x4F);
    }
    return ret;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008CFB8);
/* 98.49% and retail's 880 bytes exactly (220 insns). Writing the three inits in
   retail's order (cursor = 0; lastCursor = 0xFF; state = 2) reproduces retail's
   prologue positions, and two differences are left:
   1. s3 and s4 are swapped between `cursor` and the masked copy of arg0 (the
      andi of the parameter). Retail: cursor = s3, masked arg0 = s4. With
      `state = 2` first instead, this tie comes out retail's way but then the
      `sw s7 / li s7,2` pair is emitted at the top of the prologue (98.03%).
   2. `li s5,0x5C8` (rowOffset = 0x25 * 0x28) is emitted 7 insns earlier than
      retail's. Retail has [i = 0][andi + 0x170 chain][li s5][codes ptr], so its
      0x170 computation looks like a source statement rather than a hoisted loop
      invariant; a `s32 slotOffset = arg0 * 0x170;` statement does put it in
      retail's place and fixes the s3/s4 tie as a side effect, but the extra
      local costs a stack slot: 892 bytes.
   Everything else is instruction-identical: the 4-entry code/damage fill, the
   status-mask row table read (D_800CCD62[slot].flags, D_800CCD88[slot * 0x170],
   D_800CEF10 + rowOffset + slot * 0x690 + 0x24), the three func_8008F8F4 call
   sites, func_800930AC(codes, damage), the menu-event switch (source case order
   5,4,1,3 reproduces retail's case-body layout) and the five teardown calls.
   Tried on top of this body: u8/s16/s32 `limit`, u8/s32 `state`, s32
   `cursorTimer`, 6 declaration orders (limit/state/cursor placed first, timer
   and lastCursor swapped), the for-init comma assignment for rowOffset, a
   rowBase local for the row address, and the while-form second loop above. None
   of them move the s3/s4 tie without changing the frame. Plus the two permuter
   runs from round 167 on the state-first body.
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
    s32 rowOffset;
    s32 rowBase;
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
    for (i = 0, rowOffset = 0x25 * 0x28; i < (limit = 4); i++) {
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[g_BattleActorCharIds[arg0]].unk1E, i) != 0 &&
            (D_800CCD62[arg0].flags & statusMasks[i]) == 0) {
            codes[i] = D_800CCD88[arg0 * 0x170] * 4 + i;
            rowBase = rowOffset + arg0 * 0x690;
            damage[i] = *(u16 *)(D_800CEF10 + rowBase + 0x24);
        }
        rowOffset += 0x28;
    }
    g_BattleDisplayState->unkCB = 0;
    func_8008F8F4(0, 0x74, 0xA0, 0xAC, 0x38, 0, 1);
    func_8008F8F4(1, 0x84, 0x4C, 0x9C, 0x50, 0, 1);
    func_8008F8F4(2, 0x1C, 0xA0, 0x50, 0x18, 0, 1);
    func_800930AC(arg0, codes, damage);
    BattleInitPulseTrianglePrims();
    while (state == 2) {
        if (cursor != lastCursor) {
            BattleDrawCharStatusIfMaskUnset(arg0, cursor);
            lastCursor = cursor;
        }
        BattleStepAnimatedCursorPrim(0x8C, cursor * 0x10 + 0x58, &cursorFrame, &cursorTimer);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D328);
/* 96.12% (620 bytes against retail's 624). The address ASSOCIATION of the
   u16 |= store is the lever: written offset-chain first and gfx base last,
   `*((i + unkA3) * 0x28 + j * 0xFA0 + gfx + 0x6432)`, gcc emits retail's
   two-step `addu v0,v0,s1 / addu v0,a1,v0`; the base-first spelling of the old
   park let cse hoist `gfx + j*0xFA0` into one register and cost 2 instructions
   (93.85 with that body). Everything else is matched: both guards, the two SetX
   calls, the three byte stores and the loop tails.
   Left, both allocator/cse ties: retail re-forms `s1 + base` in the 0x6420
   load's beqz delay slot and reuses the chain (a0) for the following store, and
   the inner back-branch delay slot holds a nop where we steal `li a1,1`.
   The old body's byte-offset spellings are struct arithmetic against today's
   BattleWork header and do not compile; unkD0[j], unkA3 and unkCB are its
   fields. The count must stay a live re-read of the global in the
   for-condition - that is what makes the original reload it at the loop tail.
void func_8008D328(void) {
    u8 found;
    s32 j;
    s32 i;

    found = 0;
    for (j = 0; j < 2; j++) {
        if (g_BattleDisplayState->unkD0[j] != 0) {
            for (i = 0; i < g_BattleDisplayState->unkD0[j] * 2; i += 2) {
                u8 c;

                SetSemiTrans(g_BattleGraphicsWork + 0x641C + j * 0xFA0 + (i + g_BattleDisplayState->unkA3) * 0x28, 1);
                SetShadeTex(g_BattleGraphicsWork + 0x641C + j * 0xFA0 + (i + g_BattleDisplayState->unkA3) * 0x28, 0);
                *(u16 *)((i + g_BattleDisplayState->unkA3) * 0x28 + j * 0xFA0 + g_BattleGraphicsWork + 0x6432) |= 0x20;
                c = *(u8 *)(g_BattleGraphicsWork + j * 0xFA0 + 0x6420 + (i + g_BattleDisplayState->unkA3) * 0x28);
                if (c != 0) {
                    found = 1;
                    *(u8 *)(g_BattleGraphicsWork + j * 0xFA0 + 0x6420 + (i + g_BattleDisplayState->unkA3) * 0x28) = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                    *(u8 *)(g_BattleGraphicsWork + j * 0xFA0 + 0x6421 + (i + g_BattleDisplayState->unkA3) * 0x28) = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                    *(u8 *)(g_BattleGraphicsWork + j * 0xFA0 + 0x6422 + (i + g_BattleDisplayState->unkA3) * 0x28) = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                }
            }
        }
    }
    if (found == 0) {
        g_BattleDisplayState->unkCB = 0;
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
    u8 codes[4];
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
    for (i = 0; i < 4; i++) {
        pPrims = (POLY_FT4 *)p;
        BattleSetPrimGrey80BlendModeFromGlobal(&pPrims[i * 2 + g_BattlePrimStartIndex]);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008DE04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008E430);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008EA70);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F0A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F6E4);
/* 22.33% (816 bytes against retail's 528) with today's headers and externs: the
   57.38 this park used to claim is stale, measured when g_BattleDisplayState was
   a u8* - `g_BattleDisplayState[arg0 + 0xB0]` is a struct assignment now.
   What retail's .s shows, none of it in the body below:
   - the slot pointer g_BattleSlotGraphics[arg0] is loaded ONCE into a saved
     register (s2) and all eight prim stores PLUS the trailing `sb 0x5A4(s2)`
     go through it; re-indexing the global per store is what blows the body up;
   - g_BattlePrimStartIndex is re-read from the global before EVERY store
     (`lw` + sll/addu/sll for i*0x24), so the index must not be hoisted;
   - the four callee externs want u16 parameters: retail masks a1/a2/a3 with
     0xFFFF at each call site, and masks arg0 once at entry (so its first
     parameter is wider than u8, i.e. `s32 arg0` with a `u8 index` local).
   Next attempt: pointer local + u16 prototypes, `sh` stores through it.
void func_8008F6E4(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    u8 *p;

    ((u8 *)g_BattleDisplayState)[arg0 + 0xB0] = 0;
    *(u16 *)(g_BattleSlotGraphics[arg0] + g_BattlePrimStartIndex * 0x24 + 0x3C8) = arg1;
    *(u16 *)(g_BattleSlotGraphics[arg0] + g_BattlePrimStartIndex * 0x24 + 0x3CA) = arg2;
    *(u16 *)(g_BattleSlotGraphics[arg0] + g_BattlePrimStartIndex * 0x24 + 0x3D0) = arg1 + arg3;
    *(u16 *)(g_BattleSlotGraphics[arg0] + g_BattlePrimStartIndex * 0x24 + 0x3D2) = arg2;
    *(u16 *)(g_BattleSlotGraphics[arg0] + g_BattlePrimStartIndex * 0x24 + 0x3D8) = arg1;
    *(u16 *)(g_BattleSlotGraphics[arg0] + g_BattlePrimStartIndex * 0x24 + 0x3DA) = arg2 + arg4;
    *(u16 *)(g_BattleSlotGraphics[arg0] + g_BattlePrimStartIndex * 0x24 + 0x3E0) = arg1 + arg3;
    *(u16 *)(g_BattleSlotGraphics[arg0] + g_BattlePrimStartIndex * 0x24 + 0x3E2) = arg2 + arg4;
    BattleDrawSpriteQuad(arg0, arg1, arg2, arg3, arg4);
    func_8008DE04(arg0, arg1, arg2, arg3);
    func_8008E430(arg0, arg1, arg2, arg3, arg4);
    func_8008EA70(arg0, arg1, arg2, arg4);
    func_8008F0A8(arg0, arg1, arg2, arg3, arg4);
    *(u8 *)(g_BattleSlotGraphics[arg0] + 0x5A4) = *(u8 *)&g_BattlePrimStartIndex;
    ((u8 *)g_BattleDisplayState)[arg0 + 0xB0] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F8F4);
/* 96.33% (364 bytes = retail's size): the whole body matches; only the prologue
   save/copy order differs (gcc gives s4 to arg5 and s5 to arg1 and copies arg0
   last; the original has s4=arg1, s5=arg5 and copies arg0 first). Tried:
   separate pointer temps per block, a pFlags temp for g_BattleDisplayState (that
   load has to sit before the sh stores or gcc moves it after them),
   `*(base + index + K)` instead of `base[index + K]`, inverted if/else, two
   declaration orders, a live u16 copy of arg1 (does not raise its allocno
   priority) and two permuter runs. Pure allocno tie, no bytes to win.
   The three BattleWork byte accesses (+0xB0/+0xB8/+0xBF) go through
   `(u8 *)g_BattleDisplayState` because 0xB0..0xC5 is the header's unkA9 span:
   the old `g_BattleDisplayState + index + 0xB0` spelling is struct arithmetic
   against today's header and no longer compiles.

void func_8008F8F4(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4, u8 arg5, u8 arg6) {
    u8 *pFlags;
    u8 index;
    u8 *p;
    u8 *q;

    index = arg0;
    if (((u8 *)g_BattleDisplayState)[index + 0xB0] == 0) {
        q = BattleAllocHeapBlock(0x5A8, 0);
        g_BattleSlotGraphics[index] = q;
        bzero(q, 0x5A8);
        q = BattleAllocHeapBlock(0xE, 0);
        g_BattleSlotRects[index] = q;
        bzero(q, 0xE);
        BattleInitSlotPrims(index);
    }
    if (arg5 != 0) {
        p = g_BattleSlotRects[index];
        p[0xC] = arg0;
        pFlags = (u8 *)g_BattleDisplayState;
        ((u16 *)p)[0] = arg1;
        ((u16 *)p)[1] = arg2;
        ((u16 *)p)[2] = arg3;
        ((u16 *)p)[3] = arg4;
        ((u16 *)p)[4] = 0;
        ((u16 *)p)[5] = 0;
        *(pFlags + index + 0xBF) = 0;
        pFlags = (u8 *)g_BattleDisplayState;
        *(pFlags + index + 0xB8) = 1;
    } else {
        func_8008F6E4(index, arg1, arg2, arg3, arg4);
        if (arg6 != 0) {
            BattleUpdateFrame();
        }
    }
}
*/
