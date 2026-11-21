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
extern s32 D_8004FDEC;
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

// Best attempt (99.02%): retail's EXACT size (2276 bytes), exact register set
// (ra, s0-s4) and exact FRAME (0x70, mode copied at 0x30, &action at 0x50), so
// its instruction multiset is identical to retail's. Three allocator/scheduling
// ties are left, each one instruction: `li s3,1` sits at the END of the preheader
// where retail has it at the top; the D_8006FC40 format-string `la a0` of the
// stream-status FontPrintf lands just before the jal where retail has it before
// the a1..a3 loads; and the /3600 magic division's `mfhi` goes to t1 instead of
// straight into a2. A 4-minute permuter run (1229 iterations) on this body found
// nothing.
// Load-bearing, each measured:
// - The two UNUSED locals are what reproduce retail's locals area: `SVECTOR sv`
//   (8 bytes, unused, declared first) puts `mode` at 0x30 instead of 0x28, and
//   `u32 spare2[6]` (24 bytes, unused) after `mode` puts `action` at 0x50 instead
//   of 0x30. Anything else of 28 bytes or more (u32[7]) rounds the slot up to 32
//   and the frame becomes 0x78. No instruction references either slot in retail
//   either, so they are dead locals in the original source, not missing code.
// - The loop head uses its OWN variable (pNext). Writing `pWork = &g_MovieDebugDrawBuffers[0]`
//   there lets gcc CSE it with the preheader's base in s1, and the loop then
//   clobbers s1 (lui s1 / addiu s1 / bne v0,s1 ...) where retail re-forms the
//   address into v1: 6 instructions differ. A separate variable gives retail's
//   exact `lui v1` form.
// - `pCounters` is assigned BEFORE `pBase` (retail's s4 copy precedes its s2
//   copy); the reverse source order swaps those two instructions.
// - The teardown's RECT stores go through absolute g_MovieDebugDrawBuffers[n] indices (a
//   pointer local makes them base-relative), the SetDef*Env calls go through
//   pBase (a second variable holding the same address re-materialises it), `i = 0;`
//   comes BEFORE the MovieStreamCommand test with the first 7-iteration loop
//   written `for (; i < 7; i++)` (that keeps the move in the branch delay slot,
//   -4 bytes and the whole function's size), and the teardown passes `i = 0xF0` /
//   `i` (that stops gcc hoisting a sixth loop-invariant copy of 0xF0 into a
//   callee-saved register, +3 instructions when written as plain 0xF0 literals;
//   the value is dead there, the teardown breaks out of the loop).
// Earlier measurements: plain 0xF0 literals 98.33 (2292 bytes), a screenHeight
// local shared by both groups 98.36 (2288), teardown-only screenHeight 98.33;
// gcc 2.6.0 scores the literal body 98.62 and 2.7.2-cdk 82.75. The one tie left in
// this body IS a preset question - see the note below.

// RE-MEASURED under the unit's gcc 2.6.0 preset: 99.04%, retail's exact 2276 bytes.
// The /3600 magic division now lands in a2 as retail has it, and the only rows
// objdump shows are two relocation spellings that link to the same address
// (g_MovieStreamReadCount+0x3C vs retail's D_80076F78, and g_MovieDebugDrawBuffers+0x18 vs
// retail's g_MovieStreamSlots). BUT tools/scripts/check_bin.sh still reports the
// overlay MISMATCH: the stream-status FontPrintf below loads its D_8006FC40 format
// string into a0 LAST, where retail loads it BEFORE the g_MovieStreamBlock /
// WritePtr / BytesLeft arguments. That is a real byte difference, not a reloc
// spelling, so the body stays parked. First lever to try: give the format string
// a local ('char* pFormat = D_8006FC40;') so gcc hoists the address before the
// other three loads.
// MEASURED (round 236, un-parked unchanged): 99.04%, 2276 bytes, and the diff is
// exactly two placement rows, no instruction multiset change. (1) The D_8006FC40
// `la a0`: gcc emits it AFTER the a1..a3 global loads (its normal arg-setup order
// for every other FontPrintf in this function); retail emits it FIRST in the
// .L800708B4 block. Writing it as `char* pFormat = D_8006FC40;` as the block's first
// statement compiles to the IDENTICAL object - gcc folds the single-use constant
// address straight back into the argument, so the local never exists as a def.
// (2) The hoisted constant `li s3,1` (used by `1 - g_MovieDebugBufferParity`, the
// switch's case 1 and the two `isbg = 1` teardown stores) sits AFTER
// `pCounters = g_MovieStreamReadCount; pBase = pWork;` in the preheader where retail
// has it first, i.e. gcc's loop-movable insertion point moved, which no source
// spelling of those two assignments controls. Preset re-screened under
// gcc-2.7.2-cdk-psx: 82.76; every already-matched function of the unit is 100.00
// under the unit preset and 62.96-97.24 under cdk, so 2.6.0 stands.
INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800704E8);

