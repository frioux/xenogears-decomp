#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"
#include "psyq/libcd.h"
#include "psyq/libgpu.h"

typedef struct {
    /* 0x00 */ s16 state;
    /* 0x02 */ s16 stateTimer;
    /* 0x04 */ s16 command;
    /* 0x06 */ u8 unk6[0x12];
    /* 0x18 */ s32 (*pEnterHandler)(s32); // called once in state 0; its return is the next state
    /* 0x1C */ s32 (*pUpdateHandler)(s32); // called every frame in state 1; NULL while the slot is free
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x28];
    /* 0x4C */ s32 pSprite;
    /* 0x50 */ u8 unk50[0x30];
} Misc15Slot; // Size: 0x80

typedef struct {
    /* 0x0 */ s32 sector;
    /* 0x4 */ u32 size;
    /* 0x8 */ s32 dest;
    /* 0xC */ u8 payload[0x18]; // CD read destination, streamed by func_80096C0C
} Misc15CdRequest; // Size: 0x24

typedef struct {
    /* 0x0 */ u32 unk0[4];
} Misc15Block16; // Size: 0x10

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ u16 unkA;
    /* 0xC */ s16 unkC;
    /* 0xE */ s16 unkE;
    /* 0x10 */ MATRIX unk10;
} Misc15Xform; // Size: 0x30


extern Misc15Slot *g_WorldMapSlots;
extern void *g_WorldMapTileCellPointers[];
extern MATRIX g_WorldMapIdentityMatrix;
extern MATRIX g_WorldMapGridMatrix;
extern u32 D_8009C580[];
extern s32 g_WorldMapGridAngle;
extern s32 g_WorldMapGridShadePhase;
extern u8 *g_WorldMapTilePrimBuffer;
extern u8 *g_WorldMapTilePrimBufferCopy;
extern s32 g_WorldMapLoadQueueHead;
extern s32 g_WorldMapCdReadRetryTimer;
extern s32 g_WorldMapCdReadState;
extern Misc15CdRequest *g_WorldMapCdRequestQueue[16];
extern void *g_WorldMapFileLoadRequestQueue[16];
extern void func_800981C8(s32);
extern void WorldMapSubmitGridImageLoads(void);

extern void WorldMapClearSlotPool(void);
extern CdlLOC g_WorldMapCdReadLocation;
extern u8 *g_WorldMapCdRequestCursor;
extern s32 g_WorldMapCdExpectedSector;
extern s32 g_WorldMapCdCurrentSector;
extern s32 g_WorldMapCdReadSectorsLeft;
extern s32 g_WorldMapCdReadBytesLeft;
extern s32 g_WorldMapCdReadDest;
extern s32 D_8009BE48;
extern s32 D_8009CCB0;
extern s32 g_WorldMapCdReadErrorCount;
extern s32 g_WorldMapCdRetryCount;
extern void WorldMapCdReadCallback(s32 mode, u8 *pResult);
extern void func_80096C0C(u_char mode, u_char *pResult);
extern s32 g_WorldMapCdSectorBuffer;
extern s32 D_8009BCD0;
extern s32 D_8009BCD4;
extern u32 ArchiveGetDebugTable(void);
extern s32 WorldMapStepCdReadState(void);
extern void WorldMapReadFileEntries(void *p);
extern s32 g_WorldMapScrollX;
extern s32 D_8009BBB8;
extern s32 D_8009BBBC;
extern s16 g_WorldMapGridOrigin;
extern s16 D_8009C83A;
extern s16 D_8009C83C;

