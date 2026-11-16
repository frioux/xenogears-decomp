#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8 D_800928A0;
extern u8 D_80092708;
extern u8 D_800926F0;
extern u8 D_800926F4;
extern u8 D_800926F8;
extern u8 D_8009273C;
extern u8 D_80092740;
extern s32 D_80092744;
extern s32 D_800912DC;
extern s32 D_80059488;
extern s16 D_800926E8;
extern s16 D_800926EC;
extern u8 D_8009791B;
extern u8 D_80099037;
extern u8 D_800954E0[];
extern s32 D_800928E8;

void func_8007E964(s32 c);
u8* func_8007E8AC(s32 c);
s32 BattleGetTextWidth(u8* pText);
extern s32 D_800926DC;

void func_8008EB4C(s32 arg);
void BattleAppendExtraQueueEntry(void);

extern POLY_FT4* D_800926D4[];
extern u16 D_800926E0;
extern u16 D_800926E4;

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

extern BattleDrawRecord D_800954D8[];

// 0x20-byte entry of the D_80092874 table; func_8007EEE8 reads only the s16 at 0x4.
typedef struct {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
} BattleFilterEntry; // Size: 0x20

// 0xC-byte record whose address func_8007EEE8 queues into D_800928EC.
typedef struct {
    /* 0x00 */ u8 unk0[0xC];
} BattleQueueRecord; // Size: 0xC

extern BattleFilterEntry* D_80092874;
extern s32 D_80091964[];
extern s32** D_800928EC;
extern s32 D_80092888;

// 0xC-byte entry of the slot table BattleInitSpriteBuffers (main/misc5) allocates
// at D_800926C8; this file only clears the head of an entry.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s8 unk8;
    /* 0x9 */ s8 unk9;
    /* 0xA */ u16 unkA;
} BattleSpriteSlot; // Size: 0xC

// 0xA-byte entry of the table func_8007D6B8 (main/misc5) allocates at D_800926BC.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ s8 unk6;
    /* 0x7 */ s8 unk7;
    /* 0x8 */ u16 unk8;
} BattleSpriteEntry; // Size: 0xA

extern BattleSpriteSlot* D_800926C8;
extern BattleSpriteEntry* D_800926BC;

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 r;
    /* 0x05 */ u8 g;
    /* 0x06 */ u8 b;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ SVECTOR v0;
    /* 0x18 */ SVECTOR v1;
} BattleQuad; // Size: 0x20

// An 8-byte, byte-aligned chunk: assigning one copies 8 bytes with lwl/lwr.
typedef struct {
    u8 unk[8];
} Bytes8;

extern BattleQuad D_80094818[100];
extern s32 D_800926B0;
extern s32 D_800926B4;

// 0x14-byte sprite record of the 7x7 table BattleInitSpriteRectTable allocates at D_8009270C.
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 unk12;
} BattleSprite; // Size: 0x14

extern BattleSprite* D_8009270C;
extern void* D_80092938;

// Vertex staged in the GTE scratch block. The components are subtracted as
// unsigned halves, which is how the original loads them (lhu).
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 pad;
} BattleStageVertex; // Size: 0x8

