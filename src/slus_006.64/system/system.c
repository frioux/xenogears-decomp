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
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ undefined8 unk8[4];
    /* 0x0C */ s16 maxChars;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ u16 flags;
    /* 0x12 */ undefined8 unk12[2];
    /* 0x14 */ s16 unk14;
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
    /* 0x6E */ u8 unk6E;
    /* 0x6F */ undefined8 unk6F;
    /* 0x70 */ u32 unk70;
    /* 0x74 */ undefined8 unk74[0xE];
    /* 0x82 */ s16 unk82;
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


void SystemInitializeFont(void* pFontFile) {
    int dataOffset;

    if (pFontFile == 0) {
        HeapSetCurrentContentType(HEAP_CONTENT_NONE);
        return;
    }

    HeapPinBlock(pFontFile);
    g_SystemFontFile = pFontFile;
    D_8005935C = pFontFile;
    dataOffset = *(u16*)((u8*)pFontFile + 0x2);
    D_8005934C = *(u16*)((u8*)pFontFile + 0x4);
    D_80059350 = *(u16*)((u8*)pFontFile + 0x6);
    D_80059354 = *(u16*)((u8*)pFontFile + 0x8);
    D_80059358 = *(u16*)((u8*)pFontFile + 0xA);
    D_80059364 = *(u16*)((u8*)pFontFile + 0xC);
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

void* func_80033784(int groupIndex, int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[groupIndex], entryId);
}

void* func_800337B8(int entryId) {
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

void* func_80033878(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x18], entryId);
}

void* func_800338A8(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x19], entryId);
}

void* func_800338D8(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x12], entryId);
}

void* func_80033908(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x14], entryId);
}

void* func_80033938(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x13], entryId);
}

void* func_80033968(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x15], entryId);
}

void* func_80033998(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x1B], entryId);
}

void* func_800339C8(int groupIndex, int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x1C + groupIndex], entryId);
}

void* func_800339FC(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x30], entryId);
}

void* func_80033A2C(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x32], entryId);
}

// TODO: determine what strings are in bank 51 and give this function a better name
void *func_80033A5C(s32 entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[51], entryId);
}

void* func_80033A8C(int entryId) {
    return GetStringEntry(g_SystemDataEntries->entries[0x34], entryId);
}

void func_80033ABC(u16* pInput) {
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

void func_80033B34(u16* pInput, u8* pOutput, int count) {
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

int func_80033BAC(u8 firstByte, u8 secondByte) {
    s16 i;
    u8* pGlyph = g_SystemDataEntries->entries[0x1B];

    for (i = 0; i < 0x144; i++) {
        if (pGlyph[i * 2] == firstByte && pGlyph[i * 2 + 1] == secondByte) {
            return i;
        }
    }

    return 0x8000;
}

int func_80033C20(u8* pInput, u16* pOutput) {
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
        glyph = func_80033BAC(arg0, arg1);
        *pOutput++ = glyph;

        if ((u16)glyph == 0x8000) {
            return -1;
        }

        c = *pInput++;
    }

    return 0;
}

int func_80033CD0(SystemString* pString) {
    if (pString->flags & 0x8) {
        return pString->unk6B;
    }

    return 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80033CF0);

/* Best attempt, 92.98%. Every instruction is right except two entry-block
   ordering details:
   - gcc emits the `move v1,a0` (copy of value out of a0, which pDigit reuses)
     two insns earlier than the original does.
   - `pScanEnd = pScan + 10` is folded to `addiu v1,v0,-2` (off pEnd) and
     hoisted above the `andi`/`bne`; the original keeps a runtime `addiu v1,a0,20`
     after the branch. Assigning pScanEnd inside the if fixes that insn but then
     gcc puts the D_8005A0DE address in v1 instead of v0, and reorg steals
     `sh a1,-22(v0)` into the branch delay slot (the taken path writes v0), for
     92.96%.
   The scan loop itself only matches in this shape: while/break, while with &&
   and do/while are all rotated by loop.c; two gotos to a label after the block
   stop the rotation.
void func_80033CF0(int value, int glyphOffset, int flag) {
    u32 divisor = 1000000000;
    u16* pDigit = D_8005A0CA;
    u16* pEnd;
    u16* pScan;
    u16* pScanEnd;
    int i;
    int sign = flag;

    glyphOffset *= 16;

    if (sign != 0) {
        if (value >= 0) {
            sign = 11;
        } else {
            value = -value;
            sign = 10;
        }
    }

    for (i = 0; i < 10; i++) {
        pDigit[i] = value / divisor + glyphOffset;
        value %= divisor;
        divisor /= 10;
    }

    pEnd = D_8005A0DE;
    pScan = pEnd - 11;
    pEnd[0] = 0xFFFF;
    pEnd[-11] = glyphOffset;

    pScanEnd = pScan + 10;

    if ((glyphOffset & 0xFFF0) == glyphOffset) {
        for (;;) {
            if (pScan == pScanEnd) {
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

    func_80033ABC(pScan);
}
*/

void func_80033DD4(SystemString* pString, void* pData) {
    void* pOld = pString->pCur;

    pString->pCur = pData;
    pString->pPrev = pOld;
    pString->flags |= 0x80;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80033DF0);

void func_800345E0(SystemString* pString) {
    pString->flags &= ~0x8;

    if (pString->flags & 0x200) {
        pString->unk84 = 0;
        pString->unk6C = 0;
        pString->flags &= ~0x200;
    }
}

void func_80034614(SystemString* pString) {
    if (pString->unk84 == 0) {
        pString->unk6C = 0;
        pString->flags &= 0x2;
    }
}

void func_8003463C(SystemString* pString) {
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
    pString->unk82 = 0;
}

void func_800346A4(SystemString* pString) {
    pString->unk6C = 0;
    pString->unk84 = 0;
    pString->flags &= 0x2;
    func_8003463C(pString);
}

void func_800346D4(SystemString* pString) {
    func_800346A4(pString);
    HeapFree(pString->pChars);
    HeapFree(pString->pCharBuffer);
}

int func_80034714(SystemString* pString, void* pData) {
    SystemStringNode* pCur = pString->pBufferStack;
    SystemStringNode* pNode;

    pString->unk82++;
    HeapSetCurrentContentType(HEAP_CONTENT_MES_CUE);
    pNode = HeapAlloc(0x8, 0x2);
    pNode->pData = pData;
    pNode->pNext = NULL;

    if (pCur == NULL) {
        pString->pBufferStack = pNode;
        return pString->unk82;
    }

    while (pCur->pNext != NULL) {
        pCur = pCur->pNext;
    }

    pCur->pNext = pNode;

    return pString->unk82;
}

int func_800347AC(SystemString* pString) {
    return pString->unk4 + pString->unk0 * 4;
}

int func_800347C0(SystemString* pString) {
    int row = pString->unk2 - pString->unk16;

    if (row < 0) {
        row = pString->maxChars - 1;
    }

    return pString->unk6 + row * pString->unk14;
}

void func_80034800(SystemString* pString, u8 red, u8 green, u8 blue) {
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

void func_80034874(SystemString* pString, u8 value) {
    pString->unk6E = value;
}

void func_8003487C(SystemString* pString) {
    pString->unk6E = 0xFF;
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

int func_80034F98(u16 first, u16 second) {
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
