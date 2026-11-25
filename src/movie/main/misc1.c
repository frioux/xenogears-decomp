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
extern s32 g_MovieDarkFadeSteps[];
extern s32 g_MovieDarkFadeDurations[];
typedef struct {
    /* 0x0 */ u_char r;
    /* 0x1 */ u_char g;
    /* 0x2 */ u_char b;
    /* 0x3 */ u_char pad3;
} MovieColour; // Size: 0x4

extern MovieColour g_MovieDarkColours[];
extern MovieColour g_MovieDarkTargetColours[];
extern MovieColour g_MovieGlowColours[];
extern MovieColour g_MovieGlowTargetColours[];
extern s32 g_MovieGlowFadeSteps[];
extern s32 g_MovieGlowFadeDurations[];

// The debug screen's two double-buffered draw environments. Each one starts 0x18
// bytes before the matching MovieSlot entry (g_MovieStreamSlots); the polygon
// primitives are linked into the first words of the ordering table.
typedef struct {
    /* 0x0 */ DRAWENV drawEnv;
    /* 0x5C */ DISPENV dispEnv;
    /* 0x70 */ u_long ot[0x20];
    /* 0xF0 */ POLY_G4 polyG4[2];
} MovieDrawBuffer; // Size: 0x138

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u32 unk4;
} MovieStreamEntry; // Size: 0x8

// Retail copies this three-byte record to the stack once per iteration and never
// reads it back; the copy is part of the original source.
typedef struct {
    /* 0x0 */ s8 unk0;
    /* 0x1 */ s8 unk1;
    /* 0x2 */ s8 unk2;
} MovieDebugMode; // Size: 0x3

extern MovieDrawBuffer g_MovieDebugDrawBuffers[];
extern MovieDrawBuffer* g_MovieDebugDrawBuffer;
extern s32 g_MovieDebugBufferParity;
extern MovieDebugMode D_8006FAF4;
extern s32 D_80076E64;
extern s32 D_80076E68;
extern s32 D_80076E6C;
extern s32 D_80076E70;
extern s32 D_80076E74;
extern s32 D_80076E78;
extern s32 D_80076E80;
extern s32 D_80076E88;
extern s32 D_80076E8C;
extern u32* g_MovieDebugCounters;
extern s32 g_MovieDebugCounterTotal;
extern s32 D_8005A49C;
extern s32 g_ArchiveStreamSectionPositionErrors;
extern u_int g_ArchiveReadRetryCount;
extern u_int g_ArchiveCdDriveState;
extern s32 g_ArchiveFilePositionErrorCount;
extern s32 g_ArchiveStreamPositionErrors;
extern u32 g_ArchiveCdReadStartCount;
extern u32 g_ArchiveCdReadCompleteCount;
extern u32 g_ArchiveCdReadStatusErrorCount;
extern u32 g_ArchiveCdDataSectorCompleteCount;
extern u32 g_ArchiveCdDataSectorErrorCount;
extern s32 g_ArchiveCdReadRetryExhaustedCount;
extern u32 g_ArchiveCdHaltCompleteCount;
extern u32 g_ArchiveCdHaltErrorCount;
extern u16 g_ArchiveStreamNextSectionId;
extern u16 g_ArchiveStreamCurSectionId;
extern void ArchiveChangeStreamingFile(s32 mode);
extern void ArchiveCdDataSync(s32 mode);
extern s32 ArchiveSetIndex(s32 directoryIndex, s32 entryIndex);
extern s32 ArchiveDataSync(void);
extern s32 ArchiveGetCurFileSize(void);
extern void HeapFree(void* pMem);
extern void FontPrintf(char* pFormat, ...);
extern void FontDrawLetters(void* pOt);
extern void HeapDebugDump(u_int mode, u_int startBlockIdx, int endBlockIdx, u_int flags);
extern void MovieClearFrameBuffer(void);

