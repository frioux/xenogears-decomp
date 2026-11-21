#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/font.h"

// SystemString.flags bits
#define SYSTEM_STRING_FLAG_STOPPED 0x8       // rendering stopped, stopCode says why
#define SYSTEM_STRING_FLAG_DATA_OVERRIDE 0x80 // pCur is an override of pPrev
#define SYSTEM_STRING_FLAG_WAITING 0x200     // a wait opcode is running

// lineOffset value that means no offset
#define SYSTEM_STRING_LINE_OFFSET_NONE 0xFF

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

// System data banks (g_SystemDataEntries->entries) the named getters index

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

// Digit buffer: digits at g_SystemNumberGlyphBuffer, terminator slot at D_8005A0DE
extern u16 g_SystemNumberGlyphBuffer[];
extern u16 D_8005A0DE[];


// One row of a rendered string. SystemStringInit sets the sprites up as free-size
// textured rects (code 0x64) and the rect up for LoadImage, which uploads the
// row's glyphs to it; func_80034888 draws one sprite per render context.
typedef struct {
    /* 0x00 */ SPRT sprt[2][2];  // [render context][first/second half: split at 0x100 VRAM units]
    /* 0x50 */ RECT imageRect;   // VRAM rect the row's glyphs are uploaded to
    /* 0x58 */ s16 x;
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
extern SystemString D_80059FD8;
extern SystemStringChar D_8005A068;
// D_80059FD8.pChars, named separately because the typed store comes out 0x28(a0)
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
    pString->lineHeight = 14;
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
    pString->pChars = HeapAlloc(pString->maxChars * sizeof(SystemStringChar), 0x2);
    HeapSetCurrentContentType(HEAP_CONTENT_MES_IMAGE);
    pString->pCharBuffer = HeapAlloc(pString->imageWidth * 0x1C, 0x2);

    setlen(&pString->clearTile[0], 3);
    pString->clearTile[0].colorCode = 0x60 << 24; // r,g,b = 0, code = 0x60
    setXY0Fast(&pString->clearTile[0], pString->x - 7, pString->y - 5);
    setWHFast(&pString->clearTile[0], pString->maxWidth * 4 + 0xD, pString->maxChars * pString->lineHeight + 0xA);
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
        pString->pChars[i].x = 0;
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
    u32 base = (u32)pTable;
    for (i = 1; i <= pTable->count; i++) {
        pTable->offsets[i - 1] += base;
    }

    return pTable->count;
}

// Same as ResolveArchiveEntryPointers, but no return value
void ResolveFileEntryPointers(SystemOffsetTable* pTable) {
    u32 i;
    u32 base = (u32)pTable;

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

// TODO: determine what strings are in bank 51 and give this function a better name
void *GetGearWeaponName(s32 entryId) {
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
    u8 arg0;
    u8 arg1;

    while (c != 0) {
        if (c < g_SystemFontLeadByte) {
            arg0 = 0;
            arg1 = c;
        } else {
            arg0 = c;
            arg1 = *pInput++;
        }
        glyph = SystemFindGlyphCode(arg0, arg1);
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

    pEnd = D_8005A0DE;
    pScan = pEnd - 11;
    pEnd[0] = SYSTEM_GLYPH_CODE_END;
    pEnd[-11] = glyphOffset;

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
        pString->flags &= 0x2;
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
    pString->flags &= 0x2;
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
    pNode = HeapAlloc(sizeof(SystemStringNode), 0x2);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034888);

// Primitive code of the free-size textured rects a rendered row's sprites are
#define SYSTEM_STRING_RECT_CODE 0x64

// Render string entry to a buffer
int SystemRenderStringEntry(void* pCur, void* pCharBuffer, int mode, int flag) {
    SystemString* pString = &D_80059FD8;
    int primitiveCode;

    D_80059FD8.maxWidth = mode;
    D_80059FD8.maxChars = 1;
    D_80059FD8.maxWidth = mode | 1;
    D_80059FD8.spriteWidth = (s16)(mode | 1) * 4;
    primitiveCode = SYSTEM_STRING_RECT_CODE;
    D_80059FD8.pCur = pCur;
    D_80059FD8.imageWidth = (mode | 1) + 3;
    D_80059FD8.unk68 = 1;
    do {
        D_80059FD8.waitFrames = 0;
        D_80059FD8.waiting = 0;
        D_80059FD8.unk6A = 0;
        D_80059FD8.pCharBuffer = pCharBuffer;
        D_80059FD8.flags = 0;
        D_80059FD8.charIndex = 0;
        D_80059FD8.lineWidth = 0;
        D_80059FD8.unk69 = primitiveCode;
    } while (0);
    D_8005A000 = &D_8005A068;
    D_8005A068.x = 0;
    D_8005A068.paletteIndex = flag & 1;

    // The do/while block above and the two hoisted locals are load-bearing:
    // without them gcc hoists the constant stores above the group (28.31%), and
    // pString is what puts the group's lui/addiu for $a0 at its head.
    func_80033DF0(pString);

    return D_80059FD8.pChars->x * 4;
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034FFC);
