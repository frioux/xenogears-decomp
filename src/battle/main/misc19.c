#include "common.h"
#include "main/game.h"

extern void* D_800C34B0;
extern u8 D_800C3E50;
extern u8* D_800C3DFC;
extern u8* D_800D2DC8;
extern u8 D_800CCE30[];
extern u8 D_800CCE4A[];
extern u8 D_800CCCE8[];
extern u8 D_800D2C34;
extern u8* D_800C3E00;
extern u8 D_800CCD68[];

extern int rand(void);
extern void func_8009BE0C(void);
extern void func_8009B104(u8 arg0, u8* arg1);

s32 func_8009A0DC(u8 arg0) {
    u8* p = (u8*)(arg0 * 0x170 + (u32)D_800C34B0);
    u16 f7C = *(u16*)(p + 0x7C);
    u16 f80;

    if (f7C & 0x4000) {
        return 8;
    }
    if (f7C & 0x8000) {
        return 1;
    }
    if (f7C & 0x2000) {
        return 2;
    }
    f80 = *(u16*)(p + 0x80);
    if (f80 & 0x1000) {
        return 3;
    }
    if (f80 & 0x2000) {
        return 4;
    }
    if (f7C & 0x800) {
        return 5;
    }
    if (f7C & 0x1000) {
        return 6;
    }
    if (f7C & 0x2) {
        return 3;
    }
    if ((*(u16*)(p + 0x84) | *(u16*)(p + 0x86)) & 0x8000) {
        return 7;
    }
    return *(u16*)(p + 0x4C) < (*(u16*)(p + 0x4E) >> 3) ? 5 : 0;
}
u16 func_8009A1AC(u8 arg0) {
    u8* p = (u8*)(arg0 * 0x170 + (u32)D_800C34B0);
    u16 f7C = *(u16*)(p + 0x7C);
    u16 ret = 0;
    u16 f;

    if (f7C & 0x8000) {
        return 0;
    }
    f = *(u16*)(p + 0x80);
    if (f & 0x1000) {
        ret |= 0x8000;
    }
    if (f & 0x2000) {
        ret |= 0x4000;
    }
    if (f7C & 0x800) {
        ret |= 0x2000;
    }
    f = *(u16*)(p + 0x8C) | *(u16*)(p + 0x8E);
    if (f & 0xF00) {
        ret |= (f & 0xF00) >> 3;
    }
    if (f & 0xF000) {
        ret |= (f & 0xF000) >> 3;
    }
    return ret;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A258);
/* Best attempt: 92.90%. Structure and instruction multiset identical (the
   addiu 0x1058 of the row offset, the two strength-reduced multiplies and the
   clamp all line up); the only difference is that gcc puts the row-offset chain
   in $v0 and the arg1 chain in $v1 where the original has them in $v1/$v0, which
   cascades to the base pointer ($a1 vs $a3) and costs the original's
   `move v1,v0` for the u8 sum. Tried: one-expression form, `u8* q` slice,
   pointer-free `s32 off`, and a separate `s32 sum` + `u8 v = sum`.
u8 func_8009A258(u8 arg0, u8 arg1) {
    u8* p = D_800C34B0;
    s32 off = arg0 * 0x5F0 + 0x1058;
    u8 v = *(u8*)((u8*)p + off + arg1 * 0x28 + 0x14) + p[arg0 * 0x170 + 0x5B];

    if (v > 0x64) {
        v = 0x64;
    }
    return v;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A2D4);

u8 func_8009A7B8(u8 arg0) {
    return g_GameState.characters[arg0].unk0[5];
}

s32 func_8009A7E4(u8 arg0) {
    u8 id = *(u8*)((u8*)D_800C34B0 + arg0 * 16 + 0x581E);

    s32 ret;

    if (g_GameState.characters[4].unk0[6] == id) {
        ret = 1;
    } else if (g_GameState.characters[4].unk0[0xE] == id) {
        ret = 1;
    } else if (g_GameState.characters[4].unk0[0x16] == id) {
        ret = 1;
    } else {
        ret = g_GameState.characters[4].unk0[0x1E] == id;
    }
    return ret;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A854);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A9D0);
/* Best attempt: 99.66%. Only difference: the mfhi temp of the `% 100` expansion
   lands in $a1 instead of the original's $a0 (allocator tie-break, 2 insns).
   Tried: direct `rand() % 100 < 50`, a named `s32 r = rand()`, a named
   `s32 m = r % 100`, and `r % 100 <= 49` -- all 99.66%.
s32 func_8009A9D0(void) {
    *(u8*)((u8*)D_800C34B0 + 0x5FC2) = 0;
    if (rand() % 100 < 50) {
        func_8009BE0C();
        return 1;
    }
    return 0;
}
*/