// .rodata
extern char D_8006FAF8[];
extern char D_8006FB08[];
extern char D_8006FB18[];
extern char D_8006FB44[];
extern char D_8006FB50[];
extern char D_8006FB5C[];
extern char D_8006FB68[];
extern char D_8006FBA8[];
extern char D_8006FBE4[];
extern char D_8006FC14[];
extern char D_8006FC40[];
extern char D_8006FC64[];
extern char D_8006FC6C[];
extern char D_8006FC70[];
extern char D_8006FC8C[];
extern char D_8006FC98[];
extern char D_8006FCA4[];
extern char D_8006FCAC[];
extern char D_8006FCB4[];
extern char D_8006FCD8[];

// .bss
extern s32 g_MovieStreamCommand;
extern s32 g_MovieStreamCommandBefore;
extern s32 g_MovieStreamCommandNow;
extern MovieStreamEntry* g_MovieStreamSections;
extern u32* g_MovieStreamWritePtr;
extern s32 g_MovieStreamBytesLeft;
extern u32* g_MovieStreamBlock;
extern s32 g_MovieStreamCancelCount;
extern s32 g_MovieStreamMode;
extern s32 g_MovieRandomMode;
extern s32 g_MoviePadButtons;
extern s32 g_MoviePrevPadButtons;
extern s32 g_MovieCursorIndex;
extern s32 g_MovieHeapDebugEnabled;

