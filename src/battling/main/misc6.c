#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8 g_BattleDrawBufferIndex;
extern u8 g_BattleSpriteTimer;
extern u8 g_BattleTextRed;
extern u8 g_BattleTextGreen;
extern u8 g_BattleTextBlue;
extern u8 g_BattleTextSpriteEntry0;
extern u8 g_BattleTextSpriteEntry1;
extern s32 g_BattleTextEntryCursor;
extern s32 g_BattleTextScale;
extern s32 g_SpuChannelMask;
extern s16 g_BattleTextCursorX;
extern s16 g_BattleTextCursorY;
extern u8 D_8009791B;
extern u8 D_80099037;
extern u8 D_800954E0[];
extern s32 g_BattleDrawFlags;

void func_8007E964(s32 c);
u8* BattleGetFontGlyph(s32 c);
s32 BattleGetTextWidth(u8* pText);
extern s32 g_BattleTextPrimCount;

void BattlePlaySoundEffectById(s32 arg);
void BattleAppendExtraQueueEntry(void);

extern POLY_FT4* g_BattleTextPrimBuffers[];
extern u16 g_BattleTextTpage;
extern u16 g_BattleTextClut;

// Battle setup state; 0x3C and 0x64 point at the two TIM archives loaded here.
typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ u_long* unk3C;
    /* 0x40 */ u8 unk40[0x24];
    /* 0x64 */ u_long* unk64;
} BattleTimState;

// 0x1C-byte draw record: a DR_TPAGE followed by the SPRT BattleDrawTimedSpritePrims queues.
typedef struct {
    /* 0x00 */ DR_TPAGE tpage;
    /* 0x08 */ SPRT sprt;
} BattleDrawRecord; // Size: 0x1C

extern BattleDrawRecord g_BattleTextSpriteRecords[];

// 0x20-byte entry of the D_80092874 table; func_8007EEE8 reads only the s16 at 0x4.
typedef struct {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ s16 requiredProgress; // entry is queued once g_GameState.fieldScriptMemory[0] reaches it
    /* 0x06 */ u8 unk6[0x1A];
} BattleFilterEntry; // Size: 0x20

// 0xC-byte record whose address func_8007EEE8 queues into g_BattleEntries.
typedef struct {
    /* 0x00 */ u8 unk0[0xC];
} BattleQueueRecord; // Size: 0xC

extern BattleFilterEntry* D_80092874;
extern BattleQueueRecord D_80091964[];
extern s32** g_BattleEntries;
extern s32 g_BattleEntryCount;

// 0xC-byte entry of the slot table BattleInitSpriteBuffers (main/misc5) allocates
// at g_BattleSpriteSlots; this file only clears the head of an entry.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ s16 y; // clamped with a signed compare by the particle drawer
    /* 0x4 */ u16 z;
    /* 0x6 */ s16 life; // 0 marks an unused entry; counts down every frame
    /* 0x8 */ s8 vx; // per-frame step added to x
    /* 0x9 */ s8 vz; // per-frame step added to z
    /* 0xA */ u16 vy; // added to y every frame and itself gains 2: gravity
} BattleSpriteSlot; // Size: 0xC

// 0xA-byte entry of the table BattleInitSpriteEntryBuffers (main/misc5) allocates at g_BattleSpriteEntries.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 z;
    /* 0x6 */ s8 life; // 0 marks a free entry
    /* 0x7 */ s8 vy; // added to y every frame: gravity
    /* 0x8 */ u16 groundY; // the y the entry falls back to
} BattleSpriteEntry; // Size: 0xA

extern BattleSpriteSlot* g_BattleSpriteSlots;
extern BattleSpriteEntry* g_BattleSpriteEntries;

// The first 0x10 bytes are a flat-shaded LINE_F2: the tag word (its low 24 bits
// are the OT link, so only the length byte is a field of its own), the r/g/b word
// and the two projected XY words.
typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 len;
    /* 0x04 */ u8 r;
    /* 0x05 */ u8 g;
    /* 0x06 */ u8 b;
    /* 0x07 */ u8 code;
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ SVECTOR v0;
    /* 0x18 */ SVECTOR v1;
} BattleQuad; // Size: 0x20

// An 8-byte, byte-aligned chunk: assigning one copies 8 bytes with lwl/lwr.
typedef struct {
    u8 unk[8];
} Bytes8;

extern BattleQuad g_BattleLineQuads[100];
extern s32 g_BattleLineQuadQueueCount;
extern s32 g_BattleLineQuadDrawCount;

// 0x14-byte sprite record of the 7x7 table BattleInitSpriteRectTable allocates at g_BattleSpriteRects.
// The head is the pair of RECTs BattleLoadSelectionTextures (main/misc7) LoadImages
// out of the sprite archive: the CLUT rect, then the image rect.
typedef struct {
    /* 0x00 */ s16 clutX;
    /* 0x02 */ s16 clutY;
    /* 0x04 */ u16 clutW;
    /* 0x06 */ u16 clutH;
    /* 0x08 */ s16 texX;
    /* 0x0A */ s16 texY;
    /* 0x0C */ u16 texW;
    /* 0x0E */ u16 texH;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 unk12;
} BattleSprite; // Size: 0x14

extern BattleSprite* g_BattleSpriteRects;
extern void* g_BattleRootOrderingTable;

// Vertex staged in the GTE scratch block. The components are subtracted as
// unsigned halves, which is how the original loads them (lhu).
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 pad;
} BattleStageVertex; // Size: 0x8

// GTE scratch block at 0x1F800000: the three g_BattleViewOrigin words every
// drawer subtracts, the pair BattleDrawLineQuads stages for its RTPT, and the SZ
// read-back slot func_8007E020 reads at 0xB0.
typedef struct {
    /* 0x00 */ s32 viewX;
    /* 0x04 */ s32 viewY;
    /* 0x08 */ s32 viewZ;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ BattleStageVertex vec;
    /* 0x18 */ BattleStageVertex v0;
    /* 0x20 */ BattleStageVertex v1;
    /* 0x28 */ u8 unk28[0x88];
    /* 0xB0 */ s32 otz;
} BattleGteScratch; // Size: 0xB4

// 0x10-byte 2D primitive the particle renderer links three at a time; only the
// tag word and the projected XY at 0x8 are written here.
typedef struct {
    u32 tag; // 0x0
    u32 unk4; // 0x4
    u32 xy; // 0x8
    u32 unkC; // 0xC
} BattleParticleQuad; // Size: 0x10

extern BattleParticleQuad* g_BattleSpritePrimBuffers[];