void func_8009AA44(u8 arg0) {
    u8* p;
    u8* q;

    *(u8*)((u8*)D_800C34B0 + 0x5FC2) = 0;
    q = (u8*)D_800C34B0 + arg0 * 0x170;
    q[0x15A] |= 1;
    p = (u8*)D_800C34B0 + arg0 * 0x170;
    if ((*(u8*)(p + 0x15A) & 0x80) && (*(u16*)(p + 0x126) & 0x10)) {
        *(u16*)(p + 0x120) &= 0xFE4F;
        *(u16*)(p + 0x7C) &= 0xEFFF;
    }
    if (D_800D2C34 == 4) {
        *(u8*)((u8*)D_800C34B0 + 0x5FC7) = 0x3D;
    }
}

u8 func_8009AB00(u8 arg0) {
    u8* p = (u8*)D_800C34B0 + arg0 * 0x170;
    u8 flags = p[0x15A] & 0xFE;

    p[0x15A] = flags;
    return flags;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AB38);
/* Best attempt: 96.91%, identical size (272 bytes) and identical instruction
   multiset/order. The only difference is one allocator permutation at the
   second row recomputation: the original re-masks arg0 into $v1 and puts the
   D_800C34B0 base in $a0 (and the 3*arg0 partial in $a1), gcc masks arg0 in
   place in $a0 and puts the base in $a1 and the partial in $a2, which swaps
   $v0/$v1 in both store tails. Tried: plain `arg0 * 0x170` both times, an
   `s32 off = arg0 * 0x170` local, and a `u8 a = arg0;` copy -- all 96.91%.
void func_8009AB38(u8 arg0) {
    u8* p = (u8*)D_800C34B0 + arg0 * 0x170;
    u16 f;

    if (p[0x15A] & 0x80) {
        f = *(u16*)(p + 0x124) & 0x2000;
    } else {
        f = *(u16*)(p + 0x88) & 0x400;
    }
    if (f == 0) {
        return;
    }
    p = (u8*)D_800C34B0 + arg0 * 0x170;
    if (p[0x15A] & 0x80) {
        p = (u8*)D_800C34B0 + (arg0 * 0x690 + 0x2228);
        *(u16*)(p + 0x348) = 1;
        *(u16*)(p + 0x398) = 1;
        *(u16*)(p + 0x3C0) = 1;
        *(u16*)(p + 0x3E8) = 1;
        *(u16*)(p + 0x410) = 1;
        *(u16*)(p + 0x438) = 1;
        *(u16*)(p + 0x460) = 1;
    } else {
        p = (u8*)D_800C34B0 + (arg0 * 0x5F0 + 0x1058);
        *(u16*)(p + 0x370) = 1;
        *(u16*)(p + 0x3C0) = 1;
        *(u16*)(p + 0x3E8) = 1;
        *(u16*)(p + 0x410) = 1;
        *(u16*)(p + 0x438) = 1;
        *(u16*)(p + 0x460) = 1;
        *(u16*)(p + 0x488) = 1;
        *(u16*)(p + 0x4B0) = 1;
        *(u16*)(p + 0x4D8) = 1;
        *(u16*)(p + 0x500) = 1;
    }
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AC48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009ADA0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AEFC);
/* Best attempt: 88.27%. Everything matches except the 3..11 loop: the original
   re-forms `lui at,%hi(D_800CCD68); addu at,at,v0` at BOTH memory references,
   while gcc hoists the symbol base into a register before the loop (loop.c moves
   the invariant), which then CSEs the address and drops 7 insns. Tried: plain
   `*(u16*)(D_800CCD68 + i*0x170) &= 0xFFDF`, the `((u16*)(...))[0] &=` spelling
   (86.36%), `u8 t` vs `s32 t` for the 0x56 temp. Everything else (two D_800C34B0
   loads, the four zero stores, the `f & 0x2000` store and the func_8009B104 call)
   lines up exactly.
void func_8009AEFC(u8 arg0) {
    u8* p;
    u16 f;
    s32 t;

    *(u8*)((u8*)D_800C34B0 + 0x5FC2) = 0;
    p = arg0 * 0x170 + (u32)D_800C34B0;
    f = *(u16*)(p + 0x80);
    t = p[0x56];
    *(u16*)(p + 0x7C) = 0;
    *(u16*)(p + 0x84) = 0;
    *(u16*)(p + 0x88) = 0;
    *(u16*)(p + 0x8C) = 0;
    *(u16*)(p + 0x80) = f & 0x2000;
    if (t == 7) {
        func_8009B104(arg0, p);
    }
    if (p[0x56] == 3) {
        u8 i;

        for (i = 3; i < 11; i++) {
            *(u16*)(D_800CCD68 + i * 0x170) &= 0xFFDF;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AFD8);
/* Best attempt: 99.33%, same size (192 bytes). Jump table is
   case 0-3 -> shared tail, 4-5 -> return, 6 -> tail of the 7..19 block,
   7..19 -> the block that decrements gearPartsIds[0x6C + e00[0x6F]]. The only
   difference is that gcc duplicates the shared last decrement into the
   7..19 block instead of keeping one block that cases 0-3 jump to. `u8 idx`
   (93.92%) gives an extra andi; `s32 idx` is required.
*/
/* PARKED ATTEMPT (99.33%)
void func_8009AFD8(void) {
    u8 sel = *(u8*)((u8*)D_800C34B0 + 0x5FC2);
    s32 idx;

    switch (sel) {
    case 0:
    case 1:
    case 2:
    case 3:
        idx = *(u8*)(D_800C3E00 + 0x6F);
        break;
    case 4:
    case 5:
        return;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        idx = *(u8*)(D_800C3E00 + 0x6F);
        if (g_GameState.gearPartsIds[0x6C + idx]) {
            g_GameState.gearPartsIds[0x6C + idx]--;
        }
        idx = *(u8*)(D_800C3E00 + 0x72);
        break;
    }
    if (g_GameState.gearPartsIds[0x6C + idx]) {
        g_GameState.gearPartsIds[0x6C + idx]--;
    }
}

*/

