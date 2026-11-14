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
extern void func_800904A0(u8, u8);
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


INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FAD8);
/* 66.32%: the two func_80076A10 loops and the accumulates match; the whole
   gap is the 9-arg func_800263E4 call block. The original keeps &D_800CCB34
   in s1 across the call (`lw a3,0(s1)` for the context arg, `lbu v0,0(s1)`
   after it) and needs no extra saved register (0x50 frame, s0-s5 + ra); gcc
   rematerialises `lui/lbu D_800CCB34` after the call, needs s6 and spills
   mirrorY. The last addu is also reversed: original `addu a2,a2,v0`
   (index product first), ours `addu a2,v0,a2`. Tried: `=` vs `+=` for the
   accumulate (65.93/66.11), an explicit `u8 *pPal = (u8 *)&D_800CCB34` used
   for both the s32 context and the tail byte store (66.32). Note the 5th
   parameter is u8 (the count comes in with lbu), not s32.
void func_8008FC1C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u8 arg4) {
    u8 *pPal;
    s32 i;
    s32 offset;
    s32 size;

    *(s32 *)(D_800D2D28 + 0xFC) = 0;
    i = 0;
    for (offset = arg3; i < arg4; i++) {
        size = func_80076A10(0x65, D_800C3EA4 + (*(s32 *)(D_800D2D28 + 0xFC) * 0x50 + 0x1E68), arg0, offset);
        *(s32 *)(D_800D2D28 + 0xFC) = size + *(s32 *)(D_800D2D28 + 0xFC);
        offset += 8;
    }
    pPal = (u8 *)&D_800CCB34;
    size = func_80076A10(0x64, D_800C3EA4 + (*(s32 *)(D_800D2D28 + 0xFC) * 0x50 + 0x1E68), arg0, arg1);
    *(s32 *)(D_800D2D28 + 0xFC) = size + *(s32 *)(D_800D2D28 + 0xFC);
    size = func_800263E4(D_800D2F5C, 0x64,
        (POLY_FT4 *)(D_800C3EA4 + (*(s32 *)(D_800D2D28 + 0xFC) * 0x50 + 0x1E68)),
        *(s32 *)pPal, (s16)arg0, (s16)arg2, 0x1000, 0, 1);
    *(s32 *)(D_800D2D28 + 0xFC) = size + *(s32 *)(D_800D2D28 + 0xFC);
    D_800D2D28[0xA6] = *pPal;
    D_800D2D28[0x9D] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FC1C);
void func_8008FDE4(void) {
    func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8008FE18);
/* 99.62%: two commutative operand orders. The original loads the D_800C3EA4
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

    *(s32 *)(D_800D2D28 + 0xF8) = 0;
    i = 0;
    for (offset = 0x68; i < 9; i++) {
        size = func_80076A10(0x66, D_800C3EA4 + (*(s32 *)(D_800D2D28 + 0xF8) * 0x50 + 0xBA8), 0x20, arg0 - 4 + offset);
        *(s32 *)(D_800D2D28 + 0xF8) = size + *(s32 *)(D_800D2D28 + 0xF8);
        offset += 8;
    }
    D_800D2D28[0xA5] = *(u8 *)&D_800CCB34;
    D_800D2D28[0x9C] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8009023C);
/* 74.61% (was 50.34): duplicating the palette store into both if-arms is what
   reproduces the original's structure -- gcc computes the D_800CCB34*0x28 and
   D_800C3EA4 prefix in each arm and cross-jumps the shared tail
   (P load, <<3, addu, `sh a0,0x14E`), exactly as the original does; a single
   store after the join only reaches 50%.
   Remaining gap: the original keeps `&D_800CCB34` in s2 (`lui/addiu` once, then
   `lw v1,0(s2)` per use) and `index % 2` in s1 next to vofs in s3, and stores
   the u8-truncated `v` to 0x10(sp) before reloading the index; here gcc reloads
   D_800CCB34 with a fresh lui/lw at each call (2 extra insns, 0x28 frame with
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
        *(u16 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x14E) = g_SystemPalette2;
    } else {
        *(u16 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x14E) = g_SystemPalette1;
    }
    v = index / 2 * 13;
    u = index % 2 * 0x78;
    func_80076C78((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x140), 0x18, 0x33, u, v, 0x60);
    func_80076C78((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x190), 0x84, 0x33, (index % 2 << 6) | vofs, v, 0x10);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090310);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_800904A0);
/* 73.99%: the structure is right -- flags = *(u16 *)(D_800D2204 +
   D_800D2CE0[idx] * 0x10), the 0x4000/0x1000 row pick (9/7/8), the 3-case
   switch on flags & 0xF giving col (0xC/0xD/0x15, no default case), the two
   func_80076C78 blocks (record offsets 0x320 and 0x370, u = D_800D2F6A[ofs],
   v = D_800D2F6B[ofs], dx = D_800D2F68[ofs], ofs = row * 4 then
   (col & 0xFF) * 4) and the two palette stores at +0x32E/+0x37E selected by
   D_800D2F69[ofs]. D_800CCB34 must be reached through a `s32 *pMode =
   &D_800CCB34` local -- that is what produces the retail `lui/addiu` + `lw`
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

    flags = *(u16 *)(D_800D2204 + D_800D2CE0[arg1 * 2 + arg0] * 0x10);
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
    pMode = &D_800CCB34;
    ofs = row * 4;
    pBase = *(u8 **)(D_800C3EA4 + 0xA230);
    func_80076C78((POLY_FT4 *)(pBase + (*pMode * 0x28 + 0x320)), 0xA8, 0x33,
        D_800D2F6A[ofs], D_800D2F6B[ofs], D_800D2F68[ofs]);
    pBase = *(u8 **)(D_800C3EA4 + 0xA230);
    pRec = pBase + *pMode * 0x28;
    if (D_800D2F69[ofs] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x32E) = pal;
    ofs = (col & 0xFF) * 4;
    pBase = *(u8 **)(D_800C3EA4 + 0xA230);
    func_80076C78((POLY_FT4 *)(pBase + (*pMode * 0x28 + 0x370)), 0xD0, 0x33,
        D_800D2F6A[ofs], D_800D2F6B[ofs], D_800D2F68[ofs]);
    pBase = *(u8 **)(D_800C3EA4 + 0xA230);
    pRec = pBase + *pMode * 0x28;
    if (D_800D2F69[ofs] != 0) {
        pal = g_SystemPalette2;
    } else {
        pal = g_SystemPalette1;
    }
    *(u16 *)(pRec + 0x37E) = pal;
}
*/
void func_8009070C(u8 arg0, u8 arg1) {
    RECT rect;
    u8 *pBuf;
    u8 *pBase;
    s32 index;
    s32 len;
    s32 ofs;
    s32 palIndex;

    index = D_800D2CE0[arg1 * 2 + arg0];
    pBuf = (u8 *)func_8008AC00(0x39);
    bzero(pBuf, 0x618);
    len = SystemRenderStringEntry(GetStringEntry(D_800D329C, index), pBuf, 0x39, 0);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    func_800769E8(&rect, pBuf);
    pBase = *(u8 **)(D_800C3EA4 + 0xA230);
    palIndex = D_800CCB34;
    ofs = palIndex * 0x28 + 0x280;
    func_80076C78((POLY_FT4 *)(ofs + (s32)pBase), 0x30, 0x42, 0, 0, len & 0xFF);
    HeapFree(pBuf);
}


