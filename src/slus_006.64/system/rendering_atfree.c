#include "common.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

extern void func_8001E148(SpriteData* pSpriteData);
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void func_8001E9BC(SpriteData* pSpriteData, u_long* pOt);
extern void func_8001EE88(SpriteData* pSpriteData, u_long* pOt, s32 depth);
extern void func_8001F1D4(SpriteData* pSpriteData, u_long* pOt, s32 depth);
extern void SpriteApplyDirTransforms(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage);
extern void SpriteClearDirTransforms(SpriteData* pSpriteData);
extern void GfxQueueImageTransfer(u_long* pAddr, s32 x, s32 y, s32 width, s16 height);

// Rendering-related stuff

// The source rectangle of one animation frame on the animation texture, in
// pixels: the loop below fills x/y from the frame's descriptor entry and the
// palette upload fills the same fields with the clut's position and size.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 w;
    /* 0x6 */ u16 h;
} SpriteFrameRect; // Size: 0x8

// The four contiguous atfree stubs func_8001E3D8 .. func_8001F1D4, split out of
// system/rendering (config/slus_006.64.yaml) because their original object was built
// with cdk -G0: only that emits the addiu in the delay slot of the `jal free` they end
// with. SpriteAnimPackageTestFlag8000/GetFrameCount ride along - they sit inside the run.
extern MATRIX g_GfxBaseMatrix;
extern SVECTOR g_SpriteQuadCorners[4];
extern SVECTOR g_BattleSpriteQuadCorners[4];
extern u16 g_SpritePlaneMasks[16];
extern void* g_GfxCurWorkBuffer;
extern void* g_GfxCurWorkBufferEnd;

// SpriteData's two animation flag words are also read a byte at a time (the
// frame count, the plane mask), which the bitfields in field/actor.h cannot say.
typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ SpriteFlagWord flags_0x3C;
    /* 0x40 */ SpriteFlagWord flags_0x40;
} SpriteRenderFlagWords; // Size: 0x44

// The whole 0x3C flag word: its bit tests need the raw word, which the bitfields
// in field/actor.h only expose as separate fields.
#define SpriteFlags0x3C(pSpriteData) ((u32)((SpriteRenderFlagWords*)(pSpriteData))->flags_0x3C.flags)


