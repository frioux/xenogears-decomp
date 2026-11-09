#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

// This range of the original executable was built with gcc 2.7.2-cdk (the
// XenoPauseText preset in gears.toml); the rest of system/rendering is
// 2.7.2-psx, which does not reproduce it.
extern void* LZSSHeapDecompress(void* pCompressedData, int flags);
extern void func_8002DDE4(void* pImg, int arg1, int x, int y, int arg4, int arg5, int arg6);
extern u8 g_GfxPauseLettersCompressed[];

void GraphicsDrawPauseLetters(int x, int y) {
    void* pImg;

    pImg = LZSSHeapDecompress(g_GfxPauseLettersCompressed, 0);
    func_8002DDE4(pImg, 1, x, y, 0, 0, 0);
    DrawSync(0);
    HeapFree(pImg);
}

// 800592E4 -> EA is bss local
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering_pause_text", func_8001FB30);
