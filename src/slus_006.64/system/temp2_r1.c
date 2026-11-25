#include "common.h"
#include "system/menu.h"
#include "system/animation.h"
#include "psyq/inline_c.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2_r1", func_8002709C);
/* Best parked version 80.74% (808 bytes, retail's exact size).
Re-measured this round under the unit's own preset (XenoGraphicsDiv, gcc
2.7.2-cdk): un-parked, filled in the block typedef below (the parked copy had
its fields in // comments, so it did not compile), and it scores 80.74% at the
original's exact 808 bytes - so the fills, the loop bounds and the call set
are all right and only the register/frame layout is left, as the note below
says.
Two traps this cost, worth knowing before the next attempt:
1. THE PARK BLOCK CANNOT CONTAIN A STAR-SLASH. The first un-park declared the
   fields with psyq-style inline offset comments, whose own terminators close
   the enclosing park comment: the body then reads as live C and maspsx reports
   'symbol already defined'. The fields below therefore keep their offsets as
   trailing // comments, which also means the struct compiles as soon as the
   block is un-parked (the previously parked copy had its fields commented out
   and did not compile at all).
2. permute.py died on that same broken text ('Syntax error in base.c' at the
   star-slash) before it ever reached the body, so the permuter has NOT
   actually been tried on this function - with the block fixed above it is the
   obvious next step for a body that is register/frame layout only.

Parked at 80.74%, 808 bytes (the original's exact size), first body written
from the derivation below - so the fills, the six loops' bounds and the call
set are all right. What is left is register allocation only:
  retail frame 0xA0 with the width spill at 0x70, saved regs s0 = height,
  s1 = pVec, s2 = pColors, s3 = arg11, s5 = arg12, s6 = arg13, s7 = arg2,
  fp = arg3, and clutX/clutY NOT hoisted (reloaded from 0xB0/0xB4 inside the
  FT4 loop); this body spills arg2/arg3 to the outgoing-arg area instead,
  hoists clutY into a saved register and puts the width spill at 0x98, so the
  prologue and every stack offset are +8 from retail's.
Also note the parameter types the .s proves: width and height are s32 (a0 is
spilled with sw, height gets the plain `bgez` + addiu 0xFF of a 32-bit signed
%256 - an s16 height makes gcc emit sll/sra first), arg7/arg11/arg12/arg13 are
u16 (lhu) and pVec is read with lhu at 0/4/8 for the colour halves but lw at 8
for the `vz < 0` test, so pVec->vz is both.

// Block func_8002709C allocates: the prims GfxProjectQuadBanks fills, then the
// draw state it copies in.

typedef struct {
    POLY_FT4 ft4[16];  // 0x000
    POLY_F4 f4[4];     // 0x280
    POLY_G4 g4[2];     // 0x2E0
    s32 unk328;        // 0x328
    s32 unk32C;        // 0x32C
    s32 unk330;        // 0x330
    u16 width;         // 0x334
    u16 height;        // 0x336
    u16 unk338;        // 0x338
    u16 unk33A;        // 0x33A
    u16 vx;            // 0x33C
    u16 vy;            // 0x33E
    u16 vz;            // 0x340
    u16 unk342;        // 0x342
    u16 unk344;        // 0x344
    u16 unk346;        // 0x346
    u16 unk348;        // 0x348
    u16 unk34A;        // 0x34A
} Temp2FillBlock; // Size: 0x34C

void* func_8002709C(s32 width, s32 height, s32 arg2, s32 arg3, s32 clutX,
                    s32 clutY, u16 arg7, s32 arg8, VECTOR* pVec, u8* pColors,
                    u16 arg11, u16 arg12, u16 arg13) {
    DRAWENV env;
    Temp2FillBlock* pBlock;
    s32 i;
    s32 j;

    HeapChangeCurrentUser(4, 0);
    pBlock = HeapAlloc(sizeof(Temp2FillBlock), 0);
    if (pBlock == NULL) {
        return NULL;
    }
    GetDrawEnv(&env);
    pBlock->unk328 = arg2;
    pBlock->unk32C = arg3;
    pBlock->vx = pVec->vx;
    pBlock->vy = pVec->vy;
    pBlock->vz = pVec->vz;
    pBlock->unk346 = arg11;
    pBlock->unk348 = arg12;
    pBlock->unk34A = arg13;
    if (pVec->vz < 0) {
        pBlock->unk330 = -arg8;
    } else {
        pBlock->unk330 = arg8;
    }
    pBlock->height = height;
    pBlock->width = width;
    pBlock->unk338 = arg7;
    pBlock->unk33A = height % 256;
    for (i = 0; i < 16; i++) {
        SetPolyFT4(&pBlock->ft4[i]);
        SetShadeTex(&pBlock->ft4[i], 1);
        pBlock->ft4[i].clut = GetClut(clutX, clutY);
    }
    if (pColors == NULL) {
        pBlock->unk344 = 0;
        return pBlock;
    }
    pBlock->unk344 = 1;
    for (j = 0; j < 2; j++) {
        SetPolyF4(&pBlock->f4[j]);
        pBlock->f4[j].r0 = pColors[0];
        pBlock->f4[j].g0 = pColors[1];
        pBlock->f4[j].b0 = pColors[2];
        pBlock->f4[j].x0 = 0;
        pBlock->f4[j].y0 = 0;
        pBlock->f4[j].x1 = 0x140;
        pBlock->f4[j].y1 = 0;
        pBlock->f4[j].x2 = 0;
        pBlock->f4[j].x3 = 0x140;
    }
    pColors += 4;
    for (j = 0; j < 2; j++) {
        pBlock->g4[j].r0 = pColors[0];
        pBlock->g4[j].g0 = pColors[1];
        pBlock->g4[j].b0 = pColors[2];
        pBlock->g4[j].r1 = pColors[0];
        pBlock->g4[j].g1 = pColors[1];
        pBlock->g4[j].b1 = pColors[2];
        pColors += 4;
        pBlock->g4[j].r2 = pColors[0];
        pBlock->g4[j].g2 = pColors[1];
        pBlock->g4[j].b2 = pColors[2];
        pBlock->g4[j].r3 = pColors[0];
        pBlock->g4[j].g3 = pColors[1];
        pBlock->g4[j].b3 = pColors[2];
        pColors -= 4;
        SetPolyG4(&pBlock->g4[j]);
        pBlock->g4[j].x0 = 0;
        pBlock->g4[j].x1 = 0x140;
        pBlock->g4[j].x2 = 0;
        pBlock->g4[j].x3 = 0x140;
    }
    pColors += 4;
    for (j = 2; j < 4; j++) {
        SetPolyF4(&pBlock->f4[j]);
        pBlock->f4[j].r0 = pColors[0];
        pBlock->f4[j].g0 = pColors[1];
        pBlock->f4[j].b0 = pColors[2];
        pBlock->f4[j].x0 = 0;
        pBlock->f4[j].x1 = 0x140;
        pBlock->f4[j].x2 = 0;
        pBlock->f4[j].y2 = 0xF0;
        pBlock->f4[j].x3 = 0x140;
        pBlock->f4[j].y3 = 0xF0;
    }
    return pBlock;
}
*/

