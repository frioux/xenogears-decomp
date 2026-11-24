#include "common.h"
#include "psyq/libgpu.h"
#include "main/game.h"
#include "system/controller.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/font.h"

// SystemString.flags bits
#define SYSTEM_STRING_FLAG_PERSISTENT 0x2    // the only flag a reset keeps
#define SYSTEM_STRING_FLAG_STOPPED 0x8       // rendering stopped, stopCode says why
#define SYSTEM_STRING_FLAG_DATA_OVERRIDE 0x80 // pCur is an override of pPrev
#define SYSTEM_STRING_FLAG_WAITING 0x200     // a wait opcode is running
#define SYSTEM_STRING_FLAG_BUFFER_POP 0x4    // a buffer pop has happened; skip the next
#define SYSTEM_STRING_FLAG_UPLOAD_HOLD 0x40  // do not upload the rows to VRAM yet
// The row count passed maxChars, so the oldest row of the window moves up
#define SYSTEM_STRING_FLAG_ROW_OVERFLOW 0x1

// The string index a game state string id maps to when it names no string
#define SYSTEM_STRING_INDEX_NONE 0xFF

// Distance, in VRAM units, between two rendered rows of a string
#define SYSTEM_STRING_LINE_HEIGHT 14

// The clear tile a string is erased with overhangs its text box: this far left
// and up, and this much wider and taller
#define SYSTEM_STRING_CLEAR_MARGIN_X 7
#define SYSTEM_STRING_CLEAR_MARGIN_Y 5
#define SYSTEM_STRING_CLEAR_PAD_WIDTH 0xD
#define SYSTEM_STRING_CLEAR_PAD_HEIGHT 0xA

// lineOffset value that means no offset
#define SYSTEM_STRING_LINE_OFFSET_NONE 0xFF

// Primitive code of the free-size tile a rendered string's text is erased with
#define SYSTEM_STRING_TILE_CODE 0x60

// One entry of the glyph code table: the two bytes that identify a glyph
typedef struct {
    /* 0x0 */ u8 firstByte;
    /* 0x1 */ u8 secondByte;
} SystemGlyphEntry; // Size: 0x2

// Glyph code table (system data bank 0x1B) and the codes it holds
#define SYSTEM_GLYPH_CODE_COUNT 324
#define SYSTEM_GLYPH_CODE_END 0xFFFF
#define SYSTEM_GLYPH_CODE_NOT_FOUND 0x8000

// Height, in VRAM units, of the glyph strip one rendered row of text occupies;
// two character slots share a strip
#define SYSTEM_GLYPH_STRIP_HEIGHT 13

// Width, in VRAM units, of one texture page: a row's sprites are split into a
// first half up to this and a second half past it
#define SYSTEM_TEXTURE_PAGE_WIDTH 0x100

// Width, in VRAM units, one glyph advances the line
#define SYSTEM_GLYPH_WIDTH_NARROW 2
#define SYSTEM_GLYPH_WIDTH_WIDE 3

// The digits of a number glyph set, then the two codes after them; a set is
// this many codes apart from the next in the glyph table
#define SYSTEM_NUMBER_GLYPH_DIGIT_COUNT 10
#define SYSTEM_NUMBER_GLYPH_MINUS 10
#define SYSTEM_NUMBER_GLYPH_PLUS 11
#define SYSTEM_NUMBER_GLYPH_SET_STRIDE 16

// Slots from the terminator that ends the digits back to the sign slot
#define SYSTEM_NUMBER_GLYPH_SLOT_COUNT (SYSTEM_NUMBER_GLYPH_DIGIT_COUNT + 1)

// Place value of the most significant digit SystemFormatNumberGlyphs takes
// first: the divisor its digit loop starts at
#define SYSTEM_NUMBER_GLYPH_FIRST_DIVISOR 1000000000

// Palette in g_SystemPaletteData: this many colors in one row, in two CLUTs
#define SYSTEM_PALETTE_COLOR_COUNT 32
#define SYSTEM_PALETTE_CLUT_COLOR_COUNT 16

extern void* g_SystemDataFile;
extern void* g_SystemFontFile;
extern SystemDataEntries* g_SystemDataEntries;

extern u8 g_SystemPaletteData[];
extern s16 g_SystemPalette1;
extern s16 g_SystemPalette2;

// Header at the start of the font file. Glyph bitmaps are 0x16 bytes each
// (0x16 * code is how func_80034FFC addresses one) and start at 0x2's offset.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 glyphDataOffset;
    /* 0x4 */ u16 leadByte;              // first byte of a two-byte code
    /* 0x6 */ u16 doubleByteGlyphOffset; // bias added to a two-byte glyph's address
    /* 0x8 */ u16 narrowGlyphCount;      // this many codes from 0xC draw 2px wide
    /* 0xA */ u16 narrowTrailByteCount;  // trail bytes below this draw 2px wide
    /* 0xC */ u16 singleByteCodeBase;
} SystemFontFileHeader; // Size: 0xE

// Font file metrics, filled in by SystemInitializeFont
extern void* g_SystemFontGlyphData;
extern int g_SystemFontLeadByte;
extern int g_SystemFontDoubleByteGlyphOffset;
extern int g_SystemFontNarrowGlyphCount;
extern int g_SystemFontNarrowTrailByteCount;
extern int g_SystemFontSingleByteCodeBase;

// Scratch buffer the number/glyph converters write to
extern u8 g_SystemGlyphDecodeBuffer[];

// Digit buffer: the digits, then the terminator slot. The sign is written in
// front of the digits, at g_SystemNumberGlyphBuffer[-1].
extern u16 g_SystemNumberGlyphBuffer[];


// One row of a rendered string. SystemStringInit sets the sprites up as free-size
// textured rects (code 0x64) and the rect up for LoadImage, which uploads the
// row's glyphs to it; func_80034888 draws one sprite per render context.
typedef struct {
    /* 0x00 */ SPRT sprt[2][2];  // [render context][first/second half: split at 0x100 VRAM units]
    /* 0x50 */ RECT imageRect;   // VRAM rect the row's glyphs are uploaded to
    /* 0x58 */ s16 rowWidth;  // right edge of the row in units of 4 VRAM units
    /* 0x5A */ u8 paletteIndex;  // picks g_SystemPalette1 or g_SystemPalette2
    /* 0x5B */ u8 unk5B;
    /* 0x5C */ u8 textureV;      // VRAM y of this row's glyphs
    /* 0x5D */ u8 unk5D;
    /* 0x5E */ s16 clut;         // g_SystemPalette1 or g_SystemPalette2
} SystemStringChar; // Size: 0x60

// Free-size tile (code 0x60) whose r,g,b,code word the game writes in one
// store; spelling the colour as setRGB0's three byte stores plus setcode gives
// four sb where the original has one sw (measured: 12 bytes over).
typedef struct {
    /* 0x0 */ u_long tag;      // addr:24, len:8
    /* 0x4 */ u_long colorCode; // r, g, b, primitive code
    /* 0x8 */ s16 x0;
    /* 0xA */ s16 y0;
    /* 0xC */ s16 w;
    /* 0xE */ s16 h;
} SystemStringTile; // Size: 0x10

typedef struct SystemStringNode {
    /* 0x0 */ struct SystemStringNode* pNext;
    /* 0x4 */ void* pData;
} SystemStringNode; // Size: 0x8

// A rendered string: glyph buffer, palette and the stack of data buffers that
// the renderer walks.
typedef struct {
    /* 0x00 */ s16 lineWidth;
    /* 0x02 */ s16 charIndex;
    /* 0x04 */ s16 x;
    /* 0x06 */ s16 y;
    /* 0x08 */ s16 spriteWidth;  // (maxWidth | 1) * 4, the width of a row's sprites
    /* 0x0A */ s16 maxWidth;     // lineWidth wraps here
    /* 0x0C */ s16 maxChars;
    /* 0x0E */ s16 textureY;     // VRAM y the rows' glyph strips start at
    /* 0x10 */ u16 flags;
    /* 0x12 */ s16 imageWidth;   // LoadImage RECT width of a row's glyph strip
    /* 0x14 */ s16 lineHeight;
    /* 0x16 */ s16 scrollRow;   // first row shown; charIndex - scrollRow is the row
    /* 0x18 */ s16 rowCount;    // rows started so far, taken mod maxChars + 1 for the palette
    /* 0x1A */ undefined8 unk1A[2];
    /* 0x1C */ void* pCur;
    /* 0x20 */ void* pPrev;
    /* 0x24 */ undefined8 unk24[4];
    /* 0x28 */ SystemStringChar* pChars;
    /* 0x2C */ void* pCharBuffer;
    /* 0x30 */ DR_MODE drawMode[2];
    /* 0x48 */ SystemStringTile clearTile[2];
    /* 0x68 */ u8 unk68;
    /* 0x69 */ u8 unk69;
    /* 0x6A */ u8 unk6A;
    /* 0x6B */ u8 stopCode;     // why rendering stopped: 1 end of text, 2/3 a stop code
    /* 0x6C */ u8 waiting;      // a wait opcode ran; the next draw consumes it and stops advancing
    /* 0x6D */ undefined8 unk6D;
    /* 0x6E */ u8 lineOffset;
    /* 0x6F */ undefined8 unk6F;
    /* 0x70 */ s32 args[4];      // values the number opcodes format into the text
    /* 0x80 */ s16 stringRef;    // bank in the high byte, entry index in the low byte
    /* 0x82 */ s16 bufferCount;
    /* 0x84 */ s16 waitFrames;   // frames left to wait; 0 while running
    /* 0x86 */ s16 charDelay;    // frames left before the next character
    /* 0x88 */ s16 charDelayReload;
    /* 0x8A */ undefined8 unk8A[2];
    /* 0x8C */ SystemStringNode* pBufferStack;
} SystemString; // Size: 0x90

