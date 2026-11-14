#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"
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
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 pad1A;
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
    /* 0x4C */ u8 unk4C[2];
    /* 0x4E */ s16 unk4E;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ void (*unk58)(u8*, Misc16Work*);
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s16 unk64;
    /* 0x66 */ s16 unk66;
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

Misc16Work* func_8008D3F4(s32 arg0, s32 arg1);
void BattleEffectUpdateEntries(Misc16Work* pWork, u8* pArg1, MATRIX* pMat);
void func_8008D680(Misc16Work* pWork, MATRIX* pMat, s32 count);

void func_8008CED4(void) {
}

typedef struct {
    /* 0x0 */ void (*unk0)(u8*, Misc16Work*);
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
    /* 0xC */ s32 unkC;
} Misc16Kind; // Size: 0x10

extern Misc16Kind D_80091C74[];
extern s32 D_80091CC4[];
extern s32 D_80091CDC;

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
void FieldScriptSetActorFaceId(u8* pArg0, u8* pArg1) {
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
/* 96.92% - everything matches except the load of D_80091CC4 into $a0
   where retail uses $v1 (and therefore has a nop after the lw before the
   store to unk5C).  Tried: an s16 second parameter, a s32 one with an
   explicit (s16) cast on the index (that one fixes the unk4E store's extra
   move), and an s32 then u32 local for the loaded word declared first or
   last.  The register is an allocator pick; $a0 is free at that point in
   both versions.
Misc16Work* func_8008D3F4(s32 arg0, s32 arg1) {
    Misc16Work* pWork;
    Misc16Kind* pKind;
    u32 kind;

    HeapSetCurrentContentType(0x15);
    pWork = (Misc16Work*)HeapAlloc(0x7C, 0);
    pWork->unk24 = 0x1000;
    pWork->unk26 = 0x1000;
    pWork->unk28 = 0x1000;
    pWork->unk44 = 1;
    pWork->unk4E = arg1;
    pWork->unk2 = 0;
    pWork->unk4 = 0;
    pWork->unk6 = 0;
    pWork->unk14 = 0;
    pWork->unk16 = 0;
    pWork->unk18 = 0;
    pWork->unk34 = 0;
    pWork->unk36 = 0;
    pWork->unk38 = 0;
    pWork->unk3C = 0;
    pWork->unk3E = 0;
    pWork->unk40 = 0;
    pWork->unk0 = 0;
    pWork->unk46 = 0;
    pWork->unk48 = 0x100;
    pWork->unk4A = 0x100;
    pWork->unk2C = (s16)pWork->unk24 / 2;
    pWork->unk2E = (s16)pWork->unk26 / 2;
    pWork->unk30 = (s16)pWork->unk28 / 2;
    kind = D_80091CC4[(s16)arg1];
    pWork->unk5C = kind;
    pWork->unk10 = NULL;
    pWork->unk66 = arg0;
    pWork->unk60 = D_80091CDC;
    pWork->unk64 = 0;
    pWork->unk6A = 0x64;
    pWork->unk74 = 0xFF;
    pWork->unk75 = 0xFF;
    pWork->unk76 = 0xFF;
    pWork->unk68 = 0;
    pKind = &D_80091C74[pWork->unk66];
    pWork->unkC = pKind->unkC;
    pWork->unk50 = pKind->unk4;
    pWork->unk54 = pKind->unk8;
    pWork->unk58 = pKind->unk0;
    return pWork;
}
*/
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
/* 98.19% - byte-identical except that retail keeps pWork in $s1 and the loop
   counter in $s2 while gcc swaps them (s2/s1); the prologue's save order
   follows.  gcc-2.6.0 scores 99.79 here (2.7.2-cdk fails to compile), so the
   unit may be a 2.6.0 unit; that cannot be switched without re-checking every
   already-matched function in misc16, which are all INCLUDE_ASM.
void func_8008D5C0(Misc16Work* pWork, s32 count) {
    u8* pEntry;
    void (*pInitEntry)(u8*, Misc16Work*);
    s32 i;

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
/* func_8008D680(pWork, pMat, count): mat = *pMat; pWork->unk1C/1E/20 +=
   pMat->t[0..2]; RotMatrix(&pWork->unk34, &rotBase); MulMatrix(&mat,
   &rotBase); RotMatrix(&pWork->unk3C, &rotExtra); MulMatrix(&rotExtra,
   &mat); SetRotMatrix(&rotExtra); then two passes over pWork->unk10 (stride
   unkC, unkA entries) that both skip entries whose unk6 timer is non-zero and
   bail when --count hits -1.  Pass 1 builds a random direction
   (rsin/rcos(rand()%unk44) scaled by >>12, interpolation factor
   unk48 + rand()%unk4A) through gte_ldv0 / gte_mvmva(1,0,0,3,0) /
   gte_lddp / GPF into entry+8; pass 2 calls pWork->unk5C(pWork, entry), sets
   entry->unk6 = pWork->unk6A and calls pWork->unk50(entry).  Not attempted
   body-wise yet. */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D680);
