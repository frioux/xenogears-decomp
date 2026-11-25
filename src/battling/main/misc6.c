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
extern s32 D_80091964[];
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

// 0xA-byte entry of the table func_8007D6B8 (main/misc5) allocates at g_BattleSpriteEntries.
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E964);
/* BEST 84.08% per the round that parked it; the body below re-measures 81.04%
   (508 bytes against retail's 520) because it is one variant of it. Everything
   in the body is the right shape - the 0x28-byte POLY_FT4 glyph quad, the UV
   widths (u1 = u0 + (glyph[2] | 3), v2/v3 = v1 + glyph[3] + 1), the code/len
   byte 9, the colour word, the x advance.
   THE ONE THING THAT DECIDES ALMOST EVERY REGISTER HERE is which register
   g_BattleTextPrimCount lands in. Retail keeps it in v1 and reads it before
   the frame setup (lui v1 / lw v1 ABOVE addiu sp), so a0 still holds the
   parameter c at the glyph call and needs no copy; s1 (c, dead after the
   c != 0x28 test) is then free for (glyph[2] | 3), pGlyph stays in a3, mflo in
   t0. Ours puts the count in a0, so c has to travel s1 -> a0 with an extra
   'move a0,s1' before the jal and every other register shifts
   (pGlyph t0, width a3, mflo t1) - that single tie is worth the 12 missing
   bytes and about 15 instructions. Tried and measured, all 81.04%: no count
   local (two reads of the global), an explicit s32 count assignment, the count
   as a declaration INITIALIZER, count declared first vs last, pPkt spelled as
   pBase + count / (u8 *)pBase + count * 0x28 / &g_BattleTextPrimBuffers[idx][count]
   (one statement), and pPkt computed AFTER the glyph call (that one changes the
   register class instead: the count becomes callee-saved s0, 520 bytes at
   78.75%). Hoisting g_BattleTextTpage/g_BattleTextClut into u16 locals to
   reproduce retail's early lhu pair: 78.65%, 524 bytes.
   Still line for line: retail loads tpage/clut (lhu a1/lhu a2) BEFORE the four
   coordinate stores and stores them at 0x16/0xE afterwards, and its tail wants
   lhu for g_BattleTextCursorX (ours lh even with an explicit (u16) cast).
   Also proved here: BattleGetFontGlyph takes an s32, not a u8 - retail does no
   andi on a0 in its own body or at these call sites, and BattleGetTextWidth
   still matches 100.00% with the s32 prototype.
void func_8007E964(s32 c) {
    POLY_FT4* pPkt;
    POLY_FT4* pBase;
    u8* pGlyph;
    s16 x;
    s16 y;
    s32 gw;
    s32 dx;

    if (g_BattleTextPrimCount >= 0x65) {
        return;
    }
    pBase = (POLY_FT4*)g_BattleTextPrimBuffers[g_BattleDrawBufferIndex];
    pPkt = pBase + g_BattleTextPrimCount;
    pGlyph = BattleGetFontGlyph(c);
    if (pGlyph == 0) {
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
        *(u_long*)&pPkt->r0 = g_BattleTextRed | (g_BattleTextGreen << 8) | (g_BattleTextBlue << 16) | 0x2C000000;
        g_BattleTextPrimCount++;
    }
    g_BattleTextCursorX = (u16)g_BattleTextCursorX + 2 + ((pGlyph[2] * g_BattleTextScale) >> 8);
}
 */

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


