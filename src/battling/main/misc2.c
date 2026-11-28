#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

/* Entries of the effect context BattleEffectInitContext allocates. */
#define BATTLE_EFFECT_ENTRY_COUNT 96
/* Points above the gauge base that make one level. */
#define BATTLE_GAUGE_POINTS_PER_LEVEL 20
/* Denominator of a 0..255 gauge fraction. */
#define BATTLE_GAUGE_FRACTION_SCALE 255
/* Bit 0 of BattlePartDraw.flags: the part is drawn. */
#define BATTLE_PART_DRAW_VISIBLE 0x1
/* BattlePart.type of a part placed by its own matrix. */
#define BATTLE_PART_TYPE_MATRIX 1

/* Battle actor layout, recovered from the offsets this unit touches. */
typedef struct {
    /* 0x00 */ s32 vx;
    /* 0x04 */ s32 vy;
    /* 0x08 */ s32 vz;
    /* 0x0C */ s32 unk0C;
} BattleQuad; // Size: 0x10

/* 0x44 is read and written as one word and its upper bytes one at a time. */
typedef union {
    /* 0x0 */ u32 flags;
    struct {
        /* 0x0 */ u8 unk44;
        /* 0x1 */ u8 state;
        /* 0x2 */ u8 actionId;
        /* 0x3 */ u8 gaugePercent;
    } byte;
} BattleSlotFlags; // Size: 0x4

typedef struct {
    /* 0x00 */ BattleQuad pos;
    /* 0x10 */ BattleQuad prevPos;
    /* 0x20 */ BattleQuad endPos;
    /* 0x30 */ BattleQuad prevEndPos;
    /* 0x40 */ u8 unk40[0x3];
    /* 0x43 */ u8 partIndex;
    /* 0x44 */ BattleSlotFlags slotFlags;
    /* 0x48 */ u8 unk48;
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ s32 pActionEntry;
} BattleActorSlot; // Size: 0x54

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ u16 flags;
    /* 0x8 */ u8 index;
    /* 0x9 */ u8 frameDelta;
    /* 0xA */ u8 frameLimit;
} BattleActorState;

typedef struct {
    /* 0x0 */ u8 pad0[0x6];
    /* 0x6 */ u16 flags;
    /* 0x8 */ u8 index;
    /* 0x9 */ u8 frameDelta;
    /* 0xA */ u8 frameLimit;
} BattleActorAnimData;

typedef struct {
    /* 0x0 */ u8 pad0[0x8];
    /* 0x8 */ SVECTOR *pVerts;
} BattleMesh;

/* Draw state of one model part; bit 0 of flags is its visibility. */
typedef struct {
    /* 0x00 */ u32 flags;
    /* 0x04 */ u8 pad04[0x10];
    /* 0x14 */ BattleMesh *pMesh;
} BattlePartDraw;

/* One model part. A part of type 1 is placed by its own matrix, any other by
   the translation that matrix carries. */
typedef struct {
    /* 0x00 */ s32 type;
    /* 0x04 */ BattlePartDraw *pDraw;
    /* 0x08 */ u8 pad08[0x44];
    /* 0x4C */ MATRIX matrix;
} BattlePart; // Size: 0x6C

/* Per-part animation record in the model's part table. */
typedef struct {
    /* 0x00 */ u8 pad00[0x12];
    /* 0x12 */ s16 frame;
} BattlePartAnim; // Size: 0x14

/* The model object's part tables. */
typedef struct {
    /* 0x0 */ s16 count;
    /* 0x2 */ u8 pad02[0x2];
    /* 0x4 */ BattlePart **pParts;
    /* 0x8 */ BattlePartAnim *pAnims;
} BattleModel;

/* Actor+0x5C points at this wrapper; its 0x4 field is the model object. */
typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ BattleModel *pModel;
} BattleModelLink; // Size: 0x8

/* Per-part entry of the sequence table at D_80091130: a mode and the part
   that follows this one, -1 ending the sequence. */
typedef struct {
    /* 0x0 */ u8 mode;
    /* 0x1 */ s8 next;
} BattlePartMode; // Size: 0x2

/* Per-part timing table an actor points at from +0x7C. */
typedef struct {
    /* 0x0 */ u8 frameLimit;
    /* 0x1 */ u8 animSpeed;
} BattlePartTiming; // Size: 0x2

/* One of the 0x44-byte effect slots the actor owns at +0x64C; src/battling/
   main/misc4.c clears the same records (in-use byte and life). */
typedef struct {
    /* 0x00 */ VECTOR pos;
    /* 0x10 */ VECTOR prevPos;
    /* 0x20 */ SVECTOR velocity;
    /* 0x28 */ SVECTOR direction;
    /* 0x30 */ u8 active;
    /* 0x31 */ u8 scale;
    /* 0x32 */ u8 drawKind;
    /* 0x33 */ u8 mode;
    /* 0x34 */ u8 pad34[0x4];
    /* 0x38 */ s32 key;
    /* 0x3C */ s16 percent;
    /* 0x3E */ s16 dirWeight;
    /* 0x40 */ s16 life;
    /* 0x42 */ u8 pad42[0x2];
} BattleEffectSlot; // Size: 0x44

typedef struct Actor Actor;
struct Actor {
    /* 0x000 */ s32 x;
    /* 0x004 */ s32 y;
    /* 0x008 */ s32 z;
    /* 0x00C */ u8 pad0C[0x40];
    /* 0x04C */ u8 partIndex;
    /* 0x04D */ u8 unk4D;
    /* 0x04E */ u8 prevPartIndex;
    /* 0x04F */ u8 animSpeed;
    /* 0x050 */ s16 lastFrame;
    /* 0x052 */ u8 animSpeedBonus;
    /* 0x053 */ u8 pad53;
    /* 0x054 */ s32 angle;
    /* 0x058 */ u8 pad58[0x4];
    /* 0x05C */ BattleModelLink *pModelLink;
    /* 0x060 */ u8 pad60[0x1C];
    /* 0x07C */ BattlePartTiming *pPartTimings;
    /* 0x080 */ u8 pad80[0x4];
    /* 0x084 */ s32 pActionEntry;
    /* 0x088 */ u8 pad88[0x2C];
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ u16 gaugePoints;
    /* 0x0B8 */ u8 padB8[0x2];
    /* 0x0BA */ s16 gaugeLevel;
    /* 0x0BC */ s16 gaugeMax;
    /* 0x0BE */ s16 unkBE;
    /* 0x0C0 */ u8 padC0[0x10];
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 padD4[0x4];
    /* 0x0D8 */ Actor *pTarget;
    /* 0x0DC */ u8 padDC[0x28];
    /* 0x104 */ BattleActorSlot slots[16];
    /* 0x644 */ u32 gaugePercent;
    /* 0x648 */ u8 pad648[0x4];
    /* 0x64C */ BattleEffectSlot effectSlots[8];
    /* 0x86C */ u8 pad86C[0x98];
    /* 0x904 */ u8 *pVisiblePartIndices;
    /* 0x908 */ u8 visiblePartCount;
    /* 0x909 */ u8 typeId;
    /* 0x90A */ u8 pad90A[0x4];
    /* 0x90E */ u8 savedModelIndex;
    /* 0x90F */ u8 pad90F[0x89];
    /* 0x998 */ s16 animFrame;
    /* 0x99A */ s16 frameDelta;
    /* 0x99C */ s16 frameLimit;
    /* 0x99E */ s16 frameAccum;
    /* 0x9A0 */ u8 pad9A0[0xC2C];
    /* 0x15CC */ BattleActorState *pState;
    /* 0x15D0 */ BattleActorAnimData *pAnimData;
    /* 0x15D4 */ u8 effectColor;
    /* 0x15D5 */ u8 pad15D5[0x17];
    /* 0x15EC */ s16 unk15EC;
    /* 0x15EE */ s16 unk15EE;
    /* 0x15F0 */ u8 pad15F0[0x2];
    /* 0x15F2 */ s16 speedScale;
    /* 0x15F4 */ u8 pad15F4[0xC];
    /* 0x1600 */ u8 *unk1600;
    /* 0x1604 */ u8 pad1604[0x50];
    /* 0x1654 */ s32 gaugeLevelsMode1;
    /* 0x1658 */ s32 gaugeLevelsMode2;
}; // Size: unknown

