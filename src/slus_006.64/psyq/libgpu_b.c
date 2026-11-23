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
extern s16 g_GpuScreenWidth;
extern s16 g_GpuScreenHeight;
extern u_char g_GraphReverse;

extern volatile u_long *g_GpuGp0Port; // GP0: commands written here, GPUREAD read here
extern volatile u_long *g_GpuGp1Port; // GP1: status read here, GP1 commands written here
extern u_long *g_GpuDma2Madr;
extern u_long *g_GpuDma2Bcr;
extern volatile u_long *g_GpuDma2Chcr;
extern u_long *g_GpuDma6Madr;
extern u_long *g_GpuDma6Bcr;
extern volatile u_long *g_GpuDma6Chcr;
extern u_long *g_GpuDmaDpcr;

extern u_char g_GpuOpcodeShadow[]; // one byte per GP0 command

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

void func_80045534(DR_ENV *drEnv, DRAWENV *env) {
    DR_ENV *p = drEnv;
    RECT rect;
    int n;

    p->code[0] = func_80045A34(env->clip.x, env->clip.y);
    p->code[1] = func_80045B00((s16)(env->clip.w + env->clip.x - 1), (s16)(env->clip.y + env->clip.h - 1));
    p->code[2] = func_80045BCC(env->ofs[0], env->ofs[1]);
    p->code[3] = DrawMode(env->dfe, env->dtd, env->tpage);
    p->code[4] = func_80045C10(&env->tw);
    p->code[5] = 0xE6000000;
    n = 7;
    if (env->isbg) {
        rect.x = env->clip.x;
        rect.y = env->clip.y;
        rect.w = env->clip.w;
        rect.h = env->clip.h;
        rect.w = rect.w < 0 ? 0 : (rect.w > g_GpuScreenWidth - 1 ? g_GpuScreenWidth - 1 : rect.w);
        rect.h = rect.h < 0 ? 0 : (rect.h > g_GpuScreenHeight - 1 ? g_GpuScreenHeight - 1 : rect.h);
        rect.x -= env->ofs[0];
        rect.y -= env->ofs[1];
        ((u_long *)p)[n++] = 0x60000000 | (env->b0 << 16) | (env->g0 << 8) | env->r0;
        ((u_long *)p)[n++] = *(u_long *)&rect.x;
        ((u_long *)p)[n++] = *(u_long *)&rect.w;
        rect.x += env->ofs[0];
        rect.y += env->ofs[1];
    }
    setlen(p, n - 1);
}

void func_8004574C(DR_ENV *drEnv, DRAWENV *env) {
    DR_ENV *p = drEnv;
    RECT rect;
    int n;

    p->code[0] = func_80045A34(env->clip.x, env->clip.y);
    p->code[1] = func_80045B00((s16)(env->clip.w + env->clip.x - 1), (s16)(env->clip.y + env->clip.h - 1));
    p->code[2] = func_80045BCC(env->ofs[0], env->ofs[1]);
    p->code[3] = DrawMode(env->dfe, env->dtd, env->tpage);
    p->code[4] = func_80045C10(&env->tw);
    p->code[5] = 0xE6000000;
    n = 7;
    if (env->isbg) {
        rect.x = env->clip.x;
        rect.y = env->clip.y;
        rect.w = env->clip.w;
        rect.h = env->clip.h;
        rect.w = rect.w < 0 ? 0 : (rect.w > g_GpuScreenWidth - 1 ? g_GpuScreenWidth - 1 : rect.w);
        rect.h = rect.h < 0 ? 0 : (rect.h > g_GpuScreenHeight - 1 ? g_GpuScreenHeight - 1 : rect.h);
        if ((rect.x & 0x3F) != 0 || (rect.w & 0x3F) != 0) {
            rect.x -= env->ofs[0];
            rect.y -= env->ofs[1];
            ((u_long *)p)[n++] = 0x60000000 | (env->b0 << 16) | (env->g0 << 8) | env->r0;
            ((u_long *)p)[n++] = *(u_long *)&rect.x;
            ((u_long *)p)[n++] = *(u_long *)&rect.w;
            rect.x += env->ofs[0];
            rect.y += env->ofs[1];
        } else {
            ((u_long *)p)[n++] = 0x02000000 | (env->b0 << 16) | (env->g0 << 8) | env->r0;
            ((u_long *)p)[n++] = *(u_long *)&rect.x;
            ((u_long *)p)[n++] = *(u_long *)&rect.w;
        }
    }
    setlen(p, n - 1);
}

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

