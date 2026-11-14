#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} SVec; // Size: 0xC

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} SVec4; // Size: 0x10

typedef struct {
    /* 0x0 */ s16 left;
    /* 0x2 */ s16 top;
    /* 0x4 */ s16 width;
    /* 0x6 */ s16 height;
    /* 0x8 */ s16 kind;
    /* 0xA */ s16 unkA;
    /* 0xC */ u16 value;
    /* 0xE */ s16 mode;
} WorldMapRect; // Size: 0x10

extern int Vsync(int mode);
extern u32 ArchiveGetDebugTable(void);
extern s32 D_8009BCB8;
extern s32 D_8009BE44;
extern s32 D_8009D808;
extern SVec* D_8009BE08;
extern SVec4* D_8009D3C0;
extern void* D_8009D7D4;
extern void* D_8009D788[16];
extern void* D_8009C624[16];
extern WorldMapRect** D_8009BD00;
extern void* D_8009D7D8;
extern s16 D_8009BD24;
extern s16 D_8009CE68;
extern void WorldMapSortCdRequestsBySector(SVec* pRecord);
extern void WorldMapSortFileLoadsByOffset(SVec4* pRecord);
extern void func_800967E4(void);
extern void WorldMapWrapPosition(void* p);
extern s32 WorldMapGetTileHalfIndex(void* p);
extern s16 WorldMapGetTileKindValue(s16 row, s16 col);

// Scratchpad probe at 0x1F800000: the near/far pair at 0x20/0x30 is projected
// in place and the winning one is copied down to 0x00. v.x/v.z is a scratch
// temp; 0x10 and 0x18 are the x- and z-references WorldMapProbeCellEdgeXFromReference and
// WorldMapProbeCellEdgeZFromReference measure from. The three stores of a projection must keep the
// temp first: writing the scaled field first lets gcc forward the temp and drop
// the reload of it that the original does.
typedef struct {
    /* 0x00 */ SVec4 v;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ SVec4 near;
    /* 0x30 */ SVec4 far;
} WorldMapProbe; // Size: 0x40

s32 WorldMapSelectAreaAtPosition(SVec* p, s32 index) {
    WorldMapRect* pRect;
    u16 x;
    u16 z;

    x = (u32)p->x >> 12;
    pRect = D_8009BD00[index];
    z = (u32)p->z >> 12;
    while (pRect->kind != -1) {
        if ((x >= pRect->left) & (x <= pRect->left + pRect->width) & (z >= pRect->top) &
            (z <= pRect->top + pRect->height)) {
            if (pRect->mode == 4) {
                D_8009D7D8 = (void*)-1;
                D_8009BD24 = -1;
                D_8009CE68 = pRect->value;
            } else {
                D_8009D7D8 = pRect;
                D_8009CE68 = -1;
                D_8009BD24 = pRect->value;
            }
            return 1;
        }
        pRect++;
    }
    D_8009D7D8 = (void*)-1;
    D_8009BD24 = -1;
    D_8009CE68 = -1;
    return 0;
}

s32 WorldMapFindAreaOfModeAtPosition(SVec* p, s32 index, s32 mode) {
    WorldMapRect* pRect;
    u16 x;
    u16 z;

    x = (u32)p->x >> 12;
    pRect = D_8009BD00[index];
    z = (u32)p->z >> 12;
    while (pRect->kind != -1) {
        if ((x >= pRect->left) & (x <= pRect->left + pRect->width) & (z >= pRect->top) &
            (z <= pRect->top + pRect->height)) {
            if (pRect->mode == mode) {
                D_8009D7D8 = pRect;
                return 1;
            }
        }
        pRect++;
    }
    return 0;
}

void WorldMapNop(void) {
}

s32 WorldMapProbeCellEdgeXFromReference(SVec* pPos, SVec* pAim, WorldMapProbe* pProbe, s16 kind) {
    SVec4* pNear;
    SVec4* pFar;
    s32 scale;
    s32 edge;

    scale = (pAim->z << 12) / pAim->x;
    pNear = &pProbe->near;
    pFar = &pProbe->far;
    edge = (pProbe->unk10 & 0xFFF80000) - pPos->x;
    pProbe->v.x = edge;
    pNear->x = edge - 1;
    pFar->x = pProbe->v.x;
    pNear->z = ((scale * pNear->x) >> 12) + pPos->z;
    pFar->z = ((scale * pFar->x) >> 12) + pPos->z;
    pNear->x += pPos->x;
    pFar->x += pPos->x;
    WorldMapWrapPosition(pNear);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pNear)) == 0) {
        pProbe->v = pProbe->near;
        return 1;
    }
    WorldMapWrapPosition(pFar);
    return WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pFar)) == 0 ? 3 : 0;
}

