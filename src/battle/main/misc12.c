#include "common.h"

extern u8 D_800D2D5C[];
extern u16 D_800D2D70[];
typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 unk2[0x1A];
} BattleUnk1C; // Size: 0x1C

extern BattleUnk1C D_800C3EBE[];
extern u8 D_800C3EB4[][0x1C];
extern u16 D_800C3D64;
extern u8 D_800C3E2C;
extern u16 BattleMaskHasSlotBit(u16, u8);
extern u16 BattleSlotBitMask(u8);
extern u8 BattlePairTargetable(u8, u8);
extern u8 *D_800C3EAC;
extern u8 D_800C3E90[];
extern u8 D_800C3E9B[];
extern u16 D_800C3EC0[];
extern long ratan2(long y, long x);
extern u8 D_800C3E9B[];
extern u8 D_800D3274;
extern void func_800841E0(void);

typedef struct {
    /* 0x00 */ u16 unk0[12];
    /* 0x18 */ u8 unk18[12];
    /* 0x24 */ u16 unk24[12];
    /* 0x3C */ u8 unk3C[12];
} BattleUnk48; // Size: 0x48

extern BattleUnk48 D_800C3FE8[];

typedef struct {
    /* 0x000 */ u16 unk0;
    /* 0x002 */ u8 pad2[0x6];
    /* 0x008 */ u16 unk8;
    /* 0x00A */ u8 padA[0x56];
    /* 0x060 */ u16 unk60;
    /* 0x062 */ u16 unk62;
    /* 0x064 */ u8 pad64[0x40];
    /* 0x0A4 */ u16 unkA4;
    /* 0x0A6 */ u8 padA6[0x2];
    /* 0x0A8 */ u16 unkA8;
    /* 0x0AA */ u8 padAA[0xC6];
} BattleUnk170; // Size: 0x170

extern u8 D_800D2DCC[];
extern u8 D_800C3EB7[][0x1C];
extern u8 D_800D32A1[];
extern u8 *D_800D3364;
extern BattleUnk170 D_800CCD64[];
extern BattleUnk170 D_800CCE08[];

typedef struct {
    /* 0x00 */ u8 unk0[0x24];
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u16 unk26;
    /* 0x28 */ u16 unk28;
    /* 0x2A */ u16 unk2A;
    /* 0x2C */ u16 unk2C;
    /* 0x2E */ u16 unk2E;
    /* 0x30 */ u16 unk30;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u16 unk34;
    /* 0x36 */ u16 unk36;
    /* 0x38 */ u8 unk38[0x4];
    /* 0x3C */ u8 unk3C;
    /* 0x3D */ u8 unk3D[0x3];
} BattleActorSlot; // Size: 0x40

// 0x2F8-byte block of actor slots plus the slot menu state that follows them.
typedef struct {
    /* 0x000 */ BattleActorSlot slot[0xB];
    /* 0x2C0 */ u8 unk2C0[0x1D];
    /* 0x2DD */ u8 unk2DD;
    /* 0x2DE */ u8 unk2DE[0xC];
    /* 0x2EA */ u8 unk2EA;
    /* 0x2EB */ u8 unk2EB[0xB];
    /* 0x2F6 */ u8 unk2F6;
    /* 0x2F7 */ u8 unk2F7;
} BattleActorSlots; // Size: 0x2F8

