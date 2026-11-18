#include "common.h"

#include "psyq/libcd.h"
#include "psyq/pc.h"
#include "system/archive.h"
#include "system/menu.h"

// The 0x10-byte header of a file sector on the archive CD.
typedef struct {
    /* 0x0 */ u8 unk0[3];
    /* 0x3 */ u8 disc;
    /* 0x4 */ u32 tag;
    /* 0x8 */ u8 unk8[8];
} MenuSectorHeader; // Size: 0x10

// Height of the highlight quad drawn behind a menu row
#define MENU_ROW_QUAD_HEIGHT 13

// Distance between one party slot's prims and the next in g_Menu->pPrimBuffer
// Resource id offset to the selected (filled) variant of a menu row's cursor
#define MENU_SELECTED_RESOURCE_OFFSET 0xD
// Scratch buffer one rendered character name image is built in before LoadImage

// Destination coordinate of one name/slot texture upload, one 4-byte entry per
// menu row; only the low half is read, see MenuLoadSlotCharacterNameTexture.
typedef struct {
    /* 0x0 */ u16 pos;
    /* 0x2 */ u16 unk2;
} MenuTexturePos; // Size: 0x4

// Per-slot state bytes of the scratch prim buffer, past the 0x87C-byte block a
// party slot owns: the render context its prims were built with, and whether
// the slot has a character in it.
#define MENU_SLOT_PRIM_CONTEXT 0x130F
#define MENU_SLOT_PRIM_ENABLED 0x1310

extern u8 D_801EA8C4[0x20];
extern RECT D_801EA8E4;
extern RECT D_801EA8EC;
extern s32 g_MenuCardSlotTotals[];
extern u8 D_801EA8F5;
extern MenuSectorHeader D_801C5294;
extern char D_801C52A4[];
extern char D_801C52B8[];
extern char D_801C52CC[];
extern char D_801C52E0[];
extern char D_801C52F4[];
extern char D_801C5308[];

extern u_char g_MenuCdStatus;
extern int Vsync(int mode);
extern void ArchiveCdDataSync(int mode);
extern u_char ArchiveCdSetMode(u_char mode);
extern u32 ArchiveGetDebugTable(void);
extern void ArchiveStopTransfer(int mode);

void MenuClearGlyphTexture(void);
void MenuBuildGlyphTexture(s32 arg0);
void MenuDrawCharacterSlots(s32 arg0);
void func_801E7C50(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);
extern void* GetStringEntry(void* pStringData, u32 index);
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
void MenuUploadStringTextures(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);
void func_801E61B0(void);
void MenuBuildPlayTimePrims(u8* pEntry);
void MenuDrawSlotSprite(u8 index, u8* pEntry);
void MenuDrawSlotResourceSprites(u8 index, u8* pEntry);
void func_801E6CFC(u8 index, u8* pEntry);
void func_801E6F5C(u8 index, u8* pEntry);
void MenuLoadSlotCharacterNameTexture(s32 index, u8* pEntry, s32 arg2);
void func_801E733C(void);

extern s32 g_MenuSubMenuChoiceIds[];
extern s32 g_MenuSubMenuChoiceTextIds[];
extern MenuTexturePos g_MenuNameTextureX[];
extern MenuTexturePos g_MenuNameTextureY[];
extern s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
extern MenuTexturePos g_MenuSlotNameTextureX[];
extern MenuTexturePos g_MenuSlotNameTextureY[];
extern void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);
extern s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, u16 x, u16 y, u16 scale);
extern void MenuMovePointerCursor(s32 arg0, s32 arg1);

extern s32 D_801EA02C;
extern s32 D_801EA030;
extern s32 D_801EA034;
extern s32 D_801EA038;
extern s32 D_801EA03C;
extern s32 D_801EA040;
extern s32 D_801EA044;
extern s32 D_801EA048;

INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6CFC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6F5C);
/* Best: 68.99% (556 vs 484 bytes). Shape is certain (it is the MenuSetCharacterHpDigits
   idiom from misc7: MenuParseNumberToDigits, then a counter byte at
   unk34C + index*0x87C + 0x130A, then two loops over g_Menu->digits[6+i] that
   accumulate func_8002675C return values, the second one with an n counter for
   the x spacing). What is still wrong:
   - gcc 2.6 hoists `(u_char)index * 0x50` (the x spacing term) out of loop 1
     into the preheader; retail recomputes it in the body every iteration
     (`sll v0,s2,2; addu v0,v0,s2; sll v0,v0,4`, first insn in the beq delay
     slot). Tried: naming the sum in an s32 local (still hoisted), i*8 first,
     (u_char)index inline. This is the blocker.
   - retail keeps `(u_char)index` in s2 across both calls (`addu s2,v1,zero` in
     the jal delay slot) and i in s0 initialised in the PROLOGUE (move s0,zero
     right after sw s0,32(sp)); mine recomputes the andi after the call and puts
     `move i,0` in the jal delay slot.
   - retail's two addresses off the base differ in order: counter is
     addu a0,unk34C,offset (base first) but the poly base is
     addiu v0,offset,0xA98 then addu v0,v0,unk34C.
   - x/y need s32 (not u16) in the func_8002675C prototype or gcc narrows the
     load to `lhu` of the global; with u16 at 66.5%, with s32 68.99%.
   func_801E6F5C is the same body with: lbu fields pEntry[index+0x10] and
   pEntry[index+0x13], counter bytes 0x130C/0x130D, tables D_801EA03C/40/44/48,
   poly offsets 0x6E0/0x780, loop bound 2, digits[7+i].
void func_801E6CFC(s32 index, u8* pEntry) {
    s32 i = 0;
    s32 n;
    u32 offset;

    MenuParseNumberToDigits(*(u16*)(pEntry + (u_char)index * 2 + 4));
    offset = (u_char)index * 0x87C;
    ((u8*)g_Menu->pPrimBuffer + offset)[0x130A] = 0;
    for (; i < 3; i++) {
        if (g_Menu->digits[6 + i] != 0xFF) {
            ((u8*)g_Menu->pPrimBuffer + offset)[0x130A] += func_8002675C(g_Menu->resources,
                g_Menu->digits[6 + i],
                (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + offset + 0xA98
                    + (((u8*)g_Menu->pPrimBuffer + offset)[0x130A] * 0x50 + 0x500)),
                g_Menu->renderContext, D_801EA02C + (u_char)index * 0x50 + i * 8, D_801EA030, 0x1000);
        }
    }
    i = 0;
    MenuParseNumberToDigits(*(u16*)(pEntry + (u_char)index * 2 + 0xA));
    ((u8*)g_Menu->pPrimBuffer + offset)[0x130B] = 0;
    n = 0;
    for (; i < 3; i++) {
        if (g_Menu->digits[6 + i] != 0xFF) {
            ((u8*)g_Menu->pPrimBuffer + offset)[0x130B] += func_8002675C(g_Menu->resources,
                g_Menu->digits[6 + i],
                (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + offset + 0xA98
                    + (((u8*)g_Menu->pPrimBuffer + offset)[0x130B] * 0x50 + 0x5F0)),
                g_Menu->renderContext, D_801EA034 + (u_char)index * 0x50 + n * 8, D_801EA038, 0x1000);
            n++;
        }
    }
}
*/
void MenuLoadSlotCharacterNameTexture(s32 index, u8* pEntry, s32 arg2) {
    RECT rect;
    u8 pairs[0x14];
    u8 text[0x18];
    u8* pKey;
    u8* pValue;
    u8* pTable;
    u8* pRow;
    u8* pBuf;
    s32 i;

    i = 0;
    pEntry += (u_char)index;
    pTable = (u8*)(arg2 * MENU_IMAGE_CHUNK_SIZE + (s32)g_Menu->menuUnk2) + 0xC94;
    pValue = pairs + 1;
    pKey = pairs;
    for (; i < 0x14; i += 2) {
        pRow = pTable + pEntry[0x1C] * 0x14 + i;
        *pKey = pRow[0x24];
        pRow = pTable + pEntry[0x1C] * 0x14 + i;
        *pValue = pRow[0x25];
        if (!*pKey && !*pValue) {
            break;
        }
        pValue += 2;
        pKey += 2;
    }
    SystemDecodeGlyphCodesN((u16*)pairs, text, i / 2);
    pBuf = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);
    bzero(pBuf, MENU_STRING_IMAGE_BUFFER_SIZE);
    SystemRenderStringEntry(text, pBuf, 0x24, 0);
    rect.x = g_MenuSlotNameTextureX[(u_char)index].pos + 0x180;
    rect.y = g_MenuSlotNameTextureY[(u_char)index].pos;
    rect.w = 0x28;
    rect.h = FONT_LETTER_HEIGHT;
    LoadImage(&rect, (u_long*)pBuf);
    DrawSync(0);
    HeapFree(pBuf);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E733C);
