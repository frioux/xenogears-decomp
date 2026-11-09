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

extern u_long *D_800569A0; // GPU status register
extern u_long *D_800569A4; // GPU data register (GP0)
extern u_long *D_800569A8;
extern u_long *D_800569AC;
extern volatile u_long *D_800569B0;
extern u_long *D_800569B4;
extern u_long *D_800569B8;
extern u_long *D_800569BC;
extern u_long *D_800569C0;

extern u_char D_8005A27C[]; // one byte per GP0 command

extern void func_8004668C(u_long *p, int x, int y, int w);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_800453E8);

void func_8004546C(DR_MODE *p, RECT *pt) {
    setlen(p, 2);
    *(u_long*)&p->code[0] = func_80045BCC(pt->x, pt->y);
    *(u_long*)&p->code[1] = 0;
}

void func_800454B0(DR_MODE *p, int dfe, int dtd) {
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045A34);

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045C94);

u_long func_80045D44(void) {
    return *D_800569A4;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045D5C);

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

void func_80046668(u_long *p, int x, int y) {
    func_8004668C(p, x, 0, y);
}