// best: 79.12% (188 bytes; retail is 204 = 4 instructions more). Retail per
// axis: copy the raw parameter out (move a3,a0), truncate the parameter register
// IN PLACE (sll a0; sra a0), bltz to an out-of-line v=0, then slt of W-1 against
// the truncated value, bnez to an out-of-line clamp whose delay slot recomputes
// 'addiu v0,a2,-1' from a2 = the W copy; the keep arm is 'j merge; move v0,raw'.
// The two extra instructions per axis are the 'move a2,v0' W copy plus that
// recomputation: gcc CSEs g_GpuScreenWidth-1 into one register, so it emits
// 'addiu v1,v0,-1' once and moves it into the merge. Only the y axis comes out
// with retail's register roles (a1 = yi, a2 = truncated scratch).
// Measured this round: u_long t[4] frame + ternary (below) 79.12; the same body
// with if/else clamp blocks over u_long xi/yi (xi = x; if ((s16)xi < 0) ...)
// 65.59; gcc-2.7.2-cdk under this unit's flags 75.78; gcc-2.6.0 79.12. Earlier
// rounds: s32 sx/sy, u_short xi/yi, explicit 'int rx = x' copies (78.82, 196
// bytes), one-expression returns (73.08), parameter reassignment (79.12).
/*
u_long func_80045A34(int x, int y) {
    u_long t[4];
    u_short code;
    u_long fld;
    u_long ret;
    u_long xi;
    u_long yi;

    xi = ((s16)x < 0) ? 0 : (((s16)x > g_GpuScreenWidth - 1) ? g_GpuScreenWidth - 1 : x);
    yi = ((s16)y < 0) ? 0 : (((s16)y > g_GpuScreenHeight - 1) ? g_GpuScreenHeight - 1 : y);
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
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045A34);

/* best: 79.80% (192 bytes / 48 insns; retail is 204 / 51). Same body as the
   func_80045A34 park -- the two stubs are instruction-identical except the opcode
   word, so whatever closes one closes both.
   CLOSED the 0x10 phantom frame: retail's `.frame $sp,16` with no store anywhere is
   an unused local array; `u_long t[4]` gives vars=16 and puts `subu $sp,$sp,16` in
   the bltz delay slot exactly as retail has it. With it 70.27 -> 79.80 (ternary +
   t[4]); the if/else-if clamp without it was 60.10.
   NAMED RESIDUAL, 3 instructions, all in the x clamp: retail keeps THREE pseudos
   there -- x in $a3 (`move a3,a0`), the (s16) truncation done IN PLACE on $a0, and
   the clamp result in $v0 copied back to $a3 (`move a3,v0`) -- plus a second copy of
   the width (`move a2,v0`) so the clamp arm re-forms W-1 (`addiu v0,a2,-1`) instead
   of reusing the compare's. gcc cse1 merges the two W-1 into one register, so the
   duplicate is unreachable from any spelling that names W-1 twice.
   PROBED this round with cc1_probe.sh (11 spellings, none reproduces it): ternary
   on the parameter, ternary on the clamped local, if/else-if with and without an
   `else xc = x` keep arm, s16 mirror locals for the cast, s16 parameters, u_short
   and s16 local copies of the limit, and a CLIP16 macro with the limit textually
   duplicated. All of them coalesce xc into the parameter's register (no `move a3,a0`)
   and truncate into a fresh register.
u_long func_80045B00(int x, int y) {
    u_long t[4]; // load-bearing: retail allocates a 0x10 frame and stores nothing in it
    u_long code;
    u_long fld;
    u_long ret;
    int xc;
    int yc;

    xc = ((s16)x < 0) ? 0 : (((s16)x > g_GpuScreenWidth - 1) ? g_GpuScreenWidth - 1 : x);
    yc = ((s16)y < 0) ? 0 : (((s16)y > g_GpuScreenHeight - 1) ? g_GpuScreenHeight - 1 : y);
    if (g_GraphType != 1 && g_GraphType != 2) {
        fld = yc & 0x3FF;
        fld = fld << 10;
        code = xc & 0x3FF;
    } else {
        fld = yc & 0xFFF;
        fld = fld << 12;
        code = xc & 0xFFF;
    }
    ret = code | 0xE4000000;
    return fld | ret;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045B00);

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
   Round 419 re-measured the un-parked body at 98.53 (68 bytes, same 17 insns) and
   confirmed the residual is `andi a0,a0,...` -- gcc reuses the dead parameter x's
   $a0 and coalesces the or result with the CONSTANT, retail keeps the masked x in
   $v0 and the constant in $a0. Two new shapes measured: `fld | (code | 0xE5000000)`
   as one expression, matching retail's or tree exactly, 98.24% (loses the jr slot
   as well); `return ret | fld;` 97.94%. Broker is the parameter reuse: retail's
   andi writes a fresh register, so x was still live, which no spelling of
   `code = x & mask` in an arm reproduces.*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045BCC);

/* best: 98.53%
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libgpu_b", func_80045C94);

/* best: 92.73% (184 bytes; retail is 176). The dispatch is a switch: retail is
   beq/beq/j with the default (return p->x) block last, and the three "return p->x"
   paths already merge into retail's single lh v0,0(a0) block here. What is left is
   the shared - 0x400 - v - p->x - tail: retail keeps it inline in case 1 and case
   2's reverse arm jumps INTO it at +0x40 with li v0,1024 in the delay slot, i.e.
   gcc merged case 2's whole tail into case 1's. Every spelling tried makes gcc
   merge only the last subu, into a third block that costs one instruction.
   Measured: plain return 0x400 - p->w - p->x in both arms 88.02 (gcc reassociates
   to (0x400 - p->x) - p->w, which swaps the two loads and kills the merge);
   92.68 with the w = p->w / 2 local in case 2; 87.16 with the v = 0x400 - p->w
   form in both arms (the v/w locals are what force the natural operand order);
   92.73 with both. A 4-minute permuter run (base=457 best=317) found only the
   un-parenthesised case-2 arm, which scores 88.07.
u_long func_80045C94(RECT *p) {
    int v;
    int w;

    switch (g_GraphType) {
    case 1:
        if (g_GraphReverse) {
            v = 0x400 - p->w;
            return v - p->x;
        }
        break;
    case 2:
        if (g_GraphReverse) {
            w = p->w / 2;
            return 0x400 - w - p->x;
        }
        return p->x / 2;
    }
    return p->x;
}
*/


