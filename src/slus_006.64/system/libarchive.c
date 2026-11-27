#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"

// Temp
extern int func_800286CC(void);

#define ARCHIVE_DEBUG_BLOCK_HEADER_SIZE 0x20
#define ARCHIVE_STREAM_FILE_READY 3

// Sectors of the archive that hold the directory table and the file offset table.
#define ARCHIVE_TABLE_SECTOR 0x18
#define ARCHIVE_HEADER_SECTOR 0x28

extern s32 g_ArchiveCurFileSector;

extern s32 g_ArchiveStreamQueue;
extern s32 g_ArchiveStreamQueueIndex;
extern u16 g_ArchiveStreamReadSectionId;
extern u32 g_ArchiveCurEntryIndex;
extern void GfxDrawArchiveReadRetry(int, int, int, int);
extern int ArchiveDecodeSizeAligned(int entryIndex);
extern s16 g_ArchiveDebugStreamSectionCount;
extern s8* g_ArchiveStreamWritePointer;
extern u16 g_ArchiveStreamNextSectionId;
extern u8 g_ArchiveSectorScratch[];
extern s32 g_ArchiveDebugStreamSector;  // debug stream: sector buffer being split
extern s32 g_ArchiveDebugStreamPayload;  // debug stream: payload cursor inside that sector
extern s16 g_ArchiveDebugStreamBlockCount;  // debug stream: sub-blocks in that sector
// A second name for the same halfword: the split block re-reads the count after
// writing it, and only a distinct object stops gcc forwarding the store.
extern s16 g_ArchiveDebugStreamBlockCountReload[] __asm__("g_ArchiveDebugStreamBlockCount");
// Same for the queue arm, which re-reads the section id after storing it.
extern u16 g_ArchiveStreamNextSectionIdReload[] __asm__("g_ArchiveStreamNextSectionId");
extern u16 D_8005A4B8;  // debug stream: sector header word at +8

// Stream file: the section count, then a section table of `sectionCount` entries.
// The payload starts STREAM_FILE_HEADER_SIZE past the count, so the table has room
// for its first entries inside the header.
typedef struct {
    /* 0x0 */ s32 sectionCount;
    /* 0x4 */ ArchiveStreamFileSectionHeader sections[1];
} ArchiveStreamFileHeader;

// Archive header table, read from sector ARCHIVE_HEADER_SECTOR in ArchiveInit: one
// sector offset per directory entry, then the disc number.
typedef struct {
    /* 0x0  */ u16 sectorOffsets[60];
    /* 0x78 */ u16 discNumber;
} ArchiveHeaderTable; // Size: 0x7A

// Archive directory entry: a 3-byte sector index and a 4-byte size, both stored
// little-endian by the PC tools that built the archive.
typedef struct {
    /* 0x0 */ u8 sector[3];
    /* 0x3 */ u8 size[4];
} ArchiveEntry; // Size: 0x7

void ArchiveInit(u32 pArchiveTable, u32 pHeaderTable, u32 pDebugTable) {
    g_ArchiveCdReadStartCount = 0;
    g_ArchiveCdReadCompleteCount = 0;
    g_ArchiveCdReadStatusErrorCount = 0;
    g_ArchiveCdDataSectorCompleteCount = 0;
    g_ArchiveCdDataSectorErrorCount = 0;
    D_8005A49C = 0;
    g_ArchiveCdReadRetryExhaustedCount = 0;
    g_ArchiveCdHaltCompleteCount = 0;
    g_ArchiveCdHaltErrorCount = 0;
    
    if ( (pDebugTable == NULL) || (pDebugTable == -1) ) {
        while (CdInit() == 0);
        CdSetDebug(0);
        CdDataCallback(0);
        CdSyncCallback(NULL);
        CdReadyCallback(NULL);
        CdControl(CdlStandby, 0, &D_80059F1C);
        ArchiveCdSetMode(CdlModeSpeed | CdlModeSize1);
        ArchiveCdDataSync(0);
        Vsync(3);
    } else {
        PCinit();
    }

    if (pDebugTable != -1) {
        g_ArchiveDebugTable = pDebugTable;
    } else {
        g_ArchiveDebugTable = NULL;
    }
    
    g_ArchiveTable = pArchiveTable;
    g_ArchiveHeader = pHeaderTable;
    g_CurArchiveOffset = 0;
    g_ArchivePendingReadCount = 0;
    g_ArchiveCurFileSize = 0;
    g_ArchiveCdDriveState = 0;
    g_ArchiveDebugFileHandle = -1;
    
    if (!pDebugTable) {
        ArchiveReadFileFromCdSector(ARCHIVE_TABLE_SECTOR, pArchiveTable, (0x10 * CD_SECTOR_SIZE), 0, 0);
        ArchiveCdDataSync(0);
        ArchiveReadFileFromCdSector(ARCHIVE_HEADER_SECTOR, g_ArchiveHeader, sizeof(ArchiveHeaderTable), 0, 0);
        ArchiveCdDataSync(0);
    }
}

