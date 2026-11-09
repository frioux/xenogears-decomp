#include "common.h"
#include "psyq/libgpu.h"

extern u_long *g_GpuCurTIMHandle;

extern u_long D_800569E8;
extern u_long D_800569EC;

extern volatile u_long *D_800569A0;
extern volatile u_long *D_800569A4;

extern volatile u_long *D_800569B0;

extern u_long D_800569D4;
extern u_long D_800569D8;

extern void func_8004696C(void);
extern int func_80046F30(void);

extern u_long *D_8005A380;
extern u_long *D_8005A384;
extern u_long *D_8005A388;
extern u_long D_8005A38C;

extern char D_8001928C[]; // "id  =%08x\n"
extern char D_80019298[]; // "mode=%08x\n"
extern char D_800192A4[]; // "timaddr=%08x\n"

extern int Vsync(int mode);

extern int func_80047518(u_long *p, TIM_IMAGE *tim);
extern u_long func_80047638(void *p, int n, u_long *a, u_long *b, u_long *c);

extern void SetDQA(long dqa);
extern void SetDQB(long dqb);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004668C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004696C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80046C58);

/* best 94.39%: every insn multiset and the block layout match; the only
   difference is the tail, where retail branches on n with the n result in the
   delay slot (bnez s0 / move v0,s0 / j / li v0,1) and gcc reverses the ternary
   to beqz s0 / li v0,1 / j / move v0,s0. Tried if/return, nested if/else,
   n ? n : 1, n == 0 ? 1 : n, and a ret variable; all give the reversed form.
int func_80046DB4(int wait) {
    u_long n;

    if (wait == 0) {
        func_80046EFC();

        while (D_800569D4 != D_800569D8) {
            func_8004696C();
            if (func_80046F30())
                return -1;
        }

        while ((*D_800569B0 & 0x1000000) || !(*D_800569A4 & 0x4000000)) {
            if (func_80046F30())
                return -1;
        }

        return 0;
    }

    n = (D_800569D4 - D_800569D8) & 0x3F;
    if (n != 0)
        func_8004696C();

    if ((*D_800569B0 & 0x1000000) || !(*D_800569A4 & 0x4000000)) {
        if (n != 0)
            return n;
        return 1;
    }

    return n;
} */

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80046DB4);

void func_80046EFC(void) {
    D_800569E8 = Vsync(-1) + 0xF0;
    D_800569EC = 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80046F30);

int func_8004709C(u_long mode) {
    *D_800569A4 = 0x10000007;

    if ((*D_800569A0 & 0xFFFFFF) != 2) {
        *D_800569A0 = (*D_800569A4 & 0x3FFF) | 0xE1001000;
        (void)*D_800569A0; // the status port is read back to flush the write
        if (!(*D_800569A4 & 0x1000))
            return 0;
        if (!(mode & 8))
            return 1;
        *D_800569A4 = 0x20000504;
        return 2;
    }

    if (!(mode & 8))
        return 3;

    *D_800569A4 = 0x09000001;
    return 4;
}

void func_80047178(void *p, int value, int n) {
    u_char *pByte = p;

    while (n-- != 0)
        *pByte++ = value;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", GPU_cw);

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

u_long func_8004722C(void *p, int n) {
    return D_8005A38C = func_80047638(p, n, &D_8005A388, &D_8005A380, &D_8005A384);
}

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", SetFogNearFar);

/* best 91.54%: gcc sinks the b division after a's shift and keeps b in s0
void SetFogNearFar(int x0, int x1, int h) {
    int d;
    long a;
    long b;

    d = x1 - x0;
    if (d < 100)
        return;

    a = (-x0 * x1) / d;
    b = (x1 << 12) / d;
    a = (a << 8) / h;

    if (a < -0x8000)
        a = -0x8000;
    if (a > 0x7FFF)
        a = 0x7FFF;

    SetDQA(a);
    SetDQB(b << 12);
} */

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", D_80048BBC);

