#include "common.h"
#include "psyq/interrupts.h"

// Adapted from https://github.com/sozud/psy-q-decomp

#define DMA_CHANNEL_COUNT 7 // channels 0-6
#define DMA_DICR_ENABLE_SHIFT 16 // DICR bit that enables one channel's interrupt
#define DMA_DICR_FLAG_SHIFT 24 // DICR bit that flags one channel's interrupt
#define DMA_DICR_FLAG_MASK 0x7F

void trapIntrDMA(void);
VoidCallback_t setIntrDMA(unsigned int index, VoidCallback_t callback);
static void memclrIntrDMA(void(**callbacks)(void), unsigned int numCallbacks);

void* startIntrDMA(void) {
    memclrIntrDMA(&g_DmaInterruptCallbacks, MAX_INTERRUPT_CALLBACKS);
    *g_pDMA_DICR = 0;
    InterruptCallback(3, &trapIntrDMA);
    return &setIntrDMA;
}

extern char D_80019440; // "DMA bus error: code=%08x\n"
extern char D_8001945C; // "MADR[%d]=%08x\n"

typedef struct {
    /* 0x0 */ volatile u_long madr;
    /* 0x4 */ volatile u_long bcr;
    /* 0x8 */ volatile u_long chcr;
    /* 0xC */ u_long padC;
} DmaChannelRegs; // Size: 0x10

extern DmaChannelRegs* D_8005898C; // channel 0's registers

void trapIntrDMA(void) {
    int i;
    u_long mask;

    mask = (*g_pDMA_DICR >> DMA_DICR_FLAG_SHIFT) & DMA_DICR_FLAG_MASK;
    while (mask) {
        for (i = 0; mask && i < DMA_CHANNEL_COUNT; ++i, mask >>= 1) {
            if (mask & 1) {
                *g_pDMA_DICR &= (1 << (i + DMA_DICR_FLAG_SHIFT)) | 0xFFFFFF;
                if (g_DmaInterruptCallbacks[i]) {
                    g_DmaInterruptCallbacks[i]();
                }
            }
        }
        mask = (*g_pDMA_DICR >> DMA_DICR_FLAG_SHIFT) & DMA_DICR_FLAG_MASK;
    }

    if ((*g_pDMA_DICR & 0xFF000000) == 0x80000000 || (*g_pDMA_DICR & 0x8000)) {
        printf(&D_80019440, *g_pDMA_DICR);
        for (i = 0; i < DMA_CHANNEL_COUNT; ++i) {
            printf(&D_8001945C, i, D_8005898C[i].madr);
        }
    }
}

VoidCallback_t setIntrDMA(unsigned int index, VoidCallback_t callback) {
    VoidCallback_t old = g_DmaInterruptCallbacks[index];

    if (callback == old) {
        return old;
    }
    if (callback != NULL) {
        g_DmaInterruptCallbacks[index] = callback;
        *g_pDMA_DICR = (*g_pDMA_DICR & 0xFFFFFF) | 0x800000 | (1 << (index + DMA_DICR_ENABLE_SHIFT));
    } else {
        g_DmaInterruptCallbacks[index] = NULL;
        *g_pDMA_DICR = ((*g_pDMA_DICR & 0xFFFFFF) | 0x800000) & ~(1 << (index + DMA_DICR_ENABLE_SHIFT));
    }
    return old;
}

static void memclrIntrDMA(void(**callbacks)(void), unsigned int numCallbacks) {
    while (numCallbacks--) {
        *callbacks++ = NULL;
    }
}