void ArchiveReset(void) {
    ArchiveStopTransfer(0);
    ArchiveCdDataSync(0);
    if (g_ArchiveDebugTable == 0) {
        while (CdControlB(CdlPause, 0, &D_80059F1C) == 0);
        ArchiveCdSetMode(CdlModeSpeed | CdlModeSize1);
        ArchiveCdDataSync(0);
        Vsync(3);
    }
    CdDataCallback(0);
    CdSyncCallback(NULL);
    CdReadyCallback(NULL);
    g_ArchivePendingReadCount = 0;
    g_ArchiveCurFileSize = 0;
    g_ArchiveCdDriveState = 0;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex) {
    s32 nOffset;

    nOffset = ((ArchiveHeaderTable*)g_ArchiveHeader)->sectorOffsets[directoryIndex + entryIndex] - 1;
    g_CurArchiveOffset = nOffset;
    if (nOffset < 0) {
        g_CurArchiveOffset = 0;
        return -1;
    }
    return nOffset;
}

int ArchiveGetArchiveOffsetIndices(int* alignedIndex, int* remainder) {
    int nAligned;
    int nIndex;
    u_short* pArchiveHeaderEntry;

    pArchiveHeaderEntry = ((ArchiveHeaderTable*)g_ArchiveHeader)->sectorOffsets;
    nIndex = 0;
    
    while(nIndex < ARCHIVE_MAX_SECTIONS) {
        if (*pArchiveHeaderEntry == g_CurArchiveOffset + 1) {
            // Round down to 4-byte boundary
            nAligned = (nIndex / 4) * 4;
            
            *alignedIndex = nAligned;
            *remainder = nIndex - nAligned;
            break;
        }
        
        nIndex += 1;
        pArchiveHeaderEntry += 1;
    }
    
    if (nIndex == ARCHIVE_MAX_SECTIONS) {
        *alignedIndex = 0;
        *remainder = 0;
    }
    
    return g_CurArchiveOffset;
}

unsigned short ArchiveGetDiscNumber(void) {
    return ((ArchiveHeaderTable*)g_ArchiveHeader)->discNumber;
}

int ArchiveGetRelativeEntryIndex(int directoryIndex, int entryIndex) {
    int nOffset;
    nOffset = ((ArchiveHeaderTable*)g_ArchiveHeader)->sectorOffsets[directoryIndex + entryIndex];
    return nOffset - g_CurArchiveOffset;
}

// Only matches on GCC 2.6.X
char* ArchiveReadHddFile(char* pFilePath, int* pFileSize) {
    int nFileSize;
    int hFile;
    int i;
    int nBytesRead;
    char* pFileBuffer;

    for (i = 0; i < RETRY_COUNT; i++) {
        hFile = PCopen(pFilePath, 0, O_RDONLY);
        if (hFile != -1)
            break;
    }

    if (hFile == -1) {
        pFileBuffer = NULL;
        goto error;
    }

    nFileSize = PClseek(hFile, 0, SEEK_END);
    if (pFileSize)
        *pFileSize = nFileSize;
    PClseek(hFile, 0, SEEK_SET);
    
    pFileBuffer = HeapAlloc(nFileSize, 0);
    nBytesRead = 0;
    if (pFileBuffer) {
        for (i = 0; i < RETRY_COUNT; i++) {
            nBytesRead = PCread(hFile, pFileBuffer, nFileSize);
            if (nBytesRead != 0)
                break;
        }
    } 
    
    if (nBytesRead == 0) {
        if (pFileBuffer)
            HeapFree(pFileBuffer);
        pFileBuffer = NULL;
    }

    i = 0;
    while (PCclose(hFile) != 0) {
        i += 1;
        if (i >= RETRY_COUNT) {
            if (pFileBuffer)
                HeapFree(pFileBuffer);
            pFileBuffer = NULL;
            break;
        }        
    }

    error:
    return pFileBuffer;
}

