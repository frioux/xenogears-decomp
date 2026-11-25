#include "common.h"
#include "main/game.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

// Owned by include/system/menu.h, which this unit does not include.
#define MENU_ITEM_DESC_IMAGE_BUFFER_SIZE 0x618

// The text buffers BattleAllocTextBuffer hands out for one name string: the shared
// menu cell's (the same size is passed as the render length), and a command row's.
#define BATTLE_MENU_NAME_BUFFER_SIZE 57
#define BATTLE_COMMAND_ROW_NAME_BUFFER_SIZE 27

// Cells of the battle command grid, and the rows of item names the menu draws.
#define BATTLE_COMMAND_CELL_COUNT 48
#define BATTLE_COMMAND_ROW_COUNT 32
// The grid holds the two rows of a cell BATTLE_COMMAND_CELL_ROW_STEP cells apart.
#define BATTLE_COMMAND_CELL_ROW_STEP 16

#define BATTLE_CURSOR_STEP_FRAMES 3
#define BATTLE_CURSOR_FRAME_MAX 4
extern u8 g_BattleCommandGridIds[];
extern u8 g_BattleCommandCellUses[];
extern u8 D_800D2CC0[]; // the second 16-entry half of g_BattleCommandCellUses
extern u_long *g_BattleDigitImageBuffers[];
extern u_long *g_BattleDigitImageBuffersAlt[] __asm__("g_BattleDigitImageBuffers");
extern u_long *g_BattleTextWorkspace;
extern void BattleAllocPacketPrimitives(void);
extern void BattleLoadActionRowGraphics(void);
extern void *GetItemName(s32);
extern u8 *D_800D329C;
extern BattleGraphicsWork *g_BattleGraphicsWork;
extern s32 g_BattlePrimStartIndex[];
extern BattleWork *g_BattleDisplayState;
extern u8 *g_BattleSlotGraphics[];
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
extern void BattleBuildTileRow0Prims(u8, s32, s32, s32);
extern void BattleBuildTileRow1Prims(u8, s32, s32, s32, u16);
extern void BattleBuildTileRow2Prims(u8, s32, s32, s32);
extern void BattleBuildTileRow3Prims(u8, s32, s32, s32, u16);
extern void func_80090310(u8, u8);
extern void BattleDrawCommandCellIcons(u8, u8);
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

typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 targetW; // size the expand animation grows w to
    /* 0x6 */ u16 targetH; // size the expand animation grows h to
    /* 0x8 */ u16 w;
    /* 0xA */ u16 h;
    /* 0xC */ u8 slot;
    /* 0xD */ u8 padD;
} BattleSlotRect; // Size: 0xE

// The menu drawer's state bytes at the tail of the packet prim pool
// (BattleGraphicsWork.pPacketPrims: 0x668 bytes of POLY_FT4 pairs, then these).
// battle_work.h keeps the pool a u8* because other units append prims to it at
// byte offsets, so the view is cast on at each fetch.
typedef struct {
    /* 0x000 */ u8 unk0[0x66A];
    /* 0x66A */ u8 primStart; // prim index the menu cell prims were built at
    /* 0x66B */ u8 primsPending; // set once the cell prims are in the pool
    /* 0x66C */ u8 numberPrimStart; // prim index the number prims were built at
    /* 0x66D */ u8 labelDrawn; // set by the cell that drew its label string
    /* 0x66E */ u8 digitCount; // digit prims written into that bank
    /* 0x66F */ u8 numberPrimsPending; // set once the number prims are in the pool
} BattleMenuPrims; // Size: 0x670

// One row of the command table g_BattleCommandEntries; the flags word picks the icon
// the cell draws (battle/main/misc15 declares the same 0x10-byte row).
typedef struct {
    /* 0x0 */ u16 flags;
    /* 0x2 */ u8 pad2[0xE];
} BattleCommandEntry; // Size: 0x10
extern BattleCommandEntry g_BattleCommandEntries[];
// Per-row icon sprite coordinates and palette picker, indexed by the row times four.
extern u8 D_800D2F69[];
extern u8 D_800D2F6A[];
extern u8 D_800D2F6B[];
extern u8 g_BattleActionRowIconSprites[];



/* Retail reloads each clamp field inside the arm (the compare operands are dead across
   the branch); 2.6.0 does too, 2.7.2 CSEs them and cannot cross-jump the two stores.
   Load-bearing: the >= polarity, and the base-first spelling `*(pFlags + i + 0xB8)`
   (an array subscript emits the addu index-first, -0.25%). */
void BattleExpandSlotRects(void) {
    BattleSlotRect *p;
    u8 *pFlags;
    u8 count;
    u16 x;
    u16 y;
    s32 i;

    for (i = 0; i < 7; i++) {
        pFlags = (u8 *)g_BattleDisplayState;
        p = (BattleSlotRect *)g_BattleSlotRects[i];
        if (*(pFlags + i + 0xB8) == 0) {
            continue;
        }
        if (*(pFlags + i + 0xBF) != 0) {
            continue;
        }
        count = 0;
        if (p->w + 0x20 >= p->targetW) {
            p->w = p->targetW;
            count++;
        } else {
            p->w += 0x20;
        }
        if (p->h + 0x20 >= p->targetH) {
            p->h = p->targetH;
            count++;
        } else {
            p->h += 0x20;
        }
        if (count == 2) {
            *((u8 *)g_BattleDisplayState + i + 0xBF) = 1;
        }
        x = p->x + (p->targetW >> 1) - (p->w >> 1);
        y = p->y + (p->targetH >> 1) - (p->h >> 1);
        func_8008F6E4(p->slot, x, y, p->w, p->h);
    }
}

