#include "common.h"
#include "psyq/libcd.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"

extern void SoundPlayAudioManager(void* pManager, s32 volume, s32 frames);
extern s32 SoundGetPendingSpuCommandType(u32 flags);
extern void* SoundCreateAudioManagerOnHeap(void* pFile);
extern char* ArchiveReadHddFile(char* pFilePath, int* pFileSize);
extern void SoundLoadWdsFile(void* pWdsFile, void* pUnk);
extern void HeapFree(void* pMem);
extern s32 ArchiveDecodeSizeAbsolute(int entryIndex);
extern u32 g_ArchiveTable;
extern int ControllerGetButtonState(int controllerIndex);
extern void SoundSetCdVolumeWithFade(s32 volume, s32 frames);
extern int Vsync(s32 mode);
extern char* ArchiveGetFilePath(int entryIndex);
extern u32 ArchiveGetDebugTable(void);
extern s32 ArchiveDecodeAlignedSize(u32 entryIndex);
extern s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags);
extern void ArchiveCdDataSync(int mode);
extern int PCopen(char* name, int flags, int perms);
extern int PClseek(int fd, int offset, int mode);
extern int PCread(int fd, char* buff, int len);
extern int PCclose(int fd);

// .rodata
extern char g_StrMovieMainSeWd[];
extern char g_StrMovieBatSeWd[];
extern char g_StrMovieGearSeWd[];
extern char g_StrMovieBattle2Wd[];
extern char g_StrMovieBattle2Smd[];

// .bss
extern void* g_MovieAudioManager;
extern MATRIX g_MovieViewMatrix;
extern u32 g_MovieCurrentFrame;
extern s32 g_MovieStopCountdown;
extern s32 g_MovieDrawBufferIndex;
extern s32 g_MoviePrevDrawBufferIndex;
extern s32 g_MovieVerticalOffset;
extern u32 g_MoviePlayMode;
extern s32 g_MoviePadHoldFrames;
extern s32 g_MoviePadHoldLimit;
extern s32 g_MovieViewRoll;
extern SVECTOR g_MovieViewAngle;
// The camera and GTE globals are VECTORs and MATRIXes; retail's object still names the interior
// words of a member access (D_80077044, D_800770D0, ...), so objdiff reads under 100% for
// MovieInitGteState and MovieBuildViewMatrix while the linked bytes are identical - check_bin.sh.
extern MATRIX g_MovieColorMatrix;
extern MATRIX g_MovieLightMatrix;
extern MATRIX g_MovieTranslationMatrix;
extern MATRIX g_MovieRotationMatrix;
extern MATRIX g_MovieRotationBaseMatrix;
extern VECTOR g_MoviePrevViewPosition;
extern VECTOR g_MovieViewPosition;
extern VECTOR g_MovieViewTarget;
extern s32 g_MovieStreamFileIndex;
typedef struct {
    /* 0x0 */ u16 magic;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ u16 unk6;
    /* 0x8 */ volatile u32 frameIndex;
} MovieFileHeader; // Size: 0xC

// The stream slot is the 0x138-byte record that starts 0x18 bytes into each of the
// two draw buffers (g_MovieStreamSlots[0] is g_MovieDebugDrawBuffers[0] + 0x18), so
// its two used fields are that buffer's draw-env auto-clear flag and its disp-env
// RGB24 flag.
typedef struct {
    /* 0x0 */ u8 isbg;
    /* 0x1 */ u8 pad1[0x54];
    /* 0x55 */ u8 isrgb24;
    /* 0x56 */ u8 pad56[0xE2];
} MovieSlot; // Size: 0x138

extern MovieSlot g_MovieStreamSlots[2];
extern s32 g_MovieTotalFrames;
extern s32 g_MoviePadButtons;
extern s32 g_MoviePrevPadButtons;
extern s32 g_MovieLoopPlayback;
extern s32 g_MovieStreamRgb24;
extern RECT g_MovieClearRect; // .rodata
extern s32 g_MovieStreamArchiveSection;
extern s32 D_801D68B4;

// .rodata
extern char g_StrMovieMonitorTitle[];
extern char g_StrMovieMonitorPosition[];
extern char g_StrMovieMonitorRowOffset[];
extern char g_StrMovieMonitorHexByteSpaced[];
extern char g_StrMovieMonitorHexByte[];
extern char g_StrMovieMonitorColon[];
extern char g_StrMovieMonitorChar[];
extern char g_StrMovieMonitorSpace[];
extern char g_StrMovieNewline[];
extern char g_StrMoviePushCircleToMenu[];
extern char D_80070450[];
extern char D_80070464[];
extern char D_8007046C[];
extern char D_80070474[];
extern char D_80070488[];
extern char D_8007049C[];
extern char D_800704AC[];
extern char D_800704BC[];
extern char D_800704C4[];
extern char D_800704C8[];
extern char D_800704D4[];

extern void FontPrintf(char*, ...);
extern void FontDrawLetters(void* pOt);
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern s32 MovieUpdatePadCursor(s32 arg0, s32 arg1, s32* pOut);
// .bss: the CD sector the viewer below reads, and the row it is scrolled to
extern s32 g_MovieDebugSector;
extern s32 g_MovieDebugSectorRow;
extern s32 g_MovieCursorIndex;
extern void HeapDebugDump(u_int mode, u_int startBlockIdx, int endBlockIdx, u_int flags);
extern void func_80072F98(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h);
extern void MovieUpdateGlowPoly(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h);

// The two double-buffered draw environments the debug menu alternates between;
// each one starts 0x18 bytes before the matching MovieSlot entry (g_MovieStreamSlots).
typedef struct {
    /* 0x0  */ DRAWENV drawEnv;
    /* 0x5C */ DISPENV dispEnv;
    /* 0x70 */ u_long ot[0x20];
    /* 0xF0 */ POLY_G4 polyG4[2];
} MovieDrawBuffer; // Size: 0x138

extern MovieDrawBuffer g_MovieDebugDrawBuffers[];
extern MovieDrawBuffer* g_MovieDebugDrawBuffer;
extern s32 g_MovieDebugBufferParity;
extern s32 g_MovieHeapDebugEnabled;
extern s32 g_MoviePadButtons;
extern s32 g_MoviePrevPadButtons;

void MovieClearFrameBuffer(void) {
    RECT rect;

    rect.w = 0x280;
    rect.x = 0;
    rect.y = 0;
    rect.h = 0x1E0;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
}