void func_8009080C(u8 arg0, u8 arg1, u8 arg2) {
    u8 flag;
    u8 *p;

    if (arg2 == 0) {
        flag = D_800D2CE0[arg1 * 2 + arg0];
    } else {
        flag = D_800C3D70[arg1 * 2 + arg0];
    }
    *(u8 *)(*(u8 **)(D_800C3EA4 + 0xA230) + 0x66D) = 0;
    func_80090310(arg0, arg1);
    if (flag != 0 && arg2 == 0) {
        func_8009070C(arg0, arg1);
        func_800904A0(arg0, arg1);
        p = *(u8 **)(D_800C3EA4 + 0xA230);
        p[0x66D] = 1;
    }
    p = *(u8 **)(D_800C3EA4 + 0xA230);
    p[0x66A] = *(u8 *)&D_800CCB34;
    p = *(u8 **)(D_800C3EA4 + 0xA230);
    p[0x66B] = 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_8009093C);
/* 36.86%: first attempt. The shape is right -- the slti 0x68 / -0x68 /
   vofs 0x10 split, the two (idx*0x28 + K) palette sh stores, four
   func_80076CE8(p, x, y, u, v, dx, dy) calls with the 0x60/0x10 dx and the
   0x68 dy held in saved registers across them, and the trailing 0x668 store.
   The gap is this unit's documented wall: retail materialises
   `a2 = &D_800CCB34` (lui/addiu) and reloads `lw v0,0(a2)`, ours emits
   `lui/lw D_800CCB34` at each use; retail also keeps pal in a1 and pBase in
   a0 across the .L800909F4 join, so cc1 here reorders the two sh stores and
   pushes the D_800CCB34*0x28 multiply around. Not attempted: a pPal/pBase
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
    *(u16 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0xE) = pal;
    *(u16 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x5E) = pal;
    func_80076CE8((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28), 0x30, 0x60, 0, index, 0x60, 0x68);
    func_80076CE8((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0x50), 0xB4, 0x60, 0x78, index, 0x60, 0x68);
    func_80076CE8((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0xA0), 0x98, 0x60, vofs, index, 0x10, 0x68);
    func_80076CE8((POLY_FT4 *)(*(u8 **)(D_800C3EA4 + 0xA230) + D_800CCB34 * 0x28 + 0xF0), 0x11C, 0x60, vofs | 0x40, index, 0x10, 0x68);
    *(u8 *)(*(u8 **)(D_800C3EA4 + 0xA230) + 0x668) = D_800CCB34;
}
*/
void func_80090B90(s32 arg0, s32 arg1, s32 *pIndex, u8 *pCount) {
    u8 *p;

    if (++*pCount >= 3) {
        if (--*pIndex < 0) {
            *pIndex = 4;
        }
        *pCount = 0;
    }
    *(s32 *)(D_800D2D28 + 0x100) = func_80076A10(*pIndex + 0xE0, D_800C3EA4 + 0x27C8, arg0, arg1);
    p = D_800D2D28;
    p[0xA7] = *(u8 *)&D_800CCB34;
    *(volatile u8 *)(D_800D2D28 + 0x9E) = 1;
}
/* 72.46%: the four draw blocks have the right shape (divide-by-10 magic, the
   u16 truncations, the (idx*0x28 + K) + base order, and the literal 8 for arg6
   which gcc does hoist into a register from block 2 on, as the original does).
   Remaining gap: the original keeps `&D_800CCB34` in s2 (`lui/addiu` once in
   block 2, `lw v0,0(s2)` per use); gcc 2.7.2 emits a fresh `lui/lw` pair for
   the value at every use and folds an `s32 *pPal = &D_800CCB34;` local back
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
        pBase = *(u8 **)(D_800C3EA4 + 0xA230);
        ofs = D_800CCB34 * 0x28 + 0x460;
        func_80076C78((POLY_FT4 *)(ofs + (s32)pBase), 0x104, 0xC6, u, 0, 8);
    }
    v = *(u16 *)(D_800CCD38 + eo);
    d = v % 10;
    u = (d * 8 + 0x78) & 0xF8;
    pBase = *(u8 **)(D_800C3EA4 + 0xA230);
    ofs = D_800CCB34 * 0x28 + 0x4B0;
    func_80076C78((POLY_FT4 *)(ofs + (s32)pBase), 0x10C, 0xC6, u, 0, 8);
    v = *(u16 *)(D_800CCD3A + eo);
    d = v / 10;
    if (d != 0) {
        u = (d * 8 + 0x78) & 0xF8;
        pBase = *(u8 **)(D_800C3EA4 + 0xA230);
        ofs = D_800CCB34 * 0x28 + 0x500;
        func_80076C78((POLY_FT4 *)(ofs + (s32)pBase), 0x11C, 0xC6, u, 0, 8);
    }
    v = *(u16 *)(D_800CCD3A + eo);
    d = v % 10;
    u = (d * 8 + 0x78) & 0xF8;
    pBase = *(u8 **)(D_800C3EA4 + 0xA230);
    ofs = D_800CCB34 * 0x28 + 0x550;
    func_80076C78((POLY_FT4 *)(ofs + (s32)pBase), 0x124, 0xC6, u, 0, 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090C44);
/* 64.02% (round 36 parked this function at 64.78 with a u8 *pPal form):
   the four draw/palette blocks, the branch/join palette picks, the
   func_80090C44(arg0) call in the last store's delay slot and the trailing
   direct `lbu D_800CCB34` all have retail's shape. The gap is the mode read:
   retail materialises `lui s0,%hi; addiu s0,s0,%lo D_800CCB34` once and does
   `lw t0,0(s0)` at each of the 4 uses; gcc 2.7.2 copy-propagates the pointer
   initialiser into the loads and emits `lw D_800CCB34` (lui+lw, 2 insns) at 3
   of them, keeping the `la` only for the one use it cannot fold. Four source
   variants compiled to the same instruction stream: `s32 *pMode =
   &D_800CCB34` + `*pMode`, `u8 *pPal = (u8 *)&D_800CCB34` + casts,
   `volatile s32 *pMode`, and pBase-before-ofs vs ofs-before-pBase ordering.
   So the round-37 note that such a local reproduces retail's `la` + `lw 0(sN)`
   does not hold for straight-line code: every `la $sN,glob` + `lw 0($sN)` in
   build/src sits on a LOOP-INVARIANT address (battle/misc22 BattleFreeSceneResources,
   shop_menu ShopMenuUpdateBuyMenuExplanationGraphics, system/temp3, field
   misc/misc2/misc5). Also left: retail adds the constant to the PRODUCT
   (`sll/addu/sll; addiu a0,a0,0x3C0; addu a0,a0,t1`) where ours forms
   `pBase+0x3C0` first and adds the product -- probably a side effect of the
   same propagation, since func_8009070C's identical `ofs = idx*0x28 + K`
   phrasing matches in this unit. gcc-2.7.2-cdk compile-fails on this unit (mixed declarations).  */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16_r2", func_80090E7C);
