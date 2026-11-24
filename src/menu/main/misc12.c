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

// Height of the 8x13 font glyphs a menu row's string texture holds
#define MENU_FONT_LETTER_HEIGHT 13

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

// Digits func_801E6CFC draws of each of the two slot stats it formats:
// MenuParseNumberToDigits leaves the value in the tail of g_Menu->digits, any
// leading digit marked MENU_DIGIT_NONE.
#define MENU_SLOT_STAT_DIGIT_COUNT 3
// Where those digits start in g_Menu->digits.
#define MENU_SLOT_STAT_DIGIT_START (MENU_MAX_DIGITS - MENU_SLOT_STAT_DIGIT_COUNT)

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
extern s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags);

void MenuClearGlyphTexture(void);
void MenuBuildGlyphTexture(s32 arg0);
void MenuDrawCharacterSlots(s32 arg0);
void MenuSetStringPrimitives(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);
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
void MenuInitializeIconQuads(void);

extern s32 g_MenuSubMenuChoiceIds[];
extern s32 g_MenuSubMenuChoiceTextIds[];
extern MenuTexturePos g_MenuNameTextureX[];
extern MenuTexturePos g_MenuNameTextureY[];
extern s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
extern MenuTexturePos g_MenuSlotNameTextureX[];
extern MenuTexturePos g_MenuSlotNameTextureY[];
extern void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);
extern s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
extern void MenuMovePointerCursor(s32 arg0, s32 arg1);

extern s32 D_801EA02C;
extern s32 D_801EA030;
extern s32 D_801EA034;
extern s32 D_801EA038;
extern s32 D_801EA03C;
extern s32 D_801EA040;
extern s32 D_801EA044;
extern s32 D_801EA048;

