#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"

// Temp
extern int func_800286CC(void);

#define ARCHIVE_DEBUG_BLOCK_HEADER_SIZE 0x20
#define ARCHIVE_STREAM_FILE_READY 3

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
extern s32 D_80059F54;  // debug stream: sector buffer being split
extern s32 D_80059F58;  // debug stream: payload cursor inside that sector
extern s16 D_80059F5C;  // debug stream: sub-blocks in that sector
extern u16 D_8005A4B8;  // debug stream: sector header word at +8

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
        CdControl(7, 0, &D_80059F1C);
        ArchiveCdSetMode(0xA0);
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
        ArchiveReadFileFromCdSector(0x18, pArchiveTable, (0x10 * CD_SECTOR_SIZE), 0, 0);
        ArchiveCdDataSync(0);
        ArchiveReadFileFromCdSector(0x28, g_ArchiveHeader, 0x7A, 0, 0);
        ArchiveCdDataSync(0);
    }
}

void ArchiveReset(void) {
    ArchiveStopTransfer(0);
    ArchiveCdDataSync(0);
    if (g_ArchiveDebugTable == 0) {
        while (CdControlB(9, 0, &D_80059F1C) == 0);
        ArchiveCdSetMode(0xA0);
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

    nOffset = ((u_short*)g_ArchiveHeader)[directoryIndex + entryIndex] - 1;
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

    pArchiveHeaderEntry = g_ArchiveHeader;
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
    return *((unsigned short*)(g_ArchiveHeader + 0x78));
}

int ArchiveGetRelativeEntryIndex(int directoryIndex, int entryIndex) {
    int nOffset;
    nOffset = ((u_short*)g_ArchiveHeader)[directoryIndex + entryIndex];
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
    
    pFileBuffer = (char*) HeapAlloc(nFileSize, 0x0);
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
    u8* pArchiveEntry;
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
    
    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
    );

    exit:
    return nFileSize;
}

// Uses g_ArchiveHeaderOffsetBase as archive offset
int ArchiveDecodeSizeAligned(int entryIndex) {
    char* pFilepath;
    int hArchiveFile;
    int nFileSize;
    u8* pArchiveEntry;
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
    
    nOffset = (entryIndex + g_ArchiveHeaderOffsetBase - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
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
    u8* pArchiveEntry;
    u32 nOffset;
    
    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
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
        return nOffset + g_ArchiveDebugTable;
    }

    return NULL;
}

int ArchiveDecodeSector(int entryIndex) {
    u_char* pArchiveEntry;
    unsigned int nOffset;

    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    return (pArchiveEntry[2] << 0x10) + (pArchiveEntry[1] << 0x8) + pArchiveEntry[0];
}

