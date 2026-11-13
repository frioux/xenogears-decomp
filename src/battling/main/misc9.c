#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

extern s32 D_80092790;
extern s32 D_800927AC;
extern s32* D_800928DC;
extern u8 D_800928A0;

typedef struct {
    /* 0x0 */ u8 unk0[0x8];
} Unk8; // Size: 0x8

typedef struct {
    /* 0x0 */ s32 unk0[4];
} Unk16; // Size: 0x10

extern Unk8 D_80092768;
extern s32 D_8009871C;
extern s32 D_8009867C;
extern s32 D_80098720;
extern s32 D_80098724;
extern s32 D_80092770;
extern s32 D_80092774;
extern u8 D_8009287C;
extern s32 D_8009290C;

extern s32 D_80092794;
extern s32 D_800927B0;
extern s32 D_8009294C;
extern s16 D_80097102;
extern s16 D_8009881E;

extern int rand(void);

extern s32 D_80092944;
extern char D_800701B0[];
extern void func_8007EBE0(char* p);

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
} Poly9; // Size: unknown

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ Poly9* unk4;
} PolyOwner;

typedef struct {
    /* 0x0000 */ s32 pos[3];
    /* 0x000C */ u8 pad0C[0x15C8];
    /* 0x15D4 */ u8 unk15D4;
    /* 0x15D5 */ u8 unk15D5;
    /* 0x15D6 */ u8 unk15D6;
} BattlerData;

typedef struct {
    /* 0x000 */ s32 unk0;
    /* 0x004 */ s32 unk4;
    /* 0x008 */ s32 unk8;
    /* 0x00C */ u8 pad0C[0xC8];
    /* 0x0D4 */ u32 unkD4;
    /* 0x0D8 */ BattlerData* unkD8;
    /* 0x0DC */ u8 padDC[0x83C];
    /* 0x918 */ u8 unk918;
} Battler9;

typedef struct {
    /* 0x0 */ u8 pad0[0x8];
    /* 0x8 */ PolyOwner* unk8[3];
} PolyTable;

extern u8* D_8009288C;

extern s32 func_8008AC0C();
extern void func_80030A30(s32 index, Poly9* pPoly);
extern s32 func_80083310();
extern void func_8007A21C(s32);
extern void func_8007AC3C(void);
extern void func_800725B0(s16* p);
extern void func_80081E6C(void);
extern void func_80080D20(void* p);
extern void func_8008AE1C(u8* p);
extern void func_80086E24(void);
extern void func_80087698(s32 x0, s32 y0, s32 x1, s32 y1);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082178);

/* best 99.47%: 392 bytes, instruction multiset identical; only the
   bx/x pair is swapped (bx in s6 here, s5 in the original). Same
   allocator tie as func_80082300 below. */
