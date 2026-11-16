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

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u8 r;
    /* 0x05 */ u8 g;
    /* 0x06 */ u8 b;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ u8 unk8[6];
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u8 unk10[6];
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u8 unk18[0x10];
} SlotMotion; // Size: 0x28

typedef struct {
    /* 0x00 */ u8 unk0[0xA];
    /* 0x0A */ u8 unkA;
    /* 0x0B */ u8 unkB;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
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
    /* 0x00 */ SlotMotion unk0;
    /* 0x28 */ SlotMotion unk28;
    /* 0x50 */ u8 unk50;
    /* 0x51 */ s8 unk51;
    /* 0x52 */ u8 unk52;
    /* 0x53 */ u8 unk53;
    /* 0x54 */ u16 unk54;
    /* 0x56 */ u16 unk56;
    /* 0x58 */ u16 unk58;
    /* 0x5A */ u16 unk5A;
    /* 0x5C */ u16 unk5C;
    /* 0x5E */ u16 unk5E;
    /* 0x60 */ u32 unk60;
    /* 0x64 */ u16 unk64;
    /* 0x66 */ u16 unk66;
    /* 0x68 */ u16 unk68;
    /* 0x6A */ u16 unk6A;
    /* 0x6C */ u32 unk6C;
    /* 0x70 */ u16 unk70;
    /* 0x72 */ s16 unk72;
    /* 0x74 */ s32 unk74;
    /* 0x78 */ u32 unk78;
} BattleSlot; // Size: 0x7C

// 0xC-byte primitive template func_8007D6B8 allocates at D_800926C0/D_800926C4.
typedef struct {
    /* 0x0 */ u8 unk0[3];
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ u8 unk8[4];
} BattlePrimTemplate; // Size: 0xC

// 0xA-byte entry table func_8007D6B8 allocates at D_800926BC.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ s8 unk6;
    /* 0x7 */ s8 unk7;
    /* 0x8 */ u16 unk8;
} BattleSpriteEntry; // Size: 0xA

extern BattleSlot D_80092AD8[];
extern BattleAction D_80092A74[];
extern Color4 D_800926B8;
extern u8 D_800926B9;
extern u8 D_800926BA;
extern u32 D_800926A4;
extern u32 D_800926B0;
extern u32 D_800926B4;
extern u16 D_800926A8[];
extern u16 D_800926AA;
extern u16 D_800926AC;
extern u16 D_800926AE;
extern u16 D_80092694;
extern u16 D_800926A0;
extern s16 D_80091228[];
// CLUT x (0x80092698) and y (0x8009269C). func_8007B270 reads each of them as a
// short, this unit's other callers only want the low byte.
extern u8 D_80092698[];
extern u8 D_8009269C[];
extern s32 D_800928E8;
extern VECTOR D_80096FA8;

// 0xC-byte entry table BattleInitSpriteBuffers allocates at D_800926C8.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s8 unk8;
    /* 0x9 */ s8 unk9;
    /* 0xA */ u16 unkA;
} BattleSpriteSlot; // Size: 0xC

// 0x10-byte primitive template BattleInitSpriteBuffers allocates at D_800926CC/D_800926D0.
typedef struct {
    /* 0x0 */ u8 unk0[3];
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ u8 unk8[4];
    /* 0xC */ u16 unkC;
    /* 0xE */ u16 unkE;
} BattlePrimTemplate16; // Size: 0x10

extern u8 *D_800926BC;
extern u8 *D_800926C0;
extern u8 *D_800926C4;
extern u8 *D_800926C8;
extern u8 *D_800926CC;
extern u8 *D_800926D0;
extern u8 *D_800928DC;
extern u8 D_800928A0;

extern int rand(void);