extern u8 D_800D3014;
extern void BattlePlaySoundIfEnabled(u8);
extern u8 func_8008BED8(u8);
extern void func_8008B908(u8);
extern u8 D_800C3E29;
extern void func_800826CC(u8);
void BattleAdvanceSlotMenuState(u8 arg0) {
    u8 a;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        func_800826CC(a);
        ((BattleActorSlots *)D_800C3EAC)->unk2DE[0] = 1;
        break;
    case 0:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk26 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 7;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 0) {
            if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2E != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)D_800C3EAC)->unk2DD = 1;
            }
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 1:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk32 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 8;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2C == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 9;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 2) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 3;
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 3:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk30 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 4;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}
extern void BattleSetupActorSlotDisplay(u8);
extern void BattleEnsureTargetSlotInCandidateList(u8);
extern void func_80077698(void);
extern u8 D_800D366C;
void BattleSetActorSlotDisplayMode(u8 arg0) {
    u8 a;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk3C == 0xFF) {
            BattlePlaySoundIfEnabled(0x4F);
            break;
        }
        BattleSetupActorSlotDisplay(a);
        BattleEnsureTargetSlotInCandidateList(a);
        ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x19;
        func_80077698();
        D_800D366C = 0;
        break;
    case 1:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x12;
        break;
    case 3:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk28 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x13;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 3) {
            if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk24 != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x18;
            }
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 0:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk36 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x15;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}
void BattleSetActorSlotMenuMode(u8 arg0) {
    u8 a;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        if (func_8008BED8(a) != 0) {
            func_8008B908(a);
        } else {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x11;
        }
        break;
    case 0:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2E == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x12;
        break;
    case 3:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk28 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x13;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 3) {
            if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk24 != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x18;
            }
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 1:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}
extern void BattleSetSlotFlag15A1(u8);
extern u16 D_800D2C32;
void func_80082D4C(u8 arg0) {
    u8 a;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        BattleSetSlotFlag15A1(a);
        D_800CCD64[a].unk60 = D_800CCD64[a].unk60 + D_800D2C32;
        if (D_800CCD64[a].unk60 > D_800CCD64[a].unk62) {
            D_800CCD64[a].unk60 = D_800CCD64[a].unk62;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2EB[a] = 1;
        ((BattleActorSlots *)D_800C3EAC)->unk2EA = 0;
        ((BattleActorSlots *)D_800C3EAC)->unk2DE[0] = 1;
        break;
    case 0:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2E == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 1:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 3:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk28 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x13;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 3) {
            if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk24 != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x18;
            }
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2C == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x17;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}
extern void BattleLoadStringTableAE(u8);
extern u8 func_8008B478(u8);
extern void func_8008ADD0(u8);
void BattleAdvanceSlotMenuStateAE(u8 arg0) {
    u8 a;
    u8 *p;
    int off;
    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        BattleLoadStringTableAE(a);
        if (func_8008B478(a) != 0) {
            func_8008ADD0(a);
        } else {
            D_800C3EAC[0x2DD] = 0x13;
        }
        break;
    case 0:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x2E) == 0) {
            p[0x2DD] = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 1:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x34) == 0) {
            p[0x2DD] = 0x11;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        D_800C3EAC[0x2DD] = 0x12;
        break;
    case 3:
        p = D_800C3EAC;
        off = a * 0x40;
        if (*(u16 *)(p + off + 0x24) == 0) {
            p[0x2DD] = 0x18;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}

void func_800830A8(u8 arg0) {
    u8 a;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        if ((D_800CCD64[a].unkA8 & 0x8000) != 0) {
            D_800CCD64[a].unkA8 &= 0x7FFF;
            D_800CCD64[a].unk8 &= 0x7FFF;
        } else {
            D_800CCD64[a].unkA4 &= 0xFFDF;
            D_800CCD64[a].unk0 &= 0xEFFF;
            D_800CCD64[a].unkA8 |= 0x8000;
            D_800CCD64[a].unk8 |= 0x8000;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2DE[0] = 1;
        break;
    case 1:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2C == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x17;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 2) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x12;
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 3:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk24 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x18;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 3) {
            if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk28 != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x13;
            }
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 0:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2E == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x10;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083340);
extern u8 D_800C48EA;
extern s32 func_8009A9D0(u8);
void BattleAdvanceSlotMenuStateRandom(u8 arg0) {
    u8 a;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        if (func_8009A9D0(((BattleActorSlots *)D_800C3EAC)->unk2C0[0x13]) != 0) {
            D_800C48EA = 0x40;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2DE[0] = 1;
        break;
    case 0:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk36 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x15;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 0) {
            if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2E != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x10;
            }
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 1:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 3:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk24 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x18;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 3) {
            if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk28 != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x13;
            }
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x12;
        break;
    }
}
extern void BattleLoadStringTable96(u8);
extern u8 func_8008CFB8(u8);
void BattleAdvanceSlotMenuStateText(u8 arg0) {
    u8 a;

    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        BattleLoadStringTable96(a);
        if (func_8008CFB8(a) != 0) {
            func_8008ADD0(a);
        } else {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x18;
        }
        break;
    case 0:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk36 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x15;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 0) {
            if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2E != 0) {
                BattlePlaySoundIfEnabled(0x4F);
            } else {
                ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x10;
            }
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 1:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk34 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x16;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 2:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk2C == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x17;
            break;
        }
        if (((BattleActorSlots *)D_800C3EAC)->unk2F6 != 0 && D_800C3E29 == 2) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x12;
            ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 0;
            break;
        }
        ((BattleActorSlots *)D_800C3EAC)->unk2F6 = 1;
        BattlePlaySoundIfEnabled(0x4F);
        break;
    case 3:
        if (((BattleActorSlots *)D_800C3EAC)->slot[a].unk28 == 0) {
            ((BattleActorSlots *)D_800C3EAC)->unk2DD = 0x13;
            break;
        }
        BattlePlaySoundIfEnabled(0x4F);
        break;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083948);
