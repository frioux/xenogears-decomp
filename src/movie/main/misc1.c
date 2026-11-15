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
typedef struct {
    /* 0x0 */ u_char r;
    /* 0x1 */ u_char g;
    /* 0x2 */ u_char b;
    /* 0x3 */ u_char pad3;
} MovieColour; // Size: 0x4

extern MovieColour D_80076F8C[];
extern MovieColour D_80076F9C[];
extern s32 D_80076FEC[];
extern s32 D_80076FFC[];

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800704E8);

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u32 unk4;
} MovieStreamEntry; // Size: 0x8

extern s32 D_80076E48;
extern MovieStreamEntry* D_80076E7C;
extern u32* D_80076E84;
extern s32 D_80076E94;
extern u32* D_80076E9C;
extern s32 D_80076EA4;
extern s32 D_80076EB4;
extern s32 D_80076EB8;
extern s32 D_80076EBC;
extern s32 D_800773AC;
extern s32 D_800773B4;
extern u8 D_80076F7C[];
extern s32 D_80076F80;
extern u32* func_80028B14(void);
extern s32 func_80028F30(u8* pArg0, s32* pArg1);
extern void func_8002945C(u32* pStream);
extern void func_800294B4(s32 arg0);
extern s32 ArchiveDecodeAlignedSize(u32 entryIndex);
extern void func_80071C34(s32 arg0);

void MovieUpdateStream(void) {
    s32 size;
    u32 value;

    if ((D_800773B4 & 0x20) == 0 && (D_800773AC & 0x20) != 0) {
        func_80071C34(1);
    }
    if ((D_800773B4 & 0x10) == 0 && (D_800773AC & 0x10) != 0) {
        func_80071C34(2);
    }
    if ((D_800773B4 & 0x80) == 0 && (D_800773AC & 0x80) != 0) {
        func_80071C34(3);
    }
    if ((D_800773B4 & 0x40) == 0 && (D_800773AC & 0x40) != 0) {
        D_80076EA4++;
        func_8002A498(0);
    }
    if ((D_800773B4 & 0x1000) == 0 && (D_800773AC & 0x1000) != 0) {
        func_80071C34(7);
    }
    if ((D_800773B4 & 0x4000) == 0 && (D_800773AC & 0x4000) != 0) {
        func_80071C34(9);
    }
    if ((D_800773B4 & 0x8) == 0 && (D_800773AC & 0x8) != 0) {
        D_80076EB8 = 1 - D_80076EB8;
    }
    if ((D_800773B4 & 0x4) == 0 && (D_800773AC & 0x4) != 0 && D_80076E48 < 0xB) {
        MovieStartStreamRead();
    }

    if (D_80076EB8 == 1) {
        D_80076E9C = func_80028B14();
        if (D_80076E9C != 0) {
            if (D_80076E94 >= 0x801) {
                for (size = 0; size < 0x200; size++) {
                    *D_80076E84++ = D_80076E9C[size];
                }
            } else {
                for (size = 0; size < D_80076E94 / 4; size++) {
                    *D_80076E84++ = D_80076E9C[size];
                }
            }
            D_80076E94 -= 0x800;
            if (D_80076E94 <= 0) {
                if (D_80076E48 == 0xC) {
                    D_80076EB4++;
                    size = D_80076E7C[D_80076EB4].unk0;
                    if (size != 0) {
                        D_80076E94 = ArchiveDecodeAlignedSize(size);
                    }
                    D_80076E84 = (u32*)D_80076E7C[D_80076EB4].unk4;
                }
            }
            func_8002945C(D_80076E9C);
        }
    }
    if (D_80076EB8 == 2) {
        if (func_80028F30(D_80076F7C, &D_80076F80) == 0) {
            func_800294B4(D_80076F80);
        }
    }

    if ((D_800773B4 & 0x2000) == 0 && (D_800773AC & 0x2000) != 0) {
        func_80071C34(0xB);
    }
    if ((D_800773B4 & 0x8000) == 0 && (D_800773AC & 0x8000) != 0) {
        func_80071C34(0xD);
    }
    if (D_80076EBC != 0) {
        value = func_80074AF0() & 0xFF;
        if (value == 0) {
            if (D_80076E48 < 0xB) {
                MovieStartStreamRead();
            }
        }
        if (value - 1 < 0xC) {
            func_80071C34(value);
        }
    }
    if ((D_800773B4 & 0x100) == 0 && (D_800773AC & 0x100) != 0) {
        D_80076EBC = 1 - D_80076EBC;
    }
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800712C4);

