#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

extern u8 g_BattleSlotBits[];
extern u8 *g_BattleSpanTableMax;
extern u8 *g_BattleSpanTableMin;
extern u8 g_BattleDrawBufferIndex;
extern VECTOR g_BattleViewOrigin;
extern void func_800316C0(void *pOt, void *pPrim);

typedef struct {
    /* 0x00 */ LINE_F2 prim[2];
    /* 0x20 */ SVECTOR pos[2];
    /* 0x30 */ s16 count;
    /* 0x32 */ u8 pad32[0x2];
} BattlingSlot; // Size: 0x34

extern BattlingSlot g_BattleLineSlots[];

// The count word of a line slot, addressed through its own symbol: splat puts
// D_80095968 0x30 bytes past g_BattleLineSlots, i.e. at slot 0's count, and the
// original object reaches it with that symbol rather than off the slot base.
typedef struct {
    /* 0x00 */ s16 count;
    /* 0x02 */ u8 pad2[0x32];
} BattlingLineSlotCount; // Size: 0x34

extern BattlingLineSlotCount D_80095968[];

extern void *BattleAllocLineSlot();
extern void func_80085EC8(void *);

// The gauge primitives of one draw buffer: the shared tpage/background prims first,
// then the bar templates (barPrim), their two per-side copies, the gauge segments
// and finally the four quads the side meters use.
typedef struct {
    /* 0x000 */ DR_TPAGE tpage[2];
    /* 0x010 */ POLY_G3 background[4];
    /* 0x080 */ POLY_F4 barPrim[6];
    /* 0x110 */ DR_TPAGE barTpage;
    /* 0x118 */ POLY_F4 barFront[6];
    /* 0x1A8 */ POLY_F4 barBack[6];
    /* 0x238 */ POLY_F3 gauge[6];
    /* 0x2B0 */ POLY_F4 meter[4];
} BattlingGaugeEnv; // Size: 0x310

extern BattlingGaugeEnv g_BattleDrawEnvPair[2];

extern s32 g_BattleAppendEntryCount;
extern u32 g_NearColor;
extern u8 D_80091834[];
extern u8 D_800918B4[];
/* The battle span render params block BattleRenderShadedSpans copies to
   0x1F800120 (misc2_grid.c's BattleGridParams): four 8-byte shade entries, then
   the tpage/clut pair of each of the four span textures. */
typedef struct {
    /* 0x0 */ u16 tpage;
    /* 0x2 */ u16 clut;
} BattlingSpanTexPage; // Size: 0x4

typedef struct {
    /* 0x00 */ u8 shades[0x20];
    /* 0x20 */ BattlingSpanTexPage tex[4];
} BattlingSpanParams; // Size: 0x30

extern BattlingSpanParams g_BattleSpanRenderParams;
extern u8 *D_80092854;
extern u8 *g_BattleSpanBuffer;
extern long MathGetLargestVectorComponent(long, long, long);
extern void func_800732AC(void *, void *, s32);
extern void func_80072D18(s32, s32, s32);

typedef struct {
    /* 0x00 */ s16 vx;
    /* 0x02 */ s16 vy;
    /* 0x04 */ s16 vz;
    /* 0x06 */ u8 pad6[0x62];
} BattlingEntry68; // Size: 0x68

extern BattlingEntry68 D_8009A988[];

typedef union {
    /* 0x0 */ u32 word;
    /* 0x0 */ u8 bytes[4];
} BattlingPacketWord;

typedef struct {
    /* 0x00 */ u8 pad0[0x3];
    /* 0x03 */ u8 tagLength;
    /* 0x04 */ BattlingPacketWord color;
    /* 0x08 */ u8 pad8[0x4];
    /* 0x0C */ u16 uv0;
    /* 0x0E */ u16 clut;
    /* 0x10 */ u8 pad10[0x4];
    /* 0x14 */ u16 uv1;
    /* 0x16 */ u16 tpage;
    /* 0x18 */ u8 pad18[0x4];
    /* 0x1C */ u16 uv2;
    /* 0x1E */ u8 pad1E[0x6];
    /* 0x24 */ u16 uv3;
    /* 0x26 */ u8 pad26[0x2];
} BattlingPacket; // Size: 0x28

typedef struct {
    /* 0x0000 */ u8 pad0[0x1604];
    /* 0x1604 */ BattlingPacket packet[2];
} BattlingPacketOwner; // Size: 0x1654

