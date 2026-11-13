#include "common.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4; // Size: 0x10

typedef struct {
    /* 0x00 */ u8* unk0;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u8 unk8;
    /* 0x09 */ u8 unk9;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ u8 unkE;
    /* 0x0F */ u8 unkF;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ u32 unk2C_0 : 8;
    /* 0x2C */ u32 unk2C_8 : 1;
    /* 0x2C */ u32 unk2C_9 : 1;
    /* 0x2C */ u32 unk2C_10 : 1;
    /* 0x2C */ u32 unk2C_11 : 1;
    /* 0x2C */ u32 unk2C_12 : 1;
    /* 0x2C */ u32 unk2C_13 : 1;
    /* 0x2C */ u32 pad2C : 18;
    /* 0x30 */ u16 unk30;
    /* 0x32 */ u16 pad32;
} BattleWork; // Size: 0x34

extern s32 func_80073DE4(u8* p, s32 arg1);
extern void func_800767C8(u8* p);
extern void func_8007639C(u8* p, s32 arg1);
extern u8 D_80092884;
extern u32 func_800828C4(s32* p);
extern void func_8008F7B8(u8* p);
extern s32 func_8008F720(u8* p, s32 arg1);
extern s32 func_8008F580(u8* p);
extern s32 D_8009284C;
extern s32 func_8008F5B4(u8* p, s32 arg1);
extern void func_8008FFEC(u8* p, u8* arg1);
extern void func_80076424(u8* p);
extern u8 D_80099D98;
extern void func_8008FCC8(u8* p, u8* arg1);
extern void func_8009031C(u8* p, u8* arg1);
extern void func_80090580(u8* p, u8* arg1);
extern void func_80090990(u8* p, u8* arg1);
extern void func_8008EE1C(u8* p, s32 arg1, s32 arg2);
extern s32 func_8008F4F4(u8* p, s32 arg1);

s32 func_8008F530(u8* pArg0, s32 arg1) {
    if (arg1 != 0) {
        return func_80073DE4(pArg0, *(s16*)(pArg0 + 0xBE));
    }
    return *(s16*)(pArg0 + 0xB6) < 0x1000 - *(s16*)(pArg0 + 0xBE);
}

s32 func_8008F570(u8* pArg0) {
    return 0x1000 - *(s16*)(pArg0 + 0xBE);
}

s32 func_8008F580(u8* pArg0) {
    s32 v1;
    s32 v0;

    v1 = *(s16*)(pArg0 + 0xB6);
    v0 = *(s32*)(*(u8**)(pArg0 + 0x15FC) + 0x24);
    return (v0 - 0x200 < v1) ? !(v0 < v1) : 2;
}

s32 func_8008F5B4(u8* pArg0, s32 arg1) {
    u8* p;

    p = *(u8**)(pArg0 + 0x15FC);
    if ((rand() & 0xFF) < ((*(s32*)(p + 0x10) * 5) << 6) >> 4) {
        if (func_8008F530(pArg0, 0) != 0) {
            goto tail;
        }
        if (func_8008F4F4(pArg0, 0xC0) != 0) {
            goto tail;
        }
        if ((rand() & 0xFF) < ((*(s32*)(p + 0x1C) * 3) << 6) >> 4) {
            goto tail;
        }
        if (func_8008F4F4(pArg0, 0x80) == 0) {
            return 0;
        }
        if (*(s16*)(*(u8**)(pArg0 + 0xD8) + 0xB4) < *(s16*)(pArg0 + 0xB4)) {
            goto tail;
        }
        return 0;
    }
    if ((rand() & 0xFF) >= ((*(s32*)(p + 0x1C) * 5) << 6) >> 4) {
        return 0;
    }
tail:
    if ((rand() & 0xFF) < *(s32*)(p + 0x18)) {
        if (*(u8*)(*(u8**)(pArg0 + 0xD8) + 0xC4) == 4) {
            return 0;
        }
        if (func_8008F4F4(*(u8**)(pArg0 + 0xD8), 0x20) != 0) {
            return 1;
        }
        if (*(s16*)(*(u8**)(pArg0 + 0xD8) + 0xB4) < *(s16*)(pArg0 + 0xB4)) {
            return 0;
        }
    }
    return 1;
}
s32 func_8008F720(u8* pArg0, s32 arg1) {
    u8* p1;

    p1 = *(u8**)(pArg0 + 0x15FC);
    if (arg1 != 0) {
        if ((rand() & 0xFF) < *(s32*)(p1 + 0x10)) {
            if (D_8009284C < 0x600) {
                return 0;
            }
        }
    }
    if (func_8008F580(pArg0) != 0) {
        return 1;
    }
    return ((rand() & 0xFF) < *(s32*)(p1 + 0x1C));
}void func_8008F7B8(u8* pArg0) {
    BattleWork* p;

    p = (BattleWork*)pArg0;
    p->unk2C_9 = (rand() & 0xFF) < p->unk10;
    p->unk2C_10 = (rand() & 0xFF) < p->unk14;
    p->unk2C_12 = rand() & 1;
    p->unk2C_11 = (rand() & 0xFF) < p->unk18;
    p->unk2C_8 = ((rand() & 0xFF) < p->unk10) && ((rand() % 10) < 3);
    p->unk2C_0 = rand();
    p->unk30 = *(u16*)(p->unk0 + 0x1668);
}
void func_8008F900(u8* pArg0) {
    if (((*(u32*)(*(u8**)(pArg0 + 0xD8) + 0xD0)) & 0x60000000) == 0x20000000) {
        if ((rand() & 3) != 0) {
            return;
        }
    }
    if (D_80092884 != 0) {
        func_800767C8(pArg0);
        func_8007639C(pArg0, 4);
        func_8007639C(pArg0, 3);
        return;
    }
    if (((*(u32*)(pArg0 + 0xD0)) & 0x60000000) != 0x20000000) {
        func_8007639C(pArg0, 3);
        return;
    }
    func_8007639C(pArg0, 4);
    func_8007639C(pArg0, 3);
}s32 func_8008F9B0(s32* pArg0) {
    s32 sp[4];
    s32 a;
    s32 b;

    *(Vec4*)sp = *(Vec4*)pArg0;
    a = sp[0] - 0x3F80;
    b = sp[2] - 0x3F80;
    sp[0] = a;
    sp[2] = b;
    if (a > 0 && b > 0) {
        return 1;
    }
    return ((func_800828C4(pArg0) & 0x3000000) ^ 0x1000000) < 1;
}extern long ratan2(long y, long x);
extern s32 D_80092934;