// func_80074BA4: parked at 96.91%, 1532 bytes (retail 1528, 382 insns - mine is ONE
// instruction over). The body below is retail instruction-for-instruction except for
// the four items listed here; each was measured, not guessed.
//  1. The magic compare 0x160. Retail materialises it INSIDE each scan loop
//     ('lhu v1,0(s1); ori v0,zero,0x160; bne v1,v0'), mine has gcc's loop.c hoist it
//     into a callee-saved register in the loop preheader ('li s5,0x160' / 'li s6,0x160'),
//     which costs the load-delay nop at both sites (+8 bytes). This is the size loss.
//     Tried: the `nScan = nSize + 1; if (magic == 0x160) nScan = ...` order (which is
//     retail's, the addiu sits in the bne delay slot) and the if/else order (which gets
//     the placement right but adds a 'j' over the then-arm, +12). Not tried: anything
//     that makes the constant loop-variant.
//  2. The CD path's second found-check is missing retail's duplicated `sltu` in the bne
//     delay slot: retail's bne targets the fallback's `beqz v0` (with the compare
//     computed in the delay slot) and the other entry re-loads and re-computes at
//     0x50A4; mine targets the re-load block, so no delay-slot copy (-4 bytes).
//  3. s6/s7 are swapped: retail has nBest in s6 and nSectorSize in s7, mine the other
//     way round. Moving nBest's declaration before/after nSectorSize did not change it.
//  4. Retail's scan preheader loads nHeaderSkip BEFORE it forms the buffer address
//     (`lw t0,0x20(sp); addiu v0,sp,0x18`), so reorg cannot fill the blez delay slot;
//     mine forms the address first and it gets stolen (`addiu v0,sp,0x18` in the slot,
//     retail has nop). Same size, different insn.
// Also cosmetic: the CD scan loop reads the header through sp-relative offsets in retail
// ('lhu v1,0x18(sp)') and through s1 in mine.
// The two `MovieFileHeader` edits this needed are kept: unk4 split into u16 unk4/unk6
// (retail reads 0x4 and 0x6 as lhu) and unk8 renamed frameIndex (compared against the
// frame being sought); `frameIndex` is declared volatile because retail re-loads it at
// every two-predecessor join (0x4D64, 0x4E30, 0x4FDC, 0x50A4) and a plain field is CSE'd.
// Next lever worth a try: the permuter from this exact body.
INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80074BA4);
/*
// Returns the stream position of frame nFrame in the file the current archive section
// selects: 0 for the first frame, -1 when the frame is not in the file. Retail looks for
// the frame twice - at a frame-size estimate and at a second one 8/7 of the way along -
// then scans forward from the best candidate, so the whole search appears once for the
// host-file (debug) path and once for the CD path.
s32 func_80074BA4(s32 nFrame) {
    u8 buffer[0x1000];
    MovieFileHeader* pHeader;
    s32 state;
    s32 nFileIndex;
    s32 nBest;
    s32 nSectorSize;
    s32 nHeaderSize;
    s32 nHeaderSkip;
    char* pPath;
    s32 nHandle;
    s32 nCount;
    s32 nFrameSize;
    s32 nSize;
    s32 nScan;
    s32 nUnk4;
    s32 nRead;

    state = g_MovieStreamArchiveSection;
    nBest = 0;
    if (state == 0) {
        ArchiveSetIndex(0x18, 0);
        if (g_MovieStreamFileIndex >= (s16)ArchiveDecodeSizeAbsolute(2)) {
            return;
        }
        nFileIndex = g_MovieStreamFileIndex + 3;
        pPath = ArchiveGetFilePath(nFileIndex);
        nSectorSize = 0x800;
        nHeaderSize = 0x20;
        nHeaderSkip = 0;
    } else if (state == 1) {
        ArchiveSetIndex(0x18, 1);
        if (g_MovieStreamFileIndex >= (s16)ArchiveDecodeSizeAbsolute(1)) {
            return;
        }
        nFileIndex = g_MovieStreamFileIndex + 2;
        pPath = ArchiveGetFilePath(nFileIndex);
        nSectorSize = 0x920;
        nHeaderSize = 0x28;
        nHeaderSkip = 8;
    } else if (state == 2) {
        return -1;
    }
    if (nFrame < 2) {
        return 0;
    }
    if (ArchiveGetDebugTable() != 0) {
        nHandle = PCopen(pPath, 0, 0);
        PClseek(nHandle, 0, 2);
        PClseek(nHandle, 0, 0);
        PCread(nHandle, buffer, nHeaderSize);
        pHeader = (MovieFileHeader*)(buffer + nHeaderSkip);
        nFrameSize = pHeader->unk6;
        nSize = (nFrame - 1) * nFrameSize - (nFrame - 1) / 4;
        PClseek(nHandle, nSize * nSectorSize, 0);
        nRead = PCread(nHandle, buffer, nHeaderSize);
        if (pHeader->frameIndex == nFrame && nRead != 0) {
            goto found_hdd;
        }
        if (pHeader->frameIndex < nFrame && nBest < nSize && nRead != 0) {
            nBest = nSize;
        }
        nSize = (nFrame - 1) * nFrameSize - (nFrame - 1) / 4;
        nSize += nSize / 7;
        PClseek(nHandle, nSize * nSectorSize, 0);
        nRead = PCread(nHandle, buffer, nHeaderSize);
        pHeader = (MovieFileHeader*)(buffer + nHeaderSkip);
        if (pHeader->frameIndex == nFrame && nRead != 0) {
        found_hdd:
            if (pHeader->unk4 != 0) {
                nSize -= pHeader->unk4;
                nScan = nSize - 2;
                goto scan_hdd;
            }
            goto hdd_done;
        } else {
            if (pHeader->frameIndex < nFrame && nBest < nSize && nRead != 0) {
                nBest = nSize;
            }
            nScan = (nFrameSize - 1) * (nFrame - 1);
            if (nBest > 0) {
                nScan = nBest;
            }
        }
    scan_hdd:
        pHeader = (MovieFileHeader*)(buffer + nHeaderSkip);
        for (;;) {
            nSize = nScan;
            PClseek(nHandle, nScan * nSectorSize, 0);
            nRead = PCread(nHandle, buffer, nHeaderSize);
            nScan = nSize + 1;
            if (pHeader->magic == 0x160) {
                nScan = nSize + (pHeader->unk6 - pHeader->unk4);
            }
            if (pHeader->frameIndex == nFrame) {
                break;
            }
            if (nRead <= 0) {
                break;
            }
        }
        if (nRead == 0) {
            nSize = -1;
        }
    hdd_done:
        PCclose(nHandle);
        return nSize;
    } else {
        ArchiveReadFileFromCdSector(ArchiveDecodeSector(nFileIndex), buffer, 0x800, 0, 0);
        ArchiveCdDataSync(0);
        pHeader = (MovieFileHeader*)buffer;
        nFrameSize = pHeader->unk6;
        nCount = (ArchiveDecodeAlignedSize(nFileIndex) + nSectorSize - 1) / nSectorSize;
        nSize = (nFrame - 1) * nFrameSize - (nFrame - 1) / 4;
        ArchiveReadFileFromCdSector(ArchiveDecodeSector(nFileIndex) + nSize, buffer, 0x800, 0, 0);
        ArchiveCdDataSync(0);
        if (pHeader->frameIndex == nFrame && nSize < nCount) {
            nUnk4 = pHeader->unk4;
            goto found_cd;
        }
        if (pHeader->frameIndex < nFrame && nBest < nSize && nSize < nCount) {
            nBest = nSize;
        }
        nSize = (nFrame - 1) * nFrameSize - (nFrame - 1) / 4;
        nSize += nSize / 7;
        ArchiveReadFileFromCdSector(ArchiveDecodeSector(nFileIndex) + nSize, buffer, 0x800, 0, 0);
        ArchiveCdDataSync(0);
        if (pHeader->frameIndex == nFrame && nSize < nCount) {
            nUnk4 = pHeader->unk4;
        found_cd:
            if (nUnk4 != 0) {
                nSize -= nUnk4;
                nScan = nSize - 2;
                goto scan_cd;
            }
            goto cd_done;
        } else {
            if (pHeader->frameIndex < nFrame && nBest < nSize && nSize < nCount) {
                nBest = nSize;
            }
            nScan = (nFrameSize - 1) * (nFrame - 1);
            if (nBest > 0) {
                nScan = nBest;
            }
        }
    scan_cd:
        pHeader = (MovieFileHeader*)buffer;
        do {
            nSize = nScan;
            ArchiveReadFileFromCdSector(ArchiveDecodeSector(nFileIndex) + nSize, buffer, 0x800, 0, 0);
            ArchiveCdDataSync(0);
            nScan = nSize + 1;
            if (pHeader->magic == 0x160) {
                nScan = nSize + (pHeader->unk6 - pHeader->unk4);
            }
        } while (pHeader->frameIndex != nFrame && nScan < nCount);
    cd_done:
        if (nScan >= nCount) {
            nSize = -1;
        }
        return nSize;
    }
}
*/

