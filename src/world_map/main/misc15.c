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
    /* 0x20 */ u16 handlerState; // the state machine of the pEnter/pUpdate handler pair; the map modes declare these two words as their own state/stateTimer
    /* 0x22 */ u16 handlerStateTimer;
    /* 0x24 */ u8 unk24[0x28];
    /* 0x4C */ s32 pSprite;
    /* 0x50 */ u8 unk50[0x30];
} Misc15Slot; // Size: 0x80

typedef struct {
    /* 0x0 */ s32 sector;
    /* 0x4 */ u32 size;
    /* 0x8 */ u8 *dest;
    /* 0xC */ u8 payload[0x18]; // CD read destination, streamed by func_80096C0C
} Misc15CdRequest; // Size: 0x24

// One entry of a request's payload: the cursor walks these 0xC bytes at a time
// until it reaches the entry whose sector is 0.
typedef struct {
    /* 0x0 */ s32 sector;
    /* 0x4 */ u32 size;
    /* 0x8 */ u8 *dest;
} Misc15CdChunk; // Size: 0xC

typedef struct {
    /* 0x0 */ u32 unk0[4];
} Misc15Block16; // Size: 0x10

// g_WorldMapPathFrame: the path position, the point the camera looks at (only
// targetY is ever written, targetX/targetZ stay 0) and the frame's up axis.
typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 z;
    /* 0x6 */ s16 pad6;
    /* 0x8 */ s16 targetX;
    /* 0xA */ s16 targetY;
    /* 0xC */ s16 targetZ;
    /* 0xE */ s16 padE;
    /* 0x10 */ VECTOR upAxis; // (0,-0x1000,0) rotated by the frame's pitch/roll
} Misc15Frame; // Size: 0x20


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
extern void WorldMapBuildTileIdGrid(s32);
extern void WorldMapSubmitGridImageLoads(void);

extern void WorldMapClearSlotPool(void);
extern CdlLOC g_WorldMapCdReadLocation;
extern u8 *g_WorldMapCdRequestCursor;
extern s32 g_WorldMapCdExpectedSector;
extern s32 g_WorldMapCdCurrentSector;
extern s32 g_WorldMapCdReadSectorsLeft;
extern s32 g_WorldMapCdReadBytesLeft;
extern u8 *g_WorldMapCdReadDest;
extern s32 D_8009BE48;
extern s32 D_8009CCB0;
extern s32 g_WorldMapCdReadErrorCount;
extern s32 g_WorldMapCdRetryCount;
extern void WorldMapCdReadCallback(s32 mode, u8 *pResult);
extern void func_80096C0C(s32 mode, u8 *pResult);
extern s32 g_WorldMapCdSectorBuffer;
extern s32 D_8009BCD0;
extern s32 D_8009BCD4;
extern u8 *g_WorldMapCdSectorRemainder;
extern u32 ArchiveGetDebugTable(void);
extern s32 WorldMapStepCdReadState(void);
extern void WorldMapReadFileEntries(void *p);
extern s32 g_WorldMapScrollX;
extern s32 g_WorldMapScrollY;
extern s32 g_WorldMapScrollZ;
extern s16 g_WorldMapGridOrigin;
extern s16 D_8009C83A;
extern s16 g_WorldMapGridTileRowOrigin;

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
void func_80096C0C(s32 mode, u8 *pResult) {
    CdlLOC *pLoc;
    Misc15CdChunk *pChunk;
    CdlLOC *pSectorLoc;
    s32 sector;
    s32 nextSector;

    if (mode == 1) {
        pSectorLoc = (CdlLOC *)&g_WorldMapCdSectorBuffer;
        CdGetSector(pSectorLoc, 3);
        sector = CdPosToInt(pSectorLoc);
        if (sector == g_WorldMapCdExpectedSector) {
            if (g_WorldMapCdCurrentSector == sector) {
                if (g_WorldMapCdReadBytesLeft < 0x800) {
                    CdGetSector(g_WorldMapCdReadDest, g_WorldMapCdReadBytesLeft / 4);
                    CdGetSector(g_WorldMapCdSectorRemainder, (0x800 - g_WorldMapCdReadBytesLeft) / 4);
                } else {
                    CdGetSector(g_WorldMapCdReadDest, 0x200);
                    g_WorldMapCdReadBytesLeft -= 0x800;
                }
                if (--g_WorldMapCdReadSectorsLeft != 0) {
                    g_WorldMapCdCurrentSector++;
                    g_WorldMapCdReadDest += 0x800;
                } else {
                    pChunk = (Misc15CdChunk *)g_WorldMapCdRequestCursor;
                    nextSector = pChunk->sector;
                    g_WorldMapCdCurrentSector = nextSector;
                    g_WorldMapCdReadSectorsLeft = (pChunk->size + 0x7FF) >> 11;
                    g_WorldMapCdRequestCursor = (u8 *)(pChunk + 1);
                    g_WorldMapCdReadBytesLeft = pChunk->size;
                    g_WorldMapCdReadDest = pChunk->dest;
                    if (nextSector == 0) {
                        g_WorldMapCdReadState = 3;
                        CdReadyCallback(NULL);
                        CdControlF(CdlPause, NULL);
                    } else if (nextSector - g_WorldMapCdExpectedSector >= 0x13) {
                        g_WorldMapCdExpectedSector = nextSector;
                        g_WorldMapCdReadState = 1;
                        pLoc = &g_WorldMapCdReadLocation;
                        CdIntToPos(nextSector, pLoc);
                        CdControlF(CdlSetloc, (u8 *)pLoc);
                        return;
                    }
                }
            }
            g_WorldMapCdExpectedSector++;
            return;
        }
        g_WorldMapCdRetryCount++;
        CdReadyCallback(NULL);
        if (pResult[0] & CdlStatShellOpen) {
            g_WorldMapCdReadState = 10;
            g_WorldMapCdReadErrorCount++;
            CdControlF(CdlNop, NULL);
        } else {
            g_WorldMapCdReadState = 11;
            CdControlF(CdlGetTN, NULL);
        }
    } else {
        g_WorldMapCdRetryCount++;
        CdReadyCallback(NULL);
        if (pResult[0] & CdlStatShellOpen) {
            g_WorldMapCdReadState = 10;
            g_WorldMapCdReadErrorCount++;
            CdControlF(CdlNop, NULL);
        } else {
            g_WorldMapCdReadState = 11;
            CdControlF(CdlGetTN, NULL);
        }
    }
}

