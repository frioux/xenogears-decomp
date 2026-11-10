#include "common.h"
#include "psyq/libgte.h"

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
typedef struct {
    /* 0x00 */ u8 pad00[0x2C];
    /* 0x2C */ s16 unk2C;
} UnkA884; // Size: unknown

extern s32 D_80092640;
extern s32 D_80092668;
extern s32 D_8009266C;
extern s32 D_80092670;
extern s32 D_80092674;
extern s32 D_800928AC;
extern s32 D_80092890;
extern s32 D_8009867C;
extern u8* D_80092644;
extern UnkA884* D_8009790C;
extern UnkA884* D_80099028;
extern u8 D_80091150;
extern u8 D_80091151;
extern s32 D_80098730;
extern s32 D_80097014;
extern s32 D_8009879C;
extern s32 D_80097080;
extern s32 D_80098680;
extern s32 D_80098684;
extern s32 D_8009871C;
extern s32 D_80098720;
extern s32 D_80098724;
extern u8 D_800928C0;
extern s32 D_80092A34;
extern s32 D_80092A38;
extern s32 D_80092A44;
extern s32 D_80092A48;
extern s32 D_80092A54;
extern s32 D_80092A58;
extern s32 D_80092A64;
extern s32 D_80092A68;
extern s32 D_8009872C;
extern s32 D_80097010;
extern s32 D_80099068;
extern s32 D_8009794C;

extern void func_800732CC(void);
extern s32 rand(void);
extern void func_8007E24C();
extern void func_8008D580();
extern void func_8007BB7C();
extern void func_8007F834();
extern void func_8008E620();
extern void func_80074BA4(void);
extern void func_80074678();
extern void func_80073B7C();
extern void func_8007C100();
extern s32 func_8007CD44();
extern void func_8007D190();
extern void func_8007D7A8();
extern void func_8007DC74();
extern s32 func_80082488();
extern u32 func_800828C4();
extern void func_800828F8();
extern void func_800776A8();
extern s32 func_80083CD8();
extern void func_80088AF8(void);
extern void func_8008DC28(void);
extern void func_8008EBD0();
extern void func_8008EB88();
extern void func_8008ED6C();

typedef struct ActorScript ActorScript;
typedef struct ActorPart ActorPart;
typedef struct Actor Actor;

// Layout recovered from the offsets this unit touches; only those are known.
struct ActorScript {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ u8 padA[0x1A];
    /* 0x24 */ s16 unk24;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
}; // Size: unknown

typedef struct {
    /* 0x0 */ u8 pad00[0x6];
    /* 0x6 */ u16 unk6;
    /* 0x8 */ u8 pad08[0x2];
    /* 0xA */ u16 unkA;
} ActorSub;

