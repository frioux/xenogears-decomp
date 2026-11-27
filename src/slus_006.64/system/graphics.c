#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/graphics", GfxLoadImageAccelerated);
/*
Moves $sp into the HeapAlloc'd block (0x2000 bytes, top word at +0x1F00) around
the LoadImage call: `sw $sp,0($t0)` / `addiu $t0,$t0,-4` / `addu $sp,$t0`, and the
mirror `addiu $sp,$sp,4` / `lw $sp,0($sp)`. That is 7 of retail's 25 instructions
(28 of its 100 bytes) and no C statement produces it, so a body written without
it tops out low: the plain body below measures 60.80% / 68 bytes / 17 insns, and
the only differences are those 7 instructions plus `move $s0,$v0` sinking into the
LoadImage delay slot. An inline-asm spelling of the two halves was measured at
retail's exact size, which is why this stays a stub (game code may not use inline
asm). The same idiom is a retail-wide helper (battle func_800B626C, 800BB620,
800BEE2C, ...), so the original tree probably had a macro for it.

void GfxLoadImageAccelerated(void) {
    u8* pBuffer;

    pBuffer = HeapAlloc(0x2000, 1);
    LoadImage(g_GfxLoadImageRect, g_GfxLoadImageData);
    HeapFree(pBuffer);
}
*/


RECT* g_GfxLoadImageRect;
u_long* g_GfxLoadImageData;

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
            g_GfxLoadImageRect = &rect;
            g_GfxLoadImageData = (u_long*)pClut;
            GfxLoadImageAccelerated();
        }
    } while (0);
}
