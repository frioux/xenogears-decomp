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
extern void func_800963E4(SVec* pRecord);
extern void func_800964B0(SVec4* pRecord);
extern void func_800967E4(void);

s32 func_80094238(SVec* p, s32 index) {
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

s32 func_80094364(SVec* p, s32 index, s32 mode) {
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

void func_80094434(void) {
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_8009443C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800945C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094750);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800948D8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094A5C);
extern s32 func_80094088(SVec* p0, SVec* p1, SVec* p2);
extern s32 func_80094A5C(void* p0, SVec* p1, s32 mode, s16 kind);

s32 func_800951A8(void* p0, SVec* p1, SVec* p2, s32 mode, s16 kind) {
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

void func_800952B0(SVec* p0, SVec* p1, SVec* p2) {
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

void func_80095324(void* pOut, SVec* pDir, SVec* pResult) {
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80095F78);

void func_800960BC(void) {
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

void func_80096130(void) {
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

s32 func_8009623C(s32 x, s32 y, s32 z) {
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

s32 func_800962B0(s32 x, s32 y, s32 z, s32 w) {
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

s32 func_80096328(void) {
    s32 index = D_8009BE44;
    SVec* p = &D_8009BE08[index * 0x58];

    if (p->x != 0 && D_8009D788[index] == NULL) {
        func_800963E4(p);
        D_8009D808 = 0;
        D_8009D788[D_8009BE44] = p;
        D_8009BE44 = (D_8009BE44 + 1) & 0xF;
        return 0;
    }
    D_8009D808 = 0;
    return -1;
}

void func_800963E4(SVec* pList) {
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
void func_800964B0(SVec4* pList) {
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
s32 func_800965A4(void) {
    s32 index = D_8009BE44;
    SVec4* p = &D_8009D3C0[index * 0x58];

    if (p->x != 0 && D_8009C624[index] == NULL) {
        func_800964B0(p);
        D_8009D808 = 0;
        D_8009C624[D_8009BE44] = p;
        D_8009BE44 = (D_8009BE44 + 1) & 0xF;
        return 0;
    }
    D_8009D808 = 0;
    return -1;
}

s32 func_80096668(void) {
    s32 diff = D_8009BE44 - D_8009BCB8;

    if (diff < 0) {
        diff += 0x10;
    }
    return diff;
}

void func_80096694(void) {
    do {
        Vsync(0);
        func_800967E4();
    } while (func_80096668() != 0);
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

void func_800966CC(WorldMapLoadEntry* pEntry) {
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
