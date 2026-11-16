#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

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
        /* 0x2 */ u8 unk46;
        /* 0x3 */ u8 unk47;
    } byte;
} BattleSlotFlags; // Size: 0x4

typedef struct {
    /* 0x00 */ BattleQuad pos;
    /* 0x10 */ BattleQuad prevPos;
    /* 0x20 */ BattleQuad unk20;
    /* 0x30 */ BattleQuad unk30;
    /* 0x40 */ u8 unk40[0x3];
    /* 0x43 */ u8 unk43;
    /* 0x44 */ BattleSlotFlags unk44;
    /* 0x48 */ u8 pad48[0x8];
    /* 0x50 */ s32 unk50;
} BattleActorSlot; // Size: 0x54

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ u16 flags;
    /* 0x8 */ u8 index;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
} BattleActorState;

typedef struct {
    /* 0x0 */ u8 pad0[0x6];
    /* 0x6 */ u16 flags;
    /* 0x8 */ u8 index;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
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
    /* 0x12 */ s16 unk12;
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
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
} BattlePartTiming; // Size: 0x2

typedef struct {
    /* 0x000 */ s32 x;
    /* 0x004 */ s32 y;
    /* 0x008 */ s32 z;
    /* 0x00C */ u8 pad0C[0x40];
    /* 0x04C */ u8 partIndex;
    /* 0x04D */ u8 unk4D;
    /* 0x04E */ u8 prevPartIndex;
    /* 0x04F */ u8 unk4F;
    /* 0x050 */ s16 unk50;
    /* 0x052 */ u8 unk52;
    /* 0x053 */ u8 pad53;
    /* 0x054 */ s32 unk54;
    /* 0x058 */ u8 pad58[0x4];
    /* 0x05C */ BattleModelLink *pModelLink;
    /* 0x060 */ u8 pad60[0x1C];
    /* 0x07C */ BattlePartTiming *pPartTimings;
    /* 0x080 */ u8 pad80[0x4];
    /* 0x084 */ s32 unk84;
    /* 0x088 */ u8 pad88[0x2C];
    /* 0x0B4 */ s16 unkB4;
    /* 0x0B6 */ u16 gaugePoints;
    /* 0x0B8 */ u8 padB8[0x2];
    /* 0x0BA */ s16 gaugeLevel;
    /* 0x0BC */ s16 unkBC;
    /* 0x0BE */ u8 padBE[0x12];
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 padD4[0x4];
    /* 0x0D8 */ u8 *pTarget;
    /* 0x0DC */ u8 padDC[0x28];
    /* 0x104 */ BattleActorSlot slots[16];
    /* 0x644 */ u32 unk644;
    /* 0x648 */ u8 pad648[0x2BC];
    /* 0x904 */ u8 *pVisiblePartIndices;
    /* 0x908 */ u8 visiblePartCount;
    /* 0x909 */ u8 unk909;
    /* 0x90A */ u8 pad90A[0x4];
    /* 0x90E */ u8 unk90E;
    /* 0x90F */ u8 pad90F[0x89];
    /* 0x998 */ s16 unk998;
    /* 0x99A */ s16 unk99A;
    /* 0x99C */ s16 unk99C;
    /* 0x99E */ s16 unk99E;
    /* 0x9A0 */ u8 pad9A0[0xC2C];
    /* 0x15CC */ BattleActorState *pState;
    /* 0x15D0 */ BattleActorAnimData *pAnimData;
    /* 0x15D4 */ u8 unk15D4;
    /* 0x15D5 */ u8 pad15D5[0x17];
    /* 0x15EC */ s16 unk15EC;
    /* 0x15EE */ s16 unk15EE;
    /* 0x15F0 */ u8 pad15F0[0x2];
    /* 0x15F2 */ s16 unk15F2;
    /* 0x15F4 */ u8 pad15F4[0x60];
    /* 0x1654 */ s32 unk1654;
    /* 0x1658 */ s32 unk1658;
} Actor; // Size: unknown