/*
Derivation kept from the previous round; every offset below is taken
straight from the .s.
worker does not have to re-read 202 instructions. Every offset below is taken
straight from the .s.

Signature: 13 parameters. The only caller, func_80071828 in field/main/misc3,
sets stack slots 0x10..0x30 before the jal, i.e. args 5..13.
  a0 u16 width, a1 s16 height (a1 is stored with sh and used in the signed
  modulo below, so it is s16), a2 s32 at 0x328, a3 s32 at 0x32C,
  arg5/arg6 clutX/clutY (reloaded inside the FT4 loop, so GetClut is called
  there), arg7 u16 at 0x338, arg8 s32 negated when pVec->vz is negative,
  arg9 VECTOR* pVec (u16 read at 0, 4, 8 and the s32 at 8 tests < 0),
  arg10 u8* pColors, arg11/arg12/arg13 u16 at 0x346/0x348/0x34A.

Block returned by HeapAlloc(0x34C, 0) after HeapChangeCurrentUser(4, 0):
  0x000 POLY_FT4[16] (0x28 each), 0x280 POLY_F4[4] (0x18 each),
  0x2E0 POLY_G4[2] (0x24 each, the interleaved psyq layout),
  then s32 0x328, s32 0x32C, s32 0x330, u16 0x334 width, 0x336 height,
  0x338 arg7, 0x33A height mod 256 (signed: cdk emits the +0xFF fixup),
  0x33C/0x33E/0x340 the low halves of pVec vx/vy/vz, 0x344 flag,
  0x346/0x348/0x34A arg11..13. HeapAlloc(0x34C) is the frame check.

Statement order the asm shows: HeapChangeCurrentUser(4, 0); HeapAlloc;
if NULL return NULL; GetDrawEnv(&env) with a DRAWENV local at 0x10(sp)
(that is the whole 0xA0 frame); then 0x328, 0x32C, 0x33C, 0x33E, 0x340,
0x346, 0x348, 0x34A; then 0x330 = (pVec->vz < 0) ? -arg8 : arg8; then 0x334,
0x336, 0x338 and 0x33A; then the FT4 loop; then the pColors block.

pColors block: 0x344 = pColors ? 1 : 0. Then
  (a) two POLY_F4 at 0x280: SetPolyF4, r0/g0/b0 = pColors[0..2],
      x0=0, y0=0, x1=0x140, y1=0, x2=0, x3=0x140 - note y2 and y3 are NOT
      written here.
      then pColors += 4.
  (b) two POLY_G4 at 0x2E0: SetPolyG4, r0/g0/b0 = pColors[0..2],
      r1/g1/b1 = pColors[0..2], r2/g2/b2 = pColors[4..6],
      r3/g3/b3 = pColors[4..6], x0=0, x1=0x140, x2=0, x3=0x140.
      (the asm does addiu s2,s2,4 halfway and addiu s2,s2,-4 at the end of
      the iteration, net zero, then advances it by 4 after the loop; reading
      pColors[4..6] without the add/re-add is semantically identical but
      will not reproduce the two addius.)
      then pColors += 4.
  (c) two POLY_F4 at 0x2B0 (f4[2] and f4[3]): SetPolyF4, r0/g0/b0 =
      pColors[0..2], x0=0, x1=0x140, x2=0, y2=0xF0, x3=0x140, y3=0xF0 -
      here y0 and y1 are NOT written.
Returns the block pointer.

Why it is not written yet: the fills are six or eight byte/short stores per
primitive interleaved with three lbu loads, and the base register gcc picks
differs per block (s0 = base + i*0x28, base + i*0x18, base + i*0x24,
base + 0x30 + i*0x18), so the per-primitive statement set has to be written
EXACTLY as the original for the instruction order to come out. Start from the
fills above, diff, and only then chase the pColors add/re-add shape.
*/

