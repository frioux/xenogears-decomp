#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "system/archive.h"

/*
 * This unit's retail object materialises absolute symbol addresses into allocated
 * registers (lui $v0 / $v1 / $s0 ...) for every access, and it was assembled with
 * li left unexpanded (the XenoBattleTail preset's --dont-expand-li). Our assembler
 * always expands a one-shot absolute load/store's address into $at, so such a
 * function matches under no preset: 17 of the 21 stubs here are that class
 * (atfree.py). Re-checked with try_flags.sh --all: gcc 2.6.0 and 2.7.2-cdk score
 * identically to the unit's own preset.
 */

extern int ArchiveDataSync(void);
extern u8 D_800D2FDC;
extern void *D_800C3E1C;
extern s32 D_80059464;
extern DISPENV D_800C4A7C;
extern void *D_800C3618;
extern void func_800BE790(void);
extern void func_800245D8(void *, s32);
extern void SpriteSetCallback(void *, void *);
extern void func_8002A498(s32);

void func_800B73EC(void) {
    u8 *pWork = HeapAlloc(0x10F7C, 1);

    *(u8 **)(pWork + 4) = pWork;
    *(u8 **)(pWork + 0x20) = pWork;
    func_800B7424(pWork);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7424);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7870);
// 45.00% under every preset tried (unit preset, gcc 2.6.0, gcc 2.7.2-cdk):
// retail keeps the address in $v0 and the store in the jr delay slot, while our
// assembler always expands this one-shot absolute store with $at (atfree.py).
/*
void func_800B7C28(void) {
    D_800D2FDC = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7C28);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7C34);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B7E94);
// 45.00% under every preset tried (unit preset, gcc 2.6.0, gcc 2.7.2-cdk): the
// same at-free one-shot absolute store as func_800B7C28, retail is lui $v0 / jr /
// sw $a0 and we get $at.
/*
void func_800B8048(void *pArg) {
    D_800C3E1C = pArg;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8048);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8054);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8068);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8098);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B81BC);

// 77.69% under the unit preset, gcc 2.6.0 and gcc 2.7.2-cdk. The four SetDef*Env
// calls and the 0xE0 local already reproduce retail (retail keeps pEnv in $s0 and
// the 0xE0 in $s1, ours swaps the two). What is left is the tail: retail does one
// addiu $s0,$s0,-0xBCC (D_800C4A7C -> D_800C3EB0) and then eight sh with the
// displacements 0xBD4-0xBDA and 0x4C44-0x4C4A; our gcc folds that address to
// D_800C4A7C+0x4078 at expand time and maspsx then expands every one-shot absolute
// store through $at, so the tail costs 16 insns instead of 11 (+20 bytes).
/*
void func_800B8284(void) {
    u8 *pBase;
    s32 h = 0xE0;

    pBase = (u8 *)&D_800C4A7C;
    SetGeomOffset(0xA0, 0xA4);
    SetDefDispEnv((DISPENV *)pBase, 0, h, 0x140, h);
    SetDefDispEnv((DISPENV *)(pBase + 0x4070), 0, 0, 0x140, h);
    SetDefDrawEnv((DRAWENV *)pBase - 1, 0, 0, 0x140, h);
    SetDefDrawEnv((DRAWENV *)(pBase + 0x4014), 0, h, 0x140, h);

    pBase -= 0xBCC;
    *(s16 *)(pBase + 0xBD6) = 0xA;
    *(s16 *)(pBase + 0x4C46) = 0xA;
    *(s16 *)(pBase + 0xBD8) = 0x100;
    *(s16 *)(pBase + 0x4C48) = 0x100;
    *(s16 *)(pBase + 0xBD4) = 0;
    *(s16 *)(pBase + 0x4C44) = 0;
    *(s16 *)(pBase + 0xBDA) = 0xD8;
    *(s16 *)(pBase + 0x4C4A) = 0xD8;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8284);
void func_800B8354(void) {
    while (ArchiveDataSync() != 0) {
        func_800BE790();
    }
}

/* Archive entry table at D_800C35DC: u16 size at +0, s16 id at +2. */
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ s16 unk2;
} BattleArchiveEntry; // Size: 0x4

