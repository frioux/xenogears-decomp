#include "common.h"
#include "system/menu.h"
#include "psyq/inline_c.h"

// Split out of system/temp2 at file 0x173A4 (config/slus_006.64.yaml): the original
// object that owned func_80026BA4 reached g_GfxCurWorkBuffer/g_GfxCurWorkBufferEnd
// with the assembler's ABSOLUTE expansion (lui %hi then lw %lo), i.e. neither symbol
// was in that TU's small data. include/system/animation.h declares both as tentative
// definitions, and maspsx rewrites every reference to a tentative definition to
// %gp_rel, which the matched neighbours (GfxQueueSpriteTileData, GfxSetRenderContext,
// SpriteDrawTile) need. One TU cannot produce both shapes, so this unit declares the
// two symbols extern and nothing else moves.
extern void* g_GfxCurWorkBuffer;
extern void* g_GfxCurWorkBufferEnd;

// Queues one semi-transparent textured quad per record of the dirent.
//
// The pre-carve park sat at 68.00% and blamed the TU boundary: the old body kept
// g_GfxCurWorkBuffer in a %gp_rel form the neighbours need, and system/temp2 is
// still one unit with the matched GfxQueueSpriteTileData/GfxSetRenderContext. That
// is what the split fixed - the absolute lui/lw form comes out of this TU now.
// Best 91.88% (552 bytes / 138 insns = retail's exact size). The guard, the frame, the spill set and the whole load block are retail's instruction for instruction: pDirent at 0x10, the byte offset at 0x1C (starts 4, += 0x1C), i at 0x14, count at 0x18, u spilled at 0x20. What closed it from 68%:
//   - the guard indexes the global directly, ((POLY_FT4*)g_GfxCurWorkBuffer + count): with 'pPrim = g_GfxCurWorkBuffer;' first, cc1 gives the guard's buffer a callee-saved register and moves the frame/prologue.
//   - a texPageId local assigned BEFORE GetClut, so pData dies at the call (otherwise the record base needs a callee-saved register and pPrim gets spilled instead).
//   - the prim stores add x/y AT the store ('pPrim->x0 = x0 + x;' with x0 = pData->x), which is what makes cc1 read the x/y home slots (0x58/0x5C) AFTER the two calls, as retail does.
//   - (s16) casts on the u16 fields width/height/x/u: that is retail's lh set. Unexplained but measured: without the casts cc1 emits lhu and the body sits 3 points lower.
//   - the permuter's two edits at the top of the load block (u reused as the texPageX raw temp, rawX for pData->x) flipped the spilled value from h to u: 81.26 -> 91.88.
// Residual, 40 differing lines: $s3/$s4 are swapped between v and x0 (retail s3=x0, s4=v), and the prim store order (retail x0, v0, v1, x1, x2, x3, v2, v3, y0..y3, u0..u3; this body emits the whole v group first because the x stores wait on the home-slot reload).
// Tried and dead: pre-carve preset screens (cdk -G0 67.17, 2.6.0 67.64, 2.7.2 67.82); a pData induction pointer (69.50); pDirent+offset with sizeof in the for-increment; s16 locals for w/h/x0/u/v (142 insns, the adds then need sign extensions); count/i declaration swap and moving u to the end of the declaration list (both byte-identical); a 4-minute permuter run from 76% (its find added a store to v2 - rejected as wrong code).
/*
void func_80026BA4(MenuResourceDirectory* dir, u32 index, s32 x, s32 y, u_long* ot) {
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 i;
    u16 rawX;
    s32 count;
    s32 offset;
    s32 x0;
    s32 y0;
    s32 w;
    s32 h;
    s32 v;
    s32 u_shifted;
    s32 texPageX_lsb;
    s32 texPageX;
    s32 texPageY;
    s32 texPageId;
    s32 u;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    count = pDirent->count;
    if ((u8*)((POLY_FT4*)g_GfxCurWorkBuffer + count) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }

    for (i = 0, offset = 4; i != count; offset += 0x1C, i++) {
        pData = (MenuResourceTextureMetadata*)((u8*)pDirent + offset);
        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;

        setlen(pPrim, 9);
        setcode(pPrim, 0x2D);

        u_shifted = pData->u << 0x10;
        if (pData->texPageId != 0) {
            texPageX_lsb = u_shifted >> 0x12;
        } else {
            texPageX_lsb = u_shifted >> 0x14;
        }
        u = pData->texPageX;
        texPageX = (s16)(u & 0xFFC0) + texPageX_lsb;
        texPageY = (s16)(pData->texPageY & 0xFF00) + pData->v;

        texPageId = pData->texPageId;
        w = (s16)pData->width;
        h = (s16)pData->height;
        rawX = pData->x;
        x0 = (s16)rawX;
        y0 = pData->y;
        u = (s16)pData->u;
        v = pData->v;

        pPrim->clut = GetClut(pData->clutX, pData->clutY);
        pPrim->tpage = GetTPage(texPageId, 0, texPageX, texPageY);

        pPrim->x0 = x0 + x;
        pPrim->v0 = v;
        pPrim->v1 = v;
        pPrim->x1 = x0 + x + w;
        pPrim->x2 = x0 + x;
        pPrim->x3 = x0 + x + w;
        pPrim->v2 = v + h;
        pPrim->v3 = v + h;
        pPrim->y0 = y0 + y;
        pPrim->y1 = y0 + y;
        pPrim->y2 = y0 + y + h;
        pPrim->y3 = y0 + y + h;
        pPrim->u0 = u;
        pPrim->u1 = u + w;
        pPrim->u2 = u;
        pPrim->u3 = u + w;

        AddPrim(ot, pPrim);
    }
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2_b", func_80026BA4);



// One sprite of a menu resource dirent, converted to screen space with its
// tpage/clut resolved. x/y stay unsigned: they are added to a screen offset.
typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u8 u;
    /* 0x05 */ u8 v;
    /* 0x06 */ u8 width;
    /* 0x07 */ u8 height;
    /* 0x08 */ u16 unk8;
    /* 0x0A */ u16 tpage;
    /* 0x0C */ u16 clut;
    /* 0x0E */ u8 unkE[0xA];
} MenuResourceSprite; // Size: 0x18

