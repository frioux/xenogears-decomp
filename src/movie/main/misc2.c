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
extern s32 D_80076F2C;
extern s32 D_80076F30;
extern s32 D_80076F34;
extern s32 D_8007702C;
extern s32 D_80077030;
extern s32 D_80077034;
extern s32 D_8007703C;
extern s32 D_80077040;
extern s32 D_80077044;
extern s32 D_8007704C;
extern s16 D_800770B0;
extern s16 D_800770B2;
extern s16 D_80077070;
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
extern s16 D_800770B8;
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
extern s32 D_800770D8[];
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
s32 func_800763BC(u8 mode) {
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

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076AF0);

/* best 98.87% - the whole stream matches (380 bytes both, permuter run from this
   body found nothing in 1969 iterations) except the base of the first colour-matrix
   store: retail keeps &D_80077070 in $a0 (it survives from the `la` after
   SetGeomOffset to the SetColorMatrix call) and writes `sh v0,0(a0)`, while our
   store is the symbol form `sh v0,D_80077070` (assembler-expanded through $at).
   Shapes tried: s16* local, MATRIX* local with m[0][0], plain symbol store; also
   scored under -G0 (99.15%) and 2.7.2-cdk. The other two diagonal stores of the
   same matrix use %lo in retail too, so only the offset-0 element could fold.
void func_80076AF0(void) {
    s16* pDiag;

    SetGeomScreen(0x200);
    SetGeomOffset(0xA0, 0x78);
    pDiag = (s16*)&D_80077070;

    D_800770B8 = 0x1000;
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

    pDiag[0] = 0x969;
    D_80077072 = 0;
    D_80077074 = 0;
    D_80077076 = 0x969;
    D_80077078 = 0;
    D_8007707A = 0;
    D_8007707C = 0x969;
    D_8007707E = 0;
    D_80077080 = 0;

    SetColorMatrix((MATRIX*)pDiag);
    SetBackColor(0x60, 0x60, 0x60);
}
*/

void WorldMapScriptOp04SetScratchPosition(void) {
    long pad[6];
    MATRIX* pMatrix;

    func_80076CA4();
    pMatrix = &D_80077050;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc2", func_80076CA4);

/* best 73.42% - same instruction count (408 bytes) and the prologue now matches
   retail exactly: 0x30 frame, s1 = dz saved first, and &D_8007703C kept in $s5
   across the GTE calls. Getting the frame and $s5 needs the position/vector
   triples declared as structs (MovieVector below, D_8007702C/3C/76F2C) and a
   MovieVector* local, not the flat s32 D_ globals - and the rotation/translation
   matrices typed as MATRIX (D_800770B8, D_800770D8), not s16/s32[].
   Remaining differences, all in the tail:
   1. retail materialises &(D_800770CC) = the D_800770B8 matrix + 0x14 in $a3,
      stores the three t[] words at 0(a3), and derives the CompMatrix matrix
      argument as a3 - 0x14; we fold the three stores to the %lo(D_800770B8+0x14)
      form and materialise the base separately.
   2. retail stores the SquareRoot0 result (D_800770F4) after it has loaded
      D_80076F2C/D_8007703C and formed a3/a1; ours stores it right after the call.
   3. the translation x reload lands in $a2 in retail, $v0/$6 in ours.
   Shapes tried: s32* translation pointer + pTrans[k] (same 73.42), MATRIX member
   form (same), and the AddressCSE-first order. -G0 on this unit's neighbour
   func_80076AF0 scored 99.15% but this unit must stay on its preset (the div
   trap), and try_flags --gp -G0 fails to compile the unit at all.
   Declarations this body needs, at the top of the unit:
     typedef struct { s32 x; s32 y; s32 z; } MovieVector; // Size: 0xC
     extern MovieVector D_8007702C;
     extern MovieVector D_8007703C;
     extern MovieVector D_80076F2C;
     extern SVECTOR D_800770B0;      // was: extern s16 D_800770B0; D_800770B2
     extern MATRIX D_800770B8;       // was: extern s16 D_800770B8;
     extern MATRIX D_800770D8;       // was: extern s32 D_800770D8[];
typedef struct {
    s32 x;
    s32 y;
    s32 z;
} MovieVector; // Size: 0xC

extern MovieVector D_8007702C;
extern MovieVector D_8007703C;
extern MovieVector D_80076F2C;

void func_80076CA4(void) {
    MATRIX* pRotMatrix;
    MATRIX* pTransMatrix;
    MovieVector* pPos;
    s32 dz;
    s32 dzSq;
    s32 dx;
    s32 dxSq;
    s32 dy;

    pPos = &D_8007703C;
    dz = D_80077044 - D_80077034;
    dzSq = dz * dz;
    dx = pPos->x - D_8007702C.x;
    dxSq = dx * dx;
    dy = D_80077040 - D_80077030;
    D_800770B0.vx = ratan2(dy, SquareRoot0(dzSq + dxSq));
    D_800770B0.vy = -ratan2(dx, dz);
    pRotMatrix = &D_800770D8;
    RotMatrix(&D_800770B0, pRotMatrix);
    RotMatrixZ(D_8007704C, pRotMatrix);
    MulMatrix2(&D_800770F8, pRotMatrix);
    pTransMatrix = &D_800770B8;
    pRotMatrix->t[0] = 0;
    pRotMatrix->t[1] = 0;
    pRotMatrix->t[2] = SquareRoot0(dxSq + dy * dy + dzSq);
    pTransMatrix->t[0] = D_80076F2C.x - pPos->x;
    pTransMatrix->t[1] = D_80076F30 - D_80077040;
    pTransMatrix->t[2] = D_80076F34 - D_80077044;
    CompMatrix(pRotMatrix, pTransMatrix, &D_80077050);
}
*/
