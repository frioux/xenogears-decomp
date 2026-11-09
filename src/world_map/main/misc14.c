#include "common.h"
#include "system/memory.h"

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

extern int Vsync(int mode);
extern u32 ArchiveGetDebugTable(void);
extern s32 D_8009BCB8;
extern s32 D_8009BE44;
extern s32 D_8009D808;
extern SVec* D_8009BE08;
extern SVec4* D_8009D3C0;
extern void* D_8009D7D4;
extern void* D_8009D788[16];
extern void func_800963E4(SVec* pRecord);
extern void func_800967E4(void);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094238);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094364);

void func_80094434(void) {
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_8009443C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800945C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094750);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800948D8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80094A5C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800951A8);

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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80095324);
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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_80096130);

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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800963E4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800964B0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800965A4);

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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc14", func_800966CC);
