#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

extern u8 D_8006F978[];
extern u8 *D_800927CC;
extern u8 *D_800927D0;
extern u8 D_80095968[];
extern u8 D_800928A0;
extern VECTOR D_80096FA8;
extern void func_800316C0(void *pOt, void *pPrim);

// An 8-byte, byte-aligned chunk: assigning one copies 8 bytes with lwl/lwr.
typedef struct {
    u8 unk[8];
} Bytes8;

typedef struct {
    /* 0x00 */ u8 pad0[0x4];
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 unk5;
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 pad7[0xD];
    /* 0x14 */ u8 unk14;
    /* 0x15 */ u8 unk15;
    /* 0x16 */ u8 unk16;
    /* 0x17 */ u8 pad17[0x9];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ s16 unk22;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ u8 pad26[0x2];
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ s16 unk2C;
    /* 0x2E */ u8 pad2E[0x2];
    /* 0x30 */ s16 unk30;
    /* 0x32 */ u8 pad32[0x2];
} BattlingSlot; // Size: 0x34

extern BattlingSlot D_80095938[];

extern void *BattleAllocLineSlot();
extern void func_80085EC8(void *);

typedef struct {
    /* 0x000 */ u32 unk0[0xC4];
} BattlingWork; // Size: 0x310

extern BattlingWork D_8009A2F8[2];

extern s32 D_800928F8;
extern u32 D_80059598;
extern u8 D_80091834[];
extern u8 D_800918B4[];
extern u8 D_80091934[];
extern u8 *D_80092854;
extern u8 *D_80092858;
extern long MathGetLargestVectorComponent(long, long, long);
extern void func_800732AC(void *, void *, s32);
extern void func_80072D18(s32, s32, s32);

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 pad6[0x62];
} BattlingEntry68; // Size: 0x68

extern BattlingEntry68 D_8009A988[];

typedef union {
    /* 0x0 */ u32 word;
    /* 0x0 */ u8 bytes[4];
} BattlingPacketWord;

typedef struct {
    /* 0x00 */ u8 pad0[0x3];
    /* 0x03 */ u8 unk3;
    /* 0x04 */ BattlingPacketWord unk4;
    /* 0x08 */ u8 pad8[0x4];
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u8 pad10[0x4];
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u8 pad18[0x4];
    /* 0x1C */ u16 unk1C;
    /* 0x1E */ u8 pad1E[0x6];
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u8 pad26[0x2];
} BattlingPacket; // Size: 0x28

typedef struct {
    /* 0x0000 */ u8 pad0[0x1604];
    /* 0x1604 */ BattlingPacket packet[2];
} BattlingPacketOwner; // Size: 0x1654

extern u16 D_800927D4;
extern u16 D_800927D8;
extern u8 D_800927DC;
extern u8 D_800927E0;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087068);

void BattleInitDrawEnvPair(void) {
    func_80085EC8(D_8009A2F8);
    D_8009A2F8[1] = D_8009A2F8[0];
}

