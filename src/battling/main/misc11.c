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

typedef struct {
    /* 0x000 */ u32 unk0[0xC4];
} BattlingWork; // Size: 0x310

extern BattlingWork g_BattleDrawEnvPair[2];

extern s32 g_BattleAppendEntryCount;
extern u32 g_NearColor;
extern u8 D_80091834[];
extern u8 D_800918B4[];
extern u8 g_BattleSpanRenderParams[];
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

extern u16 g_BattleQuadTexPage;
extern u16 g_BattleQuadClut;
extern u8 g_BattleQuadTexU;
extern u8 g_BattleQuadTexV;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087068);

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
    func_800732AC((void *)0x1F800120, g_BattleSpanRenderParams, 0x30);
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
    p->unkE = g_BattleQuadClut;
    p->unk16 = g_BattleQuadTexPage;
    p->unkC = g_BattleQuadTexU | (g_BattleQuadTexV << 8);
    p->unk14 = (g_BattleQuadTexU + 0x3F) | (g_BattleQuadTexV << 8);
    p->unk1C = g_BattleQuadTexU | ((g_BattleQuadTexV + 0x3F) << 8);
    p->unk24 = (g_BattleQuadTexU + 0x3F) | ((g_BattleQuadTexV + 0x3F) << 8);
    arg0->packet[1] = arg0->packet[0];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087B74);

void BattleAppendEntry(s32 *arg0) {
    s32 i = g_BattleAppendEntryCount;

    if (i < 0x1F) {
        BattlingEntry68 *pBase = D_8009A988;
        BattlingEntry68 *p = pBase + i;
        p->unk0 = arg0[0];
        p->unk2 = arg0[1];
        p->unk4 = arg0[2];
        g_BattleAppendEntryCount = i + 1;
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087EA0);

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
