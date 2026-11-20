#include "common.h"

#include "system/menu.h"


// Byte sizes of the two menu scratch buffers this unit allocates: one prim block
// per character cell (g_Menu->pCellPrimBlocks) and g_Menu->pPrimBuffer.
#define MENU_CELL_PRIM_BLOCK_SIZE 344
#define MENU_PRIM_BUFFER_SIZE 11712

// Pixel size of one character cell: the quad extent and the vertex offsets
// MenuInitializeCellLinePrims writes.
#define MENU_CELL_SIZE 16

// VRAM rect of the glyph texture MenuClearGlyphTexture clears: 64x32 VRAM words,
// the 0x1000-byte 4bpp bitmap the glyphs are rasterised into.
#define MENU_GLYPH_TEXTURE_X 0x140
#define MENU_GLYPH_TEXTURE_Y 0xE0
#define MENU_GLYPH_TEXTURE_WIDTH 64
#define MENU_GLYPH_TEXTURE_HEIGHT 32

// One glyph's bitmap, the shape MenuCompressGlyphRows walks: 16 rows of 16
// one-byte pixels, each row compressed in place into its first 12 bytes.
#define MENU_GLYPH_ROW_COUNT 16
#define MENU_GLYPH_ROW_BYTES 16

// Length of a row after MenuCompressGlyphRows, and the most glyphs one chunk's
// glyph-code table holds.
#define MENU_GLYPH_COMPRESSED_ROW_BYTES 12
#define MENU_GLYPH_MAX_COUNT 64

// Bytes in the 4bpp glyph texture that is rasterised a glyph at a time and
// uploaded to the rect above.
#define MENU_GLYPH_TEXTURE_SIZE 0x1000

// Cell position in the menu glyph atlas: 21 cells per row, the row's y in
// D_801E99E8.
typedef struct {
    /* 0x0 */ u16 coord;
    /* 0x2 */ u16 pad2;
} MenuGlyphCellPos; // Size: 0x4

void MenuInitializePolyFt4(POLY_FT4* pPoly);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);

extern u16 g_MenuCellPrimXs[];
extern u16 g_MenuCellPrimYs[];

// Byte-identical to retail under gcc 2.6.0 (572 bytes, word for word). gcc 2.7.2's
// loop.c moves the POLY_F4 colour 0x80 out of the loop whatever the C says
// (threshold * savings * lifetime 28*1*3 = 84 >= the loop's 83 instructions), which
// also costs the extra $s8 save and 8 bytes, so this run of the file is 2.6.0.
void MenuInitializeCellQuadPrims(s32 index) {
    u8* pBase;
    u16* pX;
    u16* pY;
    RECT rect;
    s32 i = 0;
    s32 offset;

    pX = &g_MenuCellPrimXs[index * 2];
    pBase = (u8*)g_Menu->pCellPrimBlocks[index];
    pY = &g_MenuCellPrimYs[index * 2];
    offset = 0xB0;
    for (; i < 2; i++) {
        MenuInitializePolyFt4((POLY_FT4*)(pBase + i * 0x28));
        ((POLY_FT4*)(pBase + i * 0x28))->x0 = pX[0];
        ((POLY_FT4*)(pBase + i * 0x28))->y0 = pY[0];
        ((POLY_FT4*)(pBase + i * 0x28))->x1 = pX[0] + MENU_CELL_SIZE;
        ((POLY_FT4*)(pBase + i * 0x28))->y1 = pY[0];
        ((POLY_FT4*)(pBase + i * 0x28))->x2 = pX[0];
        ((POLY_FT4*)(pBase + i * 0x28))->y2 = pY[0] + MENU_CELL_SIZE;
        ((POLY_FT4*)(pBase + i * 0x28))->x3 = pX[0] + MENU_CELL_SIZE;
        ((POLY_FT4*)(pBase + i * 0x28))->y3 = pY[0] + MENU_CELL_SIZE;
        ((POLY_FT4*)(pBase + i * 0x28))->tpage = GetTPage(0, 0, 0x140, 0x80);
        SetPolyF4((POLY_F4*)(pBase + offset));
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->r0 = 0x80;
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->g0 = 0x80;
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->b0 = 0x80;
        SetSemiTrans((POLY_F4*)(pBase + offset), 1);
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->x0 = pX[0];
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->y0 = pY[0];
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->x1 = pX[0] + MENU_CELL_SIZE;
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->y1 = pY[0];
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->x2 = pX[0];
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->y2 = pY[0] + MENU_CELL_SIZE;
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->x3 = pX[0] + MENU_CELL_SIZE;
        ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->y3 = pY[0] + MENU_CELL_SIZE;
        offset += 0x18;
        MenuSetQuadVertices((SVECTOR*)(pBase + 0xE0), pX[0], pY[0], MENU_CELL_SIZE, MENU_CELL_SIZE);
    }
    rect.y = 0;
    rect.x = 0;
    rect.h = 0x100;
    rect.w = 0x100;
    SetDrawMode((DR_MODE*)(pBase + 0x140), 0, 0, GetTPage(0, 2, 0x140, 0x80), &rect);
    SetDrawMode((DR_MODE*)(pBase + 0x14C), 0, 0, GetTPage(0, 2, 0x140, 0x80), &rect);
}

