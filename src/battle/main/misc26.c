#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"

// WorkListEntry of include/system/animation.h; an overlay unit cannot include
// that header (it holds tentative definitions of main-exe globals), so the
// layout is repeated here.
typedef struct WorkListEntry {
    /* 0x00 */ void *unk0;
    /* 0x04 */ void *unk4;
    /* 0x08 */ void (*onTriggerCallback)(void *pTask);
    /* 0x0C */ void (*onFreeCallback)(void *pTask);
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ struct WorkListEntry *pNext;
} WorkListEntry; // Size: 0x1C

// One frame of a battle model, as BattleGetFrame hands it out: a header of
// self-relative (offset, count) pairs into the same allocation, then the flags
// word that records whether the part scales are already applied.
typedef struct {
    /* 0x00 */ s32 partsOffset; // byte offset of the part array, from here
    /* 0x04 */ s32 partCount;
    /* 0x08 */ u8 unk8[0x8]; // a second (offset, count) pair this unit never reads
    /* 0x10 */ s32 primsOffset; // byte offset of the primitive packet array
    /* 0x14 */ s32 primCount;
    /* 0x18 */ u32 flags; // 0x8000: part scales already applied
} BattleModelFrame; // Size: 0x1C

// A three-component scale as the timed scale task keeps it; unsigned because the
// task only halves and subtracts, and the ease ends at zero.
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 pad6;
} BattleScaleVector; // Size: 0x8

// The scale-easing timer task: a timer work-list entry, then the scale it eases
// and the countdown driving the ease.
typedef struct {
    /* 0x00 */ WorkListEntry entry;
    /* 0x1C */ BattleScaleVector current; // scale the task is at right now
    /* 0x24 */ BattleScaleVector start; // scale the ease started from
    /* 0x2C */ BattleScaleVector target; // scale the ease ends at
    /* 0x34 */ s32 frame; // frames elapsed
    /* 0x38 */ s32 framesLeft; // frames left in the ease
    /* 0x3C */ s32 halfLength; // half the ease length, the divisor for each step
} BattleScaleTask; // Size: 0x40

// The screen fade drawn over the battle: a timer entry (0x00) that steps the
// colour and a draw entry (0x1C) that emits the quad, then the two ends of the
// fade and the colour it is at on this frame. D_800C3C00 and D_800C3C50 are the
// two static ones, which is where the size comes from.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry drawEntry;
    /* 0x38 */ s32 length; // twice the frame count the fade was given
    /* 0x3C */ s32 timer; // counts the length down
    /* 0x40 */ u8 unk40;
    /* 0x41 */ u8 abr; // semi-transparency mode handed to GetTPage
    /* 0x42 */ u8 targetR;
    /* 0x43 */ u8 targetG;
    /* 0x44 */ u8 targetB;
    /* 0x45 */ u8 startR;
    /* 0x46 */ u8 startG;
    /* 0x47 */ u8 startB;
    /* 0x48 */ u8 curR;
    /* 0x49 */ u8 curG;
    /* 0x4A */ u8 curB;
    /* 0x4B */ u8 pad4B[0x5];
} BattleScreenFadeTask; // Size: 0x50

// The battle image task: a timer entry (0x00) that eases a value and a draw
// entry (0x1C) that re-applies the resulting colour tint when it moves.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry drawEntry;
    /* 0x38 */ s32 length; // twice the frame count the ease was given
    /* 0x3C */ s32 timer; // counts the length down
    /* 0x40 */ s16 start; // value the ease started from
    /* 0x42 */ s16 end; // value the ease ends at
    /* 0x44 */ s16 applied; // value the last tint was applied for
    /* 0x46 */ s16 current; // value the ease is at on this frame
    /* 0x48 */ s16 blendMode;
    /* 0x4A */ s16 r;
    /* 0x4C */ s16 g;
    /* 0x4E */ s16 b;
} BattleImageTask; // Size: 0x50

extern BattleScreenFadeTask *g_BattleScreenFadeTask;
extern BattleScaleTask *g_BattleScaleTask;
extern u8 g_BattleScreenFadeOwnBuffer;
extern u32 g_BattleImageTask;

