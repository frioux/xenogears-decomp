#include "common.h"
#include "psyq/libgpu.h"

// GPU driver vtable, selected by the video mode (see libgpu.c)
typedef struct {
    /* 0x00 */ char *version;
    /* 0x04 */ void (*unk04)();
    /* 0x08 */ int (*unk08)(u_long madr, void *p, int n, u_long param);
    /* 0x0C */ u_long unk0C;
    /* 0x10 */ void (*unk10)(u_long value);
    /* 0x14 */ void (*unk14)(void *p, int n);
    /* 0x18 */ u_long unk18;
    /* 0x1C */ u_long unk1C;
    /* 0x20 */ u_long unk20;
    /* 0x24 */ void (*unk24)();
    /* 0x28 */ u_long (*unk28)(int mode);
    /* 0x2C */ void (*unk2C)(u_long *ot, int n);
    /* 0x30 */ void (*unk30)();
    /* 0x34 */ void (*unk34)(int mode);
    /* 0x38 */ int (*unk38)();
    /* 0x3C */ int (*unk3C)(int mode);
} GpuSysFuncs; // Size: 0x40

extern GpuSysFuncs *D_800568C8;
extern u_char D_800568D0;
extern s16 D_800568D4;
extern s16 D_800568D6;
extern u_char D_800568D3;

extern u_long *D_800569A0; // GPU status register
extern u_long *D_800569A4; // GPU data register (GP0)
extern u_long *D_800569A8;
extern u_long *D_800569AC;
extern volatile u_long *D_800569B0;
extern u_long *D_800569B4;
extern u_long *D_800569B8;
extern volatile u_long *D_800569BC;
extern u_long *D_800569C0;

extern u_char D_8005A27C[]; // one byte per GP0 command

extern void func_8004668C(u_long *p, int x, int y, int w);
extern void func_80046EFC(void);
extern int func_80046F30(void);
extern u_long func_80045A34(int x, int y);
extern u_long func_80045B00(int x, int y);
extern u_long func_800459DC(int dfe, int dtd, u_short tpage);
extern u_long func_80045BCC(int x, int y);
extern u_long func_80045C10(RECT *tw);

int func_8004537C(void) {
    return (u_int)D_800568C8->unk38() >> 31;
}

void func_800453AC(DR_MODE *p, RECT *tw) {
    setlen(p, 2);
    *(u_long*)&p->code[0] = func_80045C10(tw);
    *(u_long*)&p->code[1] = 0;
}

void func_800453E8(DR_MODE *p, RECT *pt) {
    setlen(p, 2);
    *(u_long*)&p->code[0] = func_80045A34(pt->x, pt->y);
    *(u_long*)&p->code[1] = func_80045B00((s16)(pt->x + pt->w - 1), (s16)(pt->y + pt->h - 1));
}

void func_8004546C(DR_MODE *p, RECT *pt) {
    setlen(p, 2);
    *(u_long*)&p->code[0] = func_80045BCC(pt->x, pt->y);
    *(u_long*)&p->code[1] = 0;
}

void SetPriority(DR_MODE *p, int dfe, int dtd) {
    setlen(p, 2);
    *(u_long*)&p->code[0] = 0xE6000000 | (dfe ? 0x2 : 0) | (dtd ? 0x1 : 0);
    *(u_long*)&p->code[1] = 0;
}

void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw) {
    setlen(p, 2);
    *(u_long*)&p->code[0] = func_800459DC(dfe, dtd, tpage);
    *(u_long*)&p->code[1] = func_80045C10(tw);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045534);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_8004574C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_800459DC);

/* best: 58.61% -- the (s16) casts reproduce retail's clamp tests, but gcc keeps
   the incoming x in $a0 and puts the truncated temp in $a3 (retail the reverse),
   then allocates the E3 words from different registers as a consequence.
u_long func_80045A34(int x, int y) {
    if ((s16)x < 0)
        x = 0;
    else if ((s16)x > D_800568D4 - 1)
        x = D_800568D4 - 1;
    if ((s16)y < 0)
        y = 0;
    else if ((s16)y > D_800568D6 - 1)
        y = D_800568D6 - 1;
    if (D_800568D0 == 1 || D_800568D0 == 2)
        return 0xE3000000 | (x & 0xFFF) | ((y & 0xFFF) << 12);
    return 0xE3000000 | (x & 0x3FF) | ((y & 0x3FF) << 10);
}
*/

