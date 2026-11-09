#include "common.h"
#include "system/memory.h"

// .data/.bss of the overlay (supplied by splat's asm segments)
extern u8 D_80099D98;
extern u8 D_80099D9F;
extern u32 D_800912F4[];
extern u32 D_800912F0;
extern s32 D_80092734;
extern s32 D_80092950;
extern u8 D_80092884;
extern u8 D_80099D9A;
extern u8 D_80099D9B;
extern u8 D_80099D9C;
extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern u8 D_80099DA1;
extern u8 D_80099DA2;

typedef struct {
    /* 0x00 */ u8 unk0[0x12];
    /* 0x12 */ s16 unk12;
} BattleTextCursor; // Size: 0x14

void func_80083C0C(s32 arg);
void func_8007F834(void);
void func_80080C48(s32 arg);
void func_8007EE08(s32 arg);
u8 func_8007FF70(u8 value, s32 arg, s32 flags);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F854);

void func_8007F8B4(void) {
    long pad[2];

    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
}

void func_8007F8E4(void) {
    s32 arg;

    func_80080C48(0);
    if (D_80099D9F != 0 || D_80092950 == 1) {
        arg = 3;
    } else {
        D_80092950--;
        arg = 6;
    }
    func_80083C0C(arg);
}

void func_8007F948(BattleTextCursor* pCursor, s32 arg) {
    if (pCursor->unk12 == arg) {
        func_8007EE08(1);
    } else {
        func_8007EE08(0);
    }
}

u32 func_8007F97C(void) {
    return D_800912F4[D_80099D98];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F9A0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FB0C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FBEC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FE48);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FF70);

static void SetBattleFlag(u8* pFlag, s32 arg, s32 flags) {
    *pFlag = func_8007FF70(*pFlag, arg, flags);
}

static void SetFlag(u8* pFlag, s32 arg, s32 flags) {
    *pFlag = func_8007FF70(*pFlag, arg, flags);
}

typedef struct {
    /* 0x0 */ u8 unk0;
} FlagByte; // Size: 1

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080054);
/* best 55.13%: retail keeps &D_80099D98 in s0 across the call (lui+addiu, then lbu a0,0(s0)
   and sb v0,0(s0)); gcc 2.7.2 rematerialises the symbol address twice instead, whatever the
   source shape (pointer local, struct field, u8/u16*, volatile, static helper ~71%). */


INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080090);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800800CC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080108);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080144);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080180);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800801BC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800801F8);

void func_80080234(void) {
    D_80092884 = func_8007FF70(D_80092884, 1, 1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080268);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800802A4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8008040C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080570);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080644);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080780);