void MenuDrawCharacterSlots(s32 arg0) {
    s32 i = 0;
    u32 offset = 0;
    u8* pEntry;

    pEntry = (u8*)g_Menu->menuUnk2 + arg0 * MENU_IMAGE_CHUNK_SIZE + 0xC94;
    func_801E61B0();
    for (; i < MAX_PARTY_MEMBERS; i++) {
        if (*(pEntry + i + 0x1C) != CHARACTER_ID_NONE) {
            ((u8*)g_Menu->pPrimBuffer + offset)[MENU_SLOT_PRIM_ENABLED] = 1;
            MenuDrawSlotSprite(i, pEntry);
            MenuDrawSlotResourceSprites(i, pEntry);
            func_801E6CFC(i, pEntry);
            func_801E6F5C(i, pEntry);
            MenuLoadSlotCharacterNameTexture((u_char)i, pEntry, arg0);
        } else {
            ((u8*)g_Menu->pPrimBuffer + offset)[MENU_SLOT_PRIM_ENABLED] = 0;
        }
        ((u8*)g_Menu->pPrimBuffer + offset)[MENU_SLOT_PRIM_CONTEXT] = (u_char)g_Menu->renderContext;
        offset += MENU_SLOT_PRIM_STRIDE;
    }
    MenuBuildPlayTimePrims(pEntry);
    func_801E733C();
}

