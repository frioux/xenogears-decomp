#include "common.h"

extern u8 D_800D2CA4[];
extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern u8 D_800D2E62[];
extern u16 D_800D39E0;
extern u8 D_800D2E5C[];
extern u8 D_800D3400[];
extern s32 func_8007EF6C(u8** arg0, u8 arg1, u8 arg2);
extern s32 func_8007F8C0(u8** arg0, u8 arg1);
// A 0x170-byte battle table entry; only the flag word at +0 is read here.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk02[0x16E];
} BattleTableEntry; // Size: 0x170

// A 4-byte status entry; only the first byte is written here.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1[3];
} BattleStatusEntry; // Size: 0x4

extern BattleStatusEntry D_800C3D1B[];
extern u8 D_800D342E[];
extern u8 D_800D3439[];
extern u8 D_800D343E[];
extern u16 D_800C48E8;
extern s32 func_80089C08(u8 arg0);
extern u8 D_800D2DE4[];
extern BattleTableEntry D_800CCD64[];
extern u8 D_800D2E5F[];
extern u8 D_800D2E61[];
// A 0x48-byte row of the per-battle-slot table at D_800C3FFE.
typedef u16 BattleSlotRow[36]; // Size: 0x48

extern u8 D_800C3E8C;
extern u8 D_800C400B[];
extern BattleSlotRow D_800C4022[];
extern BattleSlotRow D_800C3FFE[];
extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern void func_800785D4(u8 arg0, u8 arg1);
extern void func_80078658(u8 arg0, u8 arg1);
extern void func_800787E0(u8 arg0, u8 arg1);
extern void func_8007887C(u8 arg0);
extern s32 func_8007A280(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern void func_800716D8(void);
extern void func_800793F0(u8 arg0);
extern void func_80079674(u8 arg0);
extern void func_80085350(void);
extern void func_80085388(void);
extern s32 func_80080AE4(u8 arg0);
extern void func_800B89FC(s32 arg0, u8 arg1, s32 arg2, s32 arg3);
extern s32 func_80079ED8(u8 arg0, u8 arg1, u8 arg2, s32 arg3);

void func_8007887C(u8 arg0) {
    if (D_800C3E8C != 0) {
        D_800C400B[D_800C3EAC[0x2DA] * 72] = arg0;
        D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF8;
        D_800C4022[D_800C3EAC[0x2DA]][0] = D_800C3E8C - 1;
        D_800C3EAC[0x2DA] = D_800C3EAC[0x2DA] + 1;
    }
}
void func_8007893C(u8 arg0, u8 arg1) {
    if (D_800D2E5F[arg0 * 8] != 0) {
        func_80078658(arg0, arg1);
        func_800787E0(0x1E, arg1);
        func_8007887C(arg1);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078998);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078B34);
void func_80078C9C(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xFC;
    func_800785D4(arg0, arg1);
}
void func_80078CEC(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = D_800D2E60[arg1 * 8];
    func_800785D4(arg0, arg1);
}

void func_80078D48(u8 arg0, u8 arg1) {
    *(u16*)&D_800D39E0 = *(u16*)(D_800D2E62 + arg1 * 8);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078D6C);
/* Best: 82.72%. Everything matches (including the 0x170-stride flag word) except
   that gcc sinks the D_800C3D1B store past the flag-word load. Tried: statement
   order both ways, pointer local for the entry (changes the address shape to
   base-in-register), scalar-plus-cast spelling.

void func_80078D6C(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF9;
    func_800785D4(arg0, arg1);
    D_800D2DE4[arg0] = 0xFF;
    D_800C3D1B[arg0 - 3].unk0 = 1;
    D_800CCD64[arg0].flags &= 0x8000;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078E24);
void func_80079054(u8 arg0, u8 arg1) {
    func_80079ED8(arg0, D_800D2E5D[arg1 * 8], D_800D2E60[arg1 * 8], 0);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079098);
/* Best: 70.26%. Same CSE class as func_8007916C: gcc builds &D_800D2E5D[arg1*8]
   once into a saved register across the first call instead of re-forming it with
   a fresh lui/addu from the shifted index. Tried: one expression twice, two
   index locals, nested call.

s32 func_80079098(u8 arg0, u8 arg1) {
    s32 value;

    value = func_80079ED8(arg0, D_800D2E5D[arg1 * 8], 0, 1);
    return func_80079ED8(arg0, D_800D2E5D[arg1 * 8], (D_800D2E60[arg1 * 8] + value) & 0xFF, 0);
}
*/
void func_80079114(u8 arg0, u8 arg1) {
    func_8007A280(arg0, D_800D2E5D[arg1 * 8], D_800D2E60[arg1 * 8] | (D_800D2E61[arg1 * 8] << 8), 0);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_8007916C);
/* Best: 77.89%. Instruction multiset is right; gcc keeps &D_800D2E5D[arg1*8] in
   a saved register across the first call (base-in-register, lbu 0(s0)) where the
   original re-forms it with lui/addu from the shifted index. Same CSE blocker as
   func_80079098.

s32 func_8007916C(u8 arg0, u8 arg1) {
    s32 value;

    value = func_8007A280(arg0, D_800D2E5D[arg1 * 8], 0, 1);
    return func_8007A280(arg0, D_800D2E5D[arg1 * 8],
                        D_800D2E60[arg1 * 8] + (value + (D_800D2E61[arg1 * 8] << 8)) & 0xFFFF, 0);
}
*/
void func_800791FC(u8 arg0, u8 arg1) {
    func_8007893C(arg1, arg0);
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF4;
    func_800785D4(arg0, arg1);
}
void func_80079270(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF6;
    D_800C3FFE[D_800C3EAC[0x2DA]][0] = *(u16*)(D_800D2E62 + arg1 * 8);
    func_800785D4(arg0, arg1);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800792F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800793F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079674);
void func_80079778(u8 arg0) {
    D_800C3EAC[0x2DA] = 0;
    func_80085350();
    func_80085388();
    D_800C400B[D_800C3EAC[0x2DA] * 72] = arg0;
    func_800716D8();
    func_800B89FC(1, arg0, 0, func_80080AE4(arg0));
    func_800793F0(arg0);
    func_80079674(arg0);
    while (D_800C3EAC[0x2DB] == 0) {
        func_800716D8();
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079840);
/* Best: 96.48%. Two emit-order/allocator tie-breaks left: the loop preheader
   puts `move a1,v1` after `la a2` where the original has it first, and the
   D_800C3EAC load adds index-first (`addu v0,v0,v1`) where the original adds
   base-first (`addu v1,v1,v0`). Tried: D_800C3EAC[slot * 0x40 + 0x3C] inline,
   the (mem_addr) cast form, and a u8* pBuf local (96.48%). The slot/off split
   matters: `u8 slot = arg1 - 3` with `slot * 0x40` at each use reproduces the
   original's andi-then-sll rematerialisation; folding them into one u8 off
   gives a single sll in the preheader and 93.20%.

void func_80079840(u8 arg0, u8 arg1) {
    u8 slot;
    s32 i;
    u8* pBuf;

    if (arg1 >= 3) {
        slot = arg1 - 3;
        *(u16*)&D_800D342E[slot * 0x40] = func_80089C08(arg0);
        for (i = 0; i < 5; i++) {
            D_800D3439[slot * 0x40 + i] = D_800D2CA4[i];
        }
        pBuf = D_800C3EAC + (arg0 & 0xFF) * 0x40;
        if (D_800C48E8 & func_80089C08(pBuf[0x3C])) {
            D_800D343E[slot * 0x40] = 1;
        } else {
            D_800D343E[slot * 0x40] = 0;
        }
    }
}
*/void func_80079934(u8** pCursor) {
    *pCursor += 4;
}
void func_80079948(u8** pCursor) {
    while (**pCursor >= 0x80) {
        func_80079934(pCursor);
    }
    while ((u32)((**pCursor - 0x80) & 0xFF) >= 0x70) {
        func_80079934(pCursor);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800799C8);
/* Best: 87.14%. Whole structure, both clear loops (the *p++ / p < pEnd do-while
   and the 0x8B8 / -0x48 loop) and the if/else layout are right. Left: gcc puts
   the loop test at the top (no preheader `j`) where the original rotates it to
   the bottom (`bne` back to the body, fall into the epilogue), and it gives the
   0xFF compare constant a callee-saved reg ($s2, +sw/lw) where the original
   parks it in the bne delay slot. `cmd` must be u32 (u8 gives an andi per use,
   s32 gives slti) and the >= 0x80 test must come first or the arms swap order.
   Tried: while with the load in the condition, for(;;) with two breaks,
   break vs return for the 0xFF exit, u8/u32/s32 cmd, u8 value.

void func_800799C8(u8 arg0) {
    u8 *p;
    u8 *pEnd;
    u8 *pCursor;
    u8 slot;
    u8 value;
    u32 cmd;
    s32 i;

    value = 0;
    slot = arg0 - 3;
    pCursor = *(u8 **)(D_800D3400 + slot * 0x40);
    p = D_800D2E5C;
    pEnd = p + 0x100;
    do {
        *p++ = 0;
    } while (p < pEnd);
    for (i = 0x8B8; i >= 0; i -= 0x48) {
        D_800C402F[i] = 0xFF;
    }
    for (;;) {
        cmd = *pCursor;
        if (cmd == 0xFD) {
            break;
        }
        if (cmd == 0xFF) {
            break;
        }
        if (cmd >= 0x80) {
            if ((func_8007F8C0(&pCursor, slot) & 0xFF) != 0) {
                continue;
            }
            func_80079948(&pCursor);
        } else {
            value = func_8007EF6C(&pCursor, slot, value);
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079AB0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079C24);