// GTE scratch block at 0x1F800000: the three camera offsets every drawer
// subtracts, the pair BattleDrawLineQuads stages for its RTPT, and the SZ
// read-back slot func_8007E020 reads at 0xB0.
typedef struct {
    /* 0x00 */ s32 unk00;
    /* 0x04 */ s32 unk04;
    /* 0x08 */ s32 unk08;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ BattleStageVertex vec;
    /* 0x18 */ BattleStageVertex v0;
    /* 0x20 */ BattleStageVertex v1;
    /* 0x28 */ u8 unk28[0x88];
    /* 0xB0 */ s32 otz;
} BattleGteScratch; // Size: 0xB4

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E020);
/* best 96.29%. Same size and instruction multiset as retail (556 bytes, 139 insns:
   the three RTPT stages, the 0x30-byte triple-prim block, the OT link and the
   unk2/unkA tail are instruction-identical up to register names). What is left is
   the entry block's register assignment: retail copies pOt into t6 first, then
   materialises &vec (t4), the 0x1F8000B0 read-back slot (t3), 0xFFFFFF, 0x3000000
   and the 0x1F800000 base (t1), and keeps i in t5; ours copies pOt into t5,
   materialises the read-back slot (t6) first and keeps i in t4. One 'or' also writes
   v0 where retail writes a0 - writing 'link |= 0x03000000;' before the store may fix
   that one. Permuter: 1390 -> 395 in one 3-minute run (that is where the
   embedded-assignment lever below came from), flat on a second run.
   Levers this body already carries, each of which moved it: gte_stsxy3(&q[0].xy,
   &q[1].xy, &q[2].xy) as ONE macro call, which is what materialises retail's three
   addiu before the swc2s; D_800926CC as its own pointer-array symbol instead of
   ((u8**)&D_800926C8)[1 + page] (that also fixes the relocation name); the
   0x1F800000 scratch as one struct whose otz at 0xB0 is read through the base while
   the SDK read-back writes the fixed slot 0x1F8000B0 through a local pointer - that
   split is what produces retail's 'lw v0,176(t1)' beside 'swc2 $17,0(t3)'; s16
   coordinates (a truncating add gives retail's lhu/lhu, the clamp's lh/slti needs the
   signed field); and 'for (i = 0; i < 0x218; i++, pParticle++)' rather than an
   increment inside the body, which is what puts both giv increments in the loop tail.
// 0x10-byte 2D primitive the particle renderer links three at a time; only the
// tag word and the projected XY at 0x8 are written here.
typedef struct {
    // 0x0 u32 tag;
    // 0x4 u32 unk4;
    // 0x8 u32 xy;
    // 0xC u32 unkC;
} BattleParticleQuad; // Size: 0x10

// 0xC-byte entry of the particle table D_800926C8 points at.
typedef struct {
    // 0x0 s16 unk0;
    // 0x2 s16 unk2;
    // 0x4 s16 unk4;
    // 0x6 s16 unk6; // 0 marks a free entry
    // 0x8 s8 unk8;
    // 0x9 s8 unk9;
    // 0xA s16 unkA;
} BattleParticle; // Size: 0xC

extern BattleParticleQuad* D_800926CC[];

// Projects the live particles three at a time with RTPT and links one screen
// prim per particle into the OT by its projected Z.
void func_8007E020(u32* pOt) {
    BattleGteScratch* pScratch;
    BattleParticleQuad* pQuad;
    BattleParticle* pParticle;
    s32* pOtz;
    u32 masked;
    u32* pLink;
    u32 link;
    u32 addr;
    s32 dx;
    s32 dy;
    s32 dz;
    s32 stage;
    s32 i;

    pScratch = (BattleGteScratch*)0x1F800000;
    pOtz = (s32*)0x1F8000B0;
    dx = pScratch->unk00;
    dy = pScratch->unk04;
    dz = pScratch->unk08;
    pParticle = (BattleParticle*)D_800926C8;
    pQuad = (BattleParticleQuad*)D_800926CC[D_800928A0];
    stage = 0;
    for (i = 0; i < 0x218; i++, pParticle++) {
        if (pParticle->unk6 == 0) {
            continue;
        }
        pParticle->unk0 += pParticle->unk8;
        pParticle->unk4 += pParticle->unk9;
        pScratch->vec.vx = pParticle->unk0 - dx;
        pScratch->vec.vy = pParticle->unk2 - dy;
        pScratch->vec.vz = pParticle->unk4 - dz;
        if (stage == 0) {
            gte_ldv0(&pScratch->vec);
            stage = 1;
        } else if (stage == 1) {
            gte_ldv1(&pScratch->vec);
            stage = 2;
        } else {
            gte_ldv2(&pScratch->vec);
            gte_rtpt();
            stage = 0;
            gte_stsxy3(&pQuad[0].xy, &pQuad[1].xy, &pQuad[2].xy);
            gte_stsz1(pOtz);
            pLink = (u32*)(((pScratch->otz >> 4) << 2) + (u32)pOt);
            link = *pLink;
            addr = (u32)&pQuad[0] & 0xFFFFFF;
            *pLink = addr;
            *(u32*)addr = link | 0x03000000;
            gte_stsz2(pOtz);
            pLink = (u32*)(((pScratch->otz >> 4) << 2) + (u32)pOt);
            link = *pLink;
            addr = masked = (u32)&pQuad[1] & 0xFFFFFF;
            *pLink = addr;
            *(u32*)addr = link | 0x03000000;
            gte_stsz(pOtz);
            pLink = (u32*)(((pScratch->otz >> 4) << 2) + (u32)pOt);
            link = *pLink;
            addr = (u32)&pQuad[2] & 0xFFFFFF;
            *pLink = addr;
            *(u32*)addr = link | 0x03000000;
            pQuad += 3;
        }
        pParticle->unk2 += pParticle->unkA;
        pParticle->unkA += 2;
        if (pParticle->unk2 > 0x10) {
            pParticle->unk2 = 0x10;
        }
        pParticle->unk6--;
    }
}
 */

