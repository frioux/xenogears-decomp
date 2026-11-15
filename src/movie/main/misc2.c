#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"

#define ARCHIVE_HEADER_ENTRY_SIZE 0x7

extern void SoundPlayAudioManager(void* pManager, s32 volume, s32 frames);
extern s32 func_8003BDFC(u32 flags);
extern void* SoundCreateAudioManagerOnHeap(void* pFile);
extern char* ArchiveReadHddFile(char* pFilePath, int* pFileSize);
extern void SoundLoadWdsFile(void* pWdsFile, void* pUnk);
extern void HeapFree(void* pMem);
extern s32 ArchiveDecodeSizeAbsolute(int entryIndex);
extern u32 g_ArchiveTable;
extern int ControllerGetButtonState(int controllerIndex);
extern void SoundSetCdVolumeWithFade(s32 volume, s32 frames);
extern void Vsync(s32 mode);
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
extern char D_80070304[];
extern char D_80070328[];
extern char D_8007034C[];
extern char D_80070370[];
extern char D_80070394[];

// .bss
extern void* D_8007700C;
extern MATRIX D_80077050;
extern u32 D_80077010;
extern s32 D_80077014;
extern s32 D_80077018;
extern s32 D_8007701C;
extern s32 D_80077024;
extern u32 D_80077028;
extern s32 D_80076F04;
extern s32 D_80076F08;
extern s32 D_80076F30;
extern s32 D_80076F34;
extern s32 D_80077030;
extern s32 D_80077034;
extern s32 D_80077040;
extern s32 D_80077044;
extern s32 D_8007704C;
extern SVECTOR D_800770B0;
extern MATRIX D_80077070;
extern s16 D_80077072;
extern s16 D_80077074;
extern s16 D_80077076;
extern s16 D_80077078;
extern s16 D_8007707A;
extern s16 D_8007707C;
extern s16 D_8007707E;
extern s16 D_80077080;
extern s16 D_80077090;
extern s16 D_80077092;
extern s16 D_80077094;
extern s16 D_80077096;
extern s16 D_80077098;
extern s16 D_8007709A;
extern s16 D_8007709C;
extern s16 D_8007709E;
extern s16 D_800770A0;
extern MATRIX D_800770B8;
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
extern MATRIX D_800770D8;
extern s32 D_800770EC;
extern s32 D_800770F0;
extern s32 D_800770F4;
extern s16 D_800770F8;
extern s16 D_800770FA;
extern s16 D_800770FC;
extern s16 D_800770FE;
extern s16 D_80077100;
extern s16 D_80077102;
extern s16 D_80077104;
extern s16 D_80077106;
extern s16 D_80077108;
extern s32 D_8007711C;
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

extern MovieVector D_8007702C;
extern MovieVector D_8007703C;
extern MovieVector D_80076F2C;

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 pad1[0x54];
    /* 0x55 */ u8 unk55;
    /* 0x56 */ u8 pad56[0xE2];
} MovieSlot; // Size: 0x138

extern MovieSlot D_8007713C[2];
extern s32 D_8007739C;
extern s32 D_800773AC;
extern s32 D_800773B4;
extern s32 D_80077438;
extern s32 D_80077454;
extern RECT D_800704E0; // .rodata
extern s32 D_80077448;
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
extern s32 D_80077394;
extern s32 D_800773AC;
extern s32 D_800773B4;

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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_8007519C);

/* best 91.70% - same size as retail (544 bytes); needs the state header read
   twice (debug-table path and CD path) and the local/register order fixed: our
   `result` lands in $s3 where retail has $s4. Everything else in the two paths
   matches. Rewritten from the assembly, no earlier park note existed.
s32 func_8007519C(void) {
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

    state = D_80077448;
    result = -1;
    if (state == 0) {
        ArchiveSetIndex(0x18, 0);
        if (D_8007711C >= (s16)ArchiveDecodeSizeAbsolute(2)) {
            return result;
        }
        nFileIndex = D_8007711C + 3;
        pPath = ArchiveGetFilePath(nFileIndex);
        nSectorSize = 0x800;
        nHeaderSize = 0x20;
        nHeaderSkip = 0;
    } else if (state == 1) {
        ArchiveSetIndex(0x18, 1);
        if (D_8007711C >= (s16)ArchiveDecodeSizeAbsolute(1)) {
            return result;
        }
        nFileIndex = D_8007711C + 2;
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
*/