u8 BattlePairTargetable(u8 arg0, u8 arg1) {
    u8 ret;

    ret = 0;
    if (D_800D2DCC[arg1] != 0) {
        if (D_800C3EB7[arg1][0] == 0) {
            if (D_800D32A1[arg1 * 8] == 0) {
                if (*(u8 *)(D_800D3364 + (D_800C3EB4[arg0][0] * 0x40 +
                                          D_800C3EB4[arg1][0] * 8) + 0x140) == 0) {
                    ret = (D_800CCD64[arg1].unk0 & 0xC001) == 0;
                }
            } else if ((D_800CCE08[arg1].unk0 & 0xC001) == 0) {
                ret = 1;
            }
        }
    }
    return ret;
}
u8 BattleActorTargetableWithOverride(u8 arg0, u8 arg1) {
    u8 ret;

    ret = 0;
    if (D_800D2DCC[arg0] != 0) {
        if (D_800C3EB7[arg0][0] == 0) {
            if (D_800D32A1[arg0 * 8] == 0) {
                if (arg1 == 0) {
                    ret = (D_800CCD64[arg0].unk0 & 0xC001) == 0;
                } else {
                    ret = 1;
                }
            } else {
                if (arg1 == 0) {
                    if ((D_800CCE08[arg0].unk0 & 0xC001) == 0) {
                        ret = 1;
                    }
                } else {
                    ret = 1;
                }
            }
        }
    }
    return ret;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_800841E0);