// Projects the live particles three at a time with RTPT and links one screen
// prim per particle into the OT by its projected Z.
INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E020);
// Best 99.78% (556 bytes = retail's exact size, 138 insns, 6 differing
// instructions). Re-measured round 403 by un-parking; the number stands.
// The ONLY residual is the OR that fills each prim's OT tag word, three times:
//     retail: lw a0,0(v0) ; and v1,a2,t2 ; sw v1,0(v0) ; or a0,a0,t7 ; sw a0,0(v1)
//     ours:   lw a0,0(v0) ; and v1,a2,t2 ; sw v1,0(v0) ; or v0,a0,t7 ; sw v0,0(v1)
// Every other instruction is retail's, including the 8-byte frame (s0 only) and
// the whole t-register map: t1 = 0x1F800000, t2 = mask, t3 = 0x1F8000B0,
// t4 = vec, t5 = i, t6 = pOt, t7 = 0x3000000, t8 = viewZ, t9 = viewY,
// s0 = viewX, and a1/a2/a3 = pParticle+6 / pQuad / pParticle. All 9 t regs are
// live through the loop, so this is a global_alloc tie: the OR's result pseudo
// and the loaded link pseudo are simultaneously live at the `or`, so gcc gives
// the result a fresh $v0 instead of the dying link's $a0.
// MEASURED THIS ROUND (all from this exact body):
//  - shared `link |= 0x3000000; *pTag = link;` in all three groups = 93.51%,
//    568 bytes: the OR DOES land in place in $a0 as retail has it, but the
//    whole prologue rotates by one t register (code -> t8, pLink's address
//    -> a1 instead of v0) and viewX/viewY both spill to s1+s0 (+12 bytes).
//    A dead `masked = link;`/`link ^ 0` copy does not prevent it.
//  - per-group variables (pLink0..2, pTag0..2, link0..2; plain OR and the
//    in-place OR compile identically) = 98.63%, 556 bytes: group 2 becomes
//    EXACT (lw a0 / and v1 / sw v1 / or a0,a0,t7 / sw a0,0(v1)) and groups 1
//    and 3 have the pair swapped (link $v1, masked $a0). Explicit per-group
//    `masked0..2` variables holding `&pQuad[n] & 0xFFFFFF`, used for both the
//    pLink store and the tag pointer, change nothing (still 98.63 / 556).
//  - shared pLink/pTag with only `link` split per group = 97.95%, 560 bytes,
//    139 insns.
//  - the dead store `masked = (u32)&pQuad[1] & 0xFFFFFF` in group 2 IS
//    LOAD-BEARING: delete it and its declaration and the body measures 93.19%,
//    568 bytes (a callee-saved s1 appears). It is not dead code to be cleaned.
//  - preset screen: gcc-2.7.2-cdk 94.48, gcc-2.6.0 99.50, Default 99.78.
//  - round 406: -dg/-da on this body lists the loop's 17 allocnos in the order
//    `77 78 79 158 74 76 84 73 125 75 115 85 72 127 83 82 81` and hands out
//    77 (the OR result) -> $v0, 78 (masked) -> $v1, 79 (the loaded link) -> $a0.
//    Retail has NO pseudo for the OR result, i.e. its C updated the loaded word
//    in place (the 93.51 / 98.63 family above), and a `=d` 3-operand `ior` can
//    never tie to its input, so 77 always takes a fresh caller-saved register.
//  - round 406 2.6.0 unit screen: this function is 99.50 under 2.6.0 but the
//    unit cannot switch preset - BattleQueueLineQuad 91.82, BattleDrawLineQuads
//    97.64, BattleGetTextWidth 88.28, BattleDrawTextLeft 95.69, BattleDrawTextRight
//    96.62, BattleDrawTextRightOffset 96.21, BattleByteRegTestReg 96.79,
//    BattleLoadTextTimImages 98.82, BattleGetFontGlyph 99.76. 2.6.0's residual on
//    THIS body is gp-relative global loads and swapped addu operands, not the OR.
//  - permuter, third run from this body: 1352 iterations, base 30 best 30,
//    PERMUTER_NONE (earlier runs: 4335 and 1990 iterations, both base 30
//    best 30). Do not spend another run on it.
// WHAT MADE IT 99.78%: the pOtz pointer local must be ASSIGNED INSIDE THE LOOP
// (just before gte_ldv2), not in the prologue: loop-invariant hoisting then
// emits the two constant pairs in body use order (&vec 0x1F800010 into t4, then
// the 0x1F8000B0 slot into t3), which is what fixes the whole t map. Prologue
// assignment is 96.29%; the 0x1F8000B0 literal inline at each gte_stsz is
// 95.55%. Also measured dead in earlier rounds: named `tag`/`code`/`masked`
// written three times, pVec/pScratch->vec pointer locals, indexed
// `&pOt[sz >> 4]`, base-first `pOt + sz`, a `masked` shared by more than one
// group (94.48 / 92.43, 568 bytes - the pseudo then lives across groups and
// costs a callee-saved save).
// NEXT LEAD: none left in the allocator-nudge family. If this is picked up
// again, start from the -dg/-dl dumps of the two bodies (build/src/<unit>.c.i
// .greg/.lreg) and compare the OR-result allocno's refs and live length rather
// than trying more spellings; and check the per-group (98.63) split again, where
// one of the three groups already produces retail's in-place OR.
// The one-table view is exact: this walks the same 0xC g_BattleSpriteSlots
// table BattleClearSpriteTables clears (x/y/z = s16, life, vx/vz/vy), and y is
// signed because the clamp compares it with lh/slti. The OT stores use the pTag
// pointer form that the matched neighbour BattleDrawLineQuads uses.
/*
void func_8007E020(u32* pOt) {
    BattleGteScratch* pScratch;
    BattleParticleQuad* pQuad;
    s32* pOtz;
    BattleSpriteSlot* pParticle;
    u32* pLink;
    u32* pTag;
    u32 link;
    u32 masked;
    s32 dx;
    s32 dy;
    s32 dz;
    s32 stage;
    s32 i;

    pScratch = (BattleGteScratch*)0x1F800000;
    dx = pScratch->viewX;
    dy = pScratch->viewY;
    dz = pScratch->viewZ;
    pParticle = g_BattleSpriteSlots;
    pQuad = (BattleParticleQuad*)g_BattleSpritePrimBuffers[g_BattleDrawBufferIndex];
    stage = 0;
    for (i = 0; i < 0x218; i++, pParticle++) {
        if (pParticle->life == 0) {
            continue;
        }
        pParticle->x += pParticle->vx;
        pParticle->z += pParticle->vz;
        pScratch->vec.vx = pParticle->x - dx;
        pScratch->vec.vy = pParticle->y - dy;
        pScratch->vec.vz = pParticle->z - dz;
        if (stage == 0) {
            gte_ldv0(&pScratch->vec);
            stage = 1;
        } else if (stage == 1) {
            gte_ldv1(&pScratch->vec);
            stage = 2;
        } else {
            pOtz = (s32*)0x1F8000B0;
            gte_ldv2(&pScratch->vec);
            gte_rtpt();
            stage = 0;
            gte_stsxy3(&pQuad[0].xy, &pQuad[1].xy, &pQuad[2].xy);
            gte_stsz1(pOtz);
            pLink = (u32*)(((pScratch->otz >> 4) << 2) + (u32)pOt);
            link = *pLink;
            pTag = (u32*)((u32)&pQuad[0] & 0xFFFFFF);
            *pLink = (u32)pTag;
            *pTag = link | 0x03000000;
            gte_stsz2(pOtz);
            pLink = (u32*)(((pScratch->otz >> 4) << 2) + (u32)pOt);
            link = *pLink;
            pTag = (u32*)(masked = (u32)&pQuad[1] & 0xFFFFFF);
            *pLink = (u32)pTag;
            *pTag = link | 0x03000000;
            gte_stsz(pOtz);
            pLink = (u32*)(((pScratch->otz >> 4) << 2) + (u32)pOt);
            link = *pLink;
            pTag = (u32*)((u32)&pQuad[2] & 0xFFFFFF);
            *pLink = (u32)pTag;
            *pTag = link | 0x03000000;
            pQuad += 3;
        }
        pParticle->y += pParticle->vy;
        pParticle->vy += 2;
        if (pParticle->y > 0x10) {
            pParticle->y = 0x10;
        }
        pParticle->life--;
    }
}
*/