/* The 0x60-entry effect context allocated by BattleEffectInitContext. */
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 pad02[0x12];
    /* 0x14 */ s16 x;
    /* 0x16 */ s16 y;
    /* 0x18 */ s16 z;
    /* 0x1A */ u8 pad1A[0x1A];
    /* 0x34 */ s16 rotX;
    /* 0x36 */ s16 rotY;
    /* 0x38 */ s16 rotZ;
    /* 0x3A */ u8 pad3A[0xA];
    /* 0x44 */ s16 unk44;
    /* 0x46 */ s16 unk46;
    /* 0x48 */ s16 unk48;
    /* 0x4A */ s16 unk4A;
    /* 0x4C */ u8 pad4C[0x1C];
    /* 0x68 */ s16 unk68;
    /* 0x6A */ s16 unk6A;
    /* 0x6C */ u8 pad6C[0x8];
    /* 0x74 */ u8 r;
    /* 0x75 */ u8 g;
    /* 0x76 */ u8 b;
} BattleEffect; // Size: unknown

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80072D18);

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
   source shape moved it to $t0. */
/* void func_800732AC(u32 *p0, u32 *p1, s32 n) {
    n += (s32)p1;
    do {
        *p0 = *p1;
        p1++;
        p0++;
    } while ((s32)p1 != n);
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800732AC);

extern BattleEffect *func_8008D3F4(s32, s32);
extern void BattleEffectAllocEntries(BattleEffect *, s32);
extern BattleEffect *D_80092644;

void BattleEffectInitContext(void) {
    BattleEffect *p = func_8008D3F4(3, 0);

    p->r = 0x80;
    p->g = 0x80;
    p->b = 0xC0;
    BattleEffectAllocEntries(p, 0x60);
    p->unk0 = 4;
    p->unk44 = 0x300;
    p->unk48 = 8;
    p->unk4A = 0x20;
    p->unk68 = 0;
    p->unk6A = 0x20;
    D_80092644 = p;
}

extern s32 D_80092648;
extern s32 D_80092A24;
extern s32 D_80092A28;
extern s32 D_80092A2C;
extern u8 D_80091C0C[];
extern void func_8008D680(BattleEffect *, u8 *, s32);
extern void BattleEffectUpdateEntries(BattleEffect *, u8 *, u8 *);

void BattleEffectSetMatrix(u8 *p0, u8 *p1) {
    s32 n = D_80092648;
    BattleEffect *pEffect = D_80092644;
    u8 *p = (u8 *)pEffect;

    /* Written through a byte pointer: a typed store lets gcc hoist the next
       global load above the previous store and the original interleaves. */
    *(s16 *)(p + 0x14) = D_80092A24;
    *(s16 *)(p + 0x16) = D_80092A28;
    *(s16 *)(p + 0x18) = D_80092A2C;
    *(s16 *)(p + 0x34) = 0;
    *(s16 *)(p + 0x36) = 0;
    *(s16 *)(p + 0x38) = 0;
    func_8008D680(pEffect, D_80091C0C, n);
    gte_SetTransRotMatrix_gpr(p1);
    BattleEffectUpdateEntries(pEffect, p0, p1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073424);
