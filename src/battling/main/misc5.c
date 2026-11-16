#include "common.h"
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

// One entry of the effect action table D_80092A74: the two per-frame texture
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
    /* 0x10 */ u16 unk10;
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
    // creation tick at 0x70, type 2 the second end of its beam at 0x64/0x66/0x68
    // and the key at 0x70. Only fields that mean one thing to every writer are named.
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
    /* 0x8 */ u8 unk8[4];
} BattlePrimTemplate; // Size: 0xC

// 0xA-byte entry table func_8007D6B8 allocates at g_BattleSpriteEntries: where
// func_8007D7A8 put an entry on the grid and how long it lives.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 z;
    /* 0x6 */ s8 life; // 0 marks a free entry; set to the entry's frame countdown
    /* 0x7 */ s8 vy; // added to y every frame and itself gains 2: gravity
    /* 0x8 */ u16 groundY; // the y the entry falls back to: it dies once y >= groundY
} BattleSpriteEntry; // Size: 0xA

extern BattleSlot D_80092AD8[];
extern BattleAction D_80092A74[];
extern Color4 D_800926B8;
extern u8 D_800926B9;
extern u8 D_800926BA;
extern u32 D_800926A4;
extern u32 g_BattleLineQuadQueueCount;
extern u32 g_BattleLineQuadDrawCount;
extern u16 D_800926A8[];
extern u16 D_800926AA;
extern u16 D_800926AC;
extern u16 D_800926AE;
extern u16 D_80092694;
extern u16 D_800926A0;
extern s16 D_80091228[];
extern u16 g_BattleEffectFrameCluts[];

// CLUT x (0x80092698) and y (0x8009269C). func_8007B270 reads each of them as a
// short, this unit's other callers only want the low byte.
extern u8 D_80092698[];
extern u8 D_8009269C[];
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

// 0x10-byte primitive template BattleInitSpriteBuffers allocates at D_800926CC/D_800926D0:
// a 0x60 tile prim, whose tag word is the OT link plus the prim's length.
typedef struct {
    /* 0x0 */ u8 unk0[3];
    /* 0x3 */ u8 len;
    /* 0x4 */ u32 colorCode; // r0,g0,b0,code: retail stores the literal as one word
    /* 0x8 */ u8 unk8[4];
    /* 0xC */ u16 w;
    /* 0xE */ u16 h;
} BattlePrimTemplate16; // Size: 0x10

extern u8 *g_BattleSpriteEntries;
extern u8 *D_800926C0;
extern u8 *D_800926C4;
extern u8 *g_BattleSpriteSlots;
extern u8 *D_800926CC;
extern u8 *D_800926D0;
extern u8 *g_BattleGridCells;
extern u8 g_BattleDrawBufferIndex;

extern int rand(void);

extern void BattleDrawLineQuads(s32);
extern void BattleEffectJitterSVector(SVector *pVec);
extern void func_8007D334(s32, s32, s32);
extern void func_800732AC(void *, void *, s32);
extern void func_8007E020(s32);