// Converts the sprites of one dirent into out[]; returns the sprite count.
s32 ResourceHelperGetSpriteList(MenuResourceDirectory* dir, u32 index, MenuResourceSprite* out,
                  u16 x, u16 y) {
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    u32 count;
    u32 i;
    s32 u_shifted;
    s32 texPageX_lsb;
    s32 texPageId;
    s32 texPageX;
    s32 texPageY;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (mem_addr)dir);
    pData = pDirent->data;
    count = pDirent->count;
    for (i = 0; i != count; i++, pData++, out++) {
        u_shifted = pData->u << 0x10;
        if (pData->texPageId != 0) {
            texPageX_lsb = u_shifted >> 0x12;
        } else {
            texPageX_lsb = u_shifted >> 0x14;
        }
        texPageX = (s16)(pData->texPageX & 0xFFC0) + texPageX_lsb;
        texPageY = (s16)(pData->texPageY & 0xFF00) + pData->v;
        texPageId = pData->texPageId;
        out->clut = GetClut(pData->clutX, pData->clutY);
        out->tpage = GetTPage(texPageId, 1, texPageX, texPageY);
        out->u = pData->u;
        out->v = pData->v;
        out->width = pData->width;
        out->height = pData->height;
        out->x = pData->x + x;
        out->y = pData->y + y;
    }
    return count;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2_b", func_80026F44);