/* 98.09%: the loop's induction shape. The original keeps BOTH a walking
   pointer ($a2 = p2 + i*0x44, tested as `lbu v0,0x67C(a2)`) and the byte offset
   ($t0, used as `addiu v0,t0,0x64C; addu s0,s3,v0`); gcc folds the two into one
   register, and which one survives depends on the source shape: `p2 + 0x67C +
   i * 0x44` with `pSlot = p2 + 0x64C + i * 0x44` keeps the walking pointer for
   the test but turns pSlot into a sll/addu multiply chain (89.52%), and a single
   `off` variable feeding both gives the addiu back but recomputes the test
   address as `addu v0,off,p2; lbu v0,1660(v0)` (this 98.09%). Also tried a
   separate walking pFlags with off += 0x44 (gcc drops off entirely, 98.09 with
   the pointer in $a0). Everything else - the frame (the unused s32 pad[2] at
   sp+0x30), both VECTOR locals at sp+0x10/sp+0x20, the 0x10-byte copies and all
   four calls - matches instruction for instruction.
extern u8 D_800910F4[];
extern void BattleNormalizeVectorToShort(VECTOR *, SVECTOR *);
extern void func_8008EBD0(u8 *, s32, void *, s32);

void func_80073424(VECTOR *p0, VECTOR *p1, u8 *p2, s32 arg3, s32 arg4, s32 arg5) {
    VECTOR vDiff;
    VECTOR vNormal;
    s32 pad[2];
    u8 *pSrc;
    u8 *pSlot;
    u8 *pEntry;
    s32 i;
    s32 off;
    u8 *pFlags;

    off = 0;
    pFlags = p2;
    for (i = 0; i < 9; i++) {
        if (*(u8 *)(pFlags + 0x67C) == 0) {
            pSlot = p2 + 0x64C + off;
            break;
        }
        pFlags += 0x44;
        off += 0x44;
    }
    if (i == 8) {
        return;
    }
    pSrc = *(u8 **)(p2 + 0xD8);
    vNormal.vx = *(s32 *)(pSrc + 0x0) - p0->vx;
    vNormal.vy = *(s32 *)(pSrc + 0x4) - p0->vy - 0x90;
    vNormal.vz = *(s32 *)(pSrc + 0x8) - p0->vz;
    pEntry = D_800910F4 + arg3 * 8;
    if (p1 != NULL) {
        vDiff.vx = p0->vx - p1->vx;
        vDiff.vy = p0->vy - p1->vy;
        vDiff.vz = p0->vz - p1->vz;
    } else {
        vDiff = vNormal;
    }
    BattleNormalizeVectorToShort(&vDiff, (SVECTOR *)(pSlot + 0x28));
    *(s16 *)(pSlot + 0x3E) = *(u16 *)(pEntry + 0x0);
    *(u8 *)(pSlot + 0x31) = *(u8 *)(pEntry + 0x4);
    *(s16 *)(pSlot + 0x40) = *(u8 *)(pEntry + 0x3);
    *(u8 *)(pSlot + 0x32) = *(u8 *)(pEntry + 0x2);
    *(u8 *)(pSlot + 0x33) = *(u8 *)(pEntry + 0x5);
    *(s32 *)(pSlot + 0x38) = arg5;
    *(s16 *)(pSlot + 0x3C) = arg4;
    if (*(u8 *)(pEntry + 0x6) != 0) {
        func_8008EBD0(p2, *(u8 *)(pEntry + 0x6), pSlot, 2);
    }
    BattleScaleSVectorGpf12((SVECTOR *)(pSlot + 0x28), (SVECTOR *)(pSlot + 0x20), *(u8 *)(pSlot + 0x31));
    *(VECTOR *)pSlot = *p0;
    *(VECTOR *)(pSlot + 0x10) = *(VECTOR *)pSlot;
    *(u8 *)(pSlot + 0x30) = 1;
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073644);

void BattleTransformEntryVertex(Actor *p0, s32 arg1, s32 arg2, VECTOR *pOut) {
    BattlePart *pEntry = p0->pModelLink->pModel->pParts[arg1];

    if (arg2 != 0 && pEntry->type == 1) {
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
        if (pSlot->unk44.byte.state == 2) {
            pSlot->unk44.byte.state = 1;
        } else {
            pSlot->unk44.byte.state = 0;
        }
    }
}

extern s32 D_80092650;

void BattleStoreSlotPositions(BattleQuad *p0, BattleQuad *p1, s32 arg2, u8 *p3, BattleActorSlot *p4,
                              s32 arg5, Actor *p6) {
    u32 flags;

    arg2 &= 1;
    p4->prevPos = p4->pos;
    p4->unk30 = p4->unk20;
    p4->pos = *p0;
    p4->unk20 = *p1;
    flags = p4->unk44.flags;
    flags &= ~2;
    flags |= arg2 << 1;
    p4->unk43 = arg5;
    p4->unk44.flags = flags;
    p4->unk44.byte.state = 2;
    p4->unk44.byte.unk47 = p6->unk644;
    p4->unk50 = p6->unk84;
    p4->unk44.byte.unk46 = p3[1];
    D_80092650++;
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
   2.7.2-cdk -O2 98.06, -O3 98.06, -G0 98.06, cdk -O3 compile-failed. */