s32 WorldMapUpdateLoadQueue(void) {
    s32 result = 0;
    u32 first;
    u32 second;

    first = ArchiveGetDebugTable();
    second = ArchiveGetDebugTable();
    if ((first == 0) | (second == 0xFFFFFFFF)) {
        result = WorldMapStepCdReadState();
        if (result == 0) {
            if (g_WorldMapCdRequestQueue[g_WorldMapLoadQueueHead] != NULL) {
                WorldMapStartCdRead(g_WorldMapCdRequestQueue[g_WorldMapLoadQueueHead]);
            }
        }
    } else {
        if (g_WorldMapFileLoadRequestQueue[g_WorldMapLoadQueueHead] != NULL) {
            WorldMapReadFileEntries(g_WorldMapFileLoadRequestQueue[g_WorldMapLoadQueueHead]);
            g_WorldMapFileLoadRequestQueue[g_WorldMapLoadQueueHead] = NULL;
            g_WorldMapLoadQueueHead = (g_WorldMapLoadQueueHead + 1) & 0xF;
        }
    }
    return result;
}
s32 WorldMapStepCdReadState(void) {
    switch (g_WorldMapCdReadState) {
    case 0:
        return 0;
    case 4:
        if (--g_WorldMapCdReadRetryTimer == 0) {
            g_WorldMapCdReadState++;
        }
        return 1;
    case 1:
    case 2:
    case 3:
        return 1;
    case 5:
        g_WorldMapCdReadState = 0;
        g_WorldMapCdRequestQueue[g_WorldMapLoadQueueHead] = NULL;
        g_WorldMapLoadQueueHead = (g_WorldMapLoadQueueHead + 1) & 0xF;
        return 2;
    }
    return 3;
}
void WorldMapStartCdRead(Misc15CdRequest *p) {
    CdlLOC *pLoc;
    s32 lba;
    u32 size;

    lba = p->sector;
    g_WorldMapCdReadState = 1;
    pLoc = &g_WorldMapCdReadLocation;
    /* retail keeps this dead store: the volatile store blocks gcc's elimination */
    *(u8 *volatile *)&g_WorldMapCdRequestCursor = (u8 *)p;
    g_WorldMapCdRequestCursor = p->payload;
    size = p->size;
    D_8009BE48 = 0;
    D_8009CCB0 = 0;
    g_WorldMapCdReadErrorCount = 0;
    g_WorldMapCdRetryCount = 0;
    g_WorldMapCdExpectedSector = lba;
    g_WorldMapCdCurrentSector = lba;
    g_WorldMapCdReadSectorsLeft = (size + 0x7FF) >> 11;
    g_WorldMapCdReadBytesLeft = p->size;
    g_WorldMapCdReadDest = p->dest;
    CdIntToPos(lba, pLoc);
    CdSyncCallback(WorldMapCdReadCallback);
    CdControlF(CdlSetloc, (u8 *)pLoc);
}

