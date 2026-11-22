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
    /* 0x38 */ u_int (*unk38)();
    /* 0x3C */ int (*unk3C)(int mode);
} GpuSysFuncs; // Size: 0x40

extern GpuSysFuncs *g_GpuSysFuncs;
extern u_char g_GraphType;
extern s16 D_800568D4;
extern s16 D_800568D6;
extern u_char g_GraphReverse;

extern volatile u_long *D_800569A0; // GPU status register
extern volatile u_long *D_800569A4; // GPU data register (GP0)
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
extern u_long DrawMode(int dfe, int dtd, u_short tpage);
extern u_long func_80045BCC(int x, int y);
extern u_long func_80045C10(RECT *tw);

int GetODE(void) {
    return g_GpuSysFuncs->unk38() >> 31;
}

void SetTexWindow(DR_TWIN *p, RECT *tw) {
    setlen(p, 2);
    *(u_long*)&p->code[0] = func_80045C10(tw);
    *(u_long*)&p->code[1] = 0;
}

void SetDrawArea(DR_AREA *p, RECT *pt) {
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
    *(u_long*)&p->code[0] = DrawMode(dfe, dtd, tpage);
    *(u_long*)&p->code[1] = func_80045C10(tw);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045534);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_8004574C);

u_long DrawMode(int dfe, int dtd, u_short tpage) {
    u_long mode;
    u_long tp;

    /* retail keeps the 320-wide test and the tpage word in one register: reuse tp */
    tp = g_GraphType == 1 || g_GraphType == 2;
    if (tp) {
        mode = 0xE1000000;
        if (dtd)
            mode |= 0x800;
        tp = tpage & 0x27FF;
        if (dfe)
            tp |= 0x1000;
    } else {
        mode = 0xE1000000;
        if (dtd)
            mode |= 0x200;
        tp = tpage & 0x9FF;
        if (dfe)
            tp |= 0x400;
    }
    return mode | tp;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045A34);

/* best: 79.12% (188 bytes; retail is 204). The unused u_long t[4] local IS the
   lever that buys retail's 0x10 frame, allocated in the bltz delay slot and never
   stored into: 71.18 -> 79.12% and 176 -> 188 bytes. What is left is register
   roles, not structure. Retail per axis: copy the raw parameter to a spare
   register, truncate the value to s16 IN PLACE, bltz onto an out-of-line "v = 0",
   then "slt max-1,v" + bnez onto the clamp value and a j/move of the raw copy for
   the keep case; the merge copies the winner back through v0. gcc instead keeps
   the raw parameter in $a0, truncates into a fresh register, and CSEs the two
   "D_800568D4 - 1" of the ternary into the one it fed the slt. With explicit
   copies (int rx = x; used in the keep arm) the diff shrinks to 196 bytes but
   scores 78.82. Measured worse: s32 sx/sy, u_short xi/yi, if/else clamp blocks
   (65.10), one-expression returns (73.08), parameter reassignment (79.12 same
   size). The E3 tail is func_80045BCC's tail (masked x in v0, lui 0xE3000000 in
   a0), so the u_short code / u_long ret idiom is the right one here too.
u_long func_80045A34(int x, int y) {
    u_long t[4];
    u_short code;
    u_long fld;
    u_long ret;
    u_long xi;
    u_long yi;

    xi = ((s16)x < 0) ? 0 : (((s16)x > D_800568D4 - 1) ? D_800568D4 - 1 : x);
    yi = ((s16)y < 0) ? 0 : (((s16)y > D_800568D6 - 1) ? D_800568D6 - 1 : y);
    if (g_GraphType != 1 && g_GraphType != 2) {
        fld = yi & 0x3FF;
        fld = fld << 10;
        code = xi & 0x3FF;
    } else {
        fld = yi & 0xFFF;
        fld = fld << 12;
        code = xi & 0xFFF;
    }
    ret = code | 0xE3000000;
    return fld | ret;
}
*/

