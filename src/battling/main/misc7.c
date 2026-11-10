#include "common.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/libgpu.h"
#include "system/controller.h"

/* The nine flag bytes below are declared as open arrays on purpose: -G8
   classifies a 1-byte extern as small data, which makes its address one
   gp-relative insn, so gcc rematerialises it at each use instead of holding
   it in a saved register across the call the way the original does. */
extern u8 D_80099D98[];
extern u8 D_80099D9A[];
extern u8 D_80099DA1[];
extern u8 D_80099D9B[];
extern u8 D_80099D9C[];
extern u8 D_80099D9D[];
extern u8 D_80099D9E[];
extern u8 D_80099D9F[];
extern u8 D_80099DA2[];
extern u32 D_800912F4[];
extern s32* D_80092734;
extern u8 D_80092884;
extern s32 D_800912F0;
extern s32 D_80098800[];
extern s32 D_800970E4[];
extern s32 D_80092950;
extern u8* D_8009132C[];
extern u8 D_80091300[];
extern u8 D_8006FF7C[];
extern char D_8006FF60[];
extern s32 D_8009274C;
extern s32 D_80092750;
extern u8* D_8009270C;
extern s32 D_80092700;
extern s32 D_80092704;
extern s32 D_80092710;
extern s32 D_80092714;
extern s32 D_80092720;
extern s32 D_80091364;
extern s32 D_80092888;
extern s32 D_800928C8;
extern s32 D_80092748;
extern s32** D_800928EC;
extern u8 D_8006FF5C[];
extern u8 D_8006FFA0[];
extern u8 D_8006FFA4[];
extern u8 D_8006FFA8[];
extern u8 D_8006FF68[];
extern u8 D_8006FF6C[];
extern u8 D_8006FF74[];
extern s32 D_8009130C[];
extern s32 D_8009292C;
extern u16 D_80099DA4[];

void func_8007EE08(s32 arg);
void func_8007F834(void);
void func_80080C48(s32 arg);
void func_80083C0C(s32 mode);
void func_8007E894(s32 a, s32 b);
void func_8007ECF0(u8* pText);
void func_80081100(s32 entry, s32 mode);
void func_8008EB4C(s32 arg);
void ArchiveCdDataSync(int mode);
void func_8008509C(s32 index, s32 value);
void func_80085134(s32 index);
s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags);
s32 func_8007FF70(s32 value, s32 max, s32 flags);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F854);

/* 96.25%: everything matches except the currency allocation, which is mirrored
   - the original loads/stores D_80098800 side through a1/v1 and D_800970E4
   through a0/v0, ours picks the pair the other way round, so both value and
   address registers are swapped (and the two lw/ori/sw groups with them).
   Also tried: statement order reversed (95.42), no tmp array (96.08).
   The D_80098800[]/D_800970E4[] open-array declarations below are what got it
   this far: a scalar s32 extern is small data under -G8, and then gcc
   rematerialises each address instead of holding it in a register. */
/* void func_8007F854(void) {
    s32 tmp[2];

    D_800912F0 = 1;
    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
    D_80098800[0] |= 0xC;
    D_800970E4[0] |= 0xC;
} */

void func_8007F8B4(void) {
    s32 tmp[2]; // reproduces the original's unused 8-byte stack slot

    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F8E4);

/* 87%: gcc hoists the merged `li a0,3` into the bnez delay slot and folds the
   second test into a beq to the shared call; the original keeps `j; li a0,3`
   as a middle block. Also tried: mode local, `if (A == 0 && B != 1)`. */
/* void func_8007F8E4(void) {
    func_80080C48(0);
    if (D_80099D9F != 0) {
        func_80083C0C(3);
    } else if (D_80092950 == 1) {
        func_80083C0C(3);
    } else {
        D_80092950--;
        func_80083C0C(6);
    }
} */

typedef struct {
    /* 0x0 */ u8 unk0[0x12];
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ u8 unk16[0xE];
    /* 0x24 */ s16 unk24;
    /* 0x26 */ u8 unk26[2];
    /* 0x28 */ s16 unk28;
} Misc7Struct; // Size: 0x2A

