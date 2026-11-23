#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/font.h"

// SystemString.flags bits
#define SYSTEM_STRING_FLAG_PERSISTENT 0x2    // the only flag a reset keeps
#define SYSTEM_STRING_FLAG_STOPPED 0x8       // rendering stopped, stopCode says why
#define SYSTEM_STRING_FLAG_DATA_OVERRIDE 0x80 // pCur is an override of pPrev
#define SYSTEM_STRING_FLAG_WAITING 0x200     // a wait opcode is running

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
    u32 divisor = 1000000000;
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
#define SYSTEM_STRING_FLAG_BUFFER_POP 0x4   // a buffer pop has happened; skip the next
#define SYSTEM_STRING_FLAG_WRAPPED 0x20     // the row wrapped: relayout from the top
#define SYSTEM_STRING_FLAG_UPLOAD_HOLD 0x40 // do not upload the rows to VRAM yet
#define SYSTEM_STRING_FLAG_PALETTE_DOUBLE 0x100 // the palette holds two codes per glyph
#define SYSTEM_STRING_FLAG_UPLOAD_WAIT 0x10 // an upload is in flight a frame longer

// AddPrim inlined: the clear tile goes into the ordering table through the
// two-word link below, the row sprites through func_80031798 and the draw mode
// through the AddPrim function itself.
#define SystemLinkPrim(pOt, pPrim)                                                             \
    (*(u_long*)(pPrim) = (*(u_long*)(pPrim) & 0xFF000000) | (*(u_long*)(pOt) & 0xFFFFFF),       \
     *(u_long*)(pOt) = (*(u_long*)(pOt) & 0xFF000000) | ((u_long)(pPrim) & 0xFFFFFF))

extern void func_80031798(void* pOt, void* pPrim);


// Parked at 96.83% (1564 bytes against retail's 1572: TWO instructions short).
// Everything below is measured, not guessed.
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
//   ordering elsewhere), a volatile u8 store of unk69 (no change).
// - THE REGISTER MAP IS ONE SWAP FROM RETAIL'S: s1 = row*0x60 (the derived giv)
//   and s2 = context*0x28 match, but retail has s3 = row, s4 = i where this body
//   has s3 = i, s4 = row (before it: s2 = i, s3 = context*0x28, s4 = row).
//   Declaration order of i/row does NOT move it (both orders byte-identical) and
//   neither does scoping each sprite loop's row/i in its own block. The row-reset
//   loop's own counter (local j) DOES move: sharing i across all three loops
//   gives 96.72 with that counter in s2, giving it its own j gives 96.83 with it
//   in v1/a0 (retail has s3).
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
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034888);

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
        int j;

        pString->scrollRow = 0;
        pString->rowCount = 0;
        pString->charIndex = 0;
        pString->lineWidth = 0;
        pString->pChars[0].textureV = pString->textureY;
        pString->pChars[0].clut = g_SystemPalette1;
        pString->pChars[0].paletteIndex = 0;
        pString->pChars[0].imageRect.y = pString->textureY;
        for (j = 0; j < pString->maxChars; j++) {
            pString->pChars[j].rowWidth = 0;
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
}
*/

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

