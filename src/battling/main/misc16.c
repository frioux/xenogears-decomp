#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

typedef struct Misc16Work Misc16Work;

struct Misc16Work {
    /* 0x00 */ s16 gravity;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ u16 unk8;
    /* 0x0A */ s16 count;
    /* 0x0C */ s16 entrySize;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u8* pEntries;
    /* 0x14 */ u16 x;
    /* 0x16 */ u16 y;
    /* 0x18 */ u16 z;
    /* 0x1A */ u16 pad1A;
    /* 0x1C */ u16 centerX;
    /* 0x1E */ u16 centerY;
    /* 0x20 */ u16 centerZ;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ s16 rangeX;
    /* 0x26 */ s16 rangeY;
    /* 0x28 */ s16 rangeZ;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ u16 halfRangeX;
    /* 0x2E */ u16 halfRangeY;
    /* 0x30 */ u16 halfRangeZ;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u16 rotX;
    /* 0x36 */ u16 rotY;
    /* 0x38 */ u16 rotZ;
    /* 0x3A */ u16 unk3A;
    /* 0x3C */ u16 extraRotX;
    /* 0x3E */ u16 extraRotY;
    /* 0x40 */ u16 extraRotZ;
    /* 0x42 */ u16 unk42;
    /* 0x44 */ s16 angleRange;
    /* 0x46 */ s16 unk46;
    /* 0x48 */ s16 depthBase;
    /* 0x4A */ s16 depthRange;
    /* 0x4C */ u8 unk4C[2];
    /* 0x4E */ s16 unk4E;
    /* 0x50 */ s32 pUpdateEntry;
    /* 0x54 */ s32 pCallback;
    /* 0x58 */ void (*pInitEntry)(u8*, Misc16Work*);
    /* 0x5C */ s32 pUpdate;
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s16 unk64;
    /* 0x66 */ s16 kindIndex;
    /* 0x68 */ s16 unk68;
    /* 0x6A */ s16 entryLife;
    /* 0x6C */ u8 unk6C[8];
    /* 0x74 */ u8 r;
    /* 0x75 */ u8 g;
    /* 0x76 */ u8 b;
}; // Size: 0x77 (known extent)

extern s32 D_80092834;
extern s32 D_80092838;
extern s32 D_8009283C;
extern s32 D_80092840;
extern s32 D_80092844;

extern u8 g_BattleDrawBufferIndex;

extern u16 D_80091CE0[];

extern SVECTOR* g_BattleTrailVertScratch;
extern SVECTOR* g_BattleScratchViewOrigin;

extern VECTOR g_BattleViewOrigin;
extern VECTOR g_BattleViewOriginY;
extern VECTOR g_BattleViewOriginZ;

extern s32 rand(void);
extern void func_80031870(u8* pLink, u32 addr);

Misc16Work* func_8008D3F4(s32 arg0, s32 arg1);
void BattleEffectUpdateEntries(Misc16Work* pWork, u8* pArg1, MATRIX* pMat);
void func_8008D680(Misc16Work* pWork, MATRIX* pMat, s32 count);

void BattleEffectNoop(void) {
}

typedef struct {
    /* 0x0 */ void (*pInitEntry)(u8*, Misc16Work*);
    /* 0x4 */ s32 pUpdateEntry;
    /* 0x8 */ s32 pCallback;
    /* 0xC */ s32 entrySize;
} Misc16Kind; // Size: 0x10

extern Misc16Kind D_80091C74[];
extern s32 D_80091CC4[];
extern s32 D_80091CDC;

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} Misc16Vec; // Size: 0xC