/* s32 func_80073DE4(u8 *p0, s32 arg1) {
    s32 t = *(s16 *)(p0 + 0xB6) + arg1;

    if (t >= 0x1001) {
        return ((t - 0xFF1) / 20) < *(s16 *)(p0 + 0xB4);
    }
    return 1;
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073DE4);

extern s32 D_800928C8;
extern void BattlePlaySoundEffectById(s32);

s32 BattleAccumulateGaugePoints(Actor *p0, s32 arg1, s32 arg2) {
    s16 t;
    s32 q;

    if (D_800928C8 == 4) {
        return 1;
    }
    if (D_800928C8 == 6) {
        return 1;
    }
    t = p0->gaugePoints + arg1;
    p0->gaugePoints = t;
    if (t < 0x1001) {
        return 1;
    }
    q = (t - 0x1000) / 20;
    if (q >= p0->unkB4) {
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
        p0->unk1654 += (s16)q;
        break;
    case 2:
        p0->unk1658 += (s16)q;
        break;
    }
    return 1;
}

/* 97.87% (428 bytes against the original's 432): the body is right but one
   instruction is scheduled differently. The original hoists `addiu a3,sp,0x10`
   (the vecA argument) to just after `move a0,s2`, which leaves
   `move s3,v0` free for the BattleTransformEntryVertex delay slot; ours emits it as the last
   argument, it takes that delay slot, and `move s3,v0` then falls into the
   BattleEffectIsTwoVertexType delay slot, losing one `nop`. Also `lbu a2,1(s1)` in the
   original against `lbu v1,1(s1)` here. Tried a `s32 *pVecA` temp (coalesced
   away, no change) and both if/else orders (the inverted one is the 97.87; the
   direct one is 89.81). gcc 2.7.2-cdk rejects the unit. The frame layout
   (locals at sp+0x10 and sp+0x20, 0x10 unused below) matches with two s32[4]. */
/* extern s32 BattleEffectPackKey(s32, u8, s16, s32);
extern s32 BattleTransformEntryVertex(u8 *, u8, s16, void *);
extern s32 BattleEffectIsTwoVertexType(u8);
extern void func_8007CD44(s32, void *, void *, s32);
extern void BattleEffectQueueLineByType(void *, void *);
extern void BattleEffectSetColor(u8 *);
extern void func_8007C880(s32, void *, s32, s32);
extern void BattleEffectAddSlotFromVector(void *, s32);

void func_80073F34(u8 *p0, u8 *p1) {
    s32 vecA[4];
    s32 vecB[4];
    s32 r;

    BattleEffectPackKey((*(u32 *)(p0 + 0xD0) >> 27) & 1, *(u8 *)(p1 + 2), *(s16 *)(p1 + 4), 1);
    r = BattleTransformEntryVertex(p0, *(u8 *)(p1 + 2), *(s16 *)(p1 + 4), vecA);
    if (BattleEffectIsTwoVertexType(*(u8 *)(p1 + 1)) != 0) {
        s32 *pVecB = vecB;

        BattleTransformEntryVertex(p0, *(u8 *)(p1 + 3), *(s16 *)(p1 + 6), pVecB);
        if (*(u8 *)(p1 + 1) == 0x10) {
            func_8007CD44((*(u32 *)(p0 + 0xD0) >> 27) & 1, vecA, pVecB, r);
        } else {
            BattleEffectQueueLineByType(vecA, pVecB);
        }
        return;
    }
    if (*(u8 *)(p1 + 2) != *(u8 *)(p1 + 3) || *(s16 *)(p1 + 4) != *(s16 *)(p1 + 6)) {
        BattleTransformEntryVertex(p0, *(u8 *)(p1 + 3), *(s16 *)(p1 + 6), vecB);
        vecA[0] = (vecA[0] + vecB[0]) / 2;
        vecA[1] = (vecA[1] + vecB[1]) / 2;
        vecA[2] = (vecA[2] + vecB[2]) / 2;
    }
    BattleEffectSetColor(p0 + 0x15D4);
    if (*(u8 *)(p1 + 1) >= 0x20) {
        func_8007C880((*(u32 *)(p0 + 0xD0) >> 27) & 1, vecA, r, *(u8 *)(p1 + 1) - 0x20);
    } else {
        BattleEffectAddSlotFromVector(vecA, *(u8 *)(p1 + 1));
    }
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073F34);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800740E4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074678);

void BattleUpdatePartDrawFlags(Actor *p0) {
    BattlePart **pParts = p0->pModelLink->pModel->pParts;
    s32 i;

    for (i = 0; i < p0->pModelLink->pModel->count; i++) {
        BattlePart *pPart = pParts[i];

        if (pPart->type == 1) {
            pPart->pDraw->flags &= ~1;
        }
    }
    for (i = 0; i < p0->visiblePartCount; i++) {
        BattlePart *pPart = pParts[p0->pVisiblePartIndices[i]];

        pPart->pDraw->flags |= 1;
    }
    if (p0->unk909 == 0xD) {
        pParts[0xD]->pDraw->flags |= 1;
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
    p0->unk54 = (pState->flags << 20) >> 20;
    pAnim = &p0->pModelLink->pModel->pAnims[pData->index];
    if (pData->flags & 0x1000) {
        BattleUpdatePartDrawFlags(p0);
        BattleResetPartAnim(pAnim);
    }
    func_80074678(p0, pAnim->unk12, p0->pAnimData->unk9);
    if (pData->unkA != 0) {
        func_8008B730(pAnim, pData->unk9, pData->unkA);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074BA4);
/* 99.08% (1212 bytes, the original's size): the whole body matches except the
   register the flag constant lands in, in two of the four mode-case blocks. The
   original loads -0x801 into $v1 and the reloaded flags into $a0 (`li v1,-2049;
   lw a0,208(s0); and a0,a0,v1; or a0,a0,v0`), gcc picks the opposite
   (`li a0,-2049; lw v1,208(s0)`); cases 0 and 2 come out exactly right with the
   same source shape, so it is a per-block local-alloc tie, not a source shape.
   Tried: swapping the `|` operands (93.43%, size wrong), a `flags` local for the
   read-modify-write (97.62% with the constant in $a0 everywhere), `(u8)` casts,
   declaration orders for `i`/`old`/`delta`/`ret`. Levers that DID work and must
   be kept: `part = p0->partIndex;` + `(u8)part != p0->prevPartIndex` reproduces the original's
   redundant `andi a0,a1,0xff` and, with `&D_80091130[part]` as the first
   statement of the if-body, its second `sll/addu s4` (gcc CSEs the two identical
   `&D_80091130[p0->partIndex]` forms away); `old + (p0->unk4F + p0->unk52)` gives the
   original's operand order; the read-modify-writes must be written on the FIELD
   (`p0->flags = (p0->flags & ~0x800) | ((ret & 1) << 11)`) so gcc tests the
   stored register and stores before the branch; `p0->unk998 = pAnim->unk12;`
   before `p0->unk99A = delta;`; `i = 0;` as its own statement plus `while
   (i < delta)`; unkA declared before unk9 for the two zero-inits. */