int ArchiveGetCurFileSize(void) {
    return g_ArchiveCurFileSize;
}

int ArchiveDataSync(void) {
    int nResult;

    nResult = g_ArchivePendingReadCount;
    if (nResult == 0) {
        if (g_ArchiveDebugTable == NULL && CdDataSync(1)) {
            return 1;
        }
        if (g_ArchiveCdDriveState != 0) {
            return 1;
        }
    }
    return nResult;
}

int ArchiveDecodeSize(int entryIndex) {
    char* pFilepath;
    int hArchiveFile;
    int nFileSize;
    ArchiveEntry* pEntry;
    u32 nOffset;
    
    if (g_ArchiveDebugTable) {
        pFilepath = ArchiveGetFilePath(entryIndex);
        hArchiveFile = PCopen(pFilepath, O_RDONLY, 0);
        nFileSize = PClseek(hArchiveFile, 0, SEEK_END);
        PCclose(hArchiveFile);
        if (nFileSize > 0) {
            goto exit;
        }
    }
    
    nOffset = (entryIndex + g_CurArchiveOffset - 1) * sizeof(ArchiveEntry);
    pEntry = (ArchiveEntry*)(nOffset + g_ArchiveTable);
    nFileSize = (
        (pEntry->size[3] << 0x18) + 
        (pEntry->size[2] << 0x10) + 
        (pEntry->size[1] << 0x8) + 
        pEntry->size[0]
    );

    exit:
    return nFileSize;
}

// Uses g_ArchiveHeaderOffsetBase as archive offset
int ArchiveDecodeSizeAligned(int entryIndex) {
    char* pFilepath;
    int hArchiveFile;
    int nFileSize;
    ArchiveEntry* pEntry;
    u32 nOffset;
    
    if (g_ArchiveDebugTable) {
        pFilepath = ArchiveGetFilePath(entryIndex);
        hArchiveFile = PCopen(pFilepath, O_RDONLY, 0);
        nFileSize = PClseek(hArchiveFile, 0, SEEK_END);
        PCclose(hArchiveFile);
        if (nFileSize > 0) {
            goto exit;
        }
    }
    
    nOffset = (entryIndex + g_ArchiveHeaderOffsetBase - 1) * sizeof(ArchiveEntry);
    pEntry = (ArchiveEntry*)(nOffset + g_ArchiveTable);
    nFileSize = (
        (pEntry->size[3] << 0x18) + 
        (pEntry->size[2] << 0x10) + 
        (pEntry->size[1] << 0x8) + 
        pEntry->size[0]
    );

    exit:
    nFileSize = ((nFileSize + 3) / 4) * 4;
    return nFileSize;
}

// Only matches on GCC 2.6.X
int ArchiveDecodeAlignedSize(unsigned int entryIndex) {
    s32 nSize;
    s32 nAlignedSize;
    
    nSize = ArchiveDecodeSize(entryIndex);
    nAlignedSize = ((nSize + 3 ) / 4);
    return nAlignedSize * 4;
}

int ArchiveDecodeSizeAbsolute(int entryIndex) {
    int nFileSize;
    ArchiveEntry* pEntry;
    u32 nOffset;
    
    nOffset = (entryIndex + g_CurArchiveOffset - 1) * sizeof(ArchiveEntry);
    pEntry = (ArchiveEntry*)(nOffset + g_ArchiveTable);
    nFileSize = (
        (pEntry->size[3] << 0x18) + 
        (pEntry->size[2] << 0x10) + 
        (pEntry->size[1] << 0x8) + 
        pEntry->size[0]
    );

    if (nFileSize >= 0)
        nFileSize = 0;
    else
        nFileSize = (s16) (nFileSize * -1);

    return nFileSize;
}

char* ArchiveGetFilePath(int entryIndex) {
    unsigned int nOffset;
    
    if (g_ArchiveDebugTable) {
        nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_ENTRY_NAME_BUFFER_SIZE;
        return (char*)(nOffset + g_ArchiveDebugTable);
    }

    return NULL;
}

