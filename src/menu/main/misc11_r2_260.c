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
void func_801E56E8(s32 index) {
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
        func_801E56E8(i);
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
void func_801E5B88(void) {
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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2_260", func_801E5E4C);
extern s32 D_801E9F98[];
extern s32 D_801E9FBC[];
extern s32 D_801EA494[];
extern u32 g_MenuSlotNameTextureX[];
extern u8 g_MenuSlotNameTextureY[];
extern u16 g_SystemPalette1;
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2_260", func_801E61B0);
/* Best: 85.05% (656 vs 660 bytes).  Confirmed shape: outer loop is TWO
   counters in one for ("j = 0, offset = 0; offset < 0x1974; j++, offset +=
   0x87C") which is what makes retail test the 0x87C-scaled IV (s2) and keep
   four separate givs s2/s3/s4/s5 = offset, j*0x50, 0xA98+offset, j*4 with no
   register for j itself.  `primOffset` must be a real s32 local, otherwise
   gcc folds 0xA98 into 0x50 (0xAE8) at the prim address and into 0x820 at the
   two func_801E92{7C,0C} calls; and the prim pointer has to be a u8* local or
   the same fold moves 0x820 onto it.  *(u16*)D_801E9F98 / *(u16*)D_801E9FBC
   are needed to get retail's lhu (a plain D_801E9F98[0] gives lw).
   Left, four instructions: (1) retail's primOffset lands in $s4 and j*0x50 in
   $s3, ours swaps them; (2) retail emits "addiu s5,s5,4" (j++) right after
   the g_MenuSlotNameTextureY load and ours after the call; (3) the MenuSetFt4QuadVertices argument
   setup order differs (retail forms the 0x820 sum after loading base/ctx);
   (4) retail reads the byte it stores to 0x130E/0x1311 with "lbu 776(v1)",
   ours with "lw" - (u_char), (u8) and (s8) casts on g_Menu->renderContext all
   leave the lw.  Same allocator-tie class as func_801E56E8/func_801E68AC.
   void func_801E61B0(void) {
       s32 i;
       s32 j;
       s32 offset;
       s32 primOffset;
       u8* pPrim;

       for (j = 0, offset = 0; offset < 0x1974; j++, offset += 0x87C) {
           primOffset = offset + 0xA98;
           ((u8*)g_Menu->pPrimBuffer + offset)[0x1312] = 0;
           for (i = 0; i < 9; i++) {
               if (D_801EA494[i] != 0xFFFF) {
                   ((u8*)g_Menu->pPrimBuffer + offset)[0x1312] += func_8002675C(
                       g_Menu->resources, D_801EA494[i],
                       (POLY_FT4*)(primOffset + (u32)g_Menu->pPrimBuffer +
                                   (((u8*)g_Menu->pPrimBuffer + offset)[0x1312] * 0x50 + 0x50)),
                       g_Menu->renderContext, j * 0x50 + D_801E9F98[i], D_801E9FBC[i], 0x1000);
               }
           }
           ((u8*)g_Menu->pPrimBuffer + offset)[0x130E] = (u_char)g_Menu->renderContext;
           pPrim = (u8*)(primOffset + (u32)g_Menu->pPrimBuffer);
           MenuInitializePolyFt4((POLY_FT4*)(pPrim + (g_Menu->renderContext * 0x28 + 0x820)));
           *(u16*)((u8*)g_Menu->pPrimBuffer + (g_Menu->renderContext * 0x28 + offset) + 0x12CE) =
               GetTPage(0, 0, 0x180, 0);
           *(u16*)((u8*)g_Menu->pPrimBuffer + (g_Menu->renderContext * 0x28 + offset) + 0x12C6) =
               g_SystemPalette1;
           MenuSetFt4QuadVertices((POLY_FT4*)(pPrim + (g_Menu->renderContext * 0x28 + 0x820)),
                         *(u16*)D_801E9F98 + j * 0x50, *(u16*)D_801E9FBC + 7,
                         (u_char)(g_MenuSlotNameTextureX[j] << 2), g_MenuSlotNameTextureY[j * 4], 0x48, 0xD);
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
    func_801E5B88();
    func_801E5E4C();
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2_260", func_801E6668);
/* Best: 98.90% (584 bytes, same size as retail).  Decoded shape: index selects a
   0x200-byte MenuUnk2ImageChunk; the 0x40-entry byte table at chunk+4 is walked one
   entry per glyph (two bytes when MenuGetGlyphKromAddress sets g_MenuGlyphIsDoubleByte), each entry is
   unpacked to 0x100 bits, MenuCompressGlyphRows compresses a row, and the bits are OR ed into
   a 0x1000-byte 4bpp texture at (i/16)*0x400 + (i%16)*4 + row*0x40 + col/4 before
   LoadImage of the 0x140,0xE0 RECT.  Everything matches except ONE insn: retail
   keeps addiu a0,sp,16 (the RECT address) in BOTH branch delay slots that reach the
   epilogue and has none at the head of the epilogue block; gcc keeps it in the
   epilogue block and fills only the loop-top beqz slot.  Tried: RECT declared
   before/after the loops, one shared col variable for the bit loops and the repack
   column (that fixed a v1/a0 register swap and took it from 98.34 to 98.90), the
   s32-address form (adds a move a0,v1), LoadImage args reordered.
   void func_801E6668(s32 index) {
       u8* pGlyphBits;
       u8* pBits;
       u8* pTexture;
       u8* pSrc;
       u16* pGlyph;
       RECT rect;
       s32 i;
       s32 count;
       s32 row;
       s32 col;

       pBits = HeapAlloc(0x100, 1);
       pTexture = HeapAlloc(0x1000, 1);
       bzero(pTexture, 0x1000);
       count = 0;
       pSrc = &g_Menu->menuUnk2->chunks[index].pad0[4];
       for (i = 0; i < 0x20; i++) {
           if (*pSrc == 0) {
               break;
           }
           pGlyphBits = pBits;
           pGlyph = (u16*)MenuGetGlyphKromAddress(pSrc);
           if (pGlyph != (u16*)-1) {
               for (row = 0; row < 0x10; row++) {
                   for (col = 7; col >= 0; col--) {
                       *pGlyphBits++ = (*pGlyph >> col) & 1;
                   }
                   for (col = 0xF; col >= 8; col--) {
                       *pGlyphBits++ = (*pGlyph >> col) & 1;
                   }
                   pGlyph++;
               }
               MenuCompressGlyphRows(pBits);
               for (row = 0; row < 0x10; row++) {
                   for (col = 0; col < 0xC; col++) {
                       ((u16*)pTexture)[row * 0x40 + col / 4 + (i % 0x10) * 4 +
                                         (i / 0x10) * 0x400] |=
                           pBits[row * 0x10 + col] << ((col % 4) * 4);
                   }
               }
           }
           pSrc++;
           count++;
           if (g_MenuGlyphIsDoubleByte != 0) {
               pSrc++;
               count++;
           }
           if (count >= 0x40) {
               break;
           }
       }
       rect.x = 0x140;
       rect.y = 0xE0;
       rect.w = 0x40;
       rect.h = 0x20;
       LoadImage(&rect, (u_long*)pTexture);
       DrawSync(0);
       HeapFree(pBits);
       HeapFree(pTexture);
   }
*/
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetPlayTimeDigits(u32 value);

extern s32 D_801E9FE0[];
extern s32 D_801E9FE4[];

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2_260", func_801E68AC);
/* Best: 98.29% under gcc 2.6.0 (Default flags), 91.23% under the unit's own
   2.7.2 preset.  The whole gap is gcc 2.7.2's loop.c: it hoists the loop's y
   constant 0x7A out into a free call-saved register ($s6, adding its
   save/restore and shifting the frame), where retail re-materialises it with
   "li v0,0x7A" every iteration; that also flips the addu operand order of the
   loop's prim pointer (retail "addu a2,s0,a2").  2.6.0 keeps the li inside the
   loop, exactly as retail does, and the two functions are the same wall as
   func_801E56E8 above.  Under 2.6.0 what is left is four instructions of the
   first digit block: the dividend of number / 10 lands in t0 where retail uses
   v1, retail's "li s0,0x6E" fills the load-delay slot after the g_Menu load
   where this body emits it one slot later, and the dividend's sra follows one
   instruction later.  Everything else, including the second digit block and
   both magic-number divides, is identical.
   Structure confirmed: x/y tables D_801E9FE0[0],
   D_801E9FE4[0] then D_801E9FE0[i+2] in a 7-iteration loop, prim pointers
   0x240C/0x24AC/0x254C+i*0x50 off g_Menu->pPrimBuffer, then 0x2C7C, 0x2CCC,
   0x2D1C=(number/10), 0x2D6C=(number%10) with number = pArg[0x23]+1.
   Tried: an explicit u32 offset accumulator (loop addu becomes
   "addu a2,s0,a2" but gcc spills s7 and the frame grows to 0x48, 87.42%).
   void func_801E68AC(u8* pArg) {
       s32 number;
       s32 i;

       func_8002675C(g_Menu->resources, 0xEE,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x240C),
                     g_Menu->renderContext, D_801E9FE0[0], 0x7A, 0x1000);
       func_8002675C(g_Menu->resources, 0xEE,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x24AC),
                     g_Menu->renderContext, D_801E9FE4[0], 0x7A, 0x1000);
       MenuSetPlayTimeDigits(*(u32*)pArg);
       for (i = 0; i < 7; i++) {
           func_8002675C(g_Menu->resources, ((u32*)g_Menu->playTimeDigitIds)[i],
                         (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x254C + i * 0x50),
                         g_Menu->renderContext, D_801E9FE0[i + 2], 0x7A,
                         0x1000);
       }
       func_8002675C(g_Menu->resources, 0x17,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2C7C),
                     g_Menu->renderContext, 0x8, 0x66, 0x1000);
       func_8002675C(g_Menu->resources, 0x32,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2CCC),
                     g_Menu->renderContext, 0x10, 0x66, 0x1000);
       number = pArg[0x23];
       number++;
       func_8002675C(g_Menu->resources, number / 10,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2D1C),
                     g_Menu->renderContext, 0x10, 0x6E, 0x1000);
       number = pArg[0x23];
       number++;
       func_8002675C(g_Menu->resources, number % 10,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2D6C),
                     g_Menu->renderContext, 0x18, 0x6E, 0x1000);
   }
*/

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
