#include "common.h"
#include "psyq/libgpu.h"

extern u_long *g_GpuCurTIMHandle;

extern volatile long D_800569E8;
extern volatile long D_800569EC;

extern volatile u_long *D_800569A0;
extern volatile u_long *D_800569A4;
extern u_long *D_800569A8;

extern volatile u_long *D_800569B0;
extern volatile u_long *D_800569C0;
extern u_long D_800569C4;
extern u_long D_800569C8;
extern u_long D_800569CC;

extern volatile u_long D_800569D4;
extern volatile u_long D_800569D8;
extern u_long D_800569E4;

extern int func_8004696C(void);
extern int func_8004709C(u_long mode);
extern void func_80047178(void *p, int value, int n);
extern int func_80046F30(void);

extern u_char D_8005A27C[];

/* The GPU command queue: 64 entries of 0x60 bytes, whose three used words are
   carved up as three separate symbols by splat. */
extern void (*D_8006BE34[][0x18])(u_long *p0, u_long *p1);
extern u_long *D_8006BE38[][0x18];
extern u_long *D_8006BE3C[][0x18];
extern u_long g_DrawSyncCallbackPending;
extern u_long g_SavedIntrMask;
extern void (*g_DrawSyncCallbackFn)();

extern u_long *D_8005A380;
extern u_long *D_8005A384;
extern u_long *D_8005A388;
extern u_long D_8005A38C;

extern char D_80019240[]; // "GPU timeout:que=%d,stat=%08x,chcr=%08x,madr=%08x,"
extern char D_80019274[]; // "func=(%08x)(%08x,%08x)\n"
extern char D_8001928C[]; // "id  =%08x\n"
extern char D_80019298[]; // "mode=%08x\n"
extern char D_800192A4[]; // "timaddr=%08x\n"
extern char D_800192B4[]; // "analizing TMD...\n"
extern char D_800192C8[]; // "\tid=%08X, flags=%d, nobj=%d, objid=%d\n"
extern char D_800192F0[]; // "\tvert=%08X, nvert=%d\n"
extern char D_80019308[]; // "\tnorm=%08X, nnorm=%d\n"
extern char D_80019320[]; // "\tprim=%08X, nprim=%d\n"

extern int Vsync(int mode);

extern int func_80047518(u_long *p, TIM_IMAGE *tim);
extern u_long func_80047638(u_long *tmd, int obj_no, u_long *pPrim, u_long *pVert, u_long *pNorm);

extern int SetIntrMask(int newMask);
extern int DMACallback(int id, void (*func)());

extern void SetDQA(long dqa);
extern void SetDQB(long dqb);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004668C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_8004696C);

/* best 81.20% (720 of retail's 724 bytes): everything up to and including the
   call matches, as does the guard/rotation (the explicit if + do/while is what
   makes gcc place the &D_800569C4 pointer in s0 after the guard). Two diffs are
   left. (1) The three word stores after the call: retail interleaves
   load-index / scale / load / store for each of D_800569C4, C8, CC, gcc hoists
   all three volatile loads of D_800569D8 above the first store. (2) The call
   sets its first argument from a2 instead of a1. Tried: the queue as one
   0x60-stride struct (wrong offsets), separate statements, storing C4 last, and
   a plain while loop instead of if+do/while (that loses the s0 pointer and
   drops to 76.50%).
int func_8004696C(void) {
    u_long *pQue;
    u_long *pPending;

    if (*D_800569B0 & 0x1000000)
        return 1;

    g_SavedIntrMask = SetIntrMask(0);

    if (D_800569D4 != D_800569D8 && !(*D_800569B0 & 0x1000000)) {
        pQue = &D_800569C4;
        do {
            if (((D_800569D8 + 1) & 0x3F) == D_800569D4) {
                if (!g_DrawSyncCallbackFn)
                    DMACallback(2, 0);
            }

            while (!(*D_800569A4 & 0x4000000))
                ;

            D_8006BE34[D_800569D8][0](D_8006BE38[D_800569D8][0], D_8006BE3C[D_800569D8][0]);
            *pQue = (u_long)D_8006BE34[D_800569D8][0];
            D_800569C8 = (u_long)D_8006BE38[D_800569D8][0];
            D_800569CC = (u_long)D_8006BE3C[D_800569D8][0];
            D_800569D8 = (D_800569D8 + 1) & 0x3F;
        } while (D_800569D4 != D_800569D8 && !(*D_800569B0 & 0x1000000));
    }

    SetIntrMask(g_SavedIntrMask);

    if (D_800569D4 == D_800569D8 && !(*D_800569B0 & 0x1000000)) {
        pPending = &g_DrawSyncCallbackPending;
        if (*pPending && g_DrawSyncCallbackFn) {
            *pPending = 0;
            g_DrawSyncCallbackFn();
        }
    }

    return (D_800569D4 - D_800569D8) & 0x3F;
} */