/*
NOT C-REPRODUCIBLE: this stub is handwritten PSY-Q Lite assembly, not gcc output.
Two tells, both absent from all 946 matched .s files of slus_006.64:
  `addi $a1,$zero,0x20`  - ADDI (opcode 8). gcc and maspsx only ever emit the
      ADDIU/ORI forms for `li`, so no C produces this opcode.
  `bne $zero,$t2`        - gcc's mips.md branch patterns put the register
      first (`bne $t2,$zero`, printed `bnez`); $zero first never appears.
The loop shape itself was solved before that was noticed (see below); the body
below is the best C attempt and cannot be pushed further.
Best 74.88% (160 bytes, exactly the original's size). The LOOP SHAPE:
retail's unrotated top test with `li t2,-1` hoisted once before the loop is
`while (1) { if (--count == limit) break; ... }` with `s32 limit = -1;` - a
literal -1 in the compare makes gcc rematerialise it, a limit local is what
hoists it, and the while (1) break keeps gcc from duplicating the exit test
(160 bytes instead of 168). Shapes that do NOT work: while (--count != -1) and
for (;;) { if (--count == -1) break; } 72.54, for (i = count - 1; i != -1; i--)
72.66, for (i = count; i != limit; i--) 70.59.
What is left is register allocation of the nine body values only: retail keeps
color in t6 and ir1/ir2/ir3 in t5/t4/t3, this shape gets v0/t0/v1/a1, so the
final `rgb |= 1` comes out `ori t5,t5,1` there and `li v1,1; or` here, the two
`or`s differ in operand order, and the clamp prologue fills the beqz frames slot
with the level<<7 shift where retail has a nop and shifts after. A separate
`s32 dqp; dqp = level << 7; gte_lddp(dqp);` local changed nothing.
Round 61: shifting in place (`level <<= 7; gte_lddp(level);`) DOES reproduce
retail's in-place `sll a1,a1,0x7` instead of a copy into v0 (74.63 -> 74.88);
the `level >= 0x20` clamp shape is already right. What is left is reorg: retail
leaves a nop in the clamp's bne frames slot where we have the sll, and fills the
loop's `beq a0,t2` slot with the `lhu` where we emit a nop; plus the nine body
values land in t0/v1/a1 instead of t5/t4/t3.
void func_80026F44(s32 count, s32 level, u16* pDst, u16* pSrc) {
    u16 color;
    s32 limit;
    s32 ir1;
    s32 ir2;
    s32 ir3;
    s32 rgb;

    if (level >= 0x20) {
        level = 0x20;
    }
    level <<= 7;
    gte_lddp(level);
    limit = -1;
    while (1) {
        if (--count == limit) {
            break;
        }
        color = *pSrc++;
        ir1 = color & 0x1F;
        ir2 = color & 0x3E0;
        ir3 = color & 0x7C00;
        gte_ldir123(ir1, ir2, ir3);
        gte_gpf12();
        gte_stir123_gpr(ir1, ir2, ir3);
        ir1 &= 0x1F;
        ir2 &= 0x3E0;
        ir3 &= 0x7C00;
        rgb = ir1 | ir2;
        rgb = ir3 | rgb;
        if (color != 0 && rgb == 0) {
            rgb |= 1;
        }
        *pDst++ = (color & 0x8000) | rgb;
    }
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2_b", func_80026FE8);
/*
Best 64.58% (184 bytes against 176). The content is right - the clamp, the
gte_lddp(level << 7), the per-field diff into IR1-IR3, gte_gpf12, the masked
re-add and the shared store. Two walls, both shared with func_80026F44 and
func_80025C04 above/below (same four-stub GPF family, the only stubs of the
unit the disassembler marks "Handwritten function"):
1. LOOP SHAPE. Retail enters the loop with no pre-loop test:
       addiu a0,a0,1                 <- preheader
   .L: addiu a0,a0,-1
       beqz a0,.Lend
       body
       j .L ; addi a2,a2,2
   so the decrement lives in a0 itself. Every spelling tried
   (for (i=count;i!=0;i--), i=count+1; while(--i!=0), and the shapes parked
   under func_80026F44) makes gcc's duplicate_loop_exit_test rotate it into a
   pre-loop dec+test plus a bottom test, which is 2 instructions more than
   retail. Identical under gcc 2.6.0 / 2.7.2 / 2.7.2-cdk at -O2 (all 64.58),
   so it is not the preset.
2. Retail's result sum is `add` (signed), ours `addu`, and retail re-masks
   *in* from memory per field (andi v1,t4,0x1F / 0x3E0 / 0x7C00 each time)
   instead of keeping the three masked copies, so the register allocation of
   the nine body values differs on top of the loop.

void func_80026FE8(s32 count, s32 level, u16* pDst, u16* pSrc, u16* pSrc2) {
    s32 i;
    u16 in;
    u16 base;
    s32 b1;
    s32 b2;
    s32 b3;
    s32 ir1;
    s32 ir2;
    s32 ir3;
    s32 rgb;

    if (level >= 0x21) {
        level = 0x20;
    }
    gte_lddp(level << 7);
    i = count + 1;
    while (--i != 0) {
        in = *pSrc2++;
        base = *pSrc;
        b1 = base & 0x1F;
        b2 = base & 0x3E0;
        b3 = base & 0x7C00;
        ir1 = (in & 0x1F) - b1;
        ir2 = (in & 0x3E0) - b2;
        ir3 = (in & 0x7C00) - b3;
        gte_ldir123(ir1, ir2, ir3);
        gte_gpf12();
        gte_stir123_gpr(ir1, ir2, ir3);
        ir1 = (ir1 & 0x1F) + b1;
        ir2 = (ir2 & 0x3E0) + b2;
        ir3 = (ir3 & 0x7C00) + b3;
        rgb = ir1 | ir2;
        rgb |= ir3;
        *pDst = rgb;
        pSrc++;
        pDst++;
    }
}
*/