// Matching this body leans on a handful of measured spellings:
// - `SVECTOR sv` and `u32 spare2[6]` are dead locals: `sv` (declared first) moves
//   `mode` to 0x30 and `spare2` moves `action` to 0x50, which is retail's frame.
// - The loop head uses its own variable (pNext); reusing pWork there makes gcc CSE
//   it with the preheader base and clobber s1.
// - The three loop-invariant statements below (isbg, pCounters, pBase) are written
//   INSIDE the loop, in that order: loop.c hoists them into retail's preheader as
//   `li s3,1 / la s4 / move s2`, and the hoisted `1` is what the switch's case 1
//   test compares against. Written before the loop, the `li` keeps its source slot
//   and the switch materialises its own constant instead.
// - `i = 0;` belongs inside the MovieStreamCommand block, after the status
//   FontPrintf: with it before the test, cc1 emits that call's format `la a0`
//   after the argument loads instead of before them.
// - The teardown's RECT stores go through absolute g_MovieDebugDrawBuffers[n]
//   indices (a pointer local makes them base-relative), its SetDef*Env calls go
//   through pBase, and it passes `i = 0xF0` / `i` so gcc does not hoist a sixth
//   loop-invariant 0xF0 into a callee-saved register. The value is dead: the
//   teardown breaks out of the loop.
void MovieRunDebugScreen(void) {
    SVECTOR sv;
    MovieDebugMode mode;
    u32 spare2[6];
    MovieDrawBuffer* pWork;
    MovieDrawBuffer* pBase;
    MovieDrawBuffer* pNext;
    u32* pCounters;
    u32* pCounter;
    s32 i;
    s32 action;
    s32 fileSize;
    s32 playTime;
    s32 hours;
    s32 minutes;
    s32 seconds;
    s32 cursor;
    s32 isbg;

    mode = D_8006FAF4;
    ArchiveChangeStreamingFile(0);
    MovieClearFrameBuffer();
    g_MovieReadBuffer = NULL;
    g_MovieStreamMode = 1;
    for (i = 0xF; i >= 0; i--) {
        g_MovieStreamReadCount[i] = 0;
    }

    pWork = &g_MovieDebugDrawBuffers[0];
    SetDefDrawEnv(&pWork->drawEnv, 0, 0, 0x280, 0xF0);
    SetDefDispEnv(&pWork->dispEnv, 0, 0xF0, 0x280, 0xF0);
    SetDefDrawEnv(&pWork[1].drawEnv, 0, 0xF0, 0x280, 0xF0);
    SetDefDispEnv(&pWork[1].dispEnv, 0, 0, 0x280, 0xF0);
    pWork->drawEnv.isbg = 1;
    pWork[1].drawEnv.isbg = 1;
    ArchiveSetIndex(0xC, 3);

    while (1) {
        isbg = 1;
        pCounters = g_MovieStreamReadCount;
        pBase = pWork;
        pNext = &g_MovieDebugDrawBuffers[0];
        if (g_MovieDebugDrawBuffer == pNext) {
            pNext = &g_MovieDebugDrawBuffers[1];
        }
        g_MovieDebugDrawBuffer = pNext;
        g_MovieDebugBufferParity = isbg - g_MovieDebugBufferParity;
        ClearOTagR(pNext->ot, 0x20);
        func_800712C4();
        MovieUpdatePadCursor(0, 0, &action);
        MovieUpdateStream();

        if (g_MovieRandomMode != 0) {
            FontPrintf(D_8006FAF8);
        }
        if (g_MovieStreamMode == 0) {
            FontPrintf(D_8006FB08);
        }
        FontPrintf(D_8006FB18, g_MovieStreamCommand, g_MovieStreamReadErrors, g_MovieVsyncCount,
                   g_ArchiveReadRetryCount, g_ArchiveCdDriveState);
        switch (D_80076E64) {
        case 0:
            FontPrintf(D_8006FB44);
            break;
        case 1:
            FontPrintf(D_8006FB50);
            break;
        case 2:
            FontPrintf(D_8006FB5C);
            break;
        }
        FontPrintf(D_8006FB68, g_ArchiveCdReadStartCount, g_ArchiveCdReadCompleteCount,
                   g_ArchiveCdReadStatusErrorCount, g_ArchiveCdDataSectorCompleteCount,
                   g_ArchiveCdDataSectorErrorCount, D_8005A49C, g_ArchiveCdReadRetryExhaustedCount,
                   g_ArchiveCdHaltCompleteCount, g_ArchiveCdHaltErrorCount);
        fileSize = ArchiveDataSync();
        FontPrintf(D_8006FBA8, fileSize, ArchiveGetCurFileSize(), g_ArchiveFilePositionErrorCount,
                   g_ArchiveStreamPositionErrors, g_ArchiveStreamSectionPositionErrors, g_ArchiveStreamNextSectionId,
                   g_ArchiveStreamCurSectionId);
        FontPrintf(D_8006FBE4, D_80076E68, D_80076E6C, D_80076E70, D_80076E74, D_80076E78);
        FontPrintf(D_8006FC14, g_MovieStreamSections, D_80076E80, D_80076E88, D_80076E8C);

        if (g_MovieStreamCommand >= 0xB || g_MovieStreamBytesLeft > 0) {
            FontPrintf(D_8006FC40, g_MovieDebugCounters, g_MovieStreamBlock, g_MovieStreamWritePtr,
                       g_MovieStreamBytesLeft);
            i = 0;
            for (; i < 7; i++) {
                FontPrintf(D_8006FC64, g_MovieDebugCounters[i]);
            }
            FontPrintf(D_8006FC6C);
            for (i = 0; i < 7; i++) {
                FontPrintf(D_8006FC64, g_MovieDebugCounters[i + 7]);
            }
            FontPrintf(D_8006FC6C);
            for (i = 0; i < 7; i++) {
                FontPrintf(D_8006FC64, g_MovieDebugCounters[i + 14]);
            }
            FontPrintf(D_8006FC6C);
            for (i = 0; i < 7; i++) {
                FontPrintf(D_8006FC64, g_MovieDebugCounters[i + 21]);
            }
            FontPrintf(D_8006FC6C);
            if (g_MovieStreamSections != NULL) {
                FontPrintf(D_8006FC70, g_MovieStreamSections[0].unk4, g_MovieStreamSections[1].unk4,
                           g_MovieStreamSections[2].unk4, g_MovieStreamSections[3].unk4);
            }
        }

        g_MovieDebugCounterTotal = 0;
        FontPrintf(D_8006FC8C, g_MovieStreamCancelCount);
        for (i = 0, pCounter = pCounters; i < 0xD; i++) {
            FontPrintf(D_8006FC98, i, *pCounter);
            g_MovieDebugCounterTotal += *pCounter;
            if (g_MovieStreamCommandNow == i) {
                FontPrintf(D_8006FCA4);
            }
            if (g_MovieStreamCommandBefore == i) {
                FontPrintf(D_8006FCAC);
            }
            FontPrintf(D_8006FC6C);
            pCounter++;
        }

        playTime = g_MoviePlayTimeSeconds;
        hours = playTime / 3600;
        minutes = playTime / 60 - hours * 60;
        seconds = playTime % 60;
        FontPrintf(D_8006FCB4, g_MovieDebugCounterTotal, hours, minutes, seconds);
        FontPrintf(D_8006FCD8);
        if (g_MovieHeapDebugEnabled > 0) {
            HeapDebugDump(1, 0, 6, 0x808D);
        }

        FontDrawLetters(g_MovieDebugDrawBuffer->ot);
        func_80072F98((POLY_G4*)g_MovieDebugDrawBuffer->ot, &g_MovieDebugDrawBuffer->polyG4[0], 8, 0xC, 0x270, 0xD8);
        MovieUpdateGlowPoly((POLY_G4*)g_MovieDebugDrawBuffer->ot, &g_MovieDebugDrawBuffer->polyG4[1], 7, 0xB, 0x272, 0xDA);
        DrawSync(0);
        Vsync(0);
        PutDrawEnv(&g_MovieDebugDrawBuffer->drawEnv);
        PutDispEnv(&g_MovieDebugDrawBuffer->dispEnv);
        DrawOTag(&g_MovieDebugDrawBuffer->ot[0x1F]);
        cursor = g_MovieCursorIndex;
        if ((g_MoviePrevPadButtons & 0x800) == 0 && (g_MoviePadButtons & 0x800) != 0 &&
            g_MovieStreamCommand == 0) {
            g_MovieHeapDebugEnabled = 0;
            ArchiveStopTransfer(0);
            ArchiveCdDataSync(0);
            if (g_MovieReadBuffer != NULL) {
                HeapFree(g_MovieReadBuffer);
            }
            g_MovieReadBuffer = NULL;
            if (g_MovieDebugCounters != NULL) {
                HeapFree(g_MovieDebugCounters);
            }
            g_MovieDebugCounters = NULL;
            MovieClearFrameBuffer();
            SetDefDrawEnv(&pBase->drawEnv, 0, 0, 0x140, 0xF0);
            SetDefDispEnv(&pBase->dispEnv, 0, 0xF0, 0x140, 0xF0);
            SetDefDrawEnv(&pBase[1].drawEnv, 0, 0xF0, 0x140, i = 0xF0);
            SetDefDispEnv(&pBase[1].dispEnv, 0, 0, 0x140, i);
            g_MovieDebugDrawBuffers[0].dispEnv.screen.x = 0;
            g_MovieDebugDrawBuffers[0].dispEnv.screen.y = 0xA;
            g_MovieDebugDrawBuffers[0].dispEnv.screen.w = 0x100;
            g_MovieDebugDrawBuffers[0].dispEnv.screen.h = 0xD8;
            g_MovieDebugDrawBuffers[1].dispEnv.screen.x = 0;
            g_MovieDebugDrawBuffers[1].dispEnv.screen.y = 0xA;
            g_MovieDebugDrawBuffers[1].dispEnv.screen.w = 0x100;
            g_MovieDebugDrawBuffers[1].dispEnv.screen.h = 0xD8;
            g_MovieDebugDrawBuffers[0].drawEnv.isbg = isbg;
            g_MovieDebugDrawBuffers[1].drawEnv.isbg = isbg;
            break;
        }
        g_MovieCursorIndex = cursor;
    }
}

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
extern u8 g_MovieStreamReleaseData[];
extern s32 g_MovieStreamReleaseSection;
extern u32* ArchiveReadStreamSector(void);
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
        g_MovieStreamBlock = ArchiveReadStreamSector();
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
        if (func_80028F30(g_MovieStreamReleaseData, &g_MovieStreamReleaseSection) == 0) {
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

void MovieInitDarkPoly(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 j;
    s32 brightness;

    brightness = 8;

    for (i = 0; i < 4; i++) {
        g_MovieDarkColours[i].r = (MovieGetRandomNumber() & 0xFF) / 32 + 8;
        g_MovieDarkColours[i].g = brightness;
        g_MovieDarkColours[i].b = (MovieGetRandomNumber() & 0xFF) / 3 + 0x10;
        g_MovieDarkTargetColours[i].r = (MovieGetRandomNumber() & 0xFF) / 32 + 8;
        g_MovieDarkTargetColours[i].g = brightness;
        g_MovieDarkTargetColours[i].b = (MovieGetRandomNumber() & 0xFF) / 3 + 0x10;
    }

    for (j = 0; j < 4; j++) {
        g_MovieDarkFadeSteps[j] = 0;
        g_MovieDarkFadeDurations[j] = (MovieGetRandomNumber() & 0xFF) + 0x20;
    }

    SetPolyG4(pPoly0);
    SetSemiTrans(pPoly0, 0);
    setXY4(pPoly0, x, y, x + w, y, x, y + h, x + w, y + h);
    SetPolyG4(pPoly1);
    SetSemiTrans(pPoly1, 0);
    setXY4(pPoly1, x, y, x + w, y, x, y + h, x + w, y + h);
}
// PARKED at 98.67% under the unit's gcc 2.6.0 preset: retail's EXACT size (912
// bytes) and instruction multiset. The only difference is the POSITION of two
// prologue pairs and of one latch increment: retail forms `lui s3 / addiu s3,
// g_MovieDarkFadeSteps` (and `addiu s3,s3,4`) BETWEEN the -1 colour pair and the
// -2 pair, gcc at the end of the preheader / after the last source increment.
// Everything else, including `i = 0` placed first (which retail also has, and
// which is what made its twin MovieUpdateGlowPoly match), is identical.
// Load-bearing, measured: the three `*(volatile u_char*)pColR` reads of the sums
// are what reproduce retail's RE-READ of the current colour after the three
// divisions (three lbu); the plain `*pColR` form lets gcc CSE the byte and lands
// at 84 bytes short. An explicit `s32* pStep` local (to get the steps pointer
// formed earlier) changes the durations access and lands at 880 bytes / 93.25%.
// MEASURED (round 236, un-parked unchanged): 98.67%, 912 bytes, retail's exact
// multiset. Re-measured negatives: `if (i == 0) { pColR = ...; pTargetR = ...; }`
// 98.67% (no change - block notes do not move the insertion point); `s32* pStep =
// g_MovieDarkFadeSteps;` with `pStep[i]` 91.72% / 932 bytes (the pointer takes s5
// and pColB moves to s3, so every prologue row shifts); `s32* pStep` with `*pStep`
// + `pStep++` 93.34% / 880 bytes (both arrays become pointer bivs and `i` moves to
// s1). The biv is not reachable from the source, so retail's order would need
// those two defs to be loop-hoisted movables, i.e. the assignments to live INSIDE
// the loop - which no plausible source does. Preset screen: unit preset 98.67,
// cdk 89.61.
// ROUND 419 (new measurements, no new best): the residual IS source-reachable in
// the TYPED form but not together with the re-reads. Writing all six colour walks
// as `g_MovieDarkColours[i].r/.g/.b` and `g_MovieDarkTargetColours[i].r/.g/.b`
// (no pointer locals) emits retail's EXACT preheader order - colours base at +2,
// target base at +2, then the -1/-1 and -2/-2 derived givs AROUND the steps base,
// and the same order in the latch - but scores 90.47% / 896 bytes / 222 insns
// because gcc CSEs the three sum reads that retail re-reads (the 4 missing
// insns). Making those three reads volatile on the typed form costs the giv
// derivation entirely: 81.67% / 992 bytes / 246 insns. Dropping the two R pointer
// locals and reading `pColB[-2]` / `pTargetB[-2]` instead folds the -2 offsets
// inline as displacements with no derived giv: 95.77% / 904 bytes / 224 insns.
// The hybrid (a source `s32* pStep` walking pointer kept, durations still indexed
// by `i`) hoists the durations base into a register and shrinks the frame to
// 0x48: 93.25% / 880 bytes / 218 insns. So the ordering is only produced by the
// six-derived-giv shape, and that shape cannot also produce the re-reads.
// ROUND 473 (re-measurements, no new best): 98.67% / 912 bytes confirmed, and the
// three negatives above reproduce EXACTLY (typed 90.47 / 896 / 222, typed with
// `*(volatile u_char*)&g_MovieDarkColours[i].r` sums 81.67 / 992 / 246, `pSteps`
// walking pointer with `pSteps++` placed between `pTargetG += 4` and `pColR += 4`
// - i.e. at retail's latch position - 93.34 / 880 / 218). Two facts pinned down:
// (1) the typed form's 4 missing insns are the three sum re-reads PLUS the
// `lw t0,0x18(sp)` reload of the spilled colour.g pointer (the typed form keeps
// colour.g in a register), so no single plain read spelling can close it; (2) a
// source steps pointer makes 2.6.0 hoist the DURATIONS base into a register and
// shift every colour pointer by one, so retail's `lui/addiu` steps group cannot
// be a source statement in this register budget. The park below is the best body.
INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072F98);

