#include "common.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"
#include "psyq/libgpu.h"

extern int Vsync(int mode);
extern void ArchiveCdDataSync(int mode);
extern u_char ArchiveCdSetMode(u_char mode);
extern u32 ArchiveGetDebugTable(void);
extern void func_8002A498(int mode);
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags);

// .data
extern s32 D_80076E4C;
extern s32 D_80076E5C;
extern void* D_80076EA0;
extern s32 D_80076EAC;
extern s32 D_80076EB0;
extern s32 D_80076EC8;
extern s32 D_80076ECC;
extern s32 D_80076EF4;
extern s32 D_80076EF8;
extern s32 D_80076F3C[];
// .bss
extern u_char D_80076F84[];
extern u_char D_80076FCC[];
extern u_char D_80076FCD[];
extern u_char D_80076FCE[];
extern u_char D_80076FDC[];
extern u_char D_80076FDD[];
extern u_char D_80076FDE[];
extern s32 D_80076FAC[];
extern s32 D_80076FBC[];
extern u_char D_80076F8C[];
extern u_char D_80076F8D[];
extern u_char D_80076F8E[];
extern u_char D_80076F9C[];
extern u_char D_80076F9D[];
extern u_char D_80076F9E[];
extern s32 D_80076FEC[];
extern s32 D_80076FFC[];

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800704E8);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80070DCC);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800712C4);

void func_80071BA0(void) {
    if (D_80076EA0 == NULL) {
        D_80076EA0 = HeapAlloc(0x2000, 0);
    }
    if (D_80076EA0 != NULL) {
        ArchiveReadFileFromCdSector(0x40, D_80076EA0, 0x2000, 0, 0);
    }
    D_80076F3C[0]++;
    if (D_80076E4C == 0) {
        D_80076EAC = D_80076EB0;
        D_80076EB0 = 0;
    }
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80071C34);

void func_80072428(void) {
    D_80076E5C++;
    if (++D_80076ECC >= 0x3C) {
        D_80076ECC = 0;
        D_80076EC8++;
    }
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072480);

void func_8007293C(void) {
    if (ArchiveGetDebugTable() == 0) {
        func_8002A498(0);
        ArchiveCdDataSync(0);
        ArchiveCdSetMode(0);
        ArchiveCdDataSync(0);
        Vsync(3);
        while (CdControlB(8, 0, D_80076F84) == 0) {
        }
    }
}

void func_800729A8(char* pName, char* pBuffer, int len) {
    int fd;

    fd = PCopen(pName, 0, 0);
    PCread(fd, pBuffer, len);
    PCclose(fd);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072A08);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072D84);

// Best attempt (96.65%): the instruction stream is otherwise identical. gcc emits
// the loop-index save before the div-by-3 magic and the 6th stack argument where
// retail emits it after them, and the div-by-3 quotient lands in v0 here and v1
// in retail. Naming the 8 as a local (kept below) is what fixed the equivalent
// ordering in func_80073328.
/*
void func_80072D84(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 j;
    s32 brightness;

    brightness = 8;

    for (i = 0; i < 0x10; i += 4) {
        D_80076F8C[i] = (func_80074AF0() & 0xFF) / 32 + 8;
        D_80076F8D[i] = brightness;
        D_80076F8E[i] = (func_80074AF0() & 0xFF) / 3 + 0x10;
        D_80076F9C[i] = (func_80074AF0() & 0xFF) / 32 + 8;
        D_80076F9D[i] = brightness;
        D_80076F9E[i] = (func_80074AF0() & 0xFF) / 3 + 0x10;
    }

    for (j = 0; j < 4; j++) {
        D_80076FAC[j] = 0;
        D_80076FBC[j] = (func_80074AF0() & 0xFF) + 0x20;
    }

    SetPolyG4(pPoly0);
    SetSemiTrans(pPoly0, 0);
    setXY4(pPoly0, x, y, x + w, y, x, y + h, x + w, y + h);
    SetPolyG4(pPoly1);
    SetSemiTrans(pPoly1, 0);
    setXY4(pPoly1, x, y, x + w, y, x, y + h, x + w, y + h);
}
*/

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072F98);