// The static text object SystemRenderStringEntry renders one entry through,
// and the single row of glyph sprites it uses as that string's char buffer
// (SystemStringInit allocates one instead; this path must not).
extern SystemString g_SystemString;
extern SystemStringChar g_SystemStringChars;
// g_SystemString.pChars, named separately because the typed store comes out 0x28(a0)
// off the live base pointer, 4 bytes short of retail (232 vs 236).
extern void* D_8005A000;

void SystemStringInit(SystemString* pString, s16 vramX, s16 vramY, s16 x, s16 y, s16 maxWidth, s16 maxChars) {
    int i;
    int row;
    int odd;

    pString->x = x;
    pString->flags = 0;
    pString->waitFrames = 0;
    pString->pBufferStack = NULL;
    pString->bufferCount = 0;
    pString->lineHeight = SYSTEM_STRING_LINE_HEIGHT;
    pString->unk68 = 1;
    pString->unk69 = 1;
    pString->maxWidth = maxWidth;
    pString->waiting = 0;
    pString->unk6A = 0;
    pString->unk6D = 0;
    pString->stopCode = 0;
    pString->lineOffset = SYSTEM_STRING_LINE_OFFSET_NONE;
    pString->textureY = vramY;
    pString->y = y;
    pString->maxChars = maxChars;
    pString->maxWidth |= 1;
    pString->spriteWidth = pString->maxWidth << 2;
    pString->imageWidth = pString->maxWidth + 3;

    HeapSetCurrentContentType(HEAP_CONTENT_MES_WORK);
    pString->pChars = HeapAlloc(pString->maxChars * sizeof(SystemStringChar), HEAP_ALLOC_BEST_FIT);
    HeapSetCurrentContentType(HEAP_CONTENT_MES_IMAGE);
    pString->pCharBuffer = HeapAlloc(pString->imageWidth * 0x1C, HEAP_ALLOC_BEST_FIT);

    setlen(&pString->clearTile[0], 3);
    pString->clearTile[0].colorCode = SYSTEM_STRING_TILE_CODE << 24; // r,g,b = 0, code in the top byte
    setXY0Fast(&pString->clearTile[0], pString->x - SYSTEM_STRING_CLEAR_MARGIN_X, pString->y - SYSTEM_STRING_CLEAR_MARGIN_Y);
    setWHFast(&pString->clearTile[0], pString->maxWidth * 4 + SYSTEM_STRING_CLEAR_PAD_WIDTH,
              pString->maxChars * pString->lineHeight + SYSTEM_STRING_CLEAR_PAD_HEIGHT);
    SetSemiTrans(&pString->clearTile[0], 1);
    pString->clearTile[1] = pString->clearTile[0];

    for (i = 0; i < pString->maxChars; i++) {
        setXY0Fast(&pString->pChars[i].sprt[0][0], pString->x, pString->y + pString->lineHeight * i);
        setXY0Fast(&pString->pChars[i].sprt[0][1], pString->x + SYSTEM_TEXTURE_PAGE_WIDTH, pString->y + pString->lineHeight * i);
        row = i / 2;
        odd = i & 1;

        setWHFast(&pString->pChars[i].sprt[0][0],
                  pString->spriteWidth > SYSTEM_TEXTURE_PAGE_WIDTH ? SYSTEM_TEXTURE_PAGE_WIDTH
                                                                 : pString->spriteWidth,
                  SYSTEM_GLYPH_STRIP_HEIGHT);
        setWHFast(&pString->pChars[i].sprt[0][1],
                  pString->spriteWidth <= SYSTEM_TEXTURE_PAGE_WIDTH
                      ? 0 : pString->spriteWidth - (SYSTEM_TEXTURE_PAGE_WIDTH - 0x10),
                  SYSTEM_GLYPH_STRIP_HEIGHT);

        setUV0Fast(&pString->pChars[i].sprt[0][0], (vramX & 0x3F) * 4, (vramY + row * SYSTEM_GLYPH_STRIP_HEIGHT) & 0xFF);
        setUV0Fast(&pString->pChars[i].sprt[0][1], (vramX & 0x3F) * 4, (vramY + row * SYSTEM_GLYPH_STRIP_HEIGHT) & 0xFF);

        setSprt(&pString->pChars[i].sprt[0][0]);
        setSprt(&pString->pChars[i].sprt[0][1]);
        setShadeTex(&pString->pChars[i].sprt[0][0], 1);
        setShadeTex(&pString->pChars[i].sprt[0][1], 1);

        pString->pChars[i].sprt[1][0] = pString->pChars[i].sprt[0][0];
        pString->pChars[i].sprt[1][1] = pString->pChars[i].sprt[0][1];

        pString->pChars[i].imageRect.x = vramX;
        pString->pChars[i].imageRect.y = vramY + row * SYSTEM_GLYPH_STRIP_HEIGHT;
        pString->pChars[i].imageRect.w = pString->imageWidth;
        pString->pChars[i].imageRect.h = SYSTEM_GLYPH_STRIP_HEIGHT;
        pString->pChars[i].rowWidth = 0;
        pString->pChars[i].clut = odd ? g_SystemPalette2 : g_SystemPalette1;
        pString->pChars[i].textureV = vramY + row * SYSTEM_GLYPH_STRIP_HEIGHT;
        pString->pChars[i].paletteIndex = odd;
        pString->pChars[i].unk5B = i;
    }

    SetDrawMode(&pString->drawMode[0], 0, 0, GetTPage(0, 0, vramX, vramY), NULL);
    SetDrawMode(&pString->drawMode[1], 0, 0, GetTPage(0, 0, vramX + 0x40, vramY), NULL);
}

// A loaded file/archive header: a count followed by that many offsets, each
// relative to the start of the header.
typedef struct {
    /* 0x0 */ u32 count;
    /* 0x4 */ u32 offsets[1];
} SystemOffsetTable; // Size: 0x8

unsigned int ResolveArchiveEntryPointers(SystemOffsetTable* pTable) {
    u32 i;
    mem_addr base = (mem_addr)pTable;
    for (i = 1; i <= pTable->count; i++) {
        pTable->offsets[i - 1] += base;
    }

    return pTable->count;
}

// Same as ResolveArchiveEntryPointers, but no return value
void ResolveFileEntryPointers(SystemOffsetTable* pTable) {
    u32 i;
    mem_addr base = (mem_addr)pTable;

    for (i = 1; i <= pTable->count; i++) {
        pTable->offsets[i - 1] += base;
    }
}

void* GetSystemFontFile(void) {
    return g_SystemFontFile;
}

void* GetSystemDataFile(void) {
    return g_SystemDataFile;
}

void SystemFreeFont(void) {
    HeapUnpinBlock(g_SystemFontFile);
    HeapFree(g_SystemFontFile);
    g_SystemFontFile = NULL;
}

void SystemFreeData(void) {
    HeapUnpinBlock(g_SystemDataFile);
    HeapFree(g_SystemDataFile);
    g_SystemDataFile = NULL;
}


