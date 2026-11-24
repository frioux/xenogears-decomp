/* Snippet for tools/scripts/cc1_probe.sh battle/main/misc23: which destination
 * spelling makes cc1 emit retail's base-first 'addu v0,v0,a2' for the
 * g_BattleSpriteRenderData colour stores of func_800A64E4. */
#include "common.h"

extern u8 *g_Dst;

typedef struct {
    /* 0x0 */ u8 c0[8];
    /* 0x8 */ u8 c1[8];
    /* 0x10 */ u8 c2[8];
    /* 0x18 */ u8 c3[0xC];
} ProbeSlot; // Size: 0x24

void ProbeArray(u8 *pSrc) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (g_Dst != NULL) {
            g_Dst[i * 0x24 + 0x1740] = pSrc[0];
            g_Dst[i * 0x24 + 0x1741] = pSrc[1];
            g_Dst[i * 0x24 + 0x1742] = pSrc[2];
            pSrc += 4;
        }
    }
}

void ProbeSlotRows(u8 *pSrc) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (g_Dst != NULL) {
            ((ProbeSlot *)(g_Dst + 0x1740))[i].c0[0] = pSrc[0];
            ((ProbeSlot *)(g_Dst + 0x1740))[i].c0[1] = pSrc[1];
            ((ProbeSlot *)(g_Dst + 0x1740))[i].c0[2] = pSrc[2];
            pSrc += 4;
        }
    }
}

void ProbeRowArray(u8 *pSrc) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (g_Dst != NULL) {
            ((u8(*)[0x24])(g_Dst + 0x1740))[i][0] = pSrc[0];
            ((u8(*)[0x24])(g_Dst + 0x1740))[i][1] = pSrc[1];
            ((u8(*)[0x24])(g_Dst + 0x1740))[i][2] = pSrc[2];
            pSrc += 4;
        }
    }
}

void ProbeUserOff(u8 *pSrc) {
    s32 i;
    s32 off;

    off = 0;
    for (i = 0; i < 4; i++) {
        if (g_Dst != NULL) {
            g_Dst[off + 0x1740] = pSrc[0];
            g_Dst[off + 0x1741] = pSrc[1];
            g_Dst[off + 0x1742] = pSrc[2];
            pSrc += 4;
            g_Dst[off + 0x16E0] = pSrc[0];
        }
        off += 0x24;
    }
}

void ProbeUserOffConstFirst(u8 *pSrc) {
    s32 i;
    s32 off;

    off = 0;
    for (i = 0; i < 4; i++) {
        if (g_Dst != NULL) {
            g_Dst[0x1740 + off] = pSrc[0];
            g_Dst[0x1741 + off] = pSrc[1];
            g_Dst[0x1742 + off] = pSrc[2];
            pSrc += 4;
        }
        off += 0x24;
    }
}

void ProbeEmbedOff(u8 *pSrc) {
    s32 i;
    s32 off;

    for (i = 0; i < 4; i++) {
        if (g_Dst != NULL) {
            g_Dst[(off = i * 0x24) + 0x1740] = pSrc[0];
            g_Dst[(off = i * 0x24) + 0x1741] = pSrc[1];
            g_Dst[(off = i * 0x24) + 0x1742] = pSrc[2];
            pSrc += 4;
            g_Dst[(off = i * 0x18) + 0x16E0] = pSrc[0];
        }
    }
}

void ProbeEmbedPtr(u8 *pSrc) {
    s32 i;
    u8 *pDst;

    for (i = 0; i < 4; i++) {
        if (g_Dst != NULL) {
            ((u8 *)(pDst = g_Dst + i * 0x24))[0x1740] = pSrc[0];
            ((u8 *)(pDst = g_Dst + i * 0x24))[0x1741] = pSrc[1];
            ((u8 *)(pDst = g_Dst + i * 0x24))[0x1742] = pSrc[2];
            pSrc += 4;
        }
    }
}