void func_80073328(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 j;
    s32 color;

    color = 0xFF;

    // The colour triplets are 4-byte structs and splat labels every byte, so the
    // three channels are addressed as byte arrays a stride of 4 apart.
    for (i = 0; i < 0x10; i += 4) {
        D_80076FCC[i] = color;
        D_80076FCD[i] = color;
        D_80076FCE[i] = (func_80074AF0() & 0x3F) - 0x42;
        D_80076FDC[i] = color;
        D_80076FDD[i] = color;
        D_80076FDE[i] = (func_80074AF0() & 0x3F) - 0x42;
    }

    for (j = 0; j < 4; j++) {
        D_80076FEC[j] = 0;
        D_80076FFC[j] = (func_80074AF0() & 0xFF) + 0x20;
    }

    SetPolyG4(pPoly0);
    setXY4(pPoly0, x, y, x + w, y, x, y + h, x + w, y + h);
    SetPolyG4(pPoly1);
    setXY4(pPoly1, x, y, x + w, y, x, y + h, x + w, y + h);
}

// The colour triplets live at a stride of 4 in the byte arrays splat labels, so
// they are indexed with `i * 4`; the step/total counters are plain s32 arrays.
void func_800734B8(u32* pOther, POLY_G4* pPoly, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 r;
    s32 g;
    s32 b;

    pPoly->x0 = x;
    pPoly->y0 = y;
    pPoly->y1 = y;
    pPoly->x2 = x;
    pPoly->x1 = x + w;
    pPoly->y2 = y + h;
    pPoly->x3 = x + w;
    pPoly->y3 = y + h;

    for (i = 0; i < 4; i++) {
        if (++D_80076FEC[i] > D_80076FFC[i]) {
            D_80076FEC[i] = 0;
            D_80076FFC[i] = (func_80074AF0() & 0xFF) + 0x20;
            D_80076FCC[i * 4] = D_80076FDC[i * 4];
            D_80076FCC[i * 4 + 1] = D_80076FDC[i * 4 + 1];
            D_80076FCC[i * 4 + 2] = D_80076FDC[i * 4 + 2];
            D_80076FDC[i * 4] = 0xFF;
            D_80076FDC[i * 4 + 1] = 0xFF;
            D_80076FDC[i * 4 + 2] = (func_80074AF0() & 0x3F) - 0x42;
        }

        r = (D_80076FDC[i * 4] - D_80076FCC[i * 4]) * D_80076FEC[i] / D_80076FFC[i] + D_80076FCC[i * 4];
        g = (D_80076FDC[i * 4 + 1] - D_80076FCC[i * 4 + 1]) * D_80076FEC[i] / D_80076FFC[i] + D_80076FCC[i * 4 + 1];
        b = (D_80076FDC[i * 4 + 2] - D_80076FCC[i * 4 + 2]) * D_80076FEC[i] / D_80076FFC[i] + D_80076FCC[i * 4 + 2];

        switch (i) {
        case 0:
            pPoly->r0 = r;
            pPoly->g0 = g;
            pPoly->b0 = b;
            break;
        case 1:
            pPoly->r1 = r;
            pPoly->g1 = g;
            pPoly->b1 = b;
            break;
        case 2:
            pPoly->r2 = r;
            pPoly->g2 = g;
            pPoly->b2 = b;
            break;
        case 3:
            pPoly->r3 = r;
            pPoly->g3 = g;
            pPoly->b3 = b;
            break;
        }
    }

    *(u32*)pPoly = (*(u32*)pPoly & 0xFF000000) | (*pOther & 0x00FFFFFF);
    *pOther = (*pOther & 0xFF000000) | ((u32)pPoly & 0x00FFFFFF);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800737EC);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800747AC);

s32 func_80074AF0(void) {
    s32 a;
    s32 b;
    s32 r;

    a = D_80076EF4 * 5 + 1;
    b = D_80076EF8 * 7 + 3;
    D_80076EF4 = a;
    D_80076EF8 = b;
    r = (a ^ b) + 1;
    D_80076EF4 = r;
    if (r < 0) {
        D_80076EF4 = -r;
    }
    return D_80076EF4;
}