// The render block func_8002709C allocates: sixteen textured quads, the flat and
// Gouraud rows GfxProjectQuadBanks fills (one bank of each per screen context), then
// the draw state the quads are sized from.
typedef struct {
    POLY_FT4 ft4[16];  // 0x000
    POLY_F4 f4[4];     // 0x280
    POLY_G4 g4[2];     // 0x2E0
    s32 unk328;        // 0x328
    s32 unk32C;        // 0x32C
    s32 unk330;        // 0x330
    s16 unk334;        // 0x334
    s16 unk336;        // 0x336
    s16 unk338;        // 0x338
    u16 unk33A;        // 0x33A
    u16 unk33C;        // 0x33C
    s16 unk33E;        // 0x33E
    s16 unk340;        // 0x340
    u16 unk342;        // 0x342
    s16 unk344;        // 0x344
    s16 unk346;        // 0x346
    s16 unk348;        // 0x348
    s16 unk34A;        // 0x34A
} Temp2RenderBlock; // Size: 0x34C

extern void func_800278F8(Temp2RenderBlock* pBlock, s32 left, s32 top, s32 span,
                          u_long* pOT, s32 bank);

// Projects the eye and at points into the block's two banks: the first row of
// flat quads and the gouraud row are placed at the projected eye height, the
// second flat row at the projected at height. `span` is the distance between
// the two points scaled into 0..0x100. Returns the projected eye y.
s32 GfxProjectQuadBanks(Temp2RenderBlock* pBlock, SVECTOR* pFrom, SVECTOR* pTo,
                  MATRIX* pMatrix, u_long* pOT, s32 band) {
    SVECTOR proj1;
    SVECTOR proj2;
    VECTOR delta;
    SVECTOR work;
    SVECTOR normal;
    s32 angle;
    s16 level;
    s16 y;

    if (pBlock == NULL) {
        return 0;
    }

    delta.vx = pTo->vx - pFrom->vx;
    delta.vy = 0;
    delta.vz = pTo->vz - pFrom->vz;
    VectorNormalS(&delta, &normal);

    // The normal is scaled by how far the eye sits from the plane and added to
    // the point, so the quad row follows the camera.
    work.vx = (normal.vx * pBlock->unk340) / 0x1000 + (u16)pTo->vx;
    work.vy = pBlock->unk33E;
    work.vz = (normal.vz * pBlock->unk340) / 0x1000 + (u16)pTo->vz;

    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    gte_ldv0(&work);
    gte_rtps();
    gte_stsxy(&proj1);

    if (pBlock->unk348 != 0) {
        delta.vx = pTo->vx - pFrom->vx;
        delta.vy = pTo->vy - pFrom->vy;
        delta.vz = pTo->vz - pFrom->vz;
        level = (SquareRoot0(delta.vx * delta.vx + delta.vy * delta.vy +
                             delta.vz * delta.vz) -
                 pBlock->unk34A) /
                pBlock->unk348;
        if (level < 0) {
            level = 0;
        }
        if (level > 0x100) {
            level = 0x100;
        }
    } else {
        level = 0;
    }

    angle = pBlock->unk328 * pBlock->unk330 * (ratan2(delta.vx, delta.vz) & 0xFFF);
    func_800278F8(pBlock, angle / 0x1000, proj1.vy, level, pOT, band);

    if (pBlock->unk344 > 0) {
        proj1.vx = proj1.vy - (u16)pBlock->unk32C;
        if (proj1.vx > 0xF0) {
            proj1.vx = 0xF0;
        }
        if (proj1.vx > 0) {
            pBlock->f4[band].y2 = proj1.vx;
            pBlock->f4[band].y3 = proj1.vx;
            addPrim(pOT, &pBlock->f4[band]);
        }

        // The at point is projected a second time, this one scaled by unk346.
        work.vx = ((normal.vx * pBlock->unk340) / 0x1000 * pBlock->unk346) /
                      0x100 +
                  (u16)pTo->vx;
        work.vy = (pBlock->unk33E * pBlock->unk346) / 0x100;
        work.vz = ((normal.vz * pBlock->unk340) / 0x1000 * pBlock->unk346) /
                      0x100 +
                  (u16)pTo->vz;
        gte_ldv0(&work);
        gte_rtps();
        gte_stsxy(&proj2);

        if (proj2.vy - proj1.vy > 0xF0) {
            proj2.vy = proj1.vy + 0xF0;
        }
        if (proj2.vy >= 0) {
            if (proj1.vy < 0xF0) {
                pBlock->g4[band].y0 = proj1.vy;
                pBlock->g4[band].y1 = proj1.vy;
                pBlock->g4[band].y2 = proj2.vy;
                pBlock->g4[band].y3 = proj2.vy;
                addPrim(pOT, &pBlock->g4[band]);
            }
        }
        if (proj2.vy < 0) {
            y = 0;
        } else {
            y = proj2.vy;
        }
        if (y < 0xF0) {
            pBlock->f4[band + 2].y0 = y;
            pBlock->f4[band + 2].y1 = y;
            addPrim(pOT, &pBlock->f4[band + 2]);
        }
    }
    return proj1.vy;
}