extern void BattleDrawLineQuads(s32);
extern void func_8007D0B4(SVector *pVec);
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
        if (p->unk50 != 0) {
            if (p->unk51 == p->unk52) {
                p->unk50 = 0;
            } else {
                p->unk51++;
                if (p->unk53 == 0) {
                    p->unk66 += p->unk64;
                    p->unk56 += p->unk66;
                }
            }
        }
    }
    n = D_800926A4;
    v = D_800926B0;
    D_800926B0 = 0;
    D_800926A4 = n + 1;
    D_800926B4 = v;
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007BBA0);

void BattleEffectSetColor(Color4 *pColor) {
    D_800926B8 = *pColor;
}

void BattleEffectAddSlot(SVector *pVec, s32 action) {
    BattleSlot *pSlot = D_80092AD8;
    BattleAction *pAction;
    SlotMotion *pMotion;
    s32 i;

    for (i = 0; i < 0x3C; i++, pSlot++) {
        if (pSlot->unk50 == 0) {
            break;
        }
    }
    if (i == 0x3C) {
        return;
    }
    pSlot->unk50 = 1;
    pAction = &D_80092A74[action];
    pSlot->unk53 = 0;
    pSlot->unk51 = 0;
    pSlot->unk60 = (u32)pAction;
    pSlot->unk52 = pAction->unkA;
    pSlot->unk64 = pAction->unk10;
    pSlot->unk66 = 0;
    pSlot->unk54 = pVec->vx;
    pSlot->unk56 = pVec->vy;
    pSlot->unk58 = pVec->vz;
    pMotion = &pSlot->unk0;
    pMotion->unk16 = pAction->unkE;
    if (action == 2) {
        pMotion->unk7 &= 0xFE;
        pMotion->r = D_800926B8.r;
        pMotion->g = D_800926B9;
        pMotion->b = D_800926BA;
    } else {
        pMotion->unk7 |= 1;
    }
    pMotion->unkE = pAction->unkC;
    pSlot->unk28 = *pMotion;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C280);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C880);