void MenuUpdateCharacterWindow(s32 arg0, u_char arg1) {
    MenuClearGlyphTexture();
    if (arg0 != 0xFF) {
        if (arg1 != 0) {
            MenuDrawCharacterSlots(arg0);
            MenuBuildGlyphTexture(arg0);
            ((u8*)g_Menu->pPrimBuffer)[0x2DBC] = 1;
        } else {
            MenuBuildGlyphTexture(arg0);
            ((u8*)g_Menu->pPrimBuffer)[0x2DBC] = 0;
        }
        g_Menu->pManager->characterWindowBuilt = 1;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E78C8);
// Triage r111: BUILDS now (the externs at the top of the file: u8 D_801EA8C4[0x20], RECT D_801EA8E4, RECT D_801EA8EC, s32 g_MenuCardSlotTotals[]). Real score of this body: 84.42%, 948 bytes.
/* Best: 84.42% (948 vs 904 bytes), first full shape. Decoded: a 0x20-byte CLUT
   (16 entries) is memmoved out of MenuUnk2ImageChunk.pad0[0x60] into the buffer at
   D_801EA8C4 and uploaded with LoadImage to the RECT at D_801EA8EC
   (x = index*0x10, y = index/0x10 + 0x1C1, w = 0x10, h = 1); then three 0x80-byte
   texture blocks are uploaded to the RECT at D_801EA8E4
   (x = index*4 + 0x140 - (index/0x10)*0x40, y = i*0x20 + (index/0x10)*0x10 + 0x80,
   w = 4, h = 0x10), and finally the chunk's type byte (chunks[index].pad0[2],
   values 0x11/0x12/0x13) selects six 32-bit UV words in menuUnk2->records[index]
   (those six words live inside MenuUnk2Record.unk0, hence the indexed form),
   chunks[index].pad0[3] is added to g_MenuCardSlotTotals[index/0x10], and
   records[index].unk58 is cleared when the type was unexpected.
   What is still wrong, in retail's order of appearance:
   - retail materialises &D_801EA8E4 (offset 0) in s0, derives s7 = s0+2 and uses
     a0 = s7-2 for the loop LoadImage; this shape materialises s7 (= D_801EA8E4+2)
     and derives offset 0 instead, so every plain D_801EA8E4.y/x store re-forms its
     address with lui at (11 instructions of overrun). Tried: plain struct stores,
     a RECT pointer local for the base, and the pos/neg offset swapped.
   - retail keeps the D_801EA8C4 buffer address in s1 across the memmove so the
     LoadImage argument is a plain move; this shape reloads it with la.
   - the x expression must be written index*4 + 0x140 - (index/0x10)*0x40 to get
     retail's addiu v0,v0,-0x140 then subu (the other association emits addiu +320).
   - shouldClear = 1 must be the FIRST statement (retail emits li s6,1 in the
     prologue, before the rect stores).
/*
extern u8 D_801EA8C4[0x20];
extern RECT D_801EA8E4;
extern RECT D_801EA8EC;
extern s32 g_MenuCardSlotTotals[];

void func_801E78C8(s32 index) {
    s32 i;
    u_char shouldClear;

    shouldClear = 1;
    D_801EA8E4.x = index * 4 + 0x140 - (index / 0x10) * 0x40;
    D_801EA8E4.w = 4;
    D_801EA8E4.h = 0x10;
    D_801EA8EC.x = index * 0x10;
    D_801EA8EC.y = index / 0x10 + 0x1C1;
    D_801EA8EC.w = 0x10;
    D_801EA8EC.h = 1;
    memmove(D_801EA8C4, &g_Menu->menuUnk2->chunks[index].pad0[0x60], 0x20);
    LoadImage(&D_801EA8EC, (u_long*)D_801EA8C4);
    DrawSync(0);
    for (i = 0; i < 3; i++) {
        D_801EA8E4.y = i * 0x20 + (index / 0x10) * 0x10 + 0x80;
        LoadImage(&D_801EA8E4, (u_long*)&g_Menu->menuUnk2->chunks[index].pad0[0x80 + i * 0x80]);
        DrawSync(0);
    }
    switch (g_Menu->menuUnk2->chunks[index].pad0[2]) {
        case 0x11:
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[0] = (index / 0x10) * 0x10 + 0x80;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[1] = (index / 0x10) * 0x10 + 0x80;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[2] = (index / 0x10) * 0x10 + 0x80;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[3] = (index / 0x10) * 0x10 + 0x80;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[4] = (index / 0x10) * 0x10 + 0x80;
            shouldClear = 0;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[5] = (index / 0x10) * 0x10 + 0x80;
            break;
        case 0x12:
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[0] = (index / 0x10) * 0x10 + 0x80;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[1] = (index / 0x10) * 0x10 + 0xA0;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[2] = (index / 0x10) * 0x10 + 0x80;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[3] = (index / 0x10) * 0x10 + 0xA0;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[4] = (index / 0x10) * 0x10 + 0x80;
            shouldClear = 0;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[5] = (index / 0x10) * 0x10 + 0xA0;
            break;
        case 0x13:
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[0] = (index / 0x10) * 0x10 + 0x80;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[1] = (index / 0x10) * 0x10 + 0xA0;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[2] = (index / 0x10) * 0x10 + 0xC0;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[3] = (index / 0x10) * 0x10 + 0x80;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[4] = (index / 0x10) * 0x10 + 0xA0;
            shouldClear = 0;
            ((s32*)g_Menu->menuUnk2->records[index].unk0)[5] = (index / 0x10) * 0x10 + 0xC0;
            break;
    }
    g_MenuCardSlotTotals[index / 0x10] += g_Menu->menuUnk2->chunks[index].pad0[3];
    if (shouldClear) {
        g_Menu->menuUnk2->records[index].unk58 = 0;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E7C50);
// Triage r111: BUILDS, real score 72.41%, 528 bytes. No new declarations were needed beyond MenuString/POLY_FT4.
/* Best: 72.41% under gcc-2.6.0 (62.66% under 2.7.2). Shape recovered from the asm:
   row = index / 2 and column = index & 1 are separate locals (retail keeps row in
   s4 and reuses it for the flags != 0 branch's row * 0xD), u = (row & 1) * 0x80 is
   loop-invariant and rides in s3, the else branch computes column * 0x60 as a
   fresh temp (v1, not the u variable), color = 0 is reset at the top of each
   iteration and the yOffset parameter is spilled to 16(sp) by retail. What is
   still wrong is the allocator: retail puts pStringList in s2, index in s8,
   flags in s7 and spills yOffset, my compile keeps yOffset in s8 and gives
   pStringList s3, plus the branch-1 u1/u2 store order and one extra saved reg.
void func_801E7C50(MenuString* pStringList, s32 index, s32 yOffset, s32 flags) {
    POLY_FT4* pPoly;
    u8 color;
    s32 row;
    s32 column;
    s32 u;
    s32 v;
    s32 i;

    row = index / 2;
    column = index & 1;
    u = (row & 1) * 0x80;
    for (i = 0; i < 2; i++) {
        color = 0;
        pPoly = &pStringList->polys[i];
        MenuInitializePolyFt4(pPoly);
        if ((flags & 0xFF) == 0) {
            pStringList->palette = column;
            pPoly->tpage = GetTPage(0, 0, 0x140, 0);
            pPoly->u0 = u;
            v = (index + yOffset) / 4 * 0xD;
            pPoly->v0 = v;
            pPoly->u1 = u + pStringList->width;
            pPoly->v1 = v;
            pPoly->u2 = u;
            pPoly->v2 = v + 0xD;
            pPoly->u3 = u + pStringList->width;
            pPoly->v3 = v + 0xD;
        } else {
            if (flags & 0x80) {
                color = 0x20;
                SetSemiTrans(pPoly, 1);
                setRGB0(pPoly, 0x20, 0x20, 0x20);
            }
            pStringList->palette = (flags & 0x7F) + 0xFF;
            pPoly->tpage = color | GetTPage(0, 0, 0x180, 0x80);
            v = row * 0xD + yOffset;
            pPoly->u0 = column * 0x60;
            pPoly->v0 = v;
            pPoly->v1 = v;
            pPoly->u2 = column * 0x60;
            pPoly->v2 = v + 0xD;
            pPoly->u1 = column * 0x60 + pStringList->width;
            pPoly->v3 = v + 0xD;
            pPoly->u3 = column * 0x60 + pStringList->width;
        }
        if (pStringList->palette != 0) {
            pPoly->clut = g_SystemPalette2;
        } else {
            pPoly->clut = g_SystemPalette1;
        }
    }
    pStringList->hasVertices = 0;
}
*/

void MenuUploadStringTextures(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count) {
    RECT* pRect;
    s32 i;
    s32 index;

    for (i = 0; i < count; i += 2) {
        index = i;
        pStringList[index].width = SystemRenderStringEntry(GetStringEntry(g_Menu->pStringBank, pStringIdList[index]),
            g_Menu->titleStrings[0].pVramBuffer, 0x18, 0);
        pRect = &pStringList[index].vramDest;
        pStringList[index + 1].width = SystemRenderStringEntry(GetStringEntry(g_Menu->pStringBank, pStringIdList[index + 1]),
            g_Menu->titleStrings[0].pVramBuffer, 0x18, 1);
        pStringList[index].vramDest.x = ((index / 2) & 1) * 0x20 + 0x140;
        pStringList[index].vramDest.y = ((index + yOffset) / 4) * FONT_LETTER_HEIGHT;
        pStringList[index].vramDest.w = 0x1C;
        pStringList[index].vramDest.h = FONT_LETTER_HEIGHT;
        pStringList[index + 1].vramDest = pStringList[index].vramDest;
        func_801E7C50(&pStringList[index], index, yOffset, 0);
        func_801E7C50(&pStringList[index + 1], index + 1, yOffset, 0);
        LoadImage(pRect, g_Menu->titleStrings[0].pVramBuffer);
        DrawSync(0);
    }
}
void MenuLoadStringsFromIds(u_char count, MenuString* pStringList, u_char* pStringIdList, void* pUnused) {
    MenuUploadStringTextures(pStringList, pStringIdList, 4, count);
}

void MenuClearBytes(u_char count, u_char* pBuffer) {
    s32 end;

    if (count != 0) {
        end = count + (s32)pBuffer;
        do {
            *pBuffer++ = 0;
        } while ((s32)pBuffer < end);
    }
}
extern u32 g_MenuCursorXPositions[]; // Cursor x positions
extern s32 g_MenuCursorYPositions[]; // Cursor y positions
extern u32 g_MenuRowQuadMode1Xs[];
extern u32 g_MenuRowQuadMode1Y[];
extern u32 g_MenuRowQuadMode2Xs[];
extern u32 g_MenuRowQuadMode2Ys[];
extern u32 g_MenuRowQuadMode3Ys[];
extern u32 g_MenuRowQuadMode6Xs[];
extern u32 g_MenuRowQuadMode6Ys[];
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);

