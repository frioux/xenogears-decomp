#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

extern u8 D_8006F978[];
extern u8 *D_800927CC;
extern u8 *D_800927D0;
extern u8 D_80095968[];

extern void *func_8008820C();
extern void func_80085EC8(void *);

typedef struct {
    /* 0x000 */ u32 unk0[0xC4];
} BattlingWork; // Size: 0x310

extern BattlingWork D_8009A2F8[2];

extern s32 D_800928F8;

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 pad6[0x62];
} BattlingEntry68; // Size: 0x68

extern BattlingEntry68 D_8009A988[];

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087068);

void func_800875EC(void) {
    func_80085EC8(D_8009A2F8);
    D_8009A2F8[1] = D_8009A2F8[0];
}

void func_80087650(void) {
    s32 i;

    for (i = 0; i < 0x80; i++) {
        D_800927CC[i] = 0;
        D_800927D0[i] = 0xFF;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087698);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008779C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087830);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800878DC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087AB0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087B74);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087E38);
/* Best 84.04%: only the base register differs (original keeps the base in a0 and
   sums into v0; gcc reuses the dead base register for the sum).
void func_80087E38(s32 *arg0) {
    s32 i = D_800928F8;

    if (i < 0x1F) {
        BattlingEntry68 *p = D_8009A988;

        p += i;
        p->unk0 = arg0[0];
        p->unk2 = arg0[1];
        p->unk4 = arg0[2];
        D_800928F8 = i + 1;
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087EA0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008820C);

void func_800882D4(s32 arg0, s32 *arg1, s32 arg2, s16 arg3) {
    u8 *p = func_8008820C(arg0, arg1, arg2);

    if (p != NULL) {
        *(s16 *)(p + 0x30) = arg3;
    }
}

void func_80088308(void) {
    s32 i;

    for (i = 0x141C; i >= 0; i -= 0x34) {
        *(s16 *)(D_80095968 + i) = 0;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008832C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800884E0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008859C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80088658);

s32 func_800886FC(s32 *arg0) {
    long out[3];

    gte_ldlvl(arg0);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[1] + out[2]);
}

s32 func_80088754(s32 *arg0) {
    long out[3];

    gte_ldlvl(arg0);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[2]);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800887A4);

s32 func_80088838(s32 *arg0, s32 *arg1) {
    long out[3];

    out[0] = arg1[0] - arg0[0];
    out[2] = arg1[2] - arg0[2];
    gte_ldlvl(out);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[2]);
}

void func_800888B0(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    D_8006F978[arg0 >> 3] |= bit;
}

s32 func_800888E4(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    return D_8006F978[arg0 >> 3] & bit;
}

void func_80088908(s32 arg0) {
    s32 mask = 1;

    mask <<= arg0 & 7;
    D_8006F978[arg0 >> 3] &= ~mask;
}