/*
void func_80082178(s32 arg0, s32 arg1, s32 arg2) {
    s32 x0;
    s32 y0;
    s32 x;
    s32 y;
    s32 bx;
    s32 by;
    s32 ang;
    s32 px;
    s32 py;
    s32 i;

    bx = arg0 >> 8;
    by = arg1 >> 8;
    x = bx + ((rcos(arg2 + 0x280) * 10) >> 12);
    x0 = x;
    y = by + ((rsin(arg2 + 0x280) * 10) >> 12);
    y0 = y;
    ang = arg2 + 0x580;
    for (i = 0; i < 6; i++) {
        px = bx + ((rcos(ang) * 24) >> 12);
        py = by + ((rsin(ang) * 24) >> 12);
        func_80087698(x, y, px, py);
        x = px;
        y = py;
        ang += 0x100;
    }
    px = bx + ((rcos(arg2 - 0x280) * 10) >> 12);
    py = by + ((rsin(arg2 - 0x280) * 10) >> 12);
    func_80087698(x, y, px, py);
    func_80087698(x0, y0, px, py);
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082300);

/* best 99.40%: instruction stream identical, including both spill slots;
   only the s5/s6 pair of bx and x is swapped (an allocator tie-break, six
   source shapes tried: declaration order twice, x0/x assignment order, an
   s32 start[2] array instead of the x0/y0 spill pair).
void func_80082300(s32 arg0, s32 arg1, s32 arg2) {
    s32 x0;
    s32 y0;
    s32 x;
    s32 y;
    s32 bx;
    s32 by;
    s32 px;
    s32 py;
    s32 ang;
    s32 i;

    bx = arg0 >> 8;
    by = arg1 >> 8;
    x = bx + ((rcos(arg2 + 0x100) << 4) >> 12);
    x0 = x;
    y = by + ((rsin(arg2 + 0x100) << 4) >> 12);
    y0 = y;
    ang = arg2 + 0x78A;
    for (i = 0; i < 3; i++) {
        px = bx + ((rcos(ang) << 5) >> 12);
        py = by + ((rsin(ang) << 5) >> 12);
        func_80087698(x, y, px, py);
        x = px;
        y = py;
        ang += 0x75;
    }
    px = bx + ((rcos(arg2 - 0x100) << 4) >> 12);
    py = by + ((rsin(arg2 - 0x100) << 4) >> 12);
    func_80087698(x, y, px, py);
    func_80087698(x0, y0, px, py);
}
*/

void func_80082458(Unk8* p) {
    *p = D_80092768;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082488);

s16 func_80082880(u8* p) {
    s32 tmp[12];
    s32 a;
    s32 b;
    s32 lo;
    s32 hi;

    a = *(u16*)p << 16;
    b = *(u16*)(p + 4) << 16;
    lo = a >> 24;
    hi = (b >> 24) << 7;

    return *(s16*)&D_800928DC[hi + lo];
}

u32 func_800828C4(s32* p) {
    s32 a;
    s32 b;

    a = p[0] >> 8;
    b = p[2] >> 8;
    return (u32)D_800928DC[(b << 7) + a];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800828F8);

/* best 97.57%: frame, local layout and the whole matrix block line up. Only
   residual: gcc swaps the two SVECTOR normal load registers (n.vx in v1 here,
   a2 in the original) and the two following halfword store order. Field-name
   reads (n.vx / n.vz) beat ((u16*)&n)[i]: 94.16 -> 97.57. Splitting the loop
   update into "v.vz += 8; v.vz -= ofs;" also helped.
void func_800828F8(VECTOR* p0, VECTOR* p1, s32 radius) {
    VECTOR v;
    VECTOR v2;
    VECTOR vSq;
    MATRIX m;
    MATRIX mT;
    SVECTOR n;
    s32 r;
    s32 ofs;

    v.vx = p0->vx + p1->vx - 0x3F80;
    v.vz = p0->vz + p1->vz - 0x3F80;
    Square0(&v, &vSq);
    if (radius < SquareRoot0(vSq.vx + vSq.vz)) {
        VectorNormalS(&v, &n);
        m.m[2][1] = 0;
        m.m[1][2] = 0;
        m.m[1][0] = 0;
        m.m[0][1] = 0;
        m.m[1][1] = 0x1000;
        m.m[0][0] = n.vz;
        m.m[0][2] = -n.vx;
        m.m[2][0] = n.vx;
        m.m[2][2] = n.vz;
        ApplyMatrixLV(&m, p1, &v);
        TransposeMatrix(&m, &mT);
        SetRotMatrix(&mT);
        v.vz = 0;
        while (1) {
            ApplyRotMatrixLV(&v, p1);
            v2.vx = p0->vx + p1->vx - 0x3F80;
            v2.vz = p0->vz + p1->vz - 0x3F80;
            Square0(&v2, &vSq);
            r = SquareRoot0(vSq.vx + vSq.vz);
            if (radius >= r) {
                break;
            }
            ofs = r - radius;
            v.vz += 8;
            v.vz -= ofs;
        }
    }
}
*/


