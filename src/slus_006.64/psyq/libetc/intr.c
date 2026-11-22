#include "common.h"
#include "psyq/interrupts.h"

// Adapted from https://github.com/sozud/psy-q-decomp


int setjmp(int*);
SetVsyncIntrCallback_t startIntrVSync();
VoidCallback_t setIntrDMA(int index, VoidCallback_t callback);
void trapIntrDMA(void);

void ChangeClearRCnt(int, int);

long long startIntrDMA();

/*
static InterruptEnvironment_t g_InterruptEnvironment = {
    0,
    0,
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    0,
    0,
    0,
};

static InterruptControl_t g_InterruptControl = {
    "$$Id: intr.c,v 1.76 1997/02/12 12:45:05 makoto Exp $",
    0,
    setIntr,
    startIntr,
    stopIntr,
    0,
    restartIntr,
    &g_InterruptEnvironment
};
*/



void* ResetCallback(void) { 
    return g_pInterruptControl->intrResetHandler();
}

void InterruptCallback(unsigned int irq, VoidCallback_t fn) {
    g_pInterruptControl->setIntrHandler(irq, fn);
}

VoidCallback_t DMACallback(int dma, VoidCallback_t fn) {
    return g_pInterruptControl->setDMAIntrCallback(dma, fn);
}

void VSyncCallback(VoidCallback_t fn) {
    g_pInterruptControl->setVsyncIntrCallback(4, fn);
}

int VSyncCallbacks(int ch, VoidCallback_t fn) {
    return g_pInterruptControl->setVsyncIntrCallback(ch, fn);
}

void* StopCallback(void) {
    return g_pInterruptControl->intrStopHandler(); 
}

void* RestartCallback(void) {
    return g_pInterruptControl->intrRestartHandler(); 
}

int CheckCallback(void) { 
    return g_InterruptEnvironment.inInterrupt; 
}

u_short GetIntrMask(void) { 
    return *g_pI_MASK; 
}

u_short SetIntrMask(u_short newMask) {
    u_short mask;

    mask = *g_pI_MASK;
    *g_pI_MASK = newMask;
    return mask;
}