// Parked at 58.11% (421 instructions against retail's 424, 1684 bytes against
// 1696). The whole structure below is measured.
// - PRESET IS RIGHT: 2.6.0 scores 56.87 and 2.7.2-cdk 50.35 on this body.
// - THE PROLOGUE IS INSTRUCTION-IDENTICAL except for the two-instruction frame
//   this body needs. Retail uses NO callee-saved register and has no frame:
//   pGlyph = a0, pDest = a2, stride*2 = t4, word = t0, pAhead = t1,
//   pBehind = t2, the third word's pointer = t3, i = t5. Here pGlyph lands in
//   s0 and i in s1 because the loop optimizer mints extra derived givs: it
//   replaces the pAhead/pBehind +2 and +4 word accesses with their own
//   induction variables (five addus per iteration), while retail keeps
//   immediates off t1/t2 and has exactly ONE derived giv, t3 = pDest + 2
//   (used as -2(t3) and 0(t3)). That extra pressure is the frame.
//   Measured non-fixes: a declared pMid = pDest + 2 pointer used for the third
//   word (56.32%, 427 instructions), which adds a fifth giv instead of removing
//   one; moving pDest += rowStride into the for-increment expression (no
//   change at all: 58.11%, same byte count).
// - THE 0xFF TEST IS A BYTE: retail compares firstByte with 0xFF, not with the
//   end-of-text code 0xFFFF (SYSTEM_FONT_DUMMY_CODE).
// - rowStride IS SIGN-EXTENDED ONCE AND DOUBLED TWICE in retail
//   (sll v1,a3,16 / sra v1,v1,16 / sll v0,v1,1, then a second sll v1,v1,1 for
//   pBehind), so the source scales it in two places separately; this body folds
//   both into one sra 15 and one value in t5.
// - THE TWO PLANE BODIES ARE COPIES: everything in the second is the first with
//   its masks shifted two bits (0x222 -> 0x888, 0x212 -> 0x848, 0x21 -> 0x84,
//   (word >> 4) & 2 -> (word >> 2) & 8, word & 2 -> (word << 2) & 8), which is
//   what a macro taking the shift would fold to.
// - The last write of each group goes through the third word's pointer, i.e.
//   the source addresses pDest[1] and pDest[2] (no other spelling was tried
//   before the stop rule fired).
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034FFC);