s32 WorldMapProbeCellEdgeXFromPosition(SVec* pPos, SVec* pAim, WorldMapProbe* pProbe, s16 kind) {
    SVec4* pNear;
    SVec4* pFar;
    s32 scale;
    s32 edge;

    scale = (pAim->z << 12) / pAim->x;
    pNear = &pProbe->near;
    pFar = &pProbe->far;
    edge = (pPos->x & 0xFFF80000) - pPos->x;
    pProbe->v.x = edge;
    pNear->x = edge;
    pFar->x = pProbe->v.x - 1;
    pNear->z = ((scale * pNear->x) >> 12) + pPos->z;
    pFar->z = ((scale * pFar->x) >> 12) + pPos->z;
    pNear->x += pPos->x;
    pFar->x += pPos->x;
    WorldMapWrapPosition(pNear);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pNear)) == 0) {
        pProbe->v = pProbe->near;
        return 1;
    }
    WorldMapWrapPosition(pFar);
    return WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pFar)) == 0 ? 3 : 0;
}

s32 WorldMapProbeCellEdgeZFromReference(SVec* pPos, SVec* pAim, WorldMapProbe* pProbe, s16 kind) {
    SVec4* pNear;
    SVec4* pFar;
    s32 scale;
    s32 edge;

    scale = (pAim->x << 12) / pAim->z;
    pNear = &pProbe->near;
    pFar = &pProbe->far;
    edge = (pProbe->unk18 & 0xFFF80000) - pPos->z;
    pProbe->v.z = edge;
    pNear->z = edge - 1;
    pFar->z = pProbe->v.z;
    pNear->x = ((scale * pNear->z) >> 12) + pPos->x;
    pFar->x = ((scale * pFar->z) >> 12) + pPos->x;
    pNear->z += pPos->z;
    pFar->z += pPos->z;
    WorldMapWrapPosition(pNear);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pNear)) == 0) {
        pProbe->v = pProbe->near;
        return 1;
    }
    WorldMapWrapPosition(pFar);
    return WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pFar)) == 0 ? 2 : 0;
}

s32 WorldMapProbeCellEdgeZFromPosition(SVec* pPos, SVec* pAim, WorldMapProbe* pProbe, s16 kind) {
    SVec4* pNear;
    SVec4* pFar;
    s32 scale;
    s32 edge;

    scale = (pAim->x << 12) / pAim->z;
    pNear = &pProbe->near;
    pFar = &pProbe->far;
    edge = (pPos->z & 0xFFF80000) - pPos->z;
    pProbe->v.z = edge;
    pNear->z = edge;
    pFar->z = pProbe->v.z - 1;
    pNear->x = ((scale * pNear->z) >> 12) + pPos->x;
    pFar->x = ((scale * pFar->z) >> 12) + pPos->x;
    pNear->z += pPos->z;
    pFar->z += pPos->z;
    WorldMapWrapPosition(pNear);
    if (WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pNear)) == 0) {
        pProbe->v = pProbe->near;
        return 1;
    }
    WorldMapWrapPosition(pFar);
    return WorldMapGetTileKindValue(kind, WorldMapGetTileHalfIndex(pFar)) == 0 ? 2 : 0;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094A5C);
extern s32 func_80094088(SVec* p0, SVec* p1, SVec* p2);
extern s32 func_80094A5C(void* p0, SVec* p1, s32 mode, s16 kind);

s32 WorldMapResolveStepAgainstTerrain(void* p0, SVec* p1, SVec* p2, s32 mode, s16 kind) {
    s32 ret;
    s32 state;
    s32 result;

    state = func_80094A5C(p0, p1, mode, kind);
    switch (state) {
    case 0:
        ret = 1;
        break;
    case 1:
        result = func_80094088((SVec*)0x1F800000, p1, p2);
        ret = 0;
        if (result == 0) {
            p2->z = 0;
            p2->y = 0;
            p2->x = 0;
        }
        break;
    case 2:
        p2->x = p1->x < 0 ? -0x1000 : 0x1000;
        p2->z = 0;
        p2->y = 0;
        ret = 0;
        break;
    case 3:
        p2->y = 0;
        p2->x = 0;
        p2->z = p1->z < 0 ? -0x1000 : 0x1000;
        ret = 0;
        break;
    }
    return ret;
}