/* 98.67%, 912 bytes, retail's exact instruction multiset
void func_80072F98(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
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

    i = 0;
    pColB = &g_MovieDarkColours[0].b;
    pTargetB = &g_MovieDarkTargetColours[0].b;
    pColG = pColB - 1;
    pTargetG = pTargetB - 1;
    pColR = pColB - 2;
    pTargetR = pTargetB - 2;

    setXY4(pPoly1, x, y, x + w, y, x, y + h, x + w, y + h);

    for (; i < 4; i++) {
        if (++g_MovieDarkFadeSteps[i] > g_MovieDarkFadeDurations[i]) {
            g_MovieDarkFadeSteps[i] = 0;
            g_MovieDarkFadeDurations[i] = (MovieGetRandomNumber() & 0xFF) + 0x20;
            *pColR = *pTargetR;
            *pColG = *pTargetG;
            *pColB = *pTargetB;
            *pTargetR = (MovieGetRandomNumber() & 0xFF) / 32 + 8;
            *pTargetG = 8;
            *pTargetB = (MovieGetRandomNumber() & 0xFF) / 3 + 0x10;
        }

        dr = (*pTargetR - *pColR) * g_MovieDarkFadeSteps[i] / g_MovieDarkFadeDurations[i];
        dg = (*pTargetG - *pColG) * g_MovieDarkFadeSteps[i] / g_MovieDarkFadeDurations[i];
        db = (*pTargetB - *pColB) * g_MovieDarkFadeSteps[i] / g_MovieDarkFadeDurations[i];
        r = *(volatile u_char*)pColR + dr;
        g = *(volatile u_char*)pColG + dg;
        b = *(volatile u_char*)pColB + db;

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


void MovieInitGlowPoly(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
    s32 i;
    s32 j;
    s32 color;

    color = 0xFF;

    for (i = 0; i < 4; i++) {
        g_MovieGlowColours[i].r = color;
        g_MovieGlowColours[i].g = color;
        g_MovieGlowColours[i].b = (MovieGetRandomNumber() & 0x3F) - 0x42;
        g_MovieGlowTargetColours[i].r = color;
        g_MovieGlowTargetColours[i].g = color;
        g_MovieGlowTargetColours[i].b = (MovieGetRandomNumber() & 0x3F) - 0x42;
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


void MovieUpdateGlowPoly(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h) {
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

    i = 0;
    pColB = &g_MovieGlowColours[0].b;
    pTargetB = &g_MovieGlowTargetColours[0].b;
    pColG = pColB - 1;
    pTargetG = pTargetB - 1;
    pColR = pColB - 2;
    pTargetR = pTargetB - 2;

    setXY4(pPoly1, x, y, x + w, y, x, y + h, x + w, y + h);

    for (; i < 4; i++) {
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
        r = *(volatile u_char*)pColR + dr;
        g = *(volatile u_char*)pColG + dg;
        b = *(volatile u_char*)pColB + db;

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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800737EC);

extern int ControllerGetButtonState(int controllerIndex);
extern s32 g_MovieCursorIndex;
extern s32 g_MovieHeapDebugEnabled;
extern s32 g_MoviePadToggleFlag;
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
        g_MoviePadToggleFlag = 1 - g_MoviePadToggleFlag;
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