/* void func_80074BA4(Actor *p0) {
    BattleActorState *pState = p0->pState;
    BattlePartMode *pMode;
    BattlePartAnim *pAnim;
    s32 ret;
    s32 i;
    u16 old;
    s32 delta;
    u8 part;
    s32 unkA = 0;
    s32 unk9 = 0;

    pState->flags = (pState->flags & 0xF000) | (p0->unk54 & 0xFFF);
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
            p0->unk99C = p0->pPartTimings[p0->partIndex].unk0;
        }
        if (p0->flags & 0x02000000) {
            p0->unk99C = 1;
            p0->flags &= ~0x02000000;
        }
        p0->unk4F = (p0->pPartTimings[p0->partIndex].unk1 * p0->unk15F2) / 256;
        pAnim = &p0->pModelLink->pModel->pAnims[p0->partIndex];
        BattleUpdatePartDrawFlags(p0);
        BattleResetPartAnim(pAnim);
        p0->unk99E = 0;
        p0->unk50 = -1;
        p0->flags &= ~0x10000000;
        pState->flags |= 0x1000;
        if (pMode->mode == 2) {
            p0->unk90E = p0->partIndex;
        } else {
            p0->unk90E = 0;
        }
    }
    i = 0;
    old = p0->unk99E;
    p0->unk99E = old + (p0->unk4F + p0->unk52);
    delta = ((p0->unk99E << 16) >> 20) - ((old << 16) >> 20);
    p0->unk998 = pAnim->unk12;
    p0->unk99A = delta;
    while (i < delta) {
        if (p0->unk99C != 1) {
            p0->unk99C--;
            if (p0->unk99C < 1) {
                p0->unk99C = 1;
            }
        }
        i++;
    }
    switch (pMode->mode) {
    case 0:
        if ((p0->flags & 0x800) == 0) {
            ret = func_8008B730(pAnim, delta, p0->unk99C);
            unk9 = delta;
            unkA = p0->unk99C;
            p0->flags = (p0->flags & ~0x800) | ((ret & 1) << 11);
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
        ret = func_8008B730(pAnim, delta, p0->unk99C);
        unk9 = delta;
        unkA = p0->unk99C;
        p0->flags = (p0->flags & ~0x800) | ((ret & 1) << 11);
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
            ret = func_8008B730(pAnim, delta, p0->unk99C);
            unk9 = delta;
            unkA = p0->unk99C;
            p0->flags = (p0->flags & ~0x800) | ((ret & 1) << 11);
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
            ret = func_8008B730(pAnim, delta, p0->unk99C);
            unk9 = delta;
            unkA = p0->unk99C;
            p0->flags = (p0->flags & ~0x800) | ((ret & 1) << 11);
        }
        break;
    }
    pState->unk9 = unk9;
    pState->unkA = unkA;
} */