/* HISTORY (rounds 346-364). This body measured 89.01% then; the measurements
   are still valid, and the body is now 92.59% - see the ROUND 497 note after
   the INCLUDE_ASM line for what changed since.
   The 89.01% park (1504 bytes; retail is 1508 = 377
   instructions, so the body is ONE instruction short). This is the ROUND 349
   base: the same body with the deltaHeight hoist reverted; the 89.45% / 1496
   byte variant (three short) is described in the notes below.
   Rounds 346-349 took the body 84.08 -> 86.78 -> 89.01/89.45 in single measured
   steps; read the residues below before changing anything: one of the earlier
   fixes is a MISCOMPILE, which is why the best body was not the best number.
   The tail (prim build, quad corners, RotTransPers4, uv, addPrim) is the same
   code as func_8001E9BC / func_8001EE88 / func_8001F1D4 in this file.

   Facts confirmed by measurement, each of which is load-bearing:
   - The frame loop is a DO/WHILE, not a `for`: retail has no top exit test
     (it enters at .L8001E494 straight from the preheader), and the `for` form
     makes gcc duplicate the loop exit test (+3 instructions, 1524 bytes).
   - The loop tail must be `} while (++i != <count>);` with NO `i++;` in the
     body: retail is `lw t4,i(sp); addiu t4,t4,1; bne t4,v0,.L; sw t4,i(sp)`
     (the store fills the branch delay slot). With `i++;` in the body and the
     bare `i != ...` test gcc stores i first, RELOADS it for the test and leaves
     a nop in the delay slot - 4 bytes longer, 0.9 points lower.
   - INDEXED access pFrameData[i] is REQUIRED. With `i++, pFrameData++` and
     `pFrameData->`, gcc re-bases the induction register to pFrameData+7 and
     every field goes negative; because the pointer is then also spilled the
     frame grows to 0xB0. With pFrameData[i] the frame is retail's exact 0x98
     and there is no re-basing (same finding as the func_8001E9BC note).
   - The mirror test `(flags_0xAC >> 2) & 1` must be HOISTED into a local
     (flipX) assigned BEFORE the two offset computations: retail loads pBase,
     field_0x40 and field_0xAC back to back and only then does the lb's. Reading
     the flag inside the `if` instead puts the 0xAC load after offsetX's shift
     and costs a load-delay nop (+4 bytes).
   - `pTransform->translationX != 0 || pTransform->translationY != 0` DOES
     compile to retail's single `lhu 0(a0)` plus `bnez` (gcc merges the two
     adjacent s8 loads), so the realistic spelling is right.
   - The scaling must stay `y = y * scale / 0x1000;` - that is what emits
     retail's `bgez / addiu 0xFFF / sra 12` pair, and y (translationY) is
     computed before x (translationX), both negated by bit 3 of field_0x3C.
   - ONE `lastPlane` variable carries both the current and the remembered plane
     (initialised -1, compared against `pFrameData[i].flags & 7`): retail keeps
     it in one register and indexes the plane mask, the transform table and the
     tail's `<< 2` from it. Separate `mode`/`prevMode` locals cost a second
     callee-saved register and spill prevMode to the stack.
   - The 0x3C bit tests need the WHOLE u32 word: `(field_0x3C_0 >> 3) & 1` on
     the actor.h bitfield emits `and 0xFFFFF` + `sra` with the 0xFFFFF constant
     spilled and reloaded, where retail is `lw / srl 3 / andi 1`. Hence the
     SpriteFlags0x3C macro above.
   - The frame-flag tests need a u32 cast: `pFrameData[i].flags >> 4 & 1` on the
     s32 field gives `sra`, retail has `srl` (bits 4 and 5 both).
   - offsetX/offsetY (the sprite's scaled offsets, subtracted from the projected
     corners) and transX/transY (the layer transform's scaled translation, added
     to mat.t[]) must be SEPARATE locals: both pairs are live at the same time
     in retail (s6/fp versus s0/s1) and reusing one pair is also a semantic bug.
   - `n` must be recomputed in every block that shifts by it (retail re-reads
     field_0x40 in the matrix block and again in the quad block). One `n`
     assigned before the loop is loop-invariant, so it takes a callee-saved
     register and grows the frame.
   - the 0xFFFFFF mask (addPrim) is hoisted to a callee-saved register by both.
   - The QUAD-CORNER x stores of element 0 go through the SYMBOL in retail
     (`lui t4,%hi` + `sh %lo(g_BattleSpriteQuadCorners)($t4)`) in the x arm and
     in the offset block, while the y stores and elements 1-3 use the pVerts
     register. The two ARM stores can be spelled `g_BattleSpriteQuadCorners[0].vx`
     (worth +2.8 points) - but the OFFSET-BLOCK one CANNOT: with that spelling
     cc1 2.7.2 replaces the `&pVerts[2]` argument of RotTransPers4 with
     `la $6,g_BattleSpriteQuadCorners` (element 0 instead of 2), i.e. a silent
     miscompile that objdiff cannot see because the operand is a relocation.
     It scored 88.05% at 1512 bytes and is WRONG CODE - do not re-add it.
   What is still wrong, measured (this body, 86.78% / 1504 = one instruction
   short):
   - retail copies each shifted width/height/tileX/tileY into a second register
     unconditionally (`move t0,t2`) and negates THAT copy in the bit-3/bit-4
     arms. Writing the negation into separate drawWidth/drawHeight/drawTileX/
     drawTileY locals reproduces the width and height copies, but gcc still
     folds the tileX/tileY copies away (it negates those in place and uses the
     free slot for a nop). Those two `move`s plus the element-0 offset-block
     `lui` are the missing instructions.
   - the plane-mask read: retail forms the array base in a general register
     (`lui v1,%hi` + `addiu v1,v1,%lo`) and then `addu v0,index,v1`; this C
     folds the index into the load through $at (`sll` + `lui $at` + `addu $at`),
     which also costs the beq delay slot (retail puts the `lui` there). An
     unbounded-array extern and a `u16 *pMaskTable = g_SpritePlaneMasks;` local
     were both measured: identical 86.78% / 1504, gcc folds them back.
   - the transform block's `n` is computed first here and after the first lb in
     retail (`lb translationY` / `lw field_0x40` / `lb translationX` /
     `lw field_0x3C`), which is the one remaining extra nop in the preheader.
   - the saved-register assignment is a rotation of retail's: retail is
     s4=pSpriteData, s3=pFrameData, s2=pVerts, s1=transY, s0=transX, s5=mask,
     s6=offsetX, s7=lastPlane, s8=offsetY; this body has s3=pSpriteData,
     s2=pFrameData, s1=pVerts, s4=transY, s0=transX, s5=mask, s6=lastPlane,
     s7=offsetX, s8=offsetY. That rotation is most of objdiff's remaining 13%.
   - ROUND 348: THE PERMUTER RUNS NOW. It died on the pTransform spelling
     `(SpriteDirectionTransform*)(lastPlane * sizeof(SpriteDirectionTransform) +
     (mem_addr)pDirTransforms)`: decomp-permuter re-prints the AST and its printer
     turns `sizeof(Typedef)` into a cast, so cc1 reports "conversion to
     non-scalar type requested" on text that was fine. The spelling
     `&pSpriteData->pBase->pDirTransforms->dirTransforms[lastPlane]` (and the
     same expression for rot.vy/rot.vz, which is what retail's re-read of
     pDirTransforms shows) parses and costs nothing (86.78 -> 86.74).
   - ROUND 348 measured, each re-diffed: splitting the offsetX shift into two
     statements (offsetX = the lb, then offsetX <<= n; offsetY stays one
     statement) 86.74 -> 87.86; then scaling transX BEFORE transY (the opposite
     of the order retail's `mult s1,v1` shows) 87.86 -> 89.01; then hoisting
     `deltaHeight = pFrameData[i].deltaHeight;` above the tpage store and
     using `(s8)deltaHeight` 89.01 -> 89.45 at 1496 bytes, i.e. +0.44 for -8
     bytes (reverting it gives 89.01 / 1504, ONE instruction short - the better
     base if the missing instructions matter more than the score). Hoisting
     deltaWidth the same way is 87.72 - do not.
   - ROUND 348 permuter runs, each from the body it produced: 5262 -> 4687 ->
     4112 -> 3802 -> 3514 (its own score against retail's target.o). Everything
     it found after 3802 was WRONG CODE and was not ported: a pOt copy in the
     else addPrim arm (1452 bytes / 83.76, and it drops 13 instructions),
     pVerts[(spriteFlags >> 3) * 0], and drawTileX replaced by -tileX in the x
     arms (that negates unconditionally - a different result). So the next step
     is not another permuter run from this body.
   - ROUND 349 (all re-diffed, none kept): the four unconditional copies cannot
     be forced. Interleaving each copy right after its own shift (retail's
     spacing) is a no-op: same 89.01 / 1504. Reversing the copy order (tile
     copies first) is 88.25 / 1500. Splitting the tile shifts into two
     statements is 86.77 / 1500. ONE shared scratch variable carrying both tile
     copies (tileCopy = tileX; drawTileX = tileCopy; then again for tileY, the
     round-348 sibling lever) keeps 89.01 / 1504 and gcc coalesces the scratch
     away exactly as it does drawTileX/drawTileY. Read the surviving copies as
     the allocator's answer to liveness: drawWidth and drawHeight survive
     because `width`/`height` are REASSIGNED later in the uv block, while
     tileX/tileY are never reassigned, so cc1 puts drawTileX and tileX in one
     register and negates in place (`negu a1,a1`). Retail's own `negu t0,t2`
     shows the negation reading the SOURCE variable, so the source really is
     `drawX = -x;` for all four.
   - The one missing instruction is the `move v1,a3` (drawTileY) that retail
     puts in the second `beqz` delay slot; this body's `move a0,a2` slot holds a
     maspsx load-delay nop instead. Getting both tile copies would add two
     instructions, i.e. 1512, so one instruction elsewhere also differs (this
     body emits nop plus a second beqz pair in the loop preheader where retail
     fills both delay slots).
   - NOT A PRESET: under gcc-2.7.2-cdk-psx this body is 90.15, not 100.
   - ROUND 364 (un-parked, 5 measured edits, parked at 89.06 / 1500 = TWO
     instructions short, retail 1508):
     (a) THE `s32`-FOR-`int` LEVER IS A NO-OP IN THIS UNIT: `typedef signed int
     s32` (include/types.h), so `s32` and `int` are the same type and cc1 cannot
     see the difference. Every integer local here is already effectively `s32`.
     (b) `lastPlane = -1;` must sit ABOVE the two early returns (the resource
     check and `count == 0`), not below them: retail fills the FIRST `beqz`
     delay slot with `li s7,-1`, and only then does the loop preheader lose the
     `nop` this body used to emit. 89.01/1504 -> 88.89/1500.
     (c) With (b) in place, `i = 0;` must come BEFORE `pVerts =
     g_BattleSpriteQuadCorners;` (89.06/1500). The other order also gives 1500
     but 88.89, because the `li $21,0xFFFFFF` that loop.c hoists out of the
     addPrim bitfield store (P_TAG.addr is a 24-bit bitfield, which is where the
     `and 0xFF000000` comes from) then lands in the delay slot. In retail it
     sits after `la s2` - so with (b)+(c) every other instruction of segment 0
     lines up and the ONLY residual there is that one hoisted `li`.
     (d) Declaring tileX/tileY/drawTileX/drawTileY `u32` instead of `s32`:
     identical 89.06/1500 (same width, so the same RTL).
     (e) A `while (1) { ...; if (++i == count) break; }` dispatch instead of the
     do/while: identical size, 89.05 - the invariant hoist does not move.
     WHAT THE TWO MISSING INSTRUCTIONS ARE, now proven with cc1_probe: the
     `addu a0,a2,zero` (drawTileX = tileX) and `addu v1,a3,zero` (drawTileY =
     tileY) copies. A standalone cc1_probe replica of the block emits all four
     copies in retail's exact positions - the copy right after each
     shift, the LAST one in the `beqz` delay slot - as soon as tileX/tileY stay
     LIVE past the flip block; with them dead after it cc1 coalesces
     drawTileX/tileX into one register and negates in place, which is what this
     body does. So the residual is exactly the round-349 liveness reading:
     drawWidth/drawHeight keep their copies because `width`/`height` are
     REASSIGNED in the uv block, and nothing in retail's code reads tileX/tileY
     after the negations (a2 dies at `negu a0,a2`, a3 at `negu v1,a3`), so no
     honest source makes them live. Do not re-try the four copy spellings or the
     permuter (four runs in round 348; everything past 3802 was wrong code).
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering_atfree", func_8001E3D8);

/* Best attempt (round 497): 92.59% (1496 bytes; retail is 1508 = 377 insns,
   this body 374 = THREE short). Round 497 found four more wins on top of the
   round-486 body (88.72 -> 92.59), each measured on its own:
   1. +0.04 (88.72 -> 88.76): spell the direction transform INDEX-FIRST,
      `(SpriteDirectionTransform*)(lastPlane * sizeof(SpriteDirectionTransform) +
      (mem_addr)pSpriteData->pBase->pDirTransforms)`, for pTransform and for the
      rot.vy/rot.vz re-reads. That is retail's `addu a0,a1,v0` (index first);
      `&...->dirTransforms[lastPlane]` emits it base-first.
   2. +0.03 (88.76 -> 88.79): `size = count * sizeof(POLY_FT4);` as its own
      statement before the work-buffer check; the `lui %hi(g_GfxCurWorkBuffer)`
      then lands AFTER the count*0x28 shift chain, where retail has it.
   3. +2.40 (88.79 -> 91.19): DELETE THE `pVerts` LOCAL - THE SAME LEVER THAT
      TOOK func_8001E9BC AND func_8001EE88 (write every corner as
      g_BattleSpriteQuadCorners[k]). The saved set is then retail's EXACTLY:
      s0=transX s1=transY s2=corner base (formed lui+addiu from the symbol)
      s3=pFrameData s4=pSpriteData s5=mask s6=offsetX s7=lastPlane s8=offsetY.
      The round-486 warning about this spelling is about the element-0 stores
      ONLY, and with the local gone they come out as retail's
      `lui t4,%hi` + `sh a0,0(t4)`, no miscompiled RotTransPers4 argument.
   4. +1.40 (91.19 -> 92.59): inline the shift amount in all four
      width/height/tile statements (`<< ((pSpriteData->field_0x40 >> 8) & 0x1F)`)
      instead of one shared `n = ...` statement. Same byte count, and it is what
      puts field_0x40's load inside the width chain.
   Measured dead this round, each a full scored run from this body: deltaWidth/
   deltaHeight locals before the widths (89.77 / 1464, eight short); scaling
   transY before transX (91.88, 364 differing lines - retail's `mult s1,v1`
   order is NOT the better body here); `drawWidth = -drawWidth;` in the arms
   (identical RTL - coalescing makes the spelling a no-op, 92.59); interleaving
   the four draw copies right after their own shifts (90.99).
   Presets screened on THIS body, so do not redo: gcc-2.6.0 84.70,
   gcc-2.7.2 86.72, cdk -O3 92.59 (identical to -O2). cdk -G0 -O2 is right.
   What is still missing, in size order:
   (a) TWO of retail's four unconditional draw copies. Retail is
       `move t0,t2` (drawWidth, after the width shift), `move t3,t1`
       (drawHeight), `move a0,a2` (drawTileX) and `move v1,a3` (drawTileY, in
       the second beqz delay slot); this body keeps only the drawWidth one and
       negates the other three in place. Same finding as round 349/486: the
       copies are a local-alloc coalescing decision, and none of the four
       spellings (separate locals, self-negation, interleaved assignment,
       u32 locals) changes it.
   (b) retail forms `%hi(g_BattleSpriteQuadCorners)` a THIRD time in the
       corner-offset block (`lhu v0,0(t4)` / `sh v0,0(t4)`); this body reuses
       the s2 base for element 0 there. One instruction.
   (c) retail re-forms the g_GfxCurWorkBuffer store address through $at
       (`lui at,%hi` + `sw v0,0(at)`); this body stores through the base
       register it loaded (`lw s0,0(v1)` / `sw v0,0(v1)`). One instruction.
   (d) the prim block's load order: retail issues lbu deltaWidth, lbu
       deltaHeight, lbu width and only then extends; this body is lbu
       deltaWidth, lbu width, lw field_0x40, lbu deltaHeight.
   (e) retail fills the beqz delay slot after `lbu visible`(0x68) with the
       `sb` of visible; this body stores it before the branch.
   THE FRAME IS THE ONE STRUCTURAL CLUE LEFT: retail's frame is 0x98 and has 12
   NEVER-TOUCHED bytes at 0x50-0x5B, between `rot`(0x48) and the RotTransPers4
   out-params (z 0x5C, flag 0x60, i 0x64, visible 0x68); this body's frame is
   0x88 with no hole. A 12-byte (VECTOR-sized) local there is the only layout
   that explains it, and the hole means retail's allocator needed and then lost
   three spill slots - consistent with its three extra instructions.
*/
/*
void func_8001E3D8(SpriteData* pSpriteData, void* pArg) {
    SpriteFrameData* pFrameData;
    SpriteDirectionTransform* pTransform;
    POLY_FT4* pPrim;
    MATRIX* pMatrix;
    MATRIX mat;
    SVECTOR rot;
    u_long* pOt;
    s32 count;
    s32 size;
    s32 i;
    s32 n;
    s32 lastPlane;
    s32 offsetX;
    s32 offsetY;
    s32 flipX;
    s32 transX;
    s32 transY;
    s32 tileX;
    s32 tileY;
    s32 edgeX;
    s32 edgeY;
    s32 drawWidth;
    s32 drawHeight;
    s32 drawTileX;
    s32 drawTileY;
    s32 width;
    s32 height;
    s32 texX;
    s32 texY;
    s32 z;
    s32 flag;
    u32 spriteFlags;
    u8 visible;

    flipX = ((u32)pSpriteData->flags_0xAC.flags >> 2) & 1;
    offsetY = (s8)pSpriteData->pBase->offsetY;
    offsetY = offsetY << ((pSpriteData->field_0x40 >> 8) & 0x1F);
    offsetX = (s8)pSpriteData->pBase->offsetX;
    offsetX = offsetX << ((pSpriteData->field_0x40 >> 8) & 0x1F);
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    if (flipX) {
        offsetX = -offsetX;
    }
    count = (pSpriteData->field_0x40 >> 2) & 0x3F;
    lastPlane = -1;
    size = count * sizeof(POLY_FT4);
    if ((u8*)g_GfxCurWorkBuffer + size >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }
    i = 0;
    do {
        // frames are grouped by layer: only the first frame of a run re-picks the
        // direction transform (and so the matrix the rest of the run is drawn with)
        if ((pFrameData[i].flags & 7) != lastPlane) {
            lastPlane = pFrameData[i].flags & 7;
            visible = (g_SpritePlaneMasks[lastPlane] & ((SpriteRenderFlagWords*)pSpriteData)->flags_0x3C.bytes[1]) == 0;
            pTransform = (SpriteDirectionTransform*)(lastPlane * sizeof(SpriteDirectionTransform) +
                                                   (mem_addr)pSpriteData->pBase->pDirTransforms);
            if (pSpriteData->pBase->pDirTransforms != NULL &&
                (pTransform->translationX != 0 || pTransform->translationY != 0 || pTransform->rotationZ != 0)) {
                n = (pSpriteData->field_0x40 >> 8) & 0x1F;
                transY = (s8)pTransform->translationY << n;
                transX = (s8)pTransform->translationX << n;
                if ((SpriteFlags0x3C(pSpriteData) >> 3) & 1) {
                    transX = -transX;
                }
                transX = transX * pSpriteData->scale / 0x1000;
                transY = transY * pSpriteData->scale / 0x1000;
                rot.vx = pTransform->rotationX;
                rot.vy = ((SpriteDirectionTransform*)(lastPlane * sizeof(SpriteDirectionTransform) +
                                                     (mem_addr)pSpriteData->pBase->pDirTransforms))->rotationY;
                rot.vz = ((SpriteDirectionTransform*)(lastPlane * sizeof(SpriteDirectionTransform) +
                                                     (mem_addr)pSpriteData->pBase->pDirTransforms))->rotationZ;
                if ((SpriteFlags0x3C(pSpriteData) >> 3) & 1) {
                    rot.vz = -rot.vz;
                }
                RotMatrix(&rot, &mat);
                mat.t[0] = pSpriteData->pBase->transformMatrix.t[0] + transX;
                mat.t[1] = pSpriteData->pBase->transformMatrix.t[1] + transY;
                mat.t[2] = pSpriteData->pBase->transformMatrix.t[2];
                SetMulMatrix(&pSpriteData->pBase->transformMatrix, &mat);
                pMatrix = &mat;
            } else {
                SetRotMatrix(&pSpriteData->pBase->transformMatrix);
                pMatrix = &pSpriteData->pBase->transformMatrix;
            }
            SetTransMatrix(pMatrix);
        }
        if (visible == 0) {
            continue;
        }

        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;
        setlen(pPrim, 9);
        *(u32*)&pPrim->r0 = pFrameData[i].colorAndCode;
        pPrim->tpage = pFrameData[i].texPage;
        pPrim->clut = pFrameData[i].clutID;
        width = (s16)(pFrameData[i].width + (s8)pFrameData[i].deltaWidth) << ((pSpriteData->field_0x40 >> 8) & 0x1F);
        height = (s16)(pFrameData[i].height + (s8)pFrameData[i].deltaHeight) << ((pSpriteData->field_0x40 >> 8) & 0x1F);
        tileX = pFrameData[i].tileX << ((pSpriteData->field_0x40 >> 8) & 0x1F);
        tileY = pFrameData[i].tileY << ((pSpriteData->field_0x40 >> 8) & 0x1F);
        spriteFlags = SpriteFlags0x3C(pSpriteData);
        drawWidth = width;
        drawHeight = height;
        drawTileX = tileX;
        drawTileY = tileY;
        if ((spriteFlags >> 3) & 1) {
            drawWidth = -width;
            drawTileX = -tileX;
        }
        if ((spriteFlags >> 4) & 1) {
            drawHeight = -height;
            drawTileY = -tileY;
        }

        edgeX = drawTileX + drawWidth;
        if (!((u32)pFrameData[i].flags >> 4 & 1)) {
            g_BattleSpriteQuadCorners[0].vx = drawTileX;
            g_BattleSpriteQuadCorners[1].vx = edgeX;
            g_BattleSpriteQuadCorners[2].vx = edgeX;
            g_BattleSpriteQuadCorners[3].vx = drawTileX;
        } else {
            g_BattleSpriteQuadCorners[0].vx = edgeX;
            g_BattleSpriteQuadCorners[1].vx = drawTileX;
            g_BattleSpriteQuadCorners[2].vx = drawTileX;
            g_BattleSpriteQuadCorners[3].vx = edgeX;
        }

        edgeY = drawTileY + drawHeight;
        if (!((u32)pFrameData[i].flags >> 5 & 1)) {
            g_BattleSpriteQuadCorners[0].vy = drawTileY;
            g_BattleSpriteQuadCorners[1].vy = drawTileY;
            g_BattleSpriteQuadCorners[2].vy = edgeY;
            g_BattleSpriteQuadCorners[3].vy = edgeY;
        } else {
            g_BattleSpriteQuadCorners[0].vy = edgeY;
            g_BattleSpriteQuadCorners[1].vy = edgeY;
            g_BattleSpriteQuadCorners[2].vy = drawTileY;
            g_BattleSpriteQuadCorners[3].vy = drawTileY;
        }

        g_BattleSpriteQuadCorners[0].vy -= offsetY;
        g_BattleSpriteQuadCorners[1].vy -= offsetY;
        g_BattleSpriteQuadCorners[2].vy -= offsetY;
        g_BattleSpriteQuadCorners[3].vy -= offsetY;
        g_BattleSpriteQuadCorners[0].vx -= offsetX;
        g_BattleSpriteQuadCorners[1].vx -= offsetX;
        g_BattleSpriteQuadCorners[2].vx -= offsetX;
        g_BattleSpriteQuadCorners[3].vx -= offsetX;
        RotTransPers4(&g_BattleSpriteQuadCorners[0], &g_BattleSpriteQuadCorners[1], &g_BattleSpriteQuadCorners[2], &g_BattleSpriteQuadCorners[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
                      (long*)&pPrim->x3, (long*)&pPrim->x2, &z, &flag);

        texX = pFrameData[i].texX;
        texY = pFrameData[i].texY;
        width = pFrameData[i].width - 1;
        height = pFrameData[i].height - 1;
        if (pPrim->x3 < pPrim->x0) {
            if (--texX < 0) {
                texX = 0;
                width--;
            }
        }
        pPrim->u0 = texX;
        pPrim->v0 = texY;
        pPrim->u1 = texX + width;
        pPrim->v1 = texY;
        pPrim->u2 = texX;
        pPrim->v2 = texY + height;
        pPrim->u3 = texX + width;
        pPrim->v3 = texY + height;

        if ((SpriteFlags0x3C(pSpriteData) >> 27) & 1) {
            pOt = (u_long*)((u8*)pArg - lastPlane * 4);
            addPrim(pOt, pPrim);
        } else {
            addPrim((u_long*)pArg, pPrim);
        }
    } while (++i != ((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2);
}
*/