/*
Best attempt: objdiff 81.41% (a stale note here claimed 95.11%; that body does not
reproduce it - re-measured at 81.41%, and the permuter's own scoring agrees it is
not the closest shape). Retail loads r then b then g and interleaves the -0x80
adds (lbu r; lbu b; addiu r; lbu g; addiu b; bgez r; addiu g); every source shape
tried emits the loads in a different order or routes one of the three through
$t0. permute.sh 4 min: base=1156 best=675, its best candidate splits the load
from the add (g = p1[2]; b = g - 0x80) and scores worse under objdiff (86.33%).
void func_8007B270(u8 *p0, u8 *p1) {
    s32 r;
    s32 b;
    s32 g;

    r = p0[0] - 0x80;
    b = p0[2] - 0x80;
    g = p0[1] - 0x80;
    if (r < 0) {
        r = 0;
    }
    if (g < 0) {
        g = 0;
    }
    if (b < 0) {
        b = 0;
    }
    D_800926A8[0] = ((r >> 2) & 0x1F) + ((g << 3) & 0x3E0) + ((b << 8) & 0x7C00) | -0x8000;

    r = p1[0] - 0x80;
    b = p1[2] - 0x80;
    g = p1[1] - 0x80;
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
    D_800926A0 = LoadClut2((u_long *)D_800926A8, D_80092698, D_8009269C);
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007B270);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007B388);

void BattleEffectUpdateSlots(void) {
    BattleSlot *p = D_80092AD8;
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
    n = D_800926A4;
    v = g_BattleLineQuadQueueCount;
    g_BattleLineQuadQueueCount = 0;
    D_800926A4 = n + 1;
    g_BattleLineQuadDrawCount = v;
}

void BattleEffectClearSlots(void) {
    u8 *p = (u8 *)D_80092AD8;
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

    pSlot = D_80092AD8;
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
    D_800926B8 = *pColor;
}

void BattleEffectAddSlot(SVector *pVec, s32 action) {
    BattleSlot *pSlot = D_80092AD8;
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
    pAction = &D_80092A74[action];
    pSlot->type = 0;
    pSlot->frame = 0;
    pSlot->unk60 = (u32)pAction;
    pSlot->frameCount = pAction->frameCount;
    pSlot->unk64 = pAction->unk10;
    pSlot->vy = 0;
    pSlot->x = pVec->vx;
    pSlot->y = pVec->vy;
    pSlot->z = pVec->vz;
    pMotion = &pSlot->packets[0];
    pMotion->tpage = pAction->tpage;
    if (action == 2) {
        pMotion->code &= 0xFE;
        pMotion->r = D_800926B8.r;
        pMotion->g = D_800926B9;
        pMotion->b = D_800926BA;
    } else {
        pMotion->code |= 1;
    }
    pMotion->clut = pAction->clut;
    pSlot->packets[1] = *pMotion;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C280);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C880);

/* Best attempt: objdiff 85.17%, same size as retail (548 bytes) and the same
   instruction multiset; the remaining 15% is gcc putting the D_80092698 /
   arg0*4 scaling, the t2/t1/t3 motion-copy address setup and the
   g_BattleDrawFlags / D_800926A4 / 0x70 / 0x72 pair a few slots away from retail.
   Store order matches retail exactly (0x54, 0x56, 0x72, 0x78, 0x6C, 0x70,
   0x58, 0x74), so it is scheduler/allocator, not source order. Same
   85.17 under -O3 and -G0, and 80.59 under gcc-2.7.2-cdk. */
/*
void func_8007C880(s32 arg0, VECTOR *pVec, u32 arg2, s32 arg3) {
    BattleSlot *pSlot;
    BattleSlot *pEntry;
    SlotMotion *pMotion;
    u16 id;
    s32 u;
    s32 v;
    s32 i;

    pEntry = D_80092AD8;
    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->active != 0) {
            if (pEntry->key == arg2) {
                if (pEntry->drawFlags == g_BattleDrawFlags) {
                    return;
                }
            }
        }
    }

    pSlot = D_80092AD8;
    for (i = 0; i < 0x3C; i++, pSlot++) {
        if (pSlot->active == 0) {
            break;
        }
    }
    if (i == 0x3C) {
        return;
    }

    u = D_80092698[0] * 4 + 8;
    pMotion = &pSlot->packets[0];
    u += arg0 * 4;
    pMotion->u3 = u;
    pMotion->u2 = u;
    pMotion->u1 = u;
    pMotion->u0 = u;
    v = D_8009269C[0];
    pMotion->v3 = v;
    pMotion->v2 = v;
    pMotion->v1 = v;
    pMotion->v0 = v;
    pMotion->tpage = D_80092694;
    pMotion->code &= 0xFE;
    pMotion->clut = D_800926A0;
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
    pSlot->unk70 = D_800926A4;
    pSlot->z = pVec->vz;
    pSlot->halfWidth = D_80091228[arg3];

    id = D_800926A4 - 1;
    pEntry = D_80092AD8;
    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->active != 0 && pEntry->key == arg2 && pEntry != pSlot && pEntry->type == 1 && pEntry->unk70 == id) {
            pSlot->unk6C = (u32)pEntry;
        }
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CAA4);

s32 BattleEffectPackKey(s32 a0, s32 a1, s32 a2, s32 a3) {
    return (a3 & 0x7F) | ((a0 << 7) & 0x80) | (a1 << 24) | ((a2 << 8) & 0xFFFF00);
}

/* Best attempt: objdiff 85.67%. Everything matches except the scheduling of the
   0x64/0x66/0x68 stores against the u/v byte-pair writes: retail fills their
   load-delay slots with the t1 = slot + 0x28 / t2 = slot + 0x20 / a2 = slot
   address setup of the SlotMotion copy and hoists the pVec and D_800926A4 loads,
   gcc leaves nop there and keeps each load next to its store. Tried: the u/v
   pair in ascending order, unk68 stored with vy instead of after unk6A, the
   u/v block before the unk64/68 stores, and slot->unk0 in place of a pMotion
   pointer (all 82-85%); the first two halves of the function and the trailing
   search loop are byte-identical. */