u8 BattleBuildTargetCandidateListByGroup(u8 arg0, u8 arg1, u8 arg2) {
    s32 i;
    s32 j;
    s32 n;
    s32 count1;
    u8 start1;
    s32 count2 = 0;
    u8 start2;
    u8 *p;
    u8 val;

    val = 0xFF;
    i = 0xB;
    p = D_800C3E9B;
    for (; i >= 0; i--) {
        *p-- = val;
    }
    switch (arg0) {
    case 0:
        start1 = 3;
        count1 = 8;
        break;
    case 1:
        start1 = 0;
        count1 = 3;
        break;
    case 2:
        if (arg2 == 0) {
            start1 = 3;
            count1 = 8;
            start2 = 0;
            count2 = 3;
        } else {
            start1 = 0;
            count1 = 3;
            start2 = 3;
            count2 = 8;
        }
        break;
    }
    n = 0;
    D_800D3274 = 0;
    D_800C3D64 = 0;
    for (i = start1; --count1 >= 0; i++) {
        j = i;
        if (BattleActorTargetableWithOverride(j, arg1) != 0) {
            D_800C3E90[n] = j;
            D_800C3D64 |= BattleSlotBitMask(j);
            n++;
            D_800D3274++;
        }
    }
    for (i = start2; --count2 >= 0; i++) {
        j = i;
        if (BattleActorTargetableWithOverride(j, arg1) != 0) {
            D_800C3E90[n] = j;
            D_800C3D64 |= BattleSlotBitMask(j);
            n++;
            D_800D3274++;
        }
    }
    return D_800C3E90[0];
}
u8 BattleBuildTargetCandidateList(u8 arg0) {
    s32 i;
    s32 j;
    s32 n;
    s32 k;
    u8 *p;
    u8 val;

    k = 8;
    val = 0xFF;
    i = 11;
    p = D_800C3E9B;
    for (; i >= 0; i--) {
        *p-- = val;
    }
    n = 0;
    D_800D3274 = 0;
    D_800C3D64 = 0;
    for (i = 3; --k >= 0; i++) {
        j = i;
        if (BattlePairTargetable(arg0, j) != 0) {
            D_800C3E90[n] = j;
            D_800C3D64 |= BattleSlotBitMask(j);
            n++;
            D_800D3274++;
        }
    }
    return D_800C3E90[0];
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084854);
/* 73.70%: body shape matches (do-while over D_800C3E90..D_800C3E9B, switch on
   arg1, squared-distance minimum); remaining diff is register allocation -
   retail keeps 7 saved registers, this keeps 9, because gcc hoisted the
   D_800C3EC0 base and D_800C3E90 base into saved registers that retail
   rematerialises per use. Tried: idx local, for-loop, do-while with
   ++p in the condition, nested if vs continue.
u8 func_80084854(u8 arg0, u8 arg1) {
    u8 *p;
    u8 result;
    s32 mode;
    s32 angle;
    s32 flag;
    s32 dx;
    s32 dy;
    s32 dist;
    u32 best;

    result = arg0;
    mode = arg1;
    best = 0xFFFFFF;
    p = D_800C3E90;
    do {
        if (*p != 0xFF && *p != arg0) {
            angle = ratan2(D_800C3EC0[*p * 14] - D_800C3EC0[arg0 * 14],
                           D_800C3EBE[*p].unk0 - D_800C3EBE[arg0].unk0);
            flag = 0;
            switch (mode) {
            case 0:
                if ((u16)(angle + 0x200) < 0x400) {
                    flag = 1;
                }
                break;
            case 1:
                if ((u16)(angle + 0x600) < 0x400) {
                    flag = 1;
                }
                break;
            case 2:
                if ((u16)(angle + 0x800) < 0x200) {
                    flag = 1;
                }
                if ((u16)(angle - 0x600) < 0x201) {
                    flag = 1;
                }
                break;
            case 3:
                if ((u16)(angle - 0x200) < 0x400) {
                    flag = 1;
                }
                break;
            }
            if (flag != 0) {
                dx = D_800C3EC0[*p * 14] - D_800C3EC0[arg0 * 14];
                if (dx < 0) {
                    dx = -dx;
                }
                dist = dx * dx;
                dy = D_800C3EBE[*p].unk0 - D_800C3EBE[arg0].unk0;
                if (dy < 0) {
                    dy = -dy;
                }
                dist += dy * dy;
                if (dist < best) {
                    best = dist;
                    result = *p;
                }
            }
        }
    } while (++p < D_800C3E9B);
    return result;
}
*/

void BattleEnsureTargetSlotInCandidateList(u8 arg0) {
    s32 off;
    s32 found;
    s32 i;

    off = arg0 * 0x40;
    D_800C3EAC[0x2E8] = (D_800C3EAC + off)[0x3C];
    found = 0;
    func_800841E0();
    for (i = 0; i < D_800D3274; i++) {
        if (D_800C3E90[i] == (D_800C3EAC + off)[0x3C]) {
            found++;
            break;
        }
    }
    if (found == 0) {
        D_800C3EAC[0x2E8] = D_800C3E90[0];
    }
}
extern void BattleBeginAnimTask(u16);
extern void func_800BCD98(u16);
extern void BattleUpdateFrame(void);
extern u8 func_80084854(u8, u8);
s32 BattleSelectTargetSlotLoop(u8 arg0) {
    u8 b;
    u8 m;
    u8 state;

    b = ((BattleActorSlots *)D_800C3EAC)->unk2DE[0xA];
    state = 2;
    D_800D3014 = 8;
    while (1) {
        while (D_800D3014 == 8) {
            m = b;
            BattleBeginAnimTask(BattleSlotBitMask(arg0) | BattleSlotBitMask(m));
            func_800BCD98(BattleSlotBitMask(m));
            BattleUpdateFrame();
        }
        switch (D_800D3014) {
        case 4:
        case 6:
        case 7:
            state = 1;
            ((BattleActorSlots *)D_800C3EAC)->slot[arg0].unk3C = b;
            break;
        case 5:
            BattleBeginAnimTask(BattleSlotBitMask(arg0) |
                          BattleSlotBitMask(((BattleActorSlots *)D_800C3EAC)->slot[arg0].unk3C));
            state = 0;
            func_800BCD98(BattleSlotBitMask(((BattleActorSlots *)D_800C3EAC)->slot[arg0].unk3C));
            break;
        case 0:
            b = func_80084854(b, 0);
            break;
        case 1:
            b = func_80084854(b, 1);
            break;
        case 2:
            b = func_80084854(b, 2);
            break;
        case 3:
            b = func_80084854(b, 3);
            break;
        }
        D_800D3014 = 8;
        if (state != 2) {
            break;
        }
    }
    return state;
}