u_long func_80045D44(void) {
    return *g_GpuGp1Port;
}

int func_80045D5C(u_long *ot, int n) {
    *g_GpuDmaDpcr |= 0x08000000;
    *g_GpuDma6Chcr = 0;
    *g_GpuDma6Madr = (u_long)&ot[n - 1];
    *g_GpuDma6Bcr = n;
    *g_GpuDma6Chcr = 0x11000002;
    func_80046EFC();
    while (*g_GpuDma6Chcr & 0x1000000) {
        if (func_80046F30())
            return -1;
    }
    return n;
}

/* The clear-with-colour packet this GPU driver entry DMAs from a static buffer:
   the fill node, linked by its tag to the 3-word node that restores the draw area
   and offset the fill had to overwrite. g_GpuSysFuncsTable holds this function's
   address in the g_GpuSysFuncs.unk0C slot that ClearImage/ClearImage2 pass to the
   DMA sender. */
extern u_long D_8005A238[9]; // tag + 8 words
extern u_long D_8005A25C[4]; // tag + 3 words

int func_80045E44(RECT *rect, u_long color) {
    limitRange(rect->w, 0, g_GpuScreenWidth - 1);
    limitRange(rect->h, 0, g_GpuScreenHeight - 1);

    /* A 0x60 fill is shifted by the drawing offset, so the draw area has to be set
       to the whole screen first and put back afterwards; a rectangle aligned to 64
       fills unscaled (0x02) and needs neither. */
    if ((rect->x & 0x3F) != 0 || (rect->w & 0x3F) != 0) {
        /* load-bearing: written inline, cc1 folds 0xE1000000 onto the status read
           and the result lands in the wrong register (96.03%) */
        u_long drawMode;

        drawMode = (color >> 31 << 10) | 0xE1000000;
        D_8005A238[0] = 0x08000000 | ((u_long)D_8005A25C & 0xFFFFFF);
        D_8005A238[1] = 0xE3000000;
        D_8005A238[2] = 0xE4FFFFFF;
        D_8005A238[3] = 0xE5000000;
        D_8005A238[4] = 0xE6000000;
        D_8005A238[5] = (*g_GpuGp1Port & 0x7FF) | drawMode;
        D_8005A238[6] = 0x60000000 | (color & 0xFFFFFF);
        D_8005A238[7] = *(u_long *)&rect->x;
        D_8005A238[8] = *(u_long *)&rect->w;
        D_8005A25C[0] = 0x03FFFFFF;
        D_8005A25C[1] = 0xE3000000 | func_80046638(3);
        D_8005A25C[2] = 0xE4000000 | func_80046638(4);
        D_8005A25C[3] = 0xE5000000 | func_80046638(5);
    } else {
        /* load-bearing: written inline, cc1 folds 0xE1000000 onto the status read
           and the result lands in the wrong register (96.03%) */
        u_long drawMode;

        drawMode = (color >> 31 << 10) | 0xE1000000;
        D_8005A238[0] = 0x05FFFFFF;
        D_8005A238[1] = 0xE6000000;
        D_8005A238[2] = (*g_GpuGp1Port & 0x7FF) | drawMode;
        D_8005A238[3] = 0x02000000 | (color & 0xFFFFFF);
        D_8005A238[4] = *(u_long *)&rect->x;
        D_8005A238[5] = *(u_long *)&rect->w;
    }
    func_800465EC((u_long)D_8005A238);
    return 0;
}

