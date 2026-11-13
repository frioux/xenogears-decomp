#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

extern u8 D_8006F978[];
extern u8 *D_800927CC;
extern u8 *D_800927D0;
extern u8 D_80095968[];

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

extern void *func_8008820C();
extern void func_80085EC8(void *);

typedef struct {
    /* 0x000 */ u32 unk0[0xC4];
} BattlingWork; // Size: 0x310

extern BattlingWork D_8009A2F8[2];

extern s32 D_800928F8;
extern u32 D_80059598;
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

void func_800875EC(void) {
    func_80085EC8(D_8009A2F8);
    D_8009A2F8[1] = D_8009A2F8[0];
}

void func_80087650(void) {
    s32 i;

    for (i = 0; i < 0x80; i++) {
        D_800927CC[i] = 0;
        D_800927D0[i] = 0xFF;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087698);

void func_8008779C(s32 arg0, s32 arg1, s32 arg2) {
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

void func_80087AB0(BattlingPacketOwner *arg0) {
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

void func_80087E38(s32 *arg0) {
    s32 i = D_800928F8;

    if (i < 0x1F) {
        BattlingEntry68 *p = &D_8009A988[i];

        p->unk0 = arg0[0];
        p->unk2 = arg0[1];
        p->unk4 = arg0[2];
        D_800928F8 = i + 1;
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087EA0);

void *func_8008820C(s32 *arg0, s32 *arg1, s32 arg2) {
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
void func_800882D4(s32 arg0, s32 *arg1, s32 arg2, s16 arg3) {
    u8 *p = func_8008820C(arg0, arg1, arg2);

    if (p != NULL) {
        *(s16 *)(p + 0x30) = arg3;
    }
}

void func_80088308(void) {
    s32 i;

    for (i = 0x141C; i >= 0; i -= 0x34) {
        *(s16 *)(D_80095968 + i) = 0;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008832C);

void func_800884E0(VECTOR *arg0, VECTOR *arg1) {
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

void func_8008859C(VECTOR *arg0, SVECTOR *arg1) {
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

void func_80088658(SVECTOR *arg0, SVECTOR *arg1) {
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

s32 func_800886FC(s32 *arg0) {
    long out[3];

    gte_ldlvl(arg0);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[1] + out[2]);
}

s32 func_80088754(s32 *arg0) {
    long out[3];

    gte_ldlvl(arg0);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[2]);
}

s32 func_800887A4(s32 *arg0, s32 *arg1) {
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

s32 func_80088838(s32 *arg0, s32 *arg1) {
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

void func_800888B0(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    D_8006F978[arg0 >> 3] |= bit;
}

s32 func_800888E4(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    return D_8006F978[arg0 >> 3] & bit;
}

void func_80088908(s32 arg0) {
    s32 mask = 1;

    mask <<= arg0 & 7;
    D_8006F978[arg0 >> 3] &= ~mask;
}