void MovieStartStreamRead(void) {
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

// Best attempt (89.02%): the loop, the lerp and the tag swap all come out; the
// gap is that retail spills the &D_80076FBC[i] byte offset to the stack and
// re-forms the address, while gcc here keeps it in a register giv.
/*
void func_80072F98(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 r;
    s32 g;
    s32 b;

    setXY4(pPoly1, x, y, x + w, y, x, y + h, x + w, y + h);

    for (i = 0; i < 4; i++) {
        if (++D_80076FAC[i] > D_80076FBC[i]) {
            D_80076FAC[i] = 0;
            D_80076FBC[i] = (func_80074AF0() & 0xFF) + 0x20;
            D_80076F8C[i].r = D_80076F9C[i].r;
            D_80076F8C[i].g = D_80076F9C[i].g;
            D_80076F8C[i].b = D_80076F9C[i].b;
            D_80076F9C[i].r = (func_80074AF0() & 0xFF) / 32 + 8;
            D_80076F9C[i].g = 8;
            D_80076F9C[i].b = (func_80074AF0() & 0xFF) / 3 + 0x10;
        }

        r = D_80076F8C[i].r + (D_80076F9C[i].r - D_80076F8C[i].r) * D_80076FAC[i] / D_80076FBC[i];
        g = D_80076F8C[i].g + (D_80076F9C[i].g - D_80076F8C[i].g) * D_80076FAC[i] / D_80076FBC[i];
        b = D_80076F8C[i].b + (D_80076F9C[i].b - D_80076F8C[i].b) * D_80076FAC[i] / D_80076FBC[i];

        switch (i) {
        case 0:
            pPoly1->r0 = r;
            pPoly1->g0 = g;
            pPoly1->b0 = b;
            break;
        case 1:
            pPoly1->r1 = r;
            pPoly1->g1 = g;
            pPoly1->b1 = b;
            break;
        case 2:
            pPoly1->r2 = r;
            pPoly1->g2 = g;
            pPoly1->b2 = b;
            break;
        case 3:
            pPoly1->r3 = r;
            pPoly1->g3 = g;
            pPoly1->b3 = b;
            break;
        }
    }

    pPoly1->tag = (pPoly1->tag & 0xFF000000) | (pPoly0->tag & 0xFFFFFF);
    pPoly0->tag = (pPoly0->tag & 0xFF000000) | ((u32)pPoly1 & 0xFFFFFF);
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

// Best attempt (93.37%): the six channel pointers derived from the two .b-channel
// anchors reproduce retail's lui/addiu + addiu -1/-2 prologue and the whole
// instruction stream, but retail rematerialises the three "current" channel loads
// for the final add (it has no register to keep them live) and the allocator here
// picks t2/s8/s5 where retail picks t0/s7/s6, so three loads go missing.
/*
void func_800734B8(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 r;
    s32 g;
    s32 b;
    u_char* pColR;
    u_char* pColG;
    u_char* pColB;
    u_char* pTargetR;
    u_char* pTargetG;
    u_char* pTargetB;

    pColB = D_80076FCE;
    pTargetB = D_80076FDE;
    pColG = pColB - 1;
    pTargetG = pTargetB - 1;
    pColR = pColB - 2;
    pTargetR = pTargetB - 2;

    setXY4(pPoly1, x, y, x + w, y, x, y + h, x + w, y + h);

    for (i = 0; i < 4; i++) {
        if (++D_80076FEC[i] > D_80076FFC[i]) {
            D_80076FEC[i] = 0;
            D_80076FFC[i] = (func_80074AF0() & 0xFF) + 0x20;
            *pColR = *pTargetR;
            *pColG = *pTargetG;
            *pColB = *pTargetB;
            *pTargetR = 0xFF;
            *pTargetG = 0xFF;
            *pTargetB = (func_80074AF0() & 0x3F) - 0x42;
        }

        r = *pColR + (*pTargetR - *pColR) * D_80076FEC[i] / D_80076FFC[i];
        g = *pColG + (*pTargetG - *pColG) * D_80076FEC[i] / D_80076FFC[i];
        b = *pColB + (*pTargetB - *pColB) * D_80076FEC[i] / D_80076FFC[i];

        switch (i) {
        case 0:
            pPoly1->r0 = r;
            pPoly1->g0 = g;
            pPoly1->b0 = b;
            break;
        case 1:
            pPoly1->r1 = r;
            pPoly1->g1 = g;
            pPoly1->b1 = b;
            break;
        case 2:
            pPoly1->r2 = r;
            pPoly1->g2 = g;
            pPoly1->b2 = b;
            break;
        case 3:
            pPoly1->r3 = r;
            pPoly1->g3 = g;
            pPoly1->b3 = b;
            break;
        }

        pColB += 4;
        pTargetB += 4;
        pColG += 4;
        pTargetG += 4;
        pColR += 4;
        pTargetR += 4;
    }

    pPoly1->tag = (pPoly1->tag & 0xFF000000) | (pPoly0->tag & 0xFFFFFF);
    pPoly0->tag = (pPoly0->tag & 0xFF000000) | ((u32)pPoly1 & 0xFFFFFF);
}
*/

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800734B8);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800737EC);

