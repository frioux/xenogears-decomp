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
    /* 0x3A */ u16 pad3A;
    /* 0x3C */ u16 extraRotX;
    /* 0x3E */ u16 extraRotY;
    /* 0x40 */ u16 extraRotZ;
    /* 0x42 */ u16 pad42;
    /* 0x44 */ s16 angleRange;
    /* 0x46 */ s16 unk46;
    /* 0x48 */ s16 depthBase;
    /* 0x4A */ s16 depthRange;
    /* 0x4C */ u8 unk4C[2];
    /* 0x4E */ s16 updateKind;
    /* 0x50 */ s32 pUpdateEntry;
    /* 0x54 */ s32 pCallback;
    /* 0x58 */ void (*pInitEntry)(u8*, Misc16Work*);
    /* 0x5C */ s32 pUpdate;
    /* 0x60 */ s32 pHalveEntryCountIfBelow;
    /* 0x64 */ s16 unk64;
    /* 0x66 */ s16 kindIndex;
    /* 0x68 */ s16 unk68;
    /* 0x6A */ s16 entryLife;
    /* 0x6C */ u8 unk6C[8];
    /* 0x74 */ u8 r;
    /* 0x75 */ u8 g;
    /* 0x76 */ u8 b;
}; // Size: 0x77 (known extent)

extern s32 g_BattleEffectWork;
extern s32 g_BattleEffectSpawnTimer;
extern s32 g_BattleEffectGrid;
extern s32 g_BattleEffectGridNext;
extern s32 g_BattleEffectImage;

extern u8 g_BattleDrawBufferIndex;

extern u16 g_BattleEffectClut[];

extern SVECTOR* g_BattleTrailVertScratch;
extern SVECTOR* g_BattleScratchViewOrigin;

extern VECTOR g_BattleViewOrigin;
extern VECTOR g_BattleViewOriginY;
extern VECTOR g_BattleViewOriginZ;

extern s32 rand(void);
extern void func_80031870(u8* pLink, u32 addr);

Misc16Work* BattleEffectCreateWorkKind(s32 arg0, s32 arg1);
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

extern Misc16Kind g_BattleEffectKindTable[];
extern s32 g_BattleEffectUpdateTable[];
extern s32 g_BattleEffectHalveEntryCountFn;

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} Misc16Vec; // Size: 0xC

void BattleEffectInitEntry(u8* pArg0, Misc16Work* pWork) {
    pArg0[0x13] = 2;
    pArg0[0x17] = 0x6A;
    pArg0[0x14] = pWork->r;
    pArg0[0x15] = pWork->g;
    pArg0[0x16] = pWork->b;
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
Misc16Work* BattleEffectCreateWorkKind(s32 arg0, s32 arg1) {
    Misc16Work* pWork;
    Misc16Kind* pKind;

    HeapSetCurrentContentType(0x15);
    pWork = (Misc16Work*)HeapAlloc(0x7C, 0);
    pWork->rangeX = 0x1000;
    pWork->rangeY = 0x1000;
    pWork->rangeZ = 0x1000;
    pWork->angleRange = 1;
    pWork->updateKind = arg1;
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
    /* volatile: a plain store lets gcc hoist the HalveEntryCountFn load above it */
    *(volatile s32*)&pWork->pUpdate = g_BattleEffectUpdateTable[(s16)arg1];
    pWork->pEntries = NULL;
    pWork->kindIndex = arg0;
    pWork->pHalveEntryCountIfBelow = g_BattleEffectHalveEntryCountFn;
    pWork->unk64 = 0;
    pWork->entryLife = 0x64;
    pWork->r = 0xFF;
    pWork->g = 0xFF;
    pWork->b = 0xFF;
    pWork->unk68 = 0;
    pKind = &g_BattleEffectKindTable[pWork->kindIndex];
    pWork->entrySize = pKind->entrySize;
    pWork->pUpdateEntry = pKind->pUpdateEntry;
    pWork->pCallback = pKind->pCallback;
    pWork->pInitEntry = pKind->pInitEntry;
    return pWork;
}
/* Header every battle-effect entry starts with. An entry is pWork->entrySize bytes
   long (the kind table's size), so the walk keeps a byte cursor and casts; indexing
   it (pEntries + i * entrySize) makes gcc emit a mult loop (36%).  The first
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
    u8* pEntry;
    s32 i;

    pEntry = pWork->pEntries;
    for (i = 0; i < pWork->count; i++) {
        ((Misc16Entry*)pEntry)->life = 0;
        pEntry += pWork->entrySize;
    }
}

void BattleEffectAllocEntries(Misc16Work* pWork, s32 count) {
    u8* pEntry;
    void (*pInitEntry)(u8*, Misc16Work*);
    s32 i;

    if (pWork->pEntries != NULL) {
        HeapDelayedFree(pWork->pEntries, 3);
    }
    pWork->count = count;
    HeapSetCurrentContentType(0x14);
    pEntry = (pWork->pEntries = HeapAlloc(pWork->entrySize * pWork->count, 0));
    pInitEntry = pWork->pInitEntry;
    for (i = 0; i < pWork->count; i++) {
        pInitEntry(pEntry, pWork);
        ((Misc16Entry*)pEntry)->life = 0;
        pEntry += pWork->entrySize;
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
   entry->unk6 = pWork->entryLife and calls pWork->pUpdateEntry(entry). */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D680);
