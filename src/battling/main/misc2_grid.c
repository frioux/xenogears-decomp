#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

/* Per-cell shade entry of the block BattleRenderShadedSpans copies to
   0x1F800120 (misc11.c's g_BattleSpanRenderParams): four 8-byte entries and
   four palette words. */
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ u16 unk6;
} BattleGridShade; // Size: 0x8

typedef struct {
    /* 0x00 */ BattleGridShade shades[4];
    /* 0x20 */ u32 palette[4];
} BattleGridParams; // Size: 0x30

/* One 0x20-byte shaded triangle of a grid tile: the tag whose low 24 bits link
   it into an ordering-table bucket, the GTE-lit colour, the three screen
   coordinates and the tile's shade word. */
typedef struct {
    /* 0x00 */ u32 tag;
    /* 0x04 */ u32 rgb0;
    /* 0x08 */ u32 xy0;
    /* 0x0C */ u32 rgb1;
    /* 0x10 */ u32 xy1;
    /* 0x14 */ u32 rgb2;
    /* 0x18 */ u32 xy2;
    /* 0x1C */ u16 shade;
    /* 0x1E */ u16 pad1E;
} BattleGridPacket; // Size: 0x20

/* The grid cell is two halves: the height of the tile's corner and the flags
   that pick this cell's shade entry and palette word. */
#define BATTLE_GRID_ROW_STRIDE 0x80
#define BATTLE_GRID_ROW_COUNT 0x7F
#define BATTLE_GRID_TILE_SIZE 0x100
#define BATTLE_GRID_SHADE_MASK 0xF0F0
#define BATTLE_GRID_SHADE_ENTRY 0x3
#define BATTLE_GRID_PALETTE_ENTRY 0xC
#define BATTLE_GRID_EMPTY_SPAN 0xFF
#define BATTLE_GRID_PACKET_LINK_MASK 0xFFFFFF
#define BATTLE_GRID_PACKET_CODE 0x07000000
#define BATTLE_GRID_PACKET_WORDS 0x20

extern u32 *g_BattleGridCells;
extern u8 g_BattleDrawBufferIndex;
extern u8 *D_80092854[];
extern s32 g_BattleFramePolyCount;
extern s32 g_BattleEmittedPacketCount;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2_grid", func_80072D18);

/* Handwritten function: retail cannot be compiler output, so no C body can
   reproduce it. (1) The prologue stores $s0-$s7 at -0x4..-0x24($sp) with NO
   `addiu $sp,$sp,-N` at all - gcc always allocates the frame it saves into.
   (2) RTPT, NCLIP and DPCS appear with no GTE pipeline nops, which the 3.x
   macros (gte_rtpt/gte_nclip/gte_dpcs) always emit two of. (3) It uses the
   assembler pseudo-forms `add`/`addi` (0x3288, func_8007313C), never what cc1
   emits. The lwc2/mfc2 groups are hand-interleaved across the two triangles
   (the second RTPT sits after the first packet is linked) and the loop's cell
   increment is hoisted into the middle of the body.
   Best C form, written from the assembly: 23.56%, 1040 bytes against retail's
   840 (the 12 macro nops plus the 56-byte frame the &sz1/&sz2 read-backs
   force, and the packet cursor cannot stay in a register across three
   address-taken locals). Left for the record; it also documents the data: the
   span tables at 0x1F800000/0x1F800080, the four-vertex scratch quad at
   0x1F800100 and the shade block at 0x1F800120 that BattleRenderShadedSpans
   (misc11.c) fills. */