extern Actor D_8009872C;
extern s32 D_80099D8C;
extern s32 D_80099D88;
extern u8 D_80050622;
extern s32 BattleCheckValueAboveScaledMax(u8 *, s32);

void BattleSetResultCode(s32 arg0) {
    Actor *p = &D_8009872C;
    u8 *pDst;
    s16 v;
    s32 a;
    s32 b;

    if (arg0 != 0) {
        if (BattleCheckValueAboveScaledMax((u8 *)p, 0xE0) != 0) {
            D_80050622 = 2;
            return;
        }
        if (BattleCheckValueAboveScaledMax((u8 *)p, 0x10) != 0) {
            D_80050622 = 1;
            return;
        }
        D_80050622 = 3;
        return;
    }
    if (D_80099D8C == 0 && D_80099D88 == 0) {
        D_80050622 = 0x88;
        return;
    }
    if (D_80099D88 == 0) {
        D_80050622 = 0x82;
        return;
    }
    v = p->unkBC;
    a = (v * 176) / 255;
    if (a < p->unk1654) {
        D_80050622 = 0x83;
        return;
    }
    b = (v * 160) / 255;
    if (b < p->unk1658) {
        D_80050622 = 0x84;
        return;
    }
    if (a < p->unk1658 + p->unk1654) {
        D_80050622 = 0x85;
        return;
    }
    pDst = p->pTarget;
    if (BattleCheckValueAboveScaledMax(pDst, 0xE0) != 0) {
        D_80050622 = 0x86;
        return;
    }
    if (BattleCheckValueAboveScaledMax(pDst, 0x10) != 0) {
        D_80050622 = 0x81;
        return;
    }
    D_80050622 = 0x87;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800751C8);

/* 92.66% (1352 bytes, the original's size): the whole body is right - every
   call, every global store and the tail match instruction for instruction. What
   is left: (a) the two loads of p0/p1's x/z for ratan2 come out one instruction
   later than retail's (retail hoists them above the D_8009284C store, gcc
   stores first), (b) the D_8009263C cascade's three BattleGetScreenMode blocks
   are ordered/scheduled differently, and (c) the `mode = 4 / if (== 2) mode = 2`
   pair: retail has the 4 in the bne's delay slot. Written from the assembly in
   one pass; the structure (if/else-if cascade over p0/p1's +0xD0 flags, then the
   0x1E/0x3C timer cascade on D_8009263C, then the distance/angle tail) is
   believed correct. */