/* 99.04%, 2276 bytes (retail's exact size); see the note above for the last diff
void func_800704E8(void) {
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

    pCounters = g_MovieStreamReadCount;
    pBase = pWork;
    while (1) {
        pNext = &g_MovieDebugDrawBuffers[0];
        if (g_MovieDebugDrawBuffer == pNext) {
            pNext = &g_MovieDebugDrawBuffers[1];
        }
        g_MovieDebugDrawBuffer = pNext;
        g_MovieDebugBufferParity = 1 - g_MovieDebugBufferParity;
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
                   g_ArchiveStreamPositionErrors, D_8004FDEC, g_ArchiveStreamNextSectionId,
                   g_ArchiveStreamCurSectionId);
        FontPrintf(D_8006FBE4, D_80076E68, D_80076E6C, D_80076E70, D_80076E74, D_80076E78);
        FontPrintf(D_8006FC14, g_MovieStreamSections, D_80076E80, D_80076E88, D_80076E8C);

        i = 0;
        if (g_MovieStreamCommand >= 0xB || g_MovieStreamBytesLeft > 0) {
            FontPrintf(D_8006FC40, g_MovieDebugCounters, g_MovieStreamBlock, g_MovieStreamWritePtr,
                       g_MovieStreamBytesLeft);
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
            g_MovieDebugDrawBuffers[0].drawEnv.isbg = 1;
            g_MovieDebugDrawBuffers[1].drawEnv.isbg = 1;
            break;
        }
        g_MovieCursorIndex = cursor;
    }
}
*/

