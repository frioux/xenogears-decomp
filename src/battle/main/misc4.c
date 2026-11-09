#include "common.h"

extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern u8 D_800D2E62[];
extern u16 D_800D39E0;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079778);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079840);
void func_80079934(u8** pCursor) {
    *pCursor += 4;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079948);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800799C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079AB0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079C24);