/* Best attempt: objdiff 85.17%, same size as retail (548 bytes) and the same
   instruction multiset; the remaining 15% is gcc putting the D_80092698 /
   arg0*4 scaling, the t2/t1/t3 motion-copy address setup and the
   D_800928E8 / D_800926A4 / 0x70 / 0x72 pair a few slots away from retail.
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
        if (pEntry->unk50 != 0) {
            if (pEntry->unk78 == arg2) {
                if (pEntry->unk72 == D_800928E8) {
                    return;
                }
            }
        }
    }

    pSlot = D_80092AD8;
    for (i = 0; i < 0x3C; i++, pSlot++) {
        if (pSlot->unk50 == 0) {
            break;
        }
    }
    if (i == 0x3C) {
        return;
    }

    u = D_80092698[0] * 4 + 8;
    pMotion = &pSlot->unk0;
    u += arg0 * 4;
    pMotion->unk18[0xC] = u;
    pMotion->unk18[4] = u;
    pMotion->unk10[4] = u;
    pMotion->unk8[4] = u;
    v = D_8009269C[0];
    pMotion->unk18[0xD] = v;
    pMotion->unk18[5] = v;
    pMotion->unk10[5] = v;
    pMotion->unk8[5] = v;
    pMotion->unk16 = D_80092694;
    pMotion->unk7 &= 0xFE;
    pMotion->unkE = D_800926A0;
    pSlot->unk28 = *pMotion;

    pSlot->unk52 = 7;
    pSlot->unk53 = 1;
    pSlot->unk50 = 1;
    pSlot->unk51 = 0;
    pSlot->unk54 = pVec->vx;
    pSlot->unk56 = pVec->vy;
    pSlot->unk72 = D_800928E8;
    pSlot->unk78 = arg2;
    pSlot->unk6C = 0;
    pSlot->unk70 = D_800926A4;
    pSlot->unk58 = pVec->vz;
    pSlot->unk74 = D_80091228[arg3];

    id = D_800926A4 - 1;
    pEntry = D_80092AD8;
    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->unk50 != 0 && pEntry->unk78 == arg2 && pEntry != pSlot && pEntry->unk53 == 1 && pEntry->unk70 == id) {
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
   pair in ascending order, unk68 stored with unk66 instead of after unk6A, the
   u/v block before the unk64..68 stores, and slot->unk0 in place of a pMotion
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
        if (pEntry->unk50 != 0) {
            if (pEntry->unk70 == arg3) {
                if (pEntry->unk6C == D_800928E8) {
                    return;
                }
            }
        }
    }

    pSlot = D_80092AD8;
    for (i = 0; i < 0x3C; i++, pSlot++) {
        if (pSlot->unk50 == 0) {
            break;
        }
    }
    if (i == 0x3C) {
        return;
    }

    pSlot->unk52 = 7;
    pSlot->unk53 = 2;
    pSlot->unk50 = 1;
    pSlot->unk51 = 0;
    pSlot->unk54 = pPos->vx;
    pSlot->unk56 = pPos->vy;
    pSlot->unk58 = pPos->vz;
    pSlot->unk6C = D_800928E8;
    pSlot->unk64 = pVec->vx;
    pSlot->unk66 = pVec->vy;

    pMotion = &pSlot->unk0;
    u = D_80092698[0] * 4 + 8;
    u += arg0 * 4;
    pMotion->unk18[0xC] = u;
    pMotion->unk18[4] = u;
    pMotion->unk10[4] = u;
    pMotion->unk8[4] = u;
    v = D_8009269C[0];
    pMotion->unk18[0xD] = v;
    pMotion->unk18[5] = v;
    pMotion->unk10[5] = v;
    pMotion->unk8[5] = v;

    pSlot->unk70 = arg3;
    pSlot->unk60 = 0;
    pSlot->unk6A = D_800926A4;
    pSlot->unk68 = pVec->vz;
    pMotion->unk16 = D_80092694;
    pMotion->unk7 &= 0xFE;
    pMotion->unkE = D_800926A0;
    pSlot->unk28 = *pMotion;

    id = D_800926A4 - 1;
    pEntry = D_80092AD8;
    for (i = 0; i < 0x3C; i++, pEntry++) {
        if (pEntry->unk50 != 0 && pEntry->unk70 == arg3 && pEntry != pSlot && pEntry->unk53 == 2 && pEntry->unk6A == id) {
            pSlot->unk60 = (u32)pEntry;
        }
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CD44);

void BattleEffectQueueMatrixPrims(MATRIX *pMat, s32 arg1) {
    MATRIX transposed;

    *(VECTOR *)PSX_SCRATCH = D_80096FA8;
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
    *(VECTOR *)PSX_SCRATCH = D_80096FA8;
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
void func_8007D0B4(SVector *pVec) {
    pVec->vx = (s16)(pVec->vx - 0x18) + rand() % 0x30;
    pVec->vy = (s16)(pVec->vy - 0x18) + rand() % 0x30;
    pVec->vz = (s16)(pVec->vz - 0x18) + rand() % 0x30;
}
*/

void func_8007D0B4(SVector *pVec) {
    u8 offset = 0x18;

    pVec->vx = (pVec->vx - offset) + rand() % 0x30;
    pVec->vy = (pVec->vy - offset) + rand() % 0x30;
    pVec->vz = (pVec->vz - offset) + rand() % 0x30;
}