extern u16 g_BattleQuadTexPage;
extern u16 g_BattleQuadClut;
extern u8 g_BattleQuadTexU;
extern u8 g_BattleQuadTexV;

extern u8 *g_BattleRootOrderingTable;
extern s32 g_SpuChannelMask;

// The bar primitives the gauge draws into, and the two texture rows the gauge
// quads sample from.
extern POLY_FT4 D_80095698[16];
extern POLY_FT4 g_BattleGaugeBarPacket1[];
extern DR_TPAGE D_80095918[4];
extern u8 g_BattleBarTexV;
extern u8 g_BattleGaugeFill0;
extern u8 g_BattleGaugeFill1;

// The gauge state of one battle side: where the bar sits (10.6 fixed point), how
// far it is filled and how faded it is.
typedef struct {
    /* 0x000 */ u8 unk000[0xB6];
    /* 0x0B6 */ u16 position;
    /* 0x0B8 */ u8 unk0B8[0x9];
    /* 0x0C1 */ u8 barLength;
    /* 0x0C2 */ u8 fade;
    /* 0x0C3 */ u8 unk0C3[0x2F];
    /* 0x0F2 */ s16 segments;
} BattlingGaugeState; // Size: 0xF4

extern s32 BattleSetGaugeBarPrimColor(BattlingGaugeState *pState, POLY_F4 *pPrim);
extern void BattleQueueGaugeBarPrims(u8 *pOt, void *pBase, s32 value, s32 flag);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087068);
/* Best 99.75%, at retail's exact size (1412 bytes). Everything matches but one
   register pair, twice per function, inside the two fade loops: retail keeps the
   reloaded fade byte in $v0 and the multiplied value in $v1
   (lbu v0,194 / sll v1,v0,8 / subu v1,v1,v0 / sra v1,v1,8 / addu v1,a1,v1 /
   sb v1,428), gcc 2.7.2 gives the mirror image (lbu v1 ... sb v0,428). Same
   instruction multiset, same byte count, 17 instruction diffs.
   Load-bearing shapes found on the way: the fade reload must be the loop's first
   statement (else the bar pointer takes $v1 and the reload $a0); the narrow u8
   copy of the brightness produces retail's 'move a3,a1'; and one variable reused
   for the segment count, the bar position and the loop counter is what puts the
   bar pointer in $a0, the count in $s2 and gives retail's six saved registers
   (without it the body is 8 bytes short). Also needed: the multiply as its own
   local, and the fade colour block's invariants written inside the loop so loop
   invariant motion hoists them after the 'i = 0' initialisation.
   Two permuter runs (4 and 3 minutes; 1702 and 1068 iterations) found nothing
   better (second run PERMUTER_NONE, base=90 best=90). Not a preset question:
   gcc-2.7.2-cdk scores 95.46 on this body.
   The 0x310 layout below is confirmed by func_80085EC8, which initialises the
   same block, and by misc10's parked func_800868E0. g_BattleBarTexV (a byte at
   g_BattleScreenWidth + 4) is not in any symbol file yet, so the body needs
   'g_BattleBarTexV = 0x80092860;' in config/symbol_addrs.battling.txt to link.
void func_80087068(BattlingGaugeState *arg0, BattlingGaugeState *arg1) {
    BattlingGaugeEnv *pEnv;
    POLY_FT4 *pPacket;
    s32 i;
    s32 fade;
    s32 bright;
    s32 half;
    s32 scaledFade;
    u8 barBright;

    pEnv = &g_BattleDrawEnvPair[g_BattleDrawBufferIndex];

    i = arg0->segments;
    if (i > 0) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->gauge[0]);
    }
    if (i > 1) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->gauge[1]);
    }
    if (i > 2) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->gauge[2]);
    }

    i = arg1->segments;
    if (i > 0) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->gauge[3]);
    }
    if (i > 1) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->gauge[4]);
    }
    if (i > 2) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->gauge[5]);
    }

    if (BattleSetGaugeBarPrimColor(arg0, &pEnv->meter[2]) != 0) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->meter[2]);
    }
    if (BattleSetGaugeBarPrimColor(arg1, &pEnv->meter[3]) != 0) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->meter[3]);
    }

    AddPrim(g_BattleRootOrderingTable, &D_80095698[g_BattleDrawBufferIndex]);
    AddPrim(g_BattleRootOrderingTable, &D_80095698[g_BattleDrawBufferIndex + 2]);
    AddPrim(g_BattleRootOrderingTable, &D_80095698[g_BattleDrawBufferIndex + 4]);
    AddPrim(g_BattleRootOrderingTable, &D_80095698[g_BattleDrawBufferIndex + 6]);
    AddPrim(g_BattleRootOrderingTable, &D_80095918[g_BattleDrawBufferIndex]);
    AddPrim(g_BattleRootOrderingTable, &D_80095698[g_BattleDrawBufferIndex + 8]);
    AddPrim(g_BattleRootOrderingTable, &D_80095698[g_BattleDrawBufferIndex + 10]);
    AddPrim(g_BattleRootOrderingTable, &D_80095918[g_BattleDrawBufferIndex + 2]);
    AddPrim(g_BattleRootOrderingTable, &D_80095698[g_BattleDrawBufferIndex + 14]);
    AddPrim(g_BattleRootOrderingTable, &D_80095698[g_BattleDrawBufferIndex + 12]);
    AddPrim(g_BattleRootOrderingTable, &pEnv->meter[0]);
    AddPrim(g_BattleRootOrderingTable, &pEnv->meter[1]);
    AddPrim(g_BattleRootOrderingTable, &pEnv->background[0]);
    AddPrim(g_BattleRootOrderingTable, &pEnv->background[2]);

    BattleQueueGaugeBarPrims(g_BattleRootOrderingTable, &pEnv->barPrim[0], g_BattleGaugeFill0, 0);
    BattleQueueGaugeBarPrims(g_BattleRootOrderingTable, &pEnv->barPrim[3], g_BattleGaugeFill1, 1);

    i = ((s16)arg0->position) >> 6;
    pPacket = &D_80095698[g_BattleDrawBufferIndex + 12];
    pPacket->r0 = (i >= 0x39) ? (g_SpuChannelMask << 3) : 0xFF;
    pPacket->y0 = pPacket->y1 = 0x60 - i;
    pPacket->v0 = pPacket->v1 = g_BattleBarTexV - (i - 0x40);

    i = ((s16)arg1->position) >> 6;
    pPacket = g_BattleGaugeBarPacket1 + g_BattleDrawBufferIndex;
    pPacket->r0 = (i >= 0x39) ? (g_SpuChannelMask << 3) : 0xFF;
    pPacket->y0 = pPacket->y1 = 0x60 - i;
    pPacket->v0 = pPacket->v1 = g_BattleBarTexV - (i - 0x40);

    if (arg0->fade != 0) {
        fade = 0x100 - arg0->fade;
        for (i = 0; i < 3; i++) {
            bright = (fade * 3) >> 3;
            barBright = bright;
            half = fade >> 1;
            scaledFade = arg0->fade;
            scaledFade *= 0xFF;
            pEnv->barBack[i].g0 = barBright;
            pEnv->barBack[i].b0 = half;
            pEnv->barBack[i].r0 = bright + (scaledFade >> 8);
        }
        BattleQueueGaugeBarPrims(g_BattleRootOrderingTable, &pEnv->barBack[0], arg0->barLength, 0);
    }

    if (arg1->fade != 0) {
        fade = 0x100 - arg1->fade;
        for (i = 3; i < 6; i++) {
            bright = (fade * 3) >> 3;
            barBright = bright;
            half = fade >> 1;
            scaledFade = arg1->fade;
            scaledFade *= 0xFF;
            pEnv->barBack[i].g0 = barBright;
            pEnv->barBack[i].b0 = half;
            pEnv->barBack[i].r0 = bright + (scaledFade >> 8);
        }
        BattleQueueGaugeBarPrims(g_BattleRootOrderingTable, &pEnv->barBack[3], arg1->barLength, 1);
    }

    for (i = 0; i < 6; i++) {
        AddPrim(g_BattleRootOrderingTable, &pEnv->barFront[i]);
    }
    AddPrim(g_BattleRootOrderingTable, &pEnv->tpage[0]);
}
*/