// Derivation only, never diffed (no percentage yet); kept so the next worker
// does not have to re-read 274 instructions.
//
// Signature, from its only caller GfxProjectQuadBanks (jal at 0x80027600):
//   (pBlock, s32 a1, s32 a2, s16 a3, u_long* pOT, s32 bank)
// a0 is the 0x34C block func_8002709C allocates; a3 is 0..0x100 from the
// caller's clamp and is the divisor's base everywhere below; arg6 is 0 or 1
// and picks which 0x140-byte half of the block is written. Every division is a
// real div (with the divide-by-zero and INT_MIN override sequences), so all
// divisors are runtime values. Frame 0x50, locals at 0x10 (u16), and y/a2/pOT
// are spilled to 0x18/0x1C/0x20 around the one call.
//
// 1. q  = (pBlock->unk328 << 8) / (a3 + 0x100)
//    h  = (0x140 - q) / 2
//    x  = h + (h * a3) / 0x100
//    d  = (s16)(a1 - x);
//    s3 = d % pBlock->unk328; if (s3 < 0) s3 += pBlock->unk328;
//    unk328 is loaded with lw here and with lhu at the modulo fixup, so the
//    source narrows it on one side. The signed remainder path (mfhi plus the
//    +unk328 fixup) is the same shape the caller spells at 0x80027570.
// 2. y = 0; x0 = 0;
//    if (a2 >= 0 && a2 <= pBlock->unk32C + 0xF0) y = (pBlock->unk32C << 8) / (a3 + 0x100);
//    One pre-test (bltz a2) plus one slt against unk32C+0xF0, and both paths
//    run addu x0,zero,zero, so it is a single if with an else, not two.
// 3. pPrimBase = (u8*)pBlock + (arg6 & 1) * 0x140;   a step of 0x28 * 8
//    if (y <= 0) return;   the test is sll/sra on y, so y is s16.
// 4. hoisted before the loop: u0 = (s16)pBlock->unk334 >> (2 - pBlock->unk338),
//    truncated to u16 in the 0x10(sp) local (sh and lhu of that slot).
// 5. loop for (i = 0; i < 8; i++, pPrim++), exit test slti i,8 with both
//    increments (0x28 each) in the delay slots. Per iteration the vertex
//    stores are sh +0x8, sh +0xA, sb +0xC, sb +0xD, sh +0x10, sh +0x12,
//    sb +0x14, sb +0x15, then the area +0x18/+0x1A/+0x1C/+0x1D and
//    +0x20/+0x22/+0x24/+0x25, with the u16 tpage result at +0x16.
//    So the 0x28-byte record is four vertices of { s16 x; s16 y; u8 u; u8 v }
//    at 0x8/0x10/0x18/0x20 plus a u16 tpage at 0x16 -- not a POLY_FT4; the
//    coordinates are 16-bit, which is why every coordinate store is sh.
//    Vertex u = the masked/wrapped column, v = (u8)pBlock->unk33A plus
//    (u8)pBlock->unk32C for the two lower vertices (both reread per use).
// 6. tpage = GetTPage((s16)pBlock->unk338, 0, ((s16)col / 64) * 64,
//                     ((s16)pBlock->unk336 / 256) * 256);
//    the (v < 0 ? v + 0x3F : v) >> 6 << 6 and (v < 0 ? v + 0xFF : v) >> 8 << 8
//    sequences are signed divide-then-multiply, so write the division out.
//    unk338 is lh (s16) and is used both as the tp argument and as the shift
//    amount of item 4; unk334 is lhu, unk336 is lh.
// 7. AddPrim(pOT, pPrim): the retail lw/lw/and 0xFF000000/and 0xFFFFFF/or/sw
//    pair IS libgs' AddPrim macro, so arg5 is a u_long* OT slot (reloaded from
//    the stack around GetTPage because it is call-clobbered).

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2_r1", func_800278F8);
// PARKED 86.25% (1072 bytes against retail's 1096; 268 instructions against
// 274; 148 differing lines against the previous park's 205). Measured live this
// round under the unit's preset (XenoGraphicsLibrary, gcc 2.6.0 -O2 -G8).
//
// WHAT CLOSED 75.62 -> 86.25: `col = remainder;` belongs AFTER the prim store
// block, not next to the modulo. Retail's per-iteration col copy (`move s3,a1`
// from the mfhi result) sits in the delay slot of the texColumn /64 `bgez`,
// i.e. its source assigned the running column after the eight vertex stores.
// That single move also restores retail's callee-saved pair (col $s3 / pPrim
// $s4), which EVERY earlier body had mirrored - and the mirror was what the old
// note called "5 copies plus a block order": with the pair right, 57 of the 205
// differing lines disappear on their own. Move this statement back and it
// measures 75.62 again; the position is the whole difference.
//
// Measured dead this round (do not repeat):
//  - rows diamond as `rows = 0; if (top >= 0 && top <= unk32C+0xF0) { rows =
//    ...; rowStart = 0; } else { rowStart = 0; }` 78.22 / 1076 (161 lines); the
//    plain hoisted form `rows = 0; if (...) rows = ...; rowStart = 0;` is 75.15
//    in the old context. The if/else with both arms assigning IS right, but its
//    two delay slots and retail's `mflo v0 / nop / move t2,v0` copy are not
//    reached by it: routing the quotient through the `quotient` local measures
//    BYTE-IDENTICAL (86.25), cc1 folds it back, so rows' copy needs the live
//    range a second variable gives, not a renamed temp.
//  - statement order inside the loop (u/width before vs after texColumn) is
//    byte-identical at every position tried, as is the embedded-assignment form
//    `width = 0x100 - (u = ...)`.
//  - declaration order of `pPrim` (first local vs last) is byte-identical.
//  - col fixup as an if/else: 74.36 / 1072 (re-confirmed) - and it also moves
//    the divisor register a2 -> v1 and `bank` t1 -> a2, so it is not a cheap
//    lever either.
//
// The remaining 6 instructions are all retail copies plus one narrowing:
//  - the u/width block: retail emits texColumn FIRST, keeps the literal 0x100 in
//    $v0 and copies both results (`move a0,v0` width, `move a3,v1` u, so u ends
//    in $a3 and width in $a0); this body hoists `li a0,0x100` to the block top,
//    emits u/width first and leaves width in $a2. That is sched1 ordering by
//    insn priority (levers.md c-a-second-loads-position-inside-a-block-is-not-
//    source-order), not statement order.
//  - the rows diamond's `move t2,v0` + `move t0,zero` pair and its two delay
//    slots (below).
//  - the uOffset hoist: this body narrows with an extra `sll 16 / sra 16` before
//    the sllv where retail shifts the raw modulo result.
// Facts carried over from the earlier notes that are still true:
//  - the 0x28 record IS a POLY_FT4 (psyq layout: clut 0xE, tpage 0x16, pad1
//    0x1E, pad2 0x26), so the store set names its own fields.
//  - retail's clamp-path tail is `mult / mflo / bgez / addiu 0xFF / SRL 8`
//    where this body emits a second `sra`; srl and sra agree in the low 16 bits,
//    so retail's source narrowed that intermediate to 16 bits first. The
//    remaining difference in that arm, and every other row of it matches.
//  - the rows diamond MUST be an if/else with both arms assigning, and the col
//    fixup must stay the single-assignment form (levers.md round 394/454).
//  - older-body dead ends, all still true: gcc-2.7.2 66.10, 2.7.2-cdk 69.97,
//    -O1 69.11, -O3 74.87, `s16 uOffset` with yTop dropped 70.86, `(u16)` on the
//    uOffset modulo 74.31 (drops one narrowing but mirrors a row).
//
// NEXT: this body is structurally right at 86.25 and is above the 85% line the
// permuter needs, but the redo lane only allowed a permuter run at 99%+, so it
// has still NEVER been permuted - that is the cheapest next step, followed by a
// `-dl`/`-dg` dump for the u/width/tpage ordering.
//
/*
void func_800278F8(Temp2RenderBlock* pBlock, s32 left, s32 top, s32 span,
                   u_long* pOT, s32 bank) {
    POLY_FT4* pPrim;
    s32 quotient;
    s32 half;
    s32 remainder;
    s32 col;
    s32 rows;
    s32 rowStart;
    s32 rowEnd;
    s32 u;
    s32 width;
    s32 height;
    s32 texColumn;
    s32 i;
    s32 yTop;
    s32 col16;
    u16 uOffset;

    quotient = (pBlock->unk328 << 8) / (span + 0x100);
    half = (0x140 - quotient) / 2;
    half += (half * span) / 0x100;
    remainder = (s16)(left - half) % pBlock->unk328;
    col = remainder;
    if ((s16)remainder < 0) {
        col = (u16)pBlock->unk328 + remainder;
    }
    if (top < 0 || top > pBlock->unk32C + 0xF0) {
        rows = 0;
        rowStart = 0;
    } else {
        rows = (pBlock->unk32C << 8) / (span + 0x100);
        rowStart = 0;
    }
    pPrim = (POLY_FT4*)((u8*)pBlock + (bank & 1) * 0x140);
    if ((s16)rows <= 0) {
        return;
    }
    yTop = top;
    uOffset = (pBlock->unk334 % 64) << (2 - pBlock->unk338);
    for (i = 0; i < 8; i++) {
        col16 = (s16)col;
        texColumn = (u16)pBlock->unk334 + (col16 >> (2 - pBlock->unk338));
        u = (col + uOffset) & ((0x100 >> pBlock->unk338) - 1);
        width = 0x100 - u;
        if (pBlock->unk328 < col16 + (s16)width) {
            width = (u16)pBlock->unk328 - col;
        }
        height = ((s16)width << 8) / (span + 0x100);
        if ((s16)rowStart + (s16)height >= 0x141) {
            height = 0x140 - rowStart;
            width = ((s16)height * (span + 0x100)) / 256;
        }
        remainder = (s16)(col + width) % pBlock->unk328;
        rowEnd = rowStart + height;
        pPrim->x0 = rowStart;
        pPrim->x1 = rowEnd;
        pPrim->x2 = rowStart;
        pPrim->y2 = yTop;
        pPrim->x3 = rowEnd;
        pPrim->y3 = yTop;
        pPrim->u0 = u;
        pPrim->y0 = top - rows;
        pPrim->y1 = top - rows;
        pPrim->u1 = width + u - 1;
        pPrim->v0 = pBlock->unk33A;
        pPrim->u2 = u;
        pPrim->v1 = pBlock->unk33A;
        pPrim->u3 = width + u - 1;
        pPrim->v2 = pBlock->unk33A + (u8)pBlock->unk32C;
        pPrim->v3 = pBlock->unk33A + (u8)pBlock->unk32C;
        col = remainder;
        pPrim->tpage = GetTPage(pBlock->unk338, 0,
                                (s16)texColumn / 64 * 64,
                                pBlock->unk336 / 256 * 256);
        addPrim(pOT, pPrim);
        if ((s16)rowEnd >= 0x140) {
            break;
        }
        rowStart = rowEnd;
        pPrim++;
    }
}*/







void HeapFreeIfNotNull(void* pData) {
    if (pData) {
        HeapFree(pData);
    }
}