void func_8009B098(void) {
    u8 i;

    for (i = 1; i < 3; i++) {
        u8* p = (u8*)(i * 0x170 + (u32)D_800C34B0);

        *(u16*)(p + 0x4C) = 0x64;
        *(u16*)(p + 0x4E) = 0x64;
        p[0x5E] = 0x14;
        p[0x5F] = 0xF;
        *(u16*)(p + 0x7A) = 0x1FBF;
        p[0x149] = 0;
    }
}

void func_8009B104(u8 arg0, u8* arg1) {
    u8* p = arg0 * 0x170 + (u32)D_800C34B0;
    u8* q = p + 0xA4;

    if (*(u16*)(arg1 + 0x4E) * 50 > 0x1869F) {
        if (*(u16*)(arg1 + 0x4C) * 50 > 0x1869F) {
            *(u32*)(p + 0x104) = 0x1869F;
        } else {
            *(u32*)(p + 0x104) = *(u16*)(arg1 + 0x4C) * 50;
        }
        *(u32*)(q + 0x64) = 0x1869F;
    } else {
        *(u32*)(p + 0x104) = *(u16*)(arg1 + 0x4C) * 50;
        *(u32*)(p + 0x108) = *(u16*)(arg1 + 0x4E) * 50;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B1E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B46C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B684);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BAC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BD94);

/* Best attempt: 97.33%. Everything matches except the mfhi temp of the `/ 20`
   expansion ($a1 instead of $v0) and the two registers that follow it.
   Also tried: a `u32 num` numerator local (83.67%) and a `u32* pDest` pointer
   local, which CSEs the D_800C34B0 base and comes out 100 bytes (70.17%).
void func_8009BD94(void) {
    u8* pBase = D_800C34B0;

    *(u8*)(pBase + D_800C3E50 + 0x5FA0) = 2;
    *(u32*)(D_800C3E50 * 4 + (u32)D_800C34B0 + 0x5F6C) =
        (D_800C3DFC[0x11] * *(u32*)(D_800D2DC8 + 0x64)) / 20;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BE0C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009C050);
/* Best attempt: 97.78%. Instruction multiset and all offsets match; the only
   difference is that gcc reassociates `base + 0xA4 + idx` into
   `base + (idx + 0xA4)`, so the two `addiu $x,base,0xA4/0x148` come out off the
   index register instead of off the base register (and the base lands in $a1
   instead of $a0). Splitting the bases into their own pointer locals
   (`pTimerBase = base + 0xA4; pTimer = pTimerBase + idx;`) gets the right shape
   but reorders everything and drops to 69.53%. Note `s32 flag = ... & 0x400;
   ret = flag != 0;` is required: the direct `(x & 0x400) != 0` becomes
   srl/andi instead of the original's andi/sltu.
u8 func_8009C050(u8 arg0) {
    s32 idx = arg0 * 0x170;
    u8* base = D_800CCCE8;
    u8* pTimer = base + 0xA4 + idx;
    u8* pRow = base + idx;
    u8* pState = base + 0x148 + idx;
    s32 flag = *(u16*)(pTimer + 0x7C) & 0x400;
    u8 ret = flag != 0;

    if (*(u32*)(pTimer + 0x60) < (*(u32*)(pTimer + 0x64) >> 3)) {
        if (!(*(u16*)(pRow + 0x36) & 1)) {
            ret |= 2;
        }
    }
    if (*pState == 4) {
        ret |= 4;
    }
    return ret;
}
*/

void func_8009C0E0(u8 arg0) {
    s32 off = arg0 * 0x170;

    D_800CCE30[off] = 4;
    D_800CCE4A[off] = 3;
    g_GameState.unk16BC[0].unk1E |= 0x4000;
}