void BattleClearSpriteTables(void) {
    s32 i;

    for (i = 0; i < 0x21C; i++) {
        g_BattleSpriteSlots[i].life = 0;
        g_BattleSpriteSlots[i].z = 0;
        g_BattleSpriteSlots[i].y = 0;
        g_BattleSpriteSlots[i].x = 0;
    }
    D_8009791B = 0;
    D_80099037 = 0;
    for (i = 0; i < 0xFF; i++) {
        g_BattleSpriteEntries[i].life = 0;
        g_BattleSpriteEntries[i].z = 0;
        g_BattleSpriteEntries[i].y = 0;
        g_BattleSpriteEntries[i].x = 0;
    }
}

void BattleResetLineQuadQueue(void) {
    BattleQuad* pQuad = g_BattleLineQuads;
    s32 i;

    for (i = 0; i < 100; i++) {
        pQuad[i].len = 3;
        pQuad[i].code = 0x40;
    }
    g_BattleLineQuadQueueCount = 0;
    g_BattleLineQuadDrawCount = 0;
}

void BattleQueueLineQuad(s32* pPos, s32* pPos2, u8* pColor) {
    BattleQuad* pQuad;

    if (g_BattleLineQuadQueueCount < 100) {
        pQuad = &g_BattleLineQuads[g_BattleLineQuadQueueCount];
        pQuad->v0.vx = pPos[0];
        pQuad->v0.vy = pPos[1];
        pQuad->v0.vz = pPos[2];
        pQuad->v1.vx = pPos2[0];
        pQuad->v1.vy = pPos2[1];
        pQuad->v1.vz = pPos2[2];
        pQuad->r = pColor[0];
        pQuad->g = pColor[1];
        pQuad->b = pColor[2];
        g_BattleLineQuadQueueCount++;
    }
}

void BattleDrawLineQuads(u32* pOt) {
    BattleGteScratch* pScratch;
    BattleQuad* pQuad;
    u32* pLink;
    u32* pTag;
    u32 link;
    u32 dx;
    u32 dy;
    u32 dz;
    s32 sz;
    s32 i;

    pScratch = (BattleGteScratch*)0x1F800000;
    pQuad = g_BattleLineQuads;
    dx = pScratch->viewX;
    dy = pScratch->viewY;
    dz = pScratch->viewZ;
    i = 0;
    while (i < g_BattleLineQuadDrawCount) {
        *(Bytes8*)&pScratch->v0 = *(Bytes8*)&pQuad->v0;
        *(Bytes8*)&pScratch->v1 = *(Bytes8*)&pQuad->v1;
        pScratch->v0.vx -= dx;
        pScratch->v0.vy -= dy;
        pScratch->v0.vz -= dz;
        pScratch->v1.vx -= dx;
        pScratch->v1.vy -= dy;
        pScratch->v1.vz -= dz;
        gte_ldv01(&pScratch->v0, &pScratch->v1);
        gte_rtpt();
        gte_stsxy01(&pQuad->x, &pQuad->y);
        gte_stsz2(&sz);
        pLink = (u32*)(((sz >> 4) << 2) + (u32)pOt);
        link = *pLink;
        pTag = (u32*)((u32)pQuad & 0xFFFFFF);
        *pLink = (u32)pTag;
        *pTag = link | 0x03000000;
        i++;
        pQuad++;
    }
}

void BattleSetSpriteTimer(s32 mode) {
    if (g_BattleSpriteTimer == 0 && mode == 0xA) {
        BattlePlaySoundEffectById(0x24);
    }
    g_BattleSpriteTimer = mode;
}

void BattleDrawTimedSpritePrims(void* pOt) {
    if (g_BattleSpriteTimer != 0) {
        if (g_BattleDrawFlags & 2) {
            AddPrim(pOt, &D_800954E0[g_BattleDrawBufferIndex * 0x1C]);
            AddPrim(pOt, &D_800954E0[g_BattleDrawBufferIndex * 0x1C - 8]);
        }
        g_BattleSpriteTimer--;
    }
}

s32 BattleGetTextPrimCount(void) {
    return g_BattleTextPrimCount;
}