int ArchiveDecodeSector(int entryIndex) {
    ArchiveEntry* pEntry;
    unsigned int nOffset;

    nOffset = (entryIndex + g_CurArchiveOffset - 1) * sizeof(ArchiveEntry);
    pEntry = (ArchiveEntry*)(nOffset + g_ArchiveTable);
    return (pEntry->sector[2] << 0x10) + (pEntry->sector[1] << 0x8) + pEntry->sector[0];
}

// Same as ArchiveDecodeSector, but uses g_ArchiveHeaderOffsetBase as current archive offset
int ArchiveDecodeSectorAligned(int entryIndex) {
    ArchiveEntry* pEntry;
    unsigned int nOffset;

    nOffset = (entryIndex + g_ArchiveHeaderOffsetBase - 1) * sizeof(ArchiveEntry);
    pEntry = (ArchiveEntry*)(nOffset + g_ArchiveTable);
    return (pEntry->sector[2] << 0x10) + (pEntry->sector[1] << 0x8) + pEntry->sector[0];
}



// Possible start of archive2.c

void ArchiveCdDataSync(int mode) {
    int nResult;

    if (mode == 0) {
        do {
            nResult = ArchiveDataSync();
        } while (0 < nResult);
    }
    ArchiveDataSync();
}

void* ArchiveChangeStreamingFile(void* pStreamFile) {
    void* pPrevStreamFile;

    pPrevStreamFile = g_ArchiveCurStreamFile;
    g_ArchiveCurStreamFile = pStreamFile;
    return pPrevStreamFile;
}

int ArchiveClearStreamFileSections(void) {
    ArchiveStreamFileHeader* pStream;
    int nEntries;
    int i;
    ArchiveStreamFileSectionHeader* pCurSectionHeader;
    ArchiveStreamFileSectionHeader* pSectionHeaderStart;

    pStream = g_ArchiveCurStreamFile;
    if (pStream == NULL) {
        return -1;
    }
    
    i = 0; 
    nEntries = pStream->sectionCount;
    pSectionHeaderStart = pStream->sections;
    
    pCurSectionHeader = pSectionHeaderStart;
    while (i < nEntries) {
        pCurSectionHeader[i].state = 0;
        pCurSectionHeader[i].id = 0;
        pCurSectionHeader[i].size = 0;
        pCurSectionHeader[i].unk2 = 0;
        i++;
    }
    pSectionHeaderStart->size = nEntries;
    
    return nEntries;
}