void SystemInitializeFont(SystemFontFileHeader* pFontFile) {
    u8* pFontBytes;
    int dataOffset;

    if (pFontFile == NULL) {
        HeapSetCurrentContentType(HEAP_CONTENT_NONE);
        return;
    }

    HeapPinBlock(pFontFile);
    g_SystemFontFile = pFontFile;
    g_SystemFontGlyphData = pFontFile;
    // The header words are read through a byte cursor, not as pFontFile->field:
    // as field lvalues gcc hoists all six loads above the two pointer stores and
    // retail interleaves each load with its store (66.5% vs 100%). Six u16
    // locals and a do/while block around them only reach 72.9%.
    pFontBytes = (u8*)pFontFile;
    dataOffset = *(u16*)(pFontBytes + offsetof(SystemFontFileHeader, glyphDataOffset));
    g_SystemFontLeadByte = *(u16*)(pFontBytes + offsetof(SystemFontFileHeader, leadByte));
    g_SystemFontDoubleByteGlyphOffset = *(u16*)(pFontBytes + offsetof(SystemFontFileHeader, doubleByteGlyphOffset));
    g_SystemFontNarrowGlyphCount = *(u16*)(pFontBytes + offsetof(SystemFontFileHeader, narrowGlyphCount));
    g_SystemFontNarrowTrailByteCount = *(u16*)(pFontBytes + offsetof(SystemFontFileHeader, narrowTrailByteCount));
    g_SystemFontSingleByteCodeBase = *(u16*)(pFontBytes + offsetof(SystemFontFileHeader, singleByteCodeBase));
    g_SystemFontGlyphData = pFontBytes + dataOffset;
}

void SystemInitializeData(void* pSystemData) {
    if (pSystemData == 0) {
        HeapSetCurrentContentType(HEAP_CONTENT_NONE);
        return;
    }
    HeapPinBlock(pSystemData);
    g_SystemDataFile = pSystemData;
    g_SystemDataEntries = pSystemData;
    ResolveArchiveEntryPointers(pSystemData);
    g_SystemDataEntries++;
}

void SystemInitialize(void* pSystemFont, void* pSystemData) {
    SystemInitializeFont(pSystemFont);
    SystemInitializeData(pSystemData);
}


void SystemTransferPaletteToVRAM(short xDest, short yDest) {
    RECT dest;
    setRECT(&dest, xDest, yDest, SYSTEM_PALETTE_COLOR_COUNT, 1);
    LoadImage(&dest, &g_SystemPaletteData);
    g_SystemPalette1 = GetClut(xDest, yDest);
    g_SystemPalette2 = GetClut(xDest + SYSTEM_PALETTE_CLUT_COLOR_COUNT, yDest);
}

// A string bank: two header words, then one u16 offset per entry, each
// relative to the start of the bank. The entry itself is the string.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 offsets[1];
} SystemStringBank; // Size: 0x6

void* GetStringEntry(void* stringData, u32 entry) {
    SystemStringBank* pBank = stringData;
    return (u8*)pBank + pBank->offsets[entry];
}

// One dialog's window size
typedef struct {
    /* 0x0 */ u8 width;
    /* 0x1 */ u8 height;
} DialogDimensions; // Size: 0x2

// Dialog data: a header, then (dimensionOffset entries past it) one
// DialogDimensions per dialog, in dialog order.
typedef struct {
    /* 0x0 */ u16 dimensionOffset;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ DialogDimensions dimensions[1];
} DialogDataHeader; // Size: 0x8

u8 DialogGetWidth(DialogDataHeader* pDialogData, int dialogIndex) {
    DialogDimensions* pDimensions = &pDialogData->dimensions[pDialogData->dimensionOffset] + dialogIndex;
    return pDimensions->width;
}

u8 DialogGetHeight(DialogDataHeader* pDialogData, int dialogIndex) {
    DialogDimensions* pDimensions = &pDialogData->dimensions[pDialogData->dimensionOffset] + dialogIndex;
    return pDimensions->height;
}

// The banks the getters below index are SYSTEM_STRING_BANK_* in system/archive.h
void* GetStringEntryFromBank(int groupIndex, int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[groupIndex], entryId);
}

void* GetStringEntryFromBank10(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x10], entryId);
}

void* GetAccessoryName(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_ACCESSORY], entryId);
}

void* GetItemName(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_ITEM], entryId);
}

void* GetWeaponName(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_WEAPON], entryId);
}

void* GetStringEntryFromBank18(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x18], entryId);
}

void* GetStringEntryFromBank19(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x19], entryId);
}

void* GetStringEntryFromBank12(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x12], entryId);
}

void* GetStringEntryFromBank14(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x14], entryId);
}

void* GetStringEntryFromBank13(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x13], entryId);
}

void* GetStringEntryFromBank15(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x15], entryId);
}

void* GetStringEntryFromBank1B(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GLYPH], entryId);
}

void* GetStringEntryFromBank1C(int groupIndex, int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x1C + groupIndex], entryId);
}

void* GetStringEntryFromBank30(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x30], entryId);
}

void* GetGearAccessoryName(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GEAR_ACCESSORY], entryId);
}

void* GetGearWeaponName(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GEAR_WEAPON], entryId);
}

void* GetStringEntryFromBank34(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x34], entryId);
}

void SystemDecodeGlyphCodes(u16* pInput) {
    SystemGlyphEntry* pGlyphTable = g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GLYPH];
    u8* pOutput = g_SystemGlyphDecodeBuffer;
    SystemGlyphEntry* pEntry;

    while (*pInput != SYSTEM_GLYPH_CODE_END) {
        // index-first address sum: the typed subscript pGlyphTable[*pInput]
        // emits the commuted addu and measures 99.67%
        pEntry = (SystemGlyphEntry*)((*pInput) * sizeof(SystemGlyphEntry) + (mem_addr)pGlyphTable);
        pInput++;

        if (pEntry->firstByte) {
            *pOutput++ = pEntry->firstByte;
            *pOutput++ = pEntry->secondByte;
        } else {
            *pOutput++ = pEntry->secondByte;
        }
    }

    *pOutput = 0;
}

void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count) {
    int i;
    SystemGlyphEntry* pGlyphTable = g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GLYPH];
    SystemGlyphEntry* pEntry;

    for(i = count-1; i != -1; i--) {
        pEntry = (SystemGlyphEntry*)((*pInput) * sizeof(SystemGlyphEntry) + (mem_addr)pGlyphTable);
        pInput++;

        if(pEntry->firstByte) {
            *pOutput++ = pEntry->firstByte;
            *pOutput++ = pEntry->secondByte;

        } else {
            *pOutput++ = pEntry->secondByte;
        }
    }

    *pOutput = 0;
}

int SystemFindGlyphCode(u8 firstByte, u8 secondByte) {
    s16 i;
    SystemGlyphEntry* pGlyph = g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GLYPH];

    for (i = 0; i < SYSTEM_GLYPH_CODE_COUNT; i++) {
        if (pGlyph[i].firstByte == firstByte && pGlyph[i].secondByte == secondByte) {
            return i;
        }
    }

    return SYSTEM_GLYPH_CODE_NOT_FOUND;
}

int SystemEncodeGlyphCodes(u8* pInput, u16* pOutput) {
    int c = *pInput++;
    int glyph;
    u8 firstByte;
    u8 secondByte;

    while (c != 0) {
        if (c < g_SystemFontLeadByte) {
            firstByte = 0;
            secondByte = c;
        } else {
            firstByte = c;
            secondByte = *pInput++;
        }
        glyph = SystemFindGlyphCode(firstByte, secondByte);
        *pOutput++ = glyph;

        if ((u16)glyph == SYSTEM_GLYPH_CODE_NOT_FOUND) {
            return -1;
        }

        c = *pInput++;
    }

    return 0;
}

int SystemStringGetUnk6B(SystemString* pString) {
    if (pString->flags & SYSTEM_STRING_FLAG_STOPPED) {
        return pString->stopCode;
    }

    return 0;
}

void SystemFormatNumberGlyphs(int value, int glyphOffset, int flag) {
    u32 divisor = SYSTEM_NUMBER_GLYPH_FIRST_DIVISOR;
    u16* pDigit = g_SystemNumberGlyphBuffer;
    u16* pEnd;
    u16* pScan;
    int i;
    int sign = flag;
    s32 v;

    v = value;
    glyphOffset *= SYSTEM_NUMBER_GLYPH_SET_STRIDE;

    if (sign != 0) {
        if (v >= 0) {
            sign = SYSTEM_NUMBER_GLYPH_PLUS;
        } else {
            v = -v;
            sign = SYSTEM_NUMBER_GLYPH_MINUS;
        }
    }

    for (i = 0; i < SYSTEM_NUMBER_GLYPH_DIGIT_COUNT; i++) {
        pDigit[i] = v / divisor + glyphOffset;
        v %= divisor;
        divisor /= 10;
    }

    pEnd = &g_SystemNumberGlyphBuffer[SYSTEM_NUMBER_GLYPH_DIGIT_COUNT];
    pScan = pEnd - SYSTEM_NUMBER_GLYPH_SLOT_COUNT;
    pEnd[0] = SYSTEM_GLYPH_CODE_END;
    pEnd[-SYSTEM_NUMBER_GLYPH_SLOT_COUNT] = glyphOffset;

    /* pEnd keeps the scan start so the end is re-formed as pEnd + 10 inside
       the loop; hoisting it here makes cse fold it to pEnd - 2 and reorg
       steal it into the bne delay slot. */
    pEnd = pScan;

    if ((glyphOffset & 0xFFF0) == glyphOffset) {
        for (;;) {
            if (pScan == pEnd + SYSTEM_NUMBER_GLYPH_DIGIT_COUNT) {
                goto scanDone;
            }
            pScan++;
            if (*pScan != glyphOffset) {
                goto scanDone;
            }
        }
    }
scanDone:

    if (sign != 0) {
        *--pScan = sign + glyphOffset;
    }

    SystemDecodeGlyphCodes(pScan);
}