// Round 450 (this tree): re-measured 88.75% / 608 bytes / 152 insns, which IS retail's size. Preset screen: gcc-2.7.2-psx 85.20, gcc-2.7.2-cdk will not compile this unit, so XenoMenu260 (2.6.0) is right and the unit needs no split. Residual unchanged and still the same ten mirrored operands: (a) all four counter addresses emit "addu a0,s1,a0" where retail has "addu a0,a0,s1"; (b) the prim base pair "addiu a2,s1,2712 / addu ..." is hoisted into the beq delay slot where retail forms it after the two stack-argument stores; (c) the base/slot sum takes a0 where retail has a2, so ours needs an extra t1. Nothing new tried this round beyond the preset screen: the lever list's remaining candidates (embedded-assignment temps 82.70/612, the double-negated offset 80.13/624, a slot variable 70.93-76.25) are all in this note already and all lose the register plan.
/*
void func_801E6CFC(u8 index, u8* pEntry) {
    u8* pPrims;
    s32 primOffset;
    s32 i = 0;
    s32 n;

    MenuParseNumberToDigits(*(u16*)(pEntry + index * 2 + 4));
    ((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130A] = 0;
    for (; i < MENU_SLOT_STAT_DIGIT_COUNT; i++) {
        if (g_Menu->digits[MENU_SLOT_STAT_DIGIT_START + i] != MENU_DIGIT_NONE) {
            primOffset = index * MENU_SLOT_PRIM_STRIDE + 0xA98;
            pPrims = (u8*)(primOffset + (u32)g_Menu->pPrimBuffer);
            ((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130A] += func_8002675C(
                g_Menu->resources, g_Menu->digits[MENU_SLOT_STAT_DIGIT_START + i],
                (POLY_FT4*)(pPrims + (((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130A] * 0x50 + 0x500)),
                g_Menu->renderContext, D_801EA02C + index * 0x50 + i * 8, D_801EA030, 0x1000);
        }
    }
    n = 0;
    i = 0;
    MenuParseNumberToDigits(*(u16*)(pEntry + index * 2 + 0xA));
    ((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130B] = 0;
    for (; i < MENU_SLOT_STAT_DIGIT_COUNT; ) {
        u8 count = g_Menu->digits[MENU_SLOT_STAT_DIGIT_START + i++];

        if (count != MENU_DIGIT_NONE) {
            primOffset = index * MENU_SLOT_PRIM_STRIDE + 0xA98;
            pPrims = (u8*)(primOffset + (u32)g_Menu->pPrimBuffer);
            ((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130B] += func_8002675C(
                g_Menu->resources, count,
                (POLY_FT4*)(pPrims + (((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130B] * 0x50 + 0x5F0)),
                g_Menu->renderContext, D_801EA034 + index * 0x50 + n * 8, D_801EA038, 0x1000);
            n++;
        }
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6CFC);


INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6F5C);
/* Best: 88.60% at 600 bytes, which IS retail's size (0x21F5C..0x221B4).
   The same body as func_801E6CFC above, with the MP half's own constants:
   pEntry[index+0x10] / pEntry[index+0x13] are lbu, counters 0x130C/0x130D, tables
   D_801EA03C/40/44/48, poly offsets 0x6E0/0x780, digits[7+i]. The digit load must be
   written (pEntry + index)[0x10] (round 313): that is retail's base-first
   "addu s5,v1"; pEntry[index + 0x10] gives the mirrored addu and 88.47%.
   This body's register plan IS retail's - s0 = i (move s0,zero in the prologue),
   s1 = the slot offset (gcc CSEs all four index * 0x87C into it), s2 = the masked
   index (move s2,v1 in the jal delay slot), s3 = n, s4 = the raw index param - and so
   is its instruction count. What is left is FOUR mirrored "addu"s plus where the
   prim-base pair lands:
   - every counter address: retail has base-first "addu a0,a2,s1" (pre-call) and
     "addu a0,a0,s1" (after it); this body emits "addu a0,s1,a0" at all four sites.
     The briefs' answer for base-first - keep the offset in a source VARIABLE - does
     work (slot = index * MENU_SLOT_PRIM_STRIDE, then all four addus come out
     base-first), but it costs the register plan: 73.43% / 588 bytes, with the masked
     index taking s0, i deferring into the jal delay slot and the prim offset taking
     a saved register (s3/s4). Round 321 re-derived that from scratch: the var
     variants score 73.43, 76.25, 67.48 and 71.63 with several prim spellings.
   - the prim base: retail emits "addiu v0,s1,2712 / addu v0,v0,a2" in the MIDDLE of
     the block (after the x and 0x1000 stack stores); this body puts the pair at the
     head of the block, so reorg steals the addiu into the beq delay slot (retail's
     slot holds the start of the x chain, "sll v0,s2,0x2"). Any primOffset/pPrims
     local puts it there; inlining the whole prim expression instead (six spellings,
     including the (u32) cast, the u8 deref form and the "slot + 0xA98" grouping)
     either keeps the same order or loses 15-20 points. Reading the counter into a u8
     local before the prim statements is byte-identical to this body (gcc CSEs it),
     so the evaluation order is the compiler's own, not the statement order's.
   Two 4-minute permuter runs from this body and from the round-321 variant found only
   new_var temporaries and one nonsense edit that reuses primOffset as a constant
   (base 1150 -> 1050, not a match). This looks like the same class as func_801E6CFC's
   tail: the source shape is right and the addu operand order is decided by the
   allocator, not by the C.
   Round 373 re-measured: 88.60% at 600 bytes, with the SAME ten differing
   instructions as the twin (five per half: "addu v1,s1,v1" for retail's
   "addu v1,v1,s1"; the base load in a0 for retail's a2, so "addu t1,a2,a0" +
   "addu a2,t1,a2" for retail's "addu a0,a2,s1" + "addu a2,v0,a2"; and
   "addu a0,s1,a0" for "addu a0,a0,s1"). MENU_MP_DIGIT_COUNT is 2 (header) and
   MENU_MAX_DIGITS - MENU_MP_DIGIT_COUNT gives retail's digits[7 + i] - verified
   against the .s (lbu 0x323, slti 2). An 'inline the whole prim expression' body
   (no primOffset/pPrims locals) is 88.44% at 584, 16 bytes short, so the locals
   stay.
void func_801E6F5C(u8 index, u8* pEntry) {
    u8* pPrims;
    s32 primOffset;
    s32 i = 0;
    s32 n;

    MenuParseNumberToDigits((pEntry + index)[0x10]);
    ((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130C] = 0;
    for (; i < MENU_MP_DIGIT_COUNT; i++) {
        if (g_Menu->digits[MENU_MAX_DIGITS - MENU_MP_DIGIT_COUNT + i] != MENU_DIGIT_NONE) {
            primOffset = index * MENU_SLOT_PRIM_STRIDE + 0xA98;
            pPrims = (u8*)(primOffset + (u32)g_Menu->pPrimBuffer);
            ((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130C] += func_8002675C(
                g_Menu->resources, g_Menu->digits[MENU_MAX_DIGITS - MENU_MP_DIGIT_COUNT + i],
                (POLY_FT4*)(pPrims + (((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130C] * 0x50 + 0x6E0)),
                g_Menu->renderContext, index * 0x50 + D_801EA03C + i * 8, D_801EA040, 0x1000);
        }
    }
    n = 0;
    i = 0;
    MenuParseNumberToDigits((pEntry + index)[0x13]);
    ((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130D] = 0;
    for (; i < MENU_MP_DIGIT_COUNT; ) {
        u8 digit = g_Menu->digits[MENU_MAX_DIGITS - MENU_MP_DIGIT_COUNT + i++];

        if (digit != MENU_DIGIT_NONE) {
            primOffset = index * MENU_SLOT_PRIM_STRIDE + 0xA98;
            pPrims = (u8*)(primOffset + (u32)g_Menu->pPrimBuffer);
            ((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130D] += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(pPrims + (((u8*)g_Menu->pPrimBuffer + index * MENU_SLOT_PRIM_STRIDE)[0x130D] * 0x50 + 0x780)),
                g_Menu->renderContext, index * 0x50 + D_801EA044 + n * 8, D_801EA048, 0x1000);
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
    pTable = (u8*)g_Menu->menuUnk2 + arg2 * MENU_IMAGE_CHUNK_SIZE + 0xC94;
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
// Initializes the 16 icon quads of the shared prim buffer at 0x277C (12x16 each,
// one prim per render context) and their texture page/clut entries; drawn by
// MenuRenderString's 0x10-prim call in misc4.
#define MENU_ICON_COUNT 16
#define MENU_ICON_PRIM_OFFSET 0x277C
extern u16 g_MenuIconQuadX;
extern u16 g_MenuIconQuadY;
void MenuInitializeIconQuads(void) {
    s32 primIndex;
    s32 offset;
    s32 i;

    for (i = 0; i < MENU_ICON_COUNT; i++) {
        offset = (i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET;
        MenuInitializePolyFt4((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer));
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->x0 = g_MenuIconQuadX + i * 0xC;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->y0 = g_MenuIconQuadY;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->x1 = g_MenuIconQuadX + i * 0xC + 0xC;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->y1 = g_MenuIconQuadY;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->x2 = g_MenuIconQuadX + i * 0xC;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->y2 = g_MenuIconQuadY + 0x10;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->x3 = g_MenuIconQuadX + i * 0xC + 0xC;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->y3 = g_MenuIconQuadY + 0x10;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->u0 = i * 0x10;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->v0 = 0xF0;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->u1 = i * 0x10 + 0xC;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->v1 = 0xF0;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->u2 = i * 0x10;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->v2 = 0xFF;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->u3 = i * 0x10 + 0xC;
        ((POLY_FT4*)((i * 2 + g_Menu->renderContext) * 0x28 + MENU_ICON_PRIM_OFFSET + (u32)g_Menu->pPrimBuffer))->v3 = 0xFF;
        // The index needs a temp: a plain address expression folds to index-first addu.
        ((POLY_FT4*)((u8*)g_Menu->pPrimBuffer + (primIndex = (i * 2 + g_Menu->renderContext) * 0x28) + MENU_ICON_PRIM_OFFSET))->tpage =
            GetTPage(0, 0, 0x140, 0x80);
        ((POLY_FT4*)((u8*)g_Menu->pPrimBuffer + (primIndex = (i * 2 + g_Menu->renderContext) * 0x28) + MENU_ICON_PRIM_OFFSET))->clut =
            GetClut(0, 0x1C0);
    }
}
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
    MenuInitializeIconQuads();
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
static inline void MenuSetSystemPalette(void* pPrim, u8 flag) {
    POLY_FT4* pPoly = pPrim;
    pPoly->clut = flag ? g_SystemPalette2 : g_SystemPalette1;
}

void MenuSetStringPrimitives(MenuString* pStringList, s32 index, s32 yOffset, s32 flags) {
    u8 color;
    s32 i;
    POLY_FT4* pPoly;
    // Load-bearing copy: reading `flags` directly costs this function its callee-saved
    // assignment (the loop counter and the attribute word trade registers).
    u32 attr = flags;

    for (i = 0; i < 2; i++) {
        pPoly = &pStringList->polys[i];
        color = 0;

        MenuInitializePolyFt4(pPoly);

        if (!(attr & 0xFF)) {
            pStringList->palette = (index & 1);
            setTPage(pPoly, 0, 0, 0x140, 0);

            #define texCoordV  (((index + yOffset) / 4) * MENU_FONT_LETTER_HEIGHT)
            #define texCoordU  (((index / 2) & 1) << 7)
            setUV4(
                pPoly,
                texCoordU,                      texCoordV,
                texCoordU + pStringList->width, texCoordV,
                texCoordU,                      texCoordV + MENU_FONT_LETTER_HEIGHT,
                texCoordU + pStringList->width, texCoordV + MENU_FONT_LETTER_HEIGHT
            );
            #undef texCoordV
            #undef texCoordU
        } else {
            if (!(attr & 0x80)) {
                color = 0x20;
                SetSemiTrans(pPoly, 1);
                setRGB0(pPoly, color, color, color);
            }

            pStringList->palette = (attr & 0x7F) + 0xFF;
            setTPage(pPoly, 0, 0, 0x180, 0x80);
            pPoly->tpage = color | pPoly->tpage;

            #define texCoordU ((index & 1) * 0x60)
            #define texCoordV (((index / 2) * MENU_FONT_LETTER_HEIGHT) + yOffset)
            setUV4(
                pPoly,
                texCoordU,                      texCoordV,
                texCoordU + pStringList->width, texCoordV,
                texCoordU,                      texCoordV + MENU_FONT_LETTER_HEIGHT,
                texCoordU + pStringList->width, texCoordV + MENU_FONT_LETTER_HEIGHT
            );
            #undef texCoordV
            #undef texCoordU
        }

        MenuSetSystemPalette(pPoly, pStringList->palette);
    }
    pStringList->hasVertices = 0;
}
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
        MenuSetStringPrimitives(&pStringList[index], index, yOffset, 0);
        MenuSetStringPrimitives(&pStringList[index + 1], index + 1, yOffset, 0);
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
        (pRows[row].polys + g_Menu->renderContext)->x0 = g_MenuCursorXPositions[col + row] + (pXOffsets[row] + 0x16);
        (pRows[row].polys + g_Menu->renderContext)->y0 = g_MenuCursorYPositions[col + row] - 0x22;
        (pRows[row].polys + g_Menu->renderContext)->x1 = (u16)(g_MenuCursorXPositions[col + row] + 0x16) + pXOffsets[row] + pRows[row].width;
        (pRows[row].polys + g_Menu->renderContext)->y1 = g_MenuCursorYPositions[col + row] - 0x22;
        (pRows[row].polys + g_Menu->renderContext)->x2 = g_MenuCursorXPositions[col + row] + (pXOffsets[row] + 0x16);
        (pRows[row].polys + g_Menu->renderContext)->y2 = g_MenuCursorYPositions[col + row] - 0x15;
        (pRows[row].polys + g_Menu->renderContext)->x3 = (u16)(g_MenuCursorXPositions[col + row] + 0x16) + pXOffsets[row] + pRows[row].width;
        (pRows[row].polys + g_Menu->renderContext)->y3 = g_MenuCursorYPositions[col + row] - 0x15;
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
        (pRows[row].polys + g_Menu->renderContext)->x0 = 0x4C;
        (pRows[row].polys + g_Menu->renderContext)->y0 = 0x12;
        (pRows[row].polys + g_Menu->renderContext)->x1 = pRows->width + 0x4C;
        (pRows[row].polys + g_Menu->renderContext)->y1 = 0x12;
        (pRows[row].polys + g_Menu->renderContext)->x2 = 0x4C;
        (pRows[row].polys + g_Menu->renderContext)->y2 = 0x1F;
        (pRows[row].polys + g_Menu->renderContext)->x3 = pRows->width + 0x4C;
        (pRows[row].polys + g_Menu->renderContext)->y3 = 0x1F;
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

    g_Menu->pSelectionMenu->isDisabled = 0;
    g_Menu->pSelectionMenu->isDisabledRendered = 0;
    g_Menu->pManager->shouldRenderSelectionMenu = 1;
    for (i = 1; i <= count; i++) {
        if (i != count) {
            g_Menu->pSelectionMenu->numCursors = 0;
            for (j = 0; j < i; j++) {
                g_Menu->pSelectionMenu->numCursors = func_8002675C(g_Menu->resources, pEntries[j * 2],
                    &g_Menu->pSelectionMenu->polysCursors[g_Menu->pSelectionMenu->numCursors * 2],
                    g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numCursors;
            }
            g_Menu->pSelectionMenu->cursorsRenderCtx = g_Menu->renderContext;
        }
        g_Menu->pSelectionMenu->numTexts = 0;
        if (i != 1) {
            for (j = 0; j < i - 1; j++) {
                g_Menu->pSelectionMenu->numTexts = func_8002675C(g_Menu->resources, pEntries[j * 2 + 1],
                    &g_Menu->pSelectionMenu->polysTexts[g_Menu->pSelectionMenu->numTexts * 2],
                    g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numTexts;
            }
            g_Menu->pSelectionMenu->textsRenderCtx = g_Menu->renderContext;
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
   Round 285: a 4-minute permuter run from the one-local body found nothing
   (PERMUTER_NONE base=35 best=35, 1784 iterations), and moving the single
   `pText = g_MenuSubMenuChoiceIds + 1;` init inside the outer loop (so loop.c hoists it)
   changes nothing: 99.79%, 688 bytes, same two slots. The hoisted loop-1 base invariant
   is still materialised after that statement, so the +4 form anchors either way.
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E86C8);
/* Best: 99.79% (688 bytes = retail's size, one local). Round 415 (twin lane):
   the twin ShopMenuInitializeSubmenu - same source shape, matched 100% - proves the
   shape is right, and its own .s has retail's exact prologue (andi s3 = masked arg,
   la s4 = table, addiu s5,s4,4 hoisted into the two lw v0,0x354 delay slots). Porting
   the twin body verbatim (single symbol, `idx + 1` inline, `+=` accumulate, allValid
   after the three stores) measures 91.84% / 692 bytes / 173 insns here: gcc does NOT
   hoist the (table+4) address out of the OUTER loop in this unit and re-materialises a
   second full lui/addiu in loop 2's preheader, with the masked arg taking s4 and the
   base s5. Two address locals (pIds = g_MenuSubMenuChoiceIds; pText = pIds + 1;) give
   retail's exact pair - la s4 = table then addiu s5,s4,4 - but at 88.09% because gcc
   emits them at the head of the entry block, before the remaining saves, instead of in
   the two load-delay slots. So the residual is a hoisting/allocno-ORDER difference,
   not a source-shape one: the twin's TU differs only by two extra entry-block stores
   (isDisabled/isDisabledRendered) that my stub's retail body does not have, and that is
   enough to rotate masked/base/base+4 from (s3,s4,s5) here into (s4,s5,s3). One-local
   body (below): anchor direction wrong (la on table+4, base derived as -4), 99.79%.
void func_801E86C8(u_char selected) {
    s32 i;
    s32 n;
    s32 allValid;
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
                    &g_Menu->pSubmenuPrims->polys0[g_Menu->pSubmenuPrims->polys0PrimCount * 2],
                    g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSubmenuPrims->polys0PrimCount;
                g_Menu->subMenuNumChoices++;
            } else {
                allValid = 0;
            }
        }
        g_Menu->pSubmenuPrims->polys0RenderCtx = g_Menu->renderContext;
        if (allValid) {
            for (i = 0; i < 2; i++) {
                MenuRenderFrame();
            }
        }
        g_Menu->pSubmenuPrims->polys500PrimCount = 0;
        for (i = 0; i < n; i++) {
            if (g_MenuSubMenuChoiceIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2] != 0xFFFF) {
                g_Menu->pSubmenuPrims->polys500PrimCount = func_8002675C(g_Menu->resources,
                    pText[(selected + g_Menu->mainMenuChoice) * 8 + i * 2],
                    &g_Menu->pSubmenuPrims->polys500[g_Menu->pSubmenuPrims->polys500PrimCount * 2],
                    g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSubmenuPrims->polys500PrimCount;
            }
        }
        g_Menu->pSubmenuPrims->polys500RenderCtx = g_Menu->renderContext;
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

    g_Menu->pSelectionMenu->numCursors = 0;
    g_Menu->pSelectionMenu->numTexts = 0;
    for (i = 0; i < count; i++) {
        if (i == selected) {
            index = pEntries[i * 2] + MENU_SELECTED_RESOURCE_OFFSET;
        } else {
            index = pEntries[i * 2];
        }
        g_Menu->pSelectionMenu->numCursors = func_8002675C(g_Menu->resources, index,
            &g_Menu->pSelectionMenu->polysCursors[g_Menu->pSelectionMenu->numCursors * 2],
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numCursors;
        g_Menu->pSelectionMenu->numTexts = func_8002675C(g_Menu->resources, pEntries[i * 2 + 1],
            &g_Menu->pSelectionMenu->polysTexts[g_Menu->pSelectionMenu->numTexts * 2],
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numTexts;
    }
    g_Menu->pSelectionMenu->cursorsRenderCtx = g_Menu->renderContext;
    g_Menu->pSelectionMenu->textsRenderCtx = g_Menu->renderContext;
    MenuMovePointerCursor(selected, 1);
    g_Menu->pManager->shouldRenderPointerCursor = 1;
}
void MenuSetSubMenuChoicePrims(u_char selected) {
    s32 i;
    u32 index;

    g_Menu->pSubmenuPrims->polys0PrimCount = 0;
    g_Menu->pSubmenuPrims->polys500PrimCount = 0;
    for (i = 0; i < g_Menu->subMenuNumChoices; i++) {
        if (i == g_Menu->subMenuChoice) {
            index = g_MenuSubMenuChoiceIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2] + MENU_SELECTED_RESOURCE_OFFSET;
        } else {
            index = g_MenuSubMenuChoiceIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2];
        }
        g_Menu->pSubmenuPrims->polys0PrimCount = func_8002675C(g_Menu->resources, index,
            &g_Menu->pSubmenuPrims->polys0[g_Menu->pSubmenuPrims->polys0PrimCount * 2],
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSubmenuPrims->polys0PrimCount;
        g_Menu->pSubmenuPrims->polys500PrimCount = func_8002675C(g_Menu->resources,
            g_MenuSubMenuChoiceTextIds[(selected + g_Menu->mainMenuChoice) * 8 + i * 2],
            &g_Menu->pSubmenuPrims->polys500[g_Menu->pSubmenuPrims->polys500PrimCount * 2],
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSubmenuPrims->polys500PrimCount;
    }
    g_Menu->pSubmenuPrims->polys0RenderCtx = g_Menu->renderContext;
    g_Menu->pSubmenuPrims->polys500RenderCtx = g_Menu->renderContext;
    MenuMovePointerCursor(g_Menu->subMenuChoice + 7, 1);
    g_Menu->pManager->shouldRenderPointerCursor = 1;
}
void MenuLoadCharacterNameTexture(s32 arg0, s32 arg1) {
    RECT rect;
    u8* pBuf;
    s32 nameIndex;

    pBuf = HeapAlloc(MENU_STRING_IMAGE_BUFFER_SIZE, 0);
    bzero(pBuf, MENU_STRING_IMAGE_BUFFER_SIZE);
    if ((u_char)arg0 != CHARACTER_ID_NONE) {
        nameIndex = (u_char)arg0 >> 1;
        SystemRenderStringEntry((u8*)&g_GameState.strings[nameIndex * 2], pBuf, 0x24, 0);
        SystemRenderStringEntry((u8*)&g_GameState.strings[nameIndex * 2 + 1], pBuf, 0x24, 1);
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
// Best: 99.61% (612 bytes = retail's exact size, 153 insns), ONE instruction left:
// the trailing call passes mode as "move a1,s1" where retail has
// "andi a1,s1,0xFF". Everything else - registers, delay slots, saves, frame - is
// retail's. Mechanism (proved by a 15-variant cc1_probe; lever
// e-narrow-conversion-folds-in-a-single-pred-block in tools/rounds/levers.md): the
// mask survives at all six loop-head calls and is folded at the trailing call
// because combine holds the local's constant value (2 or 3) there. Every
// straight-line spelling folds it (u_char local, if/else instead of the ternary,
// an (u_char) cast, "mode & 0xFF", a u_char or s32 copy right before the call,
// for (;;) with one call); only a real loop body keeps the andi, and that costs a
// branch retail does not have. Four permuter runs from this exact body (r318,
// r399, r414, this lane) are all PERMUTER_NONE base=200 best=200; gcc-2.7.2 and
// 2.7.2-cdk score 99.15. The one spelling that restored the andi
// (i = arg1 ? 3 : 2; mode = i;) needs a second register and a copy, 616 bytes, and
// retail has no spare saved register (s0=i, s1=mode, s2=arg0, s3=arg0*4).
// Call sites are the MenuWindow poly arrays at 0x0 (BorderCorners),
// 0x140/0x1E0/0x280/0x320 (BorderTop/Bottom/Left/Right), 0x3C0 (ScrollBarEmpty),
// 0x410 (ScrollBarEnds), indexed [i * 2 + renderContext].
/*
void func_801E8F60(u8 arg0, u8 arg1) {
    s32 mode;
    s32 i;

    mode = arg1 ? 3 : 2;
    for (i = 0; i < 4; i++) {
        MenuSetPrimShadingMode(
            &g_Menu->windows[arg0]->polysWindowBorderCorners[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
    }
    for (i = 0; i < 4; i++) {
        MenuSetPrimShadingMode(
            &g_Menu->windows[arg0]->polysWindowBorderTop[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
    }
    for (i = 0; i < 4; i++) {
        MenuSetPrimShadingMode(
            &g_Menu->windows[arg0]->polysWindowBorderBottom[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
    }
    for (i = 0; i < 4; i++) {
        MenuSetPrimShadingMode(
            &g_Menu->windows[arg0]->polysWindowBorderLeft[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
    }
    for (i = 0; i < 4; i++) {
        MenuSetPrimShadingMode(
            &g_Menu->windows[arg0]->polysWindowBorderRight[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
    }
    for (i = 0; i < 2; i++) {
        MenuSetPrimShadingMode(
            &g_Menu->windows[arg0]->polysScrollBarEnds[i * 2 + g_Menu->windows[arg0]->renderContext], mode);
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
// Best: 98.39% (760 bytes, 190 insns; retail 772 bytes / 193 insns). Two named residuals:
// (1) the 0x15 query: retail re-materialises &g_MenuCdStatus into s0 there ("lui s0 / addiu s0 /
// move a2,s0 in the jal slot / lbu v1,0(s0)"). This body reuses pStat (already s0 from the first
// three status loops), so the lui/addiu pair is missing (-2 insns). A SECOND pointer variable
// assigned at that site (pStatus = &g_MenuCdStatus) makes gcc materialise inline into a2 and
// re-derive the lbu address into v1: 98.23% / 768 bytes / 192 insns - one insn short instead of
// three, register split still different. Trying both spellings is where the next agent starts.
// (2) the tail needs a DUPLICATE return copy: retail success ends "j epilogue; addu v0,s0" and the
// merged failure return "addu v0,s0" follows it, so retail has both "move s0,zero" (result = 0)
// and the copy. ONE "return result;" gives only the fallthrough copy (96.81% / 756). An explicit
// "return result;" in the success block gives the j, but then gcc const-propagates the 0: it drops
// "move s0,zero" and writes "move v0,zero" (this body, +8 bytes). A return per path (three
// "return result;" statements) makes gcc const-propagate all three into li v0 (96.70% / 760).
// Nothing tried keeps the s0 = 0 assignment AND the duplicate copy.
// Round 450 (this tree): re-measured 98.39% / 760 bytes / 190 insns (retail 772 / 193). The two residuals measured separately: (a) writing the 0x15 query's status pointer as the literal &g_MenuCdStatus gives 98.23% / 768 / 192: gcc rematerialises the address into a2 for the argument AND into v1 for the following lbu instead of keeping one pseudo in s0, and a fresh "u_char* pStatus = &g_MenuCdStatus;" measures identically, so with both uses fresh the remat is unavoidable. (b) the success path's "result = 0; ... return result;" is const-folded to "move v0,zero" and the "addu s0,zero,zero" store disappears (one instruction short); moving the assignment after the two calls is byte-identical (760/190), so its position is not the lever. Why retail keeps that copy: retail's OTHER two returns ("addu v0,s0") sit at merge labels, where cse's register-value table is reset, while the success block is straight-line - the fold is decided inside the block, not by the calls. Preset screen: gcc-2.7.2-psx measures the same 98.39, 2.6.0 -O3 is 75.57, and -G0 / 2.7.2-cdk do not compile, so XenoMenu260 is right. MATCH needs BOTH residuals (193 insns), so repairing one cannot close it.
/*
s32 func_801E93A0(s32 disc) {
    MenuSectorHeader header;
    CdlLOC pos;
    u_char* pStat;
    s32 result;
    s32 ret;

    header = D_801C5294;
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
        ret = CdControlB(0x15, 0, pStat);
        if ((*pStat & 1) && (D_801EA8F5 & 0x40) && (ret == 0)) {
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
            return result;
        }
    }
    return result;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E93A0);


