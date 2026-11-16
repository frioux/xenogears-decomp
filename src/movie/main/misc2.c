#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"

#define ARCHIVE_HEADER_ENTRY_SIZE 0x7

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
extern s32 D_80076F30;
extern s32 D_80076F34;
extern s32 D_80077030;
extern s32 D_80077034;
extern s32 D_80077040;
extern s32 D_80077044;
extern s32 g_MovieViewRoll;
extern SVECTOR g_MovieViewAngle;
extern MATRIX g_MovieColorMatrix;
extern s16 D_80077072;
extern s16 D_80077074;
extern s16 D_80077076;
extern s16 D_80077078;
extern s16 D_8007707A;
extern s16 D_8007707C;
extern s16 D_8007707E;
extern s16 D_80077080;
extern s16 g_MovieLightMatrix;
extern s16 D_80077092;
extern s16 D_80077094;
extern s16 D_80077096;
extern s16 D_80077098;
extern s16 D_8007709A;
extern s16 D_8007709C;
extern s16 D_8007709E;
extern s16 D_800770A0;
extern MATRIX g_MovieTranslationMatrix;
extern s16 D_800770BA;
extern s16 D_800770BC;
extern s16 D_800770BE;
extern s16 D_800770C0;
extern s16 D_800770C2;
extern s16 D_800770C4;
extern s16 D_800770C6;
extern s16 D_800770C8;
extern s32 D_800770CC;
extern s32 D_800770D0;
extern s32 D_800770D4;
extern MATRIX g_MovieRotationMatrix;
extern s32 D_800770EC;
extern s32 D_800770F0;
extern s32 D_800770F4;
extern s16 g_MovieRotationBaseMatrix;
extern s16 D_800770FA;
extern s16 D_800770FC;
extern s16 D_800770FE;
extern s16 D_80077100;
extern s16 D_80077102;
extern s16 D_80077104;
extern s16 D_80077106;
extern s16 D_80077108;
extern s32 g_MovieStreamFileIndex;
typedef struct {
    /* 0x0 */ u16 magic;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
} MovieFileHeader; // Size: 0xC

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} MovieVector; // Size: 0xC

extern MovieVector g_MoviePrevViewPosition;
extern MovieVector g_MovieViewPosition;
extern MovieVector g_MovieViewTarget;

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 pad1[0x54];
    /* 0x55 */ u8 unk55;
    /* 0x56 */ u8 pad56[0xE2];
} MovieSlot; // Size: 0x138

extern MovieSlot D_8007713C[2];
extern s32 g_MovieTotalFrames;
extern s32 g_MoviePadButtons;
extern s32 g_MoviePrevPadButtons;
extern s32 g_MovieLoopPlayback;
extern s32 g_MovieStreamRgb24;
extern RECT g_MovieClearRect; // .rodata
extern s32 g_MovieStreamArchiveSection;
extern s32 D_801D68B4;

// .rodata
extern char D_8007042C[];
extern char D_80070430[];
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
extern void HeapDebugDump(u_int mode, u_int startBlockIdx, int endBlockIdx, u_int flags);
extern void func_80072F98(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h);
extern void func_800734B8(POLY_G4* pPoly0, POLY_G4* pPoly1, s32 x, s32 y, s32 w, s32 h);

// The two double-buffered draw environments the debug menu alternates between;
// each one starts 0x18 bytes before the matching MovieSlot entry (D_8007713C).
typedef struct {
    /* 0x0  */ DRAWENV drawEnv;
    /* 0x5C */ DISPENV dispEnv;
    /* 0x70 */ u_long ot[0x20];
    /* 0xF0 */ POLY_G4 polyG4[2];
} MovieDrawBuffer; // Size: 0x138

extern MovieDrawBuffer D_80077124[];
extern MovieDrawBuffer* D_80077120;
extern s32 D_8007744C;
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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80074BA4);

