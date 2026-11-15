#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern u8 *D_800C3EAC;

extern u8 *D_800C3DE8;
extern u8 *D_800D2D28;
extern u8 *D_800D2DB4;
extern void *D_800D367C;
extern u8 *D_800D3278;
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800D366C;
extern u8 D_800D32A1[];
extern u8 D_800D2DD7[];
extern u8 D_800D2DD8[];
extern u8 D_800C204C;
extern u8 D_800C3202[];
extern u8 *D_800C3EA4;
extern volatile s32 D_800CCB34;

extern s32 BattleAllocHeapBlock(s32, s32);
extern void BattleSetArchive20Entry3(void);
extern void BattleReleaseEffectSlotIfChar4(u8);
extern void BattleWaitForArchiveSync(void);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void HeapFree(void *pMem);
extern void bzero(void *p, s32 n);
extern u8 *D_800C3E24;
extern u8 D_800D2C34;
extern u8 func_80084854(u8 arg0, u8 arg1);
extern void func_80093B08(u8 arg0);
extern u16 BattleSlotBitMask(u8 arg0);
extern void func_800BC404(u16 arg0);
extern void func_800BCD98(u16 arg0);
extern void func_80088490(u8 arg0);
extern void func_8009AEFC(u8 arg0);
extern void func_800BAF48(u8 arg0);
extern void func_800883AC(u8 arg0);
extern u8 D_800CCE42[];
extern u8 D_800C3EB8[];
extern u8 D_800D2D24[];
extern u8 D_80059179;
extern u8 D_800C4022[];
extern u8 D_800C3FFE[];
extern void BattleClearFlagC6(void);
extern void func_8009413C(u8 arg0, u8 arg1);
extern u8 D_800D2C0C[];
extern void func_8009A2D4(u8 arg0);
extern void BattleBuildSpritePrimLists(u8 arg0);
extern void func_800898F0(u8 arg0);
extern void BattleUpdateFrame(void);
extern u8 BattlePairTargetable(u8 arg0, u8 arg1);
extern void BattleClearSlotFlag15A1(u8 arg0);
extern void func_8009BAC4(u8 arg0, u8 *pInfo, u8 *pField);
extern u8 func_8001BD40(s32 arg0, u8 arg1);
extern void BattleInitSlotRowsForActor(s32 arg0, s32 arg1);
extern s32 func_800877E0(s32 arg0, s32 arg1);
extern void func_800B89FC(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern void func_80087AF0(u8 arg0, u8 arg1);
extern void func_8008ADD0(u8 arg0);
extern void BattleSetSlotFlag15A1(u8 arg0);
extern u8 D_800C3E18;
extern u8 D_800C3E2C;
extern u8 D_800C48EA;
extern u8 D_800CCD38[];
extern u8 D_800CDD53[];
extern u8 D_800D3014;
extern u8 D_800C3E29;
extern u8 func_8008B478(u8 arg0);
extern void BattlePlaySoundIfEnabled(u8 arg0);
extern void BattleSetupActorSlotDisplay(u8 arg0);
extern void func_80084A7C(u8 arg0);
extern void func_80077698(void);

/* 82.95%: best of 6 shapes. The loop body matches instruction for
   instruction (four reloads of the volatile counter, the two separate
   `lbu 0(a1)` of pSrc[i], the incrementing pSrc pointer in the branch delay
   slot) and so do the prologue and the tail. What is left is loop.c's
   invariant hoisting: retail hoists `lw a3,D_800C3EA4` and `li t2,0xC` into
   the preheader and leaves `li v1,0x12` in the body, while our gcc hoists
   neither (it keeps the base load and both constants in the body, 1 insn
   shorter) and puts the `li 0xC` in the body. Writing pSrc as a pre-loop
   local (74.02) puts its init before the loop test, which retail has after;
   declaring it inside the loop body (this shape) fixes the order and the
   incrementing pointer. A `u8 *pBase = D_800C3EA4;` pre-loop local does hoist
   the load but reorders the preheader (71.44). -O3 / -G0 / cdk change nothing
   material. */
/*
void func_8007FBE0(u8 arg0, u8 arg1) {
    u8 count;
    s32 i;

    count = arg1;
    if (arg0 == arg1) {
        count = arg1 - 1;
    }
    for (i = 0; i < arg0 - 1; i++) {
        u8 *pSrc = D_800C3202 + arg0 * 6;

        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x910) = 0xC;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x912) = pSrc[i] + 0x5E;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x914) = 0x12;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x916) = pSrc[i] + 0x5E;
    }
    D_800D2D28[0x97] = count;
    D_800D2D28[0x98] = *(u8 *)&D_800CCB34;
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FBE0);
void BattleFreeStringTableAE(void) {
    if (D_800D2D28[0xAE] != 0) {
        HeapFree(D_800D367C);
        D_800D2D28[0xAE] = 0;
    }
}

void BattleLoadStringTableAE(u8 arg0) {
    s32 type;

    if (D_800D2D28[0xAE] != 0) {
        return;
    }
    BattleWaitForArchiveSync();
    if (D_800D32A1[arg0 * 8] == 0) {
        type = 1;
    } else {
        type = 2;
    }
    BattleSetArchive20Entry3();
    D_800D367C = (void *)BattleAllocHeapBlock(ArchiveDecodeAlignedSize(type), 0);
    ArchiveReadFileToBuffer(type, D_800D367C, 0, 0x80);
    BattleWaitForArchiveSync();
    D_800D2D28[0xAE] = 1;
}

void BattleFreeStringTable96(void) {
    if (D_800D2D28[0x96] != 0) {
        HeapFree(D_800C3DE8);
        D_800D2D28[0x96] = 0;
    }
}

void BattleLoadStringTable96(void) {
    s32 pBuffer;

    if (D_800D2D28[0x96] == 0) {
        BattleWaitForArchiveSync();
        BattleSetArchive20Entry3();
        pBuffer = BattleAllocHeapBlock(ArchiveDecodeAlignedSize(3), 0);
        D_800C3DE8 = (u8 *)pBuffer;
        ArchiveReadFileToBuffer(3, (void *)pBuffer, 0, 0x80);
        BattleWaitForArchiveSync();
        D_800D2D28[0x96] = 1;
    }
}

void BattleAllocDrawBuffer(void) {
    u8 *pBuffer;

    pBuffer = (u8 *)BattleAllocHeapBlock(0x5DA4, 0);
    D_800D2DB4 = pBuffer;
    bzero(pBuffer, 0x5DA4);
    *(volatile u16 *)(D_800D2DB4 + 0x5D9C) = 0xA0;
    *(volatile u16 *)(D_800D2DB4 + 0x5D9E) = 0x64;
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FF14);
/* 91.69%: only a register rotation left (retail v=$a1, digits=$a0, i*4=$a2,
   quotient=$a3, mfhi=$v0; ours shifts by one). 12 hand shapes tried plus a
   4-minute permuter run (best score 948 vs base 971, and its "fix" was a no-op
   if/else, so nothing to port). cdk gives 93.74, so not a preset question. */
/*
void func_8007FF14(u8 arg0) {
    s32 i;
    u8 v;
    s32 n;
    s32 digits;

    func_8009A2D4(arg0);
    for (i = 2; i < 5; i++) {
        v = D_800D2C0C[(i - 2) * 2];
        *(u8 *)(D_800C3EAC + (arg0 << 6) + i * 4 + 8) = 0xFF;
        *(u8 *)(D_800C3EAC + (arg0 << 6) + i * 4 + 9) = 0xFF;
        *(u8 *)(D_800C3EAC + (arg0 << 6) + i * 4 + 0xA) = 0xFF;
        n = v / 100;
        digits = 0;
        if (n != 0) {
            v -= n * 100;
            *(u8 *)(D_800C3EAC + (arg0 << 6) + i * 4 + 8) = n + 0x83;
            digits = 1;
        }
        n = v / 10;
        if (n != 0 || digits != 0) {
            v -= n * 10;
            *(u8 *)(D_800C3EAC + (arg0 << 6) + (i * 4 + digits) + 8) = n + 0x83;
            digits++;
        }
        *(u8 *)(D_800C3EAC + (arg0 << 6) + (i * 4 + digits) + 8) = v + 0x83;
    }
    BattleAllocDrawBuffer();
    if (D_800D2D24[arg0] != 7) {
        BattleBuildSpritePrimLists(arg0);
        BattleUpdateFrame();
        func_800898F0(arg0);
        BattleUpdateFrame();
        D_800D2D28[0xAD] = 1;
        D_800D2D28[0xC7] = 1;
    }
}*/



void BattleResetActorDrawState(u8 arg0) {
    s32 index;

    D_800D2D28[0xAD] = 0;
    D_800D2D28[0xC7] = 0;
    D_800D2D28[0xA8] = 0;
    index = arg0 * 8;
    if (D_800D32A1[index] == 2) {
        D_800D32A1[index] = 1;
    }
    HeapFree(D_800D2DB4);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080160);
s32 BattleFindMinValueOtherSlot(u8 arg0) {
    s32 best = 0xFF;
    s32 i = D_800D2DD7[0];
    s32 sel;
    s32 idx;
    s16 *pSlots = (s16 *)(D_800D2DD7 + 0x2F);

    do {
        idx = D_800D2DD8[i];
        i++;
        if (*(pSlots + idx) < best && idx != arg0) {
            sel = idx;
            best = *(u8 *)(pSlots + idx);
        }
        if (i == 0xB) {
            i = 0;
        }
    } while (i != D_800D2DD7[0]);
    return sel;
}

void BattleQueueCommandFE(u8 color) {
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFE;
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = color;
    D_800D366C = 0;
}

void BattleSetFlag2DBAndReleaseAssets(void) {
    D_800C3EAC[0x2DB] = 1;
    if (D_800C3EAC[0x2D3] < 3 && D_800C204C == 0) {
        BattleFreeStringTableAE();
        BattleFreeStringTable96();
        BattleResetActorDrawState(D_800C3EAC[0x2D3]);
        BattleReleaseEffectSlotIfChar4(D_800C3EAC[0x2D3]);
    }
}

void BattleSetActorFlag34(u8 index) {
    u8 *pEntry = D_800D3278 + index * 0x38;

    pEntry[0x34] = 1;
}
/* 86.16%: first full reconstruction, every block present and in retail's
   order. Remaining: (a) the BattlePairTargetable fill loop - retail copies i to a
   second reg (`move s1,s0`) and increments i in the beqz delay slot so the
   store uses the old i, we store i directly, 1 insn short; (b) case 1 reuses
   the D_800C3EAC load from the D_800C4022 store for the +0x3C read (we
   reload, +2 insns); (c) case 2: retail keeps (info[1]+0x16)*0x28 undistributed
   and re-forms D_800CDD53/D_800CCD38 as sym+reg memory operands twice each,
   our cse folds the 0x16*0x28 into the symbol and shares the address in a
   register (the same cse-address-sharing wall as func_800826CC); (d) arg0
   lives in s3 in retail, s2 here. */
/*
u8 func_80080C94(u8 arg0) {
    u8 info[8];
    u8 buf[0xB];
    u8 *pBuf;
    u8 ret = 1;
    u8 cost;
    s32 idx;
    s32 r;
    s32 i;
    s32 sel;

    BattleUpdateFrame();
    for (i = 0xA; i >= 0; i--) {
        buf[i] = 0xFF;
    }
    D_800C3EAC[0x2EA] = 1;
    D_800C3EAC[0x2DE] = 0;
    BattleClearSlotFlag15A1(arg0);
    func_8009BAC4(arg0, info, D_800C3EAC + arg0 * 0x40 + 0x2A);
    if (info[0] == 1 || info[0] == 2) {
        i = 0;
        pBuf = buf;
        for (i = 0; i < 0xB; i++) {
            if (BattlePairTargetable(arg0, i) != 0) {
                *pBuf++ = i;
            }
        }
        do {
            do {
                sel = buf[func_8001BD40(0, 0xA)];
                D_800C3E2C = sel;
            } while (sel == 0xFF);
        } while (sel == arg0);
    }
    BattleInitSlotRowsForActor(arg0, D_800C3E2C);
    idx = arg0 * 0x40;
    (D_800C3EAC + idx)[0x3C] = D_800C3E2C;
    func_800BC404(BattleSlotBitMask(arg0) | BattleSlotBitMask((D_800C3EAC + idx)[0x3C]));
    BattleUpdateFrame();
    D_800C204C = 1;
    switch (info[0]) {
    case 1:
        D_800C3E18 = 0;
        if (D_800D2D24[arg0] != 4) {
            D_800C400B[D_800C3EAC[0x2DA] * 0x48] = arg0;
            D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFD;
            *(u16 *)(D_800C4022 + D_800C3EAC[0x2DA] * 0x48) = 0;
            *(u16 *)(D_800C3FFE + D_800C3EAC[0x2DA] * 0x48) =
                BattleSlotBitMask((D_800C3EAC + idx)[0x3C]);
            D_800C3EAC[0x2DA]++;
        }
        if (D_800D32A1[arg0 * 8] == 0) {
            D_800C3EAC[0x2DC] = 0;
        } else {
            D_800C3EAC[0x2DC] = info[1];
        }
        ret = 0;
        r = func_800877E0(arg0, (D_800C3EAC + idx)[0x3C]);
        func_800B89FC(r, arg0, (D_800C3EAC + idx)[0x3C], BattleFindMinValueOtherSlot(arg0));
        func_80087AF0(arg0, info[1] + 1);
        BattleQueueCommandFE(arg0);
        break;
    case 2:
        cost = D_800CDD53[(info[1] + 0x16) * 0x28 + arg0 * 0x5F0];
        if (*(u16 *)(D_800CCD38 + arg0 * 0x170) >= cost) {
            *(u16 *)(D_800CCD38 + arg0 * 0x170) -= cost;
            ret = 0;
            D_800C3EAC[0x2E6] = info[1];
            func_8008ADD0(arg0);
        }
        break;
    case 4:
        func_800826CC(arg0);
        ret = 0;
        D_800C3EAC[0x2DE] = 1;
        break;
    default:
        break;
    }
    if (ret != 0) {
        BattleSetSlotFlag15A1(arg0);
        D_800C3EAC[0x2EA] = 0;
        D_800C3EAC[0x2DE] = 1;
    }
    while (D_800C3EAC[0x2DE] == 0 && D_800C48EA == 0 && D_800C3EAC[0x2DB] != 0) {
        BattleUpdateFrame();
    }
    D_800C204C = 0;
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080C94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8008115C);
/* 88.35%: every case body is instruction-identical to retail except two
   block-placement differences. (a) gcc cross-jumps all four plain
   `D_800C3EAC[0x2DD] = N;` case tails into one shared store block at the end of
   the function; retail keeps each store inline and only shares the
   BattlePlaySoundIfEnabled call. (b) that shared call block: retail is
   `jal; nop` with `li a0,0x4F` sitting in each predecessor's branch delay slot,
   ours is `jal; li a0,0x4F` (reorg always steals the constant into the jal slot
   when it is the last insn before the call). Same two shapes, unresolved, in
   func_800816F8 (99.90%, JTBL-only there). Tried: literal vs. local argument,
   both if/else orders, p/off locals vs. inline D_800C3EAC indexing. */
/*
void func_8008115C(u8 arg0) {
    u8 a;
    u8 *p;
    s32 off;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        p = D_800C3EAC;
        off = a * 0x40;
        if (p[off + 0x3C] == 0xFF) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            D_800D366C = 0;
            BattleSetupActorSlotDisplay(a);
            func_80084A7C(a);
            func_80077698();
            D_800C3EAC[0x2DD] = 5;
        }
        break;
    case 1:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x32) != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            D_800C3EAC[0x2DD] = 2;
        }
        break;
    case 2:
        D_800C3EAC[0x2DD] = 3;
        break;
    case 3:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x30) == 0) {
            D_800C3EAC[0x2DD] = 4;
            break;
        }
        if (p[0x2F6] != 0 && D_800C3E29 == 3) {
            if (*(u16 *)(p + off + 0x2A) != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                D_800C3EAC[0x2DD] = 0xA;
            }
            D_800C3EAC[0x2F6] = 0;
        } else {
            D_800C3EAC[0x2F6] = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 0:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x26) != 0) {
            BattlePlaySoundIfEnabled(0x4F);
        } else {
            D_800C3EAC[0x2DD] = 7;
        }
        break;
    }
}*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081318);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081504);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800816F8);
/* 99.90% and JTBL_LAYOUT: with the switch converted, the C jump table is
   emitted with `.align 3` at object offset 0x1F4, so cc1 inserts a 4-byte pad
   word and the unit's .rodata becomes 0x278 instead of 0x274 - check_bin.sh
   battle then reports MISMATCH (build/out/battle.bin is 4 bytes long) while
   every instruction of the function is byte-identical. The table cannot be
   moved to an 8-aligned offset: the three 0x20-byte asm tables before it
   (func_8008115C/80081318/80081504) plus func_80080160's 0x194 table put it at
   0x1F4, and the backend hardcodes the table alignment. Only convertible
   together with a change that makes the offset 8-aligned. Best body kept. */
/*
void func_800816F8(u8 arg0) {
    u8 a;
    u8 *p;
    s32 off;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        BattleLoadStringTableAE(a);
        if (func_8008B478(a) != 0) {
            func_8008ADD0(a);
        } else {
            D_800C3EAC[0x2DD] = 4;
        }
        break;
    case 0:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x2E) == 0) {
            p[0x2DD] = 1;
            break;
        }
        if (p[0x2F6] != 0 && D_800C3E29 == 0) {
            if (*(u16 *)(p + off + 0x26) != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                D_800C3EAC[0x2DD] = 7;
            }
            D_800C3EAC[0x2F6] = 0;
        } else {
            D_800C3EAC[0x2F6] = 1;
            BattlePlaySoundIfEnabled(0x4F);
        }
        break;
    case 1:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x32) == 0) {
            p[0x2DD] = 2;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        D_800C3EAC[0x2DD] = 3;
        break;
    case 3:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x2A) == 0) {
            p[0x2DD] = 0xA;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}*/

void BattleInitTargetSlotFlags(s32 arg0) {
    s32 i;

    if (D_800C3EAC[0x2E9] != 0) {
        return;
    }
    func_800BC404(BattleSlotBitMask(D_800C3EAC[0x2E8]));
    func_800BCD98(BattleSlotBitMask(D_800C3EAC[0x2E8]));
    i = 0;
    do {
        if (func_80084854(D_800C3EAC[0x2E8], i) != D_800C3EAC[0x2E8]) {
            (D_800C3E24 + i)[0xE6] = 1;
        } else {
            (D_800C3E24 + i)[0xE6] = 0;
        }
        i++;
    } while (i < 4);
    if (D_800D2C34 != 4) {
        func_80093B08(arg0);
    }
}
/* 92.34%: `BattleSlotBitMask` returns u16, so a separate `orBits = f(a); orBits |=
   f(b);` masks both results; writing the two calls as one `|` expression inside
   the func_800BC404 call lets gcc fold them into the single `or` + final `andi
   0xffff` the original has. What is left: the original loads D_800C3EAC once and
   shares the base between the `[0x2DA]` read and the `+arg0*0x40` pointer for
   the BattleSlotBitMask argument (we reload it), and the two `addu base,base,idx`
   come out with the index register as destination instead of the base
   (allocator tie, both values dead after). */
/*
void func_800819A4(u8 arg0) {
    s32 orBits;

    if (D_800C3EAC[0x2E9] != 0) {
        return;
    }
    D_800C3EAC[0x2E9] = 1;
    func_800BCD98(0);
    *(u8 *)((u32)D_800C3EAC + arg0 * 0x40 + 0x3C) = D_800C3EAC[0x2E8];
    BattleFreeStringTableAE();
    BattleFreeStringTable96();
    BattleClearFlagC6();
    orBits = BattleSlotBitMask(arg0) | BattleSlotBitMask(D_800C3EAC[arg0 * 0x40 + 0x3C]);
    func_800BC404(orBits);
    if (D_800D2D24[arg0] != 4) {
        D_800C400B[D_800C3EAC[0x2DA] * 0x48] = arg0;
        D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFD;
        *(u16 *)(D_800C4022 + D_800C3EAC[0x2DA] * 0x48) = 0;
        *(u16 *)(D_800C3FFE + D_800C3EAC[0x2DA] * 0x48) =
            BattleSlotBitMask(D_800C3EAC[arg0 * 0x40 + 0x3C]);
        D_800C3EAC[0x2DA]++;
    }
    func_8009413C(arg0, 0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800819A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081B58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800820A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800822C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80082504);
/* 84.29%: every instruction after the flag update matches. The blocker is the
   flag update itself: retail has the gas `sym+reg` form twice
   (`lui at,%hi(D_800CCE42); addu at,at,v0; lbu/sb %lo(at)`, i.e. the base
   rematerialised into $at for each access, 4 more insns) while cse in our gcc
   makes one `la $3,D_800CCE42` and shares it (`addu v0,v0,v1`), 8 bytes
   shorter, so the size cannot match either. gcc does emit the `sym+reg` macro
   form for D_800D32A1/D_800D2D24 in this same function; it registers an
   address only where it is used twice. Tried: plain `|=`, array-ref, pointer
   local, volatile. */
/*
void func_800826CC(u8 arg0) {
    s32 i;

    func_80088490(arg0);
    func_8009AEFC(arg0);
    func_800BAF48(arg0);
    D_800CCE42[arg0 * 0x170] |= 0x80;
    func_800883AC(arg0);
    D_800D32A1[arg0 * 8] = 2;
    if (D_800D2D24[arg0] != 7) {
        *(u8 *)(arg0 * 0x1E4 + (u32)D_800C3EA4 + 0x853D) = 2;
    }
    D_800C3EB8[arg0 * 0x1C] = 1;
    D_800C3EAC[arg0 + 0x2EB] = 1;
    for (i = 0; i < 3; i++) {
        if (g_GameState.partyMembers[i] == D_800D2D24[arg0] && D_80059179 == 0) {
            g_GameState.gearRide[i] = 1;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800826CC);