// Actor pointer at 0x15CC, aimed at the actor's own 0x9CC.

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
    /* 0x000 */ s32 unk0;
    /* 0x004 */ s32 unk4;
    /* 0x008 */ s32 unk8;
    /* 0x00C */ u8 pad0C[0x4];
    /* 0x010 */ s32 unk10;
    /* 0x014 */ s32 unk14;
    /* 0x018 */ s32 unk18;
    /* 0x01C */ u8 pad1C[0x30];
    /* 0x04C */ u8 unk4C;
    /* 0x04D */ u8 pad4D[0x2];
    /* 0x04F */ u8 unk4F;
    /* 0x050 */ u8 pad50[0x2];
    /* 0x052 */ u8 unk52;
    /* 0x053 */ u8 pad53;
    /* 0x054 */ s32 unk54;
    /* 0x058 */ u8 pad58[0x4];
    /* 0x05C */ ActorPart* unk5C;
    /* 0x060 */ u8 pad60[0x50];
    /* 0x0B0 */ s32 unkB0;
    /* 0x0B4 */ u8 padB4[0x10];
    /* 0x0C4 */ u8 unkC4;
    /* 0x0C5 */ u8 unkC5;
    /* 0x0C6 */ u8 unkC6;
    /* 0x0C7 */ u8 padC7[0x9];
    /* 0x0D0 */ u32 unkD0;
    /* 0x0D4 */ u32 unkD4;
    /* 0x0D8 */ u8 padD8[0x824];
    /* 0x8FC */ ActorScript* unk8FC;
    /* 0x900 */ u8 pad900[0xA];
    /* 0x90A */ u8 unk90A;
    /* 0x90B */ u8 unk90B;
    /* 0x90C */ u8 pad90C[0x20];
    /* 0x92C */ s32 unk92C;
    /* 0x930 */ s32 unk930;
    /* 0x934 */ s32 unk934;
    /* 0x938 */ s32 unk938;
    /* 0x93C */ s32 unk93C;
    /* 0x940 */ s32 unk940;
    /* 0x944 */ s32 unk944;
    /* 0x948 */ u8 pad948[0x4];
    /* 0x94C */ s32 unk94C;
    /* 0x950 */ s32 unk950;
    /* 0x954 */ s32 unk954;
    /* 0x958 */ u8 pad958[0x4];
    /* 0x95C */ s32 unk95C;
    /* 0x960 */ s32 unk960;
    /* 0x964 */ u8 pad964[0x8];
    /* 0x96C */ s16 unk96C;
    /* 0x96E */ s16 unk96E;
    /* 0x970 */ u8 pad970[0x28];
    /* 0x998 */ s16 unk998;
    /* 0x99A */ s16 unk99A;
    /* 0x99C */ u8 pad99C[0xC30];
    /* 0x15CC */ ActorSub* unk15CC;
    /* 0x15D0 */ u8 pad15D0[0x4];
    /* 0x15D4 */ u8 unk15D4;
}; // Size: unknown

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80078920);

void func_80078D20(Actor* p) {
    s32 tmp[4]; // unused: reproduces the original's 0x10-byte frame slot

    p->unkB0 = func_80082488(p, 1);
    p->unkD0 = (p->unkD0 & 0x9FFFFFFF) | (((func_800828C4(p) >> 24) & 3) << 29);
    if ((p->unkD0 & 0x60000000) == 0x60000000) {
        p->unk90B = 0xF;
    }
    func_800828F8(p, &p->unk10, 0x3E80);
    p->unkD0 &= 0xFFFBFFFF;
    if (p->unkB0 < p->unk4 + p->unk14) {
        p->unkD0 |= 0x40000;
        if (p->unkC4 == 4) {
            if (!(p->unkD4 & 0x10)) {
                func_8008EBD0(p, 0xD, p, 2);
                func_800776A8(p, 6);
            }
            p->unkD4 |= 0x10;
            if (p->unk14 >= 0x40) {
                p->unk14 = -p->unk14 / 3;
            } else {
                p->unk14 = 0;
            }
        } else {
            p->unk14 = 0;
        }
        p->unk4 = p->unkB0;
    }
    p->unk0 += p->unk10;
    p->unk4 += p->unk14;
    p->unk8 += p->unk18;
}

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

