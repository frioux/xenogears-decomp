#include "common.h"
#include "psyq/libgpu.h"

extern u_long *g_GpuCurTIMHandle;

extern volatile long D_800569E8;
extern volatile long D_800569EC;

extern volatile u_long *D_800569A0;
extern volatile u_long *D_800569A4;
extern u_long *D_800569A8;

extern volatile u_long *D_800569B0;
extern u_long *D_800569C0;
extern u_long D_800569C4;
extern u_long D_800569C8;
extern u_long D_800569CC;

extern volatile u_long D_800569D4;
extern volatile u_long D_800569D8;
extern u_long D_800569E4;

extern void func_8004696C(void);
extern int func_80046F30(void);

extern u_char D_8005A27C[];
extern u_char D_8006BE34[];

extern u_long *D_8005A380;
extern u_long *D_8005A384;
extern u_long *D_8005A388;
extern u_long D_8005A38C;

extern char D_80019240[]; // "GPU timeout:que=%d,stat=%08x,chcr=%08x,madr=%08x,"
extern char D_80019274[]; // "func=(%08x)(%08x,%08x)\n"
extern char D_8001928C[]; // "id  =%08x\n"
extern char D_80019298[]; // "mode=%08x\n"
extern char D_800192A4[]; // "timaddr=%08x\n"

extern int Vsync(int mode);

extern int func_80047518(u_long *p, TIM_IMAGE *tim);
extern u_long func_80047638(void *p, int n, u_long *a, u_long *b, u_long *c);

extern int SetIntrMask(int newMask);

extern void SetDQA(long dqa);
extern void SetDQB(long dqb);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004668C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004696C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_80046C58);

/* best 73.51%: prologue, both case bodies and the tail are identical; gcc
   emits the whole switch (mode and 7) as one comparison chain where retail
   splits it with a range test (beq v1,1 / slti v0,v1,2 / beqz / beqz v1,0),
   i.e. retail's case tree is balanced over the values 0,1,3,5. 8 bytes short.
   Listing the 0/5 group first does not change it.
int func_80046C58(u_long mode) {
    D_800569E4 = SetIntrMask(0);
    D_800569D8 = 0;
    D_800569D4 = D_800569D8;

    switch (mode & 7) {
    case 0:
    case 5:
        *D_800569B0 = 0x401;
        *D_800569C0 |= 0x800;
        *D_800569A4 = 0;
        func_80047178(D_8005A27C, 0, 0x100);
        func_80047178(D_8006BE34, 0, 0x1800);
        break;
    case 1:
    case 3:
        *D_800569B0 = 0x401;
        *D_800569C0 |= 0x800;
        *D_800569A4 = 0x2000000;
        *D_800569A4 = 0x1000000;
        break;

    } */

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

/* best 94.49%: identical instruction multiset and size (364 bytes); the only
   differences are the scheduler/allocator choices inside the first printf's
   argument setup (retail keeps the D_800569A4 pointer in v1 and the discarded
   status read in v0, gcc picks a2/v1) plus one address form (retail reaches
   D_800569C4 with lui+addiu, reproduced here by the volatile pointer local).
   Tried: inline vs local que, (void) read before/after the que, pointer vs
   array declaration of D_800569C4, SetIntrMask returning int vs u_short, and
   both orders of the SetIntrMask(0)/D_800569D8 assignments.
int func_80046F30(void) {
    if (D_800569E8 < Vsync(-1) || D_800569EC++ > 0xF0000) {
        volatile u_long *pStat = D_800569A4;
        volatile u_long *pFunc = &D_800569C4;

        (void)*pStat;
        printf(D_80019240, (D_800569D4 - D_800569D8) & 0x3F, *pStat, *D_800569B0,
               *D_800569A8);
        printf(D_80019274, *pFunc, D_800569C8, D_800569CC);
        D_800569E4 = SetIntrMask(0);
        D_800569D8 = 0;
        D_800569D4 = D_800569D8;
        *D_800569B0 = 0x401;
        *D_800569C0 |= 0x800;
        *D_800569A4 = 0x2000000;
        *D_800569A4 = 0x1000000;
        SetIntrMask(D_800569E4);
        return -1;
    }
    return 0;
} */

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

