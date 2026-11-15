#include "common.h"

// The per-slot battle register file (D_800D3400[slot]): the dword, word and
// byte registers the script's comparison ops test against, indexed by the
// operand byte that follows the opcode.
typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u32 unk10[4]; // dword registers
    /* 0x20 */ u16 unk20[8]; // word registers
    /* 0x30 */ u8 unk30[0x10]; // byte registers
} BattleSlotRegs; // Size: 0x40

// A 0x170-byte battle table entry; only the flag word at +0 is read here.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk02[0x16E];
} BattleTableEntry; // Size: 0x170

// A 4-byte status entry; only the first byte is read here.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1[3];
} BattleStatusEntry; // Size: 0x4

extern u8 D_800C3EB7[];
extern BattleTableEntry D_800CCD64[];
extern u8 D_800D2D24[];
extern u8 *D_800D2D28;
extern BattleStatusEntry D_800D301C[];
extern u16 D_800D39DC;
extern BattleSlotRegs D_800D3400[];

s32 BattleWordRegLeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return value >= D_800D3400[arg1].unk20[p[1]];
}
s32 BattleByteRegGeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return !(D_800D3400[arg1].unk30[p[1]] < p[2]);
}
s32 BattleWordRegGeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return D_800D3400[arg1].unk20[p[1]] >= value;
}
s32 BattleByteRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return D_800D3400[arg1].unk30[p[1]] == D_800D3400[arg1].unk30[p[2]];
}
s32 BattleWordRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return D_800D3400[arg1].unk20[p[1]] == D_800D3400[arg1].unk20[p[2]];
}
s32 BattleByteRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return D_800D3400[arg1].unk30[p[1]] <= D_800D3400[arg1].unk30[p[2]];
}
s32 BattleWordRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return D_800D3400[arg1].unk20[p[1]] <= D_800D3400[arg1].unk20[p[2]];
}
s32 BattleByteRegTestImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    s32 v = D_800D3400[arg1].unk30[p[1]];

    v &= p[2];
    return v != 0;
}
s32 BattleWordRegTestImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] + (p[3] << 8);

    return (D_800D3400[arg1].unk20[p[1]] & value) != 0;
}
s32 BattleByteRegTestReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 lhs = D_800D3400[arg1].unk30[p[1]];

    return (D_800D3400[arg1].unk30[p[2]] & lhs) != 0;
}
s32 BattleWordRegTestReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 a = D_800D3400[arg1].unk20[p[1]];
    u16 b = D_800D3400[arg1].unk20[p[2]];

    return (b & a) != 0;
}
s32 BattleByteRegNeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (D_800D3400[arg1].unk30[p[1]] ^ p[2]) != 0;
}
s32 BattleWordRegNeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return (D_800D3400[arg1].unk20[p[1]] ^ value) != 0;
}
s32 BattleByteRegNeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (D_800D3400[arg1].unk30[p[1]] ^ D_800D3400[arg1].unk30[p[2]]) != 0;
}
s32 BattleWordRegNeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (D_800D3400[arg1].unk20[p[1]] ^ D_800D3400[arg1].unk20[p[2]]) != 0;
}
s32 BattleDwordRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (D_800D3400[arg1].unk10[p[1]] ^ D_800D3400[arg1].unk10[p[2]]) == 0;
}
s32 BattleDwordRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return D_800D3400[arg1].unk10[p[1]] <= D_800D3400[arg1].unk10[p[2]];
}
s32 BattleTableEntryFlag8000(u8 **arg0) {
    u8 *p = *arg0;

    return D_800CCD64[p[1]].flags >> 15;
}
s32 BattleSlotCountIsZero(u8 **arg0) {
    u8 *p = *arg0;

    return D_800D301C[p[1]].unk0 == 0;
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

    return D_800C3EB7[idx * 0x1C] >> 7;
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
     s32 BattleSetTextScale(u8 **arg0, u8 arg1);
     s32 func_8007E98C(u8 **arg0, u8 arg1);
     s32 BattleByteRegLeImm(u8 **arg0, u8 arg1);
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
            ret = BattleSetTextScale(arg0, arg1);
            break;
        case 0x82:
            ret = func_8007E98C(arg0, arg1);
            break;
        case 0x83:
            ret = BattleByteRegLeImm(arg0, arg1);
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
        BattleFreePacketPrimitives();
        D_800D2D28[0x8E] = 0;
    }
}