extern int ControllerGetButtonState(int controllerIndex);
extern s32 D_80077118;
extern s32 D_80077394;
extern s32 D_800773B0;
extern s32 D_80076EEC;
extern s32 D_80076EF0;

s32 MovieUpdatePadCursor(s32 arg0, s32 arg1, s32* pOut) {
    D_800773B4 = D_800773AC;
    D_800773AC = ControllerGetButtonState(0);
    if (D_800773B4 == D_800773AC) {
        if (D_800773B4 != 0) {
            D_80076EEC++;
            if (D_80076EF0 < D_80076EEC) {
                D_800773B4 = 0;
                D_80076EF0 = 1;
                D_80076EEC = 0;
            }
        } else {
            D_80076EF0 = 0x10;
            D_80076EEC = 0;
        }
    } else {
        D_80076EF0 = 0x10;
        D_80076EEC = 0;
    }
    if ((D_800773B4 & 0x1000) == 0 && (D_800773AC & 0x1000) != 0) {
        if (--D_80077118 < arg0) {
            D_80077118 = arg1;
        }
    }
    if ((D_800773B4 & 0x4000) == 0 && (D_800773AC & 0x4000) != 0) {
        if (++D_80077118 > arg1) {
            D_80077118 = arg0;
        }
    }
    *pOut = 0;
    if ((D_800773B4 & 0x10) == 0 && (D_800773AC & 0x10) != 0) {
        *pOut = 1;
    }
    if ((D_800773B4 & 0x20) == 0 && (D_800773AC & 0x20) != 0) {
        *pOut = 2;
    }
    if ((D_800773B4 & 0x40) == 0 && (D_800773AC & 0x40) != 0) {
        *pOut = 3;
    }
    if ((D_800773B4 & 0x80) == 0 && (D_800773AC & 0x80) != 0) {
        *pOut = 4;
    }
    if ((D_800773B4 & 0x100) == 0 && (D_800773AC & 0x100) != 0) {
        D_800773B0 = 1 - D_800773B0;
    }
    if ((D_800773B4 & 0x800) == 0 && (D_800773AC & 0x800) != 0) {
        D_80077394 = 1 - D_80077394;
    }
    if ((D_800773B4 & 0x2000) == 0 && (D_800773AC & 0x2000) != 0) {
        return 1;
    }
    if ((D_800773B4 & 0x8000) == 0 && (D_800773AC & 0x8000) != 0) {
        return -1;
    }
    return 0;
}

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