void MenuInitializeCellLinePrims(s32 index) {
    u8* pBase;
    u16* pX;
    u16* pY;
    s32 offA;
    s32 offB;
    s32 i;

    pBase = (u8*)g_Menu->pCellPrimBlocks[index];
    pX = &g_MenuCellPrimXs[index * 2];
    pY = &g_MenuCellPrimYs[index * 2];
    for (i = 0; i < 2; i++) {
        offA = i * 0x18 + 0x50;
        offB = i * 0x18 + 0x80;
        SetLineF3((LINE_F3*)(pBase + offA));
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->r0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->g0 = 0xFF;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->b0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x0 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y0 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x1 = pX[0] + MENU_CELL_SIZE;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y1 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x2 = pX[0] + MENU_CELL_SIZE;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y2 = pY[0] + MENU_CELL_SIZE;
        MenuSetQuadVertices((SVECTOR*)(pBase + 0x100), pX[0], pY[0], MENU_CELL_SIZE, MENU_CELL_SIZE);
        SetLineF3((LINE_F3*)(pBase + offB));
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->r0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->g0 = 0xFF;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->b0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x0 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y0 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x1 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y1 = pY[0] + MENU_CELL_SIZE;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x2 = pX[0] + MENU_CELL_SIZE;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y2 = pY[0] + MENU_CELL_SIZE;
        MenuSetQuadVertices((SVECTOR*)(pBase + 0x120), pX[0], pY[0], MENU_CELL_SIZE, MENU_CELL_SIZE);
    }
}
void MenuCreateCellPrimBlocks(void) {
    void* pMem;
    int i;

    for (i = 0; i < (s32)ARRAY_COUNT(g_Menu->pCellPrimBlocks); i++) {
        pMem = HeapAlloc(MENU_CELL_PRIM_BLOCK_SIZE, 0);
        g_Menu->pCellPrimBlocks[i] = pMem;
        bzero(pMem, MENU_CELL_PRIM_BLOCK_SIZE);
        MenuInitializeCellQuadPrims(i);
        MenuInitializeCellLinePrims(i);
    }
}
extern u16 g_MenuGlyphSjisTable[];
extern u8 g_MenuGlyphIsDoubleByte;

void MenuFreeCellPrimBlocks(void) {
    int i;

    for (i = 0; i < (s32)ARRAY_COUNT(g_Menu->pCellPrimBlocks); i++) {
        HeapFree(g_Menu->pCellPrimBlocks[i]);
    }
}
extern MenuGlyphCellPos D_801E9994[0x15];
extern MenuGlyphCellPos D_801E99E8[2];