/* 98.67% (764 bytes, retail 768).  Frame, save set, spills, the *pMat copy, the
   centerX/Y/Z adds, the RotMatrix/MulMatrix chain, both loops, the call order and
   every GTE command match.  Two residues, both a base register retail materialises
   that we fold into a displacement:
   (1) retail stores the IR1-IR3 round trip through $s6 - the &dir address hoisted
       to the loop preheader (addiu s6,sp,24) and used for both the three sh and the
       three lhu.  That register exists because gte_ldir123 takes its operands as
       'r', so the loads force the address into a register; the plain stores that
       precede them fold to sh v0,24(sp) instead.  Assigning pDir above the loop
       does give the stores a register but re-prioritises the loop allocnos (dir
       takes $s4, i $s5, n $s6 where retail has i $s4, n $s5, dir $s6) and measures
       97.64%.  Dead: a non-volatile dir[], stores via (volatile u16 *) casts, a
       second store-pointer local, a do/while block round the store group, and inline
       dir[] for the loads (that one loses the register entirely, 744 bytes).
   (2) retail materialises pEntry+8 in $v0 (addiu v0,s3,8) and stores at 0/2/4($v0);
       ours folds the +8 into sh 8(s3).  Dead: the pointer local assigned before and
       after the GTE read macro, (s16 *)((u32)pEntry + 8), three explicit casts.
       That addiu is the missing 4 bytes, so this body cannot link yet.
   Also cosmetic: the two * scale >> 12 mflo results land in $t1/$t2 where retail
   reuses $t0 (sharing one temp measures 98.46).  A 4-minute permuter run from this
   body found nothing (base 287, best 287).
   The one change that took this from 97.73% is the depth: it must reuse the rnd
   local - a separate depth variable goes to caller-saved $a0, while rnd's live
   range spans the rsin/rcos/rand calls, so the reuse puts it in retail's $s1 and
   the load-delay nop after lh v0,72(s2) comes back with it.

void func_8008D680(Misc16Work* pWork, MATRIX* pMat, s32 count) {
    SVECTOR vec;
    volatile u16 dir[3];
    volatile u16* pDir;
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
    s16* pOut;

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
            rnd = pWork->depthBase + rand() % pWork->depthRange;
            gte_ldv0(&vec);
            gte_mvmva(1, 0, 0, 3, 0);
            gte_stir123_gpr(vx, vy, vz);
            pDir = dir;
            pDir[0] = vx;
            pDir[1] = vy;
            pDir[2] = vz;
            gte_lddp(rnd);
            gte_ldir123(pDir[0], pDir[1], pDir[2]);
            gte_nop();
            gte_nop();
            gte_gpf12_b();
            gte_stir123_gpr(vx, vy, vz);
            pOut = (s16*)(pEntry + 8);
            pOut[0] = vx;
            pOut[1] = vy;
            pOut[2] = vz;
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
    u8* pCursor;
    Misc16Entry* pEntry;
    void (*pCallback)(u8*, u8*);

    g_BattleTrailVertScratch = (SVECTOR*)0x1F800000;
    g_BattleScratchViewOrigin = (SVECTOR*)0x1F800030;
    g_BattleScratchViewOrigin->vx = g_BattleViewOrigin.vx;
    g_BattleScratchViewOrigin->vy = g_BattleViewOriginY.vx;
    g_BattleScratchViewOrigin->vz = g_BattleViewOriginZ.vx;
    pCursor = pWork->pEntries;
    pCallback = (void (*)(u8*, u8*))pWork->pCallback;
    for (i = 0; i < pWork->count; i++) {
        pEntry = (Misc16Entry*)pCursor;
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
            pCallback(pCursor, pArg1);
        }
        pCursor += pWork->entrySize;
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

    pWork = BattleEffectCreateWorkKind(1, NULL);
    pWork->r = 0xFF;
    pWork->g = 0xA0;
    pWork->b = 0x70;
    BattleEffectAllocEntries(pWork, 0x100);
    initValue = (pWork->gravity = 4);
    pWork->depthRange = pWork->angleRange = 0x60;
    pWork->depthBase = initValue;
    pWork->unk68 = 0;
    pWork->entryLife = 0x20;
    g_BattleEffectWork = (s32)pWork;
}
void BattleEffectSetSpawnTimer(s32 arg0) {
    g_BattleEffectSpawnTimer = arg0;
}

void BattleEffectUpdate(u8* pArg0, u8* pArg1, MATRIX* pMat, Misc16Vec* pVec) {
    MATRIX matZero;
    MATRIX matWork;
    Misc16Work* pWork;

    pWork = (Misc16Work*)g_BattleEffectWork;
    if (g_BattleEffectSpawnTimer >= 0x10) {
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
        func_8008D680(pWork, &matZero, g_BattleEffectSpawnTimer >> 4);
        g_BattleEffectSpawnTimer -= 4;
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
   command word in the disc bytes (disc/battling.bin 0x1E3DC and 0x1E3E0 are
   literally 00000000) - and $v0 = (g_BattleDrawBufferIndex << 4) + 0x10 at
   0x8008DEB4 is overwritten by sp+0x10 before it is used, so pObj ends up as
   pArg0 + (the stack address of the local vector).  A source line can produce
   neither, so no C body reproduces these bytes.  Left as INCLUDE_ASM. */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DE54);

