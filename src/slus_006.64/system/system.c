#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

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
extern void* D_8005935C;
extern int D_8005934C;
extern int D_80059350;
extern int D_80059354;
extern int D_80059358;
extern int D_80059364;

// Scratch buffer the number/glyph converters write to
extern u8 D_8005A0E4[];

// Digit buffer: digits at D_8005A0CA, terminator slot at D_8005A0DE
extern u16 D_8005A0CA[];
extern u16 D_8005A0DE[];


// A rendered string: glyph buffer, palette and the stack of data buffers that
// SystemRenderStringEntry walks.
typedef struct {
    /* 0x0 */ u8 red;
    /* 0x1 */ u8 green;
    /* 0x2 */ u8 blue;
    /* 0x3 */ undefined8 unk3;
    /* 0x4 */ undefined8 unk4[0x10];
} SystemStringLine; // Size: 0x14

typedef struct {
    /* 0x0  */ undefined8 unk0[0x4];
    /* 0x4  */ SystemStringLine lines[4];
    /* 0x54 */ undefined8 unk54[0x4];
    /* 0x58 */ s16 unk58;
    /* 0x5A */ undefined8 unk5A[2];
    /* 0x5C */ u8 unk5C;
    /* 0x5D */ undefined8 unk5D;
    /* 0x5E */ s16 unk5E;
} SystemStringChar; // Size: 0x60

typedef struct SystemStringNode {
    /* 0x0 */ struct SystemStringNode* pNext;
    /* 0x4 */ void* pData;
} SystemStringNode; // Size: 0x8

typedef struct {
    /* 0x00 */ s16 lineWidth;
    /* 0x02 */ s16 charIndex;
    /* 0x04 */ s16 x;
    /* 0x06 */ s16 y;
    /* 0x08 */ undefined8 unk8[4];
    /* 0x0C */ s16 maxChars;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ u16 flags;
    /* 0x12 */ undefined8 unk12[2];
    /* 0x14 */ s16 lineHeight;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ undefined8 unk1A[2];
    /* 0x1C */ void* pCur;
    /* 0x20 */ void* pPrev;
    /* 0x24 */ undefined8 unk24[4];
    /* 0x28 */ SystemStringChar* pChars;
    /* 0x2C */ void* pCharBuffer;
    /* 0x30 */ undefined8 unk30[0x38];
    /* 0x68 */ u8 unk68;
    /* 0x69 */ u8 unk69;
    /* 0x6A */ u8 unk6A;
    /* 0x6B */ u8 unk6B;
    /* 0x6C */ u8 unk6C;
    /* 0x6D */ undefined8 unk6D;
    /* 0x6E */ u8 lineOffset;
    /* 0x6F */ undefined8 unk6F;
    /* 0x70 */ u32 unk70;
    /* 0x74 */ undefined8 unk74[0xE];
    /* 0x82 */ s16 bufferCount;
    /* 0x84 */ s16 unk84;
    /* 0x86 */ s16 unk86;
    /* 0x88 */ s16 unk88;
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
    int dataOffset;

    if (pFontFile == NULL) {
        HeapSetCurrentContentType(HEAP_CONTENT_NONE);
        return;
    }

    HeapPinBlock(pFontFile);
    g_SystemFontFile = pFontFile;
    D_8005935C = pFontFile;
    // Read through a u16 cast rather than pFontFile->field: as field lvalues
    // gcc hoists all six loads above the two pointer stores, and retail
    // interleaves them (66.5% vs 100%).
    dataOffset = *(u16*)((u8*)pFontFile + offsetof(SystemFontFileHeader, glyphDataOffset));
    D_8005934C = *(u16*)((u8*)pFontFile + offsetof(SystemFontFileHeader, leadByte));
    D_80059350 = *(u16*)((u8*)pFontFile + offsetof(SystemFontFileHeader, doubleByteGlyphOffset));
    D_80059354 = *(u16*)((u8*)pFontFile + offsetof(SystemFontFileHeader, narrowGlyphCount));
    D_80059358 = *(u16*)((u8*)pFontFile + offsetof(SystemFontFileHeader, narrowTrailByteCount));
    D_80059364 = *(u16*)((u8*)pFontFile + offsetof(SystemFontFileHeader, singleByteCodeBase));
    D_8005935C = (u8*)pFontFile + dataOffset;
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
    setRECT(&dest, xDest, yDest, 32, 1);
    LoadImage(&dest, &g_SystemPaletteData);
    g_SystemPalette1 = GetClut(xDest, yDest);
    g_SystemPalette2 = GetClut(xDest + 16, yDest);
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
    return GetStringEntry(g_SystemDataEntries->entries[0x11], entryId);
}