// Byte-identical to retail under gcc 2.6.0 (708 bytes): only 2.6.0's local-alloc
// coalesces the multiply-high temp with the quotient pseudo, so the mfhi lands in
// $a1 as retail has it instead of $t0.
void MenuInitializeGlyphQuadPrims(void) {
    MenuGlyphCellPos* pCellX;
    MenuGlyphCellPos* pCellY;
    s32 offsetTmp;
    s32 offset;
    s32 row;
    s32 col;
    s32 i;
    s32 j;

    for (i = 0, pCellX = D_801E9994, pCellY = D_801E99E8; i < 0x20; i++) {
        for (j = 0; j < 2; j++) {
            offsetTmp = (i * 2 + j) * 0x28;
            offset = offsetTmp;
            MenuInitializePolyFt4((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer));
            row = i / 0x15;
            col = i - row * 0x15;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->x0 =
                pCellX[col].coord + row * 8;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->y0 = pCellY[row].coord;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->x1 =
                pCellX[col].coord + row * 8 + 0xC;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->y1 = pCellY[row].coord;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->x2 =
                pCellX[col].coord + row * 8;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->y2 = pCellY[row].coord + 0x10;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->x3 =
                pCellX[col].coord + row * 8 + 0xC;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->y3 = pCellY[row].coord + 0x10;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->u0 = (i % 0x10) * 0x10;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->v0 = (i / 0x10) * 0x10 - 0x20;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->u1 = (i % 0x10) * 0x10 + 0xC;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->v1 = (i / 0x10) * 0x10 - 0x20;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->u2 = (i % 0x10) * 0x10;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->v2 = (i / 0x10) * 0x10 - 0x11;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->u3 = (i % 0x10) * 0x10 + 0xC;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->v3 = (i / 0x10) * 0x10 - 0x11;
            ((POLY_FT4*)((u8*)g_Menu->pPrimBuffer + offset))->tpage = GetTPage(0, 0, 0x140, 0x80);
            ((POLY_FT4*)((u8*)g_Menu->pPrimBuffer + offset))->clut = GetClut(0, 0x1C0);
        }
    }
}

// The pair of highlight primitives this unit writes at the head of
// g_Menu->pPrimBuffer, one per render context: a flat textured quad and the
// Gouraud quad shaded behind it. The slot portrait prims follow at 0xA98
// (see MenuDrawSlotSprite).
typedef struct {
    /* 0x0 */ u8 unk0[0xA00];
    /* 0xA00 */ POLY_FT4 quads[2];
    /* 0xA50 */ POLY_G4 shades[2];
} MenuSlotQuads; // Size: 0xA98

extern u16 D_801E99F0[];
extern u16 D_801E99F8[];

void MenuInitializeSlotHighlightPrims(void) {
    u16* pX;
    s32 i;

    for (i = 0, pX = D_801E99F0; i < 2; i++) {
        MenuInitializePolyFt4(&((MenuSlotQuads*)g_Menu->pPrimBuffer)->quads[i]);
        ((POLY_FT4*)(i * 0x28 + 0xA00 + (u32)g_Menu->pPrimBuffer))->x0 = pX[0];
        ((POLY_FT4*)(i * 0x28 + 0xA00 + (u32)g_Menu->pPrimBuffer))->y0 = D_801E99F8[0];
        ((POLY_FT4*)(i * 0x28 + 0xA00 + (u32)g_Menu->pPrimBuffer))->x1 = pX[0] + 0x20;
        ((POLY_FT4*)(i * 0x28 + 0xA00 + (u32)g_Menu->pPrimBuffer))->y1 = D_801E99F8[0];
        ((POLY_FT4*)(i * 0x28 + 0xA00 + (u32)g_Menu->pPrimBuffer))->x2 = pX[0];
        ((POLY_FT4*)(i * 0x28 + 0xA00 + (u32)g_Menu->pPrimBuffer))->y2 = D_801E99F8[0] + 0x20;
        ((POLY_FT4*)(i * 0x28 + 0xA00 + (u32)g_Menu->pPrimBuffer))->x3 = pX[0] + 0x20;
        ((POLY_FT4*)(i * 0x28 + 0xA00 + (u32)g_Menu->pPrimBuffer))->y3 = D_801E99F8[0] + 0x20;
        ((MenuSlotQuads*)g_Menu->pPrimBuffer)->quads[i].tpage = GetTPage(0, 0, 0x140, 0x80);
        SetPolyG4(&((MenuSlotQuads*)g_Menu->pPrimBuffer)->shades[i]);
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->r0 = 0x80;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->g0 = 0;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->b0 = 0x80;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->r1 = 0;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->g1 = 0;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->b1 = 0x80;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->r2 = 0x10;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->g2 = 0;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->b2 = 0x10;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->r3 = 0;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->g3 = 0;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->b3 = 0x10;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->x0 = 0;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->y0 = 0x4A;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->x1 = 0x140;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->y1 = 0x4A;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->x2 = 0;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->y2 = 0x8A;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->x3 = 0x140;
        ((POLY_G4*)(i * 0x24 + 0xA50 + (u32)g_Menu->pPrimBuffer))->y3 = 0x8A;
    }
}