extern u8 D_800928B4;
extern u8 D_8009178C[];
extern s32 D_8009291C;
extern s32 D_80092910;
extern s32 D_80092908;
extern u8 D_80095584;
extern u8 D_80095585;
extern u8 D_80095586;
extern u16 D_8009558C;
extern u8 D_8009558E;
extern u16 D_80095594;
extern u8 D_80095596;
extern u16 D_8009559C;
extern u8 D_8009559E;
extern u8 D_800955A8;
extern u8 D_800955A9;
extern u8 D_800955AA;
extern u16 D_800955B0;
extern u8 D_800955B2;
extern u16 D_800955B8;
extern u8 D_800955BA;
extern u16 D_800955C0;
extern u8 D_800955C2;
extern u8 D_8009A1C4;
extern u8 D_8009A1C5;
extern u8 D_8009A1C6;
extern u8 D_8009A2BC;
extern u8 D_8009A2BD;
extern u8 D_8009A2BE;
extern u32 D_80059598;

void func_80082A70(void) {
    u8* p;
    s32 farR;
    s32 farG;
    s32 farB;
    s32 nearR;
    s32 nearG;
    s32 nearB;
    s32 nearRG;
    s32 farRG;

    p = &D_8009178C[D_800928B4 * 17];
    D_8009288C = p;
    nearR = p[0];
    nearG = p[1];
    nearB = p[2];
    D_8009291C = p[4];
    D_80092910 = p[5];
    D_80092908 = p[6];
    farR = p[8];
    farG = p[9];
    farB = p[10];
    SetNearColor(p[0xC], p[0xD], p[0xE]);
    SetFarColor(farR, farG, farB);
    D_80095585 = nearG;
    D_800955A9 = nearG;
    nearRG = nearR | (nearG << 8);
    D_80095584 = nearR;
    D_800955A8 = nearR;
    farRG = farR | (farG << 8);
    D_80095586 = nearB;
    D_800955AA = nearB;
    D_8009558C = nearRG;
    D_8009558E = nearB;
    D_800955B0 = nearRG;
    D_800955B2 = nearB;
    D_80095594 = farRG;
    D_80095596 = farB;
    D_800955B8 = farRG;
    D_800955BA = farB;
    D_8009559C = farRG;
    D_8009559E = farB;
    D_800955C0 = farRG;
    D_800955C2 = farB;
    D_8009A1C4 = farR;
    D_8009A1C5 = farG;
    D_8009A1C6 = farB;
    D_8009A2BC = farR;
    D_8009A2BD = farG;
    D_8009A2BE = farB;
    SetFogNearFar(0x800, 0x1800, 0xC0);
    D_80059598 = (D_80059598 & 0xFFFFFF) | 0x28000000;
    gte_ldrgb(&D_80059598);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082C4C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082E60);

void func_800831C8(void) {
    s32 ang;
    s32 scale;

    scale = (rand() & 0x1FFF) + 0x800;
    ang = rand() % 0x600 + 0x500;
    D_8009871C = ((rcos(ang) * scale) >> 12) + 0x4000;
    D_80098724 = ((rsin(ang) * scale) >> 12) + 0x4000;
    D_80098720 = -((rand() & 0x7FF) + 0x400);
    D_80092770 = 0x100;
    D_80092774 = 0x40;
    D_8009287C = 0x40;
    D_8009290C = 0x400;
}

