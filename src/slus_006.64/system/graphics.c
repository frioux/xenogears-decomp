#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/graphics", GfxLoadImageAccelerated);
/*
Matches on ASPSX 2.56, GCC 2.6.0

void GfxLoadImageAccelerated(void) {
    char* pBuffer;

    pBuffer = HeapAlloc(0x2000, 1);
    
    asm(""
        "\tmove  $t0, %0\n"
        "\tsw    $sp, 0($t0)\n"
        "\taddiu $t0, $t0, -4\n"
        "\taddu  $sp, $t0, $zero\n"
    :
    : "r"(pBuffer + 0x1F00)
    : "t0", "memory"
    );
    
    LoadImage(D_800592F0, D_800592F4);
    
    asm(""
        "\taddiu $sp, $sp, 4\n"
        "\tlw    $sp, 0($sp)\n"
    ::
    : "memory"
    );
    
    HeapFree(pBuffer);
}
*/


RECT* D_800592F0;
u_long* D_800592F4;

void GfxLoadClutsAccelerated(u8* pData, int x, u16 y) {
    RECT rect;
    s32 count;
    s32* pOffset;
    u16* pClut;
    s32 offset;

    pOffset = (s32*)pData;
    count = *pOffset++;
    offset = 0;
    rect.y = y;
    // Load-bearing block: without it gcc puts pData in $s3 and &rect in $s4.
    do {
        while (count-- > 0) {
            pClut = (u16*)(*pOffset++ + (mem_addr)pData);
            rect.w = *pClut++;
            rect.h = *pClut++;
            rect.x = x + offset;
            offset += 0x40;
            D_800592F0 = &rect;
            D_800592F4 = (u_long*)pClut;
            GfxLoadImageAccelerated();
        }
    } while (0);
}
