#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"

#define ARCHIVE_HEADER_ENTRY_SIZE 0x7

extern void func_80039A80(void* pManager, s32 volume, s32 frames);
extern s32 func_8003BDFC(u32 flags);
extern void* func_80039850(void* pFile);
extern char* ArchiveReadHddFile(char* pFilePath, int* pFileSize);
extern void SoundLoadWdsFile(void* pWdsFile, void* pUnk);
extern void HeapFree(void* pMem);
extern s32 ArchiveDecodeSizeAbsolute(int entryIndex);
extern u32 g_ArchiveTable;
extern int ControllerGetButtonState(int controllerIndex);
extern void SoundSetCdVolumeWithFade(s32 volume, s32 frames);

// .rodata
extern char D_80070304[];
extern char D_80070328[];
extern char D_8007034C[];
extern char D_80070370[];
extern char D_80070394[];

// .bss
extern void* D_8007700C;
extern MATRIX D_80077050;
extern u32 D_80077010;
extern s32 D_80077014;
extern s32 D_80077018;
extern s32 D_8007701C;
extern s32 D_80077024;
extern u32 D_80077028;
extern s32 D_80076F04;
extern s32 D_80076F08;
extern s32 D_80076F2C;
extern s32 D_80076F30;
extern s32 D_80076F34;
extern s32 D_8007702C;
extern s32 D_80077030;
extern s32 D_80077034;
extern s32 D_8007703C;
extern s32 D_80077040;
extern s32 D_80077044;
extern s32 D_8007704C;
extern s16 D_800770B0;
extern s16 D_800770B2;
extern s16 D_80077070;
extern s16 D_80077072;
extern s16 D_80077074;
extern s16 D_80077076;
extern s16 D_80077078;
extern s16 D_8007707A;
extern s16 D_8007707C;
extern s16 D_8007707E;
extern s16 D_80077080;
extern s16 D_80077090;
extern s16 D_80077092;
extern s16 D_80077094;
extern s16 D_80077096;
extern s16 D_80077098;
extern s16 D_8007709A;
extern s16 D_8007709C;
extern s16 D_8007709E;
extern s16 D_800770A0;
extern s16 D_800770B8;
extern s16 D_800770BA;
extern s16 D_800770BC;
extern s16 D_800770BE;
extern s16 D_800770C0;
extern s16 D_800770C2;
extern s16 D_800770C4;
extern s16 D_800770C6;
extern s16 D_800770C8;
extern s32 D_800770CC;
extern s32 D_800770D0;
extern s32 D_800770D4;
extern s32 D_800770D8[];
extern s32 D_800770EC;
extern s32 D_800770F0;
extern s32 D_800770F4;
extern s16 D_800770F8;
extern s16 D_800770FA;
extern s16 D_800770FC;
extern s16 D_800770FE;
extern s16 D_80077100;
extern s16 D_80077102;
extern s16 D_80077104;
extern s16 D_80077106;
extern s16 D_80077108;
extern s32 D_8007711C;
extern u8 D_8007713C;
extern u8 D_80077191;
extern u8 D_80077274;
extern u8 D_800772C9;
extern s32 D_8007739C;
extern s32 D_800773AC;
extern s32 D_800773B4;
extern s32 D_80077438;
extern s32 D_80077448;
extern s32 D_801D68B4;

void func_80074B58(void) {
    RECT rect;

    rect.w = 0x280;
    rect.x = 0;
    rect.y = 0;
    rect.h = 0x1E0;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80074BA4);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_8007519C);

