#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 a;
} Color4; // Size: 0x4

// Same layout as a POLY_FT4 (include/psyq/libgpu.h): tag, colour/code, then four
// vertices of x/y plus u/v, with the clut/tpage pair at the first two vertices.
typedef struct {
    /* 0x00 */ u32 tag;
    /* 0x04 */ u8 r;
    /* 0x05 */ u8 g;
    /* 0x06 */ u8 b;
    /* 0x07 */ u8 code;
    /* 0x08 */ s16 x0;
    /* 0x0A */ s16 y0;
    /* 0x0C */ u8 u0;
    /* 0x0D */ u8 v0;
    /* 0x0E */ u16 clut;
    /* 0x10 */ s16 x1;
    /* 0x12 */ s16 y1;
    /* 0x14 */ u8 u1;
    /* 0x15 */ u8 v1;
    /* 0x16 */ u16 tpage;
    /* 0x18 */ s16 x2;
    /* 0x1A */ s16 y2;
    /* 0x1C */ u8 u2;
    /* 0x1D */ u8 v2;
    /* 0x1E */ u16 pad1E;
    /* 0x20 */ s16 x3;
    /* 0x22 */ s16 y3;
    /* 0x24 */ u8 u3;
    /* 0x25 */ u8 v3;
    /* 0x26 */ u16 pad26;
} SlotMotion; // Size: 0x28

// One entry of the effect action table g_BattleEffectActions: the two per-frame texture
// offset tables, the texel size of the quad and the CLUT/tpage it draws with.
typedef struct {
    /* 0x0 */ u8 *pFrameU;
    /* 0x4 */ u8 *pFrameV;
    /* 0x8 */ u8 uSize;
    /* 0x9 */ u8 vSize;
    /* 0x0A */ u8 frameCount;
    /* 0x0B */ u8 sizeIndex; // 0..2: which of three pre-rotated corner sizes the quad uses
    /* 0x0C */ s16 clut;
    /* 0x0E */ u16 tpage;
    /* 0x10 */ s16 yAccel; // added to the type-0 slot's vy every frame, then to its y
    /* 0x12 */ u16 unk12;
} BattleAction; // Size: 0x14

typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 vy;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 pad6;
} SVector; // Size: 0x8

typedef struct {
    /* 0x00 */ SlotMotion packets[2];
    /* 0x50 */ u8 active;
    /* 0x51 */ s8 frame;
    /* 0x52 */ u8 frameCount;
    /* 0x53 */ u8 type;
    /* 0x54 */ u16 x;
    /* 0x56 */ u16 y;
    /* 0x58 */ u16 z;
    // 0x5A..0x78 is reused per `type`, which is why most of it stays unk: type 0
    // keeps its BattleAction at 0x60, type 1 a partner pointer at 0x6C with the
    // creation tick at 0x70 and the projected screen points at 0x60..0x6A (the
    // four corners at 0x60..0x66, the far end at 0x68/0x6A), type 2 the second end
    // of its beam at 0x64/0x66/0x68 and the key at 0x70. Only fields that mean one
    // thing to every writer are named.
    /* 0x5A */ u16 unk5A;
    /* 0x5C */ u16 unk5C;
    /* 0x5E */ u16 unk5E;
    /* 0x60 */ u32 unk60;
    /* 0x64 */ u16 unk64;
    /* 0x66 */ u16 vy;
    /* 0x68 */ u16 unk68;
    /* 0x6A */ u16 unk6A;
    /* 0x6C */ u32 unk6C;
    /* 0x70 */ u16 unk70;
    /* 0x72 */ s16 drawFlags; // g_BattleDrawFlags at creation, to spot a duplicate in the same pass
    /* 0x74 */ s32 halfWidth; // perpendicular offset of the quad's two sides, D_80091228[effect]
    /* 0x78 */ u32 key; // BattleEffectPackKey of the pair, so the second vertex finds the first
} BattleSlot; // Size: 0x7C

// 0xC-byte primitive template func_8007D6B8 allocates at D_800926C0/D_800926C4:
// a 0x68 tile prim, whose tag word is the OT link plus the prim's length.
typedef struct {
    /* 0x0 */ u8 unk0[3];
    /* 0x3 */ u8 len;
    /* 0x4 */ u32 colorCode; // r0,g0,b0,code: retail stores the literal as one word
    /* 0x8 */ s16 x; // projected screen position, written when the prim is queued
    /* 0xA */ s16 y;
} BattlePrimTemplate; // Size: 0xC

// 0xA-byte entry table func_8007D6B8 allocates at g_BattleSpriteEntries: where
// BattleSpawnSpriteEntries put an entry on the grid and how long it lives.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 z;
    /* 0x6 */ s8 life; // 0 marks a free entry; set to the entry's frame countdown
    /* 0x7 */ s8 vy; // added to y every frame and itself gains 2: gravity
    /* 0x8 */ s16 groundY; // the y the entry falls back to: it dies once y >= groundY
} BattleSpriteEntry; // Size: 0xA

extern BattleSlot g_BattleEffectSlots[];
extern BattleAction g_BattleEffectActions[];
extern Color4 g_BattleEffectColor;
extern u8 D_800926B9;
extern u8 D_800926BA;
extern u32 g_BattleEffectSpawnTick;
extern u32 g_BattleLineQuadQueueCount;
extern u32 g_BattleLineQuadDrawCount;
extern u16 g_BattleEffectClutPalette[];
extern u16 D_800926AA;
extern u16 D_800926AC;
extern u16 D_800926AE;
extern u16 D_80092694;
extern u16 g_BattleEffectClutId;
extern s16 D_80091228[];
extern u16 g_BattleEffectFrameCluts[];

// CLUT x (0x80092698) and y (0x8009269C). BattleEffectUploadClut reads each of them as a
// short, this unit's other callers only want the low byte.
extern u8 g_BattleEffectClutX[];
extern u8 g_BattleEffectClutY[];
extern s32 g_BattleDrawFlags;
extern VECTOR g_BattleViewOrigin;

// 0xC-byte entry table BattleInitSpriteBuffers allocates at g_BattleSpriteSlots:
// one particle's position, its per-frame movement and its remaining frames.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 z;
    /* 0x6 */ s16 life; // 0 marks a free entry; counts down every frame
    /* 0x8 */ s8 vx; // per-frame step added to x
    /* 0x9 */ s8 vz; // per-frame step added to z
    /* 0xA */ u16 vy; // added to y every frame and itself gains 2: gravity
} BattleSpriteSlot; // Size: 0xC

// 0x10-byte primitive template BattleInitSpriteBuffers allocates at g_BattleSpritePrimBuffers/g_BattleSpritePrimBufferCopy:
// a 0x60 tile prim, whose tag word is the OT link plus the prim's length.
typedef struct {
    /* 0x0 */ u8 unk0[3];
    /* 0x3 */ u8 len;
    /* 0x4 */ u32 colorCode; // r0,g0,b0,code: retail stores the literal as one word
    /* 0x8 */ s16 x; // projected screen position, written when the prim is queued
    /* 0xA */ s16 y;
    /* 0xC */ u16 w;
    /* 0xE */ u16 h;
} BattlePrimTemplate16; // Size: 0x10

extern u8 *g_BattleSpriteEntries;
extern u8 *D_800926C0[];
extern u8 *D_800926C4;
extern u8 *g_BattleSpriteSlots;
extern u8 *g_BattleSpritePrimBuffers;
extern u8 *g_BattleSpritePrimBufferCopy;
extern u8 *g_BattleGridCells;
extern u8 g_BattleDrawBufferIndex;

// The TIM archive func_800852C4 decompresses: the effect sprite sheet's 12 frame
// TIMs, then the per-effect texture TIMs the loader below reads.
typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 frameTims[12];
    /* 0x34 */ u32 unk34;
    /* 0x38 */ u32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ u32 unk48[8];
    /* 0x68 */ u32 unk68;
} BattleEffectTimArchive;