/* Best attempt: 80.03% (1200 bytes, the original is 1196; round 294 took the
   parked 76.49% body down to this by the two fixes marked FIXED). The rest of
   the body is a verified transcription; what is left is the allocator's
   choice of which nine values get s0-s8, and two statement spells.
   Facts confirmed against the .s:
   - The prim IS a POLY_FT4 (this tree's libgpu.h has the shipped per-corner
     layout: u0/v0/clut at 0xC, u1/v1/tpage at 0x14, u2/v2/pad at 0x1C,
     u3/v3/pad at 0x24, sizeof 0x28). setlen(pPrim, 9) is the tag len byte at
     3 and setcode 0x2C at 7 -> `setPolyFT4(pPrim)` then `setRGB0(pPrim,0,0,0)`.
   - 0x8004FAD8 is NOT g_BattleSpriteQuadCorners (that one is 0x8004FB98).
     It is 4 SVECTORs of scratch (g_SpriteQuadCorners), written x into .vx and the
     screen-vertical into .vz because the quad lies in the XZ plane (vy is
     never written, the .sdata is zeroed). 0x8004FAF8 is a u16[16] of
     1,2,4..0x8000 used as the per-layer mask.
   - The sprite's plane byte is byte 1 of the 0x3C word (lbu 0x3D) and the
     frame count is byte 0 of the 0x40 word >> 2 (lbu 0x40); the shared header
     has bitfields/u32 there, so both need the local view above.
   - The frame count is re-read for the loop bound (lbu 0x40 each iteration) and
     compared with `bne i,count`, so write the expression in the for test, not
     the local.
   - INDEXED access is required: with `i++, pFrameData++` gcc re-bases the
     induction register to pFrameData+7 and every offset goes negative (1216
     bytes, 70.98%); `pFrameData[i].field` keeps the base and retail's literal
     offsets 0,2,4,6,7,8,9,0x14.
   - `visible` must stay in a register: the cast `(SpriteRenderFlagWords*)
     pSpriteData` has to be written inline at each use, not held in a pointer
     local (a live pointer local costs a callee-saved register, spills the flag
     to 0x78 and grows the frame to 0xA8).
   - FIXED this round: the frame data pointer belongs BEFORE the work-buffer
     early-out (`pFrameData = pSpriteData->pBase->pCurRenderFramesData;` above
     the first if) - retail loads pBase in the branch shadow, and the two nop
     load-delay slots this C otherwise leaves in the preheader go away.
   - FIXED this round: the quad edges need two source locals (`edgeX = tileX +
     width;` before the x if, `edgeY = tileY + height;` before the z if) used by
     BOTH stores of the arm. Writing `tileX + width` twice makes gcc emit the
     addu twice (2 extra bytes per arm pair); with the local, reorg puts one
     addu in the branch delay slot exactly as retail does.
   Measured and still wrong (each re-diffed):
   - retail emits width/height/n interleaved: width, then n, then width<<=n,
     then height, then height<<=n, then tileX, tileY - that order is worth
     74.90 -> 76.48 and is what the body below does.
   - the quad y stores are in the order [2],[3],[0],[1] in both arms.
   - (a) the saved-register set is the same nine (s0-s8, retail's pOt is in
     s8/fp) but assigned differently: retail spends one on `%hi(g_SpriteQuad
     Corners)` kept alive across the loop for the element-0 store
     (`sh a0,%lo(g_SpriteQuadCorners)(s7)`), this C spends it on addPrim's
     0xFFFFFF mask (`lui s4,0xff; ori s4,s4,0xffff` in the preheader) and leaves
     four nop load-delay slots in the uv block that retail fills with its
     in-loop `lui a1,0xff` / `ori` / `lui a0,0xff00`. Spelling element 0 as
     `g_SpriteQuadCorners[0].vx` to force the %hi form does NOT work - cse folds
     it into the pVerts register and the frame grows to 0xA8 (1208 bytes); all
     four elements through the global is worse still (0xA8, `visible` spilled).
   - (b) retail splits the g_GfxBaseMatrix address into `lui s0,%hi` + TWO
     `addiu ...%lo` (one for the 8-word MATRIX copy in a1, one for ApplyMatrix'
     argument in s0, in 3/3/2-word groups); this C CSEs the address into one
     callee-saved register and copies the matrix in 4/4 groups (the spare a1 is
     what makes the fourth temp available).
   - (c) retail copies each shifted width/height/tileX/tileY into a second
     register unconditionally (`addu t0,t1,zero`) and negates THAT in the arm;
     this C negates in place. Not reachable by source shape: a 4-minute permuter
     run from this body (1630 iterations, base 7669 -> best 7224) only found
     edits that negate unconditionally, i.e. wrong code.
   - (d) retail's plane-mask read forms the array base in a register
     (`lui/addiu` then `addu`), this C folds the index into the load through
     $at (one instruction shorter).
   - (e) the uv right/bottom edges are `addiu 255` applied to texX before the
     addu (`(u + 0xFF) + width`) in retail, `(u + width) - 1` here. The C that
     would emit it is `texX + 0xFF + width`, which no programmer writes, so it
     was left alone.
   - (f) the second y average uses `sra` in retail and `srl` here, i.e. retail
     divides a signed value in both pairs; `(s16)(y2+y3)/2` is not enough to
     make the second one signed.
*/