/*
void func_8007CD44(s32 arg0, VECTOR *pPos, VECTOR *pVec, u32 arg3) {
    BattleSlot *pSlot = D_80092AD8;
    BattleSlot *pEntry = D_80092AD8;
    SlotMotion *pMotion;
    u16 id;
    s32 u;
    s32 v;
    s32 i;

    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->active != 0) {
            if (pEntry->unk70 == arg3) {
                if (pEntry->unk6C == g_BattleDrawFlags) {
                    return;
                }
            }
        }
    }

    pSlot = D_80092AD8;
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
    pSlot->unk6C = g_BattleDrawFlags;
    pSlot->unk64 = pVec->vx;
    pSlot->vy = pVec->vy;

    pMotion = &pSlot->packets[0];
    u = D_80092698[0] * 4 + 8;
    u += arg0 * 4;
    pMotion->u3 = u;
    pMotion->u2 = u;
    pMotion->u1 = u;
    pMotion->u0 = u;
    v = D_8009269C[0];
    pMotion->v3 = v;
    pMotion->v2 = v;
    pMotion->v1 = v;
    pMotion->v0 = v;

    pSlot->unk70 = arg3;
    pSlot->unk60 = 0;
    pSlot->unk6A = D_800926A4;
    pSlot->unk68 = pVec->vz;
    pMotion->tpage = D_80092694;
    pMotion->code &= 0xFE;
    pMotion->clut = D_800926A0;
    pSlot->packets[1] = *pMotion;

    id = D_800926A4 - 1;
    pEntry = D_80092AD8;
    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->active != 0 && pEntry->unk70 == arg3 && pEntry != pSlot && pEntry->type == 2 && pEntry->unk6A == id) {
            pSlot->unk60 = (u32)pEntry;
        }
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CD44);

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D334);

void BattleEffectQueueLineByType(s32 arg0, s32 arg1, s32 arg2) {
    switch (arg2) {
    case 0x11:
        func_8007D334(arg0, arg1, 0);
        break;
    case 0x12:
        func_8007D334(arg0, arg1, 1);
        break;
    case 0x13:
        func_8007D334(arg0, arg1, 2);
        break;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D6B8);

/* Best attempt: objdiff 84.85%. Indexed form reproduces retail's registers
   exactly (a0 = i*0xC, a1 = entry base, a2 = i, a3 = 0x6880B0F0, t0 = 2) and
   the six entry stores in retail order, but retail keeps a second induction
   register v1 = entry + 4 (used for the +4/+2/+6 stores, -2(v1)/(v1)/2(v1))
   and gcc anchors every store on the base instead. The pointer form
   (pEntry->unkX with pEntry++) does create the +4 biv and the right
   displacements, but shifts the whole register allocation up by one
   (i -> a3, offset -> a1) and compiles 4 instructions long: 83.82%. */

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D7A8);

/* Best attempt: objdiff 96.85%, retail's exact size (368 bytes). Everything is
   byte-identical except the register representation of the two u16 jitter
   values: retail carries them as value << 16 in s1 and extracts (s8) with a
   single sra 24, gcc carries the raw u16 and extracts with sll 24 / sra 24 (and
   for vz fuses the << 7 into sra 17). The store order (vy before life) and
   the separate s8 texel local came from the permuter (91.67 -> 95.82 ->
   96.85); one more permuter run from this body is the next step. */
/*
void func_8007D7A8(VECTOR *pSrc, s32 count) {
    BattleSpriteEntry *pEntry = (BattleSpriteEntry *)g_BattleSpriteEntries;
    u16 vx;
    s8 texel;
    u16 vz;
    u16 v;
    s32 index;
    s32 i;

    for (i = 0; i < 0xFB; i++, pEntry++) {
        if (count == 0) {
            break;
        }
        if (pEntry->life == 0) {
            vx = pSrc->vx - 32 + rand() % 64;
            texel = (s8)vx;
            pEntry->x = vx;
            vz = pSrc->vz - 32 + rand() % 64;
            index = (((s8)vz) << 7) + texel;
            pEntry->z = vz;
            v = *(u16 *)(g_BattleGridCells + index * 4);
            pEntry->groundY = v;
            pEntry->y = v;
            pEntry->vy = -(rand() % 10 + 10);
            pEntry->life = 0x14;
            count--;
        }
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D918);

void BattleInitSpriteBuffers(void) {
    u8 *pTemplateBuf;
    BattleSpriteSlot *pSlot;
    BattlePrimTemplate16 *pTemplate;
    s32 i;

    g_BattleSpriteSlots = HeapAlloc(0x1950, 0);
    pTemplateBuf = HeapAlloc(0x21C0, 0);
    D_800926CC = pTemplateBuf;
    D_800926D0 = HeapAlloc(0x21C0, 0);
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
    func_800732AC(D_800926D0, D_800926CC, 0x21C0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007DC74);