/* Best attempt: 98.96%. Only the register names of the p->unkC4 state block and
   the trailing p->unkD0/p->unkC6 update differ (mask in v1 vs a0).
void func_8007920C(Actor* p) {
    ActorScript* pScript = p->unk8FC;
    s32 pos[3][4];
    s32 flag;
    s32 state;

    func_80073B7C(p, pScript->unk1, pScript->unk4, pos[0]);
    func_80073B7C(p, pScript->unk2, pScript->unk6, pos[1]);
    p->unk93C = (pos[0][0] + pos[1][0]) / 2;
    p->unk940 = (pos[0][1] + pos[1][1]) / 2;
    p->unk944 = (pos[0][2] + pos[1][2]) / 2;

    if ((p->unkD0 & 0x80000) && (p->unkD0 & 0x60000000) != 0x20000000 && !(p->unk90A & 1)) {
        func_8007D190(pos[0], 9);
        func_8007D190(pos[1], 9);
    }

    if (p->unk90B != 0) {
        if (pos[0][1] + 0x40 > p->unkB0 - 0x20) {
            if (p->unk96E + 6 < pos[0][1]) {
                func_8007D7A8(pos[0], 0x20);
            }
        }
        if (pos[1][1] + 0x40 > p->unkB0 - 0x20) {
            if (p->unk96C + 6 < pos[1][1]) {
                func_8007D7A8(pos[1], 0x20);
            }
        }
        p->unk90B--;
    }

    if ((p->unkD0 & 0x60000000) == 0x20000000) {
        func_8007DC74(&p->unk93C, &p->unk95C);
    }

    if (func_80083CD8() != 7 && (p->unkD0 & 0x60000000) == 0x20000000 && p->unk940 >= 0x81 &&
        p->unk960 < 0x80) {
        func_8008EB88(p, 0x3A, &p->unk93C, 2);
    }

    p->unk96E = pos[0][1];
    p->unk96C = pos[1][1];
    func_80073B7C(p, pScript->unk0, pScript->unk8, &p->unk92C);

    p->unk930 -= 0x38;
    p->unk94C = (p->unk93C + p->unk92C) / 2;
    p->unk954 = (p->unk944 + p->unk934) / 2;
    p->unk950 = (p->unk940 + p->unk930) / 2;

    if ((p->unkD0 & 0x8000) && p->unkC5 == 0 && (p->unkC4 & 1)) {
        func_8007C100(&p->unk15D4);
        flag = (p->unkD0 >> 27) & 1;
        func_8007CD44(flag, &p->unk92C, &p->unk93C, flag);
        if (!(p->unkD0 & 0x10000)) {
            func_8008ED6C(p, 8);
            func_8008EBD0(p, 8, &p->unk93C, 2);
        }
    }

    if (p->unkC5 != 4) {
        if (p->unkC4 != 1) {
            p->unkD4 &= ~3;
        } else {
            p->unkD4 = (p->unkD4 & ~3) | 1;
            if (p->unkD0 & 0x8000) {
                p->unkD4 = (p->unkD4 & ~3) | 2;
            }
        }
        if (p->unkC5 == 2) {
            p->unkD4 |= 3;
        }
    }

    state = p->unkD4 & 3;
    if (state != ((p->unkD4 >> 2) & 3)) {
        switch (state) {
        case 0:
        case 3:
            func_8008ED6C(p, 7);
            func_8008ED6C(p, 9);
            break;
        case 1:
            func_8008EBD0(p, 7, p, 2);
            func_8008ED6C(p, 9);
            break;
        case 2:
            func_8008EBD0(p, 9, &p->unk93C, 2);
            func_8008ED6C(p, 7);
            break;
        }
    }

    p->unkD0 = (p->unkD0 & 0xFFFEFFFF) | (((p->unkD0 >> 15) & 1) << 16);
    p->unkC6 = p->unkC4;
    p->unkD4 = (p->unkD4 & 0xFFFFFFF3) | ((p->unkD4 & 3) << 2);
}
*/

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

void func_80079D08(Actor* p) {
    p->unk15CC->unkA = (p->unk15CC->unkA & 0xFEFF) | ((p->unkD0 << 6) & 0x100);
    p->unk15CC->unk6 = (p->unk15CC->unk6 & 0xDFFF) | ((p->unkD0 >> 2) & 0x2000);
    p->unk15CC->unk6 = (p->unk15CC->unk6 & 0xBFFF) | ((p->unkD0 >> 5) & 0x4000);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079D6C);

/* Best attempt: 91.03%. Everything matches except the third term: the original
   materialises the bit-14 test as `sll 17; slti 0` while every source shape
   tried here gives gcc's canonical `sll 17; srl 31` (a local, a u32 local, a
   multiply, `!= 0` on the masked sign bit, and the direct expression all fold
   the same way), and `li a1,-5` lands one slot earlier than the original.
void func_80079D6C(Actor* p) {
    u32 flag;

    p->unkD0 = (p->unkD0 & ~4) | ((p->unk15CC->unkA >> 6) & 4);
    p->unkD0 = (p->unkD0 & 0xFFFF7FFF) | ((p->unk15CC->unk6 << 2) & 0x8000);
    flag = ((p->unk15CC->unk6 << 17) & 0x80000000) != 0;
    p->unkD0 = (p->unkD0 & 0xFFF7FFFF) | (flag << 19);
}
*/