void BattleEffectClearResourcePointers(void) {
    g_BattleEffectImage = 0;
    g_BattleEffectGrid = 0;
    g_BattleEffectGridNext = 0;
}

void BattleEffectInitResources(void) {
    s32 i;
    RECT rect;
    s16* pFirst;
    s16* pSecond;

    if (g_BattleEffectImage == 0) {
        g_BattleEffectImage = (s32)HeapAlloc(0x1500, 1);
        g_BattleEffectGrid = (s32)HeapAlloc(0x2BC0, 1);
        g_BattleEffectGridNext = (s32)HeapAlloc(0x2BC0, 1);
    }
    pFirst = (s16*)(g_BattleEffectGrid + 0x2AE0);
    pSecond = (s16*)(g_BattleEffectGridNext + 0x2AE0);
    for (i = 0x1570; i != -1; i--) {
        *pSecond = 0;
        *pFirst = 0;
        pFirst--;
        pSecond--;
    }
    for (i = 0; i < 0x1500; i++) {
        ((u8*)g_BattleEffectImage)[i] = 0;
    }
    for (i = 0; i < 0x100; i++) {
        g_BattleEffectClut[i] |= 0x8000;
    }
    rect.x = 0;
    rect.y = 0x1FD;
    rect.w = 0xFF;
    rect.h = 1;
    LoadImage(&rect, (u_long*)g_BattleEffectClut);
}
void BattleEffectFreeResources(void) {
    if (g_BattleEffectImage != 0) {
        HeapDelayedFree((u8*)g_BattleEffectImage, 2);
        HeapDelayedFree((u8*)g_BattleEffectGrid, 2);
        HeapDelayedFree((u8*)g_BattleEffectGridNext, 2);
        g_BattleEffectImage = 0;
        g_BattleEffectGrid = 0;
        g_BattleEffectGridNext = 0;
    }
}
void BattleEffectCopyBuffers(void) {
    s32* pSrc;
    s32* pWordDst;
    u8* pByteDst;
    s32 value;
    s32 i;

    pByteDst = (u8*)g_BattleEffectImage;
    pWordDst = (s32*)g_BattleEffectGrid;
    pSrc = (s32*)g_BattleEffectGridNext;
    for (i = 0xA7F; i != -1; i--) {
        value = *pSrc++;
        *pWordDst++ = value;
        *pByteDst++ = value;
        *pByteDst++ = value >> 16;
    }
}
