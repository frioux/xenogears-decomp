#include "common.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"
#include "psyq/libgpu.h"

extern int Vsync(int mode);
extern void ArchiveCdDataSync(int mode);
extern u_char ArchiveCdSetMode(u_char mode);
extern u32 ArchiveGetDebugTable(void);
extern void ArchiveStopTransfer(int mode);
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags);

// .data
extern s32 g_MovieStreamReadErrors;
extern s32 g_MovieVsyncCount;
extern void* g_MovieReadBuffer;
extern s32 g_MovieStreamCommandBefore;
extern s32 g_MovieStreamCommandNow;
extern s32 g_MoviePlayTimeSeconds;
extern s32 g_MoviePlayTimeFrames;
extern s32 g_MovieRandomSeedA;
extern s32 g_MovieRandomSeedB;
extern s32 g_MovieStreamReadCount[];
// .bss
extern u_char g_MovieCdControlResult[];
extern u_char g_MovieGlowColours[];
extern u_char D_80076FCD[];
extern u_char D_80076FCE[];
extern u_char g_MovieGlowTargetColours[];
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
extern s32 g_MovieGlowFadeSteps[];
extern s32 g_MovieGlowFadeDurations[];

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800704E8);

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u32 unk4;
} MovieStreamEntry; // Size: 0x8

extern s32 g_MovieStreamCommand;
extern MovieStreamEntry* g_MovieStreamSections;
extern u32* g_MovieStreamWritePtr;
extern s32 g_MovieStreamBytesLeft;
extern u32* g_MovieStreamBlock;
extern s32 g_MovieStreamCancelCount;
extern s32 g_MovieStreamSectionIndex;
extern s32 g_MovieStreamMode;
extern s32 g_MovieRandomMode;
extern s32 g_MoviePadButtons;
extern s32 g_MoviePrevPadButtons;
extern u8 D_80076F7C[];
extern s32 g_MovieStreamReleaseSection;
extern u32* func_80028B14(void);
extern s32 func_80028F30(u8* pArg0, s32* pArg1);
extern void ArchiveReleaseStreamBlock(u32* pStream);
extern void ArchiveReleaseStreamSection(s32 arg0);
extern s32 ArchiveDecodeAlignedSize(u32 entryIndex);
extern void func_80071C34(s32 arg0);