void* GetItemName(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x16], entryId);
}

void* GetWeaponName(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x17], entryId);
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
    return GetStringEntry(g_SystemDataEntries->entries[0x1B], entryId);
}

void* GetStringEntryFromBank1C(int groupIndex, int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x1C + groupIndex], entryId);
}

void* GetStringEntryFromBank30(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x30], entryId);
}

void* GetGearAccessoryName(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x32], entryId);
}

// TODO: determine what strings are in bank 51 and give this function a better name
void *GetGearWeaponName(s32 entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[51], entryId);
}

void* GetStringEntryFromBank34(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x34], entryId);
}

void SystemDecodeGlyphCodes(u16* pInput) {
    u8* pEntry = g_SystemDataEntries->entries[0x1B];
    u8* pOutput = D_8005A0E4;
    u8* pPos;

    while (*pInput != 0xFFFF) {
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
    u8 *pEntry = g_SystemDataEntries->entries[0x1B];
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
    u8* pGlyph = g_SystemDataEntries->entries[0x1B];

    for (i = 0; i < 0x144; i++) {
        if (pGlyph[i * 2] == firstByte && pGlyph[i * 2 + 1] == secondByte) {
            return i;
        }
    }

    return 0x8000;
}

int SystemEncodeGlyphCodes(u8* pInput, u16* pOutput) {
    int c = *pInput++;
    int glyph;
    u8 arg0;
    u8 arg1;

    while (c != 0) {
        if (c < D_8005934C) {
            arg0 = 0;
            arg1 = c;
        } else {
            arg0 = c;
            arg1 = *pInput++;
        }
        glyph = SystemFindGlyphCode(arg0, arg1);
        *pOutput++ = glyph;

        if ((u16)glyph == 0x8000) {
            return -1;
        }

        c = *pInput++;
    }

    return 0;
}

int SystemStringGetUnk6B(SystemString* pString) {
    if (pString->flags & 0x8) {
        return pString->unk6B;
    }

    return 0;
}

void SystemFormatNumberGlyphs(int value, int glyphOffset, int flag) {
    u32 divisor = 1000000000;
    u16* pDigit = D_8005A0CA;
    u16* pEnd;
    u16* pScan;
    int i;
    int sign = flag;
    s32 v;

    v = value;
    glyphOffset *= 16;

    if (sign != 0) {
        if (v >= 0) {
            sign = 11;
        } else {
            v = -v;
            sign = 10;
        }
    }

    for (i = 0; i < 10; i++) {
        pDigit[i] = v / divisor + glyphOffset;
        v %= divisor;
        divisor /= 10;
    }

    pEnd = D_8005A0DE;
    pScan = pEnd - 11;
    pEnd[0] = 0xFFFF;
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
    pString->flags |= 0x80;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80033DF0);

void SystemStringClearStateFlags(SystemString* pString) {
    pString->flags &= ~0x8;

    if (pString->flags & 0x200) {
        pString->unk84 = 0;
        pString->unk6C = 0;
        pString->flags &= ~0x200;
    }
}

void SystemStringResetIfIdle(SystemString* pString) {
    if (pString->unk84 == 0) {
        pString->unk6C = 0;
        pString->flags &= 0x2;
    }
}

void SystemStringFreeBufferStack(SystemString* pString) {
    SystemStringNode* pNode;
    SystemStringNode* pOld;

    if (pString->unk84 != 0) {
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
    pString->unk6C = 0;
    pString->unk84 = 0;
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
    pNode = HeapAlloc(0x8, 0x2);
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
    int row = pString->charIndex - pString->unk16;

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

        pChar->lines[3].red = red;
        pChar->lines[2].red = red;
        pChar->lines[1].red = red;
        pChar->lines[0].red = red;
        pChar->lines[3].green = green;
        pChar->lines[2].green = green;
        pChar->lines[1].green = green;
        pChar->lines[0].green = green;
        pChar->lines[3].blue = blue;
        pChar->lines[2].blue = blue;
        pChar->lines[1].blue = blue;
        pChar->lines[0].blue = blue;
    }
}

void SystemStringSetLineOffset(SystemString* pString, u8 value) {
    pString->lineOffset = value;
}

void SystemStringClearLineOffset(SystemString* pString) {
    pString->lineOffset = 0xFF;
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
        if (second - D_80059364 < D_80059354) {
            return 2;
        }

        return 3;
    }

    if (first == D_8005934C) {
        if (second < D_80059358) {
            return 2;
        }
    }

    return 3;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034FFC);