extern u8 *g_GfxCurWorkBuffer;
extern u8 *g_GfxCurWorkBufferEnd;
extern u_long *g_GfxCurOT;

extern BattleScaleTask *BattleGetScaleTask();
extern void func_800B3358(BattleScaleTask *pTask);
extern void func_800B39C0(s32, s32, u8, u8, u8);
extern void BattleSetActorRectParams(s32, s32, s32, s32, s32, s32);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void BattleUploadAndFreeImageResources(void);
extern WorkListEntry *TimerWorkListAllocateTask();
extern void TimerWorkListSetTaskCallback();
extern void WorkListTaskSetOnFreeCallback();

extern u8 *D_800C3BEC;
extern s32 D_800C3BF0;
extern DR_TPAGE D_800C3BF8;

u8 *BattleGetFrame(u8 *pBase, s32 n) {
    return &pBase[n * 0x1C + 0xC];
}

s32 BattleGetPrimitiveBufferSize(BattleModelFrame *pFrame) {
    s32 count;
    s32 total;
    u8 *pPrim;
    s32 n;

    count = 0;
    total = 0;
    pPrim = (u8 *)((u32)pFrame->primsOffset + (u32)pFrame);
    n = pFrame->primCount;
    for (; count != n; count++) {
        total += (pPrim[0] + 1) * 4;
        pPrim += (pPrim[1] + 1) * 4;
    }
    return total;
}

// Advances the primitive cursor by one packet and counts the packet emitted.
void BattleAdvancePrimitiveCursor(void) {
    u8 *pCmd = D_800C3BEC;
    u8 size = pCmd[1];

    D_800C3BF0 += 1;
    D_800C3BEC = pCmd + (size + 1) * 4;
}


// What a saved primitive's code byte and colour flag select: bit 2 asks for a
// texture coordinate per vertex, bit 3 for four vertices instead of three, bit 4
// for a colour per vertex, and 0x100 marks a record that carries no colours and
// takes the flat 0x80 instead. Each combination is one of the eight POLY_*
// packets.
#define BATTLE_PRIM_KIND_MASK 0x1C
#define BATTLE_PRIM_TEXTURED 0x004
#define BATTLE_PRIM_QUAD 0x008
#define BATTLE_PRIM_GOURAUD 0x010
#define BATTLE_PRIM_UNLIT 0x100
#define BATTLE_PRIM_UNLIT_RGB 0x80 // the colour an unlit record's vertices take
#define BATTLE_TPAGE_ABR_MASK 0xFF9F // clears the semi-transparency bits of a tpage

// One 4-byte entry of a saved model primitive's trailing data: the per-vertex
// texture coordinate and its clut on a textured record, or a vertex colour.
typedef union {
    struct {
        /* 0x0 */ u8 u;
        /* 0x1 */ u8 v;
        /* 0x2 */ u16 clut;
    } tex;
    struct {
        /* 0x0 */ u8 r;
        /* 0x1 */ u8 g;
        /* 0x2 */ u8 b;
        /* 0x3 */ u8 code; // the record's own packet code, unused here
    } color;
} BattlePrimEntry; // Size: 0x4

// A saved primitive of a battle model frame (pFrame->primsOffset, walked by
// BattleGetPrimitiveBufferSize): the length of the GPU packet it becomes, the
// length of the record itself, its code byte, then the entries and finally the
// u16 part indices func_800B1F6C transforms. A textured record lists one entry
// per vertex before the colour entries, and drops the colours (into a flat
// 0x80) when bit 0 of flags is clear.
typedef struct {
    /* 0x00 */ u8 outLen; // packet length in words, minus one
    /* 0x01 */ u8 len; // record length in words, minus one
    /* 0x02 */ u8 flags; // bit 0: the record carries per-vertex colours
    /* 0x03 */ u8 code;
    /* 0x04 */ BattlePrimEntry entries[8]; // texture entries, then colour entries
    /* 0x24 */ u16 index[4]; // part indices func_800B1F6C transforms
} BattleModelPrim; // Size: 0x2C