void func_800832C0(s32 flags) {
    if (flags & 0x8000) {
        D_800927AC += 0x20;
    }
    if (flags & 0x2000) {
        D_800927AC -= 0x20;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083310);

void func_8008369C(void) {
    D_800927AC = rand();
    D_800927B0 = rand() % 12 + 4;
    if (rand() & 1) {
        D_800927B0 = -D_800927B0;
    }
    func_80083310(0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083738);

void func_80083B54(Unk16* p0, Unk16* p1) {
    *p0 = *(Unk16*)&D_8009871C;
    *p1 = *(Unk16*)&D_8009867C;
}

void func_80083BB4(s32 arg) {
    RECT rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = arg ? 0x280 : 0x140;
    rect.h = 0x1E0;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
}

void func_80083C0C(s32 mode) {
    s16* pA;
    s16* pB;
    s16 vA;
    s16 vB;

    D_80092794 = mode;
    switch (mode) {
    case 3:
        func_80081E6C();
        break;
    case 4:
        func_8007A21C(D_8009294C);
        break;
    case 8:
        func_8007AC3C();
        break;
    case 6:
        pA = &D_8009881E;
        pB = &D_80097102;
        vA = *pA;
        vB = *pB;
        func_800725B0(vA < vB ? pB - 0x79 : pA - 0x79);
        break;
    }
}

s32 func_80083CD8(void) {
    return D_80092790;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083CE8);

/* best 85.00%: instruction stream is insn-for-insn identical; gcc puts n in
   t0 and the 0x88888889 magic in a0 where the original has n in a0, the magic
   in a1 and an extra `move a2,t0` for n/1800, and gcc hoists the format
   address. Splitting the three values into locals gives byte-identical C
   output (also 85.00%).
void func_80083CE8(void) {
    char buf[0x20];
    s32 n;

    n = D_80092944;
    Sprintf(buf, D_800701B0, n / 1800, (n % 1800) / 30, (n % 30) * 99 / 30);
    func_8007EBE0(buf);
}
*/

void func_80083DCC(PolyTable* pTable, Battler9* pActor, s32 idx) {
    PolyOwner* pOwner;
    u8 alpha;
    u8 next;
    s32 a;
    s32 tmp[8];

    alpha = pActor->unk918;
    a = alpha;
    pOwner = pTable->unk8[idx];
    if (a != 0) {
        next = alpha - 0x18;
        pActor->unk918 = next;
        if (a < next) {
            pActor->unk918 = 0;
        }
    }
    if (pActor->unkD4 & 0x20) {
        pOwner->unk4->unkC = (pActor->unkD8->unk15D4 * a) / 16;
        pOwner->unk4->unkE = (pActor->unkD8->unk15D5 * a) / 16;
        pOwner->unk4->unk10 = (pActor->unkD8->unk15D6 * a) / 16;
    } else {
        pOwner->unk4->unkC = a << 4;
        pOwner->unk4->unkE = a << 3;
        pOwner->unk4->unk10 = 0;
    }
    if (D_8009288C[0x10] != 0) {
        pOwner->unk4->unkC /= 2;
        pOwner->unk4->unkE /= 2;
        pOwner->unk4->unk10 /= 2;
    }
    pOwner->unk4->unk0 = pActor->unk0;
    pOwner->unk4->unk4 = pActor->unk4;
    pOwner->unk4->unk8 = pActor->unk8;
    pOwner->unk4->unk0 -= pActor->unkD8->pos[0];
    pOwner->unk4->unk4 -= pActor->unkD8->pos[1];
    pOwner->unk4->unk8 -= pActor->unkD8->pos[2];
    func_80030A30(idx, pOwner->unk4);
    pOwner = pTable->unk8[2];
    pOwner->unk4->unkC = pOwner->unk4->unkE = pOwner->unk4->unk10 = 0;
    pOwner->unk4->unk0 = pActor->unk0;
    pOwner->unk4->unk4 = pActor->unk4;
    pOwner->unk4->unk8 = pActor->unk8;
    pOwner->unk4->unk0 -= D_8009871C;
    pOwner->unk4->unk4 -= D_80098720;
    pOwner->unk4->unk8 -= D_80098724;
    func_80030A30(2, pOwner->unk4);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800840CC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800846A0);

s32 func_800849E0(u8* p) {
    u8* p2;
    void* p3;

    p2 = *(u8**)(p + 0x284);
    func_8008AC0C(p2);
    p3 = *(void**)(p2 + D_800928A0 * 4 + 4);
    func_80080D20(p3 + 4);
    func_8008AE1C(p2);
    func_80086E24();
    return 0;
}

s32 func_80084A40(u8* p) {
    return func_8008AC0C(*(s32*)(p + 0x284));
}