/*void func_80072D18(s32 *pOt, s32 xOrigin, s32 yOrigin) {
    u8 *pSpanMax = (u8 *)0x1F800000;
    u8 *pSpanMin = (u8 *)0x1F800080;
    SVECTOR *pVerts = (SVECTOR *)0x1F800100;
    BattleGridParams *pParams = (BattleGridParams *)0x1F800120;
    BattleGridPacket *pPacket;
    BattleGridShade *pShade;
    u32 *pBucket;
    u32 *pCell;
    u32 *pEnd;
    u32 cell;
    u32 colorMask;
    u32 pal;
    u32 head;
    u32 sz1;
    u32 sz2;
    u32 sz3;
    u32 depth;
    s32 opz;
    s32 min;
    s32 span;
    s32 x;
    s32 y;
    s32 i;

    pPacket = (BattleGridPacket *)D_80092854[g_BattleDrawBufferIndex];
    for (i = 0; i < BATTLE_GRID_ROW_COUNT; i++) {
        if (pSpanMax[i] == 0) {
            continue;
        }
        min = pSpanMin[i];
        if (min == BATTLE_GRID_EMPTY_SPAN) {
            continue;
        }
        if (min >= pSpanMax[i]) {
            continue;
        }
        span = pSpanMax[i] - min;
        x = (min << 8) - xOrigin;
        y = (i << 8) - yOrigin;
        g_BattleFramePolyCount += span;
        pCell = &g_BattleGridCells[(i << 7) + min];
        pEnd = pCell + span;
        do {
            cell = *pCell;
            pVerts[3].vx = x;
            pVerts[0].vx = x;
            pVerts[2].vz = y;
            pVerts[0].vz = y;
            x += BATTLE_GRID_TILE_SIZE;
            pVerts[2].vx = x;
            pVerts[1].vx = x;
            pVerts[3].vz = y + BATTLE_GRID_TILE_SIZE;
            pVerts[1].vz = y + BATTLE_GRID_TILE_SIZE;
            pVerts[0].vy = cell;
            pVerts[1].vy = *(u16 *)(pCell + BATTLE_GRID_ROW_STRIDE + 1);
            pVerts[2].vy = *(u16 *)(pCell + 1);
            pVerts[3].vy = *(u16 *)(pCell + BATTLE_GRID_ROW_STRIDE);
            gte_ldv3c(pVerts);
            colorMask = (cell >> 16) & BATTLE_GRID_SHADE_MASK;
            pShade = &pParams->shades[(cell >> 16) & BATTLE_GRID_SHADE_ENTRY];
            pal = pParams->palette[((cell >> 16) & BATTLE_GRID_PALETTE_ENTRY) >> 2];
            gte_rtpt();
            gte_nclip();
            gte_stopz_val(opz);
            if (opz > 0) {
                gte_ldv1(&pVerts[3]);
                gte_ldv2(&pVerts[1]);
                gte_dpcs();
                gte_strgb(&pPacket->rgb0);
                pPacket->rgb1 = colorMask | pShade->unk0 | (pal & 0xFFFF0000);
                pPacket->rgb2 = colorMask | pShade->unk6 | (pal << 16);
                pPacket->shade = colorMask | pShade->unk2;
                gte_stsz1(&sz1);
                gte_stsz2(&sz2);
                gte_stsz(&sz3);
                depth = sz1;
                if (sz2 < depth) {
                    depth = sz2;
                }
                if (sz3 < depth) {
                    depth = sz3;
                }
                pBucket = (u32 *)((u8 *)pOt + ((depth >> 4) << 2));
                head = *pBucket;
                gte_stsxy2(&pPacket->xy2);
                gte_stsxy0(&pPacket->xy0);
                gte_stsxy1(&pPacket->xy1);
                pPacket = (BattleGridPacket *)((u32)pPacket & BATTLE_GRID_PACKET_LINK_MASK);
                *pBucket = (u32)pPacket;
                pPacket->tag = (head & BATTLE_GRID_PACKET_LINK_MASK) | BATTLE_GRID_PACKET_CODE;
                pPacket++;
                gte_rtpt();
                gte_nclip();
                gte_stopz_val(opz);
                if (opz > 0) {
                    gte_dpcs();
                    gte_strgb(&pPacket->rgb0);
                    pPacket->rgb1 = colorMask | pShade->unk0 | (pal & 0xFFFF0000);
                    pPacket->rgb2 = colorMask | pShade->unk4 | (pal << 16);
                    pPacket->shade = colorMask | pShade->unk6;
                    gte_stsz1(&sz1);
                    gte_stsz2(&sz2);
                    gte_stsz(&sz3);
                    depth = sz1;
                    if (sz2 < depth) {
                        depth = sz2;
                    }
                    if (sz3 < depth) {
                        depth = sz3;
                    }
                    pBucket = (u32 *)((u8 *)pOt + ((depth >> 4) << 2));
                    head = *pBucket;
                    gte_stsxy2(&pPacket->xy2);
                    gte_stsxy0(&pPacket->xy0);
                    gte_stsxy1(&pPacket->xy1);
                    pPacket = (BattleGridPacket *)((u32)pPacket & BATTLE_GRID_PACKET_LINK_MASK);
                    *pBucket = (u32)pPacket;
                    pPacket->tag = (head & BATTLE_GRID_PACKET_LINK_MASK) | BATTLE_GRID_PACKET_CODE;
                    pPacket++;
                }
            }
            pCell++;
        } while (pCell != pEnd);
    }
    head = (u32)D_80092854[g_BattleDrawBufferIndex] & BATTLE_GRID_PACKET_LINK_MASK;
    g_BattleEmittedPacketCount +=
        (((u32)pPacket & BATTLE_GRID_PACKET_LINK_MASK) - head) / BATTLE_GRID_PACKET_WORDS;
}
*/