// Best attempt: 91.95% (1172 bytes; retail is 1196 = 299 instructions, so this
// body is SIX short: the four unconditional `move` copies of the width/height/
// tileX/tileY shifts, plus retail's `lui at,%hi(g_GfxCurWorkBuffer)` re-form).
// Round 493, measured under XenoRenderingAtfree (cdk -G0). The previous park was
// 86.39% / 1200 (ONE instruction OVER); this is the same transcription with four
// changes, each measured on its own, and its saved set is now retail's EXACTLY
// (s0-s8 all agree, hp/fp included).
//  - THE `pVerts = g_SpriteQuadCorners;` LOCAL MUST GO (the round-457 / round-488
//    lever, already proved on func_8001EE88 and func_8001F1D4 in this file): a
//    local that only mirrors a global base makes loop.c hoist addPrim's
//    0x00FFFFFF mask into a callee-saved register (lui/ori in the preheader) -
//    which was the whole saved-set rotation. As a raw score 86.39 -> 82.74 (it
//    was six short at that point too), but it makes s4=i s5=lastPlane
//    s6=visible s7=%hi(g_SpriteQuadCorners) s2=the full base, and retail's
//    element-0 store through s7 (`sh a0,%lo(g_SpriteQuadCorners)(s7)`) falls out
//    of `g_SpriteQuadCorners[0].vx` for free. The miscompile the func_8001E3D8
//    note warns about (`&pVerts[2]` becoming `la $6,g_SpriteQuadCorners`) does
//    NOT happen here: RotAverage4's four arguments still come from s2, as
//    `addiu aN,s2,8 / 0x10 / 0x18`.
//  - the uv right/bottom edges are `(texX + 0xFF) + width`, not
//    `(texX + width) - 1`: the same stored byte in a u8 field, and it fills the
//    `lbu` load-delay slots - 4 instructions and 4 nops, 1176 bytes, 91.68% with
//    the draw locals below. Residual there: gcc attaches the 0xFF to the second
//    operand (width) where retail attaches it to texX, so the two loads come in
//    the other order (8 lines, no size difference; explicit parens measure the
//    same).
//  - four `draw*` locals assigned unconditionally and negated in the arms
//    (`drawWidth = width; ... if (flip) drawWidth = -width;`, which is retail's
//    shape: copies before the tests, the last one in the first `beqz` delay
//    slot) 91.68 -> 91.72. The if/else spelling instead (`if (flip) drawWidth =
//    -width; else drawWidth = width;`) is 89.58 / 1196 - retail's EXACT size but
//    it emits `j` plus a second move pair, so it is the wrong shape; do not take
//    it for the byte count.
//  - `n = ((u32)...flags_0x40.flags >> 8) & 0x1F;`: SpriteFlagWord.flags is s32,
//    so without the cast the shift is `sra`; retail has `srl`. 91.72 -> 91.95.
// What is left, in size order (all measured this round):
//  - the four copies: gcc coalesces all four here, so the negations act in place
//    (`negu a3,a3` where retail has `negu t0,t1`), the two `beqz` delay slots hold
//    nops where retail puts `move a1,a3` / `srl v0,0x4`, and every quad-corner
//    store register differs. The flip block's instruction LIST is otherwise the
//    same; its load order is not (retail loads deltaWidth, deltaHeight, width;
//    this body deltaWidth, width, deltaHeight) and that is not spellable from the
//    statement order (see func_8001E3D8's note).
//  - retail loads `count` (lbu 0x40) BEFORE `lui %hi(g_GfxCurWorkBuffer)`; this
//    body loads it after (1 line).
//  - the `scale.vz = 0` store (`sw zero,104(sp)`) is emitted before the
//    `scale.vx` store here, after it in retail (1 line).
//  - g_GfxCurWorkBuffer is `lui v1 / lw s0,0(v1) / sw v0,0(v1)` here and
//    `lui s0 / lw s0,0(s0)` + `lui at / sw v0,0(at)` in retail: retail clobbers
//    its base with the load and re-forms the address through $at, which is this
//    body's one missing instruction in the prim block.
//  - the frame-flag word lives in t4 in retail and t1 here (naming only).
// Measured dead this round (do not repeat): the if/else draw spelling above;
// keeping `pVerts` while spelling element 0 through the symbol (CSE folds it
// back, frame 0xA8, `visible` spilled). Rounds 346-458 tried the loop shape,
// `s32` vs `int`, the lastPlane/i ordering, -O3, 2.6.0 and four permuter runs
// (everything past 3802 was wrong code); those apply unchanged, see the
// func_8001E3D8 note above.
// Best attempt (round 506): 94.07% (1176 bytes; retail is 1196 = 299 insns, this
// body 294 = FIVE short: retail's four unconditional draw copies plus the
// `lui at` re-form of g_GfxCurWorkBuffer). Measured under XenoRenderingAtfree
// (cdk -G0), starting from the 91.95% park; three edits, each measured alone:
// 1. +0.69 (91.95 -> 92.64): write the scale vector in ADDRESS order,
//    `scale.vx = ...; scale.vy = scale / 2; scale.vz = 0;`. With `scale.vz = 0`
//    second (the old body) cc1 emits the vz store BEFORE the vx store; in this
//    order both stores are retail's.
// 2. +0.78 (92.64 -> 93.42): fold the shift into the width/height statements
//    (`width = (s16)(width + deltaWidth) << n;` instead of `width = ...;`
//    `width <<= n;`) - same four instructions, 1172 -> 1176 bytes.
// 3. +0.65 (93.42 -> 94.07): inline the shift AMOUNT in all four statements
//    (`<< (((u32)flags_0x40.flags >> 8) & 0x1F)`) and DROP the `n` local. cc1
//    CSEs the four reads into retail's single `lw 64(s3)` yet keeps the four
//    shift computations separate, and the load lands between the width sum and
//    the width shift - retail's position. (The same lever round 497 measured for
//    func_8001E3D8.) Residual there: the `srl`/`andi` of n come before the width
//    sum here, after it in retail (1 line).
// Measured dead this round, each a full scored run from the 94.07 body:
// - the four draws as initialised declarations in a nested block
//   (`{ s32 drawWidth = width; ... }`): byte-identical, copy propagation deletes
//   the initialisers.
// - interleaving each draw copy right after its own shift (retail's spacing) with
//   the inline shifts: 93.58 / 1176.
// - the if/else spelling of both diamonds (`else { drawWidth = width;
//   drawTileX = tileX; }`) with the inline shifts: 92.09 / 1188 / 297 insns.
//   Retail has the copies BEFORE the test and no `j`, so it is the wrong shape
//   even though it is 2 instructions closer in size.
// - declaring the four draws `s16`: byte-identical 94.07, i.e. no cross-mode tie
//   to break here (the truncation folds away).
// What is left is the four copies and the `lui at`. Retail's are
// `move t0,t1 / move t3,t2 / move a0,a2 / move a1,a3` (the last in the FIRST
// `beqz` delay slot, which this body fills with a nop) with the negations reading
// the SOURCE registers (`negu t0,t1`); this body coalesces all four
// (`negu a3,a3`). Rounds 346-497 measured every spelling of the copies dead and
// both permuter-shaped families came back as wrong code; do not re-brief them.
/*
void func_8001E9BC(SpriteData* pSpriteData, u_long* pOt) {
    MATRIX mat;
    SVECTOR vec;
    VECTOR vecResult;
    VECTOR scale;
    SpriteFrameData* pFrameData;
    POLY_FT4* pPrim;
    u32 spriteFlags;
    u32 frameFlags;
    s32 count;
    s32 lastPlane;
    u8 visible;
    s32 i;
    s32 tileX;
    s32 tileY;
    s32 width;
    s32 height;
    s32 drawWidth;
    s32 drawHeight;
    s32 drawTileX;
    s32 drawTileY;
    s32 edgeX;
    s32 edgeY;
    s32 depth;
    s32 flag;

    mat = g_GfxBaseMatrix;

    vec.vx = pSpriteData->position.x >> 16;
    vec.vy = pSpriteData->position.y >> 16;
    vec.vz = pSpriteData->position.z >> 16;

    scale.vx = pSpriteData->scale;
    scale.vy = pSpriteData->scale / 2;
    scale.vz = 0;
    ScaleMatrixL(&mat, &scale);

    vec.vy = pSpriteData->field_0x84;
    ApplyMatrix(&g_GfxBaseMatrix, &vec, &vecResult);
    mat.t[0] += vecResult.vx;
    mat.t[1] += vecResult.vy;
    mat.t[2] += vecResult.vz;
    SetRotMatrix(&mat);
    SetTransMatrix(&mat);

    count = ((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2;
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    lastPlane = -1;
    if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }

    i = 0;
    do {
        frameFlags = pFrameData[i].flags;
        if (lastPlane != (frameFlags & 7)) {
            lastPlane = frameFlags & 7;
            visible = (g_SpritePlaneMasks[lastPlane] & ((SpriteRenderFlagWords*)pSpriteData)->flags_0x3C.bytes[1]) == 0;
        }
        if (visible == 0) {
            continue;
        }

        width = (s16)(pFrameData[i].width + (s8)pFrameData[i].deltaWidth) << (((u32)((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.flags >> 8) & 0x1F);
        height = (s16)(pFrameData[i].height + (s8)pFrameData[i].deltaHeight) << (((u32)((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.flags >> 8) & 0x1F);
        tileX = pFrameData[i].tileX << (((u32)((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.flags >> 8) & 0x1F);
        tileY = pFrameData[i].tileY << (((u32)((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.flags >> 8) & 0x1F);

        spriteFlags = ((SpriteRenderFlagWords*)pSpriteData)->flags_0x3C.flags;
        drawWidth = width;
        drawHeight = height;
        drawTileX = tileX;
        drawTileY = tileY;
        if ((spriteFlags >> 3) & 1) {
            drawWidth = -width;
            drawTileX = -tileX;
        }
        if (((spriteFlags >> 4) & 1) != ((frameFlags >> 5) & 1)) {
            drawHeight = -height;
            drawTileY = -tileY;
        }

        edgeX = drawTileX + drawWidth;
        if (!((frameFlags >> 4) & 1)) {
            g_SpriteQuadCorners[0].vx = drawTileX;
            g_SpriteQuadCorners[1].vx = edgeX;
            g_SpriteQuadCorners[2].vx = edgeX;
            g_SpriteQuadCorners[3].vx = drawTileX;
        } else {
            g_SpriteQuadCorners[0].vx = edgeX;
            g_SpriteQuadCorners[1].vx = drawTileX;
            g_SpriteQuadCorners[2].vx = drawTileX;
            g_SpriteQuadCorners[3].vx = edgeX;
        }

        edgeY = drawTileY + drawHeight;
        if (!((u32)pFrameData[i].flags >> 5 & 1)) {
            g_SpriteQuadCorners[2].vz = edgeY;
            g_SpriteQuadCorners[3].vz = edgeY;
            g_SpriteQuadCorners[0].vz = drawTileY;
            g_SpriteQuadCorners[1].vz = drawTileY;
        } else {
            g_SpriteQuadCorners[2].vz = drawTileY;
            g_SpriteQuadCorners[3].vz = drawTileY;
            g_SpriteQuadCorners[0].vz = edgeY;
            g_SpriteQuadCorners[1].vz = edgeY;
        }

        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;
        setPolyFT4(pPrim);
        setRGB0(pPrim, 0, 0, 0);

        RotAverage4(&g_SpriteQuadCorners[0], &g_SpriteQuadCorners[1], &g_SpriteQuadCorners[2], &g_SpriteQuadCorners[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
            (long*)&pPrim->x3, (long*)&pPrim->x2, &depth, &flag);

        pPrim->y0 = pPrim->y1 = (s16)(pPrim->y0 + pPrim->y1) / 2;
        pPrim->y2 = pPrim->y3 = (s16)(pPrim->y2 + pPrim->y3) / 2;
        pPrim->tpage = pFrameData[i].texPage;
        pPrim->clut = pFrameData[i].clutID;
        pPrim->u0 = pFrameData[i].texX;
        pPrim->v0 = pFrameData[i].texY;
        pPrim->u1 = pFrameData[i].texX + 0xFF + pFrameData[i].width;
        pPrim->v1 = pFrameData[i].texY;
        pPrim->u2 = pFrameData[i].texX;
        pPrim->v2 = pFrameData[i].texY + 0xFF + pFrameData[i].height;
        pPrim->u3 = pFrameData[i].texX + 0xFF + pFrameData[i].width;
        pPrim->v3 = pFrameData[i].texY + 0xFF + pFrameData[i].height;

        addPrim(pOt, pPrim);
    } while (++i != (((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2));
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering_atfree", func_8001E9BC);


s32 SpriteAnimPackageTestFlag8000(u8* pFlags) {
    return pFlags[1] >> 7;
}

s32 SpriteAnimPackageGetFrameCount(u16* pFlags) {
    return (*pFlags >> 9) & 0x3F;
}

/* Attempt (0.00%: cc1 emits an EMPTY body for this shape - the .s comes out as
   just a frame + j $ra, and maspsx then leaves the stale .o, which objdiff
   reports as "0.00% (? bytes)". Removing halves of the body does not change
   that, so the trigger is structural and unidentified. Everything below is a
   verified transcription of the assembly except for the two open questions.

   func_8001EE88 is the twin of func_8001F1D4 (same prologue, same RotAverage4
   call, mirrored clip arithmetic), so whichever gets matched first hands the
   other its shape.

   Verified from the .s (address order):
   - n = (field_0x40 >> 8) & 0x1F; z = arg3 << n; count = (field_0x40 >> 2) & 0x3F
   - early out when (u8*)g_GfxCurWorkBuffer + count*40 >= g_GfxCurWorkBufferEnd,
     then when count == 0; pFrameData = pBase->pCurRenderFramesData is loaded
     BEFORE the branches (a0 = pBase, lw 0x30(a0)).
   - loop bound is a BYTE reload: (*(u8*)((u8*)pSpriteData + 0x40)) >> 2, not
     the word-based `count`; pFrameData advances by 0x18 (sizeof SpriteFrameData).
   - pPrim = g_GfxCurWorkBuffer++ (POLY_FT4, 0x28); code byte 3 = 9; then
     *(u32*)&pPrim->r0 = pFrameData->colorAndCode; tpage = texPage; clut = clutID.
   - tileX(tileX s16), width = lbu(6) + (s8)lb(8) deltaWidth, tileY, height =
     lbu(7) + (s8)lb(9) deltaHeight, all <<= n; flips are field_0x3C bits 3 and 4
     (bit 3 negates width and tileX, bit 4 negates height and tileY).
   - then SpriteFrameData->flags 0x14 bit 4 selects the x quads: {tx, tx+w, tx+w,
     tx} or {tx+w, tx, tx, tx+w} into g_BattleSpriteQuadCorners[0..3].vx. Element 0 is stored
     through the SYMBOL (%lo(g_BattleSpriteQuadCorners)($t1)) and elements 1-3 through a pointer
     local (+8/+0x10/+0x18), which is what keeps the hi part of the address live
     across the call and produces the spill/reload at 0x30($sp).
   - yMin/yMax = min/max(tileY, tileY+height) via a sign test on height; if
     (z < yMin) continue; clip = (z < yMax) ? yMax - z : 0; height -= clip;
     if (height < 0) tileY -= clip; then flags bit 5 selects {ty, ty, ty+h, ty+h}
     or the reverse for .vy.
   - RotAverage4(g_BattleSpriteQuadCorners, +1, +2, +3, (long*)&pPrim->x0, &pPrim->x1, &pPrim->x3,
     &pPrim->x2, &zDepth, &flag) - note x3 before x2; the two stack locals are at
     sp+0x28 and sp+0x2C.
   - after the call: n is reloaded, s1 (clip) is sra'd by n, texY = lbu(5) loaded
     before the height>0 test (else arm also does texY -= s1), texHeight =
     lbu(7) - s1 in BOTH arms (gcc really duplicates the load);
     texX = lbu(4); if (pPrim->x3 < pPrim->x0) { if (--texX < 0) { texX = 0;
     width = lbu(6) - 1; } }
   - UV stores in this exact order: u1=u3=texX+width, u0=u2=texX, v0=v1=texY,
     v2=v3=texY+texHeight.
   - tail is addPrim(pOt, pPrim): tag = (tag & 0xFF000000) | (*pOt & 0xFFFFFF),
     then *pOt = (*pOt & 0xFF000000) | ((u32)pPrim & 0xFFFFFF).
   Needs #include "psyq/libgpu.h" for POLY_FT4 and
   `extern SVECTOR g_BattleSpriteQuadCorners[4];` (0x20 bytes, defined by
   asm/slus_006.64/data/3F290.sdata.s, so no data definition is needed here).

   extern SVECTOR g_BattleSpriteQuadCorners[4];

   void func_8001EE88(SpriteData* pSpriteData, u_long* pOt, s32 depth) {
          SpriteFrameData* pFrameData;
       POLY_FT4* pPrim;
       s32 n;
       s32 count;
       s32 z;
       s32 i;
       s32 zDepth;
       s32 flag;
       s32 tileX;
       s32 tileY;
       s32 width;
       s32 height;
       s32 texX;
       s32 texY;
       s32 texHeight;
       s32 clip;
       u32 spriteFlags;
       s32 yMin;
       s32 yMax;

       n = (pSpriteData->field_0x40 >> 8) & 0x1F;
       z = depth << n;
       count = (pSpriteData->field_0x40 >> 2) & 0x3F;
       pFrameData = pSpriteData->pBase->pCurRenderFramesData;
       if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
           return;
       }
       if (count == 0) {
           return;
       }

          for (i = 0; i < (*(u8*)((u8*)pSpriteData + 0x40) >> 2); i++, pFrameData++) {
           pPrim = g_GfxCurWorkBuffer;
           g_GfxCurWorkBuffer = pPrim + 1;
           pPrim->code = 9;
           *(u32*)&pPrim->r0 = pFrameData->colorAndCode;
           pPrim->tpage = pFrameData->texPage;
           pPrim->clut = pFrameData->clutID;
           tileX = pFrameData->tileX;
           width = pFrameData->width;
           width += (s8)pFrameData->deltaWidth;
           tileY = pFrameData->tileY;
           height = pFrameData->height;
           height += (s8)pFrameData->deltaHeight;
           n = (pSpriteData->field_0x40 >> 8) & 0x1F;
           width <<= n;
           height <<= n;
           tileX <<= n;
           spriteFlags = pSpriteData->field_0x3C;
           tileY <<= n;
           if (spriteFlags & 8) {
               width = -width;
               tileX = -tileX;
           }
           if (spriteFlags & 0x10) {
               height = -height;
               tileY = -tileY;
           }
           if (!(pFrameData->flags & 0x10)) {
               g_BattleSpriteQuadCorners[0].vx = tileX;
               g_BattleSpriteQuadCorners[1].vx = tileX + width;
               g_BattleSpriteQuadCorners[2].vx = tileX + width;
               g_BattleSpriteQuadCorners[3].vx = tileX;
           } else {
               g_BattleSpriteQuadCorners[0].vx = tileX + width;
               g_BattleSpriteQuadCorners[1].vx = tileX;
               g_BattleSpriteQuadCorners[2].vx = tileX;
               g_BattleSpriteQuadCorners[3].vx = tileX + width;
           }

           if (height > 0) {
               yMin = tileY;
               yMax = tileY + height;
           } else {
               yMin = tileY + height;
               yMax = tileY;
           }
           if (z < yMin) {
               continue;
           }
           if (z < yMax) {
               clip = yMax - z;
           } else {
               clip = 0;
           }
           height -= clip;
           if (height < 0) {
               tileY -= clip;
           }

           if (!(pFrameData->flags & 0x20)) {
               g_BattleSpriteQuadCorners[0].vy = tileY;
               g_BattleSpriteQuadCorners[1].vy = tileY;
               g_BattleSpriteQuadCorners[2].vy = tileY + height;
               g_BattleSpriteQuadCorners[3].vy = tileY + height;
           } else {
               g_BattleSpriteQuadCorners[0].vy = tileY + height;
               g_BattleSpriteQuadCorners[1].vy = tileY + height;
               g_BattleSpriteQuadCorners[2].vy = tileY;
               g_BattleSpriteQuadCorners[3].vy = tileY;
           }

           RotAverage4(pVerts, &g_BattleSpriteQuadCorners[1], &g_BattleSpriteQuadCorners[2], &g_BattleSpriteQuadCorners[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
               (long*)&pPrim->x3, (long*)&pPrim->x2, &zDepth, &flag);

           n = (pSpriteData->field_0x40 >> 8) & 0x1F;
           texY = pFrameData->texY;
           clip >>= n;
           if (height > 0) {
               texHeight = pFrameData->height - clip;
           } else {
               texY -= clip;
               texHeight = pFrameData->height - clip;
           }
           texX = pFrameData->texX;
           if (pPrim->x3 < pPrim->x0) {
               if (--texX < 0) {
                   texX = 0;
                   width = pFrameData->width - 1;
               }
           }
           pPrim->u1 = texX + width;
           pPrim->u3 = texX + width;
           pPrim->u0 = texX;
           pPrim->u2 = texX;
           pPrim->v0 = texY;
           pPrim->v1 = texY;
           pPrim->v2 = texY + texHeight;
           pPrim->v3 = texY + texHeight;

           pPrim->tag = (pPrim->tag & 0xFF000000) | (*pOt & 0xFFFFFF);
           *pOt = (*pOt & 0xFF000000) | ((u32)pPrim & 0xFFFFFF);
       }
   }
*/
// Best attempt: 93.85% (848 bytes; retail is 844 = 211 instructions, so ONE
//    instruction over). Round 457, measured under XenoRenderingAtfree (cdk -G0).
//    Load-bearing facts, each measured:
//    - The frame loop is a DO/WHILE: retail enters with no top test and the latch is
//      `addiu i,1 / lbu 0x40 / srl 2 / bne` with the frame-pointer advance in the
//      branch delay slot. The `for` form re-reads the byte in a second exit test at
//      the top (+4 instructions).
//    - INDEXED access pFrameData[i] (never `pFrameData++` with `pFrameData->`), or
//      gcc re-bases the induction register onto pFrameData+6.
//    - The pVerts LOCAL MUST GO: writing all four corners as
//      g_BattleSpriteQuadCorners[k] is 89.56 -> 93.85. With the local the array's
//      %hi is still formed but only feeds it, and loop.c then HOISTS addPrim's
//      0xFFFFFF mask into a callee-saved register (lui/ori in the preheader), which
//      spills pOt to 48(sp) and rotates the whole saved set.
//    - width must be re-loaded from pFrameData[i].width AFTER the RotAverage4 call
//      (it is not live across it), or width takes a callee-saved register.
//    - setlen(pPrim, 9) is the tag's len byte at offset 3, NOT pPrim->code (byte 7).
//    What is left (all measured): retail's saved set is s6=pSpriteData, s7=i, s5=z
//    where this body has s5=pSpriteData, s6=i, s7=z (a three-cycle; s0-s4, s8 and
//    fp already agree), plus one nop in the flip block: retail fills the load-delay
//    slot of `lw 0x3C(s6)` with the tileY shift and the beqz slot with the second
//    `srl 1,4`; this body loads the flags first, leaves a nop, and puts the tileY
//    shift in the beqz slot. Moving the spriteFlags read between the tileX and
//    tileY shifts (retail's own order) measures identical 93.85 / 848.
//    Retail also forms `lui s0,%hi(g_GfxCurWorkBuffer) / lw s0` and stores back
//    through $at where this body keeps the base in v1 for both - same count.
//    A 3-minute 2-job permuter run from the 89.56 body found the pVerts removal
//    (2514 -> 1431); everything it offered past that was wrong code (a dead
//    `pVerts = g_BattleSpriteQuadCorners;` deletion, a `new_var2` copy of
//    g_GfxCurWorkBufferEnd).
//    
// Round 483 re-measured 93.85% / 848 bytes (211 vs 212 insns) and added three measured no-ops, each a full scored run from the body below, all byte-identical: (1) hoisting the spriteFlags read above all four shifts, (2) putting z first in the declaration block, (3) the merged condition `if (pPrim->x3 < pPrim->x0 && --texX < 0)` instead of the nested if. The residual is still the three-place coincidence described below: the work-buffer store re-forming its address through at instead of reusing the base (retail is one instruction LONGER there), the flags load's delay slot (retail fills it with the tileY shift, we leave a nop because we use the loaded word immediately), and the uv block (retail decrements texX into a temp and copies it in the bgez slot, forming the 0x00FFFFFF mask once; our body decrements in place and rematerialises the mask into both slots).
// PARK 93.85% (848 bytes; retail 844 = 211 insns, this body 212 = ONE over).
// Round 496 CLOSED the three-way $s5/$s6/$s7 rotation with `-da -dl -dg` (the
// recipe in levers.md: cpp with -DSKIP_ASM, then tools/gcc-2.7.2-cdk-psx/cc1 with
// the unit's cc_flags + -O2 -G0 -da -dl -dg). func_8001EE88's 24 allocnos are
// processed `85 95 103 102 101 92 93 91 100 84 99 98 94 327 96 201 86 97 83 80 88
// 87 81 202`, and the three contested pseudos sit at positions 18/19/20:
// 80 = pSpriteData `used 11 times across 344 insns; pointer` = 3*11*4/344 = 3837,
// 88 = i `7 times across 153` = 3660, 87 = z `7 times across 169` = 3314.
// Retail's map is z=$s5, pSpriteData=$s6, i=$s7, i.e. it needs 87 to outrank both;
// z crosses the next floor_log2 step at 8 references (3*8*4/169 = 5680) and would
// then win outright, but an 8th reference is one more instruction in a body that is
// ALREADY one over. The two near-miss margins (80 vs 88 is 4.8%, 88 vs 87 is 10%)
// are each 5-7 insns of live range, and all three live ranges are pinned by retail's
// own instruction stream (z is the preheader `sllv`, i the loop counter, pSpriteData
// the latch's `lbu 0x40`), so no spelling reaches them - park on the arithmetic.
// Measured this round, each a full scored run, all byte-identical 93.85 / 848 / 144
// differing lines: (1) declarations reordered to `tileX, width, tileY, height`
// (retail's assignment order - kept, it is what the .s shows), (2) `tileY <<= n;`
// moved above the spriteFlags read so all four shifts precede it. So the flags
// position is confirmed dead in BOTH directions and the flip block's nop is a reorg
// artifact of the branch-slot steal, not statement order.
/*
void func_8001EE88(SpriteData* pSpriteData, u_long* pOt, s32 depth) {
    SpriteFrameData* pFrameData;
    POLY_FT4* pPrim;
    s32 n;
    s32 count;
    s32 z;
    s32 i;
    s32 zDepth;
    s32 flag;
    s32 tileX;
    s32 width;
    s32 tileY;
    s32 height;
    s32 texX;
    s32 texY;
    s32 texHeight;
    s32 clip;
    u32 spriteFlags;
    s32 yMin;
    s32 yMax;
    s32 edgeX;
    s32 edgeY;

    n = (pSpriteData->field_0x40 >> 8) & 0x1F;
    z = depth << n;
    count = (pSpriteData->field_0x40 >> 2) & 0x3F;
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }
    i = 0;
    do {
        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;
        setlen(pPrim, 9);
        *(u32*)&pPrim->r0 = pFrameData[i].colorAndCode;
        pPrim->tpage = pFrameData[i].texPage;
        pPrim->clut = pFrameData[i].clutID;
        tileX = pFrameData[i].tileX;
        width = pFrameData[i].width + (s8)pFrameData[i].deltaWidth;
        tileY = pFrameData[i].tileY;
        height = pFrameData[i].height + (s8)pFrameData[i].deltaHeight;
        n = (pSpriteData->field_0x40 >> 8) & 0x1F;
        width <<= n;
        height <<= n;
        tileX <<= n;
        spriteFlags = SpriteFlags0x3C(pSpriteData);
        tileY <<= n;
        if ((spriteFlags >> 3) & 1) {
            width = -width;
            tileX = -tileX;
        }
        if ((spriteFlags >> 4) & 1) {
            height = -height;
            tileY = -tileY;
        }
        edgeX = tileX + width;
        if (!((u32)pFrameData[i].flags >> 4 & 1)) {
            g_BattleSpriteQuadCorners[0].vx = tileX;
            g_BattleSpriteQuadCorners[1].vx = edgeX;
            g_BattleSpriteQuadCorners[2].vx = edgeX;
            g_BattleSpriteQuadCorners[3].vx = tileX;
        } else {
            g_BattleSpriteQuadCorners[0].vx = edgeX;
            g_BattleSpriteQuadCorners[1].vx = tileX;
            g_BattleSpriteQuadCorners[2].vx = tileX;
            g_BattleSpriteQuadCorners[3].vx = edgeX;
        }

        if (height > 0) {
            yMin = tileY;
            yMax = tileY + height;
        } else {
            yMin = tileY + height;
            yMax = tileY;
        }
        if (z < yMin) {
            continue;
        }
        if (z < yMax) {
            clip = yMax - z;
        } else {
            clip = 0;
        }
        height -= clip;
        if (height < 0) {
            tileY -= clip;
        }

        edgeY = tileY + height;
        if (!((u32)pFrameData[i].flags >> 5 & 1)) {
            g_BattleSpriteQuadCorners[0].vy = tileY;
            g_BattleSpriteQuadCorners[1].vy = tileY;
            g_BattleSpriteQuadCorners[2].vy = edgeY;
            g_BattleSpriteQuadCorners[3].vy = edgeY;
        } else {
            g_BattleSpriteQuadCorners[0].vy = edgeY;
            g_BattleSpriteQuadCorners[1].vy = edgeY;
            g_BattleSpriteQuadCorners[2].vy = tileY;
            g_BattleSpriteQuadCorners[3].vy = tileY;
        }

        RotAverage4(&g_BattleSpriteQuadCorners[0], &g_BattleSpriteQuadCorners[1], &g_BattleSpriteQuadCorners[2], &g_BattleSpriteQuadCorners[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
            (long*)&pPrim->x3, (long*)&pPrim->x2, &zDepth, &flag);

        n = (pSpriteData->field_0x40 >> 8) & 0x1F;
        texY = pFrameData[i].texY;
        clip >>= n;
        if (height > 0) {
            texHeight = pFrameData[i].height - clip;
        } else {
            texY -= clip;
            texHeight = pFrameData[i].height - clip;
        }
        texX = pFrameData[i].texX;
        width = pFrameData[i].width;
        if (pPrim->x3 < pPrim->x0) {
            if (--texX < 0) {
                texX = 0;
                width--;
            }
        }
        pPrim->u1 = texX + width;
        pPrim->u3 = texX + width;
        pPrim->u0 = texX;
        pPrim->u2 = texX;
        pPrim->v0 = texY;
        pPrim->v1 = texY;
        pPrim->v2 = texY + texHeight;
        pPrim->v3 = texY + texHeight;

        addPrim(pOt, pPrim);
    } while (++i != (((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2));
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering_atfree", func_8001EE88);




// Best attempt: 91.22% (864 bytes; retail is 860 = 215 instructions, ONE over).
//    Round 457, measured under XenoRenderingAtfree (cdk -G0).
//    The body is a verified transcription; the loop shape, both flip arms, the clip
//    block, the RotAverage4 argument order and the uv store order are retail's, and
//    everything left is a register hand-out. Load-bearing facts:
//    - DO/WHILE with `++i != flags_0x40.bytes[0] >> 2`: retail enters the loop with
//      no top test and its latch is lbu/srl/bne with the frame advance in the delay
//      slot. The for form costs a duplicated exit test.
//    - INDEXED pFrameData[i], never pFrameData++ / pFrameData->.
//    - NO pVerts local: write all four corners as g_BattleSpriteQuadCorners[k].
//      That is what stops loop.c from hoisting addPrim's 0xFFFFFF mask into a
//      callee-saved register, which in turn costs pOt its register.
//    - width is re-loaded from pFrameData[i].width after the RotAverage4 call, and
//      texHeight = pFrameData[i].height - clip once after the if/else that sets
//      texY (retail re-reads the height byte in each arm).
//    - The clip arithmetic is the mirror of func_8001EE88: yMin/yMax from the sign
//      of height, `if (z > yMax) continue;`, clip = (z > yMin) ? z - yMin : 0, and
//      `tileY += clip;` duplicated inside BOTH arms of the height test.
//    - uv store order is u1, u3, v0, v1, u0, u2, v2, v3.
//    What is left: retail's saved set is s6=pSpriteData, s7=i, s5=z, s2=pPrim,
//    s0=height; this body has s5=pSpriteData, s6=i, s7=z, s0=pPrim, s2=height (two
//    swaps), and the flags load's delay slot holds a nop where retail puts the
//    tileY shift (this body puts that shift in the beqz slot and the second
//    `srl 1,4` at the join instead of in the delay slot).
//    Measured dead: assigning pFrameData AFTER the work-buffer early-out is
//    86.40% / 868 bytes (three instructions worse).
//    
// Round 483: 91.34% at retail's EXACT size (860 bytes / 215 insns, up from the parked 91.22 / 864 / 216). The one instruction this body used to be over was the uv block's load-delay nop in the else arm: retail loads the frame HEIGHT into the texHeight register BEFORE the texY test and subtracts clip after the merge, so both arms hold two back-to-back lbu's and neither needs a nop. Fix measured: `texHeight = pFrameData[i].height;` above the if/else and `texHeight -= clip;` after it, instead of texHeight = height - clip placed after the if/else (91.22 / 864 -> 91.34 / 860, the note above the body has the old one). Two measured no-ops this round, each a full scored run, byte-identical to the body below: hoisting the spriteFlags read above the four shifts, and a one-iteration do-while-0 label between the RotAverage4 call and the uv block (that one is WORSE: 90.60 / 864). What is left is allocation only: retail is s6=pSpriteData s7=i s5=z s0=height s2=pPrim s1=clip, this body is s5=pSpriteData s6=i s7=z s2=height s1=pPrim s0=clip, plus the flip block's nop (retail fills the lw 0x3C delay slot with the tileY shift, this body uses the loaded word immediately and puts that shift in the beqz slot) and the work-buffer store, which retail re-forms through at (one instruction LONGER there) where this body reuses the base register.
// PARK 91.34% (860 bytes = retail's EXACT 215 insns; 207 differing lines, allocation only).
// Round 496 CLOSED both rotations with `-da -dl -dg` (recipe in levers.md,
// `a-three-way-rotation-is-closed-when-the-loser-needs-a-floor-log2-step`), so this
// body has been unchanged since round 483 and the number above is re-measured, not
// stale. func_8001F1D4's 24 allocnos are processed `85 95 101 103 92 102 93 100 91 98
// 84 99 94 96 327 97 201 86 83 80 88 87 81 202`, dispositions `98 in 16` (clip),
// `84 in 17` (pPrim), `94 in 18` (height), `80 in 21` (pSpriteData), `88 in 22` (i),
// `87 in 23` (z). Retail is s0=height s1=clip s2=pPrim and z=$s5 pSpriteData=$s6
// i=$s7, i.e. BOTH trios are the REVERSE of the quota order:
//   z 7 refs/172 insns = 3255, i 7/156 = 3589, pSpriteData 11/350 = 3771 - retail's
//   order needs z to cross the next floor_log2 step (8 refs = 5680), which is one
//   instruction, in a body already at retail's exact size;
//   height 30/87 = 55172, clip 22/53 = 66415, pPrim 46/144 = 63888 - retail's order
//   needs height's live range 15 insns shorter and clip's 3 insns longer, both pinned
//   by retail's own stream (height lives to the uv block's `if (height > 0)`, clip from
//   the clip block to `clip >>= n`).
// Preset screened this round, so do not retry it: gcc-2.7.2 88.77, gcc-2.6.0 90.87,
// cdk -O3 91.34 - this body under cdk -O2 is the best of the four.
/*
void func_8001F1D4(SpriteData* pSpriteData, u_long* pOt, s32 depth) {
    SpriteFrameData* pFrameData;
    POLY_FT4* pPrim;
    s32 n;
    s32 count;
    s32 z;
    s32 i;
    s32 zDepth;
    s32 flag;
    s32 tileX;
    s32 tileY;
    s32 width;
    s32 height;
    s32 texX;
    s32 texY;
    s32 texHeight;
    s32 clip;
    u32 spriteFlags;
    s32 yMin;
    s32 yMax;
    s32 edgeX;
    s32 edgeY;

    n = (pSpriteData->field_0x40 >> 8) & 0x1F;
    z = depth << n;
    count = (pSpriteData->field_0x40 >> 2) & 0x3F;
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }
    i = 0;
    do {
        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;
        setlen(pPrim, 9);
        *(u32*)&pPrim->r0 = pFrameData[i].colorAndCode;
        pPrim->tpage = pFrameData[i].texPage;
        pPrim->clut = pFrameData[i].clutID;
        tileX = pFrameData[i].tileX;
        width = pFrameData[i].width + (s8)pFrameData[i].deltaWidth;
        tileY = pFrameData[i].tileY;
        height = pFrameData[i].height + (s8)pFrameData[i].deltaHeight;
        n = (pSpriteData->field_0x40 >> 8) & 0x1F;
        width <<= n;
        height <<= n;
        tileX <<= n;
        spriteFlags = SpriteFlags0x3C(pSpriteData);
        tileY <<= n;
        if ((spriteFlags >> 3) & 1) {
            width = -width;
            tileX = -tileX;
        }
        if ((spriteFlags >> 4) & 1) {
            height = -height;
            tileY = -tileY;
        }
        edgeX = tileX + width;
        if (!((u32)pFrameData[i].flags >> 4 & 1)) {
            g_BattleSpriteQuadCorners[0].vx = tileX;
            g_BattleSpriteQuadCorners[1].vx = edgeX;
            g_BattleSpriteQuadCorners[2].vx = edgeX;
            g_BattleSpriteQuadCorners[3].vx = tileX;
        } else {
            g_BattleSpriteQuadCorners[0].vx = edgeX;
            g_BattleSpriteQuadCorners[1].vx = tileX;
            g_BattleSpriteQuadCorners[2].vx = tileX;
            g_BattleSpriteQuadCorners[3].vx = edgeX;
        }

        if (height > 0) {
            yMin = tileY;
            yMax = tileY + height;
        } else {
            yMin = tileY + height;
            yMax = tileY;
        }
        if (z > yMax) {
            continue;
        }
        if (z > yMin) {
            clip = z - yMin;
        } else {
            clip = 0;
        }
        if (height > 0) {
            tileY += clip;
            height -= clip;
        } else {
            tileY += clip;
            height += clip;
        }

        edgeY = tileY + height;
        if (!((u32)pFrameData[i].flags >> 5 & 1)) {
            g_BattleSpriteQuadCorners[0].vy = tileY;
            g_BattleSpriteQuadCorners[1].vy = tileY;
            g_BattleSpriteQuadCorners[2].vy = edgeY;
            g_BattleSpriteQuadCorners[3].vy = edgeY;
        } else {
            g_BattleSpriteQuadCorners[0].vy = edgeY;
            g_BattleSpriteQuadCorners[1].vy = edgeY;
            g_BattleSpriteQuadCorners[2].vy = tileY;
            g_BattleSpriteQuadCorners[3].vy = tileY;
        }

        RotAverage4(&g_BattleSpriteQuadCorners[0], &g_BattleSpriteQuadCorners[1], &g_BattleSpriteQuadCorners[2],
            &g_BattleSpriteQuadCorners[3], (long*)&pPrim->x0, (long*)&pPrim->x1, (long*)&pPrim->x3,
            (long*)&pPrim->x2, &zDepth, &flag);

        n = (pSpriteData->field_0x40 >> 8) & 0x1F;
        clip >>= n;
        texHeight = pFrameData[i].height;
        if (height > 0) {
            texY = pFrameData[i].texY + clip;
        } else {
            texY = pFrameData[i].texY - clip;
        }
        texHeight -= clip;
        texX = pFrameData[i].texX;
        width = pFrameData[i].width;
        if (pPrim->x3 < pPrim->x0) {
            if (--texX < 0) {
                texX = 0;
                width--;
            }
        }
        pPrim->u1 = texX + width;
        pPrim->u3 = texX + width;
        pPrim->v0 = texY;
        pPrim->v1 = texY;
        pPrim->u0 = texX;
        pPrim->u2 = texX;
        pPrim->v2 = texY + texHeight;
        pPrim->v3 = texY + texHeight;

        addPrim(pOt, pPrim);
    } while (++i != (((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2));
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering_atfree", func_8001F1D4);




