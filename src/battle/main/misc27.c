#include "common.h"
#include "psyq/libgte.h"

extern void TimerWorkListSetTaskCallback();
extern void WorkListSetTaskCallback();
extern void func_8001E148();
extern void func_800C08CC();
extern void func_800B3CD4();
extern void func_800B51B0();
extern void func_800B5CC0();
extern void func_800B5DF4();
extern void func_800B6004();
extern void func_800B59BC();

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ s8 unk1;
    /* 0x2 */ s8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
} BattleScriptOp;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B3F04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B4EDC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B4F88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B50D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B51B0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5588);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B56E4);
// Best attempt: objdiff 95.22%, 72 bytes (the original size). Identical insn set;
// the only difference is which insn reorg steals into the jr/jal delay slot: the
// original puts the callback's %lo addiu there and leaves addiu a1,s0,0x78 before
// the call, gcc always does the reverse. Same output for three spellings (inline
// pSprite + 0x78, a pBuf local, &func vs plain func name).
/*
void func_800B56E4(u8* pWork) {
    u8* pSprite;

    pSprite = *(u8**)(pWork + 0x4);
    func_8001E148(*(void**)(pSprite + 0x38));
    func_800C08CC(5, pSprite + 0x78, func_800B51B0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B572C);
long func_800B57E4(u8* pActor) {
    VECTOR v;
    VECTOR out;

    v.vx = *(s16*)(pActor + 0xA0) - *(s16*)(pActor + 0x2);
    v.vy = *(s16*)(pActor + 0xA2) - *(s16*)(pActor + 0x6);
    v.vz = *(s16*)(pActor + 0xA4) - *(s16*)(pActor + 0xA);
    Square0(&v, &out);
    return SquareRoot0(out.vx + out.vz + out.vy);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5854);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5924);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B59BC);
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
} BattlePoint;

BattlePoint* func_800B5AC4(BattlePoint* pOut, u8* pActor, s32 index) {
    BattlePoint point;
    int ofs;

    func_800B59BC(&point, pActor, index);
    ofs = *(s16*)(pActor + 0x2);
    point.x = point.x + ofs;
    ofs = *(s16*)(pActor + 0x6);
    point.y = point.y + ofs;
    *pOut = point;
    return pOut;
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5B3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5C18);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5CC0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5DC4);
// Best attempt: objdiff 65.00%, 52 bytes (the original size). The original puts
// lui a1,%hi(func_800B5CC0) two insns early (before sw ra and the lw) and its
// %lo addiu in the jal delay slot; gcc keeps the lui/addiu pair adjacent right
// before the jal, so reorg cannot split it and fills the slot with a nop.
// Identical output for the inline form, a callback local, and a task local.
/*
void func_800B5DC4(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    TimerWorkListSetTaskCallback(*(void**)(pWork + 0x6C), func_800B5CC0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5DF4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5FBC);
// Best attempt: objdiff 71.11%, 72 bytes (the original size). One insn out of
// place: the original hoists lui a1,%hi(func_800B5DF4) above sw ra and the
// lw a0,0x6C; gcc emits the whole lui/addiu pair after the lw. Everything else,
// including the delay-slot addiu a0, matches. Same output for a callback local,
// a task local, and for placing the load before or after the sh store.
/*
void func_800B5FBC(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(pWork + 0x6C) + 0x1C, func_800B5DF4);
    pWork[0x2B] = 0x40;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B6004);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B61B0);
// Same shape as func_800B5FBC (see the note there): only the lui of the
// callback address is scheduled one insn too late.
/*
void func_800B61B0(u8* pWork) {
    *(u16*)(pWork + 0x34) = 1;
    WorkListSetTaskCallback(*(u8**)(pWork + 0x6C) + 0x1C, func_800B6004);
    pWork[0x2B] = 0x40;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B61F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B626C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B62C8);
void func_800B639C(u8* arg0, u8* pScript) {
    BattleScriptOp* pOp;

    pOp = (BattleScriptOp*)(pScript + ((*(s8*)(pScript + 1) << 8) | pScript[0]));
    func_800B3CD4(pOp->unk5, pOp->unk3, pOp->unk4, (s8)pOp->unk0, pOp->unk1, pOp->unk2);
}