/* LoadImage: the `mode = 0` local is real -- retail keeps the `beqz $s5` for it and
   picks the 0xA0/0xB0 command at the store. Both waits run `while (!(status & bit))`:
   the GPU asserts DMA request/ready, so the loop exits when the bit is SET. */
int func_800460A0(RECT *rect, u_long *p) {
    int total;
    int size;
    int blocks;
    int rest;
    int mode;

    func_80046EFC();
    mode = 0;
    rect->w = rect->w < 0 ? 0 : (rect->w > g_GpuScreenWidth ? g_GpuScreenWidth : rect->w);
    rect->h = rect->h < 0 ? 0 : (rect->h > g_GpuScreenHeight ? g_GpuScreenHeight : rect->h);
    total = rect->w * rect->h + 1;
    size = total / 2;
    if (size <= 0)
        return -1;
    rest = size >> 4;
    rest = size - rest * 16;
    blocks = size >> 4;
    while (!(*g_GpuGp1Port & 0x04000000)) {
        if (func_80046F30())
            return -1;
    }
    *g_GpuGp1Port = 0x04000000;
    *g_GpuGp0Port = 0x01000000;
    *g_GpuGp0Port = mode ? 0xB0000000 : 0xA0000000;
    *g_GpuGp0Port = *(u_long *)&rect->x;
    *g_GpuGp0Port = *(u_long *)&rect->w;
    while (--rest != -1)
        *g_GpuGp0Port = *p++;
    if (blocks != 0) {
        *g_GpuGp1Port = 0x04000002;
        *g_GpuDma2Madr = (u_long)p;
        *g_GpuDma2Bcr = (blocks << 16) | 0x10;
        *g_GpuDma2Chcr = 0x01000201;
    }
    return 0;
}

/* StoreImage: the mirror of LoadImage -- same clamp, same split into 16-word DMA
   blocks plus a CPU-sent tail, but the tail reads GPUREAD and the DMA runs
   GPUREAD -> CPU, so it waits on bit 0x08000000 after the 0xC0 command and there
   is no bg/mode argument (hence no $s5 and a 0x48 frame). */
int func_800462DC(RECT *rect, u_long *p) {
    int total;
    int size;
    int blocks;
    int rest;

    func_80046EFC();
    rect->w = rect->w < 0 ? 0 : (rect->w > g_GpuScreenWidth ? g_GpuScreenWidth : rect->w);
    rect->h = rect->h < 0 ? 0 : (rect->h > g_GpuScreenHeight ? g_GpuScreenHeight : rect->h);
    total = rect->w * rect->h + 1;
    size = total / 2;
    if (size <= 0)
        return -1;
    /* load-bearing: rest doubles as the block count so cc1 keeps the copy */
    rest = size >> 4;
    rest = size - rest * 16;
    blocks = size >> 4;
    while (!(*g_GpuGp1Port & 0x04000000)) {
        if (func_80046F30())
            return -1;
    }
    *g_GpuGp1Port = 0x04000000;
    *g_GpuGp0Port = 0x01000000;
    *g_GpuGp0Port = 0xC0000000;
    *g_GpuGp0Port = *(u_long *)&rect->x;
    *g_GpuGp0Port = *(u_long *)&rect->w;
    while (!(*g_GpuGp1Port & 0x08000000)) {
        if (func_80046F30())
            return -1;
    }
    while (--rest != -1)
        *p++ = *g_GpuGp0Port;
    if (blocks != 0) {
        *g_GpuGp1Port = 0x04000003;
        *g_GpuDma2Madr = (u_long)p;
        *g_GpuDma2Bcr = (blocks << 16) | 0x10;
        *g_GpuDma2Chcr = 0x01000200;
    }
    return 0;
}

void func_80046560(u_long value) {
    *g_GpuGp1Port = value;
    g_GpuOpcodeShadow[value >> 24] = value;
}

u_char func_80046588(int index) {
    return g_GpuOpcodeShadow[index];
}

int GpuSendWordsByCpu(u_long *src, int n) {
    int i;

    *g_GpuGp1Port = 0x04000000;
    for (i = n - 1; i != -1; i--)
        *g_GpuGp0Port = *src++;
    return 0;
}

void func_800465EC(u_long value) {
    *g_GpuGp1Port = 0x04000002;
    *g_GpuDma2Madr = value;
    *g_GpuDma2Bcr = 0;
    *g_GpuDma2Chcr = 0x01000401;
}

u_long func_80046638(u_long value) {
    *g_GpuGp1Port = value | 0x10000000;
    return *g_GpuGp0Port & 0xFFFFFF;
}

void siocons(u_long *p, int x, int y) {
    func_8004668C(p, x, 0, y);
}