extern SVECTOR D_1F8000A0;

#define WORLD_MAP_ROT_VECTOR ((SVECTOR *)0x1F8000A0)

// 99.98% (344 bytes / 86 insns = retail's exact size and count): the only difference is
// the ENCODING of the first store, `D_1F8000A0.vx` (a relocation the linker resolves to
// 0x1F8000A0) where retail's object holds the literal. Spelling that store with
// WORLD_MAP_ROT_VECTOR like its neighbours makes cse keep the whole address in $s3 and
// form it in the prologue instead of retail's `lui $at,0x1F80` + late `ori $s3` (96.91%),
// so it stays the one symbol store. Everything else is instruction-identical.
void func_80096F18(Misc15Frame *pOut, u8 *pSrc, s32 height, u16 *pAngles) {
    MATRIX *pMtx;
    s16 *pScratch;

    pScratch = (s16 *)0x1F8000A0;
    pMtx = (MATRIX *)0x1F8000F0;
    pOut->targetX = 0;
    pOut->targetY = *(s32 *)(pSrc + 4) >> 12;
    pOut->targetZ = 0;
    D_1F8000A0.vx = pAngles[0];
    WORLD_MAP_ROT_VECTOR->vy = pAngles[1];
    WORLD_MAP_ROT_VECTOR->vz = 0;
    RotMatrixYXZ(WORLD_MAP_ROT_VECTOR, pMtx);
    *(s32 *)0x1F800000 = 0;
    *(s32 *)0x1F800004 = 0;
    *(s32 *)0x1F800008 = -(height >> 12);
    ApplyMatrixLV(pMtx, (VECTOR *)0x1F800000, (VECTOR *)0x1F800010);
    pOut->x = ((VECTOR *)0x1F800010)->vx;
    pOut->y = pOut->targetY + ((VECTOR *)0x1F800010)->vy;
    pOut->z = ((VECTOR *)0x1F800010)->vz;
    pScratch[0] = 0;
    WORLD_MAP_ROT_VECTOR->vy = pAngles[1];
    WORLD_MAP_ROT_VECTOR->vz = pAngles[2];
    RotMatrixYXZ(WORLD_MAP_ROT_VECTOR, pMtx);
    pScratch[0] = 0;
    WORLD_MAP_ROT_VECTOR->vy = -0x1000;
    WORLD_MAP_ROT_VECTOR->vz = 0;
    ApplyMatrix(pMtx, WORLD_MAP_ROT_VECTOR, &pOut->upAxis);
}



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
// Best: 83.39% (504 bytes / 126 insns; retail 508 / 127, frame 0x20 = retail's).
// Re-measured this round from the parked body at this unit's preset: unchanged.
// -O3 is identical (83.39), cdk 39.61 (already in the note below).
// THE RESIDUAL IS ONE SCHEDULING DECISION, not a type or a call: cc1 emits the three
// SVECTOR negates (0x1F800040) BEFORE the nine halfword stores of
// g_WorldMapViewMatrix, retail emits them AFTER. Consequences, all downstream of that
// one move: retail's 'lui a1,0x1F800030' lands before the third 'lh' pair (filling the
// slot, so retail needs a real 'nop' at 0x27890 where this body has the lui), retail
// reuses a1 for the 0x1F800018 load and only forms a1=0x1F800040 at 0x278B8 after the
// stores, while this body forms a1=0x1F800040 up front, which blocks a1 and costs the
// two extra temporaries t4/t5 (the 127-vs-126 instruction gap).
// TRIED THIS ROUND, all no change: the parked nine-s32-locals shape (83.39), a
// 'SVECTOR *pVec' local for the negates and the ApplyMatrix argument (83.39).
// Earlier rounds: -O3, gcc-2.7.2-cdk (39.61). The register/load-order swaps inside the
// three subtraction pairs also come from that same schedule (retail loads p->x before
// p->targetX in each pair).
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

    ((VECTOR *)0x1F800000)->vx = p->targetX - p->x;
    ((VECTOR *)0x1F800000)->vy = p->targetY - p->y;
    ((VECTOR *)0x1F800000)->vz = p->targetZ - p->z;
    VectorNormal((VECTOR *)0x1F800000, (VECTOR *)0x1F800030);
    OuterProduct12((VECTOR *)0x1F800030, &p->upAxis, (VECTOR *)0x1F800000);
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
}*/
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
            g_WorldMapSlots[i].handlerState = 0;
            g_WorldMapSlots[i].handlerStateTimer = 0;
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