extern s32 g_MenuSlotResourceXs[];
extern s32 g_MenuSlotResourceYs[];
extern s32 g_MenuSlotResourceIds[];
extern u32 g_MenuSlotNameTextureX[];
extern u8 g_MenuSlotNameTextureY[];
extern u16 g_SystemPalette1;
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2_260", func_801E61B0);
/* Best: 94.90% under gcc 2.6.0 at retail's EXACT 672 bytes (168 instructions).
   Re-measured in round 335 (un-parked, compiled clean).  Four finds this round
   took the old 89.50% / 668-byte park to 94.90:
   (1) THE MISSING INSTRUCTION was the count term's constant being folded into
       the slot offset.  Retail keeps the resource prim address as
       "addiu v0,s2,0xA98; addu v0,v0,t1" plus "addiu a2,a2,0x50" on the count
       product; writing the address as pPrimBuffer + (offset + sizeof(MenuSlotQuads))
       + (count * 0x50 + 0x50) INLINE (no temporary for the count term - a
       "s32 primSize" local makes gcc put the count term in the base register
       and swaps the two) reproduces it and is 672 bytes again.
   (2) THE TWO CALL BLOCKS NEED 0x820 IN A LOCAL.  With the literal, gcc folds
       the 0x820 into the base sum ("addu a0,s5,a0; addiu a0,a0,2080") instead of
       onto the renderContext*0x28 product ("sll;addu;sll;addiu 2080;addu a0,v1,a0").
       "ctxPrimsOffset = 0x820;" assigned just before each call and used as the
       addend stops the fold, at no instruction cost (92.16 -> 94.90).
   (3) The base must be the LEFT operand: (primOffset + (u32)g_Menu->pPrimBuffer)
       + (g_Menu->renderContext * 0x28 + ctxPrimsOffset).  Product-first makes
       gcc put the product in $v1 and the base in $a0, so the vertices call's
       "addu a0,v1,a0" comes out the other way round.
   (4) textureX: retail reads g_MenuSlotNameTextureX[j] with "lw" and shifts it
       late, which no inline spelling reaches (the u_char u parameter narrows the
       load to "lbu").  "textureX = g_MenuSlotNameTextureX[j];" as its own
       statement before the call, passed as (textureX << 2) & 0xFC, is the
       idiom the matched menu/main/misc4_260 and menu/main/misc6.c callers use
       and it reproduces retail's "lw a3 / sll a3,0x2" exactly.
   WHAT IS LEFT - one allocator tie, 7 instructions, byte count exact: retail
   keeps primOffset in $s4 and the j*4 giv in $s5, ours has them swapped
   ($s4 = j*4, $s5 = primOffset), which shows up as the prologue inits
   ("li s4,0xa98" vs "move s4,zero"), the two "addu v1,sN,v1" base sums, the two
   "addu at,at,sN" texture-table indexes, "addiu sN,sN,4" and the loop-tail
   "addiu sN,sN,2172".  Both givs have two uses and the increments are in the
   same order in both compiles, so this is global.c's priority tie
   (floor_log2(refs)*refs/live_length: primOffset's live range is the longer one
   here, which is why it lands in the higher register).  Measured dead: all 24
   local declaration orders (no effect), removing primOffset and spelling
   offset + sizeof(MenuSlotQuads) at both call sites (656 bytes - gcc derives no
   biv), swapping primOffset/offset in the for-init, and three permuter runs
   (base 2035 -> 1410 -> 910 -> 710; its finds are (1)-(2) above; it has not
   touched the $s4/$s5 tie).
   (5) 2.7.2 is not an option for this unit (loop.c, see MenuInitializeCellQuadPrims).
   The body below is the 94.90% one.

   void func_801E61B0(void) {
       s32 i;
       s32 j;
       s32 offset;
       s32 primOffset;
       s32 textureX;
       s32 ctxPrimsOffset;
       u8 count;

       for (j = 0, offset = 0, primOffset = sizeof(MenuSlotQuads); offset < 0x1974;
            j++, offset += MENU_SLOT_PRIM_STRIDE, primOffset += MENU_SLOT_PRIM_STRIDE) {
           ((u8*)g_Menu->pPrimBuffer + offset)[0x1312] = 0;
           for (i = 0; i < 9; i++) {
               if (g_MenuSlotResourceIds[i] != 0xFFFF) {
                   count = ((u8*)g_Menu->pPrimBuffer + offset)[0x1312];
                   ((u8*)g_Menu->pPrimBuffer + offset)[0x1312] +=
                       func_8002675C(g_Menu->resources, g_MenuSlotResourceIds[i],
                                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer +
                                                 (offset + sizeof(MenuSlotQuads)) +
                                                 (count * 0x50 + 0x50)),
                                     g_Menu->renderContext, j * 0x50 + g_MenuSlotResourceXs[i],
                                     g_MenuSlotResourceYs[i], 0x1000);
               }
           }
           ctxPrimsOffset = 0x820;
           ((u8*)g_Menu->pPrimBuffer + offset)[0x130E] = (u_char)g_Menu->renderContext;
           MenuInitializePolyFt4((POLY_FT4*)((primOffset + (u32)g_Menu->pPrimBuffer) +
                                             (g_Menu->renderContext * 0x28 + ctxPrimsOffset)));
           *(u16*)((u8*)g_Menu->pPrimBuffer + (g_Menu->renderContext * 0x28 + offset) + 0x12CE) =
               GetTPage(0, 0, 0x180, 0);
           *(u16*)((u8*)g_Menu->pPrimBuffer + (g_Menu->renderContext * 0x28 + offset) + 0x12C6) =
               g_SystemPalette1;
           textureX = g_MenuSlotNameTextureX[j];
           ctxPrimsOffset = 0x820;
           MenuSetFt4QuadVertices((POLY_FT4*)((primOffset + (u32)g_Menu->pPrimBuffer) +
                                             (g_Menu->renderContext * 0x28 + ctxPrimsOffset)),
                                  *(u16*)g_MenuSlotResourceXs + j * 0x50, *(u16*)g_MenuSlotResourceYs + 7,
                                  (textureX << 2) & 0xFC, g_MenuSlotNameTextureY[j * 4], 0x48, 0xD);
           ((u8*)g_Menu->pPrimBuffer + offset)[0x1311] = (u_char)g_Menu->renderContext;
       }
   }
*/
void MenuAllocatePrimBuffer(void) {
    void* pMem;
    u32 unk[2]; // unused: the original frame has 8 bytes of locals we cannot express

    pMem = HeapAlloc(MENU_PRIM_BUFFER_SIZE, 0);
    g_Menu->pPrimBuffer = pMem;
    bzero(pMem, MENU_PRIM_BUFFER_SIZE);
    MenuInitializeGlyphQuadPrims();
    MenuInitializeSlotHighlightPrims();
}
void MenuFreePrimBuffer(void) {
    void* pMem;
    MenuManager* pManager;

    pMem = g_Menu->pPrimBuffer;
    HeapFree(pMem);
    pManager = g_Menu->pManager;
    *(volatile u8*)&pManager->characterWindowBuilt = FALSE;
}
void MenuClearGlyphTexture(void) {
    RECT rect;

    rect.x = MENU_GLYPH_TEXTURE_X;
    rect.y = MENU_GLYPH_TEXTURE_Y;
    rect.w = MENU_GLYPH_TEXTURE_WIDTH;
    rect.h = MENU_GLYPH_TEXTURE_HEIGHT;
    ClearImage(&rect, 0, 0, 0);
    *(volatile u8*)&g_Menu->pManager->characterWindowBuilt = FALSE;
}
void MenuCompressGlyphRows(u8* p) {
    int i;

    for (i = 0; i < MENU_GLYPH_ROW_COUNT; i++) {
        p[i * MENU_GLYPH_ROW_BYTES + 0x0] = p[i * MENU_GLYPH_ROW_BYTES + 0x0];
        p[i * MENU_GLYPH_ROW_BYTES + 0x1] = p[i * MENU_GLYPH_ROW_BYTES + 0x1] | p[i * MENU_GLYPH_ROW_BYTES + 0x2];
        p[i * MENU_GLYPH_ROW_BYTES + 0x2] = p[i * MENU_GLYPH_ROW_BYTES + 0x3];
        p[i * MENU_GLYPH_ROW_BYTES + 0x3] = p[i * MENU_GLYPH_ROW_BYTES + 0x4];
        p[i * MENU_GLYPH_ROW_BYTES + 0x4] = p[i * MENU_GLYPH_ROW_BYTES + 0x5] | p[i * MENU_GLYPH_ROW_BYTES + 0x6];
        p[i * MENU_GLYPH_ROW_BYTES + 0x5] = p[i * MENU_GLYPH_ROW_BYTES + 0x7];
        p[i * MENU_GLYPH_ROW_BYTES + 0x6] = p[i * MENU_GLYPH_ROW_BYTES + 0x8];
        p[i * MENU_GLYPH_ROW_BYTES + 0x7] = p[i * MENU_GLYPH_ROW_BYTES + 0x9] | p[i * MENU_GLYPH_ROW_BYTES + 0xA];
        p[i * MENU_GLYPH_ROW_BYTES + 0x8] = p[i * MENU_GLYPH_ROW_BYTES + 0xB];
        p[i * MENU_GLYPH_ROW_BYTES + 0x9] = p[i * MENU_GLYPH_ROW_BYTES + 0xC];
        p[i * MENU_GLYPH_ROW_BYTES + 0xA] = p[i * MENU_GLYPH_ROW_BYTES + 0xD] | p[i * MENU_GLYPH_ROW_BYTES + 0xE];
        p[i * MENU_GLYPH_ROW_BYTES + 0xB] = p[i * MENU_GLYPH_ROW_BYTES + 0xF];
    }
}
s32 MenuGetGlyphKromAddress(u8* p) {
    u8 hi;
    u8 lo;

    hi = p[0];
    lo = p[1];
    g_MenuGlyphIsDoubleByte = TRUE;
    if (hi < 0x80) {
        if (hi >= 0x20) {
            lo = *(u8*)&g_MenuGlyphSjisTable[hi];
            hi = g_MenuGlyphSjisTable[hi] >> 8;
            g_MenuGlyphIsDoubleByte = FALSE;
        } else {
            hi = 0x81;
            lo = 0x40;
            g_MenuGlyphIsDoubleByte = FALSE;
        }
    }
    return Krom2RawAdd((hi << 8) | lo);
}
void MenuBuildGlyphTexture(s32 index) {
    u8* pGlyphBits;
    u8* pBits;
    u16* pTexture;
    u8* pSrc;
    u16* pGlyph;
    s32 kromAddress;
    RECT rect;
    s32 i;
    s32 count;
    s32 row;
    s32 col;

    pBits = HeapAlloc(MENU_GLYPH_ROW_COUNT * MENU_GLYPH_ROW_BYTES, 1);
    pTexture = HeapAlloc(MENU_GLYPH_TEXTURE_SIZE, 1);
    bzero(pTexture, MENU_GLYPH_TEXTURE_SIZE);
    count = 0;
    pSrc = &g_Menu->menuUnk2->chunks[index].pad0[4];
    for (i = 0; i < 0x20; i++) {
        if (*pSrc == 0) {
            break;
        }
        pGlyphBits = pBits;
        kromAddress = MenuGetGlyphKromAddress(pSrc);
        if (kromAddress != -1) {
            pGlyph = (u16*)kromAddress;
            for (row = 0; row < MENU_GLYPH_ROW_COUNT; row++) {
                for (col = 7; col >= 0; col--) {
                    *pGlyphBits++ = (*pGlyph >> col) & 1;
                }
                for (col = 0xF; col >= 8; col--) {
                    *pGlyphBits++ = (*pGlyph >> col) & 1;
                }
                pGlyph++;
            }
            MenuCompressGlyphRows(pBits);
            for (row = 0; row < MENU_GLYPH_ROW_COUNT; row++) {
                for (col = 0; col < MENU_GLYPH_COMPRESSED_ROW_BYTES; col++) {
                    pTexture[row * 0x40 + col / 4 + (i % 0x10) * 4 +
                             (i / 0x10) * 0x400] |=
                        pBits[row * MENU_GLYPH_ROW_BYTES + col] << ((col % 4) * 4);
                }
            }
        }
        pSrc++;
        count++;
        if (g_MenuGlyphIsDoubleByte != 0) {
            pSrc++;
            count++;
        }
        if (count >= MENU_GLYPH_MAX_COUNT) {
            break;
        }
    }
    rect.x = MENU_GLYPH_TEXTURE_X;
    rect.y = MENU_GLYPH_TEXTURE_Y;
    rect.w = MENU_GLYPH_TEXTURE_WIDTH;
    rect.h = MENU_GLYPH_TEXTURE_HEIGHT;
    LoadImage(&rect, (u_long*)pTexture);
    DrawSync(0);
    HeapFree(pBits);
    HeapFree(pTexture);
}
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetPlayTimeDigits(u32 value);