void func_8007F948(Misc7Struct* p, s32 arg) {
    if (p->unk12 == arg) {
        func_8007EE08(1);
    } else {
        func_8007EE08(0);
    }
}

u32 func_8007F97C(void) {
    return D_800912F4[D_80099D98[0]];
}

void func_8007F9A0(Misc7Struct* p) {
    u8 buf[0x8];
    u8* pText;
    s32 value;

    func_8007E894(p->unk24 + p->unk28 - 0xA, p->unk14);
    func_8007EE08(0);
    func_8007F948(p, 0);
    func_8007ECF0((u8*)func_8007F97C());
    func_8007F948(p, 1);
    Sprintf(buf, D_8006FF5C, D_80099DA1[0] + 1);
    value = D_8009130C[D_80099DA1[0]];
    D_8009292C = value;
    D_80099DA4[0] = value;
    func_8007ECF0(buf);
    func_8007F948(p, 2);
    Sprintf(buf, D_8006FF60, D_80091300[D_80099D9A[0]]);
    func_8007ECF0(buf);
    func_8007F948(p, 3);
    pText = D_8006FF6C;
    if (D_80099D9D[0] != 0) {
        pText = D_8006FF68;
    }
    func_8007ECF0(pText);
    func_8007F948(p, 4);
    pText = D_8006FF74;
    if (D_80099D9E[0] != 0) {
        pText = D_8006FF68;
    }
    func_8007ECF0(pText);
    func_8007EE08(0);
}