void BattleClearSpanTables(void) {
    s32 i;

    for (i = 0; i < 0x80; i++) {
        D_800927CC[i] = 0;
        D_800927D0[i] = 0xFF;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087698);
/* Best 81.23%: two clusters left. (1) gcc puts the live interpolated value in
   a0 and the table byte in v1; the original has them the other way round
   (v1 / a0). (2) the original's loop tail is i++; slt; bnez; addu(cur += step)
   in the branch delay slot, gcc emits addu before the slt and a nop in the
   slot. Tried: for-increment order i++,cur += step and cur += step,i++;
   t declared before v. Semantics: interpolate a shaded span into the two
   buffers D_800927D0 / D_800927CC, clamped by the tables D_80091834 /
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
        p = D_800927D0 + i;
        if (v < *p) {
            t = D_80091834[i];
            if (v < t) {
                v = t;
            }
            *p = v;
        }
        p = D_800927CC + i;
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
    D_80059598 = (D_80059598 & 0xFFFFFF) | 0x24000000;
    gte_ldrgb(&D_80059598);
    func_800732AC((void *)0x1F800120, D_80091934, 0x30);
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

    D_800927CC = (u8 *)0x1F800000;
    D_800927D0 = (u8 *)0x1F800080;
    D_80092854 = HeapAlloc(0xE100, 0);
    D_80092858 = HeapAlloc(0xE100, 0);
    p = D_80092854 + 7;
    for (i = 0; i < 0x708; i++) {
        p[-4] = 7;
        p[0] = 0x24;
        p += 0x20;
    }
    func_800732AC(D_80092858, D_80092854, 0xE100);
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800878DC);

void BattleInitSemiTransQuadPacket(BattlingPacketOwner *arg0) {
    BattlingPacket *p = arg0->packet;

    p->unk4.word = 0x2C101010;
    p->unk3 = 9;
    p->unk4.bytes[3] |= 2;
    p->unkE = D_800927D8;
    p->unk16 = D_800927D4;
    p->unkC = D_800927DC | (D_800927E0 << 8);
    p->unk14 = (D_800927DC + 0x3F) | (D_800927E0 << 8);
    p->unk1C = D_800927DC | ((D_800927E0 + 0x3F) << 8);
    p->unk24 = (D_800927DC + 0x3F) | ((D_800927E0 + 0x3F) << 8);
    arg0->packet[1] = arg0->packet[0];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087B74);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087E38);
/* Best 98.08%: the only difference is which register gets the sum. The original
   keeps the base in a0 and the result in v0 (addu v0,v0,a0); gcc reuses the dead
   base register (addu a0,v0,a0) and the i+1 temp then moves to v0. Tried: p += i,
   p = p + i, p = &p[i], p = base + i, u8* + i*0x68, n = i + 1, decls outside the
   if (all 98.08 or worse), &D_8009A988[i] and i + D_8009A988 (84.04),
   and the indexed D_8009A988[i].unkN form (66.81: it re-forms the base with
   $at instead of keeping it in a0).
void func_80087E38(s32 *arg0) {
    s32 i = D_800928F8;

    if (i < 0x1F) {
        BattlingEntry68 *p = D_8009A988;

        p += i;
        p->unk0 = arg0[0];
        p->unk2 = arg0[1];
        p->unk4 = arg0[2];
        D_800928F8 = i + 1;
    }
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087EA0);

void *BattleAllocLineSlot(s32 *arg0, s32 *arg1, s32 arg2) {
    s32 i;

    for (i = 0; i < 0x64; i++) {
        BattlingSlot *p = &D_80095938[i];

        if (p->unk30 == 0) {
            p->unk30 = 1;
            p->unk20 = arg0[0];
            p->unk22 = arg0[1];
            p->unk24 = arg0[2];
            p->unk28 = arg1[0];
            p->unk2A = arg1[1];
            p->unk2C = arg1[2];
            p->unk4 = (arg2 & 2) * 0x7F;
            p->unk5 = (arg2 & 4) * 0x3F;
            p->unk6 = (arg2 & 1) * 0xFF;
            p->unk14 = (arg2 & 2) * 0x7F;
            p->unk15 = (arg2 & 4) * 0x3F;
            p->unk16 = (arg2 & 1) * 0xFF;
            return p;
        }
    }
    return NULL;
}
void BattleCreateLineSlot(s32 arg0, s32 *arg1, s32 arg2, s16 arg3) {
    u8 *p = BattleAllocLineSlot(arg0, arg1, arg2);

    if (p != NULL) {
        *(s16 *)(p + 0x30) = arg3;
    }
}

void BattleClearLineSlots(void) {
    s32 i;

    for (i = 0x141C; i >= 0; i -= 0x34) {
        *(s16 *)(D_80095968 + i) = 0;
    }
}

void BattleDrawLineSlots(void *arg0) {
    Bytes8 work[2];
    u8 *pSlot;
    s32 i;
    s32 off;

    for (i = 0; i < 0x64; i++) {
        off = i * 0x34;
        if (*(s16 *)(D_80095968 + off) != 0) {
            pSlot = (u8 *)D_80095938 + off;
            *(u16 *)(pSlot + 0x30) -= 1;
            work[0] = *(Bytes8 *)(pSlot + 0x20);
            work[1] = *(Bytes8 *)(pSlot + 0x28);
            ((u16 *)work)[0] -= D_80096FA8.vx;
            ((u16 *)work)[1] -= D_80096FA8.vy;
            ((u16 *)work)[2] -= D_80096FA8.vz;
            ((u16 *)work)[4] -= D_80096FA8.vx;
            ((u16 *)work)[5] -= D_80096FA8.vy;
            ((u16 *)work)[6] -= D_80096FA8.vz;
            gte_ldv01(work, (u8 *)work + 8);
            gte_rtpt();
            gte_stsxy01((u8 *)pSlot + D_800928A0 * 0x10 + 8, (u8 *)pSlot + D_800928A0 * 0x10 + 0xC);
            *(u8 *)((u8 *)pSlot + D_800928A0 * 0x10 + 3) = 3;
            *(u8 *)((u8 *)pSlot + D_800928A0 * 0x10 + 7) = 0x40;
            func_800316C0(arg0, (u8 *)pSlot + D_800928A0 * 0x10);
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
    D_8006F978[arg0 >> 3] |= bit;
}

s32 BattleTestSlotBit(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    return D_8006F978[arg0 >> 3] & bit;
}

void BattleClearSlotBit(s32 arg0) {
    s32 mask = 1;

    mask <<= arg0 & 7;
    D_8006F978[arg0 >> 3] &= ~mask;
}