/*
// Draw one glyph bitmap into a rendered row's pixel buffer, spreading each
// bitmap row over the row above and the row below it. pDest points one row into
// the buffer, rowStride is the buffer's row width in pixels and plane picks
// which of the two glyph sets of the page the pixels belong to.
void func_80034FFC(u16 firstByte, u16 secondByte, u16* pDest, s16 rowStride, int plane) {
    u16* pGlyph;
    u16* pAhead;
    u16* pBehind;
    u16 word;
    u16 mask;
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
        pDest[0] &= SYSTEM_FONT_PLANE0_CLEAR;
        pDest[1] &= SYSTEM_FONT_PLANE0_CLEAR;
        pDest[2] &= SYSTEM_FONT_PLANE0_CLEAR;

        pDest += rowStride;
        pAhead = pDest + rowStride;
        pBehind = pDest - rowStride;

        pDest[0] &= SYSTEM_FONT_PLANE0_CLEAR;
        pDest[1] &= SYSTEM_FONT_PLANE0_CLEAR;
        pDest[2] &= SYSTEM_FONT_PLANE0_CLEAR;

        for (i = 0; i < SYSTEM_FONT_GLYPH_ROW_COUNT; i++, pDest += rowStride) {
            pAhead[0] &= SYSTEM_FONT_PLANE0_CLEAR;
            pAhead[1] &= SYSTEM_FONT_PLANE0_CLEAR;
            pAhead[2] &= SYSTEM_FONT_PLANE0_CLEAR;

            word = *pGlyph++;

            mask = (word & 0x80) ? 0x222 : 0;
            if (word & 0x40) mask |= 0x2220;
            if (word & 0x20) mask |= 0x2200;
            if (word & 0x10) mask |= 0x2000;
            pBehind[0] |= mask;
            pAhead[0] |= mask;

            mask = (word & 0x80) ? 0x212 : 0;
            if (word & 0x40) mask |= 0x2120;
            if (word & 0x20) mask |= 0x1200;
            if (word & 0x10) mask |= 0x2000;
            pDest[0] |= mask;

            mask = (word & 0x08) ? 0x222 : ((word & 0x10) ? 0x22 : ((word >> 4) & 2));
            if (word & 0x04) mask |= 0x2220;
            if (word & 0x02) mask |= 0x2200;
            if (word & 0x01) mask |= 0x2000;
            pBehind[1] |= mask;
            pAhead[1] |= mask;

            mask = (word >> 4) & 2;
            if (word & 0x10) mask |= 0x21;
            if (word & 0x08) mask |= 0x212;
            if (word & 0x04) mask |= 0x2120;
            if (word & 0x02) mask |= 0x1200;
            if (word & 0x01) mask |= 0x2000;
            pDest[1] |= mask;

            mask = (word & 0x8000) ? 0x222 : ((word & 0x01) ? 0x22 : (word & 0x02));
            if (word & 0x4000) mask |= 0x2220;
            if (word & 0x2000) mask |= 0x2200;
            if (word & 0x1000) mask |= 0x2000;
            pBehind[2] |= mask;
            pAhead[2] |= mask;

            mask = word & 0x02;
            if (word & 0x01) mask |= 0x21;
            if (word & 0x8000) mask |= 0x212;
            if (word & 0x4000) mask |= 0x2120;
            if (word & 0x2000) mask |= 0x1200;
            if (word & 0x1000) mask |= 0x2000;
            pDest[2] |= mask;

            pAhead += rowStride;
            pBehind += rowStride;
        }
    } else {
        // The second glyph set of the page: the same rows, two bits higher
        pDest[0] &= SYSTEM_FONT_PLANE1_CLEAR;
        pDest[1] &= SYSTEM_FONT_PLANE1_CLEAR;
        pDest[2] &= SYSTEM_FONT_PLANE1_CLEAR;

        pDest += rowStride;
        pAhead = pDest + rowStride;
        pBehind = pDest - rowStride;

        pDest[0] &= SYSTEM_FONT_PLANE1_CLEAR;
        pDest[1] &= SYSTEM_FONT_PLANE1_CLEAR;
        pDest[2] &= SYSTEM_FONT_PLANE1_CLEAR;

        for (i = 0; i < SYSTEM_FONT_GLYPH_ROW_COUNT; i++, pDest += rowStride) {
            pAhead[0] &= SYSTEM_FONT_PLANE1_CLEAR;
            pAhead[1] &= SYSTEM_FONT_PLANE1_CLEAR;
            pAhead[2] &= SYSTEM_FONT_PLANE1_CLEAR;

            word = *pGlyph++;

            mask = (word & 0x80) ? 0x888 : 0;
            if (word & 0x40) mask |= 0x8880;
            if (word & 0x20) mask |= 0x8800;
            if (word & 0x10) mask |= 0x8000;
            pBehind[0] |= mask;
            pAhead[0] |= mask;

            mask = (word & 0x80) ? 0x848 : 0;
            if (word & 0x40) mask |= 0x8480;
            if (word & 0x20) mask |= 0x4800;
            if (word & 0x10) mask |= 0x8000;
            pDest[0] |= mask;

            mask = (word & 0x08) ? 0x888 : ((word & 0x10) ? 0x88 : ((word >> 2) & 8));
            if (word & 0x04) mask |= 0x8880;
            if (word & 0x02) mask |= 0x8800;
            if (word & 0x01) mask |= 0x8000;
            pBehind[1] |= mask;
            pAhead[1] |= mask;

            mask = (word >> 2) & 8;
            if (word & 0x10) mask |= 0x84;
            if (word & 0x08) mask |= 0x848;
            if (word & 0x04) mask |= 0x8480;
            if (word & 0x02) mask |= 0x4800;
            if (word & 0x01) mask |= 0x8000;
            pDest[1] |= mask;

            mask = (word & 0x8000) ? 0x888 : ((word & 0x01) ? 0x88 : ((word << 2) & 8));
            if (word & 0x4000) mask |= 0x8880;
            if (word & 0x2000) mask |= 0x8800;
            if (word & 0x1000) mask |= 0x8000;
            pBehind[2] |= mask;
            pAhead[2] |= mask;

            mask = (word << 2) & 8;
            if (word & 0x01) mask |= 0x84;
            if (word & 0x8000) mask |= 0x848;
            if (word & 0x4000) mask |= 0x8480;
            if (word & 0x2000) mask |= 0x4800;
            if (word & 0x1000) mask |= 0x8000;
            pDest[2] |= mask;

            pAhead += rowStride;
            pBehind += rowStride;
        }
    }
}
*/
