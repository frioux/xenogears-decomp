#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"

#define WORLD_MAP_GRID_CELL_COUNT 81
#define WORLD_MAP_GRID_WIDTH 9
#define WORLD_MAP_TILE_SECTOR_SIZE 0x710
#define WORLD_MAP_COORD_WRAP 0x800000
#define WORLD_MAP_SLOT_WRAP_Z_LOW 1
#define WORLD_MAP_SLOT_WRAP_Z_HIGH 2
#define WORLD_MAP_SLOT_WRAP_X_LOW 4
#define WORLD_MAP_SLOT_WRAP_X_HIGH 8

extern VECTOR g_WorldMapClipPlaneAxisY;
extern VECTOR g_WorldMapClipPlaneAxisX;
extern VECTOR g_WorldMapClipPlaneDirXNeg;
extern VECTOR g_WorldMapClipPlaneDirXPos;
extern VECTOR g_WorldMapClipPlaneDirYNeg;
extern VECTOR g_WorldMapClipPlaneDirYPos;
extern VECTOR g_WorldMapClipPlaneYZ1;
extern VECTOR g_WorldMapClipPlaneXZ0;
extern VECTOR g_WorldMapClipPlaneXZ1;
extern VECTOR g_WorldMapClipPlaneYZ0;

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} WorldMapMisc16Struct; // Size: 0xC

extern u16 g_WorldMapSlotUpdateFlags;
extern s16 g_WorldMapGridOrigin;
extern s16 g_WorldMapGridTileRowOrigin;

typedef struct {
    /* 0x0 */ s16 sin;
    /* 0x2 */ s16 pad2;
} WorldMapMisc16TblEntry; // Size: 0x4

typedef struct {
    /* 0x0 */ u32 vxy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ u16 pad6;
} WorldMapMisc16Pkt; // Size: 0x8

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 pad2;
    /* 0x4 */ s16 y;
} WorldMapMisc16Rect; // Size: 0x6

typedef struct {
    /* 0x0 */ s16 grid[81];
} WorldMapMisc16Grid; // Size: 0xA2

extern WorldMapMisc16Grid g_WorldMapPrevGridTileIds;
extern WorldMapMisc16Grid g_WorldMapGridTileIds;
extern s32 g_WorldMapTileColumns;
extern s32 g_WorldMapTileRows;

extern WorldMapMisc16TblEntry D_800523F0[];
extern s32 g_WorldMapGridShadePhase;
extern s32 g_WorldMapGridAngle;

void WorldMapComputeClipPlaneVectors(void) {
    OuterProduct0(&g_WorldMapClipPlaneDirXNeg, &g_WorldMapClipPlaneAxisY, &g_WorldMapClipPlaneXZ0);
    OuterProduct0(&g_WorldMapClipPlaneAxisY, &g_WorldMapClipPlaneDirXPos, &g_WorldMapClipPlaneXZ1);
    OuterProduct0(&g_WorldMapClipPlaneDirYNeg, &g_WorldMapClipPlaneAxisX, &g_WorldMapClipPlaneYZ0);
    OuterProduct0(&g_WorldMapClipPlaneAxisX, &g_WorldMapClipPlaneDirYPos, &g_WorldMapClipPlaneYZ1);
}

void WorldMapUpdateSlotGridOrigin(WorldMapMisc16Struct* p) {
    s32 y;
    s32 x;
    s32 z;

    x = p->x;
    z = p->z;
    g_WorldMapSlotUpdateFlags = 0;
    if (x < -WORLD_MAP_COORD_WRAP) {
        p->x = x + WORLD_MAP_COORD_WRAP;
        g_WorldMapSlotUpdateFlags = WORLD_MAP_SLOT_WRAP_X_LOW;
    } else if (x > WORLD_MAP_COORD_WRAP) {
        p->x = x - WORLD_MAP_COORD_WRAP;
        g_WorldMapSlotUpdateFlags = WORLD_MAP_SLOT_WRAP_X_HIGH;
    }
    if (z < -WORLD_MAP_COORD_WRAP) {
        p->z += WORLD_MAP_COORD_WRAP;
        g_WorldMapSlotUpdateFlags |= WORLD_MAP_SLOT_WRAP_Z_LOW;
    } else if (z > WORLD_MAP_COORD_WRAP) {
        p->z -= WORLD_MAP_COORD_WRAP;
        g_WorldMapSlotUpdateFlags |= WORLD_MAP_SLOT_WRAP_Z_HIGH;
    }
    y = p->x;
    g_WorldMapGridOrigin = (y >> 23) + 2;
    /* the z re-read has to stay below the store above it */
    y = *(volatile s32*)&p->z;
    g_WorldMapGridTileRowOrigin = (y >> 23) + 2;
}
void WorldMapBuildTileIdGrid(WorldMapMisc16Struct* p) {
    s32 col;
    s32 row;
    s32 w;
    s32 h;
    s32 colBase;
    s32 rowOff;
    s32 i;
    s32 j;
    s16* pGrid;

    w = g_WorldMapTileColumns;
    h = g_WorldMapTileRows;
    col = (p->x >> 12) / 8 - ((g_WorldMapGridOrigin + 2) << 8);
    row = (p->z >> 12) / 8 - ((g_WorldMapGridTileRowOrigin + 2) << 8);
    if (col < 0) {
        col += w << 8;
    } else if ((w << 8) < col) {
        col -= w << 8;
    }
    if (row < 0) {
        row += h << 8;
    } else if ((h << 8) < row) {
        row -= h << 8;
    }
    col >>= 8;
    row >>= 8;
    g_WorldMapPrevGridTileIds = g_WorldMapGridTileIds;
    colBase = col;
    pGrid = g_WorldMapGridTileIds.grid;
    for (i = WORLD_MAP_GRID_WIDTH - 1; i != -1; i--) {
        if (!(row < h)) {
            row = 0;
        }
        col = colBase;
        rowOff = row * w;
        for (j = WORLD_MAP_GRID_WIDTH - 1; j != -1; j--) {
            if (col >= w) {
                col = 0;
            }
            *pGrid++ = rowOff + col;
            col++;
        }
        row++;
    }
}


typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 pad6;
} WorldMapMisc16Point; // Size: 0x8

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u32 unk4;
} WorldMapMisc16Tex; // Size: 0x8

typedef struct {
    /* 0x0 */ s16 corner[4];
} WorldMapMisc16Clip; // Size: 0x8