extern s32 g_MenuPlayTimeQuadXs[];
extern s32 D_801E9FE4[];

void MenuBuildPlayTimePrims(u8* pArg) {
    s32 i;

    func_8002675C(g_Menu->resources, 0xEE,
                  (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x240C),
                  g_Menu->renderContext, g_MenuPlayTimeQuadXs[0], 0x7A, 0x1000);
    func_8002675C(g_Menu->resources, 0xEE,
                  (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x24AC),
                  g_Menu->renderContext, D_801E9FE4[0], 0x7A, 0x1000);
    MenuSetPlayTimeDigits(*(u32*)pArg);
    for (i = 0; i < (s32)ARRAY_COUNT(g_Menu->playTimeDigitIds); i++) {
        func_8002675C(g_Menu->resources, g_Menu->playTimeDigitIds[i],
                      (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x254C + i * 0x50),
                      g_Menu->renderContext, g_MenuPlayTimeQuadXs[i + 2], 0x7A, 0x1000);
    }
    func_8002675C(g_Menu->resources, 0x17,
                  (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2C7C),
                  g_Menu->renderContext, 0x8, 0x66, 0x1000);
    func_8002675C(g_Menu->resources, 0x32,
                  (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2CCC),
                  g_Menu->renderContext, 0x10, 0x66, 0x1000);
    func_8002675C(g_Menu->resources, (pArg[0x23] + 1) / 10,
                  (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2D1C),
                  g_Menu->renderContext, 0x10, 0x6E, 0x1000);
    func_8002675C(g_Menu->resources, (pArg[0x23] + 1) % 10,
                  (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2D6C),
                  g_Menu->renderContext, 0x18, 0x6E, 0x1000);
}

extern u32 g_MenuSlotSpriteX[];
extern u32 g_MenuSlotSpriteY[];

void MenuDrawSlotSprite(u8 arg0, u8* pArg1) {
    u8* pPrims;
    u32 offset;

    offset = 0xA98 + arg0 * MENU_SLOT_PRIM_STRIDE;
    pPrims = (u8*)(offset + (u32)g_Menu->pPrimBuffer);
    func_8002675C(g_Menu->resources, (pArg1 + arg0)[0x1C] + MENU_TEX_CHARACTER_PORTRAITS_SMALL, pPrims,
                  g_Menu->renderContext, g_MenuSlotSpriteX[arg0], g_MenuSlotSpriteY[arg0], 0x1000);
}
