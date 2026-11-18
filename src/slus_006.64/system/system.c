#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

// SystemString.flags bits
#define SYSTEM_STRING_FLAG_STOPPED 0x8       // rendering stopped, stopCode says why
#define SYSTEM_STRING_FLAG_DATA_OVERRIDE 0x80 // pCur is an override of pPrev
#define SYSTEM_STRING_FLAG_WAITING 0x200     // a wait opcode is running

// lineOffset value that means no offset
#define SYSTEM_STRING_LINE_OFFSET_NONE 0xFF

// Glyph code table (system data bank 0x1B) and the codes it holds
#define SYSTEM_GLYPH_CODE_COUNT 324
#define SYSTEM_GLYPH_CODE_END 0xFFFF
#define SYSTEM_GLYPH_CODE_NOT_FOUND 0x8000

// Width, in VRAM units, one glyph advances the line
#define SYSTEM_GLYPH_WIDTH_NARROW 2
#define SYSTEM_GLYPH_WIDTH_WIDE 3

// The digits of a number glyph set, then the two codes after them
#define SYSTEM_NUMBER_GLYPH_DIGIT_COUNT 10
#define SYSTEM_NUMBER_GLYPH_MINUS 10
#define SYSTEM_NUMBER_GLYPH_PLUS 11

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

// One global text object (a SystemString at 0x80059FD8) and its char buffer;
// the fields are split out because each field address is its own symbol.
extern s16 D_80059FD8;
extern s16 D_80059FDA;
extern s16 D_80059FE0;
extern s16 D_80059FE2;
extern s16 D_80059FE4;
extern s16 D_80059FE8;
extern s16 D_80059FEA;
extern void* D_80059FF4;
extern void* D_8005A000;
extern void* D_8005A004;
extern u8 D_8005A040;
extern u8 D_8005A041;
extern u8 D_8005A042;
extern u8 D_8005A044;
extern s16 D_8005A05C;
extern u8 D_8005A068[];
extern s16 D_8005A0C0;
extern u8 D_8005A0C2;

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


// One row of a rendered string. func_80032F54 sets the sprites up as free-size
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

typedef struct SystemStringNode {
    /* 0x0 */ struct SystemStringNode* pNext;
    /* 0x4 */ void* pData;
} SystemStringNode; // Size: 0x8

// A rendered string: glyph buffer, palette and the stack of data buffers that
// SystemRenderStringEntry walks.
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
    /* 0x48 */ TILE clearTile[2];
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80032F54);

unsigned int ResolveArchiveEntryPointers(u32* header) {
    int i;
    u32 base_address = (u32)header;
    u32* pointers = header;

    for (i = 1; i <= *header; i++) {
        pointers[i] += base_address;
    }

    return *header;
}

// Same as ResolveArchiveEntryPointers, but no return value
void ResolveFileEntryPointers(u32* header) {
    int i;
    u32 base_address = (u32)header;
    u32* pointers = header;

    for (i = 1; i <= *header; i++) {
        pointers[i] += base_address;
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

void* GetStringEntry(void* stringData, u32 entry) {
    return stringData + ((u16 *)stringData)[entry+2];
}

// TODO: Cleanup code
// Dialog data format:
// 0x0: Offset to dimension data
// 0x2: ?
// 0x4: ?
// 0x6: u16 dimensions[dialog_count]
u8 DialogGetWidth(u16* pDialogData, int dialogIndex) {
    u8* pDialog = (pDialogData + 3) + *pDialogData + dialogIndex;
    return pDialog[0];
}

u8 DialogGetHeight(u16* pDialogData, int dialogIndex) {
    u8* pDialog = (pDialogData + 3) + *pDialogData + dialogIndex;
    return pDialog[1];
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
    u8* pEntry = g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GLYPH];
    u8* pOutput = g_SystemGlyphDecodeBuffer;
    u8* pPos;

    while (*pInput != SYSTEM_GLYPH_CODE_END) {
        pPos = (u8*)((*pInput) * 2) + (int)pEntry;
        pInput++;

        if (pPos[0]) {
            *pOutput++ = pPos[0];
            *pOutput++ = pPos[1];
        } else {
            *pOutput++ = pPos[1];
        }
    }

    *pOutput = 0;
}

void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count) {
    int i;
    u8 *pEntry = g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GLYPH];
    u8 *pPos;

    for(i = count-1; i != -1; i--) {
        // I don't like this, but it solves a finnicky register swap
        pPos = (u8 *) ((pInput[0] * 2) + (int)pEntry);
        pInput++;

        if(pPos[0]) {
            *pOutput++ = pPos[0];
            *pOutput++ = pPos[1];

        } else {
            *pOutput++ = pPos[1];
        }
    }

    *pOutput = 0;
}

int SystemFindGlyphCode(u8 firstByte, u8 secondByte) {
    s16 i;
    u8* pGlyph = g_SystemDataEntries->entries[SYSTEM_STRING_BANK_GLYPH];

    for (i = 0; i < SYSTEM_GLYPH_CODE_COUNT; i++) {
        if (pGlyph[i * 2] == firstByte && pGlyph[i * 2 + 1] == secondByte) {
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
    glyphOffset *= 16;

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
            if (pScan == pEnd + 10) {
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

// Render string entry to a buffer
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", SystemRenderStringEntry);

/* Best attempt, 28.31%: every instruction and data reference is right and the
   statement order below is exactly the order of the original stores, but gcc
   emits the 8 constant-touching stores (D_80059FE0, D_8005A040, D_8005A041,
   D_8005A068, andi a3) about ten instructions earlier than the original, and
   reuses one li v0,1 where the original loads the constant twice.
int SystemRenderStringEntry(void* pCur, void* pCharBuffer, int mode, int flag) {
    D_80059FE2 = mode;
    D_80059FE4 = 1;
    D_80059FE2 = mode | 1;
    D_80059FE0 = (s16)(mode | 1) * 4;
    D_80059FF4 = pCur;
    D_8005A040 = 1;
    D_80059FEA = (mode | 1) + 3;
    D_8005A05C = 0;
    D_8005A044 = 0;
    D_8005A042 = 0;
    D_8005A004 = pCharBuffer;
    D_80059FE8 = 0;
    D_80059FDA = 0;
    D_80059FD8 = 0;
    D_8005A041 = 0x64;
    D_8005A000 = D_8005A068;
    D_8005A0C0 = 0;
    D_8005A0C2 = flag & 1;

    func_80033DF0(&D_80059FD8);

    return *(s16*)((u8*)D_8005A000 + 0x58) * 4;
}
*/

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