typedef struct {
    /* 0x0 */ WorldMapMisc16Tex entry[25];
} WorldMapMisc16TileMask; // Size: 0xC8

extern WorldMapMisc16Clip D_8009D650[];
extern u8 D_1F8000C8[];
extern s16 D_8009D618[];
extern WorldMapMisc16TileMask D_8009B7A8[];
extern MATRIX g_WorldMapGridMatrix;
extern MATRIX g_WorldMapViewMatrix;
extern s32 func_800987AC(WorldMapMisc16Point* p0, WorldMapMisc16Point* p1, WorldMapMisc16Point* p2, WorldMapMisc16Point* p3);

/* 88.71%, CORRECT code, 1040 bytes against retail's 1036 (259 insns, .frame 0x70,
   mask s0-s8+ra). One instruction over; the rest of the diff is the allocator's
   five-value rotation.
   WHAT IT IS: clip a 5x5 block of grid cells against four frustum planes. The prologue
   copies g_WorldMapGridMatrix to 0x1F8000F0, CompMatrix's g_WorldMapViewMatrix into
   0x1F800110 and sets both GTE matrices. 0x1F800040 gets -(x>>12 & 0x7FF) - 0x1000, the
   same for z goes to 0x1F800048 and its negation to 0x1F8000A4. Nine 8-byte points
   (vx, vy, vz: an SVECTOR layout) live at 0x1F8000A0 + 8*k, k = 0..8, a 3x3 grid: the vy
   of each is cleared first, in the descending order 0xE2, 0xDA ... 0xA2. Per cell: the
   four grid corners (0xA0/0xA8/0xB0/0xB8) go to func_800987AC (the handwritten GTE clip
   test, 4 points -> -1/0/1) and, only when the first test returns 0, the five inner
   points 0xC0/0xC8/0xD0/0xD8/0xE0 are built and handed to four more tests, always in the
   order (top-left, top-right, bottom-left, bottom-right). The first result goes to
   D_8009D618[k] (s16, 25 cells) and the four results are packed two per u32 into
   D_8009D650[k] (word0 = r0 | r1<<16, word1 = r2 | r3<<16; a non-zero first result is
   written to both words). Tail: nVariant = (x>=0x400) | (z>=0x400 ? 2 : 0) picks one of
   four 0xC8-byte records of D_8009B7A8 and ORs its 25 8-byte mask words into D_8009D650.

   THE OLD PARK AT 89.02 / 1036 WAS WRONG CODE, and its 4 missing bytes were exactly the
   tail loop's advance: it read `pMask->entry->unk0` with no mask advance, so it ORed
   entry 0 into all 25 words. Retail's asm has `lw v1,4(s5); addiu s5,s5,8; lw v0,0(a0)`,
   so the mask walks with the texture. The honest spellings are
   `pMaskEntry = pMask->entry; ... pMaskEntry++;` (88.60, kept) or `pMask->entry[row]`
   (86.15: the index form loses the back-branch delay slot that the pointer form gets).

   THE 4 REMAINING BYTES ARE THE 0xC8 INNER POINT. Retail keeps ELEVEN values live across
   the four inner calls: nine in s0-s8 (0xD8, nPack, base, pTex, col, pClip, 0xE0, row,
   0xD0) and TWO spilled - `addiu t6,s2,%lo(D_1F8000C0)` into 0x38(sp) and
   `addiu t6,s2,%lo(D_1F8000C8)` into 0x40(sp), one instruction each in the col-loop
   preheader, loaded per call. That is why its frame is 0x70. This body has TEN: 0xC0 is
   spilled at 0x38(sp) but 0xC8 is rematerialised as a lui/addiu pair at both call sites,
   one instruction more than retail's store + two loads. MEASURED THIS ROUND, all correct
   code: `pScratch + 0xC8` inline at both sites 1060; a `WorldMapMisc16Point* pC8` local
   assigned at the top of the if-block 1060. Both DO give retail's 0x70 frame, but gcc
   then spends the register pClip was in (s8) on the pointer and spills pClip - the spill
   has to be the second-cheapest value, and pClip out-ranks it in retail only.

   THE TWO MEMORY SPELLINGS ARE BOTH REAL, keep the split: the setup stores (0x40, 0x48,
   0xA4 and the nine vy zeros) are ABSOLUTE literals, which maspsx renders as
   `lui $at; sw/sh %lo($at)`, while every loop access is base + displacement off
   $s2 = 0x1F800000. Writing the loop accesses as `pScratch + K` reproduces that exactly.
   The four corner POINTERS are literal constants too (`lui+ori`, no reloc) - keep
   `(WorldMapMisc16Point*)0x1F8000A0` and pass `&pQuad[0..3]`. The five inner pointers are
   `pScratch + 0xC0..0xE0`: retail shows `addiu $tN,$s2,%lo(D_1F8000K)` because splat's
   expected/ object carries a linker reloc there, the linked bytes are the same either way
   (JTBL/RELOC class: objdiff only sees the reloc name). D_1F8000C8 must stay the NAMED
   symbol `(WorldMapMisc16Point*)D_1F8000C8` - spelled `pScratch + 0xC8` the body is 1060
   (pClip gets spilled); named, it is 1040.

   THE SHAPES THAT GOT THIS FROM 80.26 TO 88.71 (all measured, keep them):
   (1) the tail's nVariant: retail computes the x slti, then the z slti, then fills the
   `bnez` delay slot with `xori s4,v0,1` and ORs 2 after it. Written as one expression
   (`nVariant = !((x>>12 & 0x7FF) < 0x400)`) gcc keeps slti+xori adjacent and leaves a
   `nop` in the slot, which is one instruction too many; splitting it into
   `nVariant = xcond; nPack = zcond; nVariant ^= 1; if (!nPack) nVariant |= 2;` puts the
   xori where retail has it (80.26 -> 83.59). A separate local instead of reusing nPack,
   or `>= 0x400` for the `^ 1`, both measure worse (80.30 / 1044 and 80.26 / 1044).
   (2) the inner store group order: `pA0 = pScratch + 0xA0;` before the 0xB0 store, the
   0xB0 store BEFORE the 0xB4 one, and the 0xB8 store reading `*(u16*)pA0`, plus the 0xD4
   store written OFFSET-FIRST (`*(u16*)(0xD4 + pScratch)`) (83.59 -> 86.09).
   (3) the tail mask loop reads its entry inline as TWO accesses through an advancing
   pointer; a `pMask->entry[i]` index costs the back-branch slot.
   (4) `*pClip = nClip;` with `pClip++;` as the LAST statement of the col loop body (after
   the pScratch+0xA0 store): retail's `addiu s5,s5,0x2` lands in the col back-branch delay
   slot and the `bnez` slot then carries the `addu a3,pE0` copy - with `*pClip++` the two
   swap (88.03 -> 89.02 on the wrong-code body, and it still pays).
   (5) splitting the x chain into `nX = -((p->x >> 12) & 0x7FF);` + `nX = nX - 0x1000;`
   and writing the z store as `nZ = -nZ;` after `*(s32*)0x1F800048 = nZ;` (88.60 -> 88.71).
   Retail runs BOTH chains through one register (`lw v0,0(t6) ... sw v0,0x40(at)`,
   `lw v0,8(t6) ... negu v0,v0`); this build negates into v1/a0. One shared nX local for
   both chains is worse (87.71 / 1048).
   REJECTED, not source: moving SetTransMatrix into the vy-zero group measures 88.81 but
   retail has all three matrix calls adjacent in the prologue. A permuter run's best
   candidate (score 2314) needed BOTH that move and `*D_8009D618 = nClip` in place of
   `*pClip = nClip`, which is wrong code (every cell would store to element 0) - do not
   port it. Candidate 2565 was the x-chain split, which is kept.

   WHAT IS LEFT: retail col=s4 pClip=s5 0xE0=s6 row=s7 0xD0=fp with 0xC0/0xC8 spilled;
   this body 0xE0=s4 col=s5 row=s6 0xD0=s7 pClip=s8 with only 0xC0 spilled, frame 0x68.
   Same nine registers, same values, a rotation. Six hand rounds and four permuter runs
   have not moved it, and five declaration orders measure identical, so it is not a
   pseudo-number tie. Also measured dead this round: pC0/pC8 as symbol-named locals
   (1024 / 83.18 - cse rematerialises the addiu instead of spilling); all five inner points
   as symbol locals (1024 / 76.16, frame 0x50); pD0, pD8, pE0 as preheader locals
   (1040 / 81.19, retail's 0x70 frame, one instruction over); pClip assigned before the
   matrix calls (85.47) or after the vy zeros (87.05); `pClip += 1` (identical);
   `(WorldMapMisc16Point*)(pScratch + 0xB8)` for the fourth corner (78.80 / 1028);
   nXClip/nZClip locals (80.30 / 1044). Not a preset question: gcc-2.7.2-cdk scores
   70.06 and gcc-2.6.0 69.89 on this body.

   ROUND 350: the four remaining bytes ARE the fourth corner pointer, and nothing else.
   Retail forms it with `ori t6,t6,0xB8` off the 0x1F800000 it had just reloaded for the
   spill slot, where `&pQuad[3]` makes this build materialise 0x1F8000B8 from scratch
   (lui+ori) - exactly the one instruction, 1040 against 1036. Spelling it
   `(WorldMapMisc16Point*)(pScratch + 0xB8)` DOES reach retail's exact 1036 bytes, but
   it re-ranks the entire allocation (base to s1, a corner pointer into s8, pClip to s2)
   for 81.45, with every other row of the diff unchanged. So size and allocation cannot
   be had from this body: that one pointer spelling moves five registers. Also measured
   this round, all correct code and all worse: a dead `u32 pad[2]` local (frame 0x70,
   88.75 - the pad takes retail's 0x18 slot and shifts every constant slot by 8);
   `pD0`/`pE0` as ROW-loop locals (1032 / 85.86, base to s1); reusing `col` for the tail
   variant (88.54 - it DOES put the variant in a saved register with retail's
   xori-in-the-bnez-delay-slot, but pTex and the variant then swap s3/s4); a
   `do { } while (0)` around the ten 0xC0..0xE4 stores (1016 / 79.67). The tail is the
   other divergence: retail keeps the variant in s4 (col's dead register) and the mask
   base in s5 (pClip's), where this body hands both a caller-saved temp (v1 / a1).

   ROUND 382 re-measure: 88.71 / 1040 reproduced exactly under the unit's own flags (260 insns
   against retail's 259, frame 0x68 against 0x70). The extra instruction is the whole size gap:
   retail pools the scratchpad BASE in 0x30(sp) once (`lui t6,0x1f80; sw t6,0x30(sp)` in the
   prologue) and builds the 0xB8 corner from it with `ori`, where this body forms 0xB8 with
   lui+ori and pools the base for 0xC0 instead - 7 instructions against retail's 6.
   NEW LEVER MEASURED: retail computes the four INNER point pointers in the ROW LOOP
   (`addiu t6,s2,%lo(D_1F8000C0); sw t6,0x38(sp)` / 0xC8 to 0x40(sp) / 0xE0 to s6 / 0xD0 to fp,
   four addius in the row-loop head), which is how it reaches ELEVEN live values and spills two.
   Written that way (pC0/pC8/pD0/pE0 assigned at the top of the row loop) gcc HOISTS all four into
   the prologue - they are loop-invariant - and the hoist costs three saved registers: base drops
   s2 -> s1, pTex s3 -> s2, pClip s5 -> s7. It DOES reach 1036 bytes / 259 insns, retail's exact
   size, and 83.24%. So size and allocation remain mutually exclusive from any source shape tried;
   a body that links needs the 0xC8 spill (see below), not another pointer spelling.
   REMAINING BLOCKER, unchanged and now explained: 0xC8 is spelled `(Point*)D_1F8000C8` (a
   symbol_ref) so reload rematerialises it at both call sites (2 insns each) instead of spilling it
   into retail's 0x40(sp) slot (1 store + 2 loads). 0xC0, spelled `pScratch + 0xC0`, IS spilled.
   Forcing the spill would need a value gcc cannot rematerialise, and every honest spelling of
   0xC8 is a constant address, so this is not reachable from C; retail's two spills come from its
   own pseudo ordering, not from a spelling.
   ROUND 383 (redo lane) re-measure and NEW SOURCE FACT: 88.71 / 1040 / 260 reproduced.
   The +1 instruction is now accounted for instruction by instruction: retail needs 7
   instructions to materialise the 4th corner, 0xC0 and the base (lui+sw base to 0x30(sp);
   lw+ori+sw for 0xB8 off the RELOADED base; addiu+sw for 0xC0 off $s2), this body needs 8
   (lui+ori+sw for 0xB8 as a constant; lui+sw for a base copy; lw+ori+sw for 0xC0).
   NEW FACT: retail's five INNER point addresses are NAMED SYMBOLS in the 1998 source, not
   `pScratch + K`: the original object carries LO16 relocations against D_1F8000C0, C8, D0,
   D8, E0 (`grep '%lo(D_1F8000' asm/world_map/nonmatchings/main/misc16/func_800983A0.s`
   shows exactly five, all `addiu $tN,$s2,%lo(D_1F8000K)`), while the four corner addresses
   0xA0..0xB8 are literal constants with no reloc. Declaring the five as
   `extern WorldMapMisc16Point D_1F8000Cx;` and passing `&D_1F8000Cx` reproduces retail's
   EXACT SIZE, 1036 bytes / 259 insns - the note above said that size was unreachable from
   any shape, and it is reachable, just not with the right allocation: gcc rematerialises
   each `la` at its call site (10 `la`s, no preheader pool) for 74.88. Pooling them into
   pC0/pC8/pD0/pD8/pE0 locals assigned at the row-loop head keeps 259 / 1036 and gives
   78.47. Both are correct code and both are worse than this body's 88.71, so the park keeps
   the constant-pointer body. NEXT LANE: the residual is retail POOLING those five (0xC0 and
   0xC8 spilled to 0x38/0x40(sp), 0xE0/0xD0/0xD8 in s6/fp/s0) while gcc 2.7.2 - which can
   rematerialise a live-%hi symbol address in ONE addiu - correctly prefers not to. A body
   that both pools and matches the allocation has not been found; the named-symbol spelling
   plus a non-rematerialisable value is the direction.
   ROUND 469 (redo lane) re-measure: 88.71% / 1040 bytes / 260 insns reproduced exactly; the
   prologue (all seven matrix moves, the three corner pools at 0x18/0x20/0x28, the absolute
   0x40/0x48/0xA4 stores, the nine descending vy zeros) is instruction-identical to retail.
   ONE new reading of retail's 4th corner: retail's is `ori $t6,$t6,0xB8` off the base already
   in $t6 and re-pools the result in $t6's own slot 0x30(sp) - i.e. retail's source spells it as
   a plain constant 0x1F8000B8 (the same shape that folds to `ori` for 0xC0 in THIS body), and
   the 2 extra instructions it costs this body come only from the base not being live there.
   So the whole residual is still "pool the base and keep it live across the vy-zero group",
   which is the same unreachable allocation as above: park.
 */
