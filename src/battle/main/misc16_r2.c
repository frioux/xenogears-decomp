#include "common.h"
#include "main/game.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

// Cells of the battle command grid, and the rows of item names the menu draws.
#define BATTLE_COMMAND_CELL_COUNT 48
#define BATTLE_COMMAND_ROW_COUNT 32
// The grid holds the two rows of a cell BATTLE_COMMAND_CELL_ROW_STEP cells apart.
#define BATTLE_COMMAND_CELL_ROW_STEP 16

#define BATTLE_CURSOR_STEP_FRAMES 3
#define BATTLE_CURSOR_FRAME_MAX 4
extern u8 g_BattleCommandGridIds[];
extern u8 g_BattleCommandCellUses[];
extern u_long *D_800C3E5C[];
extern u_long *D_800D2DB0;
extern void BattleAllocPacketPrimitives(void);
extern void func_80076EA4(void);
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
extern void func_8008EA70(u8, s32, s32, s32);
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



INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FAD8);
/* 100.00% under gcc-2.6.0 (-O2 -G8): UN-PARK BY DELETING THIS COMMENT'S MARKERS.
   Reported as PRESET_260 for the unit: 90.53% under the unit's current Default
   (2.7.2-psx) and 74.70% under cdk. The whole remaining gap on 2.7.2 is one
   compiler behaviour: retail reloads each field inside the clamp arms
   (`lhu v0,8(a0); nop; addiu v0,v0,0x20` and `lhu v0,4(a0)`), i.e. the compare's
   operands are dead across the branch; 2.7.2 CSEs them into the compare registers
   (a2 = w, a1 = targetW) and the two `sh v0,8(a0)` stores can no longer cross-jump
   into one. gcc 2.6.0 emits retail's exact reload shape with this source, and the
   four functions already matched in this unit stay 100.00 under 2.6.0
   (try_flags.sh --all), so the unit belongs on the same 2.6.0 preset as its
   parent battle/main/misc16.
   Two shapes are load-bearing: the `>=` polarity (gcc then emits retail's
   `bnez -> += 0x20` arm with `w = bound; count++` as the fall-through), and the
   base-first pointer spelling `*(pFlags + i + 0xB8)` / `*(g_BattleDisplayState + i + 0xBF)`
   (the array spellings `pFlags[i + 0xB8]` emit `addu a1,s0,v0`, index first, and
   cost 0.25%). `count` is u8 (`andi v1,a1,0xFF` before the == 2 test); x and y
   are u16 locals, so the centre sums truncate once at the assignment.
void func_8008FAD8(void) {
    BattleSlotRect *p;
    u8 *pFlags;
    u8 count;
    u16 x;
    u16 y;
    s32 i;

    for (i = 0; i < 7; i++) {
        pFlags = g_BattleDisplayState;
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
            *(g_BattleDisplayState + i + 0xBF) = 1;
        }
        x = p->x + (p->targetW >> 1) - (p->w >> 1);
        y = p->y + (p->targetH >> 1) - (p->h >> 1);
        func_8008F6E4(p->slot, x, y, p->w, p->h);
    }
}
*/

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
/* 88.92% (1032 bytes against retail's 0x424 = 1060). Every call, rectangle, store
   and branch is retail's; the frame is retail's 0x148 and after moving `bufs` to the
   FIRST declaration slot the whole local layout is retail's too (bufs 0x18, nameRect
   0x98, tensRect 0xA0, onesRect 0xA8, tens2Rect 0xB0, ones2Rect 0xB8, blankRect 0xC0,
   cellUses 0xD0, cellIds 0x100 - gcc lays slots out in declaration order).
   What is left, all measured:
   - retail saves s0-s4 and re-forms `lui $at,%hi(D_800C3E5C) / addu $at,at,v0 /
     lw %lo(at)` at each of the four digit-buffer sites; this build hoists the table's
     address into s3 (the six-register frame), which is the two-register difference.
   - retail computes `u8 tens = cellUses[i] / 10` (the `andi 0xff` after the magic-10
     shift) but the `% 10` for ones INSIDE the `if (cellIds[i] != 0)` arm; the body
     below hoists both digits, which reorders that block and costs the 28 bytes.
   - the last block's guard is cellUses[i + 16] / 10 (D_800D2CC0 is that array's +0x10)
     and its digit index is cellUses[i + 16] / 10 WITHOUT the u8 truncation, so the two
     halves of the grid want different spellings of the same division.
   Not tried: separate `ofs`-style temporaries per digit site, a pointer local per arm,
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
    u8 ones;
    s32 i;

    if (arg2 != 0) {
        BattleAllocPacketPrimitives();
        func_80076EA4();
        func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
    }
    pBuf = BattleAllocTextBuffer(0x39);
    D_800D2DB0 = pBuf;
    bzero(pBuf, 0x618);
    blankRect.x = 0x3C0;
    blankRect.y = 0;
    blankRect.w = 0x3C;
    blankRect.h = 0xD;
    BattleLoadImageAndSync(&blankRect, D_800D2DB0);
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
            BattleLoadImageAndSync(&tensRect, D_800C3E5C[tens]);
        } else {
            BattleLoadImageAndSync(&tensRect, D_800D2DB0);
        }
        ones = cellUses[i] % 10;
        if (cellIds[i] != 0) {
            BattleLoadImageAndSync(&onesRect, D_800C3E5C[ones]);
        } else {
            BattleLoadImageAndSync(&onesRect, D_800D2DB0);
        }
        if (cellUses[i + BATTLE_COMMAND_CELL_ROW_STEP] / 10 != 0) {
            BattleLoadImageAndSync(&tens2Rect, D_800C3E5C[cellUses[i + BATTLE_COMMAND_CELL_ROW_STEP] / 10]);
        } else {
            BattleLoadImageAndSync(&tens2Rect, D_800D2DB0);
        }
        if (cellIds[i + BATTLE_COMMAND_CELL_ROW_STEP] != 0) {
            BattleLoadImageAndSync(&ones2Rect, D_800C3E5C[cellUses[i + BATTLE_COMMAND_CELL_ROW_STEP] % 10]);
        } else {
            BattleLoadImageAndSync(&ones2Rect, D_800D2DB0);
        }
    }
    for (i = 0; i < BATTLE_COMMAND_ROW_COUNT; i++) {
        HeapFree(bufs[i]);
    }
    HeapFree(D_800D2DB0);
    BattleUpdateFrame();
    g_BattleGraphicsWork->pPacketPrims[0x669] = 1;
    g_BattleDisplayState->primState = 2;
}
*//* 99.62% (212 bytes = retail's 0xd4): instruction-for-instruction identical except one
   swapped add: retail puts the -4 on the parameter (`addiu a3,s2,-4; addu a3,s0,a3`)
   where gcc 2.7.2 folds it onto the loop GIV (`addiu a3,s0,-4; addu a3,s2,a3`).
   Tried: arg0-4+offset, (arg0-4)+offset, offset+(arg0-4), a pre-loop `y = arg0` copy
   used as `y - 4 + offset` (99.02), and a 4-minute permuter run (2512 iterations, no
   improvement). The base spelling is `(u8 *)g_BattleGraphicsWork + ...`: the raw
   `g_BattleGraphicsWork + 0xBA8` in the earlier note scaled by sizeof(BattleGraphicsWork)
   (0xA2C8) and never compiled.
void func_8009023C(s32 arg0) {
    s32 i;
    s32 offset;
    s32 size;

    g_BattleDisplayState->spriteColumnPackageCount = 0;
    i = 0;
    for (offset = 0x68; i < 9; i++) {
        size = BattleDrawSpritePackage(0x66, (u8 *)g_BattleGraphicsWork + (g_BattleDisplayState->spriteColumnPackageCount * 0x50 + 0xBA8), 0x20, arg0 - 4 + offset);
        g_BattleDisplayState->spriteColumnPackageCount = size + g_BattleDisplayState->spriteColumnPackageCount;
        offset += 8;
    }
    g_BattleDisplayState->spriteColumnPrimStart = *(u8 *)&g_BattlePrimStartIndex;
    g_BattleDisplayState->spriteColumnPending = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8009023C);
/* 81.85% (400 bytes; retail 0x188 = 100 insns). The array-extern
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_800904A0);
/* 73.99%: the structure is right -- flags = *(u16 *)(g_BattleCommandEntries +
   g_BattleCommandGridIds[idx] * 0x10), the 0x4000/0x1000 row pick (9/7/8), the 3-case
   switch on flags & 0xF giving col (0xC/0xD/0x15, no default case), the two
   BattleSetPolyFT4RectH13 blocks (record offsets 0x320 and 0x370, u = D_800D2F6A[ofs],
   v = D_800D2F6B[ofs], dx = g_BattleActionRowIconSprites[ofs], ofs = row * 4 then
   (col & 0xFF) * 4) and the two palette stores at +0x32E/+0x37E selected by
   D_800D2F69[ofs]. g_BattlePrimStartIndex must be reached through a `s32 *pMode =
   &g_BattlePrimStartIndex` local -- that is what produces the retail `lui/addiu` + `lw`
   pair per call instead of a folded `lui/lw sym`.
   Remaining gaps: (1) retail keeps row in s0 (`li s0,9`) and shifts it once
   after the switch, gcc puts row in a0 and reorg duplicates `sll s0,a0,2` into
   the switch compares' delay slots; (2) retail precomputes pBase + mode*0x28 in
   the beqz delay slot of the palette test (`addu v1,v1,v0` then
   `sh v0,0x32E(v1)`), ours computes it after the branch; (3) the call argument
   is `addu a0,a0,t0` in retail (index term first) and `addu a0,t0,a0` here --
   casting the base to (s32) to flip it instead moves the multiply earlier
   (sll a0,0x3 before the +0x320) and drops to 70%; col lands in s1 here, s2 in
   retail. Tried: a `u8 *pRec` local for the palette address (in the parked body
   below, 73.99), both parentheses associativities for the call argument, and
   the (s32)-cast sum form.
void func_800904A0(u8 arg0, u8 arg1) {
    u8 *pBase;
    u8 *pRec;
    s32 *pMode;
    u16 flags;
    u16 pal;
    s32 row;
    s32 col;
    s32 ofs;

    flags = *(u16 *)(g_BattleCommandEntries + g_BattleCommandGridIds[arg1 * 2 + arg0] * 0x10);
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
    ofs = row * 4;
    pBase = *(u8 **)(g_BattleGraphicsWork + 0xA230);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(pBase + (*pMode * 0x28 + 0x320)), 0xA8, 0x33,
        D_800D2F6A[ofs], D_800D2F6B[ofs], g_BattleActionRowIconSprites[ofs]);
    pBase = *(u8 **)(g_BattleGraphicsWork + 0xA230);
    pRec = pBase + *pMode * 0x28;
    if (D_800D2F69[ofs] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    ofs = (col & 0xFF) * 4;
    pBase = *(u8 **)(g_BattleGraphicsWork + 0xA230);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(pBase + (*pMode * 0x28 + 0x370)), 0xD0, 0x33,
        D_800D2F6A[ofs], D_800D2F6B[ofs], g_BattleActionRowIconSprites[ofs]);
    pBase = *(u8 **)(g_BattleGraphicsWork + 0xA230);
    pRec = pBase + *pMode * 0x28;
    if (D_800D2F69[ofs] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}
*/
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
    bzero(pBuf, 0x618);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8009093C);
