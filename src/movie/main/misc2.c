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
extern u32 g_ArchiveTable;

// .rodata
extern char D_80070370[];
extern char D_80070394[];

// .bss
extern void* D_8007700C;
extern MATRIX D_80077050;

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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076488);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_800768D8);

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