// 88.71% / 1040 bytes / 260 insns (retail 1036 / 259). ROUND 495 (park lane) re-measured this body,
// then worked the round-483 "allocno COUNT, not order" lever three ways. (a) The pA0 local is GONE for
// good: the 0xB8 store reads `*(u16*)(pScratch + 0xA0)` directly and the code is IDENTICAL
// (88.71 / 1040 / 260), so pA0 was pure CSE. (b) The tail reading `p->x` / `p->z` inline instead of
// through the nX / nZ locals: 87.67 / 1048 / 262, worse. (c) BEST PERCENTAGE YET, NOT LINKABLE: pooling
// `D_1F8000C8` into a `WorldMapMisc16Point* pC8` local assigned at the row-loop head (retail's 11th
// live value) gives 89.27 / 1032 / 258 - ONE INSTRUCTION SHORT of retail, base rotates s2 -> s1, pTex
// s3 -> s2 and every stack slot shifts, so the overlay would move: not committable. (d) Pooling pC0 AND
// pC8 at the row-loop head: 86.83 / 1012 / 253. (e) pC8 pooled with `(WorldMapMisc16Point*)(pScratch +
// 0xB0)` in place of `&pQuad[2]` in the third call: 82.85 / 1032. SIZE IS THE HARD CONSTRAINT from here
// on: only a body of exactly 1036 bytes can ever link, and every >= 88% body measured is 1040 (4 over)
// or 1032 (4 short) - size and percentage are anti-correlated across every spelling tried so far. The
// two known 1036 spellings are both far worse: 81.45 with pScratch + 0xB8 as the 4th corner, and
// 78.47 / 74.88 (pooled / unpooled) with the five inner points as named symbol addresses.
/*
void func_800983A0(WorldMapMisc16Struct* p) {
    u8* pScratch;
    WorldMapMisc16Point* pQuad;
    WorldMapMisc16Tex* pTex;
    WorldMapMisc16Tex* pMaskEntry;
    WorldMapMisc16TileMask* pMask;
    s16* pClip;
    s32 nX;
    s32 nZ;
    s16 nClip;
    s32 nPack;
    s32 nVariant;
    s32 row;
    s32 col;

    pScratch = (u8*)0x1F800000;
    *(MATRIX*)0x1F8000F0 = g_WorldMapGridMatrix;
    CompMatrix(&g_WorldMapViewMatrix, (MATRIX*)0x1F8000F0, (MATRIX*)0x1F800110);
    SetRotMatrix((MATRIX*)0x1F800110);
    SetTransMatrix((MATRIX*)0x1F800110);

    pClip = D_8009D618;
    pQuad = (WorldMapMisc16Point*)0x1F8000A0;
    pTex = D_8009D650;

    nX = -((p->x >> 12) & 0x7FF);
    nX = nX - 0x1000;
    *(s32*)0x1F800040 = nX;

    row = 0;
    *(u16*)0x1F8000E2 = 0;
    *(u16*)0x1F8000DA = 0;
    *(u16*)0x1F8000D2 = 0;
    *(u16*)0x1F8000CA = 0;
    *(u16*)0x1F8000C2 = 0;
    *(u16*)0x1F8000BA = 0;
    *(u16*)0x1F8000B2 = 0;
    *(u16*)0x1F8000AA = 0;
    *(u16*)0x1F8000A2 = 0;

    nZ = -((p->z >> 12) & 0x7FF) - 0x1000;
    *(s32*)0x1F800048 = nZ;
    nZ = -nZ;
    *(u16*)0x1F8000A4 = nZ;

    for (; row < 5; row++) {
        *(u16*)(pScratch + 0xA0) = *(s32*)(pScratch + 0x40);
        for (col = 0; col < 5; col++) {
            *(u16*)(pScratch + 0xA8) = *(u16*)(pScratch + 0xA0) + 0x800;
            *(u16*)(pScratch + 0xAC) = *(u16*)(pScratch + 0xA4);
            *(u16*)(pScratch + 0xB0) = *(u16*)(pScratch + 0xA0);
            *(u16*)(pScratch + 0xB4) = *(u16*)(pScratch + 0xA4) - 0x800;
            *(u16*)(pScratch + 0xB8) = *(u16*)(pScratch + 0xA0) + 0x800;
            *(u16*)(pScratch + 0xBC) = *(u16*)(pScratch + 0xA4) - 0x800;
            nClip = func_800987AC(&pQuad[0], &pQuad[1], &pQuad[2], &pQuad[3]);
            *pClip = nClip;
            if (nClip == 0) {
                *(u16*)(pScratch + 0xC0) = *(u16*)(pScratch + 0xA0) + 0x400;
                *(u16*)(pScratch + 0xC4) = *(u16*)(pScratch + 0xA4);
                *(u16*)(pScratch + 0xC8) = *(u16*)(pScratch + 0xA0);
                *(u16*)(pScratch + 0xCC) = *(u16*)(pScratch + 0xA4) - 0x400;
                *(u16*)(pScratch + 0xD0) = *(u16*)(pScratch + 0xA8);
                *(u16*)(0xD4 + pScratch) = *(u16*)(pScratch + 0xCC);
                *(u16*)(pScratch + 0xD8) = *(u16*)(pScratch + 0xC0);
                *(u16*)(pScratch + 0xDC) = *(u16*)(pScratch + 0xB4);
                *(u16*)(pScratch + 0xE0) = *(u16*)(pScratch + 0xC0);
                *(u16*)(pScratch + 0xE4) = *(u16*)(pScratch + 0xCC);
                nClip = func_800987AC(&pQuad[0], (WorldMapMisc16Point*)(pScratch + 0xC0), (WorldMapMisc16Point*)D_1F8000C8, (WorldMapMisc16Point*)(pScratch + 0xE0));
                nPack = nClip & 0xFFFF;
                nClip = func_800987AC((WorldMapMisc16Point*)(pScratch + 0xC0), &pQuad[1], (WorldMapMisc16Point*)(pScratch + 0xE0), (WorldMapMisc16Point*)(pScratch + 0xD0));
                nPack |= nClip << 16;
                *(u32*)pTex = nPack;
                nClip = func_800987AC((WorldMapMisc16Point*)D_1F8000C8, (WorldMapMisc16Point*)(pScratch + 0xE0), &pQuad[2], (WorldMapMisc16Point*)(pScratch + 0xD8));
                nPack = nClip & 0xFFFF;
                nClip = func_800987AC((WorldMapMisc16Point*)(pScratch + 0xE0), (WorldMapMisc16Point*)(pScratch + 0xD0), (WorldMapMisc16Point*)(pScratch + 0xD8), &pQuad[3]);
                nPack |= nClip << 16;
            } else {
                nPack = nClip & 0xFFFF;
                nPack |= nPack << 16;
                *(u32*)pTex = nPack;
            }
            pTex->unk4 = nPack;
            pTex++;
            *(u16*)(pScratch + 0xA0) += 0x800;
            pClip++;
        }
        *(u16*)(pScratch + 0xA4) -= 0x800;
    }

    nX = p->x;
    nZ = p->z;
    nVariant = ((nX >> 12) & 0x7FF) < 0x400;
    nPack = ((nZ >> 12) & 0x7FF) < 0x400;
    nVariant = nVariant ^ 1;
    if (!nPack) {
        nVariant |= 2;
    }
    pMask = &D_8009B7A8[nVariant];
    pMaskEntry = pMask->entry;
    pTex = D_8009D650;
    for (row = 0; row < 25; row++) {
        pTex->unk0 |= pMaskEntry->unk0;
        pTex->unk4 |= pMaskEntry->unk4;
        pTex++;
        pMaskEntry++;
    }
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800983A0);






INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800987AC);
extern s32 g_WorldMapGridImageFileIndex;
extern s32 g_WorldMapGridColumnImageFileIndex;
extern s16 g_WorldMapGridCornerTileIndices[];
extern u8* g_WorldMapTileCellPointers[];
extern u32 ArchiveGetDebugTable(void);
extern s32 ArchiveDecodeSector(s32 entryIndex);
extern u8* ArchiveGetFilePath(s32 entryIndex);
extern s32 WorldMapAddCdRequest(s32 x, s32 y, s32 z);
extern s32 WorldMapAddFileLoadRequest(s32 x, s32 y, s32 z, s32 w);
extern s32 WorldMapSubmitCdRequests(void);
extern s32 WorldMapSubmitFileLoads(void);

/* 97.46% re-measured this round - 1644 bytes (retail's exact size), 411 insns, 138
//    differing lines. Un-parked, diffed and re-parked unchanged: the body below IS the
//    97.46 one, and everything it earns still holds (free pass and all four nest bodies
//    match instruction for instruction; the prologue, its save order and the epilogue
//    match too, so the saved set is retail's ten).
//    NAMED RESIDUAL, two allocator hand-outs, no structural difference left:
//    (1) the tile-id walk index. -da/-dl on this body names it pseudo 78: 20 refs across
//        24 insns, dies in 4 places, NO call crossed, and global.c gives it a preference
//        for $v0, so it lands in $v0 in all eight places (li/sll pairs). Retail keeps the
//        same value in $s0 even though its live range is three instructions and it is dead
//        before the first loop call, which only a call-crossing or block-spanning allocno
//        reaches - and every spelling that makes it span one changes the instruction count
//        (assigning it above the ArchiveDecodeSector call: 409 insns / 1636 bytes, round
//        405). -dg this round: 40 allocnos, hard regs 2-8 and 16-23 + 30, one spill of $t0,
//        so the register file is retail's exactly.
//    (2) the two 4-iteration corner loops are a 4-cycle of the same four saved registers:
//        retail (s0,s1,s3,s4) = (cellPointers+id pointer, cornerIndices base, hasSector,
//        id), ours (id, pointer, cornerIndices base, hasSector); ours also hoists the
//        tileIds base into s6 (where our 'loaded' just died) where retail re-forms it
//        through $at each iteration. Same instructions, permuted registers.
//    MEASURED AND REJECTED, do not repeat: 'grid[idx++]' 413 insns / 1652 bytes 95.92;
//    u32 idx and a separate stride-9 index variable both byte-identical; sharing i
//    between the free pass and the nests 96.90; a separate id variable in a corner loop
//    414 insns 92.86; declaration order of the loop locals changes nothing; presets cdk
//    85.85, -O3 96.96, -G0 and 2.6.0 do not compile. Unit flags are right.
//    NEXT LEVER if anyone retries: the $v0 *preference* on pseudo 78, not the allocator
//    order - identify what gives it that preference (it is the only preferred allocno in
//    the second half) and remove the source shape that creates it; a permuter run is
//    pointless while the instruction stream is exact.
// */
/*
void func_80098CC0(void) {
    s32 cell;
    u8* pSector;
    u8* pPath;
    s32 i;
    s32 j;
    s32 k;
    s32 idx;
    s32 id;
    s32 loaded;
    s32 hasImage;
    s32 hasSector;
    u32 first;
    u32 second;

    for (i = 0; i < WORLD_MAP_GRID_CELL_COUNT; i++) {
        id = g_WorldMapPrevGridTileIds.grid[i];
        if (g_WorldMapTileCellPointers[id] != NULL) {
            for (j = 0; j < WORLD_MAP_GRID_CELL_COUNT; j++) {
                if (g_WorldMapGridTileIds.grid[j] == id) {
                    break;
                }
            }
            if (j == WORLD_MAP_GRID_CELL_COUNT) {
                HeapFree(g_WorldMapTileCellPointers[id]);
                g_WorldMapTileCellPointers[id] = NULL;
            }
        }
    }

    first = ArchiveGetDebugTable();
    second = ArchiveGetDebugTable();
    loaded = 0;
    if ((first == 0) | (second == 0xFFFFFFFF)) {
        pSector = (u8*)ArchiveDecodeSector(g_WorldMapGridImageFileIndex);
        idx = 1;
        for (j = 1; j != -1; j--) {
            for (cell = 6; cell != -1; cell--) {
                id = g_WorldMapGridTileIds.grid[idx];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                    WorldMapAddCdRequest((s32)(pSector + id), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                    loaded |= 2;
                }
                idx++;
            }
            idx = 0x49;
        }
        pSector = (u8*)ArchiveDecodeSector(g_WorldMapGridColumnImageFileIndex);
        idx = 9;
        for (j = 1; j != -1; j--) {
            for (cell = 6; cell != -1; cell--) {
                id = g_WorldMapGridTileIds.grid[idx];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                    WorldMapAddCdRequest((s32)(pSector + ((id % g_WorldMapTileColumns) * g_WorldMapTileRows + id / g_WorldMapTileColumns)), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                    loaded |= 1;
                }
                idx += WORLD_MAP_GRID_WIDTH;
            }
            idx = 0x11;
        }
        hasImage = loaded & 2;
        hasSector = loaded & 1;
        for (k = 0; k < 4; k++) {
            id = g_WorldMapGridTileIds.grid[g_WorldMapGridCornerTileIndices[k]];
            if (g_WorldMapTileCellPointers[id] == NULL) {
                g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                if (hasImage) {
                    pSector = (u8*)ArchiveDecodeSector(g_WorldMapGridImageFileIndex);
                    WorldMapAddCdRequest((s32)(pSector + id), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                } else if (hasSector) {
                    pSector = (u8*)ArchiveDecodeSector(g_WorldMapGridColumnImageFileIndex);
                    WorldMapAddCdRequest((s32)(pSector + ((id % g_WorldMapTileColumns) * g_WorldMapTileRows + id / g_WorldMapTileColumns)), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                }
            }
        }
        WorldMapAddCdRequest(0, 0, 0);
        WorldMapSubmitCdRequests();
    } else {
        pPath = ArchiveGetFilePath(g_WorldMapGridImageFileIndex);
        idx = 1;
        for (j = 1; j != -1; j--) {
            for (cell = 6; cell != -1; cell--) {
                id = g_WorldMapGridTileIds.grid[idx];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                    WorldMapAddFileLoadRequest((s32)pPath, id << 11, WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                    loaded |= 2;
                }
                idx++;
            }
            idx = 0x49;
        }
        pPath = ArchiveGetFilePath(g_WorldMapGridColumnImageFileIndex);
        idx = 9;
        for (j = 1; j != -1; j--) {
            for (cell = 6; cell != -1; cell--) {
                id = g_WorldMapGridTileIds.grid[idx];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                    WorldMapAddFileLoadRequest((s32)pPath, ((id % g_WorldMapTileColumns) << 11) * g_WorldMapTileRows + ((id / g_WorldMapTileColumns) << 11), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                    loaded |= 1;
                }
                idx += WORLD_MAP_GRID_WIDTH;
            }
            idx = 0x11;
        }
        hasImage = loaded & 2;
        hasSector = loaded & 1;
        for (k = 0; k < 4; k++) {
            id = g_WorldMapGridTileIds.grid[g_WorldMapGridCornerTileIndices[k]];
            if (g_WorldMapTileCellPointers[id] == NULL) {
                g_WorldMapTileCellPointers[id] = HeapAlloc(WORLD_MAP_TILE_SECTOR_SIZE, 0);
                if (hasImage) {
                    pPath = ArchiveGetFilePath(g_WorldMapGridImageFileIndex);
                    WorldMapAddFileLoadRequest((s32)pPath, id << 11, WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                } else if (hasSector) {
                    pPath = ArchiveGetFilePath(g_WorldMapGridColumnImageFileIndex);
                    WorldMapAddFileLoadRequest((s32)pPath, ((id % g_WorldMapTileColumns) << 11) * g_WorldMapTileRows + ((id / g_WorldMapTileColumns) << 11), WORLD_MAP_TILE_SECTOR_SIZE, (s32)g_WorldMapTileCellPointers[id]);
                }
            }
        }
        WorldMapAddFileLoadRequest(0, 0, 0, 0);
        WorldMapSubmitFileLoads();
    }
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80098CC0);



extern u16 D_8009CCB4[];
extern u16 g_WorldMapTpageIds[];
extern s32 D_8009D7DC;
extern void func_80099708(u32* pSrc, u8* p1, u8* p2, WorldMapMisc16Rect* pRect);
extern void func_8009980C(u32* pSrc, u8* p1, u8* p2);

/* 95.05%, 988 bytes = retail's exact size (247 insns, .frame 0x38, saves
   s0,s1,s2,s3,s4,s5,s6,s7,s8,ra - the SAME ten this body uses, no spills). Only two
   kinds of difference remain:
   (1) two addiu operands carry a reloc in retail that a raw constant cannot: the two
       copy loops' pointer increment (retail names D_1F800002) and the CompMatrix
       matrix pointer (retail names D_1F800370). Same linked bytes.
   (2) a pure register rotation in the cell body: retail a0=x0(x0=808) v1=x2(808+0x400)
       v0=y2(812-0x400) and an index chain accumulating in v0; this body a2=x0 a0=x1
       v1=y1 and the chain accumulates in v1. All 247 instructions, all 4 volatile
       scratchpad loads in retail's order (812, 808, 808, 812) and all six rect
       stores in retail's order (330, 340, 334, 338, 33C, 344).
   WHAT GOT THIS FROM 63.53 (the old park never compiled - it used D_8009D654 as a
   second symbol and WorldMapMisc16Tex.unk2): everything scratchpad goes through ONE
   u8* pScratch = (u8*)0x1F800000, so gcc keeps the base in s2 and uses 16-bit
   displacements - with absolute literals it instead CSEs a separate lui+ori address
   per site, puts 0x1F800328/0x1F80032C in saved registers and the body grows 37
   instructions (64.71). The four rect targets must be plain stores: written volatile,
   a chained A = B = expr reads B back and costs 2 loads (250 insns).
   The scratchpad loads stay volatile - that is what makes retail reload 0x320 right
   after storing it and read 0x328/0x32C twice per cell.
   MEASURED DEAD this round: reading x1 before the index expression and x0 after the
   image-pointer lookup (80 differing lines); the chained-store form without x1/y1
   locals (reverses the 0x330/0x340 store order, same 95.04); a 4-minute permuter run
   whose best candidate (626 vs 906) INSERTS a 0x32C = -0x320 store inside the cell
   loop, which is wrong code - rejected, do not port. */