// s32, not void: retail keeps a nop in the switch guard's delay slot, which gcc
// 2.7.2 only emits for a non-void return type.
s32 func_8007D190(VECTOR *pVec, s32 action) {
    SVector vec;

    vec.vx = pVec->vx;
    vec.vy = pVec->vy;
    vec.vz = pVec->vz;
    switch (action) {
    case 8:
        func_8007D0B4(&vec);
        /* fallthrough */
    case 0:
        BattleEffectAddSlot(&vec, 0);
        break;
    case 9:
        func_8007D0B4(&vec);
        /* fallthrough */
    case 1:
        BattleEffectAddSlot(&vec, 1);
        break;
    case 10:
        func_8007D0B4(&vec);
        /* fallthrough */
    case 2:
        BattleEffectAddSlot(&vec, 2);
        break;
    case 11:
        func_8007D0B4(&vec);
        /* fallthrough */
    case 3:
        BattleEffectAddSlot(&vec, 3);
        break;
    case 12:
        func_8007D0B4(&vec);
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

    D_800926BC = HeapAlloc(0x9F6, 0);
    D_800926C0 = HeapAlloc(0xBF4, 0);
    D_800926C4 = HeapAlloc(0xBF4, 0);
    pEntry = (BattleSpriteEntry *)D_800926BC;
    for (i = 0; i < 0xFF; i++) {
        ((BattlePrimTemplate *)D_800926C0)[i].unk3 = 2;
        ((BattlePrimTemplate *)D_800926C0)[i].unk4 = 0x6880B0F0;
        ((BattlePrimTemplate *)D_800926C4)[i].unk3 = 2;
        ((BattlePrimTemplate *)D_800926C4)[i].unk4 = 0x6880B0F0;
        pEntry[i].unk4 = 0;
        pEntry[i].unk2 = 0;
        pEntry[i].unk0 = 0;
        pEntry[i].unk6 = 0;
        pEntry[i].unk4 = 0;
        pEntry[i].unk0 = 0;
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D7A8);

/* Best attempt: objdiff 91.67%. Loop head, break-on-zero, the two rand()
   modulo-64 computations ((src - 0x20) + rand() % 64, i.e. the -0x20 attached
   to the source value, not folded after the add) and the table lookup all
   match. Only the induction anchor differs: retail keeps the second pointer
   at entry+6 (lb/sb at -0/-1/1), gcc anchors it at entry+7 (every entry
   displacement is off by one) and merges (s8)vz << 7 into sll 24/sra 17
   where retail does sra 24 then sll 7. */

/*
void func_8007D7A8(VECTOR *pSrc, s32 count) {
    BattleSpriteEntry *pEntry = (BattleSpriteEntry *)D_800926BC;
    u16 vx;
    u16 vz;
    u16 v;
    s32 i;

    for (i = 0; i < 0xFB; i++, pEntry++) {
        if (count == 0) {
            break;
        }
        if (pEntry->unk6 == 0) {
            vx = pSrc->vx - 32 + rand() % 64;
            pEntry->unk0 = vx;
            vz = pSrc->vz - 32 + rand() % 64;
            pEntry->unk4 = vz;
            v = *(u16 *)(D_800928DC + ((((s8)vz) << 7) + (s8)vx) * 4);
            pEntry->unk8 = v;
            pEntry->unk2 = v;
            pEntry->unk6 = 0x14;
            pEntry->unk7 = -(rand() % 10 + 10);
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

    D_800926C8 = HeapAlloc(0x1950, 0);
    pTemplateBuf = HeapAlloc(0x21C0, 0);
    D_800926CC = pTemplateBuf;
    D_800926D0 = HeapAlloc(0x21C0, 0);
    pSlot = (BattleSpriteSlot *)D_800926C8;
    pTemplate = (BattlePrimTemplate16 *)pTemplateBuf;
    for (i = 0; i < 0x21C; i++, pSlot++, pTemplate++) {
        pTemplate->unk3 = 3;
        pTemplate->unk4 = 0x60FFD0A0;
        pTemplate->unkC = rand() % 3 + 2;
        pTemplate->unkE = rand() % 3 + 2;
        pSlot->unk4 = 0;
        pSlot->unk2 = 0;
        pSlot->unk0 = 0;
        pSlot->unk6 = 0;
        pSlot->unk9 = 0;
        pSlot->unkA = 0;
        pSlot->unk8 = 0;
    }
    func_800732AC(D_800926D0, D_800926CC, 0x21C0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007DC74);