void WorldMapAlignDirectionToReference(SVec* p0, SVec* p1, SVec* p2) {
    s32 dot = p2->x * p0->x + p2->z * p0->z;

    if (dot < 0) {
        p1->x = -p2->x;
        p1->z = -p2->z;
    } else if (dot > 0) {
        p1->x = p2->x;
        p1->z = p2->z;
    } else {
        p1->z = 0;
        p1->x = 0;
    }
    p1->y = 0;
}

void WorldMapAlignPerpendicularToReference(void* pOut, SVec* pDir, SVec* pResult) {
    s32 dot;
    s32 nx;

    *(s32*)0x1F800018 = 0;
    *(s32*)0x1F800010 = 0;
    *(s32*)0x1F800014 = -0x1000;
    OuterProduct12((VECTOR*)pOut, (VECTOR*)0x1F800010, (VECTOR*)0x1F800020);
    VectorNormal((VECTOR*)0x1F800020, (VECTOR*)0x1F800000);
    nx = ((SVec*)0x1F800000)->x;
    dot = nx * pDir->x + ((SVec*)0x1F800000)->z * pDir->z;
    if (dot < 0) {
        pResult->x = -nx;
        pResult->z = -((SVec*)0x1F800000)->z;
    } else if (dot > 0) {
        pResult->x = nx;
        pResult->z = ((SVec*)0x1F800000)->z;
    } else {
        pResult->z = 0;
        pResult->x = 0;
    }
    pResult->y = 0;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80095414);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80095CD4);
/* best: 87.03% (688 bytes vs 704). The whole body matches except the probe
   scratchpad at 0x1F800060/64/68: retail re-materialises each constant address
   (`lui $at,0x1F80` + `sw ...0x60($at)`, `lui $v0,0x1F80` + `lw ...0x64($v0)`),
   while gcc folds every 2+-use literal MEM address into a callee-saved register
   (`li $17,0x1F800000; ori $17,$17,0x60` then `sw $2,0($17)`), which also takes
   the s-registers retail gives to i/found/the sprite table. Tried: single
   `*(s32*)0x1F8000xx` literals, a typed SVec* probe local, `((SVec*)0x1F800060)`
   member accesses (75%), `volatile` on the scratch accesses, and the symbolic
   `D_1F800060.x` spelling (that one emits %hi/%lo relocations retail does not
   have). A one-store/one-load probe test still makes a const register, so the
   trigger is two MEM uses of the same literal address, not the argument uses.
   Also open: the 5th argument arrives as a word (`lw $t0,0x60($sp)`,
   `sw $t0,0x10($sp)`) but WorldMapResolveStepAgainstTerrain takes an s16, so
   ours sign-extends into $v0 first; retail never narrows.

s32 func_80095CD4(SVec* pBase, SVec* pDelta, SVec* pOut, s32 scale, s32 kind) {
    s32 height;
    s32 count;
    s32 i;
    s32 found;
    s32 ret;
    u16 spriteIndex;

    *(s32*)0x1F800060 = pBase->x + ((pDelta->x * scale) >> 12);
    *(s32*)0x1F800064 = pBase->y + ((pDelta->y * scale) >> 12);
    *(s32*)0x1F800068 = pBase->z + ((pDelta->z * scale) >> 12);
    WorldMapWrapPosition((void*)0x1F800060);

    pOut->x = pBase->x + ((pDelta->x * scale) >> 12);
    pOut->y = pBase->y + ((pDelta->y * scale) >> 12);
    pOut->z = pBase->z + ((pDelta->z * scale) >> 12);
    WorldMapWrapPosition(pOut);

    height = WorldMapGetTilePlaneHeight(*(s32*)0x1F800060, *(s32*)0x1F800068) - 0x20000;
    if (height > 0x20000) {
        height = 0x20000;
    }
    if (height < *(s32*)0x1F800064 || height < -0x280000) {
        *(s32*)0x1F800064 = height;
        pOut->y = height;
    }
    if (height < -0x280000 && *(s32*)0x1F800064 < -0x280000) {
        *(s32*)0x1F800064 = -0x280000;
        pOut->y = -0x280000;
    }

    count = WorldMapFindSpriteSet((void*)0x1F800060, (s16*)&spriteIndex);
    if (count == 0) {
        ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
    } else {
        found = 0;
        for (i = 0; i < count; i += 2) {
            if (func_80085418(&D_1F800060, 0x70, spriteIndex, D_8009D718[i]) == 0) {
                D_8009D718[i] = -1;
                found += 2;
            }
        }
        if (found == count) {
            ret = WorldMapResolveStepAgainstTerrain(pBase, pDelta, pOut, scale, kind);
        } else {
            pOut->x = -pDelta->x >> 1;
            pOut->y = -pDelta->y >> 1;
            pOut->z = -pDelta->z >> 1;
            ret = 0;
        }
    }
    return ret;
}
*/
extern void* D_8009D7C4;
extern void* D_8009C660;
extern u8 D_8009C58F;
extern s32 D_8009CD44;