u_char* ArchiveReadStreamSector(void) {
    u_char* pData;
    ArchiveStreamFileSectionHeader* pHeader;
    ArchiveStreamFileSectionHeader* pHeaders;
    StreamDataQueueEntry* pEntry;
    u_char* pPath;
    int nSections;
    int nIndex;
    int nQueueIndex;
    int nId;
    int i;

    if (g_ArchiveCurStreamFile == NULL) {
        return NULL;
    }
    pData = g_ArchiveCurStreamFile;

    pHeader = (ArchiveStreamFileSectionHeader*)(pData + 4);
    nSections = *(s32*)pData;
    pData = pData + nSections * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;

    if (g_ArchiveDebugTable != 0) {
        if (g_ArchiveDebugFileHandle == -1) {
            return NULL;
        }

        if (g_ArchiveCurFileSize <= 0) {
            return NULL;
        }

        for (i = 0; i < g_ArchiveStreamSectionCount; i++) {
            pHeaders = g_ArchiveStreamSections;
            pHeader = (ArchiveStreamFileSectionHeader*)(g_ArchiveStreamQueueIndex * sizeof(ArchiveStreamFileSectionHeader) + (u32)pHeaders);
            nIndex = g_ArchiveStreamQueueIndex;
            g_ArchiveStreamQueueIndex = nIndex + 1;
            if (g_ArchiveStreamQueueIndex >= g_ArchiveStreamSectionCount) {
                g_ArchiveStreamQueueIndex = 0;
            }
            if (pHeader->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
                break;
            }
        }

        if (pHeader->state != ARCHIVE_STREAM_FILE_NOT_LOADED) {
            return NULL;
        }

        pHeader->state = 3;
        pData = pData + nIndex * CD_SECTOR_SIZE;

        for (i = 0; i < RETRY_COUNT; i++) {
            if (PCread(g_ArchiveDebugFileHandle, pData, CD_SECTOR_SIZE) != 0) {
                goto readDone;
            }
            GfxDrawArchiveReadRetry(i, 0, 0xFF, 0);
        }
        return NULL;

    readDone:

        g_ArchiveCurFileSize = g_ArchiveCurFileSize - CD_SECTOR_SIZE;
        if (g_ArchiveCurFileSize > 0) {
            return pData;
        }

        g_ArchiveCurFileSize = 0;
        for (i = 0; i < RETRY_COUNT; i++) {
            if (PCclose(g_ArchiveDebugFileHandle) == 0) {
                break;
            }
            GfxDrawArchiveReadRetry(i, 0, 0, 0xFF);
        }

        g_ArchiveDebugFileHandle = -1;
        if (g_ArchiveStreamQueue != 0) {
            nQueueIndex = g_ArchiveStreamQueueIndex + 1;
            pEntry = ((StreamDataQueueEntry*)g_ArchiveStreamQueue) + nQueueIndex;
            nId = *(u_short*)pEntry;
            g_ArchiveStreamQueueIndex = nQueueIndex;
            g_ArchiveCurEntryIndex = nId;

            if (nId > 0 && pEntry->pData != NULL) {
                pPath = ArchiveGetFilePath(nId);
                for (i = 0; i < RETRY_COUNT; i++) {
                    g_ArchiveDebugFileHandle = PCopen(pPath, 0, 0);
                    if (g_ArchiveDebugFileHandle != -1) {
                        break;
                    }
                    GfxDrawArchiveReadRetry(i, 0xFF, 0, 0);
                }
                g_ArchiveCurFileSize = ArchiveDecodeSizeAligned(nId);
                g_ArchivePendingReadCount = g_ArchivePendingReadCount - 1;
                return pData;
            }

            g_ArchiveCurFileSize = 0;
        }

        g_ArchivePendingReadCount = 0;
        return pData;
    }

    for (i = 0; i < nSections; i++, pHeader++) {
        if (pHeader->state == 3 && pHeader->id == g_ArchiveStreamReadSectionId) {
            break;
        }
    }

    if (i == g_ArchiveStreamSectionCount) {
        return NULL;
    }

    g_ArchiveStreamReadSectionId += 1;
    return pData + i * CD_SECTOR_SIZE;
}

int ArchiveCheckStreamFileSectionStates(int sectionIndex, int maxEntries, int targetState) {
    int i;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    int nSections;
    
    i = 0;
    while (i < maxEntries) {
        pSectionHeader = g_ArchiveStreamSections + sectionIndex;
        nSections = g_ArchiveStreamSectionCount;

        while (1) {
            if (pSectionHeader->state == targetState) {
                sectionIndex++;
                pSectionHeader++;
                
                if (nSections < sectionIndex)
                    return 1;
                
                i++;
                
                if (i >= maxEntries)
                    return 0;
                
                continue; // load-bearing: without it the loop rotates and the body compiles to 84 bytes
            }
            return 1;
        }
    }

    return 0;
}

void ArchiveConsolidateStreamFileEntry(int sectionIndex) {
    short nSize;
    int nNextIndex;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    ArchiveStreamFileSectionHeader* pNextSectionHeader;

    pSectionHeader = g_ArchiveStreamSections + sectionIndex;
    nSize = pSectionHeader->size;
    nNextIndex = sectionIndex + nSize;
    
    if (nNextIndex < g_ArchiveStreamSectionCount) {
        pNextSectionHeader = g_ArchiveStreamSections + nNextIndex;
        if (pNextSectionHeader->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
            pSectionHeader->size += pNextSectionHeader->size;
        }
    }
}