/* best: 70.27% -- same clamp as func_80045A34, E4 word.
u_long func_80045B00(int x, int y) {
    x = ((s16)x < 0) ? 0 : (((s16)x > D_800568D4 - 1) ? D_800568D4 - 1 : x);
    y = ((s16)y < 0) ? 0 : (((s16)y > D_800568D6 - 1) ? D_800568D6 - 1 : y);
    if (g_GraphType == 1 || g_GraphType == 2)
        return 0xE4000000 | (x & 0xFFF) | ((y & 0xFFF) << 12);
    return 0xE4000000 | (x & 0x3FF) | ((y & 0x3FF) << 10);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045B00);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045BCC);

/* best: 98.53% (68 bytes = retail's exact size, identical instruction
   multiset; only the register holding the masked x differs: retail masks it into
   $v0 and lui's 0xE5000000 into $a0, gcc the reverse, which also swaps the two
   closing `or` operands). CAUSE MEASURED with `cc1 -O3 -G0 -dg` on this body:
   global_alloc's order is `76 75 74 73 72` and the 0xE5000000 constant allocno is
   FIRST, so it always takes $v0; the masked-x allocno comes next to last (3 refs
   over ~7 insns, priority floor_log2(refs)*refs/live*size) and can only outrank a
   2-ref, 2-insn live constant at 8+ refs, i.e. with instructions retail does not
   have. The order is unchanged with `u_long code` (SImode, 4x the HImode
   allocno_size) and with the natural no-dead-test body, so this is not a
   source-reachable tie. Measured this round, 98.53 or worse: `code |=
   0xE5000000; return fld | code;` (86.76 - the two arms then cross-jump and two
   instructions disappear), `return fld | code | 0xE5000000;` (85.29 - code takes
   $v1 and the or order flips), `if (t) ret = ...; else ret = ...; return fld |
   ret;` (98.53), u_long code (98.53), a 3-minute permuter run from this body
   (base=25 best=25, 1428 iterations). Levers already found, all load-bearing: the mode test
   written INVERTED (11-bit arm first) so gcc threads bnez onto the 12-bit arm,
   the shift as its own statement, the dead trailing test that makes reorg fill
   the bnez slot, and a SEPARATE ret plus a u_short local for the masked x
   (97.94 -> 98.53). Measured worse or equal since: the masked x assigned to ret
   (97.94), `ret | const` in place (97.94), const first in the or (98.53), s16 /
   u_long / int code (98.53 / 97.94), decl order permutations (98.53), early
   returns with one expression (69.59 / 70.71), DrawMode style const-in-arm
   (69.41), dead test on fld (59.41), dead test on code (63.47), `(u_long)x &
   mask` (98.53), two earlier permuter runs (nothing).
u_long func_80045BCC(int x, int y) {
    u_short code;
    u_long fld;
    u_long ret;

    if (g_GraphType != 1 && g_GraphType != 2) {
        fld = y & 0x7FF;
        fld = fld << 11;
        code = x & 0x7FF;
    } else {
        fld = y & 0xFFF;
        fld = fld << 12;
        code = x & 0xFFF;
    }
    if (g_GraphType || y) {
        ret = code | 0xE5000000;
        return fld | ret;
    } else {
        ret = code | 0xE5000000;
        return fld | ret;
    }
}
*/

u_long func_80045C10(RECT *tw) {
    u_long t[4]; /* load-bearing: the four dead stack stores of retail's frame */
    u_long ret;

    if (tw == 0) {
        ret = 0;
        goto done;
    }
    t[0] = (tw->x & 0xFF) >> 3;
    t[2] = (~(tw->w - 1) & 0xFF) >> 3;
    t[1] = (tw->y & 0xFF) >> 3;
    t[3] = (~(tw->h - 1) & 0xFF) >> 3;
    ret = 0xE2000000 | (t[1] << 15) | (t[0] << 10) | (t[3] << 5) | t[2];
done:
    return ret;
}

/* this is _get_tw(). Retail: 0x10 frame, the four masked fields spilled to
   0(sp)/4(sp)/8(sp)/0xc(sp) and never reloaded, then kept live in a1/v0/a2/v1.
   Four locals declared x,y,w,h reproduce that slot order, and
   (~(tw->w - 1) & 0xff) >> 3 is the form that gives retail's negu/andi/sra --
   the header macro masks INSIDE the complement, which folds differently. But
   without the dead stores gcc combine turns h's (>>3)<<5 into sll 2 + andi 0x3e0
   and drops the frame (25 insns vs retail 33). A local struct of the four words
   whose stores gcc 2.7.2 does not DSE is the untried idea that would block it.
u_long func_80045C10(RECT *tw) {
    return _get_tw(tw);
}
*/

/* best: 68.48% -- retail merges the two `return p->x` blocks and shares the
   `0x400 - w - p->x` tail between the mode 1 and mode 2 arms (case 2 jumps into
   case 1's tail); gcc keeps p in $a1 and emits every arm separately. A shared
   `offs` local with one trailing return scores worse (53.43%).
u_long func_80045C94(RECT *p) {
    if (g_GraphType == 1) {
        if (g_GraphReverse)
            return 0x400 - p->w - p->x;
        return p->x;
    } else if (g_GraphType == 2) {
        if (g_GraphReverse)
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

int GpuSendWordsByCpu(u_long *src, int n) {
    int i;

    *D_800569A4 = 0x04000000;
    for (i = n - 1; i != -1; i--)
        *D_800569A0 = *src++;
    return 0;
}

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