void WorldMapCdReadCallback(s32 mode, u8 *pResult) {
    CdlLOC *pLoc;

    if (mode == 2) {
        switch (g_WorldMapCdReadState) {
        case 1:
            g_WorldMapCdReadState = 2;
            D_8009BCD4 = 0;
            D_8009BCD0 = 0;
            g_WorldMapCdSectorBuffer = 0;
            CdReadyCallback(func_80096C0C);
            CdControlF(CdlReadS, NULL);
            break;
        case 3:
            if (g_WorldMapCdCurrentSector == 0) {
                g_WorldMapCdReadState = 4;
                g_WorldMapCdReadRetryTimer = 1;
                CdSyncCallback(NULL);
            }
            break;
        case 10:
            if (pResult[0] & CdlStatShellOpen) {
                CdControlF(CdlNop, NULL);
            } else {
                CdControlF(CdlGetTN, NULL);
                g_WorldMapCdReadState = 0xB;
            }
            break;
        case 11:
            g_WorldMapCdReadState = 0xC;
            CdControlF(CdlPause, NULL);
            break;
        case 12:
            pLoc = &g_WorldMapCdReadLocation;
            g_WorldMapCdReadState = 1;
            CdIntToPos(g_WorldMapCdExpectedSector, pLoc);
            CdControlF(CdlSetloc, (u8 *)pLoc);
            break;
        }
    } else {
        if (pResult[0] & CdlStatShellOpen) {
            g_WorldMapCdReadState = 0xA;
            g_WorldMapCdReadErrorCount++;
            CdControlF(CdlNop, NULL);
        } else {
            g_WorldMapCdReadState = 0xB;
            CdControlF(CdlGetTN, NULL);
        }
    }
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096C0C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096F18);
// Best: 59.80%. The structure is decoded and every call/argument is right (the
// scratchpad rot vector at 0x1F8000A0, the matrix at 0x1F8000F0, the vectors at
// 0x1F800000/0x1F800010, RotMatrixYXZ / ApplyMatrixLV / RotMatrixYXZ /
// ApplyMatrix, and the >>12 and ->>12 folds). What differs is the frame: gcc
// CSEs 0x1F8000A0/0xA2/0xA4 into the callee-saved s3/s4/s5, so the frame is
// 0x30 with six saved registers, while the original keeps only 0xA0 in s3 and
// reaches 0xA2/0xA4 with a fresh lui $at plus maspsx's low-half offset fold
// (sh $v0,0xA2($at)) -- a 4-byte size difference and a different allocation.
/*
void func_80096F18(Misc15Xform *pOut, u8 *pSrc, s32 height, u8 *pAngles) {
    pOut->unk8 = 0;
    pOut->unkA = *(s32 *)(pSrc + 4) >> 12;
    pOut->unkC = 0;
    *(u16 *)0x1F8000A0 = *(u16 *)(pAngles + 0);
    *(u16 *)0x1F8000A2 = *(u16 *)(pAngles + 2);
    *(u16 *)0x1F8000A4 = 0;
    RotMatrixYXZ((SVECTOR *)0x1F8000A0, (MATRIX *)0x1F8000F0);
    *(s32 *)0x1F800000 = 0;
    *(s32 *)0x1F800004 = 0;
    *(s32 *)0x1F800008 = -(height >> 12);
    ApplyMatrixLV((MATRIX *)0x1F8000F0, (VECTOR *)0x1F800000, (VECTOR *)0x1F800010);
    pOut->x = *(s32 *)0x1F800010;
    pOut->y = pOut->unkA + *(s32 *)0x1F800014;
    pOut->z = *(s32 *)0x1F800018;
    *(u16 *)0x1F8000A0 = 0;
    *(u16 *)0x1F8000A2 = *(u16 *)(pAngles + 2);
    *(u16 *)0x1F8000A4 = *(u16 *)(pAngles + 4);
    RotMatrixYXZ((SVECTOR *)0x1F8000A0, (MATRIX *)0x1F8000F0);
    *(u16 *)0x1F8000A0 = 0;
    *(u16 *)0x1F8000A2 = -0x1000;
    *(u16 *)0x1F8000A4 = 0;
    ApplyMatrix((MATRIX *)0x1F8000F0, (SVECTOR *)0x1F8000A0, (VECTOR *)((u8 *)pOut + 0x10));
}
*/
typedef struct {
    /* 0x0 */ s16 rotX;
    /* 0x2 */ s16 rotY;
    /* 0x4 */ s16 rotZ;
} Misc15Rot; // Size: 0x6

void WorldMapExtractRotationFromMatrix(MATRIX *pMtx, Misc15Rot *pRot) {
    u16 *pAngles = (u16 *)pMtx;

    if ((pAngles[6] | pAngles[8]) == 0) {
        return;
    }
    pRot->rotY = ratan2((s16)pAngles[6], (s16)pAngles[8]) & 0xFFF;
    *(MATRIX *)0x1F8000F0 = *pMtx;
    *(MATRIX *)0x1F800110 = g_WorldMapIdentityMatrix;
    RotMatrixY(pRot->rotY, (MATRIX *)0x1F800110);
    MulMatrix0((MATRIX *)0x1F8000F0, (MATRIX *)0x1F800110, (MATRIX *)0x1F800130);
    pRot->rotX = ratan2(*(s16 *)0x1F80013A, *(s16 *)0x1F800138);
    *(MATRIX *)0x1F800110 = g_WorldMapIdentityMatrix;
    RotMatrixX(pRot->rotX, (MATRIX *)0x1F800110);
    MulMatrix0((MATRIX *)0x1F800130, (MATRIX *)0x1F800110, (MATRIX *)0x1F8000F0);
    pRot->rotZ = -ratan2(*(s16 *)0x1F8000F6, *(s16 *)0x1F8000F8);
}
typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
    /* 0xC */ s16 unkC;
    /* 0xE */ s16 unkE;
    /* 0x10 */ VECTOR unk10;
} Misc15Frame; // Size: 0x1C