/* best: 70.27% (176 bytes, same size) -- the ternary form fixes the branch
   polarity and block order; gcc keeps the raw x in $a0 and truncates into $a2
   where retail copies the raw x to $a3 and truncates $a0 in place, and gcc CSEs
   the two "D_800568D4 - 1" that retail computes twice.
u_long func_80045A34(int x, int y) {
    x = ((s16)x < 0) ? 0 : (((s16)x > D_800568D4 - 1) ? D_800568D4 - 1 : x);
    y = ((s16)y < 0) ? 0 : (((s16)y > D_800568D6 - 1) ? D_800568D6 - 1 : y);
    if (D_800568D0 == 1 || D_800568D0 == 2)
        return 0xE3000000 | (x & 0xFFF) | ((y & 0xFFF) << 12);
    return 0xE3000000 | (x & 0x3FF) | ((y & 0x3FF) << 10);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045A34);


/* best: 70.27% -- same clamp as func_80045A34, E4 word.
u_long func_80045B00(int x, int y) {
    x = ((s16)x < 0) ? 0 : (((s16)x > D_800568D4 - 1) ? D_800568D4 - 1 : x);
    y = ((s16)y < 0) ? 0 : (((s16)y > D_800568D6 - 1) ? D_800568D6 - 1 : y);
    if (D_800568D0 == 1 || D_800568D0 == 2)
        return 0xE4000000 | (x & 0xFFF) | ((y & 0xFFF) << 12);
    return 0xE4000000 | (x & 0x3FF) | ((y & 0x3FF) << 10);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045B00);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045BCC);

/* best: 83.18% -- identical instruction multiset (68 bytes both ways); gcc always
   gives the shifted y term v0 and the masked x term v1, retail the other way
   round, in every operand order tried.
u_long func_80045BCC(int x, int y) {
    if (D_800568D0 == 1 || D_800568D0 == 2)
        return 0xE5000000 | (x & 0xFFF) | ((y & 0xFFF) << 12);
    return 0xE5000000 | (x & 0x7FF) | ((y & 0x7FF) << 11);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045C10);

/* this is _get_tw(): retail spills all four masked fields into 0(sp)/4/8/0xc
   (a 0x10 frame) and keeps them live in a1/v0/a2/v1; the plain macro body below
   is semantically identical but gcc folds (h<<5) into a mask and drops the frame.
u_long func_80045C10(RECT *tw) {
    return _get_tw(tw);
}
*/

/* best: 68.48% -- retail merges the two `return p->x` blocks and shares the
   `0x400 - w - p->x` tail between the mode 1 and mode 2 arms (case 2 jumps into
   case 1's tail); gcc keeps p in $a1 and emits every arm separately. A shared
   `offs` local with one trailing return scores worse (53.43%).
u_long func_80045C94(RECT *p) {
    if (D_800568D0 == 1) {
        if (D_800568D3)
            return 0x400 - p->w - p->x;
        return p->x;
    } else if (D_800568D0 == 2) {
        if (D_800568D3)
            return 0x400 - p->w / 2 - p->x;
        return p->x / 2;
    }
    return p->x;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045C94);


u_long func_80045D44(void) {
    return *D_800569A4;
}

int func_80045D5C(u_long *ot, int n) {
    *D_800569C0 |= 0x08000000;
    *D_800569BC = 0;
    *D_800569B4 = (u_long)&ot[n - 1];
    *D_800569B8 = n;
    *D_800569BC = 0x11000002;
    func_80046EFC();
    while (*D_800569BC & 0x1000000) {
        if (func_80046F30())
            return -1;
    }
    return n;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045E44);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_800460A0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_800462DC);

void func_80046560(u_long value) {
    *D_800569A4 = value;
    D_8005A27C[value >> 24] = value;
}

u_char func_80046588(int index) {
    return D_8005A27C[index];
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_8004659C);

/* best: 81.75% -- same instructions, but gcc rotates the loop (store into the
   delay slot of the backward bne) where retail keeps store then bne with
   move v0,zero in the slot.
int func_8004659C(u_long *src, int n) {
    int i;

    *D_800569A4 = 0x04000000;
    for (i = n - 1; i != -1; i--)
        *D_800569A0 = *src++;
    return 0;
}
*/

void func_800465EC(u_long value) {
    *D_800569A4 = 0x04000002;
    *D_800569A8 = value;
    *D_800569AC = 0;
    *D_800569B0 = 0x01000401;
}

u_long func_80046638(u_long value) {
    *D_800569A4 = value | 0x10000000;
    return *D_800569A0 & 0xFFFFFF;
}

void siocons(u_long *p, int x, int y) {
    func_8004668C(p, x, 0, y);
}