s32 func_8008FA2C(s32* pArg0, u8* pArg1) {
    s32 sp[4];
    s32 a;
    s32 b;
    s32 r;

    *(Vec4*)sp = *(Vec4*)pArg0;
    a = sp[0] - 0x3F80;
    b = sp[2] - 0x3F80;
    sp[0] = a;
    sp[2] = b;
    r = ratan2(sp[0], sp[2]);
    *(s16*)(pArg1 + 0xA) = (((r & 0xFFF) >= 0x201) ? 0x800 : 0xC00) - D_80092934;
    *(s16*)(pArg1 + 0xC) = 0xFF;
    return 0;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FACC);
/* 89.78%: instruction multiset matches except gcc lays the two return 0 blocks out
   at the end of the function instead of merging them before the body, and puts
   li v0,1 in a branch delay slot. Also tried: one combined condition (same),
   splitting the two early returns (same), and hoisting the 0x8F4 load before the
   second func_8008F9B0 call (worse: 78%, costs an extra saved register).
extern u8 D_800928C4;

s32 func_8008FACC(s32* pArg0, u8* pArg1) {
    s32 v;

    if (D_800928C4 == 0) {
        return 0;
    }
    if ((*(u32*)((u8*)pArg0 + 0xD0) & 0x8000000) == 0) {
        return 0;
    }
    if (func_8008F9B0(pArg0) != 0) {
        func_8008FA2C(pArg0, pArg1);
        return 1;
    }
    if (func_8008F9B0(*(s32**)((u8*)pArg0 + 0xD8)) == 0) {
        return 0;
    }
    v = *(s32*)(*(u8**)((u8*)pArg0 + 0xD8) + 0x8F4);
    if (v < 0x800) {
        *(u32*)((u8*)pArg0 + 0xD0) |= 0x8000;
        pArg1[0xE] = 1;
        *(s16*)(pArg1 + 0xC) = 0xFF;
        *(s16*)(pArg1 + 0xA) = 0x800;
        *(s32*)((u8*)pArg0 + 0x58) = 0x800;
        return 1;
    }
    if (v < 0x1001) {
        return 1;
    }
    *(u32*)((u8*)pArg0 + 0xD0) &= 0xFFFF7FFF;
    pArg1[0xE] = 0;
    *(s16*)(pArg1 + 0xC) = 0;
    *(s32*)((u8*)pArg0 + 0x58) = 0x800;
    return 1;
}
*/
void func_8008FBD8(u8* pArg0, u8* pArg1) {
    u8 v;

    v = *(u8*)(pArg1 + 0xF);
    if (v >= 2) {
        *(u32*)(pArg0 + 0xD0) |= 2;
        func_80076424(pArg0);
        return;
    }
    if (v != 0) {
        if (!(*(u8*)(pArg1 + 0x2C) & 1)) {
            return;
        }
    } else if (*(u8*)(pArg1 + 0x2C) % 6 != 0) {
        return;
    }
    *(u32*)(pArg0 + 0xD0) |= 2;
    func_80076424(pArg0);
}