/* 88.85% first attempt (736 bytes, frame 0xB8 vs retail 0xC0).  What is
   already right: the whole prologue order, the *pMat copy, the three
   unk1C/1E/20 additions (they want u16 unk14/16/18, which is why the
   struct now declares those three as u16), the RotMatrix/MulMatrix chain,
   both loops with --count == -1 and the unk50/unk5C call order.
   Still different: (1) retail spills pWork->unk50 to 0x80(sp) and keeps
   nine callee regs, gcc here keeps it in $s8 so the frame is 8 bytes
   short - one more live value is needed somewhere, probably from the GTE
   block below; (2) retail keeps pDir = sp+0x18 in $s6 across both loops
   and stores through it; (3) retail re-reads IR1-IR3 with lhu from that
   buffer before mtc2 into IR1-IR3 (a real memory round trip, so it is not
   CSE-able in our compile: gcc reuses the mfc2 results) and keeps the
   addiu pEntry+8 address instead of folding it into the sh offsets.  The
   GTE shape is the same one func_8008DDFC above matches with
   gte_ldv0 / gte_mvmva(1,0,0,3,0) / gte_stir123_gpr, so the macro set is
   available; the open question is which libgte macro spells the
   store-then-reload-into-IR round trip.
void func_8008D680(Misc16Work* pWork, MATRIX* pMat, s32 count) {
    SVECTOR vec;
    SVECTOR dir;
    MATRIX rotBase;
    MATRIX mat;
    MATRIX rotExtra;
    void (*pUpdateEntry)(u8*);
    void (*pUpdate)(Misc16Work*, u8*);
    s32 n;
    s32 i;
    s32 angle;
    s32 rnd;
    s32 scale;
    s32 depth;
    s16 vx;
    s16 vy;
    s16 vz;
    u8* pEntry;

    pUpdateEntry = (void (*)(u8*))pWork->unk50;
    pUpdate = (void (*)(Misc16Work*, u8*))pWork->unk5C;
    mat = *pMat;
    pWork->unk1C = pWork->unk14 + pMat->t[0];
    pWork->unk1E = pWork->unk16 + pMat->t[1];
    pWork->unk20 = pWork->unk18 + pMat->t[2];
    RotMatrix((SVECTOR*)&pWork->unk34, &rotBase);
    MulMatrix(&mat, &rotBase);
    RotMatrix((SVECTOR*)&pWork->unk3C, &rotExtra);
    MulMatrix(&rotExtra, &mat);
    SetRotMatrix(&rotExtra);
    pEntry = pWork->unk10;
    n = count;
    for (i = 0; i < pWork->unkA; i++) {
        if (*(s16*)(pEntry + 6) == 0) {
            n--;
            if (n == -1) {
                break;
            }
            angle = rand() % pWork->unk44;
            rnd = rand();
            vec.vy = -rsin(angle);
            scale = rcos(angle);
            vec.vx = rcos(rnd) * scale >> 12;
            vec.vz = rsin(rnd) * scale >> 12;
            depth = pWork->unk48 + rand() % pWork->unk4A;
            gte_ldv0(&vec);
            gte_mvmva(1, 0, 0, 3, 0);
            gte_stir123_gpr(vx, vy, vz);
            dir.vx = vx;
            dir.vy = vy;
            dir.vz = vz;
            gte_lddp(depth);
            gte_ldir123(dir.vx, dir.vy, dir.vz);
            gte_nop();
            gte_nop();
            gte_gpf12_b();
            gte_stir123_gpr(vx, vy, vz);
            ((s16*)(pEntry + 8))[0] = vx;
            ((s16*)(pEntry + 8))[1] = vy;
            ((s16*)(pEntry + 8))[2] = vz;
        }
        pEntry += pWork->unkC;
    }
    SetRotMatrix(&mat);
    pEntry = pWork->unk10;
    n = count;
    for (i = 0; i < pWork->unkA; i++) {
        if (*(s16*)(pEntry + 6) == 0) {
            n--;
            if (n == -1) {
                break;
            }
            pUpdate(pWork, pEntry);
            *(s16*)(pEntry + 6) = pWork->unk6A;
            pUpdateEntry(pEntry);
        }
        pEntry += pWork->unkC;
    }
}
*/
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
void BattleEffectUpdateEntries(Misc16Work* pWork, u8* pArg1, MATRIX* pMat) {
    u8 pad[0x28];
    s32 i;
    s32 value;
    u8* pEntry;
    void (*pCallback)(u8*, u8*);
    D_8009282C = (u8*)0x1F800000;
    D_80092830 = (u8*)0x1F800030;
    ((s16*)D_8009282C)[0x18] = D_80096FA8.vx;
    ((s16*)D_8009282C)[0x19] = D_80096FAC.vx;
    ((s16*)D_8009282C)[0x1A] = D_80096FB0.vx;
    pEntry = pWork->unk10;
    pCallback = (void (*)(u8*, u8*))pWork->unk54;
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
    (void)pad;
}
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
/* 93.22% - the only difference is instruction order: retail loads every
   constant at the top of the block and sinks the dependent stores (li v1,4
   before the 0x60 pair, then li v0,0x20, then both sh v1).  Same for
   -O3 (93.22) and gcc 2.6.0 (93.22).  Tried: chained vs separate
   statements, 4-pair before the 0x60-pair, s32 locals for both constants.
void func_8008DC28(void) {
    Misc16Work* pWork;

    pWork = func_8008D3F4(1, NULL);
    pWork->unk74 = 0xFF;
    pWork->unk75 = 0xA0;
    pWork->unk76 = 0x70;
    func_8008D5C0(pWork, 0x100);
    pWork->unk4A = pWork->unk44 = 0x60;
    pWork->unk48 = pWork->unk0 = 4;
    pWork->unk68 = 0;
    pWork->unk6A = 0x20;
    D_80092834 = (s32)pWork;
}
*/
void func_8008DCA8(s32 arg0) {
    D_80092838 = arg0;
}