// 95.05% re-measured (988 bytes = retail's exact size, 247 insns, .frame 0x38, same ten saves, no spills). Correct code. NAMED RESIDUAL, three parts: (1) RELOC: retail's four copy-loop/matrix args name symbols (addiu v1,v1,%lo(D_1F800002), addiu s0,s2,%lo(D_1F800370), the four addiu a3,s2,%lo(D_1F800328/330/338/340)) where this body has the folded constant - same linked bytes. (2) the four volatile coord reads: the multiset and the STORE order are retail's (0x330,0x340,0x334,0x338,0x33C,0x344) but sched1 HOISTS the second 0x328 read up next to the first pair, so this body's loads are 812,808,808,[chain],812 where retail has 812,808,[chain...tileId],808,[pointer],812. (3) a rotation in that region: retail y0=a1 x0=a0 x1=v1 y1=v0 with the id chain in v1->v0; this body y0=a1 x0=a2 x1=a0 y1=v1 with the chain in v0->v1. MEASURED DEAD this round (both byte-identical to the 95.05 body, 247/247/76 lines): writing the x1/y1 reads ABOVE the pImg statement instead of below it, and reversing the declaration order of the four coords (pseudo numbering follows first use, not the declaration list). Together with the note below (chained stores, one-pScratch base) that closes every cheap axis on the two remaining differences; the hoist is sched1's, and levers a-refs-are-three-times / a-global-allocno-priority-is say a body that emits these 247 instructions ranks the qtys the same way.
/* ROUND 474 RE-MEASUREMENT (identical to the note above): 95.05%, 988 bytes = retail's exact size, 247/247 insns, 76 differing lines. Apart from the four RELOC-only rows (retail names D_1F800002 / D_1F800370 / D_1F800328/330/338/340 where this body has the folded constant - same linked bytes) the residual is two things: sched1 hoisting this body's second volatile 0x328 read up next to the first pair, and a register rotation in the cell body. Two levers were looked up this round and both are dead ends here: a-block-split-flips-a-caller-saved-pair and a-do-while-around-a-store-makes-an-sched2-barrier both need an empty do-while barrier, which the lever itself says is not shippable source (and here it would have to fix the hoist AND the rotation in one edit), and a-second-assignment-to-the-losers-promotes-it has no free second assignment in this body (every coord is dead right after its two stores). One more row visible in the tail: retail emits the two loop increments (s4, s3) BEFORE the volatile re-read of 0x328 and this body emits them after it - the same sched1 hoist, not a second residual. */
/*
void func_8009932C(u8* p0, u8* p1, WorldMapMisc16Struct* p) {
    u8* pScratch;
    s32 i;
    s32 row;
    s32 k;
    u16 y0;
    u16 x0;
    u16 x1;
    u16 y1;
    s16 clip;
    u8* pImg;

    pScratch = (u8*)0x1F800000;
    for (i = 0; i < 0x40; i++) {
        *(u16*)(pScratch + 0x288 + i * 2) = D_8009CCB4[i];
    }
    for (i = 0; i < 7; i++) {
        *(u16*)(pScratch + 0x308 + i * 2) = g_WorldMapTpageIds[i];
    }
    *(MATRIX*)(pScratch + 0x350) = g_WorldMapGridMatrix;
    CompMatrix(&g_WorldMapViewMatrix, (MATRIX*)(pScratch + 0x350), (MATRIX*)(pScratch + 0x370));
    SetRotMatrix((MATRIX*)(pScratch + 0x370));
    SetTransMatrix((MATRIX*)(pScratch + 0x370));
    *(s32*)(pScratch + 0x318) = -((p->x >> 12) & 0x7FF) - 0x1000;
    *(volatile s32*)(pScratch + 0x320) = -((p->z >> 12) & 0x7FF) - 0x1000;
    *(volatile u16*)(pScratch + 0x32C) = -*(volatile s32*)(pScratch + 0x320);
    D_8009D7DC = 0;
    k = 0;
    for (row = 0; row < 5; row++) {
        *(volatile u16*)(pScratch + 0x328) = *(s32*)(pScratch + 0x318);
        for (i = 0; i < 5; i++, k++) {
            if (D_8009D618[k] == -1) {
                continue;
            }
            y0 = *(volatile u16*)(pScratch + 0x32C);
            x0 = *(volatile u16*)(pScratch + 0x328);
            pImg = g_WorldMapTileCellPointers[g_WorldMapGridTileIds.grid[(row + g_WorldMapGridTileRowOrigin) * WORLD_MAP_GRID_WIDTH + i + g_WorldMapGridOrigin]];
            x1 = *(volatile u16*)(pScratch + 0x328) + 0x400;
            y1 = *(volatile u16*)(pScratch + 0x32C) - 0x400;
            *(u16*)(pScratch + 0x330) = x1;
            *(u16*)(pScratch + 0x340) = x1;
            *(u16*)(pScratch + 0x334) = y0;
            *(u16*)(pScratch + 0x338) = x0;
            *(u16*)(pScratch + 0x33C) = y1;
            *(u16*)(pScratch + 0x344) = y1;
            clip = D_8009D650[k].corner[0] | D_8009D650[k].corner[1] | D_8009D650[k].corner[2] | D_8009D650[k].corner[3];
            if (clip != -1) {
                func_80099708((u32*)pImg, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)(pScratch + 0x328));
                func_80099708((u32*)(pImg + 0x144), p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)(pScratch + 0x330));
                func_80099708((u32*)(pImg + 0x288), p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)(pScratch + 0x338));
                func_80099708((u32*)(pImg + 0x3CC), p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)(pScratch + 0x340));
            } else {
                if (D_8009D650[k].corner[0] != clip) {
                    func_80099708((u32*)pImg, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)(pScratch + 0x328));
                }
                if (D_8009D650[k].corner[1] != clip) {
                    func_80099708((u32*)(pImg + 0x144), p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)(pScratch + 0x330));
                }
                if (D_8009D650[k].corner[2] != clip) {
                    func_80099708((u32*)(pImg + 0x288), p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)(pScratch + 0x338));
                }
                if (D_8009D650[k].corner[3] != clip) {
                    func_80099708((u32*)(pImg + 0x3CC), p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)(pScratch + 0x340));
                }
            }
            *(volatile u16*)(pScratch + 0x328) += 0x800;
        }
        *(volatile u16*)(pScratch + 0x32C) -= 0x800;
    }
}*/
// ROUND 487 re-measure: 95.05% / 988 bytes / 247 insns / 76 differing lines, exactly as the note
// above describes. `cc1 -da` on this unit's .i gives this function's allocno order
// (171 188 294 298 293 297 84 75 286 76 72 73 78 77 74 135; dispositions 171=a1 188=s0 294/298=v1
// 293/297=a0 84=s1 75=s2 286=s3 76=s4 72=s5 73=s6 78=s7 77=fp 135=v0) - the saved set is already
// retail's, so the
// residual is only the two v0/v1 caller-saved coords and sched1's placement of the colOrigin load
// and the 0x328 re-read. Priority there is floor_log2(n_refs)*1000/live_length, which is the
// handle to pull if anyone wants to move the v0/v1 pair; the sched1 rows are not source-order.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009932C);




