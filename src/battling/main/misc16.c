#include "common.h"
#include "psyq/libgte.h"
#include "system/memory.h"

typedef struct Misc16Work Misc16Work;

struct Misc16Work {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ u16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u8* unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ u16 unk1C;
    /* 0x1E */ u16 unk1E;
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ u16 unk2C;
    /* 0x2E */ u16 unk2E;
    /* 0x30 */ u16 unk30;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u16 unk34;
    /* 0x36 */ u16 unk36;
    /* 0x38 */ u16 unk38;
    /* 0x3A */ u16 unk3A;
    /* 0x3C */ u16 unk3C;
    /* 0x3E */ u16 unk3E;
    /* 0x40 */ u16 unk40;
    /* 0x42 */ u16 unk42;
    /* 0x44 */ s16 unk44;
    /* 0x46 */ s16 unk46;
    /* 0x48 */ s16 unk48;
    /* 0x4A */ s16 unk4A;
    /* 0x4C */ u8 unk4C[0xC];
    /* 0x58 */ void (*unk58)(u8*, Misc16Work*);
    /* 0x5C */ u8 unk5C[0xC];
    /* 0x68 */ s16 unk68;
    /* 0x6A */ s16 unk6A;
    /* 0x6C */ u8 unk6C[8];
    /* 0x74 */ u8 unk74;
    /* 0x75 */ u8 unk75;
    /* 0x76 */ u8 unk76;
}; // Size: 0x77 (known extent)

extern s32 D_80092834;
extern s32 D_80092838;
extern s32 D_8009283C;
extern s32 D_80092840;
extern s32 D_80092844;

extern s32 rand(void);

Misc16Work* func_8008D3F4(s32 arg0, s16* arg1);

void func_8008CED4(void) {
}

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
} Misc16Vec; // Size: 0xC

void func_8008CEDC(u8* pArg0, u8* pArg1) {
    pArg0[0x13] = 2;
    pArg0[0x17] = 0x6A;
    pArg0[0x14] = pArg1[0x74];
    pArg0[0x15] = pArg1[0x75];
    pArg0[0x16] = pArg1[0x76];
    *(Misc16Vec*)(pArg0 + 0x1C) = *(Misc16Vec*)(pArg0 + 0x10);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008CF30);
void func_8008CF9C(u8* pArg0, u8* pArg1) {
    *(SVECTOR*)pArg1 = *(SVECTOR*)(pArg0 + 0x1C);
}
void func_8008CFC4(Misc16Work* pWork, s16* pOut) {
    SVECTOR vec;
    VECTOR rot;

    vec.vx = rand() % pWork->unk24 - pWork->unk2C;
    vec.vy = rand() % pWork->unk26 - pWork->unk2E;
    vec.vz = rand() % pWork->unk28 - pWork->unk30;
    ApplyRotMatrix(&vec, &rot);
    pOut[0] = pWork->unk1C + rot.vx;
    pOut[1] = pWork->unk1E + rot.vy;
    pOut[2] = pWork->unk20 + rot.vz;
}

void func_8008D0A4(Misc16Work* pWork, s16* pOut) {
    pOut[0] = pWork->unk1C + rand() % pWork->unk24 - pWork->unk2C;
    pOut[1] = pWork->unk1E + rand() % pWork->unk26 - pWork->unk2E;
    pOut[2] = pWork->unk20 + rand() % pWork->unk28 - pWork->unk30;
}

void func_8008D14C(Misc16Work* pWork, s16* pOut) {
    SVECTOR vec;
    VECTOR rot;

    vec.vx = rand() % pWork->unk24 - pWork->unk2C;
    vec.vy = 0;
    vec.vz = rand() % pWork->unk28 - pWork->unk30;
    ApplyRotMatrix(&vec, &rot);
    pOut[0] = pWork->unk1C + rot.vx;
    pOut[1] = pWork->unk1E + rot.vy;
    pOut[2] = pWork->unk20 + rot.vz;
}
void func_8008D208(Misc16Work* pWork, s16* pOut) {
    s32 angle;
    s32 rnd;
    SVECTOR vec;
    VECTOR rot;

    angle = rand();
    rnd = rand();
    vec.vx = rcos(angle) * (rnd % pWork->unk24) >> 13;
    vec.vy = 0;
    vec.vz = rsin(angle) * (rnd % pWork->unk28) >> 13;
    ApplyRotMatrix(&vec, &rot);
    pOut[0] = pWork->unk1C + rot.vx;
    pOut[1] = pWork->unk1E + rot.vy;
    pOut[2] = pWork->unk20 + rot.vz;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D304);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D3F4);

