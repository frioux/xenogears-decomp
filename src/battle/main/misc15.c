#include "common.h"

extern u8 *D_800C3EAC;
extern u8 D_800C3CF4[];
extern u8 *D_800D2D28;
extern u8 D_800D366C;
extern int ArchiveDataSync(void);
extern void func_8008AA40(u8 arg0);
extern void func_800716D8(void);
extern void func_8007765C(void);
extern void func_80077980(void);
extern void func_8008FA60(s32 arg0);
extern u16 func_80089C08(u8 arg0);
extern void func_800BC404(u16 arg0);
extern void func_800BCD98(u16 arg0);
extern u16 func_80089C9C(u16 arg0, u8 arg1);
extern u16 func_80089C48(u8 arg0);
extern u8 D_800C400B[];
extern u16 D_800C4022[];
extern u8 D_800C402F[];
extern u8 D_800CCD1C[];
extern u8 D_800D2CE0[];
extern u8 D_800D2204[];
extern u8 D_800D3688[];
extern u8 D_800C3D70[];
extern u8 func_80085084(u16 arg0, u8 arg1, s32 arg2);
extern s32 func_8009A7E4(u8 arg0);
extern void func_8009A854(u8 arg0);

void func_8008AA74(u8 arg0) {
    if (D_800D366C != 0) {
        func_8008AA40(arg0);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008AAA0);
/* Best: 66.72%, 43 insns / 172 bytes = the original's exact size. `u8 digit`
   is the key: it puts i=a1, divisor=a2, magic=a3, digit=v1 exactly as retail.
   Two diffs left, both in the divmod: retail has `mfhi a0` for `arg0 %= divisor`
   (the remainder reuses the parameter's a0) where gcc emits `mfhi v0; move
   a0,v0`, and retail's `srl a2,v0,3` (divisor /= 10) sits before the sb while
   gcc's reorg puts it in the bnez delay slot (retail has a nop there). u32/u16
   digit give `mfhi a0` but move i to v1 and divisor to a1. Tried: u32 digit,
   u16 digit, (u8) cast at the store, separate value local for the dividend,
   explicit `arg0 = arg0 % divisor`, three declaration orders.
void func_8008AAA0(u32 arg0) {
    s32 i;
    u32 divisor;
    u8 digit;
    u8 *p;

    divisor = 100000000;
    for (i = 0; i < 9; i++) {
        digit = arg0 / divisor;
        arg0 %= divisor;
        divisor /= 10;
        D_800C3CF4[i] = digit;
    }
    p = D_800C3CF4;
    for (i = 1; i < 9; i++, p++) {
        if (D_800C3CF4[i] == 0) {
            *p = 0xFF;
        } else {
            if (*p == 0) {
                *p = 0xFF;
            }
            break;
        }
    }
}
*/
void func_8008AB4C(void) {
    ArchiveSetIndex(0x20, 0);
}
void func_8008AB70(void) {
    ArchiveSetIndex(0x20, 2);
}
void func_8008AB94(void) {
    ArchiveSetIndex(0x20, 3);
}
s32 func_8008ABB8(s32 arg0, s32 arg1) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc(arg0, arg1);
}
s32 func_8008AC00(s32 arg0) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc((arg0 + 3) * 26, 0);
}
void func_8008AC50(void) {
    while (ArchiveDataSync() != 0) {
        func_800716D8();
    }
}
void func_8008AC88(u16 arg0, u8 arg1) {
    u16 mask;
    s32 i;
    s32 off;
    u8 idx;

    mask = arg0 & 0xFFF8;
    for (i = 0, off = 0x450; i < 8; i++) {
        idx = i + 3;
        if (func_80089C9C(mask, idx) != 0) {
            if ((*(u16 *)(D_800CCD1C + off) & 0x800) == 0) {
                mask &= func_80089C48(idx);
            }
        }
        off += 0x170;
    }
    if (mask == 0) {
        return;
    }
    D_800C400B[D_800C3EAC[0x2DA] * 72] = arg1;
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF3;
    D_800C4022[D_800C3EAC[0x2DA] * 36] = mask;
    D_800C3EAC[0x2DA] = D_800C3EAC[0x2DA] + 1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008ADD0);