void MovieLoadSoundBanks(void) {
    char* pWdsFile;

    pWdsFile = ArchiveReadHddFile(D_80070304, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((func_8003BDFC(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);

    pWdsFile = ArchiveReadHddFile(D_80070328, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((func_8003BDFC(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);

    pWdsFile = ArchiveReadHddFile(D_8007034C, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    while ((func_8003BDFC(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);
}

void MovieCreateAudioManager(void) {
    char* pWdsFile;

    pWdsFile = ArchiveReadHddFile(D_80070370, 0);
    SoundLoadWdsFile(pWdsFile, 0);
    // func_8003BDFC returns a 16-bit status, tested in the low half
    while ((func_8003BDFC(0) << 16) != 0) {
    }
    HeapFree(pWdsFile);
    D_8007700C = SoundCreateAudioManagerOnHeap(ArchiveReadHddFile(D_80070394, 0));
}

void MoviePlayAudioManager(void) {
    SoundPlayAudioManager(D_8007700C, 0x7F, 0);
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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80075D8C);

/* best 92.41% - 1228 bytes against retail's 1232; the head, the pad-cursor
   handling, the inner loop's tail test and everything from FontPrintf(0x80070430)
   to the epilogue are instruction-identical. Three differences remain:
   1. gcc CSEs `j + 1` (the ArchiveDecodeSector/ArchiveGetFilePath argument) into a
      callee-saved register whose live range spans the whole inner loop, so the frame
      is 0x50 with 8 saves where retail has 0x48 with 7, and every local sits one
      register higher (entry in $18 where retail has $17). Retail recomputes
      `addiu a0,$s0,1` at all three call sites. Also tried: a `for` loop (same CSE,
      plus a duplicated top exit test), and the pointer-increment spelling.
   2. the cursor limit: a `count = 0x1249` local makes gcc fold `count - 0x14` and
      hoist 0x1235, retail hoists 0x1249 and does `addiu $v0,$s6,-0x14` at each of
      the two sites.
   3. the HEX/DEC ternary is laid out with the true value (D_80070474) as fallthrough;
      retail materialises the false value (D_80070488) first and branches to the true.
   Recovered for it: MovieDrawBuffer (see the typedef above) and that the two draw
   buffers D_80077124[] each start 0x18 bytes before the matching MovieSlot entry
   D_8007713C[].
void func_80075D8C(void) {
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
    char* pPath;
    char* pFileName;
    char* pCursor;
    MovieDrawBuffer* pBuffer;

    MovieClearFrameBuffer();
    entry = 0;
    ArchiveGetArchiveOffsetIndices(&alignedIndex, &remainder);
    ArchiveSetIndex(0, 0);
    count = 0x1249;
    hexMode = 0;
    showName = 0;
    do {
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
        if ((D_800773B4 & 0x1000) == 0 && (D_800773AC & 0x1000) != 0) {
            if (entry > 0) {
                entry--;
            }
        }
        if ((D_800773B4 & 0x10) == 0 && (D_800773AC & 0x10) != 0) {
            entry -= 0x14;
            if (entry < 0) {
                entry = 0;
            }
        }
        if ((D_800773B4 & 0x4000) == 0 && (D_800773AC & 0x4000) != 0) {
            if (entry < count - 0x14) {
                entry++;
            }
        }
        if ((D_800773B4 & 0x40) == 0 && (D_800773AC & 0x40) != 0) {
            entry += 0x14;
            if (entry > count - 0x14) {
                entry = count - 0x14;
            }
        }
        if ((D_800773B4 & 0x8) == 0 && (D_800773AC & 0x8) != 0) {
            hexMode = 1 - hexMode;
        }
        if ((D_800773B4 & 0x4) == 0 && (D_800773AC & 0x4) != 0) {
            showName = 1 - showName;
        }
        j = entry;
        do {
            if (WorldMapSyncGearRideActors(j) == 0) {
                FontPrintf(hexMode ? D_80070474 : D_80070488, j);
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
                        pPath = ArchiveGetFilePath(j + 1);
                        if (pPath != NULL) {
                            pFileName = pPath;
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
                            FontPrintf(D_8007042C, pPath);
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
        if (D_80077394 > 0) {
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

void MovieResetPlaybackAndClearScreen(void) {
    long pad[6];
    RECT rect;

    rect = D_800704E0;
    D_80077028 = 0;
    D_800773AC = -1;
    if (D_80077448 == 0) {
        ArchiveSetIndex(0x18, 0);
        if (D_8007711C >= (s16)ArchiveDecodeSizeAbsolute(2)) {
            return;
        }
        SetDispMask(0);
    } else if (D_80077448 == 1) {
        ArchiveSetIndex(0x18, 1);
        if (D_8007711C >= (s16)ArchiveDecodeSizeAbsolute(1)) {
            return;
        }
        SetDispMask(0);
    } else if (D_80077448 == 2) {
        return;
    }
    D_8007713C[0].unk0 = 0;
    D_8007713C[1].unk0 = 0;
    if (D_80077454 != 0) {
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
    if (D_80077454 != 0) {
        D_8007713C[0].unk55 = 0;
        D_8007713C[1].unk55 = 0;
    }
}

// non-void: gcc reorg moves the delay-slot fill (li v0,1) differently when the
// function returns void; retail keeps the nop after each branch and the 0x30 frame.
s32 MovieStartStreamWithMode(u8 mode) {
    long pad[6];
    s32 state;

    state = D_80077448;
    D_80077028 = mode;
    D_800773AC = -1;
    if (state == 0) {
        ArchiveSetIndex(0x18, 0);
        if (D_8007711C >= (s16)ArchiveDecodeSizeAbsolute(2)) {
            return;
        }
    } else if (state == 1) {
        ArchiveSetIndex(0x18, 1);
        if (D_8007711C >= (s16)ArchiveDecodeSizeAbsolute(1)) {
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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076488);

void MovieRequestFrame(u16 arg0, s32 arg1, u16 arg2) {
    D_80077010 = arg0;
    if (D_801D68B4 == 1) {
        if (arg2 != D_80077024) {
            D_80077018 = 0;
        } else {
            D_80077018 = 1;
        }
    } else if (arg2 != D_80077024) {
        D_80077018 = 0;
        D_8007701C = 0;
    } else {
        D_80077018 = 1;
        D_8007701C = 1;
    }
    if (arg0 < D_8007739C) {
        return;
    }
    if (D_80077438 != 0) {
        return;
    }
    D_80077014 = 1;
}

void MovieFadeAudioOnPadPress(void) {
    D_800773B4 = D_800773AC;
    D_800773AC = ControllerGetButtonState(0);
    if (D_800773B4 == D_800773AC) {
        if (D_800773B4 != 0) {
            D_80076F04++;
            if (D_80076F08 < D_80076F04) {
                D_800773B4 = 0;
                D_80076F08 = 1;
                D_80076F04 = 0;
            }
        } else {
            D_80076F08 = 0x5A;
            D_80076F04 = 0;
        }
    } else {
        D_80076F08 = 0x5A;
        D_80076F04 = 0;
    }
    if ((D_800773B4 & 0x40) == 0 && (D_800773AC & 0x40) != 0 && D_80077028 == 0) {
        SoundSetCdVolumeWithFade(0, 0xA);
        D_80077014 = 5;
    }
    if ((D_800773B4 & 0x800) == 0 && (D_800773AC & 0x800) != 0 && D_80077028 == 0) {
        SoundSetCdVolumeWithFade(0, 0xA);
        D_80077014 = 5;
    }
}
*/

void MovieInitGteState(void) {
    MATRIX* pColorMatrix;

    SetGeomScreen(0x200);
    SetGeomOffset(0xA0, 0x78);
    pColorMatrix = &D_80077070;

    D_800770B8.m[0][0] = 0x1000;
    D_800770C0 = 0x1000;
    D_800770C8 = 0x1000;
    D_800770F8 = 0x1000;
    D_80077100 = 0x1000;
    D_80077108 = 0x1000;

    D_80077090 = 0x93D;
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

    func_80076CA4();
    pMatrix = &D_80077050;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076CA4);

/* best 89.13% under gcc 2.7.2 -G0 (88.94 with a MATRIX* local for the trans
   stores instead of the individual symbols, 83.09 under the unit's old -G8).
   Size is retail's 420 bytes / 105 instructions and the whole head is exact;
   two differences remain:
   1. the trans-matrix store block. Retail computes ONE address into $a3
      (0x800770CC, i.e. &D_800770B8.t[0] = base+0x14), stores with $a3 as the
      base (sw v1,0(a3)) and then derives the CompMatrix argument from it
      (addiu a1,a3,-0x14). We emit a lone lui/addiu pair for the argument and a
      bare %lo(%at) store. That is gcc sharing the %hi of one symbol_ref across
      two offsets of the SAME symbol, which needs the trans base to reach the
      store through a register - a pointer local gets folded back to a symbol
      store, so all three store spellings tried (individual D_ symbols, a
      MATRIX* pTransMatrix, a s32* pTransT = pTransMatrix->t) give a %at store.
   2. one register tie: retail mult $s2,$s2 / mflo $v0 / addu $s3,$s3,$v0, we
      get $v1 for the mflo and the addu.
   Permuter run (4 min, -j 2) from this body: PERMUTER_BETTER base=950 best=775,
   but its best candidate (u32 dxSq + a pRotMatrix reassignment so that both
   CompMatrix arguments are the same pointer) is UNSOUND - it compiles to
   CompMatrix(trans, trans, res) - and porting it drops objdiff to 83.30%.
   The u32 dxSq half of it is kept here and is neutral for objdiff.
   Next lever to try: a source shape where the trans stores are reached through
   a runtime base (parameter/global pointer), since gcc folds every
   compile-time-known global address into a bare symbol store. Parked body:
void func_80076CA4(void) {
    MATRIX* pRotMatrix;
    MovieVector* pPos;
    SVECTOR* pAngle;
    s32 dz;
    s32 dx;
    s32 dzSq;
    u32 dxSq;
    s32 dy;

    pPos = &D_8007703C;
    pAngle = &D_800770B0;
    dz = D_80077044 - D_80077034;
    dzSq = dz * dz;
    dx = pPos->x - D_8007702C.x;
    dxSq = dx * dx;
    dy = D_80077040 - D_80077030;
    pAngle->vx = ratan2(dy, SquareRoot0(dzSq + dxSq));
    pAngle->vy = -ratan2(dx, dz);
    pRotMatrix = &D_800770D8;
    RotMatrix(pAngle, pRotMatrix);
    RotMatrixZ(D_8007704C, pRotMatrix);
    MulMatrix2(&D_800770F8, pRotMatrix);
    pRotMatrix->t[0] = 0;
    pRotMatrix->t[1] = 0;
    pRotMatrix->t[2] = SquareRoot0(dxSq + dy * dy + dzSq);
    D_800770CC = D_80076F2C.x - pPos->x;
    D_800770D0 = D_80076F30 - D_80077040;
    D_800770D4 = D_80076F34 - D_80077044;
    CompMatrix(pRotMatrix, (MATRIX*)&D_800770B8, &D_80077050);
}
*/