// 97.74% (round 474 re-measured, 260 bytes = retail's 65 insns, correct code). Residual: (1) the xCur increment is emitted TWO slots late - retail emits xCur first of the four increments (xCur, pPkt, p, ii, idx in the bne slot); this body emits pPkt, p, xCur, ii, idx. (2) a four-temp register rotation: retail xCur=a3 pPkt=t0 idx=t1 ii=t2, this body xCur=t1 pPkt=t2 idx=a3 ii=t0. MEASURED DEAD this round: xCur += 0x80 written first of the four (sched1 hoists that addiu above the or/sw/sh) 94.82; post-increment inside the store, pPkt->vxy = c | (xCur++ & 0xFFFF) - wrong code anyway, it adds 1 and not 0x80 - 94.82. PERMUTER (3 min, 1609 iters): base 155 best 140, and the winner is an "if (pPkt) { c = (s8)v << 19; } else { <same arm> }" - gcc merges the identical arms so it is behaviour-identical but it is NOT shippable source; ported it measures 97.97 with pPkt correct in t0 and the other three still cycled. That is the evidence that one MORE ALLOCNO REFERENCE on pPkt is what this body is missing, with no natural fifth use of pPkt to write.
// ROUND 487, NEW MECHANISM (the rotation is not reachable from source order). `cc1 -da` on this
// unit's own .i prints global.c's allocno order for this function as 84(idx) 86(ii) 81(xCur)
// 76(pPkt), and those four land in a3, t0, t1, t2 in that order. Priority is
// floor_log2(n_refs)*1000/live_length (lreg: idx 11 refs/28 insns, ii 11/33, xCur 11/34,
// pPkt 13/46 - all four in the same floor_log2 bucket, so live_length alone orders them).
// Retail's mapping is xCur,pPkt,idx,ii -> a3,t0,t1,t2, i.e. it needs the priority order
// xCur > pPkt > idx > ii, i.e. live_lengths xCur < pPkt < idx < ii - but idx's span is the
// SHORTEST of the four here, because its only uses sit in the if-arm. A body whose instruction
// stream is retail's cannot produce that order, which is why three sessions of spellings and a
// permuter run all died on this row. Also measured this round: writing the increments as
// (xCur,pPkt,p), (pPkt,xCur,p) and (p,pPkt,xCur) emits the SAME four registers, so source
// order cannot move the mapping. The lone RELOC row (`addiu t0,t0,%lo(D_1F800008)`) is splat's
// hi/lo inference on a real `addiu t0,t0,8` (word 0x25080008) - misc9's class - so a matching
// body would read ~99.9% here and needs check_bin.sh, not objdiff 100.
/*
void func_80099708(u32* pSrc, u8* p1, u8* p2, WorldMapMisc16Rect* pRect) {
    WorldMapMisc16Pkt* pPkt;
    WorldMapMisc16TblEntry* pTbl;
    u32* p;
    s32 x;
    s32 y;
    s32 xCur;
    s32 col;
    s32 iCol;
    s32 idx;
    s32 oi;
    s32 ii;
    s32 sinVal;
    s32 prod;
    u32 v;
    s32 c;

    pPkt = (WorldMapMisc16Pkt*)0x1F800000;
    p = pSrc;
    oi = 8;
    pTbl = D_800523F0;
    col = g_WorldMapGridShadePhase;
    x = pRect->x;
    y = pRect->y;
    for (; oi != -1; oi--) {
        xCur = x;
        iCol = pTbl[col & 0xFFF].sin * 2;
        idx = g_WorldMapGridAngle;
        for (ii = 8; ii != -1; ii--) {
            v = *p;
            if (v & 0x1000) {
                sinVal = pTbl[idx & 0xFFF].sin;
                prod = sinVal * iCol;
                c = (s8)v << 3;
                c = ((prod >> 20) + c) << 16;
            } else {
                c = (s8)v << 19;
            }
            pPkt->vxy = c | (xCur & 0xFFFF);
            pPkt->vz = y;
            pPkt++;
            p++;
            xCur += 0x80;
            idx += 0x200;
        }
        y -= 0x80;
        col += 0x200;
    }
    func_8009980C(pSrc, p1, p2);
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099708);




INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009980C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099BFC);