void func_800753B8(void) {
    char* pWdsFile;

    pWdsFile = ArchiveReadHddFile(D_80070304, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((func_8003BDFC(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);

    pWdsFile = ArchiveReadHddFile(D_80070328, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((func_8003BDFC(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);

    pWdsFile = ArchiveReadHddFile(D_8007034C, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((func_8003BDFC(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);
}

void func_8007548C(void) {
    char* pWdsFile;

    pWdsFile = ArchiveReadHddFile(D_80070370, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    // func_8003BDFC returns a 16-bit status, tested in the low half
    while ((func_8003BDFC(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);
    D_8007700C = func_80039850(ArchiveReadHddFile(D_80070394, 0));
}

void func_80075508(void) {
    func_80039A80(D_8007700C, 0x7F, 0);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80075534);

int func_80075D4C(u32 entryIndex) {
    u8* pArchiveEntry;
    u32 nOffset;
    int nFileSize;

    nOffset = entryIndex * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (pArchiveEntry[6] << 0x18) + (pArchiveEntry[5] << 0x10) + (pArchiveEntry[4] << 0x8) + pArchiveEntry[3];
    return nFileSize;
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80075D8C);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_8007625C);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_800763BC);

/* best 82.33% - frame needs `long pad[6]`; only the form of the D_80077448==1
   test differs (original branches on != 1 and falls into the SetIndex block).
   The state>2 path repeats the reset block and gcc cross-jumps it, which
   reproduces the original's shared tail but moves the ==2 test before the
   size check. Shapes tried: else-if chain + goto, nested else, either branch
   polarity on ==2, duplicated reset block.
void func_800763BC(u8 mode) {
    long pad[6];
    s32 nIndex;

    D_80077028 = mode;
    D_800773AC = -1;
    if (D_80077448 == 0) {
        ArchiveSetIndex(0x18, 0);
        nIndex = 2;
    } else if (D_80077448 == 1) {
        ArchiveSetIndex(0x18, 1);
        nIndex = 1;
    } else if (D_80077448 != 2) {
        goto reset;
    } else {
        return;
    }
    if (D_8007711C >= (s16)ArchiveDecodeSizeAbsolute(nIndex)) {
        return;
    }
reset:
    D_8007713C = 0;
    D_80077274 = 0;
    D_80077191 = 1;
    D_800772C9 = 1;
    func_80076488();
}
*/

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076488);

void func_800768D8(u16 arg0, s32 arg1, u16 arg2) {
    D_80077010 = arg0;
    if (D_801D68B4 == 1) {
        if (arg2 != D_80077024) {
            D_80077018 = 0;
        } else {
            D_80077018 = 1;
        }
    } else if (arg2 != D_80077024) {
        D_80077018 = 0;
        D_8007701C = 0;
    } else {
        D_80077018 = 1;
        D_8007701C = 1;
    }
    if (arg0 < D_8007739C) {
        return;
    }
    if (D_80077438 != 0) {
        return;
    }
    D_80077014 = 1;
}

void func_800769A4(void) {
    D_800773B4 = D_800773AC;
    D_800773AC = ControllerGetButtonState(0);
    if (D_800773B4 == D_800773AC) {
        if (D_800773B4 != 0) {
            D_80076F04++;
            if (D_80076F08 < D_80076F04) {
                D_800773B4 = 0;
                D_80076F08 = 1;
                D_80076F04 = 0;
            }
        } else {
            D_80076F08 = 0x5A;
            D_80076F04 = 0;
        }
    } else {
        D_80076F08 = 0x5A;
        D_80076F04 = 0;
    }
    if ((D_800773B4 & 0x40) == 0 && (D_800773AC & 0x40) != 0 && D_80077028 == 0) {
        SoundSetCdVolumeWithFade(0, 0xA);
        D_80077014 = 5;
    }
    if ((D_800773B4 & 0x800) == 0 && (D_800773AC & 0x800) != 0 && D_80077028 == 0) {
        SoundSetCdVolumeWithFade(0, 0xA);
        D_80077014 = 5;
    }
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076AF0);

/* best 98.87% - identical except the D_80077070 store: the original keeps
   &D_80077070 in $a0 (loaded right after SetGeomOffset) and stores through it,
   ours emits lui $at/sh %lo. Shapes tried: plain global store + &sym call arg,
   s16* local, u32 cast through an int local, MATRIX* local with member access,
   cast-of-struct-pointer. gcc folds every pointer form back to the symbol.
void func_80076AF0(void) {
    MATRIX* pColorMatrix;

    SetGeomScreen(0x200);
    SetGeomOffset(0xA0, 0x78);
    pColorMatrix = (MATRIX*)&D_80077070;

    D_800770B8 = 0x1000;
    D_800770C0 = 0x1000;
    D_800770C8 = 0x1000;
    D_800770F8 = 0x1000;
    D_80077100 = 0x1000;
    D_80077108 = 0x1000;

    D_80077090 = 0x93D;
    D_80077092 = -0x93D;
    D_80077094 = -0x93D;

    D_800770BA = 0;
    D_800770BC = 0;
    D_800770BE = 0;
    D_800770C2 = 0;
    D_800770C4 = 0;
    D_800770C6 = 0;
    D_800770FA = 0;
    D_800770FC = 0;
    D_800770FE = 0;
    D_80077102 = 0;
    D_80077104 = 0;
    D_80077106 = 0;
    D_80077096 = 0;
    D_80077098 = 0;
    D_8007709A = 0;
    D_8007709C = 0;
    D_8007709E = 0;
    D_800770A0 = 0;

    *(s16*)&pColorMatrix->m[0][0] = 0x969;
    D_80077072 = 0;
    D_80077074 = 0;
    D_80077076 = 0x969;
    D_80077078 = 0;
    D_8007707A = 0;
    D_8007707C = 0x969;
    D_8007707E = 0;
    D_80077080 = 0;

    SetColorMatrix(pColorMatrix);
    SetBackColor(0x60, 0x60, 0x60);
}

*/

void func_80076C68(void) {
    long pad[6];
    MATRIX* pMatrix;

    func_80076CA4();
    pMatrix = &D_80077050;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076CA4);

/* best 73.61% - the original keeps &D_8007703C in $s5 across the GTE calls
   (that is also the extra saved register in its 0x30 frame) and forms
   &D_800770B8 as (address of D_800770CC) - 0x14; a source-level pointer to
   D_8007703C gets rematerialised instead. Also reorders the D_800770CC/D0/D4
   block.
void func_80076CA4(void) {
    s32 dz;
    s32 dzSq;
    s32 dx;
    s32 dxSq;
    s32 dy;
    s32* pz;

    dz = D_80077044 - D_80077034;
    dzSq = dz * dz;
    pz = &D_8007703C;
    dx = *pz - D_8007702C;
    dxSq = dx * dx;
    dy = D_80077040 - D_80077030;
    D_800770B0 = ratan2(dy, SquareRoot0(dzSq + dxSq));
    D_800770B2 = -ratan2(dx, dz);
    RotMatrix((SVECTOR*)&D_800770B0, (MATRIX*)&D_800770D8);
    RotMatrixZ(D_8007704C, (MATRIX*)&D_800770D8);
    MulMatrix2((MATRIX*)&D_800770F8, (MATRIX*)&D_800770D8);
    D_800770EC = 0;
    D_800770F0 = 0;
    D_800770F4 = SquareRoot0(dxSq + dy * dy + dzSq);
    D_800770CC = D_80076F2C - *pz;
    D_800770D0 = D_80076F30 - D_80077040;
    D_800770D4 = D_80076F34 - D_80077044;
    CompMatrix((MATRIX*)&D_800770D8, (MATRIX*)&D_800770B8, &D_80077050);
}
*/