/* extern s32 g_BattleTransitionFrames;
extern u8 D_800928C4;
extern u8 D_80091144;
extern u8 D_80091145;
extern s32 D_80092638;
extern s32 D_80092640;
extern s32 D_80092890;
extern s32 D_80092918;
extern u8 D_800928D4;
extern u16 D_80097102;
extern u8 D_800928FC;
extern s32 D_800928C8;
extern u16 D_8009881E;
extern s32 D_8009263C;
extern u8 D_80092884;
extern s32 D_80092950;
extern s32 D_80092850;
extern s32 D_8009284C;
extern s32 D_80092934;
extern u8 D_8006FC30[];
extern u8 D_8006FC34[];
extern u8 D_8006FC3C[];
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

void func_800751C8(u8 *p0, u8 *p1) {
    char buf[0x10];
    s32 mode;

    g_BattleTransitionFrames++;
    BattleSetTextCursor(0xA0, 0x64);
    if (!BattleCheckValueAboveScaledMax(p0, 0x60) || !BattleCheckValueAboveScaledMax(p1, 0x60)) {
        if (D_800928C4 != 0 && !BattleCheckPositionPastMapCenter(p0) &&
            !BattleCheckPositionPastMapCenter(p1)) {
            D_80091144 = 1;
            D_80091145 = 0xA;
            BattleInitOnce();
        }
    }
    if (D_80092638 != 0) {
        mode = 4;
        if (D_80092640++ >= 0x3D) {
            if (D_80092890 == 2) {
                mode = 2;
            }
            BattleSetScreenMode(mode);
            printf(D_8006FC30);
            HeapAlloc(1, 0);
            printf(D_8006FC34);
        }
        if (D_80092890 == 2) {
            BattleByteRegTestReg(D_8006FC3C);
        } else {
            BattleByteRegTestReg(D_8006FC48);
        }
    } else if (*(u32 *)(p0 + 0xD0) & 0x800000) {
        if (*(u32 *)(p1 + 0xD0) & 0x800000) {
            D_80092638 = 1;
            D_800928D4 = 0;
            D_80092890 = 2;
            D_80092918++;
        } else {
            D_80097102++;
            D_80092638 = 1;
            D_800928D4 = 0;
            D_80092890 = 1;
            if (*(u32 *)(p1 + 0xD0) & 0x40) {
                D_800928FC = 0;
            } else {
                D_800928FC = 1;
            }
            BattleSetResultCode(0);
        }
    } else if (*(u32 *)(p1 + 0xD0) & 0x800000) {
        D_8009881E++;
        D_80092638 = 1;
        D_800928D4 = 0;
        D_80092890 = 0;
        D_800928FC = 0;
        BattleSetResultCode(1);
        if ((*(u32 *)(p1 + 0xD0) & 0x40) && D_800928C8 != 3) {
            printf(D_8006FC54);
            HeapAlloc(1, 2);
            printf(D_8006FC34);
            BattleMarkSlotAndQueueIfAllMarked(*(u8 *)(p1 + 0x909));
            printf(D_8006FC58);
            HeapAlloc(1, 2);
            printf(D_8006FC5C);
        }
    }
    if (D_8009263C != 0) {
        if (D_8009263C < 0x1E) {
            if (BattleGetScreenMode() != 7 && D_800928C8 != 4) {
                BattleByteRegTestReg(D_8006FC64);
            }
            D_800928D4 = 1;
        } else if (D_8009263C < 0x3C) {
            if (BattleGetScreenMode() != 7) {
                if (D_800928C8 == 4) {
                    BattleByteRegTestReg(D_8006FC6C);
                    if (D_80092884 != 0) {
                        BattleByteRegTestReg(D_8006FC74);
                        BattleByteRegTestReg(D_8006FC78);
                    }
                } else {
                    BattleByteRegTestReg(D_8006FC8C);
                    if (D_80092884 != 0) {
                        BattleByteRegTestReg(D_8006FC74);
                        BattleByteRegTestReg(D_8006FC94);
                    }
                }
            }
        } else {
            if (BattleGetScreenMode() != 7) {
                if (D_800928C8 == 4) {
                    BattleByteRegTestReg(D_8006FCA8);
                    if (D_80092884 != 0) {
                        BattleByteRegTestReg(D_8006FC74);
                        BattleByteRegTestReg(D_8006FC78);
                    }
                } else {
                    Sprintf(buf, D_8006FCB4, D_80092950);
                    BattleByteRegTestReg(buf);
                    if (D_80092884 != 0) {
                        BattleByteRegTestReg(D_8006FC74);
                        BattleByteRegTestReg(D_8006FC94);
                    }
                }
            }
        }
        D_8009263C--;
    }
    D_80092850 = BattleVectorDistance(p0, p1);
    D_8009284C = BattleVectorDistance2D(p0, p1);
    D_80092934 = ratan2(*(s32 *)(p0 + 0x0) - *(s32 *)(p1 + 0x0), *(s32 *)(p0 + 0x8) - *(s32 *)(p1 + 0x8));
} */


s32 BattleComputeScaledFieldDelta(Actor *p0, Actor *p1, s32 scale) {
    return (p1->unk15EC * scale) / 32 - p0->unk15EE;
}