// Non-void so the size-check early-outs leave v0 = the slt result, which is what
// retail returns on that edge; `return result` needs a li v0,-1 and 4 extra bytes.
s32 MovieCheckStreamFileHeader(void) {
    u8 buffer[0x1000];
    MovieFileHeader* pHeader;
    s32 state;
    s32 nFileIndex;
    s32 nBest;
    s32 nSectorSize;
    s32 nHeaderSize;
    s32 nHeaderSkip;
    char* pPath;
    s32 nHandle;
    s32 nSize;
    s32 result;

    state = g_MovieStreamArchiveSection;
    result = -1;
    if (state == 0) {
        ArchiveSetIndex(0x18, 0);
        if (g_MovieStreamFileIndex >= (s16)ArchiveDecodeSizeAbsolute(2)) {
            return;
        }
        nFileIndex = g_MovieStreamFileIndex + 3;
        pPath = ArchiveGetFilePath(nFileIndex);
        nSectorSize = 0x800;
        nHeaderSize = 0x20;
        nHeaderSkip = 0;
    } else if (state == 1) {
        ArchiveSetIndex(0x18, 1);
        if (g_MovieStreamFileIndex >= (s16)ArchiveDecodeSizeAbsolute(1)) {
            return;
        }
        nFileIndex = g_MovieStreamFileIndex + 2;
        pPath = ArchiveGetFilePath(nFileIndex);
        nSectorSize = 0x920;
        nHeaderSize = 0x28;
        nHeaderSkip = 8;
    } else if (state == 2) {
        return result;
    }

    if (ArchiveGetDebugTable() != 0) {
        nHandle = PCopen(pPath, 0, 0);
        nSize = PClseek(nHandle, 0, 2);
        PClseek(nHandle, 0, 0);
        PClseek(nHandle, nSize - nSectorSize, 0);
        PCread(nHandle, buffer, nHeaderSize);
        pHeader = (MovieFileHeader*)(buffer + nHeaderSkip);
        if (pHeader->magic == 0x160) {
            result = pHeader->frameIndex;
        }
        PCclose(nHandle);
    } else {
        nSize = (ArchiveDecodeAlignedSize(nFileIndex) + nSectorSize - 1) / nSectorSize;
        ArchiveReadFileFromCdSector(ArchiveDecodeSector(nFileIndex) + nSize - 1, buffer, 0x800, 0, 0);
        ArchiveCdDataSync(0);
        pHeader = (MovieFileHeader*)buffer;
        if (pHeader->magic == 0x160) {
            result = pHeader->frameIndex;
        }
    }
    return result;
}