extern s16 g_WorldMapCameraRot;
extern s16 g_WorldMapViewRotationY;
extern s16 g_WorldMapViewRotationZ;
extern MATRIX g_WorldMapViewMatrix;
extern s16 D_8009C80A;
extern s16 D_8009C80C;
extern s16 D_8009C80E;
extern s16 D_8009C810;
extern s16 D_8009C812;
extern s16 D_8009C814;
extern s16 D_8009C816;
extern s16 D_8009C818;

// Best: 83.39%. Every call and its arguments match (three VectorNormal /
// OuterProduct12 pairs on the scratchpad frames, the nine halfword stores into
// g_WorldMapViewMatrix..D_8009C818, the negated SVECTOR, the matrix copy and
// ApplyMatrix/TransMatrix). What differs is cc1's scheduling of the nine 32-bit
// loads against the nine halfword stores: retail batches all nine lw before the
// first sh and keeps the three lhu/negu/sh of the SVECTOR after them, while this
// compile keeps the store order but interleaves the SVECTOR negates into the load
// sequence and needs two extra temporaries (t4/t5). Not a type or frame problem:
// loading all nine words into s32 locals before the first store (round 40) gives
// the same 83.39%, and gcc-2.7.2-cdk drops it to 39.61%.
/*
void func_80097244(Misc15Frame *p) {
    MATRIX *pMtx;
    s32 m0;
    s32 m1;
    s32 m2;
    s32 m3;
    s32 m4;
    s32 m5;
    s32 m6;
    s32 m7;
    s32 m8;

    ((VECTOR *)0x1F800000)->vx = p->unk8 - p->x;
    ((VECTOR *)0x1F800000)->vy = p->unkA - p->y;
    ((VECTOR *)0x1F800000)->vz = p->unkC - p->z;
    VectorNormal((VECTOR *)0x1F800000, (VECTOR *)0x1F800030);
    OuterProduct12((VECTOR *)0x1F800030, &p->unk10, (VECTOR *)0x1F800000);
    VectorNormal((VECTOR *)0x1F800000, (VECTOR *)0x1F800010);
    OuterProduct12((VECTOR *)0x1F800030, (VECTOR *)0x1F800010, (VECTOR *)0x1F800000);
    VectorNormal((VECTOR *)0x1F800000, (VECTOR *)0x1F800020);
    pMtx = &g_WorldMapViewMatrix;
    m0 = ((VECTOR *)0x1F800010)->vx;
    m1 = ((VECTOR *)0x1F800010)->vy;
    m2 = ((VECTOR *)0x1F800010)->vz;
    m3 = ((VECTOR *)0x1F800020)->vx;
    m4 = ((VECTOR *)0x1F800020)->vy;
    m5 = ((VECTOR *)0x1F800020)->vz;
    m6 = ((VECTOR *)0x1F800030)->vx;
    m7 = ((VECTOR *)0x1F800030)->vy;
    m8 = ((VECTOR *)0x1F800030)->vz;
    pMtx->m[0][0] = m0;
    D_8009C80A = m1;
    D_8009C80C = m2;
    D_8009C80E = m3;
    D_8009C810 = m4;
    D_8009C812 = m5;
    D_8009C814 = m6;
    D_8009C816 = m7;
    D_8009C818 = m8;
    ((SVECTOR *)0x1F800040)->vx = -(u16)p->x;
    ((SVECTOR *)0x1F800040)->vy = -(u16)p->y;
    ((SVECTOR *)0x1F800040)->vz = -(u16)p->z;
    *(MATRIX *)0x1F800048 = *pMtx;
    ApplyMatrix((MATRIX *)0x1F800048, (SVECTOR *)0x1F800040, (VECTOR *)0x1F800000);
    TransMatrix(pMtx, (VECTOR *)0x1F800000);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097244);

void WorldMapBuildRotationMatrix(Misc15Rot *p) {
    MATRIX *pMtx = (MATRIX *)0x1F8000F0;

    *pMtx = g_WorldMapIdentityMatrix;
    *(MATRIX *)0x1F800110 = *pMtx;
    *(MATRIX *)0x1F800130 = *pMtx;
    RotMatrixX(-g_WorldMapCameraRot, pMtx);
    RotMatrixY(-g_WorldMapViewRotationY, (MATRIX *)0x1F800110);
    RotMatrixZ(-g_WorldMapViewRotationZ, (MATRIX *)0x1F800130);
    MulMatrix0(pMtx, (MATRIX *)0x1F800110, (MATRIX *)0x1F800150);
    MulMatrix0((MATRIX *)0x1F800130, (MATRIX *)0x1F800150, &g_WorldMapViewMatrix);
    ((SVECTOR *)0x1F8000A0)->vx = -(u16)p->rotX;
    ((SVECTOR *)0x1F8000A0)->vy = -(u16)p->rotY;
    ((SVECTOR *)0x1F8000A0)->vz = -(u16)p->rotZ;
    *pMtx = g_WorldMapViewMatrix;
    ApplyMatrix(pMtx, (SVECTOR *)0x1F8000A0, (VECTOR *)0x1F800000);
    TransMatrix(&g_WorldMapViewMatrix, (VECTOR *)0x1F800000);
}

void WorldMapAllocSlotPool(void) {
    g_WorldMapSlots = HeapAlloc(0x2000, 0);
    WorldMapClearSlotPool();
}

void WorldMapFreeSlotPool(void) {
    HeapFree(g_WorldMapSlots);
}

void WorldMapClearSlotPool(void) {
    s32 i;

    for (i = 0; i < 0x40; i++) {
        g_WorldMapSlots[i].pSprite = 0;
        g_WorldMapSlots[i].pEnterHandler = 0;
        g_WorldMapSlots[i].pUpdateHandler = 0;
    }
}

void WorldMapSetSlotEnterHandler(s32 (*pHandler)(s32), s32 index) {
    g_WorldMapSlots[index].state = 0;
    g_WorldMapSlots[index].pEnterHandler = pHandler;
}

void WorldMapAddSlot(s32 (*pEnterHandler)(s32), s32 (*pUpdateHandler)(s32)) {
    s32 i;

    for (i = 0; i < 0x40; i++) {
        if (g_WorldMapSlots[i].pUpdateHandler == 0) {
            g_WorldMapSlots[i].state = 0;
            g_WorldMapSlots[i].stateTimer = 0;
            g_WorldMapSlots[i].command = 0;
            g_WorldMapSlots[i].pEnterHandler = pEnterHandler;
            g_WorldMapSlots[i].pUpdateHandler = pUpdateHandler;
            g_WorldMapSlots[i].unk20 = 0;
            g_WorldMapSlots[i].unk22 = 0;
            break;
        }
    }
}

s32 WorldMapStartSlot(s32 index, s16 value) {
    Misc15Slot *pSlot = &g_WorldMapSlots[index];

    if (pSlot->command != 0) {
        return 0;
    }
    pSlot->state = 1;
    pSlot->command = value;
    return 1;
}

void WorldMapSetSlotState3(s32 index) {
    Misc15Slot *pSlot = &g_WorldMapSlots[index];

    pSlot->state = 3;
}

void WorldMapSetSlotState4(s32 index) {
    Misc15Slot *pSlot = &g_WorldMapSlots[index];

    pSlot->state = 4;
}

void WorldMapSetSlotWaitFrames(s32 index, u16 value) {
    Misc15Slot *pSlot = &g_WorldMapSlots[index];

    pSlot->state = 2;
    pSlot->stateTimer = value;
}

extern void SpriteDestroy(void *p);

void WorldMapUpdateSlots(void) {
    Misc15Slot *pSlot;
    s32 i;

    for (i = 0, pSlot = g_WorldMapSlots; i < 0x40; i++, pSlot++) {
        if (pSlot->pUpdateHandler == 0) {
            continue;
        }
        switch (pSlot->state) {
        case 0:
            pSlot->state = pSlot->pEnterHandler(i);
            break;
        case 1:
            pSlot->state = pSlot->pUpdateHandler(i);
            break;
        case 2:
            if (--pSlot->stateTimer <= 0) {
                pSlot->state = 1;
            }
            break;
        case 3:
            break;
        case 4:
            if (pSlot->pSprite != 0) {
                SpriteDestroy((void *)pSlot->pSprite);
            }
            break;
        }
    }
}

// try_flags: --gp -G0 gives 88.53 but drops the matched WorldMapRefreshGrid to
// 97.21 and WorldMapStartCdRead to 91.63, so the unit preset is right; gcc 2.6.0
// gives 79.25 with the parked shape.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800978FC);
// Best: 82.47%. Everything matches except that the original holds s0 =
// &g_WorldMapTilePrimBuffer across the second HeapAlloc and reads the buffer back with
// `lw v1,0(s0)`; every shape tried (global stores, u8 **ppBuffer, a
// u8 *pBuffer local) gets lui/at for the store plus a fresh lui for the read,
// so the frame has no s0 slot and the copy loop's 0x10000 constant lands in
// v0 instead of t0. The two do-while loops and the +6 slot-filler bias are
// required for the rest to match. gcc-2.7.2-cdk gives 84.65%, so the preset is
// not the answer either.
// Round 19 re-check: the parked shape is the best one (76.88% from a direct
// `*ppBuffer = ...` form; adding the pSlot[-3..1] form loses the s0 formation
// entirely -- cc1 rematerialises &g_WorldMapTilePrimBuffer as `lui $at` for the store and a
// fresh `la` for the reload, because copy-prop turns ppBuffer back into the
// constant and cse then expands each use separately. Also try_flags.sh
// compile-fails on every row in this tree (even the default one), so flag
// experiments here have to be done by hand.
/*
void func_800978FC(void) {
    u8 **ppBuffer;
    u8 *pBuffer;
    u8 *pSlot;
    Misc15Block16 *pSrc;
    Misc15Block16 *pDst;
    Misc15Block16 *pEnd;
    s32 i;

    ppBuffer = &g_WorldMapTilePrimBuffer;
    *ppBuffer = HeapAlloc(0x10000, 1);
    g_WorldMapTilePrimBufferCopy = HeapAlloc(0x10000, 1);
    pBuffer = *ppBuffer;
    pSlot = pBuffer;
    i = 0;
    do {
        i++;
        pSlot[3] = 7;
        pSlot[7] = 0x24;
        pSlot[4] = 0x80;
        pSlot[5] = 0x80;
        pSlot[6] = 0x80;
        pSlot += 0x20;
    } while (i < 0x800);
    pSrc = (Misc15Block16 *)g_WorldMapTilePrimBuffer;
    pDst = (Misc15Block16 *)g_WorldMapTilePrimBufferCopy;
    pEnd = pSrc + 0x1000;
    do {
        *pDst++ = *pSrc++;
    } while (pSrc != pEnd);
}
*/
extern void *LZSSHeapDecompress(void *pCompressedData, int flags);
extern void ModelLoadTextureBlock(void *pBlock);
extern void func_800931D8(u8 *pSrc, u8 *pDst, s32 count, u8 *pFactors);

