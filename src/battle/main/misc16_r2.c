#include "common.h"
#include "main/game.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

// Owned by include/system/menu.h, which this unit does not include.
#define MENU_ITEM_DESC_IMAGE_BUFFER_SIZE 0x618

// Cells of the battle command grid, and the rows of item names the menu draws.
#define BATTLE_COMMAND_CELL_COUNT 48
#define BATTLE_COMMAND_ROW_COUNT 32
// The grid holds the two rows of a cell BATTLE_COMMAND_CELL_ROW_STEP cells apart.
#define BATTLE_COMMAND_CELL_ROW_STEP 16

#define BATTLE_CURSOR_STEP_FRAMES 3
#define BATTLE_CURSOR_FRAME_MAX 4
extern u8 g_BattleCommandGridIds[];
extern u8 g_BattleCommandCellUses[];
extern u_long *g_BattleDigitImageBuffers[];
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
extern void func_8008DE04(u8, s32, s32, s32);
extern void func_8008E430(u8, s32, s32, s32, u16);
extern void BattleBuildTileRow2Prims(u8, s32, s32, s32);
extern void func_8008F0A8(u8, s32, s32, s32, u16);
extern void func_80090310(u8, u8);
extern void func_800904A0(u8, u8);
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

/* 86.50%: instruction-for-instruction identical to retail except one callee-saved
   register too many (s0-s6 here against retail's s0-s5), which shifts every register in
   the body by one. gcc cse keeps `arg0 << 16` alive in s3 across both calls for the two
   `(s16)arg0` conversions, where retail re-extends at each site
   (`sll a2,s3,0x10; sra a2,a2,0x10` inside the loop, `sll s0,s3,0x10` after it), and it
   then cannot reuse s1 for `&g_BattlePrimStartIndex` the way retail does after the loop.
   The array-extern `g_BattlePrimStartIndex[]` (with `[0]` for the word, `((u8 *)...)[0]`
   for the byte) and the `(u8 *)g_BattleGraphicsWork + (count * 0x50 + 0x1E68)` base both
   reproduce retail's shapes; the earlier note's `g_BattleGraphicsWork + 0x1E68` scaled by
   sizeof(BattleGraphicsWork) and never compiled.
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
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FC1C);
void BattleInitMenuColumnPrims(void) {
    func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FE18);
/* 95.58% at retail's EXACT size (1060 bytes = 0x424). Every call, rectangle, store and
   ROUND 371: re-measured, and confirmed to COMPILE; 96.18 under gcc-2.6.0 (try_flags), so
   the preset this unit wants does not close it either.
   branch is retail's; the frame is retail's 0x148 and the local layout is retail's
   (bufs 0x18, nameRect 0x98, tensRect 0xA0, onesRect 0xA8, tens2Rect 0xB0,
   ones2Rect 0xB8, blankRect 0xC0, cellUses 0xD0, cellIds 0x100 - declaration order).
   GAINED THIS ROUND (+3.04 over the 88.92 park): the ones digit is computed INSIDE
   its arm - `BattleLoadImageAndSync(&onesRect, g_BattleDigitImageBuffers[cellUses[i] % 10])` with
   no `ones` local (retail's `% 10` sits inside the `if (cellIds[i] != 0)` arm).
   Hoisting it into a u8 local reorders that whole block and costs 28 bytes; the
   tens digit DOES want its `u8 tens = cellUses[i] / 10;` local (retail's `andi 0xff`
   after the magic-10 shift) and the second grid half uses the un-truncated
   `cellUses[i + 16] / 10` spelling, so do not unify them.
   ROUND 312, worth +3.62 and the last 28 bytes: site 3's TEST reads the GLOBAL
   (`g_BattleCommandCellUses[i + 16] / 10 != 0`) while its INDEX reads the local copy.
   Retail divides TWICE there - the global read's quotient for the test (`srl 3` +
   `andi 0xff`) and the copy's quotient for the index, where gcc folds `/ 10 * 4` into
   `srl 1` + `andi 0x3FC`. The body is now retail's exact 1060 bytes.
   Remaining, ONE cause: retail re-forms `lui $at,%hi(g_BattleDigitImageBuffers) /
   addu $at,at,v0 / lw %lo(at)` at each of the four digit-buffer sites (the address
   never leaves $at), while loop.c hoists the table's address into a sixth callee-saved
   register (s4) and reads `addu v0,v0,s4; lw a1,0(v0)` - 2 insns per site, and the
   extra save/restore shifts every register in the prologue and epilogue by 4 bytes.
   Measured dead: the array form, `*(u_long **)(n * 4 + (u32)g_BattleDigitImageBuffers)`,
   `*(u_long **)((u8 *)g_BattleDigitImageBuffers + n * 4)` (a cc1_probe of the bare loop
   shows all three emit the same `la`), and `volatile` on the array declaration. gcc
   2.6.0 scores 92.77 with the unit's four matched functions still 100.00, so the preset
   is not the answer either. Next lever to try: a per-site spelling that gives each
   address pseudo a single use so loop.c has nothing shared to hoist; also untried for
   this body: separate ofs-style temporaries per digit site, a pointer local per arm,
   and the permuter.
void func_8008FE18(s32 arg0, s32 arg1, u8 arg2) {
    void *bufs[BATTLE_COMMAND_ROW_COUNT];
    RECT nameRect;
    RECT tensRect;
    RECT onesRect;
    RECT tens2Rect;
    RECT ones2Rect;
    RECT blankRect;
    u8 cellUses[BATTLE_COMMAND_CELL_COUNT];
    u8 cellIds[BATTLE_COMMAND_CELL_COUNT];
    void *pBuf;
    u8 tens;
    s32 i;

    if (arg2 != 0) {
        BattleAllocPacketPrimitives();
        BattleLoadActionRowGraphics();
        func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
    }
    pBuf = BattleAllocTextBuffer(0x39);
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
        pBuf = BattleAllocTextBuffer(0x1B);
        bufs[i] = pBuf;
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
            SystemRenderStringEntry(GetItemName(cellIds[i]), bufs[i], 0x1B, 0);
        }
        if (cellIds[i + BATTLE_COMMAND_CELL_ROW_STEP] != 0) {
            SystemRenderStringEntry(GetItemName(cellIds[i + BATTLE_COMMAND_CELL_ROW_STEP]), bufs[i], 0x1B, 1);
        }
        BattleLoadImageAndSync(&nameRect, bufs[i]);
        tens = cellUses[i] / 10;
        if (tens != 0) {
            BattleLoadImageAndSync(&tensRect, g_BattleDigitImageBuffers[tens]);
        } else {
            BattleLoadImageAndSync(&tensRect, g_BattleTextWorkspace);
        }
        if (cellIds[i] != 0) {
            BattleLoadImageAndSync(&onesRect, g_BattleDigitImageBuffers[cellUses[i] % 10]);
        } else {
            BattleLoadImageAndSync(&onesRect, g_BattleTextWorkspace);
        }
        if (g_BattleCommandCellUses[i + BATTLE_COMMAND_CELL_ROW_STEP] / 10 != 0) {
            BattleLoadImageAndSync(&tens2Rect, g_BattleDigitImageBuffers[cellUses[i + BATTLE_COMMAND_CELL_ROW_STEP] / 10]);
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
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->primState = 2;
}
*/
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
void func_80090310(u8 arg0, u8 arg1) {
    u8 *pBase;
    s32 index;
    s32 vofs;
    s32 u;
    s32 v;

    index = arg1 * 2 + arg0;
    vofs = 0;
    if (index >= 0x11) {
        index -= 0x10;
        vofs = 0x10;
        *(u16 *)(*(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex[0] * 0x28 + 0x14E) = g_SystemPalette2;
    } else {
        *(u16 *)(*(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex[0] * 0x28 + 0x14E) = g_SystemPalette1;
    }
    v = index / 2 * 13;
    u = index % 2 * 0x78;
    pBase = *(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattlePrimStartIndex[0] * 0x28 + 0x140 + (s32)pBase), 0x18, 0x33, u & 0xF8, v & 0xFF, 0x60);
    pBase = *(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(g_BattlePrimStartIndex[0] * 0x28 + 0x190 + (s32)pBase), 0x84, 0x33, (index % 2 << 6 | vofs) & 0xD0, v & 0xFF, 0x10);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090310);

/* PARKED 79.85% at retail's exact size (620 bytes = 0x26C, 155 insns) under the unit's
   gcc-2.6.0. ROUND 400 redo: the old 73.99 note came from a body that never compiled here
   (g_BattleCommandEntries, D_800D2F69/6A/6B and g_BattleActionRowIconSprites were undeclared
   in this unit; their declarations are now at the top of the file, so unparking compiles).
   Three measured levers took it 73.99 -> 79.85:
   - the row/col table indexes are INLINE (D_800D2F6A[row * 4], D_800D2F6A[col * 4], no ofs
     local) and col is a u8: that is what stops reorg duplicating the ofs sll into all three
     switch delay slots (with an ofs local it is duplicated 3 times and the body is 153 insns
     / 612 bytes). The u8 col also reproduces retail's andi s0,s2,0xFF before the second
     half's sll.
   - pMode = &g_BattlePrimStartIndex must be ASSIGNED TWICE, once per half: retail re-forms
     the lui/addiu pair at .L80090620 (the join of the first palette if/else) and cse cannot
     propagate the constant across that label, so the second half re-emits the la into the
     same register. One assignment for the whole body is 2 insns short (612 bytes). The array
     form g_BattlePrimStartIndex[0] is NOT it: it folds to the mips macro lw (that is what the
     matched BattleDrawMenuEntryString does).
   - pal is ONE value stored after the if/else join, in both halves.
   Remaining, three causes, all measured: (1) retail emits li a1,0xA8 / li a2,0x33 BEFORE the
   la of pMode in the join block, so reorg copies li a1,0xA8 into two switch delay slots; here
   the la comes first and those two slots stay nops (same instruction count, 5 lines).
   (2) the palette blocks: retail computes prim * 0x28 in pieces AROUND the pPacketPrims load
   (sll/addu, then sll 3 after the lw), this build loads g_BattleGraphicsWork first and
   finishes the multiply before the lbu D_800D2F69. Hoisting the multiply into a local
   (primOfs) reproduces that order and reads 85.86 but drops to 600 bytes / 150 insns because
   the two halves then share one address computation - it cannot link, do not ship it.
   (3) addu a0,t0,a0 here against retail's addu a0,a0,t0 (the call argument is base-first);
   the (s32)pBase and ((x + 0x320) + (s32)pBase) cast forms fold the constant onto the base
   (74.60, and 68.47 with an ofs local).
   A 4-minute permuter run found only the 600-byte primOfs edit (PERMUTER_BETTER 2375 -> 1826,
   rejected for the size mismatch). Next lever to try: a spelling that puts the two constant
   arguments first in the join block, e.g. the calls routed through local argument variables.
/*
void func_800904A0(u8 arg0, u8 arg1) {
    u8 *pBase;
    u8 *pRec;
    u16 flags;
    u16 pal;
    s32 row;
    u8 col;
    s32 *pMode;

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
    pMode = &g_BattlePrimStartIndex;
    pBase = *(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(pBase + (*pMode * 0x28 + 0x320)), 0xA8, 0x33,
        D_800D2F6A[row * 4], D_800D2F6B[row * 4], g_BattleActionRowIconSprites[row * 4]);
    pBase = *(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230);
    pRec = pBase + *pMode * 0x28;
    if (D_800D2F69[row * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    pMode = &g_BattlePrimStartIndex;
    pBase = *(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(pBase + (*pMode * 0x28 + 0x370)), 0xD0, 0x33,
        D_800D2F6A[col * 4], D_800D2F6B[col * 4], g_BattleActionRowIconSprites[col * 4]);
    pBase = *(u8 **)((u8 *)g_BattleGraphicsWork + 0xA230);
    pRec = pBase + *pMode * 0x28;
    if (D_800D2F69[col * 4] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_800904A0);


void BattleDrawMenuEntryString(u8 arg0, u8 arg1) {
    RECT rect;
    u8 *pBuf;
    u8 *pBase;
    s32 index;
    s32 len;
    s32 ofs;
    s32 palIndex;

    index = g_BattleCommandGridIds[arg1 * 2 + arg0];
    pBuf = (u8 *)BattleAllocTextBuffer(0x39);
    bzero(pBuf, MENU_ITEM_DESC_IMAGE_BUFFER_SIZE);
    len = SystemRenderStringEntry(GetStringEntry(D_800D329C, index), pBuf, 0x39, 0);
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
        func_800904A0(arg0, arg1);
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
// bytes, 68.22, and no lui/addiu form, unlike func_800904A0), `vofs = 0` hoisted above the
// if (identical 544), gcc-2.7.2 (56.88), gcc-2.7.2-cdk (compile-failed).
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
}
*/
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
// Not tried: a spelling that stops gcc duplicate-copying 'eo' at the first branch; that
// copy is the entire size gap.
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
        BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x11C, 0xC6, (d * 8 + 0x78) & 0xF8, 0, 8);
    }
    v = *(u16 *)(D_800CCD3A + eo);
    d = v % 10;
    pBase = g_BattleGraphicsWork->pPacketPrims;
    ofs = g_BattlePrimStartIndex[0] * 0x28 + 0x550;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x124, 0xC6, (d * 8 + 0x78) & 0xF8, 0, 8);
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