void BattleInitDrawEnvPair(void) {
    func_80085EC8(g_BattleDrawEnvPair);
    g_BattleDrawEnvPair[1] = g_BattleDrawEnvPair[0];
}

void BattleClearSpanTables(void) {
    s32 i;

    for (i = 0; i < 0x80; i++) {
        g_BattleSpanTableMax[i] = 0;
        g_BattleSpanTableMin[i] = 0xFF;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087698);
/* Best 81.23%: two clusters left. (1) gcc puts the live interpolated value in
   a0 and the table byte in v1; the original has them the other way round
   (v1 / a0). (2) the original's loop tail is i++; slt; bnez; addu(cur += step)
   in the branch delay slot, gcc emits addu before the slt and a nop in the
   slot. Tried: for-increment order i++,cur += step and cur += step,i++;
   t declared before v. Semantics: interpolate a shaded span into the two
   buffers g_BattleSpanTableMin / g_BattleSpanTableMax, clamped by the tables D_80091834 /
   D_800918B4+1.
void func_80087698(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 cur = arg0 << 8;
    s32 step = ((arg2 - arg0) << 8) / (arg3 - arg1);
    s32 t;
    s32 v;
    s32 i;
    u8 *p;

    if (arg1 == arg3) {
        return;
    }
    if (arg3 < arg1) {
        t = arg0;
        arg0 = arg2;
        arg2 = t;
        t = arg1;
        arg1 = arg3;
        arg3 = t;
        cur = arg0 << 8;
        step = ((arg2 - arg0) << 8) / (arg3 - arg1);
    }
    for (i = arg1; i < arg3; cur += step, i++) {
        if (i < 0) {
            continue;
        }
        if (i >= 0x80) {
            return;
        }
        v = cur >> 8;
        p = g_BattleSpanTableMin + i;
        if (v < *p) {
            t = D_80091834[i];
            if (v < t) {
                v = t;
            }
            *p = v;
        }
        p = g_BattleSpanTableMax + i;
        if (*p < v) {
            t = D_800918B4[i] + 1;
            if (t < v) {
                v = t;
            }
            *p = v;
        }
    }
}
*/

void BattleRenderShadedSpans(s32 arg0, s32 arg1, s32 arg2) {
    g_NearColor = (g_NearColor & 0xFFFFFF) | 0x24000000;
    gte_ldrgb(&g_NearColor);
    func_800732AC((void *)0x1F800120, &g_BattleSpanRenderParams, 0x30);
    func_80072D18(arg0, arg1, arg2);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087830);
/* Best 80.95%: the original CSEs &D_80092854 for the store and the following load
   and keeps it in s0 (frame 0x18); gcc recomputes the address with $at each time
   (frame 0x10, 4 bytes short). Tried: p = g + 7, p = g then p += 7 (80.95),
   p initialized at declaration (74.72), explicit u8 **pBuf = &D_80092854.
void func_80087830(void) {
    s32 i;
    u8 *p;

    g_BattleSpanTableMax = (u8 *)0x1F800000;
    g_BattleSpanTableMin = (u8 *)0x1F800080;
    D_80092854 = HeapAlloc(0xE100, 0);
    g_BattleSpanBuffer = HeapAlloc(0xE100, 0);
    p = D_80092854 + 7;
    for (i = 0; i < 0x708; i++) {
        p[-4] = 7;
        p[0] = 0x24;
        p += 0x20;
    }
    func_800732AC(g_BattleSpanBuffer, D_80092854, 0xE100);
}
*/
/* The battle texture archive after LZSSHeapDecompress and
   ResolveArchiveEntryPointers: a pointer count, then one resolved pointer per
   resource, each naming a TIM. Only the slots this unit loads are named. */
typedef struct {
    /* 0x00 */ s32 pointerCount;
    /* 0x04 */ u_long *unk04[0xD];
    /* 0x38 */ u_long *pQuadTim;
    /* 0x3C */ u_long *unk3C[3];
    /* 0x48 */ u_long *pSpanTim[4];
    /* 0x58 */ u_long *unk58[6];
    /* 0x70 */ u_long *pTim[9];
} BattlingTextureArchive; // Size: 0x94

void BattlingLoadTextureArchive(BattlingTextureArchive *pArchive) {
    TIM_IMAGE tim;
    u16 *pClut;
    u16 *pTpage;
    u16 *pPixels;
    SVECTOR pad; // retail's frame has eight bytes the C never touches
    s32 stpBits;
    s32 i;

    pClut = &g_BattleSpanRenderParams.tex[0].clut;
    pTpage = &g_BattleSpanRenderParams.tex[0].tpage;
    for (i = 0; i < 4; i++) {
        OpenTIM(pArchive->pSpanTim[i]);
        ReadTIM(&tim);
        pClut[i * 2] = GetClut(tim.crect->x, tim.crect->y);
        pTpage[i * 2] = GetTPage(1, 1, tim.prect->x, tim.prect->y);
        LoadImage(tim.crect, tim.caddr);
        LoadImage(tim.prect, tim.paddr);
    }

    OpenTIM(pArchive->pQuadTim);
    ReadTIM(&tim);
    g_BattleQuadClut = GetClut(tim.crect->x, tim.crect->y);
    g_BattleQuadTexPage = GetTPage(0, 2, tim.prect->x, tim.prect->y);
    g_BattleQuadTexU = tim.prect->x * 4;
    g_BattleQuadTexV = tim.prect->y;

    pPixels = (u16 *)tim.caddr;
    pPixels[0] = 0;
    stpBits = -0x8000;
    for (i = 1; i < 0x10; i++) {
        pPixels[i] |= stpBits;
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);

    for (i = 0x1C; i < 0x25; i++) {
        OpenTIM(pArchive->pTim[i - 0x1C]);
        ReadTIM(&tim);
        LoadImage(tim.crect, tim.caddr);
        LoadImage(tim.prect, tim.paddr);
    }
}

void BattleInitSemiTransQuadPacket(BattlingPacketOwner *arg0) {
    BattlingPacket *p = arg0->packet;

    p->color.word = 0x2C101010;
    p->tagLength = 9;
    p->color.bytes[3] |= 2;
    p->clut = g_BattleQuadClut;
    p->tpage = g_BattleQuadTexPage;
    p->uv0 = g_BattleQuadTexU | (g_BattleQuadTexV << 8);
    p->uv1 = (g_BattleQuadTexU + 0x3F) | (g_BattleQuadTexV << 8);
    p->uv2 = g_BattleQuadTexU | ((g_BattleQuadTexV + 0x3F) << 8);
    p->uv3 = (g_BattleQuadTexU + 0x3F) | ((g_BattleQuadTexV + 0x3F) << 8);
    arg0->packet[1] = arg0->packet[0];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087B74);

void BattleAppendEntry(s32 *arg0) {
    s32 i = g_BattleAppendEntryCount;

    if (i < 0x1F) {
        BattlingEntry68 *pBase = D_8009A988;
        BattlingEntry68 *p = pBase + i;
        p->vx = arg0[0];
        p->vy = arg0[1];
        p->vz = arg0[2];
        g_BattleAppendEntryCount = i + 1;
    }
}
// One flat line of a battle target's cross, colour and code written as the
// single packed word the tag length and colour live in.
typedef struct {
    /* 0x0 */ u8 pad0[0x3];
    /* 0x3 */ u8 tagLength;
    /* 0x4 */ BattlingPacketWord color;
    /* 0x8 */ s16 x0;
    /* 0xA */ s16 y0;
    /* 0xC */ s16 x1;
    /* 0xE */ s16 y1;
} BattlingCrossLine; // Size: 0x10

// A battle target's cross in one draw buffer: the X, Y and Z axis segments.
typedef struct {
    /* 0x00 */ BattlingCrossLine line[3];
} BattlingCrossPrims; // Size: 0x30

// The cross, one set of prims per draw buffer, then the centre it sits on.
typedef struct {
    /* 0x00 */ BattlingCrossPrims prims[2];
    /* 0x60 */ SVECTOR pos;
} BattlingCrossEntry; // Size: 0x68

extern BattlingCrossEntry D_8009A928[];

void func_80087EA0(u8 *pOt) {
    SVECTOR vtx[6];
    BattlingCrossEntry *pEntry;
    s32 i;
    s32 j;

    for (i = 0; i < g_BattleAppendEntryCount; i++) {
        pEntry = &D_8009A928[i];

        vtx[0].vx = pEntry->pos.vx - 0x20;
        vtx[0].vy = pEntry->pos.vy;
        vtx[0].vz = pEntry->pos.vz;
        vtx[1].vx = pEntry->pos.vx + 0x20;
        vtx[1].vy = pEntry->pos.vy;
        vtx[1].vz = pEntry->pos.vz;
        vtx[2].vx = pEntry->pos.vx;
        vtx[2].vy = pEntry->pos.vy - 0x20;
        vtx[2].vz = pEntry->pos.vz;
        vtx[3].vx = pEntry->pos.vx;
        vtx[3].vy = pEntry->pos.vy + 0x20;
        vtx[3].vz = pEntry->pos.vz;
        vtx[4].vx = pEntry->pos.vx;
        vtx[4].vy = pEntry->pos.vy;
        vtx[4].vz = pEntry->pos.vz - 0x20;
        vtx[5].vx = pEntry->pos.vx;
        vtx[5].vy = pEntry->pos.vy;
        vtx[5].vz = pEntry->pos.vz + 0x20;

        for (j = 0; j < 6; j++) {
            vtx[j].vx -= g_BattleViewOrigin.vx;
            vtx[j].vy -= g_BattleViewOrigin.vy;
            vtx[j].vz -= g_BattleViewOrigin.vz;
        }

        gte_ldv3(&vtx[0], &vtx[1], &vtx[2]);
        gte_rtpt();
        gte_stsxy3(&pEntry->prims[g_BattleDrawBufferIndex].line[0].x0,
                   &pEntry->prims[g_BattleDrawBufferIndex].line[0].x1,
                   &pEntry->prims[g_BattleDrawBufferIndex].line[1].x0);
        gte_ldv3(&vtx[3], &vtx[4], &vtx[5]);
        gte_rtpt();
        gte_stsxy3(&pEntry->prims[g_BattleDrawBufferIndex].line[1].x1,
                   &pEntry->prims[g_BattleDrawBufferIndex].line[2].x0,
                   &pEntry->prims[g_BattleDrawBufferIndex].line[2].x1);

        pEntry->prims[g_BattleDrawBufferIndex].line[0].tagLength = 3;
        pEntry->prims[g_BattleDrawBufferIndex].line[0].color.word = 0x400000FF;
        pEntry->prims[g_BattleDrawBufferIndex].line[1].tagLength = 3;
        pEntry->prims[g_BattleDrawBufferIndex].line[1].color.word = 0x4000FF00;
        pEntry->prims[g_BattleDrawBufferIndex].line[2].tagLength = 3;
        pEntry->prims[g_BattleDrawBufferIndex].line[2].color.word = 0x40FF0000;

        func_800316C0(pOt, &pEntry->prims[g_BattleDrawBufferIndex].line[0]);
        func_800316C0(pOt, &pEntry->prims[g_BattleDrawBufferIndex].line[1]);
        func_800316C0(pOt, &pEntry->prims[g_BattleDrawBufferIndex].line[2]);
    }
    g_BattleAppendEntryCount = 0;
}

void *BattleAllocLineSlot(s32 *arg0, s32 *arg1, s32 arg2) {
    s32 i;

    for (i = 0; i < 0x64; i++) {
        BattlingSlot *p = &g_BattleLineSlots[i];

        if (p->count == 0) {
            p->count = 1;
            p->pos[0].vx = arg0[0];
            p->pos[0].vy = arg0[1];
            p->pos[0].vz = arg0[2];
            p->pos[1].vx = arg1[0];
            p->pos[1].vy = arg1[1];
            p->pos[1].vz = arg1[2];
            p->prim[0].r0 = (arg2 & 2) * 0x7F;
            p->prim[0].g0 = (arg2 & 4) * 0x3F;
            p->prim[0].b0 = (arg2 & 1) * 0xFF;
            p->prim[1].r0 = (arg2 & 2) * 0x7F;
            p->prim[1].g0 = (arg2 & 4) * 0x3F;
            p->prim[1].b0 = (arg2 & 1) * 0xFF;
            return p;
        }
    }
    return NULL;
}
void BattleCreateLineSlot(s32 arg0, s32 *arg1, s32 arg2, s16 arg3) {
    BattlingSlot *pSlot = BattleAllocLineSlot(arg0, arg1, arg2);

    if (pSlot != NULL) {
        pSlot->count = arg3;
    }
}

void BattleClearLineSlots(void) {
    s32 offset;

    // A byte-offset walk: the original tests the offset itself, not an index.
    for (offset = 0x141C; offset >= 0; offset -= 0x34) {
        *(s16 *)((u8 *)D_80095968 + offset) = 0;
    }
}

void BattleDrawLineSlots(void *arg0) {
    SVECTOR pos[2];
    s32 i;

    for (i = 0; i < 0x64; i++) {
        if (D_80095968[i].count != 0) {
            BattlingSlot *pSlot = &g_BattleLineSlots[i];

            pSlot->count--;
            pos[0] = pSlot->pos[0];
            pos[1] = pSlot->pos[1];
            pos[0].vx -= g_BattleViewOrigin.vx;
            pos[0].vy -= g_BattleViewOrigin.vy;
            pos[0].vz -= g_BattleViewOrigin.vz;
            pos[1].vx -= g_BattleViewOrigin.vx;
            pos[1].vy -= g_BattleViewOrigin.vy;
            pos[1].vz -= g_BattleViewOrigin.vz;
            gte_ldv01(&pos[0], &pos[1]);
            gte_rtpt();
            gte_stsxy01(&pSlot->prim[g_BattleDrawBufferIndex].x0, &pSlot->prim[g_BattleDrawBufferIndex].x1);
            setLineF2(&pSlot->prim[g_BattleDrawBufferIndex]);
            func_800316C0(arg0, &pSlot->prim[g_BattleDrawBufferIndex]);
        }
    }
}

void BattleNormalizeVector(VECTOR *arg0, VECTOR *arg1) {
    VECTOR normal;
    s32 sqrt;
    s32 largestComponent;

    normal = *arg0;
    largestComponent = MathGetLargestVectorComponent(normal.vx, normal.vy, normal.vz);
    if (largestComponent < 0) {
        largestComponent = -largestComponent;
    }
    sqrt = SquareRoot0(largestComponent);
    normal.vx = normal.vx / sqrt;
    normal.vy = normal.vy / sqrt;
    normal.vz = normal.vz / sqrt;
    VectorNormal(&normal, arg1);
}

void BattleNormalizeVectorToShort(VECTOR *arg0, SVECTOR *arg1) {
    VECTOR normal;
    s32 sqrt;
    s32 largestComponent;

    normal = *arg0;
    largestComponent = MathGetLargestVectorComponent(normal.vx, normal.vy, normal.vz);
    if (largestComponent < 0) {
        largestComponent = -largestComponent;
    }
    sqrt = SquareRoot0(largestComponent);
    normal.vx = normal.vx / sqrt;
    normal.vy = normal.vy / sqrt;
    normal.vz = normal.vz / sqrt;
    VectorNormalS(&normal, arg1);
}

void BattleNormalizeShortToShort(SVECTOR *arg0, SVECTOR *arg1) {
    VECTOR normal;
    s32 sqrt;
    s32 largestComponent;

    normal.vx = arg0->vx;
    normal.vy = arg0->vy;
    normal.vz = arg0->vz;
    largestComponent = MathGetLargestVectorComponent(normal.vx, normal.vy, normal.vz);
    if (largestComponent < 0) {
        largestComponent = -largestComponent;
    }
    sqrt = SquareRoot0(largestComponent);
    normal.vx = normal.vx / sqrt;
    normal.vy = normal.vy / sqrt;
    normal.vz = normal.vz / sqrt;
    VectorNormalS(&normal, arg1);
}

s32 BattleVectorLength(s32 *arg0) {
    long out[3];

    gte_ldlvl(arg0);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[1] + out[2]);
}

s32 BattleVectorLength2D(s32 *arg0) {
    long out[3];

    gte_ldlvl(arg0);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[2]);
}

s32 BattleVectorDistance(s32 *arg0, s32 *arg1) {
    long out[3];

    out[0] = arg1[0] - arg0[0];
    out[1] = arg1[1] - arg0[1];
    out[2] = arg1[2] - arg0[2];
    gte_ldlvl(out);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[1] + out[2]);
}

s32 BattleVectorDistance2D(s32 *arg0, s32 *arg1) {
    long out[3];

    out[0] = arg1[0] - arg0[0];
    out[2] = arg1[2] - arg0[2];
    gte_ldlvl(out);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[2]);
}

void BattleSetSlotBit(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    g_BattleSlotBits[arg0 >> 3] |= bit;
}

s32 BattleTestSlotBit(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    return g_BattleSlotBits[arg0 >> 3] & bit;
}

void BattleClearSlotBit(s32 arg0) {
    s32 mask = 1;

    mask <<= arg0 & 7;
    g_BattleSlotBits[arg0 >> 3] &= ~mask;
}