extern u8 *g_WorldMapTexturePackData;
extern u8 g_WorldMapColorFactors[];
extern u16 D_8009CCB4[];
extern u16 g_WorldMapTpageIds[];
extern u16 g_WorldMapTpageIds2[];

void WorldMapLoadClutAndTpageIds(void) {
    RECT rect;
    u8 *pData;
    u8 *pDst;
    u8 *pFactors;
    s32 i;
    s32 x;
    s32 y;
    u16 *pClut;
    u16 *pClut2;
    u16 *pClut3;

    pData = LZSSHeapDecompress(g_WorldMapTexturePackData, 1);
    ModelLoadTextureBlock(pData);
    DrawSync(0);
    HeapFree(pData);
    HeapFree(g_WorldMapTexturePackData);
    i = 0;
    pClut = D_8009CCB4;
    pData = HeapAlloc(0x400, 1);
    pDst = HeapAlloc(0x8000, 1);
    rect.x = 0;
    rect.y = 0x1E0;
    rect.w = 0x100;
    rect.h = 2;
    StoreImage(&rect, (u_long *)pData);
    DrawSync(0);
    pFactors = g_WorldMapColorFactors;
    func_800931D8(pData, pDst, 0x20, pFactors);
    func_800931D8(pData + 0x200, pDst + 0x4000, 0x20, pFactors);
    rect.x = 0;
    rect.y = 0x1B0;
    rect.w = 0x100;
    rect.h = 0x40;
    LoadImage(&rect, (u_long *)pDst);
    DrawSync(0);
    for (; i < 0x40; i++, pClut++) {
        *pClut = GetClut(rect.x, rect.y);
        rect.y++;
    }
    x = 0x200;
    y = 0;
    i = 0;
    pClut2 = g_WorldMapTpageIds;
    for (; i < 4; x += 0x80, i++, pClut2++) {
        *pClut2 = GetTPage(1, 0, x, y);
    }
    x = 0x180;
    y = 0x100;
    i = 4;
    pClut3 = g_WorldMapTpageIds2;
    for (; i < 7; x += 0x80, i++, pClut3++) {
        *pClut3 = GetTPage(1, 0, x, y);
    }
    HeapFree(pDst);
    HeapFree(pData);
}