//  * Best: 86.08% - 1324 bytes / 331 instructions = retail's EXACT size and count
//  * (was 83.12% / 1300 / 325). Every remaining diff line is a register name, the
//  * placement of the seek block, or one load-delay nop; the instruction multiset
//  * and the frame (0x50) are retail's.
//  *
//  * Two finds this round, both the label-alias lever (levers.md
//  * a-label-alias-array-read-beats-volatile) applied to a global cc1 forwards:
//  *  - g_ArchiveDebugStreamBlockCountReload[], an array extern with
//  *    __asm__("g_ArchiveDebugStreamBlockCount"), read as the split block's count:
//  *    a distinct gcc object, so retail's lh reload survives (the plain spelling
//  *    forwards the sh through the two pSection stores - round 410 measured that
//  *    wall). It also kills the raw u16 register, which makes cc1 emit retail's
//  *    addu a2,a1,zero in the bnez delay slot: +2 instructions, and with the
//  *    guard then reading the global the pressure drop let nSpare[2] go, so the
//  *    frame is retail's 0x50 again.
//  *  - g_ArchiveStreamNextSectionIdReload[], same shape, for the QUEUE arm's
//  *    increment: retail loads that halfword twice there (0x8002927C for
//  *    pSection->id, 0x800292A0 for the increment) with no call between, so plain
//  *    C cse's them; the alias restores the second lui/lhu pair (+2 = retail's
//  *    1324). In arm 1 the two loads already come out separately because the
//  *    ArchiveConsolidateStreamFileEntry call kills the register.
//  *
//  * Residual (named):
//  *  - CALLEE-SAVED MAP ROTATION - PROVEN UNREACHABLE FROM SOURCE (round 531),
//  *    read off this body's own allocation dump (cpp with -DSKIP_ASM, then
//  *    tools/gcc-2.6.0-psx/cc1 <build.ninja cc_flags> -O2 -G8 -da -dl -dg in the
//  *    container; the .greg "N regs to allocate" list and .lreg "used X times
//  *    across Y insns" lines give levers.md's quotient
//  *    floor_log2(refs)*refs*10000*size/live):
//  *      pScratch  77  11 refs / 19 insns  = 69473 -> s0
//  *      pSection  74  21 / 72              = 46666 -> s1
//  *      i         73  27 / 115             = 37565 -> s2
//  *      pHeaders  75   9 / 202             =  5346 -> s3
//  *      pSectionOut 72  7 / 211            =  2654 -> s4
//  *      nOffset   80   3 / 51              =  2352 -> s5
//  *      pData     76   6 / 207             =  2318 -> s6
//  *      nSections 78   5 / 203             =  1970 -> s7
//  *      pDataOut  71   2 / 216             =   370 -> fp
//  *    (the hand-out is ascending s0..s7 along that descending order, fp last,
//  *    exact match with the dump's dispositions). Retail's map needs
//  *    77,73,74,75,80,76,78,72 - two deltas, both arithmetic: (a) i must outrank
//  *    pSection, 37565 vs 46666 (1.24x) = 36 refs or a 90-insn pSection range on
//  *    a 72-insn def-to-last-use; (b) pSectionOut must fall below nSections, 2654
//  *    vs 1970 (1.35x) = at most 5 refs (two fewer `*pSectionOut = 0`) or a
//  *    285-insn range on a 211-insn one. Retail's own stream pins five source
//  *    stores (its four machine stores are five minus one cross-jump merge), so
//  *    no spelling of the statements retail has reaches it - read as closed, do
//  *    not re-brief the hand-out. Measured dead before the dump: declaration-order
//  *    swaps of i/pSection byte-identical 86.08, id-store reorderings around the
//  *    nRemaining computation 82.51 and 83.51.
//  *  - the three reset seeks: retail merges all three (0x918, -0x28, -0x20)
//  *    into ONE block at 0x8002924C with a1 set per site and a taken branch
//  *    from each; this compile merges only the -0x28/-0x20 pair and inlines the
//  *    0x918 copy, which is also where the seek-tail branch polarity differs.
//  *    A shared label with an nSeek local measured 79.99% (round 410).
//  *  - lhu v0,4(s1) followed by nop, where retail fills the slot with the
//  *    pSection->id store; both orderings of that store around nRemaining
//  *    measured worse.
//  * Presets on this body: 2.6.0 (unit) 86.08, 2.7.2 86.03, cdk 80.46.
/*
s32 func_80028F30(s32* pDataOut, s32* pSectionOut) {
    s32 i;
    ArchiveStreamFileSectionHeader* pSection;
    ArchiveStreamFileSectionHeader* pHeaders;
    u8* pData;
    u8* pScratch;
    s32 nSections;
    s16 nBlockCount;
    s32 nOffset;
    s32 nRemaining;

    if (g_ArchiveCurStreamFile == NULL) {
        return 1;
    }
    pData = g_ArchiveCurStreamFile;

    pHeaders = (ArchiveStreamFileSectionHeader*)(pData + 4);
    nSections = *(s32*)pData;
    pData = pData + nSections * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;

    if (g_ArchiveDebugTable == 0) {
        goto fail;
    }
    if (g_ArchiveDebugFileHandle == -1) {
        goto fail;
    }
    if (g_ArchiveCurFileSize <= 0) {
        goto fail;
    }

    if (g_ArchiveDebugStreamSectionCount == 0) {
        for (i = 0; i < g_ArchiveStreamSectionCount; i += pSection->size) {
            pSection = g_ArchiveStreamSections + i;
            if (pSection->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
                break;
            }
        }
        if (i >= g_ArchiveStreamSectionCount) {
            goto fail;
        }

        nOffset = i * CD_SECTOR_SIZE;
        if (g_ArchiveCdMode[0] & 8) {
            pScratch = g_ArchiveSectorScratch;
            PCread(g_ArchiveDebugFileHandle, pScratch, 8);
            if (pScratch[0] == 1) {
                PClseek(g_ArchiveDebugFileHandle, 0x918, 1);
                *pSectionOut = 0;
                goto search;
            }
        }

        pScratch = (u8*)(nOffset + g_ArchiveStreamWritePointer);
        g_ArchiveDebugStreamSector = (s32)pScratch;
        PCread(g_ArchiveDebugFileHandle, pScratch, ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
        g_ArchiveDebugStreamBlockCount = *(u16*)(pScratch + 6);
        D_8005A4B8 = *(u16*)(pScratch + 8);

        if (pSection->size < g_ArchiveDebugStreamBlockCount) {
            if (g_ArchiveCdMode[0] & 8) {
                PClseek(g_ArchiveDebugFileHandle, -0x28, 1);
            } else {
                PClseek(g_ArchiveDebugFileHandle, -0x20, 1);
            }
            *pSectionOut = 0;
            goto search;
        }

        pSection->state = ARCHIVE_STREAM_FILE_READY;
        pSection->id = g_ArchiveStreamNextSectionId;
        nBlockCount = g_ArchiveDebugStreamBlockCountReload[0];
        nRemaining = pSection->size - nBlockCount;
        if (nRemaining >= 3) {
            pSection->size = nBlockCount + 1;
            pSection[nBlockCount + 1].size = nRemaining - 1;
            pSection[nBlockCount + 1].state = ARCHIVE_STREAM_FILE_NOT_LOADED;
            ArchiveConsolidateStreamFileEntry(nBlockCount + 1);
        }

        g_ArchiveStreamNextSectionId = g_ArchiveStreamNextSectionId + 1;
        g_ArchiveDebugStreamPayload = nOffset + g_ArchiveStreamWritePointer + g_ArchiveDebugStreamBlockCount * ARCHIVE_DEBUG_BLOCK_HEADER_SIZE;
        PCread(g_ArchiveDebugFileHandle, (u8*)g_ArchiveDebugStreamPayload, CD_SECTOR_SIZE - ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
        if (g_ArchiveCdMode[0] & 8) {
            PClseek(g_ArchiveDebugFileHandle, 0x118, 1);
        }
        g_ArchiveStreamQueueIndex = i;
        g_ArchiveCurFileSize = g_ArchiveCurFileSize - CD_SECTOR_SIZE;
        g_ArchiveDebugStreamSectionCount = g_ArchiveDebugStreamSectionCount + 1;

        *pSectionOut = 0;
        goto search;
    }

    if (g_ArchiveCdMode[0] & 8) {
        pScratch = g_ArchiveSectorScratch;
        PCread(g_ArchiveDebugFileHandle, pScratch, 8);
        if (pScratch[0] == 1) {
            PClseek(g_ArchiveDebugFileHandle, 0x918, 1);
            *pSectionOut = 0;
            goto search;
        }
    }

    i = g_ArchiveStreamQueueIndex + 1;
    pSection = g_ArchiveStreamSections + i;
    g_ArchiveStreamQueueIndex = i;
    pSection->state = ARCHIVE_STREAM_FILE_READY;
    pSection->id = g_ArchiveStreamNextSectionId;
    g_ArchiveStreamNextSectionId = g_ArchiveStreamNextSectionIdReload[0] + 1;
    PCread(g_ArchiveDebugFileHandle, (u8*)(g_ArchiveDebugStreamSectionCount * ARCHIVE_DEBUG_BLOCK_HEADER_SIZE + g_ArchiveDebugStreamSector), ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
    PCread(g_ArchiveDebugFileHandle, (u8*)(g_ArchiveDebugStreamSectionCount * (CD_SECTOR_SIZE - ARCHIVE_DEBUG_BLOCK_HEADER_SIZE) + g_ArchiveDebugStreamPayload), CD_SECTOR_SIZE - ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
    if (g_ArchiveCdMode[0] & 8) {
        PClseek(g_ArchiveDebugFileHandle, 0x118, 1);
    }
    g_ArchiveCurFileSize = g_ArchiveCurFileSize - CD_SECTOR_SIZE;
    g_ArchiveDebugStreamSectionCount = g_ArchiveDebugStreamSectionCount + 1;
    if (g_ArchiveDebugStreamSectionCount >= g_ArchiveDebugStreamBlockCount) {
        g_ArchiveDebugStreamSectionCount = 0;
    }

fail:
    *pSectionOut = 0;

search:
    for (i = 0; i < nSections; i++, pHeaders++) {
        if (pHeaders->state == ARCHIVE_STREAM_FILE_READY && pHeaders->id == g_ArchiveStreamReadSectionId) {
            break;
        }
    }
    if (i == g_ArchiveStreamSectionCount) {
        return 1;
    }

    pSection = (ArchiveStreamFileSectionHeader*)(pData + i * CD_SECTOR_SIZE);
    *pSectionOut = (s32)pSection;
    *pDataOut = (s32)((u8*)pSection + pSection->unk2 * ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
    if (ArchiveCheckStreamFileSectionStates(i, pSection->unk2, ARCHIVE_STREAM_FILE_READY) != 0) {
        return 1;
    }

    g_ArchiveStreamReadSectionId += pSection->unk2;
    return 0;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/libarchive", func_80028F30);



u_short ArchiveReleaseStreamBlock(ArchiveStreamFileSectionHeader* pSection) {
    ArchiveStreamFileHeader* pStream;
    u_char* pStreamData;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    u32 nIndex;
    u16 nState;

    pStream = g_ArchiveCurStreamFile;
    if (pStream == NULL) {
        return -1;
    }
    if (pSection == NULL) {
        return 0;
    }

    pSectionHeader = pStream->sections;
    pStreamData = (u_char*)pStream + pStream->sectionCount * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;
    nIndex = (u32)((u_char*)pSection - pStreamData) / CD_SECTOR_SIZE;
    nState = pSectionHeader[nIndex].state;
    pSectionHeader[nIndex].state = 0;
    return nState;
}

u_short ArchiveReleaseStreamSection(ArchiveStreamFileSectionHeader* pSection) {
    ArchiveStreamFileHeader* pStream;
    u_char* pStreamData;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    u32 nIndex;
    u16 nState;
    s32 nCount;

    pStream = g_ArchiveCurStreamFile;
    if (pStream == NULL) {
        return -1;
    }

    if (pSection == NULL) {
        return 0;
    }

    nCount = pSection->unk2;
    pSectionHeader = pStream->sections;
    pStreamData = (u_char*)pStream + pStream->sectionCount * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;
    nIndex = (u32)((u_char*)pSection - pStreamData) / CD_SECTOR_SIZE;
    nState = pSectionHeader[nIndex].state;

    while (nCount > 0) {
        pSectionHeader[nIndex + nCount - 1].state = 0;
        nCount--;
    }

    ArchiveConsolidateStreamFileEntry(nIndex);

    return nState;
}

s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags) {
    if (g_ArchiveDebugTable) {
        return -1;
    }
    
    ArchiveCdDataSync(0);
    g_ArchiveCurFileSector = sector;
    g_ArchiveCurFileSize = fileSize;
    return ArchiveReadFile(0, pDestBuffer, arg3, flags);
}

s32 ArchiveReadFileToBuffer(s32 index, u8* pBuffer, u32 arg2, u32 flags) {
    if ((index <= 0) || (ArchiveDecodeSize(index) <= 0) || (pBuffer == NULL)) {
        return -3;
    }
    ArchiveCdDataSync(0);
    g_ArchiveHeaderOffsetBase = g_CurArchiveOffset;
    g_ArchiveCurFileSector = ArchiveDecodeSector(index);
    g_ArchiveCurFileSize = ArchiveDecodeAlignedSize(index);
    return ArchiveReadFile(index, pBuffer, arg2, flags);
}
