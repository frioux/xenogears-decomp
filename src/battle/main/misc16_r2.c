#include "common.h"
#include "main/game.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

#define BATTLE_CURSOR_STEP_FRAMES 3
#define BATTLE_CURSOR_FRAME_MAX 4
extern u8 g_BattleCommandGridIds[];
extern u8 *D_800D329C;
extern BattleGraphicsWork *g_BattleGraphicsWork;
extern s32 g_BattlePrimStartIndex;
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
    /* 0x4 */ u16 unk4;
    /* 0x6 */ u16 unk6;
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
    /* 0x66C */ u8 unk66C;
    /* 0x66D */ u8 labelDrawn; // set by the cell that drew its label string
    /* 0x66E */ u8 unk66E;
    /* 0x66F */ u8 unk66F;
} BattleMenuPrims; // Size: 0x670



INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FAD8);
/* 100.00% under gcc-2.6.0 (-O2 -G8): UN-PARK BY DELETING THIS COMMENT'S MARKERS.
   Reported as PRESET_260 for the unit: 90.53% under the unit's current Default
   (2.7.2-psx) and 74.70% under cdk. The whole remaining gap on 2.7.2 is one
   compiler behaviour: retail reloads each field inside the clamp arms
   (`lhu v0,8(a0); nop; addiu v0,v0,0x20` and `lhu v0,4(a0)`), i.e. the compare's
   operands are dead across the branch; 2.7.2 CSEs them into the compare registers
   (a2 = w, a1 = unk4) and the two `sh v0,8(a0)` stores can no longer cross-jump
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
        if (p->w + 0x20 >= p->unk4) {
            p->w = p->unk4;
            count++;
        } else {
            p->w += 0x20;
        }
        if (p->h + 0x20 >= p->unk6) {
            p->h = p->unk6;
            count++;
        } else {
            p->h += 0x20;
        }
        if (count == 2) {
            *(g_BattleDisplayState + i + 0xBF) = 1;
        }
        x = p->x + (p->unk4 >> 1) - (p->w >> 1);
        y = p->y + (p->unk6 >> 1) - (p->h >> 1);
        func_8008F6E4(p->slot, x, y, p->w, p->h);
    }
}
*/