// Best: 63.31%. Same instruction multiset, different order: cc1 2.7.2's sched2
// hoists the C618/C838/C83C stores above the two loads and splits the mask into
// `li $4,0x7F0000` (kept at the top) + `ori $4,$4,0xFFFF` (emitted 8 insns
// later, just before the ands); retail keeps lui+ori adjacent at the top and the
// stores in source order, which is ASPSX 2.6x's reorder of the same RTL - we
// assemble with GAS, which only inserts load-delay nops and never moves an insn.
// Round 19 confirmed this on build/src/world_map/main/misc15.c.s. Also checked:
// writing the statements in retail's own order changes nothing (sched2 re-does
// it), the direct `p[0] & 0x7FFFFF` form gives 52.66%, and maspsx with li
// expansion on (--maspsx "--use-comm-section --run-assembler", i.e. the Default
// preset's flags) scores 72.42 here while dropping 6 already-matched functions
// in this unit (WorldMapStartSlot 91.43, WorldMapSetSlotState3 91.43, WorldMapAllocSlotPool 95.38),
// so the unit preset is right.
// Round 42: the -O2 schedule is source-order-insensitive here (four statement
// orders, retail's own included, give a byte-identical .s), and a try_flags
// --sweep peaks at 74.19% (-O1, every gcc) / 67.66% (cdk -O1), so retail's
// order is not reachable under the unit's -O2.
/*
void func_80097BC0(s32 *p) {
    u32 *pEnd;
    s32 i;
    s32 x;
    s32 z;

    g_WorldMapGridMatrix = g_WorldMapIdentityMatrix;
    i = 0xFF;
    pEnd = D_8009C580;
    for (; i >= 0; i--) {
        *pEnd-- = 0;
    }
    x = p[0];
    g_WorldMapGridShadePhase = 0x400;
    D_8009BBB8 = 0;
    g_WorldMapGridAngle = 0;
    g_WorldMapScrollX = x & 0x7FFFFF;
    z = p[2];
    g_WorldMapGridOrigin = 2;
    D_8009C83A = 0;
    D_8009C83C = 2;
    D_8009BBBC = z & 0x7FFFFF;
    func_800981C8((s32)p);
    WorldMapSubmitGridImageLoads();
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097BC0);

void WorldMapRefreshGrid(s32 arg0) {
    u32 *pEnd;
    s32 i;

    g_WorldMapGridMatrix = g_WorldMapIdentityMatrix;
    i = 0xFF;
    pEnd = D_8009C580;
    for (; i >= 0; i--) {
        *pEnd-- = 0;
    }
    g_WorldMapGridAngle = 0;
    g_WorldMapGridShadePhase = 0x400;
    func_800981C8(arg0);
    WorldMapSubmitGridImageLoads();
}

void WorldMapFreeGridImages(void) {
    s32 i;

    for (i = 0; i < 0x100; i++) {
        if (g_WorldMapTileCellPointers[i] != NULL) {
            HeapFree(g_WorldMapTileCellPointers[i]);
        }
    }
}

extern s32 g_WorldMapGridImageFileIndex;
extern s16 g_WorldMapGridTileIds[];
extern s32 ArchiveDecodeSector(s32 entryIndex);
extern u8 *ArchiveGetFilePath(s32 entryIndex);
extern s32 WorldMapAddCdRequest(s32 x, s32 y, s32 z);
extern s32 WorldMapAddFileLoadRequest(s32 x, s32 y, s32 z, s32 w);
extern s32 WorldMapSubmitCdRequests(void);
extern s32 WorldMapSubmitFileLoads(void);

void WorldMapSubmitGridImageLoads(void) {
    u32 first;
    u32 second;
    s32 i;
    s32 j;
    s16 id;
    u8 *pSector;
    u8 *pPath;

    first = ArchiveGetDebugTable();
    second = ArchiveGetDebugTable();
    if ((first == 0) | (second == 0xFFFFFFFF)) {
        pSector = (u8 *)ArchiveDecodeSector(g_WorldMapGridImageFileIndex);
        for (i = 3; i < 6; i++) {
            for (j = 3; j < 6; j++) {
                id = g_WorldMapGridTileIds[i * 9 + j];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(0x710, 0);
                    WorldMapAddCdRequest((s32)(pSector + id), 0x710, (s32)g_WorldMapTileCellPointers[id]);
                }
            }
        }
        WorldMapAddCdRequest(0, 0, 0);
        WorldMapSubmitCdRequests();
        for (i = 0; i < 9; i++) {
            for (j = 0; j < 9; j++) {
                id = g_WorldMapGridTileIds[i * 9 + j];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(0x710, 0);
                    WorldMapAddCdRequest((s32)(pSector + id), 0x710, (s32)g_WorldMapTileCellPointers[id]);
                }
            }
        }
        WorldMapAddCdRequest(0, 0, 0);
        WorldMapSubmitCdRequests();
    } else {
        pPath = ArchiveGetFilePath(g_WorldMapGridImageFileIndex);
        for (i = 0; i < 9; i++) {
            for (j = 0; j < 9; j++) {
                id = g_WorldMapGridTileIds[i * 9 + j];
                if (g_WorldMapTileCellPointers[id] == NULL) {
                    g_WorldMapTileCellPointers[id] = HeapAlloc(0x710, 0);
                    WorldMapAddFileLoadRequest((s32)pPath, id << 11, 0x710, (s32)g_WorldMapTileCellPointers[id]);
                }
            }
        }
        WorldMapAddFileLoadRequest(0, 0, 0, 0);
        WorldMapSubmitFileLoads();
    }
}