void BattleEffectInitEntry(u8* pArg0, u8* pArg1) {
    pArg0[0x13] = 2;
    pArg0[0x17] = 0x6A;
    pArg0[0x14] = pArg1[0x74];
    pArg0[0x15] = pArg1[0x75];
    pArg0[0x16] = pArg1[0x76];
    *(Misc16Vec*)(pArg0 + 0x1C) = *(Misc16Vec*)(pArg0 + 0x10);
}
void BattleEffectAddEntryPrim(u8* pArg0, u8* pArg1) {
    s32 buf[2];
    u8* pObj;
    s32 offset;

    pObj = pArg0 + 0x10 + g_BattleDrawBufferIndex * 0xC;
    offset = RotTransPers((SVECTOR*)pArg0, (long*)(pObj + 8), (long*)buf, (long*)buf);
    offset = offset >> 2;
    func_80031870(pArg1 + offset * 4, (u32)pObj);
}
void FieldScriptSetActorFaceId(u8* pArg0, u8* pArg1) {
    *(SVECTOR*)pArg1 = *(SVECTOR*)(pArg0 + 0x1C);
}
void BattleEffectComputeRandomRotatedPosition(Misc16Work* pWork, s16* pOut) {
    SVECTOR vec;
    VECTOR rot;

    vec.vx = rand() % pWork->rangeX - pWork->halfRangeX;
    vec.vy = rand() % pWork->rangeY - pWork->halfRangeY;
    vec.vz = rand() % pWork->rangeZ - pWork->halfRangeZ;
    ApplyRotMatrix(&vec, &rot);
    pOut[0] = pWork->centerX + rot.vx;
    pOut[1] = pWork->centerY + rot.vy;
    pOut[2] = pWork->centerZ + rot.vz;
}

void BattleEffectComputeRandomBoxPosition(Misc16Work* pWork, s16* pOut) {
    pOut[0] = pWork->centerX + rand() % pWork->rangeX - pWork->halfRangeX;
    pOut[1] = pWork->centerY + rand() % pWork->rangeY - pWork->halfRangeY;
    pOut[2] = pWork->centerZ + rand() % pWork->rangeZ - pWork->halfRangeZ;
}

void BattleEffectComputeRandomFlatPosition(Misc16Work* pWork, s16* pOut) {
    SVECTOR vec;
    VECTOR rot;

    vec.vx = rand() % pWork->rangeX - pWork->halfRangeX;
    vec.vy = 0;
    vec.vz = rand() % pWork->rangeZ - pWork->halfRangeZ;
    ApplyRotMatrix(&vec, &rot);
    pOut[0] = pWork->centerX + rot.vx;
    pOut[1] = pWork->centerY + rot.vy;
    pOut[2] = pWork->centerZ + rot.vz;
}
void BattleEffectComputeRandomEllipsePosition(Misc16Work* pWork, s16* pOut) {
    s32 angle;
    s32 rnd;
    SVECTOR vec;
    VECTOR rot;

    angle = rand();
    rnd = rand();
    vec.vx = rcos(angle) * (rnd % pWork->rangeX) >> 13;
    vec.vy = 0;
    vec.vz = rsin(angle) * (rnd % pWork->rangeZ) >> 13;
    ApplyRotMatrix(&vec, &rot);
    pOut[0] = pWork->centerX + rot.vx;
    pOut[1] = pWork->centerY + rot.vy;
    pOut[2] = pWork->centerZ + rot.vz;
}
void BattleEffectComputeAngledEllipsePosition(Misc16Work* pWork, s16* pOut, s32 angle) {
    SVECTOR vec;
    VECTOR rot;
    s32 rotAngle; // retail reads the incoming s2 without ever setting it

    vec.vx = rcos(rotAngle) * pWork->rangeX >> 12;
    vec.vy = rand() % pWork->rangeY - pWork->rangeY / 2;
    vec.vz = rsin(rotAngle) * pWork->rangeZ >> 12;
    ApplyRotMatrix(&vec, &rot);
    pOut[0] = pWork->centerX + rot.vx;
    pOut[1] = pWork->centerY + rot.vy;
    pOut[2] = pWork->centerZ + rot.vz;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D3F4);