void MovieUpdateStream(void) {
    s32 size;
    u32 value;

    if ((g_MoviePrevPadButtons & 0x20) == 0 && (g_MoviePadButtons & 0x20) != 0) {
        func_80071C34(1);
    }
    if ((g_MoviePrevPadButtons & 0x10) == 0 && (g_MoviePadButtons & 0x10) != 0) {
        func_80071C34(2);
    }
    if ((g_MoviePrevPadButtons & 0x80) == 0 && (g_MoviePadButtons & 0x80) != 0) {
        func_80071C34(3);
    }
    if ((g_MoviePrevPadButtons & 0x40) == 0 && (g_MoviePadButtons & 0x40) != 0) {
        g_MovieStreamCancelCount++;
        ArchiveStopTransfer(0);
    }
    if ((g_MoviePrevPadButtons & 0x1000) == 0 && (g_MoviePadButtons & 0x1000) != 0) {
        func_80071C34(7);
    }
    if ((g_MoviePrevPadButtons & 0x4000) == 0 && (g_MoviePadButtons & 0x4000) != 0) {
        func_80071C34(9);
    }
    if ((g_MoviePrevPadButtons & 0x8) == 0 && (g_MoviePadButtons & 0x8) != 0) {
        g_MovieStreamMode = 1 - g_MovieStreamMode;
    }
    if ((g_MoviePrevPadButtons & 0x4) == 0 && (g_MoviePadButtons & 0x4) != 0 && g_MovieStreamCommand < 0xB) {
        MovieStartStreamRead();
    }

    if (g_MovieStreamMode == 1) {
        g_MovieStreamBlock = func_80028B14();
        if (g_MovieStreamBlock != 0) {
            if (g_MovieStreamBytesLeft >= 0x801) {
                for (size = 0; size < 0x200; size++) {
                    *g_MovieStreamWritePtr++ = g_MovieStreamBlock[size];
                }
            } else {
                for (size = 0; size < g_MovieStreamBytesLeft / 4; size++) {
                    *g_MovieStreamWritePtr++ = g_MovieStreamBlock[size];
                }
            }
            g_MovieStreamBytesLeft -= 0x800;
            if (g_MovieStreamBytesLeft <= 0) {
                if (g_MovieStreamCommand == 0xC) {
                    g_MovieStreamSectionIndex++;
                    size = g_MovieStreamSections[g_MovieStreamSectionIndex].unk0;
                    if (size != 0) {
                        g_MovieStreamBytesLeft = ArchiveDecodeAlignedSize(size);
                    }
                    g_MovieStreamWritePtr = (u32*)g_MovieStreamSections[g_MovieStreamSectionIndex].unk4;
                }
            }
            ArchiveReleaseStreamBlock(g_MovieStreamBlock);
        }
    }
    if (g_MovieStreamMode == 2) {
        if (func_80028F30(D_80076F7C, &g_MovieStreamReleaseSection) == 0) {
            ArchiveReleaseStreamSection(g_MovieStreamReleaseSection);
        }
    }

    if ((g_MoviePrevPadButtons & 0x2000) == 0 && (g_MoviePadButtons & 0x2000) != 0) {
        func_80071C34(0xB);
    }
    if ((g_MoviePrevPadButtons & 0x8000) == 0 && (g_MoviePadButtons & 0x8000) != 0) {
        func_80071C34(0xD);
    }
    if (g_MovieRandomMode != 0) {
        value = MovieGetRandomNumber() & 0xFF;
        if (value == 0) {
            if (g_MovieStreamCommand < 0xB) {
                MovieStartStreamRead();
            }
        }
        if (value - 1 < 0xC) {
            func_80071C34(value);
        }
    }
    if ((g_MoviePrevPadButtons & 0x100) == 0 && (g_MoviePadButtons & 0x100) != 0) {
        g_MovieRandomMode = 1 - g_MovieRandomMode;
    }
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800712C4);

void MovieStartStreamRead(void) {
    if (g_MovieReadBuffer == NULL) {
        g_MovieReadBuffer = HeapAlloc(0x2000, 0);
    }
    if (g_MovieReadBuffer != NULL) {
        ArchiveReadFileFromCdSector(0x40, g_MovieReadBuffer, 0x2000, 0, 0);
    }
    g_MovieStreamReadCount[0]++;
    if (g_MovieStreamReadErrors == 0) {
        g_MovieStreamCommandBefore = g_MovieStreamCommandNow;
        g_MovieStreamCommandNow = 0;
    }
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80071C34);

void MovieUpdateFrameCounter(void) {
    g_MovieVsyncCount++;
    if (++g_MoviePlayTimeFrames >= 0x3C) {
        g_MoviePlayTimeFrames = 0;
        g_MoviePlayTimeSeconds++;
    }
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072480);

void MoviePauseCdDrive(void) {
    if (ArchiveGetDebugTable() == 0) {
        ArchiveStopTransfer(0);
        ArchiveCdDataSync(0);
        ArchiveCdSetMode(0);
        ArchiveCdDataSync(0);
        Vsync(3);
        while (CdControlB(8, 0, g_MovieCdControlResult) == 0) {
        }
    }
}

void MovieReadPcFile(char* pName, char* pBuffer, int len) {
    int fd;

    fd = PCopen(pName, 0, 0);
    PCread(fd, pBuffer, len);
    PCclose(fd);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072A08);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072D84);

