#include "common.h"

extern s32 D_8009292C;
extern s32 D_80092918;
extern s32 D_80092944;
extern s32 D_80092950;
extern s16 D_8009881E;
extern s16 D_80097102;
extern s16 D_80099DA4;
extern u8 D_800928FC;
extern u8 D_80099D9A;
extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern u8 D_80099DA1;
extern u8 D_80099DA2;

extern void func_800732CC(void);
extern void func_80088AF8(void);
extern void func_8008DC28(void);

typedef struct ActorPart ActorPart;
typedef struct Actor Actor;

// Layout recovered from the offsets this unit touches; only those are known.
struct ActorPart {
    /* 0x00 */ u8 pad00[0x34];
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ u8 pad40[0x6];
    /* 0x46 */ s16 unk46;
}; // Size: unknown

struct Actor {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ u8 pad0C[0x48];
    /* 0x54 */ s32 unk54;
    /* 0x58 */ u8 pad58[0x4];
    /* 0x5C */ ActorPart* unk5C;
}; // Size: unknown

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80078920);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80078D20);

void func_80078E94(Actor* p) {
    p->unk5C->unk34 = p->unk0;
    p->unk5C->unk38 = p->unk4;
    p->unk5C->unk3C = p->unk8;
    p->unk5C->unk46 = p->unk54;
}

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
    /* 0xC */ s16 unkC;
} UnkStruct1; // Size: 0xE

void func_80078ED4(UnkStruct1* p) {
    p->unk0 = 0x100;
    p->unk4 = 0x10;
    p->unk2 = 0;
    p->unk6 = 0;
    p->unk8 = 0;
    p->unkA = 0x30;
    p->unkC = 0x30;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80078F00);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007920C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_800796B8);

void func_80079A8C(void) {
    func_800732CC();
    func_8008DC28();
    func_80088AF8();
    D_80099D9D = 1;
    D_80099DA1 = 3;
    D_8009292C = 0x100;
    D_80099D9E = 0;
    D_80099D9A = 0;
    D_80099DA2 = 0;
    D_80099DA4 = 0x100;
}

void func_80079B04(void) {
}

void func_80079B0C(void) {
    D_80092950 = 0;
    D_8009881E = 0;
    D_80097102 = 0;
    D_80092918 = 0;
    D_80092944 = 0;
    D_800928FC = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079B44);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079D08);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079D6C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079DE0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079DF0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A21C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A344);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A6D0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A730);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A768);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A884);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A958);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007AC3C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007AE10);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007B210);