// PARKED at 99.02% (2.7.2): retail's exact size (2276 bytes), frame (0x70, mode at
// 0x30, &action at 0x50) and register set. Every instruction now matches except ONE:
// the /3600 magic division's `mfhi` lands in t1 where retail writes a2 (and so the
// next insn is `addu a2,t1,t0` against retail's `addu a2,a2,t0`). Everything else the
// diff prints is relocation spelling only: the zero loop's `addiu v1,v1,60` over
// g_MovieStreamReadCount (= 0x80076F3C + 0x3C = retail's D_80076F78) and the first
// `drawEnv.isbg = 1` (g_MovieDebugDrawBuffers + 0x18 = retail's g_MovieStreamSlots).
// PRESET_260: gcc 2.6.0 gets this right. Compiled with gcc-2.6.0-psx the whole body
// comes out instruction-identical to retail (`mfhi a2` / `addu a2,a2,t0`), and
// tools/scripts/try_flags.sh movie/main/misc1 --all --gcc gcc-2.6.0-psx scores
// func_800704E8 99.04 and ALL EIGHT already-matched C functions of the unit 100.00,
// so the unit can be switched (2.7.2 expand_divmod uses a temp register for the div
// high part, 2.6.0 writes it into the target). Not source-controllable under 2.7.2:
// 16 hand shapes below (single/multi-statement division, div temp, self-assignment,
// `60 * hours`, `playTime - minutes * 60`, three declaration orders, int vs s32,
// inline-at-call, pointer local) all measure 99.02, and a 10-minute permuter run
// (2991 iterations) is useless here: its base score is 365 for a body objdiff calls
// 99.02%, i.e. dominated by the reloc-name rows above, and its 346 "improvement" is
// that noise (measured 98.97% by objdiff).
/*
void func_800704E8(void) {
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

    pCounters = g_MovieStreamReadCount;
    pBase = pWork;
    while (1) {
        pNext = &g_MovieDebugDrawBuffers[0];
        if (g_MovieDebugDrawBuffer == pNext) {
            pNext = &g_MovieDebugDrawBuffers[1];
        }
        g_MovieDebugDrawBuffer = pNext;
        g_MovieDebugBufferParity = 1 - g_MovieDebugBufferParity;
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
                   g_ArchiveStreamPositionErrors, D_8004FDEC, g_ArchiveStreamNextSectionId,
                   g_ArchiveStreamCurSectionId);
        FontPrintf(D_8006FBE4, D_80076E68, D_80076E6C, D_80076E70, D_80076E74, D_80076E78);
        FontPrintf(D_8006FC14, g_MovieStreamSections, D_80076E80, D_80076E88, D_80076E8C);

        i = 0;
        if (g_MovieStreamCommand >= 0xB || g_MovieStreamBytesLeft > 0) {
            FontPrintf(D_8006FC40, g_MovieDebugCounters, g_MovieStreamBlock, g_MovieStreamWritePtr,
                       g_MovieStreamBytesLeft);
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
            g_MovieDebugDrawBuffers[0].drawEnv.isbg = 1;
            g_MovieDebugDrawBuffers[1].drawEnv.isbg = 1;
            break;
        }
        g_MovieCursorIndex = cursor;
    }
}
*/

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
// prologue pairs: retail forms pColR/pTargetR AFTER the loop's steps-array
// pointer (`lui s3 / addiu s3, g_MovieDarkFadeSteps`) and gcc here forms them before it.
// Everything else, including `i = 0` placed first (which retail also has, and
// which is what made its twin MovieUpdateGlowPoly match), is identical.
// Load-bearing, measured: the three `*(volatile u_char*)pColR` reads of the sums
// are what reproduce retail's RE-READ of the current colour after the three
// divisions (three lbu); the plain `*pColR` form lets gcc CSE the byte and lands
// at 84 bytes short. An explicit `s32* pStep` local (to get the steps pointer
// formed earlier) changes the durations access and lands at 880 bytes / 93.25%.
// MEASURED (round 236, un-parked unchanged): 98.67%, 912 bytes, retail's exact
// multiset; the diff is the two `sw s7 / addiu s7,s5,-2` and `sw s6 / addiu s6,s4,-2`
// pairs, which retail places AFTER the loop's steps-pointer `lui s3 / addiu s3,
// g_MovieDarkFadeSteps` and gcc places after the fourth colour pointer: the steps def is the
// compiler's strength-reduced biv and gcc inserts it at the END of the preheader.
// Re-measured negatives: `if (i == 0) { pColR = ...; pTargetR = ...; }` 98.67% (no
// change - block notes do not move the insertion point); `s32* pStep = g_MovieDarkFadeSteps;`
// with `pStep[i]` 91.72% / 932 bytes (the pointer takes s5 and pColB moves to s3, so
// every prologue row shifts); `s32* pStep` with `*pStep` + `pStep++` 93.34% / 880
// bytes (both arrays become pointer bivs and `i` moves to s1). The biv is not
// reachable from the source, so retail's order would need those two defs to be
// loop-hoisted movables, i.e. the assignments to live INSIDE the loop - which no
// plausible source does. Preset screen: unit preset 98.67, cdk 89.61.
INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072F98);

/* 98.67%, 912 bytes, retail's exact size
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