// Expands a model frame's saved primitives into the GPU packets the frame is
// drawn from: the packet code and semi-transparency mode are applied here and
// func_800B1F6C fills in the transformed vertices. abr is the semi-transparency
// mode the texture pages are stamped with, zero meaning none.
void BattleExpandModelPrims(BattleModelFrame *pFrame, u8 *pDst, s32 mode, u8 shadeTex) {
    BattleModelPrim *pSrc;
    s32 count;
    s32 abr;
    s32 i;
    s32 kind;

    pSrc = (BattleModelPrim *)((u32)pFrame->primsOffset + (u32)pFrame);
    count = pFrame->primCount;
    // assigned in two steps on purpose: that is what keeps the mask's result in
    // the abr register instead of the loaded mode's
    if (mode != 0) {
        abr = (mode - 1) & 3;
        abr <<= 5;
    }
    for (i = 0; i != count; i++) {
        setlen(pDst, pSrc->outLen);
        setcode(pDst, pSrc->code);
        SetShadeTex(pDst, shadeTex);
        if (mode != 0) {
            SetSemiTrans(pDst, mode != 0);
        }
        kind = pSrc->code & BATTLE_PRIM_KIND_MASK;
        kind |= ((pSrc->flags ^ 1) & 1) << 8; // no colours: the unlit kind
        switch (kind) {
        case BATTLE_PRIM_GOURAUD:
        case BATTLE_PRIM_GOURAUD | BATTLE_PRIM_UNLIT: {
            POLY_G3 *pPoly = (POLY_G3 *)pDst;

            pPoly->r0 = pSrc->entries[0].color.r;
            pPoly->g0 = pSrc->entries[0].color.g;
            pPoly->b0 = pSrc->entries[0].color.b;
            pPoly->r1 = pSrc->entries[1].color.r;
            pPoly->g1 = pSrc->entries[1].color.g;
            pPoly->b1 = pSrc->entries[1].color.b;
            pPoly->r2 = pSrc->entries[2].color.r;
            pPoly->g2 = pSrc->entries[2].color.g;
            pPoly->b2 = pSrc->entries[2].color.b;
            break;
        }
        case BATTLE_PRIM_GOURAUD | BATTLE_PRIM_QUAD:
        case BATTLE_PRIM_GOURAUD | BATTLE_PRIM_QUAD | BATTLE_PRIM_UNLIT: {
            POLY_G4 *pPoly = (POLY_G4 *)pDst;

            pPoly->r0 = pSrc->entries[0].color.r;
            pPoly->g0 = pSrc->entries[0].color.g;
            pPoly->b0 = pSrc->entries[0].color.b;
            pPoly->r1 = pSrc->entries[1].color.r;
            pPoly->g1 = pSrc->entries[1].color.g;
            pPoly->b1 = pSrc->entries[1].color.b;
            pPoly->r2 = pSrc->entries[2].color.r;
            pPoly->g2 = pSrc->entries[2].color.g;
            pPoly->b2 = pSrc->entries[2].color.b;
            pPoly->r3 = pSrc->entries[3].color.r;
            pPoly->g3 = pSrc->entries[3].color.g;
            pPoly->b3 = pSrc->entries[3].color.b;
            break;
        }
        case 0:
        case BATTLE_PRIM_QUAD:
        case BATTLE_PRIM_UNLIT:
        case BATTLE_PRIM_QUAD | BATTLE_PRIM_UNLIT: {
            POLY_F3 *pPoly = (POLY_F3 *)pDst;

            pPoly->r0 = pSrc->entries[0].color.r;
            pPoly->g0 = pSrc->entries[0].color.g;
            pPoly->b0 = pSrc->entries[0].color.b;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_UNLIT: {
            POLY_FT3 *pPoly = (POLY_FT3 *)pDst;

            if (mode != 0) {
                pSrc->entries[1].tex.clut = (pSrc->entries[1].tex.clut & BATTLE_TPAGE_ABR_MASK) | abr;
            }
            pPoly->r0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->tpage = pSrc->entries[1].tex.clut;
            pPoly->clut = pSrc->entries[0].tex.clut;
            pPoly->u0 = pSrc->entries[0].tex.u;
            pPoly->v0 = pSrc->entries[0].tex.v;
            pPoly->u1 = pSrc->entries[1].tex.u;
            pPoly->v1 = pSrc->entries[1].tex.v;
            pPoly->u2 = pSrc->entries[2].tex.u;
            pPoly->v2 = pSrc->entries[2].tex.v;
            break;
        }
        case BATTLE_PRIM_TEXTURED: {
            POLY_FT3 *pPoly = (POLY_FT3 *)pDst;

            if (mode != 0) {
                pSrc->entries[1].tex.clut = (pSrc->entries[1].tex.clut & BATTLE_TPAGE_ABR_MASK) | abr;
            }
            pPoly->r0 = pSrc->entries[3].color.r;
            pPoly->g0 = pSrc->entries[3].color.g;
            pPoly->b0 = pSrc->entries[3].color.b;
            pPoly->tpage = pSrc->entries[1].tex.clut;
            pPoly->clut = pSrc->entries[0].tex.clut;
            pPoly->u0 = pSrc->entries[0].tex.u;
            pPoly->v0 = pSrc->entries[0].tex.v;
            pPoly->u1 = pSrc->entries[1].tex.u;
            pPoly->v1 = pSrc->entries[1].tex.v;
            pPoly->u2 = pSrc->entries[2].tex.u;
            pPoly->v2 = pSrc->entries[2].tex.v;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_GOURAUD | BATTLE_PRIM_UNLIT: {
            POLY_GT3 *pPoly = (POLY_GT3 *)pDst;

            if (mode != 0) {
                pSrc->entries[1].tex.clut = (pSrc->entries[1].tex.clut & BATTLE_TPAGE_ABR_MASK) | abr;
            }
            pPoly->r0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->r1 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g1 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b1 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->r2 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g2 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b2 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->tpage = pSrc->entries[1].tex.clut;
            pPoly->clut = pSrc->entries[0].tex.clut;
            pPoly->u0 = pSrc->entries[0].tex.u;
            pPoly->v0 = pSrc->entries[0].tex.v;
            pPoly->u1 = pSrc->entries[1].tex.u;
            pPoly->v1 = pSrc->entries[1].tex.v;
            pPoly->u2 = pSrc->entries[2].tex.u;
            pPoly->v2 = pSrc->entries[2].tex.v;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_GOURAUD: {
            POLY_GT3 *pPoly = (POLY_GT3 *)pDst;

            if (mode != 0) {
                pSrc->entries[1].tex.clut = (pSrc->entries[1].tex.clut & BATTLE_TPAGE_ABR_MASK) | abr;
            }
            pPoly->r0 = pSrc->entries[3].color.r;
            pPoly->g0 = pSrc->entries[3].color.g;
            pPoly->b0 = pSrc->entries[3].color.b;
            pPoly->r1 = pSrc->entries[4].color.r;
            pPoly->g1 = pSrc->entries[4].color.g;
            pPoly->b1 = pSrc->entries[4].color.b;
            pPoly->r2 = pSrc->entries[5].color.r;
            pPoly->g2 = pSrc->entries[5].color.g;
            pPoly->b2 = pSrc->entries[5].color.b;
            pPoly->tpage = pSrc->entries[1].tex.clut;
            pPoly->clut = pSrc->entries[0].tex.clut;
            pPoly->u0 = pSrc->entries[0].tex.u;
            pPoly->v0 = pSrc->entries[0].tex.v;
            pPoly->u1 = pSrc->entries[1].tex.u;
            pPoly->v1 = pSrc->entries[1].tex.v;
            pPoly->u2 = pSrc->entries[2].tex.u;
            pPoly->v2 = pSrc->entries[2].tex.v;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_QUAD | BATTLE_PRIM_UNLIT: {
            POLY_FT4 *pPoly = (POLY_FT4 *)pDst;

            if (mode != 0) {
                pSrc->entries[1].tex.clut = (pSrc->entries[1].tex.clut & BATTLE_TPAGE_ABR_MASK) | abr;
            }
            pPoly->r0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->tpage = pSrc->entries[1].tex.clut;
            pPoly->clut = pSrc->entries[0].tex.clut;
            pPoly->u0 = pSrc->entries[0].tex.u;
            pPoly->v0 = pSrc->entries[0].tex.v;
            pPoly->u1 = pSrc->entries[1].tex.u;
            pPoly->v1 = pSrc->entries[1].tex.v;
            pPoly->u2 = pSrc->entries[2].tex.u;
            pPoly->v2 = pSrc->entries[2].tex.v;
            pPoly->u3 = pSrc->entries[3].tex.u;
            pPoly->v3 = pSrc->entries[3].tex.v;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_QUAD: {
            POLY_FT4 *pPoly = (POLY_FT4 *)pDst;

            if (mode != 0) {
                pSrc->entries[1].tex.clut = (pSrc->entries[1].tex.clut & BATTLE_TPAGE_ABR_MASK) | abr;
            }
            pPoly->r0 = pSrc->entries[4].color.r;
            pPoly->g0 = pSrc->entries[4].color.g;
            pPoly->b0 = pSrc->entries[4].color.b;
            pPoly->tpage = pSrc->entries[1].tex.clut;
            pPoly->clut = pSrc->entries[0].tex.clut;
            pPoly->u0 = pSrc->entries[0].tex.u;
            pPoly->v0 = pSrc->entries[0].tex.v;
            pPoly->u1 = pSrc->entries[1].tex.u;
            pPoly->v1 = pSrc->entries[1].tex.v;
            pPoly->u2 = pSrc->entries[2].tex.u;
            pPoly->v2 = pSrc->entries[2].tex.v;
            pPoly->u3 = pSrc->entries[3].tex.u;
            pPoly->v3 = pSrc->entries[3].tex.v;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_QUAD | BATTLE_PRIM_GOURAUD | BATTLE_PRIM_UNLIT: {
            POLY_GT4 *pPoly = (POLY_GT4 *)pDst;

            if (mode != 0) {
                pSrc->entries[1].tex.clut = (pSrc->entries[1].tex.clut & BATTLE_TPAGE_ABR_MASK) | abr;
            }
            pPoly->r0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b0 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->r1 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g1 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b1 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->r2 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g2 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b2 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->r3 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->g3 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->b3 = BATTLE_PRIM_UNLIT_RGB;
            pPoly->tpage = pSrc->entries[1].tex.clut;
            pPoly->clut = pSrc->entries[0].tex.clut;
            pPoly->u0 = pSrc->entries[0].tex.u;
            pPoly->v0 = pSrc->entries[0].tex.v;
            pPoly->u1 = pSrc->entries[1].tex.u;
            pPoly->v1 = pSrc->entries[1].tex.v;
            pPoly->u2 = pSrc->entries[2].tex.u;
            pPoly->v2 = pSrc->entries[2].tex.v;
            pPoly->u3 = pSrc->entries[3].tex.u;
            pPoly->v3 = pSrc->entries[3].tex.v;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_QUAD | BATTLE_PRIM_GOURAUD: {
            POLY_GT4 *pPoly = (POLY_GT4 *)pDst;

            if (mode != 0) {
                pSrc->entries[1].tex.clut = (pSrc->entries[1].tex.clut & BATTLE_TPAGE_ABR_MASK) | abr;
            }
            pPoly->r0 = pSrc->entries[4].color.r;
            pPoly->g0 = pSrc->entries[4].color.g;
            pPoly->b0 = pSrc->entries[4].color.b;
            pPoly->r1 = pSrc->entries[5].color.r;
            pPoly->g1 = pSrc->entries[5].color.g;
            pPoly->b1 = pSrc->entries[5].color.b;
            pPoly->r2 = pSrc->entries[6].color.r;
            pPoly->g2 = pSrc->entries[6].color.g;
            pPoly->b2 = pSrc->entries[6].color.b;
            pPoly->r3 = pSrc->entries[7].color.r;
            pPoly->g3 = pSrc->entries[7].color.g;
            pPoly->b3 = pSrc->entries[7].color.b;
            pPoly->tpage = pSrc->entries[1].tex.clut;
            pPoly->clut = pSrc->entries[0].tex.clut;
            pPoly->u0 = pSrc->entries[0].tex.u;
            pPoly->v0 = pSrc->entries[0].tex.v;
            pPoly->u1 = pSrc->entries[1].tex.u;
            pPoly->v1 = pSrc->entries[1].tex.v;
            pPoly->u2 = pSrc->entries[2].tex.u;
            pPoly->v2 = pSrc->entries[2].tex.v;
            pPoly->u3 = pSrc->entries[3].tex.u;
            pPoly->v3 = pSrc->entries[3].tex.v;
            break;
        }
        }
        pDst += (pSrc->outLen + 1) * 4;
        pSrc = (BattleModelPrim *)((u8 *)pSrc + (pSrc->len + 1) * 4);
    }
}

void BattleModelScaleParts(BattleModelFrame *pFrame, s32 shift) {
    SVECTOR *pPart;
    s32 n;
    s32 i;

    if (pFrame->flags & 0x8000) {
        return;
    }
    pFrame->flags |= 0x8000;
    pPart = (SVECTOR *)((u32)pFrame->partsOffset + (u32)pFrame);
    n = pFrame->partCount;
    for (i = 0; i != n; i++) {
        pPart[i].vx <<= shift;
        pPart[i].vy <<= shift;
        pPart[i].vz <<= shift;
    }
}

// Emits the drawing-mode packet SetDrawTPage filled in at D_800C3BF8 as one more
// entry of the ordering table, and claims the bytes it takes from the packet
// buffer.
void BattleEmitDrawTPage(u8 *pOt) {
    u8 *pBuf = g_GfxCurWorkBuffer;

    if ((u8 *)(pBuf + sizeof(DR_TPAGE)) < g_GfxCurWorkBufferEnd) {
        g_GfxCurWorkBuffer = pBuf + sizeof(DR_TPAGE);
        *(DR_TPAGE *)pBuf = D_800C3BF8;
        AddPrim(pOt, pBuf);
    }
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1F6C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B2AEC);

void BattleNoOp2(void) {
}

void BattleNoOp3(void) {
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3358);

void BattleFreeScaleTask(void *pEntry) {
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
    g_BattleScaleTask = NULL;
}

// Returns the one scale task, allocating it on first use and restarting the
// ease from wherever the scale currently sits on every later call.
BattleScaleTask *BattleGetScaleTask(void) {
    BattleScaleTask *pTask;

    if (g_BattleScaleTask == NULL) {
        pTask = (BattleScaleTask *)TimerWorkListAllocateTask(NULL,
                    sizeof(BattleScaleTask) - sizeof(WorkListEntry));
        TimerWorkListSetTaskCallback(pTask, func_800B3358);
        WorkListTaskSetOnFreeCallback(pTask, BattleFreeScaleTask);
        pTask->start.vx = 0;
        pTask->start.vy = 0;
        pTask->start.vz = 0;
        g_BattleScaleTask = pTask;
    } else {
        pTask = g_BattleScaleTask;
        pTask->start.vx = pTask->current.vx;
        pTask->start.vy = pTask->current.vy;
        pTask->start.vz = pTask->current.vz;
    }
    return pTask;
}

void BattleCreateTimedScaleTask(BattleScaleVector *pTarget, s32 frames) {
    BattleScaleTask *pTask = BattleGetScaleTask();

    pTask->target.vx = pTarget->vx;
    pTask->target.vy = pTarget->vy;
    pTask->target.vz = pTarget->vz;
    pTask->framesLeft = frames * 2;
    pTask->halfLength = frames * 2;
    func_800B3358(pTask);
}

// One step of the fade: while the timer runs, move the colour from the start
// toward the target in equal slices, and once it is spent settle on the target
// and release the task if the fade has ended on black.
void BattleStepScreenFade(void *pArg) {
    BattleScreenFadeTask *pTask = pArg;
    s32 step[3];

    if (pTask->timer == 0) {
        pTask->curR = pTask->targetR;
        pTask->curG = pTask->targetG;
        pTask->curB = pTask->targetB;
        if ((pTask->curR | pTask->curG | pTask->curB) == 0) {
            g_BattleScreenFadeTask->timerEntry.onFreeCallback(g_BattleScreenFadeTask);
        }
        return;
    }
    pTask->timer--;
    step[0] = pTask->targetR - pTask->startR;
    step[1] = pTask->targetG - pTask->startG;
    step[2] = pTask->targetB - pTask->startB;
    gte_lddp(pTask->timer >> 1);
    gte_ldlvl(step);
    gte_gpf0();
    gte_stlvl(step);
    step[0] = step[0] / (pTask->length >> 1);
    step[1] = step[1] / (pTask->length >> 1);
    step[2] = step[2] / (pTask->length >> 1);
    pTask->curR = pTask->targetR - step[0];
    pTask->curG = pTask->targetG - step[1];
    pTask->curB = pTask->targetB - step[2];
}

void BattleFreeScreenFadeTask(u8 *pEntry) {
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    g_BattleScreenFadeTask = NULL;
}

// Draws the fade as one semi-transparent quad over the whole screen. The 0x50
// asked of the work buffer is more than the quad and its draw mode take.
void BattleDrawScreenFade(WorkListEntry *pEntry) {
    BattleScreenFadeTask *pTask = pEntry->unk4;
    u8 *pBuf = g_GfxCurWorkBuffer;
    POLY_F4 *pPoly;
    DR_MODE *pMode;

    if ((u8 *)(pBuf + 0x50) >= g_GfxCurWorkBufferEnd) {
        return;
    }
    pPoly = (POLY_F4 *)pBuf;
    pMode = (DR_MODE *)(pBuf + sizeof(POLY_F4));
    g_GfxCurWorkBuffer = pBuf + sizeof(POLY_F4) + sizeof(DR_MODE);
    SetPolyF4(pPoly);
    SetSemiTrans(pPoly, 1);
    pPoly->r0 = pTask->curR;
    pPoly->g0 = pTask->curG;
    pPoly->b0 = pTask->curB;
    pPoly->x0 = -0x20;
    pPoly->y0 = -0x20;
    pPoly->x1 = 0x140;
    pPoly->y1 = -0x20;
    pPoly->x2 = -0x20;
    pPoly->y2 = 0xF0;
    pPoly->x3 = 0x140;
    pPoly->y3 = 0xF0;
    SetDrawMode(pMode, 0, 0, GetTPage(0, pTask->abr, 0, 0), NULL);
    AddPrim(g_GfxCurOT + 2, pPoly);
    AddPrim(g_GfxCurOT + 2, pMode);
}

void BattleStartScreenFadeOwnBuffer(s32 a0, s32 a1, u8 a2, u8 a3, u8 a4) {
    g_BattleScreenFadeOwnBuffer = 1;
    func_800B39C0(a0, a1, a2, a3, a4);
    g_BattleScreenFadeOwnBuffer = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B39C0);

s32 BattleGetActorFlag41(void) {
    if (g_BattleScreenFadeTask != NULL) {
        return g_BattleScreenFadeTask->abr;
    }
    return 1;
}

void BattleStepImageTaskValue(BattleImageTask *pArg) {
    BattleImageTask *pTask = pArg;
    void (*pOnFree)(void *);
    s32 timer;
    s32 n;
    s32 num;
    s32 base;
    s32 delta;

    timer = pTask->timer;
    if (timer != 0) {
        n = timer - 1;
        num = n * 0x20 / pTask->length;
        pArg->timer = n;
        base = pTask->end;
        delta = (base - pTask->start);
        // one statement per step: folding the multiply, the divide and the
        // subtraction together puts each result in a fresh register
        delta = delta * num;
        delta = delta / 0x20;
        delta = base - delta;
        pTask->current = delta;
    } else if (pTask->current == 0) {
        pOnFree = pTask->timerEntry.onFreeCallback;
        pOnFree(pTask);
    }
}
void BattleFreeImageTask(u8 *pEntry) {
    WorkListRemoveTask(pEntry + 0x1C);
    TimerWorkListRemoveTask(pEntry);
    HeapFree(pEntry);
    g_BattleImageTask = 0;
    BattleUploadAndFreeImageResources();
}

// Re-applies the colour tint the image task eases whenever its value has moved
// since the last application.
void BattleApplyImageTaskTint(WorkListEntry *pEntry) {
    BattleImageTask *pTask = pEntry->unk4;

    if (pTask->applied != pTask->current) {
        pTask->applied = *(u16 *)&pTask->current;
        BattleSetActorRectParams(0, pTask->blendMode, 0x20 - pTask->current,
                      pTask->r, pTask->g, pTask->b);
    }
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3CD4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3E04);