void WorldMapInitLoadQueues(void) {
    u32 first = ArchiveGetDebugTable();
    u32 second = ArchiveGetDebugTable();
    void** pSlot;
    u8* pName;
    s32 i;

    if ((first == 0) | (second == 0xFFFFFFFF)) {
        pSlot = &D_8009D7C4;
        D_8009BCB8 = 0;
        D_8009BE44 = 0;
        D_8009CD44 = 0;
        for (i = 15; i >= 0; i--) {
            *pSlot-- = NULL;
        }
        D_8009BE08 = HeapAlloc(0x4200, 0);
        D_8009D7D4 = HeapAlloc(0x800, 0);
        i = 7;
        pName = &D_8009C58F;
        D_8009D808 = 0;
        do {
            *pName-- = 0;
        } while (--i >= 0);
    } else {
        pSlot = &D_8009C660;
        D_8009BCB8 = 0;
        D_8009BE44 = 0;
        D_8009CD44 = 0;
        for (i = 15; i >= 0; i--) {
            *pSlot-- = NULL;
        }
        D_8009D3C0 = HeapAlloc(0x5800, 0);
        D_8009D7D4 = HeapAlloc(0x800, 0);
        i = 7;
        pName = &D_8009C58F;
        D_8009D808 = 0;
        do {
            *pName-- = 0;
        } while (--i >= 0);
    }
}

void WorldMapFreeLoadQueues(void) {
    u32 first = ArchiveGetDebugTable();
    u32 second = ArchiveGetDebugTable();
    void* pDebugTable;

    if ((first == 0) | (second == 0xFFFFFFFF)) {
        pDebugTable = D_8009BE08;
    } else {
        pDebugTable = D_8009D3C0;
    }
    HeapFree(pDebugTable);
    HeapFree(D_8009D7D4);
}

void WorldMapWaitForLoadSlotFree(void) {
    u32 first = ArchiveGetDebugTable();
    u32 second = ArchiveGetDebugTable();

    if ((first == 0) | (second == 0xFFFFFFFF)) {
        while (D_8009D788[D_8009BE44] != NULL) {
            Vsync(0);
            func_800967E4();
        }
    } else {
        while (D_8009C624[D_8009BE44] != NULL) {
            Vsync(0);
            func_800967E4();
        }
    }
}

s32 WorldMapAddCdRequest(s32 x, s32 y, s32 z) {
    SVec* p;
    s32 index;

    if (D_8009D808 < 0x58) {
        index = D_8009D808;
        D_8009D808 = index + 1;
        p = &D_8009BE08[D_8009BE44 * 0x58];
        p += index;
        p->x = x;
        p->y = y;
        p->z = z;
        return 0;
    }
    return -1;
}

s32 WorldMapAddFileLoadRequest(s32 x, s32 y, s32 z, s32 w) {
    SVec4* p;
    s32 index;

    if (D_8009D808 < 0x58) {
        index = D_8009D808;
        D_8009D808 = index + 1;
        p = &D_8009D3C0[D_8009BE44 * 0x58];
        p += index;
        p->x = x;
        p->y = y;
        p->z = z;
        p->w = w;
        return 0;
    }
    return -1;
}