void MovieLoadSoundBanks(void) {
    char* pWdsFile;

    pWdsFile = ArchiveReadHddFile(g_StrMovieMainSeWd, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((SoundGetPendingSpuCommandType(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);

    pWdsFile = ArchiveReadHddFile(g_StrMovieBatSeWd, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((SoundGetPendingSpuCommandType(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);

    pWdsFile = ArchiveReadHddFile(g_StrMovieGearSeWd, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((SoundGetPendingSpuCommandType(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);
}

void MovieCreateAudioManager(void) {
    char* pWdsFile;

    pWdsFile = ArchiveReadHddFile(g_StrMovieBattle2Wd, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    // SoundGetPendingSpuCommandType returns a 16-bit status, tested in the low half
    while ((SoundGetPendingSpuCommandType(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);
    g_MovieAudioManager = SoundCreateAudioManagerOnHeap(ArchiveReadHddFile(g_StrMovieBattle2Smd, 0));
}

void MoviePlayAudioManager(void) {
    SoundPlayAudioManager(g_MovieAudioManager, 0x7F, 0);
}

// Hex/ASCII dump of one CD sector, the "ABSPOS/POS" monitor screen: the pad scrolls a
// line cursor through the 0x800-byte sector buffer (rows of 0x10 bytes, 0xC rows shown)
// and steps the sector by one, by a screenful (0xC rows), by a second (0x4B sectors on a
// 75-sector-per-second disc) or by a minute (0x1194 = 4500 sectors), re-reading the
// sector only when it changed. Leaves with Circle.
void MovieDebugSectorViewer(void) {
    MovieDrawBuffer* pBuffer;
    MovieDrawBuffer* pBase;
    u8* pSectorData;
    u8* pHex;
    u8* pAscii;
    CdlLOC pos;
    s32 action;
    s32 cursor;
    s32 prevSector;
    s32 line;
    s32 i;
    u8 c;

    MovieClearFrameBuffer();
    pSectorData = HeapAlloc(0x800, 0);
    if (pSectorData == NULL) {
        return;
    }

    pBase = &g_MovieDebugDrawBuffers[0];
    SetDefDrawEnv(&pBase[0].drawEnv, 0, 0, 0x280, 0xF0);
    SetDefDispEnv(&pBase[0].dispEnv, 0, 0xF0, 0x280, 0xF0);
    SetDefDrawEnv(&pBase[1].drawEnv, 0, 0xF0, 0x280, 0xF0);
    SetDefDispEnv(&pBase[1].dispEnv, 0, 0, 0x280, 0xF0);
    // the two draw buffers' auto-clear flags and screen rects. Spelled through the
    // array, not through pBase: retail resolves every one of these ten addresses
    // itself (lui $at / sb|sh 0($at)) instead of using the base it already holds in
    // s1, which only the absolute spelling makes gcc do.
    g_MovieDebugDrawBuffers[0].drawEnv.isbg = 1;
    g_MovieDebugDrawBuffers[1].drawEnv.isbg = 1;
    g_MovieDebugDrawBuffers[0].dispEnv.screen.x = 0;
    g_MovieDebugDrawBuffers[0].dispEnv.screen.y = 0xA;
    g_MovieDebugDrawBuffers[0].dispEnv.screen.w = 0x100;
    g_MovieDebugDrawBuffers[0].dispEnv.screen.h = 0xD8;
    g_MovieDebugDrawBuffers[1].dispEnv.screen.x = 0;
    g_MovieDebugDrawBuffers[1].dispEnv.screen.y = 0xA;
    g_MovieDebugDrawBuffers[1].dispEnv.screen.w = 0x100;
    g_MovieDebugDrawBuffers[1].dispEnv.screen.h = 0xD8;
    ArchiveReadFileFromCdSector(g_MovieDebugSector, pSectorData, 0x800, 0, 0);
    ArchiveCdDataSync(0);

    for (;;) {
        pBuffer = &g_MovieDebugDrawBuffers[0];
        if (g_MovieDebugDrawBuffer == pBuffer) {
            pBuffer = &g_MovieDebugDrawBuffers[1];
        }
        g_MovieDebugDrawBuffer = pBuffer;
        g_MovieDebugBufferParity = 1 - g_MovieDebugBufferParity;
        ClearOTagR(pBuffer->ot, 0x20);
        FontPrintf(g_StrMovieMonitorTitle);
        MovieUpdatePadCursor(0, 0, &action);

        prevSector = g_MovieDebugSector;
        if ((g_MoviePrevPadButtons & 0x1000) == 0 && (g_MoviePadButtons & 0x1000) != 0) {
            if (g_MovieDebugSectorRow > 0) {
                g_MovieDebugSectorRow--;
            }
        }
        if ((g_MoviePrevPadButtons & 0x10) == 0 && (g_MoviePadButtons & 0x10) != 0) {
            g_MovieDebugSectorRow -= 0xC;
            if (g_MovieDebugSectorRow < 0) {
                g_MovieDebugSectorRow = 0;
            }
        }
        if ((g_MoviePrevPadButtons & 0x4000) == 0 && (g_MoviePadButtons & 0x4000) != 0) {
            if (g_MovieDebugSectorRow < 0x74) {
                g_MovieDebugSectorRow++;
            }
        }
        if ((g_MoviePrevPadButtons & 0x40) == 0 && (g_MoviePadButtons & 0x40) != 0) {
            g_MovieDebugSectorRow += 0xC;
            if (g_MovieDebugSectorRow >= 0x74) {
                g_MovieDebugSectorRow = 0x74;
            }
        }
        if ((g_MoviePrevPadButtons & 0x8000) == 0 && (g_MoviePadButtons & 0x8000) != 0) {
            if (g_MovieDebugSector > 0) {
                g_MovieDebugSector--;
            }
        }
        if ((g_MoviePrevPadButtons & 0x2000) == 0 && (g_MoviePadButtons & 0x2000) != 0) {
            g_MovieDebugSector++;
        }
        if ((g_MoviePrevPadButtons & 0x4) == 0 && (g_MoviePadButtons & 0x4) != 0) {
            g_MovieDebugSector -= 0x4B;
            if (g_MovieDebugSector < 0) {
                g_MovieDebugSector = 0;
            }
        }
        if ((g_MoviePrevPadButtons & 0x8) == 0 && (g_MoviePadButtons & 0x8) != 0) {
            g_MovieDebugSector += 0x4B;
        }
        if ((g_MoviePrevPadButtons & 0x1) == 0 && (g_MoviePadButtons & 0x1) != 0) {
            g_MovieDebugSector -= 0x1194;
            if (g_MovieDebugSector < 0) {
                g_MovieDebugSector = 0;
            }
        }
        if ((g_MoviePrevPadButtons & 0x2) == 0 && (g_MoviePadButtons & 0x2) != 0) {
            g_MovieDebugSector += 0x1194;
        }
        if (prevSector != g_MovieDebugSector) {
            ArchiveReadFileFromCdSector(g_MovieDebugSector, pSectorData, 0x800, 0, 0);
            ArchiveCdDataSync(0);
        }

        CdIntToPos(g_MovieDebugSector, &pos);
        FontPrintf(g_StrMovieMonitorPosition, g_MovieDebugSector, g_MovieDebugSectorRow * 0x10, pos.minute, pos.second, pos.sector);
        // the row start is walked by the text column, the hex column has its own cursor
        pAscii = pSectorData + g_MovieDebugSectorRow * 0x10;
        pHex = pAscii;
        for (line = 0; line < 0xC; line++) {
            FontPrintf(g_StrMovieMonitorRowOffset, (line + g_MovieDebugSectorRow) * 0x10);
            for (i = 0; i < 0xF; i++) {
                FontPrintf(g_StrMovieMonitorHexByteSpaced, *pHex);
                pHex++;
            }
            FontPrintf(g_StrMovieMonitorHexByte, *pHex);
            pHex++;
            FontPrintf(g_StrMovieMonitorColon);
            for (i = 0; i < 0x10; i++) {
                c = *pAscii;
                if ((u32)(c - 0x20) < 0x5E) {
                    FontPrintf(g_StrMovieMonitorChar, c);
                } else {
                    FontPrintf(g_StrMovieMonitorSpace);
                }
                pAscii++;
            }
            FontPrintf(g_StrMovieNewline);
        }
        FontPrintf(g_StrMoviePushCircleToMenu);
        if (g_MovieHeapDebugEnabled > 0) {
            HeapDebugDump(1, 0, 6, 0x808D);
        }
        FontDrawLetters(g_MovieDebugDrawBuffer->ot);
        func_80072F98((POLY_G4*)g_MovieDebugDrawBuffer->ot, &g_MovieDebugDrawBuffer->polyG4[0], 8, 0x14,
                      0x270, 0xA0);
        MovieUpdateGlowPoly((POLY_G4*)g_MovieDebugDrawBuffer->ot, &g_MovieDebugDrawBuffer->polyG4[1], 7,
                            0x13, 0x272, 0xA2);
        DrawSync(0);
        Vsync(0);
        PutDrawEnv(&g_MovieDebugDrawBuffer->drawEnv);
        PutDispEnv(&g_MovieDebugDrawBuffer->dispEnv);
        DrawOTag(&g_MovieDebugDrawBuffer->ot[0x1F]);
        cursor = g_MovieCursorIndex;
        if (action == 2) {
            break;
        }
        g_MovieCursorIndex = cursor;
    }

    HeapFree(pSectorData);
    MovieClearFrameBuffer();
    pBase = &g_MovieDebugDrawBuffers[0];
    SetDefDrawEnv(&pBase[0].drawEnv, 0, 0, 0x140, 0xF0);
    SetDefDispEnv(&pBase[0].dispEnv, 0, 0xF0, 0x140, 0xF0);
    SetDefDrawEnv(&pBase[1].drawEnv, 0, 0xF0, 0x140, 0xF0);
    SetDefDispEnv(&pBase[1].dispEnv, 0, 0, 0x140, 0xF0);
    g_MovieDebugDrawBuffers[0].drawEnv.isbg = 1;
    g_MovieDebugDrawBuffers[1].drawEnv.isbg = 1;
    g_MovieDebugDrawBuffers[0].dispEnv.screen.x = 0;
    g_MovieDebugDrawBuffers[0].dispEnv.screen.y = 0xA;
    g_MovieDebugDrawBuffers[0].dispEnv.screen.w = 0x100;
    g_MovieDebugDrawBuffers[0].dispEnv.screen.h = 0xD8;
    g_MovieDebugDrawBuffers[1].dispEnv.screen.x = 0;
    g_MovieDebugDrawBuffers[1].dispEnv.screen.y = 0xA;
    g_MovieDebugDrawBuffers[1].dispEnv.screen.w = 0x100;
    g_MovieDebugDrawBuffers[1].dispEnv.screen.h = 0xD8;
}

// One entry of the archive directory the file-size lookup below reads: a 3-byte
// sector index and a 4-byte size, little-endian. The same 0x7-byte record as
// ArchiveEntry in slus_006.64/system/libarchive.c, which owns the type.
typedef struct {
    /* 0x0 */ u8 sector[3];
    /* 0x3 */ u8 size[4];
} ArchiveEntry; // Size: 0x7

int WorldMapSyncGearRideActors(u32 entryIndex) {
    ArchiveEntry* pEntry;
    u32 nOffset;
    int nFileSize;

    nOffset = entryIndex * sizeof(ArchiveEntry);
    pEntry = (ArchiveEntry*)(nOffset + g_ArchiveTable);
    nFileSize = (pEntry->size[3] << 0x18) + (pEntry->size[2] << 0x10) + (pEntry->size[1] << 0x8) + pEntry->size[0];
    return nFileSize;
}

// func_80075D8C: parked at 99.66%, 1236 bytes (retail 1232). ONE instruction over.
// The body below is instruction-for-instruction retail everywhere else, but its
// `j = entry` sits at the top of the inner do-while, so gcc keeps that store in the
// loop HEADER and the back edge re-runs it (this listing is an infinite loop; it is
// not the real source, it is just the only shape found that reproduces the other
// 1232 bytes, including the extra copy reorg puts in the 0x4-bit bnez delay slot).
// Init BEFORE the loop is semantically right and lands that single copy in retail's
// exact place, but gcc 2.6.0 then makes `j + 1` a derived giv: a new saved register
// s1, initialised by TWO `addiu s1,s0,1` at the loop head, `move a0,s1` at all three
// call sites (retail recomputes `addiu a0,s0,1` at each), `addiu s1,s1,1` in the
// tail: 96.43%, 1248 bytes, frame 0x50, eight saves instead of seven.
// So the target is a shape with init before the loop AND no carried `j + 1`.
// Measured, all shapes not flags: for-loop `for (j = entry; j < entry + 0x14; j++)`
// 95.49%, `while (++j < entry + 0x14)` 96.43%, a `next = j + 1` local 96.49%,
// `1 + j` operand order 96.43% (commutative, canonicalised), permuter 6 min / 1805
// iterations from the in-body body (PERMUTER_NONE, base 112 best 112).
// Round 116 re-measured the outside-init family and mapped it exactly: un-parking the
// init before the loop leaves ONE difference, a derived induction variable
// `s1 = j + 1` (`addiu s1,s0,1` twice at the loop head, `move a0,s1` at all three
// j+1 call sites, `addiu s1,s1,1` in the tail), frame 0x50 and s0-s7 saved where
// retail has 0x48 and s0-s6. So the whole problem is stopping gcc 2.6.0's
// combine_givs from deriving j+1, not the loop shape. Also dead: `++j` in the
// condition (identical 96.43%), and the permuter from the OUTSIDE body
// (PERMUTER_NONE, base 1481, no candidate saved). Untried: suppressing the add
// itself (every j+1 spelling tried is still a PLUS of the BIV and a constant).
// Preset is right: -O3 77.96, -O1 91.72, 2.7.2-cdk 82.0, plain 2.7.2 98.5, 2.6.0
// -O2 -G0 (the unit's) 99.66.
// What closed 92.4 -> 99.1 earlier, in order (all source shapes, not flags):
//   1. FontPrintf(g_StrMovieNewline) takes ONE argument (MovieDebugSectorViewer calls it that way);
//      passing pPath cost a `move a1,zero`.
//   2. the hex/dec format is an if/else, not a ternary; cross-jumping then merges the
//      two FontPrintf calls and hoists `move a1,s0` above the branch, as retail does.
//   3. `count = 0x1249;` belongs INSIDE the outer do-while: it is then a
//      loop-invariant set gcc hoists as `li s6,0x1249`, and `count - 0x14` stays the
//      `addiu v0,s6,-20` retail has instead of folding to li 0x1235.
//   4. one variable for the archive path, not two: `pFileName = ArchiveGetFilePath(j+1)`
//      tested and then walked. With a separate pPath gcc tests the return value
//      directly and copies it in the `beqz` delay slot, where retail tests the copy
//      and keeps a nop in the slot (worth 99.14 -> 99.66 with the in-body init).
// Frame/registers are retail's on the in-body variant (0x48, 7 saves, s1=entry,
// s2=hexMode, s3=showName); MovieDrawBuffer and the g_MovieDebugDrawBuffers[] draw buffers (each
// 0x18 bytes before its g_MovieStreamSlots[] MovieSlot) came out of this function.
// Round 201 re-measured the in-body body: 99.66%, 1236 bytes, and the ONLY line that
// differs in the whole 308-instruction stream is one DUPLICATE `addu s0,s1,zero`.
// Ours has the `j = entry` copy in the 0x4-test's bnez delay slot AND at the join;
// retail has only the delay-slot copy because retail's back edge targets .L80075FD0
// (the WorldMapSyncGearRideActors block), so retail's `j = entry` really is OUTSIDE
// the loop - the in-body spelling can never be byte-exact. Retail also recomputes
// `addiu v0,s1,0x14` (entry + 0x14) on every iteration, i.e. retail derives NO givs
// in this loop, while the outside-init body derives `s1 = j + 1`. So the one lever
// left is suppressing combine_givs' j+1 derivation for init-outside; every loop shape
// listed above assumed that derivation was unavoidable.
//
// Round 443 re-parked the HONEST body (init before the loop, so the back edge does not
// re-run it) after re-measuring it from this file: 96.43% / 1248 bytes / 312 insns
// against retail's 308 / 1232, and the whole residual is that one derived induction
// variable - `addiu s1,s0,1` TWICE at the loop head (the second copy in the 0x4 test's
// `beqz` delay slot), `move a0,s1` at the three j+1 call sites where retail recomputes
// `addiu a0,s0,1`, `addiu s1,s1,1` before the latch's `addiu s0,s0,1`, frame 0x50 with
// s0-s7 saved against retail's 0x48 with s0-s6. Re-measured dead this round: the
// for-loop form `for (j = entry; j < entry + 0x14; j++)` 95.49% / 1260 / 315, and the
// honest body under gcc-2.7.2 95.11, so the unit's 2.6.0 -G0 preset stands. The in-body
// body the older note above measures at 99.66% / 1236 is NOT a usable base: it re-inits
// `j` at the loop header (an infinite loop) and is 4 bytes over retail, so it can never
// link. loop.c derives the j+1 giv (and the extra `move s0,s1` duplicate) exactly when
// the biv init sits outside the loop, so this body is the one to iterate on.
// Round 452 re-measured this HONEST body from the file after un-parking: 96.43% / 1248 bytes /
// 312 insns (retail 1232 / 308), residual exactly the derived s1 = j + 1 documented above.
// lever.py now carries this shape as a CLOSED lever (g-derived-giv-only-when-the-biv-init-is-
// outside), so no lane time was spent re-trying spellings; below 99% the lane rules give no
// permuter run, and the two earlier runs (in-body base 112, outside base 1481) were PERMUTER_NONE.
/*
void func_80075D8C(void) {
    s32 alignedIndex;
    s32 remainder;
    s32 action;
    s32 entry;
    s32 j;
    s32 count;
    s32 hexMode;
    s32 showName;
    s32 size;
    char* pFileName;
    char* pCursor;
    MovieDrawBuffer* pBuffer;

    MovieClearFrameBuffer();
    entry = 0;
    ArchiveGetArchiveOffsetIndices(&alignedIndex, &remainder);
    ArchiveSetIndex(0, 0);
    hexMode = 0;
    showName = 0;
    do {
        count = 0x1249;
        pBuffer = &g_MovieDebugDrawBuffers[0];
        if (g_MovieDebugDrawBuffer == pBuffer) {
            pBuffer = &g_MovieDebugDrawBuffers[1];
        }
        g_MovieDebugDrawBuffer = pBuffer;
        g_MovieDebugBufferParity = 1 - g_MovieDebugBufferParity;
        ClearOTagR(pBuffer->ot, 0x20);
        FontPrintf(D_80070450);
        FontPrintf(hexMode ? D_80070464 : D_8007046C);
        MovieUpdatePadCursor(0, 0, &action);
        if ((g_MoviePrevPadButtons & 0x1000) == 0 && (g_MoviePadButtons & 0x1000) != 0) {
            if (entry > 0) {
                entry--;
            }
        }
        if ((g_MoviePrevPadButtons & 0x10) == 0 && (g_MoviePadButtons & 0x10) != 0) {
            entry -= 0x14;
            if (entry < 0) {
                entry = 0;
            }
        }
        if ((g_MoviePrevPadButtons & 0x4000) == 0 && (g_MoviePadButtons & 0x4000) != 0) {
            if (entry < count - 0x14) {
                entry++;
            }
        }
        if ((g_MoviePrevPadButtons & 0x40) == 0 && (g_MoviePadButtons & 0x40) != 0) {
            entry += 0x14;
            if (entry > count - 0x14) {
                entry = count - 0x14;
            }
        }
        if ((g_MoviePrevPadButtons & 0x8) == 0 && (g_MoviePadButtons & 0x8) != 0) {
            hexMode = 1 - hexMode;
        }
        if ((g_MoviePrevPadButtons & 0x4) == 0 && (g_MoviePadButtons & 0x4) != 0) {
            showName = 1 - showName;
        }
        j = entry;
        do {
            if (WorldMapSyncGearRideActors(j) == 0) {
                if (hexMode) {
                    FontPrintf(D_80070474, j);
                } else {
                    FontPrintf(D_80070488, j);
                }
            } else {
                if (hexMode) {
                    FontPrintf(D_8007049C, j, ArchiveDecodeSector(j + 1));
                } else {
                    FontPrintf(D_800704AC, j, ArchiveDecodeSector(j + 1));
                }
                if (showName) {
                    if (WorldMapSyncGearRideActors(j) < 0) {
                        FontPrintf(D_800704BC, -WorldMapSyncGearRideActors(j));
                    } else {
                        pFileName = ArchiveGetFilePath(j + 1);
                        if (pFileName != NULL) {
                            if (*pFileName != '\0') {
                                pCursor = pFileName;
                                while (*pCursor != '\0') {
                                    if (*pCursor == '\\') {
                                        pFileName = pCursor + 1;
                                    }
                                    pCursor++;
                                }
                            }
                            FontPrintf(D_800704C4, pFileName);
                        } else {
                            FontPrintf(g_StrMovieNewline);
                        }
                    }
                } else {
                    if (hexMode) {
                        FontPrintf(D_800704C8, WorldMapSyncGearRideActors(j));
                    } else {
                        FontPrintf(D_800704D4, WorldMapSyncGearRideActors(j));
                    }
                }
            }
            j++;
        } while (j < entry + 0x14);
        FontPrintf(g_StrMoviePushCircleToMenu);
        if (g_MovieHeapDebugEnabled > 0) {
            HeapDebugDump(1, 0, 6, 0x808D);
        }
        FontDrawLetters(g_MovieDebugDrawBuffer->ot);
        func_80072F98((POLY_G4*)g_MovieDebugDrawBuffer->ot, &g_MovieDebugDrawBuffer->polyG4[0], 8, 0x14, 0x130, 0xC0);
        MovieUpdateGlowPoly((POLY_G4*)g_MovieDebugDrawBuffer->ot, &g_MovieDebugDrawBuffer->polyG4[1], 7, 0x13, 0x132, 0xC2);
        DrawSync(0);
        Vsync(0);
        PutDrawEnv(&g_MovieDebugDrawBuffer->drawEnv);
        PutDispEnv(&g_MovieDebugDrawBuffer->dispEnv);
        DrawOTag(&g_MovieDebugDrawBuffer->ot[0x1F]);
    } while (action != 2);
    ArchiveSetIndex(alignedIndex, remainder);
}
*/
INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80075D8C);

void MovieResetPlaybackAndClearScreen(void) {
    long pad[6];
    RECT rect;

    rect = g_MovieClearRect;
    g_MoviePlayMode = 0;
    g_MoviePadButtons = -1;
    if (g_MovieStreamArchiveSection == 0) {
        ArchiveSetIndex(0x18, 0);
        if (g_MovieStreamFileIndex >= (s16)ArchiveDecodeSizeAbsolute(2)) {
            return;
        }
        SetDispMask(0);
    } else if (g_MovieStreamArchiveSection == 1) {
        ArchiveSetIndex(0x18, 1);
        if (g_MovieStreamFileIndex >= (s16)ArchiveDecodeSizeAbsolute(1)) {
            return;
        }
        SetDispMask(0);
    } else if (g_MovieStreamArchiveSection == 2) {
        return;
    }
    g_MovieStreamSlots[0].isbg = 0;
    g_MovieStreamSlots[1].isbg = 0;
    if (g_MovieStreamRgb24 != 0) {
        g_MovieDebugDrawBuffers[0].dispEnv.isrgb24 = 1;
        g_MovieDebugDrawBuffers[1].dispEnv.isrgb24 = 1;
    }
    func_80076488();
    Vsync(0);
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    Vsync(0);
    g_MovieStreamSlots[0].isbg = 1;
    g_MovieStreamSlots[1].isbg = 1;
    if (g_MovieStreamRgb24 != 0) {
        g_MovieDebugDrawBuffers[0].dispEnv.isrgb24 = 0;
        g_MovieDebugDrawBuffers[1].dispEnv.isrgb24 = 0;
    }
}

// non-void: gcc reorg moves the delay-slot fill (li v0,1) differently when the
// function returns void; retail keeps the nop after each branch and the 0x30 frame.
s32 MovieStartStreamWithMode(u8 mode) {
    long pad[6];
    s32 state;

    state = g_MovieStreamArchiveSection;
    g_MoviePlayMode = mode;
    g_MoviePadButtons = -1;
    if (state == 0) {
        ArchiveSetIndex(0x18, 0);
        if (g_MovieStreamFileIndex >= (s16)ArchiveDecodeSizeAbsolute(2)) {
            return;
        }
    } else if (state == 1) {
        ArchiveSetIndex(0x18, 1);
        if (g_MovieStreamFileIndex >= (s16)ArchiveDecodeSizeAbsolute(1)) {
            return;
        }
    } else if (state == 2) {
        return;
    }
    g_MovieDebugDrawBuffers[0].drawEnv.isbg = 0;
    g_MovieDebugDrawBuffers[1].drawEnv.isbg = 0;
    g_MovieDebugDrawBuffers[0].dispEnv.isrgb24 = 1;
    g_MovieDebugDrawBuffers[1].dispEnv.isrgb24 = 1;
    func_80076488();
}

typedef void (*MovieFrameCallback)(u16 frame, s32 arg1, u16 arg2);
extern void func_801D37CC(int, int, int, int, int, int, int, int, int, int, int, int, MovieFrameCallback);
extern void func_801D3538(int, int, int, int, int, int, int);
extern void func_801D43B0(void);
extern void func_801D3F7C(void);
extern void func_801D4318(void);
extern void GameCheckAndHandleSoftReset(void);
extern s32 D_80077020;
extern s32 D_80077398;
extern s32 D_800773A0;
extern s32 D_800773A4;
extern s32 D_800773A8;
extern s32 g_MovieFrameStamps[];
extern s32 D_800773BC[];
extern u8 D_8004FE46;
extern void MovieRequestFrame(u16 arg0, s32 arg1, u16 arg2);

typedef struct {
    /* 0x0   */ u8 unk0[0x90];
    /* 0x90  */ RECT clearRect;
    /* 0x98  */ u8 unk98[0x200];
    /* 0x298 */ RECT moveRect;
} MovieRenderLocals; // Size: 0x2A0

// func_80076488: parked at 82.97%, 1076 bytes (retail 1076, frame 0x308 both).
// Groundwork that is now measured and reusable:
//  - the frame is one 0x2A0 local, declared here as MovieRenderLocals:
//    [0x90 pad][RECT clearRect at 0x90][0x200 pad][RECT moveRect at 0x298], placed
//    at sp+0x38, so clearRect lands on retail's 0xC8 and moveRect on 0x2D0. Nothing
//    else reproduces those two offsets (declaration order does not move them).
//  - Vsync is `int Vsync(int)`, not void: retail uses its return value as the two
//    per-frame timestamps stored to g_MovieFrameStamps/D_800773BC (stride 8, two s32
//    pointers advanced by 2, which is why retail has two separate base registers).
//  - g_MovieVerticalOffset is stored 32-bit and read `lhu` twice; D_800773A0 is `lw` in the
//    (0xF0 - x) / 2 and `lh` as an argument; g_MovieStreamRgb24 is `lhu`.
//  Residual (measured, not guessed): the g_MovieLoopPlayback boolean argument. Retail emits
//  `beqz` + `ori v0,zero,1` with the store duplicated into both branches, and both
//  `lhu` of g_MovieVerticalOffset come from ONE `t0` materialisation. Every spelling tried
//  (flags if/else, inline ternary, `!= 0`) makes gcc compute `sltu v0,zero,v0` and
//  sink the g_MovieLoopPlayback load into the argument setup, which reorders the whole
//  13-argument block. That single argument is what holds this at 82.97%.
//  Round 201 re-measured this body: still 82.97% at retail's exact size (1076 bytes,
//  same frame), and the residual is pinned to that func_801D37CC argument block:
//  retail stores the mode argument (sp+0x14) and the g_MovieLoopPlayback flag
//  (sp+0x18) inside BOTH arms of the branch, and duplicates `a0 = index` and
//  `t0 = &g_MovieVerticalOffset` in both arms too - i.e. the conditional was expanded
//  into per-arm stores of its stack slot, not into a value in a register. The `? 1 : 0`
//  spelling gives `sltu v0,zero,v0` plus one store and slides `D_801D68B4 = 0` into
//  the argument setup. The rest of the body is retail's: the frame-stamp loop differs
//  only in which callee-saved register each local gets, and the second `return 0` needs
//  a `j` where retail reorgs the copy into the branch delay slot.
/*
s32 func_80076488(void) {
    MovieRenderLocals locals;
    s32 state;
    s32 index;
    s32 mode;
    s32 i;
    s32 nFrames;
    s32 nCounter;
    s32 nRate;
    s32 v0;
    s32 v1;
    s32* pTimeA;
    s32* pTimeB;
    MovieDrawBuffer* pDrawBuffer;

    locals.clearRect = g_MovieClearRect;
    state = g_MovieStreamArchiveSection;
    g_MovieStopCountdown = 0;
    D_80077020 = 0;
    mode = 1;
    if (state == 0) {
        mode = 0;
        index = g_MovieStreamFileIndex + 3;
    } else {
        index = g_MovieStreamFileIndex + 2;
    }
    if (ArchiveDecodeSize(index) == 0x18) {
        SetDispMask(1);
        D_8004FE46 = 0;
        return;
    }
    Vsync(0);
    ClearImage(&locals.clearRect, 0, 0, 0);
    DrawSync(0);
    Vsync(0);
    g_MovieDrawBufferIndex = 0;
    g_MoviePrevDrawBufferIndex = 0;
    func_801D43B0();
    if (D_800773A0 > 0) {
        g_MovieVerticalOffset = (0xF0 - D_800773A0) / 2;
    } else {
        g_MovieVerticalOffset = 0;
    }
    func_801D3538(0x140, 0xF0, 0x80, 0x10, 0x20, 0x800, *(u16*)&g_MovieStreamRgb24);
    D_801D68B4 = 0;
    func_801D37CC(index, D_800773A8, *(u16*)&D_800773A4, *(u16*)&g_MovieTotalFrames,
        *(u16*)&D_80077398, mode, g_MovieLoopPlayback ? 1 : 0, 0, *(u16*)&g_MovieVerticalOffset, 0,
        *(u16*)&g_MovieVerticalOffset + 0xF0, *(s16*)&D_800773A0, MovieRequestFrame);
    nRate = 0x1E;
    PutDrawEnv(&g_MovieDebugDrawBuffers[g_MovieDrawBufferIndex].drawEnv);
    pDrawBuffer = g_MovieDebugDrawBuffers;
    PutDispEnv(&g_MovieDebugDrawBuffers[g_MovieDrawBufferIndex].dispEnv);
    SetDispMask(1);
    for (;;) {
        if (D_80077020 == 0) {
            nFrames = nRate / 10;
            pTimeA = g_MovieFrameStamps;
            pTimeB = D_800773BC;
            nCounter = 1;
            for (i = 0; i < nFrames; i++) {
                v0 = Vsync(1);
                func_801D3F7C();
                v1 = Vsync(1);
                if (nCounter < 0x20) {
                    *pTimeA = v0;
                    *pTimeB = v1;
                }
                nCounter += 2;
                pTimeB += 2;
                pTimeA += 2;
            }
            nRate = nRate % 10;
        }
        nRate += 0x1E;
        MovieFadeAudioOnPadPress();
        GameCheckAndHandleSoftReset();
        Vsync(0);
        PutDispEnv(&g_MovieDebugDrawBuffers[g_MoviePrevDrawBufferIndex].dispEnv);
        g_MoviePrevDrawBufferIndex = g_MovieDrawBufferIndex;
        if (g_MovieStopCountdown == 1) {
            break;
        }
        if (g_MovieStopCountdown > 1) {
            g_MovieStopCountdown -= 1;
        }
    }
    func_801D4318();
    if (g_MoviePrevDrawBufferIndex != 0) {
        return 0;
    }
    DrawSync(0);
    Vsync(0);
    locals.moveRect.x = 0;
    locals.moveRect.y = 0xF0;
    locals.moveRect.w = 0x1E0;
    locals.moveRect.h = 0xF0;
    MoveImage(&locals.moveRect, 0, 0);
    DrawSync(0);
    Vsync(0);
    PutDispEnv(&g_MovieDebugDrawBuffers[1].dispEnv);
    return 0;
}
*/
INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076488);

void MovieRequestFrame(u16 arg0, s32 arg1, u16 arg2) {
    g_MovieCurrentFrame = arg0;
    if (D_801D68B4 == 1) {
        if (arg2 != g_MovieVerticalOffset) {
            g_MovieDrawBufferIndex = 0;
        } else {
            g_MovieDrawBufferIndex = 1;
        }
    } else if (arg2 != g_MovieVerticalOffset) {
        g_MovieDrawBufferIndex = 0;
        g_MoviePrevDrawBufferIndex = 0;
    } else {
        g_MovieDrawBufferIndex = 1;
        g_MoviePrevDrawBufferIndex = 1;
    }
    if (arg0 < g_MovieTotalFrames) {
        return;
    }
    if (g_MovieLoopPlayback != 0) {
        return;
    }
    g_MovieStopCountdown = 1;
}

void MovieFadeAudioOnPadPress(void) {
    g_MoviePrevPadButtons = g_MoviePadButtons;
    g_MoviePadButtons = ControllerGetButtonState(0);
    if (g_MoviePrevPadButtons == g_MoviePadButtons) {
        if (g_MoviePrevPadButtons != 0) {
            g_MoviePadHoldFrames++;
            if (g_MoviePadHoldLimit < g_MoviePadHoldFrames) {
                g_MoviePrevPadButtons = 0;
                g_MoviePadHoldLimit = 1;
                g_MoviePadHoldFrames = 0;
            }
        } else {
            g_MoviePadHoldLimit = 0x5A;
            g_MoviePadHoldFrames = 0;
        }
    } else {
        g_MoviePadHoldLimit = 0x5A;
        g_MoviePadHoldFrames = 0;
    }
    if ((g_MoviePrevPadButtons & 0x40) == 0 && (g_MoviePadButtons & 0x40) != 0 && g_MoviePlayMode == 0) {
        SoundSetCdVolumeWithFade(0, 0xA);
        g_MovieStopCountdown = 5;
    }
    if ((g_MoviePrevPadButtons & 0x800) == 0 && (g_MoviePadButtons & 0x800) != 0 && g_MoviePlayMode == 0) {
        SoundSetCdVolumeWithFade(0, 0xA);
        g_MovieStopCountdown = 5;
    }
}

void MovieInitGteState(void) {
    MATRIX* pColorMatrix;

    SetGeomScreen(0x200);
    SetGeomOffset(0xA0, 0x78);
    pColorMatrix = &g_MovieColorMatrix;

    g_MovieTranslationMatrix.m[0][0] = 0x1000;
    g_MovieTranslationMatrix.m[1][1] = 0x1000;
    g_MovieTranslationMatrix.m[2][2] = 0x1000;
    g_MovieRotationBaseMatrix.m[0][0] = 0x1000;
    g_MovieRotationBaseMatrix.m[1][1] = 0x1000;
    g_MovieRotationBaseMatrix.m[2][2] = 0x1000;

    g_MovieLightMatrix.m[0][0] = 0x93D;
    g_MovieLightMatrix.m[0][1] = -0x93D;
    g_MovieLightMatrix.m[0][2] = -0x93D;

    g_MovieTranslationMatrix.m[0][1] = 0;
    g_MovieTranslationMatrix.m[0][2] = 0;
    g_MovieTranslationMatrix.m[1][0] = 0;
    g_MovieTranslationMatrix.m[1][2] = 0;
    g_MovieTranslationMatrix.m[2][0] = 0;
    g_MovieTranslationMatrix.m[2][1] = 0;
    g_MovieRotationBaseMatrix.m[0][1] = 0;
    g_MovieRotationBaseMatrix.m[0][2] = 0;
    g_MovieRotationBaseMatrix.m[1][0] = 0;
    g_MovieRotationBaseMatrix.m[1][2] = 0;
    g_MovieRotationBaseMatrix.m[2][0] = 0;
    g_MovieRotationBaseMatrix.m[2][1] = 0;
    g_MovieLightMatrix.m[1][0] = 0;
    g_MovieLightMatrix.m[1][1] = 0;
    g_MovieLightMatrix.m[1][2] = 0;
    g_MovieLightMatrix.m[2][0] = 0;
    g_MovieLightMatrix.m[2][1] = 0;
    g_MovieLightMatrix.m[2][2] = 0;

    pColorMatrix->m[0][0] = 0x969;
    pColorMatrix->m[0][1] = 0;
    pColorMatrix->m[0][2] = 0;
    pColorMatrix->m[1][0] = 0x969;
    pColorMatrix->m[1][1] = 0;
    pColorMatrix->m[1][2] = 0;
    pColorMatrix->m[2][0] = 0x969;
    pColorMatrix->m[2][1] = 0;
    pColorMatrix->m[2][2] = 0;

    SetColorMatrix(pColorMatrix);
    SetBackColor(0x60, 0x60, 0x60);
}

void WorldMapScriptOp04SetScratchPosition(void) {
    long pad[6];
    MATRIX* pMatrix;

    MovieBuildViewMatrix();
    pMatrix = &g_MovieViewMatrix;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
}

void MovieBuildViewMatrix(void) {
    MATRIX* pRotMatrix;
    s32* pTrans;
    VECTOR* pPos;
    SVECTOR* pAngle;
    s32 dz;
    s32 dx;
    s32 dzSq;
    s32 dxSq;
    s32 dy;

    pPos = &g_MovieViewPosition;
    pAngle = &g_MovieViewAngle;
    dz = pPos->vz - g_MoviePrevViewPosition.vz;
    dzSq = dz * dz;
    dx = pPos->vx - g_MoviePrevViewPosition.vx;
    dxSq = dx * dx;
    dy = pPos->vy - g_MoviePrevViewPosition.vy;
    pAngle->vx = ratan2(dy, SquareRoot0(dzSq + dxSq));
    pAngle->vy = -ratan2(dx, dz);
    pRotMatrix = &g_MovieRotationMatrix;
    RotMatrix(pAngle, pRotMatrix);
    RotMatrixZ(g_MovieViewRoll, pRotMatrix);
    MulMatrix2(&g_MovieRotationBaseMatrix, pRotMatrix);
    pRotMatrix->t[0] = 0;
    pRotMatrix->t[1] = 0;
    pRotMatrix->t[2] = SquareRoot0(dxSq + dy * dy + dzSq);
    pTrans = g_MovieTranslationMatrix.t;
    pTrans[0] = g_MovieViewTarget.vx - pPos->vx;
    pTrans[1] = g_MovieViewTarget.vy - pPos->vy;
    pTrans[2] = g_MovieViewTarget.vz - pPos->vz;
    CompMatrix(pRotMatrix, &g_MovieTranslationMatrix, &g_MovieViewMatrix);
}