void func_8008FC7C(u8* pArg0) {
    u8* p;

    p = *(u8**)(pArg0 + 0x15FC);
    *(s32*)(p + 0x20) = 0;
    *(u8*)(p + 0x8) = 0;
    *(s16*)(p + 0x4) = (2 - *(u8*)(p + 0xF)) * 30 + 0x5A;
    func_8008F7B8(p);
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FCC8);
void func_8008FE80(u8* pArg0) {
    s32 n;
    s32 v;

    v = rand() % 10;
    n = 1;
    if (v >= 2) {
        n = 2;
    }
    if (v >= 5) {
        n++;
    }
    for (; n != 0; n--) {
        func_8007639C(pArg0, (rand() & 1) + 1);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FF24);
/* 94.70%: instruction multiset matches; gcc swaps $a0/$a1 (the *20 product wants
   $a1, it gets $a0) and folds the +1 into the product instead of the remainder.
   Also tried: separate locals for the product and for rand() % 20 + 1, both
   parenthesised groupings, and the terms in the other order (worse: 67-75%).
void func_8008FF24(u8* pArg0, u8* pArg1) {
    if (*(u8*)(*(u8**)(pArg0 + 0xD8) + 0xC4) == 4) {
        if ((*(u32*)(pArg1 + 0x2C) & 0xA00) == 0x200) {
            func_8008F900(pArg0);
            *(s16*)(pArg1 + 0x4) = 3;
        }
    } else {
        func_8008FE80(pArg0);
        *(s16*)(pArg1 + 0x4) = (2 - *(u8*)(pArg1 + 0xF)) * 20 + rand() % 20 + 1;
    }
    func_8008F7B8(pArg1);
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FFEC);
void func_80090174(u8* pArg0) {
    u8* p;
    u8 v;

    p = *(u8**)(pArg0 + 0x15FC);
    p[0x8] = 1;
    func_8008FF24(pArg0, p);
    v = p[0xF];
    if (v >= 2) {
        p[0x9] = rand() % 8 + 1;
    } else if (v != 0) {
        p[0x9] = rand() % 6 + 1;
    } else {
        p[0x9] = rand() % 4 + 1;
    }
    *(s16*)(p + 0xC) = 0;
    p[0xE] = 0;
    func_8008F7B8(p);
}

s32 func_80090258(u8* pArg0, u8* pArg1) {
    if (*(u8*)(pArg0 + 0x911) != 0) {
        if ((rand() & 1) == 0) {
            return 1;
        }
        if ((rand() & 1) != 0 && func_8008F5B4(pArg0, 0) != 0) {
            func_8008F900(pArg0);
            return 0;
        }
        if (D_8009284C < 0x1001) {
            func_8008FFEC(pArg0, pArg1);
            return 0;
        }
        return 1;
    }
    if (func_8008F5B4(pArg0, 0) != 0) {
        func_8008F900(pArg0);
        return 0;
    }
    return 1;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8009031C);
void func_80090504(u8* pArg0) {
    u8* p;
    s32 r;

    p = *(u8**)(pArg0 + 0x15FC);
    *(u8*)(p + 0x8) = 3;
    r = rand();
    *(u8*)(p + 0x9) = r % 4 + 1;
    *(s16*)(p + 0x4) = 0;
    func_8008F7B8(p);
    *(u8*)(p + 0xE) = func_8008F720(pArg0, 1);
    *(u8*)(p + 0x2E) = 0;
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090580);
void func_80090894(u8* pArg0) {
    u8* p;

    p = *(u8**)(pArg0 + 0x15FC);
    p[0x8] = 2;
    if (rand() % 3 == 0) {
        func_800767C8(pArg0);
    }
    *(s32*)(p + 0x28) = rand() % 1536 + 0x100;
    *(s16*)(p + 0x4) = 0;
    p[0x9] = rand() % 5 + 3;
    func_8008F7B8(p);
    p[0xE] = func_8008F720(pArg0, 0);
    p[0x2E] = 0;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090990);
void func_80090C88(u8* pArg0) {
    u8* p1;
    u8* p0;

    p1 = *(u8**)(pArg0 + 0x1600);
    p0 = *(u8**)(pArg0 + 0x15FC);
    *(s32*)(p0 + 0x10) = *(u8*)(p1 + 0x6);
    *(s32*)(p0 + 0x14) = *(u8*)(p1 + 0x7);
    *(s32*)(p0 + 0x18) = *(u8*)(p1 + 0x8);
    *(s32*)(p0 + 0x1C) = *(u8*)(p1 + 0x9);
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090CC0);
void func_80090E10(u8* pArg0) {
    u8* p;

    if ((*(u32*)(pArg0 + 0xD0) & 0x40) != 0) {
        p = *(u8**)(pArg0 + 0x15FC);
        p[0xF] = D_80099D98;
        switch (p[0x8]) {
        case 0:
            func_8008FCC8(pArg0, p);
            break;
        case 1:
            func_8009031C(pArg0, p);
            break;
        case 2:
            func_80090990(pArg0, p);
            break;
        case 3:
            func_80090580(pArg0, p);
            break;
        }
        if (p[0xE] != 0) {
            *(u32*)(pArg0 + 0xD0) |= 0x8000;
        }
        if ((*(u32*)(p + 0x2C) & 0x2000) != 0) {
            *(u32*)(pArg0 + 0xD0) |= 2;
        }
        func_8008EE1C(pArg0, *(s16*)(p + 0xA), *(s16*)(p + 0xC));
    }
}