/* 96.92% - everything matches except the load of D_80091CC4 into $a0
   where retail uses $v1 (and therefore has a nop after the lw before the
   store to pUpdate).  Tried: an s16 second parameter, a s32 one with an
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
    pWork->rangeX = 0x1000;
    pWork->rangeY = 0x1000;
    pWork->rangeZ = 0x1000;
    pWork->angleRange = 1;
    pWork->unk4E = arg1;
    pWork->unk2 = 0;
    pWork->unk4 = 0;
    pWork->unk6 = 0;
    pWork->x = 0;
    pWork->y = 0;
    pWork->z = 0;
    pWork->rotX = 0;
    pWork->rotY = 0;
    pWork->rotZ = 0;
    pWork->extraRotX = 0;
    pWork->extraRotY = 0;
    pWork->extraRotZ = 0;
    pWork->gravity = 0;
    pWork->unk46 = 0;
    pWork->depthBase = 0x100;
    pWork->depthRange = 0x100;
    pWork->halfRangeX = (s16)pWork->rangeX / 2;
    pWork->halfRangeY = (s16)pWork->rangeY / 2;
    pWork->halfRangeZ = (s16)pWork->rangeZ / 2;
    kind = D_80091CC4[(s16)arg1];
    pWork->pUpdate = kind;
    pWork->pEntries = NULL;
    pWork->kindIndex = arg0;
    pWork->unk60 = D_80091CDC;
    pWork->unk64 = 0;
    pWork->entryLife = 0x64;
    pWork->r = 0xFF;
    pWork->g = 0xFF;
    pWork->b = 0xFF;
    pWork->unk68 = 0;
    pKind = &D_80091C74[pWork->kindIndex];
    pWork->entrySize = pKind->entrySize;
    pWork->pUpdateEntry = pKind->pUpdateEntry;
    pWork->pCallback = pKind->pCallback;
    pWork->pInitEntry = pKind->pInitEntry;
    return pWork;
}
*/
/* Header every battle-effect entry starts with. An entry is pWork->entrySize bytes
   long (the kind table's size), so the table is walked byte-wise; the first
   eight bytes are also the SVECTOR the particle code transforms. */
typedef struct {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 z;
    /* 0x06 */ s16 life;
    /* 0x08 */ s16 vx;
    /* 0x0A */ s16 vy;
    /* 0x0C */ s16 vz;
} Misc16Entry; // Size: 0xE (known extent)

void BattleEffectClearEntrySlots(Misc16Work* pWork) {
    s32 i;
    Misc16Entry* pEntry;

    pEntry = (Misc16Entry*)pWork->pEntries;
    for (i = 0; i < pWork->count; i++) {
        pEntry->life = 0;
        pEntry = (Misc16Entry*)((u8*)pEntry + pWork->entrySize);
    }
}

void BattleEffectAllocEntries(Misc16Work* pWork, s32 count) {
    Misc16Entry* pEntry;
    void (*pInitEntry)(u8*, Misc16Work*);
    s32 i;

    if (pWork->pEntries != NULL) {
        HeapDelayedFree(pWork->pEntries, 3);
    }
    pWork->count = count;
    HeapSetCurrentContentType(0x14);
    pEntry = (Misc16Entry*)(pWork->pEntries = HeapAlloc(pWork->entrySize * pWork->count, 0));
    pInitEntry = pWork->pInitEntry;
    for (i = 0; i < pWork->count; i++) {
        pInitEntry((u8*)pEntry, pWork);
        pEntry->life = 0;
        pEntry = (Misc16Entry*)((u8*)pEntry + pWork->entrySize);
    }
}
/* func_8008D680(pWork, pMat, count): mat = *pMat; pWork->centerX/centerY/centerZ +=
   pMat->t[0..2]; RotMatrix(&pWork->rotX, &rotBase); MulMatrix(&mat,
   &rotBase); RotMatrix(&pWork->extraRotX, &rotExtra); MulMatrix(&rotExtra,
   &mat); SetRotMatrix(&rotExtra); then two passes over pWork->pEntries (stride
   entrySize, count entries) that both skip entries whose unk6 timer is non-zero and
   bail when --count hits -1.  Pass 1 builds a random direction
   (rsin/rcos(rand()%angleRange) scaled by >>12, interpolation factor
   depthBase + rand()%depthRange) through gte_ldv0 / gte_mvmva(1,0,0,3,0) /
   gte_lddp / GPF into entry+8; pass 2 calls pWork->pUpdate(pWork, entry), sets
   entry->unk6 = pWork->entryLife and calls pWork->pUpdateEntry(entry).  Not attempted
   body-wise yet. */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D680);