void func_8008D580(Misc16Work* pWork) {
    s32 i;
    u8* pEntry;

    pEntry = pWork->unk10;
    for (i = 0; i < pWork->unkA; i++) {
        *(s16*)(pEntry + 6) = 0;
        pEntry += pWork->unkC;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D5C0);
/* 98.19% - only the s1/s2 tie-break for pWork vs the loop counter differs.
void func_8008D5C0(Misc16Work* pWork, s32 count) {
    s32 i;
    u8* pEntry;
    void (*pInitEntry)(u8*, Misc16Work*);

    if (pWork->unk10 != NULL) {
        HeapDelayedFree(pWork->unk10, 3);
    }
    pWork->unkA = count;
    HeapSetCurrentContentType(0x14);
    pEntry = HeapAlloc(pWork->unkC * pWork->unkA, 0);
    pInitEntry = pWork->unk58;
    pWork->unk10 = pEntry;
    for (i = 0; i < pWork->unkA; i++) {
        pInitEntry(pEntry, pWork);
        *(s16*)(pEntry + 6) = 0;
        pEntry += pWork->unkC;
    }
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D680);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D980);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D9F0);
/* 91.14% - the sign-extend of the stored unkA is emitted before the store
   instead of after it.  Three shapes (s16/int temporary, ternary, direct
   field re-read) all give the same v0/v1 split.
void func_8008D9F0(Misc16Work* pWork) {
    s32 value;

    if (pWork->unk2 > 0) {
        s16 halved;

        halved = -pWork->unkA / 2;
        pWork->unkA = halved;
        value = halved;
        if (value < 0) {
            value = -value;
        }
        if (value < 8) {
            pWork->unk6 = 0;
        }
    }
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DA48);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DBC0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DC28);
/* 93.22% - the original materialises the 4 into v1 before the two 0x60
   stores and stores it last; gcc rematerialises the literal at each store.
   A source local holding 4 does not survive constant folding.
void func_8008DC28(void) {
    s16 four;
    Misc16Work* pWork;

    pWork = func_8008D3F4(1, NULL);
    pWork->unk74 = 0xFF;
    pWork->unk75 = 0xA0;
    pWork->unk76 = 0x70;
    func_8008D5C0(pWork, 0x100);
    four = 4;
    pWork->unk44 = 0x60;
    pWork->unk4A = 0x60;
    pWork->unk0 = four;
    pWork->unk48 = four;
    pWork->unk68 = 0;
    pWork->unk6A = 0x20;
    D_80092834 = (s32)pWork;
}
*/
void func_8008DCA8(s32 arg0) {
    D_80092838 = arg0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DCB8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DDFC);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DE54);

void func_8008DF30(void) {
    D_80092844 = 0;
    D_8009283C = 0;
    D_80092840 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DF50);
void func_8008E064(void) {
    if (D_80092844 != 0) {
        HeapDelayedFree((u8*)D_80092844, 2);
        HeapDelayedFree((u8*)D_8009283C, 2);
        HeapDelayedFree((u8*)D_80092840, 2);
        D_80092844 = 0;
        D_8009283C = 0;
        D_80092840 = 0;
    }
}
void func_8008E0C8(void) {
    s32* pSrc;
    s32* pWordDst;
    u8* pByteDst;
    s32 value;
    s32 i;

    pByteDst = (u8*)D_80092844;
    pWordDst = (s32*)D_8009283C;
    pSrc = (s32*)D_80092840;
    for (i = 0xA7F; i != -1; i--) {
        value = *pSrc++;
        *pWordDst++ = value;
        *pByteDst++ = value;
        *pByteDst++ = value >> 16;
    }
}
