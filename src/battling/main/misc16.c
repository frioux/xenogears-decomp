#include "common.h"
#include "psyq/libgpu.h"
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

extern u8 D_800928A0;

extern u16 D_80091CE0[];

extern u8* D_8009282C;
extern u8* D_80092830;

extern VECTOR D_80096FA8;
extern VECTOR D_80096FAC;
extern VECTOR D_80096FB0;

extern s32 rand(void);
extern void func_80031870(u8* pLink, u32 addr);

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
void func_8008CF30(u8* pArg0, u8* pArg1) {
    s32 buf[2];
    u8* pObj;
    s32 offset;

    pObj = pArg0 + 0x10 + D_800928A0 * 0xC;
    offset = RotTransPers((SVECTOR*)pArg0, (long*)(pObj + 8), (long*)buf, (long*)buf);
    offset = offset >> 2;
    func_80031870(pArg1 + offset * 4, (u32)pObj);
}
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
void func_8008D304(Misc16Work* pWork, s16* pOut, s32 angle) {
    SVECTOR vec;
    VECTOR rot;
    s32 rotAngle; // retail reads the incoming s2 without ever setting it

    vec.vx = rcos(rotAngle) * pWork->unk24 >> 12;
    vec.vy = rand() % pWork->unk26 - pWork->unk26 / 2;
    vec.vz = rsin(rotAngle) * pWork->unk28 >> 12;
    ApplyRotMatrix(&vec, &rot);
    pOut[0] = pWork->unk1C + rot.vx;
    pOut[1] = pWork->unk1E + rot.vy;
    pOut[2] = pWork->unk20 + rot.vz;
}
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
void func_8008D980(Misc16Work* pWork) {
    s32 count;

    if (pWork->unkA > 0) {
        count = pWork->unk2;
        if (func_80082488(pWork, 0) < count) {
            pWork->unkA = -pWork->unkA / 2;
        }
    }
}
void func_8008D9F0(Misc16Work* pWork) {
    s32 value;

    if (pWork->unk2 > 0) {
        value = -pWork->unkA / 2;
        pWork->unkA = value;
        value = (s16)value;
        if (value < 0) {
            value = -value;
        }
        if (value < 8) {
            pWork->unk6 = 0;
        }
    }
}
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
/* 85.63% - everything including the frame (the 0x28 unused local) and the
   division-by-3 magic matches; only the mfhi scratch register differs
   (original a3, gcc a2), the usual allocator tie-break.  Tried: s16 value,
   2 * -x / 3, pad declared first, -x * 2 / 3.
void func_8008DA48(Misc16Work* pWork, u8* pArg1) {
    u8 pad[0x28]; // reproduces the original frame; never referenced
    s32 i;
    s32 value;
    u8* pEntry;
    void (*pCallback)(u8*, u8*);

    D_8009282C = (u8*)0x1F800000;
    D_80092830 = (u8*)0x1F800030;
    *(s16*)(PSX_SCRATCH + 0x30) = D_80096FA8.vx;
    *(s16*)(PSX_SCRATCH + 0x32) = D_80096FAC.vx;
    *(s16*)(PSX_SCRATCH + 0x34) = D_80096FB0.vx;
    pEntry = pWork->unk10;
    pCallback = ((void (**)(u8*, u8*))&pWork->unk4C[8])[0];
    for (i = 0; i < pWork->unkA; i++) {
        if (*(s16*)(pEntry + 6) != 0) {
            *(s16*)(pEntry + 6) = *(s16*)(pEntry + 6) - 1;
            *(u16*)(pEntry + 0xA) += pWork->unk0;
            *(u16*)(pEntry + 0x0) += *(u16*)(pEntry + 8);
            *(u16*)(pEntry + 0x2) += *(u16*)(pEntry + 0xA);
            *(u16*)(pEntry + 0x4) += *(u16*)(pEntry + 0xC);
            if (*(s16*)(pEntry + 2) > 0) {
                value = -*(s16*)(pEntry + 0xA) * 2 / 3;
                *(s16*)(pEntry + 0xA) = value;
                value = (s16)value;
                if (value < 0) {
                    value = -value;
                }
                if (value < 4) {
                    *(s16*)(pEntry + 6) = 0;
                }
            }
            pCallback(pEntry, pArg1);
        }
        pEntry += pWork->unkC;
    }
}
*/
typedef struct {
    /* 0x0 */ s32 unk0[8];
} Misc16Oct; // Size: 0x20

void func_8008DBC0(u8* pArg0, s16 arg1, u8* pArg1) {
    u8** pArray;
    u8* pObj;
    Misc16Oct unused; // reproduces the original 0x20 stack frame

    pArray = *(u8***)(*(u8**)(pArg0 + 4) + 4);
    pObj = pArray[arg1];
    *(Misc16Oct*)pArg1 = *(Misc16Oct*)(pObj + 0x4C);
}
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

void func_8008DF50(void) {
    s32 i;
    RECT rect;
    s16* pFirst;
    s16* pSecond;

    if (D_80092844 == 0) {
        D_80092844 = (s32)HeapAlloc(0x1500, 1);
        D_8009283C = (s32)HeapAlloc(0x2BC0, 1);
        D_80092840 = (s32)HeapAlloc(0x2BC0, 1);
    }
    pFirst = (s16*)(D_8009283C + 0x2AE0);
    pSecond = (s16*)(D_80092840 + 0x2AE0);
    for (i = 0x1570; i != -1; i--) {
        *pSecond = 0;
        *pFirst = 0;
        pFirst--;
        pSecond--;
    }
    for (i = 0; i < 0x1500; i++) {
        ((u8*)D_80092844)[i] = 0;
    }
    for (i = 0; i < 0x100; i++) {
        D_80091CE0[i] |= 0x8000;
    }
    rect.x = 0;
    rect.y = 0x1FD;
    rect.w = 0xFF;
    rect.h = 1;
    LoadImage(&rect, (u_long*)D_80091CE0);
}
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
