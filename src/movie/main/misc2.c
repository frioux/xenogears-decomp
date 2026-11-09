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

// .rodata
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
extern s32 D_8007711C;
extern u8 D_8007713C;
extern u8 D_80077191;
extern u8 D_80077274;
extern u8 D_800772C9;
extern s32 D_8007739C;
extern s32 D_800773AC;
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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_800753B8);

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

/* best 82.33% - branch layout of the state==2 early return differs
void func_800763BC(u8 mode) {
    long pad[6]; // unused frame slot in the original
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
        goto exit;
    } else {
        return;
    }
    if (D_8007711C >= (s16)ArchiveDecodeSizeAbsolute(nIndex)) {
        return;
    }
exit:
    D_8007713C = 0;
    D_80077274 = 0;
    D_80077191 = 1;
    D_800772C9 = 1;
    func_80076488();
}
*/

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076488);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_800768D8);

/* best 97.06% - one extra nop after the D_80077438 load
void func_800768D8(u16 arg0, s32 arg1, u16 arg2) {
    D_80077010 = arg0;
    if (D_801D68B4 == 1) {
        if (arg2 == D_80077024) {
            D_80077018 = 1;
        } else {
            D_80077018 = 0;
        }
    } else if (arg2 == D_80077024) {
        D_80077018 = 1;
        D_8007701C = 1;
    } else {
        D_80077018 = 0;
        D_8007701C = 0;
    }
    if (arg0 < D_8007739C) {
        return;
    }
    if (D_80077438 != 0) {
        return;
    }
    D_80077014 = 1;
}
*/

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_800769A4);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076AF0);

void func_80076C68(void) {
    long pad[6];
    MATRIX* pMatrix;

    func_80076CA4();
    pMatrix = &D_80077050;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076CA4);