/* The 0x60-entry effect context allocated by BattleEffectInitContext. The
   same record src/battling/main/misc16.c declares as Misc16Work. */
typedef struct {
    /* 0x00 */ s16 gravity;
    /* 0x02 */ u8 pad02[0x12];
    /* 0x14 */ s16 x;
    /* 0x16 */ s16 y;
    /* 0x18 */ s16 z;
    /* 0x1A */ u8 pad1A[0x1A];
    /* 0x34 */ s16 rotX;
    /* 0x36 */ s16 rotY;
    /* 0x38 */ s16 rotZ;
    /* 0x3A */ u8 pad3A[0xA];
    /* 0x44 */ s16 angleRange;
    /* 0x46 */ s16 unk46;
    /* 0x48 */ s16 depthBase;
    /* 0x4A */ s16 depthRange;
    /* 0x4C */ u8 pad4C[0x1C];
    /* 0x68 */ s16 unk68;
    /* 0x6A */ s16 entryLife;
    /* 0x6C */ u8 pad6C[0x8];
    /* 0x74 */ u8 r;
    /* 0x75 */ u8 g;
    /* 0x76 */ u8 b;
} BattleEffect; // Size: unknown

/* The depth is consumed out of $a2 by gte_ldir123_dp_a2, so the third
   parameter must stay in place. */
void BattleScaleSVectorGpf12(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf12_b();
    gte_stir123_lh(p1);
}

void BattleScaleSVectorGpf0(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf0_b();
    gte_stir123_lh(p1);
}