void BattleLoadTextTimImages(BattleTimState* pState) {
    TIM_IMAGE tim;
    RECT pad; // unused: reproduces the original's 8-byte frame slot
    BattleDrawRecord* pRec;
    s16* pClut;
    s32 w;
    s32 i;

    g_BattleTextPrimBuffers[0] = HeapAlloc(0xFA0, 0);
    g_BattleTextPrimBuffers[1] = HeapAlloc(0xFA0, 0);
    for (i = 0; i < 100; i++) {
        setlen(&g_BattleTextPrimBuffers[0][i], 0);
        setlen(&g_BattleTextPrimBuffers[1][i], 0);
    }
    OpenTIM(pState->unk3C);
    ReadTIM(&tim);
    pClut = (s16*)tim.caddr;
    pClut[2] = -0x6F9D;
    pClut[0] = 0;
    pClut[3] = -1;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    g_BattleTextClut = GetClut(tim.crect->x, tim.crect->y);
    g_BattleTextTpage = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    g_BattleTextPrimCount = 0;
    OpenTIM(pState->unk64);
    ReadTIM(&tim);
    pClut = (s16*)tim.caddr;
    pClut[0] = 0;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    pRec = g_BattleTextSpriteRecords;
    setlen(&pRec->sprt, 4);
    pRec->sprt.code = 0x65;
    SetDrawTPage(&pRec->tpage, 0, 0, GetTPage(0, 1, tim.prect->x, tim.prect->y));
    pRec->sprt.clut = GetClut(tim.crect->x, tim.crect->y);
    pRec->sprt.x0 = 0x40;
    pRec->sprt.y0 = 0xBE;
    pRec->sprt.w = 0xC4;
    pRec->sprt.h = 0xD;
    w = tim.prect->x;
    pRec->sprt.u0 = w << 2;
    pRec->sprt.v0 = tim.prect->y;
    g_BattleTextSpriteRecords[1] = *pRec;
}


void BattleSetTextCursor(s16 a, s16 b) {
    g_BattleTextCursorX = a;
    g_BattleTextCursorY = b;
}

// 4-byte entry of the g_BattleFontGlyphs font table: the glyph's texel origin and the
// cell size the glyph quad is stretched over.
typedef struct {
    /* 0x0 */ u8 u;
    /* 0x1 */ u8 v;
    /* 0x2 */ u8 w;
    /* 0x3 */ u8 h;
} BattleFontGlyph; // Size: 0x4

extern BattleFontGlyph g_BattleFontGlyphs[];

// Maps a character of the fixed-font charset onto its glyph record: digits to
// 0-9, letters to 10-35, and the handful of punctuation marks the battle text
// uses onto the symbol glyphs at the end of the table. NULL for anything else.
u8* BattleGetFontGlyph(s32 c) {
    if (c >= '0' && c <= '9') {
        c = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        c = c - 'A' + 10;
    } else {
        switch (c) {
        case '!':
            c = 0x24;
            break;
        case ':':
            c = 0x25;
            break;
        case '-':
            c = 0x26;
            break;
        case '/':
            c = 0x27;
            break;
        case '#':
            c = 0x28;
            break;
        case ' ':
            c = 0x29;
            break;
        case '\'':
            c = 0x2A;
            break;
        default:
            return NULL;
        }
    }
    return (u8*)&g_BattleFontGlyphs[c];
}

void BattleSetTextScale(s32 value) {
    g_BattleTextScale = value;
}