void* startIntr() {
    if (g_InterruptEnvironment.interruptsInitialized)
        return NULL;
    
    *g_pI_STAT = *g_pI_MASK = 0;
    *g_pDMA_DPCR = 0x33333333;
    memclrIntr(&g_InterruptEnvironment, sizeof(g_InterruptEnvironment) / sizeof(int));
    if (setjmp(g_InterruptEnvironment.buf))
        trapIntr();
    
    g_InterruptEnvironment.buf[JB_SP] = (int)&g_InterruptEnvironment.stack[1004];
    HookEntryInt((u_short*)g_InterruptEnvironment.buf);
    g_InterruptEnvironment.interruptsInitialized = 1;
    g_pInterruptControl->setVsyncIntrCallback = startIntrVSync();
    g_pInterruptControl->setDMAIntrCallback = startIntrDMA();
    _96_remove();
    ExitCriticalSection();
    return &g_InterruptEnvironment;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libetc/intr", trapIntr);

extern char D_80019408; // "unexpected interrupt(%04x)\n"
extern char D_80019424; // "intr timeout(%04x:%04x)\n"

/*
96.06%, 122 instructions against retail's 122 (488 bytes): every instruction is
present, every call, branch and delay slot is retail's, and the frame (0x28,
s0-s4 + ra) is identical. The residual is the load ORDER around the
`inInterrupt = 1` store and which register the final `and` lands in. Retail
loads enabledInterruptsMask BEFORE that store and *g_pI_MASK after it, so it
ends `and v1,v1,a0 / and v0,v0,v1 / move s0,v0`; cc1 here hoists the g_pI_MASK
pointer load above the store as well and lands the final and straight in s0.
The two field addresses below are absolute in retail's object (D_800578D4 =
enabledInterruptsMask, D_800578A6 = inInterrupt inside g_InterruptEnvironment),
not s1-relative, so they are read through their own symbols here.
Tried: the whole expression in one statement (94.71), the expression before the
store (93.36), the fields read through the struct instead of the absolute
symbols (94.47), the mask assignment split at the store exactly as below (96.06),
*g_pI_STAT as the first operand (89.78, and one instruction long).
permute.sh 4 min: PERMUTER_BETTER 765 -> 730, on `mask = D_800578D4;` then
`mask = mask & *g_pI_STAT;`, which diff_func scores 93.07 - rejected.

extern u_short D_800578A6;
extern u_short D_800578D4;

void trapIntr(void) {
    int i;
    u_short mask;

    if (g_InterruptEnvironment.interruptsInitialized == 0) {
        printf(&D_80019408, *g_pI_STAT);
        ReturnFromException();
    }

    mask = D_800578D4 & *g_pI_STAT;
    D_800578A6 = 1;
    mask &= *g_pI_MASK;
    while (mask) {
        for (i = 0; mask && i < 11; ++i, mask >>= 1) {
            if (mask & 1) {
                *g_pI_STAT = ~(1 << i);
                if (g_InterruptEnvironment.handlers[i]) {
                    g_InterruptEnvironment.handlers[i]();
                }
            }
        }
        mask = (D_800578D4 & *g_pI_STAT) & *g_pI_MASK;
    }

    if (*g_pI_STAT & *g_pI_MASK) {
        if (D_8005893C++ > 0x800) {
            printf(&D_80019424, *g_pI_STAT, *g_pI_MASK);
            D_8005893C = 0;
            *g_pI_STAT = 0;
        }
    } else {
        D_8005893C = 0;
    }

    D_800578A6 = 0;
    ReturnFromException();
}
*/

VoidCallback_t setIntr(int index, VoidCallback_t fn) {
    VoidCallback_t pHandlerFn;
    u_short nMask;
    int nNewMask;

    pHandlerFn = g_InterruptEnvironment.handlers[index];
    if ((fn != pHandlerFn) && g_InterruptEnvironment.interruptsInitialized) {
        nMask = *g_pI_MASK;
        *g_pI_MASK = 0;
        nNewMask = nMask & 0xFFFF;
        if (fn != NULL) {
            g_InterruptEnvironment.handlers[index] = fn;
            nNewMask = nNewMask | (1 << index);
            g_InterruptEnvironment.enabledInterruptsMask |= (1 << index);
        } else {
            g_InterruptEnvironment.handlers[index] = 0;
            nNewMask = nNewMask & ~(1 << index);
            g_InterruptEnvironment.enabledInterruptsMask &= ~(1 << index);
        }
        if (index == 0) {
            ChangeClearPAD(fn == NULL);
            ChangeClearRCnt(3, fn == NULL);
        }
        if (index == 4) {
            ChangeClearRCnt(0, fn == NULL);
        }
        if (index == 5) {
            ChangeClearRCnt(1, fn == NULL);
        }
        if (index == 6) {
            ChangeClearRCnt(2, fn == NULL);
        }
        *g_pI_MASK = nNewMask;
    }
    return pHandlerFn;
}

void* stopIntr() {
    if (!g_InterruptEnvironment.interruptsInitialized)
        return NULL;

    EnterCriticalSection();
    g_InterruptEnvironment.savedMask = *g_pI_MASK;
    g_InterruptEnvironment.savedPcr = *g_pDMA_DPCR;
    *g_pI_STAT = *g_pI_MASK = 0;
    *g_pDMA_DPCR &= 0x77777777;
    ResetEntryInt();
    g_InterruptEnvironment.interruptsInitialized = 0;
    return &g_InterruptEnvironment;
}

void* restartIntr() {
    if (g_InterruptEnvironment.interruptsInitialized)
        return 0;

    HookEntryInt((u_short*)g_InterruptEnvironment.buf);
    g_InterruptEnvironment.interruptsInitialized = 1;
    *g_pI_MASK = g_InterruptEnvironment.savedMask;
    *g_pDMA_DPCR = g_InterruptEnvironment.savedPcr;
    ExitCriticalSection();
    return &g_InterruptEnvironment;
}

static void memclrIntr(void(**callbacks)(void), unsigned int numCallbacks) {
    while (numCallbacks--) {
        *callbacks++ = NULL;
    }
}