// ROUND 446 RE-MEASURE: 89.25% at retail's EXACT 452 bytes / 113 insns under the unit's gcc-2.6.0 (the 86.50 this note used to carry was measured under 2.7.2; 2.7.2 re-measures 86.50 now, -O3 89.25, -G0 and 2.7.2-cdk do not compile). The callee-saved SET is already retail's (s0-s5 + ra, frame 0x50) and every instruction of the body and epilogue is retail's; the whole residual is the ENTRY BLOCK's ORDER, 21 diff lines. Retail: lui/lw g_BattleDisplayState BEFORE 'addiu sp,sp,-0x50', then sw s1/move s1,zero (i=0), sw s3/move s3,a0, sw s4/move s4,a1, sw ra, sw s5, sw s2, sw s0, sw zero,0xFC(v0), lbu s2,0x60(sp)+nop (the load-delay slot), beqz s2 with 'move s5,a2' in its delay slot, and 'move s0,a3' AFTER the branch in the fall-through preheader. Ours: 'addiu sp' first, lbu s2 at the very top, then the lui/lw, then the a0/a1/a2/a3 copies, i=0 last, and 'sw zero,0xFC(v0)' pushed into the beqz delay slot. So retail's incoming-stack-arg load is NOT the BB_HEAD insertion that global-alloc makes (that is why it keeps a load-delay nop and sits after the store) - it looks like a reload at its use, and our body makes 2.6.0 hoist it every time. PROBED, not guessed: a cc1_probe of this exact body shape (probe_fc1c.c: one global store, then the same 4-arg loop with a s32 arg3 offset) reproduces OUR entry order under 2.6.0 AND under 2.7.2 AND under 2.7.2-cdk (all three emit 'subu sp; lbu; lw g_State; sw/move pairs'), so no compiler in the repo turns this body into retail's entry. Measured dead this round: 's32 i = 0;'/statement order for i, a 'u8 count; count = arg4;' local (88.84, 456 bytes - adds 'move s2,v1'), 'arg3 += 8' in the for-increment instead of the offset local (89.25 identical).
// ROUND 471 RE-MEASURE: 89.25% again, retail's exact 452 bytes / 113 insns, compiles on the first run under the unit's gcc-2.6.0 (XenoBattle260 - NOT the cdk preset this round's brief named).
// Two new facts measured this round, both narrowing the entry block further.
// (1) IT IS NOT SCHED1: the built build/src/battle/main/misc16_r2.c.i recompiled with the unit's cc1 plus -fno-schedule-insns emits the SAME entry order (subu / sw s2 / lbu s2,0x60(sp) / lw g_BattleDisplayState / copies / i=0 / beq with the store in the delay slot), so the residual is in the RTL stream, as the round-446 note concluded.
// (2) RETAIL'S HOISTED 'lui/lw' ABOVE THE FRAME ADJUST IS REACHABLE IN THIS cc1 - the matched neighbour BattleDrawColumnSpriteCells emits 'lw $2,g_BattleDisplayState' BEFORE 'subu $sp,$sp,32' in this very compile - and the difference between the two is that BattleDrawColumnSpriteCells' entry block has NO incoming-stack-argument load. So the whole residual (the hoisted state load, the store before the lbu, the load-delay nop) hangs on cc1's assign_parms placing the 5th parameter's 'lbu 0x60(sp)' at BB_HEAD, which is exactly what the j-the-callers-single-sw lever below rules out as an aggregate.
/*
void func_8008FC1C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u8 arg4) {
    s32 i;
    s32 offset;
    s32 size;

    g_BattleDisplayState->menuColumnPackageCount = 0;
    i = 0;
    for (offset = arg3; i < arg4; i++) {
        size = BattleDrawSpritePackage(0x65, (u8 *)g_BattleGraphicsWork + (g_BattleDisplayState->menuColumnPackageCount * 0x50 + 0x1E68), arg0, offset);
        g_BattleDisplayState->menuColumnPackageCount = size + g_BattleDisplayState->menuColumnPackageCount;
        offset += 8;
    }
    size = BattleDrawSpritePackage(0x64, (u8 *)g_BattleGraphicsWork + (g_BattleDisplayState->menuColumnPackageCount * 0x50 + 0x1E68), arg0, arg1);
    g_BattleDisplayState->menuColumnPackageCount = size + g_BattleDisplayState->menuColumnPackageCount;
    size = func_800263E4(g_BattleSpriteArchive, 0x64,
        (u8 *)g_BattleGraphicsWork + (g_BattleDisplayState->menuColumnPackageCount * 0x50 + 0x1E68),
        g_BattlePrimStartIndex[0], (s16)arg0, (s16)arg2, 0x1000, 0, 1);
    g_BattleDisplayState->menuColumnPackageCount = size + g_BattleDisplayState->menuColumnPackageCount;
    g_BattleDisplayState->menuColumnPrimStart = ((u8 *)g_BattlePrimStartIndex)[0];
    g_BattleDisplayState->menuColumnPending = 1;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FC1C);


void BattleInitMenuColumnPrims(void) {
    func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
}

// The two load-bearing spellings, both measured: the digit table is named through two
// gcc objects (an `__asm__`-labelled alias) so loop.c has no loop-invariant table base to
// hoist, and the row store uses an embedded assignment so `bufs + i` forms base-first.
// unusedFrame[8] is the 8 bytes of local retail's frame has at 0xC8 (never read or written).
void BattleBuildCommandGridText(s32 arg0, s32 arg1, u8 arg2) {
    void *bufs[BATTLE_COMMAND_ROW_COUNT];
    RECT nameRect;
    RECT tensRect;
    RECT onesRect;
    RECT tens2Rect;
    RECT ones2Rect;
    RECT blankRect;
    u8 unusedFrame[8];
    u8 cellUses[BATTLE_COMMAND_CELL_COUNT];
    u8 cellIds[BATTLE_COMMAND_CELL_COUNT];
    void *pBuf;
    void **pSlot;
    u8 tens;
    s32 i;

    if (arg2 != 0) {
        BattleAllocPacketPrimitives();
        BattleLoadActionRowGraphics();
        func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
    }
    pBuf = BattleAllocTextBuffer(BATTLE_MENU_NAME_BUFFER_SIZE);
    g_BattleTextWorkspace = pBuf;
    bzero(pBuf, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
    blankRect.x = 0x3C0;
    blankRect.y = 0;
    blankRect.w = 0x3C;
    blankRect.h = 0xD;
    BattleLoadImageAndSync(&blankRect, g_BattleTextWorkspace);
    for (i = 0; i < BATTLE_COMMAND_CELL_COUNT; i++) {
        cellIds[i] = g_BattleCommandGridIds[i];
        cellUses[i] = g_BattleCommandCellUses[i];
    }
    for (i = 0; i < BATTLE_COMMAND_ROW_COUNT; i++) {
        pBuf = BattleAllocTextBuffer(BATTLE_COMMAND_ROW_NAME_BUFFER_SIZE);
        // The embedded assignment is what makes cc1 form `bufs + i` base-first
        // (`addu s0,v0,v1`) after the call instead of index-first before it.
        *(pSlot = bufs + i) = pBuf;
        bzero(pBuf, 0x30C);
        nameRect.x = 0x380 + (i % 2) * 0x1E;
        nameRect.y = 0x100 + (i / 2) * 0xD;
        nameRect.w = 0x1E;
        nameRect.h = 0xD;
        tensRect.x = 0x3C0 + (i % 2) * 0x10;
        tensRect.y = 0x100 + (i / 2) * 0xD;
        tensRect.w = 6;
        tensRect.h = 0xD;
        onesRect.x = 0x3C2 + (i % 2) * 0x10;
        onesRect.y = 0x100 + (i / 2) * 0xD;
        onesRect.w = 6;
        onesRect.h = 0xD;
        tens2Rect.x = 0x3C4 + (i % 2) * 0x10;
        tens2Rect.y = 0x100 + (i / 2) * 0xD;
        tens2Rect.w = 6;
        tens2Rect.h = 0xD;
        ones2Rect.x = 0x3C6 + (i % 2) * 0x10;
        ones2Rect.y = 0x100 + (i / 2) * 0xD;
        ones2Rect.w = 6;
        ones2Rect.h = 0xD;
        if (cellIds[i] != 0) {
            SystemRenderStringEntry(GetItemName(cellIds[i]), bufs[i], BATTLE_COMMAND_ROW_NAME_BUFFER_SIZE, 0);
        }
        if (cellIds[i + BATTLE_COMMAND_CELL_ROW_STEP] != 0) {
            SystemRenderStringEntry(GetItemName(cellIds[i + BATTLE_COMMAND_CELL_ROW_STEP]), bufs[i], BATTLE_COMMAND_ROW_NAME_BUFFER_SIZE, 1);
        }
        BattleLoadImageAndSync(&nameRect, bufs[i]);
        tens = cellUses[i] / 10;
        if (tens != 0) {
            BattleLoadImageAndSync(&tensRect, g_BattleDigitImageBuffersAlt[tens]);
        } else {
            BattleLoadImageAndSync(&tensRect, g_BattleTextWorkspace);
        }
        if (cellIds[i] != 0) {
            BattleLoadImageAndSync(&onesRect, g_BattleDigitImageBuffers[cellUses[i] % 10]);
        } else {
            BattleLoadImageAndSync(&onesRect, g_BattleTextWorkspace);
        }
        if (D_800D2CC0[i] / 10 != 0) {
            BattleLoadImageAndSync(&tens2Rect, g_BattleDigitImageBuffersAlt[cellUses[i + BATTLE_COMMAND_CELL_ROW_STEP] / 10]);
        } else {
            BattleLoadImageAndSync(&tens2Rect, g_BattleTextWorkspace);
        }
        if (cellIds[i + BATTLE_COMMAND_CELL_ROW_STEP] != 0) {
            BattleLoadImageAndSync(&ones2Rect, g_BattleDigitImageBuffers[cellUses[i + BATTLE_COMMAND_CELL_ROW_STEP] % 10]);
        } else {
            BattleLoadImageAndSync(&ones2Rect, g_BattleTextWorkspace);
        }
    }
    for (i = 0; i < BATTLE_COMMAND_ROW_COUNT; i++) {
        HeapFree(bufs[i]);
    }
    HeapFree(g_BattleTextWorkspace);
    BattleUpdateFrame();
    // The prim pool's column-bank flag byte; battle/main/misc17 writes this same
    // 0x668/0x669 pair with the raw index, so the pool stays a u8* here.
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->primState = 2;
}



void BattleDrawColumnSpriteCells(s32 arg0) {
    s32 i;
    s32 offset;
    s32 ofs;
    s32 size;
    s32 sum;

    g_BattleDisplayState->spriteColumnPackageCount = 0;
    i = 0;
    offset = 0x68;
next:
    sum = arg0 - 4;
    ofs = g_BattleDisplayState->spriteColumnPackageCount * 0x50;
    ofs += 0xBA8;
    size = BattleDrawSpritePackage(0x66, ofs + (s32)g_BattleGraphicsWork, 0x20, offset + sum);
    g_BattleDisplayState->spriteColumnPackageCount = size + g_BattleDisplayState->spriteColumnPackageCount;
    offset += 8;
    i++;
    if (i < 9) {
        goto next;
    }
    g_BattleDisplayState->spriteColumnPrimStart = *(u8 *)&g_BattlePrimStartIndex;
    g_BattleDisplayState->spriteColumnPending = 1;
}/* 81.85% (400 bytes; retail 0x188 = 100 insns). The array-extern
   `extern s32 g_BattlePrimStartIndex[]` + `g_BattlePrimStartIndex[0]` gave retail's
   `lui s2,%hi / addiu s2,s2,%lo / lw v1,0(s2)` (74.61 -> 81.85); the base is
   `*(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230)`, which reproduces retail's
   `lui at,0x10000 / addu at / lw -0x5DD0(at)` - the earlier `g_BattleGraphicsWork + 0xA230`
   spelling scaled by sizeof(BattleGraphicsWork) and never compiled.
   Remaining: retail keeps index in a1 and reuses s1 for index % 2, ours puts index in s1
   and reuses it for the modulo; retail sets `li a1,0x18 / li a2,0x33 / andi a3,0xf8`
   before it loads the packet base, ours after; and the 0x140 must be added to the
   prim-index product (`sll/addu/sll; addiu a0,a0,0x140`, base in the jal delay slot) where
   gcc folds it onto the base (`addiu v0,t0,0x140; addu a0,a0,v0`). An `ofs` local for
   `prim * 0x28 + 0x140` before `ofs + (s32)pBase` drops it to 65.93.
   Two shapes are load-bearing: the palette store duplicated into BOTH if-arms (gcc
   cross-jumps the shared tail into retail's `j .L80090398`) and the `& 0xF8` / `& 0xD0`
   masks on the u arguments (retail's `andi a3,a3,0xf8` / `andi a3,s1,0xd0`; a plain u8
   parameter gives `andi 0xff`).
*/
/* PARKED 98.70% at retail's exact size (400 bytes / 100 insns) under the unit's gcc-2.6.0.
   Structure, size, instruction multiset, the u8 masks, the duplicated palette store in both
   arms and the two inline `(POLY_FT4 *)(*(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230)) +
   (g_BattlePrimStartIndex[0] + 8 / + 10)` call addresses are all retail's (round 406).
   Residual, ONE cause: a local-alloc tie inside the palette block. Retail loads prim into $v0
   and its prim*5 product into $v1, then reuses $v0 for the pool base, so the join reads
   `sll v1,v1,3`; this build mirrors the pair (prim $v1, product $v0, base $v1). ROUND 419:
   a 3-minute permuter run from THIS body found nothing (1618 iterations, base=130 best=130),
   and an initialised `s32 prim = g_BattlePrimStartIndex[0];` declaration at the top of each
   arm is no change at all. The round-406 dead ends (ofs local for the prim product, POLY_FT4
   pointer + subscript, a do-while tie mover, statement and declaration order, volatile) stand.
*/
/* PARKED 98.70% (400 bytes / 100 insns = retail's exact size), unit gcc-2.6.0 -O2 -G8.
   Residual is ONE mirrored pair: retail loads prim into v0 and its prim*5 into v1 and then reuses v0
   for the pool base; this build mirrors them (prim v1, product v0, base v1), so 30 diff lines are
   register names only. ROUND 432 -dl on the live body: the two qtys are 'Register 89 used 2 times
   across 3 insns in block 1' and 'Register 90 used 2 times across 3 insns in block 1' (and 99/100 in
   block 2), i.e. IDENTICAL priority (floor_log2(2)*2/3 both), so local-alloc's hand-out is a genuine
   tie broken by qty-scan order - neither qty can be given a distinguishing ref/live count without
   changing retail's instruction stream, which is why the 1618-iteration permuter run (round 419,
   base=130 best=130) found nothing. Also dead this round: a u16 pal local assigned before the store in
   each arm (85.09%, 94 insns - the arms stop being cross-jump-mergeable), and a per-arm
   'ofs = g_BattlePrimStartIndex[0] * 0x28' local (91.35%, 104 insns). Earlier dead ends stand: an ofs
   local over the whole expression (65.93), POLY_FT4 pointer + subscript, s32 prim = ...[0] initialisers,
   statement/declaration order, volatile; 2.7.2-cdk compile-fails this body.
*/
// ROUND 446 re-measured 98.70% (400 bytes / 100 insns = retail's exact size, unit gcc-2.6.0). Residual unchanged: ONE mirrored caller-saved pair in the palette block (retail prim=$v0, prim*5=$v1, pool base reuses $v0 and the join reads 'sll v1,v1,3'; this build prim=$v1, product=$v0, base $v1). NEW this round, and KEPT because it is the realistic spelling and measures identically: the store is the prim's clut - '((POLY_FT4 *)g_BattleGraphicsWork->pPacketPrims)[g_BattlePrimStartIndex[0] + 8].clut' (the 0x140 folds into the 0x14E displacement exactly as retail's 'sh a0,334(v0)' shows), and both call bases are 'g_BattleGraphicsWork->pPacketPrims' instead of the '(u8 *)g_BattleGraphicsWork + 0xA230' cast. -dl/-da on this body: the palette block's three qtys are 72 (4 refs/6 insns, ->$v0), 80 (4/4, ->$v1), 86 (4/3, ->$v0), so the contested pair is the prim load (80) against the product; local-alloc hands v0 to the product, retail to the prim. Nothing in the C moves it: 3 prior rounds + a 1618-iteration permuter (base=130 best=130) are on the earlier note, and the store's u8*-plus-0x14E spelling (the pre-round-446 body) measures the same 98.70.
/*
void func_80090310(u8 arg0, u8 arg1) {
    s32 index;
    s32 vofs;
    s32 u;
    s32 v;

    index = arg1 * 2 + arg0;
    vofs = 0;
    if (index >= 0x11) {
        index -= 0x10;
        vofs = 0x10;
        ((POLY_FT4 *)g_BattleGraphicsWork->pPacketPrims)[g_BattlePrimStartIndex[0] + 8].clut = g_SystemPalette2;
    } else {
        ((POLY_FT4 *)g_BattleGraphicsWork->pPacketPrims)[g_BattlePrimStartIndex[0] + 8].clut = g_SystemPalette1;
    }
    v = index / 2 * 13;
    u = index % 2 * 0x78;
    BattleSetPolyFT4RectH13((POLY_FT4 *)g_BattleGraphicsWork->pPacketPrims + (g_BattlePrimStartIndex[0] + 8), 0x18, 0x33, u & 0xF8, v & 0xFF, 0x60);
    BattleSetPolyFT4RectH13((POLY_FT4 *)g_BattleGraphicsWork->pPacketPrims + (g_BattlePrimStartIndex[0] + 10), 0x84, 0x33, (index % 2 << 6 | vofs) & 0xD0, v & 0xFF, 0x10);
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090310);



void BattleDrawCommandCellIcons(u8 arg0, u8 arg1) {
    u8 *pRec;
    u16 flags;
    u16 pal;
    s32 row;
    u8 col;

    flags = g_BattleCommandEntries[g_BattleCommandGridIds[arg1 * 2 + arg0]].flags;
    if (flags & 0x4000) {
        row = 9;
    } else if (flags & 0x1000) {
        row = 7;
    } else {
        row = 8;
    }
    switch (flags & 0xF) {
    case 0:
        col = 0xC;
        break;
    case 1:
        col = 0xD;
        break;
    case 2:
        col = 0x15;
        break;
    }
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                        (g_BattlePrimStartIndex[0] * 0x28 + 0x320)),
                            0xA8, 0x33, D_800D2F6A[row * 4], D_800D2F6B[row * 4],
                            g_BattleActionRowIconSprites[row * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims - (-(g_BattlePrimStartIndex[0] * 0x28));
    if (D_800D2F69[row * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims +
                                        (g_BattlePrimStartIndex[0] * 0x28 + 0x370)),
                            0xD0, 0x33, D_800D2F6A[col * 4], D_800D2F6B[col * 4],
                            g_BattleActionRowIconSprites[col * 4]);
    pRec = g_BattleGraphicsWork->pPacketPrims - (-(g_BattlePrimStartIndex[0] * 0x28));
    if (D_800D2F69[col * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}


void BattleDrawMenuEntryString(u8 arg0, u8 arg1) {
    RECT rect;
    u8 *pBuf;
    u8 *pBase;
    s32 index;
    s32 len;
    s32 ofs;
    s32 palIndex;

    index = g_BattleCommandGridIds[arg1 * 2 + arg0];
    pBuf = (u8 *)BattleAllocTextBuffer(BATTLE_MENU_NAME_BUFFER_SIZE);
    bzero(pBuf, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
    len = SystemRenderStringEntry(GetStringEntry(D_800D329C, index), pBuf, BATTLE_MENU_NAME_BUFFER_SIZE, 0);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    BattleLoadImageAndSync(&rect, pBuf);
    pBase = g_BattleGraphicsWork->pPacketPrims;
    palIndex = g_BattlePrimStartIndex[0];
    ofs = palIndex * 0x28 + 0x280;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x30, 0x42, 0, 0, len & 0xFF);
    HeapFree(pBuf);
}


void BattleDrawMenuCell(u8 arg0, u8 arg1, u8 arg2) {
    u8 flag;

    if (arg2 == 0) {
        flag = g_BattleCommandGridIds[arg1 * 2 + arg0];
    } else {
        flag = g_BattleCommandGridAltIds[arg1 * 2 + arg0];
    }
    ((BattleMenuPrims *)g_BattleGraphicsWork->pPacketPrims)->labelDrawn = 0;
    func_80090310(arg0, arg1);
    if (flag != 0 && arg2 == 0) {
        BattleDrawMenuEntryString(arg0, arg1);
        BattleDrawCommandCellIcons(arg0, arg1);
        ((BattleMenuPrims *)g_BattleGraphicsWork->pPacketPrims)->labelDrawn = 1;
    }
    ((BattleMenuPrims *)g_BattleGraphicsWork->pPacketPrims)->primStart = ((u8 *)g_BattlePrimStartIndex)[0];
    ((BattleMenuPrims *)g_BattleGraphicsWork->pPacketPrims)->primsPending = 1;
}
// PARKED 68.62% (544 bytes / 136 insns; retail's 0x254 = 596 / 149). Re-measured from
// scratch under the unit's gcc-2.6.0; the old 36.86 body was measured under 2.7.2 and also
// multiplied the `s32 g_BattlePrimStartIndex[]` array by 0x28, which cannot compile.
// What matches: the slti 0x68 / -0x68 / vofs 0x10 split, `pal` live in a1 across the join
// from either palette symbol, the (primIdx*0x28 + K) strides, all four BattleSetPolyFT4Rect
// argument sets (u 0 / 0x78 / vofs / vofs|0x40, v = index narrowed to u8 once, dx 0x60 and
// 0x10, dy 0x68) and the trailing lbu/sb of the prim index at +0x668.
// THE WHOLE 13-INSTRUCTION GAP IS ONE BLOCK: retail's first palette store sits inside BOTH
// if-arms (the store at +0xE duplicated, ~12 instructions each, each arm also re-forming
// `lui/addiu &g_BattlePrimStartIndex`), while gcc 2.6.0 cross-jumps my two identical arm
// tails into the join and drops the copy - the same cross-jump func_80090310's note above
// hit. To match, the arms have to differ in pseudo-registers; tried and still merged: the
// store written twice from the inline expression, and a per-arm `u16 *pSlot = ...;
// *pSlot = pal;` local. The body is 13 instructions short, so no amount of register work
// can finish it - the duplicated arm tail is the target.
// Tried and dead: `s32 *pPrim = g_BattlePrimStartIndex` + `*pPrim` at every site (532
// bytes, 68.22, and no lui/addiu form, unlike BattleDrawCommandCellIcons), `vofs = 0` hoisted above the
// if (identical 544), gcc-2.7.2 (56.88), gcc-2.7.2-cdk (compile-failed).
// ROUND 427 RE-MEASURE: still 68.62% (544 bytes / 136 insns; retail 0x254 = 596 / 149) with
// the body below unchanged, so the note above stands. Measured and dead this round: the
// single store after the if with 'vofs = 0' hoisted to the top (56.14%, 132 insns - it
// loses retail's duplicated arm), gcc-2.6.0 -O1 (43.08%) and -O3 (68.62%, identical to
// -O2). The 13-instruction hole is retail's duplicated palette store, which this cc1
// always cross-jumps into the join.
/*
void func_8009093C(s32 arg0) {
    s32 index;
    s32 vofs;
    u16 pal;

    index = arg0;
    if (index >= 0x68) {
        index -= 0x68;
        vofs = 0x10;
        pal = g_SystemPalette2;
        *(u16 *)(g_BattleGraphicsWork->pPacketPrims + g_BattlePrimStartIndex[0] * 0x28 + 0xE) = pal;
    } else {
        vofs = 0;
        pal = g_SystemPalette1;
        *(u16 *)(g_BattleGraphicsWork->pPacketPrims + g_BattlePrimStartIndex[0] * 0x28 + 0xE) = pal;
    }
    *(u16 *)(g_BattleGraphicsWork->pPacketPrims + g_BattlePrimStartIndex[0] * 0x28 + 0x5E) = pal;
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + g_BattlePrimStartIndex[0] * 0x28), 0x30, 0x60, 0, index, 0x60, 0x68);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + g_BattlePrimStartIndex[0] * 0x28 + 0x50), 0xB4, 0x60, 0x78, index, 0x60, 0x68);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + g_BattlePrimStartIndex[0] * 0x28 + 0xA0), 0x98, 0x60, vofs, index, 0x10, 0x68);
    BattleSetPolyFT4Rect((POLY_FT4 *)(g_BattleGraphicsWork->pPacketPrims + g_BattlePrimStartIndex[0] * 0x28 + 0xF0), 0x11C, 0x60, vofs | 0x40, index, 0x10, 0x68);
    g_BattleGraphicsWork->pPacketPrims[0x668] = g_BattlePrimStartIndex[0];
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8009093C);


void BattleStepAnimatedCursorPrim(s32 arg0, s32 arg1, s32 *pIndex, u8 *pCount) {
    if (++*pCount >= BATTLE_CURSOR_STEP_FRAMES) {
        if (--*pIndex < 0) {
            *pIndex = BATTLE_CURSOR_FRAME_MAX;
        }
        *pCount = 0;
    }
    // The typed field (cursorPackageCount) makes cc1 hoist the prim-index load
    // above this store, which costs retail's load-delay nop (180 bytes -> 172).
    *(s32 *)((u8 *)g_BattleDisplayState + 0x100) =
        BattleDrawSpritePackage(*pIndex + 0xE0, (u8 *)g_BattleGraphicsWork + 0x27C8, arg0, arg1);
    g_BattleDisplayState->cursorPrimStart = ((u8 *)g_BattlePrimStartIndex)[0];
    *(volatile u8 *)&g_BattleDisplayState->cursorPending = 1;
}
// PARKED 77.70% (592 bytes / 148 insns; retail 0x238 = 568 / 142). Re-measured under
// the unit's gcc-2.6.0 - the older 72.46 note here was measured under 2.7.2, before the
// round-371 preset switch. Every block's shape and the
// 'lui at,%hi(D_800CCD38); addu at,at,s0; lhu %lo(at)' address form are retail's, and the
// divide/modulo magic, the four 0x28-stride prim bases and the argument order all match.
// The residual is 6 excess instructions from ONE defect: gcc keeps a redundant second copy
// of 'eo' in s4 ('move s4,s0' in block 1's beqz delay slot, 'addu at,at,s4' in block 4,
// plus its save/restore). The other two are block 3/4 masking '& 0xF8' twice ('li s0,-8'
// then 'and') when the u argument is the u8 conversion of an inline mask.
// BEST ALTERNATIVE, measured 76.39% / 580 bytes / 145 insns: hoist 's32 u;
// u = (d * 8 + 0x78) & 0xF8;' before each call. That drops both extra masks - only the s4
// copy is left - and blocks 1 and 3 then read 'sll a3,v0,3 / addiu a3,a3,120 / andi
// a3,a3,0xf8' exactly as retail. It scores 1.3 lower on diff alignment alone, so start any
// further attempt from that form.
// Tried and dead: the 2.7.2 body (72.46), distinct v/d/u locals per block (76.54, 145
// insns, copy unchanged), 'u32 eo' (identical), cdk preset (compile-failed).
// ROUND 412: the double mask IS reachable and the lever is (u16) on the sum - writing
// `(u16)(d * 8 + 0x78) & 0xF8` at all four sites drops the surplus `li -8` + `and` pair
// and takes the body to 145 insns / 580 bytes / 76.39% (from 148 / 592 / 77.70). It does
// NOT drop the `move s4,s0` copy of eo, so 12 instructions of surplus remain in the
// block-2..4 address chains (`lw t0,-24016(at)`/`addu a0,a0,t0` where retail orders the
// pool load after the `sll` chain and adds through v0/v1). A cc1_probe of the four blocks
// alone reproduces the same extra mask, so it is cc1's, not the surrounding body's.
// ROUND 427 RE-MEASURE, new best 78.83% (580 bytes / 145 insns; retail 0x238 = 568 / 142),
// up from the 77.70 park. GAINED: the round-412 lever applied to blocks 3 and 4 ONLY -
// '(u16)(d * 8 + 0x78) & 0xF8' at the 0x11C and 0x124 sites - removes the surplus
// 'li s0,-8' + 'and' pair there (the other two sites already had a single andi).
// Residual, 3 instructions = 3.0%: gcc keeps a second copy of 'eo' in s4
// ('move s4,s0' in block 1's beqz delay slot) and block 4 reads D_800CCD3A(s4) while
// blocks 1-3 read (s0); with it come sw/lw s4 at 0x28/0x40. Retail keeps eo in s0 alone.
// Tried this round and dead: block 1's single store after the if with 'vofs = 0' hoisted
// (56.14 on the sibling, not retried here), -O1 (43.08 on the sibling).
/*
void func_80090C44(u8 arg0) {
    u8 *pBase;
    s32 eo;
    s32 ofs;
    u16 v;
    u16 d;

    eo = arg0 * 0x170;
    v = *(u16 *)(D_800CCD38 + eo);
    d = v / 10;
    if (d != 0) {
        pBase = g_BattleGraphicsWork->pPacketPrims;
        ofs = g_BattlePrimStartIndex[0] * 0x28 + 0x460;
        BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x104, 0xC6, (d * 8 + 0x78) & 0xF8, 0, 8);
    }
    v = *(u16 *)(D_800CCD38 + eo);
    d = v % 10;
    pBase = g_BattleGraphicsWork->pPacketPrims;
    ofs = g_BattlePrimStartIndex[0] * 0x28 + 0x4B0;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x10C, 0xC6, (d * 8 + 0x78) & 0xF8, 0, 8);
    v = *(u16 *)(D_800CCD3A + eo);
    d = v / 10;
    if (d != 0) {
        pBase = g_BattleGraphicsWork->pPacketPrims;
        ofs = g_BattlePrimStartIndex[0] * 0x28 + 0x500;
        BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x11C, 0xC6, (u16)(d * 8 + 0x78) & 0xF8, 0, 8);
    }
    v = *(u16 *)(D_800CCD3A + eo);
    d = v % 10;
    pBase = g_BattleGraphicsWork->pPacketPrims;
    ofs = g_BattlePrimStartIndex[0] * 0x28 + 0x550;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x124, 0xC6, (u16)(d * 8 + 0x78) & 0xF8, 0, 8);
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090C44);



INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090E7C);
/* 83.46% (488 bytes = retail's exact 122 instructions). Both draw blocks and both
   ROUND 371: this body had NEVER COMPILED either - `s32 *pPrim = &g_BattlePrimStartIndex`
   on the unit's `s32 g_BattlePrimStartIndex[]` declaration is a pointer to array, so
   `*pPrim` was an array rather than the prim index. The two locals now read
   `pPrim = g_BattlePrimStartIndex;` / `pPrim2 = g_BattlePrimStartIndex;` and the four
   g_BattleGraphicsWork bases are `(u8 *)g_BattleGraphicsWork + 0xA230`; the 83.46 below
   has NOT been re-measured against that spelling.
   palette picks are retail's; the palette blocks and the tail (func_80090C44 in the
   store's delay slot, the trailing `lbu g_BattlePrimStartIndex`) are byte-identical.
   Three levers are proven here and any further attempt should start from them:
   - `ofs` must be its own local split over two statements (`ofs = *pPrim * 0x28;`
     then `ofs += 0x3C0;`): the one-statement form reassociates the constant onto the
     base (76.12%).
   - `pPrims = g_BattleGraphicsWork->pPacketPrims;` as a statement BETWEEN the product
     and the `+ 0x3C0` gives retail's `addiu a0,a0,960 / addu a0,a0,t1` (with the
     member inlined instead, gcc puts the loaded base in a0 and the offset in t0).
   - a SECOND pointer local (`pPrim2`) assigned after the first palette store for the
     second half is what makes gcc re-form `lui s0,%hi / addiu s0,%lo` at the branch
     join, retail's single 2-instruction re-materialisation and the last 8 bytes of
     the function's size (82.24 -> 83.46, 480 -> 488). One `pPrim` for all four sites
     loses them.
   `pal = D_800D2F80;` above the first call is a scratch for the dx argument (the
   permuter's find, 1155 -> 83.46); retail keeps it in v1, this build in t0.
   ROUND 312: passing D_800D2F80 inline as the dx argument instead of through that
   scratch is 82.24, and adding `u = D_800D2F82;` before it (`u` passed as the u
   argument) is exactly 83.46 again - so the scratch is load-bearing and the residual
   is NOT the u/dx/v load order on its own: retail loads them u, dx, v after
   `lw t0,0(s0)` and this build dx, u, v with dx before the sll chain.
   Remaining: retail fills the entry block's first slots with `lui v0,%hi(
   g_BattleGraphicsWork); lw v0,0(v0)` between `li a1,0xA8` and `li a2,0xA6` and
   loads D_800D2F80 before the sll chain; this build emits both after it. A short
   `s16 ofs` reproduces that entry order exactly (95.74%) but adds a sll/sra
   conversion pair per site (504 bytes), i.e. ofs is not narrow in the original.
   Not tried: a distinct RTL spelling for the pPrims member access, and a second
   permuter run from the 83.46 body.
void func_80090E7C(u8 arg0) {
    u8 *pRec;
    u8 *pPrims;
    s32 *pPrim;
    s32 *pPrim2;
    s32 ofs;
    u16 pal;

    pPrim = g_BattlePrimStartIndex;
    pPrims = g_BattleGraphicsWork->pPacketPrims;
    ofs = *pPrim * 0x28;
    pal = D_800D2F80;
    ofs += 0x3C0;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pPrims), 0xA8, 0xA6, D_800D2F82, D_800D2F83, pal);
    pRec = g_BattleGraphicsWork->pPacketPrims + *pPrim * 0x28;
    if (D_800D2F81 != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x3CE) = pal;
    pPrim2 = g_BattlePrimStartIndex;
    ofs = *pPrim2 * 0x28;
    pPrims = g_BattleGraphicsWork->pPacketPrims;
    ofs += 0x410;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pPrims), 0xEC, 0xC6, D_800D2F7E, D_800D2F7F, D_800D2F7C);
    pRec = g_BattleGraphicsWork->pPacketPrims + *pPrim2 * 0x28;
    if (D_800D2F7D != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x41E) = pal;
    func_80090C44(arg0);
    BattleDrawSpritePackage(0x71, (s32)(g_BattleGraphicsWork->pPacketPrims + 0x5A0), 0x118, 0xD1);
    ((BattleMenuPrims *)g_BattleGraphicsWork->pPacketPrims)->numberPrimStart = ((u8 *)g_BattlePrimStartIndex)[0];
}
*/