void MenuSetRowQuadVertices(s32 count, MenuString* pRows, u8* pStringIds, u32* pXOffsets, u8* pFlags, u_char row,
    u_char col, u_char mode) {
    s32 offset = 0;

    switch (mode) {
    case 0:
        MenuClearBytes(count, pFlags);
        (g_Menu->renderContext + pRows[row].polys)->x0 = g_MenuCursorXPositions[col + row] + (pXOffsets[row] + 0x16);
        (g_Menu->renderContext + pRows[row].polys)->y0 = g_MenuCursorYPositions[col + row] - 0x22;
        (g_Menu->renderContext + pRows[row].polys)->x1 = (u16)(g_MenuCursorXPositions[col + row] + 0x16) + pXOffsets[row] + pRows[row].width;
        (g_Menu->renderContext + pRows[row].polys)->y1 = g_MenuCursorYPositions[col + row] - 0x22;
        (g_Menu->renderContext + pRows[row].polys)->x2 = g_MenuCursorXPositions[col + row] + (pXOffsets[row] + 0x16);
        (g_Menu->renderContext + pRows[row].polys)->y2 = g_MenuCursorYPositions[col + row] - 0x15;
        (g_Menu->renderContext + pRows[row].polys)->x3 = (u16)(g_MenuCursorXPositions[col + row] + 0x16) + pXOffsets[row] + pRows[row].width;
        (g_Menu->renderContext + pRows[row].polys)->y3 = g_MenuCursorYPositions[col + row] - 0x15;
        break;
    case 1:
        MenuSetQuadVertices(pRows[row].vertices, g_MenuRowQuadMode1Xs[row], g_MenuRowQuadMode1Y[0], pRows[row].width, MENU_ROW_QUAD_HEIGHT);
        break;
    case 5:
        offset = 8;
        // fallthrough
    case 2:
        MenuSetQuadVertices(pRows[row].vertices, g_MenuRowQuadMode2Xs[offset + row], g_MenuRowQuadMode2Ys[col], pRows[row].width, MENU_ROW_QUAD_HEIGHT);
        break;
    case 3:
        MenuSetQuadVertices(pRows[row].vertices, 0x18, g_MenuRowQuadMode3Ys[col], pRows[row].width, MENU_ROW_QUAD_HEIGHT);
        break;
    case 4:
        (g_Menu->renderContext + pRows[row].polys)->x0 = 0x4C;
        (g_Menu->renderContext + pRows[row].polys)->y0 = 0x12;
        (g_Menu->renderContext + pRows[row].polys)->x1 = pRows->width + 0x4C;
        (g_Menu->renderContext + pRows[row].polys)->y1 = 0x12;
        (g_Menu->renderContext + pRows[row].polys)->x2 = 0x4C;
        (g_Menu->renderContext + pRows[row].polys)->y2 = 0x1F;
        (g_Menu->renderContext + pRows[row].polys)->x3 = pRows->width + 0x4C;
        (g_Menu->renderContext + pRows[row].polys)->y3 = 0x1F;
        break;
    case 6:
        MenuSetQuadVertices(pRows[row].vertices, g_MenuRowQuadMode6Xs[row], g_MenuRowQuadMode6Ys[row], pRows[row].width, MENU_ROW_QUAD_HEIGHT);
        break;
    }
    pRows[row].renderContext = g_Menu->renderContext;
    pFlags[row] = 1;
}
void MenuRenderFrame(void);