/* 88.85% first attempt (736 bytes, frame 0xB8 vs retail 0xC0).  What is
   already right: the whole prologue order, the *pMat copy, the three
   centerX/centerY/centerZ additions (they want u16 x/y/z, which is why the
   struct now declares those three as u16), the RotMatrix/MulMatrix chain,
   both loops with --count == -1 and the pUpdateEntry/pUpdate call order.
   Still different: (1) retail spills pWork->pUpdateEntry to 0x80(sp) and keeps
   nine callee regs, gcc here keeps it in $s8 so the frame is 8 bytes
   short - one more live value is needed somewhere, probably from the GTE
   block below; (2) retail keeps pDir = sp+0x18 in $s6 across both loops
   and stores through it; (3) retail re-reads IR1-IR3 with lhu from that
   buffer before mtc2 into IR1-IR3 (a real memory round trip, so it is not
   CSE-able in our compile: gcc reuses the mfc2 results) and keeps the
   addiu pEntry+8 address instead of folding it into the sh offsets.  The
   GTE shape is the same one BattleEffectRotateVectorByDepth above matches with
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

    pUpdateEntry = (void (*)(u8*))pWork->pUpdateEntry;
    pUpdate = (void (*)(Misc16Work*, u8*))pWork->pUpdate;
    mat = *pMat;
    pWork->centerX = pWork->x + pMat->t[0];
    pWork->centerY = pWork->y + pMat->t[1];
    pWork->centerZ = pWork->z + pMat->t[2];
    RotMatrix((SVECTOR*)&pWork->rotX, &rotBase);
    MulMatrix(&mat, &rotBase);
    RotMatrix((SVECTOR*)&pWork->extraRotX, &rotExtra);
    MulMatrix(&rotExtra, &mat);
    SetRotMatrix(&rotExtra);
    pEntry = pWork->pEntries;
    n = count;
    for (i = 0; i < pWork->count; i++) {
        if (*(s16*)(pEntry + 6) == 0) {
            n--;
            if (n == -1) {
                break;
            }
            angle = rand() % pWork->angleRange;
            rnd = rand();
            vec.vy = -rsin(angle);
            scale = rcos(angle);
            vec.vx = rcos(rnd) * scale >> 12;
            vec.vz = rsin(rnd) * scale >> 12;
            depth = pWork->depthBase + rand() % pWork->depthRange;
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
        pEntry += pWork->entrySize;
    }
    SetRotMatrix(&mat);
    pEntry = pWork->pEntries;
    n = count;
    for (i = 0; i < pWork->count; i++) {
        if (*(s16*)(pEntry + 6) == 0) {
            n--;
            if (n == -1) {
                break;
            }
            pUpdate(pWork, pEntry);
            *(s16*)(pEntry + 6) = pWork->entryLife;
            pUpdateEntry(pEntry);
        }
        pEntry += pWork->entrySize;
    }
}
*/
void BattleEffectHalveEntryCountIfBelow(Misc16Work* pWork) {
    s32 count;

    if (pWork->count > 0) {
        count = pWork->unk2;
        if (func_80082488(pWork, 0) < count) {
            pWork->count = -pWork->count / 2;
        }
    }
}
void BattleEffectHalveEntryCount(Misc16Work* pWork) {
    s32 value;

    if (pWork->unk2 > 0) {
        value = -pWork->count / 2;
        pWork->count = value;
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
void BattleEffectHalveEntryCount(Misc16Work* pWork) {
    s32 value;

    if (pWork->unk2 > 0) {
        s16 halved;

        halved = -pWork->count / 2;
        pWork->count = halved;
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
    u8 pad[0x28]; // unused; reproduces the original's 0x28 bytes behind the frame
    s32 i;
    s32 value;
    Misc16Entry* pEntry;
    void (*pCallback)(u8*, u8*);

    g_BattleTrailVertScratch = (SVECTOR*)0x1F800000;
    g_BattleScratchViewOrigin = (SVECTOR*)0x1F800030;
    g_BattleScratchViewOrigin->vx = g_BattleViewOrigin.vx;
    g_BattleScratchViewOrigin->vy = g_BattleViewOriginY.vx;
    g_BattleScratchViewOrigin->vz = g_BattleViewOriginZ.vx;
    pEntry = (Misc16Entry*)pWork->pEntries;
    pCallback = (void (*)(u8*, u8*))pWork->pCallback;
    for (i = 0; i < pWork->count; i++) {
        if (pEntry->life != 0) {
            pEntry->life = pEntry->life - 1;
            pEntry->vy += pWork->gravity;
            pEntry->x += pEntry->vx;
            pEntry->y += pEntry->vy;
            pEntry->z += pEntry->vz;
            if (pEntry->y > 0) {
                value = -pEntry->vy * 2 / 3;
                pEntry->vy = value;
                value = (s16)value;
                if (value < 0) {
                    value = -value;
                }
                if (value < 4) {
                    pEntry->life = 0;
                }
            }
            pCallback((u8*)pEntry, pArg1);
        }
        pEntry = (Misc16Entry*)((u8*)pEntry + pWork->entrySize);
    }
    (void)pad;
}
/* One model part, as far as the effect code needs it: parts are placed by the
   matrix they carry at 0x4C. */
typedef struct {
    /* 0x00 */ u8 pad00[0x4C];
    /* 0x4C */ MATRIX matrix;
} Misc16Part; // Size: 0x6C

typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ Misc16Part** pParts;
} Misc16PartTable; // Size: 0x8

/* An object the effect is attached to: the model whose part table +0x4 holds. */
typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ Misc16PartTable* pModel;
} Misc16Object; // Size: 0x8

void BattleEffectCopyObjectMatrix(Misc16Object* pObject, s16 index, MATRIX* pMat) {
    MATRIX unused; // reproduces the original 0x20 byte stack frame

    *pMat = pObject->pModel->pParts[index]->matrix;
    (void)unused;
}
void BattleEffectCreateWork(void) {
    s16 initValue;
    Misc16Work* pWork;

    pWork = func_8008D3F4(1, NULL);
    pWork->r = 0xFF;
    pWork->g = 0xA0;
    pWork->b = 0x70;
    BattleEffectAllocEntries(pWork, 0x100);
    initValue = (pWork->gravity = 4);
    pWork->depthRange = pWork->angleRange = 0x60;
    pWork->depthBase = initValue;
    pWork->unk68 = 0;
    pWork->entryLife = 0x20;
    D_80092834 = (s32)pWork;
}
void BattleEffectSetSpawnTimer(s32 arg0) {
    D_80092838 = arg0;
}

void BattleEffectUpdate(u8* pArg0, u8* pArg1, MATRIX* pMat, Misc16Vec* pVec) {
    MATRIX matZero;
    MATRIX matWork;
    Misc16Work* pWork;

    pWork = (Misc16Work*)D_80092834;
    if (D_80092838 >= 0x10) {
        BattleEffectCopyObjectMatrix((Misc16Object*)pArg1, 0x27, &matWork);
        MatrixNormal(&matWork, &matZero);
        matZero.t[2] = 0;
        matZero.t[1] = 0;
        matZero.t[0] = 0;
        pWork->x = pVec->x;
        pWork->y = pVec->y;
        pWork->z = pVec->z;
        pWork->extraRotX = 0;
        pWork->extraRotY = 0;
        pWork->extraRotZ = 0;
        pWork->rotX = 0;
        pWork->rotY = 0;
        pWork->rotZ = 0x800;
        func_8008D680(pWork, &matZero, D_80092838 >> 4);
        D_80092838 -= 4;
    }
    gte_SetTransRotMatrix_gpr(pMat);
    BattleEffectUpdateEntries(pWork, pArg0, pMat);
}
void BattleEffectRotateVectorByDepth(SVECTOR* pIn, SVECTOR* pOut, s32 depth) {
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
   command word in the disc bytes - and $v0 = (g_BattleDrawBufferIndex << 4) + 0x10 at
   0x8008DEB4 is overwritten by sp+0x10 before it is used, so pObj ends up as
   pArg0 + (the stack address of the local vector).  Left as INCLUDE_ASM. */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DE54);

void BattleEffectClearResourcePointers(void) {
    D_80092844 = 0;
    D_8009283C = 0;
    D_80092840 = 0;
}

void BattleEffectInitResources(void) {
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
void BattleEffectFreeResources(void) {
    if (D_80092844 != 0) {
        HeapDelayedFree((u8*)D_80092844, 2);
        HeapDelayedFree((u8*)D_8009283C, 2);
        HeapDelayedFree((u8*)D_80092840, 2);
        D_80092844 = 0;
        D_8009283C = 0;
        D_80092840 = 0;
    }
}
void BattleEffectCopyBuffers(void) {
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