// best 87.66% (124 bytes against retail's 128, 31 of 32 insns) and that number needs a VOLATILE read (see below); the clean body here is 76.72% / 100 bytes because gcc CSEs the two identical reads into one. Retail reads g_SpuChannelMask TWICE in two ADJACENT lw at the top of the arg != 0 arm, red from the first (v1) and green from the second (a0), then a SECOND ori 0xFF, then sb blue, then red's chain, then green's chain. Everything else is right: store order blue/red/green, sll-addu-sll by 20, the 0xFF local giving li+subu where a literal 0xFF gives nor (72.66%), the else arm's one ori in the beqz delay slot. RE-MEASURED THIS ROUND: (1) the parked clean body 76.72% / 100 bytes, 25 insns; (2) statement order red, blue, green compiles BYTE-IDENTICAL to blue, red, green (gcc sinks the blue store and merges anyway - a store to g_BattleTextBlue does NOT invalidate the memory-CSE entry in gcc 2.7.2), still 76.72; (3) volatile on the green read 87.66% / 124 bytes / 31 insns, the two lw present but the volatile one is pinned at its statement instead of hoisted to the top, and that 4-byte gap cannot be closed from C because a volatile load may not move. So the residual is TWO SOURCE READS OF ONE ADDRESS, which needs two symbol names for 0x80059488; config/symbol_addrs.slus_006.64.txt has only g_SpuChannelMask there and the overlay yaml loads that file, so a duplicate entry stops splat (see the e-a-main-exe lever). Do not ship the volatile spelling: it is 4 bytes short and cannot link. ALSO MEASURED: the round-471 alias trick (a second declaration 'extern s32 g_SpuChannelMaskAlt __asm__("g_SpuChannelMask");' read by the green statement, which defeats cse because the two DECLs are distinct gcc objects while relocating to one symbol) gives the same two loads at the same 87.66% / 124 bytes / 31 insns with the second load pinned at its own statement - and it is insensitive to statement order, so the alias buys no more than volatile does and needs no extra symbol in symbol_addrs.
/*
void func_8007EE68(s32 arg) {
    s32 white = 0xFF;

    if (arg != 0) {
        g_BattleTextBlue = 0xFF;
        g_BattleTextRed = white - g_SpuChannelMask * 20;
        g_BattleTextGreen = white - g_SpuChannelMask * 20;
    } else {
        g_BattleTextRed = white;
        g_BattleTextGreen = white;
        g_BattleTextBlue = white;
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EE68);



INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EEE8);
/* best 90.10% (204 bytes = retail's exact size). Two residuals left, both
   allocator ties:
   1. the two induction pointers get a0/a1 the wrong way round. Retail keeps the
      0xC record pointer in a0 and the 0x20 filter pointer in a1; ours is the
      reverse. Nothing in the source moves it: both assignment orders, both
      declaration orders, the indexed filter form (D_80092874[i]) instead of the
      pointer form, and every for-list order measure 89.7-90.1% or identical.
   2. the move a3,v0 that copies the HeapAlloc result into pQueue sits at the end
      of the entry block here and right after the call in retail.
   WHAT DID FIX THE REST: the record pointer is a BattleQueueRecord* (0xC bytes),
   so pRecord++ emits retail's addiu 0xC - the earlier 90.45% body indexed an
   s32* (&D_80091964[i], +4) which is WRONG CODE for i > 0 even though objdiff
   scored it higher. Also load-bearing, a couple of points each: i = 0; as its
   own statement above the for (retail's move a2,zero is the first thing after
   the jal) and the increment list in the order pEntry++, i++, pRecord++ (that is
   what puts the record's addiu 0xC in the bnez delay slot and the filter's
   addiu 0x20 first). Two source names for one address are NOT involved here.
/*
void func_8007EEE8(s32 arg) {
    BattleFilterEntry* pEntry;
    BattleQueueRecord* pRecord;
    s32 i;
    s32 limit;
    s32** pQueue;

    limit = g_GameState.fieldScriptMemory[0];
    pQueue = HeapAlloc(0xC4, 1);
    i = 0;
    pEntry = D_80092874;
    pRecord = (BattleQueueRecord*)D_80091964;
    g_BattleEntries = pQueue;
    g_BattleEntryCount = 0;
    for (; i < 0x31; pEntry++, i++, pRecord++) {
        if (arg == 0 || pEntry->requiredProgress <= limit) {
            pQueue[g_BattleEntryCount++] = (s32*)pRecord;
        }
    }
    if (arg != 0) {
        BattleAppendExtraQueueEntry();
    }
}
 */

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

// best 90.98% at 512 bytes. RETAIL IS 508 BYTES / 127 INSTRUCTIONS, NOT 512 as the previous note said (count the .s: 0x8007F05C to 0x8007F068 = 0x1FC), so this body is 4 bytes OVER, and the two residuals are now named exactly:
//    1. OUR +1 INSTRUCTION: retail loads the entry's first word into a0 (lw a0,0(v0)) and its load-delay slot is filled by 'lui v1,%hi(g_BattleSpriteRects)'; ours loads it into v1 (the dying g_BattleEntries pointer register), so the next instruction would clobber v1, gcc emits a nop, and the sprite chain shifts down (sll v0,v1,2 / addu v0,v0,v1). Fixing this one register removes the extra instruction.
//    2. OUR -1 INSTRUCTION in the shaded arm: retail has 'addiu t0,t0,-0x40' THEN 'sra t0,t0,4', i.e. it derives half from the COLOUR value in shade's own clobbered register (the addiu +0x40 is in the bne delay slot, in place). Our source's 'half = (color - 0x40) >> 4' is folded by cc1 to 'sra a0,t1,4' on shade, which also keeps shade alive in a second register (t1) and drags the x1/y1 association and the last three sw registers with it.
//    MEASURED THIS ROUND, all from this body: (a) splitting x1/y1 into 'x1 = x0 + 0x34; x1 += grow;' (retail's (x0+0x34)+2k association) = 75.08% / 512 / 128, the allocator collapses and shade moves to a1 - do not repeat it; (b) 'color += 0x40;' inside the shaded arm instead of 'grow = shade + 0x40;' before the if = 84.49% / 504 / 126: the addiu +0x40 leaves the delay slot and the -0x40 STEALS TO BE FOLDED AGAIN, so the in-place colour idea does not by itself give retail's two instructions; (c) 'pSprite = &g_BattleSpriteRects[*g_BattleEntries[idx]];' (indexing the entry word inline, no count local) compiles BYTE-IDENTICAL to this body - cse reassociates it, so the a0/v1 choice is not reachable that way.
//    Unchanged from the earlier note, each measured: the grow/color two-name alias is what puts retail's addiu +0x40 in the bne delay slot; color|(color<<8)|(color<<16) in that order gives 'or v0,t0,v0'; sprite fields are s16 (that is what makes the uv stores lhu and the GetTPage/GetClut args lh); permuter already ran three times on this family (base 1855 -> 1465, 866 -> 656) and the old 'y1 split' lead is 508 bytes i.e. one instruction short - it cannot link.
/*
void func_8007F05C(s32 idx, POLY_FT4* pPkt, s32 flag, s32 x, s32 shade) {
    BattleSprite* pSprite;
    s32 count;
    s32 color;
    s32 half;
    s32 grow;
    s32 u;
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;

    if (flag != 0) {
        x += 0xD3;
    } else {
        x += 0x33;
    }
    count = g_BattleEntryCount;
    if (idx > count - 1) {
        idx -= count;
    }
    if (idx < 0) {
        idx += count;
    }
    count = *g_BattleEntries[idx];
    grow = shade + 0x40;
    pSprite = &g_BattleSpriteRects[count];
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
        y1 = y0 + 0x38 + grow;
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
