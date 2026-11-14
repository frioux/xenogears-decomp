#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern u8 *D_800C3EAC;
extern u8 *D_800D2D28;
extern u8 D_800C3EB4[];
extern u8 D_800D301C[];
extern u16 D_800D39DC;
extern u16 D_800C48E8;
extern u16 D_800D2C94;
extern u16 D_800D2C96;
extern u16 D_800D2C98;
extern u8 D_800D2CA9;
extern u8 D_800D2CAA;
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800C3FFE[];
extern u8 D_800CCD34[];
extern u8 D_800CCD36[];
extern u8 D_800CCD68[];
extern u8 D_800CCD74[];
extern u8 D_800D366C;
extern u8 D_800C3E18;
extern u8 D_800D32A1[];
extern u8 D_800C3EB5[];
extern u8 D_800D301D[];
extern u8 D_800C3EBE[];
extern u8 D_800C3EC0[];
extern u8 *D_800D3364;
extern u8 D_800C48EC[];
extern u8 D_800C48EE[];
extern u8 D_800C48F0[];

extern void func_80085454(u8);
extern void func_80085618(u8);
extern void func_80098C6C(u16);
extern void func_800941A4(void);
extern u16 func_80089C08(u8);
extern void func_80085D34(void);
extern s32 func_800877E0(s32, s32);
extern s32 func_80080AE4(s32);
extern void func_800B89FC(s32, s32, s32, s32);
extern u8 func_80089C48(u8);
extern s32 func_8009ADA0(u8, s32 *);
extern void func_80085388(void);
extern void func_800BE538(u8, s32, s32, s32);
extern u8 D_800C4000[];
extern u8 D_800C3FE8[];
extern u8 *D_800C3EA4;
extern s32 D_800CCB34;
extern u8 *D_800D2DB4;
extern s32 func_80033784(s32, s32);
extern void func_80076D58(u8 *, u8, u8);
extern void func_80076C78(POLY_FT4 *, s16, s16, u8, u8, u8);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern s32 func_80076A10(s32, s32, s16, s16);
extern u8 D_800D2C34[];
extern u8 D_800D2D24[];
extern u8 D_800C34CC[];
extern s32 func_80089C6C(s32, u8);
extern s32 D_800C3A7C;
extern s32 D_800C3A80;
extern s32 D_800C3A84;
extern s32 D_800C3A88;
extern s32 D_800C3A8C;
extern s32 D_800C3A90;
extern u8 D_800C3A94;
extern u8 D_800C3A98;
extern s32 D_800C3A9C;
extern s32 D_800C2080;
extern s32 D_800C2084;
extern u8 D_800C207C;
extern u8 func_8001BD40(s32, u8);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085618);
void func_80085AC4(s32 arg0) {
    s32 i = 2;
    s16 *p;

    *(u16 *)(D_800CCD34 + (u8)arg0 * 0x170) = *(u16 *)(D_800CCD36 + (u8)arg0 * 0x170);

    p = (s16 *)(D_800CCD68 + (u8)arg0 * 0x170);
    for (; i >= 0; i -= 2) {
        *p = 0;
        p -= 2;
    }

    i = 4;
    p = (s16 *)(D_800CCD74 + (u8)arg0 * 0x170);
    for (; i >= 0; i -= 2) {
        *p = 0;
        p -= 2;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085B58);
/* 92.25%: retail schedules the D_800C3EAC pointer load (lui/lw) into the lhu's
   load-delay slot and leaves the sh after it; gcc puts the sh first and a nop.
void func_80085B58(s32 arg0) {
    s32 buf[3];
    s32 i;
    u16 v;

    buf[0] = buf[1] = buf[2] = 0;
    if (func_8009ADA0((u8)arg0, buf) != 0) {
        for (i = 0; i < 3; i++) {
            if (buf[i] != 0) {
                D_800C3EAC[0x2DA] = 0;
                func_80085388();
                D_800C4000[(u8)arg0] = i + 8;
                v = buf[i];
                *(u16 *)(D_800C3FE8 + (u8)arg0 * 2) = v;
                func_80085618(D_800C3EAC[0x2DA]);
            }
        }
        func_800BE538((u8)arg0, buf[0], buf[1], buf[2]);
    }
}
*/
void func_80085C48(s32 arg0, u16 arg1, u16 arg2) {
    u16 v = D_800D39DC;

    D_800C48E8 = 0;
    D_800D2C94 = arg1;
    D_800D2C96 = v;
    func_80098C6C(arg2);
}
void func_80085C88(u8 arg0) {
    u8 v = arg0;

    func_80085454(v);
    func_80085618(v);
    D_800D2D28[0xAD] = 0;
}
void func_80085CCC(u8 arg0, u16 arg1, u16 arg2) {
    u8 *p = D_800C3EAC;
    u16 v = D_800D39DC;
    u8 c;

    D_800C48E8 = 0;
    D_800D2CA9 = arg0;
    c = p[0x2DC];
    D_800D2C94 = arg1;
    D_800D2C98 = arg2;
    D_800D2C96 = v;
    D_800D2CAA = c + 0xFF;
    func_800941A4();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085D34);
/* 90.42%: the three calls are byte-identical except that retail's
   `addu $a1,$a1,$v0` (offset first, base second) comes out base-first, and the
   epilogue differs (retail keeps `sb` after `lw $ra` with a nop in the jr slot).
   Tried: parenthesised offset, array-index form, an s32 ofs local, and the
   swapped operand order (that one makes gcc fold the +0x9C8 onto the base).
void func_80085D34(void) {
    s32 v;

    D_800D2D28[0x7B] = 0;
    v = func_80076A10(D_800C3EAC[0x2D4] + 0xF, (s32)(D_800C3EA4 + (D_800D2D28[0x7B] * 0x50 + 0x9C8)), 0x2A, 0xD0);
    D_800D2D28[0x7B] += v;
    v = func_80076A10(D_800C3EAC[0x2D4] + 0xF, (s32)(D_800C3EA4 + (D_800D2D28[0x7B] * 0x50 + 0x9C8)), 0x32, 0xD0);
    D_800D2D28[0x7B] += v;
    v = func_80076A10(D_800C3EAC[0x2D5] + 0xF, (s32)(D_800C3EA4 + (D_800D2D28[0x7B] * 0x50 + 0x9C8)), 0x3A, 0xD0);
    D_800D2D28[0x7B] += v;
    D_800D2D28[0xA4] = D_800CCB34;
}
*/
void func_80085E78(void) {
    s32 i;

    for (i = 0; i < 7; i++) {
        u8 *p = D_800C3EAC + i;

        p[0x2CC] = 0xFF;
    }
    D_800C3EAC[0x2D6] = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085EB4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086028);
/* 88.08%: instruction multiset is identical (same size, same prologue save
   order, same delay-slot placement); every difference is the s-register NUMBER
   gcc hands to each call-crossing value. Retail: s0=q s1=arg0 s2=arg2 s3=arg1
   s4=r s5=arg3 s6=arg5 s7=arg4 fp=arg1*4; ours: s0=arg5 s1=q s2=arg0 s3=r
   s4=arg1 s5=arg3 s6=arg1*4 s7=arg4 s8=arg2. The copies happen in the same
   order, so it is an allocator priority tie, not a shape difference.
   `arg1 * 0x50 + 0x5550 + D_800D2DB4` (symbol added last) is what puts the
   `addu` in the func_80076D58 jal delay slot. The three masks 0xFFFC/0xFFF8/
   0xF8 on the func_80076C78 coordinates are literal in the source.
s32 func_80086028(u8 arg0, s32 arg1, s32 arg2, u8 arg3, u32 **arg4, u8 arg5) {
    s32 q;
    s32 r;
    s32 len;
    RECT rect;

    q = arg1 / 2;
    r = arg1 % 2;
    func_80076D58((u8 *)(arg1 * 0x50 + 0x5550 + D_800D2DB4), r, 3);
    len = SystemRenderStringEntry((u8 *)func_80033784(D_800D2D24[arg0], arg3), *arg4, 0x1B, r);
    rect.x = q * 0x1E + 0x3C0;
    rect.y = 0x1A;
    rect.w = 0x1E;
    rect.h = 0xD;
    LoadImage(&rect, *arg4);
    func_80076C78((POLY_FT4 *)((D_800CCB34 + arg1 * 2) * 0x28 + 0x5550 + D_800D2DB4),
                  ((arg2 + arg5 + 1) * 0x10 + 0x50 + arg1 * 4) & 0xFFFC,
                  (0xC8 - arg1 * 0x10) & 0xFFF8,
                  (q * 0x78) & 0xF8,
                  0x1A,
                  len);
    D_800D2DB4[0x5D80]++;
    return arg1 + 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800861D0);
u8 func_80086B88(s32 arg0, u8 arg1) {
    s32 ok = 1;
    s32 idx;

    idx = arg0 + (D_800C3EAC[0x2CC] + 1) * 3;
    if (D_800C3EAC[0x2D6] != 0) {
        if (D_800D2C34[0] == 4) {
            idx = arg0 + 0xC;
        }
        if ((u16)func_80089C6C(*(u16 *)&g_GameState.unk16BC[D_800D2D24[arg1]].unk8,
                               D_800C34CC[idx & 0xFF]) != 0) {
            if (D_800C3EAC[0x2CC] != 0xFF && D_800D2C34[0] != 4 && D_800D2C34[0] < D_800C3EAC[0x2CC] + 1) {
                ok = 0;
            }
        } else {
            ok = 0;
        }
    }
    return ok;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086C88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086F98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800877E0);
/* 59.15%: retail recomputes `D_800C3EB4[a*0x1C] * 0x40` and `[b*0x1C] * 8`
   inside the loop on every one of the three blocks, while gcc hoists them into
   the loop preheader as loop invariants (the difference is ~60 bytes). Spelling
   the index inline (`D_800C3EB4[a * 0x1C]`, no ofsA/ofsB locals, 62.57%) makes
   gcc form `la D_800C3EB4` base pointers instead of retail's per-access
   lui `%hi` + addu + `%lo`. Also open: retail materialises `+ 0x140` with an
   explicit addiu before the `+ i` in the first block (it re-reads through the
   same $a0) while ours folds 0x140 into the load displacement.
s32 func_800877E0(s32 arg0, s32 arg1) {
    s32 i;
    s32 o;
    u8 a;
    u8 b;
    s32 ofsA;
    s32 ofsB;
    s32 res;

    res = 0;
    for (i = 0, o = 0; i < 9; i++, o += 6) {
        *(u16 *)(D_800C48EC + o) = 0xFFFF;
        *(u16 *)(D_800C48EE + o) = 0xFFFF;
    }
    a = arg0;
    ofsA = a * 0x1C;
    b = arg1;
    ofsB = b * 0x1C;
    *(u16 *)(D_800C48EC + 0) = *(u16 *)(D_800C3EBE + ofsA);
    *(u16 *)(D_800C48EE + 0) = *(u16 *)(D_800C3EC0 + ofsA);
    D_800C48F0[0] = 0;
    o = 6;
    for (i = 1; i < 8; i++, o += 6) {
        if (*(u8 *)(D_800D3364 + D_800C3EB4[ofsA] * 0x40 + D_800C3EB4[ofsB] * 8 + 0x140 + i) == 0xFF) {
            break;
        }
        *(u16 *)(D_800C48EC + o) =
            *(u16 *)(D_800D3364 + ((*(u8 *)(D_800D3364 + D_800C3EB4[ofsA] * 0x40 + D_800C3EB4[ofsB] * 8 + 0x140 + i) & 7) << 5));
        *(u16 *)(D_800C48EE + o) =
            *(u16 *)(D_800D3364 + ((*(u8 *)(D_800D3364 + D_800C3EB4[ofsA] * 0x40 + D_800C3EB4[ofsB] * 8 + 0x140 + i) & 7) << 5) + 2);
        D_800C48F0[o] = *(u8 *)(D_800D3364 + D_800C3EB4[ofsA] * 0x40 + D_800C3EB4[ofsB] * 8 + 0x140 + i) & 0x80;
    }
    if (D_800D2D24[a] == 4) {
        res = 1;
    }
    return res;
}
*/
void func_800879A8(s32 arg0, s32 arg1) {
    s32 i;
    s32 o;

    for (i = 0; i < 0x20; i++) {
        o = i * 0x48;
        D_800C402F[o] = 0xFF;
        D_800C400B[o] = arg0;
        *(u16 *)(D_800C3FFE + o) = func_80089C08(arg1);
    }
}
void func_80087A38(u8 arg0) {
    s32 idx;
    s32 r;

    func_80085D34();
    idx = arg0 * 0x40;
    func_800879A8(arg0, (D_800C3EAC + idx)[0x3C]);
    D_800D366C = 0;
    r = func_800877E0(arg0, (D_800C3EAC + idx)[0x3C]);
    func_800B89FC(r, arg0, (D_800C3EAC + idx)[0x3C], func_80080AE4(arg0));
    D_800D366C = 1;
    D_800C3E18 = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80087AF0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80087EDC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800881B8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800883AC);
/* 92.28%: instruction multiset identical; gcc hoists the func_80089C48 argument
   load into the load-delay slot after the D_800D301C load while retail leaves a
   nop there and loads the argument after the store.
void func_800883AC(u8 arg0) {
    s32 ofs;
    u8 sel;
    u8 ret;

    sel = (arg0 >= 3) * 8;
    if (D_800D32A1[arg0 * 8] != 0) {
        sel |= 0x10;
    }
    ofs = arg0 * 28;
    D_800D301C[(D_800C3EB4[ofs] + sel) << 2]--;
    ret = func_80089C48(D_800C3EB5[ofs]);
    D_800D301D[(D_800C3EB4[ofs] + sel) << 2] &= ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80088490);
/* 63.19%: first block and the five `D_800C3EB4[ofs]` reloads match retail exactly;
   still open: gcc branches on the skip condition (`beq $2,$0,store`, loop as
   fall-through) where retail branches INTO the loop (`bnez $2,loop`) and puts
   `li a1,0x40` in that delay slot; gcc keeps `n` in $a2 for the whole function
   (retail loads it into $v1 and copies to $a2 on the skip and break paths, which
   also makes retail's loop-exhaust path store $a2); gcc emits the
   `D_800D301C` flag store before `D_800D301D` (swap them in the source does not
   change it - sched2 reorders); and `D_800D3364 + idx*8` gives `addu idx,base`
   where retail has `addu base,idx` (writing `idx*8 + D_800D3364` does not change
   it either). `* 4` instead of `<< 2` scores 68.19% but folds the +0x10 into
   `%lo` (`lbu v0,64(at)`) instead of retail's `addiu`/`sll` pair, i.e. it is a
   wrong instruction choice that the fuzzy score likes; `<< 2` reproduces the
   shape func_800885D0 matched with. Dropping the `ofs` local and spelling
   `arg0 * 0x1C` inline is worse (56.81%: gcc switches to a `la` base pointer and
   loads `D_800D3364` twice).
void func_80088490(u8 arg0) {
    s32 ofs;
    s32 i;
    s32 n;

    ofs = arg0 * 0x1C;
    n = D_800C3EB4[ofs];
    if (D_800D301C[(n + 0x10) << 2] != 0) {
        for (i = 0; i < 8; i++) {
            if (D_800D301C[(i + 0x10) << 2] == 0) {
                n = i;
                break;
            }
        }
    }
    D_800C3EB4[ofs] = n;
    D_800C3EB5[ofs] = 0;
    D_800D301D[(D_800C3EB4[ofs] + 0x10) << 2] = 1;
    D_800D301C[(D_800C3EB4[ofs] + 0x10) << 2] = 1;
    *(u16 *)(D_800C3EBE + ofs) = *(u16 *)(D_800D3364 + D_800C3EB4[ofs] * 8 + 0x100);
    *(u16 *)(D_800C3EC0 + ofs) = *(u16 *)(D_800D3364 + D_800C3EB4[ofs] * 8 + 0x102);
}
*/
u8 func_800885D0(u8 arg0) {
    return D_800D301C[(D_800C3EB4[arg0 * 28] + 0x18) << 2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_8008860C);
void func_8008887C(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 dx;
    s32 dy;

    D_800C3A7C = arg0;
    D_800C3A80 = arg1;
    D_800C3A84 = arg2;
    D_800C3A88 = arg3;
    if (arg2 == arg0 || arg3 == arg1) {
        return;
    }
    if (arg2 < arg0) {
        dx = arg0 - arg2;
        D_800C3A94 = 1;
    } else {
        dx = arg2 - arg0;
        D_800C3A94 = 0;
    }
    if (arg3 < arg1) {
        dy = arg1 - arg3;
        D_800C3A98 = 1;
    } else {
        dy = arg3 - arg1;
        D_800C3A98 = 0;
    }
    if (dy <= dx) {
        D_800C3A8C = 0x100;
        D_800C3A90 = (dy << 8) / dx;
    } else {
        D_800C3A90 = 0x100;
        D_800C3A8C = (dx << 8) / dy;
    }
    D_800C2080 = 0;
    D_800C2084 = 0;
    D_800C3A9C = func_8001BD40(1, 8);
    D_800C207C = 0;
}