s32 WorldMapSubmitCdRequests(void) {
    s32 index = D_8009BE44;
    SVec* p = &D_8009BE08[index * 0x58];

    if (p->x != 0 && D_8009D788[index] == NULL) {
        WorldMapSortCdRequestsBySector(p);
        D_8009D808 = 0;
        D_8009D788[D_8009BE44] = p;
        D_8009BE44 = (D_8009BE44 + 1) & 0xF;
        return 0;
    }
    D_8009D808 = 0;
    return -1;
}

void WorldMapSortCdRequestsBySector(SVec* pList) {
    SVec* pCur = pList;
    s32 tmp[3];

    while (pCur[1].x != 0) {
        if (pCur[0].x > (u32)pCur[1].x) {
            tmp[0] = pCur[0].x;
            tmp[1] = pCur[0].y;
            tmp[2] = pCur[0].z;
            pCur[0].x = pCur[1].x;
            pCur[0].y = pCur[1].y;
            pCur[0].z = pCur[1].z;
            pCur[1].x = tmp[0];
            pCur[1].y = tmp[1];
            pCur[1].z = tmp[2];
            if (pList < pCur) {
                pCur--;
            }
        } else {
            pCur++;
        }
    }
}
void WorldMapSortFileLoadsByOffset(SVec4* pList) {
    SVec4* pCur = pList;
    u32 tmp[4];

    while (pCur[1].x != 0) {
        if ((u32)pCur[0].y > (u32)pCur[1].y) {
            tmp[0] = pCur[0].x;
            tmp[1] = pCur[0].y;
            tmp[2] = pCur[0].z;
            tmp[3] = pCur[0].w;
            pCur[0].x = pCur[1].x;
            pCur[0].y = pCur[1].y;
            pCur[0].z = pCur[1].z;
            pCur[0].w = pCur[1].w;
            pCur[1].x = tmp[0];
            pCur[1].y = tmp[1];
            pCur[1].z = tmp[2];
            pCur[1].w = tmp[3];
            if (pList < pCur) {
                pCur--;
            }
        } else {
            pCur++;
        }
    }
}
s32 WorldMapSubmitFileLoads(void) {
    s32 index = D_8009BE44;
    SVec4* p = &D_8009D3C0[index * 0x58];

    if (p->x != 0 && D_8009C624[index] == NULL) {
        WorldMapSortFileLoadsByOffset(p);
        D_8009D808 = 0;
        D_8009C624[D_8009BE44] = p;
        D_8009BE44 = (D_8009BE44 + 1) & 0xF;
        return 0;
    }
    D_8009D808 = 0;
    return -1;
}

s32 WorldMapGetQueuedLoadCount(void) {
    s32 diff = D_8009BE44 - D_8009BCB8;

    if (diff < 0) {
        diff += 0x10;
    }
    return diff;
}

void WorldMapWaitForQueuedLoads(void) {
    do {
        Vsync(0);
        func_800967E4();
    } while (WorldMapGetQueuedLoadCount() != 0);
}


typedef struct {
    /* 0x0 */ char* pName;
    /* 0x4 */ s32 offset;
    /* 0x8 */ s32 size;
    /* 0xC */ char* pBuffer;
} WorldMapLoadEntry; // Size: 0x10

extern int PCopen(char* name, int flags, int perms);
extern int PClseek(int fd, int offset, int mode);
extern int PCread(int fd, char* buff, int len);
extern int PCclose(int fd);
extern s32 D_8009BE48;
extern s32 D_8009CCA0;
extern s32 D_8009CCA8;
extern s32 D_8009CCB0;

void WorldMapReadFileEntries(WorldMapLoadEntry* pEntry) {
    s32 retry;
    s32 fd;

    D_8009BE48 = 0;
    D_8009CCB0 = 0;
    D_8009CCA8 = 0;
    D_8009CCA0 = 0;
    for (; pEntry->pName != NULL; pEntry++) {
        retry = 0;
        do {
            fd = PCopen(pEntry->pName, 0, 0);
            if (fd != -1) {
                break;
            }
        } while (++retry < 8);
        if (fd == -1) {
            continue;
        }
        PClseek(fd, pEntry->offset, 0);
        retry = 0;
        do {
            if (PCread(fd, pEntry->pBuffer, pEntry->size) != 0) {
                break;
            }
        } while (++retry < 8);
        retry = 0;
        do {
            if (PCclose(fd) == 0) {
                break;
            }
        } while (++retry < 8);
    }
}