// Best attempt (99.40%): byte count, frame and every instruction except the
// destination register of the two div-by-3 sign fixups. Retail keeps the quotient
// in v1 (mfhi v1 / subu v1,v1,v0 / addiu v1,v1,0x10 / sb v1); gcc 2.7.2 puts it in
// v0, the dying dividend/sign register (subu v0,v1,v0). gcc only writes the fixup
// into v1 when v0 is still LIVE across the subu - proved by keeping the masked
// random byte in a local that a later statement reads, which flips the first site
// to v1 - and no plausible source of this function keeps it live. Eight source
// shapes (shared temp, per-site temp, quotient local + separate add, u8 temp,
// dividend local, reversed operand order, MovieColour pointer locals) and two
// permuter runs (2376 iterations) all land here.
/*
void func_80072D84(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 j;
    s32 brightness;

    brightness = 8;

    for (i = 0; i < 4; i++) {
        D_80076F8C[i].r = (MovieGetRandomNumber() & 0xFF) / 32 + 8;
        D_80076F8C[i].g = brightness;
        D_80076F8C[i].b = (MovieGetRandomNumber() & 0xFF) / 3 + 0x10;
        D_80076F9C[i].r = (MovieGetRandomNumber() & 0xFF) / 32 + 8;
        D_80076F9C[i].g = brightness;
        D_80076F9C[i].b = (MovieGetRandomNumber() & 0xFF) / 3 + 0x10;
    }

    for (j = 0; j < 4; j++) {
        D_80076FAC[j] = 0;
        D_80076FBC[j] = (MovieGetRandomNumber() & 0xFF) + 0x20;
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

void MovieInitGlowPoly(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 j;
    s32 color;

    color = 0xFF;

    // The colour triplets are 4-byte structs and splat labels every byte, so the
    // three channels are addressed as byte arrays a stride of 4 apart.
    for (i = 0; i < 0x10; i += 4) {
        g_MovieGlowColours[i] = color;
        D_80076FCD[i] = color;
        D_80076FCE[i] = (MovieGetRandomNumber() & 0x3F) - 0x42;
        g_MovieGlowTargetColours[i] = color;
        D_80076FDD[i] = color;
        D_80076FDE[i] = (MovieGetRandomNumber() & 0x3F) - 0x42;
    }

    for (j = 0; j < 4; j++) {
        g_MovieGlowFadeSteps[j] = 0;
        g_MovieGlowFadeDurations[j] = (MovieGetRandomNumber() & 0xFF) + 0x20;
    }

    SetPolyG4(pPoly0);
    setXY4(pPoly0, x, y, x + w, y, x, y + h, x + w, y + h);
    SetPolyG4(pPoly1);
    setXY4(pPoly1, x, y, x + w, y, x, y + h, x + w, y + h);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800734B8);

// Best attempt (93.54%, 804 bytes against retail's 820): retail's instruction
// multiset plus four more reads. Retail keeps the six channel pointers in
// s5/s4/t0/s8/s7/s6 with the pColG pointer (D_80076FCD) in caller-saved t0,
// saved and restored around the three calls, and pPoly0 on the stack at 0x10(sp);
// gcc here picks s7/s4/t2/s6/s8/s5 with the same one-caller-saved shape. Those
// two allocation differences also change CSE: retail re-reads *pColR/*pColG/
// *pColB after the three divisions (three extra lbu) instead of keeping the bytes
// loaded for the deltas. Six declaration orders of the pointer locals all compile
// identically; the combined single-expression form reads 93.37% at the same size,
// and a second pointer to the same address (forced reloads) reads 94.34% at 812
// bytes. The permuter's best was the same 812-byte form.
/*
void func_800734B8(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 r;
    s32 g;
    s32 b;
    s32 dr;
    s32 dg;
    s32 db;
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
        if (++g_MovieGlowFadeSteps[i] > g_MovieGlowFadeDurations[i]) {
            g_MovieGlowFadeSteps[i] = 0;
            g_MovieGlowFadeDurations[i] = (MovieGetRandomNumber() & 0xFF) + 0x20;
            *pColR = *pTargetR;
            *pColG = *pTargetG;
            *pColB = *pTargetB;
            *pTargetR = 0xFF;
            *pTargetG = 0xFF;
            *pTargetB = (MovieGetRandomNumber() & 0x3F) - 0x42;
        }

        dr = (*pTargetR - *pColR) * g_MovieGlowFadeSteps[i] / g_MovieGlowFadeDurations[i];
        dg = (*pTargetG - *pColG) * g_MovieGlowFadeSteps[i] / g_MovieGlowFadeDurations[i];
        db = (*pTargetB - *pColB) * g_MovieGlowFadeSteps[i] / g_MovieGlowFadeDurations[i];
        r = *pColR + dr;
        g = *pColG + dg;
        b = *pColB + db;

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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800737EC);

extern int ControllerGetButtonState(int controllerIndex);
extern s32 g_MovieCursorIndex;
extern s32 g_MovieHeapDebugEnabled;
extern s32 D_800773B0;
extern s32 g_MoviePadRepeatCount;
extern s32 g_MoviePadRepeatDelay;

s32 MovieUpdatePadCursor(s32 arg0, s32 arg1, s32* pOut) {
    g_MoviePrevPadButtons = g_MoviePadButtons;
    g_MoviePadButtons = ControllerGetButtonState(0);
    if (g_MoviePrevPadButtons == g_MoviePadButtons) {
        if (g_MoviePrevPadButtons != 0) {
            g_MoviePadRepeatCount++;
            if (g_MoviePadRepeatDelay < g_MoviePadRepeatCount) {
                g_MoviePrevPadButtons = 0;
                g_MoviePadRepeatDelay = 1;
                g_MoviePadRepeatCount = 0;
            }
        } else {
            g_MoviePadRepeatDelay = 0x10;
            g_MoviePadRepeatCount = 0;
        }
    } else {
        g_MoviePadRepeatDelay = 0x10;
        g_MoviePadRepeatCount = 0;
    }
    if ((g_MoviePrevPadButtons & 0x1000) == 0 && (g_MoviePadButtons & 0x1000) != 0) {
        if (--g_MovieCursorIndex < arg0) {
            g_MovieCursorIndex = arg1;
        }
    }
    if ((g_MoviePrevPadButtons & 0x4000) == 0 && (g_MoviePadButtons & 0x4000) != 0) {
        if (++g_MovieCursorIndex > arg1) {
            g_MovieCursorIndex = arg0;
        }
    }
    *pOut = 0;
    if ((g_MoviePrevPadButtons & 0x10) == 0 && (g_MoviePadButtons & 0x10) != 0) {
        *pOut = 1;
    }
    if ((g_MoviePrevPadButtons & 0x20) == 0 && (g_MoviePadButtons & 0x20) != 0) {
        *pOut = 2;
    }
    if ((g_MoviePrevPadButtons & 0x40) == 0 && (g_MoviePadButtons & 0x40) != 0) {
        *pOut = 3;
    }
    if ((g_MoviePrevPadButtons & 0x80) == 0 && (g_MoviePadButtons & 0x80) != 0) {
        *pOut = 4;
    }
    if ((g_MoviePrevPadButtons & 0x100) == 0 && (g_MoviePadButtons & 0x100) != 0) {
        D_800773B0 = 1 - D_800773B0;
    }
    if ((g_MoviePrevPadButtons & 0x800) == 0 && (g_MoviePadButtons & 0x800) != 0) {
        g_MovieHeapDebugEnabled = 1 - g_MovieHeapDebugEnabled;
    }
    if ((g_MoviePrevPadButtons & 0x2000) == 0 && (g_MoviePadButtons & 0x2000) != 0) {
        return 1;
    }
    if ((g_MoviePrevPadButtons & 0x8000) == 0 && (g_MoviePadButtons & 0x8000) != 0) {
        return -1;
    }
    return 0;
}

s32 MovieGetRandomNumber(void) {
    s32 a;
    s32 b;
    s32 dr;
    s32 dg;
    s32 db;
    s32 r;

    a = g_MovieRandomSeedA * 5 + 1;
    b = g_MovieRandomSeedB * 7 + 3;
    g_MovieRandomSeedA = a;
    g_MovieRandomSeedB = b;
    r = (a ^ b) + 1;
    g_MovieRandomSeedA = r;
    if (r < 0) {
        g_MovieRandomSeedA = -r;
    }
    return g_MovieRandomSeedA;
}
