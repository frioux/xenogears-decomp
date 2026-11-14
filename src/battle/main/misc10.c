#include "common.h"

extern u8 D_800C3EB7[];
extern u16 D_800CCD64[];
extern u8 D_800D2D24[];
extern u8 *D_800D2D28;
extern u8 D_800D301C[];
extern u16 D_800D39DC;
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D3430[];

s32 BattleWordRegLeImm(u8 **arg0, u8 arg1) {
    u16 value = (*arg0)[2] | ((*arg0)[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return value >= pRow[(*arg0)[1]];
}
s32 BattleByteRegGeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return !(pRow[p[1]] < p[2]);
}
s32 BattleWordRegGeImm(u8 **arg0, u8 arg1) {
    u16 value = (*arg0)[2] | ((*arg0)[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return pRow[(*arg0)[1]] >= value;
}
s32 BattleByteRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return pRow[p[1]] == pRow[p[2]];
}
s32 BattleWordRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return pRow[p[1]] == pRow[p[2]];
}
s32 BattleByteRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return pRow[p[1]] <= pRow[p[2]];
}
s32 BattleWordRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return pRow[p[1]] <= pRow[p[2]];
}
s32 BattleByteRegTestImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    s32 v = pRow[p[1]];

    v &= p[2];
    return v != 0;
}
s32 BattleWordRegTestImm(u8 **arg0, u8 arg1) {
    u16 value = (*arg0)[2] + ((*arg0)[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return (pRow[(*arg0)[1]] & value) != 0;
}
s32 BattleByteRegTestReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    u8 lhs = pRow[p[1]];

    return (pRow[p[2]] & lhs) != 0;
}
s32 BattleWordRegTestReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    u16 a = pRow[p[1]];
    u16 b = pRow[p[2]];

    return (b & a) != 0;
}
s32 BattleByteRegNeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return (pRow[p[1]] ^ p[2]) != 0;
}
s32 BattleWordRegNeImm(u8 **arg0, u8 arg1) {
    u16 value = (*arg0)[2] | ((*arg0)[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return (pRow[(*arg0)[1]] ^ value) != 0;
}
s32 BattleByteRegNeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return (pRow[p[1]] ^ pRow[p[2]]) != 0;
}
s32 BattleWordRegNeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    return (pRow[p[1]] ^ pRow[p[2]]) != 0;
}
s32 BattleDwordRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);

    return (pRow[p[1]] ^ pRow[p[2]]) == 0;
}
s32 BattleDwordRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);

    return pRow[p[1]] <= pRow[p[2]];
}
s32 BattleTableEntryFlag8000(u8 **arg0) {
    u8 *p = *arg0;

    return D_800CCD64[p[1] * 0xB8] >> 15;
}
s32 BattleSlotCountIsZero(u8 **arg0) {
    u8 *p = *arg0;

    return D_800D301C[p[1] * 4] == 0;
}
s32 BattleStateWordLowBitsClear(void) {
    return (D_800D39DC & 3) == 0;
}
s32 func_8007EEE8(void) {
    s32 ret = 1;
    s32 i = 0;
    s32 ofs = 0x54;

    for (; i < 8; i++) {
        if ((D_800D39DC >> 5) != 0) {
            if ((D_800C3EB7[ofs] & 0x80) == 0) {
                ret = 0;
                break;
            }
        }
        ofs += 0x1C;
    }

    return ret;
}
s32 BattleActorSlotHasFlag80(u8 arg0) {
    s32 idx = arg0 + 3;

    return D_800C3EB7[idx * 28] >> 7;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007EF6C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007F8C0);