void func_80079DE0(void) {
    D_80092640 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079DF0);

void func_8007A21C(s32 unk0) {
    if (unk0 < 0xFF) {
        D_800928AC = unk0 - 2;
        D_800928C0 -= unk0;
    } else {
        D_800928AC = 0xFF;
    }
    *(VECTOR*)&D_80092A34 = *(VECTOR*)&D_8009872C;
    *(VECTOR*)&D_80092A44 = *(VECTOR*)&D_80097010;
    *(VECTOR*)&D_80092A54 = *(VECTOR*)&D_80099068;
    *(VECTOR*)&D_80092A64 = *(VECTOR*)&D_8009794C;
    D_80092A68 = 0x100;
    D_80092A58 = 0x100;
    D_80092A48 = 0x100;
    D_80092A38 = 0x100;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A344);

void func_8007A6D0(Actor* p) {
    D_8009292C = 0x100;
    if (D_800928AC < 0x78) {
        p->unk4F = 0x10;
        p->unk52 = 0;
        p->unk4C = 0x10;
        p->unkD0 |= 0x400;
    } else {
        p->unk4F = 0x10;
        p->unk52 = 0;
        p->unk4C = 0;
        p->unkD0 |= 0x400;
    }
}

void func_8007A730(Actor* p) {
    D_8009292C = 0x100;
    p->unk4F = 0x10;
    p->unk52 = 0;
    p->unk4C = 9;
    p->unkD0 |= 0x2000400;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A768);

/* Best attempt: 92.44%. Only the D_80098680/D_8009867C/D_80098720 block differs:
   post-reload scheduling batches the p->unk0 and D_80092670 loads ahead of the
   D_80098680 store, and `p->unk4 - 0x400 - D_80092670` reassociates into
   `p->unk4 - (D_80092670 + 0x400)` (addiu on the loaded global). A `y` local
   keeps the addiu on p->unk4 but lengthens its live range and still batches.
void func_8007A768(Actor* p) {
    ActorScript* pScript = p->unk8FC;
    s32 r;

    D_80092668 = pScript->unk24;
    D_8009266C = pScript->unk28;
    D_80092670 = pScript->unk26;
    D_80092674 = pScript->unk2A;
    SetGeomScreen(0x200);
    r = rand();
    D_80098680 = p->unk4;
    D_8009867C = p->unk0;
    D_80098720 = (p->unk4 - 0x400) - D_80092670;
    D_80098684 = p->unk8;
    D_8009871C = D_8009867C + (((rcos(r) << 2) * D_80092674) >> 12);
    D_80098724 = D_80098684 + (((rsin(r) << 2) * D_80092674) >> 12);
}
*/

void func_8007A884(void) {
    UnkA884* p;
    s32 v;

    func_8007E24C();
    func_8008D580(D_80092644);
    func_8007BB7C();
    func_8007F834();
    func_8008E620();
    p = (D_80092890 != 0) ? D_8009790C : D_80099028;
    if (p->unk2C != 0) {
        D_80091150 = 1;
        v = 0x10;
    } else {
        D_80091150 = 2;
        v = -1;
    }
    D_80091151 = v;
    D_80098730 = 0x100;
    D_80097014 = 0x100;
    D_8009879C = 0x100;
    D_80097080 = 0x100;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A958);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007AC3C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007AE10);

void func_8007B210(Actor* p, u8 unk4C) {
    p->unk15CC = (ActorSub*)((u8*)p + 0x9CC);
    p->unk4F = 0x10;
    p->unk4C = unk4C;
    p->unk52 = 0;
    D_8009292C = 0x100;
    func_80074BA4();
    func_80074678(p, p->unk998, p->unk99A);
}