// best 81.04% (508 bytes against retail 520 = 3 insns short, 127 vs 130 insns). The
//    body re-measures 81.04 this round; the 84.08% the earlier note quotes was a
//    variant that was not kept.
//    ROOT RESIDUAL: the register g_BattleTextPrimCount lands in. Retail keeps it in
//    v1, loaded BEFORE the frame setup, while a0 still holds the parameter c; ours
//    loads it into a0 after the prologue, so c needs a move into a0 before the
//    BattleGetFontGlyph jal (retail puts the base+count addu in that delay slot
//    instead), pGlyph shifts a3 -> t0, mflo t0 -> t1 and the whole body mirrors.
//    The 3 missing insns are load-delay nops retail has and ours does not need,
//    a consequence of that order, not a separate hole.
//    MEASURED THIS ROUND: `s32 count = g_BattleTextPrimCount;` as the first
//    declaration (the shape misc2/misc9 BattleEffectSetMatrix has, whose retail
//    does load before its frame) compiles BYTE-IDENTICALLY to the two direct global
//    reads - 81.04 / 508 bytes - so a declaration-with-initialiser does not hoist
//    the load here; gcc-2.7.2-cdk scores 81.31 on the same body, so the preset is
//    not the way out.
//    ALREADY MEASURED (do not repeat): no count local, an explicit assignment, the
//    count as a declaration initializer, declared first vs last, pPkt spelled as
//    pBase + count / (u8 *)pBase + count * 0x28 / &g_BattleTextPrimBuffers[i][count],
//    pPkt computed after the glyph call (callee-saved s0, 520 bytes, 78.75%),
//    tpage/clut hoisted into u16 locals (78.65% / 524).
//    The rest of the body is retail line for line: the 0x28-byte POLY_FT4 quad, the
//    UV widths u1 = u0 + (glyph[2] | 3) and v2/v3 = v1 + glyph[3] + 1, code byte 9,
//    the 0x2C000000 colour word, the cursor advance by 2 + (glyph[2] * scale >> 8).
//    BattleGetFontGlyph takes an s32 (retail does no andi on a0), and
//    BattleGetTextWidth still matches 100.00% with that prototype.
// best 96.77 percent, 516 bytes against retail 520 (129 vs 130 insns). TWO FIXES moved it 81.04 -> 96.77 this round. (1) The count has to be a LOCAL whose uses are TWO statements - 'pPkt = g_BattleTextPrimBuffers[g_BattleDrawBufferIndex]; pPkt += count;' - not 'base + count' in one expression and not two inline reads of the global: only the two-statement form makes cc1 hoist 'lw v1,g_BattleTextPrimCount' ABOVE the frame setup into v1 and put 'addu s0,s0,v0' in the BattleGetFontGlyph delay slot, which is retail's whole head (the old note's 'untypeable emission order' is only the one-expression spelling). (2) The old body was MISSING the length store: retail's 'li v0,9 / sb v0,3(s0)' is setlen(pPkt, 9), written between the clut store and the 0x2C000000 colour word; that is 8 of the 12 missing bytes. REMAINING, one allocator tie plus its consequence: pGlyph takes t0 where retail has a3, so the arm shifts one register (gw a3 vs retail s1, which reuses dead c, and mflo t1 vs t0), and 'li v0,0x28' lands in the beqz a3 delay slot where retail has a nop - that nop is the last missing instruction. Permuter this round: base 423 best 320, and its ONLY edit is 'volatile unsigned int' on the return type, which would change the signature five matched callers use - rejected as wrong code.
/*
void func_8007E964(s32 c) {
    s32 count = g_BattleTextPrimCount;
    POLY_FT4* pPkt;
    u8* pGlyph;
    s16 x;
    s16 y;
    s32 gw;
    s32 dx;

    if (count >= 0x65) {
        return;
    }
    pPkt = g_BattleTextPrimBuffers[g_BattleDrawBufferIndex];
    pPkt += count;
    pGlyph = BattleGetFontGlyph(c);
    if (pGlyph == NULL) {
        return;
    }
    if (c != 0x28) {
        y = g_BattleTextCursorY;
        x = g_BattleTextCursorX;
        *(u_long*)&pPkt->x0 = x | (y << 16);
        gw = pGlyph[2] | 3;
        dx = (gw * g_BattleTextScale) >> 8;
        *(u_long*)&pPkt->x1 = (x + dx) | (y << 16);
        *(u_long*)&pPkt->x2 = x | ((y + pGlyph[3]) << 16);
        *(u_long*)&pPkt->x3 = (x + dx) | ((y + pGlyph[3]) << 16);
        *(u_short*)&pPkt->u0 = pGlyph[0] | (pGlyph[1] << 8);
        *(u_short*)&pPkt->u1 = (pGlyph[0] + gw) | (pGlyph[1] << 8);
        *(u_short*)&pPkt->u2 = pGlyph[0] | ((pGlyph[1] + (pGlyph[3] + 1)) << 8);
        *(u_short*)&pPkt->u3 = (pGlyph[0] + gw) | ((pGlyph[1] + (pGlyph[3] + 1)) << 8);
        pPkt->tpage = g_BattleTextTpage;
        pPkt->clut = g_BattleTextClut;
        setlen(pPkt, 9);
        *(u_long*)&pPkt->r0 = g_BattleTextRed | (g_BattleTextGreen << 8) | (g_BattleTextBlue << 16) | 0x2C000000;
        g_BattleTextPrimCount++;
    }
    g_BattleTextCursorX = (u16)g_BattleTextCursorX + 2 + ((pGlyph[2] * g_BattleTextScale) >> 8);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E964);




s32 BattleGetTextWidth(u8* pText) {
    s32 width = 0;

    while (*pText != 0) {
        u8* pGlyph = BattleGetFontGlyph(*pText);

        pText++;
        width += 2 + ((pGlyph[2] * g_BattleTextScale) >> 8);
    }
    return width;
}

void BattleDrawTextLeft(u8* pText) {
    s32 y = g_BattleTextCursorX;

    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    g_BattleTextCursorX = y;
    g_BattleTextCursorY += 0x14;
}

void BattleByteRegTestReg(u8* pText) {
    s32 y = g_BattleTextCursorX;

    g_BattleTextCursorX -= BattleGetTextWidth(pText) / 2;
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    g_BattleTextCursorX = y;
    g_BattleTextCursorY += 0x14;
}

void BattleDrawTextRight(u8* pText) {
    s32 y = g_BattleTextCursorX;

    g_BattleTextCursorX -= BattleGetTextWidth(pText);
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    g_BattleTextCursorX = y;
    g_BattleTextCursorY += 0x14;
}

void BattleDrawTextRightOffset(u8* pText, s32 dy) {
    s32 tmp[2]; // unused: reproduces the original's 8-byte frame slot
    s32 y = g_BattleTextCursorX;

    g_BattleTextCursorX = y - dy;
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    g_BattleTextCursorX = y;
    g_BattleTextCursorY += 0x14;
}

void BattleSetTextColor(s32 arg) {
    s32 white = 0xFF;

    if (arg != 0) {
        g_BattleTextGreen = 0xFF;
        g_BattleTextBlue = 0;
        g_BattleTextRed = g_SpuChannelMask * 20;
    } else {
        g_BattleTextRed = white;
        g_BattleTextGreen = white;
        g_BattleTextBlue = white;
    }
}


// best 87.66% / 124 bytes / 31 insns needs a VOLATILE (or __asm__-alias) read of g_SpuChannelMask, and 124 is not
// retail's 128, so it cannot link; THIS clean body is 76.72% / 100 bytes / 25 insns (re-measured this round).
//    Retail reads g_SpuChannelMask TWICE (lui/lw v1 then lui/lw a0, adjacent), derives red from the first and green
//    from the second, redoes the whole sll/addu/sll x20 chain for each, and BOTH loads sit above the `sb blue`.
//    gcc 2.7.2 CSEs the load AND the multiply when the two statements name one object, so this body needs two
//    source-level names for the one address: `volatile` gives 87.66% but pins the second load at its own statement
//    (a volatile load may not move), and the round-471 `extern s32 g_SpuChannelMaskAlt __asm__("g_SpuChannelMask")`
//    alias gives the same 124 / 31 - so neither closes the 4-byte gap. config/symbol_addrs.slus_006.64.txt has
//    only g_SpuChannelMask there and a duplicate entry stops splat. The two-volatile-casts lever (misc8) does not
//    apply: it pins a RELOAD BELOW a store, and retail here has both loads ABOVE the store.
//    Everything else is retail and order-insensitive: store order blue/red/green, the 0xFF local giving li+subu
//    where a literal 0xFF gives `nor` (72.66%), the else arm's single ori in the beqz delay slot.
// best 96.88% / 124 bytes / 31 insns (retail 128 / 32, so it cannot link). WHAT GOT THE HEAD EXACT, measured this round:
//    the two reads of g_SpuChannelMask need MIXED access, not two volatiles: `channelMask = *(volatile s32*)&g_SpuChannelMask;`
//    as the first statement (that is retail's first load, pinned there) and a PLAIN `g_SpuChannelMask` read for green, which
//    gcc then hoists above the `sb blue` so the two loads come out adjacent in retail's v1-then-a0 order. plain/plain is
//    76.72% (one load, one mul chain: both CSEd); volatile/volatile pins the second load at its own statement (87.66%,
//    measured before with an __asm__ alias too - the alias makes the same symbol_ref, so it cannot defeat the CSE either);
//    a QI local in the ELSE arm separates the arms' constants and gives retail's 128 bytes / 32 insns but 93.59%, because
//    reorg then hoists the IF arm's materialisation into the delay slot and leaves the else arm's in its block (the mirror
//    of retail). Store order blue/red/green and the arm order are insensitive. THE MISSING INSTRUCTION is a THIRD 0xFF materialisation: retail has li v0 in the branch delay slot for the ELSE arm, a separate li v0 for sb blue inside the if arm, and li v1 as the subtraction base; ours shares one constant across the arms so the delay-slot li also serves the if arm's store. The two arms each owned a 0xFF object in the original, which no single-source-constant body reproduces: gcc merges a literal 0xFF with a white local for a QI store.
/*
void func_8007EE68(s32 arg) {
    s32 white = 0xFF;

    if (arg != 0) {
        s32 channelMask;

        channelMask = *(volatile s32*)&g_SpuChannelMask;
        g_BattleTextBlue = white;
        g_BattleTextRed = white - channelMask * 20;
        g_BattleTextGreen = white - g_SpuChannelMask * 20;
    } else {
        g_BattleTextRed = white;
        g_BattleTextGreen = white;
        g_BattleTextBlue = white;
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EE68);




// best 90.10% (204 bytes = retail's exact size), re-measured this round after 4 spellings.
//    THE RESIDUAL IS ONE EMISSION-ORDER FACT, and the register hand-out follows it:
//    our cc1 ALWAYS emits `la $5,D_80091964` before `lw $4,D_80092874`, and the
//    pseudo created first takes a1 (=$5), the second a0 (=$4). Retail emits the
//    FILTER load first (lui a1 / lw a1) and the record address second (lui a0 /
//    addiu a0), i.e. the exact mirror, and its `move a3,v0` for pQueue sits right
//    after the jal instead of after the two address computations - the same
//    hoisting of the constant. Insensitive to everything source-shaped (all 90.10,
//    204 bytes, 51 insns, identical stream): filter-init first vs record-init first,
//    the record init moved below the two global stores, `g_BattleEntries = pQueue =
//    HeapAlloc(0xC4, 1)` as one chained statement, and declaration order. NOT free:
//    `i = 0;` above the HeapAlloc call makes i live across the call and it takes a
//    callee-saved register (77.63%, 212 bytes), so i must be initialised after it.
//    Everything else is retail: the 0x31 bound, `slt` on the lhu limit against the
//    s16 entry field, the count store, the 0xC/0x20 increments with the record's
//    addiu in the bnez delay slot, and the trailing guarded call.
// re-measured 90.10% (204 bytes = retail's exact size, 51 insns) after un-parking; the lever 'an symbol address is always hoisted above the load in the preheader' (round 480) names this exact function and is still the cause, so no new spelling was spent on the la/lw order.
// re-measured 90.45% / 204 bytes = retail's exact size / 51 insns (round 506: two new spellings, both dead ends).
//    NAMED RESIDUAL: the hoisted record base `la` is inserted ABOVE the source-position filter load, so the entry
//    block comes out `la a1,D_80091964 / lw a0,D_80092874 / move a3,v0` where retail has `move a3,v0 / lui a1+lw a1 /
//    lui a0+addiu a0` - three rows, one cause (lever an-symbol-address-is-always-hoisted-above-the-load-in-the-preheader,
//    round 480, this function).
//    New this round: (1) the record address taken as `&D_80091964[i]` off a typed `extern BattleQueueRecord D_80091964[]`
//    (LSR walks it by 0xC so retail's `addiu a0,a0,12` stays in the bnez slot) and (2) BOTH tables indexed, no pointer
//    locals at all (`D_80092874[i].requiredProgress`, `&D_80091964[i]`) - one and the same 51-insn stream, the la still
//    hoisted first; the 90.10 -> 90.45 is the reloc rows only. gcc-2.6.0 gives the identical 90.45, cdk 77.49, -G0
//    does not compile. The indexed body is kept because it is the realistic form (two typed tables, no casts).
/*
void func_8007EEE8(s32 arg) {
    s32 i;
    s32 limit;
    s32** pQueue;

    limit = g_GameState.fieldScriptMemory[0];
    pQueue = HeapAlloc(0xC4, 1);
    i = 0;
    g_BattleEntries = pQueue;
    g_BattleEntryCount = 0;
    for (; i < 0x31; i++) {
        if (arg == 0 || D_80092874[i].requiredProgress <= limit) {
            pQueue[g_BattleEntryCount++] = (s32*)&D_80091964[i];
        }
    }
    if (arg != 0) {
        BattleAppendExtraQueueEntry();
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EEE8);





void BattleInitSpriteRectTable(void) {
    s32 tmp[12]; // unused: reproduces the original's 0x48 frame
    BattleSprite* pSprite;
    s32 count;
    s32 row;
    s32 col;
    s16 y;
    s16 x;

    g_BattleSpriteRects = HeapAlloc(0x3D4, 0);
    pSprite = g_BattleSpriteRects;
    count = 0x1FF;
    for (row = 0; row < 7; row++) {
        y = 0x1A0 + row * 0x20;
        for (col = 0; col < 7; col++) {
            x = col * 0x40;
            pSprite->clutX = 0x200;
            pSprite->clutY = count--;
            pSprite->clutW = 0x80;
            pSprite->clutH = 1;
            pSprite->texX = y;
            pSprite->texY = x;
            pSprite->texW = 0x1E;
            pSprite->texH = 0x40;
            pSprite++;
        }
    }
}

// best 96.42% / 504 bytes / 126 insns - ONE INSTRUCTION SHORT of retail's 508/127, so it cannot link.
//    THE HEAD IS NOW EXACT (prologue, `lw t0,48(sp)`, the idx wrap, g_BattleEntries) after two changes:
//    (1) form the sprite base BEFORE the queue entry - `pSprite = g_BattleSpriteRects;` then
//    `pSprite = &pSprite[*g_BattleEntries[idx]];` - which kills the old extra nop: 90.98 -> 93.65% and the body
//    to retail's exact 508 bytes / 127 insns. (2) `x = x + 0xD3;` (not `x += 0xD3;`) plus reusing the dead
//    `color` local for y1's partial sum (`color = y0 + 0x38; y1 = color + grow;`, a permuter find) gives
//    retail's y1 pair `addiu v0,v0,56 / addu v0,v0,a2`: 96.42%, one instruction short.
//    THE THREE RESIDUALS, named: retail `lw a0,0(v0)` with the rects base in v1, ours `lw v1,0(v0)` with the base
//    in s1 (value and base columns swapped - allocator tie; inline indexing, the count local, base-first and
//    value-first all measured); retail `addiu t0,t0,-0x40 / sra t0,t0,4` where cc1 folds `(color - 0x40) >> 4`
//    back to `sra a0,t0,4` on shade (this IS the missing instruction); and our x1 folds 0x34 into grow's addiu
//    (`addiu v1,a2,52 / addu v1,a1,v1`) where retail keeps `addiu v1,a0,52 / sll a2,t0,1 / addu v1,v1,a2`.
//    Permuter, three runs this family (base 666 -> 455 -> 445); its best 445 body is this one with
//    `half = 0x34; x1 = x0 + half + grow;` = 96.50% (a folding-constant local, still 504 bytes).
//    Round 504: computing the color word's operand INSIDE the arm (`color = shade + 0x40;` as the else
//    arm's first statement, `grow = shade + 0x40` deleted from before the if) DOES bring the missing
//    `addiu t0,t0,-0x40` back and gives retail's exact 508 bytes / 127 insns, but it also moves the
//    shade load out of the prologue (`lw t0,48(sp)` becomes `lw a1,48(sp)` after the saves), i.e. the
//    whole hand-out rotates: 88.33%. So the -0x40 IS reachable only together with that rotation; the
//    96.42% body keeps the prologue exact. Neither links.
//    Dead ends measured this round, do not repeat: `x1 = x0 + 0x34; x1 += grow;` 83.94% / 512 / 128 (the
//    allocator collapses and shade moves to a1), `color = x0 + 0x34; x1 = color + grow;` 78.43% / 512 / 128,
//    and the parameter-reassignment shape (`shade += 0x40` inside the arm) 504 bytes / 126.
/*
void func_8007F05C(s32 idx, POLY_FT4* pPkt, s32 flag, s32 x, s32 shade) {
    BattleSprite* pSprite;
    s32 count;
    s32 color;
    s32 half;
    s32 grow;
    s32 n;
    s32 u;
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;

    if (flag != 0) {
        x = x + 0xD3;
    } else {
        x = x + 0x33;
    }
    n = g_BattleEntryCount;
    if (idx > n - 1) {
        idx -= n;
    }
    if (idx < 0) {
        idx += n;
    }
    pSprite = g_BattleSpriteRects;
    count = *g_BattleEntries[idx];
    grow = shade + 0x40;
    pSprite = &pSprite[count];
    color = grow;
    if (shade == 0x40) {
        setlen(pPkt, 9);
        pPkt->code = 0x2D;
        *(u_long*)&pPkt->x0 = x | (0x30 << 16);
        *(u_long*)&pPkt->x1 = (x + 0x3C) | (0x30 << 16);
        *(u_long*)&pPkt->x2 = x | (0x70 << 16);
        *(u_long*)&pPkt->x3 = (x + 0x3C) | (0x70 << 16);
    } else {
        setlen(pPkt, 9);
        *(u_long*)&pPkt->r0 = color | (color << 8) | (color << 16) | 0x2C000000;
        half = (color - 0x40) >> 4;
        x0 = x + 4 - half;
        y0 = 0x34 - half;
        grow = half * 2;
        x1 = x0 + 0x34 + grow;
        color = y0 + 0x38;
        y1 = color + grow;
        *(u_long*)&pPkt->x0 = x0 | (y0 << 16);
        *(u_long*)&pPkt->x1 = x1 | (y0 << 16);
        *(u_long*)&pPkt->x2 = x0 | (y1 << 16);
        *(u_long*)&pPkt->x3 = x1 | (y1 << 16);
    }
    u = pSprite->texX * 2;
    *(u_short*)&pPkt->u0 = u | (pSprite->texY << 8);
    *(u_short*)&pPkt->u1 = u + 0x3B | (pSprite->texY << 8);
    *(u_short*)&pPkt->u2 = u | ((pSprite->texY + 0x3F) << 8);
    *(u_short*)&pPkt->u3 = u + 0x3B | ((pSprite->texY + 0x3F) << 8);
    pPkt->clut = GetClut(pSprite->clutX, pSprite->clutY);
    pPkt->tpage = GetTPage(1, 0, pSprite->texX & 0xFF80, pSprite->texY);
    AddPrim(g_BattleRootOrderingTable, pPkt);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007F05C);




// 4-s16 set of cursor-arrow part offsets, indexed by the sign of the cursor's
// step: entry 0 for a leftward step, entry 1 for a rightward one.
typedef struct {
    /* 0x0 */ s16 indexDelta0;
    /* 0x2 */ s16 indexDelta1;
    /* 0x4 */ s16 indexDelta2;
    /* 0x6 */ s16 xOffset;
} BattleCursorOffset; // Size: 0x8

