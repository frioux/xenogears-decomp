#include "common.h"
#include "psyq/libgpu.h"

extern u_long *g_GpuCurTIMHandle;

extern RECT D_8005A380;
extern u_long D_8005A384;
extern RECT D_8005A388;
extern u_long D_8005A38C;

extern u_long D_800569E8;
extern u_long D_800569EC;

extern char D_8001928C[]; // "id  =%08x\n"
extern char D_80019298[]; // "mode=%08x\n"
extern char D_800192A4[]; // "timaddr=%08x\n"

extern int Vsync(int mode);

extern int func_80047518(u_long *p, TIM_IMAGE *tim);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004668C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004696C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80046C58);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80046DB4);

void func_80046EFC(void) {
    D_800569E8 = Vsync(-1) + 0xF0;
    D_800569EC = 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80046F30);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004709C);

void func_80047178(void *p, int value, int n) {
    u_char *pByte = p;

    while (n-- != 0)
        *pByte++ = value;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_800471A4);

int OpenTIM(u_long *addr) {
    g_GpuCurTIMHandle = addr;
    return 0;
}

TIM_IMAGE *ReadTIM(TIM_IMAGE *timimg) {
    int size;

    if ((size = func_80047518(g_GpuCurTIMHandle, timimg)) == -1)
        return 0;

    g_GpuCurTIMHandle += size;
    return timimg;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004722C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004726C);

int func_80047518(u_long *p, TIM_IMAGE *tim) {
    int clutSize;
    int size;

    if (*p++ != 0x10)
        return -1;

    tim->mode = *p++;

    if (GetGraphDebug() == 2)
        printf(D_8001928C, 0x10);
    if (GetGraphDebug() == 2)
        printf(D_80019298, tim->mode);
    if (GetGraphDebug() == 2)
        printf(D_800192A4, p);

    if (tim->mode & 8) {
        clutSize = *p >> 2;
        tim->crect = (RECT *)(p + 1);
        tim->caddr = (u_long *)(p + 3);
        p += clutSize;
    } else {
        tim->crect = 0;
        tim->caddr = 0;
        clutSize = 0;
    }

    size = (*p >> 2) + 2;
    tim->prect = (RECT *)(p + 1);
    tim->paddr = (u_long *)(p + 3);
    return clutSize + size;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80047638);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_800477D0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80048AB0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", D_80048BBC);