void BattleScaleStridedSVectorGpf12(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh4(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf12_b();
    gte_stir123_lh(p1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_8007313C);

/* Handwritten function: retail hand-schedules the sequence (it loads all three
   halves of the incoming vector up front into $t4-$t6, starts the next MVMVA's
   IR load between the previous group's read-back and its stores, and has a
   single hand-placed nop), and its MVMVA carries no nops at all, so a macro
   call cannot put the commands back in retail's order. Best C form gives
   24.98%. */
/* void func_800731F8(MATRIX *m, SVECTOR *v) {
    s32 x;
    s32 y;
    s32 z;

    gte_SetRotMatrix(m);
    gte_ldir123(v->vx, 0, 0);
    gte_mvmva_b(1, 0, 3, 3, 0);
    gte_stir123_gpr(x, y, z);
    m->m[0][0] = x;
    m->m[1][0] = y;
    m->m[2][0] = z;
    gte_ldir123(0, v->vy, 0);
    gte_mvmva_b(1, 0, 3, 3, 0);
    gte_stir123_gpr(x, y, z);
    m->m[0][1] = x;
    m->m[1][1] = y;
    m->m[2][1] = z;
    gte_ldir123(0, 0, v->vz);
    gte_mvmva_b(1, 0, 3, 3, 0);
    gte_stir123_gpr(x, y, z);
    m->m[0][2] = x;
    m->m[1][2] = y;
    m->m[2][2] = z;
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800731F8);

/* 98.75%: only the load temp differs, the original uses $t0 where gcc picks
   $v0 (`lw t0,0(a1)` against `lw v0,0(a1)`). Tried byte and word pointer
   parameters, `p0 += 4` against `p0++`, a separate u32 temp, an `end` pointer
   local, and a while loop (that one also grows a pre-test, 71.25%). The
   do/while with `n += (s32)p1` is right; the register is an allocator pick -
   $2, $3 and $7 are all free here, so gcc's first fresh pseudo is $v0, and no
   source shape moved it to $t0. Round 199: verified the pick is not the
   allocator carrying state from the previous function (adding a dummy function
   above leaves 98.75), and that an explicit `u32 t` temp, s32 pointers and a
   second pointer local (`u32 *end = (u32 *)((s32)p1 + n)`, 71.25%, which puts
   `end` in $v1) all leave the copy temp in $v0 - $v0 is this function's first
   free register, so the pick is out of source reach.
   Round 288: this is the same class as func_80073DE4 and func_80073F34 below - a
   dying single-use temp takes the LOWEST free call-clobbered register from our
   cc1 ($v0 here, $v1 there) where retail reused a higher one ($t0, $a2). Five
   more shapes measured byte-identical at 98.75%/32 bytes (`*p0++ = *p1++`,
   `p0[0] = p1[0]`, the comma form, `(u32 *)` casts, s32 pointers), gcc-2.6.0 is
   also 98.75, 2.7.2-cdk rejects the unit, and a 3-minute permuter run from this
   body (2072 iterations) found nothing.
   Round 394: re-measured 98.75%/32 bytes; a declaration-with-initialiser
   inside the loop (`u32 v = *p1; *p0 = v;`, the lever for a load/temp
   allocator tie) is byte-identical to the plain copy, so the pick stands.
   Round 408: re-measured again from the park, 98.75%/32 bytes, only `lw
   t0,0(a1)` vs `lw v0,0(a1)`. Below 99 so no permuter run; the note above is
   the whole story.
   Round 424: re-measured 98.75%/32 bytes (only `lw t0,0(a1)` vs `lw v0,0(a1)`);
   -O3 also 98.75. Below 99 so no permuter. The pick is local_alloc order
   ($v0 before $t0 in REG_ALLOC_ORDER), out of source reach; see the note above. */
/*
void func_800732AC(u32 *p0, u32 *p1, s32 n) {
    n += (s32)p1;
    do {
        *p0 = *p1;
        p1++;
        p0++;
    } while ((s32)p1 != n);
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800732AC);

extern BattleEffect *BattleEffectCreateWorkKind(s32, s32);
extern void BattleEffectAllocEntries(BattleEffect *, s32);
extern BattleEffect *g_BattleEffect;

void BattleEffectInitContext(void) {
    BattleEffect *p = BattleEffectCreateWorkKind(3, 0);

    p->r = 0x80;
    p->g = 0x80;
    p->b = 0xC0;
    BattleEffectAllocEntries(p, BATTLE_EFFECT_ENTRY_COUNT);
    p->gravity = 4;
    p->angleRange = 0x300;
    p->depthBase = 8;
    p->depthRange = 0x20;
    p->unk68 = 0;
    p->entryLife = 0x20;
    g_BattleEffect = p;
}

extern s32 g_BattleEffectEntryCount;
extern s32 g_BattleEffectPosX;
extern s32 g_BattleEffectPosY;
extern s32 g_BattleEffectPosZ;
extern MATRIX g_BattleBaseMatrix;
extern void func_8008D680(BattleEffect *, MATRIX *, s32);
extern void BattleEffectUpdateEntries(BattleEffect *, u8 *, MATRIX *);

void BattleEffectSetMatrix(u8 *p0, MATRIX *p1) {
    s32 n = g_BattleEffectEntryCount;
    BattleEffect *pEffect = g_BattleEffect;

    /* The first two position stores are volatile on purpose: they are the
       scheduling barriers that keep each global's load next to its own store
       (plain field stores let gcc hoist all three loads, 63.89%), while the
       third store has to stay movable to fill the call's delay slot (with it
       volatile too, 95.19%). */
    *(volatile s16 *)&pEffect->x = g_BattleEffectPosX;
    *(volatile s16 *)&pEffect->y = g_BattleEffectPosY;
    pEffect->z = g_BattleEffectPosZ;
    pEffect->rotX = 0;
    pEffect->rotY = 0;
    pEffect->rotZ = 0;
    func_8008D680(pEffect, &g_BattleBaseMatrix, n);
    gte_SetTransRotMatrix_gpr(p1);
    BattleEffectUpdateEntries(pEffect, p0, p1);
}

/* Draw parameters for one effect type, indexed by the caller's type id. */
typedef struct {
    /* 0x0 */ u16 dirWeight;
    /* 0x2 */ u8 drawKind;
    /* 0x3 */ u8 life;
    /* 0x4 */ u8 scale;
    /* 0x5 */ u8 mode;
    /* 0x6 */ u8 soundId;
    /* 0x7 */ u8 pad7;
} BattleEffectTemplate; // Size: 0x8

extern BattleEffectTemplate g_BattleEffectTemplateTable[];
extern void BattleNormalizeVectorToShort(VECTOR *, SVECTOR *);
extern void BattlePlayActorSoundPair(u8 *, s32, void *, s32);

void BattleSpawnEffectSlot(VECTOR *p0, VECTOR *p1, Actor *p2, s32 arg3, s32 arg4, s32 arg5) {
    VECTOR vDiff;
    VECTOR vNormal;
    s32 pad[2];
    BattleEffectTemplate *pEntry;
    BattleEffectSlot *pSlot;
    Actor *pTarget;
    s32 i;

    /* Retail scans nine slots although the actor holds eight, and rejects a
       find at the last index rather than an exhausted scan. */
    for (i = 0; i < 9; i++) {
        if (p2->effectSlots[i].active == 0) {
            pSlot = &p2->effectSlots[i];
            break;
        }
    }
    if (i == 8) {
        return;
    }
    pTarget = p2->pTarget;
    vNormal.vx = pTarget->x - p0->vx;
    vNormal.vy = pTarget->y - p0->vy - 0x90;
    vNormal.vz = pTarget->z - p0->vz;
    pEntry = &g_BattleEffectTemplateTable[arg3];
    if (p1 != NULL) {
        vDiff.vx = p0->vx - p1->vx;
        vDiff.vy = p0->vy - p1->vy;
        vDiff.vz = p0->vz - p1->vz;
    } else {
        vDiff = vNormal;
    }
    BattleNormalizeVectorToShort(&vDiff, &pSlot->direction);
    pSlot->dirWeight = pEntry->dirWeight;
    pSlot->scale = pEntry->scale;
    pSlot->life = pEntry->life;
    pSlot->drawKind = pEntry->drawKind;
    pSlot->mode = pEntry->mode;
    pSlot->key = arg5;
    pSlot->percent = arg4;
    if (pEntry->soundId != 0) {
        BattlePlayActorSoundPair(p2, pEntry->soundId, pSlot, 2);
    }
    BattleScaleSVectorGpf12(&pSlot->direction, &pSlot->velocity, pSlot->scale);
    pSlot->pos = *p0;
    pSlot->prevPos = pSlot->pos;
    pSlot->active = 1;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073644);

void BattleTransformEntryVertex(Actor *p0, s32 arg1, s32 arg2, VECTOR *pOut) {
    BattlePart *pEntry = p0->pModelLink->pModel->pParts[arg1];

    if (arg2 != 0 && pEntry->type == BATTLE_PART_TYPE_MATRIX) {
        gte_SetRotMatrix_gpr(&pEntry->matrix);
        gte_ldv0(&pEntry->pDraw->pMesh->pVerts[arg2 - 1]);
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(pOut);
        pOut->vx += p0->x;
        pOut->vy += p0->y;
        pOut->vz += p0->z;
    } else {
        pOut->vx = pEntry->matrix.t[0] + p0->x;
        pOut->vy = pEntry->matrix.t[1] + p0->y;
        pOut->vz = pEntry->matrix.t[2] + p0->z;
    }
}

void BattleCommitSlotStates(Actor *p0) {
    BattleActorSlot *pSlot;
    s32 i;

    for (i = 0; i < 16; i++) {
        pSlot = &p0->slots[i];
        if (pSlot->slotFlags.byte.state == 2) {
            pSlot->slotFlags.byte.state = 1;
        } else {
            pSlot->slotFlags.byte.state = 0;
        }
    }
}

extern s32 g_BattleSlotStoreCount;

void BattleStoreSlotPositions(BattleQuad *p0, BattleQuad *p1, s32 arg2, u8 *p3, BattleActorSlot *p4,
                              s32 arg5, Actor *p6) {
    u32 flags;

    arg2 &= 1;
    p4->prevPos = p4->pos;
    p4->prevEndPos = p4->endPos;
    p4->pos = *p0;
    p4->endPos = *p1;
    flags = p4->slotFlags.flags;
    flags &= ~2;
    flags |= arg2 << 1;
    p4->partIndex = arg5;
    p4->slotFlags.flags = flags;
    p4->slotFlags.byte.state = 2;
    p4->slotFlags.byte.gaugePercent = p6->gaugePercent;
    p4->pActionEntry = p6->pActionEntry;
    p4->slotFlags.byte.actionId = p3[1];
    g_BattleSlotStoreCount++;
}

/* 98.06%: inverting the condition (`if (t >= 0x1001)`) is what makes reorg put
   `li v0,1` in the bnez delay slot instead of a separate block with a `j`, and
   `hw + arg1` fixes the addu operand order. Left over: the original keeps the
   sum in the parameter's $a1 and the hw temp in $v1 (`lh v1; addu a1,v1,a1;
   slti v1,a1`), ours assigns the sum to $v1 and the temp to $v0. Also tried
   `arg1 += hw` in place, a separate s32/s16 local, and `t` declared after the
   load - the sum pseudo never coalesces back into $a1. Also retried this
   session as `arg1 = *(s16 *)(p0 + 0xB6) + arg1` in place and with a `hw`
   local (both 97.78%, operands come out swapped as well); 2.6.0 gives 97.22
   and 2.7.2-cdk 97.78, so the preset is not the lever. Retested this session:
   2.7.2-cdk -O2 98.06, -O3 98.06, -G0 98.06, cdk -O3 compile-failed. Round 199:
   retail's allocation is OURS ONE REGISTER HIGHER - retail `lh v1,0xB6` + sum
   in `$a1` (the parameter's own register, freed by the add) against ours
   `lh v0` + sum in `$v1` - and `arg1 += hw;` / `arg1 += *(s16 *)(p0 + 0xB6);`
   do NOT coalesce the sum into that `$a1` (both `addu v1,a1,v0`, 97.78), so the
   in-place spelling is not the lever either. A single shared exit (`result`
   variable) is worse: it grows the frame to 76 bytes and 84.17%.
   Round 288: a 3-minute permuter run from this body (2072 iterations,
   base=best=35) found nothing; see func_800732AC above for why the whole class
   (a dying single-use temp taking our cc1's lowest free register, $v0 here for
   the `lh` against retail's $v1) is a local_alloc difference, not a source
   shape.
   Round 333: 98.61% (72 bytes, retail's size) with the load and the sum spelled
   as TWO statements and a bare declaration (`s32 t; t = *(s16 *)(p0 + 0xB6);
   t = t + arg1;`) - that alone fixes retail's `lh v1,0xB6`. The two remaining
   instructions are the DESTINATIONS: ours `addu v1,v1,a1` / `slti v0,v1,4097`
   against retail `addu a1,v1,a1` / `slti v1,a1,4097`. Root cause measured this
   round: with the sum spelled INLINE in the condition (`if ((t + arg1) >=
   0x1001)`) cc1 DOES put the sum in the parameter's $a1 (`addu a1,v1,a1`, load
   also in $v1), but then the sum has a single use, so the slti redefines $a1 in
   place (`slti a1,a1,4097`) where retail used the dead $v1. Giving the sum its
   second use (the division) is what moves it back to $v1 and the load to $v0
   (98.06). Retail keeps a multi-use sum in the argument register across the
   branch; our local_alloc will not, and that is why no spelling reaches 100.
   Two permuter runs this round (from this body and from the inline one) only
   re-found the inline form with the DIVISION reading the raw load instead of
   the sum, i.e. by changing what the body computes.
   Round 394: four more shapes at 72 bytes, all 18 instructions: `arg1 =
   hw + arg1;` with the load as a separate statement 97.78 (97.22 under 2.6.0),
   `s32 t = *(s16 *)(p0 + 0xB6) + arg1;` as one initialised declaration 98.06,
   and `t = arg1; hw = *(s16 *)(p0 + 0xB6); t = t + hw;` (the copy-to-local
   spelling that local-alloc ties) 97.78. The two-statement form above stays the
   best at 98.61. Preset ruled out for this body: 2.6.0 98.06, 2.7.2-cdk
   compile-failed (unit-wide), -O3 98.61, -O1 59.72, -G0 compile-failed. gcc
   2.8.1 (Psy-Q 4.4) is ruled out for the whole unit: under it this body scores
   61.39 and six of the unit's fourteen matched functions fall to 61-96
   (BattleEffectInitContext 88.56, BattleStoreSlotPositions 86.87), so misc2 is
   not a 2.8.1 unit. The residual is the one instruction pair: retail keeps a
   multi-use sum in the dying parameter's $a1 (`addu a1,v1,a1`) and reuses the
   dead $v1 for the slti, ours puts the sum in $v1 and the slti in $v0. The
   neighbouring park above (99.91%) reports the same signature - "gcc 2.7.2's
   local_alloc will not reuse an argument register that it charged to this
   block; retail's compiler did" - so this unit has three stubs in one
   allocator class; do not spend another lane hand-dialling it.
   Round 394 also measured an empty `do { t = t + arg1; } while (0)` around the
   sum: byte-identical 98.61, the tie does not move.
   Round 410: un-parked and re-measured 98.61% / 72 bytes (retail's size). The
   realistic typing of the same body - `Actor *p0`, `t = (s16)p0->gaugePoints`,
   `p0->gaugeValue` and BATTLE_GAUGE_POINTS_PER_LEVEL (the parked body below) -
   is byte-identical at 98.61, so the residual is not a typing artefact.
   Also measured this round, both 18 insns / 72 bytes: writing the sum twice so
   cse merges it into one two-use pseudo (`if ((t + arg1) >= 0x1001) return
   ((t + arg1 - 0xFF1) / 20) < ...`) is 98.06 and still allocates the sum to
   `$v1`; a `u32 amount = arg1` copy of the parameter (the parameter-copy
   signedness lever) coalesces to no instruction and leaves the code identical.
   One fact for a later lane: retail reads 0xB6 with `lh` here but with `lhu`
   in BattleAccumulateGaugePoints (asm/battling/main/misc2.s line 986), so the
   field is u16 and this function's original C read it as signed.
   Round 425: re-parked at 98.61% / 72 bytes (retail's size, 18 insns). The
   residual is ONE root cause, the sum's register: retail keeps it in the dying
   parameter's $a1 (addu a1,v1,a1 / slti v1,a1,4097 / addiu v1,a1,-4081) and
   ours puts it in the load temp's $v1 (addu v1,v1,a1 / slti v0,v1,4097), with
   the bnez operand and the addiu following. Fresh evidence: the -da .lreg dump
   for this exact body shows cc1's RTL makes the sum the SAME pseudo as the load
   ((set (reg/v:SI 74) (plus:SI (reg/v:SI 74) (reg:SI 5 a1))), "Register 74
   used 5 times across 7 insns", live into basic block 2), so it is a GLOBAL
   allocno handed $v1 by REG_ALLOC_ORDER - the load temp never gets its own
   register to lose. A cc1 probe (same cc_flags, -G8) of five other spellings -
   arg1 += t, arg1 = arg1 + t, arg1 = t + arg1, s16 load + s32 sum, and an
   initialised-declaration sum - emits lh $2 / addu $3 / slt $2 in all five,
   i.e. load $v0, sum $v1, compare $v0, and none hands the sum the parameter's
   $a1. Retail's hand-out needs an allocno preferenced to $a1 that our cc1 has
   no source shape for: park it, do not re-dial.
   Round 438 (REDO, un-parked and re-measured): the parked body is honest at
   98.61% / 72 bytes / 18 insns (retail's size), confirmed twice. Two new
   spellings, both 97.78% / 72 bytes / 10 differing lines, that pin the
   allocator behaviour rather than guess at it: (a) `t = arg1; t += (s16)
   p0->gaugePoints;` emits lh $v0 / addu $v1,a1,v0 / slti $v0 - so with the sum
   as a pseudo DISTINCT from the load the allocator still hands the sum $v1 and
   demotes $a1 to a source, never a destination; (b) `arg1 += (s16)
   p0->gaugePoints;` (the parameter modified in place, i.e. the one spelling
   that would give the sum $a1 for free) emits the identical 10 differences,
   which is source-level proof of the local_alloc rule in the note above: cc1
   2.7.2 abandons a parameter's incoming register once the parameter is set in
   this block. Do not re-dial; the only lever left is a compiler whose
   local_alloc keeps arguments' home registers, and no preset in the repo does
   (2.6.0 98.06, -O3 98.61, 2.8.1 breaks six matched functions). */
// Round 550 (REDO lane, un-parked and re-measured): 98.61% / 72 bytes / 18 insns,
//    retail's exact size. The residual is one allocator pair - retail addu a1,v1,a1 /
//    slti v1,a1,4097, ours addu v1,v1,a1 / slti v0,v1,4097 - and lever.py's
//    a-two-use-sum-does-not-tie entry closes it: the sum is the load's own pseudo, a
//    global allocno handed $v1 by REG_ALLOC_ORDER, and no source shape hands it the
//    dying parameter's $a1. Already measured by a five-spelling cc1 probe, two
//    permuter runs, and 2.6.0 / 2.7.2-cdk / -O3 / -G0 (97-98). Not re-dialled.
/*
s32 func_80073DE4(Actor *p0, s32 arg1) {
    s32 t;

    t = (s16)p0->gaugePoints;
    t = t + arg1;
    if (t >= 0x1001) {
        return ((t - 0xFF1) / BATTLE_GAUGE_POINTS_PER_LEVEL) < p0->gaugeValue;
    }
    return 1;
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073DE4);




extern s32 g_BattleMode;
extern void BattlePlaySoundEffectById(s32);

s32 BattleAccumulateGaugePoints(Actor *p0, s32 arg1, s32 arg2) {
    s16 t;
    s32 q;

    if (g_BattleMode == 4) {
        return 1;
    }
    if (g_BattleMode == 6) {
        return 1;
    }
    t = p0->gaugePoints + arg1;
    p0->gaugePoints = t;
    if (t < 0x1001) {
        return 1;
    }
    q = (t - 0x1000) / BATTLE_GAUGE_POINTS_PER_LEVEL;
    if (q >= p0->gaugeValue) {
        p0->gaugePoints = t - arg1;
        p0->gaugeLevel = 0;
        if (BattleGetScreenMode() != 4) {
            BattlePlaySoundEffectById(0x2E);
        }
        return 0;
    }
    p0->gaugePoints = 0x1000;
    p0->gaugeLevel = q;
    switch (arg2) {
    case 1:
        p0->gaugeLevelsMode1 += (s16)q;
        break;
    case 2:
        p0->gaugeLevelsMode2 += (s16)q;
        break;
    }
    return 1;
}

/* 99.91% (432 bytes, the original's size): the only difference is the register
   the `p1[1]` test value lands in - retail `lbu a2,1(s1); li v0,0x10; bne a2,v0`,
   ours `lbu v1,1(s1)` with the same `li v0,0x10` and the same bne. Nothing else
   differs: frame, the `move s3,v0` in the second call's delay slot, the
   `addiu a0,sp,0x10` in the bne slot and the three one-word averages all match,
   and a constant local moves the value to `$v0`, which shows the allocator is
   stepping over `$a2` on purpose (the block already put the earlier
   `lh a2,6(s1)` argument there, and gcc 2.7.2's local_alloc will not reuse an
   argument register that it charged to this block; retail's compiler did).
   Tried this round, all 432 bytes and 99.91%: u8 and s32 locals (block-scope and
   top-of-function, embedded assignment `if ((type = p1[1]) == 0x10)`); the
   test as a one-case `switch`; swapped operands `0x10 == p1[1]`; `!(x != 0x10)`;
   a `s32 lineType = 0x10` constant local (96.41%, 440 bytes); a volatile read
   (97.90%, 436 bytes); a `do { ... } while (0)` around the if/else (81.74%, it
   moves p1 to `$s0`); an empty `do { } while (0)` before the test; and the
   BattleTransformEntryVertex arguments hoisted into s32 locals. Permuter run this
   round from this exact body: 1726 iterations, base=best=10, nothing.
   Next lever not tried: an edit that makes the vertex offset load reach `$a2`
   through a copy suggestion (a source-level tie between the two uses), which is
   where a stronger permuter mutation set or a manual RTL read would start.
   Round 199 re-checked: 2.6.0 gives 99.91 and 2.7.2-cdk 94.17, so the preset is
   not the lever; wrapping only the second BattleTransformEntryVertex call in
   do { } while (0) leaves 99.91. The earlier 'block-scope u8/s32 local, 432
   bytes' readings were STALE: gcc 2.7.2 rejects a declaration after a statement
   (cc1 exit 33, 'parse error'), diff_func.sh then scores the previous variant's
   object, so nothing changed size. Declare such locals at the top of the
   function when retrying this class.
   Round 288: un-parked and re-measured at 99.91%/432 bytes against retail's 432,
   so the only residual is still `lbu v1,1(s1)` for `lbu a2,1(s1)` (same
   instruction count, same delay slot). Nine equivalent spellings of the test
   (`0x10 == p1[1]`, `(u8)` cast, `(p1[1] & 0xFF)`, `p1[1] - 0x10`, u8 and s32
   block locals, a one-case `switch`) all measure 99.91; only inverting the arms
   moves anything (91.99). A 3-minute permuter run from this exact body (2195
   iterations, base=best=10) found nothing either. Diagnosis is the same as
   func_800732AC above: retail's cc1 picked a register our local_alloc will not
   (it gave this dying single-use temp $a2 while $v1, $a0 and $a1 were free), so
   the three parks of this unit are one compiler-version difference and not worth
   another source-shape round. */
/* 99.91% (432 bytes, the original's size): re-measured this round from the parked body;
   the only difference is the register the `p1[1]` test value lands in - retail
   `lbu a2,1(s1); li v0,0x10; bne a2,v0`, ours `lbu v1,1(s1)` with the same
   `li v0,0x10` and the same bne, 108 vs 108 instructions.
   Tried this round: a `u16 lineType` local for the test (unchanged, 99.91);
   preset screen (2.6.0 99.91, 2.7.2-cdk 94.17 in an earlier round); a third
   3-minute permuter run from this body (1507 iterations, base=best=10, none).
   Earlier rounds measured a dozen spellings of the test (0x10 == p1[1], casts,
   block-scope u8/s32 locals, a one-case switch, inverting the arms at 91.99) and
   a constant local (96.41), a volatile read (97.90) and do/while wrappers (81.74).
   Diagnosis unchanged: retail's cc1 handed this dying single-use temp `$a2`
   (the register it charged to the block's earlier `lh a2,6(s1)` argument) where
   our local_alloc takes the lowest free register `$v1` while $a2, $a0, $a1 are
   free - an allocator tie no source shape has moved, so this park is a
   compiler-version residual, not an unwritten body. */
/* Round 417: un-parked and re-measured 99.91% / 432 bytes / 108 insns; the single
   differing line is still `lbu a2,1(s1)` (retail) against `lbu v1,1(s1)` (ours),
   with the same `ori v0,0x10` and the same `bne`. A fourth 3-minute permuter run
   from this exact body (1109 iterations, base=best=10) found nothing. New evidence
   this round, so the next round does not repeat it: `cc1 ... -O2 -G8 -dl` on
   build/src/battling/main/misc2.c.i (the unit's real flags) shows the test value is
   a 2-reference, block-local pseudo living 3 insns in block 1, GR_REGS with NO copy
   suggestion and no call crossed, so its register is nothing but the first free one
   in REG_ALLOC_ORDER - an allocno contest retail's cc1 resolved to $a2 only through
   a copy suggestion this dying single-use temp cannot create. The block-1 pseudos
   are 98 and 99 (2 refs each over 3 and 4 insns) and both are block-local, so this
   is a local-alloc hand-out, not a global priority quotient: no C spelling of the
   test moves it. Park stands. */
// Round 540: un-parked and re-measured 99.91% / 432 bytes / 108 insns (retail's exact size); the single differing line is still 'lbu a2,1(s1)' (retail) against 'lbu v1,1(s1)' (ours) with the same 'ori v0,0x10' and the same 'bne'. New shape this round: a two-case 'switch (p1[1]) { case 0x10: ... default: ... }' instead of the if/else - byte-identical, 99.91. 'try_flags --opt -O3' does not even produce a comparable object (0.0) and '--gp -G0' fails to compile the body in this unit, so the preset is out. A SIXTH permuter run from this exact body (2185 iterations, base=best=10) found nothing. Diagnosis unchanged and now backed by three levers: this is the local-alloc hand-out of a dying single-use temp (levers 'a-twin-port-cannot-fix-a-load-tie' and 'a-pluss-result-and-a-loads-dest-both-take-the-dying-base-register'), out of source reach. Park stands.
/*
void func_80073F34(Actor *p0, u8 *p1) {
    VECTOR vecA;
    VECTOR vecB;
    s32 r;

    r = BattleEffectPackKey((p0->flags >> 27) & 1, p1[2], *(s16 *)(p1 + 4), 1);
    BattleTransformEntryVertex(p0, p1[2], *(s16 *)(p1 + 4), &vecA);
    if (BattleEffectIsTwoVertexType(p1[1]) != 0) {
        BattleTransformEntryVertex(p0, p1[3], *(s16 *)(p1 + 6), &vecB);
        if (p1[1] == 0x10) {
            func_8007CD44((p0->flags >> 27) & 1, &vecA, &vecB, r);
        } else {
            BattleEffectQueueLineByType(&vecA, &vecB);
        }
        return;
    }
    if (p1[2] != p1[3] || *(s16 *)(p1 + 4) != *(s16 *)(p1 + 6)) {
        BattleTransformEntryVertex(p0, p1[3], *(s16 *)(p1 + 6), &vecB);
        vecA.vx = (vecA.vx + vecB.vx) / 2;
        vecA.vy = (vecA.vy + vecB.vy) / 2;
        vecA.vz = (vecA.vz + vecB.vz) / 2;
    }
    BattleEffectSetColor(&p0->effectColor);
    if (p1[1] >= 0x20) {
        func_8007C880((p0->flags >> 27) & 1, &vecA, r, p1[1] - 0x20);
    } else {
        BattleEffectAddSlotFromVector(&vecA, p1[1]);
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073F34);



extern s32 g_BattleDrawFlags;
extern s32 g_BattleGaugeHighlightValue;
/* Per-side gauge record; the later fields are separate D_ symbols in the
   symbol table (0x80096FBC/FC0/FC4/FC8). misc3 and misc4 declare the same
   0x14-byte record. */
typedef struct {
    /* 0x00 */ u32 value;
    /* 0x04 */ u8 percent;
    /* 0x05 */ u8 pad5[0x3];
    /* 0x08 */ u32 prevValue;
    /* 0x0C */ u32 frameStart;
    /* 0x10 */ u32 frameCurrent;
} BattleSlotGauge; // Size: 0x14

extern BattleSlotGauge g_BattleSlotGauges[];
extern s32 BattleEffectPackKey(s32, u8, s16, s32);
extern void func_8007CD44(s32, void *, void *, s32);
extern void func_8007C880(s32, void *, s32, s32);
extern void BattleEffectAddSlotFromVector(void *, s32);
extern void BattleResetMessageQueue(Actor *);

/* Side of the battle the actor fights on: bit 27 of its flags. Retail re-reads
   the flags at every site, so the expression stays inline. */
#define BATTLE_ACTOR_SIDE(p) ((((p)->flags) >> 27) & 1)

// PARKED 99.33% (objdiff 1424 bytes against retail's 1416, 356 insns against 357; the .s
// size says retail is 1428 bytes, ours 1424, so this body is 4 bytes short and cannot link).
// Round 476 took it from 92.03 to 99.33 WITHOUT changing the frame size by hand. The frame
// (0x78), the saved set, every save offset, the vertex/effect prologue, both spawn arms, the
// gauge stores, the kind dispatch and both slot-search loops now match retail exactly.
// The four shapes that did it:
// - the gauge store block is three CHAINED assignments
//   (`frameStart = frameCurrent = pActor->frameAccum`,
//   `value = prevValue = g_BattleGaugeHighlightValue`): gcc CSEs the side index AND the
//   loaded value across ONE statement but not across two, which is retail's pair-of-stores
//   shape (one `lh 0x99E(s1)` and one `srl/andi/sll/addu` chain per pair, re-formed per pair,
//   five stores -> three re-forms).
// - the kind dispatch is THREE separate statements (`if (kind < 4) goto search;`,
//   `if (kind >= 0x27) goto search;`, `if (kind < 0x21) goto search;`): the single `&&` chain
//   folds to one `addiu -3X / sltiu` range test, and the earlier note's "three ifs are worth
//   ~20 extra bytes" was measured on a body that still had the old store block.
// - the 0x21..0x26 arm calls BattleSpawnEffectSlot in BOTH arms of an if/else (`single != 0`
//   passes 0, else `&vecB`): gcc cross-jumps the identical call tail, which is retail's `j`
//   into the shared argument setup. A ternary gives a different, 2-instruction-shorter arm.
// - a dead `u_long unusedFrame[4]` declared LAST takes vars 40 -> retail's 56, which puts
//   vecA at 0x20, vecB at 0x30 and the saves at 0x58/0x5C..0x70 exactly as retail has them.
//   Declared FIRST the same array lands at 0x20 and pushes the vectors up (96.39%, all
//   stack operands differ). This is levers.md's j-an-unused-local-array-is.
// Residual, named:
// - ONE instruction: the found-slot block's third argument copy `move a2,s4` (arg3 = single).
//   reorg's eager fill hoists it out of the block into the delay slot of
//   `bne v0,a1,<body>`; retail keeps a `nop` in that slot and the copy inside the block.
//   This is the c-eager-fill-steals-the-targets-arg0-copy class. A 3-minute permuter run
//   (1353 iterations) did not move it (base=251 best=251); a ternary, an if/else with a
//   pointer local, and swapping the ternary's operands all compiled to the same shape.
// - the five gauge stores print as `sw v0,16(at)` / `%lo(g_BattleSlotGauges)` against
//   retail's `sw v0,0(at)` / `%lo(D_80096FC8)`: same linked bytes (typed-data-addend class).
// Preset re-checked on this body: 2.6.0 89.53, 2.7.2-cdk 83.89, -G0 identical to unit flags.
// Re-measured this session: 99.33%, objdiff 1424 bytes / 356 insns against retail 1428 / 357. The whole
// residual is ONE instruction: retail keeps a nop in the delay slot of the third
// `bne v0,a1,<found1>` and copies a2 = s4 (arg 3) inside the block; cc1 puts `move $6,$20`
// first in the target block and reorg.rs eager fill hoists it into that slot, which deletes
// the nop (4 bytes short, so it can never link). cc1 own .s shows the steal before maspsx.
// Second 3-minute permuter run this session: PERMUTER_NONE, base=251 best=251, 1320
// iterations (matches the earlier 1353-iteration run) - the fill is not reachable by any
// small edit. Presets re-checked earlier: 2.6.0 89.53, cdk 83.89, -G0 identical.
// Round 547 audit re-measure: 99.33%, objdiff 1424 bytes / 356 insns against retail 1428 / 357.
// Residual is still exactly ONE instruction: retail keeps a nop in the delay slot of the third
// `bne v0,a1,<found1>` and copies its third argument (a2 = s4 = single) as the block third insn;
// cc1 + reorg puts `move $6,$20` in that slot, so the body is 4 bytes short and can never link.
// NEW evidence this round (the reason to stop hunting source shapes): compiled the unit .i with the
// unit flags PLUS -fno-delayed-branch, which prints the pre-reorg block, and it is byte-for-byte
// retail order -- `addu $4,$sp,32 / addu $5,$sp,48 / move $6,$20 / sw $16,16($sp) / lbu $2,2($18) /
// move $7,$18 / sw $17,24($sp) / sw $2,20($sp) / jal BattleStoreSlotPositions`. So the target block
// fed to reorg is NOT the difference: reorg scans the block, skips the two address copies (both set
// registers that are live on the fall-through loop tail: $4 by `addu $4,$4,84`, $5 = g_BattleDrawFlags
// compared in the next iteration) and takes the only safe insn, the a2 copy ($6 is dead on that path
// until the later `bne $6,$0`). Nothing in the source can move that copy later without changing the
// block, and -dR carries no DEBUG_REORG output in this cc1, so the heuristic cannot be inspected
// further. Presets re-checked on THIS body: 2.6.0 89.53, 2.7.2-cdk 83.89, -G0 identical to unit flags.
// Permuter: two runs on this exact body (1353 and 1320 iterations) both PERMUTER_NONE, base=251
// best=251; a third was not run rather than repeated. Hand shapes measured dead earlier and again
// named here: ternary instead of if/else, if/else with a pointer local, swapped ternary operands.
// One more honesty note on this body: the two `taken` tests below (the post-loop `if (taken != 0)`
// and the second loop guard) are a DEAD local kept only because it reproduces retail `bnez a2` /
// `beqz a2`; the same two tests written on `single` (the honest spelling) measure 98.80% /
// 1420 bytes / 355 insns, because gcc then tests a different register. Fix that before shipping
// this body, if it ever links.
// Do not re-brief spellings for this one; it needs a reorg-level answer or a different assembler path.
/*
void func_800740E4(Actor *pActor, u8 *p1, s32 arg2) {
    VECTOR vecA;
    VECTOR vecB;
    BattleActorSlot *pSlot;
    s32 key;
    s32 single;
    s32 kind;
    s32 percent;
    s32 mode;
    s32 i;
    u_long unusedFrame[4];

    key = BattleEffectPackKey(BATTLE_ACTOR_SIDE(pActor), p1[2], *(s16 *)(p1 + 4), 0);
    if (p1[2] == p1[3] && *(s16 *)(p1 + 4) == *(s16 *)(p1 + 6)) {
        BattleTransformEntryVertex(pActor, p1[2], *(s16 *)(p1 + 4), &vecA);
        if (((u8 *)pActor->pActionEntry)[2] != 0 && (p1[1] & 0x40) == 0) {
            func_8007C880(BATTLE_ACTOR_SIDE(pActor), &vecA, key, 0);
        }
        vecB = vecA;
        single = 1;
    } else {
        BattleTransformEntryVertex(pActor, p1[2], *(s16 *)(p1 + 4), &vecA);
        BattleTransformEntryVertex(pActor, p1[3], *(s16 *)(p1 + 6), &vecB);
        if (((u8 *)pActor->pActionEntry)[2] != 0 && (p1[1] & 0x40) == 0) {
            func_8007CD44(BATTLE_ACTOR_SIDE(pActor), &vecA, &vecB, key);
        }
        single = 0;
    }
    if (arg2 == 0) {
        return;
    }
    if (p1[1] == 0x20) {
        if (BattleAccumulateGaugePoints(pActor, pActor->unkBE, 1) != 0 && BattleGetScreenMode() != 4) {
            goto spawn;
        }
        mode = BattleGetScreenMode();
        if (mode != 4 || (pActor->pAnimData->flags & 0x8000) == 0) {
            goto simple;
        }
    spawn:
        if (single == 0) {
            vecA.vx = (vecA.vx + vecB.vx) / 2;
            vecA.vy = (vecA.vy + vecB.vy) / 2;
            vecA.vz = (vecA.vz + vecB.vz) / 2;
        }
        BattleSpawnEffectSlot(&vecA, 0, pActor, 0, pActor->unk1600[0x18], key);
        BattleResetMessageQueue(pActor);
        pActor->pState->flags |= 0x8000;
        goto store;
    simple:
        BattleEffectAddSlotFromVector(&vecA, 9);
        BattleResetMessageQueue(pActor);
        pActor->pState->flags &= 0x7FFF;
    store:
        g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].frameStart = g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].frameCurrent = pActor->frameAccum;
        g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].value = g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].prevValue = g_BattleGaugeHighlightValue;
        g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].percent = pActor->unk1600[0x18];
        return;
    }
    if (g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].prevValue != g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].value) {
        g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].frameStart = pActor->frameAccum;
        g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].prevValue = g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].value;
    }
    g_BattleSlotGauges[BATTLE_ACTOR_SIDE(pActor)].frameCurrent = pActor->frameAccum;
    kind = p1[1];
    percent = pActor->gaugePercent;
    if (kind == 4) {
        BattleSpawnEffectSlot(&vecA, 0, pActor, 1, percent, key);
        return;
    }
    if (kind < 4) {
        goto search;
    }
    if (kind >= 0x27) {
        goto search;
    }
    if (kind < 0x21) {
        goto search;
    }
    if (single != 0) {
        BattleSpawnEffectSlot(&vecA, 0, pActor, kind - 0x20, percent, key);
    } else {
        BattleSpawnEffectSlot(&vecA, &vecB, pActor, kind - 0x20, percent, key);
    }
    return;
search:
    {
        s32 taken = 0;

        for (i = 0; i < 16; i++) {
            pSlot = &pActor->slots[i];
            if (pSlot->slotFlags.byte.state == 1 && pSlot->unk4C == key && pSlot->unk48 != g_BattleDrawFlags) {
                BattleStoreSlotPositions(&vecA, &vecB, single, p1, pSlot, p1[2], pActor);
                pSlot->slotFlags.flags &= ~1;
                pSlot->unk48 = g_BattleDrawFlags;
                return;
            }
        }
        if (taken != 0) {
            return;
        }
        for (i = 0; i < 16 && taken == 0; i++) {
            pSlot = &pActor->slots[i];
            if (pSlot->slotFlags.byte.state == 0) {
                BattleStoreSlotPositions(&vecA, &vecB, single, p1, pSlot, p1[2], pActor);
                pSlot->unk4C = key;
                pSlot->slotFlags.flags |= 1;
                pSlot->unk48 = g_BattleDrawFlags;
                return;
            }
        }
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800740E4);




INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074678);

void BattleUpdatePartDrawFlags(Actor *p0) {
    BattlePart **pParts = p0->pModelLink->pModel->pParts;
    s32 i;

    for (i = 0; i < p0->pModelLink->pModel->count; i++) {
        BattlePart *pPart = pParts[i];

        if (pPart->type == BATTLE_PART_TYPE_MATRIX) {
            pPart->pDraw->flags &= ~BATTLE_PART_DRAW_VISIBLE;
        }
    }
    for (i = 0; i < p0->visiblePartCount; i++) {
        BattlePart *pPart = pParts[p0->pVisiblePartIndices[i]];

        pPart->pDraw->flags |= BATTLE_PART_DRAW_VISIBLE;
    }
    if (p0->typeId == 0xD) {
        pParts[0xD]->pDraw->flags |= BATTLE_PART_DRAW_VISIBLE;
    }
}



extern void func_80074678(Actor *, s32, u8);
extern void BattleResetPartAnim(BattlePartAnim *);
extern s32 func_8008B730(BattlePartAnim *, s32, s32);
extern BattlePartMode D_80091130[];

void BattleActorApplyAnimationCommand(Actor *p0) {
    BattleActorState *pState = p0->pState;
    BattleActorAnimData *pData = p0->pAnimData;
    BattlePartAnim *pAnim;

    p0->x = pState->x;
    p0->y = pState->y;
    p0->z = pState->z;
    p0->partIndex = pData->index;
    p0->angle = (pState->flags << 20) >> 20;
    pAnim = &p0->pModelLink->pModel->pAnims[pData->index];
    if (pData->flags & 0x1000) {
        BattleUpdatePartDrawFlags(p0);
        BattleResetPartAnim(pAnim);
    }
    func_80074678(p0, pAnim->frame, p0->pAnimData->frameDelta);
    if (pData->frameLimit != 0) {
        func_8008B730(pAnim, pData->frameDelta, pData->frameLimit);
    }
}

void func_80074BA4(Actor *p0) {
    BattleActorState *pState = p0->pState;
    BattlePartMode *pMode;
    BattlePartAnim *pAnim;
    s32 ret;
    s32 i;
    u16 old;
    s32 delta;
    s32 oldFrames;
    u8 part;
    s32 flags;
    s32 unkA = 0;
    s32 unk9 = 0;

    pState->flags = (pState->flags & 0xF000) | (p0->angle & 0xFFF);
    pState->x = p0->x;
    pState->y = p0->y;
    pState->z = p0->z;
    pState->index = p0->partIndex;
    pMode = &D_80091130[p0->partIndex];
    pAnim = &p0->pModelLink->pModel->pAnims[p0->partIndex];
    pState->flags &= ~0x1000;
    part = p0->partIndex;
    if ((u8)part != p0->prevPartIndex || (p0->flags & 0x10000000) != 0) {
        pMode = &D_80091130[part];
        p0->prevPartIndex = part;
        p0->flags = (p0->flags | 0x1000) & ~0x800;
        if ((p0->flags & 0x10000000) == 0) {
            p0->frameLimit = p0->pPartTimings[p0->partIndex].frameLimit;
        }
        if (p0->flags & 0x02000000) {
            p0->frameLimit = 1;
            p0->flags &= ~0x02000000;
        }
        p0->animSpeed = (p0->pPartTimings[p0->partIndex].animSpeed * p0->speedScale) / 256;
        pAnim = &p0->pModelLink->pModel->pAnims[p0->partIndex];
        BattleUpdatePartDrawFlags(p0);
        BattleResetPartAnim(pAnim);
        p0->frameAccum = 0;
        p0->lastFrame = -1;
        p0->flags &= ~0x10000000;
        pState->flags |= 0x1000;
        if (pMode->mode == 2) {
            p0->savedModelIndex = p0->partIndex;
        } else {
            p0->savedModelIndex = 0;
        }
    }
    i = 0;
    old = p0->frameAccum;
    oldFrames = old << 16;
    p0->frameAccum = old + (p0->animSpeed + p0->animSpeedBonus);
    delta = ((p0->frameAccum << 16) >> 20) - (oldFrames >> 20);
    p0->animFrame = pAnim->frame;
    p0->frameDelta = delta;
    while (i < delta) {
        if (p0->frameLimit != 1) {
            p0->frameLimit--;
            if (p0->frameLimit < 1) {
                p0->frameLimit = 1;
            }
        }
        i++;
    }
    switch (pMode->mode) {
    case 0:
        if ((p0->flags & 0x800) == 0) {
            ret = func_8008B730(pAnim, delta, p0->frameLimit);
            unk9 = delta;
            unkA = p0->frameLimit;
            flags = p0->flags;
            flags &= ~0x800;
            flags |= (ret & 1) << 11;
            p0->flags = flags;
            if (p0->flags & 0x800) {
                if (pMode->next == -1) {
                    p0->flags &= ~0x1000;
                } else {
                    p0->partIndex = pMode->next;
                }
            }
        }
        break;
    case 1:
        p0->flags &= ~0x1000;
        ret = func_8008B730(pAnim, delta, p0->frameLimit);
        unk9 = delta;
        unkA = p0->frameLimit;
        flags = p0->flags;
        flags &= ~0x800;
        flags |= (ret & 1) << 11;
        p0->flags = flags;
        if (p0->flags & 0x800) {
            p0->flags |= 0x10000000;
            if (pMode->next != -1) {
                p0->partIndex = pMode->next;
            }
        }
        break;
    case 2:
        p0->flags &= ~0x1000;
        if ((p0->flags & 0x800) == 0) {
            ret = func_8008B730(pAnim, delta, p0->frameLimit);
            unk9 = delta;
            unkA = p0->frameLimit;
            flags = p0->flags;
            flags &= ~0x800;
            flags |= (ret & 1) << 11;
            p0->flags = flags;
        }
        if (p0->flags & 0x400) {
            if (pMode->next == -1) {
                p0->flags &= ~0x1000;
            } else {
                p0->partIndex = pMode->next;
            }
            p0->flags &= ~0x400;
        }
        break;
    case 3:
        if ((p0->flags & 0x800) == 0) {
            ret = func_8008B730(pAnim, delta, p0->frameLimit);
            unk9 = delta;
            unkA = p0->frameLimit;
            flags = p0->flags;
            flags &= ~0x800;
            flags |= (ret & 1) << 11;
            p0->flags = flags;
        }
        break;
    }
    pState->frameDelta = unk9;
    pState->frameLimit = unkA;
}
extern Actor g_BattleActor1;
extern s32 D_80099D8C;
extern s32 D_80099D88;
extern u8 g_BattleResultCode;
extern s32 BattleCheckValueAboveScaledMax(Actor *, s32);

void BattleSetResultCode(s32 arg0) {
    Actor *p = &g_BattleActor1;
    Actor *pDst;
    s16 v;
    s32 a;
    s32 b;

    if (arg0 != 0) {
        if (BattleCheckValueAboveScaledMax(p, 0xE0) != 0) {
            g_BattleResultCode = 2;
            return;
        }
        if (BattleCheckValueAboveScaledMax(p, 0x10) != 0) {
            g_BattleResultCode = 1;
            return;
        }
        g_BattleResultCode = 3;
        return;
    }
    if (D_80099D8C == 0 && D_80099D88 == 0) {
        g_BattleResultCode = 0x88;
        return;
    }
    if (D_80099D88 == 0) {
        g_BattleResultCode = 0x82;
        return;
    }
    v = p->gaugeMax;
    a = (v * 176) / BATTLE_GAUGE_FRACTION_SCALE;
    if (a < p->gaugeLevelsMode1) {
        g_BattleResultCode = 0x83;
        return;
    }
    b = (v * 160) / BATTLE_GAUGE_FRACTION_SCALE;
    if (b < p->gaugeLevelsMode2) {
        g_BattleResultCode = 0x84;
        return;
    }
    if (a < p->gaugeLevelsMode2 + p->gaugeLevelsMode1) {
        g_BattleResultCode = 0x85;
        return;
    }
    pDst = p->pTarget;
    if (BattleCheckValueAboveScaledMax(pDst, 0xE0) != 0) {
        g_BattleResultCode = 0x86;
        return;
    }
    if (BattleCheckValueAboveScaledMax(pDst, 0x10) != 0) {
        g_BattleResultCode = 0x81;
        return;
    }
    g_BattleResultCode = 0x87;
}

/* Load-bearing spellings: the two down counters are unbounded-ARRAY externs (a
   scalar makes maspsx emit the bare `sw sym` macro instead of lui/addiu through
   a saved register), and the facing-angle tail reads the actor positions through
   a BattleQuad view - the raw `*(s32 *)(p + 0)` form makes gcc order the two x
   loads after the g_BattleActorDistance2D store. */
extern s32 g_BattleTransitionFrames;
extern u8 g_BattleFacingUpdateEnabled;
extern u8 g_BattleReinitFlag;
extern u8 g_BattleReinitValue;
extern s32 g_BattleRoundOver;
extern s32 g_BattleRoundOverFrames;
extern s32 g_BattleResultSide;
extern s32 g_BattleBothCombatantsDownCount;
extern u8 g_BattleInputEnabled;
extern u16 g_BattleCombatant0DownCount[];
extern u8 g_BattleTextPadIndex;
extern s32 g_BattleMode;
extern u16 g_BattleCombatant1DownCount[];
extern s32 g_BattlePromptTimer;
extern u8 g_BattleOptionOn;
extern s32 g_BattleRoundNumber;
extern s32 g_BattleAiTargetDistance;
extern s32 g_BattleActorDistance2D;
extern s32 g_BattleFacingAngle;
extern u8 D_8006FC30[];
extern u8 D_8006FC34[];
extern u8 g_StrBattleMoveCheck3[];
extern u8 D_8006FC48[];
extern u8 D_8006FC54[];
extern u8 D_8006FC58[];
extern u8 D_8006FC5C[];
extern u8 D_8006FC64[];
extern u8 D_8006FC6C[];
extern u8 D_8006FC74[];
extern u8 D_8006FC78[];
extern u8 D_8006FC8C[];
extern u8 D_8006FC94[];
extern u8 D_8006FCA8[];
extern u8 D_8006FCB4[];
extern void BattleSetTextCursor(s32, s32);
extern s32 BattleCheckPositionPastMapCenter(u8 *);
extern void BattleInitOnce(void);
extern void BattleSetScreenMode(s32);
extern void BattleByteRegTestReg(u8 *);
extern void BattleMarkSlotAndQueueIfAllMarked(s32);
extern s32 BattleGetScreenMode(void);
extern s32 Sprintf(char *, char *, ...);
extern s32 BattleVectorDistance(u8 *, u8 *);
extern s32 BattleVectorDistance2D(u8 *, u8 *);

void BattleUpdateRoundOverState(Actor *p0, Actor *p1) {
    char buf[0x10];
    s32 mode;

    g_BattleTransitionFrames++;
    BattleSetTextCursor(0xA0, 0x64);
    if (!BattleCheckValueAboveScaledMax(p0, 0x60) || !BattleCheckValueAboveScaledMax(p1, 0x60)) {
        if (g_BattleFacingUpdateEnabled != 0 && !BattleCheckPositionPastMapCenter(p0) &&
            !BattleCheckPositionPastMapCenter(p1)) {
            g_BattleReinitFlag = 1;
            g_BattleReinitValue = 0xA;
            BattleInitOnce();
        }
    }
    if (g_BattleRoundOver != 0) {
        if (g_BattleRoundOverFrames++ >= 0x3D) {
            if (g_BattleResultSide == 2) {
                mode = 2;
            } else {
                mode = 4;
            }
            BattleSetScreenMode(mode);
            printf(D_8006FC30);
            HeapAlloc(1, 0);
            printf(D_8006FC34);
        }
        if (g_BattleResultSide == 2) {
            BattleByteRegTestReg(g_StrBattleMoveCheck3);
        } else {
            BattleByteRegTestReg(D_8006FC48);
        }
    } else if (p0->flags & 0x800000) {
        if (p1->flags & 0x800000) {
            g_BattleRoundOver = 1;
            g_BattleInputEnabled = 0;
            g_BattleResultSide = 2;
            g_BattleBothCombatantsDownCount++;
        } else {
            u32 flags;

            g_BattleCombatant0DownCount[0]++;
            g_BattleInputEnabled = 0;
            flags = p1->flags;
            g_BattleRoundOver = 1;
            g_BattleResultSide = 1;
            if (flags & 0x40) {
                g_BattleTextPadIndex = 0;
            } else {
                g_BattleTextPadIndex = 1;
            }
            BattleSetResultCode(0);
        }
    } else if (p1->flags & 0x800000) {
        g_BattleCombatant1DownCount[0]++;
        g_BattleRoundOver = 1;
        g_BattleInputEnabled = 0;
        g_BattleResultSide = 0;
        g_BattleTextPadIndex = 0;
        BattleSetResultCode(1);
        if ((p1->flags & 0x40) && g_BattleMode != 3) {
            printf(D_8006FC54);
            HeapAlloc(1, 2);
            printf(D_8006FC34);
            BattleMarkSlotAndQueueIfAllMarked(p1->typeId);
            printf(D_8006FC58);
            HeapAlloc(1, 2);
            printf(D_8006FC5C);
        }
    }
    if (g_BattlePromptTimer != 0) {
        if (g_BattlePromptTimer < 0x1E) {
            if (BattleGetScreenMode() != 7 && g_BattleMode != 4) {
                BattleByteRegTestReg(D_8006FC64);
            }
            g_BattleInputEnabled = 1;
        } else if (g_BattlePromptTimer < 0x3C) {
            if (BattleGetScreenMode() != 7) {
                if (g_BattleMode == 4) {
                    BattleByteRegTestReg(D_8006FC6C);
                    if (g_BattleOptionOn != 0) {
                        BattleByteRegTestReg(D_8006FC74);
                        BattleByteRegTestReg(D_8006FC78);
                    }
                } else {
                    BattleByteRegTestReg(D_8006FC8C);
                    if (g_BattleOptionOn != 0) {
                        BattleByteRegTestReg(D_8006FC74);
                        BattleByteRegTestReg(D_8006FC94);
                    }
                }
            }
        } else {
            if (BattleGetScreenMode() != 7) {
                if (g_BattleMode == 4) {
                    BattleByteRegTestReg(D_8006FCA8);
                    if (g_BattleOptionOn != 0) {
                        BattleByteRegTestReg(D_8006FC74);
                        BattleByteRegTestReg(D_8006FC78);
                    }
                } else {
                    Sprintf(buf, D_8006FCB4, g_BattleRoundNumber);
                    BattleByteRegTestReg(buf);
                    if (g_BattleOptionOn != 0) {
                        BattleByteRegTestReg(D_8006FC74);
                        BattleByteRegTestReg(D_8006FC94);
                    }
                }
            }
        }
        g_BattlePromptTimer--;
    }
    g_BattleAiTargetDistance = BattleVectorDistance(p0, p1);
    g_BattleActorDistance2D = BattleVectorDistance2D(p0, p1);
    g_BattleFacingAngle = ratan2(((BattleQuad *)p0)->vx - ((BattleQuad *)p1)->vx,
                                 ((BattleQuad *)p0)->vz - ((BattleQuad *)p1)->vz);
}

s32 BattleComputeScaledFieldDelta(Actor *p0, Actor *p1, s32 scale) {
    return (p1->unk15EC * scale) / 32 - p0->unk15EE;
}