/* 66.32%: the two BattleDrawSpritePackage loops and the accumulates match; the whole
   gap is the 9-arg func_800263E4 call block. The original keeps &g_BattlePrimStartIndex
   in s1 across the call (`lw a3,0(s1)` for the context arg, `lbu v0,0(s1)`
   after it) and needs no extra saved register (0x50 frame, s0-s5 + ra); gcc
   rematerialises `lui/lbu g_BattlePrimStartIndex` after the call, needs s6 and spills
   mirrorY. The last addu is also reversed: original `addu a2,a2,v0`
   (index product first), ours `addu a2,v0,a2`. Tried: `=` vs `+=` for the
   accumulate (65.93/66.11), an explicit `u8 *pPal = (u8 *)&g_BattlePrimStartIndex` used
   for both the s32 context and the tail byte store (66.32). Note the 5th
   parameter is u8 (the count comes in with lbu), not s32.
void func_8008FC1C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u8 arg4) {
    u8 *pPal;
    s32 i;
    s32 offset;
    s32 size;

    *(s32 *)(g_BattleDisplayState + 0xFC) = 0;
    i = 0;
    for (offset = arg3; i < arg4; i++) {
        size = BattleDrawSpritePackage(0x65, g_BattleGraphicsWork + (*(s32 *)(g_BattleDisplayState + 0xFC) * 0x50 + 0x1E68), arg0, offset);
        *(s32 *)(g_BattleDisplayState + 0xFC) = size + *(s32 *)(g_BattleDisplayState + 0xFC);
        offset += 8;
    }
    pPal = (u8 *)&g_BattlePrimStartIndex;
    size = BattleDrawSpritePackage(0x64, g_BattleGraphicsWork + (*(s32 *)(g_BattleDisplayState + 0xFC) * 0x50 + 0x1E68), arg0, arg1);
    *(s32 *)(g_BattleDisplayState + 0xFC) = size + *(s32 *)(g_BattleDisplayState + 0xFC);
    size = func_800263E4(g_BattleSpriteArchive, 0x64,
        (POLY_FT4 *)(g_BattleGraphicsWork + (*(s32 *)(g_BattleDisplayState + 0xFC) * 0x50 + 0x1E68)),
        *(s32 *)pPal, (s16)arg0, (s16)arg2, 0x1000, 0, 1);
    *(s32 *)(g_BattleDisplayState + 0xFC) = size + *(s32 *)(g_BattleDisplayState + 0xFC);
    g_BattleDisplayState[0xA6] = *pPal;
    g_BattleDisplayState[0x9D] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FC1C);
void BattleInitMenuColumnPrims(void) {
    func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FE18);
/* 99.62%: two commutative operand orders. The original loads the g_BattleGraphicsWork
   index part first (sll/addu/sll) then `addiu a1,a1,0xBA8` then `addu a1,a1,v0`
   with the pointer base; gcc either reverses the final addu (pointer base, as
   below) or, with the base cast to s32, folds 0xBA8 onto the base side
   (`addiu v0,v0,0xBA8; addu a1,a1,v0`). For the 4th argument gcc moves the -4
   onto the loop induction variable (`addiu a3,s0,-4; addu a3,a3,s2`) whichever
   way the sum is written, because fold attaches constants to the GIV; the
   original has `addiu a3,s2,-4; addu a3,s0,a3`. Tried: arg0-4+offset,
   (arg0-4)+offset, offset+arg0-4, offset+(arg0-4), base-first and
   index-first with and without an (s32) cast on the base, and a `size` temp for
   the block-size accumulate (that temp fixed the rest of the loop, 65 -> 99.62).
void func_8009023C(s32 arg0) {
    s32 i;
    s32 offset;
    s32 size;

    *(s32 *)(g_BattleDisplayState + 0xF8) = 0;
    i = 0;
    for (offset = 0x68; i < 9; i++) {
        size = BattleDrawSpritePackage(0x66, g_BattleGraphicsWork + (*(s32 *)(g_BattleDisplayState + 0xF8) * 0x50 + 0xBA8), 0x20, arg0 - 4 + offset);
        *(s32 *)(g_BattleDisplayState + 0xF8) = size + *(s32 *)(g_BattleDisplayState + 0xF8);
        offset += 8;
    }
    g_BattleDisplayState[0xA5] = *(u8 *)&g_BattlePrimStartIndex;
    g_BattleDisplayState[0x9C] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8009023C);
/* 74.61% (was 50.34): duplicating the palette store into both if-arms is what
   reproduces the original's structure -- gcc computes the g_BattlePrimStartIndex*0x28 and
   g_BattleGraphicsWork prefix in each arm and cross-jumps the shared tail
   (P load, <<3, addu, `sh a0,0x14E`), exactly as the original does; a single
   store after the join only reaches 50%.
   Remaining gap: the original keeps `&g_BattlePrimStartIndex` in s2 (`lui/addiu` once, then
   `lw v1,0(s2)` per use) and `index % 2` in s1 next to vofs in s3, and stores
   the u8-truncated `v` to 0x10(sp) before reloading the index; here gcc reloads
   g_BattlePrimStartIndex with a fresh lui/lw at each call (2 extra insns, 0x28 frame with
   three saves instead of 0x30 with four). Keeping `i2 = index % 2` in a local
   and using it twice drops it to 72.33. void func_80090310(u8 arg0, u8 arg1) {
    s32 index;
    s32 vofs;
    s32 u;
    s32 v;

    index = arg1 * 2 + arg0;
    vofs = 0;
    if (index >= 0x11) {
        index -= 0x10;
        vofs = 0x10;
        *(u16 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x14E) = g_SystemPalette2;
    } else {
        *(u16 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x14E) = g_SystemPalette1;
    }
    v = index / 2 * 13;
    u = index % 2 * 0x78;
    BattleSetPolyFT4RectH13((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x140), 0x18, 0x33, u, v, 0x60);
    BattleSetPolyFT4RectH13((POLY_FT4 *)(*(u8 **)(g_BattleGraphicsWork + 0xA230) + g_BattlePrimStartIndex * 0x28 + 0x190), 0x84, 0x33, (index % 2 << 6) | vofs, v, 0x10);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090310);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_800904A0);
/* 73.99%: the structure is right -- flags = *(u16 *)(g_BattleCommandEntries +
   g_BattleCommandGridIds[idx] * 0x10), the 0x4000/0x1000 row pick (9/7/8), the 3-case
   switch on flags & 0xF giving col (0xC/0xD/0x15, no default case), the two
   BattleSetPolyFT4RectH13 blocks (record offsets 0x320 and 0x370, u = D_800D2F6A[ofs],
   v = D_800D2F6B[ofs], dx = D_800D2F68[ofs], ofs = row * 4 then
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
        D_800D2F6A[ofs], D_800D2F6B[ofs], D_800D2F68[ofs]);
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
        D_800D2F6A[ofs], D_800D2F6B[ofs], D_800D2F68[ofs]);
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
    palIndex = g_BattlePrimStartIndex;
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
    ((BattleMenuPrims *)g_BattleGraphicsWork->pPacketPrims)->primStart = *(u8 *)&g_BattlePrimStartIndex;
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
    g_BattleDisplayState->cursorPrimStart = *(u8 *)&g_BattlePrimStartIndex;
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
/* 64.02% (round 36 parked this function at 64.78 with a u8 *pPal form):
   the four draw/palette blocks, the branch/join palette picks, the
   func_80090C44(arg0) call in the last store's delay slot and the trailing
   direct `lbu g_BattlePrimStartIndex` all have retail's shape. The gap is the mode read:
   retail materialises `lui s0,%hi; addiu s0,s0,%lo g_BattlePrimStartIndex` once and does
   `lw t0,0(s0)` at each of the 4 uses; gcc 2.7.2 copy-propagates the pointer
   initialiser into the loads and emits `lw g_BattlePrimStartIndex` (lui+lw, 2 insns) at 3
   of them, keeping the `la` only for the one use it cannot fold. Four source
   variants compiled to the same instruction stream: `s32 *pMode =
   &g_BattlePrimStartIndex` + `*pMode`, `u8 *pPal = (u8 *)&g_BattlePrimStartIndex` + casts,
   `volatile s32 *pMode`, and pBase-before-ofs vs ofs-before-pBase ordering.
   So the round-37 note that such a local reproduces retail's `la` + `lw 0(sN)`
   does not hold for straight-line code: every `la $sN,glob` + `lw 0($sN)` in
   build/src sits on a LOOP-INVARIANT address (battle/misc22 BattleFreeSceneResources,
   shop_menu ShopMenuUpdateBuyMenuExplanationGraphics, system/temp3, field
   misc/misc2/misc5). Also left: retail adds the constant to the PRODUCT
   (`sll/addu/sll; addiu a0,a0,0x3C0; addu a0,a0,t1`) where ours forms
   `pBase+0x3C0` first and adds the product -- probably a side effect of the
   same propagation, since BattleDrawMenuEntryString's identical `ofs = idx*0x28 + K`
   phrasing matches in this unit. gcc-2.7.2-cdk compile-fails on this unit (mixed declarations).  */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090E7C);