/* Archive read request filled in on the stack and handed to func_80029AFC. */
typedef struct {
    /* 0x0  */ u16 unk0;
    /* 0x2  */ u16 pad2;
    /* 0x4  */ void *unk4;
    /* 0x8  */ u16 unk8;
    /* 0xA  */ u16 padA;
    /* 0xC  */ void *unkC;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 pad12;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 pad1A;
    /* 0x1C */ s32 unk1C;
} BattleArchiveRequest; // Size: 0x20

extern BattleArchiveEntry D_800C35DC[];
extern void func_800C0F70(void);
extern void func_80029AFC(void *pRequest, s32 arg1, s32 arg2);
extern void SoundAddSedsEntry(void *pEntry);
extern void *SoundLoadWdsFile(void *pBuffer, s32 arg1);
extern void SoundFreeWdsEntry(void *pWds);
extern void SoundRemoveSedsEntry(void *pBuffer);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);
extern s32 func_8003A5D0(s32 arg0);
extern s16 func_8003BDFC(s32 arg0);

// 91.66% under the unit preset. Structure is complete and in order; what is left:
// (1) register roles - retail has s0=pEntry, s1=pTable, s2=pBuffer1, s3=pWds,
// s4=arg0, s5=arg1, while arg0 takes $s0 here and pEntry stays live across the two
// poll loops in $s3 (retail re-forms &D_800C35DC[arg0] after the loops with a fresh
// lui/addiu, so pEntry is dead there and $s3 is free for pWds);
// (2) the two store-then-reload pairs retail keeps ("sh v0,16(sp)" + "lhu a0,16(sp)",
// likewise at 0x18(sp)) - gcc forwards the value from the register here.
// Tried and worse: request accessed through a pointer local (88.0, needs $s6),
// pTable[arg0] spelled inline instead of the pEntry local (88.0).
/*
void func_800B838C(s32 arg0, s32 arg1) {
    BattleArchiveRequest request;
    BattleArchiveEntry *pEntry;
    BattleArchiveEntry *pTable;
    void *pBuffer1;
    void *pBuffer2;
    void *pWds;
    s32 id;

    func_800C0F70();
    func_800B8354();
    ArchiveSetIndex(0x2C, 1);

    pTable = D_800C35DC;
    pEntry = &pTable[arg0];
    request.unk0 = pEntry->unk0;
    pBuffer1 = HeapAlloc(ArchiveDecodeAlignedSize(request.unk0), 0);
    request.unk4 = pBuffer1;
    if (arg0 == 8) {
        request.unk8 = pTable[8].unk0 + 1;
    } else {
        request.unk8 = pEntry->unk0 + 1 + arg1;
    }
    pBuffer2 = HeapAlloc(ArchiveDecodeAlignedSize(request.unk8), 0);
    request.unkC = pBuffer2;
    request.unk14 = 0;
    request.unk10 = 0;
    func_80029AFC(&request, 0, 0);

    func_800B8354();
    SoundAddSedsEntry(pBuffer1);
    pWds = SoundLoadWdsFile(pBuffer2, 0);
    while (func_8003BDFC(0) != 0) {
        func_800BE790();
    }
    id = pEntry->unk2 + arg1 + (*(u16 *)((u8 *)pBuffer1 + 0x14) << 16);
    SoundPlaySoundEffectOnFreePair(id);
    while (func_8003A5D0(id) != 0) {
        func_800BE790();
    }
    SoundRemoveSedsEntry(pBuffer1);
    SoundFreeWdsEntry(pWds);
    HeapFree(pBuffer2);
    HeapFree(pBuffer1);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B838C);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B853C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8774);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8840);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B88C4);

void func_800B89F4(void) {
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B89FC);
// 81.63% under every preset tried (unit preset, gcc 2.6.0, gcc 2.7.2-cdk): retail
// CSEs both symbol addresses into $s0 and stores in the delay slot; the one-shot
// D_80059464 load goes through $at here (atfree.py).
/*
void func_800B8D04(void) {
    func_800B8354();
    BattleLoadArchiveEntryFile();
    while (D_80059464 != func_800BF720()) {
        func_800BE790();
    }
    func_800BF3A4();
    if (D_800C3618 != NULL) {
        HeapFree(D_800C3618);
        D_800C3618 = NULL;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8D04);

void func_800B8D7C(void) {
    func_8002A498(0);
    func_800B8D04();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8DA4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc29", func_800B8EBC);

void func_800B9020(u8 *pSprite) {
    func_800245D8(pSprite, *(s8 *)(pSprite + 0xB0));
    SpriteSetCallback(pSprite, 0);
}