// Same as ArchiveDecodeSector, but uses g_ArchiveHeaderOffsetBase as current archive offset
int ArchiveDecodeSectorAligned(int entryIndex) {
    u_char* pArchiveEntry;
    unsigned int nOffset;

    nOffset = (entryIndex + g_ArchiveHeaderOffsetBase - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    return (pArchiveEntry[2] << 0x10) + (pArchiveEntry[1] << 0x8) + pArchiveEntry[0];
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
    int nEntries;
    int i;
    void* pStreamData;
    ArchiveStreamFileSectionHeader* pCurSectionHeader;
    ArchiveStreamFileSectionHeader* pSectionHeaderStart;

    pStreamData = g_ArchiveCurStreamFile;
    if (pStreamData == NULL) {
        return -1;
    }
    
    i = 0; 
    nEntries = *(int*)pStreamData;
    pSectionHeaderStart = (ArchiveStreamFileSectionHeader*)(pStreamData + 4);
    
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/libarchive", func_80028B14);

/* Best: 98.25% at retail's exact 844 bytes; the instruction stream, block layout and
 * every branch target now match. The remaining 27 diff rows are REGISTER NAMES only
 * (retail: nSections a1, sections base a3, limit copy a1, queue-index load v1 and its
 * copy a2; ours: nSections a2, sections base a2, limit copy v1, load v0, copy a1), so
 * this is an allocation tie and not a source shape - 7 declaration orders, the
 * `(index * sizeof(T) + (mem_addr)base)` spelling for both pointers and a
 * declaration-initialiser variant all measured the identical 98.25/844.
 *
 * The two levers that got here from 97.87% (836 bytes, two instructions short) are both
 * in the debug-table scan and both are load-bearing:
 *  - the loop bound and the wrap test read g_ArchiveStreamSectionCount DIRECTLY in the
 *    source, not through a `nLimit` local. cse merges the two global loads into one
 *    pseudo, loop.c then hoists it and emits retail's `move a1,v0` preheader copy,
 *    while the zero-trip `blez` keeps using the load register. A `nLimit` local is
 *    coalesced into the load register and costs both the copy and 4 bytes.
 *  - `nIndex = g_ArchiveStreamQueueIndex++;` (not two statements) is what produces
 *    retail's separate queue-index copy.
 * The permuter cannot run this body: its randomization at rng_seed 0 (the forced base
 * candidate) emits invalid C and it reports "Unable to compile base.c".
 *
 * u_char* func_80028B14(void) {
 *     u_char* pData;
 *     ArchiveStreamFileSectionHeader* pHeader;
 *     ArchiveStreamFileSectionHeader* pHeaders;
 *     StreamDataQueueEntry* pEntry;
 *     u_char* pPath;
 *     int nSections;
 *     int nIndex;
 *     int nId;
 *     int i;
 *
 *     if (g_ArchiveCurStreamFile == NULL) {
 *         return NULL;
 *     }
 *     pData = g_ArchiveCurStreamFile;
 *
 *     pHeader = (ArchiveStreamFileSectionHeader*)(pData + 4);
 *     nSections = *(s32*)pData;
 *     pData = pData + nSections * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;
 *
 *     if (g_ArchiveDebugTable != 0) {
 *         if (g_ArchiveDebugFileHandle == -1) {
 *             return NULL;
 *         }
 *
 *         if (g_ArchiveCurFileSize <= 0) {
 *             return NULL;
 *         }
 *
 *         for (i = 0; i < g_ArchiveStreamSectionCount; i++) {
 *             pHeaders = g_ArchiveStreamSections;
 *             nIndex = g_ArchiveStreamQueueIndex++;
 *             pHeader = pHeaders + nIndex;
 *             if (g_ArchiveStreamQueueIndex >= g_ArchiveStreamSectionCount) {
 *                 g_ArchiveStreamQueueIndex = 0;
 *             }
 *             if (pHeader->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
 *                 break;
 *             }
 *         }
 *
 *         if (pHeader->state != ARCHIVE_STREAM_FILE_NOT_LOADED) {
 *             return NULL;
 *         }
 *
 *         pHeader->state = 3;
 *         pData += nIndex * CD_SECTOR_SIZE;
 *
 *         for (i = 0; i < RETRY_COUNT; i++) {
 *             if (PCread(g_ArchiveDebugFileHandle, pData, CD_SECTOR_SIZE) != 0) {
 *                 goto readDone;
 *             }
 *             GfxDrawArchiveReadRetry(i, 0, 0xFF, 0);
 *         }
 *         return NULL;
 *
 *     readDone:
 *
 *         g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *         if (g_ArchiveCurFileSize > 0) {
 *             return pData;
 *         }
 *
 *         g_ArchiveCurFileSize = 0;
 *         for (i = 0; i < RETRY_COUNT; i++) {
 *             if (PCclose(g_ArchiveDebugFileHandle) == 0) {
 *                 break;
 *             }
 *             GfxDrawArchiveReadRetry(i, 0, 0, 0xFF);
 *         }
 *
 *         g_ArchiveDebugFileHandle = -1;
 *         if (g_ArchiveStreamQueue != 0) {
 *             nIndex = g_ArchiveStreamQueueIndex + 1;
 *             pEntry = ((StreamDataQueueEntry*)g_ArchiveStreamQueue) + nIndex;
 *             nId = *(u_short*)pEntry;
 *             g_ArchiveStreamQueueIndex = nIndex;
 *             g_ArchiveCurEntryIndex = nId;
 *
 *             if (nId > 0 && pEntry->pData != NULL) {
 *                 pPath = ArchiveGetFilePath(nId);
 *                 for (i = 0; i < RETRY_COUNT; i++) {
 *                     g_ArchiveDebugFileHandle = PCopen(pPath, 0, 0);
 *                     if (g_ArchiveDebugFileHandle != -1) {
 *                         break;
 *                     }
 *                     GfxDrawArchiveReadRetry(i, 0xFF, 0, 0);
 *                 }
 *                 g_ArchiveCurFileSize = ArchiveDecodeSizeAligned(nId);
 *                 g_ArchivePendingReadCount -= 1;
 *                 return pData;
 *             }
 *
 *             g_ArchiveCurFileSize = 0;
 *         }
 *
 *         g_ArchivePendingReadCount = 0;
 *         return pData;
 *     }
 *
 *     for (i = 0; i < nSections; i++, pHeader++) {
 *         if (pHeader->state == 3 && pHeader->id == g_ArchiveStreamReadSectionId) {
 *             break;
 *         }
 *     }
 *
 *     if (i == g_ArchiveStreamSectionCount) {
 *         return NULL;
 *     }
 *
 *     g_ArchiveStreamReadSectionId += 1;
 *     return pData + i * CD_SECTOR_SIZE;
 * }
 */

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
                
                continue;
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/libarchive", func_80028F30);

/* Best: 39.90% (1280 bytes against retail's 1324). First written body, ~90% of the
 * instruction multiset in place; what is left is STRUCTURE, not registers:
 *  - retail's frame is 0x50, ours 0x48: two stack locals too few, i.e. at least one
 *    source local is missing (retail's s5 holds the stream pointer and is then reused
 *    for pData - `addu s5,v0,zero` then `addiu s5,v0,0x24` - so it is ONE variable
 *    reassigned, which is done here, yet a slot is still short).
 *  - retail keeps i in s1 and pSection in s2 concurrently with pHeaders in s3 through
 *    the whole debug block; ours reuses them.
 * Known-good facts about the shape, all read off the .s and already encoded below:
 *  - `if (g_ArchiveDebugTable == 0) goto fail;` INVERTED, with the pData reassignment
 *    landing in the beqz delay slot; the debug body then falls into a shared
 *    `fail: *pSectionOut = 0;` whose label is also the target of every early exit.
 *  - The debug path and the fail path both continue into the section SEARCH loop
 *    (retail L80029378), and the loop's bound is nSections (the stream-file header
 *    count), while the success test compares i against g_ArchiveStreamSectionCount -
 *    two different counters, exactly as in func_80028B14's non-debug scan.
 *  - The `seek` block is shared by three call sites (scratch[0] == 1 twice, and
 *    `pSection->size < D_80059F5C`), the last of which passes -0x28 or -0x20 depending
 *    on g_ArchiveCdMode & 8; retail then stores 0 through pSectionOut and jumps PAST
 *    the shared fail store.
 *  - D_80059F54 / D_80059F58 / D_80059F5C (s16) / D_8005A4B8 are the debug stream's
 *    per-sector state and have no symbols yet; they are splat-owned data labels, so a
 *    struct view has to be declared here rather than defined.
 *
 * s32 func_80028F30(s32* pDataOut, s32* pSectionOut) {
 *     ArchiveStreamFileSectionHeader* pSection;
 *     ArchiveStreamFileSectionHeader* pHeaders;
 *     u8* pData;
 *     u8* pSector;
 *     s32 nSections;
 *     s32 nOffset;
 *     s32 nIndex;
 *     s32 nSeek;
 *     s32 nRemaining;
 *     s32 i;
 *
 *     if (g_ArchiveCurStreamFile == NULL) {
 *         return 1;
 *     }
 *
 *     pData = g_ArchiveCurStreamFile;
 *     pHeaders = (ArchiveStreamFileSectionHeader*)(pData + 4);
 *     nSections = *(s32*)pData;
 *     pData = pData + nSections * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;
 *
 *     if (g_ArchiveDebugTable == 0) {
 *         goto fail;
 *     }
 *     {
 *         if (g_ArchiveDebugFileHandle == -1) {
 *             goto fail;
 *         }
 *         if (g_ArchiveCurFileSize <= 0) {
 *             goto fail;
 *         }
 *
 *         if (g_ArchiveDebugStreamSectionCount != 0) {
 *             if (g_ArchiveCdMode[0] & 8) {
 *                 PCread(g_ArchiveDebugFileHandle, (char*)g_ArchiveSectorScratch, 8);
 *                 if (g_ArchiveSectorScratch[0] == 1) {
 *                     nSeek = 0x918;
 *                     goto seek;
 *                 }
 *             }
 *
 *             nIndex = g_ArchiveStreamQueueIndex + 1;
 *             pSection = (ArchiveStreamFileSectionHeader*)(nIndex * sizeof(ArchiveStreamFileSectionHeader) + (mem_addr)g_ArchiveStreamSections);
 *             g_ArchiveStreamQueueIndex = nIndex;
 *             pSection->state = ARCHIVE_STREAM_FILE_READY;
 *             pSection->id = g_ArchiveStreamNextSectionId;
 *             g_ArchiveStreamNextSectionId = g_ArchiveStreamNextSectionId + 1;
 *             PCread(g_ArchiveDebugFileHandle, (char*)(D_80059F54 + g_ArchiveDebugStreamSectionCount * ARCHIVE_DEBUG_BLOCK_HEADER_SIZE), CD_SECTOR_SIZE - ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
 *             PCread(g_ArchiveDebugFileHandle, (char*)(D_80059F58 + g_ArchiveDebugStreamSectionCount * (CD_SECTOR_SIZE - ARCHIVE_DEBUG_BLOCK_HEADER_SIZE)), CD_SECTOR_SIZE - ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
 *             if (g_ArchiveCdMode[0] & 8) {
 *                 PClseek(g_ArchiveDebugFileHandle, 0x118, 1);
 *             }
 *             g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *             g_ArchiveDebugStreamSectionCount = g_ArchiveDebugStreamSectionCount + 1;
 *             if (g_ArchiveDebugStreamSectionCount >= D_80059F5C) {
 *                 g_ArchiveDebugStreamSectionCount = 0;
 *             }
 *             *pSectionOut = 0;
 *             goto search;
 *         }
 *
 *         i = 0;
 *         while (i < g_ArchiveStreamSectionCount) {
 *             pSection = (ArchiveStreamFileSectionHeader*)(i * sizeof(ArchiveStreamFileSectionHeader) + (mem_addr)g_ArchiveStreamSections);
 *             if (pSection->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
 *                 break;
 *             }
 *             i += pSection->size;
 *         }
 *         if (i >= g_ArchiveStreamSectionCount) {
 *             goto fail;
 *         }
 *
 *         nOffset = i * CD_SECTOR_SIZE;
 *         if (g_ArchiveCdMode[0] & 8) {
 *             PCread(g_ArchiveDebugFileHandle, (char*)g_ArchiveSectorScratch, 8);
 *             if (g_ArchiveSectorScratch[0] == 1) {
 *                 nSeek = 0x918;
 *                 goto seek;
 *             }
 *         }
 *
 *         pSector = (u8*)(nOffset + (mem_addr)g_ArchiveStreamWritePointer);
 *         D_80059F54 = (s32)pSector;
 *         PCread(g_ArchiveDebugFileHandle, (char*)pSector, ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
 *         D_80059F5C = *(u16*)(pSector + 6);
 *         D_8005A4B8 = *(u16*)(pSector + 8);
 *
 *         if (pSection->size < D_80059F5C) {
 *             if (g_ArchiveCdMode[0] & 8) {
 *                 nSeek = -0x28;
 *             } else {
 *                 nSeek = -0x20;
 *             }
 *             goto seek;
 *         }
 *
 *         pSection->state = ARCHIVE_STREAM_FILE_READY;
 *         pSection->id = g_ArchiveStreamNextSectionId;
 *         nRemaining = pSection->size - D_80059F5C;
 *         if (nRemaining >= 3) {
 *             pSection->size = D_80059F5C + 1;
 *             pSection[D_80059F5C + 1].size = nRemaining - 1;
 *             pSection[D_80059F5C + 1].state = ARCHIVE_STREAM_FILE_NOT_LOADED;
 *             ArchiveConsolidateStreamFileEntry(D_80059F5C + 1);
 *         }
 *
 *         g_ArchiveStreamNextSectionId = g_ArchiveStreamNextSectionId + 1;
 *         D_80059F58 = nOffset + (mem_addr)g_ArchiveStreamWritePointer + D_80059F5C * ARCHIVE_DEBUG_BLOCK_HEADER_SIZE;
 *         PCread(g_ArchiveDebugFileHandle, (char*)D_80059F58, CD_SECTOR_SIZE - ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
 *         if (g_ArchiveCdMode[0] & 8) {
 *             PClseek(g_ArchiveDebugFileHandle, 0x118, 1);
 *         }
 *         g_ArchiveStreamQueueIndex = i;
 *         g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *         g_ArchiveDebugStreamSectionCount = g_ArchiveDebugStreamSectionCount + 1;
 *         *pSectionOut = 0;
 *         goto search;
 *
 *     seek:
 *         PClseek(g_ArchiveDebugFileHandle, nSeek, 1);
 *         *pSectionOut = 0;
 *         goto search;
 *     }
 *
 * fail:
 *     *pSectionOut = 0;
 *
 * search:
 *     for (i = 0; i < nSections; i++, pHeaders++) {
 *         if (pHeaders->state == ARCHIVE_STREAM_FILE_READY && pHeaders->id == g_ArchiveStreamReadSectionId) {
 *             break;
 *         }
 *     }
 *
 *     if (i == g_ArchiveStreamSectionCount) {
 *         return 1;
 *     }
 *
 *     pSection = (ArchiveStreamFileSectionHeader*)(pData + i * CD_SECTOR_SIZE);
 *     *pSectionOut = (s32)pSection;
 *     *pDataOut = (s32)((u8*)pSection + pSection->unk2 * ARCHIVE_DEBUG_BLOCK_HEADER_SIZE);
 *     if (ArchiveCheckStreamFileSectionStates(i, pSection->unk2, ARCHIVE_STREAM_FILE_READY) != 0) {
 *         return 1;
 *     }
 *
 *     g_ArchiveStreamReadSectionId += pSection->unk2;
 *     return 0;
 * }
 */

u_short ArchiveReleaseStreamBlock(u_char* pSection) {
    u_char* pStream;
    u_char* pStreamEnd;
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

    pSectionHeader = (ArchiveStreamFileSectionHeader*)(pStream + 4);
    pStreamEnd = pStream + *(s32*)pStream * 8 + 0x24;
    nIndex = (u32)(pSection - pStreamEnd) >> 11;
    nState = pSectionHeader[nIndex].state;
    pSectionHeader[nIndex].state = 0;
    return nState;
}

u_short ArchiveReleaseStreamSection(u_char* pSection) {
    u_char* pStream;
    u_char* pStreamEnd;
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

    nCount = *(u16*)(pSection + 6);
    pSectionHeader = (ArchiveStreamFileSectionHeader*)(pStream + 4);
    pStreamEnd = pStream + *(s32*)pStream * 8 + 0x24;
    nIndex = (u32)(pSection - pStreamEnd) >> 11;
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

s32 ArchiveReadFileToBuffer(s32 index, s32 pBuffer, u32 arg2, u32 flags) {
    if ((index <= 0) || (ArchiveDecodeSize(index) <= 0) || (pBuffer == NULL)) {
        return -3;
    }
    ArchiveCdDataSync(0);
    g_ArchiveHeaderOffsetBase = g_CurArchiveOffset;
    g_ArchiveCurFileSector = ArchiveDecodeSector(index);
    g_ArchiveCurFileSize = ArchiveDecodeAlignedSize(index);
    return ArchiveReadFile(index, pBuffer, arg2, flags);
}