// The palette banks the sprite sheet's TIMs carry, shifted by the third TIM's
// rectangle y (the per-effect brightness offset).
extern u8 D_800911D8[];
extern u8 D_800911E8[];
extern u8 D_800911F8[];
extern u8 D_80091208[];
extern u8 D_80091218[];
extern u8 g_BattleEffectFrameU[];
extern u8 D_800947F4[];
extern u16 D_80094800[];
extern u16 D_80092678;
extern u16 D_8009267C;
extern u16 D_80092680;
extern u16 D_80092684;
extern u16 D_80092688;
extern u16 D_8009268C;
extern u16 D_80092690;
extern void BattleInitSpriteBuffers(void);
extern void BattleResetLineQuadQueue(void);

extern int rand(void);

extern void BattleDrawLineQuads(s32);
extern void BattleEffectJitterSVector(SVector *pVec);
extern s32 BattleVectorLength(s32 *arg0);
extern s32 BattleVectorLength2D(s32 *arg0);
extern void BattleQueueLineQuad(s32 *pPos, s32 *pPos2, u8 *pColor);
extern void BattleEffectQueueLineQuads(VECTOR *pA, VECTOR *pB, s32 type);
extern void func_800732AC(void *, void *, s32);
extern void func_8007E020(s32);

void BattleEffectUploadClut(u8 *p0, u8 *p1) {
    s32 r;
    s32 b;
    s32 g;

    r = p0[0] - 0x80;
    g = p0[1] - 0x80;
    b = p0[2] - 0x80;
    if (r < 0) {
        r = 0;
    }
    if (g < 0) {
        g = 0;
    }
    if (b < 0) {
        b = 0;
    }
    g_BattleEffectClutPalette[0] = ((r >> 2) & 0x1F) + ((g << 3) & 0x3E0) + ((b << 8) & 0x7C00) | -0x8000;

    r = p1[0] - 0x80;
    g = p1[1] - 0x80;
    b = p1[2] - 0x80;
    if (r < 0) {
        r = 0;
    }
    if (g < 0) {
        g = 0;
    }
    if (b < 0) {
        b = 0;
    }
    D_800926AA = ((r >> 2) & 0x1F) + ((g << 3) & 0x3E0) + ((b << 8) & 0x7C00) | -0x8000;

    D_800926AC = 0;
    D_800926AE = 0x1111;
    g_BattleEffectClutId = LoadClut2((u_long *)g_BattleEffectClutPalette, *(s16 *)g_BattleEffectClutX, *(s16 *)g_BattleEffectClutY);
}
// The slot walk must be a pointer walk: the indexed form (&g_BattleEffectSlots[i])
// loses the biased base register gcc forms over the packet's byte fields and the
// body comes out two instructions short. stpBit is s16 because retail materialises
// the mask with li/or rather than ori, which only a signed 16-bit value produces.
void BattleEffectInitTextures(BattleEffectTimArchive *pTim) {
    TIM_IMAGE tim;
    RECT rect; // unused: retail's frame has 8 bytes of slack under the TIM image
    BattleAction *pAction;
    BattleSlot *pSlot;
    SlotMotion *pPacket;
    u16 *pPal;
    u16 u;
    u16 v;
    s16 stpBit;
    u8 shift;
    s32 i;

    // The sheet's palette is still the unshifted one until the first TIM fills it.
    shift = D_800911E8[0] == 0;
    for (i = 0; i < 12; i++) {
        OpenTIM(pTim->frameTims[i]);
        ReadTIM(&tim);
        g_BattleEffectFrameU[i] = *(u8 *)&tim.prect->x << 2;
        D_800947F4[i] = *(u16 *)&tim.prect->y;
        D_80094800[i] = GetClut(tim.crect->x, tim.crect->y);
        LoadImage(tim.crect, tim.caddr);
        LoadImage(tim.prect, tim.paddr);
    }

    pAction = &g_BattleEffectActions[0];
    pAction->pFrameU = g_BattleEffectFrameU;
    pAction->pFrameV = D_800947F4;
    pAction->uSize = 0x26;
    pAction->vSize = 0x26;
    pAction->clut = 0;
    pAction->tpage = GetTPage(0, 1, 0x3C0, 0x100);
    pAction->frameCount = 0xC;
    pAction->yAccel = 0;
    pAction->sizeIndex = 0;

    OpenTIM(pTim->unk34);
    ReadTIM(&tim);
    pPal = (u16 *)tim.caddr;
    *pPal = 0;
    stpBit = 0x8000;
    for (i = 1; i < 0x10; i++) {
        pPal[i] |= stpBit;
    }
    if (shift) {
        for (i = 0; i < 0x10; i++) {
            D_800911E8[i] += *(u8 *)&tim.prect->y;
        }
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    pAction = &g_BattleEffectActions[1];
    pAction->pFrameU = D_800911D8;
    pAction->pFrameV = D_800911E8;
    pAction->uSize = 0x1F;
    pAction->vSize = 0x1F;
    pAction->clut = GetClut(tim.crect->x, tim.crect->y);
    pAction->tpage = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    pAction->frameCount = 0x10;
    pAction->yAccel = -2;
    pAction->sizeIndex = 0;

    OpenTIM(pTim->unk40);
    ReadTIM(&tim);
    pPal = (u16 *)tim.caddr;
    *pPal = 0;
    stpBit = 0x8000;
    for (i = 1; i < 0x10; i++) {
        pPal[i] |= stpBit;
    }
    if (shift) {
        for (i = 0; i < 0xC; i++) {
            D_80091208[i] += *(u8 *)&tim.prect->y;
        }
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    pAction = &g_BattleEffectActions[2];
    pAction->pFrameU = D_800911F8;
    pAction->pFrameV = D_80091208;
    pAction->uSize = 0xF;
    pAction->vSize = 0xF;
    pAction->clut = GetClut(tim.crect->x, tim.crect->y);
    pAction->tpage = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    pAction->frameCount = 0xC;
    pAction->yAccel = 0;
    pAction->sizeIndex = 1;

    // The light-beam action is the second one drawn on transparent texels.
    g_BattleEffectActions[3] = g_BattleEffectActions[1];
    g_BattleEffectActions[3].tpage = (g_BattleEffectActions[3].tpage & 0xFF9F) | 0x40;

    OpenTIM(pTim->unk68);
    ReadTIM(&tim);
    if (shift) {
        for (i = 0; i < 0x10; i++) {
            D_80091218[i] += *(u8 *)&tim.prect->y;
        }
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    pAction = &g_BattleEffectActions[4];
    pAction->pFrameU = D_800911F8;
    pAction->pFrameV = D_80091218;
    pAction->uSize = 0xF;
    pAction->vSize = 0xF;
    pAction->clut = GetClut(tim.crect->x, tim.crect->y);
    pAction->tpage = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    pAction->frameCount = 0x10;
    pAction->yAccel = 0;
    pAction->sizeIndex = 2;

    pSlot = g_BattleEffectSlots;
    for (i = 0; i < 0x3C; i++, pSlot++) {
        pPacket = &pSlot->packets[0];
        ((u8 *)&pPacket->tag)[3] = 9;
        *(u32 *)&pPacket->r = 0x2C808080;
        pPacket->code |= 2;
        pSlot->packets[1] = *pPacket;
        pSlot->active = 0;
        pSlot->frame = 0;
    }

    OpenTIM(pTim->unk38);
    ReadTIM(&tim);
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    u = ((u16 *)tim.prect)[0];
    v = ((u16 *)tim.prect)[1];
    D_80092678 = u;
    D_8009267C = v;
    D_80092680 = GetClut(tim.crect->x, tim.crect->y);
    D_80092684 = GetTPage(0, 2, tim.prect->x, tim.prect->y);

    OpenTIM(pTim->unk44);
    ReadTIM(&tim);
    pPal = (u16 *)tim.caddr;
    *pPal = 0;
    stpBit = 0x8000;
    for (i = 1; i < 0x10; i++) {
        pPal[i] |= stpBit;
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    D_80092688 = ((u16 *)tim.prect)[0] << 2 & 0xFF;
    D_8009268C = *(u8 *)&tim.prect->y;
    D_80092690 = GetClut(tim.crect->x, tim.crect->y);
    D_80092694 = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    g_BattleEffectSpawnTick = 0x8000;
    *(u16 *)g_BattleEffectClutX = tim.crect->x;
    *(u16 *)g_BattleEffectClutY = tim.crect->y + 1;

    func_8007D6B8();
    BattleInitSpriteBuffers();
    BattleResetLineQuadQueue();
}




void BattleEffectUpdateSlots(void) {
    BattleSlot *p = g_BattleEffectSlots;
    s32 i;
    u32 n;
    u32 v;

    for (i = 0; i < 0x3C; i++, p++) {
        if (p->active != 0) {
            if (p->frame == p->frameCount) {
                p->active = 0;
            } else {
                p->frame++;
                if (p->type == 0) {
                    p->vy += p->unk64;
                    p->y += p->vy;
                }
            }
        }
    }
    n = g_BattleEffectSpawnTick;
    v = g_BattleLineQuadQueueCount;
    g_BattleLineQuadQueueCount = 0;
    g_BattleEffectSpawnTick = n + 1;
    g_BattleLineQuadDrawCount = v;
}

void BattleEffectClearSlots(void) {
    u8 *p = (u8 *)g_BattleEffectSlots;
    s32 i = 0x3B;

    do {
        p[0x50] = 0;
        i--;
        p += 0x7C;
    } while (i >= 0);
}

/* Best attempt: objdiff 94.47%, 1368 bytes against retail's 1376 (two
   instructions short), same calls and branches throughout. Everything from the
   slot loop on is line-for-line: the slot walk, the V0 = slot - camera build,
   the SVECTOR copy that produces the two lwl/lwr pairs, the three-case corner
   switch (bodies must be listed case 0, 1, 2), gte_ldv01/RTPT, the bounding-box
   fill and the u/v block. What is left is the SIX-BLOCK CORNER PROLOGUE: retail
   stores the scratch vertex halves with constant addresses
   (`lui $at,0x1f80; sh $v0,0x12($at); sh $v0,0x10($at)`) and only reuses a
   register for the x half from block 2 on, while gcc 2.7.2 puts 0x1F800012 into
   a register in block 1 and stores both halves through registers. Tried: pure
   literal casts (94.27%), a s16 width local, an SVECTOR *pVec for the x half
   only (94.47%, kept below), w as s16. NOT tried: the permuter.
   Also open: `addiu v1,a3,%lo(D_1F800010)` / `%lo(D_1F8000B0)` vs our
   `addiu v1,a3,0x10` (a relocation-only difference - the TYPE/RELOC exception),
   `sw $v1,0($a0)` (retail stores the tag through the masked address) vs our
   `sw $v1,0($a2)`, and `addu $v0,$v0,$t7` vs our `addu $v0,$t7,$v0` for
   `&pOt[otz]` (try the embedded-assignment lever).
   Struct facts this body established: BattleAction 0x0/0x4 are the per-frame u
   and v offset tables, 0x8/0x9 the quad's texel size, 0xB the size select,
   and its clut at 0xC is tested with `lh`, so it is s16. */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007BBA0);

/*
void func_8007BBA0(MATRIX *pMatrix, MATRIX *pRotMatrix, u32 *pOt) {
    u8 *pScratch = (u8 *)0x1F800000;
    SVECTOR *pVec = (SVECTOR *)0x1F800010;
    BattleSlot *pSlot;
    BattleAction *pAction;
    SlotMotion *pPacket;
    u32 *pLink;
    u32 link;
    s32 i;
    s16 w;

    // The six quad corners are rotated once, for three sprite widths.
    *(s16 *)0x1F800014 = 0;
    gte_SetRotMatrixRows_gpr(pRotMatrix);

    w = -0x60;
    *(s16 *)0x1F800012 = w;
    pVec->vx = w;
    gte_ldv0((SVECTOR *)0x1F800010);
    gte_mvmva(1, 0, 0, 3, 0);
    gte_stlvnl((void *)0x1F800030);

    w = 0x60;
    *(s16 *)0x1F800012 = w;
    pVec->vx = w;
    gte_ldv0((SVECTOR *)0x1F800010);
    gte_mvmva(1, 0, 0, 3, 0);
    gte_stlvnl((void *)0x1F800040);

    w = -0x48;
    *(s16 *)0x1F800012 = w;
    pVec->vx = w;
    gte_ldv0((SVECTOR *)0x1F800010);
    gte_mvmva(1, 0, 0, 3, 0);
    gte_stlvnl((void *)0x1F800050);

    w = 0x48;
    *(s16 *)0x1F800012 = w;
    pVec->vx = w;
    gte_ldv0((SVECTOR *)0x1F800010);
    gte_mvmva(1, 0, 0, 3, 0);
    gte_stlvnl((void *)0x1F800060);

    w = -0x20;
    *(s16 *)0x1F800012 = w;
    pVec->vx = w;
    gte_ldv0((SVECTOR *)0x1F800010);
    gte_mvmva(1, 0, 0, 3, 0);
    gte_stlvnl((void *)0x1F800070);

    w = 0x20;
    *(s16 *)0x1F800012 = w;
    pVec->vx = w;
    gte_ldv0((SVECTOR *)0x1F800010);
    gte_mvmva(1, 0, 0, 3, 0);
    gte_stlvnl((void *)0x1F800080);

    gte_SetRotTransMatrix_gpr(pMatrix);

    pSlot = g_BattleEffectSlots;
    for (i = 0; i < 0x3C; i++, pSlot++) {
        if (pSlot->active == 0) {
            continue;
        }
        if (pSlot->type != 0) {
            continue;
        }
        pAction = (BattleAction *)pSlot->unk60;
        pPacket = &pSlot->packets[g_BattleDrawBufferIndex];

        *(s16 *)(pScratch + 0x10) = pSlot->x - *(s32 *)(pScratch + 0x00);
        *(s16 *)(pScratch + 0x12) = pSlot->y - *(s32 *)(pScratch + 0x04);
        *(s16 *)(pScratch + 0x14) = pSlot->z - *(s32 *)(pScratch + 0x08);
        *(SVECTOR *)(pScratch + 0x20) = *(SVECTOR *)(pScratch + 0x10);

        // Widen the projected point by the pre-rotated corner of this size.
        switch (pAction->sizeIndex) {
        case 0:
            *(u16 *)(pScratch + 0x10) += *(s32 *)(pScratch + 0x30);
            *(u16 *)(pScratch + 0x12) += *(s32 *)(pScratch + 0x34);
            *(u16 *)(pScratch + 0x14) += *(s32 *)(pScratch + 0x38);
            *(u16 *)(pScratch + 0x20) += *(s32 *)(pScratch + 0x40);
            *(u16 *)(pScratch + 0x22) += *(s32 *)(pScratch + 0x44);
            *(u16 *)(pScratch + 0x24) += *(s32 *)(pScratch + 0x48);
            break;
        case 1:
            *(u16 *)(pScratch + 0x10) += *(s32 *)(pScratch + 0x50);
            *(u16 *)(pScratch + 0x12) += *(s32 *)(pScratch + 0x54);
            *(u16 *)(pScratch + 0x14) += *(s32 *)(pScratch + 0x58);
            *(u16 *)(pScratch + 0x20) += *(s32 *)(pScratch + 0x60);
            *(u16 *)(pScratch + 0x22) += *(s32 *)(pScratch + 0x64);
            *(u16 *)(pScratch + 0x24) += *(s32 *)(pScratch + 0x68);
            break;
        case 2:
            *(u16 *)(pScratch + 0x10) += *(s32 *)(pScratch + 0x70);
            *(u16 *)(pScratch + 0x12) += *(s32 *)(pScratch + 0x74);
            *(u16 *)(pScratch + 0x14) += *(s32 *)(pScratch + 0x78);
            *(u16 *)(pScratch + 0x20) += *(s32 *)(pScratch + 0x80);
            *(u16 *)(pScratch + 0x22) += *(s32 *)(pScratch + 0x84);
            *(u16 *)(pScratch + 0x24) += *(s32 *)(pScratch + 0x88);
            break;
        }

        gte_ldv01((SVECTOR *)(pScratch + 0x10), (SVECTOR *)(pScratch + 0x20));
        gte_rtpt();
        gte_stsxy0(&pPacket->x0);
        gte_stsxy1(&pPacket->x3);
        gte_stsz2(pScratch + 0xB0);

        // The quad is the screen-aligned box of the two projected corners.
        pPacket->x1 = pPacket->x3;
        pPacket->y1 = pPacket->y0;
        pPacket->x2 = pPacket->x0;
        pPacket->y2 = pPacket->y3;

        pPacket->u0 = pAction->pFrameU[pSlot->frame - 1];
        pPacket->v0 = pAction->pFrameV[pSlot->frame - 1];
        pPacket->u2 = pPacket->u0;
        pPacket->v1 = pPacket->v0;
        pPacket->u1 = pPacket->u0 + pAction->uSize;
        pPacket->v2 = pPacket->v0 + pAction->vSize;
        pPacket->u3 = pPacket->u0 + pAction->uSize;
        pPacket->v3 = pPacket->v0 + pAction->vSize;

        if (pAction->clut == 0) {
            pPacket->clut = g_BattleEffectFrameCluts[pSlot->frame];
        }

        if (*(s32 *)(pScratch + 0xB0) >= 0x41) {
            *(s32 *)(pScratch + 0xB0) -= 0x40;
        }
        pLink = &pOt[*(s32 *)(pScratch + 0xB0) >> 4];
        link = *pLink;
        *pLink = (u32)pPacket & 0xFFFFFF;
        pPacket->tag = link | 0x09000000;
    }
}
*/

void BattleEffectSetColor(Color4 *pColor) {
    g_BattleEffectColor = *pColor;
}

void BattleEffectAddSlot(SVector *pVec, s32 action) {
    BattleSlot *pSlot = g_BattleEffectSlots;
    BattleAction *pAction;
    SlotMotion *pMotion;
    s32 i;

    for (i = 0; i < 0x3C; i++, pSlot++) {
        if (pSlot->active == 0) {
            break;
        }
    }
    if (i == 0x3C) {
        return;
    }
    pSlot->active = 1;
    pAction = &g_BattleEffectActions[action];
    pSlot->type = 0;
    pSlot->frame = 0;
    pSlot->unk60 = (u32)pAction;
    pSlot->frameCount = pAction->frameCount;
    pSlot->unk64 = pAction->yAccel;
    pSlot->vy = 0;
    pSlot->x = pVec->vx;
    pSlot->y = pVec->vy;
    pSlot->z = pVec->vz;
    pMotion = &pSlot->packets[0];
    pMotion->tpage = pAction->tpage;
    if (action == 2) {
        pMotion->code &= 0xFE;
        pMotion->r = g_BattleEffectColor.r;
        pMotion->g = D_800926B9;
        pMotion->b = D_800926BA;
    } else {
        pMotion->code |= 1;
    }
    pMotion->clut = pAction->clut;
    pSlot->packets[1] = *pMotion;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C280);

/* Best attempt: objdiff 85.17%, 548 bytes = retail's exact size and instruction count.
Round 292's two permuter edits (u = u + arg0*4 instead of +=, and hoisting the
CLUT word into the local v before the store) re-measure NEUTRAL in round 370 -
85.17 either way - so the residual is unchanged: retail's LOAD-DELAY SLOTS are
filled with the following statement's load (lw pVec->vx is followed by
lui/lw g_BattleDrawFlags, lw pVec->vz by lw g_BattleEffectSpawnTick, both hoisted
above the store group) where our allocation leaves each load next to its own
store, and the motion-copy address setup comes out one register over. Store order
matches retail exactly (0x54, 0x56, 0x72, 0x78, 0x6C, 0x70, 0x58, 0x74) and the
first scan loop, the slot search and the trailing key search are byte-identical.
Same 85.17 under -O3, -G0 and gcc-2.7.2-cdk. The drawFlags scan pointer anchors
at slots+0x72 in both, so that part is only a relocation addend difference. */
/*
void func_8007C880(s32 arg0, VECTOR *pVec, u32 arg2, s32 arg3) {
    BattleSlot *pSlot;
    BattleSlot *pEntry;
    SlotMotion *pMotion;
    u16 id;
    s32 u;
    s32 v;
    s32 i;

    pEntry = g_BattleEffectSlots;
    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->active != 0) {
            if (pEntry->key == arg2) {
                if (pEntry->drawFlags == g_BattleDrawFlags) {
                    return;
                }
            }
        }
    }

    pSlot = g_BattleEffectSlots;
    for (i = 0; i < 0x3C; i++, pSlot++) {
        if (pSlot->active == 0) {
            break;
        }
    }
    if (i == 0x3C) {
        return;
    }

    u = g_BattleEffectClutX[0] * 4 + 8;
    pMotion = &pSlot->packets[0];
    u = u + arg0 * 4;
    pMotion->u3 = u;
    pMotion->u2 = u;
    pMotion->u1 = u;
    pMotion->u0 = u;
    v = g_BattleEffectClutY[0];
    pMotion->v3 = v;
    pMotion->v2 = v;
    pMotion->v1 = v;
    pMotion->v0 = v;
    pMotion->tpage = D_80092694;
    pMotion->code &= 0xFE;
    pMotion->clut = g_BattleEffectClutId;
    pSlot->packets[1] = *pMotion;

    pSlot->frameCount = 7;
    pSlot->type = 1;
    pSlot->active = 1;
    pSlot->frame = 0;
    pSlot->x = pVec->vx;
    pSlot->y = pVec->vy;
    pSlot->drawFlags = g_BattleDrawFlags;
    pSlot->key = arg2;
    pSlot->unk6C = 0;
    pSlot->unk70 = g_BattleEffectSpawnTick;
    pSlot->z = pVec->vz;
    pSlot->halfWidth = D_80091228[arg3];

    id = g_BattleEffectSpawnTick - 1;
    pEntry = g_BattleEffectSlots;
    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->active != 0 && pEntry->key == arg2 && pEntry != pSlot && pEntry->type == 1 && pEntry->unk70 == id) {
            pSlot->unk6C = (u32)pEntry;
        }
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C880);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CAA4);

s32 BattleEffectPackKey(s32 a0, s32 a1, s32 a2, s32 a3) {
    return (a3 & 0x7F) | ((a0 << 7) & 0x80) | (a1 << 24) | ((a2 << 8) & 0xFFFF00);
}

/* Best attempt: objdiff 85.67%. Everything matches except the scheduling of the
   0x64/0x66/0x68 stores against the u/v byte-pair writes: retail fills their
   load-delay slots with the t1 = slot + 0x28 / t2 = slot + 0x20 / a2 = slot
   address setup of the SlotMotion copy and hoists the pVec and g_BattleEffectSpawnTick loads,
   gcc leaves nop there and keeps each load next to its store. Tried: the u/v
   pair in ascending order, unk68 stored with vy instead of after unk6A, the
   u/v block before the unk64/68 stores, and slot->unk0 in place of a pMotion
   pointer (all 82-85%); the first two halves of the function and the trailing
   search loop are byte-identical.
   RE-MEASURED round 475: 85.67 does not reproduce; the plain body is 82.95% /
   144 insns / 576 bytes. THE FIND: the type-2 (two-vertex beam) slot's two
   dupe-check fields are the wrong WIDTH in BattleSlot and that is the whole
   instruction deficit. Retail reads the field at 0x6C with lh and stores it with
   sh (a 16-bit g_BattleDrawFlags copy) and the field at 0x70 with lw and stores it
   with sw (the BattleEffectPackKey); BattleSlot has them the other way round
   (u32 unk6C, u16 unk70), because 0x6C and 0x70 are where the TYPE 1 slot keeps
   its unk6C and unk70. Writing the accesses 16-bit/32-bit (the casts below, a
   probe form) gives 84.53% / 143 insns / 572 bytes. The real fix is a union over
   0x6C..0x73 holding a beam view (0x6C u16 drawFlags, 0x6E pad, 0x70 u32 key)
   beside the line view the struct has now (0x6C u32 unk6C, 0x70 u16 unk70,
   0x72 s16 drawFlags); func_8007C880's asm really does touch 0x72/0x78, so the
   two views cannot both be plain fields. Do the union first, then re-measure.
   RESIDUAL after the widths: the block between the pPos stores and the
   packets[1] struct copy. Retail fills the load-delay slots of the
   g_BattleEffectClutX, g_BattleEffectClutY, D_80092694 and
   g_BattleEffectSpawnTick loads with the next load or with the u/v store pairs
   and spreads the move a2,t0, addiu t1,t0,40 and addiu t2,t0,32 address setups
   through the block; ours emits a nop after each of those loads and pairs each
   load with its store, ending 2 instructions / 8 bytes longer. Both scan loops
   and the trailing search loop are byte-identical apart from the first loop's
   anchor: retail names D_80092B44 (= g_BattleEffectSlots + 0x6C), we name
   g_BattleEffectSlots, same linked bytes. */
// PARK 86.91% at retail's EXACT size (564 bytes, 141 instructions; the old note's
// 85.67% was the 16-bit-cast form which read 572 bytes / 143). Two fixes this round:
// the duplicate check reads 0x6C SIGNED ('lh' in retail, 'lhu' before) so the cast is
// (s16 *)&pEntry->unk6C, and the u setup belongs BETWEEN the unk64 and the vy store -
// 'u = g_BattleEffectClutX[0] * 4 + 8; pSlot->vy = pVec->vy; u += arg0 * 4;' - which
// removes the two extra 'nop' load-delay slots (the +8 bytes). The two scan loops,
// both loop tails, the 0x28-byte packets[0]->packets[1] block copy and the whole
// creation head are now instruction-identical; the first loop's anchor is written
// g_BattleEffectSlots+0x6C where retail names D_80092B44 (same linked bytes).
// REMAINING, entirely the tail's load/store interleaving: retail front-loads the
// value setup - addiu t1,t0,40 right after 'lw v0,0(a2)', then lw v1,4(a2),
// lw a1,8(a2) (pVec->vz), lw a0,g_BattleEffectSpawnTick and lhu D_80092694 all
// ABOVE the u/v byte-pair groups, plus 'move a2,t0' and 'sw a3,112' before them -
// while our body pairs each load with the store that uses it. Measured dead this
// round: hoisting 'pSlot->unk6A = g_BattleEffectSpawnTick; pSlot->unk68 = pVec->vz;'
// above the u/v groups (142 insns / 568 bytes / 86.42, and the third loop's
// registers shift). That makes it a scheduler question, not a statement-order one.
// Also measured dead: the peephole form 'link |= ...' style reordering is N/A here;
// preset screens were 2.6.0 84.5x / cdk does not compile this unit.
/*
void func_8007CD44(s32 arg0, VECTOR *pPos, VECTOR *pVec, u32 arg3) {
    BattleSlot *pSlot = g_BattleEffectSlots;
    BattleSlot *pEntry = g_BattleEffectSlots;
    SlotMotion *pMotion;
    u16 id;
    s32 u;
    s32 v;
    s32 i;

    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->active != 0) {
            if (*(u32 *)&pEntry->unk70 == arg3) {
                if (*(s16 *)&pEntry->unk6C == g_BattleDrawFlags) {
                    return;
                }
            }
        }
    }

    pSlot = g_BattleEffectSlots;
    for (i = 0; i < 0x3C; i++, pSlot++) {
        if (pSlot->active == 0) {
            break;
        }
    }
    if (i == 0x3C) {
        return;
    }

    pSlot->frameCount = 7;
    pSlot->type = 2;
    pSlot->active = 1;
    pSlot->frame = 0;
    pSlot->x = pPos->vx;
    pSlot->y = pPos->vy;
    pSlot->z = pPos->vz;
    *(u16 *)&pSlot->unk6C = g_BattleDrawFlags;
    pSlot->unk64 = pVec->vx;
    u = g_BattleEffectClutX[0] * 4 + 8;
    pSlot->vy = pVec->vy;
    u += arg0 * 4;

    pMotion = &pSlot->packets[0];
    pMotion->u3 = u;
    pMotion->u2 = u;
    pMotion->u1 = u;
    pMotion->u0 = u;
    v = g_BattleEffectClutY[0];
    pMotion->v3 = v;
    pMotion->v2 = v;
    pMotion->v1 = v;
    pMotion->v0 = v;

    *(u32 *)&pSlot->unk70 = arg3;
    pSlot->unk60 = 0;
    pSlot->unk6A = g_BattleEffectSpawnTick;
    pSlot->unk68 = pVec->vz;
    pMotion->tpage = D_80092694;
    pMotion->code &= 0xFE;
    pMotion->clut = g_BattleEffectClutId;
    pSlot->packets[1] = *pMotion;

    id = g_BattleEffectSpawnTick - 1;
    pEntry = g_BattleEffectSlots;
    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->active != 0 && *(u32 *)&pEntry->unk70 == arg3 && pEntry != pSlot && pEntry->type == 2 && pEntry->unk6A == id) {
            pSlot->unk60 = (u32)pEntry;
        }
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CD44);


void func_8007D918(u32 *pOt);

void BattleEffectQueueMatrixPrims(MATRIX *pMat, s32 arg1) {
    MATRIX transposed;

    *(VECTOR *)PSX_SCRATCH = g_BattleViewOrigin;
    TransposeMatrix(pMat, &transposed);
    func_8007BBA0(pMat, &transposed, arg1);
    func_8007C280(pMat, &transposed, arg1);
    func_8007CAA4(pMat, NULL, arg1);
    gte_SetRotMatrix_gpr(pMat);
    func_8007D918(arg1);
    func_8007E020(arg1);
    BattleDrawLineQuads(arg1);
}

void BattleEffectQueuePrims(s32 arg0) {
    *(VECTOR *)PSX_SCRATCH = g_BattleViewOrigin;
    BattleDrawLineQuads(arg0);
}

/*
Best attempt: objdiff 76.82%. Retail computes (vx - 0x18) INSIDE the modulo's
divide sequence (mult; lhu vx; addiu -0x18; mfhi; ...; addu), i.e. the load and
the subtract are emitted before mfhi; every shape tried puts the addiu after the
whole `rand() % 0x30` expansion. Tried: `vx - 0x18 + rand() % 0x30` (73.11%),
`vx + rand() % 0x30 - 0x18` (76.82%), `(s16)(vx - 0x18) + rand() % 0x30` (76.82%,
the cast is folded away), a `u16 v = pVec->vx` temp (65.42%), and two statements
per field (74.65%, the load then hoists above the call). Same under gcc-2.6.0 and
gcc-2.7.2-cdk. Not run through the permuter yet.
void BattleEffectJitterSVector(SVector *pVec) {
    pVec->vx = (s16)(pVec->vx - 0x18) + rand() % 0x30;
    pVec->vy = (s16)(pVec->vy - 0x18) + rand() % 0x30;
    pVec->vz = (s16)(pVec->vz - 0x18) + rand() % 0x30;
}
*/

void BattleEffectJitterSVector(SVector *pVec) {
    u8 offset = 0x18;

    pVec->vx = (pVec->vx - offset) + rand() % 0x30;
    pVec->vy = (pVec->vy - offset) + rand() % 0x30;
    pVec->vz = (pVec->vz - offset) + rand() % 0x30;
}

// s32, not void: retail keeps a nop in the switch guard's delay slot, which gcc
// 2.7.2 only emits for a non-void return type.
s32 BattleEffectAddSlotFromVector(VECTOR *pVec, s32 action) {
    SVector vec;

    vec.vx = pVec->vx;
    vec.vy = pVec->vy;
    vec.vz = pVec->vz;
    switch (action) {
    case 8:
        BattleEffectJitterSVector(&vec);
        /* fallthrough */
    case 0:
        BattleEffectAddSlot(&vec, 0);
        break;
    case 9:
        BattleEffectJitterSVector(&vec);
        /* fallthrough */
    case 1:
        BattleEffectAddSlot(&vec, 1);
        break;
    case 10:
        BattleEffectJitterSVector(&vec);
        /* fallthrough */
    case 2:
        BattleEffectAddSlot(&vec, 2);
        break;
    case 11:
        BattleEffectJitterSVector(&vec);
        /* fallthrough */
    case 3:
        BattleEffectAddSlot(&vec, 3);
        break;
    case 12:
        BattleEffectJitterSVector(&vec);
        /* fallthrough */
    case 4:
        BattleEffectAddSlot(&vec, 4);
        break;
    }
}

s32 BattleEffectIsTwoVertexType(s32 arg0) {
    if (arg0 < 0x10) {
        return 0;
    }
    return arg0 < 0x20;
}

void BattleEffectJitterVector(VECTOR *pSrc, VECTOR *pDst) {
    pDst->vx = pSrc->vx + rand() % 64 - 32;
    pDst->vy = pSrc->vy + rand() % 64 - 32;
    pDst->vz = pSrc->vz + rand() % 64 - 32;
}

void BattleEffectQueueLineQuads(VECTOR *pA, VECTOR *pB, s32 type) {
    VECTOR pos;
    VECTOR step;
    VECTOR prev;
    VECTOR jittered;
    u8 color[3];
    s32 i;
    s32 j;
    s32 u;

    step = *pB;
    step.vx -= pA->vx;
    step.vy -= pA->vy;
    step.vz -= pA->vz;
    BattleVectorLength((s32 *)&step);
    step.vx /= 7;
    step.vy /= 7;
    step.vz /= 7;
    for (i = 0; i < 3; i++) {
        pos = *pA;
        prev = *pA;
        for (j = 0; j < 7; j++) {
            pos.vx += step.vx;
            pos.vy += step.vy;
            pos.vz += step.vz;
            BattleEffectJitterVector(&pos, &jittered);
            switch (type) {
            case 0:
                color[0] = rand() & 0x3F;
                color[1] = rand() % 191 + 0x40;
                color[2] = 0xFF;
                break;
            case 1:
                u = rand() % 256 + 0x40;
                if (u < 0x100) {
                    color[1] = u;
                } else {
                    color[1] = 0xFF;
                }
                color[0] = color[1] = color[1];
                color[2] = u / 3;
                break;
            case 2:
                if (j & 1) {
                    color[2] = 0xFF;
                    color[1] = 0xFF;
                    color[0] = 0xFF;
                } else {
                    color[0] = 0xFF;
                    color[2] = 0;
                    color[1] = 0;
                }
                break;
            }
            if (j != 6) {
                BattleQueueLineQuad((s32 *)&prev, (s32 *)&jittered, color);
            } else {
                BattleQueueLineQuad((s32 *)&prev, (s32 *)pB, color);
            }
            prev = jittered;
        }
    }
}
void BattleEffectQueueLineByType(s32 arg0, s32 arg1, s32 arg2) {
    switch (arg2) {
    case 0x11:
        BattleEffectQueueLineQuads(arg0, arg1, 0);
        break;
    case 0x12:
        BattleEffectQueueLineQuads(arg0, arg1, 1);
        break;
    case 0x13:
        BattleEffectQueueLineQuads(arg0, arg1, 2);
        break;
    }
}

/* PARK 84.85% at 224 bytes, 4 instructions SHORT of retail's 240. The stored
   body did not even COMPILE until round 370: D_800926C0 is declared here as an
   ARRAY extern (other units index it by g_BattleDrawBufferIndex), so the plain
   assignment D_800926C0 = HeapAlloc(...) is a type error - write
   D_800926C0[0] = HeapAlloc(...) and read it back as D_800926C0[0] and the body
   builds and measures 84.85, which is where the round-3xx note's number came
   from.
   Two residuals, both read off the diff:
   (1) the six entry stores need a SECOND induction register: retail keeps
   v1 = pEntry + 4 and stores z at 0(v1), y at -2(v1), life at 2(v1) with x at
   0(a1); ours anchors all six on the base as 4(a0)/2(a0)/0(a0)/6(a0).
   (2) retail allocates all FOUR D_800926C0/D_800926C4 pointer loads to $v0, so
   three of them get a load-delay nop (nothing is eligible to fill the slot);
   ours puts the third load in $v1, reorg hoists it into the previous slot and we
   come out two instructions short there.
   Tried with no effect: a non-void (s32) return type (no change at all, so the
   stolen-slot lever is not the mechanism here), the pointer form pEntry++ with
   pEntry->z (creates the +4 biv but shifts every other register: 83.82). NOT
   tried: the permuter. */
/* PARK 95.02%, 232 bytes against retail's 240. The empty do-while block is a
   permuter find this round (PERMUTER_BETTER base=786 best=316): it splits the
   basic block so the four D_800926C0/D_800926C4 pointer loads are no longer
   merged, which took the body 84.85 -> 95.02. Residual: retail keeps TWO
   induction registers over the entry table (a1 = record base for the two x
   stores, v1 = base+4 anchored for z/y/life) while this compile anchors all six
   stores on v1 and uses a1 for the loop counter, and it is 2 instructions short
   because retail's three load-delay slots hold nops (all four pointer loads go
   to $v0) where ours fills two of them. Tried in earlier rounds: declaration
   order, pEntry++ pointer form (83.82), a non-void return type. */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D6B8);

/*
void func_8007D6B8(void) {
    BattleSpriteEntry *pEntry;
    s32 i;

    g_BattleSpriteEntries = HeapAlloc(0x9F6, 0);
    D_800926C0[0] = HeapAlloc(0xBF4, 0);
    D_800926C4 = HeapAlloc(0xBF4, 0);
    pEntry = (BattleSpriteEntry *)g_BattleSpriteEntries;
    for (i = 0; i < 0xFF; i++) {
        ((BattlePrimTemplate *)D_800926C0[0])[i].len = 2;
        ((BattlePrimTemplate *)D_800926C0[0])[i].colorCode = 0x6880B0F0;
        // The empty block is load-bearing: it splits the basic block so the four
        // template-pointer loads are not merged (retail loads each one separately).
        do {
        } while (0);
        ((BattlePrimTemplate *)D_800926C4)[i].len = 2;
        ((BattlePrimTemplate *)D_800926C4)[i].colorCode = 0x6880B0F0;
        pEntry[i].z = 0;
        pEntry[i].y = 0;
        pEntry[i].x = 0;
        pEntry[i].life = 0;
        pEntry[i].z = 0;
        pEntry[i].x = 0;
    }
}
*/


/*
void func_8007D6B8(void) {
    BattlePrimTemplate *pTemplate;
    BattleSpriteEntry *pEntry;
    s32 i;

    g_BattleSpriteEntries = HeapAlloc(0x9F6, 0);
    D_800926C0 = HeapAlloc(0xBF4, 0);
    D_800926C4 = HeapAlloc(0xBF4, 0);
    pEntry = (BattleSpriteEntry *)g_BattleSpriteEntries;
    for (i = 0; i < 0xFF; i++) {
        ((BattlePrimTemplate *)D_800926C0)[i].len = 2;
        ((BattlePrimTemplate *)D_800926C0)[i].colorCode = 0x6880B0F0;
        ((BattlePrimTemplate *)D_800926C4)[i].len = 2;
        ((BattlePrimTemplate *)D_800926C4)[i].colorCode = 0x6880B0F0;
        pEntry[i].z = 0;
        pEntry[i].y = 0;
        pEntry[i].x = 0;
        pEntry[i].life = 0;
        pEntry[i].z = 0;
        pEntry[i].x = 0;
    }
}
*/

void BattleSpawnSpriteEntries(VECTOR *pSrc, s32 count) {
    BattleSpriteEntry *pEntry = (BattleSpriteEntry *)g_BattleSpriteEntries;
    u16 vx;
    u16 vz;
    s32 a;
    s32 b;
    s32 row;
    s32 col;
    u16 v;
    s32 index;
    s32 i;

    for (i = 0; i < 0xFB; i++, pEntry++) {
        if (count == 0) {
            break;
        }
        if (pEntry->life == 0) {
            vx = pSrc->vx - 32 + rand() % 64;
            pEntry->x = vx;
            a = vx << 16;
            vz = pSrc->vz - 32 + rand() % 64;
            pEntry->z = vz;
            b = vz << 16;
            col = a >> 24;
            row = (b >> 24) << 7;
            index = row + col;
            v = *(u16 *)(g_BattleGridCells + index * 4);
            pEntry->groundY = v;
            pEntry->y = v;
            pEntry->vy = -(rand() % 10 + 10);
            pEntry->life = 0x14;
            count--;
        }
    }
}

// PARK 92.53% at retail's exact size (528 bytes, 131 instructions). NOTE FIX: the
// previous park was measured on a body with stride 0xA per iteration where retail's
// loop advances the entry pointer 0x14 (= two 0xA entries, counter steps 2), so the
// old note's body visited half the array it should; the parked body below is
// semantically right and scores the same 92.53%. The prologue, the giv anchor
// (entry+7), the multiply chain, the three-vertex RTPT block and the OT link are
// INSTRUCTION-IDENTICAL to retail; the whole residual is one permutation of five
// caller-saved registers holding the loop longs:
//   retail  regular entry=$t1 anchor=$a1 prim=$a2 n=$a3 i=$t0
//   ours    entry=$a0 anchor=$a2 prim=$a3 n=$t0 i=$t1
// i.e. ours is retail shifted one register towards caller-saved, and the cause is
// visible in the prologue: retail emits 'addu $t1,$a0,$v0' (the loaded
// g_BattleSpriteEntries pointer dies and the entry base takes a NEW register) while
// every spelling we have tried emits 'addu $a0,$a0,$v0' (the base coalesces into
// the pointer's $a0, so the four remaining longs each take one register lower).
// Tried this round with no effect on that coalesce or the score: pEntry += 2 to fix
// the stride (92.53), '&((BattleSpriteEntry *)g_BattleSpriteEntries)[buffer * 2]'
// (92.49), an explicit 'u8 *pEntries = g_BattleSpriteEntries;' local (92.49 - gcc
// optimises the local away and still coalesces). Previously tried (round 312):
// eight declaration permutations, 'i = buffer' hoisted before the loop init
// (92.78 but 524 bytes, one instruction short), s32/u32 camera reads, u32
// state/counter; preset screen 2.6.0 93.42, 2.7.2-cdk 82.89, -O3 92.52, -G0 92.52.
// The permuter cannot run on this body (the inline-asm GTE macros fail its parser),
// so this one needs either a source shape that stops the coalesce or an allocator
// build with different REG_ALLOC_ORDER. Facts: D_800926C0 is an ARRAY of
// prim-template pointers indexed by g_BattleDrawBufferIndex (D_800926C4 is element
// 1); BattleSpriteEntry is 0xA bytes, groundY is s16 read with lh and the gravity
// tail compares a s16 copy of y; the function needs 'extern void
// func_8007D918(u32 *pOt);' in scope for BattleEffectQueueMatrixPrims, which calls
// it.
// PARK 98.71% at retail's exact size (528 bytes, 131 instructions). The prologue,
// the multiply chain, the GTE load/rtpt block, both loop shapes and the gravity tail
// are now instruction-identical; the old 92.53% note was stale on two points.
// Two fixes this round, both real: 'n = 0' belongs AFTER gte_rtpt() (retail's
// 'move a3,zero' is the instruction after RTPT, not before it) - that alone removed
// the whole five-register shift the old note blamed on the a0 coalesce (92.53 ->
// 98.48) - and the OT bucket address wants the offset-first cast sum
// '(u32 *)((sz >> 4) * 4 + (u32)pOt)' for retail's index-first 'addu v0,v0,t6'
// (98.48 -> 98.71); the three gte_stsxy0/1/2 addresses written as three pointer
// locals reproduce retail's three 'addiu a0/a1/v0' before the three swc2.
// REMAINING (register naming only, 24 differing lines): retail's 'or a0,a0,t7;
// sw a0,0(v1)' reuses the loaded link register as the OR destination where ours
// allocates the dead v0 ('or v0,a0,t7'), at all three sites; and in the third block
// the scheduler puts 'addiu a2,a2,36' (pTemplate += 3) where retail has
// 'and v1,v1,t3'. Measured dead: 'link |= 0x02000000;' (the in-place form grows
// the frame, 134 insns / 540 bytes / 92.45%), routing the second store through a
// reassigned pLink (byte-identical to the cast form, 98.71), and preset screens
// gcc-2.6.0 98.14 / gcc-2.7.2-cdk fails to compile. The permuter cannot parse this
// body (inline-asm GTE macros).
/*
void func_8007D918(u32 *pOt) {
    BattlePrimTemplate *pTemplate;
    BattleSpriteEntry *pEntry;
    u8 *pScratch = (u8 *)0x1F800000;
    u32 *pLink;
    s16 *pXy0;
    s16 *pXy1;
    s16 *pXy2;
    s32 camX;
    s32 camY;
    s32 camZ;
    u32 link;
    s32 buffer;
    s16 y;
    s32 i;
    s32 n;

    camX = *(s32 *)(pScratch + 0x00);
    camY = *(s32 *)(pScratch + 0x04);
    camZ = *(s32 *)(pScratch + 0x08);
    buffer = g_BattleDrawBufferIndex;
    pEntry = (BattleSpriteEntry *)(g_BattleSpriteEntries + buffer * sizeof(BattleSpriteEntry));
    pTemplate = (BattlePrimTemplate *)D_800926C0[buffer];
    n = 0;
    for (i = buffer; i < 0xFC; i += 2, pEntry += 2) {
        if (pEntry->life == 0) {
            continue;
        }
        *(s16 *)(pScratch + 0x10) = pEntry->x - camX;
        *(s16 *)(pScratch + 0x12) = pEntry->y - camY;
        *(s16 *)(pScratch + 0x14) = pEntry->z - camZ;
        if (n == 0) {
            gte_ldv0((SVECTOR *)(pScratch + 0x10));
            n = 1;
        } else if (n == 1) {
            gte_ldv1((SVECTOR *)(pScratch + 0x10));
            n = 2;
        } else {
            gte_ldv2((SVECTOR *)(pScratch + 0x10));
            gte_rtpt();
            n = 0;
            pXy0 = &pTemplate[0].x;
            pXy1 = &pTemplate[1].x;
            pXy2 = &pTemplate[2].x;
            gte_stsxy0(pXy0);
            gte_stsxy1(pXy1);
            gte_stsxy2(pXy2);
            gte_stsz1(pScratch + 0xB0);
            pLink = (u32 *)((*(s32 *)(pScratch + 0xB0) >> 4) * 4 + (u32)pOt);
            link = *pLink;
            *pLink = (u32)(pTemplate + 0) & 0xFFFFFF;
            pLink = (u32 *)((u32)(pTemplate + 0) & 0xFFFFFF);
            *pLink = link | 0x02000000;
            gte_stsz2(pScratch + 0xB0);
            pLink = (u32 *)((*(s32 *)(pScratch + 0xB0) >> 4) * 4 + (u32)pOt);
            link = *pLink;
            *pLink = (u32)(pTemplate + 1) & 0xFFFFFF;
            pLink = (u32 *)((u32)(pTemplate + 1) & 0xFFFFFF);
            *pLink = link | 0x02000000;
            gte_stsz(pScratch + 0xB0);
            pLink = (u32 *)((*(s32 *)(pScratch + 0xB0) >> 4) * 4 + (u32)pOt);
            link = *pLink;
            *pLink = (u32)(pTemplate + 2) & 0xFFFFFF;
            pLink = (u32 *)((u32)(pTemplate + 2) & 0xFFFFFF);
            *pLink = link | 0x02000000;
            pTemplate += 3;
        }
        y = pEntry->y + pEntry->vy;
        pEntry->y = y;
        if (y >= pEntry->groundY) {
            pEntry->life = 0;
        }
        pEntry->vy += 2;
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D918);




void BattleInitSpriteBuffers(void) {
    u8 *pTemplateBuf;
    BattleSpriteSlot *pSlot;
    BattlePrimTemplate16 *pTemplate;
    s32 i;

    g_BattleSpriteSlots = HeapAlloc(0x1950, 0);
    pTemplateBuf = HeapAlloc(0x21C0, 0);
    g_BattleSpritePrimBuffers = pTemplateBuf;
    g_BattleSpritePrimBufferCopy = HeapAlloc(0x21C0, 0);
    pSlot = (BattleSpriteSlot *)g_BattleSpriteSlots;
    pTemplate = (BattlePrimTemplate16 *)pTemplateBuf;
    for (i = 0; i < 0x21C; i++, pSlot++, pTemplate++) {
        pTemplate->len = 3;
        pTemplate->colorCode = 0x60FFD0A0;
        pTemplate->w = rand() % 3 + 2;
        pTemplate->h = rand() % 3 + 2;
        pSlot->z = 0;
        pSlot->y = 0;
        pSlot->x = 0;
        pSlot->life = 0;
        pSlot->vz = 0;
        pSlot->vy = 0;
        pSlot->vx = 0;
    }
    func_800732AC(g_BattleSpritePrimBufferCopy, g_BattleSpritePrimBuffers, 0x21C0);
}


/* PARK 80.47% at 932 bytes / 233 insns against retail's 940 / 235: TWO
   INSTRUCTIONS SHORT, so it can never link. Kept because it is the closest body by
   a wide margin (the pointer-walk shape it replaces was 78.83% / 940 / 235).
   THE FIND THIS ROUND: indexing the slot array instead of walking a pointer -
   `pSlot = &((BattleSpriteSlot *)g_BattleSpriteSlots)[i];` INSIDE the loop - makes
   gcc keep the `dy / 2` divisor in the loop. The loop now emits retail's own
   `srl v1,s3,0x1f / addu v1,s3,v1 / sra v1,v1,0x1` immediately before the `div`,
   the sp+0x40 spill of the hoisted divisor is gone, and the frame is retail's
   0x68 with an identical saved set (0x40..0x64) and epilogue. The preheader
   `sra v0,s4,0x1f / subu` pair also moved back into the loop, matching retail's
   correction in the rand() delay slot. The vec/norm build, the length/6 budget,
   the count clamp, the three-arm spray and the height/gravity tail all line up.
   Residuals, read off the 2-instruction deficit plus these rows:
   (a) TWO INDUCTION REGISTERS: retail keeps s7 = base and s1 = base + 2 and
   increments both (`addiu s1,s1,12` plus `addiu s7,s7,12`); this body counts a
   byte offset in s7 (`move s7,zero` before the loop) and forms s1 = base + s7
   once, so stores come out at 0/2/4/6/8/9/10(s1) where retail has -2/0/2/4/6/7/8
   (s1) with x through s7. That is 1 of the 2 missing insns, and it is why retail
   anchored at +2: y..vy then fit 0..8 and only x needs the base register.
   (b) THE PREHEADER APPLIES THE `len2 >> 31` CORRECTION TWICE: it stores the
   corrected `spread` at sp+0x38 and then subtracts the correction again in the
   loop, where retail stores it uncorrected and corrects once. Harmless (len2 is
   clamped >= 0 by `if (len2 < 0x10) len2 = 0;`, so both corrections are zero),
   but it is 2 of our insns and not the source's shape.
   (c) `li v0,0xfffc` / `li v0,0xfff0` where retail has `li v0,-4` / `li v0,-16`:
   the two negative stores want a signed spelling. On the pointer-walk body an s16
   temp bought 79.08 vs 78.83; not tried on this body.
   (d) the two norm loads are merged here (`lw v0,16(sp); lw v1,24(sp); sw; sw`)
   where retail has load/nop/store pairs.
   (e) the spawn sums fold the `-0x10` after the `% 32`; retail hoists it into the
   bgez delay slot as `addiu a1,v0,-0x10`.
   Tried this round, both worse: the correction inlined at the four uses (74.59%,
   236 insns) and an early `pBase` local with `&pBase[i]` (79.81%, 230 insns - it
   reloads g_BattleSpriteSlots after the two vector calls). Not a preset question:
   the pointer-walk body measured gcc 2.6.0 80.00, cdk 72.27, and -O3 / -G0
   unchanged. Next: permute THIS body (the /2 hoist that defeated the two earlier
   permuter runs is gone) or find the second induction register.
*/
/*
void func_8007DC74(VECTOR *pA, VECTOR *pB) {
    VECTOR vec;
    VECTOR norm;
    BattleSpriteSlot *pSlot;
    u32 count;
    s32 spread;
    s32 dy;
    s32 len2;
    s32 steps;
    s32 t;
    s32 q;
    s32 i;

    if (pA->vy < 0x81 && pB->vy < 0x81) {
        return;
    }

    vec.vz = pB->vx - pA->vx;
    vec.vx = pA->vz - pB->vz;
    vec.vy = pA->vy - pB->vy;

    steps = BattleVectorLength((s32 *)&vec) / 6;
    if (steps <= 0) {
        steps = 1;
    }
    vec.vy = 0x1000;

    len2 = BattleVectorLength2D((s32 *)&vec);
    if (len2 < 0x10) {
        len2 = 0;
    }
    if (vec.vz != 0 || vec.vx != 0) {
        VectorNormal(&vec, &vec);
        norm.vx = vec.vx;
        norm.vz = vec.vz;
    }

    dy = pA->vy - pB->vy;
    if (dy < 0) {
        dy = -dy;
    }
    count = dy / 3 + 1;
    if (count >= 0x79) {
        count = 0x78;
    }
    spread = len2 / 7;

    for (i = 0; i < 0x218; i++) {
        pSlot = &((BattleSpriteSlot *)g_BattleSpriteSlots)[i];
        if (steps == 0) {
            break;
        }
        if (pSlot->life != 0) {
            continue;
        }
        if (len2 != 0) {
            t = rand() % 0x1000;
            q = spread - (len2 >> 31);
            if (rand() & 1) {
                pSlot->vx = (norm.vx * t >> 14) + q;
                pSlot->vz = (norm.vz * t >> 14) + q;
            } else {
                pSlot->vx = -((norm.vx * t) >> 14) - q;
                pSlot->vz = -((norm.vz * t) >> 14) - q;
            }
        } else {
            pSlot->vx = (rand() & 0xF) - 8;
            pSlot->vz = (rand() & 0xF) - 8;
        }
        if (dy >= 9) {
            pSlot->life = count;
            pSlot->vy = 1 - rand() % (dy / 2);
        } else {
            pSlot->vy = -4;
            pSlot->life = 0x14;
        }
        pSlot->x = pA->vx - 0x10 + (rand() % 32) + pSlot->vx * 2;
        pSlot->y = -0x10;
        pSlot->z = pA->vz - 0x10 + (rand() % 32) + pSlot->vz * 2;
        steps--;
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007DC74);