void SystemStringSetCurrentData(SystemString* pString, void* pData) {
    void* pOld = pString->pCur;

    pString->pCur = pData;
    pString->pPrev = pOld;
    pString->flags |= SYSTEM_STRING_FLAG_DATA_OVERRIDE;
}

// Parking note, round 445 - all of it measured, not guessed.
// SIZES: retail is 508 instructions (2032 bytes), frame 0x48, cc1 vars=16. This
// body is 512 instructions (2048 bytes), frame 0x40, vars=8 -> 87.71%. The OLD
// note's "521 against retail's 540, NINETEEN short" was wrong: those are objdump
// line counts (label rows + relocations + the 16 jtbl words). Retail is 508 and the
// old body was 521, so it was THIRTEEN TOO LONG. The old note's other point, the
// 8-byte frame, is real: retail's source had two more spilled pseudos.
// FIXED THIS ROUND, in order of what they bought:
// 1. The row block recomputed the char offset once per store (77.74 -> 82.99). A
//    local `index = pString->charIndex` and pChars[index].x five times is retail's
//    shape: it keeps charIndex in t0 across all five stores and reloads only the
//    pChars base, because the sb/sh stores alias the struct. Reading
//    pString->charIndex each time makes gcc reload it after every store.
// 2. The opcode 0 arm was SEMANTICALLY WRONG in the old park (82.99 -> 87.47). It is
//    not `waitFrames = pCur[2]; pCur += 3`. Retail 0x34044: if (flags &
//    DATA_OVERRIDE) { flags &= ~DATA_OVERRIDE; pCur = pPrev + 1; continue; } else
//    { flags |= STOPPED; stopCode = 1; waiting = 1; return; }.
// 3. Case 0x5: retail has ONE SystemStringSetCurrentData per arm (0x342D8) fed by a
//    pData local, and the bridge's GetStringEntry (entries[0x1A], 0) is its own call
//    site (0x34298). Writing it that way is 87.16 / 512; three separate calls is
//    87.47 / 516 and gcc merges two of them into the shared tail. Kept the smaller
//    one: neither links, and 512 is two instructions closer to 508.
// 4. `u16 index`, not s32: retail tests `index < 0x80` with sltiu and s32 gave slti
//    (87.16 -> 87.71, same bytes).
// STILL OPEN: 4 extra instructions. (a) Retail has an `andi 0xFFFF` after the u8 load
// in three places (the opcode test, case 0x1, case 0x5's index) that gcc emits for
// no spelling tried here - it looks like a u8 -> u16 conversion of an int-punned
// load, and the opcode test has an `andi 0xFF` in front of it as well. (b) The
// frame/vars gap above. (c) Register allocation: retail holds pCur in a1 across the
// loop head (lbu a0,0(a1), lbu v1,1(a1)) where this body uses v1, and the case index
// is a3 there vs a1 here. (d) Case 0x9/0xA/0xC: retail sets the two
// SystemFormatNumberGlyphs flag args in the case bodies before the shared tail
// starts, so its tail begins at the index read; assigning them to locals first does
// not reproduce it (gcc folds the constants back - measured, no change).
// TRIED AND REJECTED: count++ placed after the SetCurrentData call (87.73 but 517
// instructions); preset is right (this body 87.71, gcc-2.6.0 87.55, gcc-2.7.2-cdk
// 80.76 - do not re-screen it).
// STILL VALID FROM THE OLD NOTE: the outer dispatch is an if/else chain in the order
// 0, 3, 0xF, 2, 1, draw (as a switch gcc emits a second jump table, 65.28%); the 0xF
// sub-switch is a real 16-case switch in ascending order whose table lands at +0 of
// this unit's .rodata range; `row` must be s32; textureV sums as `row / 2 * 13 +
// textureY` but imageRect.y as `textureY + row / 2 * 13` (opposite addu operand
// order, still open).
// pCur is void* in SystemString (this file's own struct) so the script is read
// through a u8 pointer; retyping pCur would not be safe for the matched neighbours.
/*
void func_80033DF0(SystemString* pString) {
    s32 count;
    u16 index;
    s32 firstByte;
    s32 secondByte;
    s32 advance;
    u16 width;
    u16 opcode;
    s32 row;
    void* pData;

    count = pString->unk69;

    // The line is full: move to the next row of the window
    if (pString->lineWidth > pString->maxWidth) {
        pString->lineWidth = 0;
        pString->charIndex++;
        pString->rowCount++;

        if (pString->charIndex >= pString->maxChars) {
            pString->charIndex = 0;
            pString->flags |= SYSTEM_STRING_FLAG_ROW_OVERFLOW;
        }

        if (pString->flags & SYSTEM_STRING_FLAG_ROW_OVERFLOW) {
            pString->pChars[pString->scrollRow].rowWidth = 0;
            pString->scrollRow++;

            if (pString->scrollRow >= pString->maxChars) {
                pString->scrollRow = 0;
            }
        }

        row = pString->rowCount % (pString->maxChars + 1);
        index = pString->charIndex;
        pString->pChars[index].textureV = row / 2 * SYSTEM_GLYPH_STRIP_HEIGHT + pString->textureY;
        pString->pChars[index].clut = (row & 1) ? g_SystemPalette2 : g_SystemPalette1;
        pString->pChars[index].paletteIndex = row & 1;
        pString->pChars[index].unk5B = row;
        pString->pChars[index].imageRect.y = pString->textureY + row / 2 * SYSTEM_GLYPH_STRIP_HEIGHT;
    }

    if (pString->waiting != 0) {
        pString->waiting = 0;
        pString->flags &= ~SYSTEM_STRING_FLAG_BUFFER_POP;
        return;
    }

    while (--count != -1) {
        opcode = ((u8*)pString->pCur)[0];

        if (opcode == 0x0) {
            // end of the entry: go back to the data the override replaced, or stop
            if (pString->flags & SYSTEM_STRING_FLAG_DATA_OVERRIDE) {
                pString->flags &= ~SYSTEM_STRING_FLAG_DATA_OVERRIDE;
                pString->pCur = (u8*)pString->pPrev + 1;
                continue;
            }

            pString->flags |= SYSTEM_STRING_FLAG_STOPPED;
            pString->stopCode = 1;
            pString->waiting = 1;
            return;
        } else if (opcode == 0x3) {
            // end of the entry
            pString->stopCode = 3;
            pString->flags |= SYSTEM_STRING_FLAG_STOPPED;
            pString->pCur = (u8*)pString->pCur + 1;
            return;
        } else if (opcode == 0xF) {
            switch (((u8*)pString->pCur)[1]) {
            case 0x0: // end of the entry: wait, then resume after these three bytes
                pString->waitFrames = ((u8*)pString->pCur)[2];
                pString->pCur = (u8*)pString->pCur + 3;
                return;
            case 0x1: // set the delay one glyph is drawn with
                index = ((u8*)pString->pCur)[2];

                if (index != 0) {
                    count += index;
                    pString->unk6A = pString->unk68;
                    pString->unk68 = index;
                    pString->unk69 = index;
                } else {
                    pString->unk68 = pString->unk6A;
                    pString->unk69 = pString->unk6A;
                    pString->unk6A = 0;
                }

                pString->pCur = (u8*)pString->pCur + 3;
                break;
            case 0x2: // end of the entry: wait, then resume after these three bytes
                pString->waitFrames = ((u8*)pString->pCur)[2];
                pString->pCur = (u8*)pString->pCur + 3;
                pString->waiting = 1;
                return;
            case 0x3: // render a string of the bank the next byte names
                index = ((u8*)pString->pCur)[2];
                secondByte = ((u8*)pString->pCur)[3];
                count++;
                pString->pCur = (u8*)pString->pCur + 3;
                SystemStringSetCurrentData(pString, GetStringEntry(g_SystemDataEntries->entries[index], secondByte));
                break;
            case 0x4: // render the string stringRef names
                index = pString->stringRef;
                pString->pCur = (u8*)pString->pCur + 1;

                switch (index & 0xFF00) {
                case 0x000:
                    SystemStringSetCurrentData(
                        pString, GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_ITEM], index & 0xFF));
                    break;
                case 0x100:
                    SystemStringSetCurrentData(
                        pString, GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_WEAPON], index & 0xFF));
                    break;
                case 0x200:
                    SystemStringSetCurrentData(pString, GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_ACCESSORY],
                                                                      index & 0xFF));
                    break;
                case 0x300:
                    SystemStringSetCurrentData(pString, GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GEAR_WEAPON],
                                                                      index & 0xFF));
                    break;
                case 0x400:
                    SystemStringSetCurrentData(pString, GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GEAR_ACCESSORY],
                                                                      index & 0xFF));
                    break;
                }
                break;
            case 0x5: // render one of the game state's strings
                index = ((u8*)pString->pCur)[2];
                count++;
                pString->pCur = (u8*)pString->pCur + 2;

                if (index < 0x80) {
                    pData = &g_GameState.strings[index];
                } else if (((u8*)&g_GameState.fieldScriptMemory[0x1C2])[index] == SYSTEM_STRING_INDEX_NONE) {
                    pData = GetStringEntry(g_SystemDataEntries->entries[0x1A], 0);
                } else {
                    pData = &g_GameState.strings[((u8*)&g_GameState.fieldScriptMemory[0x1C2])[index]];
                }

                SystemStringSetCurrentData(pString, pData);
                break;
            case 0x6: // render a string of the weapon bank
                index = ((u8*)pString->pCur)[2];
                count++;
                pString->pCur = (u8*)pString->pCur + 2;
                SystemStringSetCurrentData(pString,
                                           GetStringEntry(g_SystemDataEntries->entries[SYSTEM_STRING_BANK_WEAPON], index));
                break;
            case 0x7: // render a string of bank 0x18
                index = ((u8*)pString->pCur)[2];
                count++;
                pString->pCur = (u8*)pString->pCur + 2;
                SystemStringSetCurrentData(pString, GetStringEntry(g_SystemDataEntries->entries[0x18], index));
                break;
            case 0x8: // render a string of bank 0x19
                index = ((u8*)pString->pCur)[2];
                count++;
                pString->pCur = (u8*)pString->pCur + 2;
                SystemStringSetCurrentData(pString, GetStringEntry(g_SystemDataEntries->entries[0x19], index));
                break;
            case 0x9: // draw one of the string's numbers
                index = ((u8*)pString->pCur)[2];
                count++;
                pString->pCur = (u8*)pString->pCur + 2;
                SystemFormatNumberGlyphs(pString->args[index], 0, 0);
                SystemStringSetCurrentData(pString, g_SystemGlyphDecodeBuffer);
                break;
            case 0xA: // draw one of the string's numbers, with a sign
                index = ((u8*)pString->pCur)[2];
                count++;
                pString->pCur = (u8*)pString->pCur + 2;
                SystemFormatNumberGlyphs(pString->args[index], 1, 0);
                SystemStringSetCurrentData(pString, g_SystemGlyphDecodeBuffer);
                break;
            case 0xB: // set the offset a row is drawn at
                index = ((u8*)pString->pCur)[2];
                pString->pCur = (u8*)pString->pCur + 2;
                pString->unk6D = index;
                break;
            case 0xC: // draw one of the string's numbers, with a plus sign
                index = ((u8*)pString->pCur)[2];
                count++;
                pString->pCur = (u8*)pString->pCur + 2;
                SystemFormatNumberGlyphs(pString->args[index], 1, 1);
                SystemStringSetCurrentData(pString, g_SystemGlyphDecodeBuffer);
                break;
            case 0xD: // end of the entry: wait on the upload before going on
                pString->waitFrames = ((u8*)pString->pCur)[2];
                pString->pCur = (u8*)pString->pCur + 3;
                pString->waiting = 1;
                pString->flags |= SYSTEM_STRING_FLAG_WAITING;
                return;
            case 0xE: // set the delay between two glyphs
                index = ((u8*)pString->pCur)[2];
                pString->unk6A = pString->unk68;
                pString->unk68 = 1;
                pString->unk69 = 1;
                pString->pCur = (u8*)pString->pCur + 3;
                pString->charDelayReload = index;
                pString->charDelay = index;
                return;
            case 0xF: // render the string a button's name comes from
                index = g_ControllerButtonMappings[((u8*)pString->pCur)[2]];
                count++;
                pString->pCur = (u8*)pString->pCur + 2;
                SystemStringSetCurrentData(pString, GetStringEntry(g_SystemDataEntries->entries[0x31], index));
                break;
            }
        } else if (opcode == 0x2) {
            // stop, and let the caller decide what to do with the string
            pString->stopCode = 2;
            pString->flags |= SYSTEM_STRING_FLAG_STOPPED | SYSTEM_STRING_FLAG_UPLOAD_HOLD;
            pString->pCur = (u8*)pString->pCur + 1;

            if (((u8*)pString->pCur)[0] == 1) {
                pString->pCur = (u8*)pString->pCur + 1;
            }

            return;
        } else if (opcode == 0x1) {
            // the line is full whatever is left of it: wrap on the next call
            pString->lineWidth = 100;
            pString->pCur = (u8*)pString->pCur + 1;
            return;
        } else {
            // any other byte is a glyph code
            advance = 1;

            if (opcode < g_SystemFontLeadByte) {
                firstByte = 0;
                secondByte = opcode;
            } else {
                firstByte = opcode;
                secondByte = ((u8*)pString->pCur)[1];
                advance = 2;
            }

            width = SystemGetGlyphWidth(firstByte, secondByte);

            // The glyph does not fit on this line: the wrap happens on the next call
            if (pString->maxWidth < pString->lineWidth + width) {
                pString->lineWidth += width;
                return;
            }

            func_80034FFC(firstByte, secondByte, (u16*)pString->pCharBuffer + pString->lineWidth, pString->imageWidth,
                          pString->pChars[pString->charIndex].paletteIndex);
            pString->pCur = advance + (u8*)pString->pCur;
            pString->lineWidth += width;
            pString->pChars[pString->charIndex].rowWidth = pString->lineWidth;
        }
    }
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80033DF0);


void SystemStringClearStateFlags(SystemString* pString) {
    pString->flags &= ~SYSTEM_STRING_FLAG_STOPPED;

    if (pString->flags & SYSTEM_STRING_FLAG_WAITING) {
        pString->waitFrames = 0;
        pString->waiting = 0;
        pString->flags &= ~SYSTEM_STRING_FLAG_WAITING;
    }
}

void SystemStringResetIfIdle(SystemString* pString) {
    if (pString->waitFrames == 0) {
        pString->waiting = 0;
        pString->flags &= SYSTEM_STRING_FLAG_PERSISTENT;
    }
}

void SystemStringFreeBufferStack(SystemString* pString) {
    SystemStringNode* pNode;
    SystemStringNode* pOld;

    if (pString->waitFrames != 0) {
        return;
    }

    pNode = pString->pBufferStack;

    while (pNode != NULL) {
        pOld = pNode;
        pNode = pNode->pNext;
        HeapFree(pOld);
    }

    pString->pBufferStack = NULL;
    pString->bufferCount = 0;
}

void SystemStringReset(SystemString* pString) {
    pString->waiting = 0;
    pString->waitFrames = 0;
    pString->flags &= SYSTEM_STRING_FLAG_PERSISTENT;
    SystemStringFreeBufferStack(pString);
}

void SystemStringFree(SystemString* pString) {
    SystemStringReset(pString);
    HeapFree(pString->pChars);
    HeapFree(pString->pCharBuffer);
}

int SystemStringPushBuffer(SystemString* pString, void* pData) {
    SystemStringNode* pCur = pString->pBufferStack;
    SystemStringNode* pNode;

    pString->bufferCount++;
    HeapSetCurrentContentType(HEAP_CONTENT_MES_CUE);
    pNode = HeapAlloc(sizeof(SystemStringNode), HEAP_ALLOC_BEST_FIT);
    pNode->pData = pData;
    pNode->pNext = NULL;

    if (pCur == NULL) {
        pString->pBufferStack = pNode;
        return pString->bufferCount;
    }

    while (pCur->pNext != NULL) {
        pCur = pCur->pNext;
    }

    pCur->pNext = pNode;

    return pString->bufferCount;
}

int SystemStringGetColumnOffset(SystemString* pString) {
    return pString->x + pString->lineWidth * 4;
}

int SystemStringGetRowOffset(SystemString* pString) {
    int row = pString->charIndex - pString->scrollRow;

    if (row < 0) {
        row = pString->maxChars - 1;
    }

    return pString->y + row * pString->lineHeight;
}

void SystemStringSetColor(SystemString* pString, u8 red, u8 green, u8 blue) {
    SystemStringChar* pChar;
    int i;

    for (i = 0; i < pString->maxChars; i++) {
        pChar = &pString->pChars[i];

        pChar->sprt[1][1].r0 = red;
        pChar->sprt[1][0].r0 = red;
        pChar->sprt[0][1].r0 = red;
        pChar->sprt[0][0].r0 = red;
        pChar->sprt[1][1].g0 = green;
        pChar->sprt[1][0].g0 = green;
        pChar->sprt[0][1].g0 = green;
        pChar->sprt[0][0].g0 = green;
        pChar->sprt[1][1].b0 = blue;
        pChar->sprt[1][0].b0 = blue;
        pChar->sprt[0][1].b0 = blue;
        pChar->sprt[0][0].b0 = blue;
    }
}

void SystemStringSetLineOffset(SystemString* pString, u8 value) {
    pString->lineOffset = value;
}

void SystemStringClearLineOffset(SystemString* pString) {
    pString->lineOffset = SYSTEM_STRING_LINE_OFFSET_NONE;
}

// Width, in the units a row's width is counted in (4 VRAM units), at which the
// row's glyphs cross from the first texture page into the second
#define SYSTEM_STRING_PAGE_SPLIT_WIDTH 0x40

// A rendered string's flag bits that this unit only sets and clears
#define SYSTEM_STRING_FLAG_WRAPPED 0x20     // the row wrapped: relayout from the top
#define SYSTEM_STRING_FLAG_PALETTE_DOUBLE 0x100 // the palette holds two codes per glyph
#define SYSTEM_STRING_FLAG_UPLOAD_WAIT 0x10 // an upload is in flight a frame longer

// AddPrim inlined: the clear tile goes into the ordering table through the
// two-word link below, the row sprites through func_80031798 and the draw mode
// through the AddPrim function itself.
#define SystemLinkPrim(pOt, pPrim)                                                             \
    (*(u_long*)(pPrim) = (*(u_long*)(pPrim) & 0xFF000000) | (*(u_long*)(pOt) & 0xFFFFFF),       \
     *(u_long*)(pOt) = (*(u_long*)(pOt) & 0xFF000000) | ((u_long)(pPrim) & 0xFFFFFF))

extern void func_80031798(void* pOt, void* pPrim);


// Parked at 97.26% (1564 bytes against retail's 1572: TWO instructions short).
// Everything below is measured, not guessed.
// - THE ROW-RESET LOOP COUNTS WITH `row`, NOT A LOCAL: `for (row = 0; row <
//   pString->maxChars; row++) pString->pChars[row].rowWidth = 0;` (with
//   `row = pString->scrollRow;` after it) is what makes row's pseudo live across
//   the sprite loops' calls, so global-alloc gives row s3 and i s4 - retail's map
//   - and the reset loop's own counter comes out in s3 with the byte offset in
//   v1, exactly as retail has it (96.83 -> 97.26, and both sprite loops' giv
//   chains, the mult operands and the row-reset loop become instruction
//   identical).
// - THE SOURCE HAS NO pChar / pSprite LOCALS. Retail re-reads pString->pChars
//   (lw v1,40(s0)) for every single store and re-forms the sprite address each
//   time; a pointer local CSEs all of that and comes out ~180 bytes short at 68%.
//   Everything stays spelled inline (pString->pChars[row].sprt[context][1].v0 = ...).
// - THE BUFFER POP NEEDS pNode's ASSIGNMENT FIRST: pNode = pString->pBufferStack;
//   pString->pCur = pNode->pData; pString->pBufferStack =
//   pString->pBufferStack->pNext; HeapFree(pNode); is what puts the a0 load (the
//   HeapFree argument and pData's base) before the v1 load (pNext) AND restores
//   retail's nop in the `beqz bufferCount` delay slot (both observed).
// - THE unk6A BLOCK NEEDS A SECOND STORE: the branch body must also write
//   pString->unk69 = pString->unk6A before unk6A = 0. Retail stores 0x69 twice
//   (once in the branch, once after the join) and reloads unk6A for the first
//   one; without that statement gcc reuses the test's load.
//   Those two shapes took the park 1556 -> 1564 bytes and make both blocks
//   instruction-identical to retail's.
// - THE REMAINING TWO INSTRUCTIONS ARE IN THE unk69/upload-hold SEAM. Retail:
//   lbu 0x68 / nop / sb 0x69 / lhu 0x10(flags) / nop / andi 0x40. This body lets
//   sched2 raise the flags load ABOVE the sb (lbu / lhu / sb / andi), which drops
//   both the load-delay nop after lbu and the one after the lhu. Measured
//   non-fixes: a volatile u16 read of flags in the if (96.04%, 1576 bytes, breaks
//   ordering elsewhere), a volatile u8 store of unk69 (no change), and reading
//   flags once into a `flags = *(volatile u16*)&pString->flags;` local used by the
//   whole hold test (97.26%, 1564 bytes - byte-identical, sched2 still steals the
//   load into the lbu's delay slot, so a volatile lvalue is not a scheduling
//   barrier here). Retail's merge block is [sb, lhu, andi...] with the lhu owning
//   an unfillable nop slot; ours schedules the load first and reorg fills the
//   preceding block's lbu slot with it, which is the whole 2-instruction gap.
// - THE REGISTER MAP MATCHES RETAIL'S NOW: s1 = row*0x60 (the derived giv), s2 =
//   context*0x28, s3 = row, s4 = i. See the reset-counter bullet: it was the
//   local `j` counter that forced s3 = i. (Declaration order of i/row and scoping
//   each loop's locals in their own block are both no-ops, byte-identical.)
// - THE ~9 `addu v1,s2,v1` SITES (context sum base-first) come out
//   `addu v1,v1,s3` here. Retail has BOTH orders in the same function (0x252A0
//   base-first, 0x2532C index-first), so this is per-site, not one spelling.
// - THE CLEAR-TILE TAIL (retail 0x34DCC..0x34E68) is the same 26 instructions in
//   a different order: retail materialises 0xFFFFFF in a2 and 0xFF000000 in v1
//   AFTER the mflo, this body hoists 0xFF000000 into a1 before the mult and does
//   the `addu s0,ctx*0x10` sum one statement later. Retail's shape is
//   addiu a1,a1,0x48 / addu a1,s0,a1 as two statements; this body's `&clearTile
//   [context]` + `(maxWidth|1)` group re-derives it after the tag load.
// - The frame matches retail exactly (0x80, vars=80): the older note claiming
//   0x78 was wrong.
// - Loop 2's width clamp must stay an if/else (the ternary costs 0.99%).
// 97.26%, 1564 bytes (2 instructions short). Repro: unpark and diff.
/*
void func_80034888(SystemString* pString, u_long* pOt, int context) {
    SystemStringNode* pNode;
    int i;
    int row;

    if (!(pString->flags & SYSTEM_STRING_FLAG_BUFFER_POP)) {
        if (pString->bufferCount == 0) {
            return;
        }

        pNode = pString->pBufferStack;
        pString->pCur = pNode->pData;
        pString->pBufferStack = pString->pBufferStack->pNext;
        HeapFree(pNode);

        pString->bufferCount--;
        pString->flags = (pString->flags & SYSTEM_STRING_FLAG_PERSISTENT) | 0x24;
        if (pString->unk6A != 0) {
            pString->unk68 = pString->unk6A;
            pString->unk69 = pString->unk6A;
            pString->unk6A = 0;
        }
        pString->unk69 = pString->unk68;
        pString->charDelayReload = 0;
        pString->charDelay = 0;
    }

    pString->unk69 = (pString->flags & SYSTEM_STRING_FLAG_PALETTE_DOUBLE) ? pString->unk68 * 3 : pString->unk68;

    if (pString->flags & SYSTEM_STRING_FLAG_UPLOAD_HOLD) {
        if (!(pString->flags & SYSTEM_STRING_FLAG_STOPPED)) {
            pString->flags = (pString->flags & ~SYSTEM_STRING_FLAG_UPLOAD_HOLD) | SYSTEM_STRING_FLAG_WRAPPED;
        }
    }

    if (pString->flags & SYSTEM_STRING_FLAG_WRAPPED) {
        pString->scrollRow = 0;
        pString->rowCount = 0;
        pString->charIndex = 0;
        pString->lineWidth = 0;
        pString->pChars[0].textureV = pString->textureY;
        pString->pChars[0].clut = g_SystemPalette1;
        pString->pChars[0].paletteIndex = 0;
        pString->pChars[0].imageRect.y = pString->textureY;
        for (row = 0; row < pString->maxChars; row++) {
            pString->pChars[row].rowWidth = 0;
        }
        pString->flags &= ~(SYSTEM_STRING_FLAG_WRAPPED | 0x1);
    }

    row = pString->scrollRow;
    for (i = 0; i < pString->maxChars; i++) {
        if (row >= pString->maxChars) {
            row = 0;
        }

        if (pString->lineOffset != i) {
            pString->pChars[row].sprt[context][1].code |= 1;
        } else {
            pString->pChars[row].sprt[context][1].code &= ~1;
        }

        // The second half of the row is only drawn when the row is wider than
        // one texture page
        if (pString->pChars[row].rowWidth > SYSTEM_STRING_PAGE_SPLIT_WIDTH) {
            pString->pChars[row].sprt[context][1].v0 = pString->pChars[row].textureV;
            pString->pChars[row].sprt[context][1].clut = pString->pChars[row].clut;
            pString->pChars[row].sprt[context][1].y0 = pString->y + pString->lineHeight * i;
            pString->pChars[row].sprt[context][1].w =
                (pString->pChars[row].rowWidth - SYSTEM_STRING_PAGE_SPLIT_WIDTH) * 4;
            func_80031798(pOt, &pString->pChars[row].sprt[context][1]);
        }
        row++;
    }

    AddPrim(pOt, &pString->drawMode[1]);

    row = pString->scrollRow;
    for (i = 0; i < pString->maxChars; i++) {
        if (row >= pString->maxChars) {
            row = 0;
        }

        if (pString->lineOffset != i) {
            pString->pChars[row].sprt[context][0].code |= 1;
        } else {
            pString->pChars[row].sprt[context][0].code &= ~1;
        }

        // The first half of a row is a whole texture page wide as soon as the
        // row's width passes the split
        if (pString->pChars[row].rowWidth != 0) {
            pString->pChars[row].sprt[context][0].v0 = pString->pChars[row].textureV;
            pString->pChars[row].sprt[context][0].clut = pString->pChars[row].clut;
            pString->pChars[row].sprt[context][0].y0 = pString->y + pString->lineHeight * i;
            if (pString->pChars[row].rowWidth > SYSTEM_STRING_PAGE_SPLIT_WIDTH) {
                pString->pChars[row].sprt[context][0].w = SYSTEM_TEXTURE_PAGE_WIDTH;
            } else {
                pString->pChars[row].sprt[context][0].w = pString->pChars[row].rowWidth * 4;
            }
            func_80031798(pOt, &pString->pChars[row].sprt[context][0]);
        }
        row++;
    }

    if (pString->waitFrames != 0) {
        pString->waitFrames--;
    } else if (pString->charDelay != 0) {
        pString->charDelay--;
    } else {
        pString->charDelay = pString->charDelayReload;

        if (!(pString->flags & (SYSTEM_STRING_FLAG_UPLOAD_HOLD | SYSTEM_STRING_FLAG_STOPPED |
                                SYSTEM_STRING_FLAG_UPLOAD_WAIT))) {
            func_80033DF0(pString);
            LoadImage(&pString->pChars[pString->charIndex].imageRect, pString->pCharBuffer);
        }
    }

    if (pString->waitFrames != 0) {
        if (--pString->waitFrames == -1) {
            pString->flags &= ~SYSTEM_STRING_FLAG_UPLOAD_WAIT;
        }
    }

    if (!(pString->flags & SYSTEM_STRING_FLAG_PERSISTENT)) {
        setXY0Fast(&pString->clearTile[context], pString->x - SYSTEM_STRING_CLEAR_MARGIN_X,
                   pString->y - SYSTEM_STRING_CLEAR_MARGIN_Y);
        setWHFast(&pString->clearTile[context], (pString->maxWidth | 1) * 4 + SYSTEM_STRING_CLEAR_PAD_WIDTH,
                  pString->maxChars * pString->lineHeight + SYSTEM_STRING_CLEAR_PAD_HEIGHT);
        SystemLinkPrim(pOt, &pString->clearTile[context]);
    }

    pString->flags &= ~SYSTEM_STRING_FLAG_PALETTE_DOUBLE;
    AddPrim(pOt, &pString->drawMode[0]);
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034888);



// Primitive code of the free-size textured rects a rendered row's sprites are
#define SYSTEM_STRING_RECT_CODE 0x64

// Render string entry to a buffer
int SystemRenderStringEntry(void* pCur, void* pCharBuffer, int mode, int flag) {
    SystemString* pString = &g_SystemString;
    int primitiveCode;

    g_SystemString.maxWidth = mode;
    g_SystemString.maxChars = 1;
    g_SystemString.maxWidth = mode | 1;
    g_SystemString.spriteWidth = (s16)(mode | 1) * 4;
    primitiveCode = SYSTEM_STRING_RECT_CODE;
    g_SystemString.pCur = pCur;
    g_SystemString.imageWidth = (mode | 1) + 3;
    g_SystemString.unk68 = 1;
    do {
        g_SystemString.waitFrames = 0;
        g_SystemString.waiting = 0;
        g_SystemString.unk6A = 0;
        g_SystemString.pCharBuffer = pCharBuffer;
        g_SystemString.flags = 0;
        g_SystemString.charIndex = 0;
        g_SystemString.lineWidth = 0;
        g_SystemString.unk69 = primitiveCode;
    } while (0);
    D_8005A000 = &g_SystemStringChars;
    g_SystemStringChars.rowWidth = 0;
    g_SystemStringChars.paletteIndex = flag & 1;

    // The do/while block above and the two hoisted locals are load-bearing:
    // without them gcc hoists the constant stores above the group (28.31%), and
    // pString is what puts the group's lui/addiu for $a0 at its head.
    func_80033DF0(pString);

    return g_SystemString.pChars->rowWidth * 4;
}


int SystemGetGlyphWidth(u16 first, u16 second) {
    if (first == 0) {
        if (second - g_SystemFontSingleByteCodeBase < g_SystemFontNarrowGlyphCount) {
            return SYSTEM_GLYPH_WIDTH_NARROW;
        }

        return SYSTEM_GLYPH_WIDTH_WIDE;
    }

    if (first == g_SystemFontLeadByte) {
        if (second < g_SystemFontNarrowTrailByteCount) {
            return SYSTEM_GLYPH_WIDTH_NARROW;
        }
    }

    return SYSTEM_GLYPH_WIDTH_WIDE;
}

// Bytes one glyph takes in the font file's glyph table: 11 words of 8 pixels,
// one bit per pixel
#define SYSTEM_FONT_GLYPH_SIZE 0x16
// Rows of a glyph bitmap
#define SYSTEM_FONT_GLYPH_ROW_COUNT 11
// Bytes the double-byte glyph table spends on one lead byte: 0x100 trail bytes
// of 0x16 bytes each
#define SYSTEM_FONT_LEAD_BYTE_STRIDE 0x1600

// Two glyph sets share a texture page, so a 4bpp pixel holds one set in bits 0-1
// and the other in bits 2-3: a destination row is cleared with the other set's
// pair before this glyph is written into it
#define SYSTEM_FONT_PLANE0_CLEAR 0xCCCC
#define SYSTEM_FONT_PLANE1_CLEAR 0x3333

// Byte value whose doubled code names the dummy glyph
#define SYSTEM_FONT_DUMMY_CODE 0xFF

// Dummy glyph bitmap the renderer draws for the dummy code (0x16 bytes)
extern u16 g_SystemFontDummyGlyph[];

// One glyph set's copy of the row loop: SHIFT is the set's bit offset inside a
// 4bpp pixel of the destination row and CLEAR_PIXELS the other set's pair, which
// every word the glyph is drawn into is cleared with first.
#define SYSTEM_FONT_DRAW_GLYPH_SET(SHIFT, CLEAR_PIXELS)                              \
    pDest[0] &= CLEAR_PIXELS;                                                        \
    pDest[1] &= CLEAR_PIXELS;                                                        \
    pDest[2] &= CLEAR_PIXELS;                                                        \
    pDest += rowStride;                                                              \
    pDest[0] &= CLEAR_PIXELS;                                                        \
    pDest[1] &= CLEAR_PIXELS;                                                        \
    pDest[2] &= CLEAR_PIXELS;                                                        \
    for (i = 0; i < SYSTEM_FONT_GLYPH_ROW_COUNT; i++, pDest += rowStride) {           \
        pRow2 = pDest + 2;                                                           \
        (pDest + rowStride)[0] &= CLEAR_PIXELS;                                      \
        (pDest + rowStride)[1] &= CLEAR_PIXELS;                                      \
        (pDest + rowStride)[2] &= CLEAR_PIXELS;                                      \
        word = *pGlyph++;                                                            \
        mask = (word & 0x80) ? (0x222 << SHIFT) : 0;                                 \
        if (word & 0x40) mask |= (0x2220 << SHIFT);                                  \
        if (word & 0x20) mask |= (0x2200 << SHIFT);                                  \
        if (word & 0x10) mask |= (0x2000 << SHIFT);                                  \
        (pDest - rowStride)[0] |= mask;                                              \
        (pDest + rowStride)[0] |= mask;                                              \
        mask = (word & 0x80) ? (0x212 << SHIFT) : 0;                                 \
        if (word & 0x40) mask |= (0x2120 << SHIFT);                                  \
        if (word & 0x20) mask |= (0x1200 << SHIFT);                                  \
        if (word & 0x10) mask |= (0x2000 << SHIFT);                                  \
        pDest[0] |= mask;                                                            \
        mask = (word & 0x08) ? (0x222 << SHIFT)                                      \
                             : ((word & 0x10) ? (0x22 << SHIFT)                      \
                                              : ((word >> (4 - SHIFT)) & (2 << SHIFT))); \
        if (word & 0x04) mask |= (0x2220 << SHIFT);                                  \
        if (word & 0x02) mask |= (0x2200 << SHIFT);                                  \
        if (word & 0x01) mask |= (0x2000 << SHIFT);                                  \
        (pDest - rowStride)[1] |= mask;                                              \
        (pDest + rowStride)[1] |= mask;                                              \
        mask = (word >> (4 - SHIFT)) & (2 << SHIFT);                                 \
        if (word & 0x10) mask |= (0x21 << SHIFT);                                    \
        if (word & 0x08) mask |= (0x212 << SHIFT);                                   \
        if (word & 0x04) mask |= (0x2120 << SHIFT);                                  \
        if (word & 0x02) mask |= (0x1200 << SHIFT);                                  \
        if (word & 0x01) mask |= (0x2000 << SHIFT);                                  \
        pRow2[-1] |= mask;                                                            \
        mask = (word & 0x8000) ? (0x222 << SHIFT)                                    \
                               : ((word & 0x01) ? (0x22 << SHIFT)                    \
                                                : ((word << SHIFT) & (2 << SHIFT))); \
        if (word & 0x4000) mask |= (0x2220 << SHIFT);                                \
        if (word & 0x2000) mask |= (0x2200 << SHIFT);                                \
        if (word & 0x1000) mask |= (0x2000 << SHIFT);                                \
        (pDest - rowStride)[2] |= mask;                                              \
        (pDest + rowStride)[2] |= mask;                                              \
        mask = (word << SHIFT) & (2 << SHIFT);                                       \
        if (word & 0x01) mask |= (0x21 << SHIFT);                                    \
        if (word & 0x8000) mask |= (0x212 << SHIFT);                                 \
        if (word & 0x4000) mask |= (0x2120 << SHIFT);                                \
        if (word & 0x2000) mask |= (0x1200 << SHIFT);                                \
        if (word & 0x1000) mask |= (0x2000 << SHIFT);                                \
        pRow2[0] |= mask;                                                            \
    }

// Draw one glyph bitmap into a rendered row's pixel buffer, spreading each
// bitmap row over the row above and the row below it. pDest points one row into
// the buffer, rowStride is the buffer's row width in pixels and plane picks
// which of the two glyph sets of the page the pixels belong to.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034FFC);
/*
Parked at 66.80% (392 instructions / 1568 bytes against retail's 424 / 1696). Every
branch, load, store and mask constant is measured right; the whole residual is how
much redundant bit-testing gcc keeps.

- THE RESIDUAL IS CSE OF THE BIT TESTS. Retail recomputes `word & bit` at every use
  (its $v0/$v1 churn): 28 andi per iteration, and the sltu+negu bool twice per plane,
  once for the 0x222 column and again for the 0x212 one. This body keeps
  `word & 0x40`, `word & 0x20`, `word & 0x10` and the 0x80 bool in registers across
  all six columns. Whole-function op diff (objdump, retail first): andi 91/67 (-24),
  ori 53/44 (-9), or 24/18 (-6), sltu+negu 4/2 (-2), bnez 18/12 (-6), j 12/7 (-5),
  nop 16/0 (-16) - and the compiler's side has 6 more beqz, 6 addiu, 6 move, 9 li.
  The 16 missing nops and 5 missing j are the `bnez / ori C / j / or` select shape
  gcc uses when it folds the last `mask |= C` of a column into both arms of the
  branch; this body ORs in place instead.
- SHOULD FALL OUT OF THE GIV SHAPE. Retail carries FOUR pointers round the loop
  (a2 = pDest, t1 = pDest + rowStride, t2 = pDest - rowStride, t3 = pDest + 2) each
  with `addu p,p,t4` (t4 = 2*rowStride, invariant), keeps no callee-saved register
  and has no frame; t3 covers both pDest[1] (-0x2) and pDest[2] (0x0), so it is the
  one derived giv retail has. Fewer live values in this body is exactly why gcc had
  room to hoist the tests.
- MEASURED THIS SESSION (all with the macro below, only the pointer spelling changed):
  * pDest as the loop-carried pointer, (pDest +/- rowStride)[k] inline, plus
    `pRow2 = pDest + 2` for the pDest[1]/pDest[2] stores where retail's t3 is:
    66.80%, 392 / 1568, no frame. Best; this is the body.
    (the same body without pRow2 is 66.79%, 391 / 1564, so the lead was worth +4 bytes)
  * pAbove / pRow / pBelow all loop-carried (`i++, pAbove += rowStride, ...` over
    pAbove = pDest, pRow = pDest + rowStride, pBelow = pRow + rowStride): 59.33%,
    411 / 1644 with an 8-byte frame and $16 saved. It DOES recompute more of the
    tests (391 -> 411 instructions) - the pressure theory holds - but gcc mints a
    +4 derived giv per base (three, where retail has one), needs a register for it
    and loses more than it gains.
  * (earlier round) pAbove/pBelow assigned from pDest at the top of the body:
    64.92%, 397 / 1588. Both pointers incremented in a second for-update: 60.46%,
    413 / 1652.
- PRESET IS RIGHT: 2.6.0 scores 64.95 and 2.7.2-cdk 62.19 on this body; Default
  (2.7.2 -O2 -G8) is the compile above.
- WHAT TO BREAK: cross-block CSE of `and:SI(reg,const)`. -O2's cse follows the
  ladder's conditional jumps, and nothing tried so far stops it: the tests are the
  same tokens in each column, and copies of `word` (a second local, a different
  constant spelling) either propagate away or change the instruction. Either make
  each column's tests non-identical RTL, or raise live values to retail's twelve
  without minting givs or a frame.
// The dummy-glyph test is the byte 0xFF against firstByte and secondByte, not the
// end-of-text code 0xFFFF (retail compares 0xFF).
void func_80034FFC(u16 firstByte, u16 secondByte, u16* pDest, s16 rowStride, int plane) {
    u16* pGlyph;
    u16* pRow2;
    u16 word;
    int mask;
    int i;

    if (firstByte == 0) {
        pGlyph = (u16*)((u8*)g_SystemFontGlyphData +
                        (secondByte - g_SystemFontSingleByteCodeBase) * SYSTEM_FONT_GLYPH_SIZE);
    } else if (firstByte == SYSTEM_FONT_DUMMY_CODE && secondByte == SYSTEM_FONT_DUMMY_CODE) {
        pGlyph = g_SystemFontDummyGlyph;
    } else {
        pGlyph = (u16*)((u8*)g_SystemFontGlyphData + secondByte * SYSTEM_FONT_GLYPH_SIZE +
                        g_SystemFontDoubleByteGlyphOffset +
                        (firstByte - g_SystemFontLeadByte) * SYSTEM_FONT_LEAD_BYTE_STRIDE);
    }

    if (plane == 0) {
        SYSTEM_FONT_DRAW_GLYPH_SET(0, SYSTEM_FONT_PLANE0_CLEAR)
    } else {
        SYSTEM_FONT_DRAW_GLYPH_SET(2, SYSTEM_FONT_PLANE1_CLEAR)
    }
}
*/