/* 36.86%: first attempt. The shape is right -- the slti 0x68 / -0x68 /
   vofs 0x10 split, the two (idx*0x28 + K) palette sh stores, four
   BattleSetPolyFT4Rect(p, x, y, u, v, dx, dy) calls with the 0x60/0x10 dx and the
   0x68 dy held in saved registers across them, and the trailing 0x668 store.
   The gap is this unit's documented wall: retail materialises
   `a2 = &g_BattlePrimStartIndex` (lui/addiu) and reloads `lw v0,0(a2)`, ours emits
   `lui/lw g_BattlePrimStartIndex` at each use; retail also keeps pal in a1 and pBase in
   a0 across the .L800909F4 join, so cc1 here reorders the two sh stores and
   pushes the g_BattlePrimStartIndex*0x28 multiply around. Not attempted: a pPal/pBase
   local pair (the same fold that parked func_80090C44 / func_80090E7C).
void func_8009093C(s32 arg0) {
    s32 index;
    u16 vofs;
    u16 pal;

    index = arg0;
    if (index >= 0x68) {
        index -= 0x68;
        vofs = 0x10;
        pal = g_SystemPalette2;
    } else {
        vofs = 0;
        pal = g_SystemPalette1;
    }
    *(u16 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0xE) = pal;
    *(u16 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x5E) = pal;
    BattleSetPolyFT4Rect((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28), 0x30, 0x60, 0, index, 0x60, 0x68);
    BattleSetPolyFT4Rect((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x50), 0xB4, 0x60, 0x78, index, 0x60, 0x68);
    BattleSetPolyFT4Rect((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0xA0), 0x98, 0x60, vofs, index, 0x10, 0x68);
    BattleSetPolyFT4Rect((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0xF0), 0x11C, 0x60, vofs | 0x40, index, 0x10, 0x68);
    *(u8 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + 0x668) = g_BattlePrimStartIndex;
}
*/
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
/* 72.46%: the four draw blocks have the right shape (divide-by-10 magic, the
   u16 truncations, the (idx*0x28 + K) + base order, and the literal 8 for arg6
   which gcc does hoist into a register from block 2 on, as the original does).
   Remaining gap: the original keeps `&g_BattlePrimStartIndex` in s2 (`lui/addiu` once in
   block 2, `lw v0,0(s2)` per use); gcc 2.7.2 emits a fresh `lui/lw` pair for
   the value at every use and folds an `s32 *pPal = &g_BattlePrimStartIndex;` local back
   into exactly that. That also renumbers the magic-10 register (s2 here, s1 in
   the original) and pushes `mfhi` into t1. Same wall as func_8009023C above.
   Tried: inline arg expression (69.01), separate `ofs` local + `ofs + (s32)pBase`
   (70.15), `s32 *pPal` local (70.15, folded away), the u8 `u` mask local below
   (72.46).
void func_80090C44(u8 arg0) {
    u8 *pBase;
    s32 eo;
    s32 ofs;
    u32 v;
    u16 d;
    u8 u;

    eo = arg0 * 0x170;
    v = *(u16 *)(D_800CCD38 + eo);
    d = v / 10;
    if (d != 0) {
        u = (d * 8 + 0x78) & 0xF8;
        pBase = *(u8 **)(g_BattleGraphicsWork + 0xA230);
        ofs = g_BattlePrimStartIndex * 0x28 + 0x460;
        BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x104, 0xC6, u, 0, 8);
    }
    v = *(u16 *)(D_800CCD38 + eo);
    d = v % 10;
    u = (d * 8 + 0x78) & 0xF8;
    pBase = *(u8 **)(g_BattleGraphicsWork + 0xA230);
    ofs = g_BattlePrimStartIndex * 0x28 + 0x4B0;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x10C, 0xC6, u, 0, 8);
    v = *(u16 *)(D_800CCD3A + eo);
    d = v / 10;
    if (d != 0) {
        u = (d * 8 + 0x78) & 0xF8;
        pBase = *(u8 **)(g_BattleGraphicsWork + 0xA230);
        ofs = g_BattlePrimStartIndex * 0x28 + 0x500;
        BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x11C, 0xC6, u, 0, 8);
    }
    v = *(u16 *)(D_800CCD3A + eo);
    d = v % 10;
    u = (d * 8 + 0x78) & 0xF8;
    pBase = *(u8 **)(g_BattleGraphicsWork + 0xA230);
    ofs = g_BattlePrimStartIndex * 0x28 + 0x550;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)pBase), 0x124, 0xC6, u, 0, 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090C44);
extern void func_80090C44(u8);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090E7C);
/* 79.25% (480 bytes; retail is 488 = 122 insns). The four draw/palette blocks, both
   branch/join palette picks, `func_80090C44(arg0)` in the last store's delay slot and
   the trailing direct `lbu g_BattlePrimStartIndex` all have retail's shape, and the
   two palette blocks are byte-identical. The gap is entirely in the two
   BattleSetPolyFT4RectH13 argument setups, and the levers found are:
   - the index must come from a pointer local (`s32 *pPrim = &g_BattlePrimStartIndex`,
     dereferenced at each use): that is what makes retail materialise
     `lui/addiu %hi/%lo` into s0 and reload `lw 0(s0)` per use. Any array spelling
     (`g_BattlePrimStartIndex[0]`) folds to a bare `lui/lw` pair.
   - `ofs = *pPrim * 0x28 + 0x3C0` in its own local before the call (the inline
     `... + 0x3C0 + (s32)g_BattleGraphicsWork->pPacketPrims` reassociates the constant
     onto the base instead: 76.12%).
   Remaining, all measured: retail adds the offset in the product's own register
   (`addiu a0,a0,0x3C0` then `addu a0,a0,t1`) where this build ends the ofs in t0/v0
   (`addu t0,v0,960; addu a0,t0,a0`), and retail loads the D_800D2F8x words in the order
   u, dx, v while gcc here emits v, dx after the base load. A 5-minute, 2-job permuter
   run from this body found no zero (best candidate wrapped the address in an inline
   helper, not portable); its only portable suggestion, inlining the ofs expression,
   is the 76.12% form above.
void func_80090E7C(u8 arg0) {
    u8 *pRec;
    s32 *pPrim;
    s32 ofs;
    u16 pal;

    pPrim = &g_BattlePrimStartIndex;
    ofs = *pPrim * 0x28 + 0x3C0;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)g_BattleGraphicsWork->pPacketPrims), 0xA8, 0xA6, D_800D2F82, D_800D2F83, D_800D2F80);
    pRec = g_BattleGraphicsWork->pPacketPrims + *pPrim * 0x28;
    if (D_800D2F81 != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x3CE) = pal;
    ofs = *pPrim * 0x28 + 0x410;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(ofs + (s32)g_BattleGraphicsWork->pPacketPrims), 0xEC, 0xC6, D_800D2F7E, D_800D2F7F, D_800D2F7C);
    pRec = g_BattleGraphicsWork->pPacketPrims + *pPrim * 0x28;
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