u16 BattleGetSameRowSlotMask(void) {
    s32 i;
    u16 ret;

    ret = 0;
    for (i = 0; i < 0xB; i++) {
        if (BattleMaskHasSlotBit(D_800C3D64, i) != 0) {
            if (D_800C3EB4[D_800C3E2C][0] == D_800C3EB4[i][0]) {
                ret |= BattleSlotBitMask(i);
            }
        }
    }
    return ret;
}
extern u16 BattleSlotBitMaskInverted(u8);
extern u8 D_800C3E9A[];
/* 99.81% at best: every instruction and the frame (0x30, 7 saved regs) match
   except the order of the two callee-saved parameter copies in the entry block -
   retail emits sw s6,0x28 / move s6,a0 before sw s1,0x14 / move s1,a2, gcc 2.6.0
   here emits the s1 pair first. Permuter stalls at 30 on this body, two runs.
   The tail fill loop is what took it from 98.35% to 99.81%: the 0x4000 branch's
   countdown must reuse n, not a fresh local, so the counter lands in s1 as in
   retail (val stays a u8 0xFF; p = D_800C3E9A; for (; n >= 0; n--) *p-- = val).
   n = 0 must also precede the D_800C3E2C = 0xFF store in the 0x8000 branch.
   Tried, worse or neutral: swapping the flags/hi assignments (98.62%), a separate
   j local for the countdown (99.63%), flags declared first, other declaration
   orders of i/n/off/off8/val.
   Note for an un-park: the two offsets must stay explicit source variables
   (off = 0x170 * i, off8 = 8 * i, both from 0); writing D_800CCD64[i].unk0 and
   D_800D32A1[i * 8] makes gcc fold the D_800D32A1 base into the pointer
   (lui/addiu in the preheader instead of move s5,zero) and it loses 5%.
u16 func_80084DE4(u16 arg0, u16 arg1, u8 arg2, u8 arg3, u8 arg4) {
    s32 i;
    s32 n;
    s32 off;
    s32 off8;
    u16 flags;
    u16 mask;
    u8 grp;
    u8 first;
    u8 *p;
    u8 val;
    s32 hi;

    flags = arg0;
    hi = 0;
    switch (arg3) {
    case 0:
        flags = arg1;
        break;
    case 1:
        flags = 0x3000;
        break;
    case 2:
        flags = 0x2001;
        break;
    }
    if ((flags & 0x8000) != 0) {
        hi = 1;
    }
    if ((flags & 0x1000) != 0) {
        first = 0;
        grp = 0;
        mask = 0xFFF8;
    } else {
        first = 1;
        grp = 1;
        mask = 7;
    }
    if ((flags & 0x2000) != 0) {
        grp = 2;
        mask = 0xFFFF;
    }
    if (arg4 != 0) {
        D_800C3E2C = BattleBuildTargetCandidateList(arg2);
    } else {
        D_800C3E2C = BattleBuildTargetCandidateListByGroup(grp, hi, first);
    }
    D_800C3D64 &= mask;
    if ((flags & 0x8000) != 0) {
        n = 0;
        D_800C3E2C = 0xFF;
        for (i = 0, off = 0, off8 = 0; i < 0xB; i++, off8 += 8) {
            if (BattleMaskHasSlotBit(D_800C3D64, i) != 0) {
                if (D_800D32A1[off8] == 0) {
                    if ((*(u16 *)((u8 *)D_800CCD64 + off) & 0x8000) != 0) {
                        D_800C3E2C = i;
                        D_800C3E90[n] = i;
                        n++;
                    } else {
                        D_800C3D64 &= BattleSlotBitMaskInverted(i);
                    }
                } else {
                    if ((*(u16 *)((u8 *)D_800CCE08 + off) & 0x8000) != 0) {
                        D_800C3E2C = i;
                        D_800C3E90[n] = i;
                        n++;
                    } else {
                        D_800C3D64 &= BattleSlotBitMaskInverted(i);
                    }
                }
            }
            off += 0x170;
        }
        while (n < 0xB) {
            D_800C3E90[n] = 0xFF;
            n++;
        }
    } else if ((flags & 0x4000) != 0) {
        D_800C3E2C = arg2;
        D_800C3D64 = BattleSlotBitMask(arg2);
        val = 0xFF;
        n = 10;
        p = D_800C3E9A;
        for (; n >= 0; n--) {
            *p-- = val;
        }
    }
    return flags & 0xFFFF;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084DE4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80085084);
u32 BattleActorPosXGreaterThan(u8 arg0, u8 arg1) {
    return D_800C3EBE[arg0].unk0 > D_800C3EBE[arg1].unk0;
}
void BattleClearSlotCurveState(void) {
    s32 i;

    for (i = 0; i < 11; i++) {
        D_800D2D5C[i] = 0xFF;
        D_800D2D70[i] = 0;
    }
}
void BattleClearSlotCurveEntry(void) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
        D_800C3FE8[D_800C3EAC[0x2DA]].unk0[i] = 0;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk18[i] = 0xFF;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk24[i] = 0;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk3C[i] = 0xFF;
    }
}
extern u16 D_800D2C54[][2];
extern u8 D_800D2C88[];
extern u8 D_800D2D5C[];
extern u16 D_800D2D70[];
void BattleApplySlotCurve(u8 arg0) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
        D_800C3FE8[arg0].unk0[i] = D_800D2C54[i][0];
        D_800C3FE8[arg0].unk18[i] = D_800D2C88[i];
        switch (D_800D2C88[i]) {
        case 0:
        case 5:
            if (D_800D2D5C[i] == 0 || D_800D2D5C[i] == 5) {
                D_800D2D70[i] = D_800D2D70[i] + D_800D2C54[i][0];
            } else if (D_800D2D5C[i] == 2) {
                if ((s16)D_800D2C54[i][0] - (s16)D_800D2D70[i] >= 0) {
                    D_800D2D70[i] = (s16)D_800D2C54[i][0] - (s16)D_800D2D70[i];
                    D_800D2D5C[i] = 0;
                } else {
                    D_800D2D70[i] = (s16)D_800D2D70[i] - (s16)D_800D2C54[i][0];
                }
            } else {
                D_800D2D70[i] = D_800D2C54[i][0];
                D_800D2D5C[i] = D_800D2C88[i];
            }
            break;
        case 2:
            if (D_800D2D5C[i] == 2) {
                D_800D2D70[i] = D_800D2D70[i] + D_800D2C54[i][0];
            } else if (D_800D2D5C[i] == 0 || D_800D2D5C[i] == 5) {
                if ((s16)D_800D2C54[i][0] - (s16)D_800D2D70[i] < 0) {
                    D_800D2D70[i] = (s16)D_800D2D70[i] - (s16)D_800D2C54[i][0];
                } else {
                    D_800D2D70[i] = (s16)D_800D2C54[i][0] - (s16)D_800D2D70[i];
                    D_800D2D5C[i] = 2;
                }
            } else {
                D_800D2D70[i] = D_800D2C54[i][0];
                D_800D2D5C[i] = D_800D2C88[i];
            }
            break;
        }
        D_800C3FE8[arg0].unk24[i] = D_800D2D70[i];
        D_800C3FE8[arg0].unk3C[i] = D_800D2D5C[i];
    }
}