void func_8008B108(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = D_800D2D28[0xB3] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B168);
/* Best: 99.36%. Same tail as func_8008BC98: the two `D_800C3EAC + arg0*0x40`
   base adds come out index-first (`addu v1,s1,v1`, `addu s1,s1,v0`) where the
   original has base-first (`addu v1,v1,s1`, `addu v0,v0,s1`). Both operands die
   at each add, so it is a global.c tie-break. Tried: subscript vs pointer
   arithmetic for the entry address. Everything else is identical.
void func_8008B168(u8 arg0) {
    u16 value;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = D_800D2D28[0xB3] = 1;
    D_800D2D28[0xB7] = 1;
    value = func_80089C08(arg0) | func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]);
    func_800BC404(value);
    func_800BCD98(func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]));
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B224);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B478);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B908);
void func_8008BC40(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BC98);
/* Best: 99.35%. Parking note: two `addu` operand orders differ (original
   base-first `addu v1,v1,s1` / `addu v0,v0,s1`, ours index-first). Eight source
   spellings (subscript, pointer arith, u8(*)[0x40] cast, (arg0<<6), base+0x3C
   indexed, u32 integer add, off locals) all compile byte-identically, so this is
   the allocator picking which dying operand coalesces with the dest, not a source
   shape. Same tail in func_8008B168 / func_8008C3F0 / func_8008CD28.
void func_8008BC98(u8 arg0) {
    u16 value;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = 1;
    D_800D2D28[0xB7] = 2;
    value = func_80089C08(arg0) | func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]);
    func_800BC404(value);
    func_800BCD98(func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]));
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BD50);
/* Best: 79.27%, 94 insns / 376 bytes = the original's exact size. Verified
   against retail: every parameter is used after a call (s5/s4/s3 saves
   match byte-for-byte), and the index `arg2 * 2 + arg1` must be written
   inline at every use -- a saved `off` local moves it to s0 and drops the
   s4/s3 saves, because retail rematerialises the index after the calls.
   Remaining diff is the base address: retail materialises D_800D2CE0 into
   a0 (lui/addiu) and adds the index with a register addu, and re-loads
   *p in the arg3==0 branch; gcc folds the base into the load operand
   ($at form, 3 insns instead of 4). Tried: p local, pBase local, (u32)
   cast with and without parens (parens fold straight back to $at, no
   parens give the register form but reassociate base+arg2*2+arg1), off
   local. Also unmatched: the arg3 branch is emitted inverted (retail
   `bnez arg3` with the *p path falling through).
s32 func_8008BD50(u8 arg0, u8 arg1, u8 arg2, u8 arg3) {
    u8 value;
    u16 entry;
    u8 *p;
    u8 *pBase;
    s32 result;

    result = 0;
    pBase = D_800D2CE0;
    p = pBase + (arg2 * 2 + arg1);
    entry = *(u16 *)(D_800D2204 + *p * 0x10);
    if (arg3 == 0) {
        value = *p;
    } else {
        value = D_800C3D70[arg2 * 2 + arg1];
    }
    if (value == 0) {
        func_8008AA74(0x4F);
    } else if (arg3 == 0) {
        func_8008BC40(1);
        D_800D2D28[0xC6] = 1;
        if (func_80085084(entry, arg0, 0) != 0) {
            result = 1;
        } else {
            D_800D2D28[0xC6] = 0;
            func_8008BC98(arg0);
        }
    } else {
        if (func_8009A7E4(value - 0x32) != 0) {
            func_8009A854(value);
            value = D_800D3688[arg2 * 2 + arg1] - 1;
            D_800D3688[arg2 * 2 + arg1] = value;
            if (value == 0) {
                D_800C3D70[arg2 * 2 + arg1] = 0;
            }
            result = 2;
        }
    }
    return result;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BED8);
void func_8008C360(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 != 0) {
        D_800D2D28[0xC6] = 0;
        func_8008FA60(0);
        func_8008FA60(1);
        func_800716D8();
        func_8007765C();
        func_80077980();
    } else {
        D_800D2D28[0xB0] = D_800D2D28[0xB1] = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C3F0);
/* Best: 99.36%. Same tail as func_8008BC98: the two `D_800C3EAC + arg0*0x40`
   base adds come out index-first (`addu v1,s1,v1`, `addu s1,s1,v0`) where the
   original has base-first (`addu v1,v1,s1`, `addu v0,v0,s1`). Both operands die
   at each add, so it is a global.c tie-break. Tried: subscript vs pointer
   arithmetic for the entry address. Everything else is identical.
void func_8008C3F0(u8 arg0) {
    u16 value;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB7] = 3;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = 1;
    value = func_80089C08(arg0) | func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]);
    func_800BC404(value);
    func_800BCD98(func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]));
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C4A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C81C);
void func_8008CCCC(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008CD28);
/* Best: 99.36%. Same tail as func_8008BC98: the two `D_800C3EAC + arg0*0x40`
   base adds come out index-first (`addu v1,s1,v1`, `addu s1,s1,v0`) where the
   original has base-first (`addu v1,v1,s1`, `addu v0,v0,s1`). Both operands die
   at each add, so it is a global.c tie-break. Tried: subscript vs pointer
   arithmetic for the entry address. Everything else is identical.
void func_8008CD28(u8 arg0) {
    u16 value;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = 1;
    D_800D2D28[0xB7] = 4;
    value = func_80089C08(arg0) | func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]);
    func_800BC404(value);
    func_800BCD98(func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]));
}
*/

