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

// A three-component scale as the timed scale task keeps it: signed, because the
// scale it eases to can be negative and the step subtracts one from the other.
typedef struct {
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ s16 pad6;
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
// fade and the colour it is at on this frame. g_BattleOwnedScreenFadeTask and g_BattleStaticScreenFadeTask are the
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
// the two statically allocated fades: one the overlay owns, one shared
extern BattleScreenFadeTask g_BattleOwnedScreenFadeTask;
extern BattleScreenFadeTask g_BattleStaticScreenFadeTask;
extern BattleScreenFadeTask *g_BattleOwnedScreenFade; // the owned one, once it is set up
// the scale the frame being drawn is drawn at, mirrored to the other field of
// the interlaced pair on every other frame
extern s16 D_800C354C;
extern s16 D_800C354E;
extern s16 D_800C3550;
extern u8 g_BattleScreenFadeBlocked; // set while a fade is already being driven outside the battle
extern u8 g_FlagTimerWorkListAdds;
extern s32 g_NumFlaggedTimerWorkListEntries;

extern u8 *g_GfxCurWorkBuffer;
extern u8 *g_GfxCurWorkBufferEnd;
extern u_long *g_GfxCurOT;

extern BattleScaleTask *BattleGetScaleTask();
extern void BattleStepScaleTask(BattleScaleTask *pTask);
extern void BattleStartScreenFade(s32 frames, s32 abr, u8 r, u8 g, u8 b);
extern void BattleSetActorRectParams(s32, s32, s32, s32, s32, s32);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void WorkListAddTask(WorkListEntry *pData, WorkListEntry *pEntry);
extern void TimerWorkListAddTask(WorkListEntry *pData, WorkListEntry *pEntry);
extern void WorkListSetTaskCallback(WorkListEntry *pTask, void *pCallback);
extern void BattleUploadAndFreeImageResources(void);
extern WorkListEntry *TimerWorkListAllocateTask();
extern void TimerWorkListSetTaskCallback();
extern void WorkListTaskSetOnFreeCallback();

extern u8 *g_BattlePrimCursor;
extern s32 g_BattlePrimCount;
extern DR_TPAGE g_BattleDrawTPage;
extern s32 SpriteColorAddClamp(s32 color, s32 value);

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
    u8 *pCmd = g_BattlePrimCursor;
    u8 size = pCmd[1];

    g_BattlePrimCount += 1;
    g_BattlePrimCursor = pCmd + (size + 1) * 4;
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

// Emits the drawing-mode packet SetDrawTPage filled in at g_BattleDrawTPage as one more
// entry of the ordering table, and claims the bytes it takes from the packet
// buffer.
void BattleEmitDrawTPage(u8 *pOt) {
    u8 *pBuf = g_GfxCurWorkBuffer;

    if ((u8 *)(pBuf + sizeof(DR_TPAGE)) < g_GfxCurWorkBufferEnd) {
        g_GfxCurWorkBuffer = pBuf + sizeof(DR_TPAGE);
        *(DR_TPAGE *)pBuf = g_BattleDrawTPage;
        AddPrim(pOt, pBuf);
    }
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B1F6C);

// Decoded from the .s (not written): 765 insns, 23 GTE case bodies and its own
// 29-word jtbl. Scratch notes, cheapest first:
//
// Prototype is the one temp2.c already declares: '(void *pFrame, u8 *pDst, u_long *pOT, int
//   flags, s32 depthBias, s32 shadeMode)' → a0–a3 + two stack args; 'depthBias' at 0x98 (callers
//   pass 0xFEC), 'shadeMode' at 0x9C (callers pass 'flags>>5'); a3 is never read. Frame 0x88,
//   eight callee-saved + '$fp' (0xFFFFFF / 0xFF000000 masks).
// Frame fields read by this function: 0x0 SVECTOR part table (index>>'*8'), 0x8 normal table
//   (the 'NormalColor'/'NormalColor3' base — this is 'BattleModelFrame.unk8''s first word,
//   evidence it is a normals offset), 0x10 prim records, 0x14 record count.
// 'shadeMode != 0' → 'SetDrawTPage(&g_BattleDrawTPage, 0, 0, ((shadeMode-1)&3)<<5)' at entry and
//   a 'BattleEmitDrawTPage(&pOT[otIndex])' after each packet link; the OT link is written as two
//   mask assignments ('*pDst = (*pDst&0xFF000000)|(*ot&0xFFFFFF); *ot =
//   (*ot&0xFF000000)|((u32)pDst&0xFFFFFF);') — the second read of '*ot' is a real reload.
// Per record: 'kind = (code & 0x1C) | ((flags^1)&1)<<8' → first switch (15 cases, comparison
//   tree, exactly BattleExpandModelPrims' 'kind' values) picks 3 or 4 vertex pointers out of
//   'rec->index[]' (u16 at 0x8; index groups 0x8/0xA/0xC, 0x14–0x1A, 0x18–0x1E, 0x24–0x2A, …).
//   The 0x100-bit kinds call 'NormalColor(&normals[index[0]], &pDst->rgb0)' — 0x114 calls
//   'NormalColor3(..., &pDst+4, +0x10, +0x1C)' — saving/restoring the packet code byte ('lbu/sb
//   0x7($s2)') around the call.
// Second switch on 'code & 0x1C' ('jtbl_800707DC', 29 words, indexed by the raw masked value,
//   live entries 0,4,8,0xC,0x10,0x14,0x18,0x1C): 3-vertex cases inline
//   'gte_ldv3'/'gte_rtpt'/'gte_stflg_val'/'gte_nclip' (bail on flag bit 0x4000)/'gte_stopz'
//   (saves MAC0 *before* AVSZ3 overwrites it, then reloads it and bails on '<=
//   0')/'gte_avsz3'/'gte_stotz' → 'otIndex = (OTZ >> g_GfxOTDepthShift) + depthBias', clamp min
//   5, bail at 0x1000, then 'swc2 $12/$13/$14' into the packet's x/y words; 4-vertex cases call
//   'RotAverageNclip4(v0..v3, (long*)(pDst+8…), &p, &otz, &flag)' and test flag bit 0x8000. The
//   different cases use four different sxy spacings (8/0xC/0x10, 8/0x10/0x18, 8/0x14/0x20, and
//   the 4-vertex 8/0xC/0x10/0x14 and 8/0x10/0x18/0x20 variants), so check
//   'include/psyq/libgpu.h''s POLY_* offsets per case before writing them.
// Loop tail: 'pDst += (outLen+1)*4; pRec += (len+1)*4;' with the record counter living in a
//   *stack slot* (0x58) and '&otz' cached in another (0x5C) — i.e. an address-taken local, not a
//   register.
// What I changed: two comment lines above the two 'INCLUDE_ASM' stubs recording the stack-switch
//   evidence (convention-precedent: 'rendering_pause_text.c:23', 'field/misc2.c:1190'). No code
//   change; the unit still has its 3 stubs.
// Skipped/not checked: 'func_800B1F6C' was never compiled, so there is no percentage for it; and
//   I did not re-derive the POLY_* per-case offsets, which is the first thing its writer needs.
//   The tree is left in matching mode by 'check_bin.sh', as the brief allows at end of batch.

// Applies the model's tint to the colour bytes of every packet a frame expands
// to, one channel sum per vertex, and mirrors them into the packet copy the
// second render context draws from. Only the colours are written here: the
// vertex and texture coordinates come from the passes that call func_800B1F6C.
void BattleShadeModelPrims(BattleModelFrame *pFrame, u8 *pDst, u8 *pDstCopy, s32 addR, s32 addG, s32 addB) {
    BattleModelPrim *pSrc;
    s32 count;
    s32 i;
    s32 kind;

    pSrc = (BattleModelPrim *)((u32)pFrame->primsOffset + (u32)pFrame);
    count = pFrame->primCount;
    for (i = 0; i != count; i++) {
        kind = pSrc->code & BATTLE_PRIM_KIND_MASK;
        kind |= ((pSrc->flags ^ 1) & 1) << 8; // no colours: the unlit kind
        switch (kind) {
        case 0: {
            POLY_F3 *pPoly = (POLY_F3 *)pDst;
            POLY_F3 *pCopy = (POLY_F3 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(pSrc->entries[0].color.r, addR);
            pPoly->g0 = SpriteColorAddClamp(pSrc->entries[0].color.g, addG);
            pPoly->b0 = SpriteColorAddClamp(pSrc->entries[0].color.b, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            break;
        }
        case BATTLE_PRIM_GOURAUD: {
            POLY_G3 *pPoly = (POLY_G3 *)pDst;
            POLY_G3 *pCopy = (POLY_G3 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(pSrc->entries[0].color.r, addR);
            pPoly->g0 = SpriteColorAddClamp(pSrc->entries[0].color.g, addG);
            pPoly->b0 = SpriteColorAddClamp(pSrc->entries[0].color.b, addB);
            pPoly->r1 = SpriteColorAddClamp(pSrc->entries[1].color.r, addR);
            pPoly->g1 = SpriteColorAddClamp(pSrc->entries[1].color.g, addG);
            pPoly->b1 = SpriteColorAddClamp(pSrc->entries[1].color.b, addB);
            pPoly->r2 = SpriteColorAddClamp(pSrc->entries[2].color.r, addR);
            pPoly->g2 = SpriteColorAddClamp(pSrc->entries[2].color.g, addG);
            pPoly->b2 = SpriteColorAddClamp(pSrc->entries[2].color.b, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            pCopy->r1 = pPoly->r1;
            pCopy->g1 = pPoly->g1;
            pCopy->b1 = pPoly->b1;
            pCopy->r2 = pPoly->r2;
            pCopy->g2 = pPoly->g2;
            pCopy->b2 = pPoly->b2;
            break;
        }
        case BATTLE_PRIM_GOURAUD | BATTLE_PRIM_QUAD: {
            POLY_G4 *pPoly = (POLY_G4 *)pDst;
            POLY_G4 *pCopy = (POLY_G4 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(pSrc->entries[0].color.r, addR);
            pPoly->g0 = SpriteColorAddClamp(pSrc->entries[0].color.g, addG);
            pPoly->b0 = SpriteColorAddClamp(pSrc->entries[0].color.b, addB);
            pPoly->r1 = SpriteColorAddClamp(pSrc->entries[1].color.r, addR);
            pPoly->g1 = SpriteColorAddClamp(pSrc->entries[1].color.g, addG);
            pPoly->b1 = SpriteColorAddClamp(pSrc->entries[1].color.b, addB);
            pPoly->r2 = SpriteColorAddClamp(pSrc->entries[2].color.r, addR);
            pPoly->g2 = SpriteColorAddClamp(pSrc->entries[2].color.g, addG);
            pPoly->b2 = SpriteColorAddClamp(pSrc->entries[2].color.b, addB);
            pPoly->r3 = SpriteColorAddClamp(pSrc->entries[3].color.r, addR);
            pPoly->g3 = SpriteColorAddClamp(pSrc->entries[3].color.g, addG);
            pPoly->b3 = SpriteColorAddClamp(pSrc->entries[3].color.b, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            pCopy->r1 = pPoly->r1;
            pCopy->g1 = pPoly->g1;
            pCopy->b1 = pPoly->b1;
            pCopy->r2 = pPoly->r2;
            pCopy->g2 = pPoly->g2;
            pCopy->b2 = pPoly->b2;
            pCopy->r3 = pPoly->r3;
            pCopy->g3 = pPoly->g3;
            pCopy->b3 = pPoly->b3;
            break;
        }
        case BATTLE_PRIM_QUAD: {
            POLY_F4 *pPoly = (POLY_F4 *)pDst;
            POLY_F4 *pCopy = (POLY_F4 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(pSrc->entries[0].color.r, addR);
            pPoly->g0 = SpriteColorAddClamp(pSrc->entries[0].color.g, addG);
            pPoly->b0 = SpriteColorAddClamp(pSrc->entries[0].color.b, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_UNLIT: {
            POLY_FT3 *pPoly = (POLY_FT3 *)pDst;
            POLY_FT3 *pCopy = (POLY_FT3 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            break;
        }
        case BATTLE_PRIM_TEXTURED: {
            POLY_FT3 *pPoly = (POLY_FT3 *)pDst;
            POLY_FT3 *pCopy = (POLY_FT3 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(pSrc->entries[3].color.r, addR);
            pPoly->g0 = SpriteColorAddClamp(pSrc->entries[3].color.g, addG);
            pPoly->b0 = SpriteColorAddClamp(pSrc->entries[3].color.b, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_GOURAUD | BATTLE_PRIM_UNLIT: {
            POLY_GT3 *pPoly = (POLY_GT3 *)pDst;
            POLY_GT3 *pCopy = (POLY_GT3 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pPoly->r1 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g1 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b1 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pPoly->r2 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g2 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b2 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            pCopy->r1 = pPoly->r1;
            pCopy->g1 = pPoly->g1;
            pCopy->b1 = pPoly->b1;
            pCopy->r2 = pPoly->r2;
            pCopy->g2 = pPoly->g2;
            pCopy->b2 = pPoly->b2;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_GOURAUD: {
            POLY_GT3 *pPoly = (POLY_GT3 *)pDst;
            POLY_GT3 *pCopy = (POLY_GT3 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(pSrc->entries[3].color.r, addR);
            pPoly->g0 = SpriteColorAddClamp(pSrc->entries[3].color.g, addG);
            pPoly->b0 = SpriteColorAddClamp(pSrc->entries[3].color.b, addB);
            pPoly->r1 = SpriteColorAddClamp(pSrc->entries[4].color.r, addR);
            pPoly->g1 = SpriteColorAddClamp(pSrc->entries[4].color.g, addG);
            pPoly->b1 = SpriteColorAddClamp(pSrc->entries[4].color.b, addB);
            pPoly->r2 = SpriteColorAddClamp(pSrc->entries[5].color.r, addR);
            pPoly->g2 = SpriteColorAddClamp(pSrc->entries[5].color.g, addG);
            pPoly->b2 = SpriteColorAddClamp(pSrc->entries[5].color.b, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            pCopy->r1 = pPoly->r1;
            pCopy->g1 = pPoly->g1;
            pCopy->b1 = pPoly->b1;
            pCopy->r2 = pPoly->r2;
            pCopy->g2 = pPoly->g2;
            pCopy->b2 = pPoly->b2;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_QUAD | BATTLE_PRIM_UNLIT: {
            POLY_FT4 *pPoly = (POLY_FT4 *)pDst;
            POLY_FT4 *pCopy = (POLY_FT4 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_QUAD: {
            POLY_FT4 *pPoly = (POLY_FT4 *)pDst;
            POLY_FT4 *pCopy = (POLY_FT4 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(pSrc->entries[4].color.r, addR);
            pPoly->g0 = SpriteColorAddClamp(pSrc->entries[4].color.g, addG);
            pPoly->b0 = SpriteColorAddClamp(pSrc->entries[4].color.b, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_QUAD | BATTLE_PRIM_GOURAUD | BATTLE_PRIM_UNLIT: {
            POLY_GT4 *pPoly = (POLY_GT4 *)pDst;
            POLY_GT4 *pCopy = (POLY_GT4 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b0 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pPoly->r1 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g1 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b1 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pPoly->r2 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g2 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b2 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pPoly->r3 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addR);
            pPoly->g3 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addG);
            pPoly->b3 = SpriteColorAddClamp(BATTLE_PRIM_UNLIT_RGB, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            pCopy->r1 = pPoly->r1;
            pCopy->g1 = pPoly->g1;
            pCopy->b1 = pPoly->b1;
            pCopy->r2 = pPoly->r2;
            pCopy->g2 = pPoly->g2;
            pCopy->b2 = pPoly->b2;
            pCopy->r3 = pPoly->r3;
            pCopy->g3 = pPoly->g3;
            pCopy->b3 = pPoly->b3;
            break;
        }
        case BATTLE_PRIM_TEXTURED | BATTLE_PRIM_QUAD | BATTLE_PRIM_GOURAUD: {
            POLY_GT4 *pPoly = (POLY_GT4 *)pDst;
            POLY_GT4 *pCopy = (POLY_GT4 *)pDstCopy;

            pPoly->r0 = SpriteColorAddClamp(pSrc->entries[4].color.r, addR);
            pPoly->g0 = SpriteColorAddClamp(pSrc->entries[4].color.g, addG);
            pPoly->b0 = SpriteColorAddClamp(pSrc->entries[4].color.b, addB);
            pPoly->r1 = SpriteColorAddClamp(pSrc->entries[5].color.r, addR);
            pPoly->g1 = SpriteColorAddClamp(pSrc->entries[5].color.g, addG);
            pPoly->b1 = SpriteColorAddClamp(pSrc->entries[5].color.b, addB);
            pPoly->r2 = SpriteColorAddClamp(pSrc->entries[6].color.r, addR);
            pPoly->g2 = SpriteColorAddClamp(pSrc->entries[6].color.g, addG);
            pPoly->b2 = SpriteColorAddClamp(pSrc->entries[6].color.b, addB);
            pPoly->r3 = SpriteColorAddClamp(pSrc->entries[7].color.r, addR);
            pPoly->g3 = SpriteColorAddClamp(pSrc->entries[7].color.g, addG);
            pPoly->b3 = SpriteColorAddClamp(pSrc->entries[7].color.b, addB);
            pCopy->r0 = pPoly->r0;
            pCopy->g0 = pPoly->g0;
            pCopy->b0 = pPoly->b0;
            pCopy->r1 = pPoly->r1;
            pCopy->g1 = pPoly->g1;
            pCopy->b1 = pPoly->b1;
            pCopy->r2 = pPoly->r2;
            pCopy->g2 = pPoly->g2;
            pCopy->b2 = pPoly->b2;
            pCopy->r3 = pPoly->r3;
            pCopy->g3 = pPoly->g3;
            pCopy->b3 = pPoly->b3;
            break;
        }
        }
        pDst += (pSrc->outLen + 1) * 4;
        pDstCopy += (pSrc->outLen + 1) * 4;
        pSrc = (BattleModelPrim *)((u8 *)pSrc + (pSrc->len + 1) * 4);
    }
}

void BattleNoOp2(void) {
}

void BattleNoOp3(void) {
}

// One step of the scale ease: while the countdown runs, move the scale from
// where it started toward the target by an equal slice a frame, taking the slice
// out of the countdown left; once it is spent settle on the target. The scale is
// then published, negated on every other frame for the interlaced draw, and the
// task releases itself once it has settled at a zero scale with nothing left.
void BattleStepScaleTask(BattleScaleTask *pTask) {
    s32 step[3];

    if (pTask->framesLeft == 0) {
        pTask->current.vx = pTask->target.vx;
        pTask->current.vy = pTask->target.vy;
        pTask->current.vz = pTask->target.vz;
    } else {
        pTask->framesLeft--;
        step[0] = pTask->target.vx - pTask->start.vx;
        step[1] = pTask->target.vy - pTask->start.vy;
        step[2] = pTask->target.vz - pTask->start.vz;
        gte_lddp(pTask->framesLeft >> 1);
        gte_ldlvl(step);
        gte_gpf0();
        gte_stlvl(step);
        step[0] = step[0] / (pTask->halfLength >> 1);
        step[1] = step[1] / (pTask->halfLength >> 1);
        step[2] = step[2] / (pTask->halfLength >> 1);
        pTask->current.vx = pTask->target.vx - step[0];
        pTask->current.vy = pTask->target.vy - step[1];
        pTask->current.vz = pTask->target.vz - step[2];
    }
    pTask->frame++;
    if (pTask->frame & 2) {
        D_800C354C = -pTask->current.vx;
    } else {
        D_800C354C = pTask->current.vx;
    }
    if (pTask->frame & 2) {
        D_800C354E = -pTask->current.vy;
    } else {
        D_800C354E = pTask->current.vy;
    }
    if (pTask->frame & 2) {
        D_800C3550 = -pTask->current.vz;
    } else {
        D_800C3550 = pTask->current.vz;
    }
    if (pTask->current.vx == 0 && pTask->current.vy == 0 && pTask->current.vz == 0 && pTask->framesLeft == 0) {
        pTask->entry.onFreeCallback(pTask);
    }
}

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
        TimerWorkListSetTaskCallback(pTask, BattleStepScaleTask);
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
    BattleStepScaleTask(pTask);
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
    BattleStartScreenFade(a0, a1, a2, a3, a4);
    g_BattleScreenFadeOwnBuffer = 0;
}

// Starts a screen fade: on the first call the work-list task is set up and the
// fade runs from black, on a later one it restarts from the colour the last fade
// ended at. The task's length is twice the frame count the caller asks for,
// because the step counts it down twice per frame. Nothing happens while another
// subsystem is already driving a fade.
void BattleStartScreenFade(s32 frames, s32 abr, u8 r, u8 g, u8 b) {
    BattleScreenFadeTask *pTask;
    WorkListEntry *pDraw;

    if (g_BattleScreenFadeBlocked != 0) {
        return;
    }
    frames *= 2;
    if (g_BattleScreenFadeOwnBuffer != 0) {
        if (g_BattleScreenFadeTask == NULL) {
            pTask = &g_BattleOwnedScreenFadeTask;
            g_BattleOwnedScreenFade = pTask;
            goto created;
        }
        pTask = g_BattleOwnedScreenFade;
        goto restart;
    } else if (g_BattleScreenFadeTask == NULL) {
        pTask = &g_BattleStaticScreenFadeTask;
        g_BattleScreenFadeTask = pTask;
    } else {
        pTask = g_BattleScreenFadeTask;
        goto restart;
    }
created:
    TimerWorkListAddTask(NULL, &pTask->timerEntry);
    pDraw = &pTask->drawEntry;
    WorkListAddTask(&pTask->timerEntry, pDraw);
    pTask->timerEntry.unk14 &= 0x7FFFFFFF; // clears the flagged bit the work list just set
    if (g_FlagTimerWorkListAdds != 0) {
        g_NumFlaggedTimerWorkListEntries--;
    }
    TimerWorkListSetTaskCallback(pTask, BattleStepScreenFade);
    WorkListSetTaskCallback(pDraw, BattleDrawScreenFade);
    WorkListTaskSetOnFreeCallback(pTask, BattleFreeScreenFadeTask);
    pTask->timerEntry.unk4 = pTask;
    pTask->drawEntry.unk4 = pTask;
    pTask->unk40 = 0;
    pTask->startR = 0;
    pTask->startG = 0;
    pTask->startB = 0;
    goto setTarget;
restart:
    pTask->startR = pTask->curR;
    pTask->startG = pTask->curG;
    pTask->startB = pTask->curB;
setTarget:
    pTask->abr = abr;
    pTask->targetR = r;
    pTask->targetG = g;
    pTask->targetB = b;
    pTask->length = frames;
    pTask->timer = frames;
    BattleStepScreenFade(pTask);
}

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


// Switches the stack into its HeapAlloc block around func_800A5EB4 (sw $sp,0($t0),
// then sp = the block; the old sp read back with lw $sp,0($sp)); no C statement
// emits it.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3CD4);
// Same alternate-stack idiom: HeapAlloc 0x1000, sp moved to the block at +0xEFC,
// the three MoveImage calls made on it, then sp read back out of the block.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc26", func_800B3E04);
