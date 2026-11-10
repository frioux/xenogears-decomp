#include "common.h"
#include "main/game.h"

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
extern u8 D_800D301D[];
extern u8 D_800C3EB5[];
extern u8 D_800C4000[];
extern u16 D_800C3FE8[];
extern u8 D_800D2C34;
extern u8 D_800D2D24[];
extern u8 D_800C34CC[];
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

extern void func_80085454(u8);
extern void func_80085618(u8);
extern void func_80098C6C(u16);
extern void func_800941A4(void);
extern u16 func_80089C08(u8);
extern u16 func_80089C48(u8);
extern void func_80085D34(void);
extern s32 func_800877E0(s32, s32);
extern s32 func_80080AE4(s32);
extern void func_800B89FC(s32, s32, s32, s32);
extern s32 func_8009ADA0(u8, s32 *);
extern void func_80085388(void);
extern void func_800BE538(u8, s32, s32, s32);
extern s32 func_80089C6C(s32, u8);
extern s32 func_8001BD40(s32, s32);

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
void func_80085B58(s32 arg0) {
    s32 buf[3];
    s32 i;
    u8 idx;

    idx = arg0;
    buf[0] = buf[1] = buf[2] = 0;
    if (func_8009ADA0(idx, buf) != 0) {
        for (i = 0; i < 3; i++) {
            if (buf[i] != 0) {
                D_800C3EAC[0x2DA] = 0;
                func_80085388();
                D_800C4000[idx] = i + 8;
                D_800C3FE8[idx] = *(u16 *)&buf[i];
                func_80085618(D_800C3EAC[0x2DA]);
            }
        }
        func_800BE538((u8)arg0, buf[0], buf[1], buf[2]);
    }
}
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800861D0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086B88);
/* Best: 89.09%. Same instruction multiset, three position differences: the
   original holds &D_800D2C34 in s2 across the call (`lbu v1,0(s2)`) where gcc
   rematerialises `lui/lbu`; it computes `i = arg0 + (cnt+1)*3` before loading
   D_800C3EAC[0x2D6] and puts `li s0,1` in the beqz slot, ours does the reverse.
   A `u8 *pCfg = &D_800D2C34` local does not stop the rematerialisation, and the
   `s32 mode = 4` local is needed to keep the `s1 = 4` register (that part
   matches).
s32 func_80086B88(s32 arg0, s32 arg1) {
    s32 i;
    s32 ret;

    i = arg0 + (D_800C3EAC[0x2CC] + 1) * 3;
    ret = 1;
    if (D_800C3EAC[0x2D6] != 0) {
        u8 *pCfg = &D_800D2C34;
        s32 mode = 4;

        if (*pCfg == mode) {
            i = arg0 + 0xC;
        }
        if ((u16)func_80089C6C((u16)g_GameState.unk16BC[D_800D2D24[arg1 & 0xFF]].unk8,
                               D_800C34CC[i & 0xFF]) == 0) {
            ret = 0;
        } else if (D_800C3EAC[0x2CC] != 0xFF && *pCfg != mode &&
                   *pCfg < D_800C3EAC[0x2CC] + 1) {
            ret = 0;
        }
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086C88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086F98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800877E0);
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
/* Best: 92.28%. Everything identical except the `lbu a0, D_800C3EB5(o)` load:
   the original leaves a nop in the load-delay slot of `lbu v0, D_800D301C(idx)`
   and reads the call argument after the `sb`, ours steals it into the slot.
   Tried: statement split, the arg in a u16 local, plain compound `&=`.
void func_800883AC(u8 arg0) {
    u8 off;
    s32 o;

    off = (arg0 >= 3) * 8;
    if (D_800D32A1[arg0 * 8] != 0) {
        off |= 0x10;
    }
    o = arg0 * 28;
    D_800D301C[(D_800C3EB4[o] + off) << 2]--;
    D_800D301D[(D_800C3EB4[o] + off) << 2] &= func_80089C48(D_800C3EB5[o]);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80088490);
u8 func_800885D0(u8 arg0) {
    return D_800D301C[(D_800C3EB4[arg0 * 28] + 0x18) << 2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_8008860C);
void func_8008887C(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 w;
    s32 h;

    D_800C3A7C = arg0;
    D_800C3A80 = arg1;
    D_800C3A84 = arg2;
    D_800C3A88 = arg3;
    if (arg2 == arg0 || arg3 == arg1) {
        return;
    }
    if (arg2 < arg0) {
        w = arg0 - arg2;
        D_800C3A94 = 1;
    } else {
        w = arg2 - arg0;
        D_800C3A94 = 0;
    }
    if (arg3 < arg1) {
        h = arg1 - arg3;
        D_800C3A98 = 1;
    } else {
        h = arg3 - arg1;
        D_800C3A98 = 0;
    }
    if (w < h) {
        D_800C3A90 = 0x100;
        D_800C3A8C = (w << 8) / h;
    } else {
        D_800C3A8C = 0x100;
        D_800C3A90 = (h << 8) / w;
    }
    D_800C2080 = 0;
    D_800C2084 = 0;
    D_800C3A9C = func_8001BD40(1, 8) & 0xFF;
    D_800C207C = 0;
}