void MenuRevealSelectionMenu(s32 count, s32* pEntries) {
    s32 i;
    s32 j;
    u32 off;

    g_Menu->pSelectionMenu->isDisabled = 0;
    g_Menu->pSelectionMenu->isDisabledRendered = 0;
    g_Menu->pManager->shouldRenderSelectionMenu = 1;
    for (i = 1; i <= count; i++) {
        if (i != count) {
            g_Menu->pSelectionMenu->numCursors = 0;
            for (j = 0; j < i; j++) {
                g_Menu->pSelectionMenu->numCursors = func_8002675C(g_Menu->resources, pEntries[j * 2],
                    (POLY_FT4*)(g_Menu->pSelectionMenu->numCursors * 0x50 + (s32)g_Menu->pSelectionMenu),
                    g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numCursors;
            }
            g_Menu->pSelectionMenu->cursorsRenderCtx = *(u8*)&g_Menu->renderContext;
        }
        g_Menu->pSelectionMenu->numTexts = 0;
        if (i != 1) {
            for (j = 0; j < i - 1; j++) {
                off = g_Menu->pSelectionMenu->numTexts * 0x50 + 0x8C0;
                g_Menu->pSelectionMenu->numTexts = func_8002675C(g_Menu->resources, pEntries[j * 2 + 1],
                    (POLY_FT4*)(off + (s32)g_Menu->pSelectionMenu),
                    g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numTexts;
            }
            g_Menu->pSelectionMenu->textsRenderCtx = *(u8*)&g_Menu->renderContext;
        }
        for (j = 0; j < 2; j++) {
            MenuRenderFrame();
        }
    }
}

// Triage r111: BUILDS, real score 99.79%, 688 bytes = retail's size. Only g_MenuSubMenuChoiceIds / g_MenuSubMenuChoiceIds+4 base order is left; a typed 2-word struct view is WRONG (retail's index math is the flat s32[] one: sll i,1 + sll (sel+choice),3 + addu + sll 2).
/* Best: 99.79% (688 bytes, exact instruction multiset, only the two table
   bases swap). The missing structural piece: loop 2's guard tests the CURSOR
   word of the same entry, not the text word -- retail compares
   g_MenuSubMenuChoiceIds[idx] (computed off s4) but passes g_MenuSubMenuChoiceIds[idx+1] to
   func_8002675C. That is why retail keeps the byte offset in a register and
   materialises the second base once in the entry block as `addiu s5,s4,4`.
   What is left is only how the two bases are formed: retail has
   lui/addiu of g_MenuSubMenuChoiceIds into s4 then `addiu s5,s4,4`; this body has
   lui/addiu of g_MenuSubMenuChoiceIds+4 into s5 then `addiu s4,s5,-4`. Two pointer locals
   (`pCursor = g_MenuSubMenuChoiceIds; pText = pCursor + 1;` or the +1 form) get the right
   pair but gcc then hoists both inits to the top of the entry block, ahead of
   the s2/s1/s3 saves (88.09%); initialising pText inside the outer loop gives
   90.16%. Swapping the pText init between the two unk354 stores does not
   change the order either.
   Round 104: forcing the base-first pair with two pointer locals
   (`pCursor = g_MenuSubMenuChoiceIds; pText = pCursor + 1;` and pCursor[] reads in both
   loops) DOES give retail's s4 = &g_MenuSubMenuChoiceIds + addiu s5,s4,4, but it drops to
   88.09%: the invariant then hoists to the very top of the entry block, ahead of
   the li/andi saves and the 0x1400 store, instead of sitting between the two
   0x354 stores. Moving the assignment between the stores in the source changes
   nothing (still 88.09). Permuter 3 min from the 99.79% body: no gain. So the
   99.79% body below is the best one; what is left is only which of
   g_MenuSubMenuChoiceIds / g_MenuSubMenuChoiceIds+4 gcc materialises first.
   Round 113 (measured): the pair DIRECTION is solvable, the PROLOGUE order is
   the wall. Two pointer locals in every spelling tried (`pCursor = g_MenuSubMenuChoiceIds;
   pText = pCursor + 1;`, `pText = &g_MenuSubMenuChoiceIds[1]`, `(s32*)((u8*)pCursor + 4)`,
   the two inits swapped, an empty do/while after the pair) all give retail's
   exact pair (la s4 = g_MenuSubMenuChoiceIds then addu s5,s4,4) and all land on 88.09%,
   because gcc emits BOTH invariants at the head of the entry block, ahead of the
   remaining saves: sw s2 / li s2 / la s4 / sw s5 / addu s5,s4,4 / sw ra / sw s3
   / sw s1 / sw s0 / lw v0,852. One address local keeps retail's order (its init
   is scheduled into the lw 852 delay slot) but can only derive the OTHER base:
   loop-1 base as `pCursor[idx]` with the text access inline (`pCursor[idx + 1]`,
   `g_MenuSubMenuChoiceIds[idx + 1]`) or both as symbols (`g_MenuSubMenuChoiceTextIds[idx]`) drops s5
   entirely - 86.67% and 91.27%, 680 bytes. Permuter 4 min from the 99.79% body:
   no gain. Next lever: something that stops the entry block from being the
   loop preheader, so the hoisted pair lands after the stores.
   Round 112: the inline `g_MenuSubMenuChoiceIds[idx + 1]` form (no pText local) makes gcc
   form a second full lui/addiu of g_MenuSubMenuChoiceTextIds in loop 2's preheader (91.84%), and
   a single pointer local folds to the symbol the same way. Confirmed the two
   pointer locals give retail's exact pair (`lui/addiu s4` + `addiu s5,s4,4`) but
   88.09% because both invariants are emitted inside the prologue save region;
   adding a live `u_char sel = selected;` copy does not move them.
   Round 114 (measured, 8 shapes). The anchor direction follows reload_cse:
   whichever table address is materialised FIRST in the RTL survives as the
   `la` and the other becomes `addiu <other>,<that>,+-4`. Baseline body anchors
   g_MenuSubMenuChoiceIds+4 (s5) so the cursor base is `addiu s4,s5,-4`. Whenever a SECOND
   live pointer local exists (dependent `pText = pCursor + 1`, two independent
   ones, two `u8*` ones, either source order, with or without a do/while) the
   pair is right (`la s4,g_MenuSubMenuChoiceIds` + `addiu s5,s4,4`) but the `la` is emitted
   inside the save region (right after `sw s4`) instead of retail's slot after
   `lw v0,852`, so 88.09% (688 bytes). Moving both inits inside the outer loop
   so loop.c hoists them gives the same split (`la` in the save region,
   `addu s5,s4,4` in retail's slot) at 88.16%. With one local (this body) the
   save order is retail's exactly and the pair lands in retail's slots.
   Left to solve: which of `la g_MenuSubMenuChoiceIds` / `la g_MenuSubMenuChoiceIds+4` is emitted first
   without displacing it into the save region.
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E86C8);
/* Best: 99.79% (688 bytes, exact instruction multiset; only the two table bases' direction differs).

void func_801E86C8(u_char selected) {
    s32 i;
    s32 n;
    s32 allValid;
    u32 off;
    s32* pText;

    allValid = 1;
    g_Menu->pSubmenuPrims->polys0PrimCount = 0;
    g_Menu->pSubmenuPrims->polys500PrimCount = 0;
    g_Menu->pManager->shouldRenderSubmenuStrings = 1;
    pText = g_MenuSubMenuChoiceIds + 1;
    for (n = 1; n < 5; n++) {
        g_Menu->pSubmenuPrims->polys0PrimCount = 0;
        g_Menu->subMenuNumChoices = 0;
        for (i = 0; i < n; i++) {
            if (g_MenuSubMenuChoiceIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2] != 0xFFFF) {
                g_Menu->pSubmenuPrims->polys0PrimCount = func_8002675C(g_Menu->resources,
                    g_MenuSubMenuChoiceIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2],
                    (POLY_FT4*)(g_Menu->pSubmenuPrims->polys0PrimCount * 0x50 + (s32)g_Menu->pSubmenuPrims),
                    g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSubmenuPrims->polys0PrimCount;
                g_Menu->subMenuNumChoices++;
            } else {
                allValid = 0;
            }
        }
        g_Menu->pSubmenuPrims->polys0RenderCtx = *(u8*)&g_Menu->renderContext;
        if (allValid) {
            for (i = 0; i < 2; i++) {
                MenuRenderFrame();
            }
        }
        g_Menu->pSubmenuPrims->polys500PrimCount = 0;
        for (i = 0; i < n; i++) {
            if (g_MenuSubMenuChoiceIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2] != 0xFFFF) {
                off = g_Menu->pSubmenuPrims->polys500PrimCount * 0x50 + 0x500;
                g_Menu->pSubmenuPrims->polys500PrimCount = func_8002675C(g_Menu->resources,
                    pText[(selected + g_Menu->mainMenuChoice) * 8 + i * 2],
                    (POLY_FT4*)(off + (s32)g_Menu->pSubmenuPrims),
                    g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSubmenuPrims->polys500PrimCount;
            }
        }
        g_Menu->pSubmenuPrims->polys500RenderCtx = *(u8*)&g_Menu->renderContext;
        if (allValid) {
            for (i = 0; i < 2; i++) {
                MenuRenderFrame();
            }
        }
    }
}
*/
void MenuSetSelectionMenuPrims(u_char count, u_char selected, s32* pEntries) {
    s32 i;
    u32 index;
    u32 off;

    g_Menu->pSelectionMenu->numCursors = 0;
    g_Menu->pSelectionMenu->numTexts = 0;
    for (i = 0; i < count; i++) {
        if (i == selected) {
            index = pEntries[i * 2] + MENU_SELECTED_RESOURCE_OFFSET;
        } else {
            index = pEntries[i * 2];
        }
        g_Menu->pSelectionMenu->numCursors = func_8002675C(g_Menu->resources, index,
            (POLY_FT4*)(g_Menu->pSelectionMenu->numCursors * 0x50 + (s32)g_Menu->pSelectionMenu),
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numCursors;
        // kept in a local so gcc does not fold 0x8C0 onto the base pointer
        off = g_Menu->pSelectionMenu->numTexts * 0x50 + 0x8C0;
        g_Menu->pSelectionMenu->numTexts = func_8002675C(g_Menu->resources, pEntries[i * 2 + 1],
            (POLY_FT4*)(off + (s32)g_Menu->pSelectionMenu),
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numTexts;
    }
    g_Menu->pSelectionMenu->cursorsRenderCtx = *(u8*)&g_Menu->renderContext;
    g_Menu->pSelectionMenu->textsRenderCtx = *(u8*)&g_Menu->renderContext;
    MenuMovePointerCursor(selected, 1);
    g_Menu->pManager->shouldRenderPointerCursor = 1;
}
void MenuSetSubMenuChoicePrims(u_char selected) {
    s32 i;
    u32 index;
    u32 off;

    g_Menu->pSubmenuPrims->polys0PrimCount = 0;
    g_Menu->pSubmenuPrims->polys500PrimCount = 0;
    for (i = 0; i < g_Menu->subMenuNumChoices; i++) {
        if (i == g_Menu->subMenuChoice) {
            index = g_MenuSubMenuChoiceIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2] + MENU_SELECTED_RESOURCE_OFFSET;
        } else {
            index = g_MenuSubMenuChoiceIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2];
        }
        g_Menu->pSubmenuPrims->polys0PrimCount = func_8002675C(g_Menu->resources, index,
            (POLY_FT4*)(g_Menu->pSubmenuPrims->polys0PrimCount * 0x50 + (s32)g_Menu->pSubmenuPrims),
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSubmenuPrims->polys0PrimCount;
        off = g_Menu->pSubmenuPrims->polys500PrimCount * 0x50 + 0x500;
        g_Menu->pSubmenuPrims->polys500PrimCount = func_8002675C(g_Menu->resources,
            g_MenuSubMenuChoiceTextIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2],
            (POLY_FT4*)(off + (s32)g_Menu->pSubmenuPrims),
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSubmenuPrims->polys500PrimCount;
    }
    g_Menu->pSubmenuPrims->polys0RenderCtx = *(u8*)&g_Menu->renderContext;
    g_Menu->pSubmenuPrims->polys500RenderCtx = *(u8*)&g_Menu->renderContext;
    MenuMovePointerCursor(g_Menu->subMenuChoice + 7, 1);
    g_Menu->pManager->shouldRenderPointerCursor = 1;
}
void MenuLoadCharacterNameTexture(s32 arg0, s32 arg1) {
    RECT rect;
    u8* pBuf;
    s32 offset;

    pBuf = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);
    bzero(pBuf, MENU_STRING_IMAGE_BUFFER_SIZE);
    if ((u_char)arg0 != CHARACTER_ID_NONE) {
        offset = ((u_char)arg0 >> 1) * 0x28;
        SystemRenderStringEntry((u8*)&g_GameState + offset, pBuf, 0x24, 0);
        SystemRenderStringEntry((u8*)&g_GameState + offset + 0x14, pBuf, 0x24, 1);
    }
    rect.x = g_MenuNameTextureX[((arg1 * 2) & 0x1FC) / sizeof(MenuTexturePos)].pos + 0x180;
    rect.y = g_MenuNameTextureY[((arg1 * 2) & 0x1FC) / sizeof(MenuTexturePos)].pos;
    rect.w = 0x28;
    rect.h = FONT_LETTER_HEIGHT;
    LoadImage(&rect, (u_long*)pBuf);
    DrawSync(0);
    HeapFree(pBuf);
}

void MenuSetPrimShadingMode(POLY_FT4* pPoly, u_char mode) {
    SetShadeTex(pPoly, 0);
    switch (mode) {
    case 1:
        pPoly->tpage |= 0x20;
        SetSemiTrans(pPoly, 1);
        // fallthrough
    case 3:
        setRGB0(pPoly, 33, 33, 33);
        break;
    case 0:
        SetSemiTrans(pPoly, 0);
        // fallthrough
    case 2:
        setRGB0(pPoly, 128, 128, 128);
        break;
    }
}
// Triage r111: as stored here it is 98.20% (612 bytes = retail's size). The addiu-on-offset shape does work: with "s32 off; off = (i * 2 + pWindow->renderContext) * 0x28 + K; MenuSetPrimShadingMode((POLY_FT4*)(off + (u32)pWindow), mode);" in each of the five K != 0 loops, the final block in its own braced { s32 off; ... } with (u_char)mode, and loop 1 as "(...) * 0x28 + (u32)pWindow" (base last), it reaches 99.51%. Residual: the last call re-truncates mode (retail andi a1,s1,0xFF, gcc proves 2..3 and emits move a1,s1) and retail loads pWindow into v1 there where gcc picks a2. Permuter 3 min from the 99.51% body: no gain (base=215 best=215).
// Round 121: the call sites are the MenuWindow poly arrays (polysWindowBorderCorners[8] at 0x0,
// polysWindowBorderTop/Bottom/Left/Right[4], polysScrollBarEnds[4] at 0x410, polysScrollBarEmpty[2]),
// indexed [i * 2 + pWindow->renderContext]; that form gives retail's "addiu a0,a0,K" on the index
// chain and 612 bytes. 99.08%: every call's final add is mirrored ("addu a0,a2,a0" vs retail's
// "addu a0,a0,a2"), the last block loads the window into a2 where retail uses v1, and the last call
// emits "move a1,s1" where retail keeps "andi a1,s1,0xFF".
// Round 121: the call sites are the MenuWindow poly arrays (polysWindowBorderCorners at 0x0,
// polysWindowBorderTop/Bottom/Left/Right at 0x140/0x1E0/0x280/0x320, polysScrollBarEnds at 0x410,
// polysScrollBarEmpty at 0x3C0) indexed [i * 2 + pWindow->renderContext]: that reproduces retail's
// "addiu a0,a0,K" on the index chain and lands on 612 bytes, retail's exact size.
/* Best: 99.61% (612 bytes). ONE instruction is left: the final call passes mode as "move a1,s1"
   where retail keeps "andi a1,s1,0xFF" (same value, since mode is only 2 or 3). Every other
   instruction, register and delay slot is retail's.
   What got here from 96.65: write each call as
   "&g_Menu->windows[arg0]->polyX[i * 2 + g_Menu->windows[arg0]->renderContext]". A per-loop
   "pWindow = g_Menu->windows[arg0]" local instead gives retail's chain in a0 and the addiu on the
   chain but the final add mirrored ("addu a0,a2,a0" vs retail's "addu a0,a0,a2"); the
   int-local/inline spellings ("off + (u32)pWindow", "chain + K + (u32)pWindow") fix that add order
   but then fold K onto the window ("addiu v0,a2,0xK") - 96.70-99.02.
   Tried for the andi: (u_char)mode cast, "mode & 0xFF", an empty do/while before it, dropping the
   pWindow local entirely, and 2 permuter runs from this body (no gain, base=200 best=200).
   Round 123 (7 more spellings, all 99.61% / 612 bytes, all identical): "(u_char)mode" on the last
   call, "u8 shade = mode;" in its own block, "s32 shade = mode;" (assigned and declared-then-
   assigned), "mode & 0xFF", "do { call } while (0)", and "u_char mode;" as the declaration. The
   delta is a gcc 2.6.0 value-tracking artifact, NOT a source shape: a scratch probe compiled in
   this unit (`s32 mode = sel ? 3 : 2; call(p, mode); call(p, mode);`) elides the narrowing in
   straight-line code, and with a loop in front (`for (...) call(p + i, mode); call(p, mode);`) the
   loop body keeps `andi` while the trailing call still elides it. So retail's andi cannot be
   restored by how the last argument is spelled; a different wrap-up block would be needed, and
   gcc 2.7.2 scores this function worse (99.15 under try_flags.sh).
   void func_801E8F60(u8 arg0, u8 arg1) {
       s32 mode;
       s32 i;

       mode = arg1 ? 3 : 2;
       for (i = 0; i < 4; i++) {
           MenuSetPrimShadingMode(&g_Menu->windows[arg0]->polysWindowBorderCorners[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
       }
       for (i = 0; i < 4; i++) {
           MenuSetPrimShadingMode(&g_Menu->windows[arg0]->polysWindowBorderTop[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
       }
       for (i = 0; i < 4; i++) {
           MenuSetPrimShadingMode(&g_Menu->windows[arg0]->polysWindowBorderBottom[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
       }
       for (i = 0; i < 4; i++) {
           MenuSetPrimShadingMode(&g_Menu->windows[arg0]->polysWindowBorderLeft[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
       }
       for (i = 0; i < 4; i++) {
           MenuSetPrimShadingMode(&g_Menu->windows[arg0]->polysWindowBorderRight[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
       }
       for (i = 0; i < 2; i++) {
           MenuSetPrimShadingMode(&g_Menu->windows[arg0]->polysScrollBarEnds[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
       }
       MenuSetPrimShadingMode(&g_Menu->windows[arg0]->polysScrollBarEmpty[g_Menu->windows[arg0]->renderContext], mode);
   }
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8F60);
void MenuSetPolySemiTransGrey(P_TAG* pTag) {
    SetSemiTrans(pTag, 1);
    SetShadeTex(pTag, 0);
    setRGB0(pTag, 128, 128, 128);
}
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, short x, short y, u_char u, u_char v, s32 width, s32 height) {
    pPoly->x0 = x;
    pPoly->y0 = y;
    pPoly->y1 = y;
    pPoly->x2 = x;
    pPoly->u0 = u;
    pPoly->u2 = u;
    pPoly->x1 = x + width;
    pPoly->y2 = y + height;
    pPoly->x3 = x + width;
    pPoly->y3 = y + height;
    pPoly->v0 = v;
    pPoly->u1 = u + width;
    pPoly->v1 = v;
    pPoly->v2 = v + height;
    pPoly->u3 = u + width;
    pPoly->v3 = v + height;
}
void MenuInitializePolyFt4(POLY_FT4* pPoly) {
    SetPolyFT4(pPoly);
    SetSemiTrans(pPoly, 0);
    SetShadeTex(pPoly, 0);
    setRGB0(pPoly, 128, 128, 128);
}
void MenuPauseCdDrive(void) {
    if (ArchiveGetDebugTable() == 0) {
        ArchiveStopTransfer(0);
        ArchiveCdDataSync(0);
        ArchiveCdSetMode(0);
        ArchiveCdDataSync(0);
        Vsync(3);
        for (;;) {
            Vsync(3);
            if (CdControlB(CdlStop, 0, &g_MenuCdStatus) != 0) {
                break;
            }
        }
    }
}
void MenuReadPcFile(char* pName, char* pBuffer, int len) {
    int fd;

    fd = PCopen(pName, 0, 0);
    PCread(fd, pBuffer, len);
    PCclose(fd);
}
// Triage r111: BUILDS (needed the MenuSectorHeader view + externs at the top of the file; the body's own externs for g_ArchiveTable/Header/DebugTable stay). As stored: 96.65%. Changing three lines - "header = D_801C5294;" instead of a pointer cast, "CdControlB(0x15, 0, pStat)" instead of &g_MenuCdStatus, and "(*pStat & 1)" for the status test - gives 96.81% (756 bytes): the remaining 16 bytes are (a) retail re-materialises &g_MenuCdStatus into s0 before the 0x15 query and reads *pStat through it where this body re-derives the address (a2 for the call, v1 for the read); a second pStat = &g_MenuCdStatus there scores worse (96.65%, address in a2 only), and (b) retail's last block ends with j epilogue + move v0,s0 where this body falls through into the shared move v0,s0, and retail places move s0,zero (result = 0) after the argument setup where this body puts it in the bne delay slot.
/* Best: 96.65% under gcc-2.6.0 (764 bytes vs retail's 772). Recovered shape:
   a 16-byte sector header struct copied from the zero datum D_801C5294 (that
   copy reproduces exactly), ArchiveCdDataSync(0), then either the PC-file path
   when ArchiveGetDebugTable() != 0 (three MenuReadPcFile reads, cdrom*.mdg/fid/
   fnd chosen by `disc == 1`, merged tail by -fcross-jumping) or the CD path:
   CdIntToPos(0,&pos) then three CdControlB(1,0,&g_MenuCdStatus) status loops, the
   0x13/2/0x15 sector-17 header read loop, then ArchiveCdSetMode(0xA0) and the
   0x18/0x28 archive table reads; returns 2 if the header tag word is not
   "_XEN" (0x4E45585F), 3 if header byte 3 is not `disc + 0x30`, else 0.
   What is left: retail keeps &g_MenuCdStatus in a saved register (s0) across the
   first three status loops -- a `u_char* pStat = &g_MenuCdStatus;` local is what
   took it 91.33 -> 96.65 -- and it re-forms the address with a fresh lui/addiu
   for the 0x13 loop and again for the 0x15 call (mine reuses pStat in the 0x13
   loop), so the frame is 8 bytes short and the register/loop-exit shapes differ
   around the 0x13/0x15 retry: retail's is `if ((stat & 1) && (D_801EA8F5 &
   0x40) && ret == 0) return 2; if (ret != 0) break;` inside the 0x13 loop.
typedef struct {
    u32 unk0;
    u32 tag;
    u32 unk8;
    u32 unkC;
} MenuSectorHeader; // Size: 0x10

extern u8 D_801C5294[];
extern char D_801C52A4[];
extern char D_801C52B8[];
extern char D_801C52CC[];
extern char D_801C52E0[];
extern char D_801C52F4[];
extern char D_801C5308[];
extern u_char g_MenuCdStatus;
extern u_char D_801EA8F5;
extern s32 g_ArchiveTable;
extern s32 g_ArchiveHeader;
extern s32 g_ArchiveDebugTable;
extern s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags);

s32 func_801E93A0(s32 disc) {
    MenuSectorHeader header;
    CdlLOC pos;
    u_char* pStat;
    s32 result;
    s32 ret;

    header = *(MenuSectorHeader*)D_801C5294;
    ArchiveCdDataSync(0);
    if (ArchiveGetDebugTable() != 0) {
        result = 0;
        if (disc == 1) {
            MenuReadPcFile(D_801C52A4, (char*)g_ArchiveTable, 0x8000);
            MenuReadPcFile(D_801C52B8, (char*)g_ArchiveHeader, 0x7A);
            MenuReadPcFile(D_801C52CC, (char*)g_ArchiveDebugTable, 0x40000);
        } else {
            MenuReadPcFile(D_801C52E0, (char*)g_ArchiveTable, 0x8000);
            MenuReadPcFile(D_801C52F4, (char*)g_ArchiveHeader, 0x7A);
            MenuReadPcFile(D_801C5308, (char*)g_ArchiveDebugTable, 0x40000);
        }
        return result;
    }
    CdIntToPos(0, &pos);
    pStat = &g_MenuCdStatus;
    do {
        Vsync(3);
        CdControlB(1, 0, pStat);
    } while ((*pStat & 0x10) == 0);
    do {
        Vsync(3);
        CdControlB(1, 0, pStat);
    } while ((*pStat & 0x10) != 0);
    do {
        Vsync(3);
        ret = CdControlB(1, 0, pStat);
    } while (((*pStat & 2) == 0) || (ret == 0));
    for (;;) {
        Vsync(3);
        if (CdControlB(0x13, 0, &g_MenuCdStatus) == 0) {
            continue;
        }
        do {
            Vsync(3);
        } while (CdControlB(2, (u_char*)&pos, &g_MenuCdStatus) == 0);
        ret = CdControlB(0x15, 0, &g_MenuCdStatus);
        if ((g_MenuCdStatus & 1) && (D_801EA8F5 & 0x40) && (ret == 0)) {
            return 2;
        }
        if (ret != 0) {
            break;
        }
    }
    ArchiveCdSetMode(0xA0);
    ArchiveCdDataSync(0);
    Vsync(3);
    Vsync(3);
    ArchiveReadFileFromCdSector(0x17, &header, 0x10, 0, 0);
    ArchiveCdDataSync(0);
    result = 2;
    if (header.tag == 0x4E45585F) {
        result = 3;
        if (((u8*)&header)[3] == disc + 0x30) {
            result = 0;
            ArchiveReadFileFromCdSector(0x18, (void*)g_ArchiveTable, 0x8000, 0, 0);
            ArchiveCdDataSync(0);
            ArchiveReadFileFromCdSector(0x28, (void*)g_ArchiveHeader, 0x7A, 0, 0);
            ArchiveCdDataSync(0);
        }
    }
    return result;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E93A0);