/* 92.62% - RODATA-BLOCKED, do not convert: the switch's table is jtbl_8006FE0C,
   which splat already emits inside the 0x133C-byte asm rodata blob
   (build/asm/battle/data/0.rodata.s, 132 tables). cc1 emits its own 0x70-byte
   table into misc10.c.o(.rodata); linker/battle.ld puts every unit's C .rodata
   after that blob and battle_TEXT_START immediately follows it, so the whole
   overlay shifts by 0x70 and make check fails while objdiff still reads ~93%.
   Unconvertible until the overlay's rodata block as a whole moves to C, and an
   if/else chain cannot reproduce the sltiu plus table dispatch.
   Code shape: 174 insns vs the original's 172. Remaining differences are the
   block ordering of the loop tail (retail: `or v1,s3,s0` fills the load-delay
   slot after `lw (*arg0)` and the back edge is `beqz v0,looptop` with
   `addu s3,v1,zero` in its delay slot; gcc emits `bnez v0,<store>` plus a `j`)
   and one extra nop. Tried: acc = ret | acc (91.16), ret = acc then break,
   for(;;) variants, inverting the `< 0x80` test to `>= 0x80 with continue`,
   a named `next` temp for acc | ret (92.01), and ((cont & 0xFF) == 0) to force
   the retail's `andi v0,s4,0xff` (92.62, the andi is right, the tail ordering
   is not).
   Needs these declarations, kept out of the file until it is matched:
     s32 func_8007E954(u8 **arg0, u8 arg1);
     s32 func_8007E98C(u8 **arg0, u8 arg1);
     s32 func_8007E9D0(u8 **arg0, u8 arg1);
     void BattleScriptCursorSkipWord(u8 **pCursor);

s32 func_8007F8C0(u8 **arg0, u8 arg1) {
    s32 ret = 0;
    s32 cont = 0;
    s32 acc = 0;
    s32 next;

    for (;;) {
        switch ((*arg0)[0]) {
        case 0x80:
        case 0x9A:
            ret = 1;
            break;
        case 0x81:
            ret = func_8007E954(arg0, arg1);
            break;
        case 0x82:
            ret = func_8007E98C(arg0, arg1);
            break;
        case 0x83:
            ret = func_8007E9D0(arg0, arg1);
            break;
        case 0x84:
            ret = BattleWordRegLeImm(arg0, arg1);
            break;
        case 0x85:
            ret = BattleByteRegGeImm(arg0, arg1);
            break;
        case 0x86:
            ret = BattleWordRegGeImm(arg0, arg1);
            break;
        case 0x87:
            ret = BattleByteRegEqReg(arg0, arg1);
            break;
        case 0x88:
            ret = BattleWordRegEqReg(arg0, arg1);
            break;
        case 0x89:
            ret = BattleByteRegLeReg(arg0, arg1);
            break;
        case 0x8A:
            ret = BattleWordRegLeReg(arg0, arg1);
            break;
        case 0x8B:
            ret = BattleByteRegTestImm(arg0, arg1);
            break;
        case 0x8C:
            ret = BattleWordRegTestImm(arg0, arg1);
            break;
        case 0x8D:
            ret = BattleByteRegTestReg(arg0, arg1);
            break;
        case 0x8E:
            ret = BattleWordRegTestReg(arg0, arg1);
            break;
        case 0x8F:
            ret = BattleByteRegNeImm(arg0, arg1);
            break;
        case 0x90:
            ret = BattleWordRegNeImm(arg0, arg1);
            break;
        case 0x91:
            ret = BattleByteRegNeReg(arg0, arg1);
            break;
        case 0x92:
            ret = BattleWordRegNeReg(arg0, arg1);
            break;
        case 0x93:
            ret = BattleDwordRegEqReg(arg0, arg1);
            break;
        case 0x94:
            ret = BattleDwordRegLeReg(arg0, arg1);
            break;
        case 0x95:
            ret = BattleTableEntryFlag8000(arg0);
            break;
        case 0x96:
            ret = BattleSlotCountIsZero(arg0);
            break;
        case 0x97:
            ret = BattleStateWordLowBitsClear();
            break;
        case 0x98:
            ret = func_8007EEE8();
            break;
        case 0x99:
            cont = 1;
            break;
        case 0x9B:
            ret = BattleActorSlotHasFlag80(arg1);
            break;
        }
        BattleScriptCursorSkipWord(arg0);
        if (!(cont & 0xFF)) {
            break;
        }
        next = acc | ret;
        if ((*arg0)[0] >= 0x80) {
            acc = next;
            continue;
        }
        ret = acc;
        break;
    }

    return ret & 0xFF;
}
 */
void BattleReleaseEffectSlotIfChar4(u8 arg0) {
    if (D_800D2D24[arg0] == 4 && D_800D2D28[0x8E] != 0) {
        func_8008FA60(0);
        func_8007765C();
        D_800D2D28[0x8E] = 0;
    }
}