// Non-void so the size-check early-outs leave v0 = the slt result, which is what
// retail returns on that edge; `return result` needs a li v0,-1 and 4 extra bytes.
s32 MovieCheckStreamFileHeader(void) {
    u8 buffer[0x1000];
    MovieFileHeader* pHeader;
    s32 state;
    s32 nFileIndex;
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
            result = pHeader->unk8;
        }
        PCclose(nHandle);
    } else {
        nSize = (ArchiveDecodeAlignedSize(nFileIndex) + nSectorSize - 1) / nSectorSize;
        ArchiveReadFileFromCdSector(ArchiveDecodeSector(nFileIndex) + nSize - 1, buffer, 0x800, 0, 0);
        ArchiveCdDataSync(0);
        pHeader = (MovieFileHeader*)buffer;
        if (pHeader->magic == 0x160) {
            result = pHeader->unk8;
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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80075534);

int WorldMapSyncGearRideActors(u32 entryIndex) {
    u8* pArchiveEntry;
    u32 nOffset;
    int nFileSize;

    nOffset = entryIndex * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (pArchiveEntry[6] << 0x18) + (pArchiveEntry[5] << 0x10) + (pArchiveEntry[4] << 0x8) + pArchiveEntry[3];
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
//   1. FontPrintf(D_8007042C) takes ONE argument (func_80075534 calls it that way);
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
// s2=hexMode, s3=showName); MovieDrawBuffer and the D_80077124[] draw buffers (each
// 0x18 bytes before its D_8007713C[] MovieSlot) came out of this function.
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
        pBuffer = &D_80077124[0];
        if (D_80077120 == pBuffer) {
            pBuffer = &D_80077124[1];
        }
        D_80077120 = pBuffer;
        D_8007744C = 1 - D_8007744C;
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
        do {
            j = entry;
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
                            FontPrintf(D_8007042C);
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
        FontPrintf(D_80070430);
        if (g_MovieHeapDebugEnabled > 0) {
            HeapDebugDump(1, 0, 6, 0x808D);
        }
        FontDrawLetters(D_80077120->ot);
        func_80072F98((POLY_G4*)D_80077120->ot, &D_80077120->polyG4[0], 8, 0x14, 0x130, 0xC0);
        func_800734B8((POLY_G4*)D_80077120->ot, &D_80077120->polyG4[1], 7, 0x13, 0x132, 0xC2);
        DrawSync(0);
        Vsync(0);
        PutDrawEnv(&D_80077120->drawEnv);
        PutDispEnv(&D_80077120->dispEnv);
        DrawOTag(&D_80077120->ot[0x1F]);
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
    D_8007713C[0].unk0 = 0;
    D_8007713C[1].unk0 = 0;
    if (g_MovieStreamRgb24 != 0) {
        D_8007713C[0].unk55 = 1;
        D_8007713C[1].unk55 = 1;
    }
    func_80076488();
    Vsync(0);
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    Vsync(0);
    D_8007713C[0].unk0 = 1;
    D_8007713C[1].unk0 = 1;
    if (g_MovieStreamRgb24 != 0) {
        D_8007713C[0].unk55 = 0;
        D_8007713C[1].unk55 = 0;
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
    D_8007713C[0].unk0 = 0;
    D_8007713C[1].unk0 = 0;
    D_8007713C[0].unk55 = 1;
    D_8007713C[1].unk55 = 1;
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
    PutDrawEnv(&D_80077124[g_MovieDrawBufferIndex].drawEnv);
    pDrawBuffer = D_80077124;
    PutDispEnv(&D_80077124[g_MovieDrawBufferIndex].dispEnv);
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
        PutDispEnv(&D_80077124[g_MoviePrevDrawBufferIndex].dispEnv);
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
    PutDispEnv(&D_80077124[1].dispEnv);
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
    D_800770C0 = 0x1000;
    D_800770C8 = 0x1000;
    g_MovieRotationBaseMatrix = 0x1000;
    D_80077100 = 0x1000;
    D_80077108 = 0x1000;

    g_MovieLightMatrix = 0x93D;
    D_80077092 = -0x93D;
    D_80077094 = -0x93D;

    D_800770BA = 0;
    D_800770BC = 0;
    D_800770BE = 0;
    D_800770C2 = 0;
    D_800770C4 = 0;
    D_800770C6 = 0;
    D_800770FA = 0;
    D_800770FC = 0;
    D_800770FE = 0;
    D_80077102 = 0;
    D_80077104 = 0;
    D_80077106 = 0;
    D_80077096 = 0;
    D_80077098 = 0;
    D_8007709A = 0;
    D_8007709C = 0;
    D_8007709E = 0;
    D_800770A0 = 0;

    pColorMatrix->m[0][0] = 0x969;
    D_80077072 = 0;
    D_80077074 = 0;
    D_80077076 = 0x969;
    D_80077078 = 0;
    D_8007707A = 0;
    D_8007707C = 0x969;
    D_8007707E = 0;
    D_80077080 = 0;

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

// objdiff reads 99.33% on this one, not 100: the t[] stores name g_MovieRotationMatrix+24 where
// retail's object names D_800770F0 - same bytes, same addend (see AGENTS.md).
void MovieBuildViewMatrix(void) {
    MATRIX* pRotMatrix;
    s32* pTrans;
    MovieVector* pPos;
    SVECTOR* pAngle;
    s32 dz;
    s32 dx;
    s32 dzSq;
    s32 dxSq;
    s32 dy;

    pPos = &g_MovieViewPosition;
    pAngle = &g_MovieViewAngle;
    dz = D_80077044 - D_80077034;
    dzSq = dz * dz;
    dx = pPos->x - g_MoviePrevViewPosition.x;
    dxSq = dx * dx;
    dy = D_80077040 - D_80077030;
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
    pTrans[0] = g_MovieViewTarget.x - pPos->x;
    pTrans[1] = D_80076F30 - D_80077040;
    pTrans[2] = D_80076F34 - D_80077044;
    CompMatrix(pRotMatrix, &g_MovieTranslationMatrix, &g_MovieViewMatrix);
}
