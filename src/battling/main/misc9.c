#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
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

extern s32 func_8008AC0C();
extern s32 func_80083310();
extern void func_8007A21C(s32);
extern void func_8007AC3C(void);
extern void func_800725B0(s16* p);
extern void func_80081E6C(void);
extern void func_80080D20(void* p);
extern void func_8008AE1C(u8* p);
extern void func_80086E24(void);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082178);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082300);

void func_80082458(Unk8* p) {
    *p = D_80092768;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082488);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082880);

/* best 88.24%: identical instruction set (the unused s32 tmp[12] is what
   reproduces the original's 0x30 frame); only the sll/sra pairing of the two
   sign extensions is interleaved differently.
s16 func_80082880(u8* p) {
    s32 tmp[12];
    s32 a;
    s32 b;

    a = ((s32)*(u16*)p << 16) >> 24;
    b = ((s32)*(u16*)(p + 4) << 16) >> 24;

    return *(s16*)&D_800928DC[(b << 7) + a];
}
*/

u32 func_800828C4(s32* p) {
    s32 a;
    s32 b;

    a = p[0] >> 8;
    b = p[2] >> 8;
    return (u32)D_800928DC[(b << 7) + a];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800828F8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082A70);

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083C0C);

/* best 87.02%: switch order and layout match; gcc re-forms the two
   D_8009881E/D_80097102 addresses per branch instead of keeping them in
   a1/a0 across the loads, costing two lui/addiu pairs.
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
*/

s32 func_80083CD8(void) {
    return D_80092790;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083CE8);

/* best 85.00%: all divisions/moduli and the *99 expansion line up; gcc picks
   different registers for the three arguments and hoists the format address.
void func_80083CE8(void) {
    char buf[0x20];
    s32 n;

    n = D_80092944;
    Sprintf(buf, D_800701B0, n / 1800, (n % 1800) / 30, (n % 30) * 99 / 30);
    func_8007EBE0(buf);
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083DCC);

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