void func_8007FB0C(Misc7Struct* p) {
    u8 buf[0x8];

    func_8007E894(p->unk24 + p->unk28 - 0xA, p->unk14);
    func_8007EE08(0);
    func_8007ECF0(D_8006FF7C);
    func_8007F948(p, 1);
    func_8007ECF0(D_8009132C[D_80099DA2[0]]);
    func_80081100(D_80099DA2[0] + 0x15, 1);
    func_8007F948(p, 2);
    Sprintf(buf, D_8006FF60, D_80091300[D_80099D9A[0]]);
    func_8007ECF0(buf);
    func_8007EE08(0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FBEC);

void func_8007FE48(Misc7Struct* p) {
    u8 buf[0x10];
    u8* pText;
    u8 flag;

    func_8007EE08(0);
    func_8007E894(p->unk24 + p->unk28 - 0xA, p->unk14);
    func_8007ECF0(D_8006FF7C);
    func_8007ECF0(D_8006FF7C);
    func_8007ECF0(D_8006FF7C);
    func_8007ECF0(D_8006FF7C);
    func_8007F948(p, 4);
    flag = D_80099D9F[0];
    if (flag != 0) {
        Sprintf(buf, D_8006FF5C, flag);
        pText = buf;
    } else {
        pText = D_8006FFA0;
    }
    func_8007ECF0(pText);
    func_8007F948(p, 5);
    func_8007ECF0((u8*)D_800912F4[D_80099D98[0]]);
    func_8007F948(p, 6);
    pText = D_8006FFA8;
    if (D_80092884 != 0) {
        pText = D_8006FFA4;
    }
    func_8007ECF0(pText);
    func_8007EE08(0);
}

s32 func_8007FF70(s32 value, s32 max, s32 flags) {
    s32 delta = 1;
    s32 pad;
    s32 status;

    if (flags & 4) {
        delta = -1;
    }
    pad = 0;
    if (flags & 2) {
        status = D_8009274C;
    } else {
        status = D_80092750;
    }
    if (status & 0x2000) {
        value += delta;
    }
    if (status & 0x8000) {
        value -= delta;
    }
    if (status & 0xA000) {
        pad = 1;
    }
    if (flags & 1) {
        if (value == -1) {
            value = max;
        }
        if (value > max) {
            value = 0;
        }
    } else {
        if (value == -1) {
            pad = 0;
            value = 0;
        }
        if (value > max) {
            pad = 0;
            value = max;
        }
    }
    if (pad) {
        func_8008EB4C(0x20);
    }
    return value;
}

void func_80080054(void) {
    D_80099D98[0] = func_8007FF70(D_80099D98[0], 2, 0);
}

void func_80080090(void) {
    D_80099DA1[0] = func_8007FF70(D_80099DA1[0], 7, 2);
}

void func_800800CC(void) {
    D_80099D9A[0] = func_8007FF70(D_80099D9A[0], 4, 2);
}

void func_80080108(void) {
    D_80099D9B[0] = func_8007FF70(D_80099D9B[0], 1, 1);
}

void func_80080144(void) {
    D_80099D9C[0] = func_8007FF70(D_80099D9C[0], 1, 1);
}

void func_80080180(void) {
    D_80099D9D[0] = func_8007FF70(D_80099D9D[0], 1, 1);
}

void func_800801BC(void) {
    D_80099D9E[0] = func_8007FF70(D_80099D9E[0], 1, 1);
}

void func_800801F8(void) {
    D_80099D9F[0] = func_8007FF70(D_80099D9F[0], 3, 2);
}

s32 func_80080234(void) {
    s32 value = func_8007FF70(D_80092884, 1, 1);

    D_80092884 = value;
    return value;
}

void func_80080268(void) {
    D_80099DA2[0] = func_8007FF70(D_80099DA2[0], 0xD, 3);
}

void func_800802A4(void) {
    s32 same;

    if (D_80091364 == 0) {
        if (g_C1ButtonStateReleased & 0x40) {
            D_80092710 &= -2;
            func_80085134(0);
            func_8008EB4C(0x22);
        }
    }
    if ((D_80092710 & 1) == 0) {
        same = (D_80092700 == D_80092704);
        do {
            D_80092700 = func_8007FF70(D_80092700, D_80092888 - 1, 3);
        } while (D_80092700 == D_80092704 && (D_80092748 & 1) == 0 && !same);
        if (D_80091364 == 0) {
            if (g_C1ButtonStateReleased & 0x20) {
                D_80092710 |= 1;
                func_8008EB4C(0x21);
                func_8008509C(0, *D_800928EC[D_80092700]);
            }
        }
    }
}

void func_8008040C(void) {
    s32 same;

    if (D_80092750 & 0x40) {
        if (D_800928C8 == 2) {
            D_80092710 &= -3;
        } else {
            D_80092710 = 0;
            func_80085134(1);
            func_8008EB4C(0x22);
            return;
        }
    }
    if ((D_80092710 & 2) == 0) {
        same = (D_80092700 == D_80092704);
        do {
            D_80092704 = func_8007FF70(D_80092704, D_80092888 - 1, 3);
        } while (D_80092700 == D_80092704 && (D_80092748 & 1) == 0 && !same);
        if (D_80092750 & 0x20) {
            D_80092710 |= 2;
            func_8008EB4C(0x21);
            func_8008509C(1, *D_800928EC[D_80092704]);
        }
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080570);

void func_80080644(s32 idx0, s32 idx1) {
    u8* pBuf;
    u8* pSrc;
    u8* pSprite;

    pBuf = HeapAlloc(0x2000, 0);
    ArchiveReadFileFromCdSector(ArchiveDecodeSector(6) + idx0 * 2, pBuf, 0x1000, 0, 0);
    ArchiveReadFileFromCdSector(ArchiveDecodeSector(6) + idx1 * 2, pBuf + 0x1000, 0x1000, 0, 0);
    D_80092700 = idx0;
    D_80092704 = idx1;
    D_80092714 = idx0;
    D_80092720 = idx1;
    D_80092710 = 3;
    ArchiveCdDataSync(0);
    pSrc = pBuf + 0x1000;
    pSprite = D_8009270C + idx0 * 0x14;
    LoadImage((RECT*)pSprite, (u_long*)pBuf);
    LoadImage((RECT*)(pSprite + 8), (u_long*)(pBuf + 0x100));
    pSprite = D_8009270C + idx1 * 0x14;
    LoadImage((RECT*)pSprite, (u_long*)pSrc);
    LoadImage((RECT*)(pSprite + 8), (u_long*)(pBuf + 0x1100));
    HeapDelayedFree(pBuf, 2);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080780);