extern BattleCursorOffset D_800912E0[];
extern POLY_FT4 D_80099DA8[];
extern u8 D_8006FE88[];
extern s32 g_BattleCursorIndex0;
extern s32 g_BattleCursorIndex1;
extern s32 g_BattleSlotConfirmFlags;
extern s32 g_BattlePrevCursorIndex0;
extern s32 g_BattleCursor0Step;
extern s32 g_BattleCursor0Offset;
extern s32 g_BattlePrevCursorIndex1;
extern s32 g_BattleCursor1Step;
extern s32 g_BattleCursor1Offset;
extern s32 g_BattleSlot0TextColor;
extern s32 g_BattleSlot1TextColor;

void func_8007F05C(s32 idx, POLY_FT4* pPkt, s32 flag, s32 x, s32 shade);
s32 BattleTestSlotBit(s32 slot);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007F258);
/* best 81.01% (1456 bytes against retail's 1496). The whole HUD tail from
   BattleSetTextCursor(0xA0,0x78) to the closing BattleSetTextColor(0) is exact,
   as is the 0x50 frame: it needs an unused s32 tmp[8] local (retail spends 32
   bytes on nothing), and D_80099DA8[g_BattleDrawBufferIndex * 10] reproduces the mul-by-0x190
   chain. What is left, all inside the two cursor blocks:
   1. THE ALIAS TRAP, and the reason this cannot reach 100.00%: retail stores the
      step to g_BattleCursor0Step, then loads g_BattleCursor0Step, stores it back to g_BattleCursor0Step and
      then to g_BattleCursor0Offset (sw / lw / sw / sw, four instructions, twice). A store
      then a load of the same word means the original source had TWO symbol names
      at 0x80092718 (gcc cannot forward across differing objects), i.e.
      A = step; B = A; C = B; with A and B aliases. symbol_addrs.battling.txt has
      no duplicate addresses, so the second name cannot be spelled without adding
      one to config/symbol_addrs.battling.txt (untested: splat may then rename the
      asm label). Ours emits sw + sw, two instructions, and that is 8 of the 40
      missing bytes (retail also spends more in the whole block; see 2-4).
   2. the abs of delta: retail keeps delta live and negates a COPY
      (bgez v1; move v0,v1; negu v0,v0; slti v0,v0,4), ours coalesces and gcc
      then duplicates the slti into the bgez delay slot (4 insns vs retail's 3
      after the bgez). Tried step = delta then if (step<0) step = -step, the
      if/else form, and the ternary delta < 0 ? -delta : delta - all identical.
   3. the step sign: retail is bgtz v1 with li -0x24 in the delay slot; ours is
      the inverted blez with li 0x24 first, whichever polarity the source uses.
   4. register assignment: retail keeps pPkt in s1, the sign mask in s0, the
      0x38E38E39 magic in s2 and the 0x24 in s3; ours puts pPkt in s0, which
      perturbs every instruction that touches the packet pointer.
   Also proved: the second and fourth calls of each block pass the shade argument
   as ((0x24 - abs(step)) << 6) / 0x24 (magic 0x38E38E39, shift 3) and the third
   and fourth pass x = -0x24 / 0x24 with shade 0; the offset table D_800912E0 is
   two 8-byte s16 quads, {1,-1,2,36} for a negative step and {-1,-2,1,-36} for a
   non-negative one, indexed by the s32 compare g_BattleCursor0Step >= 0, and the compare
   is evaluated BEFORE the first call of the block while its sll 3 lands inside
   the if body - so write the compare into a local above the call, not
   &D_800912E0[expr] at the use. Last call of block 1 does not advance pPkt.

void func_8007F258(void* pOt, s32 arg) {
    s32 tmp[8]; // unused: reproduces the original's 0x50 frame
    POLY_FT4* pPkt;
    BattleCursorOffset* pOff;
    s32 delta;
    s32 step;
    s32 shade;
    s32 sign;

    pPkt = &D_80099DA8[g_BattleDrawBufferIndex * 10];

    delta = g_BattlePrevCursorIndex0 - g_BattleCursorIndex0;
    if (delta != 0) {
        if (delta < 0) {
            step = -delta;
        } else {
            step = delta;
        }
        if (step >= 4) {
            delta = -delta;
        }
        if (delta > 0) {
            step = -0x24;
        } else {
            step = 0x24;
        }
        g_BattleCursor0Step = step;
        g_BattleCursor0Offset = g_BattleCursor0Step;
    }
    delta = g_BattlePrevCursorIndex1 - g_BattleCursorIndex1;
    if (delta != 0) {
        if (delta < 0) {
            step = -delta;
        } else {
            step = delta;
        }
        if (step >= 4) {
            delta = -delta;
        }
        if (delta > 0) {
            step = -0x24;
        } else {
            step = 0x24;
        }
        g_BattleCursor1Step = step;
        g_BattleCursor1Offset = g_BattleCursor1Step;
    }

    step = g_BattleCursor0Step;
    if (step != 0) {
        if (step > 0) {
            step -= 2;
        } else {
            step += 2;
        }
        g_BattleCursor0Step = step;
    }
    step = g_BattleCursor0Offset;
    if (step != 0) {
        if (step > 0) {
            step -= 4;
        } else {
            step += 4;
        }
        g_BattleCursor0Offset = step;
    }
    step = g_BattleCursor1Step;
    if (step != 0) {
        if (step > 0) {
            step -= 2;
        } else {
            step += 2;
        }
        g_BattleCursor1Step = step;
    }
    step = g_BattleCursor1Offset;
    if (step != 0) {
        if (step > 0) {
            step -= 4;
        } else {
            step += 4;
        }
        g_BattleCursor1Offset = step;
    }

    sign = g_BattleCursor0Step >= 0;
    step = g_BattleCursor0Offset;
    if (step < 0) {
        step = -step;
    }
    shade = ((0x24 - step) << 6) / 0x24;
    g_BattlePrevCursorIndex0 = g_BattleCursorIndex0;
    g_BattlePrevCursorIndex1 = g_BattleCursorIndex1;
    func_8007F05C(g_BattleCursorIndex0, pPkt++, 0, g_BattleCursor0Offset, shade);
    if ((g_BattleSlotConfirmFlags & 1) == 0) {
        pOff = &D_800912E0[sign];
        step = g_BattleCursor0Step;
        if (step < 0) {
            step = -step;
        }
        shade = ((0x24 - step) << 6) / 0x24;
        func_8007F05C(g_BattleCursorIndex0 + pOff->indexDelta0, pPkt++, 0, g_BattleCursor0Step + pOff->xOffset, shade);
        func_8007F05C(g_BattleCursorIndex0 + pOff->indexDelta1, pPkt++, 0, -0x24, 0);
        func_8007F05C(g_BattleCursorIndex0 + pOff->indexDelta2, pPkt++, 0, 0x24, 0);
    }

    sign = g_BattleCursor1Step >= 0;
    step = g_BattleCursor1Offset;
    if (step < 0) {
        step = -step;
    }
    shade = ((0x24 - step) << 6) / 0x24;
    func_8007F05C(g_BattleCursorIndex1, pPkt++, 1, g_BattleCursor1Offset, shade);
    if ((g_BattleSlotConfirmFlags & 2) == 0) {
        pOff = &D_800912E0[sign];
        step = g_BattleCursor1Step;
        if (step < 0) {
            step = -step;
        }
        shade = ((0x24 - step) << 6) / 0x24;
        func_8007F05C(g_BattleCursorIndex1 + pOff->indexDelta0, pPkt++, 1, g_BattleCursor1Step + pOff->xOffset, shade);
        func_8007F05C(g_BattleCursorIndex1 + pOff->indexDelta1, pPkt++, 1, -0x24, 0);
        func_8007F05C(g_BattleCursorIndex1 + pOff->indexDelta2, pPkt, 1, 0x24, 0);
    }

    BattleSetTextCursor(0xA0, 0x78);
    BattleByteRegTestReg(D_8006FE88);
    BattleSetTextCursor(0x50, 0x78);
    if (BattleTestSlotBit(g_BattleEntries[g_BattleCursorIndex0][0]) != 0) {
        func_8007EE68(g_BattleSlot0TextColor);
    } else {
        BattleSetTextColor(g_BattleSlot0TextColor);
    }
    BattleByteRegTestReg((u8*)g_BattleEntries[g_BattleCursorIndex0][2]);
    BattleSetTextCursor(0xF0, 0x78);
    if (BattleTestSlotBit(g_BattleEntries[g_BattleCursorIndex1][0]) != 0) {
        func_8007EE68(g_BattleSlot1TextColor);
    } else {
        BattleSetTextColor(g_BattleSlot1TextColor);
    }
    BattleByteRegTestReg((u8*)g_BattleEntries[g_BattleCursorIndex1][2]);
    BattleSetTextColor(0);
}
 */

void BattleClearTextSpriteEntries(void) {
    g_BattleTextSpriteEntry1 = 0;
    g_BattleTextSpriteEntry0 = 0;
    g_BattleTextEntryCursor = 0;
}
