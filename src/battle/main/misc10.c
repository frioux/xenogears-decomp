#include "common.h"

// The per-slot battle register file (g_BattleSlotRegs[slot]): the dword, word and
// byte registers the script's comparison ops test against, indexed by the
// operand byte that follows the opcode.
typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u32 dwordRegs[4];
    /* 0x20 */ u16 wordRegs[8];
    /* 0x30 */ u8 byteRegs[0x10];
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
extern BattleTableEntry g_BattleSlotEntries[];
extern u8 g_BattleActorCharIds[];
extern u8 *g_BattleDisplayState;
extern BattleStatusEntry g_BattleCharSlotUsage[];
extern u16 g_BattleStateFlags;
extern BattleSlotRegs g_BattleSlotRegs[];

s32 BattleWordRegLeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return value >= g_BattleSlotRegs[arg1].wordRegs[p[1]];
}
s32 BattleByteRegGeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return !(g_BattleSlotRegs[arg1].byteRegs[p[1]] < p[2]);
}
s32 BattleWordRegGeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return g_BattleSlotRegs[arg1].wordRegs[p[1]] >= value;
}
s32 BattleByteRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].byteRegs[p[1]] == g_BattleSlotRegs[arg1].byteRegs[p[2]];
}
s32 BattleWordRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].wordRegs[p[1]] == g_BattleSlotRegs[arg1].wordRegs[p[2]];
}
s32 BattleByteRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].byteRegs[p[1]] <= g_BattleSlotRegs[arg1].byteRegs[p[2]];
}
s32 BattleWordRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].wordRegs[p[1]] <= g_BattleSlotRegs[arg1].wordRegs[p[2]];
}
s32 BattleByteRegTestImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    s32 v = g_BattleSlotRegs[arg1].byteRegs[p[1]];

    v &= p[2];
    return v != 0;
}
s32 BattleWordRegTestImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] + (p[3] << 8);

    return (g_BattleSlotRegs[arg1].wordRegs[p[1]] & value) != 0;
}
s32 BattleByteRegTestReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 lhs = g_BattleSlotRegs[arg1].byteRegs[p[1]];

    return (g_BattleSlotRegs[arg1].byteRegs[p[2]] & lhs) != 0;
}
s32 BattleWordRegTestReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 a = g_BattleSlotRegs[arg1].wordRegs[p[1]];
    u16 b = g_BattleSlotRegs[arg1].wordRegs[p[2]];

    return (b & a) != 0;
}
s32 BattleByteRegNeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (g_BattleSlotRegs[arg1].byteRegs[p[1]] ^ p[2]) != 0;
}
s32 BattleWordRegNeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return (g_BattleSlotRegs[arg1].wordRegs[p[1]] ^ value) != 0;
}
s32 BattleByteRegNeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (g_BattleSlotRegs[arg1].byteRegs[p[1]] ^ g_BattleSlotRegs[arg1].byteRegs[p[2]]) != 0;
}
s32 BattleWordRegNeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (g_BattleSlotRegs[arg1].wordRegs[p[1]] ^ g_BattleSlotRegs[arg1].wordRegs[p[2]]) != 0;
}
s32 BattleDwordRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (g_BattleSlotRegs[arg1].dwordRegs[p[1]] ^ g_BattleSlotRegs[arg1].dwordRegs[p[2]]) == 0;
}
s32 BattleDwordRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].dwordRegs[p[1]] <= g_BattleSlotRegs[arg1].dwordRegs[p[2]];
}
s32 BattleTableEntryFlag8000(u8 **arg0) {
    u8 *p = *arg0;

    return g_BattleSlotEntries[p[1]].flags >> 15;
}
s32 BattleSlotCountIsZero(u8 **arg0) {
    u8 *p = *arg0;

    return g_BattleCharSlotUsage[p[1]].unk0 == 0;
}
s32 BattleStateWordLowBitsClear(void) {
    return (g_BattleStateFlags & 3) == 0;
}
s32 func_8007EEE8(void) {
    s32 ret = 1;
    s32 i = 0;
    s32 ofs = 0x54;

    for (; i < 8; i++) {
        if ((g_BattleStateFlags >> 5) != 0) {
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
/* Best: 99.30%. The switch and the loop are right; one copy is missing (684
   bytes against the original's 688). Retail's loop tail is

       andi v0,s4,0xff ; beqz v0,exit ; andi v0,s0,0xff
       lw v0,0(s1) ; or v1,s3,s0 ; lbu v0,0(v0) ; nop ; sltiu v0,v0,0x80
       beqz v0,looptop ; addu s3,v1,zero ; addu s0,s3,zero ; andi v0,s0,0xff

   i.e. the OR lands in a temp (v1) which is copied into the accumulator in the
   back edge's delay slot, and the result is copied into ret afterwards. gcc
   folds `acc = next; ret = acc;` into `ret = acc|ret; acc = ret`, which loses
   that copy (`or s0,s3,s0` + `move s3,s0`, 684 bytes, this body). Spelling the
   second statement `ret = next;` keeps the temp and the exact size but then the
   exit branch loses its own duplicated `andi v0,s0,0xff` (96.95%). Also tried:
   for(;;) with continue/break (93.49), chained `ret = acc = next;` (96.95),
   `acc = next = acc | ret;` (96.02), a u8 opcode local for the loop test
   (98.92), two inline returns (93.49), and 1225 permuter iterations from this
   body (RESULT PERMUTER_NONE; re-run round 173 from this body, 995 more
   iterations, again NONE). `ret = next;` AFTER the loop is the one shape that
   does reproduce the temp the way retail has it, but it makes the exit a
   separate block (j + `andi v0,v1,0xff`) and the body 692 bytes (95.81), i.e.
   4 too many instead of 4 too few. Preset is not the problem: a --sweep puts
   this body at 99.3 under every 2.7.2 -G4/-G8/-mcpu combination, 94.59 under
   2.7.2-cdk -O2, and -O3 does not compile it. The old RODATA-BLOCKED note was stale:
   config/battle.yaml line 61 gives main/misc10 its own .rodata range, so the
   switch's table stays in the unit. Needs these declarations:
     s32 BattleSetTextScale(u8 **arg0, u8 arg1);
     s32 BattleWordRegEqImm(u8 **arg0, u8 arg1);
     s32 BattleByteRegLeImm(u8 **arg0, u8 arg1);
     void BattleScriptCursorSkipWord(u8 **pCursor);

s32 func_8007F8C0(u8 **pCursor, u8 arg1) {
    s32 ret = 0;
    s32 cont = 0;
    s32 acc = 0;
    s32 next;

    do {
        switch ((*pCursor)[0]) {
        case 0x80:
        case 0x9A:
            ret = 1;
            break;
        case 0x99:
            cont = 1;
            break;
        case 0x81:
            ret = BattleSetTextScale(pCursor, arg1);
            break;
        case 0x82:
            ret = BattleWordRegEqImm(pCursor, arg1);
            break;
        case 0x83:
            ret = BattleByteRegLeImm(pCursor, arg1);
            break;
        case 0x84:
            ret = BattleWordRegLeImm(pCursor, arg1);
            break;
        case 0x85:
            ret = BattleByteRegGeImm(pCursor, arg1);
            break;
        case 0x86:
            ret = BattleWordRegGeImm(pCursor, arg1);
            break;
        case 0x87:
            ret = BattleByteRegEqReg(pCursor, arg1);
            break;
        case 0x88:
            ret = BattleWordRegEqReg(pCursor, arg1);
            break;
        case 0x89:
            ret = BattleByteRegLeReg(pCursor, arg1);
            break;
        case 0x8A:
            ret = BattleWordRegLeReg(pCursor, arg1);
            break;
        case 0x8B:
            ret = BattleByteRegTestImm(pCursor, arg1);
            break;
        case 0x8C:
            ret = BattleWordRegTestImm(pCursor, arg1);
            break;
        case 0x8D:
            ret = BattleByteRegTestReg(pCursor, arg1);
            break;
        case 0x8E:
            ret = BattleWordRegTestReg(pCursor, arg1);
            break;
        case 0x8F:
            ret = BattleByteRegNeImm(pCursor, arg1);
            break;
        case 0x90:
            ret = BattleWordRegNeImm(pCursor, arg1);
            break;
        case 0x91:
            ret = BattleByteRegNeReg(pCursor, arg1);
            break;
        case 0x92:
            ret = BattleWordRegNeReg(pCursor, arg1);
            break;
        case 0x93:
            ret = BattleDwordRegEqReg(pCursor, arg1);
            break;
        case 0x94:
            ret = BattleDwordRegLeReg(pCursor, arg1);
            break;
        case 0x95:
            ret = BattleTableEntryFlag8000(pCursor);
            break;
        case 0x96:
            ret = BattleSlotCountIsZero(pCursor);
            break;
        case 0x97:
            ret = BattleStateWordLowBitsClear();
            break;
        case 0x98:
            ret = func_8007EEE8();
            break;
        case 0x9B:
            ret = BattleActorSlotHasFlag80(arg1);
            break;
        }
        BattleScriptCursorSkipWord(pCursor);
        if (!(cont & 0xFF)) {
            return ret & 0xFF;
        }
        next = acc | ret;
        acc = next;
        ret = acc;
    } while ((*pCursor)[0] >= 0x80);

    return ret & 0xFF;
}
 */
void BattleReleaseEffectSlotIfChar4(u8 arg0) {
    if (g_BattleActorCharIds[arg0] == 4 && g_BattleDisplayState[0x8E] != 0) {
        BattleFreeSlotBuffers(0);
        BattleFreePacketPrimitives();
        g_BattleDisplayState[0x8E] = 0;
    }
}
