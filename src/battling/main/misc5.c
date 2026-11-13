#include "common.h"
#include "psyq/libgte.h"
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
    /* 0x68 */ u8 unk68[0x14];
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
extern u16 D_800926A0;
extern s16 D_80092698;
extern s16 D_8009269C;
extern VECTOR D_80096FA8;

extern u8 *D_800926BC;
extern u8 *D_800926C0;
extern u8 *D_800926C4;
extern u8 *D_800928DC;

extern int rand(void);

extern void func_8007E3CC(s32);
extern void func_8007D334(s32, s32, s32);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007B270);

/* Best attempt: objdiff 95.11%. Only the load order of the first three bytes
   differs (the original computes b = p[2] - 0x80 between the two loads); the
   instruction multiset and registers are identical. Re-confirmed: writing the
   three loads in the same order (r, b, g) still gives gcc `lbu p[0]; addiu r;
   lbu p[2]; lbu p[1];` while retail has `lbu p[0]; lbu p[2]; addiu r; lbu
   p[1];` - an allocator tie, not a source shape. */
/*
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007B388);

void func_8007BACC(void) {
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

void func_8007BB7C(void) {
    u8 *p = (u8 *)D_80092AD8;
    s32 i = 0x3B;

    do {
        p[0x50] = 0;
        i--;
        p += 0x7C;
    } while (i >= 0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007BBA0);

void func_8007C100(Color4 *pColor) {
    D_800926B8 = *pColor;
}

void func_8007C124(SVector *pVec, s32 action) {
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CAA4);

s32 func_8007CD14(s32 a0, s32 a1, s32 a2, s32 a3) {
    return (a3 & 0x7F) | ((a0 << 7) & 0x80) | (a1 << 24) | ((a2 << 8) & 0xFFFF00);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CD44);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CF78);

void func_8007D068(s32 arg0) {
    *(VECTOR *)PSX_SCRATCH = D_80096FA8;
    func_8007E3CC(arg0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D0B4);

/* Best attempt: objdiff 76.82%. The instruction multiset and the registers are
   identical; the only difference is which side the -0x18 lands on. Original:
   `lhu; addiu -0x18; ... addu vx,r` i.e. (vx - 0x18) + rand() % 0x30. Every
   source shape tried folds to (vx + r) - 0x18, which schedules the addiu after
   the modulo: `vx - 0x18 + rand() % 0x30` (73.11%, folds to vx + (r - 0x18)),
   `vx + rand() % 0x30 - 0x18` (76.82%), `vx -= 0x18 - rand() % 0x30`, and a
   `u16 v = pVec->vx;` temp (65.42%, keeps -0x18 in a saved reg). Same 76.82%
   under gcc-2.6.0 and gcc-2.7.2-cdk, so it is not a compiler-version fold. */
/*
void func_8007D0B4(SVector *pVec) {
    pVec->vx = pVec->vx + rand() % 0x30 - 0x18;
    pVec->vy = pVec->vy + rand() % 0x30 - 0x18;
    pVec->vz = pVec->vz + rand() % 0x30 - 0x18;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D190);

s32 func_8007D25C(s32 arg0) {
    if (arg0 < 0x10) {
        return 0;
    }
    return arg0 < 0x20;
}

void func_8007D274(VECTOR *pSrc, VECTOR *pDst) {
    pDst->vx = pSrc->vx + rand() % 64 - 32;
    pDst->vy = pSrc->vy + rand() % 64 - 32;
    pDst->vz = pSrc->vz + rand() % 64 - 32;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D334);

void func_8007D65C(s32 arg0, s32 arg1, s32 arg2) {
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007DB28);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007DC74);