// Best: 99.80% (204 bytes / 51 insns = retail's exact size and count).
// The one residual is a REGISTER TIE on the pEnd constant: retail holds 0x10000
// in $t0 and adds it in place (`lui t0,0x1` ... `addu t0,a2,t0`), this body gets
// `lui v0,0x1` ... `addu t0,a2,v0`, i.e. the constant pseudo and the addu output
// do not share a register. Everything else is instruction-identical.
// What closed the old 82.47% wall (s0 = &g_WorldMapTilePrimBuffer held across the
// second HeapAlloc, retail's `sw v0,0(s0)` + `lw v1,0(s0)`): the array-typed
// alias below. A plain `&g_WorldMapTilePrimBuffer` (also via a `u8 **` local)
// folds back to a bare symbol store plus a fresh `lui`/`lw` for the read.
// Tried on top of the 99.80 body, all no better: pEnd from `(u8 *)pSrc + 0x10000`
// (identical code), pEnd computed from the global before pSrc (208 bytes,
// 97.45), src/end swapped in the copy loop (208 bytes, 97.35), and a 3-minute
// permuter run (PERMUTER_NONE, base=10, 1659 iterations).
// try_flags (pre-alias, parked shape): --gp -G0 88.53 but it drops the matched
// WorldMapRefreshGrid to 97.21 and WorldMapStartCdRead to 91.63; gcc 2.6.0 79.25.
extern u8 *g_WorldMapTilePrimBufferSlots[] __asm__("g_WorldMapTilePrimBuffer");
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800978FC);
/*
void func_800978FC(void) {
    u8 **ppBuffer;
    u8 *pBuffer;
    u8 *pSlot;
    Misc15Block16 *pSrc;
    Misc15Block16 *pDst;
    Misc15Block16 *pEnd;
    s32 i;

    ppBuffer = g_WorldMapTilePrimBufferSlots;
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


// The volatile stores pin the statement order. Any plain spelling lets sched2
// hoist the C838/C83C/0x400 stores above the two loads and split the 0x7FFFFF
// mask into a lui at the top plus an ori 8 insns later (63.31%); the volatile
// read of p[2] is what keeps that load out of the sh group, where a plain load
// is hoisted a block early and needs a nop (91.77%).
void WorldMapSetGridPosition(s32 *p) {
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
    *(volatile s32 *)&g_WorldMapScrollY = 0;
    *(volatile s32 *)&g_WorldMapGridAngle = 0;
    *(volatile s32 *)&g_WorldMapGridShadePhase = 0x400;
    *(volatile s32 *)&g_WorldMapScrollX = x & 0x7FFFFF;
    z = *(volatile s32 *)&p[2];
    *(volatile s16 *)&g_WorldMapGridOrigin = 2;
    *(volatile s16 *)&D_8009C83A = 0;
    *(volatile s16 *)&g_WorldMapGridTileRowOrigin = 2;
    g_WorldMapScrollZ = z & 0x7FFFFF;
    WorldMapBuildTileIdGrid((s32)p);
    WorldMapSubmitGridImageLoads();
}

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
    WorldMapBuildTileIdGrid(arg0);
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