int func_80046C58(u_long mode) {
    u_char v;

    D_800569E4 = SetIntrMask(0);
    D_800569D8 = 0;
    D_800569D4 = D_800569D8;

    v = mode & 7;
    switch (v) {
    case 5:
    case 0:
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
    }

    SetIntrMask(D_800569E4);

    if ((mode & 7) == 0)
        return func_8004709C(mode);

    return 0;
}


int func_80046DB4(int mode) {
    u_long n;

    if (mode == 0) {
        func_80046EFC();

        while (D_800569D4 != D_800569D8) {
            func_8004696C();
            if (func_80046F30() != 0)
                return -1;
        }

        while ((*D_800569B0 & 0x1000000) || !(*D_800569A4 & 0x4000000)) {
            if (func_80046F30() != 0)
                return -1;
        }

        return 0;
    }

    n = (D_800569D4 - D_800569D8) & 0x3F;
    if (n != 0)
        func_8004696C();

    if ((*D_800569B0 & 0x1000000) || !(*D_800569A4 & 0x4000000)) {
        if (n == 0)
            return 1;
        return n;
    }

    return n;
}



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

int OpenTMD(u_long *tmd, int obj_no) {
    return D_8005A38C = func_80047638(tmd, obj_no, &D_8005A388, &D_8005A380, &D_8005A384);
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

/* Object table entry of a TMD model (struct TMD_STRUCT in libgs.h). */
typedef struct {
    /* 0x0 */ u_long vertop;
    /* 0x4 */ long vern;
    /* 0x8 */ u_long nortop;
    /* 0xC */ long norn;
    /* 0x10 */ u_long primtop;
    /* 0x14 */ long primn;
    /* 0x18 */ u_long scale;
} TmdObj; // Size: 0x1C

u_long func_80047638(u_long *tmd, int obj_no, u_long *pPrim, u_long *pVert, u_long *pNorm) {
    TmdObj *pObj = (TmdObj *)((u_char *)tmd + 0xC);

    if (GetGraphDebug() == 2)
        printf(D_800192B4);
    if (GetGraphDebug() == 2)
        printf(D_800192C8, tmd[0], tmd[1], tmd[2], obj_no);
    if (GetGraphDebug() == 2)
        printf(D_800192F0, pObj[obj_no].vertop, pObj[obj_no].vern);
    if (GetGraphDebug() == 2)
        printf(D_80019308, pObj[obj_no].nortop, pObj[obj_no].norn);
    if (GetGraphDebug() == 2)
        printf(D_80019320, pObj[obj_no].primtop, pObj[obj_no].primn);

    *pVert = (u_long)((u_char *)pObj + pObj[obj_no].vertop);
    *pNorm = (u_long)((u_char *)pObj + pObj[obj_no].nortop);
    *pPrim = (u_long)((u_char *)pObj + pObj[obj_no].primtop);

    return pObj[obj_no].primn;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", func_800477D0);

void SetFogNearFar(long a, long b, long h) {
    long d;
    long n;
    long f;

    d = b - a;
    if (d < 100)
        return;

    n = ((-a * b) / d << 8) / h;
    f = ((b << 12) / d) << 12;

    if (n < -0x8000)
        n = -0x8000;
    if (n > 0x7FFF)
        n = 0x7FFF;

    SetDQA(n);
    SetDQB(f);
}
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_c", D_80048BBC);