void BattleClearSpriteTables(void) {
    s32 i;

    for (i = 0; i < 0x21C; i++) {
        D_800926C8[i].unk6 = 0;
        D_800926C8[i].unk4 = 0;
        D_800926C8[i].unk2 = 0;
        D_800926C8[i].unk0 = 0;
    }
    D_8009791B = 0;
    D_80099037 = 0;
    for (i = 0; i < 0xFF; i++) {
        D_800926BC[i].unk6 = 0;
        D_800926BC[i].unk4 = 0;
        D_800926BC[i].unk2 = 0;
        D_800926BC[i].unk0 = 0;
    }
}

void BattleResetLineQuadQueue(void) {
    BattleQuad* pQuad = D_80094818;
    s32 i;

    for (i = 0; i < 100; i++) {
        pQuad[i].unk3 = 3;
        pQuad[i].unk7 = 0x40;
    }
    D_800926B0 = 0;
    D_800926B4 = 0;
}

void BattleQueueLineQuad(s32* pPos, s32* pPos2, u8* pColor) {
    BattleQuad* pQuad;

    if (D_800926B0 < 100) {
        pQuad = &D_80094818[D_800926B0];
        pQuad->v0.vx = pPos[0];
        pQuad->v0.vy = pPos[1];
        pQuad->v0.vz = pPos[2];
        pQuad->v1.vx = pPos2[0];
        pQuad->v1.vy = pPos2[1];
        pQuad->v1.vz = pPos2[2];
        pQuad->r = pColor[0];
        pQuad->g = pColor[1];
        pQuad->b = pColor[2];
        D_800926B0++;
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
    pQuad = D_80094818;
    dx = pScratch->unk00;
    dy = pScratch->unk04;
    dz = pScratch->unk08;
    i = 0;
    while (i < D_800926B4) {
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
    if (D_80092708 == 0 && mode == 0xA) {
        func_8008EB4C(0x24);
    }
    D_80092708 = mode;
}

void BattleDrawTimedSpritePrims(void* pOt) {
    if (D_80092708 != 0) {
        if (D_800928E8 & 2) {
            AddPrim(pOt, &D_800954E0[D_800928A0 * 0x1C]);
            AddPrim(pOt, &D_800954E0[D_800928A0 * 0x1C - 8]);
        }
        D_80092708--;
    }
}

s32 BattleGetTextPrimCount(void) {
    return D_800926DC;
}

void func_8007E634(BattleTimState* pState) {
    TIM_IMAGE tim;
    RECT pad; // unused: reproduces the original's 8-byte frame slot
    BattleDrawRecord* pRec;
    s16* pClut;
    s32 w;
    s32 i;

    D_800926D4[0] = HeapAlloc(0xFA0, 0);
    D_800926D4[1] = HeapAlloc(0xFA0, 0);
    for (i = 0; i < 100; i++) {
        setlen(&D_800926D4[0][i], 0);
        setlen(&D_800926D4[1][i], 0);
    }
    OpenTIM(pState->unk3C);
    ReadTIM(&tim);
    pClut = (s16*)tim.caddr;
    pClut[2] = -0x6F9D;
    pClut[0] = 0;
    pClut[3] = -1;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    D_800926E4 = GetClut(tim.crect->x, tim.crect->y);
    D_800926E0 = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    D_800926DC = 0;
    OpenTIM(pState->unk64);
    ReadTIM(&tim);
    pClut = (s16*)tim.caddr;
    pClut[0] = 0;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    pRec = D_800954D8;
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
    D_800954D8[1] = *pRec;
}


void BattleSetTextCursor(s16 a, s16 b) {
    D_800926E8 = a;
    D_800926EC = b;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E8AC);

void BattleSetTextScale(s32 value) {
    D_800912DC = value;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E964);
/* best 84.08%. Body shape is right: the 0x28-byte POLY_FT4 glyph quad, the
   UV slot widths (u1 = u0 + (glyph[2]|3), v2/v3 = v1 + glyph[3] + 1), the
   colour word and the x advance all reproduce. Four residual differences, all
   in gcc's hands:
   1. prologue: retail loads D_800926DC into v1 BEFORE addiu sp and multiplies
      the packet index with v0; ours loads it into a0, multiplies into v1 and
      therefore needs an extra `move a0,s1` before the jal (c must survive the
      call in s1 either way). Tried: an s32 i local, D_800926DC read twice, a
      pBase local, (POLY_FT4*)D_800926D4[idx] + i and + i*0x28 - all identical
      or worse.
   2. the lbu of glyph[2] is hoisted above the first sw in retail and not here
      (retail also keeps (glyph[2]|3) in s1 across the stores).
   3. the tail wants lhu for D_800926E8 and keeps x+2 in v0; ours emits lh and
      a fresh register even with an explicit (u16) cast.
   4. no extra stack slot: the 0x20 frame with s0/s1/ra is exact.
   Also proved here: func_8007E8AC takes an s32, not a u8 - retail does no andi
   on a0 in its own body or at these call sites, and BattleGetTextWidth still matches
   100.00% with the s32 prototype.
void func_8007E964(s32 c) {
    POLY_FT4* pPkt;
    POLY_FT4* pBase;
    u8* pGlyph;
    s16 x;
    s16 y;
    s32 gw;
    s32 dx;

    if (D_800926DC >= 0x65) {
        return;
    }
    pBase = (POLY_FT4*)D_800926D4[D_800928A0];
    pPkt = pBase + D_800926DC;
    pGlyph = func_8007E8AC(c);
    if (pGlyph == 0) {
        return;
    }
    if (c != 0x28) {
        y = D_800926EC;
        x = D_800926E8;
        *(u_long*)&pPkt->x0 = x | (y << 16);
        gw = pGlyph[2] | 3;
        dx = (gw * D_800912DC) >> 8;
        *(u_long*)&pPkt->x1 = (x + dx) | (y << 16);
        *(u_long*)&pPkt->x2 = x | ((y + pGlyph[3]) << 16);
        *(u_long*)&pPkt->x3 = (x + dx) | ((y + pGlyph[3]) << 16);
        *(u_short*)&pPkt->u0 = pGlyph[0] | (pGlyph[1] << 8);
        *(u_short*)&pPkt->u1 = (pGlyph[0] + gw) | (pGlyph[1] << 8);
        *(u_short*)&pPkt->u2 = pGlyph[0] | ((pGlyph[1] + (pGlyph[3] + 1)) << 8);
        *(u_short*)&pPkt->u3 = (pGlyph[0] + gw) | ((pGlyph[1] + (pGlyph[3] + 1)) << 8);
        pPkt->tpage = D_800926E0;
        pPkt->clut = D_800926E4;
        *(u_long*)&pPkt->r0 = D_800926F0 | (D_800926F4 << 8) | (D_800926F8 << 16) | 0x2C000000;
        D_800926DC++;
    }
    D_800926E8 = (u16)D_800926E8 + 2 + ((pGlyph[2] * D_800912DC) >> 8);
}
 */

s32 BattleGetTextWidth(u8* pText) {
    s32 width = 0;

    while (*pText != 0) {
        u8* pGlyph = func_8007E8AC(*pText);

        pText++;
        width += 2 + ((pGlyph[2] * D_800912DC) >> 8);
    }
    return width;
}

void BattleDrawTextLeft(u8* pText) {
    s32 y = D_800926E8;

    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void BattleByteRegTestReg(u8* pText) {
    s32 y = D_800926E8;

    D_800926E8 -= BattleGetTextWidth(pText) / 2;
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void BattleDrawTextRight(u8* pText) {
    s32 y = D_800926E8;

    D_800926E8 -= BattleGetTextWidth(pText);
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void BattleDrawTextRightOffset(u8* pText, s32 dy) {
    s32 tmp[2]; // unused: reproduces the original's 8-byte frame slot
    s32 y = D_800926E8;

    D_800926E8 = y - dy;
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void BattleSetTextColor(s32 arg) {
    s32 white = 0xFF;

    if (arg != 0) {
        D_800926F4 = 0xFF;
        D_800926F8 = 0;
        D_800926F0 = D_80059488 * 20;
    } else {
        D_800926F0 = white;
        D_800926F4 = white;
        D_800926F8 = white;
    }
}


INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EE68);
/* best 72.66%: the original loads D_80059488 TWICE into two registers (v1 for
   0xF0, a0 for 0xF4) and subtracts with li 0xFF + subu; gcc CSEs the two
   identical loads of the one extern and then strength-reduces 0xFF - (x * 20)
   to nor, since only the stored byte matters. The original subu means its two
   MEMs were two different symbols that both resolve to 0x80059488 (aliases),
   and splat can only name one of them, so the second name cannot be spelled
   without a hand-edited linker/undefined_syms_auto.battling.txt (generated).
   A `*(volatile s32*)&D_80059488` second read does produce two lw, but the
   volatile one cannot be hoisted to the block head and the nor is still there:
   72.03%, no better.
void func_8007EE68(s32 arg) {
    if (arg != 0) {
        D_800926F8 = 0xFF;
        D_800926F0 = 0xFF - D_80059488 * 20;
        D_800926F4 = 0xFF - D_80059488 * 20;
    } else {
        D_800926F0 = 0xFF;
        D_800926F4 = 0xFF;
        D_800926F8 = 0xFF;
    }
}
 */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EEE8);
/* best 90.45%: every instruction and relocation matches except which of the two
   loop induction pointers gets a0 and which gets a1 (the original keeps the
   0x20-byte table pointer in a1 and the 0xC-byte record pointer in a0) and, as
   a consequence, where the HeapAlloc result is copied into a3 in the loop
   preheader. Frame (0x20, ra at 0x18), the s0/s1 split, the
   g_GameState+0x1930 lhu, the filter compare and the D_80092888
   read-modify-write are exact. Tried: index and pointer forms for both
   induction variables and every mixture, i*3 over an s32 array, a nested if
   with continue, split increment, six declaration orders, and -G0 / -O3 /
   gcc 2.6.0 / gcc 2.7.2-cdk (90.45 or worse under all). What is left is a
   global.c allocator tie-break between two allocnos with identical ref counts
   and live ranges.
void func_8007EEE8(s32 arg) {
    s32 i;
    s32 limit;
    s32** pQueue;

    limit = g_GameState.unk1930[0];
    pQueue = HeapAlloc(0xC4, 1);
    D_800928EC = pQueue;
    D_80092888 = 0;
    for (i = 0; i < 0x31; i++) {
        if (arg == 0 || D_80092874[i].unk4 <= limit) {
            pQueue[D_80092888++] = &D_80091964[i];
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

    D_8009270C = HeapAlloc(0x3D4, 0);
    pSprite = D_8009270C;
    count = 0x1FF;
    for (row = 0; row < 7; row++) {
        y = 0x1A0 + row * 0x20;
        for (col = 0; col < 7; col++) {
            x = col * 0x40;
            pSprite->unk0 = 0x200;
            pSprite->unk2 = count--;
            pSprite->unk4 = 0x80;
            pSprite->unk6 = 1;
            pSprite->unk8 = y;
            pSprite->unkA = x;
            pSprite->unkC = 0x1E;
            pSprite->unkE = 0x40;
            pSprite++;
        }
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007F05C);
/* best 84.45% (504 bytes against retail's 512). Whole body is right: the flag
   x offset (0xD3/0x33), the two-part idx wrap against D_80092888, the
   0x14-byte sprite lookup through D_800928EC/D_8009270C, the shade==0x40 fast
   path with (x,0x30)-(x+0x3C,0x70), the shaded path's colour word
   (shade+0x40 in r/g/b, code 0x2C) and its quad (x0 = x+4-k, y0 = 0x34-k,
   w = 0x34+2k, h = 0x38+2k, k = shade>>4), the four u/v words
   (u = sprite->unk8*2 with u1 = u+0x3B, v1 = sprite->unkA+0x3F) and the
   GetClut/GetTPage/AddPrim tail are all exact.
   Two residuals, both scheduling:
   1. retail spends one addiu +0x40 in the bne delay slot and then gets k back
      with addiu -0x40 in the shaded arm; ours keeps shade in a1 and adds 0x40
      inside the arm, i.e. one instruction shorter (the 8 missing bytes).
   2. x1 = (x0 + 0x34) + 2k in retail, ours folds to x0 + (0x34 + 2k).
   Tried: one setlen before the branch (the permuter prefers it in its own
   metric but it costs retail's duplicate store), a grow/2k local, splitting
   x1/y1 into two statements, an index local, every declaration order. The
   permuter ran 3 minutes (base 1855 -> 1465) and only found the setlen move.
   Sprite field types here are s16: that is what makes the uv stores read
   unkA with lhu (truncated store) and the GetTPage/GetClut args with lh.
void func_8007F05C(s32 idx, POLY_FT4* pPkt, s32 flag, s32 x, s32 shade) {
    BattleSprite* pSprite;
    s32 count;
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
    count = D_80092888;
    if (idx > count - 1) {
        idx -= count;
    }
    if (idx < 0) {
        idx += count;
    }
    count = *D_800928EC[idx];
    pSprite = &D_8009270C[count];
    if (shade == 0x40) {
        setlen(pPkt, 9);
        pPkt->code = 0x2D;
        *(u_long*)&pPkt->x0 = x | (0x30 << 16);
        *(u_long*)&pPkt->x1 = (x + 0x3C) | (0x30 << 16);
        *(u_long*)&pPkt->x2 = x | (0x70 << 16);
        *(u_long*)&pPkt->x3 = (x + 0x3C) | (0x70 << 16);
    } else {
        s32 color = shade + 0x40;

        setlen(pPkt, 9);
        *(u_long*)&pPkt->r0 = (color << 8) | color | (color << 16) | 0x2C000000;
        half = shade >> 4;
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
    u = pSprite->unk8 * 2;
    *(u_short*)&pPkt->u0 = u | (pSprite->unkA << 8);
    *(u_short*)&pPkt->u1 = u + 0x3B | (pSprite->unkA << 8);
    *(u_short*)&pPkt->u2 = u | ((pSprite->unkA + 0x3F) << 8);
    *(u_short*)&pPkt->u3 = u + 0x3B | ((pSprite->unkA + 0x3F) << 8);
    pPkt->clut = GetClut(pSprite->unk0, pSprite->unk2);
    pPkt->tpage = GetTPage(1, 0, pSprite->unk8 & 0xFF80, pSprite->unkA);
    AddPrim(D_80092938, pPkt);
}
 */

// 4-s16 set of cursor-arrow part offsets, indexed by the sign of the cursor's
// step: entry 0 for a leftward step, entry 1 for a rightward one.
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} BattleCursorOffset; // Size: 0x8

extern BattleCursorOffset D_800912E0[];
extern POLY_FT4 D_80099DA8[];
extern u8 D_8006FE88[];
extern s32 D_80092700;
extern s32 D_80092704;
extern s32 D_80092710;
extern s32 D_80092714;
extern s32 D_80092718;
extern s32 D_8009271C;
extern s32 D_80092720;
extern s32 D_80092724;
extern s32 D_80092728;
extern s32 D_8009272C;
extern s32 D_80092730;

void func_8007F05C(s32 idx, POLY_FT4* pPkt, s32 flag, s32 x, s32 shade);
s32 BattleTestSlotBit(s32 slot);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007F258);
/* best 81.01% (1456 bytes against retail's 1496). The whole HUD tail from
   BattleSetTextCursor(0xA0,0x78) to the closing BattleSetTextColor(0) is exact,
   as is the 0x50 frame: it needs an unused s32 tmp[8] local (retail spends 32
   bytes on nothing), and D_80099DA8[D_800928A0 * 10] reproduces the mul-by-0x190
   chain. What is left, all inside the two cursor blocks:
   1. THE ALIAS TRAP, and the reason this cannot reach 100.00%: retail stores the
      step to D_80092718, then loads D_80092718, stores it back to D_80092718 and
      then to D_8009271C (sw / lw / sw / sw, four instructions, twice). A store
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
   non-negative one, indexed by the s32 compare D_80092718 >= 0, and the compare
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

    pPkt = &D_80099DA8[D_800928A0 * 10];

    delta = D_80092714 - D_80092700;
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
        D_80092718 = step;
        D_8009271C = D_80092718;
    }
    delta = D_80092720 - D_80092704;
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
        D_80092724 = step;
        D_80092728 = D_80092724;
    }

    step = D_80092718;
    if (step != 0) {
        if (step > 0) {
            step -= 2;
        } else {
            step += 2;
        }
        D_80092718 = step;
    }
    step = D_8009271C;
    if (step != 0) {
        if (step > 0) {
            step -= 4;
        } else {
            step += 4;
        }
        D_8009271C = step;
    }
    step = D_80092724;
    if (step != 0) {
        if (step > 0) {
            step -= 2;
        } else {
            step += 2;
        }
        D_80092724 = step;
    }
    step = D_80092728;
    if (step != 0) {
        if (step > 0) {
            step -= 4;
        } else {
            step += 4;
        }
        D_80092728 = step;
    }

    sign = D_80092718 >= 0;
    step = D_8009271C;
    if (step < 0) {
        step = -step;
    }
    shade = ((0x24 - step) << 6) / 0x24;
    D_80092714 = D_80092700;
    D_80092720 = D_80092704;
    func_8007F05C(D_80092700, pPkt++, 0, D_8009271C, shade);
    if ((D_80092710 & 1) == 0) {
        pOff = &D_800912E0[sign];
        step = D_80092718;
        if (step < 0) {
            step = -step;
        }
        shade = ((0x24 - step) << 6) / 0x24;
        func_8007F05C(D_80092700 + pOff->unk0, pPkt++, 0, D_80092718 + pOff->unk6, shade);
        func_8007F05C(D_80092700 + pOff->unk2, pPkt++, 0, -0x24, 0);
        func_8007F05C(D_80092700 + pOff->unk4, pPkt++, 0, 0x24, 0);
    }

    sign = D_80092724 >= 0;
    step = D_80092728;
    if (step < 0) {
        step = -step;
    }
    shade = ((0x24 - step) << 6) / 0x24;
    func_8007F05C(D_80092704, pPkt++, 1, D_80092728, shade);
    if ((D_80092710 & 2) == 0) {
        pOff = &D_800912E0[sign];
        step = D_80092724;
        if (step < 0) {
            step = -step;
        }
        shade = ((0x24 - step) << 6) / 0x24;
        func_8007F05C(D_80092704 + pOff->unk0, pPkt++, 1, D_80092724 + pOff->unk6, shade);
        func_8007F05C(D_80092704 + pOff->unk2, pPkt++, 1, -0x24, 0);
        func_8007F05C(D_80092704 + pOff->unk4, pPkt, 1, 0x24, 0);
    }

    BattleSetTextCursor(0xA0, 0x78);
    BattleByteRegTestReg(D_8006FE88);
    BattleSetTextCursor(0x50, 0x78);
    if (BattleTestSlotBit(D_800928EC[D_80092700][0]) != 0) {
        func_8007EE68(D_8009272C);
    } else {
        BattleSetTextColor(D_8009272C);
    }
    BattleByteRegTestReg((u8*)D_800928EC[D_80092700][2]);
    BattleSetTextCursor(0xF0, 0x78);
    if (BattleTestSlotBit(D_800928EC[D_80092704][0]) != 0) {
        func_8007EE68(D_80092730);
    } else {
        BattleSetTextColor(D_80092730);
    }
    BattleByteRegTestReg((u8*)D_800928EC[D_80092704][2]);
    BattleSetTextColor(0);
}
 */

void BattleClearTextSpriteEntries(void) {
    D_80092740 = 0;
    D_8009273C = 0;
    D_80092744 = 0;
}