void func_8008DCB8(u8* pArg0, u8* pArg1, MATRIX* pMat, Misc16Vec* pVec) {
    MATRIX matZero;
    MATRIX matWork;
    Misc16Work* pWork;

    pWork = (Misc16Work*)D_80092834;
    if (D_80092838 >= 0x10) {
        func_8008DBC0(pArg1, 0x27, (u8*)&matWork);
        MatrixNormal(&matWork, &matZero);
        matZero.t[2] = 0;
        matZero.t[1] = 0;
        matZero.t[0] = 0;
        pWork->unk14 = pVec->unk0;
        pWork->unk16 = pVec->unk4;
        pWork->unk18 = pVec->unk8;
        pWork->unk3C = 0;
        pWork->unk3E = 0;
        pWork->unk40 = 0;
        pWork->unk34 = 0;
        pWork->unk36 = 0;
        pWork->unk38 = 0x800;
        func_8008D680(pWork, &matZero, D_80092838 >> 4);
        D_80092838 -= 4;
    }
    gte_SetTransRotMatrix_gpr(pMat);
    BattleEffectUpdateEntries(pWork, pArg0, pMat);
}
void func_8008DDFC(SVECTOR* pIn, SVECTOR* pOut, s32 depth) {
    register s16 vx asm("$8");
    register s16 vy asm("$9");
    register s16 vz asm("$10");

    gte_ldv0(pIn);
    gte_lddp_a2(depth);
    gte_mvmva(1, 0, 0, 3, 0);
    gte_stmac123_gpr_ldir123();
    gte_nop();
    gte_nop();
    gte_gpf12_b();
    gte_stir123_gpr(vx, vy, vz);
    pOut->vx = vx;
    pOut->vy = vy;
    pOut->vz = vz;
}
/* Retail's code is not compiler output: the GTE sequence at 0x8008DEC4 is
   lwc2 $0/lwc2 $1 then two nops and straight to swc2 $14 - there is no RTPS
   command word in the disc bytes - and $v0 = (D_800928A0 << 4) + 0x10 at
   0x8008DEB4 is overwritten by sp+0x10 before it is used, so pObj ends up as
   pArg0 + (the stack address of the local vector).  Left as INCLUDE_ASM. */
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
